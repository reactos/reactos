/****************************Module*Header******************************\
* Module Name: unpkbuff.c                                               *
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
#include <port1632.h>
#include <memory.h>

#include "pbrush.h"

#define    BUFFER_SIZE 1024

/* file buffer vars. */
static HANDLE hBuf;
static LPBYTE pBuf;
static int iBuf;

BOOL InitFileBuffer(void)
{
   if (!(hBuf = LocalAlloc(LHND, BUFFER_SIZE)))
       goto error1;

   if (!(pBuf = LocalLock(hBuf)))
       goto error2;

   iBuf = BUFFER_SIZE;

   return TRUE;

error2:
   LocalFree(hBuf);
   hBuf = NULL;

error1:
   return FALSE;
}

void DeleteFileBuffer(void)
{
   if (hBuf) {
       LocalUnlock(hBuf);
       LocalFree(hBuf);
       hBuf = NULL;
       pBuf = NULL;
   }
}

BYTE bgetc(HANDLE fh)
{
   if (iBuf == BUFFER_SIZE) {
       MyByteReadFile(fh, pBuf, BUFFER_SIZE);
       iBuf = 0;
   }

   return pBuf[iBuf++];
}

BOOL UnpkBuff(BYTE *buff, int row, DHDR hdr, int nPlanes, HANDLE fh)
{
   unsigned    cntr;
   int         i;
   BYTE        runChar, runCount;
   BYTE       *buffPtr, *endPtr;

   /* upack scan lines and transfer to fileBuff */
   runCount = 0;
   for (i = 0; i < nPlanes; ++i) {
       buffPtr = buff + i * hdr.bplin;

       endPtr = buffPtr + hdr.bplin;
       while (buffPtr < endPtr) {
           if (runCount == 1) {
               *buffPtr++ = runChar;
               runCount--;
           } else {
               cntr = min((unsigned)runCount, (unsigned)(endPtr - buffPtr));
               RepeatFill(buffPtr, runChar, cntr);
               runCount -= (BYTE) cntr;
               buffPtr += cntr;
           }

           if (buffPtr == endPtr)
               break;

           runCount = bgetc(fh);
           if ((runCount & ESCbits) != ESCbits) {
               *buffPtr++ = runCount;
               runCount = 0;
           } else {
               runCount &= ~ESCbits;
               runChar = bgetc(fh);
           }
       }
   }

   /* fill in intensity plane with 1s if we are reading 3 plane file */
   if (nPlanes != hdr.nPlanes)
       RepeatFill(buffPtr, 0xff, hdr.bplin);

   return TRUE;
}
