/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/general.c
 * PURPOSE:     General System Information
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2006-2007 Colin Finck <mail@colinfinck.de>
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

    if ((LastError == 0) ||
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
        if (!lpBuf)
            return;

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

static INT
SetProcNameString(HWND hwnd,
                  HKEY hKey,
                  LPTSTR Value,
                  UINT uID1,
                  UINT uID2)
{
    LPTSTR lpBuf = NULL;
    DWORD BufSize = 0;
    DWORD Type;
    INT Ret = 0;
    TCHAR szBuf[31];
    TCHAR* szLastSpace;
    INT LastSpace = 0;

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
        if (!lpBuf)
            return 0;

        if (RegQueryValueEx(hKey,
                            Value,
                            NULL,
                            &Type,
                            (PBYTE)lpBuf,
                            &BufSize) == ERROR_SUCCESS)
        {
            if (BufSize > ((30 + 1) * sizeof(TCHAR)))
            {
                /* Wrap the Processor Name String like XP does:                           *
                *   - Take the first 30 characters and look for the last space.          *
                *     Then wrap the string after this space.                             *
                *   - If no space is found, wrap the string after character 30.          *
                *                                                                        *
                * For example the Processor Name String of a Pentium 4 is right-aligned. *
                * With this wrapping the first line looks centered.                      */

                _tcsncpy(szBuf, lpBuf, 30);
                szBuf[30] = 0;
                szLastSpace = _tcsrchr(szBuf, ' ');

                if (szLastSpace == 0)
                {
                    LastSpace = 30;
                }
                else
                {
                    LastSpace = (szLastSpace - szBuf);
                    szBuf[LastSpace] = 0;
                }

                _tcsncpy(szBuf, lpBuf, LastSpace);

                SetDlgItemText(hwnd,
                               uID1,
                               szBuf);

                SetDlgItemText(hwnd,
                               uID2,
                               lpBuf+LastSpace+1);

                /* Return the number of used lines */
                Ret = 2;
            }
            else
            {
                SetDlgItemText(hwnd,
                             uID1,
                             lpBuf);

                Ret = 1;
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpBuf);
    }

    return Ret;
}

static VOID
MakeFloatValueString(double* dFloatValue,
                     LPTSTR szOutput,
                     LPTSTR szAppend)
{
    TCHAR szDecimalSeparator[4];

    /* Get the decimal separator for the current locale */
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSeparator, sizeof(szDecimalSeparator) / sizeof(TCHAR)) > 0)
    {
        UCHAR uDecimals;
        UINT uIntegral;

        /* Show the value with two decimals */
        uIntegral = (UINT)*dFloatValue;
        uDecimals = (UCHAR)((UINT)(*dFloatValue * 100) - uIntegral * 100);

        wsprintf(szOutput, _T("%u%s%02u %s"), uIntegral, szDecimalSeparator, uDecimals, szAppend);
    }
}

static VOID
SetProcSpeed(HWND hwnd,
             HKEY hKey,
             LPTSTR Value,
             UINT uID)
{
    TCHAR szBuf[64];
    DWORD BufSize = sizeof(DWORD);
    DWORD Type = REG_SZ;
    PROCESSOR_POWER_INFORMATION ppi;

    ZeroMemory(&ppi,
               sizeof(ppi));

    if ((CallNtPowerInformation(ProcessorInformation,
                                NULL,
                                0,
                                (PVOID)&ppi,
                                sizeof(ppi)) == STATUS_SUCCESS &&
         ppi.CurrentMhz != 0) ||
         RegQueryValueEx(hKey,
                         Value,
                         NULL,
                         &Type,
                         (PBYTE)&ppi.CurrentMhz,
                         &BufSize) == ERROR_SUCCESS)
    {
        if (ppi.CurrentMhz < 1000)
        {
            wsprintf(szBuf, _T("%lu MHz"), ppi.CurrentMhz);
        }
        else
        {
            double flt = ppi.CurrentMhz / 1000.0;
            MakeFloatValueString(&flt, szBuf, _T("GHz"));
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
    MEMORYSTATUSEX MemStat;
    TCHAR Buf[32];
    INT CurMachineLine = IDC_MACHINELINE1;

    /*
     * Get Processor information
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
                       CurMachineLine);
        CurMachineLine++;

        CurMachineLine += SetProcNameString(hwnd,
                                            hKey,
                                            _T("ProcessorNameString"),
                                            CurMachineLine,
                                            CurMachineLine + 1);

        SetProcSpeed(hwnd,
                     hKey,
                     _T("~MHz"),
                     CurMachineLine);
        CurMachineLine++;
    }


    /* Get total physical RAM */
    MemStat.dwLength = sizeof(MemStat);
    if (GlobalMemoryStatusEx(&MemStat))
    {
        TCHAR szStr[32];
        double dTotalPhys;

        if (MemStat.ullTotalPhys > 1024 * 1024 * 1024)
        {
            UINT i = 0;
            static const UINT uStrId[] = {
                IDS_GIGABYTE,
                IDS_TERABYTE,
                IDS_PETABYTE
            };

            // We're dealing with GBs or more
            MemStat.ullTotalPhys /= 1024 * 1024;

            if (MemStat.ullTotalPhys > 1024 * 1024)
            {
                // We're dealing with TBs or more
                MemStat.ullTotalPhys /= 1024;
                i++;

                if (MemStat.ullTotalPhys > 1024 * 1024)
                {
                    // We're dealing with PBs or more
                    MemStat.ullTotalPhys /= 1024;
                    i++;

                    dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                }
                else
                {
                    dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
                }
            }
            else
            {
                dTotalPhys = (double)MemStat.ullTotalPhys / 1024;
            }

            LoadString(hApplet, uStrId[i], szStr, sizeof(szStr) / sizeof(TCHAR));
            MakeFloatValueString(&dTotalPhys, Buf, szStr);
        }
        else
        {
            // We're dealing with MBs, don't show any decimals
            LoadString(hApplet, IDS_MEGABYTE, szStr, sizeof(szStr) / sizeof(TCHAR));
            wsprintf(Buf, _T("%u %s"), (UINT)MemStat.ullTotalPhys / 1024 / 1024, szStr);
        }

        SetDlgItemText(hwnd, CurMachineLine, Buf);
    }
}


/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PIMGINFO pImgInfo;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    pImgInfo = (PIMGINFO)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pImgInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMGINFO));
            if (pImgInfo == NULL)
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pImgInfo);

            InitImageInfo(pImgInfo);
            GetSystemInformation(hwndDlg);
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pImgInfo);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_LICENCE)
            {
                DialogBox(hApplet,
                          MAKEINTRESOURCE(IDD_LICENCE),
                          hwndDlg,
                          LicenceDlgProc);

                return TRUE;
            }
            break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if (lpDrawItem->CtlID == IDC_ROSIMG)
            {
                HDC hdcMem;
                LONG left;

                /* position image in centre of dialog */
                left = (lpDrawItem->rcItem.right - pImgInfo->cxSource) / 2;

                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, pImgInfo->hBitmap);
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

        case WM_NOTIFY:
        {
            NMHDR *nmhdr = (NMHDR *)lParam;

            if (nmhdr->idFrom == IDC_ROSHOMEPAGE_LINK && nmhdr->code == NM_CLICK)
            {
                PNMLINK nml = (PNMLINK)nmhdr;

                ShellExecuteW(hwndDlg,
                              L"open",
                              nml->item.szUrl,
                              NULL,
                              NULL,
                              SW_SHOWNORMAL);
            }
            break;
        }

    }

    return FALSE;
}
