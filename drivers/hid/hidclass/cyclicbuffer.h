#pragma once
#include "wdm.h"

typedef struct _HIDCHASS_CYCLIC_BUFFER
{
    UINT32 elementCount;
    UINT32 elementSize;
    UINT32 startIndex;
    UINT32 endIndex;

    PVOID buffer;
} HIDCHASS_CYCLIC_BUFFER, *PHIDCHASS_CYCLIC_BUFFER;

NTSTATUS HidClass_CyclicBufferUpdateSize(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN UINT32 elementCount);

void HidClass_CyclicBufferInitialize(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN UINT32 elementSize);

void HidClass_CyclicBufferPut(IN PHIDCHASS_CYCLIC_BUFFER buffer, IN PVOID item);

BOOLEAN HidClass_CyclicBufferIsEmpty(IN PHIDCHASS_CYCLIC_BUFFER buffer);

BOOLEAN HidClass_CyclicBufferGet(IN PHIDCHASS_CYCLIC_BUFFER buffer, OUT PVOID item);
