/*
 *  ReactOS applications
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: vmwinst.c,v 1.2 2004/04/12 16:09:45 weiden Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VMware(r) driver installation utility
 * FILE:        subsys/system/vmwinst/vmwinst.c
 * PROGRAMMERS: Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include "vmwinst.h"

HINSTANCE hAppInstance;
BOOL StartVMwConfigWizard, DriverFilesFound, ActivateVBE = FALSE, UninstallDriver = FALSE;

static WCHAR DestinationDriversPath[MAX_PATH+1];
static WCHAR CDDrive = L'\0';
static WCHAR PathToVideoDrivers45[MAX_PATH+1] = L"X:\\program files\\VMware\\VMware Tools\\Drivers\\video\\winnt2k\\";
static WCHAR PathToVideoDrivers40[MAX_PATH+1] = L"X:\\video\\winnt2k\\";
static WCHAR DestinationPath[MAX_PATH+1];
static WCHAR *vmx_fb = L"vmx_fb.dll";
static WCHAR *vmx_mode = L"vmx_mode.dll";
static WCHAR *vmx_svga = L"vmx_svga.sys";

static WCHAR *SrcPath = PathToVideoDrivers45;

/* Helper functions */

LONG WINAPI ExceptionHandler(LPEXCEPTION_POINTERS ExceptionInfo)
{
  /* This is rude, but i don't know how to continue execution properly, that's why
     we just exit here when we're not running inside of VMware */
  ExitProcess(ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_PRIVILEGED_INSTRUCTION);
  return EXCEPTION_CONTINUE_EXECUTION;
}

BOOL
DetectVMware(int *Version)
{
  int magic, ver;
  
  magic = 0;
  ver = 0;
  
  /* Try using a VMware I/O port. If not running in VMware this'll throw an
     exception! */
  __asm__ __volatile__("inl  %%dx, %%eax"
    : "=a" (ver), "=b" (magic)
    : "0" (0x564d5868), "d" (0x5658), "c" (0xa));

  if(magic == 0x564d5868)
  {
    *Version = ver;
    return TRUE;
  }
  
  return FALSE;
}

BOOL
ProcessMessage(void)
{
  MSG msg;
  if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return TRUE;
  }
  return FALSE;
}

void
ProcessMessages(void)
{
  while(ProcessMessage());
}

