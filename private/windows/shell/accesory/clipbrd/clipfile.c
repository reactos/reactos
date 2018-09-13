/*
 * CLIPFILE.C - Windows Clipboard File I/O Routines
 *
 *  Copyright 1985-92, Microsoft Corporation
 */

/* NOTE:
 *   When saving the contents of the clipboard we SetClipboardData(fmt, NULL)
 *   to free up the memory associated with each clipboard format.  Then
 *   after we are done saving we take over as the clipboard owner.  This
 *   causes OWNERDRAW formats to be lost in the save process.
 */


/*
*
*    Touched by    :    Anas Jarrah
*    On Date        :    May 11/1992.
*    Revision remarks by Anas Jarrah ext #15201
*    This file has been changed to comply with the Unicode standard
*    Following is a quick overview of what I have done.
*
*    Was            Changed it into        Remark
*    ===            ===============        ======
*    CHAR              TCHAR             if it refers to a text elements
*    LPCHAR & LPSTR    LPTSTR            if it refers to text.
*    LPCHAR & LPSTR    LPBYTE            if it does not refer to text
*    "..."             TEXT("...")       compile time macro resolves it.
*    '...'             TEXT('...')       same
*    strlen            CharStrLen        compile time macro resolves it.
*    strcpy            CharStrCpy        compile time macro resolves it.
*    strcmp            CharStrCmp        compile time macro resolves it.
*    strcat            CharStrCat        compile time macro resolves it.
*    LoadResource      LoadResource(A/W) NT compiles resource strings into
*                                         Unicode binaries
*    MOpenFile()       MapOpenFile/MOpenFile   Depending on whether Unicode is defined or not.
*                                              This is a temporary cluge, That has to be taken care of.
*
*
*/

#include "clipbrd.h"
#include "dib.h"

#define ATTRDIRLIST 0xC010

// Windows 3.1 programs were packed on byte boundaries.
#pragma pack(1)
// Windows 3.1 BITMAP struct - used to save Win 3.1 .CLP files
typedef struct {
   WORD bmType;
   WORD bmWidth;
   WORD bmHeight;
   WORD bmWidthBytes;
   BYTE bmPlanes;
   BYTE bmBitsPixel;
   LPVOID bmBits;
   } WIN31BITMAP;
#pragma pack()

/* EXTERN data */
extern BOOL fAnythingToRender;

/* FORWARD procs */
HBITMAP PASCAL BitmapToBitmap(HBITMAP, WORD, WORD);

TCHAR       szFileSpecifier[] = TEXT("*.CLP");
TCHAR       szFileName[MAX_PATH];
BOOL        fNTReadFileFormat;
#ifdef JAPAN
extern TCHAR szCaptionName[];
#endif

/* ofSaveStruct is required; Otherwise, the following bug will occur
 *   When the contents of the clipboard are loaded from a file, the
 *   ofStruct is used to open the file and the data handles assigned to
 *   the clipboard are NULL because of DELAYED RENDERING; Delayed
 *   rendering will be done by reading from the file (ofStruct). But,
 *   now, if the contents are to be saved into another file, it should
 *   use the same ofStruct, because data has to be obtained from this
 *   file using delayed rendering; So, a temporary ofSaveStruct is used
 *   to hold the saved file's info; Once the save is successful, the
 *   contents of ofSaveStruct are copied onto ofStruct
 */
/* the ofStruct and ofSaveStruct are replaced by szFileName and
 * szSaveFileName strings.
 */
TCHAR       szSaveFileName[MAX_PATH];
BOOL        fNTSaveFileFormat;


/*
 *  IsWriteable()
 *
 * Test if a clipboard format is writeable(i.e. if it makes sense to write it)
 * OWNERDRAW and others can't be written because we (CLIPBRD) will become the
 * owner when the files are reopened.
 */

BOOL NEAR PASCAL IsWriteable(UINT Format)

{
  /* Are the PRIVATEFIRST and PRIVATELAST things right? */
  if ((Format >= CF_PRIVATEFIRST && Format <= CF_PRIVATELAST) || Format == CF_OWNERDISPLAY)
      return(FALSE);
  if (!fNTSaveFileFormat &&
      (Format == CF_UNICODETEXT || Format == CF_ENHMETAFILE || Format == CF_DSPENHMETAFILE))
      return(FALSE);
  return(TRUE);
}


