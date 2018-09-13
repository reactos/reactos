/*
** init.c - Routines dealing with I/O and expansion buffers for LZCOPY() and
**          DOS command-line programs.
**
** Author:  DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>

#endif

#include "common.h"
#include "buffers.h"
#include "lzcommon.h"

PLZINFO InitGlobalBuffers(
   DWORD dwOutBufSize,
   DWORD dwRingBufSize,
   DWORD dwInBufSize)
{
   PLZINFO pLZI;

   if (!(pLZI = (PLZINFO)LocalAlloc(LPTR, sizeof(LZINFO)))) {
      return(NULL);
   }

   // Set up ring buffer.  N.b., extra (cbStrMax - 1) bytes used to
   // facilitate string comparisons near end of ring buffer.
   // (The size allocated for the ring buffer may be at most 4224, since
   //  that's the ring buffer length embedded in the LZFile structs in
   //  lzexpand.h.)

   if (dwRingBufSize == 0) {
      dwRingBufSize = MAX_RING_BUF_LEN;
   }

   if ((pLZI->rgbyteRingBuf = (BYTE FAR *)FALLOC(dwRingBufSize * sizeof(BYTE))) == NULL)
      // Bail out, since without the ring buffer, we can't decode anything.
      return(NULL);


   if (dwInBufSize == 0) {
      dwInBufSize = MAX_IN_BUF_SIZE;
   }

   if (dwOutBufSize == 0) {
      dwOutBufSize = MAX_OUT_BUF_SIZE;
   }

   for (pLZI->ucbInBufLen = dwInBufSize, pLZI->ucbOutBufLen = dwOutBufSize;
      pLZI->ucbInBufLen > 0U && pLZI->ucbOutBufLen > 0U;
      pLZI->ucbInBufLen -= IN_BUF_STEP, pLZI->ucbOutBufLen -= OUT_BUF_STEP)
   {
      // Try to set up input buffer.  N.b., extra byte because rgbyteInBuf[0]
      // will be used to hold last byte from previous input buffer.
      if ((pLZI->rgbyteInBuf = (BYTE *)FALLOC(pLZI->ucbInBufLen + 1U)) == NULL)
         continue;

      // And try to set up output buffer...
      if ((pLZI->rgbyteOutBuf = (BYTE *)FALLOC(pLZI->ucbOutBufLen)) == NULL)
      {
         FFREE(pLZI->rgbyteInBuf);
         continue;
      }

      return(pLZI);
   }

   // Insufficient memory for I/O buffers.
   FFREE(pLZI->rgbyteRingBuf);
   return(NULL);
}

PLZINFO InitGlobalBuffersEx()
{
   return(InitGlobalBuffers(MAX_OUT_BUF_SIZE, MAX_RING_BUF_LEN, MAX_IN_BUF_SIZE));
}

VOID FreeGlobalBuffers(
   PLZINFO pLZI)
{

   // Sanity check

   if (!pLZI) {
      return;
   }

   if (pLZI->rgbyteRingBuf)
   {
      FFREE(pLZI->rgbyteRingBuf);
      pLZI->rgbyteRingBuf = NULL;
   }

   if (pLZI->rgbyteInBuf)
   {
      FFREE(pLZI->rgbyteInBuf);
      pLZI->rgbyteInBuf = NULL;
   }

   if (pLZI->rgbyteOutBuf)
   {
      FFREE(pLZI->rgbyteOutBuf);
      pLZI->rgbyteOutBuf = NULL;
   }

   // Buffers deallocated ok.

   // reset thread info
   LocalFree(pLZI);
}


/*
** int GetIOHandle(char ARG_PTR *pszFileName, BOOL bRead, int ARG_PTR *pdosh);
**
** Opens input and output files.
**
** Arguments:  pszFileName - source file name
**             bRead       - mode for opening file TRUE for read and FALSE
**                           for write
**             pdosh       - pointer to buffer for DOS file handle to be
**                           filled in
**
** Returns:    int - TRUE if file opened successfully.  LZERROR_BADINHANDLE
**                   if input file could not be opened.  LZERROR_BADOUTHANDLE
**                   if output file could not be opened.  Fills in
**                   *pdosh with open DOS file handle, or NO_DOSH if
**                   pszFileName is NULL.
**
** Globals:    cblInSize  - set to length of input file
*/
INT GetIOHandle(CHAR ARG_PTR *pszFileName, BOOL bRead, INT ARG_PTR *pdosh, LONG *pcblInSize)
{
   if (pszFileName == NULL)
      *pdosh = NO_DOSH;
   else if (bRead == WRITE_IT)
   {
      // Set up output DOS file handle.
      if ((*pdosh = FCREATE(pszFileName)) == -1)
         return(LZERROR_BADOUTHANDLE);
   }
   else // (bRead == READ_IT)
   {
      if ((*pdosh = FOPEN(pszFileName)) == -1)
         return(LZERROR_BADINHANDLE);

      // Move to the end of the input file to find its length,
      // then return to the beginning.
      if ((*pcblInSize = FSEEK(*pdosh, 0L, SEEK_END)) < 0L ||
          FSEEK(*pdosh, 0L, SEEK_SET) != 0L)
      {
         FCLOSE(*pdosh);
         return(LZERROR_BADINHANDLE);
      }
   }

   return(TRUE);
}


