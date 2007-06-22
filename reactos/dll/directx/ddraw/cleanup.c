/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
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
       are it any more I forget to release ?
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

