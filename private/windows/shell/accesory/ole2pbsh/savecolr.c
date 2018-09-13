/****************************Module*Header******************************\
* Module Name: savecolr.c                                               *
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
#include "port1632.h"

#include "pbrush.h"
#include "colr.h"

BOOL SaveColr(HWND hWnd, LPTSTR fileName)
{
   int         i;
   HANDLE      fh;
   COLORhdr    hdr;

   if ((fh = MyOpenFile(fileName, NULL, OF_CREATE | OF_WRITE))
           == INVALID_HANDLE_VALUE)
   {
       if (GetLastError () == 0x04)
           SimpleMessage(IDSCantCreate, fileName, MB_OK | MB_ICONEXCLAMATION);
       else
           SimpleMessage(IDSCantOpen, fileName, MB_OK | MB_ICONEXCLAMATION);
       return FALSE;
   }

   /* fill in color header */
   hdr.tag    = 'C';
   hdr.colors = MAXcolors;
   for (i = 0; i < sizeof(hdr.reserved); ++i)
       hdr.reserved[i] = 0;

   if (!MyByteWriteFile(fh, &hdr, sizeof(hdr))) {
       SimpleMessage(IDSHdrSave, fileName, MB_OK | MB_ICONEXCLAMATION);
       MyCloseFile(fh);
       return FALSE;
   }

   if (!MyByteWriteFile(fh, rgbColor, MAXcolors * sizeof(DWORD))) {
       SimpleMessage(IDSUnableSave, fileName, MB_OK | MB_ICONEXCLAMATION);
       MyCloseFile(fh);
       return FALSE;
   }

   MyCloseFile(fh);
   return TRUE;
}
