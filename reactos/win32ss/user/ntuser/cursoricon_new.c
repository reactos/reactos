/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Cursor and icon functions
 * FILE:             subsystems/win32/win32k/ntuser/cursoricon.c
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
 * There's a M:N relationship between processes and (shared) cursor/icons.
 * A process can have multiple cursor/icons and a cursor/icon can be used
 * by multiple processes. To keep track of this we keep a list of all
 * cursor/icons (CurIconList) and per cursor/icon we keep a list of
 * CURICON_PROCESS structs starting at CurIcon->ProcessList.
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserIcon);

static LIST_ENTRY gCurIconList;
static PAGED_LOOKASIDE_LIST *pgProcessLookasideList;

SYSTEM_CURSORINFO gSysCursorInfo;

BOOL
InitCursorImpl()
{
    pgProcessLookasideList = ExAllocatePool(NonPagedPool, sizeof(PAGED_LOOKASIDE_LIST));
    if(!pgProcessLookasideList)
        return FALSE;

    ExInitializePagedLookasideList(pgProcessLookasideList,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(CURICON_PROCESS),
                                   TAG_DIB,
                                   128);
    InitializeListHead(&gCurIconList);

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

    if(!(DesktopWindow = UserGetDesktopWindow()))
    {
        return FALSE;
    }

    CurInfo = IntGetSysCursorInfo();

    /* Clip cursor position */
    if (!CurInfo->bClipped)
        rcClip = DesktopWindow->rcClient;
    else
        rcClip = CurInfo->rcClip;

    if(x >= rcClip.right)  x = rcClip.right - 1;
    if(x < rcClip.left)    x = rcClip.left;
    if(y >= rcClip.bottom) y = rcClip.bottom - 1;
    if(y < rcClip.top)     y = rcClip.top;

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

/*
 * We have to register that this object is in use by the current
 * process. The only way to do that seems to be to walk the list
 * of cursor/icon objects starting at W32Process->CursorIconListHead.
 * If the object is already present in the list, we don't have to do
 * anything, if it's not present we add it and inc the ProcessCount
 * in the object. Having to walk the list kind of sucks, but that's
 * life...
 */
static BOOLEAN FASTCALL
ReferenceCurIconByProcess(PCURICON_OBJECT CurIcon)
{
    PPROCESSINFO Win32Process;
    PCURICON_PROCESS Current;

    Win32Process = PsGetCurrentProcessWin32Process();

    LIST_FOR_EACH(Current, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
    {
        if (Current->Process == Win32Process)
        {
            /* Already registered for this process */
            return TRUE;
        }
    }

    /* Not registered yet */
    Current = ExAllocateFromPagedLookasideList(pgProcessLookasideList);
    if (NULL == Current)
    {
        return FALSE;
    }
    InsertHeadList(&CurIcon->ProcessList, &Current->ListEntry);
    Current->Process = Win32Process;

    return TRUE;
}

static
PCURICON_OBJECT
FASTCALL
IntFindExistingCurIconObject(
    PUNICODE_STRING pustrModule,
    PUNICODE_STRING pustrRsrc,
    FINDEXISTINGCURICONPARAM* param)
{
    PCURICON_OBJECT CurIcon;

    LIST_FOR_EACH(CurIcon, &gCurIconList, CURICON_OBJECT, ListEntry)
    {
        /* See if we are looking for an icon or a cursor */
        if(MAKEINTRESOURCE(CurIcon->rt) != (param->bIcon ? RT_ICON : RT_CURSOR))
            continue;
        /* See if module names match */
        if(RtlCompareUnicodeString(pustrModule, &CurIcon->ustrModule, TRUE) == 0)
        {
            /* They do. Now see if this is the same resource */
            if(IS_INTRESOURCE(CurIcon->strName.Buffer) && IS_INTRESOURCE(pustrRsrc->Buffer))
            {
                if(CurIcon->strName.Buffer != pustrRsrc->Buffer)
                    continue;
            }
            else if(IS_INTRESOURCE(CurIcon->strName.Buffer) || IS_INTRESOURCE(pustrRsrc->Buffer))
                continue;
            else if(RtlCompareUnicodeString(pustrRsrc, &CurIcon->strName, TRUE) != 0)
                continue;
            
            if ((param->cx == CurIcon->cx) && (param->cy == CurIcon->cy))
            {
                if (! ReferenceCurIconByProcess(CurIcon))
                {
                    return NULL;
                }

                return CurIcon;
            }
        }
    }

    return NULL;
}

PCURICON_OBJECT
IntCreateCurIconHandle(DWORD dwNumber)
{
    PCURICON_OBJECT CurIcon;
    HANDLE hCurIcon;
    
    if(dwNumber == 0)
        dwNumber = 1;

    CurIcon = UserCreateObject(gHandleTable, NULL, NULL, &hCurIcon, TYPE_CURSOR, sizeof(CURICON_OBJECT));

    if (!CurIcon)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CurIcon->Self = hCurIcon;
    InitializeListHead(&CurIcon->ProcessList);

    if (! ReferenceCurIconByProcess(CurIcon))
    {
        ERR("Failed to add process\n");
        UserDereferenceObject(CurIcon);
        UserDeleteObject(hCurIcon, TYPE_CURSOR);
        return NULL;
    }

    InsertHeadList(&gCurIconList, &CurIcon->ListEntry);

    return CurIcon;
}

BOOLEAN FASTCALL
IntDestroyCurIconObject(PCURICON_OBJECT CurIcon, PPROCESSINFO ppi, BOOLEAN bForce)
{
    HBITMAP bmpMask, bmpColor, bmpAlpha;
    BOOLEAN Ret, bListEmpty, bFound = FALSE;
    PCURICON_PROCESS Current = NULL;
    
    /* Check if this is the current cursor */
    if(CurIcon->CURSORF_flags & CURSORF_CURRENT)
    {
        UserDereferenceObject(CurIcon);
        return FALSE;
    }
    
    /* For handles created without any data (error handling) */
    if(IsListEmpty(&CurIcon->ProcessList))
        goto emptyList;

    /* Now find this process in the list of processes referencing this object and
       remove it from that list */
    LIST_FOR_EACH(Current, &CurIcon->ProcessList, CURICON_PROCESS, ListEntry)
    {
        if (Current->Process == ppi)
        {
            bFound = TRUE;
            bListEmpty = RemoveEntryList(&Current->ListEntry);
            break;
        }
    }
    
    if(!bFound)
    {
        /* This object doesn't belong to this process */
        EngSetLastError(ERROR_INVALID_HANDLE);
        /* Caller expects us to dereference! */
        UserDereferenceObject(CurIcon);
        return FALSE;
    }
    
    /* We found our process, but we're told to not destroy it in case it is shared */
    if((CurIcon->ustrModule.Buffer != NULL) && !bForce)
    {
        /* Tests show this is a valid call */
        UserDereferenceObject(CurIcon);
        return TRUE;
    }

    ExFreeToPagedLookasideList(pgProcessLookasideList, Current);

    /* If there are still processes referencing this object we can't destroy it yet */
    if (!bListEmpty)
    {
        if(CurIcon->head.ppi == ppi)
        {
            /* Set the first process of the list as owner */
            Current = CONTAINING_RECORD(CurIcon->ProcessList.Flink, CURICON_PROCESS, ListEntry);
            UserSetObjectOwner(CurIcon, TYPE_CURSOR, Current->Process);
        }
        UserDereferenceObject(CurIcon);
        return TRUE;
    }

emptyList:
    /* Remove it from the list */
    RemoveEntryList(&CurIcon->ListEntry);

    bmpMask = CurIcon->hbmMask;
    bmpColor = CurIcon->hbmColor;
    bmpAlpha = CurIcon->hbmAlpha;

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
    
    if(!IS_INTRESOURCE(CurIcon->strName.Buffer))
        ExFreePoolWithTag(CurIcon->strName.Buffer, TAG_STRING);
    if(CurIcon->ustrModule.Buffer)
        ReleaseCapturedUnicodeString(&CurIcon->ustrModule, UserMode);

    /* We were given a pointer, no need to keep the reference anylonger! */
    UserDereferenceObject(CurIcon);
    Ret = UserDeleteObject(CurIcon->Self, TYPE_CURSOR);

    return Ret;
}

VOID FASTCALL
IntCleanupCurIcons(struct _EPROCESS *Process, PPROCESSINFO Win32Process)
{
    PCURICON_OBJECT CurIcon, tmp;

    /* Run through the list of icon objects */
    LIST_FOR_EACH_SAFE(CurIcon, tmp, &gCurIconList, CURICON_OBJECT, ListEntry)
    {
        UserReferenceObject(CurIcon);
        IntDestroyCurIconObject(CurIcon, Win32Process, TRUE);
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
    if(IconInfo)
    {
        /* Fill data */
        ii.fIcon = is_icon(CurIcon);
        ii.xHotspot = CurIcon->xHotspot;
        ii.yHotspot = CurIcon->yHotspot;

        /* Copy bitmaps */
        ii.hbmMask = BITMAP_CopyBitmap(CurIcon->hbmMask);
        GreSetObjectOwner(ii.hbmMask, GDI_OBJ_HMGR_POWNED);
        ii.hbmColor = BITMAP_CopyBitmap(CurIcon->hbmColor);
        GreSetObjectOwner(ii.hbmColor, GDI_OBJ_HMGR_POWNED);
        colorBpp = CurIcon->bpp;

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
    }

    if (!NT_SUCCESS(Status))
    {
        WARN("Status: 0x%08x.\n", Status);
        SetLastNtError(Status);
        goto leave;
    }

    /* Give back the module name */
    if(lpModule)
    {
        if(!CurIcon->ustrModule.Buffer)
        {
            EngSetLastError(ERROR_INVALID_HANDLE);
            goto leave;
        }
        /* Copy what we can */
        _SEH2_TRY
        {
            ProbeForWrite(lpModule, sizeof(UNICODE_STRING), 1);
            ProbeForWrite(lpModule->Buffer, lpModule->MaximumLength, 1);
            lpModule->Length = min(lpModule->MaximumLength, CurIcon->ustrModule.Length);
            RtlCopyMemory(lpModule->Buffer, CurIcon->ustrModule.Buffer, lpModule->Length);
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
    
    if(lpResName)
    {
        if(!CurIcon->strName.Buffer)
        {
            EngSetLastError(ERROR_INVALID_HANDLE);
            goto leave;
        }
        /* Copy it */
        _SEH2_TRY
        {
            ProbeForWrite(lpResName, sizeof(UNICODE_STRING), 1);
            if(IS_INTRESOURCE(CurIcon->strName.Buffer))
            {
                lpResName->Buffer = CurIcon->strName.Buffer;
                lpResName->Length = 0;
            }
            else
            {
                lpResName->Length = min(lpResName->MaximumLength, CurIcon->strName.Length);
                RtlCopyMemory(lpResName->Buffer, CurIcon->strName.Buffer, lpResName->Length);
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
    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        goto cleanup;
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
    UserEnterExclusive();

    CurInfo = IntGetSysCursorInfo();
    CurIcon = (PCURICON_OBJECT)CurInfo->CurrentCursorObject;

    SafeCi.cbSize = sizeof(CURSORINFO);
    SafeCi.flags = ((CurIcon && CurInfo->ShowingCursor >= 0) ? CURSOR_SHOWING : 0);
    SafeCi.hCursor = (CurIcon ? (HCURSOR)CurIcon->Self : (HCURSOR)0);

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
    /* FIXME: Check if process has WINSTA_WRITEATTRIBUTES */
    PSYSTEM_CURSORINFO CurInfo;
    PWND DesktopWindow = NULL;

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
    PCURICON_OBJECT CurIcon;
    BOOL ret;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserDestroyCursorIcon\n");
    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        RETURN(FALSE);
    }

    ret = IntDestroyCurIconObject(CurIcon, PsGetCurrentProcessWin32Process(), bForce);
    /* Note: IntDestroyCurIconObject will remove our reference for us! */

    RETURN(ret);

CLEANUP:
    TRACE("Leave NtUserDestroyCursorIcon, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
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

    TRACE("Enter NtUserFindExistingCursorIcon\n");
    
    /* Capture resource name (it can be an INTRESOURCE == ATOM) */
    Status = ProbeAndCaptureUnicodeStringOrAtom(&ustrRsrcSafe, pustrRsrc);
    if(!NT_SUCCESS(Status))
        return NULL;
    Status = ProbeAndCaptureUnicodeString(&ustrModuleSafe, UserMode, pustrModule);
    if(!NT_SUCCESS(Status))
        goto done;
    
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

    UserEnterExclusive();
    CurIcon = IntFindExistingCurIconObject(&ustrModuleSafe, &ustrRsrcSafe, &paramSafe);
    if (CurIcon)
        Ret = CurIcon->Self;
    UserLeave();

done:
    if(!IS_INTRESOURCE(ustrRsrcSafe.Buffer))
        ExFreePoolWithTag(ustrRsrcSafe.Buffer, TAG_STRING);
    ReleaseCapturedUnicodeString(&ustrModuleSafe, UserMode);
    
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
    /* FIXME: Check if process has WINSTA_READATTRIBUTES */
    PSYSTEM_CURSORINFO CurInfo;
    RECTL Rect;
    NTSTATUS Status;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserGetClipCursor\n");
    UserEnterExclusive();

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

    TRACE("Enter NtUserSetCursor\n");
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
        hOldCursor = (HCURSOR)pcurOld->Self;
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
  _In_     PCURSORDATA pCursorData)
{
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret = FALSE;
    
    TRACE("Enter NtUserSetCursorIconData\n");
    
    /* If a module name is provided, we need a resource name, and vice versa */
    if((pustrModule && !pustrRsrc) || (!pustrModule && pustrRsrc))
        return FALSE;
    
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
        CurIcon->xHotspot = pCursorData->xHotspot;
        CurIcon->yHotspot = pCursorData->yHotspot;
        CurIcon->cx = pCursorData->cx;
        CurIcon->cy = pCursorData->cy;
        CurIcon->rt = pCursorData->rt;
        CurIcon->bpp = pCursorData->bpp;
        CurIcon->hbmMask = pCursorData->hbmMask;
        CurIcon->hbmColor = pCursorData->hbmColor;
        CurIcon->hbmAlpha = pCursorData->hbmAlpha;
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
    
    if(pustrModule)
    {
        /* We use this convenient function, because INTRESOURCEs and ATOMs are the same */
        Status = ProbeAndCaptureUnicodeStringOrAtom(&CurIcon->strName, pustrRsrc);
        if(!NT_SUCCESS(Status))
            goto done;
        Status = ProbeAndCaptureUnicodeString(&CurIcon->ustrModule, UserMode, pustrModule);
        if(!NT_SUCCESS(Status))
            goto done;
    }


    if(!CurIcon->hbmMask)
    {
        ERR("NtUserSetCursorIconData was got no hbmMask.\n");
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    GreSetObjectOwner(CurIcon->hbmMask, GDI_OBJ_HMGR_PUBLIC);

    if(CurIcon->hbmColor)
        GreSetObjectOwner(CurIcon->hbmColor, GDI_OBJ_HMGR_PUBLIC);
    
    if(CurIcon->hbmAlpha)
        GreSetObjectOwner(CurIcon->hbmAlpha, GDI_OBJ_HMGR_PUBLIC);
    
    Ret = TRUE;

done:
    if(!Ret)
    {
        if(!IS_INTRESOURCE(CurIcon->strName.Buffer))
            ExFreePoolWithTag(CurIcon->strName.Buffer, TAG_STRING);
        if(CurIcon->ustrModule.Buffer)
            ReleaseCapturedUnicodeString(&CurIcon->ustrModule, UserMode);
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
    if((diFlags & DI_NORMAL) == 0)
    {
        ERR("DrawIconEx called without mask or color bitmap to draw.\n");
        return FALSE;
    }

    hbmMask = pIcon->hbmMask;
    hbmColor = pIcon->hbmColor;
    hbmAlpha = pIcon->hbmAlpha;
    
    if (istepIfAniCur)
        ERR("NtUserDrawIconEx: istepIfAniCur is not supported!\n");
    
    /*
     * Get our objects. 
     * Shared locks are enough, we are only reading those bitmaps
     */
    psurfMask = SURFACE_ShareLockSurface(hbmMask);
    if(psurfMask == NULL)
    {
        ERR("Unable to lock the mask surface.\n");
        return FALSE;
    }
    
    /* Color bitmap is not mandatory */
    if(hbmColor == NULL)
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
    if(!pdc)
    {
        ERR("Could not lock the destination DC.\n");
        SURFACE_ShareUnlockSurface(psurfMask);
        if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
        return FALSE;
    }
    /* Calculate destination rectangle */
    RECTL_vSetRect(&rcDest, xLeft, yTop, xLeft + cxWidth, yTop + cyHeight);
    IntLPtoDP(pdc, (LPPOINT)&rcDest, 2);
    RECTL_vOffsetRect(&rcDest, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y);
    
    /* Prepare the underlying surface */
    DC_vPrepareDCsForBlit(pdc, rcDest, NULL, rcDest);

    /* We now have our destination surface and rectangle */
    psurfDest = pdc->dclevel.pSurface;
    
    if(psurfDest == NULL)
    {
        /* Empty DC */
        DC_vFinishBlit(pdc, NULL);
        DC_UnlockDc(pdc);
        SURFACE_ShareUnlockSurface(psurfMask);
        if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
        return FALSE;
    }
    
    /* Set source rect */
    RECTL_vSetRect(&rcSrc, 0, 0, pIcon->cx, pIcon->cy);

    /* Fix width parameter, if needed */
    if (!cxWidth)
    {
        if(diFlags & DI_DEFAULTSIZE)
            cxWidth = is_icon(pIcon) ? 
                UserGetSystemMetrics(SM_CXICON) : UserGetSystemMetrics(SM_CXCURSOR);
        else
            cxWidth = pIcon->cx;
    }
    
    /* Fix height parameter, if needed */
    if (!cyHeight)
    {
        if(diFlags & DI_DEFAULTSIZE)
            cyHeight = is_icon(pIcon) ? 
                UserGetSystemMetrics(SM_CYICON) : UserGetSystemMetrics(SM_CYCURSOR);
        else
            cyHeight = pIcon->cy;
    }

    /* Should we render off-screen? */
    bOffScreen = hbrFlickerFreeDraw && 
        (GDI_HANDLE_GET_TYPE(hbrFlickerFreeDraw) == GDI_OBJECT_TYPE_BRUSH);

    if (bOffScreen)
    {
        /* Yes: Allocate and paint the offscreen surface */
        EBRUSHOBJ eboFill;
        PBRUSH pbrush = BRUSH_ShareLockBrush(hbrFlickerFreeDraw);
        
        TRACE("Performing off-screen rendering.\n");
        
        if(!pbrush)
        {
            ERR("Failed to get brush object.\n");
            goto Cleanup;
        }

#if 0 //We lock the hdc surface during the whole function it makes no sense to use an offscreen surface for "flicker free" drawing
        psurfOffScreen = SURFACE_AllocSurface(STYPE_BITMAP,
            cxWidth, cyHeight, psurfDest->SurfObj.iBitmapFormat,
            0, 0, NULL);
        if(!psurfOffScreen)
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
            
        if(!Ret)
        {
            ERR("Failed to paint the off-screen surface.\n");
            goto Cleanup;
        }
        
        /* We now have our destination surface */
        psurfDest = psurfOffScreen;
#else
        pdcClipObj = pdc->rosdc.CombinedClip;
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
            
        if(!Ret)
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
        pdcClipObj = pdc->rosdc.CombinedClip;
        // psurfOffScreen = NULL;
    }

    /* Now do the rendering */
	if(hbmAlpha && ((diFlags & DI_NORMAL) == DI_NORMAL))
	{
	    BLENDOBJ blendobj = { {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA } };
        PSURFACE psurf = NULL;

        psurf = SURFACE_ShareLockSurface(hbmAlpha);
        if(!psurf)
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
        if(Ret) goto done;
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

        if(!Ret)
        {
            ERR("Failed to mask the bitmap data.\n");
            goto Cleanup;
        }
    }

    if(diFlags & DI_IMAGE)
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

            if(!Ret)
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

            if(!Ret)
            {
                ERR("Failed to render the icon bitmap.\n");
                goto Cleanup;
            }
        }
    }

done:
#if 0
    /* We're done. Was it a double buffered draw ? */
    if(bOffScreen)
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
    if(pdc)
    {
        DC_vFinishBlit(pdc, NULL);
        DC_UnlockDc(pdc);
    }

#if 0
    /* Delete off screen rendering surface */
    if(psurfOffScreen)
        GDIOBJ_vDeleteObject(&psurfOffScreen->BaseObject);
#endif

    /* Unlock other surfaces */
    SURFACE_ShareUnlockSurface(psurfMask);
    if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);

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

/* EOF */
