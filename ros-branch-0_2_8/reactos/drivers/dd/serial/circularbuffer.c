/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/circularbuffer.c
 * PURPOSE:         Operations on a circular buffer
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "serial.h"

NTSTATUS
InitializeCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer,
	IN ULONG BufferSize)
{
	DPRINT("Serial: InitializeCircularBuffer(pBuffer %p, BufferSize %lu)\n", pBuffer, BufferSize);
	ASSERT(pBuffer);
	pBuffer->Buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, BufferSize * sizeof(UCHAR), SERIAL_TAG);
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
	DPRINT("Serial: FreeCircularBuffer(pBuffer %p)\n", pBuffer);
	ASSERT(pBuffer);
	if (pBuffer->Buffer != NULL)
		ExFreePoolWithTag(pBuffer->Buffer, SERIAL_TAG);
	return STATUS_SUCCESS;
}

BOOLEAN
IsCircularBufferEmpty(
	IN PCIRCULAR_BUFFER pBuffer)
{
	DPRINT("Serial: IsCircularBufferEmpty(pBuffer %p)\n", pBuffer);
	ASSERT(pBuffer);
	return (pBuffer->ReadPosition == pBuffer->WritePosition);
}

NTSTATUS
PushCircularBufferEntry(
	IN PCIRCULAR_BUFFER pBuffer,
	IN UCHAR Entry)
{
	ULONG NextPosition;
	DPRINT("Serial: PushCircularBufferEntry(pBuffer %p, Entry 0x%x)\n", pBuffer, Entry);
	ASSERT(pBuffer);
	ASSERT(pBuffer->Length);
	NextPosition = (pBuffer->WritePosition + 1) % pBuffer->Length;
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
	DPRINT("Serial: PopCircularBufferEntry(pBuffer %p)\n", pBuffer);
	ASSERT(pBuffer);
	ASSERT(pBuffer->Length);
	if (IsCircularBufferEmpty(pBuffer))
		return STATUS_ARRAY_BOUNDS_EXCEEDED;
	*Entry = pBuffer->Buffer[pBuffer->ReadPosition];
	pBuffer->ReadPosition = (pBuffer->ReadPosition + 1) % pBuffer->Length;
	return STATUS_SUCCESS;
}

NTSTATUS
IncreaseCircularBufferSize(
	IN PCIRCULAR_BUFFER pBuffer,
	IN ULONG NewBufferSize)
{
	PUCHAR NewBuffer;

	DPRINT("Serial: IncreaseCircularBufferSize(pBuffer %p, NewBufferSize %lu)\n", pBuffer, NewBufferSize);
	ASSERT(pBuffer);
	ASSERT(pBuffer->Length);
	if (pBuffer->Length > NewBufferSize)
		return STATUS_INVALID_PARAMETER;
	else if (pBuffer->Length == NewBufferSize)
		return STATUS_SUCCESS;

	NewBuffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, NewBufferSize * sizeof(UCHAR), SERIAL_TAG);
	if (!NewBuffer)
		return STATUS_INSUFFICIENT_RESOURCES;
	RtlCopyMemory(NewBuffer, pBuffer->Buffer, pBuffer->Length * sizeof(UCHAR));
	ExFreePoolWithTag(pBuffer->Buffer, SERIAL_TAG);
	pBuffer->Buffer = NewBuffer;
	pBuffer->Length = NewBufferSize;
	return STATUS_SUCCESS;
}
