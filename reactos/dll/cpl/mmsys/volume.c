/* $Id: main.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            lib/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 */

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>
#include "mmsys.h"
#include "resource.h"

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;


static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hApplet,
                                 MAKEINTRESOURCE(IDB_SPEAKIMG),
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

void
InitVolumeControls(HWND hwndDlg)
{
    UINT NumWavOut;
    WAVEOUTCAPS woc;
    MMRESULT errcode;
    DWORD dwDeviceID;
    DWORD dwStatus;

    NumWavOut = waveOutGetNumDevs();
    if (!NumWavOut)
    {
        //FIXME
        // deactivate all controls
        return;
    }

    errcode = waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&dwDeviceID, (DWORD_PTR)&dwStatus);
    if (errcode != MMSYSERR_NOERROR)
    {
        MessageBox(hwndDlg, _T("Failed to enumerate default device"), NULL, MB_OK);
        return;
    }

    if (waveOutGetDevCaps(dwDeviceID, &woc, sizeof(WAVEOUTCAPS)) != MMSYSERR_NOERROR)
    {
        MessageBox(hwndDlg, _T("waveOutGetDevCaps failed"), NULL, MB_OK);
        return;
    }

    SendDlgItemMessage(hwndDlg, IDC_DEVICE_NAME, WM_SETTEXT, (WPARAM)0, (LPARAM)woc.szPname);

    if (!(woc.dwSupport & WAVECAPS_VOLUME))
    {
        /// the device does not support volume changes
        /// disable volume control
        EnableWindow(GetDlgItem(hwndDlg, IDC_VOLUME_TRACKBAR), FALSE);
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 10));
        SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPAGESIZE, (WPARAM)FALSE, (LPARAM)1);
        SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(0, 10));
        SendDlgItemMessage(hwndDlg, IDC_VOLUME_TRACKBAR, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)4);
    }
}

/* Volume property page dialog callback */
//static INT_PTR CALLBACK
INT_PTR CALLBACK
VolumeDlgProc(HWND hwndDlg,
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
            InitImageInfo(&ImgInfo);
            InitVolumeControls(hwndDlg);
            break;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_SPEAKIMG)
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
