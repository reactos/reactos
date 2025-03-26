/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/stubs.c
 * PURPOSE:         User32.dll stubs
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      08-F05-2001  CSH  Created
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/*
 * @unimplemented
 */
DWORD
WINAPI
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
// Need to call NtQueryInformationProcess and send ProcessId not hProcess.
  return NtUserWaitForInputIdle(hProcess, dwMilliseconds, FALSE);
}

/******************************************************************************
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 *
 * @unimplemented
 */
VOID
WINAPI
SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%lu): stub\n", dwLevel);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetAppCompatFlags(HTASK hTask)
{
    PCLIENTINFO pci = GetWin32ClientInfo();

    return pci->dwCompatFlags;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetAppCompatFlags2(HTASK hTask)
{
    PCLIENTINFO pci = GetWin32ClientInfo();

    return pci->dwCompatFlags2;
}

/*
 * @unimplemented
 */
VOID
WINAPI
LoadLocalFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
WINAPI
LoadRemoteFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
WINAPI
RegisterSystemThread ( DWORD flags, DWORD reserved )
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
UINT
WINAPI
UserRealizePalette ( HDC hDC )
{
  return NtUserxRealizePalette(hDC);
}


/*************************************************************************
 *		SetSysColorsTemp (USER32.@) (Wine 10/22/2008)
 *
 * UNDOCUMENTED !!
 *
 * Called by W98SE desk.cpl Control Panel Applet:
 * handle = SetSysColorsTemp(ptr, ptr, nCount);     ("set" call)
 * result = SetSysColorsTemp(NULL, NULL, handle);   ("restore" call)
 *
 * pPens is an array of COLORREF values, which seems to be used
 * to indicate the color values to create new pens with.
 *
 * pBrushes is an array of solid brush handles (returned by a previous
 * CreateSolidBrush), which seems to contain the brush handles to set
 * for the system colors.
 *
 * n seems to be used for
 *   a) indicating the number of entries to operate on (length of pPens,
 *      pBrushes)
 *   b) passing the handle that points to the previously used color settings.
 *      I couldn't figure out in hell what kind of handle this is on
 *      Windows. I just use a heap handle instead. Shouldn't matter anyway.
 *
 * RETURNS
 *     heap handle of our own copy of the current syscolors in case of
 *                 "set" call, i.e. pPens, pBrushes != NULL.
 *     TRUE (unconditionally !) in case of "restore" call,
 *          i.e. pPens, pBrushes == NULL.
 *     FALSE in case of either pPens != NULL and pBrushes == NULL
 *          or pPens == NULL and pBrushes != NULL.
 *
 * I'm not sure whether this implementation is 100% correct. [AM]
 */

static HPEN SysColorPens[COLOR_MENUBAR + 1];
static HBRUSH SysColorBrushes[COLOR_MENUBAR + 1];

DWORD_PTR
WINAPI
SetSysColorsTemp(const COLORREF *pPens,
                 const HBRUSH *pBrushes,
				 DWORD_PTR n)
{
    DWORD i;

    if (pPens && pBrushes) /* "set" call */
    {
        /* allocate our structure to remember old colors */
        LPVOID pOldCol = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)+n*sizeof(HPEN)+n*sizeof(HBRUSH));
        LPVOID p = pOldCol;
        *(DWORD_PTR *)p = n; p = (char*)p + sizeof(DWORD);
        memcpy(p, SysColorPens, n*sizeof(HPEN)); p = (char*)p + n*sizeof(HPEN);
        memcpy(p, SysColorBrushes, n*sizeof(HBRUSH)); p = (char*)p + n*sizeof(HBRUSH);

        for (i=0; i < n; i++)
        {
            SysColorPens[i] = CreatePen( PS_SOLID, 1, pPens[i] );
            SysColorBrushes[i] = pBrushes[i];
        }

        return (DWORD_PTR) pOldCol;
    }
    if (!pPens && !pBrushes) /* "restore" call */
    {
        LPVOID pOldCol = (LPVOID)n;
        LPVOID p = pOldCol;
        DWORD nCount = *(DWORD *)p;
        p = (char*)p + sizeof(DWORD);

        for (i=0; i < nCount; i++)
        {
            DeleteObject(SysColorPens[i]);
            SysColorPens[i] = *(HPEN *)p; p = (char*)p + sizeof(HPEN);
        }
        for (i=0; i < nCount; i++)
        {
            SysColorBrushes[i] = *(HBRUSH *)p; p = (char*)p + sizeof(HBRUSH);
        }
        /* get rid of storage structure */
        HeapFree(GetProcessHeap(), 0, pOldCol);

        return TRUE;
    }
    return FALSE;
}

/*
 * @unimplemented
 */
