#  File: D:\WACKER\ext\makefile.t (Created: 06-May-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.7 $
#  $Date: 1998/02/09 11:54:50 $
#

MKMF_SRCS   = iconext.c     defclsf.c

HDRS        =

EXTHDRS     =

SRCS        =

OBJS        =

#-------------------#

RCSFILES =      \wacker\ext\hticons.rc   \wacker\ext\hticons.def \
		\wacker\ext\iconext.c    \wacker\ext\makefile.t  \
		\wacker\term\ver_ico.rc

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : hticons.dll

#-------------------#

CFLAGS += /Fd$(BD)\hticons

!if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
!endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : hticons.sym
%endif

LFLAGS += /DLL /entry:IconEntry $(**,M\.res) /PDB:$(BD)\hticons \
	  uuid.lib msvcrt.lib kernel32.lib /BASE:0x11000000

#-------------------#

hticons.dll + hticons.exp + hticons.lib : $(OBJS) hticons.def hticons.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:hticons.def -out:$(@,M\.dll)
    @(cd $(BD) $; bind $(@,M.dll))

hticons.res : hticons.rc \
		   \wacker\term\newcon.ico               \wacker\term\delphi.ico        \
		   \wacker\term\att.ico                  \wacker\term\dowjones.ico      \
		   \wacker\term\mci.ico                  \wacker\term\genie.ico         \
		   \wacker\term\compuser.ico     \wacker\term\gen01.ico         \
		   \wacker\term\gen02.ico                \wacker\term\gen03.ico         \
		   \wacker\term\gen04.ico                \wacker\term\gen05.ico         \
		   \wacker\term\gen06.ico                \wacker\term\gen07.ico         \
		   \wacker\term\gen08.ico                \wacker\term\gen09.ico         \
		   \wacker\term\gen10.ico                \wacker\term\ver_ico.rc        \
		   \wacker\term\version.h
	rc -r /D$(BLD_VER) /DWIN32 -i\wacker -fo$@        \
	    \wacker\ext\hticons.rc

hticons.sym : hticons.map
	mapsym -o $@ $**

#-------------------#
