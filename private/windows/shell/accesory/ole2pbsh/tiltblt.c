/****************************Module*Header******************************\
* Module Name: tiltblt.c                                                *
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
//#define NOEXTERN
#include "pbrush.h"


void FAR PASCAL TiltBlt(short x, short y, LPHANDLE dc)
{
   MaskBlt(*dc,x,y,pickWid,1,pickDC,0,y,monoBM,0,y,MASKROP(SRCCOPY,0x00aa0000));
}
