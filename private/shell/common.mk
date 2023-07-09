##########################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991-96
#	All Rights Reserved.
#
##########################################################################

#
# Shell Applet/DLL makefile
#
#
# Required definitions:
#
#     ROOT
#        Path to common project root.
#
#     NAME
#        Base name of project used for:
#           .def input file, if any
#           .rc input file
#           .rcv input file
#           .res output file
#
# Definitions used if defined:
#
#     BUILDDLL
#        Build .exe if this is not defined.
#
#     BUILDLIB
#        Build .exe if this is not defined.
#
#     BUILD
#        One of:
#           debug    debug Win32 build
#           retail   retail Win32 build
#           all      debug and retail
#           depend   generate dependencies
#
#     DEFNAME
#        Use NAME.def if not defined
#
#     RESNAME
#        Use NAME.res if not defined
#
#     RCNAME
#        Use NAME.rc if not defined
#
#     RCVNAME
#        Use NAME.rcv if not defined
#
#     ILINK
#        Use incremental link
#
#     CVWRETAIL
#        Compile with debug flags compatible with VC++ debugger.
#
#     LEGO
#        Compile with flags for LEGO support.
#
#     BROWSE
#        Create .sbr files for browser database.
#
#     DBCS
#        Define DBCS.
#
#     MAKELIST
#        Make an assembly listing for each compiled file.
#
#     PRIVINC
#        Use NAME.h/pch as precompiled header if not defined.
#
#     APPEXT
#        Use .dll, .exe or .lib (based upon BUILDDLL) as extension if this 
#        is not defined.
#
#     DLLBASE
#        Specifies the base address of the component, as passed
#        to the linker.  May also be:
#            PREFBASE   use coffbase.txt 
#
#     DLLENTRY
#        Use LibMain as the DLL entry-point if not defined.  Valid
#        only if BUILDDLL is defined.
#
#     RES_DIR
#        Use .\messages\usa as the resources dir, if not defined.
#
#     STATOBJx
#        Other object modules or libraries to use to build an import
#        library.  'x' may be 0 thru 9.  Start with 0.
#
#     FIRSTOBJS
#        Guaranteed to be the first object modules linked, if this
#        really matters to you.
#
#     PCHOBJx
#        C object modules compiled with a precompiled header.  'x'
#        may be 0 thru 9.  Start with 0.
#
#     THKOBJx
#        Object modules used for thunks.  'x' may be 0 thru 9.  Start 
#        with 0.
#
#     MISCOBJx
#        All other object modules.  'x' may be 0 thru 9.  Start with 0.
#        
#     LIBx
#        List of libraries to link, in order of 'x' (which may be 
#        0 thru 9).  If LIB0 is not defined, then "libw" and "mnocrtw"
#        are prepended to the link line.
#
#     CLEANLIST
#        List of files to clean, outside of the default files.
#
#     DESTINATION
#        Directory to copy default targets to after successful build.
#
#     CFLAGS
#        C compiler switches to be used on cl command line.
#
#     SRCDIR
#        The directory that contains the source.  If not defined,
#        then it is set to the parent.
#
#     WIN32
#        Build 32-bit component.
#
#     NOPDB
#        Compile with /Zd debug option rather than /Zi
#

WANT_C1032      = TRUE

!ifdef ILINK
DOILINK         = TRUE
!endif

!ifndef NAME
!ERROR NAME variable not defined.
!endif

!ifndef ROOT
!ERROR ROOT environment variable not defined.
!endif

#
# Set destination directory.
#

!if "$(BUILD)" == "debug" || "$(BUILD)" == "retail" || "$(BUILD)" == "maxdebug"
DEFAULTVERDIR   = $(BUILD)
!endif

#
# Set tools' paths
#

INCLUDES        = includes.exe

#
# Set tool options
#

INCLUDES_SWITCHES = -e -i -L. -S.


#
# Set fundamentals
#

!ifndef PRIVINC
PRIVINC         = $(NAME)
!endif

