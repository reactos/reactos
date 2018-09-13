/*
** winlzexp.c - Windows LZExpand library routines for manipulating compressed
**              files.
**
** Author:  DavidDi
*/


/*
** Notes:
** -----
**
** The LZInit() function returns either DOS handles or LZFile struct
** identifiers of some kind, depending upon how it is called.  The LZ
** functions LZSeek(), LZRead(), and LZClose() needed some way to
** differentiate between DOS file handles and the LZFile struct identifiers.
** As the functions stand now, they use DOS file handles as themselves, and
** table offsets as LZFile identifiers.  The table offsets are incremented by
** some base value, LZ_TABLE_BIAS, in order to push their values past all
** possible DOS file handle values.  The table offsets (- LZ_TABLE_BIAS) are
** used as indices in rghLZFileTable[] to retrieve a global handle to an
** LZFile struct.  The table of global handles is allocated statically from
** the DLL's data segment.  The LZFile struct's are allocated from global
** heap space and are moveable.  This scheme might also be implemented as a
** linked list of global handles.
**
** Now the resulting benefit from this scheme is that DOS file handles and
** LZFile struct identifiers can be differentiated, since DOS file handles
** are always < LZ_TABLE_BIAS, and LZFile struct identifiers are always
** >= LZ_TABLE_BIAS.  This dichotomy may be used in macros, like the sample
** ones provided in lzexpand.h, to select the appropriate function to call
** (e.g., LZSeek() or _llseek()) in order to avoid the overhead of an extra
** function call for uncompressed files.  LZSeek(), LZRead(), and LZClose()
** are, however, "smart" enough to figure out whether they are dealing with
** DOS file handles or table offsets, and take action appropriately.  As an
** extreme example, LZOpenFile(), LZSeek(), LZRead, and LZClose() can be used
** as replacements for OpenFile(), _llseek(), _lread(), and _lclose.  In this
** case, the program using the DLL functions could call them without ever
** caring whether the files it was reading were LZ compressed or not.
*/


/* WIN32 MODS

**  Since the above is a DOS only hack, I have to change the logic for
** for the 0-255 file handle deal'o.  The original code, tests greater than
** LZ_TABLE_BIAS for file structures.  What I will do, is convert file handles
** returned from OpenFile, to a range 0-255.  Once the test is complete, I'll
** use the file handle as an offset into a 255 element array, which will
** contain the WIN32 file handle.  So there will be an additional array
** fhWin32File[255], which will be allocated sequencially starting at 0.
** Unfortunately, this means everywhere the file handle is used, must be converted
*/

// Headers
///////////
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "..\libs\common.h"
#include "..\libs\buffers.h"
#include "..\libs\header.h"
#include "..\libs\lzcommon.h"

#include "lzpriv.h"
#include "wchar.h"

#if DEBUG
#include "stdio.h"
#endif

// Globals
///////////

// Semaphore for File Table allocation
RTL_CRITICAL_SECTION semTable;

// table of handles to LZFile structs
HANDLE rghLZFileTable[MAX_LZFILES] = {0};

// next free entry in rghLZFileTable[]
static INT iNextLZFileEntry = 0;

HFILE fhWin32File[MAX_LZFILES] = {0};

