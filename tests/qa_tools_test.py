"""Exercise QA failure paths: stale reports, wrong version, missing assets and duplicates."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('qa', ROOT / 'tools/qa.py')
qa = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qa)

class GateTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.root = Path(self.folder.name)
        (self.root / 'CMakeLists.txt').write_text('project(tp_remaster VERSION 0.6.0 LANGUAGES CXX)')
        (self.root / 'src').mkdir()
        (self.root / 'src/feature.cpp').write_text('original')
        (self.root / 'res').mkdir()
        (self.root / 'res/button.bti').write_bytes(b'asset')
        (self.root / 'res/.gitkeep').touch()
        self.package = self.root / 'build/mods/test.dusk'
        self.package.parent.mkdir(parents=True)
        self.host = self.root / 'build/host'
        self.host.write_bytes(b'pinned executable')
        self.report = self.root / 'build/report.json'
        self.bundle()

    def tearDown(self):
        self.folder.cleanup()

    def bundle(self, version='0.6.0', asset=b'asset', include_asset=True):
        with zipfile.ZipFile(self.package, 'w') as archive:
            archive.writestr('mod.json', json.dumps({'id': 'dev.yaloalo.tp_remaster', 'version': version}))
            archive.writestr('lib/linux-x86_64/mod.so', b'library')
            if include_asset:
                archive.writestr('res/button.bti', asset)

    def record(self, status='verified-candidate'):
        self.report.write_text(json.dumps({'status': status, 'source_sha256': qa.source_digest(self.root),
            'package_sha256': qa.digest(self.package), 'host': {'path': str(self.host), 'sha256': qa.digest(self.host)}}))

    def test_correct_bundle_and_report(self):
        self.assertEqual(qa.audit_package(self.package, self.root)[0]['version'], '0.6.0')
        self.record()
        self.assertEqual(qa.verify_report(self.report, self.package, self.root)['status'], 'verified-candidate')

    def test_wrong_version_missing_or_stale_resources(self):
        for args in ({'version': '0.5.0'}, {'include_asset': False}, {'asset': b'old'}):
            with self.subTest(args=args):
                self.bundle(**args)
                with self.assertRaises(ValueError): qa.audit_package(self.package, self.root)

    def test_unsafe_and_duplicate_entries(self):
        for name in ('../outside', '/absolute', 'res/button.bti'):
            with self.subTest(name=name):
                self.bundle()
                with zipfile.ZipFile(self.package, 'a') as archive:
                    archive.writestr(name, b'bad')
                with self.assertRaises(ValueError): qa.audit_package(self.package, self.root)

    def test_partial_failed_and_running_reports_rejected(self):
        for status in ('failed', 'running', 'tests-and-package-passed'):
            self.record(status)
            with self.assertRaises(ValueError): qa.verify_report(self.report, self.package, self.root)

    def test_source_package_and_host_changes_rejected(self):
        self.record()
        (self.root / 'src/feature.cpp').write_text('changed')
        with self.assertRaises(ValueError): qa.verify_report(self.report, self.package, self.root)
        self.record()
        self.bundle(asset=b'changed')
        with self.assertRaises(ValueError): qa.verify_report(self.report, self.package, self.root)
        self.record()
        self.host.write_bytes(b'new release')
        with self.assertRaises(ValueError): qa.verify_report(self.report, self.package, self.root)

    def test_reward_data_changes_invalidate_source(self):
        self.record()
        (self.root / 'data').mkdir()
        (self.root / 'data/reward_catalog.json').write_text('{"changed": true}')
        with self.assertRaises(ValueError): qa.verify_report(self.report, self.package, self.root)

    def test_build_outputs_do_not_invalidate_source(self):
        before = qa.source_digest(self.root)
        (self.root / 'build/report.log').write_text('new output')
        self.assertEqual(before, qa.source_digest(self.root))

    def candidate(self):
        build = self.root / 'build'
        self.package.rename(build / 'mods/tp_remaster.dusk')
        self.package = build / 'mods/tp_remaster.dusk'
        (build / 'qa').mkdir()
        self.report = build / 'qa/report.json'
        self.record()
        data = json.loads(self.report.read_text())
        data.update(version='0.6.0', finished_utc='2026-10-15T12:00:00Z', checks=[
            {'name': name, 'status': 'passed'} for name in
            ('regression suites', 'bundle and compiled library',
             'exact-host imports and hooks', 'source unchanged during QA')])
        self.report.write_text(json.dumps(data))
        return build, qa.archive_candidate(build)

    def test_launch_survives_edits_and_failed_build_without_using_failed_bytes(self):
        build, frozen = self.candidate()
        original = frozen.read_bytes()
        (self.root / 'src/feature.cpp').write_text('work in progress')
        self.assertEqual(qa.launch_candidate(build), frozen)
        self.package.write_bytes(b'unverified build')
        self.record('failed')
        self.assertEqual(qa.launch_candidate(build), frozen)
        self.assertEqual(frozen.read_bytes(), original)
        # An incomplete report claiming success cannot be promoted either.
        self.record('verified-candidate')
        self.assertEqual(qa.launch_candidate(build), frozen)

    def test_corrupt_archive_and_changed_host_cannot_launch(self):
        build, frozen = self.candidate()
        self.record('running')
        original = frozen.read_bytes()
        frozen.write_bytes(b'corrupt')
        with self.assertRaises(ValueError): qa.launch_candidate(build)
        frozen.write_bytes(original)
        self.host.write_bytes(b'other host')
        with self.assertRaises(ValueError): qa.launch_candidate(build)

if __name__ == '__main__':
    unittest.main()
