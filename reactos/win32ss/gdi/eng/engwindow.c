/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI WNDOBJ Functions
 * FILE:              subsystems/win32/win32k/eng/engwindow.c
 * PROGRAMER:         Gregor Anich
 */

#include <win32k.h>
#include <debug.h>
DBG_DEFAULT_CHANNEL(EngWnd);

INT gcountPWO = 0;

/*
 * Calls the WNDOBJCHANGEPROC of the given WNDOBJ
 */
VOID
FASTCALL
IntEngWndCallChangeProc(
    _In_ XCLIPOBJ *Clip,
    _In_ FLONG   flChanged)
{
    if (Clip->ChangeProc == NULL)
    {
        return;
    }

    /* check flags of the WNDOBJ */
    flChanged &= Clip->Flags;
    if (flChanged == 0)
    {
        return;
    }

    TRACE("Calling WNDOBJCHANGEPROC (0x%p), Changed = 0x%x\n",
           Clip->ChangeProc, flChanged);

    /* Call the WNDOBJCHANGEPROC */
    if (flChanged == WOC_CHANGED)
        Clip->ChangeProc(NULL, flChanged);
    else
        Clip->ChangeProc(&Clip->WndObj, flChanged);
}

/*
 * Fills the CLIPOBJ and client rect of the WNDOBJ with the data from the given WND
 */
BOOLEAN
FASTCALL
IntEngWndUpdateClipObj(
    XCLIPOBJ* Clip,
    PWND Window)
{
    PREGION visRgn;

    TRACE("IntEngWndUpdateClipObj\n");

    visRgn = VIS_ComputeVisibleRegion(Window, TRUE, TRUE, TRUE);
    if (visRgn != NULL)
    {
        if (visRgn->rdh.nCount > 0)
        {
            IntEngUpdateClipRegion(Clip, visRgn->rdh.nCount, visRgn->Buffer, &visRgn->rdh.rcBound);
            TRACE("Created visible region with %lu rects\n", visRgn->rdh.nCount);
            TRACE("  BoundingRect: %d, %d  %d, %d\n",
                   visRgn->rdh.rcBound.left, visRgn->rdh.rcBound.top,
                   visRgn->rdh.rcBound.right, visRgn->rdh.rcBound.bottom);
            {
                ULONG i;
                for (i = 0; i < visRgn->rdh.nCount; i++)
                {
                    TRACE("  Rect #%lu: %ld,%ld  %ld,%ld\n", i+1,
                           visRgn->Buffer[i].left, visRgn->Buffer[i].top,
                           visRgn->Buffer[i].right, visRgn->Buffer[i].bottom);
                }
            }
        }
        REGION_Delete(visRgn);
    }
    else
    {
        /* Fall back to client rect */
        IntEngUpdateClipRegion(Clip, 1, &Window->rcClient, &Window->rcClient);
    }

    /* Update the WNDOBJ */
    Clip->WndObj.rclClient = Window->rcClient;
    Clip->WndObj.coClient.iUniq++;

    return TRUE;
}

/*
 * Updates all WNDOBJs of the given WND and calls the change-procs.
 */
VOID
FASTCALL
IntEngWindowChanged(
    _In_    PWND  Window,
    _In_    FLONG flChanged)
{
    XCLIPOBJ *Clip;

    ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

    Clip = UserGetProp(Window, AtomWndObj);
    if (!Clip)
    {
        return;
    }

    ASSERT(Clip->Hwnd == Window->head.h);
    // if (Clip->WndObj.pvConsumer != NULL)
    {
        /* Update the WNDOBJ */
        switch (flChanged)
        {
        case WOC_RGN_CLIENT:
            /* Update the clipobj and client rect of the WNDOBJ */
            IntEngWndUpdateClipObj(Clip, Window);
            break;

        case WOC_DELETE:
            /* FIXME: Should the WNDOBJs be deleted by win32k or by the driver? */
            break;
        }

        /* Call the change proc */
        IntEngWndCallChangeProc(Clip, flChanged);

        /* HACK: Send WOC_CHANGED after WOC_RGN_CLIENT */
        if (flChanged == WOC_RGN_CLIENT)
        {
            IntEngWndCallChangeProc(Clip, WOC_CHANGED);
        }
    }
}

/*
 * @implemented
 */