/* try to open the file */
BOOL
FileExists(WCHAR *Path, WCHAR *File)
{
  WCHAR FileName[MAX_PATH + 1];
  HANDLE FileHandle;
  
  FileName[0] = L'\0';
  wcscat(FileName, Path);
  wcscat(FileName, File);

  FileHandle = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  
  if(FileHandle == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  if(GetFileSize(FileHandle, NULL) <= 0)
  {
    CloseHandle(FileHandle);
    return FALSE;
  }
  
  CloseHandle(FileHandle);
  return TRUE;
}

/* Copy file */
BOOL
InstallFile(WCHAR *Destination, WCHAR *File)
{
  static char Buffer[1024];
  WCHAR SourceFileName[MAX_PATH + 1];
  WCHAR DestFileName[MAX_PATH + 1];
  HANDLE SourceFileHandle, DestFileHandle;
  DWORD DataRead, DataWritten;
  
  SourceFileName[0] = L'\0';
  DestFileName[0] = L'\0';
  wcscat(SourceFileName, SrcPath);
  wcscat(SourceFileName, File);
  wcscat(DestFileName, Destination);
  wcscat(DestFileName, File);
  
  SourceFileHandle = CreateFile(SourceFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(SourceFileHandle == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }
  DestFileHandle = CreateFile(DestFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(DestFileHandle == INVALID_HANDLE_VALUE)
  {
    CloseHandle(SourceFileHandle);
    return FALSE;
  }
  
  while(ReadFile(SourceFileHandle, Buffer, sizeof(Buffer), &DataRead, NULL) && DataRead > 0)
  {
    if(!WriteFile(DestFileHandle, Buffer, DataRead, &DataWritten, NULL) ||
       DataRead != DataWritten)
    {
      CloseHandle(SourceFileHandle);
      CloseHandle(DestFileHandle);
      DeleteFile(DestFileName);
      return FALSE;
    }
  }
  
  CloseHandle(SourceFileHandle);
  CloseHandle(DestFileHandle);
  return TRUE;
}

/* Find the drive with the inserted VMware cd-rom */
BOOL
IsVMwareCDInDrive(WCHAR *Drv)
{
  static WCHAR Drive[4] = L"X:\\";
  WCHAR Current;
  
  *Drv = L'\0';
  for(Current = 'C'; Current <= 'Z'; Current++)
  {
    Drive[0] = Current;
#if CHECKDRIVETYPE
    if(GetDriveType(Drive) == DRIVE_CDROM)
    {
#endif
      PathToVideoDrivers40[0] = Current;
      PathToVideoDrivers45[0] = Current;
      if(SetCurrentDirectory(PathToVideoDrivers45))
        SrcPath = PathToVideoDrivers45;
      else if(SetCurrentDirectory(PathToVideoDrivers40))
        SrcPath = PathToVideoDrivers40;
      else
      {
        SetCurrentDirectory(DestinationPath);
        continue;
      }
      
      if(FileExists(SrcPath, vmx_fb) &&
         FileExists(SrcPath, vmx_mode) &&
         FileExists(SrcPath, vmx_svga))
      {
        *Drv = Current;
        return TRUE;
      }
#if CHECKDRIVETYPE
    }
#endif
  }
  
  return FALSE;
}

BOOL
LoadResolutionSettings(DWORD *ResX, DWORD *ResY, DWORD *ColDepth)
{
  HKEY hReg;
  DWORD Type, Size;
  
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                  L"SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\vmx_svga\\Device0", 
                  0, KEY_QUERY_VALUE, &hReg) != ERROR_SUCCESS)
  {
    return FALSE;
  }
  if(RegQueryValueEx(hReg, L"DefaultSettings.BitsPerPel", 0, &Type, (BYTE*)ColDepth, &Size) != ERROR_SUCCESS ||
     Type != REG_DWORD)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  if(RegQueryValueEx(hReg, L"DefaultSettings.XResolution", 0, &Type, (BYTE*)ResX, &Size) != ERROR_SUCCESS ||
     Type != REG_DWORD)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  if(RegQueryValueEx(hReg, L"DefaultSettings.YResolution", 0, &Type, (BYTE*)ResY, &Size) != ERROR_SUCCESS ||
     Type != REG_DWORD)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  RegCloseKey(hReg);
  return TRUE;
}

BOOL
IsVmwSVGAEnabled(VOID)
{
  HKEY hReg;
  DWORD Type, Size, Value;
  
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                  L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga", 
                  0, KEY_QUERY_VALUE, &hReg) != ERROR_SUCCESS)
  {
    return FALSE;
  }
  if(RegQueryValueEx(hReg, L"Start", 0, &Type, (BYTE*)&Value, &Size) != ERROR_SUCCESS ||
     Type != REG_DWORD)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  RegCloseKey(hReg);
  return (Value == 1);
}

BOOL
SaveResolutionSettings(DWORD ResX, DWORD ResY, DWORD ColDepth)
{
  HKEY hReg;
  
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                  L"SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\vmx_svga\\Device0", 
                  0, KEY_QUERY_VALUE, &hReg) != ERROR_SUCCESS)
  {
    return FALSE;
  }
  if(RegSetValueEx(hReg, L"DefaultSettings.BitsPerPel", 0, REG_DWORD, (BYTE*)&ColDepth, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  if(RegSetValueEx(hReg, L"DefaultSettings.XResolution", 0, REG_DWORD, (BYTE*)&ResX, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  if(RegSetValueEx(hReg, L"DefaultSettings.YResolution", 0, REG_DWORD, (BYTE*)&ResY, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  RegCloseKey(hReg);
  return TRUE;
}

BOOL
EnableDriver(WCHAR *Key, BOOL Enable)
{
  DWORD Value;
  HKEY hReg;
  
  Value = (Enable ? 1 : 4);
  
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_SET_VALUE, &hReg) != ERROR_SUCCESS)
  {
    return FALSE;
  }
  if(RegSetValueEx(hReg, L"Start", 0, REG_DWORD, (BYTE*)&Value, sizeof(DWORD)) != ERROR_SUCCESS)
  {
    RegCloseKey(hReg);
    return FALSE;
  }
  
  RegCloseKey(hReg);
  return TRUE;
}

/* Activate the vmware driver and deactivate the others */
BOOL
EnableVmwareDriver(BOOL VBE, BOOL VGA, BOOL VMX)
{
  if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\VBE", VBE))
  {
    return FALSE;
  }
  if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\vga", VGA))
  {
    return FALSE;
  }
  if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga", VMX))
  {
    return FALSE;
  }
  
  return TRUE;
}

/* GUI */

void
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGE));
  psp->dwSize = sizeof(PROPSHEETPAGE);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hAppInstance;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
}

