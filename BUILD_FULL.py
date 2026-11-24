"""Build both MultiSplit components with Clang/LLD, using an upstream PPSSPP DLL or a MultiSplit 1.0 release DLL/ZIP."""
from pathlib import Path
import hashlib, struct, subprocess, sys, zipfile
root = Path(__file__).resolve().parent
if len(sys.argv) != 2:
    raise SystemExit('Usage: python BUILD_FULL.py <upstream_ppsspp.dll | MultiSplit_release.dll | MultiSplit_release.zip>')
p = Path(sys.argv[1])
if p.suffix.lower() == '.zip':
    with zipfile.ZipFile(p) as z:
        candidates = [x for x in z.namelist() if x.endswith('/ppsspp_multisplit_libretro.dll')]
        if len(candidates) != 1:
            raise ValueError('Expected one MultiSplit release DLL in the archive')
        core = z.read(candidates[0])
else:
    core = p.read_bytes()
if core[-32:-16] == b'PSPVKDUOPAYLOAD1':
    e_len, core_len = struct.unpack('<QQ', core[-16:])
    if not core_len or core_len + e_len + 32 >= len(core):
        raise ValueError('Invalid embedded payload lengths')
    core = core[-32-core_len:-32]
if core[:2] != b'MZ':
    raise ValueError('Expected a Windows DLL')
print('Engine SHA256:', hashlib.sha256(core).hexdigest())
def run(*args):
    subprocess.run(args, cwd=root, check=True)
for src in ('worker', 'bridge'):
    run('clang', '--target=x86_64-pc-windows-msvc', '-std=c11', '-O2', '-fno-stack-protector', '-fno-builtin', '-c', src+'.c', '-o', src+'.obj')
for lib in ('kernel32', 'msvcrt'):
    run('lld-link', '/lib', '/def:'+lib+'.def', '/machine:x64', '/out:'+lib+'.lib')
run('lld-link', '/subsystem:console', '/entry:worker_entry', '/nodefaultlib', '/machine:x64', 'worker.obj', 'kernel32.lib', 'msvcrt.lib', '/out:psp_worker.exe')
run('lld-link', '/dll', '/noentry', '/nodefaultlib', '/machine:x64', 'bridge.obj', 'kernel32.lib', '/out:bridge.dll')
e = (root/'psp_worker.exe').read_bytes()
b = (root/'bridge.dll').read_bytes()
out = root/'ppsspp_multisplit_libretro.dll'
out.write_bytes(b+e+core+b'PSPVKDUOPAYLOAD1'+struct.pack('<QQ',len(e),len(core)))
print(out, out.stat().st_size, hashlib.sha256(out.read_bytes()).hexdigest())
