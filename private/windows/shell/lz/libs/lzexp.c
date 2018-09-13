/*
** lzexp.c - Routines used in Lempel-Ziv (from their 1977 article) expansion.
**
** Author:  DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL
#include <io.h>
#endif

#include "common.h"
#include "buffers.h"
#include "header.h"
#include "lzcommon.h"


/*
** int LZDecode(int doshSource, int doshDest, long cblExpandedLength,
**              BOOL bRestartDecoding, BOOL bFirstAlg);
**
** Expand input file to output file.
**
** Arguments:  doshSource        - DOS file handle of open input file
**             doshDest          - DOS file handle of open output file
**             cblExpandedLength - amount of output file to expand
**             bRestartDecoding  - flag indicating whether or not to start
**                                 decoding from scratch
**             bFirstAlg         - flag indicating whether to use ALG_FIRST
**                                 or ALG_LZ
**
** Returns:    int - TRUE if expansion was successful.  One of the LZERROR_
**                   codes if the expansion failed.
**
** Globals:
**
** The number of bytes actually expanded will be >= cblExpandedLength.  The
** number of bytes actually expanded may be calculated as
** (pbyteOutBuf - rgbyteOutBuf).  The expansion will overrun the
** cblExpandedLength request by at most (cbMaxMatchLen - 1) bytes.
*/
INT LZDecode(
   INT doshSource,
   INT doshDest,
   LONG cblExpandedLength,
   BOOL bRestartDecoding,
   BOOL bFirstAlg,
   PLZINFO pLZI)
{
   INT i,
       cb,                          // number of bytes to unpack
       f;                           // holds ReadByte() return values
   INT oStart;                      // buffer offset for unpacking
   BYTE byte1, byte2;               // input byte holders


   // !!! Assumes parm pLZI is always valid

#if 0
   if (bFirstAlg == TRUE)
      pLZI->cbMaxMatchLen = FIRST_MAX_MATCH_LEN;
   else
      pLZI->cbMaxMatchLen = LZ_MAX_MATCH_LEN;
#else
   pLZI->cbMaxMatchLen = FIRST_MAX_MATCH_LEN;
#endif

   // Start decoding from scratch?
   if (bRestartDecoding == TRUE)
   {
      // Rewind the compressed input file to just after the compressed file
      // header.
      if (FSEEK(doshSource, (LONG)HEADER_LEN, SEEK_SET) != (LONG)HEADER_LEN) {
         return(LZERROR_BADINHANDLE);
      }

      // Rewind output file.
      if (doshDest != NO_DOSH &&
          FSEEK(doshDest, 0L, SEEK_SET) != 0L) {
         return(LZERROR_BADOUTHANDLE);
      }

      // Set up a fresh buffer state.
      ResetBuffers();

      // Initialize ring buffer.
      for (i = 0; i < RING_BUF_LEN - pLZI->cbMaxMatchLen; i++)
         pLZI->rgbyteRingBuf[i] = BUF_CLEAR_BYTE;

      // Initialize decoding globals.
      pLZI->uFlags = 0U;
      pLZI->iCurRingBufPos = RING_BUF_LEN - pLZI->cbMaxMatchLen;
   }

   if ((f = ReadByte(byte1)) != TRUE && f != END_OF_INPUT) {
      return(f);
   }

   // Decode one encoded unit at a time.
   FOREVER
   {
      if (f == END_OF_INPUT)  // EOF reached
         break;

      // Have we expanded enough data yet?
      if (pLZI->cblOutSize > cblExpandedLength)    // Might want to make this >=.
      {
         UnreadByte();
         return(TRUE);
      }

      // High order byte counts the number of bits used in the low order
      // byte.
      if (((pLZI->uFlags >>= 1) & 0x100) == 0)
      {
         // Set bit mask describing the next 8 bytes.
         pLZI->uFlags = ((DWORD)byte1) | 0xff00;

         if ((f = ReadByte(byte1)) != TRUE) {
            return(LZERROR_READ);
         }
      }

      if (pLZI->uFlags & 1)
      {
         // Just store the literal byte in the buffer.
         if ((f = WriteByte(byte1)) != TRUE) {
            return(f);
         }

         pLZI->rgbyteRingBuf[pLZI->iCurRingBufPos++] = byte1;
         pLZI->iCurRingBufPos &= RING_BUF_LEN - 1;
      }
      else
      {
         // Extract the offset and count to copy from the ring buffer.
         if ((f = ReadByte(byte2)) != TRUE) {
            return(LZERROR_READ);
         }

         cb = (INT)byte2;
         oStart = (cb & 0xf0) << 4 | (INT)byte1;
         cb = (cb & 0x0f) + MAX_LITERAL_LEN;

         for (i = 0; i <= cb; i++)
         {
            byte1 = pLZI->rgbyteRingBuf[(oStart + i) & (RING_BUF_LEN - 1)];

            if ((f = WriteByte(byte1)) != TRUE) {
               return( f );
            }

            pLZI->rgbyteRingBuf[pLZI->iCurRingBufPos++] = byte1;
            pLZI->iCurRingBufPos &= RING_BUF_LEN - 1;
         }
      }

      if ((f = ReadByte(byte1)) != TRUE && f != END_OF_INPUT) {
         return(f);
      }
   }

   return(TRUE);
}