!ifndef APPEXT
!ifdef BUILDDLL
APPEXT          = dll
!elseif defined(BUILDLIB)
APPEXT          = lib
! else
APPEXT          = exe
!endif
!endif # APPEXT

# Default .def file name.
!ifndef DEFNAME
DEFNAME         = $(NAME).def
!endif

# Default .res file name.
!ifndef RESNAME
RESNAME         = $(NAME).res
!endif

# Default .rc file name.
!ifndef RCNAME
RCNAME          = $(NAME).rc
!endif

# Default .rcv file name.
!ifndef RCVNAME
RCVNAME         = $(NAME).rcv
!endif

# Default entry point and base for dlls.
!ifdef WIN32
!ifdef BUILDDLL
!if "$(DLLBASE)" == "PREFBASE"
DLLBASE         = @$(ROOT)\dev\inc\coffbase.txt,$(NAME)
!endif
!ifndef DLLENTRY
DLLENTRY        = LibMain
!endif
!endif
!endif

# Default to having the retail version of a dll produce a public lib
!if "$(VERDIR)" == "retail" && defined(BUILDDLL)
MKPUBLIC        = TRUE
!endif

#
# Lists of object modules
#
STATOBJS        = $(STATOBJ0) $(STATOBJ1) $(STATOBJ2) $(STATOBJ3) \
                  $(STATOBJ4) $(STATOBJ5) $(STATOBJ6) $(STATOBJ7) \
                  $(STATOBJ8) $(STATOBJ9)

MISCOBJS        = $(FIRSTOBJS) $(MISCOBJ0) $(MISCOBJ1) $(MISCOBJ2) \
                  $(MISCOBJ3) $(MISCOBJ4) $(MISCOBJ5) $(MISCOBJ6) \
                  $(MISCOBJ7) $(MISCOBJ8) $(MISCOBJ9)

PCHOBJS         = $(PCHOBJ0) $(PCHOBJ1) $(PCHOBJ2) $(PCHOBJ3) $(PCHOBJ4) \
                  $(PCHOBJ5) $(PCHOBJ6) $(PCHOBJ7) $(PCHOBJ8) $(PCHOBJ9)

THKOBJS         = $(THKOBJ0) $(THKOBJ1) $(THKOBJ2) $(THKOBJ3) $(THKOBJ4) \
                  $(THKOBJ5) $(THKOBJ6) $(THKOBJ7) $(THKOBJ8) $(THKOBJ9)

!ifdef PCHOBJ0
INFERPCH        = TRUE
MISCOBJ9        = $(MISCOBJ9) pch.obj
PCH_C_SRC       = pch.c
!endif # PCHOBJ0

!ifdef CPPPCHOBJS
INFERPCHCPP     = TRUE
MISCOBJ9        = $(MISCOBJ9) pchcpps.obj
PCH_CPP_SRC     = pchcpps.cpp
!endif # CPPPCHOBJS

OBJS            = $(MISCOBJS) $(PCHOBJS) $(THKOBJS) $(CPPOBJS) $(CPPPCHOBJS)

!ifndef LIB0
LIB0            = libw mnocrtw
!endif
LIBS            = $(LIB0) $(LIB1) $(LIB2) $(LIB3) $(LIB4) $(LIB5) \
                  $(LIB6) $(LIB7) $(LIB8) $(LIB9)


#-----------------------------------------------------------------------
#  Branch depending on the level of makefile recursion
#-----------------------------------------------------------------------

!ifndef VERDIR

# (repcmd doesn't seem to like leading or trailing spaces)
!ifdef VERSIONLIST
VERSIONLIST     =debug retail $(VERSIONLIST)
!else
VERSIONLIST     =debug retail
!endif

COMMONMKFILE    = makefile


!include $(ROOT)\shell\shell.mk


$(RESNAME):
	cd $(BUILD)
	$(MAKE) BUILD=$(BUILD) VERDIR=$(BUILD) $(MISC) -f ..\makefile $(RESNAME)
	cd ..


!else ## VERDIR


!ifndef SRCDIR
SRCDIR          = ..
!endif

