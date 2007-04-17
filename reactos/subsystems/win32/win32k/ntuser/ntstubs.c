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

DWORD
STDCALL
NtUserGetComboBoxInfo(
   DWORD Unknown0,
   DWORD Unknown1)
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
NtUserGetListBoxInfo(
   DWORD Unknown0)
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
NtUserValidateHandleSecure(
   DWORD Unknown0)
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

/* EOF */
