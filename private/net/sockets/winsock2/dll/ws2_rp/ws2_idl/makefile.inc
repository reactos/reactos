#------------------------------------------------------------------------
#  Copyright (c)1997 Microsoft Corporation, All Rights Reserved.
#
#  Makefile.inc
#
#------------------------------------------------------------------------

WS2_HEADER=..\..\include\ws2_if.h

all:   domidl

clean: delsources domidl

delsources:
    del ws2_if.h ws2_if_p.c ws2_if_i.c dlldata.c

domidl: ws2_if_p.c

ws2_if_p.c ws2_if_i.c dlldata.c : ws2_if.idl
    midl -ms_ext -app_config -c_ext -header $(WS2_HEADER) -I $(_NTBINDIR)\public\sdk\inc -I $(_NTBINDIR)\public\sdk\inc\crt ws2_if.idl