#
# Macros for 'clean' command
#
CLEANLIST       = $(CLEANLIST) $(NAME).$(APPEXT) *.pch $(RESNAME)
!ifdef INFERPCH
CLEANLIST       = $(CLEANLIST) pch.c
!endif # INFERPCH
!ifdef INFERPCHCPP
CLEANLIST       = $(CLEANLIST) pchcpps.cpp
!endif # INFERPCHCPP
!ifdef BUILDDLL
CLEANLIST       = $(CLEANLIST) $(NAME).rdf $(NAME).lib
!endif

#
# Set 32-bitness for WIN32
#

!ifndef WIN32
IS_16           = TRUE
!else
IS_32           = TRUE
!endif

IS_PRIVATE      = TRUE
IS_SDK          = TRUE
MASM6           = TRUE

#
# Set compile flags
#

!ifndef WIN32

#
# Win16 flags
#

!if "$(VERDIR)" == "debug"
CFLAGS          = $(CFLAGS) -Od -Zid /f-	
AFLAGS          = $(AFLAGS) /Zim
RCFLAGS         = $(RCFLAGS) -DDEBUG
!else
CFLAGS          = $(CFLAGS) -Oxs
AFLAGS          = $(AFLAGS) /Zm

!ifdef CVWRETAIL
CFLAGS          = $(CFLAGS) -Zid
AFLAGS          = $(AFLAGS) /Zi
!endif

!endif  # VERDIR

!else   # !WIN32

#
# Win32 flags
#

# (error out on strict warnings, like the NT build does)
CFLAGS          = $(CFLAGS) -W3 -WX

CFLAGS          = $(CFLAGS) -Gz -GF -Gy # stdcall
					# strings are const, merged
					# function separation
!if "$(VERDIR)" == "debug"

CFLAGS          = $(CFLAGS) -Od

!ifdef NOPDB
CFLAGS          = $(CFLAGS) -Zd
!else
CFLAGS          = $(CFLAGS) -Zi
!endif

L32FLAGS        = $(L32FLAGS) -debug
RCFLAGS         = $(RCFLAGS) -DDEBUG

# (make sure we can get .pdb files that work)
NOMERGETEXT     = TRUE    

!else  # DEBUG

# (full opt, favor size)
CFLAGS          = $(CFLAGS) -Oxs

!endif # DEBUG

#
# Support incremental linking.
#

!ifdef    DOILINK
CFLAGS          = $(CFLAGS) -Zi
L32FLAGS        = $(L32FLAGS) -incremental:yes -debug
!endif  # DOILINK

#
# Support for lego
#

!ifdef    LEGO
AFLAGS          = $(AFLAGS) /Zi
LEGO_LIBFLAGS   = -debugtype:cv
!endif  # LEGO

!endif # WIN32

#
# Create .SBR files for browser database
#

!ifdef BROWSE
CFLAGS          = $(CFLAGS) -Fr
!endif

FEATURE_IE40 = 1

!ifdef FEATURE_IE40
CFLAGS          = $(CFLAGS) -DFEATURE_IE40 -DNASH
!endif

#
# Hideous hack to ensure CL and ML are set in the environment
#
!if [set CL=;]
!endif

!if [set ML=;]
!endif

#
# Set international things
#

# note INTL_SRC, and LANG are external macros set by international
!ifdef LANG
TARGETS         = $(TARGETS) $(NAME).$(LANG)
!else
TARGETS         = $(TARGETS) $(NAME).$(APPEXT)

!if defined(BUILDDLL) && defined(MKPUBLIC)
TARGETS         = $(TARGETS) $(NAME).lib
!endif

!endif  # LANG

#
# Include other shell makefile
#

!include $(ROOT)\shell\shell.mk

#
# More compile flags after the include
#

!ifdef DBCS
CFLAGS          = $(CFLAGS) -DDBCS
AFLAGS          = $(AFLAGS) -DDBCS
RCFLAGS         = $(RCFLAGS) -DDBCS
!endif

# Don't build with memphis structures
CFLAGS          = $(CFLAGS) -D_WIN32_WINDOWS=0x0400

