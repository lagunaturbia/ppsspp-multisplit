"""Rebuild the Windows x64 MultiSplit libretro bridge with Clang / lld-link.

  python BUILD.py /path/to/PPSSPP_MultiSplit_1.0_Windows_x64.zip

The release ZIP supplies the original unmodified PPSSPP library and the
user-tested 0.2 helper. The bridge is rebuilt from the source in this archive.
The original GPL PPSSPP engine source is at:
https://github.com/hrydgard/ppsspp/tree/cdf563b8
"""
from pathlib import Path
import zipfile, struct, subprocess, hashlib, sys
root=Path(__file__).resolve().parent
if len(sys.argv)<2:
    raise SystemExit('Usage: python BUILD.py /path/to/PPSSPP_MultiSplit_1.0_Windows_x64.zip')
source=Path(sys.argv[1])
if source.suffix.lower()=='.zip':
    with zipfile.ZipFile(source) as z:
        matched=[x for x in z.namelist() if x.endswith('/ppsspp_multisplit_libretro.dll')]
        if not matched:raise ValueError('No MultiSplit DLL in ZIP')
        old=z.read(matched[0])
else:
    old=source.read_bytes()
assert old[-32:-16]==b'PSPVKDUOPAYLOAD1'
exe_len,core_len=struct.unpack('<QQ',old[-16:])
helper=old[-32-exe_len-core_len:-32-core_len]
core=old[-32-core_len:-32]
assert hashlib.sha256(core).hexdigest()=='c792d088a9ceb4db6ed6b6d1002a94efe0bc52fcfdcb356f9945083d20ca9e0f'
assert hashlib.sha256(helper).hexdigest()=='4e1ae8643d9626640b8b9b8f5b8d9bb3c9deee6f9a5ae885a311844348c3bfc5'

def run(*args):subprocess.run(args,cwd=root,check=True)
run('clang','--target=x86_64-pc-windows-msvc','-std=c11','-O2','-fno-stack-protector','-fno-builtin','-c','bridge.c','-o','bridge.obj')
run('lld-link','/lib','/def:kernel32.def','/machine:x64','/out:kernel32.lib')
run('lld-link','/dll','/noentry','/nodefaultlib','/machine:x64','bridge.obj','kernel32.lib','/out:bridge.dll')
b=bytearray((root/'bridge.dll').read_bytes())
peoff=struct.unpack_from('<I',b,0x3c)[0]
assert b[peoff:peoff+4]==b'PE\0\0'
struct.pack_into('<I',b,peoff+8,0)  # Normalize linker timestamp for reproducibility.
out=root/'ppsspp_multisplit_libretro.dll'
out.write_bytes(b+helper+core+b'PSPVKDUOPAYLOAD1'+struct.pack('<QQ',len(helper),len(core)))
print(out, out.stat().st_size, hashlib.sha256(out.read_bytes()).hexdigest())
