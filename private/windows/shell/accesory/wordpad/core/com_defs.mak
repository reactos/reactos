#---------------------------------------------------------------------------
# Common startup stuff...
#
# Supported target OSes are DOS, MAC and NT
# Supported EXE types are EXE and DLL
# Supported compilers are C8 (VCW1.50), WINGS and C8_32, C9
# The host OS must be 32-bit; NT or Chicago (someday)
#
# OLE2 variable if defined implies we want to include OLE2 libraries.
#
#---------------------------------------------------------------------------

# Verify that OS and EX have been set appropriately, else break
!IFNDEF OS
!ERROR You must define OS to build this converter.
!ENDIF

!UNDEF DOS
!UNDEF MAC
!UNDEF NT

$(OS)=

!IFNDEF DOS
!IFNDEF MAC
!IFNDEF NT
!ERROR You must define OS to one of [DOS|MAC|NT] to build this converter.
!ENDIF
!ENDIF
!ENDIF


!IFNDEF EX
!ERROR You must define EX to build this converter.
!ENDIF

!UNDEF EXE
!UNDEF DLL

$(EX)=

!IFNDEF EXE
!IFNDEF DLL
!ERROR You must define EX to EXE or DLL to build this converter.
!ENDIF
!ENDIF

# define 'combo' macros that specify both an OS and EXE type
!IFDEF DLL

!IFDEF DOS
WIN_DLL=
!ELSEIFDEF MAC
MAC_EC=
!ELSEIFDEF NT
NT_DLL=
!ELSE
!ERROR You must define OS to one of [DOS|MAC|NT] to build this converter DLL.
!ENDIF

!ELSE # !DLL

!IFDEF DOS
DOS_SA=
!ELSEIFDEF MAC
MAC_SA=
!ELSEIFDEF NT
!ERROR No such thing (currently) as an NT standalone (only NT DLL)
!ELSE
!ERROR You must define OS to one of [DOS|MAC|NT] to build this converter.
!ENDIF

!ENDIF # !DLL


# If project location isn't explicitly set, assume it's in c:\conv
# (like all our other tools do.)
!IFNDEF CONV_PRJ
CONV_PRJ=C:\CONV
!ENDIF


# default language
!IFNDEF LANG
LANG = USA
!ENDIF

#--------------
#  TOOLS
#--------------

# the build should use the standard Windows NT shell, even if the caller
# uses another shell, like 4NT
COMSPEC  = $(SYSTEMROOT)\system32\cmd.exe

# compiler independent tools
# these shouldn't need to concern themselves with what type of binary it is
# (i.e. bound, protect, 32-bit)
#
# Actually, we do need to concern ourselves with the type of binary -- we
# need to know the architecture for NT tools (MIPS/Alpha/x86)

# ARCH is the host architecture for NT tools
# ARCHDIR is the dir for the TARGET architecture.  This only applies to NT blds
# BLDTOOLS is the root of the bldtools tree
# BLDTOOLSBIN is the COMMON bldtools directory (no executables here)
# BLDTOOLSBINNT is the Architecture dependant build tools

ARCHDIR=
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
ARCH=X86
!else if "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
ARCH=ALPHA
!else if "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
ARCH=MIPS
!else if "$(PROCESSOR_ARCHITECTURE)" == "PPC"
ARCH=PPC
!else
!ERROR Don't know how to build on $(PROCESSOR_ARCHITECTURE) yet...
!endif

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
NTARCH=i386
!else
NTARCH=$(ARCH)
!endif

BLDTOOLS    = $(CONV_PRJ)\bldtools
BLDTOOLSBIN = $(CONV_PRJ)\bldtools\bin 
BLDTOOLSBINNT = $(BLDTOOLSBIN)$(ARCH)

AWK      = $(BLDTOOLSBINNT)\gawk.exe
CHSTRANS = $(BLDTOOLSBINNT)\chstrans.exe
CP       = $(BLDTOOLSBINNT)\cp.exe
ECHOWM   = $(BLDTOOLSBINNT)\echowm.exe
FORKIZE  = $(BLDTOOLSBINNT)\forkize.exe
RM       = del /f /q
MV       = move
FIXCV    = $(BLDTOOLSBINNT)\fixcv.exe
SORT     = $(BLDTOOLSBINNT)\sort.exe
TOK      = $(BLDTOOLSBINNT)\tok.exe
TOUCH    = $(BLDTOOLSBINNT)\touch.exe
!if "$(ARCH)" == "X86"
MKRTFTBL = $(BLDTOOLSBINNT)\mkrtftbl.exe
!else
MKRTFTBL = $(PERL) $(BLDTOOLSBIN)\mkrtftbl.bat
!endif
MKSTRID  = $(AWK) -f $(BLDTOOLSBIN)\mkstrid.awk
MKSTRRES = $(AWK) -f $(BLDTOOLSBIN)\mkstrres.awk
WAITACC  = $(BLDTOOLSBINNT)\waitacc.exe
VICTORY  = $(BLDTOOLSBINNT)\victory.exe
DEFEAT   = $(BLDTOOLSBINNT)\defeat.exe