/*
 *  ReadClipboardFromFile()
 *
 * Read in a clipboard file and register all the formats in delayed mode.
 * to render things for real reopen the file specified by szFileName (was ofStruct).
 *
 * NOTE:
 *    This makes us the clipboard owner.
 *
 * Bug 14564:  Changed return value to a short integer noting why the
 * reading failed.
 * Return Value:  0  Success
 *                1  Improper format
 *                2  OpenClipboard failed
 */
#define READFILE_SUCCESS         0
#define READFILE_IMPROPERFORMAT  1
#define READFILE_OPENCLIPBRDFAIL 2

short NEAR PASCAL ReadClipboardFromFile(HWND hwnd,INT fh)

{
register WORD i;
FILEHEADER    FileHeader;
FORMATHEADER  FormatHeader;

    /* Read the File Header */
    _lread(fh, (LPBYTE)&FileHeader, sizeof(FILEHEADER));

    /* Sanity check, make sure this is one of ours. */
#ifndef JAPAN
    if (FileHeader.magic == CLP_NT_ID)
        fNTReadFileFormat = TRUE;
    else if (FileHeader.magic == CLP_ID)
        fNTReadFileFormat = FALSE;
    else
        return(READFILE_IMPROPERFORMAT);

    if (FileHeader.FormatCount > 100)
        return(READFILE_IMPROPERFORMAT);
#else
    if (FileHeader.magic == CLP_NT_ID)
        fNTReadFileFormat = TRUE;
    else if (FileHeader.magic == CLP_ID)
        fNTReadFileFormat = FALSE;
    else
        goto improperformat;

    if (FileHeader.FormatCount > 100)
    {

improperformat:
    TCHAR szOutMessage[BUFFERLEN];
    TCHAR szCapBuffer[SMALLBUFFERLEN];

    LoadStringW(hInst, IDS_NAME, szCapBuffer, SMALLBUFFERLEN);
    LoadStringW(hInst, IDS_ENOTVALIDFILE, szOutMessage, BUFFERLEN);
    MessageBox(hwnd, szOutMessage,szCapBuffer, MB_OK | MB_ICONEXCLAMATION);
        return(READFILE_IMPROPERFORMAT);
    }
#endif

    /* We become the clipboard owner here! */
    if (!OpenClipboard(hwnd))
        return(READFILE_OPENCLIPBRDFAIL);

    EmptyClipboard();

    for (i=0; i < FileHeader.FormatCount; i++)
        {

        if (fNTReadFileFormat)
            _lread(fh, (LPBYTE)&(FormatHeader.FormatID), sizeof(FormatHeader.FormatID));
        else {
            FormatHeader.FormatID = 0;  /* initialize the high WORD */
            _lread(fh, (LPBYTE)&(FormatHeader.FormatID), sizeof(WORD));
        }
        _lread(fh, (LPBYTE)&(FormatHeader.DataLen), sizeof(FormatHeader.DataLen));
        _lread(fh, (LPBYTE)&(FormatHeader.DataOffset), sizeof(FormatHeader.DataOffset));
        _lread(fh, (LPBYTE)&(FormatHeader.Name), sizeof(FormatHeader.Name));

        if (PRIVATE_FORMAT(FormatHeader.FormatID))
            FormatHeader.FormatID = (UINT)RegisterClipboardFormat(FormatHeader.Name);

        /* Delayed Render. */
        SetClipboardData(FormatHeader.FormatID, NULL);
        }

    /* Now, clipbrd viewer has something to render */
    if (FileHeader.FormatCount > 0)
        fAnythingToRender = TRUE;

    CloseClipboard();
    return(READFILE_SUCCESS);
}


/*
 *  OpenClipboardFile()
 */

void NEAR PASCAL OpenClipboardFile(HWND hwnd)

