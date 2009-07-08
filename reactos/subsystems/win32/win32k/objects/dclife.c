/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/dclife.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>
#include <bugcodes.h>

#define NDEBUG
#include <debug.h>

/** Internal functions ********************************************************/

HDC FASTCALL
DC_AllocDC(PUNICODE_STRING Driver)
{
    PDC  NewDC;
    PDC_ATTR pdcattr;
    HDC  hDC;
    PWSTR Buf = NULL;
    XFORM xformTemplate;
    PBRUSH pbrush;

    if (Driver != NULL)
    {
        Buf = ExAllocatePoolWithTag(PagedPool, Driver->MaximumLength, TAG_DC);
        if (!Buf)
        {
            DPRINT1("ExAllocatePoolWithTag failed\n");
            return NULL;
        }
        RtlCopyMemory(Buf, Driver->Buffer, Driver->MaximumLength);
    }

    NewDC = (PDC)GDIOBJ_AllocObjWithHandle(GDI_OBJECT_TYPE_DC);
    if (!NewDC)
    {
        if (Buf)
        {
            ExFreePoolWithTag(Buf, TAG_DC);
        }
        DPRINT1("GDIOBJ_AllocObjWithHandle failed\n");
        return NULL;
    }

    hDC = NewDC->BaseObject.hHmgr;

    NewDC->pdcattr = &NewDC->dcattr;
    DC_AllocateDcAttr(hDC);

    if (Driver != NULL)
    {
        RtlCopyMemory(&NewDC->rosdc.DriverName, Driver, sizeof(UNICODE_STRING));
        NewDC->rosdc.DriverName.Buffer = Buf;
    }
    pdcattr = NewDC->pdcattr;

    // FIXME: no floating point in the kernel!
    xformTemplate.eM11 = 1.0f;
    xformTemplate.eM12 = 0.0f;
    xformTemplate.eM21 = 0.0f;
    xformTemplate.eM22 = 1.0f;
    xformTemplate.eDx = 0.0f;
    xformTemplate.eDy = 0.0f;
    XForm2MatrixS(&NewDC->dclevel.mxWorldToDevice, &xformTemplate);
    XForm2MatrixS(&NewDC->dclevel.mxDeviceToWorld, &xformTemplate);
    XForm2MatrixS(&NewDC->dclevel.mxWorldToPage, &xformTemplate);

    // Setup syncing bits for the dcattr data packets.
    pdcattr->flXform = DEVICE_TO_PAGE_INVALID;

    pdcattr->ulDirty_ = 0;  // Server side

    pdcattr->iMapMode = MM_TEXT;
    pdcattr->iGraphicsMode = GM_COMPATIBLE;
    pdcattr->jFillMode = ALTERNATE;

    pdcattr->szlWindowExt.cx = 1; // Float to Int,,, WRONG!
    pdcattr->szlWindowExt.cy = 1;
    pdcattr->szlViewportExt.cx = 1;
    pdcattr->szlViewportExt.cy = 1;

    pdcattr->crForegroundClr = 0;
    pdcattr->ulForegroundClr = 0;

    pdcattr->ulBackgroundClr = 0xffffff;
    pdcattr->crBackgroundClr = 0xffffff;

    pdcattr->ulPenClr = RGB(0, 0, 0);
    pdcattr->crPenClr = RGB(0, 0, 0);

    pdcattr->ulBrushClr = RGB(255, 255, 255);   // Do this way too.
    pdcattr->crBrushClr = RGB(255, 255, 255);

//// This fixes the default brush and pen settings. See DC_InitDC.

    /* Create the default fill brush */
    pdcattr->hbrush = NtGdiGetStockObject(WHITE_BRUSH);
    NewDC->dclevel.pbrFill = BRUSH_ShareLockBrush(pdcattr->hbrush);
    EBRUSHOBJ_vInit(&NewDC->eboFill, NewDC->dclevel.pbrFill, NULL);

    /* Create the default pen / line brush */
    pdcattr->hpen = NtGdiGetStockObject(BLACK_PEN);
    NewDC->dclevel.pbrLine = PEN_ShareLockPen(pdcattr->hpen);
    EBRUSHOBJ_vInit(&NewDC->eboLine, NewDC->dclevel.pbrLine, NULL);

    /* Create the default text brush */
    pbrush = BRUSH_ShareLockBrush(NtGdiGetStockObject(BLACK_BRUSH));
    EBRUSHOBJ_vInit(&NewDC->eboText, pbrush, NULL);
    pdcattr->ulDirty_ |= DIRTY_TEXT;

    /* Create the default background brush */
    pbrush = BRUSH_ShareLockBrush(NtGdiGetStockObject(WHITE_BRUSH));
    EBRUSHOBJ_vInit(&NewDC->eboBackground, pbrush, NULL);

    pdcattr->hlfntNew = NtGdiGetStockObject(SYSTEM_FONT);
    TextIntRealizeFont(pdcattr->hlfntNew,NULL);

    NewDC->dclevel.hpal = NtGdiGetStockObject(DEFAULT_PALETTE);
    NewDC->dclevel.laPath.eMiterLimit = 10.0;

    NewDC->dclevel.lSaveDepth = 1;

    return NewDC;
}