# set up compiler package based on target environment

!IFDEF DOS
COMPILER = C8
C8=
!ELSEIFDEF MAC
COMPILER = WINGS
WINGS=
!ELSEIFDEF NT
#COMPILER = C8_32
#C8_32=
COMPILER = C9
C9=
ARCHDIR=\$(ARCH)
!ELSE
!ERROR You must define OS to one of [DOS|MAC|NT] to build this converter.
!ENDIF

COMP    = $(BLDTOOLS)\$(COMPILER)

!ifdef NT_WORDPAD
!ifndef NT_TOOLS
COMPBIN = $(SYSTEMROOT)\mstools
!else
COMPBIN = $(NT_TOOLS)
!endif
COMPLIB = $(_NTDRIVE)\nt\public\sdk\lib\$(NTARCH)
COMPINC = $(_NTDRIVE)\nt\public\inc
!else
COMPBIN = $(BLDTOOLS)\$(COMPILER)\bin$(ARCHDIR)
COMPLIB = $(BLDTOOLS)\$(COMPILER)\lib$(ARCHDIR)
COMPINC = $(BLDTOOLS)\$(COMPILER)\include
!endif

# set up compiler/linker/assembler/resource compiler
# note rc's aren't clear yet, some need wrappers to find right rcpp and .err files
!IFDEF WINGS
CC    = $(COMPBIN)\cl.exe /nologo
LINK  = $(COMPBIN)\link.exe /nologo
LIBEXE= $(COMPBIN)\link.exe -lib
MASM  = $(COMPBIN)\asm68.exe
RC    = $(COMPBIN)\rc.exe /nologo
MRC   = $(COMPBIN)\mrc.exe
CPP   = $(COMPBIN)\cpp.exe
BSCMAKE= $(COMPBIN)\BscMake.exe /nologo
RCEXT =
!ELSEIFDEF C8
CC    = $(COMPBIN)\cl.exe /nologo
LINK  = $(COMPBIN)\link.exe /nologo
LIBEXE= $(COMPBIN)\lib.exe
MASM  = $(BLDTOOLSBIN)\_masm.bat
RC    = $(BLDTOOLSBIN)\_winrc31.bat
RCEXT = R
!ELSEIF defined(C8_32) || defined(C9)
CC    = $(COMPBIN)\cl.exe
LINK  = $(COMPBIN)\link.exe
LIBEXE= $(COMPBIN)\lib.exe
MASM  = $(COMPBIN)\masm386.exe
RC    = $(COMPBIN)\rc.exe
RCEXT = P
!ELSE
!ERROR Compiler must be one of [WINGS|C8|C8_32|C9]
!ENDIF

# Default to no specific RC dependencies (individual makefiles can change this)
RCDEP = 

# where cmacros.inc and include files for assembly languages files are.
AINC = $(BLDTOOLS)\include

# where are the common routines?
WPDIR = $(CONV_PRJ)\core
CHMAP = $(CONV_PRJ)\chmap

LOCALCLEAN = 

#---------------------------------------------------------------------------
# FOR Browser Database.
#---------------------------------------------------------------------------

!IFDEF  TAG
PWBTAG = /FR$(@R).sbr
!ENDIF

!IFDEF ASMLISTING
PWBTAG = $(PWBTAG) /Fc$(@R).lst
!ENDIF

#---------------------------------------------------------------------------
# C compiler flags

# Hook to allow user to specify cflags on the command line.
# Only for 'interactive' use; nothing checked in should have any
# dependencies on MYFLAGS.
CFLAGS= $(MYFLAGS)
PCHFLAGS=/Yuconv.h /Fp$(DIR)\conv.pch

CFLAGS= $(CFLAGS) /c /D$(OS) /D$(PRODUCT) /D_$(ARCH)_ /W3

