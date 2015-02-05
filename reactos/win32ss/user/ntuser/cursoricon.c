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
 * There's a M:N relationship between processes and (shared) cursor/icons.
 * A process can have multiple cursor/icons and a cursor/icon can be used
 * by multiple processes. To keep track of this we keep a list of all
 * cursor/icons (CurIconList) and per cursor/icon we keep a list of
 * CURICON_PROCESS structs starting at CurIcon->ProcessList.
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserIcon);

static PPAGED_LOOKASIDE_LIST pgProcessLookasideList;
static LIST_ENTRY gCurIconList;

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
    PLIST_ENTRY ListEntry;
    PCURICON_PROCESS Current;

    Win32Process = PsGetCurrentProcessWin32Process();

    ListEntry = CurIcon->ProcessList.Flink;
    while (ListEntry != &CurIcon->ProcessList)
    {
        Current = CONTAINING_RECORD(ListEntry, CURICON_PROCESS, ListEntry);
        ListEntry = ListEntry->Flink;
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
    IntReferenceProcessInfo(Win32Process);

    return TRUE;
}

PCURICON_OBJECT FASTCALL
IntFindExistingCurIconObject(HMODULE hModule,
                             HRSRC hRsrc, LONG cx, LONG cy)
{
    PLIST_ENTRY ListEntry;
    PCURICON_OBJECT CurIcon;

    ListEntry = gCurIconList.Flink;
    while (ListEntry != &gCurIconList)
    {
        CurIcon = CONTAINING_RECORD(ListEntry, CURICON_OBJECT, ListEntry);
        ListEntry = ListEntry->Flink;

        // if (NT_SUCCESS(UserReferenceObjectByPointer(Object, TYPE_CURSOR))) // <- huh????
//      UserReferenceObject(  CurIcon);
//      {
        if ((CurIcon->hModule == hModule) && (CurIcon->hRsrc == hRsrc))
        {
            if (cx && ((cx != CurIcon->Size.cx) || (cy != CurIcon->Size.cy)))
            {
//               UserDereferenceObject(CurIcon);
                continue;
            }
            if (! ReferenceCurIconByProcess(CurIcon))
            {
                return NULL;
            }

            return CurIcon;
        }
//      }
//      UserDereferenceObject(CurIcon);

    }

    return NULL;
}

HANDLE
IntCreateCurIconHandle(BOOLEAN Anim)
{
    PCURICON_OBJECT CurIcon;
    HANDLE hCurIcon;

    CurIcon = UserCreateObject(
        gHandleTable,
        NULL,
        GetW32ThreadInfo(),
        &hCurIcon,
        TYPE_CURSOR,
        sizeof(CURICON_OBJECT));

    if (!CurIcon)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CurIcon->Self = hCurIcon;
    InitializeListHead(&CurIcon->ProcessList);

    if (!ReferenceCurIconByProcess(CurIcon))
    {
        ERR("Failed to add process\n");
        UserDeleteObject(hCurIcon, TYPE_CURSOR);
        UserDereferenceObject(CurIcon);
        return NULL;
    }

    InsertHeadList(&gCurIconList, &CurIcon->ListEntry);

    UserDereferenceObject(CurIcon);

    return hCurIcon;
}

