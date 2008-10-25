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
NTAPI
NtUserAssociateInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED
    return 0;
}


BOOL
NTAPI
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
NTAPI
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
NTAPI
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
STDCALL
NtUserConvertMemHandle(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
NTAPI
NtUserDdeGetQualityOfService(
   IN HWND hwndClient,
   IN HWND hWndServer,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
NTAPI
NtUserDdeSetQualityOfService(
   IN  HWND hwndClient,
   IN  PSECURITY_QUALITY_OF_SERVICE pqosNew,
   OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
NTAPI
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
NTAPI
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
STDCALL
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
STDCALL
NtUserEvent(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
NTAPI
NtUserExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
NTAPI
NtUserGetAltTabInfo(
   HWND hwnd,
   INT  iItem,
   PALTTABINFO pati,
   LPTSTR pszItemText,
   UINT   cchItemText,
   BOOL   Ansi)
{
   UNIMPLEMENTED

   return 0;
}

HBRUSH
NTAPI
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
STDCALL
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
STDCALL
NtUserGetCPD(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
STDCALL
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
NTAPI
NtUserImpersonateDdeClientWindow(
   HWND hWndClient,
   HWND hWndServer)
{
   UNIMPLEMENTED

   return 0;
}

NTSTATUS
STDCALL
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
STDCALL
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
NTAPI
NtUserLockWorkStation(VOID)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserMNDragLeave(VOID)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserMNDragOver(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserModifyUserStartupInfoFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserNotifyIMEStatus(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
STDCALL
NtUserRegisterTasklist(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
STDCALL
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
STDCALL
NtUserSetConsoleReserveKeys(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserSetDbgTag(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
STDCALL
NtUserSetRipFlags(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
STDCALL
NtUserSetSysColors(
   int cElements,
   IN CONST INT *lpaElements,
   IN CONST COLORREF *lpaRgbValues,
   FLONG Flags)
{
  DWORD Ret = FALSE;
  NTSTATUS Status = STATUS_SUCCESS;
  UserEnterExclusive();
  _SEH_TRY
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
  _SEH_HANDLE
  {
      Status = _SEH_GetExceptionCode();
  }
  _SEH_END;
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
STDCALL
NtUserSetThreadState(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
NTAPI
NtUserTrackMouseEvent(
   LPTRACKMOUSEEVENT lpEventTrack)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
STDCALL
NtUserUpdateInputContext(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserUpdateInstance(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
NTAPI
NtUserUserHandleGrantAccess(
   IN HANDLE hUserHandle,
   IN HANDLE hJob,
   IN BOOL bGrant)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserWaitForMsgAndEvent(
   DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
STDCALL
NtUserYieldTask(VOID)
{
   UNIMPLEMENTED

   return 0;
}


DWORD
STDCALL
NtUserCheckImeHotKey(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserConsoleControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserCreateInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserDestroyInputContext(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserDisableThreadIme(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetAppImeLevel(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetAtomName(
    ATOM nAtom,
    LPWSTR lpBuffer)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
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
NTAPI
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserInitialize(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
NtUserPrintWindow(
    HWND hwnd,
    HDC  hdcBlt,
    UINT nFlags)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
NtUserProcessConnect(
    HANDLE Process,
    PUSERCONNECT pUserConnect,
    DWORD Size)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
NtUserQueryInputContext(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserRegisterUserApiHook(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
NTAPI
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserSetImeInfoEx(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
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
NTAPI
NtUserSetThreadLayoutHandles(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
NTAPI
NtUserSoundSentry(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

/* http://www.cyber-ta.org/releases/malware-analysis/public/SOURCES/b47155634ccb2c30630da7e3666d3d07/b47155634ccb2c30630da7e3666d3d07.trace.html#NtUserGetIconSize */
DWORD
NTAPI
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
NTAPI
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
NTAPI
NtUserUnregisterUserApiHook(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
NTAPI
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
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserValidateTimerCallback(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
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
NTAPI
NtUserRemoteRedrawScreen(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}


/* EOF */