VOID FASTCALL
DC_FreeDC(HDC DCToFree)
{
    DC_FreeDcAttr(DCToFree);
    if (!IsObjectDead(DCToFree))
    {
        if (!GDIOBJ_FreeObjByHandle(DCToFree, GDI_OBJECT_TYPE_DC))
        {
            DPRINT1("DC_FreeDC failed\n");
        }
    }
    else
    {
        DPRINT1("Attempted to Delete 0x%x currently being destroyed!!!\n",DCToFree);
    }
}

BOOL INTERNAL_CALL
DC_Cleanup(PVOID ObjectBody)
{
    PDC pDC = (PDC)ObjectBody;

    /* Free driver name (HACK) */
    if (pDC->rosdc.DriverName.Buffer)
        ExFreePoolWithTag(pDC->rosdc.DriverName.Buffer, TAG_DC);

    /* Clean up selected objects */
    DC_vSelectSurface(pDC, NULL);
    DC_vSelectFillBrush(pDC, NULL);
    DC_vSelectLineBrush(pDC, NULL);

    /* Dereference default brushes */
    BRUSH_ShareUnlockBrush(pDC->eboText.pbrush);
    BRUSH_ShareUnlockBrush(pDC->eboBackground.pbrush);

    return TRUE;
}

BOOL
FASTCALL
DC_SetOwnership(HDC hDC, PEPROCESS Owner)
{
    PDC pDC;

    if (!GDIOBJ_SetOwnership(hDC, Owner)) return FALSE;
    pDC = DC_LockDc(hDC);
    if (pDC)
    {
        if (pDC->rosdc.hClipRgn)
        {
            if (!GDIOBJ_SetOwnership(pDC->rosdc.hClipRgn, Owner)) return FALSE;
        }
        if (pDC->rosdc.hVisRgn)
        {
            if (!GDIOBJ_SetOwnership(pDC->rosdc.hVisRgn, Owner)) return FALSE;
        }
        if (pDC->rosdc.hGCClipRgn)
        {
            if (!GDIOBJ_SetOwnership(pDC->rosdc.hGCClipRgn, Owner)) return FALSE;
        }
        if (pDC->dclevel.hPath)
        {
            if (!GDIOBJ_SetOwnership(pDC->dclevel.hPath, Owner)) return FALSE;
        }
        DC_UnlockDc(pDC);
    }

    return TRUE;
}


