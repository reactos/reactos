/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native User stubs
 * FILE:             subsys/win32k/ntuser/stubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       04-06-2001  CSH  Created
 */
#include <win32k.h>

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
  NTSTATUS Status;
  PETHREAD Thread, ThreadTo;
  PTHREADINFO pti, ptiTo;
  BOOL Ret = FALSE;

  UserEnterExclusive();
  Status = PsLookupThreadByThreadId((HANDLE)idAttach, &Thread);
  if (!NT_SUCCESS(Status))
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     goto Exit;
  }
  Status = PsLookupThreadByThreadId((HANDLE)idAttachTo, &ThreadTo);
  if (!NT_SUCCESS(Status))
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     ObDereferenceObject(Thread);
     goto Exit;
  }

  pti = PsGetThreadWin32Thread(Thread);
  ptiTo = PsGetThreadWin32Thread(ThreadTo);
  ObDereferenceObject(Thread);
  ObDereferenceObject(ThreadTo);

  Ret = UserAttachThreadInput( pti, ptiTo, fAttach);

Exit:
  UserLeave();
  return Ret;
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
   UserEnterExclusive();

   if ((cbSize != sizeof(MOUSEMOVEPOINT)) || (nBufPoints < 0) || (nBufPoints > 64))
   {
      UserLeave();
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return -1;
   }

   _SEH2_TRY
   {
      ProbeForRead(lppt, cbSize, 1);
      ProbeForWrite(lpptBuf, cbSize, 1);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       EngSetLastError(ERROR_NOACCESS);
   }
   _SEH2_END;

/*
   Call a subfunction of GetMouseMovePointsEx!
   switch(resolution)
   {
     case GMMP_USE_DISPLAY_POINTS:
     case GMMP_USE_HIGH_RESOLUTION_POINTS:
          break;
     default:
        EngSetLastError(GMMP_ERR_POINT_NOT_FOUND);
        return GMMP_ERR_POINT_NOT_FOUND;
   }
  */
   UNIMPLEMENTED
   UserLeave();
   return -1;
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
   NTSTATUS Status = STATUS_SUCCESS;
   DPRINT("Enter NtUserInitializeClientPfnArrays User32 0x%x\n",hmodUser);

   if (ClientPfnInit) return Status;

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead( pfnClientA, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientW, sizeof(PFNCLIENT), 1);
      ProbeForRead( pfnClientWorker, sizeof(PFNCLIENTWORKER), 1);
      RtlCopyMemory(&gpsi->apfnClientA, pfnClientA, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientW, pfnClientW, sizeof(PFNCLIENT));
      RtlCopyMemory(&gpsi->apfnClientWorker, pfnClientWorker, sizeof(PFNCLIENTWORKER));

      //// FIXME! HAX! Temporary until server side is finished.
      //// Copy the client side procs for now.
      RtlCopyMemory(&gpsi->aStoCidPfn, pfnClientW, sizeof(gpsi->aStoCidPfn));

      hModClient = hmodUser;
      ClientPfnInit = TRUE;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status =_SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed reading Client Pfns from user space.\n");
      SetLastNtError(Status);
   }
   
   UserLeave();
   return Status;
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
     UserSendNotifyMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
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

DWORD
APIENTRY
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide)
{
  RECTL NewPos;
  UINT SwFlags;
  PWND pWnd;

  DPRINT("Enter NtUserMinMaximize\n");
  UserEnterExclusive();

  pWnd = UserGetWindowObject(hWnd);
  if ( !pWnd ||                          // FIXME:
        pWnd == IntGetDesktopWindow() || // pWnd->fnid == FNID_DESKTOP
        pWnd == IntGetMessageWindow() )  // pWnd->fnid == FNID_MESSAGEWND
  {
     goto Exit;
  }

  if ( cmd > SW_MAX || pWnd->state2 & WNDS2_INDESTROY)
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     goto Exit;
  }

  co_WinPosMinMaximize(pWnd, cmd, &NewPos);

  SwFlags = Hide ? SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED : SWP_NOZORDER|SWP_FRAMECHANGED;

  co_WinPosSetWindowPos( pWnd,
                         NULL,
                         NewPos.left,
                         NewPos.top,
                         NewPos.right,
                         NewPos.bottom,
                         SwFlags);

  co_WinPosShowWindow(pWnd, cmd);

Exit:
  DPRINT("Leave NtUserMinMaximize\n");
  UserLeave();
  return 0; // Always NULL?
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