{
INT     fh;

   lstrcpy(szFileName, TEXT(""));
   OFN.lpstrTitle = szOpenCaption;
   OFN.lpstrFile  = szFileName;
   /* Added OFN_FILEMUSTEXIST.  4 March 1991   clarkc   */
   /* Added OFN_HIDEREADONLY. Happy now, Patrick? :) 1 Oct 1992 a-mgates.*/
   OFN.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    /* All long pointers should be defined immediately before the call.
     * L.Raman - 2/12/91
     */
   OFN.lpstrDefExt       = (LPTSTR)szDefExt;
   OFN.lpstrFilter       = (LPTSTR)szFilterSpec;
   OFN.lpstrCustomFilter = (LPTSTR)szCustFilterSpec;

   fh = GetOpenFileName ((LPOPENFILENAME) &OFN);
   if (fh)
      {
      fh = (INT)CreateFile((LPCTSTR)szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

      if (fh > 0)
         {
         short nReadError;

         if (ClearClipboard(hwnd))
            {
            if (nReadError = ReadClipboardFromFile(hwnd, fh))
               {
               TCHAR szErr[MSGMAX];

               LoadString (hInst, IDS_READERR + nReadError, szErr, MSGMAX);
               MessageBox(hwnd,szErr,szAppName,MB_OK | MB_ICONEXCLAMATION);
               }
           }
        _lclose(fh);
        }
     }

  /* On GetOpenFileName failure CommDlgExtendedError will return a value.*/
  if(CommDlgExtendedError())
     {
     MemErrorMessage();
     }

}


/*
 *  WriteFormatBlock() -
 */
DWORD NEAR PASCAL WriteFormatBlock(INT fh,DWORD offset,DWORD DataOffset,
                                    DWORD DataLen,UINT Format,LPSTR szName)
{
FORMATHEADER    FormatHeader;
DWORD           dwBytesWritten = 0;

    FormatHeader.FormatID = Format;
    FormatHeader.DataLen = DataLen;
    FormatHeader.DataOffset = DataOffset;
    lstrcpy(FormatHeader.Name, szName);
    _llseek(fh, offset, 0);

    if (fNTSaveFileFormat)
        dwBytesWritten += _lwrite(fh, (LPSTR)&(FormatHeader.FormatID), sizeof(FormatHeader.FormatID));
    else
        dwBytesWritten += _lwrite(fh, (LPSTR)&(FormatHeader.FormatID), sizeof(WORD));
    dwBytesWritten += _lwrite(fh, (LPSTR)&(FormatHeader.DataLen), sizeof(FormatHeader.DataLen));
    dwBytesWritten += _lwrite(fh, (LPSTR)&(FormatHeader.DataOffset), sizeof(FormatHeader.DataOffset));
    dwBytesWritten += _lwrite(fh, (LPSTR)&(FormatHeader.Name), sizeof(FormatHeader.Name));
    return(dwBytesWritten);
}


/*
 *  lread() - Good ol' _lread that will handle >64k
 */
#define MAXREAD   ((LONG)  (60L * 1024L))

DWORD APIENTRY lread(INT hFile,void FAR *pBuffer,DWORD dwBytes)
{
DWORD   dwByteCount = dwBytes;
#ifdef WIN16
BYTE    huge *hpBuffer = pBuffer;

    while (dwByteCount > MAXREAD)
        {
    if (_lread(hFile, hpBuffer, (WORD)MAXREAD) != MAXREAD)
            return(0);

        dwByteCount -= MAXREAD;
        hpBuffer += MAXREAD;
        }
#else
BYTE    FAR  *hpBuffer = pBuffer;
#endif
    if ((DWORD)_lread(hFile, hpBuffer, dwByteCount) != dwByteCount)
        return(0);

    return(dwBytes);
}


/*
 *  lwrite() -  Good ol' _lwrite that will handle >64k
 */
DWORD APIENTRY lwrite(INT hFile,void FAR *pBuffer,DWORD dwBytes)

{
DWORD   dwByteCount=dwBytes;
#ifdef WIN16
BYTE    huge *hpBuffer = pBuffer;

    while (dwByteCount > MAXREAD)
        {
    if (_lwrite(hFile, (LPSTR)hpBuffer, (WORD)MAXREAD) != MAXREAD)
            return(0);
        dwByteCount -= MAXREAD;
        hpBuffer += MAXREAD;
        }

#else
BYTE    FAR  *hpBuffer = pBuffer;
#endif

    if ((DWORD)_lwrite(hFile, (LPSTR)hpBuffer, dwByteCount) != dwByteCount)
        return(0);

    return(dwBytes);
}


/*
 *  WriteDataBlock() -
 *
 * Returns:
 *    # of bytes written to the output file
 *
 * NOTE: Write saves the name of a temp file in the clipboard for it's
 * own internal clipboard format.  This file goes aways when Write
 * (or windows?) shuts down.  Thus saving Write clipboards won't work
 * (should we special case hack this?)
 *
 */
DWORD NEAR PASCAL WriteDataBlock(register INT hFile,LONG offset,UINT Format)
{
WORD    wPalEntries;
LPBYTE   lpData;
DWORD   dwSize;
BITMAP  bitmap;
HANDLE  hMF;
HANDLE  hBitmap;
HANDLE  hLogPalette;
register HANDLE hData;
LPLOGPALETTE    lpLogPalette;
LPMETAFILEPICT  lpMFP;
HENHMETAFILE    hEMF;
WIN31BITMAP     bmWin31;

    if (!(hData = GetClipboardData(Format)))
        return(0);

    if(_llseek(hFile, offset, 0) != (int)offset)
        return(0);

    /* We have to special case a few common formats but most things
     * get handled in the default case.
     */
    switch (Format)
        {
        case CF_ENHMETAFILE:
            hEMF = hData;
            dwSize = (DWORD) GetEnhMetaFileBits(hEMF, 0, NULL); /* Get data size */
            if (!(hData = GlobalAlloc(GHND, dwSize)))   /* allocate mem for EMF bits */
                return(0);
            if (!(lpData = GlobalLock(hData)))
                return(0);
            if (!GetEnhMetaFileBits(hEMF, dwSize, (LPBYTE)lpData))
                return(0);
            dwSize = lwrite(hFile, lpData, dwSize);
            GlobalUnlock(hData);
            GlobalFree(hData);
            break;


        case CF_METAFILEPICT:
            if (!(lpMFP = (LPMETAFILEPICT)GlobalLock(hData))) /* get header */
                return(0);

            if (fNTSaveFileFormat)
                _lwrite(hFile, (LPBYTE)lpMFP, sizeof(METAFILEPICT)); /* write header */
            else {
                /* If we save the metafile in the Windows 3.1 .CLP file format
                   we have to save the METAFILEPICT structure as a 16bit METAFILEPICT
                   structure. This may cause loss of information if the
                   hight half of the METAFILEPICT structure's fields are used.
                   [pierrej 5/27/92]                                        */

                _lwrite(hFile, (LPBYTE)&(lpMFP->mm), sizeof(WORD));
                _lwrite(hFile, (LPBYTE)&(lpMFP->xExt), sizeof(WORD));
                _lwrite(hFile, (LPBYTE)&(lpMFP->yExt), sizeof(WORD));
                _lwrite(hFile, (LPBYTE)&(lpMFP->hMF), sizeof(WORD));
            }

            hMF = lpMFP->hMF;

            GlobalUnlock(hData);            /* unlock the header */

            /* A-MGates 9/15/92 - Converted this block to use */
            /* GetMetaFileBitsEx                              */

            /* Figure out how big a block we need */
            dwSize = GetMetaFileBitsEx(hMF, 0, NULL);
            if (0 == dwSize)
               {
               return(0);
               }

            hData = GlobalAlloc(GMEM_MOVEABLE, dwSize);
            if (!(lpData = GlobalLock(hData)))
               {
               return(0);
               }

            if (dwSize != GetMetaFileBitsEx(hMF, dwSize, lpData))
               {
               GlobalUnlock(hData);
               GlobalFree(hData);
               return(0);
               }

            dwSize = lwrite(hFile, lpData, dwSize); /* spit them out */

            GlobalUnlock(hData);
            GlobalFree(hData);

            if(dwSize)
                if (fNTSaveFileFormat)
                    dwSize += sizeof(METAFILEPICT);     /* we wrote this much data */
                else
                    dwSize += SIZE_OF_WIN31_METAFILEPICT_STRUCT;
            break;

        case CF_BITMAP:
            // hBitmap = hData;

            /* Writing DDBs to disk is stupid. Therefore, we */
            /* write an intelligent CF_DIB block instead.    */
            /* A-MGATES 9/29/92                              */

            Format = CF_DIB;

            hBitmap = DibFromBitmap((HBITMAP)hData, BI_RGB, 4, NULL);

            lpData = GlobalLock(hBitmap);

            // dwSize might be too big, but we can live with that.
            dwSize = GlobalSize(lpData);

            _lwrite(hFile, lpData, dwSize);

            GlobalUnlock(hBitmap);
            GlobalFree(hBitmap);

            break;

#ifdef ICKYOLDCODE

            if (!fNTSaveFileFormat)
                hBitmap = BitmapToBitmap(hBitmap, 4, 1);

            GetObject(hBitmap, sizeof(BITMAP), (LPBYTE) &bitmap);
            dwSize = (DWORD)bitmap.bmWidthBytes * bitmap.bmHeight * bitmap.bmPlanes;

            if (!fNTSaveFileFormat)
               {
               // Round up to the nearest TWO bytes when saving to Win 3.1,
               // not the nearest four, which is what GetObject gives you.
               // Note: The WidthBytes calculation does not include
               // bmPlanes in the multiplication because it seems to represent
               // bytes in a given plane.
               bitmap.bmWidthBytes =
                     ((bitmap.bmWidth * bitmap.bmBitsPixel)+ 15 ) >> 3;
                      // ">> 3" == " / 8", except cheaper.

               if (bitmap.bmWidthBytes & 1)
                  {
                  bitmap.bmWidthBytes++;
                  }
               }

            if (!(hData = GlobalAlloc(GHND, dwSize)))
                return(0);

            if (!(lpData = GlobalLock(hData)))
                {
                GlobalFree(hData);
                return(0);
                }

            GetBitmapBits(hBitmap, dwSize, lpData);

            if (fNTSaveFileFormat)
                _lwrite(hFile, (LPBYTE) & bitmap, sizeof(BITMAP));
            else {
                /* If we save the bitmap in the Windows 3.1 .CLP file format
                   we have to save the BITMAP structure as a 16bit BITMAP
                   structure. This may cause loss of information if the
                   hight half of the BITMAP structure's fields are used.
                   [pierrej 5/27/92]                                        */

                bmWin31.bmType = bitmap.bmType;
                bmWin31.bmWidth = bitmap.bmWidth;
                bmWin31.bmHeight = bitmap.bmHeight;
                bmWin31.bmWidthBytes = bitmap.bmWidthBytes;
                bmWin31.bmPlanes = bitmap.bmPlanes;
                bmWin31.bmBitsPixel = bitmap.bmBitsPixel;
                bmWin31.bmBits = bitmap.bmBits;

                _lwrite(hFile, (LPBYTE) &bmWin31, sizeof(WIN31BITMAP));
            }
            dwSize = lwrite(hFile, lpData, dwSize);

            GlobalUnlock(hData);
            GlobalFree(hData);
            if(dwSize)
                if (fNTSaveFileFormat)
                    dwSize += sizeof(BITMAP);
                else
                    dwSize += sizeof(WIN31BITMAP);
            break;

#endif

        case CF_PALETTE:
            /* Get the number of palette entries */
            GetObject(hData, sizeof(WORD), (LPBYTE)&wPalEntries);

            /* Allocate enough place to build the LOGPALETTE struct */
            dwSize = (DWORD)(sizeof(LOGPALETTE) + (LONG)wPalEntries * sizeof(PALETTEENTRY));
            if (!(hLogPalette = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dwSize)))
                {
                dwSize = 0L;
                goto Palette_Error;
                }

            if (!(lpLogPalette = (LPLOGPALETTE)GlobalLock(hLogPalette)))
                {
                dwSize = 0L;
                goto Palette_Error;
                }

            lpLogPalette->palVersion = 0x300;      /* Windows 3.00 */
            lpLogPalette->palNumEntries = wPalEntries;

        if (GetPaletteEntries(hData, 0, wPalEntries,
              (LPPALETTEENTRY)(lpLogPalette->palPalEntry)) == 0)
                {
                dwSize = 0L;
                goto Palette_Error;
                }

            /* Write the LOGPALETTE structure onto disk */
            dwSize = lwrite(hFile, (LPBYTE)lpLogPalette, dwSize);

Palette_Error:
            if (lpLogPalette)
                GlobalUnlock(hLogPalette);
            if (hLogPalette)
                GlobalFree(hLogPalette);
            break;

        default:
            dwSize = GlobalSize(hData);

            if (0 ==(lpData = GlobalLock(hData)) )
               {
               return 0;
               }

            dwSize = _lwrite(hFile, lpData, dwSize);
            GlobalUnlock(hData);
            break;
        }

    /* Return the number of bytes written. */
    return(dwSize);
}