HDC FASTCALL
IntGdiCreateDC(
    PUNICODE_STRING Driver,
    PUNICODE_STRING Device,
    PVOID pUMdhpdev,
    CONST PDEVMODEW InitData,
    BOOL CreateAsIC)
{
    HDC      hdc;
    PDC      pdc;
    PDC_ATTR pdcattr;
    HRGN     hVisRgn;
    UNICODE_STRING StdDriver;
    BOOL calledFromUser;
    HSURF hsurf;

    RtlInitUnicodeString(&StdDriver, L"DISPLAY");

    DPRINT("DriverName: %wZ, DeviceName: %wZ\n", Driver, Device);

    if (NULL == Driver || 0 == RtlCompareUnicodeString(Driver, &StdDriver, TRUE))
    {
        if (CreateAsIC)
        {
            if (! IntPrepareDriverIfNeeded())
            {
                /* Here, we have two possibilities:
                 * a) return NULL, and hope that the caller
                 *    won't call us in a loop
                 * b) bugcheck, but caller is unable to
                 *    react on the problem
                 */
                /*DPRINT1("Unable to prepare graphics driver, returning NULL ic\n");
                return NULL;*/
                KeBugCheck(VIDEO_DRIVER_INIT_FAILURE);
            }
        }
        else
        {
            calledFromUser = UserIsEntered();
            if (!calledFromUser)
            {
                UserEnterExclusive();
            }

            if (! co_IntGraphicsCheck(TRUE))
            {
                if (!calledFromUser)
                {
                    UserLeave();
                }
                DPRINT1("Unable to initialize graphics, returning NULL dc\n");
                return NULL;
            }

            if (!calledFromUser)
            {
                UserLeave();
            }

        }
    }

    /*  Check for existing DC object  */
    if ((hdc = DC_FindOpenDC(Driver)) != NULL)
    {
        hdc = NtGdiCreateCompatibleDC(hdc);
        if (!hdc)
            DPRINT1("NtGdiCreateCompatibleDC() failed\n");
        return hdc;
    }

    /*  Allocate a DC object  */
    pdc = DC_AllocDC(Driver);
    if (pdc == NULL)
    {
        DPRINT1("DC_AllocDC() failed\n");
        return NULL;
    }
    hdc = pdc->BaseObject.hHmgr;
    pdcattr = pdc->pdcattr;

    pdc->dctype = DC_TYPE_DIRECT;

    pdc->dhpdev = PrimarySurface.hPDev;
    if (pUMdhpdev) pUMdhpdev = pdc->dhpdev; // set DHPDEV for device.
    pdc->ppdev = (PVOID)&PrimarySurface;
    hsurf = (HBITMAP)PrimarySurface.pSurface; // <- what kind of haxx0ry is that?
    pdc->dclevel.pSurface = SURFACE_ShareLockSurface(hsurf);

    // ATM we only have one display.
    pdcattr->ulDirty_ |= DC_PRIMARY_DISPLAY;

    pdc->rosdc.bitsPerPixel = pdc->ppdev->GDIInfo.cBitsPixel *
                              pdc->ppdev->GDIInfo.cPlanes;
    DPRINT("Bits per pel: %u\n", pdc->rosdc.bitsPerPixel);

    pdc->flGraphicsCaps  = PrimarySurface.DevInfo.flGraphicsCaps;
    pdc->flGraphicsCaps2 = PrimarySurface.DevInfo.flGraphicsCaps2;

    pdc->dclevel.hpal = NtGdiGetStockObject(DEFAULT_PALETTE);

    pdcattr->jROP2 = R2_COPYPEN;

    pdc->erclWindow.top = pdc->erclWindow.left = 0;
    pdc->erclWindow.right  = pdc->ppdev->GDIInfo.ulHorzRes;
    pdc->erclWindow.bottom = pdc->ppdev->GDIInfo.ulVertRes;
    pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE; // Default is CCW.

    pdcattr->iCS_CP = ftGdiGetTextCharsetInfo(pdc,NULL,0);

    hVisRgn = NtGdiCreateRectRgn(0, 0, pdc->ppdev->GDIInfo.ulHorzRes,
                                 pdc->ppdev->GDIInfo.ulVertRes);

    if (!CreateAsIC)
    {
        pdc->pSurfInfo = NULL;
//    pdc->dclevel.pSurface =
        DC_UnlockDc(pdc);

        /*  Initialize the DC state  */
        DC_InitDC(hdc);
        IntGdiSetTextColor(hdc, RGB(0, 0, 0));
        IntGdiSetBkColor(hdc, RGB(255, 255, 255));
    }
    else
    {
        /* From MSDN2:
           The CreateIC function creates an information context for the specified device.
           The information context provides a fast way to get information about the
           device without creating a device context (DC). However, GDI drawing functions
           cannot accept a handle to an information context.
         */
        pdc->dctype = DC_TYPE_INFO;
//    pdc->pSurfInfo =
        DC_vSelectSurface(pdc, NULL);
        pdcattr->crBackgroundClr = pdcattr->ulBackgroundClr = RGB(255, 255, 255);
        pdcattr->crForegroundClr = RGB(0, 0, 0);
        DC_UnlockDc(pdc);
    }

    if (hVisRgn)
    {
        GdiSelectVisRgn(hdc, hVisRgn);
        GreDeleteObject(hVisRgn);
    }

    IntGdiSetTextAlign(hdc, TA_TOP);
    IntGdiSetBkMode(hdc, OPAQUE);

    return hdc;
}


