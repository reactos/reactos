#	Makefile for C++/C-ANSI expression evaluators for DOS
#
#	The following arguments are passed in from the master makefile
#
#		CLL 	compile command
#		MLL 	masm command
#		ODIR	object directory
#       DLL     target name
#       LFLAGS  linker flags
#		FIXUPS.ASM - FWait support that does a	word nop ( mov ax,ax ) instead of int 3?


#	Inference rules

.SUFFIXES: .dll .obj .c .asm

.c{$(ODIR)}.obj:
        $(CLL) @<<
$(COPT) -Fo$*.obj -Yudebexpr.h -Fp$(ODIR)\precomp.pch
$<
<<

.asm{$(ODIR)}.obj:
		$(MLL) -DDOS $<,$*.obj;

CEXPROBJS = \
		$(ODIR)\precomp.obj     \
		$(ODIR)\dosdll.obj	    \
		$(ODIR)\debapi.obj	    \
        $(ODIR)\debeval.obj	    \
		$(ODIR)\deblex.obj	    \
		$(ODIR)\deblexer.obj	\
        $(ODIR)\debparse.obj	\
		$(ODIR)\debsym.obj	    \
        $(ODIR)\debtree.obj	    \
		$(ODIR)\debtyper.obj    \
        $(ODIR)\debfmt.obj	    \
		$(ODIR)\deberr.obj	    \
        $(ODIR)\debbind.obj	    \
		$(ODIR)\debutil.obj	    \
        $(ODIR)\debwalk.obj	    \
		$(ODIR)\debsrch.obj	    \
        $(ODIR)\fixups.obj	    \
		$(ODIR)\debsup.obj	    \
        $(ODIR)\fmemset.obj	    \
		$(ODIR)\fmemcpy.obj	    \
        $(ODIR)\fstrnicm.obj	\
		$(ODIR)\fstrcat.obj	    \
        $(ODIR)\fstrchr.obj	    \
		$(ODIR)\fstrncat.obj	\
        $(ODIR)\fstrncmp.obj	\
		$(ODIR)\fstrcpy.obj	    \
        $(ODIR)\fstrlen.obj	    \
		$(ODIR)\fstrncpy.obj	\
        $(ODIR)\ctype.obj	    \
		$(ODIR)\diffhlp.obj	    \
        $(ODIR)\aflmul.obj	    \
		$(ODIR)\affalshl.obj	\
        $(ODIR)\affalshr.obj	\
		$(ODIR)\afuldiv.obj	    \
        $(ODIR)\afldiv.obj	    \
		$(ODIR)\aflrem.obj	    \
        $(ODIR)\afulrem.obj	    \
		$(ODIR)\affalmul.obj	\
        $(ODIR)\emftol.obj	    \
		$(ODIR)\aflshl.obj	    \
        $(ODIR)\aflshr.obj	    \
		$(ODIR)\affaulsh.obj	\
        $(ODIR)\afulshr.obj     \
		$(ODIR)\fmemmove.obj    \
        $(ODIR)\afhdiff.obj     \
		$(ODIR)\affauldi.obj    \
        $(ODIR)\fmemcmp.obj     \
		$(ODIR)\emfcmp.obj	    \
        $(ODIR)\empty.obj


all:	$(CEXPROBJS)
	link $(LFLAGS) @<<$(ODIR)\$(DLL).lrf
 $(ODIR)\dosdll.obj 	+
 $(ODIR)\precomp.obj	 +
 $(ODIR)\debapi.obj 	+
 $(ODIR)\debtyper.obj   +
 $(ODIR)\debutil.obj 	+
 $(ODIR)\debsym.obj 	+
 $(ODIR)\debeval.obj 	+
 $(ODIR)\deblex.obj  	+
 $(ODIR)\deblexer.obj 	+
 $(ODIR)\debparse.obj 	+
 $(ODIR)\debbind.obj 	+
 $(ODIR)\debsrch.obj 	+
 $(ODIR)\debfmt.obj  	+
 $(ODIR)\debsup.obj 	+
 $(ODIR)\debtree.obj 	+
 $(ODIR)\debwalk.obj 	+
 $(ODIR)\deberr.obj 	+
 $(ODIR)\fmemset.obj  $(ODIR)\fmemcpy.obj  $(ODIR)\fstrnicm.obj +
 $(ODIR)\fstrcat.obj  $(ODIR)\fstrchr.obj  $(ODIR)\fstrncat.obj +
 $(ODIR)\fstrncmp.obj $(ODIR)\fstrcpy.obj  $(ODIR)\fstrlen.obj	+
 $(ODIR)\fstrncpy.obj $(ODIR)\ctype.obj    $(ODIR)\diffhlp.obj	+
 $(ODIR)\aflmul.obj   $(ODIR)\affalshl.obj $(ODIR)\affalshr.obj +
 $(ODIR)\afuldiv.obj  $(ODIR)\afldiv.obj   $(ODIR)\aflrem.obj	+
 $(ODIR)\afulrem.obj  $(ODIR)\affalmul.obj $(ODIR)\emftol.obj	+		      +
 $(ODIR)\aflshl.obj   $(ODIR)\aflshr.obj   $(ODIR)\affaulsh.obj +
 $(ODIR)\afulshr.obj  $(ODIR)\affauldi.obj $(ODIR)\fmemcmp.obj  +
 $(ODIR)\fmemmove.obj $(ODIR)\emfcmp.obj +
 $(ODIR)\fixups.obj +
 $(ODIR)\afhdiff.obj
 $(DLL).dll
$(ODIR)\$(DLL).map
/dosseg/seg:256/m/nod/noe/far;
<<KEEP
        mapsym /n $(ODIR)\$(DLL)


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

$(ODIR)\empty.obj:		empty.c
		$(CLL) $(COPT) -Fo$(ODIR)\empty.obj empty.c

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

$(ODIR)\dosdll.obj:	ovlhdr.inc

$(ODIR)\AFLSHL.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFFALSHL.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
				del $(@F)

$(ODIR)\AFLMUL.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\EMFCMP.obj :	 llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FMEMSET.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRNICM.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\CTYPE.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\DIFFHLP.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFFALSHR.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFLDIV.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFULDIV.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FMEMCPY.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FMEMCMP.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRCAT.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRCHR.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRNCAT.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFFALMUL.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFLREM.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFULREM.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRNCMP.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRLEN.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRCPY.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\EMFTOL.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\FSTRNCPY.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFLSHR.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFFAULSH.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFFAULDI.obj :    llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFULSH.obj :      llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\AFULSHR.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\fmemmove.obj :	   llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)

$(ODIR)\afhdiff.obj :	  llibcer.lib
                lib $** *$(@F);
		copy $(@F) $(ODIR)
                del $(@F)
