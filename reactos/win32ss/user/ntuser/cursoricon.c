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
                pcur->CURSORF_flags |= CURSORF_GLOBAL|CURSORF_LINKED;
                UserReferenceObject(pcur);
                //
                //  The active switch between LR shared and Global public.
                //  This is hacked around to support this while at the initial system start up.
                //
                pcur->head.ppi = NULL;
                //
                pcur->pcurNext = gcurFirst;
                gcurFirst = pcur;
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

    UserDereferenceObject(CurIcon);

    return hCurIcon;
}

BOOLEAN
IntDestroyCurIconObject(
    _In_ PVOID Object)
{
    PCURICON_OBJECT CurIcon = Object;

    /* Try finding it in its process cache */
    if (CurIcon->CURSORF_flags & CURSORF_LRSHARED)
    {
        PPROCESSINFO ppi;

        ppi = CurIcon->head.ppi;
        if (ppi->pCursorCache == CurIcon)
        {
            ppi->pCursorCache = CurIcon->pcurNext;
            UserDereferenceObject(CurIcon);
        }
        else
        {
            PCURICON_OBJECT CacheCurIcon = ppi->pCursorCache;
            while (CacheCurIcon)
            {
                if (CacheCurIcon->pcurNext == CurIcon)
                {
                    CacheCurIcon->pcurNext = CurIcon->pcurNext;
                    break;
                }
                CacheCurIcon = CacheCurIcon->pcurNext;
            }

            /* We must have found it! */
            ASSERT(CacheCurIcon != NULL);
            UserDereferenceObject(CurIcon);
        }
        CurIcon->CURSORF_flags &= ~CURSORF_LINKED;
    }

    /* We just mark the handle as being destroyed.
     * Deleting all the stuff will be deferred to the actual struct free. */
    UserDeleteObject(CurIcon->head.h, TYPE_CURSOR);
    return TRUE;
}

