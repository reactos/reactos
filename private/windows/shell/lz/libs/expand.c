/*
** expand.c - Main expansion routine for LZA file expansion program.
**
** Author: DavidDi
*/


// Headers
///////////

#ifndef LZA_DLL

#include <dos.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#endif

#include "common.h"
#include "lzcommon.h"
#include "buffers.h"
#include "header.h"


/*
** N.b., one reason DOS file handles are used for file references in this
** module is that using FILE *'s for file references poses a problem.
** fclose()'ing a file which was fopen()'ed in write "w" or append "a" mode
** stamps the file with the current date.  This undoes the intended effect of
** CopyDateTimeStamp().  We could also get around this fclose() problem by
** first fclose()'ing the file, and then fopen()'ing it again in read "r"
** mode.
**
** Using file handles also allows us to bypass stream buffering, so reads and
** writes may be done with whatever buffer size we choose.  Also, the
** lower-level DOS file handle functions are faster than their stream
** counterparts.
*/


/*
** int CopyFile(int doshSource, int doshDest);
**
** Copy file.
**
** Arguments:  doshSource  - source DOS file handle
**             doshDest    - destination DOS file handle
**
** Returns:    int - TRUE if successful.  One of the LZERROR_ codes if
**                   unsuccessful.
**
** Globals:    none
*/
/* WIN32 MOD, CopyFile is a win32 API!*/
INT lz_CopyFile(INT doshSource, INT doshDest, PLZINFO pLZI)
{
   DWORD ucbRead, ucbWritten;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // Rewind input file again.
   if (FSEEK(doshSource, 0L, SEEK_SET) != 0L) {
      return(LZERROR_BADINHANDLE);
   }

   // Rewind output file.
   if (doshDest != NO_DOSH &&
       FSEEK(doshDest, 0L, SEEK_SET) != 0L) {
      return( LZERROR_BADOUTHANDLE );
   }

   // Set up a fresh buffer state.
   ResetBuffers();

   while ((ucbRead = FREAD(doshSource, pLZI->rgbyteInBuf, pLZI->ucbInBufLen)) > 0U &&
#ifdef LZA_DLL
           ucbRead != (DWORD)(-1))
#else
           FERROR() == 0)