NTSTATUS
APIENTRY
NtUserProcessConnect(
    HANDLE Process,
    PUSERCONNECT pUserConnect,
    DWORD Size)
{
  NTSTATUS Status = STATUS_SUCCESS;
  DPRINT("NtUserProcessConnect\n");
  if (pUserConnect && ( Size == sizeof(USERCONNECT) ))
  {
     PPROCESSINFO W32Process;
     UserEnterShared();
     GetW32ThreadInfo();
     W32Process = PsGetCurrentProcessWin32Process();
     _SEH2_TRY
     {
        pUserConnect->siClient.psi = gpsi;
        pUserConnect->siClient.aheList = gHandleTable;
        pUserConnect->siClient.ulSharedDelta = (ULONG_PTR)W32Process->HeapMappings.KernelMapping -
                                               (ULONG_PTR)W32Process->HeapMappings.UserMapping;
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
        Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END
     if (!NT_SUCCESS(Status))
     {
        SetLastNtError(Status);
     }
     UserLeave();
     return Status;
  }
  return STATUS_UNSUCCESSFUL;
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

BOOL
APIENTRY
NtUserRegisterUserApiHook(
    PUNICODE_STRING m_dllname1,
    PUNICODE_STRING m_funname1,
    DWORD dwUnknown3,
    DWORD dwUnknown4)
{
    UserEnterExclusive();
    UNIMPLEMENTED;
    UserLeave();
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

NTSTATUS
APIENTRY
NtUserSetInformationThread(IN HANDLE ThreadHandle,
                           IN USERTHREADINFOCLASS ThreadInformationClass,
                           IN PVOID ThreadInformation,
                           IN ULONG ThreadInformationLength)

{
    if (ThreadInformationClass == UserThreadInitiateShutdown)
    {
        DPRINT1("Shutdown initiated\n");
    }
    else if (ThreadInformationClass == UserThreadEndShutdown)
    {
        DPRINT1("Shutdown ended\n");
    }
    else
    {
        UNIMPLEMENTED;
    }

    return STATUS_SUCCESS;
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

BOOL
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

BOOL
APIENTRY
NtUserValidateTimerCallback(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam)
{
  BOOL Ret = FALSE;

  UserEnterShared();

  Ret = ValidateTimerCallback(PsGetCurrentThreadWin32Thread(), lParam);

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

/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT hRect,
   HMENU hMenu,
   HFONT hFont)
{
   /* we'll use this function just for caching the menu bar */
   UNIMPLEMENTED
   return 0;
}

/*
 * FillWindow: Called from User; Dialog, Edit and ListBox procs during a WM_ERASEBKGND.
 */
/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFillWindow(HWND hWndPaint,
                 HWND hWndPaint1,
                 HDC  hDC,
                 HBRUSH hBrush)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
   UNIMPLEMENTED

   return 1;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserLockWindowUpdate(HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserRealChildWindowFromPoint(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetInternalWindowPos(
   HWND    hwnd,
   UINT    showCmd,
   LPRECT  rect,
   LPPOINT pt)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserUpdateLayeredWindow(
   HWND hwnd,
   HDC hdcDst,
   POINT *pptDst,
   SIZE *psize,
   HDC hdcSrc,
   POINT *pptSrc,
   COLORREF crKey,
   BLENDFUNCTION *pblend,
   DWORD dwFlags,
   RECT *prcDirty)
{
   UNIMPLEMENTED

   return 0;
}

/*
 *    @unimplemented
 */
HWND APIENTRY
NtUserWindowFromPhysicalPoint(POINT Point)
{
   UNIMPLEMENTED

   return NULL;
}

/*
 * NtUserResolveDesktopForWOW
 *
 * Status
 *    @unimplemented
 */

DWORD APIENTRY
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
   UNIMPLEMENTED
   return 0;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserEndMenu(VOID)
{
   UNIMPLEMENTED

   return 0;
}

/*
 * @implemented
 */
/* NOTE: unused function */
BOOL APIENTRY
NtUserTrackPopupMenuEx(
   HMENU hMenu,
   UINT fuFlags,
   int x,
   int y,
   HWND hWnd,
   LPTPMPARAMS lptpm)
{
   UNIMPLEMENTED

   return FALSE;
}

DWORD APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
    UNIMPLEMENTED;

    return 0;
}

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
   UNIMPLEMENTED

   return(0);
}

/*
 * NtUserSetWindowStationUser
 *
 * Status
 *    @unimplemented
 */

DWORD APIENTRY
NtUserSetWindowStationUser(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}

BOOL APIENTRY NtUserAddClipboardFormatListener(
    HWND hwnd
)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY NtUserRemoveClipboardFormatListener(
    HWND hwnd
)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY NtUserGetUpdatedClipboardFormats(
    PUINT lpuiFormats,
    UINT cFormats,
    PUINT pcFormatsOut
)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserGetCursorFrameInfo(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3)
{
    UNIMPLEMENTED

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
