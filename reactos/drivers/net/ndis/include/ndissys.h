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

#ifdef _MSC_VER
#include <basetsd.h>
#include <ntddk.h>
#include <windef.h>
#include <ndis.h>
#else /* _MSC_VER */
#include <ddk/ntddk.h>
#include <net/ndis.h>
#endif /* _MSC_VER */

#include <debug.h>


#ifndef _MSC_VER
/* FIXME: The following should be moved to ntddk.h */

/* i386 specific constants */

/* Page size for the Intel 386 is 4096 */
#define PAGE_SIZE (ULONG)0x1000

/* 4096 is 2^12. Used to find the virtual page number from a virtual address */
#define PAGE_SHIFT 12L

/*
 * ULONG ADDRESS_AND_SIZE_TO_SPAN_PAGES(
 *     IN  PVOID   Va,
 *     IN  ULONG   Size);
 */
#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Size)            \
    ((((ULONG)((ULONG)(Size) - 1) >> PAGE_SHIFT) +          \
    (((((ULONG)(Size - 1) & (PAGE_SIZE - 1)) +              \
    ((ULONG)Va & (PAGE_SIZE - 1)))) >> PAGE_SHIFT)) + 1)

/*
 * ULONG MmGetMdlByteCount(
 *     IN  PMDL    Mdl)
 */
#define MmGetMdlByteCount(Mdl)  \
    ((Mdl)->ByteCount)

#endif


/* Exported functions */
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT STDCALL
#endif


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

#endif /* __NDISSYS_H */

/* EOF */
