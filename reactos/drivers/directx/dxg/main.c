
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

#include <ntddk.h>
#include <dxg_int.h>


NTSTATUS
DriverEntry(IN PVOID Context1,
            IN PVOID Context2)
{
    return 0;
}

NTSTATUS
APIENTRY
DxDdStartupDxGraphics (ULONG SizeEngDrv,
                       PDRVENABLEDATA pDxEngDrv,
                       ULONG SizeDxgDrv,
                       PDRVENABLEDATA pDxgDrv,
                       PULONG DirectDrawContext,
                       PEPROCESS Proc )
{
#if 0
    PDRVFN drv_func;

    /* Test see if the data is vaild we got from win32k.sys */
    if (size_EngDrv != sizeof(DRVENABLEDATA)) ||
       (size_DXG_INDEX_API != sizeof(DRVENABLEDATA))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* rest static value */
    gpDummyPage = 0;
    gcDummyPageRefCnt = 0;
    ghsemDummyPage = 0;

    /* 
     * Setup internal driver functions list we got from dxg driver functions list
     */
    pDxgDrv->iDriverVersion = 0x80000; /* Note 12/1-2004 : DirectX 8 ? */
    pDxgDrv->c = gcDxgFuncs;
    pDxgDrv->pdrvfn = gaDxgFuncs;

    /* check how many driver functions and fail if the value does not match */
    if (pDxEngDrv->c !=  DXENG_INDEX_DxEngLoadImage + 1)
    {
        return STATUS_INTERNAL_ERROR;
    }

    /*
     * Check if all drv functions are sorted right 
     * and if it really are exported 
     */

    for (i=1 ; i < DXENG_INDEX_DxEngLoadImage + 1; i++)
    {
        drv_func = &EngDrv->pdrvfn[i];

        if ((drv_func->iFunc != i) ||
            (drv_func->pfn == 0))
        {
            return STATUS_INTERNAL_ERROR;
        }
    }

    gpEngFuncs = pDxEngDrv->pdrvfn;

    /* Note 12/1-2004 : Why is this set to 0x618 */
    *DirectDrawContext = 0x618;

    if (DdHmgCreate())
    {
        ghsemDummyPage = EngCreateSemaphore();

        if (ghsemDummyPage)
        {
            gpepSession = Proc;
            return STATUS_SUCCESS;
        }
    }

    DdHmgDestroy();

    if (ghsemDummyPage)
    {
        EngDeleteSemaphore(ghsemDummyPage);
        ghsemDummyPage = 0;
    }
#endif
    return STATUS_NO_MEMORY;
}

NTSTATUS
DxDdCleanupDxGraphics()
{
#if 0
    DdHmgDestroy();

    if (!ghsemDummyPage)
    {
        if (!gpDummyPage)
        {
            ExFreePoolWithTag(gpDummyPage,0);
            gpDummyPage = 0;
            gcDummyPageRefCnt = 0;
        }
        EngDeleteSemaphore(ghsemDummyPage)
    }
#endif
    return 0;
}

BOOL
DdHmgDestroy()
{
#if 0
    ghFreeDdHmgr = 0;
    gcMaxDdHmgr = 0;
    gcSizeDdHmgr = 0;
    gpentDdHmgrLast = 0;

    if (gpentDdHmgr)
    {
        EngFreeMem(gpentDdHmgr);
        gpentDdHmgr = 0; 
    }

    if (ghsemHmgr)
    {
        EngDeleteSemaphore(ghsemHmgr);
    }
#endif
    return TRUE;
}