# Compiler specific flags
!IFDEF WINGS
CFLAGS= $(CFLAGS) /D_MAC /Zpe /Q68b
!ELSEIFDEF C8
CFLAGS= $(CFLAGS) /DWIN /DPC /Zpe
!ELSEIF DEFINED (C8_32) || DEFINED(C9)
# /DNT done by /D$(OS) above
# Use intrinsic memset set for NT?
CFLAGS= $(CFLAGS) /DPC /Zpe /Oi
#CFLAGS= $(CFLAGS) /DPC /Zp1 /Ze /Oi-
!ELSE
!ERROR Compiler must be one of [WINGS|C8|C8_32|C9]
!ENDIF

# EXE type specific flags
!IFDEF MAC_EC
CFLAGS= $(CFLAGS) /DMACEC
!ELSEIFDEF MAC_SA
CFLAGS= $(CFLAGS) /DMACSA
!ENDIF

# Flat32 Helper (saves #if defined(NT) || defined(MAC))
!IF defined (NT) || defined(MAC)
CFLAGS= $(CFLAGS) /DFLAT32
!ENDIF

# pass NEEDCRT to C source
!IFDEF NEEDCRT
CFLAGS= $(CFLAGS) /DNEEDCRT
!ENDIF


#---------------------------------------------------------------------------
# Include Path : The list of directories that compiler will search through
#                to find .h files.  Each file is preceded with a /I to be
#                used when setting CL compiler variable. 

#some of these can be handled by $COMPINC
!IFDEF WINGS
CINCLUDEPATH=/I$(BLDTOOLS)\wings\include /I$(BLDTOOLS)\wings\include\macos
!IFDEF DOCFILE
CINCLUDEPATH=$(CINCLUDEPATH) /I$(BLDTOOLS)\ole2\mac
!ENDIF
!ELSEIFDEF C8
CINCLUDEPATH=/I$(BLDTOOLS)\c8\include
!IFDEF DOCFILE
CINCLUDEPATH=$(CINCLUDEPATH) /I$(BLDTOOLS)\ole2\dos
!ENDIF
!ELSEIFDEF C8_32
CINCLUDEPATH=/I$(BLDTOOLS)\c8_32\include
!error c8_32?
!ELSEIFDEF C9
#NOTE:  No special docfile stuff -- its in Daytona already!
!IFDEF NT_WORDPAD
CINCLUDEPATH=/I$(_NTDRIVE)\nt\public\sdk\inc /I$(_NTDRIVE)\nt\public\sdk\inc\crt
!ELSE
CINCLUDEPATH=/I$(BLDTOOLS)\c9\include
!ENDIF
!ELSE
!ERROR Compiler must be one of [WINGS|C8|C8_32|C9]
!ENDIF

# /X specifies that the INCLUDE environment variable should be ignored
INCLUDEPATH=/X /I. /I$(DIR) /I$(WPDIR) /I$(BLDTOOLS)\include /I$(CHMAP) $(CINCLUDEPATH)


#---------------------------------------------------------------------------
# set DOCFILE flags and assign appropriate directory for OLE2 headers
# deal also with STATIC docfiles

!IF DEFINED(DOCFILE) || defined(OLE2)

OLEDIR=$(BLDTOOLS)\ole2
!IFDEF DOCFILE
CFLAGS=$(CFLAGS) /DDOCFILE
!ENDIF

!IFDEF MAC
OLEINC=$(OLEDIR)\mac
!IFDEF STATICDF
NEEDCRT=YES
CFLAGS=$(CFLAGS) /DSTATICDF
!ENDIF
!ELSEIFDEF DOS
OLEINC=$(OLEDIR)\dos
!ELSEIFDEF NT
!IFDEF NT_WORDPAD
OLEINC=$(_NTDRIVE)\nt\public\sdk\inc
!ELSE
OLEINC=$(CINCLUDEPATH:/I=)
!ENDIF
!ENDIF

!IFNDEF NT
INCLUDEPATH=$(INCLUDEPATH) /I$(OLEINC)
!ENDIF # NT
!ENDIF # DOCFILE || OLE2


#---------------------------------------------------------------------------
# Finally set the include path for the build

CFLAGS = $(CFLAGS) $(INCLUDEPATH)
AFLAGS = /T /I$(AINC)


#---------------------------------------------------------------------------
# Final executable file extension

PEX = pex				### Portable EXecutable, Linker will create .PEX file.
!IFDEF EXE
EXT = exe
!ELSE # DLL
!IFDEF MAC
EXT = exe
!ELSE
EXT = cnv
!ENDIF
!ENDIF


#---------------------------------------------------------------------------
# Link flags

