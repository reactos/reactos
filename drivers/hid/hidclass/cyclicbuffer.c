/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Ring buffer
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "precomp.h"

NTSTATUS HidClass_CyclicBufferUpdateSize(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN UINT32 elementCount)
{
    buffer->elementCount = elementCount + 1;
    if (buffer->buffer != NULL)
    {
        buffer->endIndex = 0;
        buffer->startIndex = 0;
        ExFreePool(buffer->buffer);
        buffer->buffer = NULL;
    }
    buffer->buffer = ExAllocatePoolWithTag(NonPagedPool, buffer->elementSize * buffer->elementCount, HIDCLASS_TAG);

    // FIXME: HANDLE ERROR
    return STATUS_SUCCESS;
}

void HidClass_CyclicBufferInitialize(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN UINT32 elementSize)
{
    buffer->elementSize = elementSize;
    buffer->endIndex = 0;
    buffer->startIndex = 0;
    buffer->buffer = NULL;

    HidClass_CyclicBufferUpdateSize(buffer, 16);
}

void HidClass_CyclicBufferPut(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN PVOID item)
{
    PUCHAR currentItem;

    currentItem = buffer->buffer;
    currentItem += buffer->elementSize * buffer->endIndex;
    RtlCopyMemory(currentItem, item, buffer->elementSize);
    buffer->endIndex = (buffer->endIndex + 1) % buffer->elementCount;
    if (buffer->endIndex == buffer->startIndex)
    {
        buffer->startIndex = (buffer->startIndex + 1) % buffer->elementCount;
    }
}

BOOLEAN HidClass_CyclicBufferIsEmpty(IN PHIDCHASS_CYCLIC_BUFFER buffer)
{
    return buffer->startIndex == buffer->endIndex;
}

BOOLEAN HidClass_CyclicBufferGet(IN PHIDCHASS_CYCLIC_BUFFER buffer, OUT PVOID item)
{
    PUCHAR currentItem;
    BOOLEAN result = FALSE;

    if (buffer->startIndex != buffer->endIndex)
    {
        currentItem = buffer->buffer;
        currentItem += buffer->elementSize * buffer->startIndex;
        RtlCopyMemory(item, currentItem, buffer->elementSize);
        buffer->startIndex = (buffer->startIndex + 1) % buffer->elementCount;
        result = TRUE;
    }

    return result;
}
