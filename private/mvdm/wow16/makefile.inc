#
# Common MAKEFILE definitions for WOW16
#

.SUFFIXES:
.SUFFIXES: .c   .asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib .rc .res

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
DEST=$(ALT_PROJECT)\obj\i386
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
DEST=$(ALT_PROJECT)\obj\alpha
!endif

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

!IF 0
!IF "$(QFE_BUILD)" != "1"
CL16=cl16
!ELSE
CL16=cl
!ENDIF
!ENDIF

ASM     = masm386
LINK    = link16 /batch
LINK16  = link16 /batch
CL      = cl16
RC      = rc16
MAPSYM  = mapsym
IMPLIB  = implib
MKPUB   = mkpublic
CVPACK  = cvpack

!ifdef INCLUDE
INCS    =
!else
!IFDEF LVL_2
INCS    = -I..\..\inc -I..\..\..\inc
RINCS   = -I..\..\inc
ENVINCS = ..\..\inc;..\..\..\inc
!ELSE
INCS    = -I..\inc -I..\..\inc
RINCS   = -I..\inc
ENVINCS = ..\inc;..\..\inc
!ENDIF
!endif


DEFINES = -DWOW $(MVDMFLAGS)


AOBJ    = -Ml -t $(DEFINES) $(INCS)
CW16    = -AS -G2sw -Os -W3 -Zp $(DEFINES)
CW16B   = $(CW16) -B1 c1l.exe -B2 c2l.exe -B3 c3l.exe
LINKOPTS = /map /align:16

!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="retail" && "$(NTDEBUG)" != "ntsdnodbg"
AOBJ    = $(AOBJ) -Zd
CW16    = $(CW16) /Od /Oi /Zd
LINKOPTS = $(LINKOPTS) /LI
!endif


.asm{$(DEST)}.obj:
    $(ASM) $(AOBJ) $(*F),$*;

.asm{$(DEST)}.lst:
    $(ASM) $(AOBJ) -l $(*F),nul,$*.lst;


.c{$(DEST)}.obj:
    @set include=$(ENVINCS);$(INCLUDE)
    $(CL) -c -nologo $(CW16) -Fo$* $(*F).c

.c{$(DEST)}.lst:
    @set include=$(ENVINCS);$(INCLUDE)
    $(CL) -c -nologo $(CW16) -Fonul -Fc$*.lst $(*F).c


.def{$(DEST)}.lib:
    implib $*.lib $(*F).def

{$(DEST)}.map{$(DEST)}.sym:
    mapsym -o $*.sym $*

.rc{$(DEST)}.res:
    $(RC) -r -fo$*.res $(INCS)  $(*F).rc $*