/* Property page dialog callback */
int CALLBACK
PageWelcomeProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
          break;
        }
        case PSN_WIZNEXT:
        {
          if(DriverFilesFound)
          {
            if(!EnableVmwareDriver(FALSE, FALSE, TRUE))
            {
              WCHAR Msg[1024];
              LoadString(hAppInstance, IDS_FAILEDTOACTIVATEDRIVER, Msg, sizeof(Msg) / sizeof(WCHAR));
              MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
              SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_WELCOMEPAGE);
              return TRUE;
            }
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CONFIG);
            return TRUE;
          }
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

/* Property page dialog callback */
int CALLBACK
PageInsertDiscProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
          break;
        case PSN_WIZNEXT:
          PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
          ProcessMessages();
          if(!IsVMwareCDInDrive(&CDDrive))
          {
            WCHAR Msg[1024];
            LoadString(hAppInstance, IDS_FAILEDTOLOCATEDRIVERS, Msg, sizeof(Msg) / sizeof(WCHAR));
            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INSERT_VMWARE_TOOLS);
            return TRUE;
          }
          
          if(!InstallFile(DestinationPath, vmx_fb) ||
             !InstallFile(DestinationPath, vmx_mode) ||
             !InstallFile(DestinationDriversPath, vmx_svga))
          {
            WCHAR Msg[1024];
            LoadString(hAppInstance, IDS_FAILEDTOCOPYFILES, Msg, sizeof(Msg) / sizeof(WCHAR));
            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INSERT_VMWARE_TOOLS);
            return TRUE;
          }
          
          if(!EnableVmwareDriver(FALSE, FALSE, TRUE))
          {
            WCHAR Msg[1024];
            LoadString(hAppInstance, IDS_FAILEDTOACTIVATEDRIVER, Msg, sizeof(Msg) / sizeof(WCHAR));
            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INSTALLATION_FAILED);
            return TRUE;
          }
          
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
          break;
      }
      break;
    }
  }
  return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK
PageInstallFailedProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
          break;
      }
      break;
    }
  }
  return FALSE;
}

void
FillComboBox(HWND Dlg, int idComboBox, int From, int To)
{
  int i;
  WCHAR Text[256];
  
  for(i = From; i <= To; i++)
  {
    if(LoadString(hAppInstance, i, Text, 255) > 0)
    {
      SendDlgItemMessage(Dlg, idComboBox, CB_ADDSTRING, 0, (LPARAM)Text);
    }
  }
}

typedef struct
{
  int ControlID;
  int ResX;
  int ResY;
} MAPCTLRES;

