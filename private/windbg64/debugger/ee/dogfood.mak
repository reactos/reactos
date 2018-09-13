#	Makefile for C/C++ expression evaluator for NT
#
#	The following arguments are passed in from the master makefile
#
#		CC	 	compile command
#		ASM 	masm command
#		EELANG	language:
#		    	  can = ANSI C
#		    	  cxx = C++
#		OS  	operating system:
#		    	  n0  = i386
#		    	  n1  = MIPS
#
#	In addition, you can, if you choose, set DIR_CEXPR in your
#	environment to be the canonical path to the source files (including
#	a trailing backslash); this will cause files to be compiled with
#	a full canonical path to the source files, which makes it easier
#	for WinDbg or other debuggers to find them.


#---------------------------------------------------------------------
#	Macros
#---------------------------------------------------------------------

!ifndef NTPACK4
packingc=-Zp8
packinga=-DHOST32_PACK8
!else
packingc=-Zp4
packinga=-Zp4
!endif

CC = cl
ASM =ml 

!if "$(LANGAPI)" == ""
LANGAPI=\langapi
!endif

CFLAGS =  -c -W3 -MD -YX -G4 -Gyf -DWIN32 -DHOST32 -DADDR_32 $(CCMISC) -DWINQCXX -Di386 -D_X86_ \
	-Fo$(ODIR)\ -I $(LANGAPI)\include -I $(LANGAPI)\debugger $(packingc)\

!ifdef TARGETNB09
LPDB = -PDB:none
!else
LPDB = -PDB:$(ODIR)\cexpr.pbd
!endif

!ifndef ZSWITCH
ZSWITCH = -Zi -Fd$(ODIR)\cexpr.pdb
!endif

!ifdef RELEASE
DLL     =een0cxx
ODIR	=rdogfood
LFLAGS	=-debug:none
CFLAGS = $(CFLAGS) -O2
!else
DLL     =een0cxxd
ODIR	=dogfood
LFLAGS	=-debug:full -debugtype:cv $(LPDB)
CFLAGS = $(CFLAGS) -Od $(ZSWITCH) -G4 -Gyf -DDEBUGVER 
!endif

!ifndef NOBROWSER
CFLAGS = $(CFLAGS) -FR$(ODIR)^\
!endif

LIBS = msvcrt.lib

CEXPROBJS = \
	$(ODIR)\debapi.obj \
	$(ODIR)\debbind.obj \
	$(ODIR)\deberr.obj \
	$(ODIR)\debeval.obj \
	$(ODIR)\debfmt.obj \
	$(ODIR)\deblex.obj \
	$(ODIR)\deblexer.obj \
	$(ODIR)\debparse.obj \
	$(ODIR)\debsrch.obj \
	$(ODIR)\debsup.obj \
	$(ODIR)\debsym.obj \
	$(ODIR)\debtree.obj \
	$(ODIR)\debtyper.obj \
	$(ODIR)\debutil.obj \
	$(ODIR)\debwalk.obj \
	$(ODIR)\ldouble.obj \

# prev line must be blank


#---------------------------------------------------------------------
#	Inference rules
#---------------------------------------------------------------------


.SUFFIXES: .dll .obj .c .asm


.c{$(ODIR)}.obj:
		$(CC) $(CFLAGS)	$(DIR_CEXPR)$<


.asm{$(ODIR)}.obj:
		$(ASM) -Fo$*.obj -nologo -c -Cx -W2 -DHOST32 -DWIN32 $(packinga) $(DIR_CEXPR)$<


#---------------------------------------------------------------------
#	Targets
#---------------------------------------------------------------------


all: $(ODIR) $(DLL).dll


$(ODIR):
	@-mkdir $(ODIR)


$(DLL).dll: $(CEXPROBJS) dogfood.mak
	link $(LFLAGS) -def:<<$(ODIR)\$(DLL).def @<<$(ODIR)\$(DLL).lnk
LIBRARY $(DLL) initinstance
PROTMODE
DATA multiple nonshared

EXPORTS
	DBGVersionCheck
	EEInitializeExpr
<<KEEP
-dll
-implib:$(ODIR)\$(DLL).lib
-out:$(DLL).dll
-machine:$(CPU)
-base:@dllbase.txt,een0cxx
$(CEXPROBJS: = ^
)
$(LIBS)
<<KEEP
!ifndef NOBROWSER
	bscmake /n /o een0cxx $(ODIR)\*.sbr
!endif


#---------------------------------------------------------------------
#	Dependencies
#---------------------------------------------------------------------


HEADERS = $(LANGAPI)\debugger\types.h \
          $(LANGAPI)\debugger\cvtypes.h \
          $(LANGAPI)\include\cvinfo.h \
          $(LANGAPI)\debugger\shapi.h \
          $(LANGAPI)\debugger\eeapi.h \
          debdef.h \
          shfunc.h \
          debexpr.h \
          errors.h \
          debops.h

$(ODIR)\deblexer.obj:	debops.inc
$(ODIR)\debapi.obj: 	$(HEADERS) $(LANGAPI)\include\version.h
$(ODIR)\debbind.obj:	$(HEADERS) debsym.h
$(ODIR)\deberr.obj: 	$(HEADERS)
$(ODIR)\debeval.obj:	$(HEADERS)
$(ODIR)\debfmt.obj: 	$(HEADERS) fmtstr.h ldouble.h
$(ODIR)\deblex.obj:	$(HEADERS) ldouble.h
$(ODIR)\debparse.obj:	$(HEADERS)
$(ODIR)\debsrch.obj:	$(HEADERS)
$(ODIR)\debsup.obj: 	$(HEADERS)
$(ODIR)\debsym.obj: 	$(HEADERS) debsym.h
$(ODIR)\debtree.obj:	$(HEADERS)
$(ODIR)\debtyper.obj:	$(HEADERS)
$(ODIR)\debutil.obj:	$(HEADERS)
$(ODIR)\debwalk.obj:	$(HEADERS)
$(ODIR)\ldouble.obj:	ldouble.h ldouble.c
