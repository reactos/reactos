/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/circularbuffer.c
 * PURPOSE:         Operations on a circular buffer
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

//#define NDEBUG
#include "serial.h"

NTSTATUS
InitializeCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer,
	IN ULONG BufferSize)
{
	pBuffer->Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize * sizeof(UCHAR), SERIAL_TAG);
	if (!pBuffer->Buffer)
		return STATUS_INSUFFICIENT_RESOURCES;
	pBuffer->Length = BufferSize;
	pBuffer->ReadPosition = pBuffer->WritePosition = 0;
	return STATUS_SUCCESS;
}

NTSTATUS
FreeCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer)
{
	ExFreePoolWithTag(pBuffer->Buffer, SERIAL_TAG);
	return STATUS_SUCCESS;
}

BOOLEAN
IsCircularBufferEmpty(
	IN PCIRCULAR_BUFFER pBuffer)
{
	return (pBuffer->ReadPosition == pBuffer->WritePosition);
}

NTSTATUS
PushCircularBufferEntry(
	IN PCIRCULAR_BUFFER pBuffer,
	IN UCHAR Entry)
{
	ASSERT(pBuffer->Length);
	ULONG NextPosition = (pBuffer->WritePosition + 1) % pBuffer->Length;
	if (NextPosition == pBuffer->ReadPosition)
		return STATUS_BUFFER_TOO_SMALL;
	pBuffer->Buffer[pBuffer->WritePosition] = Entry;
	pBuffer->WritePosition = NextPosition;
	return STATUS_SUCCESS;
}

NTSTATUS
PopCircularBufferEntry(
	IN PCIRCULAR_BUFFER pBuffer,
	OUT PUCHAR Entry)
{
	ASSERT(pBuffer->Length);
	if (IsCircularBufferEmpty(pBuffer))
		return STATUS_ARRAY_BOUNDS_EXCEEDED;
	*Entry = pBuffer->Buffer[pBuffer->ReadPosition];
	pBuffer->ReadPosition = (pBuffer->ReadPosition + 1) % pBuffer->Length;
	return STATUS_SUCCESS;
}