WNDOBJ*
APIENTRY
EngCreateWnd(
    SURFOBJ          *pso,
    HWND              hWnd,
    WNDOBJCHANGEPROC  pfn,
    FLONG             fl,
    int               iPixelFormat)
{
    XCLIPOBJ *Clip = NULL;
    WNDOBJ *WndObjUser = NULL;
    PWND Window;
    BOOL calledFromUser;
    DECLARE_RETURN(WNDOBJ*);

    TRACE("EngCreateWnd: pso = 0x%p, hwnd = 0x%p, pfn = 0x%p, fl = 0x%lx, pixfmt = %d\n",
            pso, hWnd, pfn, fl, iPixelFormat);

    if (fl & (WO_RGN_WINDOW | WO_RGN_DESKTOP_COORD | WO_RGN_UPDATE_ALL))
    {
        FIXME("Unsupported flags: 0x%lx\n", fl & ~(WO_RGN_CLIENT_DELTA | WO_RGN_CLIENT | WO_RGN_SURFACE_DELTA | WO_RGN_SURFACE));
    }

    calledFromUser = UserIsEntered();
    if (!calledFromUser) {
        UserEnterShared();
    }

    /* Get window object */
    Window = UserGetWindowObject(hWnd);
    if (Window == NULL)
    {
        RETURN( NULL);
    }

    /* Create WNDOBJ */
    Clip = EngAllocMem(FL_ZERO_MEMORY, sizeof (XCLIPOBJ), GDITAG_WNDOBJ);
    if (Clip == NULL)
    {
        ERR("Failed to allocate memory for a WND structure!\n");
        RETURN( NULL);
    }
    IntEngInitClipObj(Clip);

    /* Fill the clipobj */
    if (!IntEngWndUpdateClipObj(Clip, Window))
    {
        EngFreeMem(Clip);
        RETURN( NULL);
    }

    /* Fill user object */
    WndObjUser = &Clip->WndObj;
    WndObjUser->psoOwner = pso;
    WndObjUser->pvConsumer = NULL;

    /* Fill internal object */
    Clip->Hwnd = hWnd;
    Clip->ChangeProc = pfn;
    /* Keep track of relevant flags */
    Clip->Flags = fl & (WO_RGN_CLIENT_DELTA | WO_RGN_CLIENT | WO_RGN_SURFACE_DELTA | WO_RGN_SURFACE | WO_DRAW_NOTIFY);
    if (fl & WO_SPRITE_NOTIFY)
        Clip->Flags |= WOC_SPRITE_OVERLAP | WOC_SPRITE_NO_OVERLAP;
    /* Those should always be sent */
    Clip->Flags |= WOC_CHANGED | WOC_DELETE;
    Clip->PixelFormat = iPixelFormat;

    /* associate object with window */
    UserSetProp(Window, AtomWndObj, Clip);
    ++gcountPWO;

    TRACE("EngCreateWnd: SUCCESS: %p!\n", WndObjUser);

    RETURN( WndObjUser);

CLEANUP:

    if (!calledFromUser) {
        UserLeave();
    }

    END_CLEANUP;
}


/*
 * @implemented
 */
VOID
APIENTRY
EngDeleteWnd(
    IN WNDOBJ *pwo)
{
    XCLIPOBJ* Clip = CONTAINING_RECORD(pwo, XCLIPOBJ, WndObj);
    PWND Window;
    BOOL calledFromUser;

    TRACE("EngDeleteWnd: pwo = 0x%p\n", pwo);

    calledFromUser = UserIsEntered();
    if (!calledFromUser) {
        UserEnterExclusive();
    }

    /* Get window object */
    Window = UserGetWindowObject(Clip->Hwnd);
    if (Window == NULL)
    {
        ERR("Couldnt get window object for WndObjInt->Hwnd!!!\n");
    }
    else
    {
        /* Remove object from window */
        UserRemoveProp(Window, AtomWndObj);
    }
    --gcountPWO;

    if (!calledFromUser) {
        UserLeave();
    }

    /* Free resources */
    IntEngFreeClipResources(Clip);
    EngFreeMem(Clip);
}


/*
 * @implemented
 */
BOOL
APIENTRY
WNDOBJ_bEnum(
    IN WNDOBJ  *pwo,
    IN ULONG  cj,
    OUT ULONG  *pul)
{
    /* Relay */
    return CLIPOBJ_bEnum(&pwo->coClient, cj, pul);
}


/*
 * @implemented
 */
ULONG
APIENTRY
WNDOBJ_cEnumStart(
    IN WNDOBJ  *pwo,
    IN ULONG  iType,
    IN ULONG  iDirection,
    IN ULONG  cLimit)
{
    /* Relay */
    // FIXME: Should we enumerate all rectangles or not?
    return CLIPOBJ_cEnumStart(&pwo->coClient, FALSE, iType, iDirection, cLimit);
}


/*
 * @implemented
 */
VOID
APIENTRY
WNDOBJ_vSetConsumer(
    IN WNDOBJ  *pwo,
    IN PVOID  pvConsumer)
{
    XCLIPOBJ* Clip = CONTAINING_RECORD(pwo, XCLIPOBJ, WndObj);
    BOOL Hack;

    TRACE("WNDOBJ_vSetConsumer: pwo = 0x%p, pvConsumer = 0x%p\n", pwo, pvConsumer);

    Hack = (pwo->pvConsumer == NULL);
    pwo->pvConsumer = pvConsumer;

    /* HACKHACKHACK
     *
     * MSDN says that the WNDOBJCHANGEPROC will be called with the most recent state
     * when a WNDOBJ is created - we do it here because most drivers will need pvConsumer
     * in the callback to identify the WNDOBJ I think.
     *
     *  - blight
     */
    if (Hack)
    {
        FIXME("Is this hack really needed?\n");
        IntEngWndCallChangeProc(Clip, WOC_RGN_CLIENT);
        IntEngWndCallChangeProc(Clip, WOC_CHANGED);
        IntEngWndCallChangeProc(Clip, WOC_DRAWN);
    }
}

/* EOF */