HDESK
WINAPI
GetInputDesktop ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetAccCursorInfo ( PCURSORINFO pci )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetRawInputDeviceInfoW(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
LONG
WINAPI
CsrBroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetRawInputDeviceInfoA(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
LRESULT
WINAPI
DefRawInputProc(
    PRAWINPUT* paRawInput,
    INT nInput,
    UINT cbSizeHeader)
{
  if (cbSizeHeader == sizeof(RAWINPUTHEADER))
     return S_OK;
  return 1;
}

/*
 * @unimplemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
GetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    if(pRawInputDeviceList)
        memset(pRawInputDeviceList, 0, sizeof *pRawInputDeviceList);
    if(puiNumDevices)
       *puiNumDevices = 0;

    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
GetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
RegisterRawInputDevices(
    PCRAWINPUTDEVICE pRawInputDevices,
    UINT uiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI DisplayExitWindowsWarnings(ULONG flags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI ReasonCodeNeedsBugID(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI ReasonCodeNeedsComment(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI CtxInitUser32(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI EnterReaderModeHelper(HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID WINAPI InitializeLpkHooks(FARPROC *hookfuncs)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
WORD WINAPI InitializeWin32EntryTable(UCHAR* EntryTablePlus0x1000)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI IsServerSideWindow(HWND wnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID WINAPI AllowForegroundActivation(VOID)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID WINAPI ShowStartGlass(DWORD unknown)
{
  UNIMPLEMENTED;
}

/*
 * @implemented
 */
DWORD WINAPI GetMenuIndex(HMENU hMenu, HMENU hSubMenu)
{
  return NtUserGetMenuIndex(hMenu, hSubMenu);
}

/*
 * @unimplemented
 */
DWORD WINAPI UserRegisterWowHandlers(PVOID Unknown1, PVOID Unknown2)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
WINAPI
BuildReasonArray(PVOID Pointer)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINAPI
CreateSystemThreads(DWORD Unused)
{
    /* Thread call for remote processes (non-CSRSS) only */
    NtUserxCreateSystemThreads(TRUE);
    ExitThread(0);
}

BOOL
WINAPI
DestroyReasons(PVOID Pointer)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
WINAPI
DeviceEventWorker(HWND hwnd, WPARAM wParam, LPARAM lParam, DWORD Data, ULONG_PTR *uResult)
{
    USER_API_MESSAGE ApiMessage;
    PUSER_DEVICE_EVENT_MSG pusem = &ApiMessage.Data.DeviceEventMsg;

    pusem->hwnd = hwnd;
    pusem->wParam = wParam;
    pusem->lParam = lParam;
    pusem->Data = Data;
    pusem->Result = 0;

    TRACE("DeviceEventWorker : hwnd %p, wParam %d, lParam %d, Data %d, uResult %p\n", hwnd, wParam, lParam, Data, uResult);

    if ( lParam == 0 )
    {
        CsrClientCallServer( (PCSR_API_MESSAGE)&ApiMessage,
                              NULL,
                              CSR_CREATE_API_NUMBER( USERSRV_SERVERDLL_INDEX, UserpDeviceEvent ),
                              sizeof(*pusem) );
    }
    else
    {
        PCSR_CAPTURE_BUFFER pcsrcb = NULL;
        PDEV_BROADCAST_HDR pdev_br = (PDEV_BROADCAST_HDR)lParam;
        ULONG BufferSize = pdev_br->dbch_size;

        pcsrcb = CsrAllocateCaptureBuffer( 1, BufferSize );

        if ( !pcsrcb )
        {
            return STATUS_NO_MEMORY;
        }

        CsrCaptureMessageBuffer( pcsrcb, (PVOID)lParam, BufferSize, (PVOID*)&pusem->lParam );

        CsrClientCallServer( (PCSR_API_MESSAGE)&ApiMessage,
                              pcsrcb,
                              CSR_CREATE_API_NUMBER( USERSRV_SERVERDLL_INDEX, UserpDeviceEvent ),
                              sizeof(*pusem) );

        CsrFreeCaptureBuffer( pcsrcb );
    }

    if (NT_SUCCESS(ApiMessage.Status))
    {
        *uResult = pusem->Result;
    }

    return ApiMessage.Status;
}

BOOL
WINAPI
GetReasonTitleFromReasonCode(DWORD dw1, DWORD dw2, DWORD dw3)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
IsSETEnabled(VOID)
{
    /*
     * Determines whether the Shutdown Event Tracker is enabled.
     *
     * See http://undoc.airesoft.co.uk/user32.dll/IsSETEnabled.php
     * for more information.
     */
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
RecordShutdownReason(DWORD dw0)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
UserLpkTabbedTextOut(
    DWORD dw1,
    DWORD dw2,
    DWORD dw3,
    DWORD dw4,
    DWORD dw5,
    DWORD dw6,
    DWORD dw7,
    DWORD dw8,
    DWORD dw9,
    DWORD dw10,
    DWORD dw11,
    DWORD dw12)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
WINAPI
Win32PoolAllocationStats(DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5)
{
    UNIMPLEMENTED;
    return FALSE;
}

