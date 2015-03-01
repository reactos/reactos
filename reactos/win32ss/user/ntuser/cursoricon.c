/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Cursor and icon functions
 * FILE:             win32ss/user/ntuser/cursoricon.c
 * PROGRAMER:        ReactOS Team
 */
/*
 * We handle two types of cursors/icons:
 * - Private
 *   Loaded without LR_SHARED flag
 *   Private to a process
 *   Can be deleted by calling NtDestroyCursorIcon()
 *   CurIcon->hModule, CurIcon->hRsrc and CurIcon->hGroupRsrc set to NULL
 * - Shared
 *   Loaded with LR_SHARED flag
 *   Possibly shared by multiple processes
 *   Immune to NtDestroyCursorIcon()
 *   CurIcon->hModule, CurIcon->hRsrc and CurIcon->hGroupRsrc are valid
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserIcon);

SYSTEM_CURSORINFO gSysCursorInfo;

PCURICON_OBJECT gcurFirst = NULL; // After all is done, this should be WINLOGO!

//
//   System Cursors
//
SYSTEMCURICO gasyscur[] = {
    {OCR_NORMAL,     NULL},
    {OCR_IBEAM,      NULL},
    {OCR_WAIT,       NULL},
    {OCR_CROSS,      NULL},
    {OCR_UP,         NULL},
    {OCR_ICON,       NULL},
    {OCR_SIZE,       NULL},
    {OCR_SIZENWSE,   NULL},
    {OCR_SIZENESW,   NULL},
    {OCR_SIZEWE,     NULL},
    {OCR_SIZENS,     NULL},
    {OCR_SIZEALL,    NULL},
    {OCR_NO,         NULL},
    {OCR_HAND,       NULL},
    {OCR_APPSTARTING,NULL},
    {OCR_HELP,       NULL},
    };

//
//   System Icons
//
SYSTEMCURICO gasysico[] = {
    {OIC_SAMPLE, NULL},
    {OIC_HAND,   NULL},
    {OIC_QUES,   NULL},
    {OIC_BANG,   NULL},
    {OIC_NOTE,   NULL},
    {OIC_WINLOGO,NULL},
    };

BOOL
InitCursorImpl(VOID)
{
    gSysCursorInfo.Enabled = FALSE;
    gSysCursorInfo.ButtonsDown = 0;
    gSysCursorInfo.bClipped = FALSE;
    gSysCursorInfo.LastBtnDown = 0;
    gSysCursorInfo.CurrentCursorObject = NULL;
    gSysCursorInfo.ShowingCursor = -1;
    gSysCursorInfo.ClickLockActive = FALSE;
    gSysCursorInfo.ClickLockTime = 0;

    return TRUE;
}

static
VOID
IntInsertCursorIntoList(
    _Inout_ PCURICON_OBJECT pcur)
{
    PPROCESSINFO ppi = pcur->head.ppi;
    PCURICON_OBJECT *ppcurHead;
    NT_ASSERT((pcur->CURSORF_flags & (CURSORF_GLOBAL|CURSORF_LRSHARED)) != 0);
    NT_ASSERT((pcur->CURSORF_flags & CURSORF_LINKED) == 0);

    /* Get the right list head */
    ppcurHead = (pcur->CURSORF_flags & CURSORF_GLOBAL) ?
        &gcurFirst : &ppi->pCursorCache;

    UserReferenceObject(pcur);
    pcur->pcurNext = *ppcurHead;
    *ppcurHead = pcur;
    pcur->CURSORF_flags |= CURSORF_LINKED;
}

// FIXME: should think about using a LIST_ENTRY!
static
VOID
IntRemoveCursorFromList(
    _Inout_ PCURICON_OBJECT pcur)
{
    PPROCESSINFO ppi = pcur->head.ppi;
    PCURICON_OBJECT *ppcurHead;
    PCURICON_OBJECT *ppcur;
    NT_ASSERT((pcur->CURSORF_flags & (CURSORF_GLOBAL|CURSORF_LRSHARED)) != 0);
    NT_ASSERT((pcur->CURSORF_flags & CURSORF_LINKED) != 0);

    NT_ASSERT((pcur->CURSORF_flags & CURSORF_GLOBAL) == 0);

    /* Get the right list head */
    ppcurHead = (pcur->CURSORF_flags & CURSORF_GLOBAL) ?
        &gcurFirst : &ppi->pCursorCache;

    /* Loop all cursors in the cache */
    for (ppcur = ppcurHead;
         (*ppcur) != NULL;
         ppcur = &(*ppcur)->pcurNext)
    {
        /* Check if this is the one we are looking for */
        if ((*ppcur) == pcur)
        {
            /* Remove it from the list */
            (*ppcur) = pcur->pcurNext;

            /* Dereference it */
            UserDereferenceObject(pcur);
            pcur->CURSORF_flags &= ~CURSORF_LINKED;
            return;
        }
    }

    /* We did not find it, this must not happen */
    NT_ASSERT(FALSE);
}

VOID
IntLoadSystenIcons(HICON hcur, DWORD id)
{
    PCURICON_OBJECT pcur;
    int i;
    PPROCESSINFO ppi;

    if (hcur)
    {
        pcur = UserGetCurIconObject(hcur);
        if (!pcur)
        {
            EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
            return;
        }

        ppi = PsGetCurrentProcessWin32Process();

        if (!(ppi->W32PF_flags & W32PF_CREATEDWINORDC))
           return;

        // Set Small Window Icon and do not link.
        if ( id == OIC_WINLOGO+1 )
        {
            pcur->CURSORF_flags |= CURSORF_GLOBAL;
            UserReferenceObject(pcur);
            pcur->head.ppi = NULL;
            return;
        }

        for (i = 0 ; i < 6; i++)
        {
            if (gasysico[i].type == id)
            {
                gasysico[i].handle = pcur;
                pcur->CURSORF_flags |= CURSORF_GLOBAL;

                //
                //  The active switch between LR shared and Global public.
                //  This is hacked around to support this while at the initial system start up.
                //
                pcur->head.ppi = NULL;

                IntInsertCursorIntoList(pcur);
                return;
            }
        }
    }
}

PSYSTEM_CURSORINFO
IntGetSysCursorInfo()
{
    return &gSysCursorInfo;
}

FORCEINLINE
BOOL
is_icon(PCURICON_OBJECT object)
{
    return MAKEINTRESOURCE(object->rt) == RT_ICON;
}

/* This function creates a reference for the object! */
PCURICON_OBJECT FASTCALL UserGetCurIconObject(HCURSOR hCurIcon)
{
    PCURICON_OBJECT CurIcon;

    if (!hCurIcon)
    {
        EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
        return NULL;
    }

    if (UserObjectInDestroy(hCurIcon))
    {
        WARN("Requesting invalid/destroyed cursor.\n");
        EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
        return NULL;
    }

    CurIcon = (PCURICON_OBJECT)UserReferenceObjectByHandle(hCurIcon, TYPE_CURSOR);
    if (!CurIcon)
    {
        /* We never set ERROR_INVALID_ICON_HANDLE. lets hope noone ever checks for it */
        EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
        return NULL;
    }

    ASSERT(CurIcon->head.cLockObj >= 1);
    return CurIcon;
}

BOOL UserSetCursorPos( INT x, INT y, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook)
{
    PWND DesktopWindow;
    PSYSTEM_CURSORINFO CurInfo;
    MSG Msg;
    RECTL rcClip;
    POINT pt;

    if (!(DesktopWindow = UserGetDesktopWindow()))
    {
        return FALSE;
    }

    CurInfo = IntGetSysCursorInfo();

    /* Clip cursor position */
    if (!CurInfo->bClipped)
        rcClip = DesktopWindow->rcClient;
    else
        rcClip = CurInfo->rcClip;

    if (x >= rcClip.right)  x = rcClip.right - 1;
    if (x < rcClip.left)    x = rcClip.left;
    if (y >= rcClip.bottom) y = rcClip.bottom - 1;
    if (y < rcClip.top)     y = rcClip.top;

    pt.x = x;
    pt.y = y;

    /* 1. Generate a mouse move message, this sets the htEx and Track Window too. */
    Msg.message = WM_MOUSEMOVE;
    Msg.wParam = UserGetMouseButtonsState();
    Msg.lParam = MAKELPARAM(x, y);
    Msg.pt = pt;
    co_MsqInsertMouseMessage(&Msg, flags, dwExtraInfo, Hook);

    /* 2. Store the new cursor position */
    gpsi->ptCursor = pt;

    return TRUE;
}

