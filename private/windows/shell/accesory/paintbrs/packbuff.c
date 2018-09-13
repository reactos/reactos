/****************************Module*Header******************************\
* Module Name: packbuff.c                                               *
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

#define    BUFFER_SIZE 1024

/* bitmaps are ordered <b, g, r, i> but PCX is ordered <r, g, b, i> ... */
/* of course, the ordering of bitmaps is not defined, so this operation */
/* could fail ... */
short PlaneIndex[] = { 2, 1, 0, 3 };

BOOL PackBuff(BYTE *buff, int row, int byteWid, HANDLE fh)
{
  int  i, rByteWid;
  BYTE *buffPtr, *endPtr;
  BYTE runChar, runCount;
  BYTE pOutBuf[BUFFER_SIZE], *pOutPos, *pOutEnd;

   pOutPos = pOutBuf;
   pOutEnd = pOutPos + BUFFER_SIZE;

   rByteWid = (filePlanes == 4) ? (fileByteWid / 4) : fileByteWid;

   for (i = 0; i < filePlanes; ++i) {
       if (filePlanes == 4)
           buffPtr = buff + row * byteWid + PlaneIndex[i] * rByteWid;
       else if (filePlanes == 3)
           buffPtr = buff + row * byteWid + i * rByteWid;
       else
           buffPtr = buff;

       endPtr = buffPtr + rByteWid;

       for (runCount = 1, runChar = *buffPtr++; buffPtr <= endPtr;
            ++buffPtr) {
           if (buffPtr != endPtr && *buffPtr == runChar
               && runCount < MAXcount)
               ++runCount;
           else if (*buffPtr != runChar && runCount < MINcount
                    && (runChar & ESCbits) != ESCbits) {
               if (pOutPos + runCount - 1 >= pOutEnd) {
                   if (!MyByteWriteFile(fh, pOutBuf, pOutPos - pOutBuf))
                       return FALSE;
                   pOutPos = pOutBuf;
               }

               while (runCount--)
                   *pOutPos++ = runChar;

               runCount = 1;
               runChar = *buffPtr;
           } else {
               runCount |= ESCbits;

               if (pOutPos + 2 >= pOutEnd) {
                   if (!MyByteWriteFile(fh, pOutBuf, pOutPos - pOutBuf))
                       return FALSE;
                   pOutPos = pOutBuf;
               }

               *pOutPos++ = runCount;
               *pOutPos++ = runChar;

               runCount = 1;
               runChar = *buffPtr;
           }
       }
   }

   if (pOutPos > pOutBuf && !MyByteWriteFile(fh, pOutBuf, pOutPos - pOutBuf))
       return FALSE;

   return TRUE;
}
