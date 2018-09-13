/*
** compress.c - Main compression routine for LZA file compression program.
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
** int Compress(char ARG_PTR *pszSource, char ARG_PTR *pszDest,
**              BYTE byteAlgorithm, BYTE byteExtensionChar);
**
** Compress one file to another.
**
** Arguments:  pszSource         - name of file to compress
**             pszDest           - name of compressed output file
**             byteAlgorithm     - compression algorithm to use
**             byteExtensionChar - compressed file name extension character
**
** Returns:    int - TRUE if compression finished successfully.  One of the
**                   LZERROR_ codes if not.
**
** Globals:    none
*/
INT Compress(
   NOTIFYPROC pfnNotify,
   CHAR ARG_PTR *pszSource,
   CHAR ARG_PTR *pszDest,
   BYTE byteAlgorithm,
   BOOL bDoRename,
   PLZINFO pLZI)
{
   INT doshSource,            // input file handle
       doshDest,              // output file handle
       nRetVal = TRUE;
   FH FHOut;                  // compressed header info struct
   CHAR szDestFileName[MAX_PATH];
   BYTE byteExtensionChar;

   // Sanity check
   if (!pLZI) {
      return(LZERROR_GLOBLOCK);
   }

   // Set up input file handle. Set cblInSize to length of input file.
   if ((nRetVal = GetIOHandle(pszSource, READ_IT, & doshSource, &pLZI->cblInSize)) != TRUE)
      return(nRetVal);

   // Rewind input file.
   if (FSEEK(doshSource, 0L, SEEK_SET) != 0L)
   {
      FCLOSE(doshSource);
      return(LZERROR_BADINHANDLE);
   }

   // Create destination file name.

   STRCPY(szDestFileName, pszDest);

   if (bDoRename == TRUE)
      // Rename output file.
      byteExtensionChar = MakeCompressedName(szDestFileName);
   else
      byteExtensionChar = '\0';

   // Ask if we should compress this file.
   if (! (*pfnNotify)(pszSource, szDestFileName, NOTIFY_START_COMPRESS))
   {
      // Don't compress file.    This error condition should be handled in
      // pfnNotify, so indicate that it is not necessary for the caller to
      // display an error message.
      FCLOSE(doshSource);
      return(BLANK_ERROR);
   }

   // Set up output file handle.
   if ((nRetVal = GetIOHandle(szDestFileName, WRITE_IT, & doshDest, &pLZI->cblInSize)) != TRUE)
   {
      FCLOSE(doshSource);
      return(nRetVal);
   }

   // Fill in compressed file header.
   MakeHeader(& FHOut, byteAlgorithm, byteExtensionChar, pLZI);

   // Write compressed file header to output file.
   if ((nRetVal = WriteHdr(& FHOut, doshDest, pLZI)) != TRUE)
      goto COMPRESS_EXIT;

   // Compress input file into output file.
   switch (byteAlgorithm)
   {
      case ALG_FIRST:
#if 0
      case ALG_LZ:
#endif
         nRetVal = LZEncode(doshSource, doshDest, pLZI);
         break;

      default:
         nRetVal = LZERROR_UNKNOWNALG;
         break;
   }

   if (nRetVal != TRUE)
      goto COMPRESS_EXIT;

   // Copy date and time stamp from source file to destination file.
   nRetVal = CopyDateTimeStamp(doshSource, doshDest);

COMPRESS_EXIT:
   // Close files.
   FCLOSE(doshSource);
   FCLOSE(doshDest);

   return(nRetVal);
}

