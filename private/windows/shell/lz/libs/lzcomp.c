/*
** lzcomp.c - Routines used in Lempel-Ziv compression (a la 1977 article).
**
** Author:  DavidDi
*/


// Headers
///////////

#include "common.h"
#include "buffers.h"
#include "header.h"
#include "lzcommon.h"


/*
** int LZEncode(int doshSource, int doshDest);
**
** Compress input file into output file.
**
** Arguments:  doshSource    - open DOS file handle of input file
**             doshDest      - open DOS file handle of output file
**
** Returns:    int - TRUE if compression was successful.  One of the LZERROR_
**                   codes if the compression failed.
**
** Globals:
*/
INT LZEncode(INT doshSource, INT doshDest, PLZINFO pLZI)
{
   INT   i, len, f,
         iCurChar,      // current ring buffer position
         iCurString,    // start of current string in ring buffer
         iCodeBuf,      // index of next open buffer position
         cbLastMatch;   // length of last match
   BYTE byte,           // temporary storage for next byte to write
        byteMask,       // bit mask (and counter) for eight code units
        codeBuf[1 + 8 * MAX_LITERAL_LEN]; // temporary storage for encoded data

#if 0
   pLZI->cbMaxMatchLen = LZ_MAX_MATCH_LEN;
#else
   pLZI->cbMaxMatchLen = FIRST_MAX_MATCH_LEN;
#endif

   ResetBuffers();

   pLZI->cblOutSize += HEADER_LEN;

   // Initialize encoding trees.
   if (!LZInitTree(pLZI)) {
      return( LZERROR_GLOBALLOC );
   }

   // CodeBuf[1..16] saves eight units of code, and CodeBuf[0] works as eight
   // flags.  '1' representing that the unit is an unencoded letter (1 byte),
   // '0' a position-and-length pair (2 bytes).  Thus, eight units require at
   // most 16 bytes of code, plus the one byte of flags.
   codeBuf[0] = (BYTE)0;
   byteMask = (BYTE)1;
   iCodeBuf = 1;

   iCurString = 0;
   iCurChar = RING_BUF_LEN - pLZI->cbMaxMatchLen;

   for (i = 0; i < RING_BUF_LEN - pLZI->cbMaxMatchLen; i++)
      pLZI->rgbyteRingBuf[i] = BUF_CLEAR_BYTE;

   // Read bytes into the last cbMaxMatchLen bytes of the buffer.
   for (len = 0; len < pLZI->cbMaxMatchLen && ((f = ReadByte(byte)) != END_OF_INPUT);
        len++)
   {
      if (f != TRUE) {
         return( f );
      }

      pLZI->rgbyteRingBuf[iCurChar + len] = byte;
   }

   // Insert the cbMaxMatchLen strings, each of which begins with one or more
   // 'space' characters.  Note the order in which these strings are inserted.
   // This way, degenerate trees will be less likely to occur.
   for (i = 1; i <= pLZI->cbMaxMatchLen; i++)
      LZInsertNode(iCurChar - i, FALSE, pLZI);

   // Finally, insert the whole string just read.  The global variables
   // cbCurMatch and iCurMatch are set.
   LZInsertNode(iCurChar, FALSE, pLZI);

   do // while (len > 0)
   {
      // cbCurMatch may be spuriously long near the end of text.
      if (pLZI->cbCurMatch > len)
         pLZI->cbCurMatch = len;

      if (pLZI->cbCurMatch <= MAX_LITERAL_LEN)
      {
         // This match isn't long enough to encode, so copy it directly.
         pLZI->cbCurMatch = 1;
         // Set 'one uncoded byte' bit flag.
         codeBuf[0] |= byteMask;
         // Write literal byte.
         codeBuf[iCodeBuf++] = pLZI->rgbyteRingBuf[iCurChar];
      }
      else
      {
         // This match is long enough to encode.  Send its position and
         // length pair.  N.b., pLZI->cbCurMatch > MAX_LITERAL_LEN.
         codeBuf[iCodeBuf++] = (BYTE)pLZI->iCurMatch;
         codeBuf[iCodeBuf++] = (BYTE)((pLZI->iCurMatch >> 4 & 0xf0) |
                                      (pLZI->cbCurMatch - (MAX_LITERAL_LEN + 1)));
      }

      // Shift mask left one bit.
      if ((byteMask <<= 1) == (BYTE)0)
      {
         // Send at most 8 units of code together.
         for (i = 0; i < iCodeBuf; i++)
            if ((f = WriteByte(codeBuf[i])) != TRUE) {
               return( f );
            }

         // Reset flags and mask.
         codeBuf[0] = (BYTE)0;
         byteMask = (BYTE)1;
         iCodeBuf = 1;
      }

      cbLastMatch = pLZI->cbCurMatch;

      for (i = 0; i < cbLastMatch && ((f = ReadByte(byte)) != END_OF_INPUT);
           i++)
      {
         if (f != TRUE) {
            return( f );
         }

         // Delete old string.
         LZDeleteNode(iCurString, pLZI);
         pLZI->rgbyteRingBuf[iCurString] = byte;

         // If the start position is near the end of buffer, extend the
         // buffer to make string comparison easier.
         if (iCurString < pLZI->cbMaxMatchLen - 1)
            pLZI->rgbyteRingBuf[iCurString + RING_BUF_LEN] = byte;

         // Increment position in ring buffer modulo RING_BUF_LEN.
         iCurString = (iCurString + 1) & (RING_BUF_LEN - 1);
         iCurChar = (iCurChar + 1) & (RING_BUF_LEN - 1);

         // Register the string in rgbyteRingBuf[r..r + cbMaxMatchLen - 1].
         LZInsertNode(iCurChar, FALSE, pLZI);
      }

      while (i++ < cbLastMatch)
      {
         // No need to read after the end of the input, but the buffer may
         // not be empty.
         LZDeleteNode(iCurString, pLZI);
         iCurString = (iCurString + 1) & (RING_BUF_LEN - 1);
         iCurChar = (iCurChar + 1) & (RING_BUF_LEN - 1);
         if (--len)
            LZInsertNode(iCurChar, FALSE, pLZI);
      }
   } while (len > 0);   // until there is no input to process

   if (iCodeBuf > 1)
      // Send remaining code.
      for (i = 0; i < iCodeBuf; i++)
         if ((f = WriteByte(codeBuf[i])) != TRUE) {
            return( f );
         }

   // Flush output buffer to file.
   if ((f = FlushOutputBuffer(doshDest, pLZI)) != TRUE) {
      return( f );
   }

   LZFreeTree(pLZI);
   return(TRUE);
}
