/*
** buffers.c - Routines dealing with I/O and expansion buffers for LZCopy()
**             and DOS command-line programs.
**
** Author:  DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL

#include <dos.h>
#include <fcntl.h>

#endif

#include "common.h"
#include "buffers.h"

/*
** int ReadInBuf(BYTE ARG_PTR *pbyte, int doshSource);
**
** Read input file into input buffer.
**
** Arguments:  pbyte      - pointer to storage for first byte read from file
**                          into buffer
**             doshSource - DOS file handle to open input file
**
** Returns:    int - TRUE or END_OF_INPUT if successful.  LZERROR_BADINHANDLE
**                   if not.
**
** Globals:    rgbyteInBuf[0] - holds last byte from previous buffer
**             pbyteInBufEnd  - set to point to first byte beyond end of data
**                              in input buffer
**             bLastUsed      - reset to FALSE if currently TRUE
*/
INT ReadInBuf(BYTE ARG_PTR *pbyte, INT doshSource, PLZINFO pLZI)
{
   DWORD ucbRead;          // number of bytes actually read

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   pLZI->rgbyteInBuf[0] = *(pLZI->pbyteInBufEnd - 1);

   if ((ucbRead = FREAD(doshSource, &pLZI->rgbyteInBuf[1], pLZI->ucbInBufLen))
       != pLZI->ucbInBufLen)
   {
#ifdef LZA_DLL
      if (ucbRead == (DWORD)(-1)) {
#else
      if (_error != 0U) {
#endif
         // We were handed a bad input file handle.
         return(LZERROR_BADINHANDLE);
      }
      else if (ucbRead > 0U)
         // Read last ucbRead bytes of input file.  Change input buffer end
         // to account for shorter read.
         pLZI->pbyteInBufEnd = &pLZI->rgbyteInBuf[1] + ucbRead;
      else  { // (ucbRead == 0U) {
         // We couldn't read any bytes from input file (EOF reached).
         return(END_OF_INPUT);
      }
   }

   // Reset read pointer to beginning of input buffer.
   pLZI->pbyteInBuf = &pLZI->rgbyteInBuf[1];

   // Was an UnreadByte() done at the beginning of the last buffer?
   if (pLZI->bLastUsed)
   {
      // Return the last byte from the previous input buffer
      *pbyte = pLZI->rgbyteInBuf[0];
      pLZI->bLastUsed = FALSE;
   }
   else
      // Return the first byte from the new input buffer.
      *pbyte = *pLZI->pbyteInBuf++;

   return(TRUE);
}


/*
** int WriteOutBuf(BYTE byteNext, int doshDest);
**
** Dumps output buffer to output file.  Prompts for new floppy disk if the
** old one if full.  Continues dumping to output file of same name on new
** floppy disk.
**
** Arguments:  byteNext - first byte to be added to empty buffer after buffer
**                        is written
**             doshDest - output DOS file handle
**
** Returns:    int - TRUE if successful.  LZERROR_BADOUTHANDLE or
**                   LZERROR_WRITE if unsuccessful.
**
** Globals:    pbyteOutBuf - reset to point to free byte after byteNext in
**                           rgbyteOutBuf
*/
INT WriteOutBuf(BYTE byteNext, INT doshDest, PLZINFO pLZI)
{
   DWORD ucbToWrite,       // number of bytes to write from buffer
            ucbWritten,       // number of bytes actually written
            ucbTotWritten;    // total number of bytes written to output

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // How much of the buffer should be written to the output file?
   ucbTotWritten = ucbToWrite = (DWORD)(pLZI->pbyteOutBuf - pLZI->rgbyteOutBuf);
   // Reset pointer to beginning of buffer.
   pLZI->pbyteOutBuf = pLZI->rgbyteOutBuf;

   // Write to ouput file.
   if (doshDest != NO_DOSH &&
       (ucbWritten = FWRITE(doshDest, pLZI->pbyteOutBuf, ucbToWrite)) != ucbToWrite)
   {
#ifdef LZA_DLL
      if (ucbWritten == (DWORD)(-1)) {
#else
      if (_error != 0U) {
#endif
         // Bad DOS file handle.
         return(LZERROR_BADOUTHANDLE);
      }
      else {
         // Insufficient space on destination drive.
         return(LZERROR_WRITE);
      }
   }

   // Add the next byte to the buffer.
   *pLZI->pbyteOutBuf++ = byteNext;

   return(TRUE);
}