/*
** int  APIENTRY LZInit(int hWin32File);
**
** Sets up LZFile struct for a file that has already been opened by
** OpenFile().
**
** Arguments:  hWin32File - source DOS file handle
**
** Returns:    int - LZFile struct table offset or DOS file handle if
**                   successful.  One of the LZERROR_ codes if unsuccessful.
**
** Globals:    iNextLZFile entry advanced, or returned to beginning from end.
*/
INT  APIENTRY LZInit(INT hWin32File)
{
   HANDLE hLZFile;            // handle to new LZFile struct
   LZFile *lpLZ;          // pointer to new LZFile struct
   FH FHComp;                 // header info structure from input file
   BOOL bHdr;                 // holds GetHdr() return value
   INT iTableIndex,           // holds rghLZFileTable[] slot to be filled by
                              //    new LZFile struct handle
       iStartEntry;           // original value of iNextLZFileEntry

   LONG cblInSize = 0;
   INT nRet;

   // Did the read succeed?
   if ((bHdr = GetHdr((FH *)&FHComp, hWin32File, &cblInSize)) != TRUE
       && cblInSize >= (LONG)HEADER_LEN) {

      return(LZERROR_BADINHANDLE);
   }

   // Check for uncompressed input file.
   if (bHdr != TRUE || IsCompressed(& FHComp) != TRUE)
   {
      // This is an uncompressed file - rewind it.
      if (FSEEK(hWin32File, 0L, SEEK_SET) != 0L) {
         return(LZERROR_BADINHANDLE);
      }
      else {
         // And return DOS file handle.
        return(ConvertWin32FHToDos(hWin32File));
     }
   }

   // Check compression algorithm used.
   if (RecognizeCompAlg(FHComp.byteAlgorithm) != TRUE) {
      return(LZERROR_UNKNOWNALG);
   }

   // Find next free rghLZFileTable[] entry.  N.b., we depend upon LZClose()
   // to free unused entries up.
   RtlEnterCriticalSection(&semTable);

   iStartEntry = iNextLZFileEntry;

   while (rghLZFileTable[iNextLZFileEntry] != NULL)
   {
      if (++iNextLZFileEntry >= MAX_LZFILES)
         // Return to beginning of table.
         iNextLZFileEntry = 0;

      if (iNextLZFileEntry == iStartEntry) {
         // We've gone full circle through rghLZFileTable[].
         // It's full, so bail out.
         nRet = LZERROR_GLOBALLOC;
         goto LZInitExit;
      }
   }

   // Keep track of the rghLZFileTable() slot to be filled by a handle to the
   // new LZFile struct.
   iTableIndex = iNextLZFileEntry;

   // Allocate global storage for the new LZFile struct, initializing all
   // fields to 0.

   hLZFile = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)sizeof(LZFile));
   if (!hLZFile) {

      nRet = LZERROR_GLOBALLOC;
      goto LZInitExit;
   }

   // Lock that baby up.
   if ((lpLZ = (LZFile *)GlobalLock(hLZFile)) == NULL)
   {
      GlobalFree(hLZFile);

      nRet =LZERROR_GLOBLOCK;
      goto LZInitExit;
   }

   // Initialize the new LZFile struct's general information fields.
   lpLZ->dosh = hWin32File;
   lpLZ->byteAlgorithm = FHComp.byteAlgorithm;
   lpLZ->wFlags = 0;
   lpLZ->cbulUncompSize = FHComp.cbulUncompSize;
   lpLZ->cbulCompSize = FHComp.cbulCompSize;
   lpLZ->lCurSeekPos = 0L;

   // LZRead/LZSeeks expansion data is kept on a per file basis
   lpLZ->pLZI = NULL;

   // Enter new handle in table of handles.
   rghLZFileTable[iTableIndex] = hLZFile;

   /* WIN32 NOTE, dont need convert below, as forces a non file handle
    * to the API.
    */

   GlobalUnlock(hLZFile);

   // Advance to next free entry.
   if (++iNextLZFileEntry >= MAX_LZFILES)
      iNextLZFileEntry = 0;

   nRet = LZ_TABLE_BIAS + iTableIndex;

LZInitExit:

   RtlLeaveCriticalSection(&semTable);

   // Return rghLZFileTable[] offset of the new LZFile struct's handle's
   // entry + the table bias.
   return(nRet);
}


/*
** int  APIENTRY GetExpandedNameA(LPSTR lpszSource, LPSTR lpszBuffer);
**
** Looks in the header of a compressed file to find its original expanded
** name.
**
** Arguments:  lpszSource - name of input file
**             lpszBuffer - pointer to a buffer that will be filled in with
**                          the expanded name of the compressed source file
**
** Returns:    int - TRUE if successful.  One of the LZERROR_ codes if
**                   unsuccessful.
**
** Globals:    none
*/
INT  APIENTRY GetExpandedNameA(LPSTR lpszSource, LPSTR lpszBuffer)
{
   INT doshSource,         // source DOS file handle
       bHdr;               // holds GetHdr() return value
   FH FHComp;              // header info structure from input file
   OFSTRUCT ofOpenBuf;     // source struct for OpenFile() call
   LONG cblInSize = 0;

   // Try to open the source file.
   if ((doshSource = (HFILE)MOpenFile(lpszSource, (LPOFSTRUCT)(& ofOpenBuf), OF_READ))
       == -1)
      return(LZERROR_BADVALUE);

   // Get the compressed file header.
   if ((bHdr = GetHdr((FH *)&FHComp, doshSource, &cblInSize)) != TRUE
       && cblInSize >= (LONG)HEADER_LEN)
      return(LZERROR_BADVALUE);

   // Close source file.
   FCLOSE(doshSource);

   // Return expanded name same as source name for uncompressed files.
   STRCPY(lpszBuffer, lpszSource);

   // Check for compressed input file.
   if (bHdr == TRUE && IsCompressed(& FHComp) == TRUE)
      MakeExpandedName(lpszBuffer, FHComp.byteExtensionChar);

   return(TRUE);
}