!IFDEF WINGS
!IFDEF MAC_SA
LFLAGS= -machine:m68k -entry:mainCRTStartup -out:$(DIR)\$(EXENAME).$(PEX) -NODEFAULTLIB -MAP
!ELSE # MAC_EC
LFLAGS= -machine:m68k -entry:EcMain -out:$(DIR)\$(EXENAME).$(PEX) -NODEFAULTLIB ^
		-MAP -section:ENTRY,,resource="EXCC"@0
!ENDIF
!ELSEIFDEF NT
!if "$(ARCH)" == "X86"
LFLAGS= -base:0x01400000 -machine:i386 -out:$(DIR)\$(EXENAME).$(PEX) -NODEFAULTLIB -MAP
!else
LFLAGS= -base:0x01400000 -machine:$(ARCH) -out:$(DIR)\$(EXENAME).$(PEX) -NODEFAULTLIB -MAP
!endif
!if "$(ARCH)" == "X86"
DLLINIT=_DllMainCRTStartup@12		# C run-time startup
!else
DLLINIT=_DllMainCRTStartup
!endif
!ELSE
LFLAGS= /NOD /MAP /NOE /NOPACKCODE /FARCALL
!ENDIF

#---------------------------------------------------------------------------
# RC flags

!IFDEF WINGS
RCFLAGS= /I$(BLDTOOLS)\wings\include\mrc /I$(DIR) /I$(WPDIR) /DMAC /DUSESTRINGS
!ENDIF

!IFDEF OFFICIAL
RCFLAGS = $(RCFLAGS) /DOFFICIAL
!ENDIF

!IFDEF FINAL
RCFLAGS = $(RCFLAGS) /DFINAL
!ENDIF

!IFDEF MAC_SA
RCFLAGS = $(RCFLAGS) /DMACSA
!ENDIF

#---------------------------------------------------------------------------
# debug options

!IFDEF NONDEBUG
MAKEKIND = NONDEBUG
LNKEXT = ndb
!IFNDEF MAC
LMEMLIB= $(CONVLIB)\lmem.d86
!ENDIF #MAC

!IFDEF PROFILE
CFLAGS = /Od /Zi /Gs $(CFLAGS)
!IFDEF MAC
CFLAGS = $(CFLAGS) /Gh /DPROFILE
!ELSEIFDEF DOS
LFLAGS = /CO $(LFLAGS)
!ENDIF

!ELSEIFNDEF DLL	### ifdef PROFILE
CFLAGS= /Ot $(CFLAGS)
!ELSEIFDEF WINGS ### ifdef PROFILE
CFLAGS= /Ob1git /Gs $(CFLAGS)
!ELSEIFDEF NT_WORDPAD
CFLAGS= /Zi /Os /MD /DNT_WORDPAD $(CFLAGS)
LFLAGS= -debug -debugtype:both -opt:REF -pdb:none $(LFLAGS)
!ELSEIFDEF NT
!if "$(ARCH)" == "ALPHA"
CFLAGS= /Ob1i /Gs $(CFLAGS)
!else
CFLAGS= /Ob1git /Gs $(CFLAGS)
!endif
!ELSE
CFLAGS= /Ob1cegilt /Gs $(CFLAGS)
!ENDIF           ### ifndef DLL

!ELSE            ### ifdef NONDEBUG

MAKEKIND = DEBUG
LNKEXT = lnk
!IFNDEF MAC
LMEMLIB= $(CONVLIB)\dlmem.d86 $(CONVLIB)\debug.d86
!ENDIF #MAC
!IF "$(ARCH)" == "ALPHA"
CFLAGS= /Od /Zil /DDEBUG $(CFLAGS) /Gs
AFLAGS= /Zi /DDEBUG $(AFLAGS)
!ELSE
CFLAGS= /Od /Zi /DDEBUG $(CFLAGS) /Gs
AFLAGS= /Zi /DDEBUG $(AFLAGS)
!ENDIF
!IFDEF WINGS
!IFDEF PROFILE
CFLAGS = $(CFLAGS) /Gh /DPROFILE
!ENDIF
CFLAGS = $(CFLAGS) /Q68m /Fd$(DIR)\$(PRODUCT).pdb
LFLAGS= $(LFLAGS) -debug:notmapped,full -debugtype:cv
!ELSEIFDEF NT
LFLAGS= $(LFLAGS) -debug -debugtype:cv -pdb:none
!ELSE #WINGS/NT
CFLAGS = $(CFLAGS) /Z7
LFLAGS= /CO $(LFLAGS)
!ENDIF #WINGS/NT
RCFLAGS= /DDEBUG $(RCFLAGS)
!ENDIF           ### ifdef NONDEBUG


