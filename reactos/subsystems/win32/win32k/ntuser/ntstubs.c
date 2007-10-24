/* $Id$
 *
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

DWORD
STDCALL
NtUserAttachThreadInput(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserBitBltSysBmp(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4,
   DWORD Unknown5,
   DWORD Unknown6,
   DWORD Unknown7)
{
   UNIMPLEMENTED

   return 0;
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
NtUserCallHwnd(
   DWORD Unknown0,
   DWORD Unknown1)
{
   switch (Unknown0)
   {
      case HWND_ROUTINE_REGISTERSHELLHOOKWINDOW:
         if (IntIsWindow((HWND) Unknown1))
            return IntRegisterShellHookWindow((HWND) Unknown1);
         return FALSE;
         break;
      case HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW:
         if (IntIsWindow((HWND) Unknown1))
            return IntDeRegisterShellHookWindow((HWND) Unknown1);
         return FALSE;
   }
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserCallHwndParam(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserCallHwndParamLock(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

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

DWORD
STDCALL
NtUserDdeGetQualityOfService(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
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

DWORD
STDCALL
NtUserDdeSetQualityOfService(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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

DWORD
STDCALL
NtUserDrawAnimatedRects(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}

BOOL
STDCALL
NtUserEnumDisplayDevices (
   PUNICODE_STRING lpDevice, /* device name */
   DWORD iDevNum, /* display device */
   PDISPLAY_DEVICE lpDisplayDevice, /* device information */
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
STDCALL
NtUserExcludeUpdateRgn(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserGetAltTabInfo(
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

BOOL
STDCALL
NtUserGetComboBoxInfo(
   HWND hWnd,
   PCOMBOBOXINFO pcbi)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserGetControlBrush(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserGetControlColor(
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

DWORD
STDCALL
NtUserGetTitleBarInfo(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserImpersonateDdeClientWindow(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserInitializeClientPfnArrays(
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
   DWORD Unknown10)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
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
NtUserNotifyWinEvent(
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

DWORD
STDCALL
NtUserSetSysColors(
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
NtUserSetThreadState(
   DWORD Unknown0,
   DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserTrackMouseEvent(
   DWORD Unknown0)
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

DWORD
STDCALL
NtUserUserHandleGrantAccess(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

DWORD
STDCALL
NtUserWaitForInputIdle(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
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
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

/* for hints how the prototype might be, see
   http://forum.grafika.cz/read.php?23,1816012,1816139,quote=1
   http://www.cyber-ta.org/releases/malware-analysis/public/SOURCES/b47155634ccb2c30630da7e3666d3d07/b47155634ccb2c30630da7e3666d3d07.trace.html#NtUserGetIconSize */
BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    DWORD dwUnknown2, // Most of the time Zero.
    PLONG plcx,       // &size.cx
    PLONG plcy)       // &size.cy
{
    UNIMPLEMENTED;
    return FALSE;
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
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetRawInputData(
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
NtUserGetRawInputDeviceInfo(
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
NtUserGetRawInputDeviceList(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
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
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
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

DWORD
NTAPI
NtUserPrintWindow(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
NtUserProcessConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
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

/* http://www.reactos.org/pipermail/ros-kernel/2003-November/000589.html */
HWINSTA
NTAPI
NtUserRegisterClassExWOW(
    CONST WNDCLASSEXW* lpwcx,
    BOOL bUnicodeClass,
    WNDPROC wpExtra,
    DWORD dwUnknown4,
    DWORD dwUnknown5,
    DWORD dwUnknown6,
    DWORD dwUnknown7)
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

DWORD
NTAPI
NtUserRegisterRawInputDevices(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3)
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

DWORD
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

DWORD
NTAPI
NtUserGetLayeredWindowAttributes(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
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
