#!/bin/sh
#
# UINX installation script
#
# To use: change ROS_INSTALL to where you keep your reactos files
#

ROS_INSTALL=~/ros/iso
ROS_INSTALL_TESTS=$ROS_INSTALL/test

mkdir $ROS_INSTALL
mkdir $ROS_INSTALL/bin
mkdir $ROS_INSTALL_TESTS
mkdir $ROS_INSTALL/symbols
mkdir $ROS_INSTALL/system32
mkdir $ROS_INSTALL/system32/config
mkdir $ROS_INSTALL/system32/drivers
mkdir $ROS_INSTALL/media
mkdir $ROS_INSTALL/media/fonts

cp boot.bat $ROS_INSTALL
cp bootc.lst $ROS_INSTALL
cp aboot.bat $ROS_INSTALL
cp loaders/dos/loadros.com $ROS_INSTALL
cp ntoskrnl/ntoskrnl.exe $ROS_INSTALL/system32
cp ntoskrnl/ntoskrnl.sym $ROS_INSTALL/symbols
cp hal/halx86/hal.dll $ROS_INSTALL/system32
cp drivers/fs/vfat/vfatfs.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/cdfs/cdfs.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/fs_rec/fs_rec.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/ms/msfs.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/np/npfs.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/ntfs/ntfs.sys $ROS_INSTALL/system32/drivers
cp drivers/fs/mup/mup.sys $ROS_INSTALL/system32/drivers
cp drivers/bus/acpi/acpi.sys $ROS_INSTALL/system32/drivers
cp drivers/bus/isapnp/isapnp.sys $ROS_INSTALL/system32/drivers
cp drivers/bus/pci/pci.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/floppy/floppy.sys $ROS_INSTALL/system32/drivers
cp drivers/lib/bzip2/unbzip2.sys $ROS_INSTALL/system32/drivers
cp drivers/input/keyboard/keyboard.sys $ROS_INSTALL/system32/drivers
cp drivers/input/mouclass/mouclass.sys $ROS_INSTALL/system32/drivers
cp drivers/input/psaux/psaux.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/blue/blue.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/beep/beep.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/debugout/debugout.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/null/null.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/serial/serial.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/serenum/serenum.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/vga/miniport/vgamp.sys $ROS_INSTALL/system32/drivers
cp drivers/dd/vga/display/vgaddi.dll $ROS_INSTALL/system32
cp drivers/dd/videoprt/videoprt.sys $ROS_INSTALL/system32/drivers
cp drivers/net/afd/afd.sys $ROS_INSTALL/system32/drivers
cp drivers/net/dd/ne2000/ne2000.sys $ROS_INSTALL/system32/drivers
cp drivers/net/dd/pcnet/pcnet.sys $ROS_INSTALL/system32/drivers
cp drivers/net/dd/miniport/nscirda/nscirda.sys $ROS_INSTALL/system32/drivers
cp drivers/net/ndis/ndis.sys $ROS_INSTALL/system32/drivers
cp drivers/net/packet/packet.sys $ROS_INSTALL/system32/drivers
cp drivers/net/tdi/tdi.sys $ROS_INSTALL/system32/drivers
cp drivers/net/tcpip/tcpip.sys $ROS_INSTALL/system32/drivers
cp drivers/net/wshtcpip/wshtcpip.dll $ROS_INSTALL/system32
cp drivers/storage/atapi/atapi.sys $ROS_INSTALL/system32/drivers
cp drivers/storage/scsiport/scsiport.sys $ROS_INSTALL/system32/drivers
cp drivers/storage/cdrom/cdrom.sys $ROS_INSTALL/system32/drivers
cp drivers/storage/disk/disk.sys $ROS_INSTALL/system32/drivers
cp drivers/storage/class2/class2.sys $ROS_INSTALL/system32/drivers
cp subsys/system/autochk/autochk.exe $ROS_INSTALL/system32
cp subsys/system/cmd/cmd.exe $ROS_INSTALL/system32
cp subsys/system/services/services.exe $ROS_INSTALL/system32
cp subsys/system/setup/setup.exe $ROS_INSTALL/system32
cp subsys/system/winlogon/winlogon.exe $ROS_INSTALL/system32
cp services/eventlog/eventlog.exe $ROS_INSTALL/system32
cp services/rpcss/rpcss.exe $ROS_INSTALL/system32
cp lib/advapi32/advapi32.dll $ROS_INSTALL/system32
cp lib/crtdll/crtdll.dll $ROS_INSTALL/system32
cp lib/fmifs/fmifs.dll $ROS_INSTALL/system32
cp lib/freetype/freetype.dll $ROS_INSTALL/system32
cp lib/gdi32/gdi32.dll $ROS_INSTALL/system32
cp lib/iphlpapi/iphlpapi.dll $ROS_INSTALL/system32
cp lib/kernel32/kernel32.dll $ROS_INSTALL/system32
cp lib/libpcap/libpcap.dll $ROS_INSTALL/system32
cp lib/msafd/msafd.dll $ROS_INSTALL/system32
cp lib/msvcrt/msvcrt.dll $ROS_INSTALL/system32
cp lib/ntdll/ntdll.dll $ROS_INSTALL/system32
cp lib/packet/packet.dll $ROS_INSTALL/system32
cp lib/secur32/secur32.dll $ROS_INSTALL/system32
cp lib/shell32/roshel32.dll $ROS_INSTALL/system32
cp lib/snmpapi/snmpapi.dll $ROS_INSTALL/system32
cp lib/syssetup/syssetup.dll $ROS_INSTALL/system32
cp lib/user32/user32.dll $ROS_INSTALL/system32
cp lib/version/version.dll $ROS_INSTALL/system32
cp lib/winedbgc/winedbgc.dll $ROS_INSTALL/system32
cp lib/winmm/winmm.dll $ROS_INSTALL/system32
cp lib/winspool/winspool.drv $ROS_INSTALL/system32
cp lib/ws2_32/ws2_32.dll $ROS_INSTALL/system32
cp lib/ws2help/ws2help.dll $ROS_INSTALL/system32
cp lib/wshirda/wshirda.dll $ROS_INSTALL/system32
cp lib/wsock32/wsock32.dll $ROS_INSTALL/system32
cp subsys/smss/smss.exe $ROS_INSTALL/system32
cp subsys/csrss/csrss.exe $ROS_INSTALL/system32
cp subsys/ntvdm/ntvdm.exe $ROS_INSTALL/system32
cp subsys/win32k/win32k.sys $ROS_INSTALL/system32
cp subsys/system/usetup/usetup.exe $ROS_INSTALL/system32
cp apps/utils/cat/cat.exe $ROS_INSTALL/bin
cp apps/utils/partinfo/partinfo.exe $ROS_INSTALL/bin
cp apps/utils/objdir/objdir.exe $ROS_INSTALL/bin
cp apps/utils/pice/module/pice.sys $ROS_INSTALL/system32/drivers
cp apps/utils/pice/module/pice.sym $ROS_INSTALL/symbols
cp apps/utils/pice/pice.cfg $ROS_INSTALL/symbols
cp apps/utils/sc/sc.exe $ROS_INSTALL/bin
cp apps/tests/hello/hello.exe $ROS_INSTALL/bin
cp apps/tests/args/args.exe $ROS_INSTALL/bin
cp apps/tests/apc/apc.exe $ROS_INSTALL/bin
cp apps/tests/shm/shmsrv.exe $ROS_INSTALL/bin
cp apps/tests/shm/shmclt.exe $ROS_INSTALL/bin
cp apps/tests/lpc/lpcsrv.exe $ROS_INSTALL/bin
cp apps/tests/lpc/lpcclt.exe $ROS_INSTALL/bin
cp apps/tests/thread/thread.exe $ROS_INSTALL/bin
cp apps/tests/event/event.exe $ROS_INSTALL/bin
cp apps/tests/file/file.exe $ROS_INSTALL/bin
cp apps/tests/pteb/pteb.exe $ROS_INSTALL/bin
cp apps/tests/consume/consume.exe $ROS_INSTALL/bin
cp apps/tests/vmtest/vmtest.exe $ROS_INSTALL_TESTS
cp apps/tests/gditest/gditest.exe $ROS_INSTALL_TESTS
cp apps/tests/shaptest/shaptest.exe $ROS_INSTALL_TESTS
cp apps/tests/dibtest/dibtest.exe $ROS_INSTALL_TESTS
cp apps/tests/mstest/msserver.exe $ROS_INSTALL_TESTS
cp apps/tests/mstest/msclient.exe $ROS_INSTALL_TESTS
cp apps/tests/nptest/npserver.exe $ROS_INSTALL_TESTS
cp apps/tests/nptest/npclient.exe $ROS_INSTALL_TESTS
cp apps/tests/atomtest/atomtest.exe $ROS_INSTALL_TESTS
cp apps/tests/mutex/mutex.exe $ROS_INSTALL/bin
cp apps/tests/winhello/winhello.exe $ROS_INSTALL/bin
cp apps/tests/eventpair/eventpair.exe $ROS_INSTALL_TESTS
cp apps/tests/threadwait/threadwait.exe $ROS_INSTALL_TESTS
cp apps/tests/multiwin/multiwin.exe $ROS_INSTALL/bin
cp apps/tests/wm_paint/wm_paint.exe $ROS_INSTALL_TESTS
cp apps/tests/bitblt/lena.bmp $ROS_INSTALL_TESTS
cp apps/tests/bitblt/bitblt.exe $ROS_INSTALL_TESTS
cp apps/tests/sectest/sectest.exe $ROS_INSTALL_TESTS
cp apps/tests/isotest/isotest.exe $ROS_INSTALL_TESTS
cp apps/tests/regtest/regtest.exe $ROS_INSTALL_TESTS
cp apps/tests/hivetest/hivetest.exe $ROS_INSTALL_TESTS
cp apps/tests/restest/restest.exe $ROS_INSTALL_TESTS
cp apps/tests/tokentest/tokentest.exe $ROS_INSTALL_TESTS
cp apps/tests/icontest/icontest.exe $ROS_INSTALL_TESTS
cp apps/tests/icontest/icon.ico $ROS_INSTALL_TESTS
cp apps/testsets/msvcrt/fileio/fileio.exe $ROS_INSTALL_TESTS
cp apps/testsets/msvcrt/mbtowc/mbtowc.exe $ROS_INSTALL_TESTS
cp apps/testsets/test/test.exe $ROS_INSTALL_TESTS
cp apps/testsets/testperl/testperl.exe $ROS_INSTALL_TESTS
cp media/fonts/*.ttf $ROS_INSTALL/media/fonts
cp media/nls/c_1252.nls $ROS_INSTALL/system32/ansi.nls
cp media/nls/c_437.nls $ROS_INSTALL/system32/oem.nls
cp media/nls/l_intl.nls $ROS_INSTALL/system32/casemap.nls
cp ntoskrnl/ntoskrnl.map $ROS_INSTALL/symbols

tools/mkhive/mkhive bootdata $ROS_INSTALL/system32/config

echo Done.