!ifndef WIN32
!ifndef NOPASCAL
CFLAGS          = $(CFLAGS) -Gc
!endif

!ifdef NOMORECFLAGS
CL              = $(CFLAGS)
!else
!ifdef BUILDDLL
!ifdef LARGEDLL
CL              = $(CFLAGS) -ALw -GD -W3 -DBUILDDLL
!else
CL              = $(CFLAGS) -AMw -GD -W3 -DBUILDDLL
!endif
!else
CL              = $(CFLAGS) -AMd -GA -W3
!endif  # BUILDDLL

!if "$(VERDIR)" == "retail"
CL              = $(CL) -G3
!else
CL              = $(CL) -G2
!endif  
!endif  # NOMORECFLAGS
!endif  # !WIN32

!ifdef MAKELIST
CL              = $(CL) -Fc
!endif

!ifdef WIN32
RCFLAGS         = $(RCFLAGS) -DWIN32
!ifdef BUILDDLL
CL              = $(CFLAGS) -W3 -DBUILDDLL -DWIN32 -D_X86_
!else
CL              = $(CFLAGS) -W3 -DWIN32 -D_X86_
!endif
!endif

ML              = $(AFLAGS)

CCH             = $(CC) -Yc$(PRIVINC).h
CCU             = $(CC) -Yu$(PRIVINC).h
CCX             = $(CC) 

LFLAGS          = /ALIGN:16 /MAP /NOE /NOD
!if "$(VERDIR)" == "debug" || DEFINED(CVWRETAIL)
LFLAGS          = /CO $(LFLAGS)                 # debug linker flags
L32FLAGS        = $(L32FLAGS) -debug
!endif

!ifndef RES_DIR
RES_DIR         = $(SRCDIR)\messages\usa
!endif


##############
# build rules
##############


default:        $(NAME).$(APPEXT)


#
# Individual makefiles should have more dependencies if needed
# Note that the RES file doesn't really depend on the PCH file, but
# it does depend on everthing the PCH file depends on.
#

$(RESNAME):	$(RES_DIR)\$(RCNAME) $(RES_DIR)\$(RCVNAME)
!ifdef WIN32
        @set OLDPATH=$(PATH)
        @set PATH=$(ROOT)\dev\tools\c1032\bin;$(PATH)
!endif
	$(RC) -r $(RCFLAGS) -I$(SRCDIR) -I$(RES_DIR) -Fo$(RESNAME) $(RES_DIR)\$(RCNAME)
!ifdef WIN32
        @set PATH=%OLDPATH%
!endif

!ifdef INFERPCH
$(RESNAME):	$(PRIVINC).pch
!else # INFERPCH
!ifdef INFERPCHCPP
$(RESNAME):	pchcpps.pch
!endif # INFERPCHCPP
!endif # INFERPCH


##################
# inference rules
##################


{$(SRCDIR)}.c.lst:
        @$(CC) -Fc$*.lst $(SRCDIR)\$*.c

{$(SRCDIR)}.cpp.lst:
        @$(CC) -Fc$*.lst $(SRCDIR)\$*.cpp

{$(SRCDIR)}.c.obj:
        @$(CC) $(SRCDIR)\$*.c

{$(SRCDIR)}.cpp.obj:
        @$(CC) $(SRCDIR)\$*.cpp

{$(SRCDIR)}.asm.obj:
        @$(ASM) $(SRCDIR)\$*.asm

!ifdef INFERPCH
$(PCH_C_SRC):
	echo #include "$(PRIVINC).h" > $(PCH_C_SRC)
!endif # INFERPCH

!ifdef INFERPCHCPP
$(PCH_CPP_SRC):
	echo #include "$(PRIVINC).h" > $(PCH_CPP_SRC)
!endif # INFERPCHCPP


#
# Rules for compiling modules
#
#   (Individual makefiles should have more dependencies if needed)
#

