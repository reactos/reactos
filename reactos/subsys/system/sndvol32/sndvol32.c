/*
 * ReactOS Sound Volume Control
 * Copyright (C) 2004 Thomas Weidenmueller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * VMware is a registered trademark of VMware, Inc.
 */
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        subsys/system/sndvol32/sndvol32.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */
#include "sndvol32.h"

HINSTANCE hAppInstance;
ATOM MainWindowClass;
HWND hMainWnd;
HANDLE hAppHeap;

#define GetDialogData(hwndDlg, type) \
  ( P##type )GetWindowLongPtr((hwndDlg), DWLP_USER)
#define GetWindowData(hwnd, type) \
  ( P##type )GetWindowLongPtr((hwnd), GWL_USERDATA)

/******************************************************************************/

typedef struct _PREFERENCES_CONTEXT
{
  PMIXER_WINDOW MixerWindow;
  PSND_MIXER Mixer;
  HWND hwndDlg;
} PREFERENCES_CONTEXT, *PPREFERENCES_CONTEXT;

typedef struct _PREFERENCES_FILL_DEVICES
{
  PPREFERENCES_CONTEXT PrefContext;
  HWND hComboBox;
  UINT Selected;
} PREFERENCES_FILL_DEVICES, *PPREFERENCES_FILL_DEVICES;

static BOOL CALLBACK
FillDeviceComboBox(PSND_MIXER Mixer, UINT Id, LPCTSTR ProductName, PVOID Context)
{
  LRESULT lres;
  PPREFERENCES_FILL_DEVICES FillContext = (PPREFERENCES_FILL_DEVICES)Context;
  
  lres = SendMessage(FillContext->hComboBox, CB_ADDSTRING, 0, (LPARAM)ProductName);
  if(lres != CB_ERR)
  {
    /* save the index so we don't screw stuff when the combobox is sorted... */
    SendMessage(FillContext->hComboBox, CB_SETITEMDATA, (WPARAM)lres, Id);

    if(Id == FillContext->Selected)
    {
      SendMessage(FillContext->hComboBox, CB_SETCURSEL, (WPARAM)lres, 0);
    }
  }
  
  return TRUE;
}

static INT_PTR CALLBACK
DlgPreferencesProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PPREFERENCES_CONTEXT Context;
  
  switch(uMsg)
  {
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDOK:
        case IDCANCEL:
        {
          EndDialog(hwndDlg, LOWORD(wParam));
          break;
        }
      }
      break;
    }
    
    case MM_MIXM_LINE_CHANGE:
    {
      DBG("MM_MIXM_LINE_CHANGE\n");
      break;
    }

    case MM_MIXM_CONTROL_CHANGE:
    {
      DBG("MM_MIXM_CONTROL_CHANGE\n");
      break;
    }
    
    case WM_INITDIALOG:
    {
      PREFERENCES_FILL_DEVICES FillDevContext;
      
      SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
      Context = (PPREFERENCES_CONTEXT)((LONG_PTR)lParam);
      Context->hwndDlg = hwndDlg;
      Context->Mixer = SndMixerCreate(hwndDlg);
      
      FillDevContext.PrefContext = Context;
      FillDevContext.hComboBox = GetDlgItem(hwndDlg, IDC_MIXERDEVICE);
      FillDevContext.Selected = SndMixerGetSelection(Context->Mixer);
      SndMixerEnumProducts(Context->Mixer,
                           FillDeviceComboBox,
                           &FillDevContext);
      return TRUE;
    }
    
    case WM_DESTROY:
    {
      Context = GetDialogData(hwndDlg, PREFERENCES_CONTEXT);
      if(Context->Mixer != NULL)
      {
        SndMixerDestroy(Context->Mixer);
      }
      break;
    }

    case WM_CLOSE:
    {
      EndDialog(hwndDlg, IDCANCEL);
      break;
    }
  }
  
  return 0;
}

/******************************************************************************/

static VOID
DeleteMixerWindowControls(PMIXER_WINDOW MixerWindow)
{
}

BOOL
RebuildMixerWindowControls(PMIXER_WINDOW MixerWindow)
{
  DeleteMixerWindowControls(MixerWindow);
  
  return TRUE;
}