/*
** int  APIENTRY GetExpandedNameW(LPSTR lpszSource, LPSTR lpszBuffer);
**
** Wide Character version of GetExpandedName.  Converts the filename to
** the ANSI Character set and calls the ANSI version.
**
*/
INT  APIENTRY GetExpandedNameW(LPWSTR lpszSource, LPWSTR lpszBuffer)
{
    UNICODE_STRING TempW;
    ANSI_STRING TempA;
    NTSTATUS Status;
    NTSTATUS StatusR;
    CHAR szBuffer[MAX_PATH];


    TempW.Buffer = lpszSource;
    TempW.Length = wcslen(lpszSource)*sizeof(WCHAR);
    TempW.MaximumLength = TempW.Length + sizeof(WCHAR);

    TempA.Buffer = szBuffer;
    TempA.MaximumLength = MAX_PATH;
    StatusR = RtlUnicodeStringToAnsiString(&TempA, &TempW, FALSE);
    if (!NT_SUCCESS(StatusR))
        return LZERROR_GLOBALLOC;

    Status = GetExpandedNameA(szBuffer, (LPSTR)lpszBuffer);

    if (Status != -1) {
        strcpy(szBuffer, (LPSTR)lpszBuffer);
        TempA.Length = strlen(szBuffer);
        TempA.MaximumLength = TempA.Length+sizeof(CHAR);

        TempW.Buffer = lpszBuffer;
        TempW.MaximumLength = MAX_PATH;
        StatusR = RtlAnsiStringToUnicodeString(&TempW, &TempA, FALSE);
        if (!NT_SUCCESS(StatusR))
            return LZERROR_GLOBALLOC;
    }

    return Status;
}


//
// INT  LZCreateFileW(LPCWSTR lpFileName, DWORD fdwAccess)
//
// Opens a file (using CreateFile) and sets up an LZFile struct for
// expanding it.
//
// Arguments:  lpFileName  - name of input file
//             fdwAccess   - CreateFile access type - (e.g. GENERIC_READ)
//             fdwShareMode - Share mode  - (e.g. FILE_SHARE_READ)
//             fdwCreate - Action to be taken - (e.g. OPEN_EXISTING)
//
// Returns:    INT - LZFile struct table offset or WIN32 file HANDLE if
//             successful.  One of the LZERROR_ codes if unsuccessful.
//
INT
LZCreateFileW(
    LPWSTR lpFileName,
    DWORD fdwAccess,
    DWORD fdwShareMode,
    DWORD fdwCreate,
    LPWSTR lpCompressedName)
{
    HANDLE hWin32;           // WIN32 file HANDLE returned from CreateFileW()
    INT lzh;                 // LZ File struct ID returned from LZInit()
    static WCHAR pszCompName[MAX_PATH];  // buffer for compressed name

    lstrcpyW((LPWSTR)pszCompName, lpFileName);

    // Just for Vlad, only try to open the compressed version of the original
    // file name if we can't find the original file.  All other errors get
    // returned immediately.

    hWin32 = CreateFileW(pszCompName, fdwAccess, fdwShareMode, NULL, fdwCreate,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hWin32 == INVALID_HANDLE_VALUE) {
        DWORD dwErr = GetLastError();

        if (dwErr == ERROR_FILE_NOT_FOUND) {

            // Let's try to open the file of the corresponding compressed name.
            MakeCompressedNameW((LPWSTR)pszCompName);

            hWin32 = CreateFileW(pszCompName, fdwAccess, fdwShareMode,
                NULL, fdwCreate, FILE_ATTRIBUTE_NORMAL, NULL);
        }
    }

     // Error opening file?
     if (hWin32 == INVALID_HANDLE_VALUE) {
        return(LZERROR_BADINHANDLE);
     }

    // Don't call LZinit() on files opened in other read only mode

    if (fdwCreate != OPEN_EXISTING) {
       lzh = ConvertWin32FHToDos((HFILE)((DWORD_PTR)hWin32));
       if (lzh == LZERROR_GLOBALLOC) {
          CloseHandle(hWin32);
       }

       return(lzh);
    }

    // File has been opened with read-only action - call LZInit() to detect
    // whether or not it's an LZ file, and to create structures for expansion
    // if it is an LZ file.
    lzh = LZInit((INT)((DWORD_PTR)hWin32));

    // Close DOS file handle if LZInit() is unsuccessful.
    if (lzh < 0)
       CloseHandle(hWin32);

    // Pass real file name to caller
    //
    // we believe the caller have enough buffer.
    //
    if( lpCompressedName != NULL )
        lstrcpyW(lpCompressedName,pszCompName);

    // Return LZ struct ID or LZERROR_ code.
    return(lzh);
}