#---------------------------------------------------------------------------
# OS specific options

!IFDEF NT
# Since we're using static C runtimes, don't want _MT or _DLL
CFLAGS = $(CFLAGS) /DDLL
!ELSE # !NT

!IFNDEF WINGS 
LFLAGS = $(LFLAGS) /WARNFIXUP 
!IFNDEF DLL
LFLAGS = $(LFLAGS) /Stack:6144
!ENDIF #DLL
CFLAGS= $(CFLAGS) /DMS_DOS /DMSDOS
!ENDIF #WINGS
!ENDIF #NT

#---------------------------------------------------------------------------
# standard libraries are both EXE type and compiler specific

!IFDEF DLL

!IFDEF WINGS
CLIB = $(COMPLIB)\interfac.lib
!IF DEFINED(NEEDCRT) || DEFINED(PROFILE)
CLIB = $(CLIB) $(COMPLIB)\libc.lib
!ELSE
CLIB = $(CLIB) $(COMPLIB)\xlibc.lib
!ENDIF #NEEDCRT
!ELSEIFDEF C8
CLIB = $(COMPLIB)\mcnv.lib $(COMPLIB)\libw.lib $(BLDTOOLS)\lib\crmgr.w86
!IFDEF NEEDCRT
CLIB = $(CLIB) $(COMPLIB)\mdllcew.lib
!ENDIF #NEEDCRT
!ELSEIF DEFINED (C8_32) || DEFINED(C9)
# Use C runtimes for NT, even with DLL
# Note: _Should_ use $(guilibs) from ntwin32.mak, but don't need them all
!IFDEF NT_WORDPAD
CLIB = $(CLIB) $(COMPLIB)\msvcrt.lib
!ELSE
CLIB = $(CLIB) $(COMPLIB)\libc.lib
!ENDIF
CLIB = $(CLIB) $(COMPLIB)\kernel32.lib $(COMPLIB)\user32.lib $(COMPLIB)\advapi32.lib $(COMPLIB)\version.lib
!ENDIF #WINGS

!IFNDEF MAC
!IF !DEFINED(C8_32) && !DEFINED(C9)
CFLAGS= $(CFLAGS) /Gw /G2	# real mode entry points and 286 instructions
!ENDIF #C8_32
CFLAGS= $(CFLAGS) /DDLL		# don't define DLL for the Mac!
!ENDIF #MAC

!ELSE # !DLL, i.e. a standalone

!IFDEF WINGS
CLIB=$(COMPLIB)\libc.lib $(COMPLIB)\interfac.lib
!ELSEIFDEF C8
CLIB=$(COMPLIB)\mlibce.lib
!ENDIF
!IFDEF DOS
CFLAGS=$(CFLAGS) /DDOSSA
!ENDIF

!ENDIF # !DLL

#---------------------------------------------------------------------------
# OLE libs

DFLIBS=
!IF DEFINED(DOCFILE) || defined(OLE2)

# set the appropriate path for the OLE2 libraries
!IFDEF WINGS
OLELIB=$(OLEDIR)\mac

!IFDEF STATICDF
CLIB = $(CLIB) $(OLELIB)\static.lib
# $(OLELIB)\ole2guid.obj $(OLELIB)\stdalocd.obj
#$(OLELIB)\dfstubd.obj $(OLELIB)\nyistubd.obj $(OLELIB)\olenyi.obj $(OLELIB)\olestubd.obj 

!ELSE	# !STATICDF 

!ifdef ASLM
CFLAGS = $(CFLAGS) /DASLM
DFLIBS = $(DIR)\wole2.lib $(OLELIB)\aslm.lib
!else	#!ASLM
!ifdef NONDEBUG
DFLIBS = $(OLELIB)\olenrf.obj
!else	#!NONDEBUG
DFLIBS = $(OLELIB)\olendf.obj
!endif	#NONDEBUG
!endif	#ASLM

CLIB = $(CLIB) $(DFLIBS)
!ENDIF	#!STATICDF

!ELSEIFDEF DOS	#!WINGS
OLELIB=$(OLEDIR)\dos
CLIB = $(CLIB) $(OLELIB)\compobj.lib $(OLELIB)\storage.lib $(OLELIB)\ole2.lib

!ELSEIFDEF NT	#!WINGS && !DOS
OLELIB=$(COMPLIB)
CLIB = $(CLIB) $(OLELIB)\ole32.lib
!ENDIF #!WINGS && !DOS

!ENDIF #DOCFILE || OLE2

