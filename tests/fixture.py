"""Compile production callback bodies with explicit, deterministic host doubles."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def function(source, name):
    match = re.search(r'^(?:HookAction|void|bool|BOOL|int|float|double|f32|s16) ' + re.escape(name) + r'\(', source, re.M)
    if not match:
        raise ValueError(f"Production function not found: {name}")
    start = match.start()
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'

def run(code):
    with tempfile.TemporaryDirectory(prefix='tp-qa-') as directory:
        cpp = Path(directory) / 'test.cpp'
        binary = Path(directory) / 'test'
        cpp.write_text(code)
        subprocess.run(['c++', '-std=c++20', '-O0', '-UNDEBUG', '-I' + str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