/*
** int  APIENTRY LZOpenFileA(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuf,
**                           WORD wStyle);
**
** Opens a file and sets up an LZFile struct for expanding it.
**
** Arguments:  lpFileName  - name of input file
**             lpReOpenBuf - pointer to LPOFSTRUCT to be used by OpenFile()
**             wStyle      - OpenFile() action to take
**
** Returns:    int - LZFile struct table offset or DOS file handle if
**                   successful.  One of the LZERROR_ codes if unsuccessful.
**
** Globals:    none
*/
INT APIENTRY LZOpenFileA(LPSTR lpFileName, LPOFSTRUCT lpReOpenBuf, WORD wStyle)
{
   INT dosh,                     // DOS file handle returned from OpenFile()
       lzh;                      // LZ File struct ID returned from LZInit()
   CHAR pszCompName[MAX_PATH];   // buffer for compressed name

   STRCPY((LPSTR)pszCompName, lpFileName);

   // Just for Vlad, only try to open the compressed version of the original
   // file name if we can't find the original file.  All other errors get
   // returned immediately.

   if ((dosh = OpenFile(pszCompName, lpReOpenBuf, wStyle)) == -1 &&
       lpReOpenBuf->nErrCode == DEE_FILENOTFOUND)
   {
      // Let's try to open the file of the corresponding compressed name.
      MakeCompressedName(pszCompName);

      dosh = (HFILE) MOpenFile((LPSTR)pszCompName, lpReOpenBuf, wStyle);
   }

   // Error opening file?
   if (dosh == -1)
      return(LZERROR_BADINHANDLE);

   // Don't call LZinit() on files opened in other than O_RDONLY mode.
   // Ignore the SHARE bits.
   if ((wStyle & STYLE_MASK) != OF_READ) {
      lzh = ConvertWin32FHToDos(dosh);
      if (lzh == LZERROR_GLOBALLOC) {
         FCLOSE(dosh);
      }
      return(lzh);
   }

   // File has been opened with OF_READ style - call LZInit() to detect
   // whether or not it's an LZ file, and to create structures for expansion
   // if it is an LZ file.
   lzh = LZInit(dosh);

   // Close DOS file handle if LZInit() is unsuccessful.
   if (lzh < 0)
      FCLOSE(dosh);

   // Return LZ struct ID or LZERROR_ code.
   return(lzh);
}

/*
** int  APIENTRY LZOpenFileW(LPCWSTR lpFileName, LPOFSTRUCT lpReOpenBuf,
**                           WORD wStyle);
**
** Wide Character version of LZOpenFile.  Converts the filename to
** the ANSI Character set and calls the ANSI version.
**
*/
INT APIENTRY LZOpenFileW(LPWSTR lpFileName, LPOFSTRUCT lpReOpenBuf, WORD wStyle)
{
    UNICODE_STRING FileName;
    ANSI_STRING AnsiString;
    NTSTATUS StatusR;
    CHAR szFileName[MAX_PATH];


    FileName.Buffer = lpFileName;
    FileName.Length = wcslen(lpFileName)*sizeof(WCHAR);
    FileName.MaximumLength = FileName.Length + sizeof(WCHAR);

    AnsiString.Buffer = szFileName;
    AnsiString.MaximumLength = MAX_PATH;
    StatusR = RtlUnicodeStringToAnsiString(&AnsiString, &FileName, FALSE);
    if (!NT_SUCCESS(StatusR))
        return LZERROR_GLOBALLOC;

    return(LZOpenFileA(szFileName, lpReOpenBuf, wStyle));
}



