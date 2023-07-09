#	Makefile for C/C++ expression evaluator for NT
#
#	The following arguments are passed in from the master makefile
#
#		CLL 	compile command
#		MLL 	masm command
#		EELANG	language:
#		    	  can = ANSI C
#		    	  cxx = C++
#		OS  	operating system:
#		    	  n0  = i386
#		    	  n1  = MIPS
#		    	  n2  = ALPHA
#		    	  n3  = PPC
#	                  m0  = macintosh 68k
#	                  m1  = macintosh ppc
#
#	In addition, you can, if you choose, set DIR_CEXPR in your
#	environment to be the canonical path to the source files (including
#	a trailing backslash); this will cause files to be compiled with
#	a full canonical path to the source files, which makes it easier
#	for WinDbg or other debuggers to find them.


#---------------------------------------------------------------------
#	Macros
#---------------------------------------------------------------------

!if ("$(REL)" == "no") || ("$(DBGINFO)" != "") || ("$(CODEVIEW)" != "")
USE_CODEVIEW = 1
!endif

LFLAGS =

!if "$(REL)" == "yes"
D=
! if "$(LEGOBLD)" != ""
LFLAGS	 = -debug -debugtype:cv,fixup
! endif
RCDEF	 = -DRETAIL
!else
D=d
RCDEF	 =
!endif

DLL 	=ee$(OS)$(EELANG)$D
ODIR	=o$(OS)$(EELANG)$D

!if "$(USE_CODEVIEW)" != ""
! ifdef TARGETNB09
LFLAGS	=-debug -debugtype:cv -pdb:none
! else
LFLAGS	=-debug -debugtype:cv -pdb:$(DLL).pdb
! endif
!endif

# NOTE: The below inclusion of M0 and M1 will work because we currently only
# cross target the mac from x86. This will need to change if we ever target
# the mac from a host other than x86.
!if "$(OS)" == "n0" || \
    "$(OS)" == "m0" || \
    "$(OS)" == "m1"
CPUDEF	=   -Di386 -D_X86_
!elseif "$(OS)" == "n1"
CPUDEF	=   -D_MIPS_ -DMIPS -DTARGET_MIPS
!elseif "$(OS)" == "n2"
CPUDEF	=   -D_ALPHA_ -DALPHA -DTARGET_ALPHA
!elseif "$(OS)" == "n3"
CPUDEF	=   -D_PPC_ -DPPC -DTARGET_PPC
!else
!error OS must be n0 for i386, n1 for MIPS, n2 for ALPHA, or n3 for PowerPC
!endif

LIBS = msvcrt$D.lib kernel32.lib user32.lib $(PROFLIBS)
LEXER = deb

CEXPROBJS = \
    $(ODIR)\debapi.obj \
    $(ODIR)\debbind.obj \
    $(ODIR)\deberr.obj \
    $(ODIR)\debeval.obj \
    $(ODIR)\debfmt.obj \
    $(ODIR)\deblex.obj \
    $(ODIR)\deblexr.obj \
    $(ODIR)\debparse.obj \
    $(ODIR)\debsrch.obj \
    $(ODIR)\debsup.obj \
    $(ODIR)\debsym.obj \
    $(ODIR)\debtree.obj \
    $(ODIR)\debtyper.obj \
    $(ODIR)\debutil.obj \
    $(ODIR)\debwalk.obj \
    $(ODIR)\dllmain.obj \
    $(ODIR)\ldouble.obj \
    $(ODIR)\$(DLL).res

#---------------------------------------------------------------------
#	Inference rules
#---------------------------------------------------------------------

.SUFFIXES: .dll .obj .c .asm

.c{$(ODIR)}.obj:
    @$(CLL) @<<$(ODIR)\cl.rsp $(DIR_CEXPR)$<
$(COPT) -MD$D
$(CPUDEF)
-Fo$(ODIR)\
-Fd$(ODIR)\msvc.pdb
<<KEEP

#---------------------------------------------------------------------
#	Targets
#---------------------------------------------------------------------

all: $(ODIR) $(DLL).dll

$(ODIR):
    @-mkdir $(ODIR)

$(ODIR)\$(DLL).res:
    rc $(RCDEF) $(CPUDEF) -I. -I$(LANGAPI)\include -r -fo$(ODIR)\$(DLL).tmp <<$(ODIR)\$(DLL).rc
#include "appver.h"
#define VER_INTERNALNAME_STR		"$(DLL).dll"
#define VER_FILEDESCRIPTION_STR 	"Microsoft\256 C/C++ Expression Evaluator"
#define VER_ORIGINALFILENAME_STR	"$(DLL).dll"
#include "version.rc"
#include "debmsg.rc"
<<keep
    @cvtres -$(CPU) -o $(ODIR)\$(DLL).res $(ODIR)\$(DLL).tmp

$(DLL).dll: $(CEXPROBJS) eent.mak
    link -def:<<$(ODIR)\$(DLL).def @<<$(ODIR)\$(DLL).lnk
LIBRARY $(DLL)

EXPORTS
    DBGVersionCheck
    EEInitializeExpr
<<KEEP
$(LFLAGS)
-dll
-implib:$(ODIR)\$(DLL).lib
-out:$(DLL).dll
-map:$(DLL).map
-base:@dllbase.txt,ee$(OS)$(EELANG)
$(CEXPROBJS: = ^
)
$(LIBS)
<<KEEP

#---------------------------------------------------------------------
#       Dependencies
#---------------------------------------------------------------------

HEADERS = $(LANGAPI)\debugger\types.h \
          $(LANGAPI)\debugger\cvtypes.h \
          $(LANGAPI)\include\cvinfo.h \
          $(LANGAPI)\debugger\shapi.h \
          $(LANGAPI)\debugger\eeapi.h \
          debdef.h \
          shfunc.h \
          debexpr.h \
	  resource.h \
          debops.h

$(LEXER):	debops.inc
$(ODIR)\debapi.obj: 	$(HEADERS) $(LANGAPI)\include\version.h
$(ODIR)\debbind.obj:	$(HEADERS) debsym.h
$(ODIR)\deberr.obj:	$(HEADERS)
$(ODIR)\debeval.obj:	$(HEADERS)
$(ODIR)\debfmt.obj: 	$(HEADERS) fmtstr.h ldouble.h
$(ODIR)\deblex.obj:	$(HEADERS) ldouble.h
$(ODIR)\debparse.obj:	$(HEADERS)
$(ODIR)\debsrch.obj:	$(HEADERS)
$(ODIR)\debsup.obj: 	$(HEADERS) debsym.h
$(ODIR)\debsym.obj: 	$(HEADERS) debsym.h
$(ODIR)\debtree.obj:	$(HEADERS)
$(ODIR)\debtyper.obj:	$(HEADERS)
$(ODIR)\debutil.obj:	$(HEADERS)
$(ODIR)\debwalk.obj:	$(HEADERS)
$(ODIR)\ldouble.obj:	ldouble.h ldouble.c
$(ODIR)\$(DLL).res:	version.rc debmsg.rc resource.h stdver.h $(LANGAPI)\include\version.h appver.h
