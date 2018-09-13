/****************************Module*Header******************************\
* Module Name: freepick.c                                               *
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

void FreePick()
{
   if (pickDC)
       DeleteDC(pickDC);
   if(pickBM)
       DeleteObject(pickBM);
   if (saveDC)
       DeleteDC(saveDC);
   if (saveBM)
       DeleteObject(saveBM);
   if (monoDC)
       DeleteDC(monoDC);
   if (monoBM)
       DeleteObject(monoBM);
   pickDC = (HDC)pickBM = (HDC)NULL;
   saveDC = (HDC)saveBM = (HDC)NULL;
   monoDC = (HDC)monoBM = (HDC)NULL;
}