/* This function will return the number of clipboard formats compatible with
   the Windows 3.1 clipboard, this excludes CF_UNICODETEXT, CF_ENHMETAFILE and
   CF_DSPENHMETAFILE
*/

int Count16BitClipboardFormats(void)
{
    int iCount;

    iCount = CountClipboardFormats();
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
        iCount--;
    if (IsClipboardFormatAvailable(CF_ENHMETAFILE))
        iCount--;
    if (IsClipboardFormatAvailable(CF_DSPENHMETAFILE))
        iCount--;

    return iCount;
}


/*
 *  SaveClipboardData() - Writes a clipboard file.
 *
 * In:
 *    hwnd        handle of wnd that becomes the clipboard owner
 *    szFileName  file handle to read from
 *
 * NOTE:
 *    When done we call ReadClipboardFromFile(). this makes us the
 *    clipboard owner.
 */

BOOL NEAR PASCAL SaveClipboardData(HWND    hwnd,LPTSTR szLocalFileName)

{
INT    fh;
register UINT Format;
DWORD     HeaderPos;
DWORD     DataPos;
DWORD     datasize;
BOOL      fComplain;
BOOL      fDIBUsed;
HCURSOR   hCursor;
FILEHEADER FileHeader;
TCHAR      szComplaint[BUFFERLEN];
TCHAR      szName[CCHFMTNAMEMAX];
WORD      wHeaderSize;
UINT      uiSizeHeaderToWrite;

    /* First open the clipboard */
    if (!OpenClipboard(hwndMain))
        return(FALSE);

    /* Open the file */

    fh = (INT)CreateFile((LPCTSTR)szLocalFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == -1)
        {
        GetLastError();
        CloseClipboard();
        return(FALSE);
        }
    /* Fill out the file header structure */
    if (fNTSaveFileFormat) {
        FileHeader.magic = CLP_NT_ID;          /* magic number to tag our files */
        uiSizeHeaderToWrite = (sizeof(UINT) + 2*sizeof(DWORD) + CCHFMTNAMEMAX*sizeof(TCHAR));
    } else {
        FileHeader.magic = CLP_ID;          /* magic number to tag our files */
        uiSizeHeaderToWrite = (sizeof(WORD) + 2*sizeof(DWORD) + CCHFMTNAMEMAX*sizeof(TCHAR));
    }
    FileHeader.FormatCount = 0;          /* dummy for now */

    /* Update HeaderPos and DataPos */
    HeaderPos = sizeof(FILEHEADER);

    /* This is the maximum number of formats that will be written.  Potentially
     * some may fail and some space will be wasted.
     * In 32bit the number of bytes written to the disk isn't sizeof(FORMATHEADER)
     * because of DWORD alignment in the FORMATHEADER structure. Instead we write
     * the format headre structure one field at a time to remain compatible with
     * the 16bit Windows versions.
     */

    if (fNTSaveFileFormat)
        DataPos = HeaderPos + (uiSizeHeaderToWrite * CountClipboardFormats());
    else
        DataPos = HeaderPos + (uiSizeHeaderToWrite * Count16BitClipboardFormats());

    /* Now loop throught the data, one format at a time, and write out the data. */
    fComplain = FALSE;

    LoadString(hInst, IDS_FMTNOTSAV, szComplaint, BUFFERLEN);

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    /* Enumerate all formats */
    Format = 0;
    fDIBUsed = FALSE;
    while ((Format = EnumClipboardFormats(Format)) != 0)
        {
        if (IsWriteable(Format))
            {
            // DO NOT write CF_BITMAP to disk. Transform to CF_DIB
            // and write that instead.
            if (CF_BITMAP == Format || CF_DIB == Format)
               {
               if (!fDIBUsed)
                  {
                  fDIBUsed = TRUE;
                  }
               // If I've already done DIB, go on to the next format.
               else continue;
               }

            GetClipboardName((Format == CF_BITMAP ? CF_DIB : Format),
                  szName, sizeof(szName));

            if ((datasize = WriteDataBlock(fh, DataPos, Format)) != 0)
                {
                /* Create a Format header and write it to the file */
                wHeaderSize = (WORD)WriteFormatBlock(fh,
                     HeaderPos, DataPos, datasize,
                     (Format == CF_BITMAP ? CF_DIB : Format),
                     (LPTSTR)szName);
                if(wHeaderSize < uiSizeHeaderToWrite)
                    {
                    fComplain = TRUE;
                    break;
                    }
                HeaderPos += wHeaderSize;

                /* Update the data pos for the next block */
                DataPos += datasize;

                FileHeader.FormatCount++;       /* this format has been written */
                }
            else
                {
                fComplain = TRUE;
                break;
                }
            }
        }

    ShowCursor(FALSE);
    SetCursor(hCursor);

    if (fComplain)
        {
#ifdef JAPAN
        /* Use Japanese message for caption instead of app name */
        MessageBox(hwnd, szComplaint, szCaptionName, MB_OK | MB_ICONEXCLAMATION);
#else
        MessageBox(hwnd, szComplaint, szAppName, MB_OK | MB_ICONEXCLAMATION);
#endif
        CloseClipboard();
        _lclose(fh);
        return(FALSE);
        }

    CloseClipboard();      /* we are done looking at this */

    _llseek(fh, 0L, 0);     /* move back to the start of file */

    /* Write the File Header with the correct number of formats written */
    _lwrite(fh, (LPBYTE) & FileHeader, sizeof(FILEHEADER));

    /* Now we open the clipboard and become the owner.  this places
     * all the things we just saved in the clipboard (and throws out
     * those things we didn't save)
     */
    _llseek(fh, 0L, 0);

    /* ofStruct will be used for reopening the file */
    lstrcpy(szFileName, szSaveFileName);
    fNTReadFileFormat = fNTSaveFileFormat;

    ReadClipboardFromFile(hwndMain, fh);

    _lclose(fh);

    return(TRUE);
}

