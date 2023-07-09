all: ..\precomp.h ..\msg00001.bin

..\precomp.h: ..\usersrv.h ..\globals.h ..\strid.h  ..\..\inc\user.h ..\..\inc\csrmsg.h \
              $(BASEDIR)\private\inc\winuserp.h  ..\..\..\w32inc\winuserk.h

..\strid.h ..\res.rc ..\msg00001.bin: ..\strid.mc
    mc -v -U ..\strid.mc -r ..\ -h ..\