/* Property page dialog callback */
BOOL CALLBACK
PageConfigProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      DWORD ResX = 0, ResY = 0, ColDepth = 0;
      int cbSel;
      
      FillComboBox(hwndDlg, IDC_COLORQUALITY, 10001, 10003);
      if(LoadResolutionSettings(&ResX, &ResY, &ColDepth))
      {
        SendDlgItemMessage(hwndDlg, ResX + ResY, BM_SETCHECK, BST_CHECKED, 0);
        switch(ColDepth)
        {
          case 8:
            cbSel = 0;
            break;
          case 16:
            cbSel = 1;
            break;
          case 32:
            cbSel = 2;
            break;
          default:
            cbSel = -1;
            break;
        }
        SendDlgItemMessage(hwndDlg, IDC_COLORQUALITY, CB_SETCURSEL, cbSel, 0);
      }
      break;
    }
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
        {
          PropSheet_SetWizButtons(GetParent(hwndDlg), (StartVMwConfigWizard ? PSWIZB_FINISH | PSWIZB_BACK : PSWIZB_FINISH));
          break;
        }
        case PSN_WIZBACK:
        {
          if(StartVMwConfigWizard)
          {
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
            return TRUE;
          }
          if(DriverFilesFound)
          {
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_WELCOMEPAGE);
            return TRUE;
          }
          break;
        }
        case PSN_WIZFINISH:
        {
          DWORD rx = 800, ry = 600, cd = 32;
          int i;
          static MAPCTLRES Resolutions[11] = {
            {540, 640, 480},
            {1400, 800, 600},
            {1792, 1024, 768},
            {2016, 1152, 864},
            {2240, 1280, 960},
            {2304, 1280, 1024},
            {2450, 1400, 1050},
            {2800, 1600, 1200},
            {3136, 1792, 1344},
            {3248, 1856, 1392},
            {3360, 1920, 1440}
          };
          for(i = 0; i < 11; i++)
          {
            if(SendDlgItemMessage(hwndDlg, Resolutions[i].ControlID, BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
              rx = Resolutions[i].ResX;
              ry = Resolutions[i].ResY;
              break;
            }
          }
          
          switch(SendDlgItemMessage(hwndDlg, IDC_COLORQUALITY, CB_GETCURSEL, 0, 0))
          {
            case 0:
              cd = 8;
              break;
            case 1:
              cd = 16;
              break;
            case 2:
              cd = 32;
              break;
          }
          
          SaveResolutionSettings(rx, ry, cd);
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK
PageChooseActionProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      SendDlgItemMessage(hwndDlg, IDC_CONFIGSETTINGS, BM_SETCHECK, BST_CHECKED, 0);
      break;
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
          break;
        case PSN_WIZBACK:
        {
          SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
          return TRUE;
        }
        case PSN_WIZNEXT:
        {
          static ULONG SelPage[4] = {IDD_CONFIG, IDD_SELECTDRIVER, IDD_SELECTDRIVER, IDD_CHOOSEACTION};
          int i;
          
          for(i = IDC_CONFIGSETTINGS; i <= IDC_UNINSTALL; i++)
          {
            if(SendDlgItemMessage(hwndDlg, i, BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
              break;
            }
          }
          
          UninstallDriver = (i == IDC_UNINSTALL);
          
          SetWindowLong(hwndDlg, DWL_MSGRESULT, SelPage[i - IDC_CONFIGSETTINGS]);
          return TRUE;
        }
      }
      break;
    }
  }
  return FALSE;
}

/* Property page dialog callback */
BOOL CALLBACK
PageSelectDriverProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      SendDlgItemMessage(hwndDlg, IDC_VGA, BM_SETCHECK, BST_CHECKED, 0);
      break;
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hwndDlg), (UninstallDriver ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK | PSWIZB_FINISH));
          break;
        case PSN_WIZBACK:
        {
          SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
          return TRUE;
        }
        case PSN_WIZNEXT:
        {
          ActivateVBE = (SendDlgItemMessage(hwndDlg, IDC_VBE, BM_GETCHECK, 0, 0) == BST_CHECKED);

          if(UninstallDriver)
          {
            return FALSE;
          }
          return TRUE;
        }
        case PSN_WIZFINISH:
        {
          if(UninstallDriver)
          {
            return FALSE;
          }
          ActivateVBE = (SendDlgItemMessage(hwndDlg, IDC_VBE, BM_GETCHECK, 0, 0) == BST_CHECKED);
          if(!EnableVmwareDriver(ActivateVBE,
                                 !ActivateVBE,
                                 FALSE))
          {
            WCHAR Msg[1024];
            LoadString(hAppInstance, (ActivateVBE ? IDS_FAILEDTOSELVBEDRIVER : IDS_FAILEDTOSELVGADRIVER), Msg, sizeof(Msg) / sizeof(WCHAR));
            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
            SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTDRIVER);
            return TRUE;
          }
          break;
        }
      }
      break;
    }
  }
  return FALSE;
}

