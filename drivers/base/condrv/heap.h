/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver
 * FILE:            drivers/base/condrv/heap.h
 * PURPOSE:         Heap Helpers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

#define ConDrvAllocPoolNonPageable(Flags, Size, Tag)   \
    __pragma(message("WARNING - Flags parameter ignored. You may encounter problems!")) \
    ExAllocatePoolWithTag(NonPagedPool, Size, Tag)

#define ConDrvAllocPoolPageable(Flags, Size, Tag)  \
    __pragma(message("WARNING - Flags parameter ignored. You may encounter problems!")) \
    ExAllocatePoolWithTag(PagedPool, Size, Tag)

#define ConDrvFreePool(PoolBase, Tag)   \
    ExFreePoolWithTag(PoolBase, Tag)

/* EOF */
