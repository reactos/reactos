/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        include/buffer.h
 * PURPOSE:     Buffer management routine definitions
 */
#ifndef __BUFFER_H
#define __BUFFER_H

#include <ndissys.h>


/* FIXME: Possibly move this to ntddk.h */
typedef struct _NETWORK_HEADER
{
    MDL Mdl;                                /* Memory Descriptor List */
    struct _NETWORK_HEADER *Next;           /* Link to next NDIS buffer in pool */
    struct _NDIS_BUFFER_POOL *BufferPool;   /* Link to NDIS buffer pool */
} NETWORK_HEADER, *PNETWORK_HEADER;

typedef struct _NDIS_BUFFER_POOL
{
    KSPIN_LOCK SpinLock;
    PNETWORK_HEADER FreeList;
    NETWORK_HEADER Buffers[0];
} NDIS_BUFFER_POOL, *PNDIS_BUFFER_POOL;

#endif /* __BUFFER_H */

/* EOF */