VOID
ShowUninstNotice(HWND Owner)
{
  WCHAR Msg[1024];
  LoadString(hAppInstance, IDS_UNINSTNOTICE, Msg, sizeof(Msg) / sizeof(WCHAR));
  MessageBox(Owner, Msg, NULL, MB_ICONINFORMATION);
}

BOOL CALLBACK
PageDoUninstallProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_NOTIFY:
    {
      LPNMHDR pnmh = (LPNMHDR)lParam;
      switch(pnmh->code)
      {
        case PSN_SETACTIVE:
          PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
          break;
        case PSN_WIZFINISH:
        {
          if(UninstallDriver)
          {
            if(!EnableVmwareDriver(ActivateVBE,
                                   !ActivateVBE,
                                   FALSE))
            {
              WCHAR Msg[1024];
              LoadString(hAppInstance, (ActivateVBE ? IDS_FAILEDTOSELVBEDRIVER : IDS_FAILEDTOSELVGADRIVER), Msg, sizeof(Msg) / sizeof(WCHAR));
              MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
              SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_SELECTDRIVER);
              return TRUE;
            }
            ShowUninstNotice(GetParent(hwndDlg));
          }
          return FALSE;
        }
      }
      break;
    }
  }
  return FALSE;
}

static LONG
CreateWizard(VOID)
{
  PROPSHEETPAGE psp[7];
  PROPSHEETHEADER psh;
  WCHAR Caption[1024];
  
  LoadString(hAppInstance, IDS_WIZARD_NAME, Caption, sizeof(Caption) / sizeof(TCHAR));
  
  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_WIZARD;
  psh.hwndParent = NULL;
  psh.hInstance = hAppInstance;
  psh.hIcon = 0;
  psh.pszCaption = Caption;
  psh.nPages = 7;
  psh.nStartPage = (StartVMwConfigWizard ? 4 : 0);
  psh.ppsp = psp;
  
  InitPropSheetPage(&psp[0], IDD_WELCOMEPAGE, PageWelcomeProc);
  InitPropSheetPage(&psp[1], IDD_INSERT_VMWARE_TOOLS, PageInsertDiscProc);
  InitPropSheetPage(&psp[2], IDD_CONFIG, PageConfigProc);
  InitPropSheetPage(&psp[3], IDD_INSTALLATION_FAILED, PageInstallFailedProc);
  InitPropSheetPage(&psp[4], IDD_CHOOSEACTION, PageChooseActionProc);
  InitPropSheetPage(&psp[5], IDD_SELECTDRIVER, PageSelectDriverProc);
  InitPropSheetPage(&psp[6], IDD_DOUNINSTALL, PageDoUninstallProc);
  
  return (LONG)(PropertySheet(&psh) != -1);
}

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  LPTOP_LEVEL_EXCEPTION_FILTER OldHandler;
  int Version;
  WCHAR *lc;
  
  hAppInstance = hInstance;
  
  /* Setup our exception "handler" ;-) */
  OldHandler = SetUnhandledExceptionFilter(ExceptionHandler);
  
  if(!DetectVMware(&Version))
  {
    ExitProcess(1);
    return 1;
  }
  
  /* restore the exception handler */
  SetUnhandledExceptionFilter(OldHandler);
  
  lc = DestinationPath;
  lc += GetSystemDirectory(DestinationPath, MAX_PATH) - 1;
  if(lc >= DestinationPath && *lc != L'\\')
  {
    wcscat(DestinationPath, L"\\");
  }
  DestinationDriversPath[0] = L'\0';
  wcscat(DestinationDriversPath, DestinationPath);
  wcscat(DestinationDriversPath, L"drivers\\");
  
  SetCurrentDirectory(DestinationPath);
  
  DriverFilesFound = FileExists(DestinationPath, vmx_fb) &&
                     FileExists(DestinationPath, vmx_mode) &&
                     FileExists(DestinationDriversPath, vmx_svga);
  
  StartVMwConfigWizard = DriverFilesFound && IsVmwSVGAEnabled();
  
  /* Show the wizard */
  CreateWizard();
  
  return 2;
}