HDC APIENTRY
NtGdiOpenDCW(
    PUNICODE_STRING Device,
    DEVMODEW *InitData,
    PUNICODE_STRING pustrLogAddr,
    ULONG iType,
    BOOL bDisplay,
    HANDLE hspool,
    VOID *pDriverInfo2,
    VOID *pUMdhpdev)
{
    UNICODE_STRING SafeDevice;
    DEVMODEW SafeInitData;
    PVOID Dhpdev;
    HDC Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    if (InitData)
    {
        _SEH2_TRY
        {
            if (pUMdhpdev)
            {
                ProbeForWrite(pUMdhpdev, sizeof(PVOID), 1);
            }
            ProbeForRead(InitData, sizeof(DEVMODEW), 1);
            RtlCopyMemory(&SafeInitData, InitData, sizeof(DEVMODEW));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return NULL;
        }
        /* FIXME - InitData can have some more bytes! */
    }

    if (Device)
    {
        Status = IntSafeCopyUnicodeString(&SafeDevice, Device);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return NULL;
        }
    }

    Ret = IntGdiCreateDC(Device ? &SafeDevice : NULL,
                         NULL,
                         pUMdhpdev ? &Dhpdev : NULL,
                         InitData ? &SafeInitData : NULL,
                         (BOOL) iType); // FALSE 0 DCW, TRUE 1 ICW

    // FIXME!!!!
    if (pUMdhpdev) pUMdhpdev = Dhpdev;

    return Ret;
}

HDC FASTCALL
IntGdiCreateDisplayDC(HDEV hDev, ULONG DcType, BOOL EmptyDC)
{
    HDC hDC;
    UNICODE_STRING DriverName;
    RtlInitUnicodeString(&DriverName, L"DISPLAY");

    if (DcType != DC_TYPE_MEMORY)
        hDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, (DcType == DC_TYPE_INFO));
    else
        hDC = NtGdiCreateCompatibleDC(NULL); // OH~ Yuck! I think I taste vomit in my mouth!
//
// There is room to grow here~
//

//
// If NULL, first time through! Build the default (was window) dc!
// Setup clean DC state for the system.
//
    if (hDC && !defaultDCstate) // Ultra HAX! Dedicated to GvG!
    { // This is a cheesy way to do this.
        PDC dc = DC_LockDc(hDC);
        defaultDCstate = ExAllocatePoolWithTag(PagedPool, sizeof(DC), TAG_DC);
        if (!defaultDCstate)
        {
            DC_UnlockDc(dc);
            return NULL;
        }
        RtlZeroMemory(defaultDCstate, sizeof(DC));
        defaultDCstate->pdcattr = &defaultDCstate->dcattr;
        DC_vCopyState(dc, defaultDCstate);
        DC_UnlockDc(dc);
    }
    return hDC;
}