/*
** LONG  APIENTRY LZSeek(int oLZFile, long lSeekTo, int nMode);
**
** Works like _llseek(), but in the expanded image of a compressed file,
** without expanding the compressed file to disk.
**
** Arguments:  oLZFile - source LZFile struct identifier or DOS file handle
**             lSeekTo - number of bytes past nMode target to seek
**             nMode   - seek mode as in _llseek()
**
** Returns:    LONG - Offset of the seek target if successful.  One of the
**                    LZERROR_ codes if unsuccessful.
**
** Globals:    none
*/
LONG APIENTRY
LZSeek(
   INT oLZFile,
   LONG lSeekTo,
   INT nMode)
{
   HANDLE hSourceStruct;      // handle to LZFile struct
   LZFile *lpLZ;          // pointer to LZFile struct
   LONG lExpSeekTarget;       // target seek offset

   // Check input LZFile struct indentifier / DOS file handle.
   if (oLZFile < 0 || oLZFile >= LZ_TABLE_BIAS + MAX_LZFILES)
      return(LZERROR_BADINHANDLE);

   // We were passed a regular DOS file handle, so just do an _llseek() on it.
   if (oLZFile < LZ_TABLE_BIAS)
      return(FSEEK(ConvertDosFHToWin32(oLZFile), lSeekTo, nMode));

   // We're dealing with a compressed file.  Get the associated LZFile struct.
   hSourceStruct = rghLZFileTable[oLZFile - LZ_TABLE_BIAS];

   if ((lpLZ = (LZFile *)GlobalLock(hSourceStruct)) == NULL)
      return(LZERROR_GLOBLOCK);

   // Figure out what our seek target is.
   if (nMode == SEEK_SET)
      lExpSeekTarget = 0L;
   else if (nMode == SEEK_CUR)
      lExpSeekTarget = lpLZ->lCurSeekPos;
   else if (nMode == SEEK_END)
      lExpSeekTarget = (LONG)lpLZ->cbulUncompSize;
   else
   {
      GlobalUnlock(hSourceStruct);
      return(LZERROR_BADVALUE);
   }

   // Add bias.
   lExpSeekTarget += lSeekTo;

   // Make sure the desired expanded file position is in the expanded file
   // bounds.  It's only an error to seek before the beginning of the file;
   // it's not an error to seek after the end of the file, as in _llseek().
   if (lExpSeekTarget < 0L)
   {
      GlobalUnlock(hSourceStruct);
      return(LZERROR_BADVALUE);
   }

   // Seek target is ok.  Set the file pointer for the expanded file image.
   lpLZ->lCurSeekPos = lExpSeekTarget;

   GlobalUnlock(hSourceStruct);

   // Return the offset of the seek target.
   return(lExpSeekTarget);
}


