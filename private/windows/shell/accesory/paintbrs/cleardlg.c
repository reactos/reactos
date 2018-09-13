/****************************Module*Header******************************\
* Module Name: cleardlg.c                                               *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
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
#define  NOOPENFILE
#define  NOSCROLL
#define  NOVIRTUALKEYCODES
#define  NOICONS
#include <windows.h>
#include "port1632.h"
#include "pbrush.h"
#include "fixedpt.h"

static BOOL bWasFirstClear = TRUE;
static BOOL FirstClear = TRUE; /* is this the first time to clear screen ? */

extern BOOL imageFlag;

extern HWND pbrushWnd[];
extern int nNewImageWidth, nNewImageHeight, nNewImagePlanes, nNewImagePixels;
extern TCHAR winIniAppName[], winIniWidthName[], winIniHeightName[],
            winIniClrName[];
extern DWORD *rgbColor;
extern DWORD colorColor[];
static TCHAR   szBW[] = TEXT("B/W");
static TCHAR   szCOLOR[] = TEXT("COLOR");
int uAttrUnit = IDIN;

BOOL FAR PASCAL ClearDlg(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
   static WORD button;
   static LONG lWidth, lHeight;
   static BOOL bChangeOk = TRUE;

   HDC dc;
   int wid, hgt, numcolors;

   BOOL bDecimal;
   TCHAR buff[20];
   HCURSOR oldcsr;

   switch (message) {
       case WM_COMMAND:
           switch (GET_WM_COMMAND_ID(wParam,lParam)) {
               case IDDEFAULT:
                   dc = GetDisplayDC(pbrushWnd[PARENTid]);
                   lWidth = ShortToNum(GetDeviceCaps(dc, HORZRES));
                   lHeight = ShortToNum(GetDeviceCaps(dc, VERTRES));
                   ReleaseDC(pbrushWnd[PARENTid], dc);

                   if (button != IDPELS) {
                       lWidth = PelsToNum(lWidth, TRUE, button == IDIN);
                       lHeight = PelsToNum(lHeight, FALSE, button == IDIN);
                   }

                   bChangeOk = FALSE;
                   SetDlgItemNum(hDlg, IDWIDTH, lWidth, button != IDPELS);
                   SetDlgItemNum(hDlg, IDHEIGHT, lHeight, button != IDPELS);
                   bChangeOk = TRUE;
                   break;

               case IDOK:
                   if (imageFlag &&
                       SimpleMessage(IDS_RESETIMAGE, NULL, MB_YESNO) == IDNO)
                   {
                       EndDialog(hDlg, FALSE);
                       break;
                   }

                   if(!GetDlgItemNum(hDlg, IDWIDTH, &lWidth))
                       lWidth = 0;
                   if(!GetDlgItemNum(hDlg, IDHEIGHT, &lHeight))
                       lHeight = 0;

                   if (button != IDPELS) {
                       lWidth = NumToPels(lWidth, TRUE, button == IDIN);
                       lHeight = NumToPels(lHeight, FALSE, button == IDIN);
                   }

                   /* make sure dimensions are valid */

                   if (lWidth > ShortToNum(32767) || lWidth <= 0) {
                       PbrushOkError(IDSInvalidWidth, MB_ICONHAND);
                       SetFocus(GetDlgItem(hDlg, IDWIDTH));
                       break;
                   }

                   if (lHeight > ShortToNum(32767) || lHeight <= 0) {
                       PbrushOkError(IDSInvalidHeight, MB_ICONHAND);
                       SetFocus(GetDlgItem(hDlg, IDHEIGHT));
                       break;
                   }

                   oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

                   nNewImageHeight = FLOOR(lHeight);
                   nNewImageWidth = FLOOR(lWidth);
                   wsprintf(buff, TEXT("%d"), nNewImageWidth);
                   WriteProfileString(winIniAppName, winIniWidthName, buff);
                   wsprintf(buff, TEXT("%d"), nNewImageHeight);
                   WriteProfileString(winIniAppName, winIniHeightName, buff);

                   if (IsDlgButtonChecked(hDlg, ID2)) {
                       nNewImagePlanes = nNewImagePixels = 1;
                       lstrcpy(buff, szBW);
                   } else {
                       nNewImagePlanes = nNewImagePixels = 0;
                       lstrcpy(buff, szCOLOR);
                   }
                   WriteProfileString(winIniAppName, winIniClrName, buff);

                   SetCursor(oldcsr);

                   uAttrUnit = button;
                   PostMessage(pbrushWnd[PARENTid], WM_COMMAND,
                                GET_WM_COMMAND_MPS(FILEnew,NULL,0));
                   EndDialog(hDlg, TRUE);
                   break;

               case IDCANCEL:
                   EndDialog(hDlg, FALSE);
                   break;

               case IDCM:
               case IDIN:
               case IDPELS:
                   if (!GetDlgItemNum(hDlg, IDWIDTH, &lWidth))
                       lWidth = 0;
                   if (!GetDlgItemNum(hDlg, IDHEIGHT, &lHeight))
                       lHeight = 0;

                   if (GET_WM_COMMAND_ID(wParam,lParam) == button)
                       break;

                   switch (button) {
                       case IDCM:
                           if (GET_WM_COMMAND_ID(wParam,lParam) == IDIN) {
                               bDecimal = TRUE;
                               lWidth = CMToInches(lWidth);
                               lHeight = CMToInches(lHeight);
                           } else {
                               bDecimal = FALSE;
                               lWidth = NumToPels(lWidth, TRUE, FALSE);
                               lHeight = NumToPels(lHeight, FALSE, FALSE);
                           }
                           break;

                       case IDIN:
                           if (GET_WM_COMMAND_ID(wParam,lParam) == IDCM) {
                               bDecimal = TRUE;
                               lWidth = InchesToCM(lWidth);
                               lHeight = InchesToCM(lHeight);
                           } else {
                               bDecimal = FALSE;
                               lWidth = NumToPels(lWidth, TRUE, TRUE);
                               lHeight = NumToPels(lHeight, FALSE, TRUE);
                           }
                           break;

                       case IDPELS:
                           if (GET_WM_COMMAND_ID(wParam,lParam) == IDCM) {
                               lWidth = PelsToNum(lWidth, TRUE, FALSE);
                               lHeight = PelsToNum(lHeight, FALSE, FALSE);
                           } else {
                               lWidth = PelsToNum(lWidth, TRUE, TRUE);
                               lHeight = PelsToNum(lHeight, FALSE, TRUE);
                           }
                           bDecimal = TRUE;
                           break;
                   }

                   bChangeOk = FALSE;
                   SetDlgItemNum(hDlg, IDWIDTH, lWidth, bDecimal);
                   SetDlgItemNum(hDlg, IDHEIGHT, lHeight, bDecimal);
                   bChangeOk = TRUE;

                   CheckRadioButton(hDlg, IDIN, IDPELS,
                           button = GET_WM_COMMAND_ID(wParam,lParam));
                   break;

               case ID2:
               case ID256:
                   CheckRadioButton(hDlg, ID2, ID256,
                              GET_WM_COMMAND_ID(wParam,lParam));
                   break;

               default:
                   break;
           }
           break;

       case WM_INITDIALOG:
           CenterWindow(hDlg);

           dc = GetDisplayDC(pbrushWnd[PARENTid]);
           numcolors = GetDeviceCaps(dc, NUMCOLORS);
           ReleaseDC(pbrushWnd[PARENTid], dc);

           button = uAttrUnit;

           if (FirstClear) {
               dc = GetDisplayDC(pbrushWnd[PARENTid]);
               wid = GetDeviceCaps(dc, HORZRES);
               hgt = GetDeviceCaps(dc, VERTRES);

               ReleaseDC(pbrushWnd[PARENTid], dc);

               CheckRadioButton (hDlg, ID2, ID256, ID2);
               if (numcolors == 2)
                   EnableWindow(GetDlgItem(hDlg, ID256), FALSE);
               else {
                   GetProfileString(winIniAppName, winIniClrName,
                                    szCOLOR, buff, CharSizeOf(buff));

                   if (lstrcmp(buff, szBW))
                       CheckRadioButton(hDlg, ID2, ID256, ID256);
               }

               FirstClear = FALSE;
           } else {
               CheckRadioButton(hDlg, ID2, ID256, ID2);
               if (numcolors == 2)
                   EnableWindow(GetDlgItem(hDlg, ID256), FALSE);
               else if (1 != nNewImagePixels || 1 != nNewImagePlanes)
                   CheckRadioButton(hDlg, ID2, ID256, ID256);
           }

           if (uAttrUnit != IDPELS) {
               lWidth = PelsToNum(ShortToNum(nNewImageWidth), TRUE,
                                  uAttrUnit == IDIN);
               lHeight = PelsToNum(ShortToNum(nNewImageHeight), FALSE,
                                   uAttrUnit == IDIN);
           } else {
               lWidth = ShortToNum(nNewImageWidth);
               lHeight = ShortToNum(nNewImageHeight);
           }

           bChangeOk = FALSE;
           SetDlgItemNum(hDlg, IDWIDTH, lWidth, uAttrUnit != IDPELS);
           SetDlgItemNum(hDlg, IDHEIGHT, lHeight, uAttrUnit != IDPELS);
           bChangeOk = TRUE;

           CheckRadioButton(hDlg, IDIN, IDPELS, uAttrUnit);
           break;

       default:
           return FALSE;
   }

   return TRUE;
}
