#pragma once

static __inline PVOID
SharedPtrToUser(PVOID Ptr)
{
    ASSERT(Ptr != NULL);
    ASSERT(gSharedInfo.ulSharedDelta != 0);
    return (PVOID)((ULONG_PTR)Ptr - gSharedInfo.ulSharedDelta);
}

static __inline PVOID
DesktopPtrToUser(PVOID Ptr)
{
    PCLIENTINFO pci;
    PDESKTOPINFO pdi;
    GetW32ThreadInfo();
    pci = GetWin32ClientInfo();
    pdi = pci->pDeskInfo;

    ASSERT(Ptr != NULL);
    ASSERT(pdi != NULL);
    if ((ULONG_PTR)Ptr >= (ULONG_PTR)pdi->pvDesktopBase &&
        (ULONG_PTR)Ptr < (ULONG_PTR)pdi->pvDesktopLimit)
    {
        return (PVOID)((ULONG_PTR)Ptr - pci->ulClientDelta);
    }
    else
    {
        /* NOTE: This is slow as it requires a call to win32k. This should only be
                 neccessary if a thread wants to access an object on a different
                 desktop */
        return (PVOID)NtUserxGetDesktopMapping(Ptr);
    }
}

static __inline BOOL
IsThreadHooked(PCLIENTINFO pci)
{
    return (pci->fsHooks|pci->pDeskInfo->fsHooks) != 0;
}

static __inline PDESKTOPINFO
GetThreadDesktopInfo(VOID)
{
    PTHREADINFO ti;
    PDESKTOPINFO di = NULL;

    ti = GetW32ThreadInfo();
    if (ti != NULL)
        di = GetWin32ClientInfo()->pDeskInfo;

    return di;
}

static __inline BOOL
IsCallProcHandle(IN WNDPROC lpWndProc)
{
    /* FIXME - check for 64 bit architectures... */
    return ((ULONG_PTR)lpWndProc & 0xFFFF0000) == 0xFFFF0000;
}

#define STATIC_UISTATE_GWL_OFFSET (sizeof(HFONT)+sizeof(HICON))// see UISTATE_GWL_OFFSET in static.c

/* Retrieve the UI state for the control */
static __inline BOOL STATIC_update_uistate(HWND hwnd, BOOL unicode)
{
    LONG flags, prevflags;

    if (unicode)
        flags = DefWindowProcW(hwnd, WM_QUERYUISTATE, 0, 0);
    else
        flags = DefWindowProcA(hwnd, WM_QUERYUISTATE, 0, 0);

    prevflags = GetWindowLongW(hwnd, STATIC_UISTATE_GWL_OFFSET);

    if (prevflags != flags)
    {
        SetWindowLongW(hwnd, STATIC_UISTATE_GWL_OFFSET, flags);
        return TRUE;
    }

    return FALSE;
}

static __inline void LoadUserApiHook()
{
   if (!gfServerProcess &&
       !IsInsideUserApiHook() &&
       (gpsi->dwSRVIFlags & SRVINFO_APIHOOK) &&
       !RtlIsThreadWithinLoaderCallout())
   {
      NtUserCallNoParam(NOPARAM_ROUTINE_LOADUSERAPIHOOK);
   }
}

#define UserHasDlgFrameStyle(Style, ExStyle)                                   \
 (((ExStyle) & WS_EX_DLGMODALFRAME) ||                                         \
  (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define UserHasThickFrameStyle(Style, ExStyle)                                 \
  (((Style) & WS_THICKFRAME) &&                                                \
   (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define UserHasThinFrameStyle(Style, ExStyle)                                  \
  (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

/* macro for source compatibility with wine */
#define WIN_GetFullHandle(h) ((HWND)(h))

#define HOOKID_TO_FLAG(HookId) (1 << ((HookId) + 1))
#define ISITHOOKED(HookId) (GetWin32ClientInfo()->fsHooks & HOOKID_TO_FLAG(HookId) ||\
                           (GetWin32ClientInfo()->pDeskInfo && GetWin32ClientInfo()->pDeskInfo->fsHooks & HOOKID_TO_FLAG(HookId)))