/*
** int  APIENTRY LZRead(int oLZFile, LPSTR lpBuf, int nCount);
**
** Works like _lread(), but in the expanded image of a compressed file,
** without expanding the compressed file to disk.
**
** Arguments:  oLZFile - source LZFile struct identifier or DOS file handle
**             lpBuf   - pointer to destination buffer for bytes read
**             nCount  - number of bytes to read
**
** Returns:    int - Number of bytes copied to destination buffer if
**                   successful.  One of the LZERROR_ codes if unsuccessful.
**
** Globals:    none
*/
INT  APIENTRY LZRead(INT oLZFile, LPSTR lpBuf, INT nCount)
{
   INT f;
   HANDLE hSourceStruct;      // handle to LZFile struct
   LZFile *lpLZ;          // pointer to LZFile struct

   INT cbWritten = 0,         // total number of bytes copied to
                              //    destination buffer
       cbCopied,              // number of bytes copied to destination
                              // buffer during each read iteration
       iCurReadPos,           // current read offset in expanded buffer
       nNumExpBufBytes;       // number of bytes in expanded data buffer
   LONG lNextDecodeTarget,    // expanded file image read target for decoding
        lStartCopyOffset,     // expanded file buffer offset where we should
                              //    start copying to destination buffer (cast
                              //    to iCurReadPos when this start position
                              //    is actually in the buffer)
        lNextExpEndOffset;    // expanded file offset of the start of the
                              //    next desired block of expanded data
   BOOL bRestartDecoding;     // flag indicating whether or not decoding
                              //    needs to be restarted, set to TRUE when
                              //    the current seek position is smaller than
                              //    the offset of the beginning of the
                              //    expanded file buffer
   BYTE *lpbyteBuf;           // byte pointer version of lpBuf

   LONG lExpBufStart;
   LONG lExpBufEnd;

   PLZINFO pLZI;

   // Check input LZFile struct indentifier / DOS file handle.
   if (oLZFile < 0 || oLZFile >= LZ_TABLE_BIAS + MAX_LZFILES)
      return(LZERROR_BADINHANDLE);

   // Can't read a negative number of bytes.
   if (nCount < 0)
      return(LZERROR_BADVALUE);

   // We were passed a regular DOS file handle, so just do an _lread() on it.
   if (oLZFile < LZ_TABLE_BIAS)
      return(FREAD(ConvertDosFHToWin32(oLZFile), lpBuf, nCount));

   // We're dealing with a compressed file.  Get the associated LZFile struct.
   hSourceStruct = rghLZFileTable[oLZFile - LZ_TABLE_BIAS];

   if ((lpLZ = (LZFile *)GlobalLock(hSourceStruct)) == NULL)
      return(LZERROR_GLOBLOCK);

   if (!(pLZI = lpLZ->pLZI)) {
      // Initialize buffers
      lpLZ->pLZI = InitGlobalBuffers(EXP_BUF_LEN, MAX_RING_BUF_LEN, IN_BUF_LEN + 1);

      if (!(pLZI = lpLZ->pLZI)) {
         return(LZERROR_GLOBALLOC);
      }

      ResetBuffers();
   }

   lExpBufStart = pLZI->cblOutSize - (LONG)(pLZI->pbyteOutBuf - pLZI->rgbyteOutBuf);
   lExpBufEnd = (LONG)(pLZI->pbyteOutBufEnd - pLZI->rgbyteOutBuf);

   // Do we need to restart decoding?
   if (! (lpLZ->wFlags & LZF_INITIALIZED))
   {
      lpLZ->wFlags |= LZF_INITIALIZED;
      bRestartDecoding = TRUE;
   }
   else if (lpLZ->lCurSeekPos < lExpBufStart)
      bRestartDecoding = TRUE;
   else
      bRestartDecoding = FALSE;

   // Set up byte pointer version of lpBuf.
   lpbyteBuf = lpBuf;

   // Copy bytes until buffer is filled or EOF in expanded file image is
   // reached.
   while (nCount > 0 && lpLZ->lCurSeekPos < (LONG)lpLZ->cbulUncompSize)
   {
      /* How many expanded data bytes are in the expanded data buffer?
       * (pbyteOutBuf points AFTER the last valid byte in rgbyteOutBuf[].)
       */
      nNumExpBufBytes = (INT)(pLZI->pbyteOutBuf - pLZI->rgbyteOutBuf);

      /* Is the start of the desired expanded data currently in the bounds of
       * the expanded data buffer?
       */
      lStartCopyOffset = lpLZ->lCurSeekPos - lExpBufStart;
      if (lStartCopyOffset < lExpBufEnd)
         /* It's ok to set iCurReadPos to a negative value here, since we
          * will only use expanded data from the expanded data buffer if
          * iCurReadPos is non-negative.
          */
         iCurReadPos = (INT)lStartCopyOffset;
      else
         iCurReadPos = -1;

      /* Now, if iCurReadPos > 0, some of the expanded data in the expanded
       * data buffer should be copied to the caller's buffer.  If not, we
       * need to continue expanding or restart expanding.
       */
      if (iCurReadPos >= 0)
      {
         /* Copy available expanded data from expanded data buffer. */
         for (cbCopied = 0;
              iCurReadPos < nNumExpBufBytes && nCount > 0;
              cbCopied++, nCount--)
            *lpbyteBuf++ = pLZI->rgbyteOutBuf[iCurReadPos++];

         // Update expanded file pointer.
         lpLZ->lCurSeekPos += (LONG)cbCopied;

         // Keep track of bytes written to buffer.
         cbWritten += cbCopied;
      }

      /* Expand more data, restarting the expansion process first if
       * necessary.
       */
      if (nCount > 0 && lpLZ->lCurSeekPos < (LONG)lpLZ->cbulUncompSize)
      {
         /* If we get here, we've copied all the available expanded data out
          * of rgbyteOutBuf[], through pbyteOutBuf, and we need to expand
          * more data.
          */

         /* Where is the end of the next desired expanded data block? */
         if (bRestartDecoding)
         {
            /* Seek back to start of target data, allowing for buffer
             * overflow.
             */
            lNextExpEndOffset = lpLZ->lCurSeekPos - MAX_OVERRUN;

            /* Don't try to read before offset 0! */
            if (lNextExpEndOffset < 0)
               lNextExpEndOffset = 0;
         }
         else
            /* Continue decoding. */
            lNextExpEndOffset = lExpBufStart
                                + (LONG)nNumExpBufBytes
                                + lExpBufEnd
                                - MAX_OVERRUN;

         /* How much farther should we expand?  The target decoding offset
          * should be the smallest expanded file offset of the following:
          *
          *    1) the last byte in the largest expanded data block that will
          *       safely fit in the expanded data buffer, while guaranteeing
          *       that the start of this block is also in the expanded data
          *       buffer
          *    2) the last requested expanded data byte
          *    3) the last byte in the expanded file
          */
         lNextDecodeTarget = MIN(lNextExpEndOffset,
                                 MIN(lpLZ->lCurSeekPos + (LONG)nCount,
                                     (LONG)lpLZ->cbulUncompSize - 1L));

         // Reset expanded data buffer to empty state.
         pLZI->pbyteOutBuf = pLZI->rgbyteOutBuf;

         // Refill rgbyteOutBuf[] with expanded data.
         switch (lpLZ->byteAlgorithm)
         {
            case ALG_FIRST:
               f = LZDecode(lpLZ->dosh, NO_DOSH, lNextDecodeTarget,
                  bRestartDecoding, TRUE, pLZI);
               break;

            default:
               f = LZERROR_UNKNOWNALG;
               break;
         }

         // Did the decoding go ok?
         if (f != TRUE)
         {
            // Uh oh.  Something went wrong.
            GlobalUnlock(hSourceStruct);
            return(f);
         }

         /* Now how many expanded data bytes are in the expanded data buffer?
          * (pbyteOutBuf points AFTER the last valid byte in rgbyteOutBuf[].)
          */
#if DEBUG
         printf("pbyteOutBuf: 0x%x,  rgbyteOutBuf: 0x%x \n", pLZI->pbyteOutBuf, pLZI->rgbyteOutBuf);
#endif

         nNumExpBufBytes = (INT)(pLZI->pbyteOutBuf - pLZI->rgbyteOutBuf);

         /* Check to make sure we actually read some bytes. */
         if (nNumExpBufBytes <= 0)
         {
            GlobalUnlock(hSourceStruct);
            return(LZERROR_READ);
         }

         /* What is the offset of the start of the expanded data buffer in
          * the expanded file image?
          */
         lExpBufStart = pLZI->cblOutSize - (LONG)nNumExpBufBytes;

         /* Did LZDecode() satisfy the read request, or did the compressed
          * file end prematurely?
          */
         if (pLZI->cblOutSize < lNextDecodeTarget)
         {
            /* Oh oh.  lNextDecodeTarget cannot exceed the expanded file
             * bounds, so the compressed file must have ended prematurely.
             */
            GlobalUnlock(hSourceStruct);
            return(LZERROR_READ);
         }

         // Reset flag so we continue decoding where we left off.
         bRestartDecoding = FALSE;
      }
   }

   GlobalUnlock(hSourceStruct);

   // Return number of bytes copied to destination buffer.
   return(cbWritten);
}

