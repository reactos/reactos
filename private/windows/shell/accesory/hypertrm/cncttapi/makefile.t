#  File: D:\WACKER\cncttapi\makefile.t (Created: 08-Feb-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.6 $
#  $Date: 1997/09/09 15:24:58 $
#

MKMF_SRCS		= cncttapi.c tapidlgs.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES =  \wacker\cncttapi\makefile.t \
            \wacker\cncttapi\cncttapi.def \
            \wacker\cncttapi\cncttapi.rc \
            $(SRCS) \
            $(HDRS) \

NOTUSED  = \wacker\cncttapi\cnctdrv.hh \

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : cncttapi.dll

#-------------------#

LFLAGS += -DLL -entry:cnctdrvEntry tapi32.lib $(**,M\.exp) $(**,M\.lib)

#-------------------#

cncttapi.dll : $(OBJS) cncttapi.def cncttapi.res cncttapi.exp tdll.lib \
					   comstd.lib
	link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

cncttapi.res : cncttapi.rc
	rc -r -i\wacker -fo$@ cncttapi.rc

#-------------------#
