#  File: D:\WACKER\htrn_jis\makefile.t (Created: 24-Aug-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.7 $
#  $Date: 1998/02/09 11:54:48 $
#

.PATH.c=.;\wacker\htrn_jis;

MKMF_SRCS       =       htrn_jis.c

HDRS            =

EXTHDRS     =

SRCS        =

OBJS        =

#-------------------#

RCSFILES =      \wacker\htrn_jis\htrn_jis.rc    \
			\wacker\htrn_jis\htrn_jis.def   \
			\wacker\htrn_jis\htrn_jis.c             \
			\wacker\htrn_jis\htrn_jis.h             \
			\wacker\htrn_jis\htrn_jis.hh    \
			\wacker\term\ver_jis.rc                 \
			\wacker\htrn_jis\makefile.t

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : htrn_jis.dll

#-------------------#

CFLAGS += /Fd$(BD)\htrn_jis

!if defined(USE_BROWSER) && $(VERSION) == WIN_DEBUG
CFLAGS += /Fr$(BD)/
!endif

%if defined(MAP_AND_SYMBOLS)
TARGETS : htrn_jis.sym
%endif

%if $(VERSION) == WIN_DEBUG
LFLAGS = $(LDEBUGEXT) /DLL /entry:transJisEntry $(BD)\hypertrm.lib \
	 user32.lib $(**,M\.res) /PDB:$(BD)\htrn_jis $(MAP_OPTIONS) \
	 
%else
LFLAGS += /DLL /entry:transJisEntry $(BD)\hypertrm.lib user32.lib \
	 $(**,M\.res) /PDB:$(BD)\htrn_jis
%endif

#-------------------#

htrn_jis.dll + htrn_jis.exp + htrn_jis.lib : $(OBJS) htrn_jis.def htrn_jis.res
    @echo Linking $(@,F) ...
    @link $(LFLAGS) $(OBJS:X) /DEF:htrn_jis.def -out:$(@,M\.dll)
    @(cd $(BD) $; bind $(@,M.dll))

htrn_jis.res : htrn_jis.rc \wacker\term\ver_jis.rc \wacker\term\version.h
	rc -r /D$(BLD_VER) /DWIN32 -i\wacker -fo$@        \
	    \wacker\htrn_jis\htrn_jis.rc

htrn_jis.sym : htrn_jis.map
	mapsym -o $@ $**

#-------------------#
