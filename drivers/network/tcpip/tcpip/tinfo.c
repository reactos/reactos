/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/tinfo.c
 * PURPOSE:     Transport layer information
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

TDI_STATUS InfoTransportLayerTdiQueryEx( UINT InfoClass,
					 UINT InfoType,
					 UINT InfoId,
					 PVOID Context,
					 TDIEntityID *id,
					 PNDIS_BUFFER Buffer,
					 PUINT BufferSize ) {
    if( InfoClass == INFO_CLASS_GENERIC &&
	InfoType == INFO_TYPE_PROVIDER &&
	InfoId == ENTITY_TYPE_ID ) {
	ULONG Temp = CL_TL_UDP;
	return InfoCopyOut( (PCHAR)&Temp, sizeof(Temp), Buffer, BufferSize );
    }

    return TDI_INVALID_REQUEST;
}

TDI_STATUS InfoTransportLayerTdiSetEx( UINT InfoClass,
				       UINT InfoType,
				       UINT InfoId,
				       PVOID Context,
				       TDIEntityID *id,
				       PCHAR Buffer,
				       UINT BufferSize ) {
    return TDI_INVALID_REQUEST;
}