HANDLE
IntCreateCurIconHandle(BOOLEAN Animated)
{
    PCURICON_OBJECT CurIcon;
    HANDLE hCurIcon;

    CurIcon = UserCreateObject(
        gHandleTable,
        NULL,
        GetW32ThreadInfo(),
        &hCurIcon,
        TYPE_CURSOR,
        Animated ? sizeof(ACON) : sizeof(CURICON_OBJECT));

    if (!CurIcon)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (Animated)
    {
        /* We MUST set this flag, to track whether this is an ACON! */
        CurIcon->CURSORF_flags |= CURSORF_ACON;
    }

    NT_ASSERT(CurIcon->pcurNext == NULL);
    UserDereferenceObject(CurIcon);

    return hCurIcon;
}

BOOLEAN
IntDestroyCurIconObject(
    _In_ PVOID Object)
{
    PCURICON_OBJECT CurIcon = Object;

    /* Check if the cursor is in a list */
    if (CurIcon->CURSORF_flags & CURSORF_LINKED)
    {
        /* Remove the cursor from it's list */
        IntRemoveCursorFromList(CurIcon);
    }

    /* We just mark the handle as being destroyed.
     * Deleting all the stuff will be deferred to the actual struct free. */
    UserDeleteObject(CurIcon->head.h, TYPE_CURSOR);
    return TRUE;
}

VOID
FreeCurIconObject(
    _In_ PVOID Object)
{
    PCURICON_OBJECT CurIcon = Object;

    if (!(CurIcon->CURSORF_flags & CURSORF_ACON))
    {
        HBITMAP bmpMask = CurIcon->hbmMask;
        HBITMAP bmpColor = CurIcon->hbmColor;
        HBITMAP bmpAlpha = CurIcon->hbmAlpha;

        /* Delete bitmaps */
        if (bmpMask)
        {
            GreSetObjectOwner(bmpMask, GDI_OBJ_HMGR_POWNED);
            NT_VERIFY(GreDeleteObject(bmpMask) == TRUE);
            CurIcon->hbmMask = NULL;
        }
        if (bmpColor)
        {
            GreSetObjectOwner(bmpColor, GDI_OBJ_HMGR_POWNED);
            NT_VERIFY(GreDeleteObject(bmpColor) == TRUE);
            CurIcon->hbmColor = NULL;
        }
        if (bmpAlpha)
        {
            GreSetObjectOwner(bmpAlpha, GDI_OBJ_HMGR_POWNED);
            NT_VERIFY(GreDeleteObject(bmpAlpha) == TRUE);
            CurIcon->hbmAlpha = NULL;
        }
    }
    else
    {
        PACON AniCurIcon = (PACON)CurIcon;
        UINT i;

        for (i = 0; i < AniCurIcon->cpcur; i++)
        {
            NT_VERIFY(UserDereferenceObject(AniCurIcon->aspcur[i]) == TRUE);
            NT_VERIFY(IntDestroyCurIconObject(AniCurIcon->aspcur[i]) == TRUE);
        }
        ExFreePoolWithTag(AniCurIcon->aspcur, USERTAG_CURSOR);
    }

    if (CurIcon->CURSORF_flags & CURSORF_LRSHARED)
    {
        if (!IS_INTRESOURCE(CurIcon->strName.Buffer))
            ExFreePoolWithTag(CurIcon->strName.Buffer, TAG_STRING);
        if (CurIcon->atomModName)
            RtlDeleteAtomFromAtomTable(gAtomTable, CurIcon->atomModName);
        CurIcon->strName.Buffer = NULL;
        CurIcon->atomModName = 0;
    }

    /* Finally free the thing */
    FreeProcMarkObject(CurIcon);
}