#---------------------------------------------------------------------------
# Memory models

!IFDEF WINGS
CFLAGS= /AS $(CFLAGS)
!ELSEIFDEF C8
CFLAGS= /AM $(CFLAGS)
!ENDIF #!WINGS

#---------------------------------------------------------------------------
# EXE type stuff

!IFDEF DLL
LIBS= $(CLIB)
!ELSE
LIBS= $(LMEMLIB) $(CLIB)
!ENDIF

!IF DEFINED(PROFILE) && DEFINED(WINGS)
LIBS=$(COMPLIB)\mprof.lib $(LIBS)
!ENDIF

#---------------------------------------------------------------------------
# Character mapping stuff

!IFDEF NEED_CHMAP

CHLIBNAME=
!IFDEF WIN_DLL
!IFDEF NONDEBUG
CHLIBNAME = ChMapfrd
!ELSE
CHLIBNAME = ChMapdrd
!ENDIF #NONDEBUG
!ELSEIFDEF DOS_SA
!IFDEF NONDEBUG
CHLIBNAME = ChMapfrs
!ELSE
CHLIBNAME = ChMapdrs
!ENDIF #NONDEBUG
!ELSEIFDEF MAC_EC
!IFDEF NONDEBUG
CHLIBNAME = ChMapfrd
!ELSE
CHLIBNAME = ChMapdrd
!ENDIF #NONDEBUG
!ELSEIFDEF MAC_SA
!IFDEF NONDEBUG
CHLIBNAME = ChMapfrs
!ELSE
CHLIBNAME = ChMapdrs
!ENDIF #NONDEBUG
!ELSEIFDEF NT_DLL
!IFDEF NONDEBUG
CHLIBNAME = ChMapfpd
!ELSE
CHLIBNAME = ChMapdpd
!ENDIF #NONDEBUG
!ENDIF #WIN_DLL

CHMAPLIB = $(DIR)\$(CHLIBNAME).lib
LIBS = $(LIBS) $(CHMAPLIB)
CFLAGS = $(CFLAGS) /DCHMAP

!IFDEF DLL
CHMAPDLL = DLL
!ELSE
CHMAPDLL = NODLL
!ENDIF

!ENDIF # NEED_CHMAP

#---------------------------------------------------------------------------
# Internationalization stuff

!IF "$(LANG)" == "RUS"
RUSSIA=YES
!ELSEIF "$(LANG)" == "RU2"
RUSSIA2=YES
!ELSEIF "$(LANG)" == "GRE"
GREECE=YES
!ELSEIF "$(LANG)" == "TUR"
TURKEY=YES
!ELSEIF "$(LANG)" == "HUN" || "$(LANG)" == "CZE" || "$(LANG)" == "POL"
EAST_EUROPE=YES
!ENDIF

!IFDEF EAST_EUROPE
COUNTRYFLAG = /DEAST_EUROPE
!ELSEIFDEF RUSSIA
COUNTRYFLAG = /DRUSSIA
!ELSEIFDEF RUSSIA2
COUNTRYFLAG = /DRUSSIA2
!ELSEIFDEF GREECE
COUNTRYFLAG = /DGREECE
!ELSEIFDEF TURKEY
COUNTRYFLAG = /DTURKEY
!ELSE
COUNTRYFLAG =
!ENDIF

CFLAGS = $(CFLAGS) $(COUNTRYFLAG)


#---------------------------------------------------------------------------
# Allow codeview for fast builds if CODEVIEW defined on build line

!IFDEF CODEVIEW
!IFDEF WINGS
CFLAGS = $(CFLAGS) /Zi
!ELSE
CFLAGS = $(CFLAGS) /Z7i
!ENDIF
LFLAGS = /CO $(LFLAGS)
!ENDIF

!IFDEF SSDIR
RCFLAGS = $(RCFLAGS) /I $(SSDIR)
!ENDIF

!IFDEF DOS
!IFDEF WIN31
WINVERRCFLAGS = /31
!ELSE
WINVERRCFLAGS = /30
!ENDIF
!ENDIF

#---------------------------------------------------------------------------
# Define build directories

!IFNDEF DIR     #if DIR not defined from Command line
!IFDEF NONDEBUG
DIR=$(OS)$(EX)_F    # DOSEXE_F/MACEXE_F/DOSDLL_F/MACDLL_F
!ELSE
DIR=$(OS)$(EX)_D    # DOSEXE_D/MACEXE_D/DOSDLL_D/MACDLL_D
!ENDIF  # NONDEBUG
!ENDIF



#---------------------------------------------------------------------------
# File lists

