/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/sas.c
 * PURPOSE:         Secure Attention Sequence 
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 28/03/2004
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <ntsecapi.h>
#include <wchar.h>
#include <userenv.h>
#include <reactos/winlogon.h>

#include "setup.h"
#include "winlogon.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

#define HK_CTRL_ALT_DEL 0
#define HK_CTRL_SHIFT_ESC   1

#ifdef __USE_W32API
extern BOOL STDCALL SetLogonNotifyWindow(HWND Wnd, HWINSTA WinSta);
#endif

void
DispatchSAS(PWLSESSION Session, DWORD dwSasType)
{
  Session->SASAction = dwSasType;
  
}

void
UninitSAS(PWLSESSION Session)
{
  if(Session->SASWindow)
  {
    DestroyWindow(Session->SASWindow);
    Session->SASWindow = NULL;
  }
}

BOOL
SetupSAS(PWLSESSION Session, HWND hwndSAS)
{
  /* Register Ctrl+Alt+Del Hotkey */
  if(!RegisterHotKey(hwndSAS, HK_CTRL_ALT_DEL, MOD_CONTROL | MOD_ALT, VK_DELETE))
  {
    DPRINT1("WL-SAS: Unable to register Ctrl+Alt+Del hotkey!\n");
    return FALSE;
  }
  
  /* Register Ctrl+Shift+Esc */
  Session->TaskManHotkey = RegisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE);
  if(!Session->TaskManHotkey)
  {
    DPRINT1("WL-SAS: Warning: Unable to register Ctrl+Alt+Esc hotkey!\n");
  }
  return TRUE;
}

BOOL
DestroySAS(PWLSESSION Session, HWND hwndSAS)
{
  /* Unregister hotkeys */
  
  UnregisterHotKey(hwndSAS, HK_CTRL_ALT_DEL);
  
  if(Session->TaskManHotkey)
  {
    UnregisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC);
  }
  
  return TRUE;
}

#define EWX_ACTION_MASK 0x0b
static LRESULT
HandleExitWindows(DWORD RequestingProcessId, UINT Flags)
{
  UINT Action;
  HANDLE Process;
  HANDLE Token;
  BOOL CheckResult;
  PPRIVILEGE_SET PrivSet;

  /* Check parameters */
  Action = Flags & EWX_ACTION_MASK;
  if (EWX_LOGOFF != Action && EWX_SHUTDOWN != Action && EWX_REBOOT != Action
      && EWX_POWEROFF != Action)
  {
    DPRINT1("Invalid ExitWindows action 0x%x\n", Action);
    return STATUS_INVALID_PARAMETER;
  }

  /* Check privilege */
  if (EWX_LOGOFF != Action)
  {
    Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, RequestingProcessId);
    if (NULL == Process)
    {
      DPRINT1("OpenProcess failed with error %d\n", GetLastError());
      return STATUS_INVALID_HANDLE;
    }
    if (! OpenProcessToken(Process, TOKEN_QUERY, &Token))
    {
      DPRINT1("OpenProcessToken failed with error %d\n", GetLastError());
      CloseHandle(Process);
      return STATUS_INVALID_HANDLE;
    }
    CloseHandle(Process);
    PrivSet = HeapAlloc(GetProcessHeap(), 0, sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES));
    if (NULL == PrivSet)
    {
      DPRINT1("Failed to allocate mem for privilege set\n");
      CloseHandle(Token);
      return STATUS_NO_MEMORY;
    }
    PrivSet->PrivilegeCount = 1;
    PrivSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    if (! LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &PrivSet->Privilege[0].Luid))
    {
      DPRINT1("LookupPrivilegeValue failed with error %d\n", GetLastError());
      HeapFree(GetProcessHeap(), 0, PrivSet);
      CloseHandle(Token);
      return STATUS_UNSUCCESSFUL;
    }
    if (! PrivilegeCheck(Token, PrivSet, &CheckResult))
    {
      DPRINT1("PrivilegeCheck failed with error %d\n", GetLastError());
      HeapFree(GetProcessHeap(), 0, PrivSet);
      CloseHandle(Token);
      return STATUS_ACCESS_DENIED;
    }
    HeapFree(GetProcessHeap(), 0, PrivSet);
    CloseHandle(Token);
    if (! CheckResult)
    {
      DPRINT1("SE_SHUTDOWN privilege not enabled\n");
      return STATUS_ACCESS_DENIED;
    }
  }

  /* FIXME actually start logoff/shutdown now */

  return 1;
}

LRESULT CALLBACK
SASProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PWLSESSION Session = (PWLSESSION)GetWindowLongPtr(hwnd, GWL_USERDATA);
  if(!Session)
  {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
  switch(uMsg)
  {
    case WM_HOTKEY:
    {
      switch(wParam)
      {
        case HK_CTRL_ALT_DEL:
          DPRINT1("SAS: CTR+ALT+DEL\n");
          break;
        case HK_CTRL_SHIFT_ESC:
          DPRINT1("SAS: CTR+SHIFT+ESC\n");
          break;
      }
      return 0;
    }
    case WM_CREATE:
    {
      if(!SetupSAS(Session, hwnd))
      {
        /* Fail! */
        return 1;
      }
      return 0;
    }
    case PM_WINLOGON_EXITWINDOWS:
    {
      return HandleExitWindows((DWORD) wParam, (UINT) lParam);
    }
    case WM_DESTROY:
    {
      DestroySAS(Session, hwnd);
      return 0;
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL
InitializeSAS(PWLSESSION Session)
{
  WNDCLASSEX swc;
  
  /* register SAS window class.
     WARNING! MAKE SURE WE ARE IN THE WINLOGON DESKTOP! */
  swc.cbSize = sizeof(WNDCLASSEXW);
  swc.style = CS_SAVEBITS;
  swc.lpfnWndProc = SASProc;
  swc.cbClsExtra = 0;
  swc.cbWndExtra = 0;
  swc.hInstance = hAppInstance;
  swc.hIcon = NULL;
  swc.hCursor = NULL;
  swc.hbrBackground = NULL;
  swc.lpszMenuName = NULL;
  swc.lpszClassName = WINLOGON_SAS_CLASS;
  swc.hIconSm = NULL;
  RegisterClassEx(&swc);
  
  /* create invisible SAS window */
  Session->SASWindow = CreateWindowEx(0, WINLOGON_SAS_CLASS, WINLOGON_SAS_TITLE, WS_POPUP,
                                      0, 0, 0, 0, 0, 0, hAppInstance, NULL);
  if(!Session->SASWindow)
  {
    DPRINT1("WL: Failed to create SAS window\n");
    return FALSE;
  }
  
  /* Save the Session pointer so the window proc can access it */
  SetWindowLongPtr(Session->SASWindow, GWL_USERDATA, (DWORD_PTR)Session);
  
  /* Register SAS window to receive SAS notifications */
  if(!SetLogonNotifyWindow(Session->SASWindow, Session->InteractiveWindowStation))
  {
    UninitSAS(Session);
    DPRINT1("WL: Failed to register SAS window\n");
    return FALSE;
  }
  
  return TRUE;
}