BOOLEAN FASTCALL
IntDestroyCurIconObject(PCURICON_OBJECT CurIcon, PPROCESSINFO ppi)
{
    PSYSTEM_CURSORINFO CurInfo;
    HBITMAP bmpMask, bmpColor;
    BOOLEAN Ret, bListEmpty, bFound = FALSE;
    PLIST_ENTRY ListEntry;
    PCURICON_PROCESS Current;

    /* For handles created without any data (error handling) */
    if(IsListEmpty(&CurIcon->ProcessList))
        goto emptyList;

    /* Now find this process in the list of processes referencing this object and
       remove it from that list */
    ListEntry = CurIcon->ProcessList.Flink;
    while (ListEntry != &CurIcon->ProcessList)
    {
        Current = CONTAINING_RECORD(ListEntry, CURICON_PROCESS, ListEntry);
        ListEntry = ListEntry->Flink;
        if (Current->Process == ppi)
        {
            bFound = TRUE;
            bListEmpty = RemoveEntryList(&Current->ListEntry);
            IntDereferenceProcessInfo(ppi);
            break;
        }
    }

    if(!bFound)
    {
        /* This object doesn't belong to this process */
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
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

    CurInfo = IntGetSysCursorInfo();

    if (CurInfo->CurrentCursorObject == CurIcon)
    {
        /* Hide the cursor if we're destroying the current cursor */
        UserSetCursor(NULL, TRUE);
    }

    bmpMask = CurIcon->IconInfo.hbmMask;
    bmpColor = CurIcon->IconInfo.hbmColor;

    /* Delete bitmaps */
    if (bmpMask)
    {
        GreSetObjectOwner(bmpMask, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(bmpMask);
        CurIcon->IconInfo.hbmMask = NULL;
    }
    if (bmpColor)
    {
        GreSetObjectOwner(bmpColor, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(bmpColor);
        CurIcon->IconInfo.hbmColor = NULL;
    }

    /* We were given a pointer, no need to keep the reference anylonger! */
    UserDereferenceObject(CurIcon);
    Ret = UserDeleteObject(CurIcon->Self, TYPE_CURSOR);

    return Ret;
}

HCURSOR FASTCALL
IntSetCursor(
    HCURSOR hCursor)
{
    PCURICON_OBJECT pcurOld, pcurNew;
    HCURSOR hOldCursor = NULL;

    if (hCursor)
    {
        pcurNew = UserGetCurIconObject(hCursor);
        if (!pcurNew)
        {
            EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
            goto leave;
        }
    }
    else
    {
        pcurNew = NULL;
    }

    pcurOld = UserSetCursor(pcurNew, FALSE);
    if (pcurOld)
    {
        hOldCursor = (HCURSOR)pcurOld->Self;
        UserDereferenceObject(pcurOld);
    }
leave:
    return hOldCursor;
}

BOOL FASTCALL
IntDestroyCursor(
  HANDLE hCurIcon,
  BOOL bForce)
{
    PCURICON_OBJECT CurIcon;
    BOOL ret;

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        return FALSE;
    }

    ret = IntDestroyCurIconObject(CurIcon, PsGetCurrentProcessWin32Process());
    /* Note: IntDestroyCurIconObject will remove our reference for us! */

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
NtUserGetIconInfo(
    HANDLE hCurIcon,
    PICONINFO IconInfo,
    PUNICODE_STRING lpInstName, // Optional
    PUNICODE_STRING lpResName,  // Optional
    LPDWORD pbpp,               // Optional
    BOOL bInternal)
{
    ICONINFO ii;
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret = FALSE;
    DWORD colorBpp = 0;

    TRACE("Enter NtUserGetIconInfo\n");
    UserEnterExclusive();

    if (!IconInfo)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto leave;
    }

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        goto leave;
    }

    RtlCopyMemory(&ii, &CurIcon->IconInfo, sizeof(ICONINFO));

    /* Copy bitmaps */
    ii.hbmMask = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmMask);
    ii.hbmColor = BITMAP_CopyBitmap(CurIcon->IconInfo.hbmColor);

    if (pbpp)
    {
        PSURFACE psurfBmp;

        psurfBmp = SURFACE_ShareLockSurface(CurIcon->IconInfo.hbmColor);
        if (psurfBmp)
        {
            colorBpp = BitsPerFormat(psurfBmp->SurfObj.iBitmapFormat);
            SURFACE_ShareUnlockSurface(psurfBmp);
        }
    }

    /* Copy fields */
    _SEH2_TRY
    {
        ProbeForWrite(IconInfo, sizeof(ICONINFO), 1);
        RtlCopyMemory(IconInfo, &ii, sizeof(ICONINFO));

        /// @todo Implement support for lpInstName
        if (lpInstName)
        {
            RtlInitEmptyUnicodeString(lpInstName, NULL, 0);
        }

        /// @todo Implement support for lpResName
        if (lpResName)
        {
            RtlInitEmptyUnicodeString(lpResName, NULL, 0);
        }

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

    if (NT_SUCCESS(Status))
        Ret = TRUE;
    else
        SetLastNtError(Status);

    UserDereferenceObject(CurIcon);

leave:
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
        RtlCopyMemory(plcx, &CurIcon->Size.cx, sizeof(LONG));
        ProbeForWrite(plcy, sizeof(LONG), 1);
        RtlCopyMemory(plcy, &CurIcon->Size.cy, sizeof(LONG));
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
  _In_  HANDLE hCurIcon,
  _In_  BOOL bForce)
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

    ret = IntDestroyCurIconObject(CurIcon, PsGetCurrentProcessWin32Process());
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
APIENTRY
NtUserFindExistingCursorIcon(
    HMODULE hModule,
    HRSRC hRsrc,
    LONG cx,
    LONG cy)
{
    PCURICON_OBJECT CurIcon;
    HANDLE Ret = (HANDLE)0;
    DECLARE_RETURN(HICON);

    TRACE("Enter NtUserFindExistingCursorIcon\n");
    UserEnterExclusive();

    CurIcon = IntFindExistingCurIconObject(hModule, hRsrc, cx, cy);
    if (CurIcon)
    {
        Ret = CurIcon->Self;

//      IntReleaseCurIconObject(CurIcon); // FIXME: Is this correct? Does IntFindExistingCurIconObject add a ref?
        RETURN(Ret);
    }

    EngSetLastError(ERROR_INVALID_CURSOR_HANDLE);
    RETURN((HANDLE)0);

CLEANUP:
    TRACE("Leave NtUserFindExistingCursorIcon, ret=%p\n",_ret_);
    UserLeave();
    END_CLEANUP;
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
    }
    else
    {
        pcurNew = NULL;
    }

    pcurOld = UserSetCursor(pcurNew, FALSE);
    if (pcurOld)
    {
        hOldCursor = (HCURSOR)pcurOld->Self;
        UserDereferenceObject(pcurOld);
    }

leave:
    UserLeave();
    return hOldCursor;
}