/*
 *  SaveClipboardToFile() -
 */
void NEAR PASCAL SaveClipboardToFile(HWND hwnd)
{
INT    hFile;

OFN.lpstrTitle = szSaveCaption;
OFN.lpstrFile  = szSaveFileName;
szSaveFileName[0] = 0;
OFN.Flags      = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
                 OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;

/* All long pointers should be defined immediately before the call. */
OFN.lpstrDefExt       = (LPTSTR)szDefExt;
OFN.lpstrFilter       = (LPTSTR)szFilterSpec;
OFN.lpstrCustomFilter = NULL;
OFN.lpfnHook          = NULL;
OFN.nFilterIndex      = 1;

hFile = GetSaveFileName ((LPOPENFILENAME) &OFN);
if (hFile)
   {
   // The first filter listed is "NT Clipboard File". The second is
   // "Windows 3.1 Clipboard file".
   fNTSaveFileFormat = (1 == OFN.nFilterIndex);

   hFile = (INT)CreateFile((LPCTSTR)szSaveFileName, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != -1)
      /* The file already exists and the user wants to
       * overwrite! Now, we have to check whether this
       * file is the most recently saved clip file;
       * If so, it will be used for delayed rendering; So,
       * before we overwrite this file we must read all data
       * from it thro RENDERALLFORMATS;
       * Fix for Bug #5602 --SANKAR-- 10-16-89
       */
      {
      CloseHandle((HANDLE)hFile);
      if (lstrcmp(szSaveFileName, szFileName) == 0)
         {
         SendMessage(hwndMain, WM_RENDERALLFORMATS, 0, 0L);
         }
      }

   if (!SaveClipboardData(hwnd, (LPTSTR)szSaveFileName))
      {
      /* If Failure, Delete the incomplete file */
      DeleteFile(szSaveFileName);
      }
   }
}


