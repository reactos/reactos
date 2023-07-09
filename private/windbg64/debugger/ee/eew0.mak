#	Makefile for C++ expression evaluator for Windows
#
#	The following arguments are passed in from the master makefile
#
#		CLL 	compile command
#		MLL 	masm command
#		ODIR	object directory
#       DLL     target name
#       LFLAGS  linker flags

#	Inference rules

.SUFFIXES: .dll .obj .c .asm

.c{$(ODIR)}.obj:
    $(CLL) @<<
$(COPT) -Fo$*.obj -Yudebexpr.h -Fp$(ODIR)\precomp.pch
$<
<<

.asm{$(ODIR)}.obj:
		$(MLL) $<,$*.obj;

CEXPROBJS = \
		$(ODIR)\precomp.obj $(ODIR)\debapi.obj	$(ODIR)\debeval.obj   \
		$(ODIR)\deblex.obj	$(ODIR)\deblexer.obj	$(ODIR)\debparse.obj	\
		$(ODIR)\debsym.obj	$(ODIR)\debtree.obj	$(ODIR)\debtyper.obj	\
		$(ODIR)\debfmt.obj	$(ODIR)\deberr.obj	$(ODIR)\debbind.obj \
		$(ODIR)\debutil.obj	$(ODIR)\debwalk.obj	$(ODIR)\debsrch.obj \
		$(ODIR)\fixups.obj	$(ODIR)\debsup.obj	$(ODIR)\libentry.obj \
		$(ODIR)\libmain.obj

all:	$(CEXPROBJS) $(ODIR)\llibcv.lib $(ODIR)\$(DLL).res
	link  @<<$(ODIR)\$(DLL).lnk, <<$(ODIR)\$(DLL).def
$(ODIR)\precomp.obj $(ODIR)\debapi.obj	$(ODIR)\debeval.obj   +
$(ODIR)\deblex.obj	$(ODIR)\deblexer.obj	$(ODIR)\debparse.obj	+
$(ODIR)\debsym.obj	$(ODIR)\debtree.obj	$(ODIR)\debtyper.obj	+
$(ODIR)\debsrch.obj	$(ODIR)\debfmt.obj	$(ODIR)\deberr.obj	+
$(ODIR)\debbind.obj	$(ODIR)\debutil.obj	$(ODIR)\debwalk.obj	+
$(ODIR)\fixups.obj	$(ODIR)\debsup.obj	$(ODIR)\libentry.obj	+
$(ODIR)\libmain.obj
$(DLL).dll $(LFLAGS)
$(ODIR)\$(DLL)
libw $(ODIR)\llibcv oldnames /m/far/noe/nod/a:16/NOE
<<KEEP
LIBRARY $(DLL) INITGLOBAL
EXETYPE WINDOWS
CODE MOVEABLE DISCARDABLE
DATA MOVEABLE SINGLE
HEAPSIZE 0

EXPORTS
    DBGVERSIONCHECK    @1
    EEINITIALIZEEXPR   @2
    WEP                @3 RESIDENTNAME

<<KEEP
	rc.exe -3 $(ODIR)\$(DLL).res $(DLL).dll
	mapsym $(ODIR)\$(DLL).map


$(ODIR)\$(DLL).res:
    rc.exe -I. -r <<$(ODIR)\$(DLL).rc
#include "windows.h"
#include "verstamp.h"
#define VER_FILETYPE			    VFT_DLL
#define VER_FILESUBTYPE 			VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR 	"Microsoft\256 C/C++ Expression Evaluator"
#define VER_INTERNALNAME_STR		"$(DLL)"
#define VER_LEGALCOPYRIGHT_YEARS	"1993"
#define VER_ORIGINALFILENAME_STR	"$(DLL).DLL"
#include "common.ver"
<<keep


#************************************************************************
#
#	C expression evaluator
#
#************************************************************************


$(ODIR)\precomp.obj:    \
	$(CVINC)\types.h    \
    $(CVINC)\cvtypes.h  \
    $(CVINC)\cvinfo.h   \
    $(CVINC)\shapi.h    \
    $(CVINC)\eeapi.h    \
    debdef.h            \
    shfunc.h            \
    debexpr.h           \
    errors.h            \
    debops.h
        $(CLL) @<<
$(COPT) -Fo$(ODIR)\precomp.obj -Ycdebexpr.h -Fp$(ODIR)\precomp.pch precomp.c
<<

$(ODIR)\debeval.obj:	$(ODIR)\precomp.obj version.h

$(ODIR)\deblexer.obj:	debops.inc

$(ODIR)\debapi.obj: 	$(ODIR)\precomp.obj version.h

$(ODIR)\debbind.obj:	$(ODIR)\precomp.obj debsym.h

$(ODIR)\deberr.obj: 	$(ODIR)\precomp.obj

$(ODIR)\debfmt.obj: 	$(ODIR)\precomp.obj fmtstr.h

$(ODIR)\deblex.obj: 	$(ODIR)\precomp.obj

$(ODIR)\debparse.obj:	$(ODIR)\precomp.obj

$(ODIR)\debsym.obj: 	$(ODIR)\precomp.obj debsym.h

$(ODIR)\debsup.obj: 	$(ODIR)\precomp.obj

$(ODIR)\debtree.obj:	$(ODIR)\precomp.obj

$(ODIR)\debtyper.obj:	$(ODIR)\precomp.obj

$(ODIR)\debutil.obj:	$(ODIR)\precomp.obj

$(ODIR)\debwalk.obj:	$(ODIR)\precomp.obj

$(ODIR)\debsrch.obj:	$(ODIR)\precomp.obj

$(ODIR)\fixups.obj: 	fixups.asm

$(ODIR)\libentry.obj:	libentry.asm

$(ODIR)\libmain.obj:	libmain.c
		$(CLL) @<<
$(COPT) -Fo$(ODIR)\libmain.obj libmain.c
<<

$(ODIR)\llibcv.lib: ldllcew.lib
		lib ldllcew.lib -fltusedc.obj -fixups.obj,,$(ODIR)\llibcv.lib;
