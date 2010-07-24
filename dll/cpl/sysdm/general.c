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

#define ANIM_STEP 2
#define ANIM_TIME 50

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;

PIMGINFO pImgInfo = NULL;

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

LRESULT CALLBACK RosImageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static UINT timerid = 0, top = 0, offset;
	static HBITMAP hBitmap2;
	RECT r;
	NONCLIENTMETRICS ncm;
	HFONT hfont;
	BITMAP bitmap;
	HDC dc, sdc;
	TCHAR devtext[2048];
	switch (uMsg)
	{
		case WM_LBUTTONDBLCLK:
			if (wParam & (MK_CONTROL | MK_SHIFT))
			{
				if (timerid == 0)
				{
					top = 0; // set top
					
					// build new bitmap
					GetObject(pImgInfo->hBitmap, sizeof(BITMAP), &bitmap);
					dc = CreateCompatibleDC(GetDC(NULL));
					if (dc == NULL)
					{
						break;
					}
					sdc = CreateCompatibleDC(dc);
					if (sdc == NULL)
					{
						DeleteDC(dc);
						break;
					}
					ncm.cbSize = sizeof(NONCLIENTMETRICS);
					SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

					hfont = CreateFontIndirect(&ncm.lfMessageFont);
					SelectObject(dc, hfont);
					SetRect(&r, 0, 0, 0, 0);
            				LoadString(hApplet, IDS_DEVS, devtext, sizeof(devtext) / sizeof(TCHAR));
					DrawText(dc, devtext, -1, &r, DT_CALCRECT);
					hBitmap2 = CreateBitmap(pImgInfo->cxSource, (2 * pImgInfo->cySource) + (r.bottom + 1 - r.top), bitmap.bmPlanes, bitmap.bmBitsPixel, NULL);
					SelectObject(sdc, pImgInfo->hBitmap);
					SelectObject(dc, hBitmap2);
					offset = 0;
					BitBlt(dc, 0, offset, bitmap.bmWidth, bitmap.bmHeight, sdc, 0, 0, SRCCOPY);
					offset += bitmap.bmHeight;

					SetRect(&r, 0, offset, bitmap.bmWidth, offset + (r.bottom - r.top) + 1);
					FillRect(dc, &r, GetSysColorBrush(COLOR_3DFACE));
					SetBkMode(dc, TRANSPARENT);
					OffsetRect(&r, 1, 1);
					SetTextColor(dc, GetSysColor(COLOR_BTNSHADOW));
					DrawText(dc, devtext, -1, &r, DT_CENTER);
					OffsetRect(&r, -1, -1);
					SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
					DrawText(dc, devtext, -1, &r, DT_CENTER);
					offset += r.bottom - r.top;

					BitBlt(dc, 0, offset, bitmap.bmWidth, bitmap.bmHeight, sdc, 0, 0, SRCCOPY);
					offset += bitmap.bmHeight;
					DeleteDC(sdc);
					DeleteDC(dc);

					timerid = SetTimer(hwnd, 1, ANIM_TIME, NULL);
				}
			}
			break;
		case WM_LBUTTONDOWN:
			if (timerid)
			{
				KillTimer(hwnd, timerid);
				top = 0;
				timerid = 0;
				DeleteObject(hBitmap2);
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;
		case WM_TIMER:
			top += ANIM_STEP;
			if (top > offset - pImgInfo->cySource)
			{
				KillTimer(hwnd, timerid);
				top = 0;
				timerid = 0;
				DeleteObject(hBitmap2);
			}
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT PS;
			HDC hdcMem, hdc;
			LONG left;
			if (wParam != 0)
			{
				hdc = (HDC)wParam;
			} else
			{	
			   hdc = BeginPaint(hwnd,&PS);
			}
			GetClientRect(hwnd,&PS.rcPaint);

	                /* position image in center of dialog */
			left = (PS.rcPaint.right - pImgInfo->cxSource) / 2;
			hdcMem = CreateCompatibleDC(hdc);
	                
			if (hdcMem != NULL)
	                {
				SelectObject(hdcMem, timerid ? hBitmap2 : pImgInfo->hBitmap);
                		BitBlt(hdc,
	                           left,
        	                   PS.rcPaint.top,
                	           PS.rcPaint.right - PS.rcPaint.left,
				   PS.rcPaint.top + pImgInfo->cySource,
        	                   hdcMem,
                	           0,
                        	   top,
	                           SRCCOPY);
				DeleteDC(hdcMem);
			}
			if (wParam == 0)
				EndPaint(hwnd,&PS);
	          break;
		}
	}
	return TRUE;
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

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pImgInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMGINFO));
            if (pImgInfo == NULL)
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }

            InitImageInfo(pImgInfo);
            SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_ROSIMG), GWL_WNDPROC, (LONG)RosImageProc);
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