//
// VOID  LZCloseFile(INT oLZFile);
//
// Close a file and free the associated LZFile struct.
//
// Arguments:  oLZFile - source LZFile struct identifier or WIN32 file handle
//
// Returns:    VOID
//
// Globals:    rghLZFileTable[] entry cleared.
//

VOID
LZCloseFile(
    INT oLZFile)
{
   HANDLE hSourceStruct;      // handle to LZFile struct
   LZFile *lpLZ;          // pointer to LZFile struct

   // Check input LZFile struct indentifier / DOS file handle.
   if (oLZFile < 0 || oLZFile >= LZ_TABLE_BIAS + MAX_LZFILES)
      return;

   // We were passed a regular DOS file handle, so just close it.
   if (oLZFile < LZ_TABLE_BIAS) {
      CloseHandle((HANDLE)IntToPtr(ConvertDosFHToWin32(oLZFile)));
      // also need to clean out the file array entry
      fhWin32File[oLZFile] = 0;

      return;
   }

   // We're dealing with a compressed file.  Get the associated LZFile struct.
   hSourceStruct = rghLZFileTable[oLZFile - LZ_TABLE_BIAS];

   // Clear rghLZFIleTable[] entry.
   rghLZFileTable[oLZFile - LZ_TABLE_BIAS] = NULL;

   // Close the file and free the associated LZFile struct.
   if ((lpLZ = (LZFile *)GlobalLock(hSourceStruct)) != NULL)
   {
      CloseHandle((HANDLE)IntToPtr(lpLZ->dosh));

      if (lpLZ->pLZI) {
         FreeGlobalBuffers(lpLZ->pLZI);
      }

      GlobalUnlock(hSourceStruct);

      GlobalFree(hSourceStruct);

   }

   return;
}


