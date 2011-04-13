#pragma once

#include "hardware.h"

/* destroys memory allocator */
VOID
NTAPI
DmaMemAllocator_Destroy(
    IN LPDMA_MEMORY_ALLOCATOR Allocator);

/* create memory allocator */
NTSTATUS
NTAPI
DmaMemAllocator_Create(
    IN LPDMA_MEMORY_ALLOCATOR *OutMemoryAllocator);

/* initializes memory allocator */
NTSTATUS
NTAPI
DmaMemAllocator_Initialize(
    IN OUT LPDMA_MEMORY_ALLOCATOR Allocator,
    IN ULONG DefaultBlockSize,
    IN PKSPIN_LOCK Lock,
    IN PHYSICAL_ADDRESS PhysicalBase,
    IN PVOID VirtualBase,
    IN ULONG Length);

/* allocates memory from allocator */
NTSTATUS
NTAPI
DmaMemAllocator_Allocate(
    IN LPDMA_MEMORY_ALLOCATOR Allocator,
    IN ULONG Size,
    OUT PVOID *OutVirtualAddress,
    OUT PPHYSICAL_ADDRESS OutPhysicalAddress);

/* frees memory block from allocator */
VOID
NTAPI
DmaMemAllocator_Free(
    IN LPDMA_MEMORY_ALLOCATOR Allocator,
    IN PVOID VirtualAddress,
    IN ULONG Size);
