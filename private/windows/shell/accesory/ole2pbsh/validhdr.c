/****************************Module*Header******************************\
* Module Name: validhdr.c                                               *
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

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"


BOOL ValidHdr(HWND hWnd, DHDR *hdr, LPTSTR name)
{
   if (hdr->manuf != 10 || hdr->encod != 1) {
       SimpleMessage(IDSBadHeader, name, MB_OK | MB_ICONEXCLAMATION);
       return FALSE;
   } else
       return TRUE;
}
