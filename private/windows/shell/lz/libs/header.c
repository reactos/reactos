/*
** header.c - Routines used to access compressed file header information.
**
** written by DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL

#include <io.h>
#include <dos.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#endif

#include "common.h"
#include "buffers.h"
#include "header.h"


/*
** int WriteHdr(PFH pFH, int doshDest);
**
** Write compressed file header to output file.
**
** Arguments:  pFH      - pointer to source header information structure
**             doshDest - DOS file handle of open output file
**
** Returns:    int - TRUE if successful.  LZERROR_BADOUTHANDLE if
**                   unsuccessful.
**
** Globals:    none
**
** header format:
**                8 bytes  -->   compressed file signature
**                1 byte   -->   algorithm label
**                1 byte   -->   extension char
**                4 bytes  -->   uncompressed file size (LSB to MSB)
**
**       length = 14 bytes
*/
INT WriteHdr(PFH pFH, INT doshDest, PLZINFO pLZI)
{
   INT i, j;
   DWORD ucbWritten;
   BYTE rgbyteHeaderBuf[HEADER_LEN];   // temporary storage for next header byte to write

   // Sanity check
   if (!pLZI) {
      return(LZERROR_GLOBLOCK);
   }

   // Copy the compressed file signature.
   for (i = 0; i < COMP_SIG_LEN; i++)
      rgbyteHeaderBuf[i] = pFH->rgbyteMagic[i];

   // Copy the algorithm label and file name extension character.
   rgbyteHeaderBuf[i++] = pFH->byteAlgorithm;
   rgbyteHeaderBuf[i++] = pFH->byteExtensionChar;

   // Copy input file size (long ==> 4 bytes),
   // LSB first to MSB last.
   for (j = 0; j < 4; j++)
      rgbyteHeaderBuf[i++] = (BYTE)((pFH->cbulUncompSize >> (8 * j)) &
                                    (DWORD)BYTE_MASK);

   // Write header to file.
   if ((ucbWritten = FWRITE(doshDest, rgbyteHeaderBuf, HEADER_LEN)) != HEADER_LEN)
   {
#ifdef LZA_DLL
      if (ucbWritten == (DWORD)(-1))
#else
      if (_error != 0U)
#endif
         // Bad DOS file handle.
         return(LZERROR_BADOUTHANDLE);
      else
         // Insufficient space on destination drive.
         return(LZERROR_WRITE);
   }

   // Keep track of bytes written.
   pLZI->cblOutSize += (LONG)ucbWritten;

   // Header written ok.
   return(TRUE);
}


/*
** int GetHdr(PFH pFH, int doshSource);
**
** Get compressed file header.
**
** Arguments:  pFH        - pointer to destination header information structure
**             doshSource - DOS file handle of open input file
**
** Returns:    int - TRUE if compressed file header read successfully.  One
**                   the LZERROR_ codes if not.
**
** Globals:    none
*/
INT GetHdr(PFH pFH, INT doshSource, LONG *pcblInSize)
{
   DWORD ucbRead;
   BYTE rgbyteHeaderBuf[HEADER_LEN];
   INT i, j;

   // Get input file length and move back to beginning of input file.
   if ((*pcblInSize = FSEEK(doshSource, 0L, SEEK_END)) <  0L ||
       FSEEK(doshSource, 0L, SEEK_SET) != 0L)
      return(LZERROR_BADINHANDLE);

   if ((ucbRead = FREAD(doshSource, rgbyteHeaderBuf, HEADER_LEN))
       != HEADER_LEN)
   {
#ifdef LZA_DLL
      if (ucbRead == (DWORD)(-1))
#else
      if (_error != 0U)
#endif
         // We were handed a bad input file handle.
         return((INT)LZERROR_BADINHANDLE);
      else
         // Input file shorter than compressed header size.
         return(LZERROR_READ);
   }

   // Put compressed file signature into rgbyteMagic[] of header info struct.
   for (i = 0; i < COMP_SIG_LEN; i++)
      pFH->rgbyteMagic[i] = rgbyteHeaderBuf[i];

   // Get algorithm label and file name extension character.
   pFH->byteAlgorithm = rgbyteHeaderBuf[i++];
   pFH->byteExtensionChar = rgbyteHeaderBuf[i++];

   // Extract uncompressed file size, LSB --> MSB (4 bytes in long).
   pFH->cbulUncompSize = 0UL;
   for (j = 0; j < 4; j++)
      pFH->cbulUncompSize |= ((DWORD)(rgbyteHeaderBuf[i++]) << (8 * j));

   // Stick compressed file size into header info struct.
   pFH->cbulCompSize = (DWORD)*pcblInSize;

   // File header read ok.
   return(TRUE);
}


/*
** BOOL IsCompressed(PFH pFHIn);
**
** See if a file is in compressed form by comparing its file signature with
** the expected compressed file signature.
**
** Arguments:  pFHIn - pointer to header info struct to check
**
** Returns:    BOOL - TRUE if file signature matches expected compressed file
**                    signature.  FALSE if not.
**
** Globals:    none
*/
BOOL IsCompressed(PFH pFHIn)
{
   INT i;
   // storage for FHIn's compressed file signature (used to make it an sz)
   CHAR rgchBuf[COMP_SIG_LEN + 1];

   // Copy file info struct's compressed file signature into rgchBuf[] to
   // make it an sz.
   for (i = 0; i < COMP_SIG_LEN; i++)
      rgchBuf[i] = pFHIn->rgbyteMagic[i];

   rgchBuf[i] = '\0';

   return((STRCMP(rgchBuf, COMP_SIG) == 0) ? TRUE : FALSE);
}


/*
** void MakeHeader(PFH pFHBlank, BYTE byteAlgorithm,
**                 BYTE byteExtensionChar);
**
** Arguments:  pFHBlank          - pointer to compressed file header struct
**                                 that is to be filled in
**             byteAlgorithm     - algorithm label
**             byteExtensionChar - uncompressed file name extension character
**
** Returns:    void
**
** Globals:    none
**
** Global cblInSize is used to fill in expanded file length field.
** Compressed file length field is set to 0 since it isn't written.
**
*/
VOID MakeHeader(PFH pFHBlank, BYTE byteAlgorithm,
                BYTE byteExtensionChar, PLZINFO pLZI)
{
   INT i;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // Fill in compressed file signature.
   for (i = 0; i < COMP_SIG_LEN; i++)
      pFHBlank->rgbyteMagic[i] = (BYTE)(*(COMP_SIG + i));

   // Fill in algorithm and extesion character.
   pFHBlank->byteAlgorithm = byteAlgorithm;
   pFHBlank->byteExtensionChar = byteExtensionChar;

   // Fill in file sizes.  (cbulCompSize not written to compressed file
   // header, so just set it to 0UL.)
   pFHBlank->cbulUncompSize = (DWORD)pLZI->cblInSize;
   pFHBlank->cbulCompSize = 0UL;
}