#endif
   {
      if ((ucbWritten = FWRITE(doshDest, pLZI->rgbyteInBuf, ucbRead)) != ucbRead)
#ifdef LZA_DLL
         if (ucbWritten != (DWORD)(-1)) {
#else
         if (FERROR() != 0) {
#endif
            return(LZERROR_BADOUTHANDLE);
         }
         else {
            return(LZERROR_WRITE);
         }

      pLZI->cblOutSize += ucbWritten;

      if (ucbRead != pLZI->ucbInBufLen)
         break;
   }

#ifdef LZA_DLL
   // here, ucbRead ==  0,    EOF (proper loop termination)
   //               == -1,    bad DOS handle
   if (ucbRead == (DWORD)(-1)) {
#else
   // here, FERROR() == 0U,   EOF (proper loop termination)
   //                != 0U,   bad DOS handle
   if (FERROR() != 0U) {
#endif
      return(LZERROR_BADINHANDLE);
   }

   // Copy successful - return number of bytes copied.
   return(TRUE);
}


/*
** int ExpandOrCopyFile(int doshDource, int doshDest);
**
** Expands one file to another.
**
** Arguments:  doshSource - source DOS file handle
**             doshDest   - destination DOS file handle
**
** Returns:    int - TRUE if expansion finished successfully.  One of the
**                   LZERROR_ codes if not.
**
** Globals:    none
*/
INT ExpandOrCopyFile(INT doshSource, INT doshDest, PLZINFO pLZI)
{
   INT f;
   FH FHInfo;                 // compressed header info struct
   BOOL bExpandingFile;

   // !!! Assumes pLZI parm is valid.  No sanity check (should be done above in caller).

   // Get compressed file header.
   if (GetHdr(&FHInfo, doshSource, &pLZI->cblInSize) != TRUE
       && pLZI->cblInSize >= (LONG)HEADER_LEN)
      // read error occurred
      return(LZERROR_BADINHANDLE);

   // Expand or copy input file to output file.
   bExpandingFile = (IsCompressed(& FHInfo) == TRUE);

   if (bExpandingFile)
   {
      switch (FHInfo.byteAlgorithm)
      {
         case ALG_FIRST:
            f = LZDecode(doshSource, doshDest, (LONG)FHInfo.cbulUncompSize - 1L,
               TRUE, TRUE, pLZI);
            break;

#if 0
         case ALG_LZ:
            f = LZDecode(doshSource, doshDest, (LONG)FHInfo.cbulUncompSize - 1L,
               TRUE, FALSE, pLZI);
            break;
#endif

         default:
            f = LZERROR_UNKNOWNALG;
            break;
      }
   }
   else
      f = lz_CopyFile(doshSource, doshDest, pLZI);

   if (f != TRUE)
      return(f);

   // Flush output buffer to file.
   if ((f = FlushOutputBuffer(doshDest, pLZI)) != TRUE)
      return(f);

   // Copy date and time stamp from source file to destination file.
   if ((f = CopyDateTimeStamp(doshSource, doshDest)) != TRUE)
      return(f);

   // Did we expand the exact number of bytes we expected to from the
   // compressed file header entry?
   if (bExpandingFile &&
       (DWORD)pLZI->cblOutSize != FHInfo.cbulUncompSize)
      return(LZERROR_READ);

   // Expansion / copying finished successfully.
   return(TRUE);
}


/*
** int Expand(char ARG_PTR *pszSource, char ARG_PTR *pszDest, BOOL bDoRename);
**
** Expands one file to another.
**
** Arguments:  pszSource - name of file to compress
**             pszDest   - name of compressed output file
**             bDoRename - flag for output file renaming
**
** Returns:    int - TRUE if expansion finished successfully.  One of the
**                   LZERROR_ codes if not.
**
** Globals:    none
*/
INT Expand(
   NOTIFYPROC pfnNotify,
   CHAR ARG_PTR *pszSource,
   CHAR ARG_PTR *pszDest,
   BOOL bDoRename,
   PLZINFO pLZI)
{
   INT doshSource,            // input file handle
       doshDest,              // output file handle
       f;
   FH FHInfo;                 // compressed header info struct
   CHAR szDestFileName[MAX_PATH];

   // Sanity check
   if (!pLZI) {
      return(LZERROR_GLOBLOCK);
   }

   // Set up input file handle. Set cblInSize to length of input file.
   if ((f = GetIOHandle(pszSource, READ_IT, & doshSource, &pLZI->cblInSize)) != TRUE)
      return(f);

   if (GetHdr(&FHInfo, doshSource, &pLZI->cblInSize) != TRUE &&
       pLZI->cblInSize >= (LONG)HEADER_LEN)
   {
      // Read error occurred.
      FCLOSE(doshSource);
      return(LZERROR_BADINHANDLE);
   }

   // Create destination file name.

   STRCPY(szDestFileName, pszDest);

#if 0
   if (bDoRename == TRUE && FHInfo.byteAlgorithm != ALG_FIRST)
#else
   if (bDoRename == TRUE)
#endif
   {
      // Rename output file using expanded file name extension character
      // stored in compressed file header.
      MakeExpandedName(szDestFileName, FHInfo.byteExtensionChar);
   }

   // Ask if we should compress this file.
   if (! (*pfnNotify)(pszSource, szDestFileName, (WORD)
                      (IsCompressed(&FHInfo) ?  NOTIFY_START_EXPAND : NOTIFY_START_COPY)))
   {
      // Don't expand / copy file.  This error condition should be handled in
      // pfnNotify, so indicate that it is not necessary for the caller to
      // display an error message.
      FCLOSE(doshSource);
      return(BLANK_ERROR);
   }

   // Set up output file handle.
   if ((f = GetIOHandle(szDestFileName, WRITE_IT, & doshDest, &pLZI->cblInSize)) != TRUE)
   {
      FCLOSE(doshSource);
      return(f);
   }

   // Expand or copy input file into output file.
   f = ExpandOrCopyFile(doshSource, doshDest, pLZI);

   // Close files.
   FCLOSE(doshSource);
   FCLOSE(doshDest);

   return(f);
}

