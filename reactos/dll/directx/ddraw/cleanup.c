/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 dll/directx/ddraw/cleanup.c
 * PURPOSE:              DirectDraw Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"

VOID
Cleanup(LPDDRAWI_DIRECTDRAW_INT This)
{
    DX_WINDBG_trace();

    if (ddgbl.lpDDCBtmp != NULL)
    {
        DxHeapMemFree(ddgbl.lpDDCBtmp);
    }

    if (ddgbl.lpdwFourCC != NULL)
    {
        DxHeapMemFree(ddgbl.lpdwFourCC);
    }

    if (ddgbl.lpModeInfo != NULL)
    {
        DxHeapMemFree(ddgbl.lpModeInfo);
    }

    DdDeleteDirectDrawObject(&ddgbl);

    /*
       anything else to release?
    */

    /* release the linked interface */
    //while (IsBadWritePtr( This->lpVtbl, sizeof( LPDDRAWI_DIRECTDRAW_INT )) )
    //{
    //    LPDDRAWI_DIRECTDRAW_INT newThis = This->lpVtbl;
    //    if (This->lpLcl != NULL)
    //    {
    //        DeleteDC(This->lpLcl->hDC);
    //        DxHeapMemFree(This->lpLcl);
    //    }

    //    DxHeapMemFree(This);
    //    This = newThis;
    //}

    /* release unlinked interface */
    if (This->lpLcl != NULL)
    {
        DxHeapMemFree(This->lpLcl);
    }
    //if (This != NULL)
    //{
    //    DxHeapMemFree(This);
    //}

}