/*
** VOID  APIENTRY LZClose(int oLZFile);
**
** Close a file and free the associated LZFile struct.
**
** Arguments:  oLZFile - source LZFile struct identifier or DOS file handle
**
** Returns:    VOID
**
** Globals:    rghLZFileTable[] entry cleared.
*/
VOID  APIENTRY LZClose(INT oLZFile)
{
   HANDLE hSourceStruct;      // handle to LZFile struct
   LZFile *lpLZ;          // pointer to LZFile struct

   // Check input LZFile struct indentifier / DOS file handle.
   if (oLZFile < 0 || oLZFile >= LZ_TABLE_BIAS + MAX_LZFILES)
      return;

   // We were passed a regular DOS file handle, so just close it.
   if (oLZFile < LZ_TABLE_BIAS)
   {
      FCLOSE(ConvertDosFHToWin32(oLZFile));
      /* also need to clean out the file array entry */
      fhWin32File[oLZFile] = 0;

      return;
   }

   // We're dealing with a compressed file.  Get the associated LZFile struct.
   hSourceStruct = rghLZFileTable[oLZFile - LZ_TABLE_BIAS];

   // Clear rghLZFIleTable[] entry.
   rghLZFileTable[oLZFile - LZ_TABLE_BIAS] = NULL;

   // Close the file and free the associated LZFile struct.
   if ((lpLZ = (LZFile *)GlobalLock(hSourceStruct)) != NULL)
   {
      FCLOSE(lpLZ->dosh);

      if (lpLZ->pLZI) {
         FreeGlobalBuffers(lpLZ->pLZI);
      }

      GlobalUnlock(hSourceStruct);

      GlobalFree(hSourceStruct);

   }

   return;
}


/* WIN32 MODS   */

INT ConvertWin32FHToDos(HFILE DoshSource)
{

    INT x;

    /* here we are given an NT file handle, need save this into
     * fhWin32File[], test for overflow, also need see
     * if there is a free slot in the array */

    /* If error, return greater than MAX_LZFILES */

    RtlEnterCriticalSection(&semTable);

    /* walk array, looking for a free slot (free slot = 0) */
    for(x = 0; x < MAX_LZFILES; x++){
        if(fhWin32File[x] == 0)
            break;
    }
    if(x < MAX_LZFILES){
       /* no overflow, save into array*/
       fhWin32File[x] = DoshSource;
    }
    else{
       x = LZERROR_GLOBALLOC;
    }

    RtlLeaveCriticalSection(&semTable);

    return(x);

}


HFILE ConvertDosFHToWin32(INT DoshSource)
{

    /* here, we are given the pseudo Dos File Handle, need convert to
     * real file handle, for use by API.
     */

    if (DoshSource >= MAX_LZFILES || DoshSource < 0 ||
        fhWin32File[DoshSource] == 0) {
        return (HFILE)DoshSource;
    }
    else{
        return(fhWin32File[DoshSource]);
    }

}


/****************************************************************************
   FUNCTION: LibMain(HANDLE, ULONG, LPVOID)

   PURPOSE:
             The LibMain function should perform additional initialization
             tasks required by the DLL.  In this example, no initialization
             tasks are required.  LibMain should return a value of 1 if
             the initialization is successful.

*******************************************************************************/
BOOLEAN APIENTRY
LibMain(
    HANDLE hInst,
    DWORD dwReason,
    LPVOID lpReserved)
{
   switch (dwReason) {
   case DLL_PROCESS_ATTACH:
       if (!NT_SUCCESS(RtlInitializeCriticalSection(&semTable))) {
          return(FALSE);
       }

       DisableThreadLibraryCalls(hInst);

       break;

   case DLL_PROCESS_DETACH:
       if (!lpReserved) {
           RtlDeleteCriticalSection(&semTable);
       }
       break;

   default:
       break;
   }

   return (TRUE);

   UNREFERENCED_PARAMETER(hInst);
   UNREFERENCED_PARAMETER(lpReserved);
}