HEADERS = $(WPDIR)\conv.h $(WPDIR)\convcom.h $(WPDIR)\convtype.h \
$(WPDIR)\ConvFile.h $(WPDIR)\ConvUtil.h $(WPDIR)\ConvIo.h $(WPDIR)\convout.h $(WPDIR)\convtime.h \
$(WPDIR)\wptowp.h $(WPDIR)\Rtf.h $(WPDIR)\Heap.h $(WPDIR)\Debug.h\
$(WPDIR)\converr.h $(WPDIR)\ErrRet.h $(WPDIR)\machine.h $(WPDIR)\Global.h \
$(WPDIR)\codeseg.h $(WPDIR)\ConvTabl.h $(WPDIR)\ResUtil.h \
$(WPDIR)\iszrtf.h

!IFDEF WIN_DLL
HEADERS = $(HEADERS) $(WPDIR)\convwin.h $(WPDIR)\winlib.h $(WPDIR)\initc.h \
$(WPDIR)\convmwin.h $(WPDIR)\winutil.h $(WPDIR)\WinHeap.h \
$(BLDTOOLS)\include\crmgr.h
!elseifdef MAC_EC
HEADERS = $(HEADERS) $(WPDIR)\convmac.h $(WPDIR)\maclib.h $(WPDIR)\convmec.h \
$(WPDIR)\macutil.h $(WPDIR)\exconv.h $(WPDIR)\MacHeap.h
!elseifdef MAC_SA
HEADERS = $(HEADERS) $(WPDIR)\convmac.h $(WPDIR)\maclib.h $(WPDIR)\convmec.h \
$(WPDIR)\macsa.h $(WPDIR)\macutil.h $(WPDIR)\exconv.h $(WPDIR)\MacHeap.h
!elseifdef DOS_SA
HEADERS = $(HEADERS) $(WPDIR)\convdos.h $(WPDIR)\main.h $(WPDIR)\convmsta.h
!elseifdef NT_DLL
HEADERS = $(HEADERS) $(WPDIR)\convnt.h $(WPDIR)\winlib.h $(WPDIR)\initc.h \
$(WPDIR)\convmwin.h $(WPDIR)\NtHeap.h
!endif

!ifdef NEED_CHMAP
HEADERS = $(HEADERS) $(CHMAP)\chmap.h 
!endif

!ifdef DOCFILE
# Note: compobj.h not used directly, but included in ole2.h
HEADERS = $(HEADERS) $(WPDIR)\convdfio.h $(OLEINC)\compobj.h $(OLEINC)\ole2.h
!endif

!ifdef OLE2
# Note: compobj.h not used directly, but included in ole2.h
HEADERS = $(HEADERS) $(OLEINC)\compobj.h $(OLEINC)\ole2.h
!endif

COMOBJ1= $(DIR)\PreComp.Obj $(DIR)\ConvUtil.obj $(DIR)\convio.obj $(DIR)\convcom.obj $(DIR)\convout.obj $(DIR)\convtime.obj
COMOBJ2= $(DIR)\ConvTabl.obj $(DIR)\ConvTbl2.obj $(DIR)\WpToWp.obj
COMOBJ3= $(DIR)\Heap.obj $(DIR)\Global.obj

!IFNDEF MAC
COMOBJ3= $(COMOBJ3) $(DIR)\winutil.obj
!IFNDEF NT
COMOBJ3= $(COMOBJ3) $(DIR)\ConvUtl1.obj
!ENDIF #!NT
!ENDIF #!MAC

!IFDEF MAC
COMOBJ3 = $(COMOBJ3) $(DIR)\MacLoIo.obj $(DIR)\MacUtil.obj
!ELSEIFDEF WIN_DLL
COMOBJ3 = $(COMOBJ3) $(DIR)\WinLoIo.obj
!ELSEIFDEF NT
COMOBJ3 = $(COMOBJ3) $(DIR)\NtLoIo.obj
!IF "$(ARCH)" == "MIPS"
COMOBJ3 = $(COMOBJ3) $(DIR)\mipsseek.obj
!ENDIF #!MIPS
!ENDIF

!IFNDEF NONDEBUG
COMOBJ3= $(COMOBJ3) $(DIR)\debug.obj
!ENDIF

!IFDEF DOCFILE
COMOBJ3= $(COMOBJ3) $(DIR)\convdfio.obj
!ENDIF