VOID FASTCALL
IntCleanupCurIconCache(PPROCESSINFO Win32Process)
{
    PCURICON_OBJECT CurIcon;

    /* Run through the list of icon objects */
    while (Win32Process->pCursorCache)
    {
        CurIcon = Win32Process->pCursorCache;
        Win32Process->pCursorCache = CurIcon->pcurNext;
        UserDereferenceObject(CurIcon);
    }
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetIconInfo(
  _In_       HANDLE hCurIcon,
  _Out_opt_  PICONINFO IconInfo,
  _Out_opt_  PUNICODE_STRING lpModule,   // Optional
  _Out_opt_  PUNICODE_STRING lpResName,  // Optional
  _Out_opt_  LPDWORD pbpp,               // Optional
  _In_       BOOL bInternal)
{
    ICONINFO ii;
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret = FALSE;
    DWORD colorBpp = 0;

    TRACE("Enter NtUserGetIconInfo\n");

    /* Check if something was actually asked */
    if (!IconInfo && !lpModule && !lpResName)
    {
        WARN("Nothing to fill.\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        WARN("UserGetIconObject(0x%p) Failed.\n", hCurIcon);
        UserLeave();
        return FALSE;
    }

    /* Give back the icon information */
    if (IconInfo)
    {
        PCURICON_OBJECT FrameCurIcon = CurIcon;
        if (CurIcon->CURSORF_flags & CURSORF_ACON)
        {
            /* Get information from first frame. */
            FrameCurIcon = ((PACON)CurIcon)->aspcur[0];
        }

        /* Fill data */
        ii.fIcon = is_icon(FrameCurIcon);
        ii.xHotspot = FrameCurIcon->xHotspot;
        ii.yHotspot = FrameCurIcon->yHotspot;

        /* Copy bitmaps */
        ii.hbmMask = BITMAP_CopyBitmap(FrameCurIcon->hbmMask);
        GreSetObjectOwner(ii.hbmMask, GDI_OBJ_HMGR_POWNED);
        ii.hbmColor = BITMAP_CopyBitmap(FrameCurIcon->hbmColor);
        GreSetObjectOwner(ii.hbmColor, GDI_OBJ_HMGR_POWNED);
        colorBpp = FrameCurIcon->bpp;

        /* Copy fields */
        _SEH2_TRY
        {
            ProbeForWrite(IconInfo, sizeof(ICONINFO), 1);
            RtlCopyMemory(IconInfo, &ii, sizeof(ICONINFO));

            if (pbpp)
            {
                ProbeForWrite(pbpp, sizeof(DWORD), 1);
                *pbpp = colorBpp;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if (!NT_SUCCESS(Status))
        {
            WARN("Status: 0x%08x.\n", Status);
            SetLastNtError(Status);
            goto leave;
        }
    }

    /* Give back the module name */
    if (lpModule)
    {
        ULONG BufLen = 0;
        if (!CurIcon->atomModName)
            goto leave;

        RtlQueryAtomInAtomTable(gAtomTable, CurIcon->atomModName, NULL, NULL, NULL, &BufLen);
        /* Get the module name from the atom table */
        _SEH2_TRY
        {
            BufLen += sizeof(WCHAR);
            if (BufLen > (lpModule->MaximumLength))
            {
                lpModule->Length = 0;
                lpModule->MaximumLength = BufLen;
            }
            else
            {
                ProbeForWrite(lpModule->Buffer, lpModule->MaximumLength, 1);
                BufLen = lpModule->MaximumLength;
                RtlQueryAtomInAtomTable(gAtomTable, CurIcon->atomModName, NULL, NULL, lpModule->Buffer, &BufLen);
                lpModule->Length = BufLen;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto leave;
        }
    }

    if (lpResName)
    {
        if (!CurIcon->strName.Buffer)
            goto leave;

        /* Copy it */
        _SEH2_TRY
        {
            ProbeForWrite(lpResName, sizeof(UNICODE_STRING), 1);
            if (IS_INTRESOURCE(CurIcon->strName.Buffer))
            {
                lpResName->Buffer = CurIcon->strName.Buffer;
                lpResName->Length = 0;
                lpResName->MaximumLength = 0;
            }
            else if (lpResName->MaximumLength < CurIcon->strName.MaximumLength)
            {
                lpResName->Length = 0;
                lpResName->MaximumLength = CurIcon->strName.MaximumLength;
            }
            else
            {
                ProbeForWrite(lpResName->Buffer, lpResName->MaximumLength, 1);
                RtlCopyMemory(lpResName->Buffer, CurIcon->strName.Buffer, CurIcon->strName.Length);
                lpResName->Length = CurIcon->strName.Length;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        goto leave;
    }

    Ret = TRUE;

leave:
    UserDereferenceObject(CurIcon);

    TRACE("Leave NtUserGetIconInfo, ret=%i\n", Ret);
    UserLeave();

    return Ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetIconSize(
    HANDLE hCurIcon,
    UINT istepIfAniCur,
    PLONG plcx,       // &size.cx
    PLONG plcy)       // &size.cy
{
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL bRet = FALSE;

    TRACE("Enter NtUserGetIconSize\n");
    UserEnterShared();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        goto cleanup;
    }

    if (CurIcon->CURSORF_flags & CURSORF_ACON)
    {
        /* Use first frame for animated cursors */
        PACON AniCurIcon = (PACON)CurIcon;
        CurIcon = AniCurIcon->aspcur[0];
        UserDereferenceObject(AniCurIcon);
        UserReferenceObject(CurIcon);
    }

    _SEH2_TRY
    {
        ProbeForWrite(plcx, sizeof(LONG), 1);
        *plcx = CurIcon->cx;
        ProbeForWrite(plcy, sizeof(LONG), 1);
        *plcy = CurIcon->cy;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (NT_SUCCESS(Status))
        bRet = TRUE;
    else
        SetLastNtError(Status); // Maybe not, test this

    UserDereferenceObject(CurIcon);

cleanup:
    TRACE("Leave NtUserGetIconSize, ret=%i\n", bRet);
    UserLeave();
    return bRet;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetCursorInfo(
    PCURSORINFO pci)
{
    CURSORINFO SafeCi;
    PSYSTEM_CURSORINFO CurInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    PCURICON_OBJECT CurIcon;
    BOOL Ret = FALSE;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserGetCursorInfo\n");
    UserEnterShared();

    CurInfo = IntGetSysCursorInfo();
    CurIcon = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;

    SafeCi.cbSize = sizeof(CURSORINFO);
    SafeCi.flags = ((CurIcon && CurInfo->ShowingCursor >= 0) ? CURSOR_SHOWING : 0);
    SafeCi.hCursor = (CurIcon ? CurIcon->head.h : NULL);

    SafeCi.ptScreenPos = gpsi->ptCursor;

    _SEH2_TRY
    {
        if (pci->cbSize == sizeof(CURSORINFO))
        {
            ProbeForWrite(pci, sizeof(CURSORINFO), 1);
            RtlCopyMemory(pci, &SafeCi, sizeof(CURSORINFO));
            Ret = TRUE;
        }
        else
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
    }

    RETURN(Ret);

CLEANUP:
    TRACE("Leave NtUserGetCursorInfo, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

BOOL
APIENTRY
UserClipCursor(
    RECTL *prcl)
{
    PSYSTEM_CURSORINFO CurInfo;
    PWND DesktopWindow = NULL;

    if (!CheckWinstaAttributeAccess(WINSTA_WRITEATTRIBUTES))
    {
        return FALSE;
    }

    CurInfo = IntGetSysCursorInfo();

    DesktopWindow = UserGetDesktopWindow();

    if (prcl != NULL && DesktopWindow != NULL)
    {
        if (prcl->right < prcl->left || prcl->bottom < prcl->top)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        CurInfo->bClipped = TRUE;

        /* Set nw cliping region. Note: we can't use RECTL_bIntersectRect because
           it sets rect to 0 0 0 0 when it's empty. For more info see monitor winetest */
        CurInfo->rcClip.left = max(prcl->left, DesktopWindow->rcWindow.left);
        CurInfo->rcClip.right = min(prcl->right, DesktopWindow->rcWindow.right);
        if (CurInfo->rcClip.right < CurInfo->rcClip.left)
            CurInfo->rcClip.right = CurInfo->rcClip.left;

        CurInfo->rcClip.top = max(prcl->top, DesktopWindow->rcWindow.top);
        CurInfo->rcClip.bottom = min(prcl->bottom, DesktopWindow->rcWindow.bottom);
        if (CurInfo->rcClip.bottom < CurInfo->rcClip.top)
            CurInfo->rcClip.bottom = CurInfo->rcClip.top;

        /* Make sure cursor is in clipping region */
        UserSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y, 0, 0, FALSE);
    }
    else
    {
        CurInfo->bClipped = FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserClipCursor(
    RECTL *prcl)
{
    RECTL rclLocal;
    BOOL bResult;

    if (prcl)
    {
        _SEH2_TRY
        {
            /* Probe and copy rect */
            ProbeForRead(prcl, sizeof(RECTL), 1);
            rclLocal = *prcl;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            EngSetLastError(ERROR_INVALID_PARAMETER);
            _SEH2_YIELD(return FALSE;)
        }
        _SEH2_END

        prcl = &rclLocal;
    }

    UserEnterExclusive();

    /* Call the internal function */
    bResult = UserClipCursor(prcl);

    UserLeave();

    return bResult;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserDestroyCursor(
  _In_   HANDLE hCurIcon,
  _In_   BOOL bForce)
{
    BOOL ret;
    PCURICON_OBJECT CurIcon = NULL;

    TRACE("Enter NtUserDestroyCursorIcon (%p, %i)\n", hCurIcon, bForce);
    UserEnterExclusive();

    CurIcon = UserGetCurIconObject(hCurIcon);
    if (!CurIcon)
    {
        ret = FALSE;
        goto leave;
    }

    if (!bForce)
    {
        /* Can not destroy global objects */
        if (CurIcon->head.ppi == NULL)
        {
           ERR("Trying to delete global cursor!\n");
           ret = TRUE;
           goto leave;
        }

        /* Maybe we have good reasons not to destroy this object */
        if (CurIcon->head.ppi != PsGetCurrentProcessWin32Process())
        {
            /* No way, you're not touching my cursor */
            ret = FALSE;
            goto leave;
        }

        if (CurIcon->CURSORF_flags & CURSORF_CURRENT)
        {
            WARN("Trying to delete current cursor!\n");
            ret = FALSE;
            goto leave;
        }

        if (CurIcon->CURSORF_flags & CURSORF_LRSHARED)
        {
            WARN("Trying to delete shared cursor.\n");
            /* This one is not an error */
            ret = TRUE;
            goto leave;
        }
    }

    /* Destroy the handle */
    ret = IntDestroyCurIconObject(CurIcon);

leave:
    if (CurIcon)
        UserDereferenceObject(CurIcon);
    TRACE("Leave NtUserDestroyCursorIcon, ret=%i\n", ret);
    UserLeave();
    return ret;
}


/*
 * @implemented
 */
HICON
NTAPI
NtUserFindExistingCursorIcon(
  _In_  PUNICODE_STRING pustrModule,
  _In_  PUNICODE_STRING pustrRsrc,
  _In_  FINDEXISTINGCURICONPARAM* param)
{
    PCURICON_OBJECT CurIcon;
    HICON Ret = NULL;
    UNICODE_STRING ustrModuleSafe, ustrRsrcSafe;
    FINDEXISTINGCURICONPARAM paramSafe;
    NTSTATUS Status;
    PPROCESSINFO pProcInfo = PsGetCurrentProcessWin32Process();
    RTL_ATOM atomModName;

    TRACE("Enter NtUserFindExistingCursorIcon\n");

    _SEH2_TRY
    {
        ProbeForRead(param, sizeof(*param), 1);
        RtlCopyMemory(&paramSafe, param, sizeof(paramSafe));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    /* Capture resource name (it can be an INTRESOURCE == ATOM) */
    Status = ProbeAndCaptureUnicodeStringOrAtom(&ustrRsrcSafe, pustrRsrc);
    if (!NT_SUCCESS(Status))
        return NULL;
    Status = ProbeAndCaptureUnicodeString(&ustrModuleSafe, UserMode, pustrModule);
    if (!NT_SUCCESS(Status))
        goto done;
    Status = RtlLookupAtomInAtomTable(gAtomTable, ustrModuleSafe.Buffer, &atomModName);
    ReleaseCapturedUnicodeString(&ustrModuleSafe, UserMode);
    if (!NT_SUCCESS(Status))
    {
        /* The module is not in the atom table. No chance to find the cursor */
        goto done;
    }

    UserEnterShared();
    CurIcon = pProcInfo->pCursorCache;
    while (CurIcon)
    {
        /* Icon/cursor */
        if (paramSafe.bIcon != is_icon(CurIcon))
        {
            CurIcon = CurIcon->pcurNext;
            continue;
        }
        /* See if module names match */
        if (atomModName == CurIcon->atomModName)
        {
            /* Check if this is an INTRESOURCE */
            if (IS_INTRESOURCE(CurIcon->strName.Buffer))
            {
                /* Compare if it matches the one we are looking for. This also
                   handles the case, where ustrRsrcSafe is not an INTRESOURCE */
                if (CurIcon->strName.Buffer == ustrRsrcSafe.Buffer)
                {
                    /* INT resources match */
                    break;
                }
            }
            else if (RtlCompareUnicodeString(&ustrRsrcSafe, &CurIcon->strName, TRUE) == 0)
            {
                /* Resource name strings match */
                break;
            }
        }
        CurIcon = CurIcon->pcurNext;
    }

    /* Now search Global Cursors or Icons. */
    if (CurIcon == NULL)
    {
        CurIcon = gcurFirst;
        while (CurIcon)
        {
            /* Icon/cursor */
            if (paramSafe.bIcon != is_icon(CurIcon))
            {
                CurIcon = CurIcon->pcurNext;
                continue;
            }
            /* See if module names match */
            if (atomModName == CurIcon->atomModName)
            {
                /* They do. Now see if this is the same resource */
                if (IS_INTRESOURCE(CurIcon->strName.Buffer) != IS_INTRESOURCE(ustrRsrcSafe.Buffer))
                {
                    /* One is an INT resource and the other is not -> no match */
                    CurIcon = CurIcon->pcurNext;
                    continue;
                }
                if (IS_INTRESOURCE(CurIcon->strName.Buffer))
                {
                    if (CurIcon->strName.Buffer == ustrRsrcSafe.Buffer)
                    {
                        /* INT resources match */
                        break;
                    }
                }
                else if (RtlCompareUnicodeString(&ustrRsrcSafe, &CurIcon->strName, TRUE) == 0)
                {
                    /* Resource name strings match */
                    break;
                }
            }
            CurIcon = CurIcon->pcurNext;
        }
    }
    if (CurIcon)
        Ret = CurIcon->head.h;
    UserLeave();

done:
    if (!IS_INTRESOURCE(ustrRsrcSafe.Buffer))
        ExFreePoolWithTag(ustrRsrcSafe.Buffer, TAG_STRING);

    return Ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetClipCursor(
    RECTL *lpRect)
{
    PSYSTEM_CURSORINFO CurInfo;
    RECTL Rect;
    NTSTATUS Status;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserGetClipCursor\n");
    UserEnterShared();

    if (!CheckWinstaAttributeAccess(WINSTA_READATTRIBUTES))
    {
        RETURN(FALSE);
    }

    if (!lpRect)
        RETURN(FALSE);

    CurInfo = IntGetSysCursorInfo();
    if (CurInfo->bClipped)
    {
        Rect = CurInfo->rcClip;
    }
    else
    {
        Rect.left = 0;
        Rect.top = 0;
        Rect.right = UserGetSystemMetrics(SM_CXSCREEN);
        Rect.bottom = UserGetSystemMetrics(SM_CYSCREEN);
    }

    Status = MmCopyToCaller(lpRect, &Rect, sizeof(RECT));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        RETURN(FALSE);
    }

    RETURN(TRUE);

CLEANUP:
    TRACE("Leave NtUserGetClipCursor, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}


/*
 * @implemented
 */
HCURSOR
APIENTRY
NtUserSetCursor(
    HCURSOR hCursor)
{
    PCURICON_OBJECT pcurOld, pcurNew;
    HCURSOR hOldCursor = NULL;

    TRACE("Enter NtUserSetCursor: %p\n", hCursor);
    UserEnterExclusive();

    if (hCursor)
    {
        pcurNew = UserGetCurIconObject(hCursor);
        if (!pcurNew)
        {
            EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
            goto leave;
        }
        pcurNew->CURSORF_flags |= CURSORF_CURRENT;
    }
    else
    {
        pcurNew = NULL;
    }

    pcurOld = UserSetCursor(pcurNew, FALSE);
    if (pcurOld)
    {
        hOldCursor = pcurOld->head.h;
    /*
        Problem:

        System Global Cursors start out having at least 2 lock counts. If a system
        cursor is the default cursor and is returned to the caller twice in its
        life, the count will reach zero. Causing an assert to occur in objects.

        This fixes a SeaMonkey crash while the mouse crosses a boundary.
     */
        if (pcurOld->CURSORF_flags & CURSORF_GLOBAL)
        {
           TRACE("Returning Global Cursor hcur %p\n",hOldCursor);
           goto leave;
        }

        /* See if it was destroyed in the meantime */
        if (UserObjectInDestroy(hOldCursor))
            hOldCursor = NULL;
        pcurOld->CURSORF_flags &= ~CURSORF_CURRENT;
        UserDereferenceObject(pcurOld);
    }

leave:
    UserLeave();
    return hOldCursor;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserSetCursorContents(
    HANDLE hCurIcon,
    PICONINFO UnsafeIconInfo)
{
    FIXME(" is UNIMPLEMENTED.\n");
    return FALSE;
}


static
BOOL
IntSetCursorData(
    _Inout_ PCURICON_OBJECT pcur,
    _In_opt_ PUNICODE_STRING pustrName,
    _In_ ATOM atomModName,
    _In_ const CURSORDATA* pcursordata)
{
    NT_ASSERT((pcur->CURSORF_flags & CURSORF_ACON) == 0);

    /* Check if the CURSORF_ACON is also set in the cursor data */
    if (pcursordata->CURSORF_flags & CURSORF_ACON)
    {
        ERR("Mismatch in CURSORF_flags! cursor: 0x%08lx, data:  0x%08lx\n",
            pcur->CURSORF_flags, pcursordata->CURSORF_flags);
        return FALSE;
    }

    /* Check if this cursor was already set */
    if (pcur->hbmMask != NULL)
    {
        ERR("Cursor data already set!\n");
        return FALSE;
    }

    /* We need a mask */
    if (pcursordata->hbmMask == NULL)
    {
        ERR("NtUserSetCursorIconData was got no hbmMask.\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    /* Take ownership of the mask bitmap */
    if (!GreSetObjectOwner(pcursordata->hbmMask, GDI_OBJ_HMGR_PUBLIC))
    {
        goto Cleanup;
    }

    /* Check if we have a color bitmap */
    if (pcursordata->hbmColor)
    {
        /* Take ownership of the color bitmap */
        if (!GreSetObjectOwner(pcursordata->hbmColor, GDI_OBJ_HMGR_PUBLIC))
        {
            goto Cleanup;
        }
    }

    /* Check if we have an alpha bitmap */
    if (pcursordata->hbmAlpha)
    {
        /* Take ownership of the alpha bitmap */
        if (!GreSetObjectOwner(pcursordata->hbmAlpha, GDI_OBJ_HMGR_PUBLIC))
        {
            goto Cleanup;
        }
    }

    /* Free the old name */
    if (pcur->strName.Buffer != NULL)
    {
        ExFreePoolWithTag(pcur->strName.Buffer, TAG_STRING);
        pcur->strName.Buffer = NULL;
        pcur->strName.Length = 0;
        pcur->strName.MaximumLength = 0;
    }

    /* Free the module atom */
    if (pcur->atomModName != 0)
    {
        NT_VERIFY(NT_SUCCESS(RtlDeleteAtomFromAtomTable(gAtomTable, pcur->atomModName)));
    }

    /* Now set the new cursor data */
    pcur->atomModName = atomModName;
    pcur->rt = pcursordata->rt;
    pcur->CURSORF_flags = pcursordata->CURSORF_flags & CURSORF_USER_MASK;
    pcur->xHotspot = pcursordata->xHotspot;
    pcur->yHotspot = pcursordata->yHotspot;
    pcur->hbmMask = pcursordata->hbmMask;
    pcur->hbmColor = pcursordata->hbmColor;
    pcur->hbmAlpha = pcursordata->hbmAlpha;
    pcur->rcBounds.left = 0;
    pcur->rcBounds.top = 0;
    pcur->rcBounds.right = pcursordata->cx;
    pcur->rcBounds.bottom = pcursordata->cy;
    pcur->hbmUserAlpha = pcursordata->hbmUserAlpha;
    pcur->bpp = pcursordata->bpp;
    pcur->cx = pcursordata->cx;
    pcur->cy = pcursordata->cy;
    if (pustrName != NULL)
    {
        pcur->strName = *pustrName;
    }

    return TRUE;

Cleanup:

    if (pcursordata->hbmMask != NULL)
    {
        GreSetObjectOwner(pcursordata->hbmMask, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pcursordata->hbmMask);
    }

    if (pcursordata->hbmColor != NULL)
    {
        GreSetObjectOwner(pcursordata->hbmColor, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pcursordata->hbmColor);
    }

    if (pcursordata->hbmAlpha != NULL)
    {
        GreSetObjectOwner(pcursordata->hbmAlpha, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pcursordata->hbmAlpha);
    }

    return FALSE;
}

static
BOOL
IntSetAconData(
    _Inout_ PACON pacon,
    _In_opt_ PUNICODE_STRING pustrName,
    _In_ ATOM atomModName,
    _In_ PCURSORDATA pcursordata)
{
    PCURICON_OBJECT *aspcur;
    DWORD *aicur;
    INT *ajifRate;
    PCURSORDATA pcdFrame;
    HCURSOR hcurFrame;
    UINT cjSize, i;

    NT_ASSERT((pacon->CURSORF_flags & CURSORF_ACON) != 0);
    NT_ASSERT((ULONG_PTR)pcursordata->aspcur > MmUserProbeAddress);
    NT_ASSERT((ULONG_PTR)pcursordata->aicur > MmUserProbeAddress);
    NT_ASSERT((ULONG_PTR)pcursordata->ajifRate > MmUserProbeAddress);
    NT_ASSERT(pcursordata->cpcur > 0);
    NT_ASSERT(pcursordata->cicur > 0);

    /* Check if the CURSORF_ACON is also set in the cursor data */
    if (!(pcursordata->CURSORF_flags & CURSORF_ACON))
    {
        ERR("Mismatch in CURSORF_flags! acon: 0x%08lx, data:  0x%08lx\n",
            pacon->CURSORF_flags, pcursordata->CURSORF_flags);
        return FALSE;
    }

    /* Check if this acon was already set */
    if (pacon->aspcur != NULL)
    {
        ERR("Acon data already set!\n");
        return FALSE;
    }

    /* Calculate size: one cursor object for each frame, and a frame
       index and jiffies for each "step" */
    cjSize = (pcursordata->cpcur * sizeof(CURICON_OBJECT*)) +
             (pcursordata->cicur * sizeof(DWORD)) +
             (pcursordata->cicur * sizeof(INT));

    /* Allocate a buffer */
    aspcur = ExAllocatePoolWithTag(PagedPool, cjSize, USERTAG_CURSOR);
    if (aspcur == NULL)
    {
        ERR("Failed to allocate memory (cpcur = %u, cicur = %u)\n",
            pcursordata->cpcur, pcursordata->cicur);
        return FALSE;
    }

    /* Set the pointers */
    aicur = (DWORD*)&aspcur[pcursordata->cpcur];
    ajifRate = (INT*)&aicur[pcursordata->cicur];

    /* Get a pointer to the cursor data for each frame */
    pcdFrame = pcursordata->aspcur;

    /* Create the cursors */
    for (i = 0; i < pcursordata->cpcur; i++)
    {
        /* Create a cursor for this frame */
        hcurFrame = IntCreateCurIconHandle(FALSE);
        if (hcurFrame == NULL)
        {
            ERR("Failed to create a cursor for frame %u\n", i);
            aspcur[i] = NULL;
            goto Cleanup;
        }

        /* Get a pointer to the frame cursor */
        aspcur[i] = UserGetCurIconObject(hcurFrame);
        NT_ASSERT(aspcur[i] != NULL);

        /* Mark this cursor as an acon frame */
        pcdFrame->CURSORF_flags |= CURSORF_ACONFRAME;

        /* Set the cursor data for this frame */
        if (!IntSetCursorData(aspcur[i], NULL, 0, &pcdFrame[i]))
        {
            ERR("Failed to set cursor data for frame %u\n", i);
            goto Cleanup;
        }
    }

    /* Free the old name */
    if (pacon->strName.Buffer != NULL)
    {
        ExFreePoolWithTag(pacon->strName.Buffer, TAG_STRING);
        pacon->strName.Buffer = NULL;
        pacon->strName.Length = 0;
        pacon->strName.MaximumLength = 0;
    }

    /* Free the module atom */
    if (pacon->atomModName != 0)
    {
        NT_VERIFY(NT_SUCCESS(RtlDeleteAtomFromAtomTable(gAtomTable, pacon->atomModName)));
    }

    /* Free the previous frames */
    if (pacon->aspcur != NULL)
    {
        for (i = 0; i < pacon->cpcur; i++)
        {
            NT_VERIFY(UserDereferenceObject(pacon->aspcur[i]) == TRUE);
            NT_VERIFY(IntDestroyCurIconObject(pacon->aspcur[i]) == TRUE);
        }
        ExFreePoolWithTag(pacon->aspcur, USERTAG_CURSOR);
    }

    /* Finally set the data in the acon */
    pacon->atomModName = atomModName;
    pacon->rt = pcursordata->rt;
    pacon->CURSORF_flags = pcursordata->CURSORF_flags & CURSORF_USER_MASK;
    pacon->cpcur = pcursordata->cpcur;
    pacon->cicur = pcursordata->cicur;
    pacon->aspcur = aspcur;
    pacon->aicur = aicur;
    pacon->ajifRate = ajifRate;
    pacon->iicur = 0;
    if (pustrName != NULL)
    {
        pacon->strName = *pustrName;
    }

    return TRUE;

Cleanup:

    /* Clean up the cursors we created */
    for (i = 0; i < pcursordata->cpcur; i++)
    {
        if (aspcur[i] == NULL)
            break;

        /* Destroy this cursor */
        NT_VERIFY(UserDereferenceObject(aspcur[i]) == TRUE);
        NT_VERIFY(IntDestroyCurIconObject(aspcur[i]) == TRUE);
    }

    /* Delete the allocated structure */
    ExFreePoolWithTag(aspcur, USERTAG_CURSOR);

    return FALSE;
}

BOOL
APIENTRY
UserSetCursorIconData(
    _In_ HCURSOR hcursor,
    _In_opt_ PUNICODE_STRING pustrModule,
    _In_opt_ PUNICODE_STRING pustrRsrc,
    _In_ PCURSORDATA pcursordata)
{
    PCURICON_OBJECT pcur;
    ATOM atomModName;
    NTSTATUS status;
    BOOL bResult;

    /* Do we have a module name? */
    if (pustrModule != NULL)
    {
        /* Create an atom for the module name */
        status = RtlAddAtomToAtomTable(gAtomTable,
                                       pustrModule->Buffer,
                                       &atomModName);
        if (!NT_SUCCESS(status))
        {
            ERR("Failed to create atom from module name '%wZ': %0x8lx\n",
                pustrModule, status);
            return FALSE;
        }
    }
    else
    {
        /* No module name atom */
        atomModName = 0;
    }

    /* Reference the cursor */
    pcur = UserGetCurIconObject(hcursor);
    if (pcur == NULL)
    {
        ERR("Failed to reference cursor %p\n", hcursor);
        bResult = FALSE;
        goto Exit;
    }

    /* Check if this is an acon */
    if (pcur->CURSORF_flags & CURSORF_ACON)
    {
        bResult = IntSetAconData((PACON)pcur,
                                 pustrRsrc,
                                 atomModName,
                                 pcursordata);
    }
    else
    {
        bResult = IntSetCursorData(pcur,
                                   pustrRsrc,
                                   atomModName,
                                   pcursordata);
    }

Exit:

    /* Check if we had success */
    if (bResult != FALSE)
    {
        /* Check if this is an LRSHARED cursor now */
        if (pcur->CURSORF_flags & CURSORF_LRSHARED)
        {
            /* Insert the cursor into the list. */
            IntInsertCursorIntoList(pcur);
        }
    }
    else
    {
        /* Cleanup on failure */
        if (atomModName != 0)
        {
            NT_VERIFY(NT_SUCCESS(RtlDeleteAtomFromAtomTable(gAtomTable, atomModName)));
        }
    }

    /* Dereference the cursor and return the result */
    UserDereferenceObject(pcur);
    return bResult;
}


/*
 * @implemented
 */
__kernel_entry
BOOL
APIENTRY
NtUserSetCursorIconData(
    _In_ HCURSOR hcursor,
    _In_opt_ PUNICODE_STRING pustrModule,
    _In_opt_ PUNICODE_STRING pustrRsrc,
    _In_ const CURSORDATA* pCursorData)
{
    CURSORDATA cursordata;
    UNICODE_STRING ustrModule, ustrRsrc;
    _SEH2_VOLATILE PVOID pvBuffer;
    UINT cjSize;
    NTSTATUS status;
    BOOL bResult;

    TRACE("Enter NtUserSetCursorIconData\n");

    /* Initialize buffer, so we can handle cleanup */
    ustrRsrc.Buffer = NULL;
    ustrModule.Buffer = NULL;
    pvBuffer = NULL;

    _SEH2_TRY
    {
        /* Probe and capture the cursor data structure */
        ProbeForRead(pCursorData, sizeof(*pCursorData), 1);
        cursordata = *pCursorData;

        /* Check if this is an animated cursor */
        if (cursordata.CURSORF_flags & CURSORF_ACON)
        {
            /* Check of the range is ok */
            if ((cursordata.cpcur == 0) || (cursordata.cicur == 0) ||
                (cursordata.cpcur > 1000) || (cursordata.cicur > 1000))
            {
                ERR("Range error (cpcur = %u, cicur = %u)\n",
                    cursordata.cpcur, cursordata.cicur);
                bResult = FALSE;
                goto Exit;
            }

            /* Calculate size: one cursor data structure for each frame,
               and a frame index and jiffies for each "step" */
            cjSize = (cursordata.cpcur * sizeof(CURSORDATA)) +
                     (cursordata.cicur * sizeof(DWORD)) +
                     (cursordata.cicur * sizeof(INT));

            /* Allocate a buffer */
            pvBuffer = ExAllocatePoolWithTag(PagedPool, cjSize, USERTAG_CURSOR);
            if (pvBuffer == NULL)
            {
                ERR("Failed to allocate memory (cpcur = %u, cicur = %u)\n",
                    cursordata.cpcur, cursordata.cicur);
                bResult = FALSE;
                goto Exit;
            }

            /* Set the pointers */
            cursordata.aspcur = (CURSORDATA*)pvBuffer;
            cursordata.aicur = (DWORD*)&cursordata.aspcur[cursordata.cpcur];
            cursordata.ajifRate = (INT*)&cursordata.aicur[cursordata.cicur];

            /* Probe and copy aspcur */
            ProbeForRead(pCursorData->aspcur, cursordata.cpcur * sizeof(CURSORDATA), 1);
            RtlCopyMemory(cursordata.aspcur,
                          pCursorData->aspcur,
                          cursordata.cpcur * sizeof(CURSORDATA));

            /* Probe and copy aicur */
            ProbeForRead(pCursorData->aicur, cursordata.cicur * sizeof(DWORD), 1);
            RtlCopyMemory(cursordata.aicur,
                          pCursorData->aicur,
                          cursordata.cicur * sizeof(DWORD));

            /* Probe and copy ajifRate */
            ProbeForRead(pCursorData->ajifRate, cursordata.cicur * sizeof(INT), 1);
            RtlCopyMemory(cursordata.ajifRate,
                          pCursorData->ajifRate,
                          cursordata.cicur * sizeof(INT));
        }
        else
        {
            /* This is a standard cursor, we don't use the pointers */
            cursordata.aspcur = NULL;
            cursordata.aicur = NULL;
            cursordata.ajifRate = NULL;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        bResult = FALSE;
        goto Exit;
    }
    _SEH2_END

    /* Check if we got a module name */
    if (pustrModule != NULL)
    {
        /* Capture the name */
        status = ProbeAndCaptureUnicodeString(&ustrModule, UserMode, pustrModule);
        if (!NT_SUCCESS(status))
        {
            ERR("Failed to copy pustrModule: status 0x%08lx\n", status);
            goto Exit;
        }
    }

    /* Check if we got a resource name */
    if (pustrRsrc != NULL)
    {
        /* We use this function, because INTRESOURCEs and ATOMs are the same */
        status = ProbeAndCaptureUnicodeStringOrAtom(&ustrRsrc, pustrRsrc);
        if (!NT_SUCCESS(status))
        {
            ERR("Failed to copy pustrRsrc: status 0x%08lx\n", status);
            goto Exit;
        }
    }

    /* Acquire the global user lock */
    UserEnterExclusive();

    /* Call the internal function */
    bResult = UserSetCursorIconData(hcursor,
                                    pustrModule ? &ustrModule : NULL,
                                    pustrRsrc ? &ustrRsrc : NULL,
                                    &cursordata);

    /* Release the global user lock */
    UserLeave();

Exit:

    /* Free the captured module name */
    if ((ustrModule.Buffer != NULL) && !IS_INTRESOURCE(ustrModule.Buffer))
    {
        ReleaseCapturedUnicodeString(&ustrModule, UserMode);
    }

    if (pvBuffer != NULL)
    {
        ExFreePoolWithTag(pvBuffer, USERTAG_CURSOR);
    }

    /* Additional cleanup on failure */
    if (bResult == FALSE)
    {
        if (ustrRsrc.Buffer != NULL)
        {
            ExFreePoolWithTag(ustrRsrc.Buffer, TAG_STRING);
        }
    }

    TRACE("Leave NtUserSetCursorIconData, bResult = %i\n", bResult);

    return bResult;
}

/* Mostly inspired from wine code.
 * We use low level functions because:
 *  - at this point, the icon bitmap could have a different bit depth than the DC,
 *    making it thus impossible to use NtCreateCompatibleDC and selecting the bitmap.
 *    This happens after a mode setting change.
 *  - it avoids massive GDI objects locking when only the destination surface needs it.
 *  - It makes (small) performance gains.
 */
BOOL
UserDrawIconEx(
    HDC hDc,
    INT xLeft,
    INT yTop,
    PCURICON_OBJECT pIcon,
    INT cxWidth,
    INT cyHeight,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags)
{
    PSURFACE psurfDest, psurfMask, psurfColor; //, psurfOffScreen = NULL;
    PDC pdc = NULL;
    BOOL Ret = FALSE;
    HBITMAP hbmMask, hbmColor, hbmAlpha;
    BOOL bOffScreen;
    RECTL rcDest, rcSrc;
    CLIPOBJ* pdcClipObj = NULL;
    EXLATEOBJ exlo;

    /* Stupid case */
    if ((diFlags & DI_NORMAL) == 0)
    {
        ERR("DrawIconEx called without mask or color bitmap to draw.\n");
        return FALSE;
    }

    if (pIcon->CURSORF_flags & CURSORF_ACON)
    {
        ACON* pAcon = (ACON*)pIcon;
        if (istepIfAniCur >= pAcon->cicur)
        {
            ERR("NtUserDrawIconEx: istepIfAniCur too big!\n");
            return FALSE;
        }
        pIcon = pAcon->aspcur[pAcon->aicur[istepIfAniCur]];
    }

    hbmMask = pIcon->hbmMask;
    hbmColor = pIcon->hbmColor;
    hbmAlpha = pIcon->hbmAlpha;

    /*
     * Get our objects.
     * Shared locks are enough, we are only reading those bitmaps
     */
    psurfMask = SURFACE_ShareLockSurface(hbmMask);
    if (psurfMask == NULL)
    {
        ERR("Unable to lock the mask surface.\n");
        return FALSE;
    }

    /* Color bitmap is not mandatory */
    if (hbmColor == NULL)
    {
        /* But then the mask bitmap must have the information in it's bottom half */
        ASSERT(psurfMask->SurfObj.sizlBitmap.cy == 2*pIcon->cy);
        psurfColor = NULL;
    }
    else if ((psurfColor = SURFACE_ShareLockSurface(hbmColor)) == NULL)
    {
        ERR("Unable to lock the color bitmap.\n");
        SURFACE_ShareUnlockSurface(psurfMask);
        return FALSE;
    }

    pdc = DC_LockDc(hDc);
    if (!pdc)
    {
        ERR("Could not lock the destination DC.\n");
        SURFACE_ShareUnlockSurface(psurfMask);
        if (psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
        return FALSE;
    }

    /* Fix width parameter, if needed */
    if (!cxWidth)
    {
        if (diFlags & DI_DEFAULTSIZE)
            cxWidth = is_icon(pIcon) ?
                UserGetSystemMetrics(SM_CXICON) : UserGetSystemMetrics(SM_CXCURSOR);
        else
            cxWidth = pIcon->cx;
    }

    /* Fix height parameter, if needed */
    if (!cyHeight)
    {
        if (diFlags & DI_DEFAULTSIZE)
            cyHeight = is_icon(pIcon) ?
                UserGetSystemMetrics(SM_CYICON) : UserGetSystemMetrics(SM_CYCURSOR);
        else
            cyHeight = pIcon->cy;
    }

    /* Calculate destination rectangle */
    RECTL_vSetRect(&rcDest, xLeft, yTop, xLeft + cxWidth, yTop + cyHeight);
    IntLPtoDP(pdc, (LPPOINT)&rcDest, 2);
    RECTL_vOffsetRect(&rcDest, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y);

    /* Prepare the underlying surface */
    DC_vPrepareDCsForBlit(pdc, &rcDest, NULL, NULL);

    /* We now have our destination surface and rectangle */
    psurfDest = pdc->dclevel.pSurface;

    if (psurfDest == NULL)
    {
        /* Empty DC */
        DC_vFinishBlit(pdc, NULL);
        DC_UnlockDc(pdc);
        SURFACE_ShareUnlockSurface(psurfMask);
        if (psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
        return FALSE;
    }

    /* Set source rect */
    RECTL_vSetRect(&rcSrc, 0, 0, pIcon->cx, pIcon->cy);

    /* Should we render off-screen? */
    bOffScreen = hbrFlickerFreeDraw &&
        (GDI_HANDLE_GET_TYPE(hbrFlickerFreeDraw) == GDI_OBJECT_TYPE_BRUSH);

    if (bOffScreen)
    {
        /* Yes: Allocate and paint the offscreen surface */
        EBRUSHOBJ eboFill;
        PBRUSH pbrush = BRUSH_ShareLockBrush(hbrFlickerFreeDraw);

        TRACE("Performing off-screen rendering.\n");

        if (!pbrush)
        {
            ERR("Failed to get brush object.\n");
            goto Cleanup;
        }

#if 0 //We lock the hdc surface during the whole function it makes no sense to use an offscreen surface for "flicker free" drawing
        psurfOffScreen = SURFACE_AllocSurface(STYPE_BITMAP,
            cxWidth, cyHeight, psurfDest->SurfObj.iBitmapFormat,
            0, 0, NULL);
        if (!psurfOffScreen)
        {
            ERR("Failed to allocate the off-screen surface.\n");
            BRUSH_ShareUnlockBrush(pbrush);
            goto Cleanup;
        }

        /* Paint the brush */
        EBRUSHOBJ_vInit(&eboFill, pbrush, psurfOffScreen, 0x00FFFFFF, 0, NULL);
        RECTL_vSetRect(&rcDest, 0, 0, cxWidth, cyHeight);

        Ret = IntEngBitBlt(&psurfOffScreen->SurfObj,
            NULL,
            NULL,
            NULL,
            NULL,
            &rcDest,
            NULL,
            NULL,
            &eboFill.BrushObject,
            &pbrush->ptOrigin,
            ROP4_PATCOPY);

        /* Clean up everything */
        EBRUSHOBJ_vCleanup(&eboFill);
        BRUSH_ShareUnlockBrush(pbrush);

        if (!Ret)
        {
            ERR("Failed to paint the off-screen surface.\n");
            goto Cleanup;
        }

        /* We now have our destination surface */
        psurfDest = psurfOffScreen;
#else
        pdcClipObj = &pdc->co.ClipObj;
        /* Paint the brush */
        EBRUSHOBJ_vInit(&eboFill, pbrush, psurfDest, 0x00FFFFFF, 0, NULL);

        Ret = IntEngBitBlt(&psurfDest->SurfObj,
            NULL,
            NULL,
            pdcClipObj,
            NULL,
            &rcDest,
            NULL,
            NULL,
            &eboFill.BrushObject,
            &pbrush->ptOrigin,
            ROP4_PATCOPY);

        /* Clean up everything */
        EBRUSHOBJ_vCleanup(&eboFill);
        BRUSH_ShareUnlockBrush(pbrush);

        if (!Ret)
        {
            ERR("Failed to paint the off-screen surface.\n");
            goto Cleanup;
        }
#endif
    }
    else
    {
        /* We directly draw to the DC */
        TRACE("Performing on screen rendering.\n");
        pdcClipObj = &pdc->co.ClipObj;
        // psurfOffScreen = NULL;
    }

    /* Now do the rendering */
    if (hbmAlpha && ((diFlags & DI_NORMAL) == DI_NORMAL))
    {
        BLENDOBJ blendobj = { {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA } };
        PSURFACE psurf = NULL;

        psurf = SURFACE_ShareLockSurface(hbmAlpha);
        if (!psurf)
        {
            ERR("SURFACE_LockSurface failed!\n");
            goto NoAlpha;
        }

        /* Initialize color translation object */
        EXLATEOBJ_vInitialize(&exlo, psurf->ppal, psurfDest->ppal, 0xFFFFFFFF, 0xFFFFFFFF, 0);

        /* Now do it */
        Ret = IntEngAlphaBlend(&psurfDest->SurfObj,
                               &psurf->SurfObj,
                               pdcClipObj,
                               &exlo.xlo,
                               &rcDest,
                               &rcSrc,
                               &blendobj);

        EXLATEOBJ_vCleanup(&exlo);
        SURFACE_ShareUnlockSurface(psurf);
        if (Ret) goto done;
        ERR("NtGdiAlphaBlend failed!\n");
    }
NoAlpha:
    if (diFlags & DI_MASK)
    {
        DWORD rop4 = (diFlags & DI_IMAGE) ? ROP4_SRCAND : ROP4_SRCCOPY;

        EXLATEOBJ_vInitSrcMonoXlate(&exlo, psurfDest->ppal, 0x00FFFFFF, 0);

        Ret = IntEngStretchBlt(&psurfDest->SurfObj,
                               &psurfMask->SurfObj,
                               NULL,
                               pdcClipObj,
                               &exlo.xlo,
                               NULL,
                               &rcDest,
                               &rcSrc,
                               NULL,
                               NULL,
                               NULL,
                               rop4);

        EXLATEOBJ_vCleanup(&exlo);

        if (!Ret)
        {
            ERR("Failed to mask the bitmap data.\n");
            goto Cleanup;
        }
    }

    if (diFlags & DI_IMAGE)
    {
        if (psurfColor)
        {
            DWORD rop4 = (diFlags & DI_MASK) ? ROP4_SRCINVERT : ROP4_SRCCOPY ;

            EXLATEOBJ_vInitialize(&exlo, psurfColor->ppal, psurfDest->ppal, 0x00FFFFFF, 0x00FFFFFF, 0);

            Ret = IntEngStretchBlt(&psurfDest->SurfObj,
                                   &psurfColor->SurfObj,
                                   NULL,
                                   pdcClipObj,
                                   &exlo.xlo,
                                   NULL,
                                   &rcDest,
                                   &rcSrc,
                                   NULL,
                                   NULL,
                                   NULL,
                                   rop4);

            EXLATEOBJ_vCleanup(&exlo);

            if (!Ret)
            {
                ERR("Failed to render the icon bitmap.\n");
                goto Cleanup;
            }
        }
        else
        {
            /* Mask bitmap holds the information in its bottom half */
            DWORD rop4 = (diFlags & DI_MASK) ? ROP4_SRCINVERT : ROP4_SRCCOPY;
            RECTL_vOffsetRect(&rcSrc, 0, pIcon->cy);

            EXLATEOBJ_vInitSrcMonoXlate(&exlo, psurfDest->ppal, 0x00FFFFFF, 0);

            Ret = IntEngStretchBlt(&psurfDest->SurfObj,
                                   &psurfMask->SurfObj,
                                   NULL,
                                   pdcClipObj,
                                   &exlo.xlo,
                                   NULL,
                                   &rcDest,
                                   &rcSrc,
                                   NULL,
                                   NULL,
                                   NULL,
                                   rop4);

            EXLATEOBJ_vCleanup(&exlo);

            if (!Ret)
            {
                ERR("Failed to render the icon bitmap.\n");
                goto Cleanup;
            }
        }
    }

done:
#if 0
    /* We're done. Was it a double buffered draw ? */
    if (bOffScreen)
    {
        /* Yes. Draw it back to our DC */
        POINTL ptSrc = {0, 0};

        /* Calculate destination rectangle */
        RECTL_vSetRect(&rcDest, xLeft, yTop, xLeft + cxWidth, yTop + cyHeight);
        IntLPtoDP(pdc, (LPPOINT)&rcDest, 2);
        RECTL_vOffsetRect(&rcDest, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y);

        /* Get the clip object */
        pdcClipObj = pdc->rosdc.CombinedClip;

        /* We now have our destination surface and rectangle */
        psurfDest = pdc->dclevel.pSurface;

        /* Color translation */
        EXLATEOBJ_vInitialize(&exlo, psurfOffScreen->ppal, psurfDest->ppal, 0x00FFFFFF, 0x00FFFFFF, 0);

        /* Blt it! */
        Ret = IntEngBitBlt(&psurfDest->SurfObj,
                           &psurfOffScreen->SurfObj,
                           NULL,
                           pdcClipObj,
                           &exlo.xlo,
                           &rcDest,
                           &ptSrc,
                           NULL,
                           NULL,
                           NULL,
                           ROP4_SRCCOPY);

        EXLATEOBJ_vCleanup(&exlo);
    }
#endif
Cleanup:
    if (pdc)
    {
        DC_vFinishBlit(pdc, NULL);
        DC_UnlockDc(pdc);
    }

#if 0
    /* Delete off screen rendering surface */
    if (psurfOffScreen)
        GDIOBJ_vDeleteObject(&psurfOffScreen->BaseObject);
#endif

    /* Unlock other surfaces */
    SURFACE_ShareUnlockSurface(psurfMask);
    if (psurfColor) SURFACE_ShareUnlockSurface(psurfColor);

    return Ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserDrawIconEx(
    HDC hdc,
    int xLeft,
    int yTop,
    HICON hIcon,
    int cxWidth,
    int cyHeight,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags,
    BOOL bMetaHDC, // When TRUE, GDI functions need to be handled in User32!
    PVOID pDIXData)
{
    PCURICON_OBJECT pIcon;
    BOOL Ret;

    TRACE("Enter NtUserDrawIconEx\n");
    UserEnterExclusive();

    if (!(pIcon = UserGetCurIconObject(hIcon)))
    {
        ERR("UserGetCurIconObject(0x%p) failed!\n", hIcon);
        UserLeave();
        return FALSE;
    }

    Ret = UserDrawIconEx(hdc,
                         xLeft,
                         yTop,
                         pIcon,
                         cxWidth,
                         cyHeight,
                         istepIfAniCur,
                         hbrFlickerFreeDraw,
                         diFlags);

    UserDereferenceObject(pIcon);

    UserLeave();
    return Ret;
}

/*
 * @unimplemented
 */
HCURSOR
NTAPI
NtUserGetCursorFrameInfo(
    HCURSOR hCursor,
    DWORD istep,
    INT* rate_jiffies,
    DWORD* num_steps)
{
    PCURICON_OBJECT CurIcon;
    HCURSOR ret;
    INT jiffies = 0;
    DWORD steps = 1;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("Enter NtUserGetCursorFrameInfo\n");
    UserEnterShared();

    if (!(CurIcon = UserGetCurIconObject(hCursor)))
    {
        UserLeave();
        return NULL;
    }

    ret = CurIcon->head.h;

    if (CurIcon->CURSORF_flags & CURSORF_ACON)
    {
        PACON AniCurIcon = (PACON)CurIcon;
        if (istep >= AniCurIcon->cicur)
        {
            UserDereferenceObject(CurIcon);
            UserLeave();
            return NULL;
        }
        jiffies = AniCurIcon->ajifRate[istep];
        steps = AniCurIcon->cicur;
        ret = AniCurIcon->aspcur[AniCurIcon->aicur[istep]]->head.h;
    }

    _SEH2_TRY
    {
        ProbeForWrite(rate_jiffies, sizeof(INT), 1);
        ProbeForWrite(num_steps, sizeof(DWORD), 1);
        *rate_jiffies = jiffies;
        *num_steps = steps;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        WARN("Status: 0x%08x.\n", Status);
        SetLastNtError(Status);
        ret = NULL;
    }

    UserDereferenceObject(CurIcon);
    UserLeave();

    TRACE("Leaving NtUserGetCursorFrameInfo, ret = 0x%p\n", ret);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserSetSystemCursor(
    HCURSOR hcur,
    DWORD id)
{
    PCURICON_OBJECT pcur, pcurOrig = NULL;
    int i;
    PPROCESSINFO ppi;
    BOOL Ret = FALSE;
    UserEnterExclusive();

    if (!CheckWinstaAttributeAccess(WINSTA_WRITEATTRIBUTES))
    {
        goto Exit;
    }

    if (hcur)
    {
        pcur = UserGetCurIconObject(hcur);
        if (!pcur)
        {
            EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
            goto Exit;
        }

        ppi = PsGetCurrentProcessWin32Process();

        for (i = 0 ; i < 16; i++)
        {
           if (gasyscur[i].type == id)
           {
              pcurOrig = gasyscur[i].handle;

              if (pcurOrig) break;

              if (ppi->W32PF_flags & W32PF_CREATEDWINORDC)
              {
                 gasyscur[i].handle = pcur;
                 pcur->CURSORF_flags |= CURSORF_GLOBAL;
                 pcur->head.ppi = NULL;
                 IntInsertCursorIntoList(pcur);
                 Ret = TRUE;
              }
              break;
           }
        }
        if (pcurOrig)
        {
           FIXME("Need to copy cursor data or do something! pcurOrig %p new pcur %p\n",pcurOrig,pcur);
        }
    }
Exit:
    UserLeave();
    return Ret;
}

/* EOF */
