#  File: D:\wacker\term\makefile.t (Created: 24-Nov-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.30 $
#  $Date: 1998/02/09 11:54:50 $
#

MKMF_SRCS       = \wacker\term\term.c

HDRS            =

EXTHDRS         =

SRCS            =

OBJS            =

#-------------------#

RCSFILES = \wacker\term\makefile.t              \wacker\term\online.rc  \
		   \wacker\term\ver_exe.rc              \wacker\term\ver_dll.rc \
		   \wacker\term\version.h               \
		   $(SRCS) $(EXTHDRS)

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : \wacker\term\ver_exe.i hypertrm.exe

%if defined(MAP_AND_SYMBOLS)
TARGETS : h.sym
%endif

LFLAGS += msvcrt.lib

#-------------------#

# Run ver_exe.rc through C-preprocessor.
#
\wacker\term\ver_exe.i : \wacker\term\ver_exe.rc
	cl /nologo /P /D${VERSION} /Tc\wacker\term\ver_exe.rc

hypertrm.exe : $(OBJS) hypertrm.lib online.res
    # The incremental linker uses the base name of the target to build an
    # .ILK file.  Our DLL has the same name as our executable so build it
    # H.EXE and then rename it.
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) $(**,M\.lib) $(**,M\.res) -out:$(BD)\h.exe
    @-copy $(BD)\h.exe $@
    @(cd $(BD) $; bind $@)

online.res : online.rc \wacker\term\ver_exe.rc
    *rc -r /D$(BLD_VER) /DWIN32 -i\wacker -fo$@ \wacker\term\online.rc

h.sym : h.map
	mapsym -o $@ $**
