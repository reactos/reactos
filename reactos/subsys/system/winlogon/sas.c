/* $Id: sas.c,v 1.1 2004/03/28 12:21:41 weiden Exp $
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

#include "setup.h"
#include "winlogon.h"
#include "resource.h"

#define SAS_CLASS   L"SAS window class"
#define HK_CTRL_ALT_DEL 0
#define HK_CTRL_SHIFT_ESC   1

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
    DbgPrint("WL-SAS: Unable to register Ctrl+Alt+Del hotkey!\n");
    return FALSE;
  }
  
  /* Register Ctrl+Shift+Esc */
  Session->TaskManHotkey = RegisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE);
  if(!Session->TaskManHotkey)
  {
    DbgPrint("WL-SAS: Warning: Unable to register Ctrl+Alt+Esc hotkey!\n");
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

LRESULT CALLBACK
SASProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PWLSESSION Session = (PWLSESSION)GetWindowLong(hwnd, GWL_USERDATA);
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
          DbgPrint("SAS: CTR+ALT+DEL\n");
          break;
        case HK_CTRL_SHIFT_ESC:
          DbgPrint("SAS: CTR+SHIFT+ESC\n");
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
  swc.lpszClassName = SAS_CLASS;
  swc.hIconSm = NULL;
  RegisterClassEx(&swc);
  
  /* create invisible SAS window */
  Session->SASWindow = CreateWindowEx(0, SAS_CLASS, L"SAS", WS_OVERLAPPEDWINDOW,
                                      0, 0, 0, 0, 0, 0, hAppInstance, NULL);
  if(!Session->SASWindow)
  {
    DbgPrint("WL: Failed to create SAS window\n");
    return FALSE;
  }
  
  /* Save the Session pointer so the window proc can access it */
  SetWindowLong(Session->SASWindow, GWL_USERDATA, (LONG)Session);
  
  #if 0
  /* Register SAS window to receive SAS notifications
     FIXME - SetLogonNotifyWindow() takes only one parameter,
             definition in funcs.h is wrong! */
  if(!SetLogonNotifyWindow(Session->SASWindow))
  {
    UninitSAS(Session);
    DbgPrint("WL: Failed to register SAS window\n");
    return FALSE;
  }
  #endif
  
  return TRUE;
}