void
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
            GreDeleteObject(bmpMask);
            CurIcon->hbmMask = NULL;
        }
        if (bmpColor)
        {
            GreSetObjectOwner(bmpColor, GDI_OBJ_HMGR_POWNED);
            GreDeleteObject(bmpColor);
            CurIcon->hbmColor = NULL;
        }
        if (bmpAlpha)
        {
            GreSetObjectOwner(bmpAlpha, GDI_OBJ_HMGR_POWNED);
            GreDeleteObject(bmpAlpha);
            CurIcon->hbmAlpha = NULL;
        }
    }
    else
    {
        PACON AniCurIcon = (PACON)CurIcon;
        UINT i;

        for (i = 0; i < AniCurIcon->cpcur; i++)
        {
            UserDereferenceObject(AniCurIcon->aspcur[i]);
            IntDestroyCurIconObject(AniCurIcon->aspcur[i]);
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
        WARN("UserGetIconObject(0x%08x) Failed.\n", hCurIcon);
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

    TRACE("Enter NtUserDestroyCursorIcon (%p, %u)\n", hCurIcon, bForce);
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
    //
    //    Now search Global Cursors or Icons.
    //
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


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserSetCursorIconData(
  _In_     HCURSOR Handle,
  _In_opt_ PUNICODE_STRING pustrModule,
  _In_opt_ PUNICODE_STRING pustrRsrc,
  _In_     const CURSORDATA* pCursorData)
{
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN Ret = FALSE;
    BOOLEAN IsShared = FALSE, IsAnim = FALSE;
    DWORD numFrames;
    UINT i = 0;

    TRACE("Enter NtUserSetCursorIconData\n");

    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(Handle)))
    {
        UserLeave();
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    _SEH2_TRY
    {
        ProbeForRead(pCursorData, sizeof(*pCursorData), 1);
        if (pCursorData->CURSORF_flags & CURSORF_ACON)
        {
            /* This is an animated cursor */
            PACON AniCurIcon = (PACON)CurIcon;
            DWORD numSteps;

            numFrames = AniCurIcon->cpcur = pCursorData->cpcur;
            numSteps = AniCurIcon->cicur = pCursorData->cicur;
            AniCurIcon->iicur = pCursorData->iicur;
            AniCurIcon->rt = pCursorData->rt;

            /* Calculate size: one cursor object for each frame, and a frame index and jiffies for each "step" */
            AniCurIcon->aspcur = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE, /* Let SEH catch allocation failures */
                numFrames * sizeof(CURICON_OBJECT*) + numSteps * (sizeof(DWORD) + sizeof(INT)),
                USERTAG_CURSOR);
            AniCurIcon->aicur = (DWORD*)(AniCurIcon->aspcur + numFrames);
            AniCurIcon->ajifRate = (INT*)(AniCurIcon->aicur + numSteps);

            RtlZeroMemory(AniCurIcon->aspcur, numFrames * sizeof(CURICON_OBJECT*));

            ProbeForRead(pCursorData->aicur, numSteps * sizeof(DWORD), 1);
            RtlCopyMemory(AniCurIcon->aicur, pCursorData->aicur, numSteps * sizeof(DWORD));
            ProbeForRead(pCursorData->ajifRate, numSteps * sizeof(INT), 1);
            RtlCopyMemory(AniCurIcon->ajifRate, pCursorData->ajifRate, numSteps * sizeof(INT));

            AniCurIcon->CURSORF_flags = pCursorData->CURSORF_flags;
            pCursorData = pCursorData->aspcur;

            IsAnim = TRUE;
        }
        else
        {
            CurIcon->xHotspot = pCursorData->xHotspot;
            CurIcon->yHotspot = pCursorData->yHotspot;
            CurIcon->cx = pCursorData->cx;
            CurIcon->cy = pCursorData->cy;
            CurIcon->rt = pCursorData->rt;
            CurIcon->bpp = pCursorData->bpp;
            CurIcon->hbmMask = pCursorData->hbmMask;
            CurIcon->hbmColor = pCursorData->hbmColor;
            CurIcon->hbmAlpha = pCursorData->hbmAlpha;
            CurIcon->CURSORF_flags = pCursorData->CURSORF_flags;
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
        goto done;
    }

    if (IsAnim)
    {
        PACON AniCurIcon = (PACON)CurIcon;
        /* This is an animated cursor. Create a cursor object for each frame and set up the data */
        for (i = 0; i < numFrames; i++)
        {
            HANDLE hCurFrame = IntCreateCurIconHandle(FALSE);
            if (!NtUserSetCursorIconData(hCurFrame, NULL, NULL, pCursorData))
                goto done;
            AniCurIcon->aspcur[i] = UserGetCurIconObject(hCurFrame);
            if (!AniCurIcon->aspcur[i])
                goto done;
            pCursorData++;
        }
    }

    if (CurIcon->CURSORF_flags & CURSORF_LRSHARED)
    {
        IsShared = TRUE;
    }

    // Support global public cursors and icons too.
    if (!IsAnim || IsShared)
    {
        if (pustrRsrc && pustrModule)
        {
            UNICODE_STRING ustrModuleSafe;
            /* We use this convenient function, because INTRESOURCEs and ATOMs are the same */
            Status = ProbeAndCaptureUnicodeStringOrAtom(&CurIcon->strName, pustrRsrc);
            if (!NT_SUCCESS(Status))
                goto done;
            Status = ProbeAndCaptureUnicodeString(&ustrModuleSafe, UserMode, pustrModule);
            if (!NT_SUCCESS(Status))
                goto done;
            Status = RtlAddAtomToAtomTable(gAtomTable, ustrModuleSafe.Buffer, &CurIcon->atomModName);
            ReleaseCapturedUnicodeString(&ustrModuleSafe, UserMode);
            if (!NT_SUCCESS(Status))
                goto done;
        }
    }

    if (!CurIcon->hbmMask)
    {
        ERR("NtUserSetCursorIconData was got no hbmMask.\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    GreSetObjectOwner(CurIcon->hbmMask, GDI_OBJ_HMGR_PUBLIC);

    if (CurIcon->hbmColor)
        GreSetObjectOwner(CurIcon->hbmColor, GDI_OBJ_HMGR_PUBLIC);

    if (CurIcon->hbmAlpha)
        GreSetObjectOwner(CurIcon->hbmAlpha, GDI_OBJ_HMGR_PUBLIC);

    if (IsShared)
    {
        /* Update process cache in case of shared cursor */
        PPROCESSINFO ppi = CurIcon->head.ppi;
        UserReferenceObject(CurIcon);
        CurIcon->pcurNext = ppi->pCursorCache;
        ppi->pCursorCache = CurIcon;
        CurIcon->CURSORF_flags |= CURSORF_LINKED;
    }

    Ret = TRUE;

done:
    if (!Ret && (!IsAnim || IsShared))
    {
        if (!IS_INTRESOURCE(CurIcon->strName.Buffer))
            ExFreePoolWithTag(CurIcon->strName.Buffer, TAG_STRING);
    }

    if (!Ret && IsAnim)
    {
        PACON AniCurIcon = (PACON)CurIcon;
        for (i = 0; i < numFrames; i++)
        {
            if (AniCurIcon->aspcur[i])
            {
                UserDereferenceObject(AniCurIcon->aspcur[i]);
                IntDestroyCurIconObject(AniCurIcon->aspcur[i]);
            }
        }
        AniCurIcon->cicur = 0;
        AniCurIcon->cpcur = 0;
        ExFreePoolWithTag(AniCurIcon->aspcur, USERTAG_CURSOR);
        AniCurIcon->aspcur = NULL;
        AniCurIcon->aicur = NULL;
        AniCurIcon->ajifRate = NULL;
    }

    UserDereferenceObject(CurIcon);
    TRACE("Leave NtUserSetCursorIconData, ret=%i\n",Ret);
    UserLeave();

    return Ret;
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
        ERR("UserGetCurIconObject(0x%08x) failed!\n", hIcon);
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

    TRACE("Leaving NtUserGetCursorFrameInfo, ret = 0x%08x\n", ret);

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
                 pcur->CURSORF_flags |= CURSORF_GLOBAL|CURSORF_LINKED;
                 UserReferenceObject(pcur);
                 pcur->head.ppi = NULL;
                 pcur->pcurNext = gcurFirst;
                 gcurFirst = pcur;
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
