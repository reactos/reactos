/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndissys.h
 * PURPOSE:     NDIS library definitions
 * NOTES:       Spin lock acquire order:
 *                - Miniport list lock
 *                - Adapter list lock
 */
#ifndef __NDISSYS_H
#define __NDISSYS_H

#define NDIS50 1    /* Use NDIS 5.0 structures by default */

#include <ddk/ntddk.h>
#include <ddk/ndis.h>

#include <debug.h>

#ifdef DBG

#define DEBUG_REFCHECK(Object) {            \
    if ((Object)->RefCount <= 0) {          \
        NDIS_DbgPrint(MIN_TRACE, ("Object at (0x%X) has invalid reference count (%d).\n", \
            (Object), (Object)->RefCount)); \
        }                                   \
}

#else

#define DEBUG_REFCHECK(Object)

#endif


/*
 * VOID ReferenceObject(
 *     PVOID Object)
 */
#define ReferenceObject(Object)                  \
{                                                \
    DEBUG_REFCHECK(Object);                      \
    NDIS_DbgPrint(DEBUG_REFCOUNT, ("Referencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));          \
                                                 \
    InterlockedIncrement(&((Object)->RefCount)); \
}

/*
 * VOID DereferenceObject(
 *     PVOID Object)
 */
#define DereferenceObject(Object)                         \
{                                                         \
    DEBUG_REFCHECK(Object);                               \
    NDIS_DbgPrint(DEBUG_REFCOUNT, ("Dereferencing object at (0x%X). RefCount (%d).\n", \
        (Object), (Object)->RefCount));                   \
                                                          \
    if (InterlockedDecrement(&((Object)->RefCount)) == 0) \
        PoolFreeBuffer(Object);                           \
}


#define MIN(value1, value2) \
    ((value1 < value2)? value1 : value2)

#define MAX(value1, value2) \
    ((value1 > value2)? value1 : value2)

#endif /* __NDISSYS_H */

/* EOF */
