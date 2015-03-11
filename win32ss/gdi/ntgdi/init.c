/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Initialization of GDI
 * FILE:             win32ss/gdi/ntgdi/init.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>
#include <kdros.h>

BOOL NTAPI GDI_CleanupForProcess(struct _EPROCESS *Process);

NTSTATUS
GdiProcessCreate(PEPROCESS Process)
{
    PPROCESSINFO ppiCurrent = PsGetProcessWin32Process(Process);
    ASSERT(ppiCurrent);

    InitializeListHead(&ppiCurrent->PrivateFontListHead);
    ExInitializeFastMutex(&ppiCurrent->PrivateFontListLock);

    InitializeListHead(&ppiCurrent->GDIBrushAttrFreeList);
    InitializeListHead(&ppiCurrent->GDIDcAttrFreeList);

    /* Map the GDI handle table to user land */
    Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(Process);
    Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;

    /* Create pools for GDI object attributes */
    ppiCurrent->pPoolDcAttr = GdiPoolCreate(sizeof(DC_ATTR), 'acdG');
    ppiCurrent->pPoolBrushAttr = GdiPoolCreate(sizeof(BRUSH_ATTR), 'arbG');
    ppiCurrent->pPoolRgnAttr = GdiPoolCreate(sizeof(RGN_ATTR), 'agrG');
    ASSERT(ppiCurrent->pPoolDcAttr);
    ASSERT(ppiCurrent->pPoolBrushAttr);
    ASSERT(ppiCurrent->pPoolRgnAttr);

    return STATUS_SUCCESS;
}

NTSTATUS
GdiProcessDestroy(PEPROCESS Process)
{
    PPROCESSINFO ppiCurrent = PsGetProcessWin32Process(Process);
    ASSERT(ppiCurrent);
    ASSERT(ppiCurrent->peProcess == Process);

    /* And GDI ones too */
    GDI_CleanupForProcess(Process);

    /* So we can now free the pools */
    GdiPoolDestroy(ppiCurrent->pPoolDcAttr);
    GdiPoolDestroy(ppiCurrent->pPoolBrushAttr);
    GdiPoolDestroy(ppiCurrent->pPoolRgnAttr);

    return STATUS_SUCCESS;
}


NTSTATUS
GdiThreadCreate(PETHREAD Thread)
{
    return STATUS_SUCCESS;
}

NTSTATUS
GdiThreadDestroy(PETHREAD Thread)
{
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtGdiInit(VOID)
{
    return TRUE;
}

/* EOF */
