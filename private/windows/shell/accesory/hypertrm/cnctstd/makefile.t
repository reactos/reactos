#  File: D:\WACKER\cnctstd\makefile.t (Created: 19-Jan-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.4 $
#  $Date: 1997/09/09 15:24:59 $
#

# I need a naming convention to keep object files from colliding.
# The last letter of the filename designates the driver type.  For
# the hayes (standard) driver, we'll use 's'.  For TAPI, probably 't'.

MKMF_SRCS		= cnctdrvs.c  cncts.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES = \wacker\cnctstd\makefile.t \
            \wacker\cnctstd\cnctstd.def \
            $(SRCS) \
            $(HDRS) \

NOTUSED  =

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : cnctstd.dll

#-------------------#

LFLAGS += -DLL -entry:cnctdrvEntry $(**,M\.exp) $(**,M\.lib)

#-------------------#

cnctstd.dll : $(OBJS) cnctstd.def cnctstd.exp tdll.lib comstd.lib
	link $(LFLAGS) $(OBJS:X) -out:$@

#-------------------#