$(PRIVINC).pch pch.obj:	$(PCH_C_SRC) $(SRCDIR)\$(PRIVINC).h
!ifndef WIN32
        @$(CCH) -I$(SRCDIR) -NT _TEXT $(PCH_C_SRC)
!else
        @$(CCH) -I$(SRCDIR) $(FORCE_CPP) $(PCH_C_SRC)
!endif

pchcpps.pch pchcpps.obj:	$(PCH_CPP_SRC) $(SRCDIR)\$(PRIVINC).h
        @$(CCH) -I$(SRCDIR) -Fppchcpps.pch $(PCH_CPP_SRC)

!ifndef WIN32
!ifdef PCHOBJ0
$(PCHOBJ0):	$(PRIVINC).pch
!ifdef CODESEG0
        @$(CCU) -NT $(CODESEG0) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ1
$(PCHOBJ1):	$(PRIVINC).pch
!ifdef CODESEG1
        @$(CCU) -NT $(CODESEG1) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ2
$(PCHOBJ2):	$(PRIVINC).pch
!ifdef CODESEG2
        @$(CCU) -NT $(CODESEG2) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ3
$(PCHOBJ3):	$(PRIVINC).pch
!ifdef CODESEG3
        @$(CCU) -NT $(CODESEG3) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ4
$(PCHOBJ4):	$(PRIVINC).pch
!ifdef CODESEG4
        @$(CCU) -NT $(CODESEG4) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ5
$(PCHOBJ5):	$(PRIVINC).pch
!ifdef CODESEG5
        @$(CCU) -NT $(CODESEG5) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ6
$(PCHOBJ6):	$(PRIVINC).pch
!ifdef CODESEG6
        @$(CCU) -NT $(CODESEG6) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ7
$(PCHOBJ7):	$(PRIVINC).pch
!ifdef CODESEG7
        @$(CCU) -NT $(CODESEG7) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ8
$(PCHOBJ8):	$(PRIVINC).pch
!ifdef CODESEG8
        @$(CCU) -NT $(CODESEG8) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!ifdef PCHOBJ9
$(PCHOBJ9):	$(PRIVINC).pch
!ifdef CODESEG9
        @$(CCU) -NT $(CODESEG9) $(SRCDIR)\$*.c
!else
        @$(CCU) $(SRCDIR)\$*.c
!endif
!endif
!endif

!ifdef WIN32
!ifdef CPPOBJS
$(CPPOBJS):
        @$(CCX) $(SRCDIR)\$*.cpp
!endif # CPPOBJS
!ifdef CPPPCHOBJS
$(CPPPCHOBJS):	pchcpps.pch
        @$(CCU) -Fppchcpps.pch $(SRCDIR)\$*.cpp
!endif # CPPPCHOBJS

!ifdef PCHOBJ0
$(PCHOBJS):	$(PRIVINC).pch
        @$(CCU) $(FORCE_CPP) $(SRCDIR)\$*.c
!endif # PCHOBJ0
!endif # WIN32

#
# Compose the list of dependencies for the project
#

DEPENDS         = $(DEPENDS) $(OBJS) 

!ifndef BUILDLIB
DEPENDS         = $(DEPENDS) $(SRCDIR)\$(DEFNAME)
!endif

!if defined(WIN32) && !defined(BUILDLIB)
DEPENDS         = $(DEPENDS) $(RESNAME)
!endif

#
# Rule for building app or DLL (16-bit)
#

!ifndef WIN32
$(NAME).$(APPEXT)::	$(DEPENDS)
	$(LINK16) @<<
	$(LFLAGS) +
!ifdef FIRSTOBJS
        $(FIRSTOBJS) +
!endif
!ifdef THKOBJS
	$(THKOBJS) +
!endif
!ifdef MISCOBJ0
	$(MISCOBJ0) +
!endif
!ifdef MISCOBJ1
	$(MISCOBJ1) +
!endif
!ifdef MISCOBJ2
	$(MISCOBJ2) +
!endif
!ifdef MISCOBJ3
	$(MISCOBJ3) +
!endif
!ifdef MISCOBJ4
	$(MISCOBJ4) +
!endif
!ifdef MISCOBJ5
	$(MISCOBJ5) +