/*
 * @implemented
 */
BOOL
APIENTRY
NtUserSetCursorContents(
    HANDLE hCurIcon,
    PICONINFO UnsafeIconInfo)
{
    PCURICON_OBJECT CurIcon;
    ICONINFO IconInfo;
    PSURFACE psurfBmp;
    NTSTATUS Status;
    BOOL Ret = FALSE;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserSetCursorContents\n");
    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        RETURN(FALSE);
    }

    /* Copy fields */
    Status = MmCopyFromCaller(&IconInfo, UnsafeIconInfo, sizeof(ICONINFO));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        goto done;
    }

    /* Delete old bitmaps */
    if ((CurIcon->IconInfo.hbmColor)
			&& (CurIcon->IconInfo.hbmColor != IconInfo.hbmColor))
    {
        GreDeleteObject(CurIcon->IconInfo.hbmColor);
    }
    if ((CurIcon->IconInfo.hbmMask)
			&& CurIcon->IconInfo.hbmMask != IconInfo.hbmMask)
    {
        GreDeleteObject(CurIcon->IconInfo.hbmMask);
    }

    /* Copy new IconInfo field */
    CurIcon->IconInfo = IconInfo;

    if (CurIcon->IconInfo.hbmColor)
    {
        psurfBmp = SURFACE_ShareLockSurface(CurIcon->IconInfo.hbmColor);
        if (!psurfBmp)
            goto done;

        CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
        CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
        SURFACE_ShareUnlockSurface(psurfBmp);
        GreSetObjectOwner(CurIcon->IconInfo.hbmColor, GDI_OBJ_HMGR_PUBLIC);
    }
    else
    {
        psurfBmp = SURFACE_ShareLockSurface(CurIcon->IconInfo.hbmMask);
        if (!psurfBmp)
            goto done;

        CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
        CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy / 2;

        SURFACE_ShareUnlockSurface(psurfBmp);
    }
    GreSetObjectOwner(CurIcon->IconInfo.hbmMask, GDI_OBJ_HMGR_PUBLIC);

    Ret = TRUE;

done:

    if (CurIcon)
    {
        UserDereferenceObject(CurIcon);
    }
    RETURN(Ret);