//
// Purpose:
//    Convert an existing bitmap to the given number of planes and
//    bits/pixel.
//
// Parameters:
//    hBitmap - Handle to the existing bitmap.
//    wPlanes - Number of planes in the destination bitmap.
//    wBitCount - Number of bits/pel in the destination bitmap.
//
// Returns:
//    A handle to the converted bitmap.
//
////////////////////////////////////////////////////////////////////////////
HBITMAP PASCAL BitmapToBitmap(HBITMAP hBitmap, WORD wPlanes, WORD wBitCount)
{
    BITMAP  bm;
    BITMAPINFOHEADER    BmpInfoHeader;
    HANDLE  hDib, hBmp;
    LPBYTE  lpDib, lpBits;
    DWORD   dwLength, dwSize;
    int     iColorTable;
    int     iColorUsed;
    HDC     hDC;

    /*
    ** Get the size of the bitmap.  These values are used to setup the memory
    ** requirements for the DIB.
    */
    GetObject(hBitmap,sizeof(BITMAP),(LPSTR)&bm);
    if ((bm.bmBitsPixel == wBitCount) && (bm.bmPlanes == wPlanes))
        return hBitmap;

    #if 0
    dwSize = dwLength = (DWORD)(bm.bmWidthBytes+sizeof(WORD)) * bm.bmHeight * bm.bmPlanes;
    #else
    dwSize = dwLength = (DWORD)(((bm.bmWidth * wBitCount + 7)/8)*2 + 1)/2
         * wPlanes * bm.bmHeight;
    #endif

    switch(bm.bmBitsPixel * bm.bmPlanes)
    {
        case 1:
            iColorTable = sizeof(RGBQUAD) * 2;
            break;

        case 4:
            iColorTable = sizeof(RGBQUAD) * 16;
            break;

        case 8:
            iColorTable = sizeof(RGBQUAD) * 256;
            break;

        case 24:
        default:
            iColorTable = 0;
            break;
    }
    iColorUsed = iColorTable / sizeof(RGBQUAD);
    dwLength   += (sizeof(BITMAPINFOHEADER) + iColorTable);


    /*
    ** Create the DIB.  First, to the size of the bitmap.  We will calculate
    ** the new memory requirements if DIB-Compression is desired.
    */
    if(hDib = GlobalAlloc(GHND,dwLength)) {
        if(lpDib = GlobalLock(hDib))
           {
           ((LPBITMAPINFOHEADER)lpDib)->biSize          = sizeof(BITMAPINFOHEADER);
           ((LPBITMAPINFOHEADER)lpDib)->biWidth         = bm.bmWidth;
           ((LPBITMAPINFOHEADER)lpDib)->biHeight        = bm.bmHeight;
           ((LPBITMAPINFOHEADER)lpDib)->biPlanes        = 1;
           ((LPBITMAPINFOHEADER)lpDib)->biBitCount      = bm.bmBitsPixel*bm.bmPlanes;
           ((LPBITMAPINFOHEADER)lpDib)->biCompression   = BI_RGB;
           ((LPBITMAPINFOHEADER)lpDib)->biSizeImage     = 0;
           ((LPBITMAPINFOHEADER)lpDib)->biXPelsPerMeter = 0;
           ((LPBITMAPINFOHEADER)lpDib)->biYPelsPerMeter = 0;
           ((LPBITMAPINFOHEADER)lpDib)->biClrUsed       = 0;
           ((LPBITMAPINFOHEADER)lpDib)->biClrImportant  = 0;


           // Figure out where the bits go
           lpBits = (LPBYTE)lpDib+sizeof(BITMAPINFOHEADER)+iColorTable;
           hDC = GetDC(hwndMain);
           if (NULL != hDC)
              {
              if (bm.bmHeight == GetDIBits(hDC,hBitmap,0,
                        bm.bmHeight,lpBits,(LPBITMAPINFO)lpDib,
                        // DIB_PAL_INDICES))
                        DIB_RGB_COLORS))
                 {
                 BmpInfoHeader.biSize          = sizeof(BITMAPINFOHEADER);
                 BmpInfoHeader.biWidth         = bm.bmWidth;
                 BmpInfoHeader.biHeight        = bm.bmHeight;
                 BmpInfoHeader.biPlanes        = wPlanes;
                 BmpInfoHeader.biBitCount      = wBitCount;
                 BmpInfoHeader.biCompression   = BI_RGB;
                 BmpInfoHeader.biSizeImage     = dwSize;
                 BmpInfoHeader.biXPelsPerMeter = 0;
                 BmpInfoHeader.biYPelsPerMeter = 0;
                 BmpInfoHeader.biClrUsed       = iColorUsed;
                 BmpInfoHeader.biClrImportant  = iColorUsed;
                 hBmp = CreateDIBitmap(NULL, &BmpInfoHeader, CBM_INIT,
                            lpBits, (LPBITMAPINFO)lpDib, DIB_RGB_COLORS);
                 }
              ReleaseDC(hwndMain, hDC);
              }
           GlobalUnlock(hDib);
           }
        GlobalFree(hDib);
        }

    return(hBmp);
}
