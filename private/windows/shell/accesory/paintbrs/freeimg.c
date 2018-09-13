/****************************Module*Header******************************\
* Module Name: freeimg.c                                                *
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
#include "pbrush.h"

void FreeTemp()
{
   if (fileDC) {
       DeleteDC(fileDC);
       if (fileBitmap)
           DeleteObject(fileBitmap);
       fileBitmap = NULL;
       fileDC = NULL;
   }

   if (hfileBuff) {
       LocalUnlock(hfileBuff);
       LocalFree(hfileBuff);
       hfileBuff = NULL;
       fileBuff = NULL;
   }
}
