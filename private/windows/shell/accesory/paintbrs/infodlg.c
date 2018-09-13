/****************************Module*Header******************************\
* Module Name: infodlg.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation			*
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#define  NOMINMAX
#define  NOKANJI
#define  NOWH
#define  NOCOMM
#define  NOSOUND
#define  NOSCROLL
#define  NOVIRTUALKEYCODES
#define  NOICONS

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

BOOL FAR PASCAL InfoDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   TCHAR buf[10];

   switch (message) {
       case WM_COMMAND:
           EndDialog(hDlg, TRUE);
           break;

       case WM_INITDIALOG:
           CenterWindow(hDlg);

           if (wFileType == BITMAPFILE || wFileType == MSPFILE) {
               SetDlgItemInt(hDlg, IDWIDTH, BitmapHeader.wid, FALSE);
               SetDlgItemInt(hDlg, IDHEIGHT, BitmapHeader.hgt, FALSE);
               wsprintf(buf, TEXT("%ld"), 1L << (BitmapHeader.bitcount 
                                           * BitmapHeader.planes));
               SetDlgItemText(hDlg, IDCOLORS, buf);
               SetDlgItemInt(hDlg, IDPLANES, BitmapHeader.planes, FALSE);
           } else {
               SetDlgItemInt(hDlg, IDWIDTH, imageHdr.x2 - imageHdr.x1 + 1,
                             FALSE);
               SetDlgItemInt(hDlg, IDHEIGHT, imageHdr.y2 - imageHdr.y1 + 1,
                             FALSE);
               wsprintf(buf, TEXT("%ld"), 1L << (imageHdr.bitpx * imageHdr.nPlanes));
               SetDlgItemText(hDlg, IDCOLORS, buf);
               SetDlgItemInt(hDlg, IDPLANES, imageHdr.nPlanes, FALSE);
           }
           break;

       default:
           return FALSE;
   }

   return TRUE;
}
