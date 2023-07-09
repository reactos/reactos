callcf.c: ..\callcf.tpl ..\..\inc\$(ALT_PROJECT)\scf.lst
    listmung ..\..\inc\$(ALT_PROJECT)\scf.lst ..\callcf.tpl >callcf.c

..\ssend.c: ..\..\inc\ntcb.h ..\..\inc\$(ALT_PROJECT)\cb.lst ..\..\inc\messages.h

..\ntstubs.c: ..\..\inc\messages.h

..\precomp.h: ..\userk.h ..\globals.h ..\..\inc\user.h ..\..\inc\ntuser.h $(BASEDIR)\private\inc\winuserp.h \
        ..\..\..\w32inc\winuserk.h ..\..\..\w32inc\w32p.h ..\access.h strid.h

strid.h msg00001.bin strid.rc: ..\strid.mc
    mc -v -U ..\strid.mc

kbdus.c: ..\..\kbd\txt\kbdus.txt #\nt\private\sdktools\kbdtool\obj\i386\kbdtool.exe
        kbdtool -k ..\..\kbd\txt\kbdus.txt