!endif
!ifdef MISCOBJ6
	$(MISCOBJ6) +
!endif
!ifdef MISCOBJ7
	$(MISCOBJ7) +
!endif
!ifdef MISCOBJ8
	$(MISCOBJ8) +
!endif
!ifdef MISCOBJ9
	$(MISCOBJ9) +
!endif
!ifdef PCHOBJ0
	$(PCHOBJ0) +
!endif
!ifdef PCHOBJ1
	$(PCHOBJ1) +
!endif
!ifdef PCHOBJ2
	$(PCHOBJ2) +
!endif
!ifdef PCHOBJ3
	$(PCHOBJ3) +
!endif
!ifdef PCHOBJ4
	$(PCHOBJ4) +
!endif
!ifdef PCHOBJ5
	$(PCHOBJ5) +
!endif
!ifdef PCHOBJ6
	$(PCHOBJ6) +
!endif
!ifdef PCHOBJ7
	$(PCHOBJ7) +
!endif
!ifdef PCHOBJ8
	$(PCHOBJ8) +
!endif
!ifdef PCHOBJ9
	$(PCHOBJ9) +
!endif
	
	$(NAME).$(APPEXT)
	$(NAME).map
!ifdef LIB0
	$(LIB0) +
!endif
!ifdef LIB1
	$(LIB1) +
!endif
!ifdef LIB2
	$(LIB2) +
!endif
!ifdef LIB3
	$(LIB3) +
!endif
!ifdef LIB4
	$(LIB4) +
!endif
!ifdef LIB5
	$(LIB5) +
!endif
!ifdef LIB6
	$(LIB6) +
!endif
!ifdef LIB7
	$(LIB7) +
!endif
!ifdef LIB8
	$(LIB8) +
!endif
!ifdef LIB9
	$(LIB9) +
!endif
	
	$(SRCDIR)\$(DEFNAME)
<<
	$(MAPSYM) $(NAME).map
!ifdef BUILDDLL #[
!ifdef MKPUBLIC #[
# Use the stripped def file to produce the lib.
	mkpublic $(SRCDIR)\$(DEFNAME) $(NAME).rdf
	$(IMPLIB) $(NAME).lib $(NAME).rdf
!else #][
# Use the normal def file to produce the lib.
	$(IMPLIB) $(NAME).lib $(SRCDIR)\$(DEFNAME)
!endif #]
!endif #]

!endif


!ifdef    WIN32

#
# Tell the world we're building Nashville bits
#
!if defined(FEATURE_IE40)
!message
!message FEATURE_IE40 is turned on
!message
!endif

#
# Rule for building static library
#

!if defined(BUILDLIB)

$(NAME).$(APPEXT):      $(DEPENDS)
	$(LINK32) -lib $(LEGOLIBS) @<<
-out:$(NAME).$(APPEXT)
$(MISCOBJS) $(PCHOBJS) $(CPPOBJS) $(CPPPCHOBJS)
<<

!endif      # BUILDLIB


#
# Rule for building lib, derived from DLL
#

!if defined(BUILDDLL)

$(NAME).lib $(NAME).rxp:	$(SRCDIR)\$(DEFNAME) $(STATOBJS)
	$(LINK32) -lib $(LEGO_LIBFLAGS) $(LEGOLIBS) @<<
-out:$(NAME).lib
-def:$(SRCDIR)\$(DEFNAME)
$(MISCOBJS) $(PCHOBJS) $(CPPOBJS) $(CPPPCHOBJS)
<<
!ifdef STATOBJ0
	$(LINK32) -lib $(LEGOLIBS) $(NAME).lib $(STATOBJS)
!endif      # STATOBJ0
	if exist $(NAME).rxp del $(NAME).rxp
	ren $(NAME).exp $(NAME).rxp
!endif      # BUILDDLL 

#
# Rule for building DLL or EXE
#

!if !defined(BUILDLIB)

