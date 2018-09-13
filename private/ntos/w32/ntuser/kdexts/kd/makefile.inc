HDEPENDENCIES=..\..\exts.h $(BASEDIR)\private\inc\stdexts.h

GENUEDEFDEFINES=/DKERNEL /D_$(ALT_PROJECT)_
!if !$(FREEBUILD)
GENUEDEFDEFINES=$(GENUEDEFDEFINES) /DDEBUGTAGS=1
!endif

userkdx.def: ..\..\genuedef.c $(HDEPENDENCIES)
        $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP ..\..\genuedef.c >userkdx.def

#
# Dependencies for dummy targets
#
..\alpha.c:             ..\..\alpha.c
..\i386.c:              ..\..\i386.c
..\kdexts.c:            ..\..\kdexts.c
..\mips.c:              ..\..\mips.c
..\ppc.c:               ..\..\ppc.c
..\userexts.c:          ..\..\userkdx.h \
                        ..\..\userexts.c \
                        $(HDEPENDENCIES) \
                        $(BASEDIR)\private\inc\stdexts.c
..\userexts.rc:         ..\..\userexts.rc

..\..\precomp.h: ..\..\userkdx.h
..\..\userkdx.h: ..\..\..\kernel\userk.h \
                 ..\..\..\inc\user.h \
                 ..\..\..\inc\ntuser.h \
                 $(BASEDIR)\private\inc\winuserp.h \
                 $(BASEDIR)\private\inc\immp.h \
                 ..\..\..\..\w32inc\winuserk.h

..\$(TARGET_DIRECTORY)\dsotable.c: ..\..\userkdx.h ..\kd.txt ..\..\dsocode.c
    $(C_PREPROCESSOR) ..\..\userkdx.h > userkdx.pph
    structo -i h -p -s ..\kd.txt -o $@ userkdx.pph
    del userkdx.pph
