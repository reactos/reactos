/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/general.c
 * PURPOSE:     General System Information
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */


#include "precomp.h"

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;


void
ShowLastWin32Error(HWND hWndOwner)
{
  LPTSTR lpMsg;
  DWORD LastError;

  LastError = GetLastError();

  if((LastError == 0) ||
      !FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL,
                     LastError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR)&lpMsg,
                     0,
                     NULL))
  {
    return;
  }

  MessageBox(hWndOwner, lpMsg, NULL, MB_OK | MB_ICONERROR);

  LocalFree((LPVOID)lpMsg);
}


static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hApplet,
                                 MAKEINTRESOURCE(IDB_ROSBMP),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}


static VOID
SetRegTextData(HWND hwnd,
               HKEY hKey,
               LPTSTR Value,
               UINT uID)
{
    LPTSTR lpBuf = NULL;
    DWORD BufSize = 0;
    DWORD Type;

    if (RegQueryValueEx(hKey,
                        Value,
                        NULL,
                        &Type,
                        NULL,
                        &BufSize) == ERROR_SUCCESS)
    {
        lpBuf = HeapAlloc(GetProcessHeap(),
                          0,
                          BufSize);
        if (!lpBuf) return;

        if (RegQueryValueEx(hKey,
                            Value,
                            NULL,
                            &Type,
                            (PBYTE)lpBuf,
                            &BufSize) == ERROR_SUCCESS)
        {
            SetDlgItemText(hwnd,
                           uID,
                           lpBuf);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpBuf);
    }
}

static  VOID
SetProcSpeed(HWND hwnd,
             HKEY hKey,
             LPTSTR Value,
             UINT uID)

{
    TCHAR szBuf[64];
    DWORD dwBuf;
    DWORD BufSize = sizeof(DWORD);
    DWORD Type = REG_SZ;

    if (RegQueryValueEx(hKey,
                        Value,
                        NULL,
                        &Type,
                        (PBYTE)&dwBuf,
                        &BufSize) == ERROR_SUCCESS)
    {
        if (dwBuf < 1000)
        {
            wsprintf(szBuf, _T("%lu MHz"), dwBuf);
        }
        else
        {
            double flt = dwBuf / 1000.0;
            wsprintf(szBuf, _T("%l GHz"), flt);
        }

        SetDlgItemText(hwnd,
                       uID,
                       szBuf);
    }
}


static VOID
GetSystemInformation(HWND hwnd)
{
    HKEY hKey;
    TCHAR ProcKey[] = _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    MEMORYSTATUS MemStat;
    TCHAR Buf[32];
    INT Ret = 0;
    


    /* Get Processor information *
     * although undocumented, this information is being pulled
     * directly out of the registry instead of via setupapi as it
     * contains all the info we need, and should remain static
     */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     ProcKey,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        SetRegTextData(hwnd, 
                       hKey, 
                       _T("VendorIdentifier"), 
                       IDC_PROCESSORMANUFACTURER);

        SetRegTextData(hwnd, 
                       hKey, 
                       _T("ProcessorNameString"), 
                       IDC_PROCESSOR);

        SetProcSpeed(hwnd, 
                     hKey, 
                     _T("~MHz"), 
                     IDC_PROCESSORSPEED);
    }


    /* Get total physical RAM */
    MemStat.dwLength = sizeof(MemStat);
    GlobalMemoryStatus(&MemStat);

    if (MemStat.dwTotalPhys < KB_DIV)
        Ret = wsprintf(Buf, _T("%luKB of RAM"), MemStat.dwTotalPhys/KB_DIV);
    else if (MemStat.dwTotalPhys >= KB_DIV && MemStat.dwTotalPhys < GB_DIV)
        Ret = wsprintf(Buf, _T("%luMB of RAM"), MemStat.dwTotalPhys/MB_DIV);
    else if (MemStat.dwTotalPhys > GB_DIV)
        Ret = wsprintf(Buf, _T("%luGB of RAM"), MemStat.dwTotalPhys/GB_DIV);

    if (Ret)
    {
        SetDlgItemText(hwnd,
                       IDC_SYSTEMMEMORY,
                       Buf);
    }
}


/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    static IMGINFO ImgInfo;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hLink = GetDlgItem(hwndDlg, IDC_ROSHOMEPAGE_LINK);

            TextToLink(hLink,
                       _T("http://www.reactos.org"),
                       NULL);

            InitImageInfo(&ImgInfo);
            GetSystemInformation(hwndDlg);
        }
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_LICENCE)
            {
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_LICENCE),
                          hwndDlg,
                          LicenceDlgProc);

                return TRUE;
            }
        }
        break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_ROSIMG)
            {
                HDC hdcMem;
                LONG left;

                /* position image in centre of dialog */
                left = (lpDrawItem->rcItem.right - ImgInfo.cxSource) / 2;

                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, ImgInfo.hBitmap);
                    BitBlt(lpDrawItem->hDC,
                           left,
                           lpDrawItem->rcItem.top,
                           lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                           lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
            return TRUE;
        }

    }

    return FALSE;
}