!ifdef        BUILDDLL
$(NAME).$(APPEXT)::	$(DEPENDS) $(NAME).rxp $(NAME).lib
!else       # BUILDDLL
$(NAME).$(APPEXT)::	$(DEPENDS)
!endif      # BUILDDLL
	$(LINK32) -link @<<
$(L32FLAGS)
-out:$(NAME).$(APPEXT)
!ifndef    DOILINK
-map:$(NAME).map
!endif   # DOILINK
!ifdef     BUILDDLL
-dll
!ifdef         DLLBASE
-base:$(DLLBASE)
!else        # DLLBASE
-base:0x410000
!endif 	     # DLLBASE
$(NAME).rxp
!else   # BUILDDLL
-base:0x400000
!endif  # BUILDDLL
$(MISCOBJS) $(PCHOBJS) $(CPPOBJS) $(CPPPCHOBJS)
$(LIBS)
$(RESNAME)
<<
!ifdef     DOILINK
	pdbmap $(NAME).$(APPEXT)
!endif   # DOILINK
	$(MAPSYM) -s $(NAME).map

!ifdef     MKPUBLIC
$(NAME).$(APPEXT)::$(NAME).rlb

$(NAME).rlb : $(DEPENDS) $(SRCDIR)\$(DEFNAME) $(STATOBJS)
	mkpublic $(SRCDIR)\$(DEFNAME) $(NAME).rdf
	$(LINK32) -lib $(LEGOLIBS) @<<
-out:$(NAME).rlb
-def:$(NAME).rdf
$(MISCOBJS) $(PCHOBJS) $(CPPOBJS) $(CPPPCHOBJS)
<<
!ifdef STATOBJ0
	$(LINK32) -lib $(LEGOLIBS) @<<
-out:$(NAME).rlb
$(NAME).rlb $(STATOBJS)
<<
!endif      # STATOBJ0
!endif   # MKPUBLIC
!endif  # BUILDLIB

!endif # WIN32


#
# Rule for building thunks
#

!ifndef WIN32 #[
# // If there are any THKOBJS then build them here.
# // REVIEW - HACK You'd be better off not working out what the next line does :-)
!if "$(THKOBJS)" != "          " #[
# // Include files inserted by the thunk compiler for the 16-bit half.
THUNKDIR=$(ROOT)\shell\thunk

THUNKINCS16=$(THUNKDIR)\thk.inc \
	    $(THUNKDIR)\winerror.inc \
	    $(THUNKDIR)\win31err.inc

$(THKOBJS): $(THUNKDIR)\$(VERDIR)\$(@B).asm $(THUNKDIR)\$(@B).inc \
$(THUNKDIR)\fltthk.inc
    @set OLDML=%ML%
    @set OLDINCLUDE=%INCLUDE%
    @set ML=-DIS_16 -nologo -W2 -Zd -c -Cx -DMASM6 -DDEBLEVEL=1 $(DDEBUG) -Zd -Gc
    @set INCLUDE=$(THUNKDIR);$(INCLUDE)
    @mlx -Fo$@ $(THUNKDIR)\$(VERDIR)\$(@B).asm
    @set ML=%OLDML%
    @set INCLUDE=%OLDINCLUDE%
!endif #]
!endif #]

#
# Rule for binding resources to Win16 app or DLL
#

!ifndef WIN32
$(NAME).$(APPEXT)::	$(DEPENDS) $(RESNAME)
	$(RC) $(RCFLAGS) $(RCFFLAGS) -fe $(NAME).$(APPEXT) $(RESNAME)
!endif

#
# Include source file dependencies.
#

!if exist($(SRCDIR)\depend.mk)
!include $(SRCDIR)\depend.mk
!elseif "$(BUILD)" != "depend"
!message Warning: DEPEND.MK not found.
!endif


#
# Build source file dependencies.
#
#  If nmake complains it doesn't know how to make depend.mk, you
#  need to add a rule in your makefile that explains how to make
#  it.
#

!if "$(BUILD)" != "depend"
depend:
        $(MAKE) BUILD=depend
!else
depend: $(SRCDIR)\depend.mk
!endif

!endif ## VERDIR
