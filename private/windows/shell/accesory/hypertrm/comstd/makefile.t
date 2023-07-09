#  File: D:\WACKER\comstd\makefile.t (Created: 08-Dec-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.5 $
#  $Date: 1997/09/09 15:25:04 $
#

MKMF_SRCS       = \wacker\comstd\comstd.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES = \wacker\comstd\makefile.t \
            \wacker\comstd\comstd.rc \
            \wacker\comstd\comstd.def \
            $(SRCS) \
            $(HDRS) \

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : comstd.dll

#-------------------#

LFLAGS += -DLL -entry:ComStdEntry $(**,M\.exp) $(**,M\.lib)

comstd.dll : $(OBJS) comstd.def comstd.res comstd.exp tdll.lib
	link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

comstd.res : comstd.rc
	rc -r -i\wacker -fo$@ comstd.rc

#-------------------#
