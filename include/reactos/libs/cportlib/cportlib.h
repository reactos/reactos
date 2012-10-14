/*
 * PROJECT:         ReactOS ComPort Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/lib/cportlib/cportlib.h
 * PURPOSE:         Header for the ComPort Library
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntdef.h>

#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

#define CPPORT_FLAG_MODEM_CONTROL	0x02
typedef struct _CPPORT
{
	PUCHAR Address;
	ULONG Baud;
	USHORT Flags;
} CPPORT, *PCPPORT;
	
VOID
NTAPI
CpInitialize(
	IN PCPPORT Port,
	IN PUCHAR Address,
	IN ULONG Rate
	);

VOID
NTAPI
CpEnableFifo(
	IN PUCHAR Address,
	IN BOOLEAN Enable
	);

BOOLEAN
NTAPI
CpDoesPortExist(
	IN PUCHAR Address
	);
	
UCHAR
NTAPI
CpReadLsr(
	IN PCPPORT Port,
	IN UCHAR ExpectedValue
	);

VOID
NTAPI
CpSetBaud(
	IN PCPPORT Port,
	IN ULONG Rate
	);

USHORT
NTAPI
CpGetByte(
	IN PCPPORT Port,
	IN PUCHAR Byte,
	IN BOOLEAN Wait,
	IN BOOLEAN Poll
	);
	
VOID
NTAPI
CpPutByte(
	IN PCPPORT Port,
	IN UCHAR Byte
	);
