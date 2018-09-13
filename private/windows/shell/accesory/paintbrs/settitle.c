/****************************Module*Header******************************\
* Module Name: settitle.c                                               *
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

#include "onlypbr.h"
#undef NOKERNEL
#undef NOLSTRING

#include <windows.h>
#include "port1632.h"

#include <string.h>
#include "pbrush.h"

extern HWND pbrushWnd[];
extern TCHAR pgmTitle[];

void SetTitle(TCHAR *pstr)
{
   TCHAR title[2*WNDTITLElen];

   if (lstrlen(pstr) > WNDTITLElen)
        pstr[WNDTITLElen] = 0;
   lstrcpy(title, pgmTitle);
   lstrcat(title, TEXT(" - "));
   lstrcat(title, pstr);
   SetWindowText(pbrushWnd[PARENTid], title);
}
