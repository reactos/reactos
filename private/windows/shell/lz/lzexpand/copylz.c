/*
** copylz.c - CopyLZFile() and buffer management functions.
**
** Author:  DavidDi
**
** This module is compiled twice - once with LZA_DLL defined for the Windows
** DLL, and once without LZDLL defined for static DOS library.
*/


// Headers
///////////


#include "..\libs\common.h"
#include "..\libs\buffers.h"
#include "..\libs\header.h"

#include "lzpriv.h"


/*
** int  APIENTRY LZStart(void);
**
** If the global buffers are not already initialized, allocates buffers in
** preparation for calls to CopyLZFile().  Increments the global buffer lock
** count.  Sets the global buffers' base pointers and lengths.
**
** Arguments:  void
**
** Returns:    int - TRUE if successful.  LZERROR_GLOBALLOC if unsuccessful.
**
** Globals:    none
*/

INT  APIENTRY LZStart(VOID)
{
   return(TRUE);
}


/*
** VOID  APIENTRY LZDone(void);
**
** If any of the global buffers have not already been freed, frees them and
** resets the buffers' base array pointers to NULL.  Decrements the global
** buffer lock count.
**
** Arguments:  void
**
** Returns:    VOID
**
** Globals:    none
*/
VOID  APIENTRY LZDone(VOID)
{
   return;
}

/*
** CopyLZFile()
**
** Alias for LZCopy().  This is stupid.  Originally, LZCopy() and
** CopyLZFile() were intended for different purposes, but they were confused
** and misused so much they were made identical.
*/
LONG APIENTRY CopyLZFile(HFILE doshSource, HFILE doshDest)
{
   return(LZCopy(doshSource, doshDest));
}

/*
** LZCopy()
**
** Expand a compressed file, or copy an uncompressed file.
**
** Arguments:  doshSource - source DOS file handle
**             doshDest   - destination DOS file handle
**
** Returns:    LONG - Number of bytes written if copy was successful.
**                    One of the LZERROR_ codes if unsuccessful.
**
** Globals:    none
*/
LONG  APIENTRY LZCopy(HFILE doshSource, HFILE doshDest)
{
   INT f;
   LONG lRetVal;
   PLZINFO pLZI;

   // If it's a compressed file handle, translate to a DOS handle.
   if (doshSource >= LZ_TABLE_BIAS)
   {
      LZFile *lpLZ;       // pointer to LZFile struct
      HANDLE hLZFile;         // handle to LZFile struct

      if ((hLZFile = rghLZFileTable[doshSource - LZ_TABLE_BIAS]) == NULL)
      {
         return(LZERROR_BADINHANDLE);
      }

      if ((lpLZ = (LZFile *)GlobalLock(hLZFile)) == NULL)
      {
         return(LZERROR_GLOBLOCK);
      }

      doshSource = lpLZ->dosh;
      doshDest = ConvertDosFHToWin32(doshDest);

      GlobalUnlock(hLZFile);
   }
   else {
      doshDest   = ConvertDosFHToWin32(doshDest);
      doshSource = ConvertDosFHToWin32(doshSource);
   }

   // Initialize buffers
   pLZI = InitGlobalBuffersEx();

   if (!pLZI) {
      return(LZERROR_GLOBALLOC);
   }

   ResetBuffers();

   // Expand / copy file.
   if ((f = ExpandOrCopyFile(doshSource, doshDest, pLZI)) != TRUE) {
      // Expansion / copy failed.
      lRetVal = (LONG)f;
   } else {
      // Expansion / copy successful - return number of bytes written.
      lRetVal = pLZI->cblOutSize;
   }

   // Free global buffers.
   FreeGlobalBuffers(pLZI);

   return(lRetVal);
}

