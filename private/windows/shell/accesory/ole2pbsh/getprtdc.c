/****************************Module*Header******************************\
* Module Name: getprtdc.c                                               *
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

#include <windows.h>
//#include <drivinit.h>
#include <port1632.h>
#include "pbrush.h"

extern HPALETTE hPalette;
extern PRINTDLG PD;

BOOL bPrtCreateErr = FALSE;

HDC GetPrtDC(void)
{
   HDC          hdc;
   LPDEVMODE    lpDevMode;
   LPDEVNAMES   lpDevNames;
   short        dmCopies;

    if(!PD.hDevNames) /* Retrieve default printer if none selected. */
        GetDefaultPort();

    if(!PD.hDevNames){
        bPrtCreateErr = TRUE;
        return NULL;
    }

    lpDevNames = (LPDEVNAMES)GlobalLock(PD.hDevNames);

    if (PD.hDevMode) {
        lpDevMode = (LPDEVMODE)GlobalLock(PD.hDevMode);
        /* Save control panel setting of dmCopies and overwrite with 1 as
         * pbrush prompts for number of copies desired to be printed anyway.
         */
        dmCopies  = lpDevMode->dmCopies;
        lpDevMode->dmCopies = 1;
    } else
        lpDevMode = NULL;

    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *  will be NULL after GlobalLock()
     */


    hdc = CreateDC(((LPTSTR)lpDevNames)+lpDevNames->wDriverOffset,
                   ((LPTSTR)lpDevNames)+lpDevNames->wDeviceOffset,
                   ((LPTSTR)lpDevNames)+lpDevNames->wOutputOffset,
                   lpDevMode);

    GlobalUnlock(PD.hDevNames);

    if(PD.hDevMode) {
        lpDevMode->dmCopies = dmCopies; /* Replace original dmCopies for PrintDlg compatibilities. */
        GlobalUnlock(PD.hDevMode);
    }

    bPrtCreateErr = !hdc;
    return hdc;
}

BOOL FAR PASCAL  GetDefaultPort(void)
{
   FreePrintHandles();

   PD.lStructSize       = sizeof(PRINTDLG);
   PD.hDevMode          = NULL;
   PD.hDevNames         = NULL;
   PD.Flags             = PD_PRINTSETUP|PD_RETURNDEFAULT;
   if(PrintDlg(&PD))
          return TRUE;
   else
          return FALSE;
}

HDC GetDisplayDC(HWND hWnd)
{
   HDC hDC;

   hDC = GetDC(hWnd);
   if (hPalette) {
      SelectPalette(hDC, hPalette, 0);
      RealizePalette(hDC);
   }

   return hDC;
}