BOOL
FASTCALL
IntGdiDeleteDC(HDC hDC, BOOL Force)
{
    PDC DCToDelete = DC_LockDc(hDC);

    if (DCToDelete == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!Force)
    {
        if (DCToDelete->fs & DC_FLAG_PERMANENT)
        {
            DPRINT1("No! You Naughty Application!\n");
            DC_UnlockDc(DCToDelete);
            return UserReleaseDC(NULL, hDC, FALSE);
        }
    }

    /*  First delete all saved DCs  */
    while (DCToDelete->dclevel.lSaveDepth > 1)
    {
        PDC  savedDC;
        HDC  savedHDC;

        savedHDC = DCToDelete->hdcNext;
        savedDC = DC_LockDc(savedHDC);
        if (savedDC == NULL)
        {
            break;
        }
        DCToDelete->hdcNext = savedDC->hdcNext;
        DCToDelete->dclevel.lSaveDepth--;
        DC_UnlockDc(savedDC);
        IntGdiDeleteDC(savedHDC, Force);
    }

    /*  Free GDI resources allocated to this DC  */
    if (!(DCToDelete->dclevel.flPath & DCPATH_SAVESTATE))
    {
        /*
        NtGdiSelectPen (DCHandle, STOCK_BLACK_PEN);
        NtGdiSelectBrush (DCHandle, STOCK_WHITE_BRUSH);
        NtGdiSelectFont (DCHandle, STOCK_SYSTEM_FONT);
        DC_LockDC (DCHandle); NtGdiSelectXxx does not recognize stock objects yet  */
    }
    if (DCToDelete->rosdc.hClipRgn)
    {
        GreDeleteObject(DCToDelete->rosdc.hClipRgn);
    }
    if (DCToDelete->rosdc.hVisRgn)
    {
        GreDeleteObject(DCToDelete->rosdc.hVisRgn);
    }
    if (NULL != DCToDelete->rosdc.CombinedClip)
    {
        IntEngDeleteClipRegion(DCToDelete->rosdc.CombinedClip);
    }
    if (DCToDelete->rosdc.hGCClipRgn)
    {
        GreDeleteObject(DCToDelete->rosdc.hGCClipRgn);
    }
    PATH_Delete(DCToDelete->dclevel.hPath);

    DC_UnlockDc(DCToDelete);
    DC_FreeDC(hDC);
    return TRUE;
}

HDC FASTCALL
DC_FindOpenDC(PUNICODE_STRING  Driver)
{
    return NULL;
}

/*!
 * Initialize some common fields in the Device Context structure.
*/
VOID FASTCALL
DC_InitDC(HDC  DCHandle)
{
//  NtGdiRealizeDefaultPalette(DCHandle);

////  Removed for now.. See above brush and pen.
//  NtGdiSelectBrush(DCHandle, NtGdiGetStockObject( WHITE_BRUSH ));
//  NtGdiSelectPen(DCHandle, NtGdiGetStockObject( BLACK_PEN ));
////
    //NtGdiSelectFont(DCHandle, hFont);

    /*
      {
        int res;
        res = CLIPPING_UpdateGCRegion(DCToInit);
        ASSERT ( res != ERROR );
      }
    */
}

/*
* @unimplemented
*/
BOOL
APIENTRY
NtGdiMakeInfoDC(
    IN HDC hdc,
    IN BOOL bSet)
{
    UNIMPLEMENTED;
    return FALSE;
}

