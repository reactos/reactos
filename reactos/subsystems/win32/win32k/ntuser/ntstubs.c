/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native User stubs
 * FILE:             subsys/win32k/ntuser/stubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       04-06-2001  CSH  Created
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>

DWORD
APIENTRY
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED
    return 0;
}


BOOL
APIENTRY
NtUserAttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach)
{
   UNIMPLEMENTED

   return 0;
}

//
// Works like BitBlt, http://msdn.microsoft.com/en-us/library/ms532278(VS.85).aspx
//
BOOL
APIENTRY
NtUserBitBltSysBmp(
   HDC hdc,
   INT nXDest,
   INT nYDest,
   INT nWidth,
   INT nHeight,
   INT nXSrc,
   INT nYSrc,
   DWORD dwRop )
{
   BOOL Ret = FALSE;
   UserEnterExclusive();

   Ret = NtGdiBitBlt( hdc,
                   nXDest,
                   nYDest,
                   nWidth, 
                  nHeight, 
                hSystemBM,
                    nXSrc, 
                    nYSrc, 
                    dwRop,
                        0,
                        0);

   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserBuildHimcList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserConvertMemHandle(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserCreateLocalMemHandle(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserDdeGetQualityOfService(
   IN HWND hwndClient,
   IN HWND hWndServer,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserDdeInitialize(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserDdeSetQualityOfService(
   IN  HWND hwndClient,
   IN  PSECURITY_QUALITY_OF_SERVICE pqosNew,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserDragObject(
   HWND    hwnd1,
   HWND    hwnd2,
   UINT    u1,
   DWORD   dw1,
   HCURSOR hc1
)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserDrawAnimatedRects(
   HWND hwnd,
   INT idAni,
   RECT *lprcFrom,
   RECT *lprcTo)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserEnumDisplayDevices (
   PUNICODE_STRING lpDevice, /* device name */
   DWORD iDevNum, /* display device */
   PDISPLAY_DEVICEW lpDisplayDevice, /* device information */
   DWORD dwFlags ) /* reserved */
{
   DPRINT1("NtUserEnumDisplayDevices() is UNIMPLEMENTED!\n");
   if (lpDevice->Length == 0 && iDevNum > 0)
   {
      /* Only one display device present */
      return FALSE;
   }
   else if (lpDevice->Length != 0)
   {
       /* Can't enumerate monitors :( */
       return FALSE;
   }
   if (lpDisplayDevice->cb < sizeof(DISPLAY_DEVICE))
      return FALSE;

   wcscpy(lpDisplayDevice->DeviceName, L"\\\\.\\DISPLAY1");
   wcscpy(lpDisplayDevice->DeviceString, L"<Unknown>");
   lpDisplayDevice->StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP
                                 | DISPLAY_DEVICE_MODESPRUNED
                                 | DISPLAY_DEVICE_PRIMARY_DEVICE
                                 | DISPLAY_DEVICE_VGA_COMPATIBLE;
   lpDisplayDevice->DeviceID[0] = L'0';
   lpDisplayDevice->DeviceKey[0] = L'0';
   return TRUE;
}

DWORD
APIENTRY
NtUserEvent(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPWSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi)
{
   UNIMPLEMENTED

   return 0;
}

HBRUSH
APIENTRY
NtUserGetControlBrush(
   HWND hwnd,
   HDC  hdc,
   UINT ctlType)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * Called from PaintRect, works almost like wine PaintRect16 but returns hBrush.
 */
HBRUSH
APIENTRY
NtUserGetControlColor(
   HWND hwndParent,
   HWND hwnd, 
   HDC hdc,
   UINT CtlMsg) // Wine PaintRect: WM_CTLCOLORMSGBOX + hbrush
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserGetCPD(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserGetImeHotKey(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserGetMouseMovePointsEx(
   UINT cbSize,
   LPMOUSEMOVEPOINT lppt,
   LPMOUSEMOVEPOINT lpptBuf,
   int nBufPoints,
   DWORD resolution)
{
/*
   if (cbSize != sizeof (MOUSEMOVEPOINT)
   {
       SetLastWin32Error(GMMP_ERR_POINT_NOT_FOUND);
       return GMMP_ERR_POINT_NOT_FOUND;
   }

   if (!lppt)
   {
       SetLastWin32Error(GMMP_ERR_POINT_NOT_FOUND);
       return GMMP_ERR_POINT_NOT_FOUND;
   }

   if (!lpptBuf)
   {
       SetLastWin32Error(GMMP_ERR_POINT_NOT_FOUND);
       return GMMP_ERR_POINT_NOT_FOUND;
   }

   switch(resolution)
   {
     case GMMP_USE_DISPLAY_POINTS:
     case GMMP_USE_HIGH_RESOLUTION_POINTS:
          break;
     default:
        SetLastWin32Error(GMMP_ERR_POINT_NOT_FOUND);
        return GMMP_ERR_POINT_NOT_FOUND;
   }
  */
   UNIMPLEMENTED

   return 0;
}



BOOL
APIENTRY
NtUserImpersonateDdeClientWindow(
   HWND hWndClient,
   HWND hWndServer)
{
   UNIMPLEMENTED

   return 0;
}

NTSTATUS
APIENTRY
NtUserInitializeClientPfnArrays(
  PPFNCLIENT pfnClientA,
  PPFNCLIENT pfnClientW,
  PPFNCLIENTWORKER pfnClientWorker,
  HINSTANCE hmodUser)
{
   UNIMPLEMENTED

   return STATUS_UNSUCCESSFUL;
}

DWORD
APIENTRY
NtUserInitTask(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5,
   DWORD Unknown6,
   DWORD Unknown7,
   DWORD Unknown8,
   DWORD Unknown9,
   DWORD Unknown10,
   DWORD Unknown11)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserLockWorkStation(VOID)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserMNDragLeave(VOID)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserMNDragOver(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserModifyUserStartupInfoFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserNotifyIMEStatus(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserQueryUserCounters(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserRegisterTasklist(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserSBGetParms(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserSetConsoleReserveKeys(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserSetDbgTag(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserSetImeHotKey(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserSetRipFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserSetSysColors(
   int cElements,
   IN CONST INT *lpaElements,
   IN CONST COLORREF *lpaRgbValues,
   FLONG Flags)
{
  DWORD Ret = FALSE;
  NTSTATUS Status = STATUS_SUCCESS;
  UserEnterExclusive();
  _SEH2_TRY
  {
     ProbeForRead(lpaElements,
                   sizeof(INT),
                   1);
     ProbeForRead(lpaRgbValues,
                   sizeof(INT),
                   1);
// Developers: We are thread locked and calling gdi.
     Ret = IntSetSysColors(cElements, (INT*)lpaElements, (COLORREF*)lpaRgbValues);
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
      Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;
  if (!NT_SUCCESS(Status))
  {
      SetLastNtError(Status);
      Ret = FALSE;
  }
  if (Ret)
  {
     UserPostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
  }
  UserLeave();
  return Ret;
}

DWORD
APIENTRY
NtUserSetThreadState(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserTrackMouseEvent(
   LPTRACKMOUSEEVENT lpEventTrack)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserUpdateInputContext(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserUpdateInstance(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
APIENTRY
NtUserUserHandleGrantAccess(
   IN HANDLE hUserHandle,
   IN HANDLE hJob,
   IN BOOL bGrant)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserWaitForMsgAndEvent(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserWin32PoolAllocationStats(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
APIENTRY
NtUserYieldTask(VOID)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
APIENTRY
NtUserCheckImeHotKey(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserConsoleControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCreateInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDestroyInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDisableThreadIme(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetAppImeLevel(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetAtomName(
    ATOM nAtom,
    LPWSTR lpBuffer)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

/*
    Called from win32csr.
 */
NTSTATUS
APIENTRY
NtUserInitialize(
  DWORD   dwWinVersion,
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    UserEnterExclusive();
    UNIMPLEMENTED;
// Check to see we have the right version.
// Initialize Power Request List.
// Initialize Media Change.
// Initialize CSRSS
// {
//    Startup DxGraphics.
//    calls ** IntGdiGetLanguageID() and sets it **.
//    Enables Fonts drivers, Initialize Font table & Stock Fonts.
// }
// Set W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED | W32PF_IOWINSTA)
// Create Object Directory,,, Looks like create workstation. "\\Windows\\WindowStations"
// Create Event for Diconnect Desktop.
// Initialize Video.
// {
//     DrvInitConsole.
//     DrvChangeDisplaySettings.
//     Update Shared Device Caps.
//     Initialize User Screen.
// }
// Create ThreadInfo for this Thread!
// Set Global SERVERINFO Error flags.
// Load Resources.
    UserLeave();
    return STATUS_SUCCESS;
}

DWORD
APIENTRY
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserNotifyProcessCreate(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserPrintWindow(
    HWND hwnd,
    HDC  hdcBlt,
    UINT nFlags)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
APIENTRY
NtUserProcessConnect(
    HANDLE Process,
    PUSERCONNECT pUserConnect,
    DWORD Size)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserQueryInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRealInternalGetMessage(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserResolveDesktop(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeInfoEx(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationThread(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSoundSentry(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserTestForInteractiveUser(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

/* http://www.cyber-ta.org/releases/malware-analysis/public/SOURCES/b47155634ccb2c30630da7e3666d3d07/b47155634ccb2c30630da7e3666d3d07.trace.html#NtUserGetIconSize */
DWORD
APIENTRY
NtUserCalcMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserPaintMenuBar(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserUnregisterUserApiHook(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
    UNIMPLEMENTED;
    return 0;
}

/* ValidateRect gets redirected to NtUserValidateRect:
   http://blog.csdn.net/ntdll/archive/2005/10/19/509299.aspx */
BOOL
APIENTRY
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserValidateTimerCallback(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam)
{
  BOOL Ret = FALSE;
  PWINDOW_OBJECT Window = NULL;

  UserEnterShared();

  if (hWnd)
  {
     Window = UserGetWindowObject(hWnd);
     if (!Window || !Window->Wnd)
        goto Exit;
  }

  Ret = ValidateTimerCallback(GetW32ThreadInfo(), Window, wParam, lParam);

Exit:
  UserLeave();
  return Ret;
}

DWORD
APIENTRY
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteRedrawScreen(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRemoteStopScreenUpdates(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}


/* EOF */