LRESULT CALLBACK
MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PMIXER_WINDOW MixerWindow;
  LRESULT Result = 0;
  
  switch(uMsg)
  {
    case WM_COMMAND:
    {
      MixerWindow = GetWindowData(hwnd, MIXER_WINDOW);
      
      switch(LOWORD(wParam))
      {
        case IDC_PROPERTIES:
        {
          PREFERENCES_CONTEXT Preferences;

          Preferences.MixerWindow = MixerWindow;
          Preferences.Mixer = NULL;
          
          if(DialogBoxParam(hAppInstance,
                            MAKEINTRESOURCE(IDD_PREFERENCES),
                            hwnd,
                            DlgPreferencesProc,
                            (LPARAM)&Preferences) == IDOK)
          {
            /* FIXME - update window */
          }
          break;
        }
        
        case IDC_EXIT:
        {
          PostQuitMessage(0);
          break;
        }
      }
      break;
    }
    
    case MM_MIXM_LINE_CHANGE:
    {
      DBG("MM_MIXM_LINE_CHANGE\n");
      break;
    }
    
    case MM_MIXM_CONTROL_CHANGE:
    {
      DBG("MM_MIXM_CONTROL_CHANGE\n");
      break;
    }
    
    case WM_CREATE:
    {
      MixerWindow = ((LPCREATESTRUCT)lParam)->lpCreateParams;
      SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)MixerWindow);
      MixerWindow->hWnd = hwnd;
      MixerWindow->hStatusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                                                   NULL, hwnd, 0);
      if(MixerWindow->hStatusBar != NULL)
      {
        MixerWindow->Mixer = SndMixerCreate(MixerWindow->hWnd);
        if(MixerWindow->Mixer != NULL)
        {
          TCHAR szProduct[MAXPNAMELEN];

          if(SndMixerGetProductName(MixerWindow->Mixer, szProduct, sizeof(szProduct) / sizeof(szProduct[0])) > 0)
          {
            SendMessage(MixerWindow->hStatusBar, WM_SETTEXT, 0, (LPARAM)szProduct);
          }
          
          if(!RebuildMixerWindowControls(MixerWindow))
          {
            DBG("Rebuilding mixer window controls failed!\n");
            SndMixerDestroy(MixerWindow->Mixer);
            Result = -1;
          }
        }
        else
        {
          Result = -1;
        }
      }
      else
      {
        DBG("Failed to create status window!\n");
        Result = -1;
      }
      break;
    }
    
    case WM_DESTROY:
    {
      MixerWindow = GetWindowData(hwnd, MIXER_WINDOW);
      if(MixerWindow->Mixer != NULL)
      {
        SndMixerDestroy(MixerWindow->Mixer);
      }
      break;
    }
    
    case WM_CLOSE:
    {
      PostQuitMessage(0);
      break;
    }
    
    default:
    {
      Result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
  }

  return Result;
}

static BOOL
RegisterApplicationClasses(VOID)
{
  WNDCLASSEX wc;
  
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = sizeof(PMIXER_WINDOW);
  wc.hInstance = hAppInstance;
  wc.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_MAINAPP));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = SZ_APP_CLASS;
  wc.hIconSm = NULL;
  MainWindowClass = RegisterClassEx(&wc);
  
  return MainWindowClass != 0;
}

static VOID
UnregisterApplicationClasses(VOID)
{
  UnregisterClass(SZ_APP_CLASS, hAppInstance);
}

static HWND
CreateApplicationWindow(VOID)
{
  LPTSTR lpAppTitle;
  HWND hWnd;
  
  PMIXER_WINDOW MixerWindow = HeapAlloc(hAppHeap, 0, sizeof(MIXER_WINDOW));
  if(MixerWindow == NULL)
  {
    return NULL;
  }
  
  /* load the application title */
  if(RosAllocAndLoadString(&lpAppTitle,
                           hAppInstance,
                           IDS_SNDVOL32) == 0)
  {
    lpAppTitle = NULL;
  }

  if(mixerGetNumDevs() > 0)
  {
    hWnd = CreateWindowEx(WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT,
                          SZ_APP_CLASS,
                          lpAppTitle,
                          WS_DLGFRAME | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL,
                          LoadMenu(hAppInstance, MAKEINTRESOURCE(IDM_MAINMENU)),
                          hAppInstance,
                          MixerWindow);
  }
  else
  {
    LPTSTR lpErrMessage;
    
    /*
     * no mixer devices are available!
     */

    hWnd = NULL;
    RosAllocAndLoadString(&lpErrMessage,
                          hAppInstance,
                          IDS_NOMIXERDEVICES);
    MessageBox(NULL, lpErrMessage, lpAppTitle, MB_ICONINFORMATION);
    LocalFree(lpErrMessage);
  }
  
  if(lpAppTitle != NULL)
  {
    LocalFree(lpAppTitle);
  }
  
  if(hWnd == NULL)
  {
    HeapFree(hAppHeap, 0, MixerWindow);
  }
  
  return hWnd;
}

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  MSG Msg;
  
  hAppInstance = hInstance;
  hAppHeap = GetProcessHeap();
  
  InitCommonControls();

  if(!RegisterApplicationClasses())
  {
    DBG("Failed to register application classes (LastError: %d)!\n", GetLastError());
    return 1;
  }
  
  hMainWnd = CreateApplicationWindow();
  if(hMainWnd == NULL)
  {
    DBG("Failed to creat application window (LastError: %d)!\n", GetLastError());
    return 1;
  }

  while(GetMessage(&Msg, NULL, 0, 0))
  {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  DestroyWindow(hMainWnd);

  UnregisterApplicationClasses();

  return 0;
}