HDC APIENTRY
NtGdiCreateCompatibleDC(HDC hDC)
{
    PDC pdcNew, pdcOld;
    PDC_ATTR pdcattrNew, pdcattrOld;
    HDC hdcNew, DisplayDC = NULL;
    HRGN hVisRgn;
    UNICODE_STRING DriverName;
    DWORD Layout = 0;
    HSURF hsurf;

    if (hDC == NULL)
    {
        RtlInitUnicodeString(&DriverName, L"DISPLAY");
        DisplayDC = IntGdiCreateDC(&DriverName, NULL, NULL, NULL, TRUE);
        if (NULL == DisplayDC)
        {
            DPRINT1("Failed to create DisplayDC\n");
            return NULL;
        }
        hDC = DisplayDC;
    }

    /*  Allocate a new DC based on the original DC's device  */
    pdcOld = DC_LockDc(hDC);
    if (NULL == pdcOld)
    {
        if (NULL != DisplayDC)
        {
            NtGdiDeleteObjectApp(DisplayDC);
        }
        DPRINT1("Failed to lock hDC\n");
        return NULL;
    }
    pdcNew = DC_AllocDC(&pdcOld->rosdc.DriverName);
    if (!pdcNew)
    {
        DPRINT1("Failed to create pdcNew\n");
        DC_UnlockDc(pdcOld);
        if (DisplayDC)
        {
            NtGdiDeleteObjectApp(DisplayDC);
        }
        return  NULL;
    }
    hdcNew = pdcNew->BaseObject.hHmgr;

    pdcattrOld = pdcOld->pdcattr;
    pdcattrNew = pdcNew->pdcattr;

    /* Copy information from original DC to new DC  */
    pdcNew->dclevel.hdcSave = hdcNew;

    pdcNew->dhpdev = pdcOld->dhpdev;

    pdcNew->rosdc.bitsPerPixel = pdcOld->rosdc.bitsPerPixel;

    /* DriverName is copied in the AllocDC routine  */
    pdcattrNew->ptlWindowOrg   = pdcattrOld->ptlWindowOrg;
    pdcattrNew->szlWindowExt   = pdcattrOld->szlWindowExt;
    pdcattrNew->ptlViewportOrg = pdcattrOld->ptlViewportOrg;
    pdcattrNew->szlViewportExt = pdcattrOld->szlViewportExt;

    pdcNew->dctype        = DC_TYPE_MEMORY; // Always!
    hsurf      = NtGdiGetStockObject(DEFAULT_BITMAP);
    pdcNew->dclevel.pSurface = SURFACE_ShareLockSurface(hsurf);
    pdcNew->ppdev          = pdcOld->ppdev;
    pdcNew->dclevel.hpal    = pdcOld->dclevel.hpal;

    pdcattrNew->lTextAlign      = pdcattrOld->lTextAlign;
    pdcattrNew->lBkMode         = pdcattrOld->lBkMode;
    pdcattrNew->jBkMode         = pdcattrOld->jBkMode;
    pdcattrNew->jROP2           = pdcattrOld->jROP2;
    pdcattrNew->dwLayout        = pdcattrOld->dwLayout;
    if (pdcattrOld->dwLayout & LAYOUT_ORIENTATIONMASK) Layout = pdcattrOld->dwLayout;
    pdcNew->dclevel.flPath     = pdcOld->dclevel.flPath;
    pdcattrNew->ulDirty_        = pdcattrOld->ulDirty_;
    pdcattrNew->iCS_CP          = pdcattrOld->iCS_CP;

    pdcNew->erclWindow = (RECTL){0, 0, 1, 1};

    DC_UnlockDc(pdcNew);
    DC_UnlockDc(pdcOld);
    if (NULL != DisplayDC)
    {
        NtGdiDeleteObjectApp(DisplayDC);
    }

    hVisRgn = NtGdiCreateRectRgn(0, 0, 1, 1);
    if (hVisRgn)
    {
        GdiSelectVisRgn(hdcNew, hVisRgn);
        GreDeleteObject(hVisRgn);
    }
    if (Layout) NtGdiSetLayout(hdcNew, -1, Layout);

    DC_InitDC(hdcNew);
    return hdcNew;
}


BOOL
APIENTRY
NtGdiDeleteObjectApp(HANDLE DCHandle)
{
    /* Complete all pending operations */
    NtGdiFlushUserBatch();

    if (GDI_HANDLE_IS_STOCKOBJ(DCHandle)) return TRUE;

    if (GDI_HANDLE_GET_TYPE(DCHandle) != GDI_OBJECT_TYPE_DC)
        return GreDeleteObject((HGDIOBJ) DCHandle);

    if (IsObjectDead((HGDIOBJ)DCHandle)) return TRUE;

    if (!GDIOBJ_OwnedByCurrentProcess(DCHandle))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return IntGdiDeleteDC(DCHandle, FALSE);
}

BOOL
APIENTRY
NewNtGdiDeleteObjectApp(HANDLE DCHandle)
{
  GDIOBJTYPE ObjType;

  if (GDI_HANDLE_IS_STOCKOBJ(DCHandle)) return TRUE;

  ObjType = GDI_HANDLE_GET_TYPE(DCHandle) >> GDI_ENTRY_UPPER_SHIFT;

  if (GreGetObjectOwner( DCHandle, ObjType))
  {
     switch(ObjType)
     {
        case GDIObjType_DC_TYPE:
          return IntGdiDeleteDC(DCHandle, FALSE);

        case GDIObjType_RGN_TYPE:
        case GDIObjType_SURF_TYPE:
        case GDIObjType_PAL_TYPE:
        case GDIObjType_LFONT_TYPE:
        case GDIObjType_BRUSH_TYPE:
          return GreDeleteObject((HGDIOBJ) DCHandle);

        default:
          return FALSE;
     }
  }
  return (DCHandle != NULL);
}