CLEANUP:
    TRACE("Leave NtUserSetCursorContents, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}


/*
 * @implemented
 */
#if 0
BOOL
APIENTRY
NtUserSetCursorIconData(
    HANDLE Handle,
    HMODULE hModule,
    PUNICODE_STRING pstrResName,
    PICONINFO pIconInfo)
{
    PCURICON_OBJECT CurIcon;
    PSURFACE psurfBmp;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Ret = FALSE;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserSetCursorIconData\n");
    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(Handle)))
    {
        RETURN(FALSE);
    }

    CurIcon->hModule = hModule;
    CurIcon->hRsrc = NULL; //hRsrc;
    CurIcon->hGroupRsrc = NULL; //hGroupRsrc;

    _SEH2_TRY
    {
        ProbeForRead(pIconInfo, sizeof(ICONINFO), 1);
        RtlCopyMemory(&CurIcon->IconInfo, pIconInfo, sizeof(ICONINFO));

        CurIcon->IconInfo.hbmMask = BITMAP_CopyBitmap(pIconInfo->hbmMask);
        CurIcon->IconInfo.hbmColor = BITMAP_CopyBitmap(pIconInfo->hbmColor);

        if (CurIcon->IconInfo.hbmColor)
        {
            if ((psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmColor)))
            {
                CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
                CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
                SURFACE_UnlockSurface(psurfBmp);
                GreSetObjectOwner(CurIcon->IconInfo.hbmMask, GDI_OBJ_HMGR_PUBLIC);
            }
        }
        if (CurIcon->IconInfo.hbmMask)
        {
            if (CurIcon->IconInfo.hbmColor == NULL)
            {
                if ((psurfBmp = SURFACE_LockSurface(CurIcon->IconInfo.hbmMask)))
                {
                    CurIcon->Size.cx = psurfBmp->SurfObj.sizlBitmap.cx;
                    CurIcon->Size.cy = psurfBmp->SurfObj.sizlBitmap.cy;
                    SURFACE_UnlockSurface(psurfBmp);
                }
            }
            GreSetObjectOwner(CurIcon->IconInfo.hbmMask, GDI_OBJ_HMGR_PUBLIC);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
        SetLastNtError(Status);
    else
        Ret = TRUE;

    UserDereferenceObject(CurIcon);
    RETURN(Ret);

CLEANUP:
    TRACE("Leave NtUserSetCursorIconData, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}
#else
BOOL
APIENTRY
NtUserSetCursorIconData(
    HANDLE hCurIcon,
    PBOOL fIcon,
    POINT *Hotspot,
    HMODULE hModule,
    HRSRC hRsrc,
    HRSRC hGroupRsrc)
{
    PCURICON_OBJECT CurIcon;
    NTSTATUS Status;
    POINT SafeHotspot;
    BOOL Ret = FALSE;
    DECLARE_RETURN(BOOL);

    TRACE("Enter NtUserSetCursorIconData\n");
    UserEnterExclusive();

    if (!(CurIcon = UserGetCurIconObject(hCurIcon)))
    {
        RETURN(FALSE);
    }

    CurIcon->hModule = hModule;
    CurIcon->hRsrc = hRsrc;
    CurIcon->hGroupRsrc = hGroupRsrc;

    /* Copy fields */
    if (fIcon)
    {
        Status = MmCopyFromCaller(&CurIcon->IconInfo.fIcon, fIcon, sizeof(BOOL));
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto done;
        }
    }
    else
    {
        if (!Hotspot)
            Ret = TRUE;
    }

    if (Hotspot)
    {
        Status = MmCopyFromCaller(&SafeHotspot, Hotspot, sizeof(POINT));
        if (NT_SUCCESS(Status))
        {
            CurIcon->IconInfo.xHotspot = SafeHotspot.x;
            CurIcon->IconInfo.yHotspot = SafeHotspot.y;

            Ret = TRUE;
        }
        else
            SetLastNtError(Status);
    }

    if (!fIcon && !Hotspot)
    {
        Ret = TRUE;
    }

done:
	if(Ret)
	{
		/* This icon is shared now */
		GreSetObjectOwner(CurIcon->IconInfo.hbmMask, GDI_OBJ_HMGR_PUBLIC);
		if(CurIcon->IconInfo.hbmColor)
		{
			GreSetObjectOwner(CurIcon->IconInfo.hbmColor, GDI_OBJ_HMGR_PUBLIC);
		}
	}
    UserDereferenceObject(CurIcon);
    RETURN(Ret);


CLEANUP:
    TRACE("Leave NtUserSetCursorIconData, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}
#endif

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
    PSURFACE psurfDest, psurfMask, psurfColor, psurfOffScreen;
    PDC pdc = NULL;
    BOOL Ret = FALSE;
    HBITMAP hbmMask, hbmColor;
    BOOL bOffScreen, bAlpha = FALSE;
    RECTL rcDest, rcSrc;
    CLIPOBJ* pdcClipObj = NULL;
    EXLATEOBJ exlo;

    /* Stupid case */
    if((diFlags & DI_NORMAL) == 0)
    {
        ERR("DrawIconEx called without mask or color bitmap to draw.\n");
        return FALSE;
    }

    hbmMask = pIcon->IconInfo.hbmMask;
    hbmColor = pIcon->IconInfo.hbmColor;

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
        ASSERT(psurfMask->SurfObj.sizlBitmap.cy == 2*pIcon->Size.cy);
        psurfColor = NULL;
    }
    else if ((psurfColor = SURFACE_ShareLockSurface(hbmColor)) == NULL)
    {
        ERR("Unable to lock the color bitmap.\n");
        SURFACE_ShareUnlockSurface(psurfMask);
        return FALSE;
    }

    /* Set source rect */
    RECTL_vSetRect(&rcSrc, 0, 0, pIcon->Size.cx, pIcon->Size.cy);

    /* Check for alpha */
    if (psurfColor &&
       (psurfColor->SurfObj.iBitmapFormat == BMF_32BPP) &&
       (diFlags & DI_IMAGE))
    {
        PFN_DIB_GetPixel fnSource_GetPixel = NULL;
        INT i, j;

        /* In order to correctly display 32 bit icons Windows first scans the image,
           because information about transparency is not stored in any image's headers */
        fnSource_GetPixel = DibFunctionsForBitmapFormat[BMF_32BPP].DIB_GetPixel;
        for (i = 0; i < psurfColor->SurfObj.sizlBitmap.cx; i++)
        {
            for (j = 0; j < psurfColor->SurfObj.sizlBitmap.cy; j++)
            {
                bAlpha = ((BYTE)(fnSource_GetPixel(&psurfColor->SurfObj, i, j) >> 24) & 0xff);
                if (bAlpha)
                    break;
            }
            if (bAlpha)
                break;
        }
    }

    /* Fix width parameter, if needed */
    if (!cxWidth)
    {
        if(diFlags & DI_DEFAULTSIZE)
            cxWidth = pIcon->IconInfo.fIcon ?
                UserGetSystemMetrics(SM_CXICON) : UserGetSystemMetrics(SM_CXCURSOR);
        else
            cxWidth = pIcon->Size.cx;
    }

    /* Fix height parameter, if needed */
    if (!cyHeight)
    {
        if(diFlags & DI_DEFAULTSIZE)
            cyHeight = pIcon->IconInfo.fIcon ?
                UserGetSystemMetrics(SM_CYICON) : UserGetSystemMetrics(SM_CYCURSOR);
        else
            cyHeight = pIcon->Size.cy;
    }

    /* Should we render off-screen? */
    bOffScreen = hbrFlickerFreeDraw && (GDI_HANDLE_GET_TYPE(hbrFlickerFreeDraw) == GDI_OBJECT_TYPE_BRUSH);

    if (bOffScreen)
    {
        /* Yes: Allocate and paint the offscreen surface */
        EBRUSHOBJ eboFill;
        PBRUSH pbrush = BRUSH_ShareLockBrush(hbrFlickerFreeDraw);

        TRACE("Performing off-screen rendering.\n");

        if(!pbrush)
        {
            ERR("Failed to get brush object.\n");
            SURFACE_ShareUnlockSurface(psurfMask);
            if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
            return FALSE;
        }

        if (!psurfColor)
        {
           ERR("Attention HAX FIX API DrawIconEx TEST Line 37: psurfColor is NULL going with Mask!\n");
           psurfColor = SURFACE_ShareLockSurface(hbmMask);
        }

        psurfOffScreen = SURFACE_AllocSurface(STYPE_BITMAP,
            cxWidth, cyHeight, psurfColor->SurfObj.iBitmapFormat,
            0, 0, NULL);
        if(!psurfOffScreen)
        {
            ERR("Failed to allocate the off-screen surface.\n");
            SURFACE_ShareUnlockSurface(psurfMask);
            if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
            BRUSH_ShareUnlockBrush(pbrush);
            return FALSE;
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
            SURFACE_ShareUnlockSurface(psurfMask);
            if(psurfColor) SURFACE_ShareUnlockSurface(psurfColor);
            GDIOBJ_vDeleteObject(&psurfOffScreen->BaseObject);
            return FALSE;
        }

        /* We now have our destination surface */
        psurfDest = psurfOffScreen;
    }
    else
    {
        /* We directly draw to the DC */
        TRACE("Performing on screen rendering.\n");

        psurfOffScreen = NULL;
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
        DC_vPrepareDCsForBlit(pdc, &rcDest, NULL, NULL);

        /* Get the clip object */
        pdcClipObj = &pdc->co.ClipObj;

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
    }

    /* Now do the rendering */
	if(bAlpha && (diFlags & DI_IMAGE))
	{
	    BLENDOBJ blendobj = { {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA } };
        BYTE Alpha;
        INT i, j;
        PSURFACE psurf = NULL;
        PBYTE ptr ;
        HBITMAP hsurfCopy = NULL;

        hsurfCopy = BITMAP_CopyBitmap(hbmColor);
        if(!hsurfCopy)
        {
            ERR("BITMAP_CopyBitmap failed!");
            goto CleanupAlpha;
        }

        psurf = SURFACE_ShareLockSurface(hsurfCopy);
        if(!psurf)
        {
            ERR("SURFACE_LockSurface failed!\n");
            goto CleanupAlpha;
        }

        /* Premultiply with the alpha channel value */
        for (i = 0; i < psurf->SurfObj.sizlBitmap.cy; i++)
        {
			ptr = (PBYTE)psurf->SurfObj.pvScan0 + i*psurf->SurfObj.lDelta;
            for (j = 0; j < psurf->SurfObj.sizlBitmap.cx; j++)
            {
                Alpha = ptr[3];
                ptr[0] = (ptr[0] * Alpha) / 0xff;
                ptr[1] = (ptr[1] * Alpha) / 0xff;
                ptr[2] = (ptr[2] * Alpha) / 0xff;

				ptr += 4;
            }
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

    CleanupAlpha:
        if(psurf) SURFACE_ShareUnlockSurface(psurf);
        if(hsurfCopy) NtGdiDeleteObjectApp(hsurfCopy);
		if(Ret) goto done;
		ERR("NtGdiAlphaBlend failed!\n");
    }

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
            RECTL_vOffsetRect(&rcSrc, 0, pIcon->Size.cy);

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
    /* We're done. Was it a double buffered draw ? */
    if(bOffScreen)
    {
        /* Yes. Draw it back to our DC */
        POINTL ptSrc = {0, 0};
        pdc = DC_LockDc(hDc);
        if(!pdc)
        {
            ERR("Could not lock the destination DC.\n");
            return FALSE;
        }
        /* Calculate destination rectangle */
        RECTL_vSetRect(&rcDest, xLeft, yTop, xLeft + cxWidth, yTop + cyHeight);
        IntLPtoDP(pdc, (LPPOINT)&rcDest, 2);
        RECTL_vOffsetRect(&rcDest, pdc->ptlDCOrig.x, pdc->ptlDCOrig.y);

        /* Prepare the underlying surface */
        DC_vPrepareDCsForBlit(pdc, &rcDest, NULL, NULL);

        /* Get the clip object */
        pdcClipObj = &pdc->co.ClipObj;

        /* We now have our destination surface and rectangle */
        psurfDest = pdc->dclevel.pSurface;
        if(!psurfDest)
        {
            /* So, you did all of this for an empty DC. */
            DC_UnlockDc(pdc);
            goto Cleanup2;
        }

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
Cleanup:
    if(pdc)
    {
        DC_vFinishBlit(pdc, NULL);
        DC_UnlockDc(pdc);
    }

Cleanup2:
    /* Delete off screen rendering surface */
    if(psurfOffScreen)
        GDIOBJ_vDeleteObject(&psurfOffScreen->BaseObject);

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
        ERR("UserGetCurIconObject() failed!\n");
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
    STUB

    return 0;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserSetSystemCursor(
    HCURSOR hcur,
    DWORD id)
{
    return FALSE;
}

/* EOF */
