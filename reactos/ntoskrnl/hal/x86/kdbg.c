/* $Id: kdbg.c,v 1.1 1999/09/05 20:54:57 ea Exp $
 *
 * reactos/ntoskrnl/hal/x86/kdbg.c
 *
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>

#include <internal/debug.h>


/* HAL.KdPortInitialize */
BOOLEAN
STDCALL
KdPortInitialize (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return FALSE;
}


/* HAL.KdPortGetByte */
BYTE
STDCALL
KdPortGetByte (
	VOID
	)
{
	return (BYTE) 0;
}


/* HAL.KdPortPollByte */
BYTE
STDCALL
KdPortPollByte (
	VOID
	)
{
	return  (BYTE) 0;
}

/* HAL.KdPortPutByte */
VOID
STDCALL
KdPortPutByte (
	BYTE	ByteToSend
	)
{
}


/* HAL.KdPortRestore */
VOID
STDCALL
KdPortRestore (
	VOID
	)
{
}


/* HAL.KdPortSave */
VOID
STDCALL
KdPortSave (
	VOID
	)
{
}


/* EOF */
