'''
PROJECT:     ReactOS baseaddress updater
LICENSE:     MIT (https://spdx.org/licenses/MIT)
PURPOSE:     Update baseaddresses of all modules
COPYRIGHT:   Copyright 2017,2018 Mark Jansen (mark.jansen@reactos.org)
'''

# FIXME: user32 always at 0x77a20000

import os
import struct
import sys
try:
    import pefile
except ImportError:
    print '# Please install pefile from pip or https://github.com/erocarrera/pefile'
    print '# Using fallback'
    print

DLL_EXTENSIONS = (
    '.dll'
)

OTHER_EXTENSIONS = (
    '.acm', '.ax', '.cpl', '.drv', '.ocx'
)

PRIORITIES = (
    'ntdll.dll',
    'kernel32.dll',
    'msvcrt.dll',
    'advapi32.dll',
    'gdi32.dll',
    'user32.dll',
    'dhcpcsvc.dll',
    'dnsapi.dll',
    'icmp.dll',
    'iphlpapi.dll',
    'ws2_32.dll',
    'ws2help.dll',
    'shlwapi.dll',
    'rpcrt4.dll',
    'comctl32.dll',
    'ole32.dll',
    'winspool.drv',
    'winmm.dll',
    'comdlg32.dll',
    'shell32.dll',
    'lz32.dll',
    'version.dll',
    'oleaut32.dll',
    'setupapi.dll',
    'mpr.dll',
    'crypt32.dll',
    'wininet.dll',
    'urlmon.dll',
    'psapi.dll',
    'imm32.dll',
    'msvfw32.dll',
    'dbghelp.dll',
    'devmgr.dll',
    'msacm32.dll',
    'netapi32.dll',
    'powrprof.dll',
    'secur32.dll',
    'wintrust.dll',
    'avicap32.dll',
    'cabinet.dll',
    'dsound.dll',
    'glu32.dll',
    'opengl32.dll',
    'riched20.dll',
    'smdll.dll',
    'userenv.dll',
    'uxtheme.dll',
    'cryptui.dll',
    'csrsrv.dll',
    'basesrv.dll',
    'winsrv.dll',
    'dplayx.dll',
    'gdiplus.dll',
    'msimg32.dll',
    'mswsock.dll',
    'oledlg.dll',
    'rasapi32.dll',
    'rsaenh.dll',
    'samlib.dll',
    'sensapi.dll',
    'sfc_os.dll',
    'snmpapi.dll',
    'spoolss.dll',
    'usp10.dll',
)

EXCLUDE = (
    'bmfd.dll',
    'bootvid.dll',
    'freeldr_pe.dll',
    'ftfd.dll',
    'fusion.dll',
    'hal.dll',
    'halaacpi.dll',
    'halacpi.dll',
    'halapic.dll',
    'kbda1.dll',
    'kbda2.dll',
    'kbda3.dll',
    'kbdal.dll',
    'kbdarme.dll',
    'kbdarmw.dll',
    'kbdaze.dll',
    'kbdazel.dll',
    'kbdbe.dll',
    'kbdbga.dll',
    'kbdbgm.dll',
    'kbdbgt.dll',
    'kbdblr.dll',
    'kbdbr.dll',
    'kbdbur.dll',
    'kbdcan.dll',
    'kbdcr.dll',
    'kbdcz.dll',
    'kbdcz1.dll',
    'kbdda.dll',
    'kbddv.dll',
    'kbdes.dll',
    'kbdest.dll',
    'kbdfc.dll',
    'kbdfi.dll',
    'kbdfr.dll',
    'kbdgeo.dll',
    'kbdgerg.dll',
    'kbdgneo.dll',
    'kbdgr.dll',
    'kbdgrist.dll',
    'kbdhe.dll',
    'kbdheb.dll',
    'kbdhu.dll',
    'kbdic.dll',
    'kbdinasa.dll',
    'kbdinben.dll',
    'kbdindev.dll',
    'kbdinguj.dll',
    'kbdinmal.dll',
    'kbdir.dll',
    'kbdit.dll',
    'kbdja.dll',
    'kbdkaz.dll',
    'kbdko.dll',
    'kbdla.dll',
    'kbdlt1.dll',
    'kbdlv.dll',
    'kbdmac.dll',
    'kbdne.dll',
    'kbdno.dll',
    'kbdpl.dll',
    'kbdpl1.dll',
    'kbdpo.dll',
    'kbdro.dll',
    'kbdru.dll',
    'kbdru1.dll',
    'kbdsg.dll',
    'kbdsk.dll',
    'kbdsk1.dll',
    'kbdsw.dll',
    'kbdtat.dll',
    'kbdth0.dll',
    'kbdth1.dll',
    'kbdth2.dll',
    'kbdth3.dll',
    'kbdtuf.dll',
    'kbdtuq.dll',
    'kbduk.dll',
    'kbdur.dll',
    'kbdurs.dll',
    'kbdus.dll',
    'kbdusa.dll',
    'kbdusl.dll',
    'kbdusr.dll',
    'kbdusx.dll',
    'kbduzb.dll',
    'kbdvntc.dll',
    'kbdycc.dll',
    'kbdycl.dll',
    'kdcom.dll',
    'kdvbox.dll',
    'setupldr_pe.dll',
    'vgaddi.dll',
    'dllexport_test_dll1.dll',
    'dllexport_test_dll2.dll',
    'dllimport_test.dll',
    'MyEventProvider.dll',
    'w32kdll_2k3sp2.dll',
    'w32kdll_ros.dll',
    'w32kdll_xpsp2.dll',
)


