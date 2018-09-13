#  File: D:\WACKER\xfer\makefile.t (Created: 08-Dec-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 1.18 $
#  $Date: 1997/09/09 15:25:07 $
#

MKMF_SRCS		= xferdll.c \
				  xfr_todo.c \
				  xfr_srvc.c \
				  xfr_dsp.c \
				  x_entry.c \
				  x_params.c \
				  cmprs0.c \
				  cmprs1.c \
				  cmprs2.c \
				  itime.c \
				  foo.c \
				  zmdm.c \
				  zmdm_snd.c \
				  zmdm_rcv.c \
				  hpr.c \
				  hpr_sd.c \
				  hpr_res.c \
				  hpr_rcv0.c \
				  hpr_rcv1.c \
				  hpr_snd0.c \
				  hpr_snd1.c \
				  mdmx.c \
				  mdmx_sd.c \
				  mdmx_res.c \
				  mdmx_crc.c \
				  mdmx_rcv.c \
				  mdmx_snd.c \
				  krm.c \
				  krm_res.c \
				  krm_rcv.c \
				  krm_snd.c \
				  x_xy_dlg.c \
				  x_zm_dlg.c \
				  x_hp_dlg.c \
				  x_kr_dlg.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES = \wacker\xfer\makefile.t \
            \wacker\xfer\xferdll.def \
            $(SRCS) \
            $(HDRS) \

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : xferdll.dll

#-------------------#

LFLAGS += -DLL -entry:XferEntry comctl32.lib \
			$(**,M\.exp) $(**,M\.lib)

xferdll.dll : $(OBJS) xferdll.def xferdll.exp tdll.lib
	link $(LFLAGS) $(OBJS:X) -out:$@

#-------------------#
