#	Makefile for C++ expression evaluator for OS/2
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
$(COPT) -Fo$*.obj -Yudebexpr.h -Fp$(ODIR)\precomp.pch $<
<<

.asm{$(ODIR)}.obj:
		$(MLL) $<,$*.obj;

CEXPROBJS = \
		$(ODIR)\precomp.obj $(ODIR)\debapi.obj	 $(ODIR)\debeval.obj  \
		$(ODIR)\deblex.obj	$(ODIR)\deblexer.obj $(ODIR)\debparse.obj	 \
		$(ODIR)\debsym.obj	$(ODIR)\debtree.obj $(ODIR)\debtyper.obj	\
		$(ODIR)\debfmt.obj	$(ODIR)\deberr.obj	$(ODIR)\debbind.obj \
		$(ODIR)\debutil.obj $(ODIR)\debwalk.obj $(ODIR)\debsrch.obj \
		$(ODIR)\fixups.obj	$(ODIR)\debsup.obj

all:	$(CEXPROBJS) $(ODIR)\llibcv.lib
	link  @<<$(ODIR)\$(DLL).lnk, <<$(ODIR)\$(DLL).def
$(ODIR)\precomp.obj $(ODIR)\debapi.obj	$(ODIR)\debeval.obj   +
$(ODIR)\deblex.obj	$(ODIR)\deblexer.obj	$(ODIR)\debparse.obj	+
$(ODIR)\debsym.obj	$(ODIR)\debtree.obj $(ODIR)\debtyper.obj	+
$(ODIR)\debsrch.obj $(ODIR)\debfmt.obj	$(ODIR)\deberr.obj	+
$(ODIR)\debbind.obj $(ODIR)\debutil.obj $(ODIR)\debwalk.obj +
$(ODIR)\fixups.obj	$(ODIR)\debsup.obj
$(DLL).dll $(LFLAGS)
$(ODIR)\$(DLL)
os2 $(ODIR)\llibcv /m/far/e/nod/a:16
<<KEEP
LIBRARY $(DLL) initinstance
PROTMODE
DATA multiple nonshared

EXPORTS
    DBGVERSIONCHECK
	EEINITIALIZEEXPR

<<KEEP

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

$(ODIR)\llibcv.lib: llibcep.lib
		lib llibcep.lib -fltusedc.obj -fixups.obj,,$(ODIR)\llibcv.lib;