!IFDEF MAC_EC
COMOBJ4 = $(DIR)\convmec.obj $(DIR)\dojmp.obj $(DIR)\setjmp.obj $(DIR)\maccdres.obj
COMOBJ5 = $(DIR)\ecentry.obj $(DIR)\loaddata.obj $(DIR)\loadseg.obj
COMOBJ6 = $(DIR)\maclib.obj $(DIR)\macheap.obj $(DIR)\regh.obj $(DIR)\convmac.obj
!ELSEIFDEF MAC_SA
COMOBJ4 = $(DIR)\macsa.obj $(DIR)\maclib.obj $(DIR)\macheap.obj $(DIR)\regh.obj $(DIR)\convmac.obj
COMOBJ5 = $(DIR)\ecentry.obj $(DIR)\convmec.obj $(DIR)\dojmp.obj
!ELSEIFDEF EXE
COMOBJ4= $(DIR)\ConvMSta.obj $(DIR)\main.obj $(DIR)\lmemheap.obj $(DIR)\convdos.obj
!ELSE #DLL
!IFNDEF NT
COMOBJ3B= $(DIR)\initc.obj $(DIR)\winlib.obj
COMOBJ4= $(DIR)\init.obj $(DIR)\doslib.obj $(DIR)\doswin.obj
!ELSE
COMOBJ3B= $(DIR)\initcnt.obj $(DIR)\winlib.obj
COMOBJ4=
!ENDIF #NT
COMOBJ5= $(DIR)\convmwin.obj $(DIR)\winheap.obj
COMOBJ6= $(DIR)\convwin.obj
!IFNDEF NONDEBUG
!IFNDEF NT
COMOBJ6= $(COMOBJ6) $(DIR)\windebug.obj
!ENDIF #NT
!ENDIF #NONDEBUG
!ENDIF #EXE

# Note:  initc.obj MUST be first .obj in .lnk file so that CreateStack 
# doesn't trash various globals.

COMOBJS = $(COMOBJ3B) $(COMOBJ4) $(COMOBJ2) $(COMOBJ3) $(COMOBJ1) $(COMOBJ5) $(COMOBJ6)


#---------------------------------------------------------------------------
#
# compile/assemble rules
#

DEPENDENT=$<

.c.pp:
	$(ECHOWM) Preprocessing $<...
	$(CC) $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /E $(DEPENDENT) $< > temp.pre
	$(SED) "/^$$/d" <temp.pre >$@
	$(RM) temp.pre

.c.asm:
	$(ECHOWM) Creating source file $<...
	$(CC) $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /FAs /Fa $<

{$(WPDIR)}.c.asm:
	$(ECHOWM) Creating source file $<...
	$(CC) $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /FAs /Fa $<
	
{$(WPDIR)}.c.pp:
	$(ECHOWM) Preprocessing $<...
	$(CC) $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /E $(DEPENDENT) $< > temp.pre
	$(SED) "/^$$/d" <temp.pre >$@
	$(RM) temp.pre

{.}.c{$(DIR)}.obj:
	@if exist $*.sbr del $*.sbr
	$(CC)  $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /Fo$@ $(DEPENDENT)
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

{$(WPDIR)}.c{$(DIR)}.obj:
	@if exist $*.sbr del $*.sbr
	$(CC)  $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /Fo$@ $(DEPENDENT)
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

!IFNDEF WINGS

{.}.asm{$(DIR)}.obj:
	$(MASM) $(AFLAGS) $(DEPENDENT),$@,nul;
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

{$(WPDIR)}.asm{$(DIR)}.obj:
	$(MASM) $(AFLAGS) $(DEPENDENT),$@,nul;
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

!ELSE

{$(WPDIR)}.asm{$(DIR)}.obj:
	$(CPP) $(CINCLUDEPATH:/=-) $(DEPENDENT) >$(DIR)\foo.pp
	$(MASM) -z -c -o $@ $(DIR)\foo.pp
	del $(DIR)\foo.pp
	
!ENDIF


!IFDEF SSDIR

{$(SSDIR)}.c{$(DIR)}.obj:
	@if exist $*.sbr del $*.sbr
	$(CC)  $(CFLAGS) $(PCHFLAGS) $(PWBTAG) /Fo$@ $(DEPENDENT)
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

{$(SSDIR)}.asm{$(DIR)}.obj:
	$(MASM) $(AFLAGS) $(DEPENDENT),$@,nul;
!IF DEFINED(SHAREDRIVE) && !DEFINED(NONDEBUG)
	$(FIXCV) $(SHAREDRIVE) $(BUILDDRIVE) $@
!ENDIF

!ENDIF

#---------------------------------------------------------------------------
# End of common block
#---------------------------------------------------------------------------

