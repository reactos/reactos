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


DWORD WINAPI
GetSystemInformation(HWND hwnd)
{
    UNREFERENCED_PARAMETER(hwnd);

  return 0;
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
                          LicenceDialogProc);

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