def size_of_image_fallback(filename):
    with open(filename, 'rb') as fin:
        if fin.read(2) != 'MZ':
            print filename, 'No dos header found!'
            return 0
        fin.seek(0x3C)
        e_lfanew = struct.unpack('i', fin.read(4))[0]
        fin.seek(e_lfanew)
        if fin.read(4) != 'PE\0\0':
            print filename, 'No PE header found!'
            return 0
        fin.seek(e_lfanew + 0x18)
        pe_magic = struct.unpack('h', fin.read(2))[0]
        if pe_magic != 0x10b:
            print filename, 'is not a 32 bit exe!'
            return 0
        fin.seek(e_lfanew + 0x50)
        pe_size_of_image = struct.unpack('i', fin.read(4))[0]
        return pe_size_of_image

def size_of_image(filename):
    if 'pefile' in globals():
        return pefile.PE(filename, fast_load=True).OPTIONAL_HEADER.SizeOfImage
    return size_of_image_fallback(filename)

def onefile(current_address, filename, size):
    name, ext = os.path.splitext(filename)
    postfix = ''
    if ext in('.acm', '.drv') and filename != 'winspool.drv':
        name = filename
    if current_address == 0:
        current_address = 0x7c920000
        postfix = ' # should be above 0x%08x' % current_address
    else:
        current_address = (current_address - size - 0x2000 - 0xffff) & 0xffff0000
    print 'set(baseaddress_%-30s 0x%08x)%s' % (name, current_address, postfix)
    return current_address

def run_dir(target):
    print '# Generated from', target
    print '# Generated by sdk/tools/gen_baseaddress.py'
    found_dlls = {}
    found_files = {}
    for root, _, files in os.walk(target):
        for dll in [filename for filename in files if filename.endswith(DLL_EXTENSIONS)]:
            if not dll in EXCLUDE and not dll.startswith('api-ms-win-'):
                found_dlls[dll] = size_of_image(os.path.join(root, dll))
        extrafiles = [filename for filename in files if filename.endswith(OTHER_EXTENSIONS)]
        for extrafile in extrafiles:
            if not extrafile in EXCLUDE:
                found_files[extrafile] = size_of_image(os.path.join(root, extrafile))

    current_address = 0
    for curr in PRIORITIES:
        if curr in found_dlls:
            current_address = onefile(current_address, curr, found_dlls[curr])
            del found_dlls[curr]
        elif curr in found_files:
            current_address = onefile(current_address, curr, found_files[curr])
            del found_files[curr]
        else:
            print '# Did not find', curr, '!'

    print '# Extra dlls'
    for curr in sorted(found_dlls):
        current_address = onefile(current_address, curr, found_dlls[curr])
    print '# Extra files'
    for curr in sorted(found_files):
        current_address = onefile(current_address, curr, found_files[curr])

def main(dirs):
    if len(dirs) < 1:
        trydir = os.getcwd()
        print '# No path specified, trying', trydir
        dirs = [trydir]
    for onedir in dirs:
        run_dir(onedir)

if __name__ == '__main__':
    main(sys.argv[1:])
