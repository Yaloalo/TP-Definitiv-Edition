#!/usr/bin/env python3
"""Build gate: production regression fixtures, bundle audit and optional exact-host ABI.

CMake invokes this after packaging on every native Unix build. This script never
builds recursively. A verified candidate is still awaiting in-game playtests.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def source_digest(root=ROOT):
    files = []
    for directory in ('src', 'res', 'tests', 'tools', 'docs', 'data', 'cmake', '.github'):
        files.extend(p for p in (root / directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
    files.extend(p for p in root.iterdir() if p.is_file() and p.suffix != '.pyc')
    value = hashlib.sha256()
    for path in sorted(files):
        value.update(path.relative_to(root).as_posix().encode() + b'\0')
        value.update(bytes.fromhex(digest(path)))
    return value.hexdigest()

def audit_package(package, root=ROOT):
    version = re.search(r'project\(tp_remaster VERSION ([\d.]+)', (root / 'CMakeLists.txt').read_text()).group(1)
    with zipfile.ZipFile(package) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError('Duplicate bundle entries')
        for name in names:
            path = PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or '\\' in name:
                raise ValueError(f'Unsafe bundle entry: {name}')
        if archive.testzip() is not None:
            raise ValueError('Corrupt bundle entry')
        metadata = json.loads(archive.read('mod.json'))
        if metadata.get('id') != 'dev.yaloalo.tp_remaster' or metadata.get('version') != version:
            raise ValueError('Bundle ID/version does not match this source')
        libraries = [name for name in names if name.startswith('lib/') and name.endswith('/mod.so')]
        if len(libraries) != 1 or not archive.read(libraries[0]):
            raise ValueError('Expected one nonempty native Unix mod library')
        # Match pinned SDK CopyModAssets.cmake: dotfiles/directories are omitted.
        expected = {p.relative_to(root).as_posix(): p for p in (root / 'res').rglob('*')
                    if p.is_file() and not any(part.startswith('.') for part in p.relative_to(root / 'res').parts)}
        present = {n for n in names if n.startswith('res/') and not n.endswith('/')}
        if present != set(expected):
            raise ValueError('Bundle resources do not match source resources')
        for name, path in expected.items():
            if hashlib.sha256(archive.read(name)).hexdigest() != digest(path):
                raise ValueError(f'Stale resource: {name}')
        return metadata, libraries[0]

def verify_report(report, package, root=ROOT):
    data = json.loads(Path(report).read_text())
    if data.get('status') != 'verified-candidate':
        raise ValueError('This build has not passed the complete automated QA gate')
    if data.get('source_sha256') != source_digest(root) or data.get('package_sha256') != digest(package):
        raise ValueError('QA report is stale: source or package changed; rebuild first')
    host = data.get('host', {})
    if not host.get('path') or digest(host['path']) != host.get('sha256'):
        raise ValueError('The tested host changed or is unavailable; rerun QA against the installed release')
    return data

def run_gate(options):
    build = options.build.resolve()
    output = build / 'qa'
    output.mkdir(parents=True, exist_ok=True)
    report_path = output / 'report.json'
    package = build / 'mods/tp_remaster.dusk'
    start_source = source_digest()
    report = {'started_utc': datetime.now(timezone.utc).isoformat(), 'status': 'running',
              'source_sha256': start_source, 'checks': [], 'in_game_validation': 'pending',
              'story_routes': json.loads((ROOT / 'tests/story_routes.json').read_text())}

    def save():
        temporary = report_path.with_suffix('.tmp')
        temporary.write_text(json.dumps(report, indent=2) + '\n')
        temporary.replace(report_path)

    def check(name, action):
        try:
            details = action()
            report['checks'].append({'name': name, 'status': 'passed', 'details': details})
            print(f'QA PASS: {name}', flush=True)
        except Exception as error:
            report['checks'].append({'name': name, 'status': 'failed', 'error': str(error)})
            print(f'QA FAIL: {name}: {error}', file=sys.stderr, flush=True)
        save()

    def command(name, arguments):
        path = output / (name + '.log')
        with path.open('w') as stream:
            result = subprocess.run([str(a) for a in arguments], cwd=ROOT, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=240)
        if result.returncode:
            raise RuntimeError(f'Exit {result.returncode}; see {path}\n' + path.read_text()[-5000:])
        return path.read_text().strip()

    save()  # Invalidate any previous success before starting a new gate.
    def tests():
        junit = output / 'ctest.xml'
        junit.unlink(missing_ok=True)
        args = ['ctest', '--test-dir', build, '--output-on-failure', '--parallel', '2',
                '--timeout', '120', '--output-junit', junit]
        if options.config:
            args += ['-C', options.config]
        command('tests', args)
        cases = ET.parse(junit).getroot().findall('.//testcase')
        if not cases or any(c.find('failure') is not None or c.find('skipped') is not None for c in cases):
            raise ValueError('No tests ran, or a test failed/was skipped')
        required = set(json.loads((ROOT / 'tests/required_suites.json').read_text()))
        names = {c.attrib['name'] for c in cases}
        if missing := required - names:
            raise ValueError('Required suites missing: ' + ', '.join(sorted(missing)))
        return {'count': len(cases), 'suites': sorted(names), 'junit': str(junit)}

    check('regression suites', tests)
    with tempfile.TemporaryDirectory(prefix='tp-qa-bundle-') as temporary:
        library = Path(temporary) / 'mod.so'
        def bundle():
            metadata, entry = audit_package(package)
            with zipfile.ZipFile(package) as archive:
                library.write_bytes(archive.read(entry))
            # Compare the packaged library against a freshly stripped build,
            # rather than trusting an old staging directory or an old .dusk.
            stripped = Path(temporary) / 'built.so'
            stripped.write_bytes(options.mod.read_bytes())
            command('strip', [options.strip, '--strip-unneeded', stripped])
            if digest(library) != digest(stripped):
                raise ValueError('Packaged library differs from the compiled mod')
            source_files = [p for p in (ROOT / 'src').rglob('*') if p.is_file()]
            if any(p.stat().st_mtime_ns > options.mod.stat().st_mtime_ns for p in source_files):
                raise ValueError('Gameplay source is newer than compiled library; run the default build target')
            report['package_sha256'] = digest(package)
            report['version'] = metadata['version']
            return {'path': str(package), 'library_sha256': digest(library)}
        check('bundle and compiled library', bundle)
        if options.host and library.is_file():
            def host():
                path = options.host.resolve()
                report['host'] = {'path': str(path), 'sha256': digest(path), 'sdk': 'v2.0.3'}
                imports = command('imports', ['bash', ROOT / 'tools/check_symbols.sh', library, path])
                hooks = command('hooks', [sys.executable, ROOT / 'tools/check_hook_symbols.py', library, path,
                    '--dynamic-source', ROOT / 'src/input/mouse_controls.cpp',
                    '--dynamic-source', ROOT / 'src/input/combat_controller.cpp',
                    '--dynamic-source', ROOT / 'src/core/tuning.cpp'])
                return imports + '\n' + hooks
            check('exact-host imports and hooks', host)
        elif options.host:
            report['checks'].append({'name': 'exact-host imports and hooks', 'status': 'not-run',
                                     'reason': 'Bundle audit failed; no verified packaged library to inspect'})
        elif options.require_host:
            def missing_host():
                raise ValueError('Set DUSKLIGHT_BINARY to the extracted v2.0.3 executable')
            check('exact-host imports and hooks', missing_host)
        else:
            report['checks'].append({'name': 'exact-host imports and hooks', 'status': 'not-run',
                                     'reason': 'No host supplied; this is not a verified candidate'})
    def unchanged():
        if source_digest() != start_source:
            raise ValueError('Source changed while testing; rerun the build')
        return True
    check('source unchanged during QA', unchanged)
    failed = any(c['status'] == 'failed' for c in report['checks'])
    report['status'] = 'failed' if failed else ('verified-candidate' if options.host else 'tests-and-package-passed')
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    save()
    print(f'QA: {report["status"]}; report: {report_path}', flush=True)
    return 1 if failed else 0

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT / 'build')
    parser.add_argument('--mod', type=Path)
    parser.add_argument('--strip', default='strip')
    parser.add_argument('--host', type=Path, default=os.environ.get('DUSKLIGHT_BINARY') or None)
    parser.add_argument('--require-host', action='store_true')
    parser.add_argument('--config', default='')
    parser.add_argument('--verify', action='store_true', help='Reject stale/unverified packages before launching')
    options = parser.parse_args()
    if options.verify:
        try:
            verify_report(options.build / 'qa/report.json', options.build / 'mods/tp_remaster.dusk')
        except (ValueError, OSError, KeyError) as error:
            print(f'QA launch gate: {error}', file=sys.stderr)
            return 1
        return 0
    if options.mod is None:
        options.mod = options.build / 'tp_remaster.so'
    return run_gate(options)

if __name__ == '__main__':
    sys.exit(main())
