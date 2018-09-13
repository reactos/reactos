
/*****************************************************************************

                                C L I P F I L E

    Name:       clipfile.c
    Date:       19-Apr-1994
    Creator:    Unknown

    Description:
        Windows Clipboard File I/O Routines.

        NOTE:
          When saving the contents of the clipboard we SetClipboardData(fmt, NULL)
          to free up the memory associated with each clipboard format.  Then
          after we are done saving we take over as the clipboard owner.  This
          causes OWNERDRAW formats to be lost in the save process.

    History:
        19-Apr-1994 John Fu     Add code to make CF_BITMAP available when
                                there's CF_DIB.
                                Add CF_UNICODETEXT in GetClipboardName.
                                Fix AddNetInfoToClipboard not to dereference
                                GlobalLock(hData) when it's NULL.

        16-Jun-1994 John Fu     Fix SaveClipboardData() so when CF_DIB exists
                                in clipboard, CF_BITMAP will not be converted.

        08-Jul-1994 John Fu     Fix call to RenderFormatDibToBitmap to use
                                palette from file.

        13-Mar-1995 John Fu     Fix computer name buffer size.

        15-Mar-1995 John Fu     Add GetClipboardData to make sure CF_DIB
                                really exists.

*****************************************************************************/





#include <windows.h>
#include <windowsx.h>
#include <string.h>

#include "common.h"
#include "clipfile.h"
#include "clipfile.h"
#include "dib.h"
#include "debugout.h"
#include "render.h"
#include "security.h"







BOOL    fAnythingToRender;

TCHAR   szFileSpecifier[] = TEXT("*.CLP");
TCHAR   szFileName[MAX_PATH+1];
TCHAR   szSaveFileName[MAX_PATH+1];     // Saved filename for delayed render

BOOL    fNTReadFileFormat;
BOOL    fNTSaveFileFormat;

UINT    cf_link;
UINT    cf_objectlink;
UINT    cf_linkcopy;
UINT    cf_objectlinkcopy;





BOOL    AddDIBtoDDB(VOID);







/*******************

 File read routines

*******************/





/*
 *      ReadFileHeader
 *
 *  Purpose: Read the file header in the given .clp file, and get the number
 *     of formats. Also sets the fNTReadFileFormat flag appropriately.
 *
 *  Parameters:
 *     fh - Handle to the file.
 *
 *  Returns:
 *     The number of formats, or 0 if it isn't a valid .clp file.
 */

unsigned ReadFileHeader(
    HANDLE  fh)
{
FILEHEADER  FileHeader;
DWORD       dwBytesRead;


    // PINFO(TEXT("ClSrv\\RdFileHdr"));

    /* Read the File Header */
    SetFilePointer(fh, 0, NULL, FILE_BEGIN);
    ReadFile(fh, &FileHeader, sizeof(FileHeader), &dwBytesRead, NULL);

    if (dwBytesRead == sizeof(FILEHEADER))
        {
        // Make sure that this is a .CLP file
        if (FileHeader.magic == CLPBK_NT_ID ||
            FileHeader.magic == CLP_NT_ID)
            {
            fNTReadFileFormat = TRUE;
            }
        else if (FileHeader.magic == CLP_ID)
            {
            fNTReadFileFormat = FALSE;
            }
        else
            {
            PERROR(TEXT("Invalid magic member (not long enough?)\r\n"));
            FileHeader.FormatCount = 0;
            }

        // Check number of formats for additional reassurance.
        if (FileHeader.FormatCount > 100)
            {
            PERROR(TEXT("Too many formats!!!\r\n"));
            FileHeader.FormatCount = 0;
            }
        }
    else
        {
        PERROR("Read err\r\n");
        FileHeader.FormatCount = 0;
        }

    if (FileHeader.FormatCount)
        {
        // PINFO(TEXT("\r\n"));
        }


    return(FileHeader.FormatCount);

}







/*
 *      ReadFormatHeader
 */

BOOL ReadFormatHeader(
    HANDLE          fh,
    FORMATHEADER    *pfh,
    unsigned        iFormat)
{
DWORD           dwMrPibb;
OLDFORMATHEADER OldFormatHeader;



    // PINFO(TEXT("ClSrv\\RdFmtHdr"));

    if (NULL == pfh || NULL == fh)
        {
        PERROR("RdFmtHdr got NULL pointer\r\n");
        return FALSE;
        }

    SetFilePointer (fh,
                    sizeof(FILEHEADER) + iFormat *
                      (fNTReadFileFormat ? sizeof(FORMATHEADER) : sizeof(OLDFORMATHEADER)),
                    NULL,
                    FILE_BEGIN);

    if (fNTReadFileFormat)
        {
        ReadFile(fh, pfh, sizeof(FORMATHEADER), &dwMrPibb, NULL);

        if (dwMrPibb != sizeof(FORMATHEADER))
            {
            PERROR(TEXT("Bad new format rd\r\n"));
            return FALSE;
            }
        }
    else
        {
        ReadFile(fh, &OldFormatHeader, sizeof(OldFormatHeader), &dwMrPibb, NULL);

        if (dwMrPibb != sizeof(OLDFORMATHEADER))
            {
            PERROR(TEXT("Bad old format rd\r\n"));
            return FALSE;
            }

        pfh->FormatID   = OldFormatHeader.FormatID;
        pfh->DataLen    = OldFormatHeader.DataLen;
        pfh->DataOffset = OldFormatHeader.DataOffset;

        MultiByteToWideChar (CP_ACP,
                             MB_PRECOMPOSED,
                             OldFormatHeader.Name,
                             -1,
                             pfh->Name,
                             CCHFMTNAMEMAX);
        }

    // PINFO(TEXT("\r\n"));
    return TRUE;


}







/*
 *      ReadClipboardFromFile()
 *
 *  Read in a clipboard file and register all the formats in delayed mode.
 *  to render things for real reopen the file specified by ofStruct.
 *
 *  NOTE:
 *     This makes us the clipboard owner.
 *
 *  Bug 14564:  Changed return value to a short integer noting why the
 *  reading failed.
 *  Return Value:   READFILE_IMPROPERFORMAT
 *                  READFILE_OPENCLIPBRDFAIL
 *                  READFILE_SUCCESS
 */


short ReadClipboardFromFile(
    HWND    hwnd,
    HANDLE  fh)
{
register unsigned   i;
unsigned            cFormats;
FORMATHEADER        FormatHeader;



    PINFO(TEXT("Entering ReadClipboardFromFile\r\n"));

    if (!(cFormats = ReadFileHeader(fh)) )
        {
        return(READFILE_IMPROPERFORMAT);
        }


    /* We become the clipboard owner here! */
    if (!SyncOpenClipboard(hwnd))
        {
        PERROR(TEXT("Could not open clipboard!!!"));
        return(READFILE_OPENCLIPBRDFAIL);
        }

    EmptyClipboard();

    for (i=0; i < cFormats; i++)
        {
        ReadFormatHeader (fh, &FormatHeader, i);

        if (PRIVATE_FORMAT(FormatHeader.FormatID))
            {
            FormatHeader.FormatID = RegisterClipboardFormatW ((LPWSTR)FormatHeader.Name);
            }

        /*Delayed Render. */
        PINFO(TEXT("Set up delayed render for format %d .\r\n"), FormatHeader.FormatID);
        SetClipboardData (FormatHeader.FormatID, NULL);


        if (FormatHeader.FormatID == CF_DIB)
            SetClipboardData (CF_BITMAP, NULL);
        }




    /* Now, clipbrd viewer has something to render */
    if (cFormats > 0)
        {
        PINFO(TEXT("fAnythingToRender = TRUE\r\n"));
        fAnythingToRender = TRUE;
        }

    SyncCloseClipboard();

    return(READFILE_SUCCESS);

}







/*
 *      OpenClipboardFile
 */

DWORD OpenClipboardFile(
    HWND    hwnd,
    LPTSTR  szName)
{
HANDLE  fh;
DWORD   dwErr = NO_ERROR;


    PINFO(TEXT("OpenClipboardFile: %s \r\n"),szName);

    fh = CreateFile ((LPCTSTR)szName,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);

    if (fh != INVALID_HANDLE_VALUE)
        {
        // Store file name for delayed rendering stuff.
        lstrcpy(szSaveFileName, szName);

        // Read the sucker.
        switch (ReadClipboardFromFile (hwnd, fh))
            {
            case READFILE_IMPROPERFORMAT:
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            case READFILE_OPENCLIPBRDFAIL:
                dwErr = ERROR_CLIPBOARD_NOT_OPEN;
                break;
            case READFILE_SUCCESS:
            default:
                dwErr = NO_ERROR;
                break;
            }
        CloseHandle (fh);
        }
    else
        {
        PERROR(TEXT("ClSrv\\OpClpFile: can't open file!"));
        dwErr = GetLastError ();
        }

    return dwErr;

}






/*
 *      RenderFormatFormFile
 *
 *  Purpose: Go get the given format from the given file.
 */

HANDLE RenderFormatFromFile(
    LPTSTR  szFile,
    WORD    wFormat)
{
HANDLE          fh;
FORMATHEADER    FormatHeader;
HANDLE          hData = NULL;
unsigned        cFormats;
unsigned        i;
BOOL            bHasDib = FALSE;


    PINFO(TEXT("ClSrv\\RndrFmtFromFile: Opening file %s.\r\n"),szSaveFileName);

    fh = CreateFile (szFile,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);

    if (INVALID_HANDLE_VALUE == fh)
        {
        PERROR(TEXT("Can't open file\r\n"));
        goto done;
        }


    cFormats = ReadFileHeader(fh);


    // If ReadFile didn't get the whole header, don't try to read anything
    // else.
    if (0 == cFormats)
        {
        PERROR(TEXT("Bad file header.\r\n"));
        goto done;
        }


    for (i=0; i < cFormats; i++)
        {
        ReadFormatHeader(fh, &FormatHeader, i);

        PINFO(TEXT("Got format %ws\r\n"),FormatHeader.Name);

        if (PRIVATE_FORMAT(FormatHeader.FormatID))
            {
            FormatHeader.FormatID = RegisterClipboardFormatW(FormatHeader.Name);
            }

        if (FormatHeader.FormatID == wFormat)
            {
            hData = RenderFormat(&FormatHeader, fh);
            }

        if (FormatHeader.FormatID == CF_DIB)
            bHasDib = TRUE;
        }



    // JYF make CF_BITMAP available when there's CF_DIB

    if (!hData && wFormat == CF_BITMAP && bHasDib)
        {
        if (SetFilePointer (fh, 0, 0, FILE_BEGIN) == 0xFFFFFFFF)
            {
            PERROR(TEXT("Cannot set file pointer to FILE_BEGIN\n"));
            goto done;
            }


        cFormats = ReadFileHeader (fh);

        for (i=0; i < cFormats; i++)
            {
            ReadFormatHeader (fh, &FormatHeader, i);

            PINFO (TEXT("Got format %ws\n"), FormatHeader.Name);

            if (FormatHeader.FormatID == CF_DIB)
                hData = RenderFormatDibToBitmap (&FormatHeader,
                                                 fh,
                                                 RenderFormatFromFile (szFile, CF_PALETTE));
            }
        }



done:

    if (fh != INVALID_HANDLE_VALUE)
        CloseHandle (fh);

    return(hData);

}





/*
 *      RenderAllFromFile
 *
 *  Purpose: Go get all formats from the given file.
 */

HANDLE RenderAllFromFile(
    LPTSTR  szFile)
{
HANDLE          fh;
FORMATHEADER    FormatHeader;
HANDLE          hData;
unsigned        cFormats;
unsigned        i;


    /* Check if the clipbrd viewer has done any File I/O before.
     * If it has not, then it has nothing to render!  Sankar
     */
    if (CountClipboardFormats() && fAnythingToRender)
        {
        /* Empty the clipboard */
        if (!SyncOpenClipboard(hwndApp))
            {
            PERROR("Couldn't open clipboard!\r\n");
            }
        else
            {
            EmptyClipboard();

            fh = CreateFile (szFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

            if (INVALID_HANDLE_VALUE != fh)
                {
                cFormats = ReadFileHeader(fh);

                // If ReadFile didn't get the whole header, don't try to read anything
                // else.
                if (0 == cFormats)
                    {
                    PERROR(TEXT("Bad file header.\r\n"));
                    }

                for (i=0; i < cFormats; i++)
                    {
                    ReadFormatHeader(fh, &FormatHeader, i);

                    PINFO(TEXT("Got format %ws\r\n"),FormatHeader.Name);

                    if (PRIVATE_FORMAT(FormatHeader.FormatID))
                        {
                        FormatHeader.FormatID =
                           RegisterClipboardFormatW(FormatHeader.Name);
                        }

                    // Render the format and set it into the clipboard
                    hData = RenderFormat(&FormatHeader, fh);
                    if ( hData != NULL )
                        {
                        if (!SetClipboardData(FormatHeader.FormatID, hData))
                            {
                            PERROR(TEXT("SetClipboardData fail\n\r"));
                            }
                        }
                    else
                        {
                        PERROR(TEXT("hData == NULL, bad\r\n"));
                        }
                    }
                CloseHandle(fh);
                }
            else
                {
                PERROR(TEXT("Can't open file\r\n"));
                }

            SyncCloseClipboard();
            }
        }

    return(0L);


}














/********************

 File write routines

********************/






/*
 *      IsWriteable()
 *
 *  Test if a clipboard format is writeable(i.e. if it makes sense to write it)
 *  OWNERDRAW and others can't be written because we (CLIPBRD) will become the
 *  owner when the files are reopened.
 */

BOOL IsWriteable(WORD Format)

{
    /* Are the PRIVATEFIRST and PRIVATELAST things right? */
    if ((Format >= CF_PRIVATEFIRST && Format <= CF_PRIVATELAST)
          || Format == CF_OWNERDISPLAY)
        {
        return FALSE;
        }

    // If we're not saving an NT clipboard, don't save NT-specific formats.
    if (!fNTSaveFileFormat &&
        (Format == CF_UNICODETEXT || Format == CF_ENHMETAFILE
         || Format == CF_DSPENHMETAFILE)
       )
        {
        return(FALSE);
        }

    return(TRUE);
}






/*
 *      Count16BitClipboardFormats
 *
 *  This function will return the number of clipboard formats compatible with
 *  the Windows 3.1 clipboard, this excludes CF_UNICODETEXT, CF_ENHMETAFILE and
 *  CF_DSPENHMETAFILE
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
 *      WriteFormatBlock
 *
 *  Purpose: Writes the format header for a single data format.
 *
 *  Parameters:
 *   fh - File handle to write to.
 *   offset - Position in the file to write the format block.
 *   DataOffset - Position in the file where the data for this format will be.
 *   DataLen    - Length of the data for this format.
 *   Format     - The format number.
 *   szName     - Name of the format.
 *
 *  Returns:
 *   The number of bytes written to the file.
 */

DWORD WriteFormatBlock(
    HANDLE  fh,
    DWORD   offset,
    DWORD   DataOffset,
    DWORD   DataLen,
    UINT    Format,
    LPWSTR  wszName)
{
DWORD   dwBytesWritten = 0;


    SetFilePointer(fh, offset, NULL, FILE_BEGIN);

    if (fNTSaveFileFormat)
        {
        FORMATHEADER    FormatHeader;

        memset (&FormatHeader, 0, sizeof(FormatHeader));

        FormatHeader.FormatID   = Format;
        FormatHeader.DataLen    = DataLen;
        FormatHeader.DataOffset = DataOffset;

        lstrcpyW(FormatHeader.Name, wszName);
        WriteFile (fh, &FormatHeader, sizeof(FormatHeader), &dwBytesWritten, NULL);
        }
    else
        {
        OLDFORMATHEADER OldFormatHeader;

        memset(&OldFormatHeader,0, sizeof(OldFormatHeader));

        OldFormatHeader.FormatID   = (WORD)Format;
        OldFormatHeader.DataLen    = DataLen;
        OldFormatHeader.DataOffset = DataOffset;

        WideCharToMultiByte(CP_ACP,
                            0,
                            wszName,
                            -1,
                            OldFormatHeader.Name,
                            CCHFMTNAMEMAX,
                            NULL,
                            NULL);
        WriteFile (fh,
                   &OldFormatHeader,
                   sizeof(OldFormatHeader),
                   &dwBytesWritten,
                   NULL);
        }

    return(dwBytesWritten);

}





/*
 *      WriteDataBlock() -
 *
 *  Returns:
 *     # of bytes written to the output file
 *
 *  NOTE: Write saves the name of a temp file in the clipboard for it's
 *  own internal clipboard format.  This file goes aways when Write
 *  (or windows?) shuts down.  Thus saving Write clipboards won't work
 *  (should we special case hack this?)
 *
 */

DWORD WriteDataBlock(
    register HANDLE hFile,
    DWORD           offset,
    WORD            Format)
{
WORD            wPalEntries;
LPSTR           lpData;
DWORD           dwSize = 0;
BITMAP          bitmap;
HANDLE          hMF;
HANDLE          hBitmap;
register HANDLE hData;
LPLOGPALETTE    lpLogPalette;
LPMETAFILEPICT  lpMFP;
DWORD           dwMFPSize;
BOOL            fOK = FALSE;


    if (!(hData = GetClipboardData(Format)) ||
        SetFilePointer (hFile, offset, NULL, FILE_BEGIN) != offset)
        {
        PERROR(TEXT("WriteDataBlock: couldn't get format data\n\r"));
        return 0;
        }



    /* We have to special case a few common formats but most things
     * get handled in the default case.
     */

    switch (Format)
        {
        case CF_ENHMETAFILE:
            dwSize = (DWORD) GetEnhMetaFileBits(hData, 0, NULL); /* Get data size */

            if (lpData = GlobalAllocPtr(GHND, dwSize))   /* allocate mem for EMF bits */
                {
                if (GetEnhMetaFileBits(hData, dwSize, (LPBYTE)lpData))
                    {
                    WriteFile(hFile, lpData, dwSize, &dwSize, NULL);
                    fOK = TRUE;
                    }
                GlobalFreePtr(lpData);
                }

            if (!fOK)
                {
                PERROR(TEXT("WriteDataBlock: couldn't write CF_ENHMETAFILE\r\n"));
                dwSize = 0;
                }
            break;

        case CF_METAFILEPICT:
            if (lpMFP = (LPMETAFILEPICT)GlobalLock(hData)) /* get header */
                {
                // Write METAFILEPICT header -- if we're saving in Win31 format,
                // write the old-style header.
                if (fNTSaveFileFormat)
                    {
                    WriteFile(hFile, lpMFP, sizeof(METAFILEPICT),
                        &dwMFPSize, NULL);
                    }
                else
                    {
                    WIN31METAFILEPICT w31mfp;
                    /* If we save the metafile in the Windows 3.1 .CLP file format
                       we have to save the METAFILEPICT structure as a 16bit METAFILEPICT
                       structure. This may cause loss of information if the
                       high half of the METAFILEPICT structure's fields are used.
                       [pierrej 5/27/92]                                        */

                    w31mfp.mm   = (WORD)lpMFP->mm;
                    w31mfp.xExt = (WORD)lpMFP->xExt;
                    w31mfp.yExt = (WORD)lpMFP->yExt;
                    w31mfp.hMF  = (WORD)0;

                    WriteFile(hFile, &w31mfp, sizeof(WIN31METAFILEPICT), &dwMFPSize, NULL);
                    }

                hMF = lpMFP->hMF;

                GlobalUnlock(hData);            /* unlock the header */

                /* Figure out how big a block we need */
                dwSize = GetMetaFileBitsEx(hMF, 0, NULL);
                if (dwSize)
                    {
                    if (lpData = GlobalAllocPtr(GHND, dwSize))
                        {
                        if (dwSize == GetMetaFileBitsEx(hMF, dwSize, lpData))
                            {
                            WriteFile(hFile, lpData, dwSize, &dwSize, NULL);

                            dwSize += dwMFPSize;
                            }
                        else
                            {
                            dwSize = 0;
                            }

                        GlobalFreePtr(lpData);
                        }
                    else
                        {
                        dwSize = 0;
                        }
                    }
                }
            break;

        case CF_BITMAP:

            /* Writing DDBs to disk is stupid. Therefore, we */
            /* write an intelligent CF_DIB block instead.    */
            /* A-MGATES 9/29/92                              */

            Format = CF_DIB;

            GetObject((HBITMAP)hData, sizeof(BITMAP), &bitmap);

            if (hBitmap = DibFromBitmap ((HBITMAP)hData,
                                         BI_RGB,
                                         (WORD) (bitmap.bmBitsPixel * bitmap.bmPlanes),
                                         IsClipboardFormatAvailable(CF_PALETTE) ?
                                           GetClipboardData(CF_PALETTE) : NULL))
               {
               if (lpData = GlobalLock(hBitmap))
                   {
                   // dwSize might be too big, but we can live with that.
                   dwSize = (DWORD)GlobalSize(lpData);

                   WriteFile(hFile, lpData, dwSize, &dwSize, NULL);

                   // Clean up
                   GlobalUnlock(hBitmap);
                   GlobalFree(hBitmap);
                   }
               }
            break;

        case CF_PALETTE:
            /* Get the number of palette entries */
            GetObject(hData, sizeof(WORD), (LPBYTE)&wPalEntries);

            /* Allocate enough place to build the LOGPALETTE struct */
            dwSize = (DWORD)(sizeof(LOGPALETTE) +
                 (LONG)wPalEntries * sizeof(PALETTEENTRY));
            if (lpLogPalette = (LPLOGPALETTE)GlobalAllocPtr(GHND, dwSize))
                {
                lpLogPalette->palVersion = 0x300;      /* Windows 3.00 */
                lpLogPalette->palNumEntries = wPalEntries;

                if (GetPaletteEntries(hData, 0, wPalEntries,
                   (LPPALETTEENTRY)(lpLogPalette->palPalEntry)) != 0)
                    {
                    /* Write the LOGPALETTE structure onto disk */
                    WriteFile(hFile, lpLogPalette, dwSize, &dwSize, NULL);
                    }
                else
                    {
                    dwSize = 0;
                    }

                GlobalFreePtr(lpLogPalette);
                }
            else
                {
                dwSize = 0L;
                }
            break;

        default:
            dwSize = (DWORD)GlobalSize(hData);

            // Just lock the data down and write it out.
            if (lpData = GlobalLock(hData))
                {
                WriteFile(hFile, lpData, dwSize, &dwSize, NULL);
                GlobalUnlock(hData);
                }
            else
                {
                dwSize = 0;
                }

            break;
            }


    /* Return the number of bytes written. */
    return(dwSize);


}









/*
 *      GetClipboardNameW
 */

void GetClipboardNameW(
    register int    fmt,
    LPWSTR          wszName,
    register int    iSize)
{
LPWSTR  lprgch = NULL;
HANDLE  hrgch  = NULL;


    *wszName = '\0';


    /* Get global memory that everyone can get to */
    if (!(hrgch = GlobalAlloc(GMEM_MOVEABLE, (LONG)(iSize + 1)*sizeof(WCHAR))))
        {
        PERROR(TEXT("GetClipboardNameW: bad alloc\r\n"));
        goto done;
        }


    if (!(lprgch = (LPWSTR)GlobalLock(hrgch)))
       {
       PERROR(TEXT("GetClipboardNameW: bad lock\r\n"));
       goto done;
       }


    memset(lprgch, 0, (iSize+1)*sizeof(WCHAR));

    switch (fmt)
        {
        case CF_RIFF:
        case CF_WAVE:
        case CF_PENDATA:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_TEXT:
        case CF_BITMAP:
        case CF_METAFILEPICT:
        case CF_ENHMETAFILE:
        case CF_OEMTEXT:
        case CF_DIB:
        case CF_PALETTE:
        case CF_DSPTEXT:
        case CF_DSPBITMAP:
        case CF_DSPMETAFILEPICT:
        case CF_DSPENHMETAFILE:
        case CF_UNICODETEXT:
            LoadStringW(hInst, fmt, lprgch, iSize);
            break;

        case CF_OWNERDISPLAY:         /* Clipbrd owner app supplies name */
            // Note: This should NEVER happen because this function only gets
            // called when we're writing a given clipboard format. Clipbrd can't
            // get away with writing CF_OWNERDISPLAY because we become clipboard
            // owner when we re-read the file, and we won't know how to deal.

            PERROR(TEXT("GetClipboardName on OwnerDisplay format!\r\n"));

            // *lprgch = '\0';
            // SendOwnerMessageW(WM_ASKCBFORMATNAME, iSize, (LPARAM)lprgch);

            // if (!*lprgch)

            LoadStringW(hInst, fmt, lprgch, iSize);
            break;

        default:
            GetClipboardFormatNameW(fmt, lprgch, iSize);
            break;
        }

    lstrcpyW(wszName, lprgch);


done:

    if (lprgch) GlobalUnlock(hrgch);
    if (hrgch)  GlobalFree(hrgch);

}







/*
 *      SaveClipboardData() - Writes a clipboard file.
 *
 *  In:
 *     hwnd        handle of wnd that becomes the clipboard owner
 *     szFileName  file handle to read from
 *     fPage       TRUE if this is a clipbook page (which means we secure it)
 *
 *  NOTE:
 *     When done we call ReadClipboardFromFile(). this makes us the
 *     clipboard owner.
 *
 *  Returns:
 *      NO_ERROR if no error otherwise an error code.
 */

DWORD SaveClipboardData(
    HWND    hwnd,
    LPTSTR  szFileName,
    BOOL    fPage)
{
register HANDLE fh;
register WORD   Format;

SECURITY_ATTRIBUTES sa;

DWORD       HeaderPos;
DWORD       DataPos;
DWORD       datasize;
HCURSOR     hCursor;
FILEHEADER  FileHeader;
// Must be  WCHAR... it be format name!
WCHAR       wszName[CCHFMTNAMEMAX];
UINT        wHeaderSize;
UINT        uiSizeHeaderToWrite;
BOOL        fDIBUsed = FALSE;
DWORD       dwTemp;
DWORD       dwRet = NO_ERROR;



    /* First open the clipboard */
    if (!SyncOpenClipboard(hwndApp))
        return ERROR_CLIPBOARD_NOT_OPEN;


    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = (fPage ? CurrentUserOnlySD() : NULL);
    sa.bInheritHandle = FALSE;

    fh = CreateFile((LPCTSTR)szFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
               &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fh == INVALID_HANDLE_VALUE)
        {
        PERROR ("Error opening clipboard file!\r\n");
        dwRet = GetLastError ();
        goto done;
        }




    /* Fill out the file header structure */
    if (fNTSaveFileFormat)
        {
        FileHeader.magic = CLPBK_NT_ID;          /* magic number to tag our files */
        uiSizeHeaderToWrite = sizeof(FORMATHEADER);
        }
    else
        {
        FileHeader.magic = CLP_ID;          /* magic number to tag our files */
        uiSizeHeaderToWrite = sizeof(OLDFORMATHEADER);
        }


    FileHeader.FormatCount = 0;          /* dummy for now */

    /* Update HeaderPos and DataPos */
    HeaderPos = sizeof(FILEHEADER);

    /* This is the maximum number of formats that will be written.  Potentially
     * some may fail and some space will be wasted.
     */
    if (fNTSaveFileFormat)
        {
        DataPos = HeaderPos + (uiSizeHeaderToWrite * CountClipboardFormats());
        }
    else
        {
        DataPos = HeaderPos + (uiSizeHeaderToWrite * Count16BitClipboardFormats());
        }


   /* Now loop throught the data, one format at a time, and write out the data. */
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);


    /* Enumerate all formats */
    Format = 0;

    while ((Format = (WORD)EnumClipboardFormats(Format)))
        {
        if (IsWriteable(Format))
            {

            // DO NOT write CF_BITMAP to disk. Transform to CF_DIB
            // and write that instead.

            // If there's CF_DIB, then don't do CF_BITMAP

            if (CF_BITMAP == Format)
                if (IsClipboardFormatAvailable (CF_DIB)
                    && GetClipboardData (CF_DIB))
                    continue;   // We have DIB, don't worry about BITMAP.


            if (CF_BITMAP == Format || CF_DIB == Format)
                {
                if (!fDIBUsed)
                    fDIBUsed = TRUE;
                else
                    // Already done DIB, go on to the next format.
                    continue;
                }


            GetClipboardNameW (Format == CF_BITMAP ? CF_DIB : Format,
                               wszName,
                               sizeof(wszName));


            PINFO(TEXT("SClipboardData: writing %ls (#)%d\r\n"), wszName,Format);

            if (datasize = WriteDataBlock(fh, DataPos, Format))
                {
                /* Create a Format header and write it to the file */
                wHeaderSize = (WORD)WriteFormatBlock (fh,
                                                      HeaderPos,
                                                      DataPos,
                                                      datasize,
                                                      Format == CF_BITMAP ? CF_DIB : Format,
                                                      wszName);
                if (wHeaderSize < uiSizeHeaderToWrite)
                    {
                    PERROR(TEXT("SaveClipboardData: error writing format block\n\r"));
                    dwRet = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                    }
                HeaderPos += wHeaderSize;

                /* Update the data pos for the next block */
                DataPos += datasize;

                FileHeader.FormatCount++;   /* this format has been written */
                }
            else
                {
                //JYF PERROR(TEXT("SaveClipboardData: error writing data block\n\r"));
                //JYF dwRet = ERROR_NOT_ENOUGH_MEMORY;
                //JYF break;
                }
            }
        }


    ShowCursor(FALSE);
    SetCursor(hCursor);

    SyncCloseClipboard();      /* we are done looking at this */


    // Go back and write the file header at the front of the file
    SetFilePointer (fh, 0L, NULL, FILE_BEGIN);

    if (!WriteFile (fh, &FileHeader, sizeof(FileHeader), &dwTemp, NULL))
        dwRet = GetLastError ();


    /* Now we open the clipboard and become the owner.  this places
     * all the things we just saved in the clipboard (and throws out
     * those things we didn't save)
     */

    // Set us back to the beginning
    SetFilePointer(fh, 0L, NULL, FILE_BEGIN);

    /* Under NT, the save filename will be used to get the file back */
    lstrcpy(szSaveFileName, szFileName);

    PINFO(TEXT("sAVEcLIPBOARDdATA: Copied name %s to name %s\r\n"), szSaveFileName, szFileName);
    fNTReadFileFormat = fNTSaveFileFormat;

    if (dwRet == NO_ERROR) //bRet)
        {
        //bRet = !ReadClipboardFromFile(hwndApp, fh);
        switch (ReadClipboardFromFile (hwndApp, fh))
            {
            case READFILE_IMPROPERFORMAT:
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                break;
            case READFILE_OPENCLIPBRDFAIL:
                dwRet = ERROR_CLIPBOARD_NOT_OPEN;
                break;
            case READFILE_SUCCESS:
            default:
                dwRet = NO_ERROR;
                break;
            }
        }

    CloseHandle(fh);

    if (dwRet != NO_ERROR)
        {
        PERROR(TEXT("SCD: Trouble in ReadClipboardFromFile\r\n"));
        DeleteFile(szFileName);
        }




done:

    if (sa.lpSecurityDescriptor)
        {
        GlobalFree((HGLOBAL)sa.lpSecurityDescriptor);
        }


    SyncCloseClipboard();


    return dwRet;

}




/*
 *      SaveClipboardToFile() -
 *  Parameters:
 *     hwnd - Passed to SaveClipboardData
 *     szShareName - Clipbook page name
 *     szFileName  - Filename to save to
 *     fPage - TRUE if this is a clbook page, FALSE if a file saved
 *        by the user.
 *
 *  Returns: NO_ERROR if no error occured otherwise an error code.
 *
 */

DWORD SaveClipboardToFile(
    HWND    hwnd,
    TCHAR   *szShareName,
    TCHAR   *szFileName,
    BOOL    fPage)
{
DWORD   dwErr = NO_ERROR;

    PINFO(TEXT("\r\n Entering SaveClipboardToFile\r\n"));
    if (fPage)
        {
        AddNetInfoToClipboard( szShareName );
        AddPreviewFormat();
        }


    dwErr = SaveClipboardData(hwnd, szFileName, fPage);

    if (dwErr != NO_ERROR)
        {
        /* If Failure, Delete the incomplete file */
        PERROR(TEXT("SaveClipboardData failed!"));
        DeleteFile(szSaveFileName);
        }

    return dwErr;

}









/*
 *      AddPreviewFormat
 */

BOOL AddPreviewFormat (VOID)
{
LPMETAFILEPICT  lpMF;
HANDLE          hClpData;
HANDLE          hBmpData;
HBITMAP         hBitmap;
HBITMAP         hClpBmp;
HBITMAP         hOldDstBmp;
HBITMAP         hOldSrcBmp;
BITMAP          Bitmap;
HDC             hDC;
HDC             hDstDC;
HDC             hSrcDC;
LPBYTE          lpBmp;
int             ret = FALSE;
RECT            rc;
int             OldMode;



    if (!IsClipboardFormatAvailable(CF_TEXT)         &&
        !IsClipboardFormatAvailable(CF_BITMAP)       &&
        !IsClipboardFormatAvailable(CF_METAFILEPICT) &&
        !IsClipboardFormatAvailable(CF_ENHMETAFILE)  &&
        !IsClipboardFormatAvailable(CF_UNICODETEXT))
        return FALSE;


    if ( !SyncOpenClipboard(hwndApp))
        return FALSE;


    if ( !(hBmpData = GlobalAlloc ( GHND, 64 * 64 / 8 )) )
        {
        SyncCloseClipboard();
        return FALSE;
        }


    hDC = GetDC ( hwndApp );
    hDstDC = CreateCompatibleDC ( hDC );
    hSrcDC = CreateCompatibleDC ( hDC );
    ReleaseDC ( hwndApp, hDC );

    if ( !( hBitmap = CreateBitmap ( 64, 64, 1, 1, NULL )) )
        PERROR (TEXT("CreateBitmap failed\n\r"));


    hOldDstBmp = SelectObject ( hDstDC, hBitmap );

    rc.top = rc.left = 0;
    rc.bottom = rc.right = 64;


    PatBlt ( hDstDC, 0, 0, 64, 64, WHITENESS );



    if (IsClipboardFormatAvailable(CF_ENHMETAFILE))
        {
        HENHMETAFILE hemf;
        ENHMETAHEADER enheader;

        if (hemf = (HENHMETAFILE)GetClipboardData(CF_ENHMETAFILE))
            {
            GetEnhMetaFileHeader(hemf, sizeof(enheader), &enheader);

            SaveDC(hDstDC);
            SetMapMode( hDstDC, MM_ISOTROPIC);
            SetViewportExtEx(hDstDC, 64, 64, NULL);
            SetWindowExtEx(hDstDC, enheader.rclBounds.right, enheader.rclBounds.bottom, NULL);
            PlayEnhMetaFile(hDstDC, hemf, (LPRECT)&enheader.rclBounds);
            RestoreDC(hDstDC, -1);
            }
        else
            {
            PERROR(TEXT("GetClipboardData fail on CF_ENHMETAFILE\r\n"));
            }
        }
    else if ( IsClipboardFormatAvailable ( CF_METAFILEPICT ))
        {
        if ( hClpData = GetClipboardData ( CF_METAFILEPICT ))
            {
            if ( lpMF = (LPMETAFILEPICT)GlobalLock ( hClpData ) )
                {
                SaveDC(hDstDC);
                SetMapMode( hDstDC, lpMF->mm);
                if ( lpMF->xExt >= lpMF->yExt )
                    {
                    SetViewportExtEx( hDstDC, 64,
                       (int)((64L*(LONG)lpMF->yExt)/(LONG)lpMF->xExt), NULL);
                    SetViewportOrgEx ( hDstDC, 0,
                       (64 - (int)((64L*(LONG)lpMF->yExt)/(LONG)lpMF->xExt))
                       / 2, NULL );
                    }
                else
                    {
                    SetViewportExtEx( hDstDC,
                       (int)((64L*(LONG)lpMF->xExt)/(LONG)lpMF->yExt),64, NULL);
                    SetViewportOrgEx( hDstDC,
                       ( 64 - (int)((64L*(LONG)lpMF->xExt)/(LONG)lpMF->yExt))
                       / 2, 0, NULL);
                    }
                if ( !PlayMetaFile ( hDstDC, lpMF->hMF ))
                    PERROR(TEXT("playmetafile failed\n\r"));
                GlobalUnlock ( hClpData );
                RestoreDC( hDstDC, -1 );
                }
            else
               PERROR(TEXT("couldn't LOCK it though...\n\r"));
            }
        else
           PERROR(TEXT("couldn't GET it though...\n\r"));
        }
    else if ( IsClipboardFormatAvailable ( CF_BITMAP ))
        {
        if ( hClpBmp = GetClipboardData ( CF_BITMAP ))
            {
            GetObject ( hClpBmp, sizeof(BITMAP), &Bitmap );
            hOldSrcBmp = SelectObject ( hSrcDC, hClpBmp );
            OldMode = SetStretchBltMode ( hDstDC, COLORONCOLOR);
            StretchBlt ( hDstDC, 0, 0, 64, 64,
                     hSrcDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
                     SRCCOPY );
            SetStretchBltMode ( hDstDC, OldMode );
            SelectObject ( hSrcDC, hOldSrcBmp );
            }
        }
    else if ( IsClipboardFormatAvailable ( CF_TEXT ))
        {
        LPSTR lpText;
        HFONT hSmallFont, hOldFont;

        if ( hClpData = GetClipboardData ( CF_TEXT ))
            {
            lpText = (LPSTR)GlobalLock ( hClpData );
            FillRect ( hDstDC, &rc, GetStockObject ( WHITE_BRUSH ) );
            hSmallFont = CreateFont( -6,
               0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
               VARIABLE_PITCH | FF_SWISS, TEXT("Small Fonts")
               );
            hOldFont = SelectObject ( hDstDC, hSmallFont );
            DrawTextA( hDstDC,lpText, lstrlenA(lpText),
               &rc, DT_LEFT);
            SelectObject ( hDstDC, hOldFont );
            DeleteObject ( hSmallFont );
            GlobalUnlock ( hClpData );
            }
        }
    else if ( IsClipboardFormatAvailable (CF_UNICODETEXT))
        {
        LPWSTR lpText;
        HFONT hSmallFont, hOldFont;

        if ( hClpData = GetClipboardData ( CF_UNICODETEXT ))
            {
            lpText = (LPWSTR)GlobalLock ( hClpData );
            FillRect ( hDstDC, &rc, GetStockObject ( WHITE_BRUSH ) );
            hSmallFont = CreateFont( -6,
               0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
               VARIABLE_PITCH | FF_SWISS, TEXT("Small Fonts")
               );
            hOldFont = SelectObject ( hDstDC, hSmallFont );
            DrawTextW( hDstDC,lpText, lstrlenW(lpText),
               &rc, DT_LEFT);
            SelectObject ( hDstDC, hOldFont );
            DeleteObject ( hSmallFont );
            GlobalUnlock ( hClpData );
            }
        }

    SelectObject ( hDstDC, hOldDstBmp );
    DeleteDC ( hDstDC );
    DeleteDC ( hSrcDC );

    lpBmp = GlobalLock ( hBmpData );

    if ( GetBitmapBits ( hBitmap, 64 * 64 / 8, lpBmp ) != 64*64/8 )
//      if ( GetBitmapBits ( hBitmap, 64 * 64 / 2, lpBmp ) != 64*64/2 )
        PERROR(TEXT("GetBitmapBits failed\n\r"));

    GlobalUnlock ( hBmpData );

    SetClipboardData ( cf_preview, hBmpData );
    ret = TRUE;

    DeleteObject ( hBitmap );
    SyncCloseClipboard();


    return ret;

}









/*
 *      AddCopiedFormat
 */

BOOL AddCopiedFormat (
    UINT    ufmtOriginal,
    UINT    ufmtCopy)
{
LPBYTE  lpOriginal;
LPBYTE  lpCopy;
HANDLE  hOriginal;
HANDLE  hCopy = NULL;
BOOL    ret = FALSE;
int     i;


    if (IsClipboardFormatAvailable(ufmtOriginal) && SyncOpenClipboard(hwndApp))
        {
        if ( hOriginal = GetClipboardData(ufmtOriginal))
            {
            if ( hCopy = GlobalAlloc( GHND, GlobalSize(hOriginal)))
                {
                if ( lpOriginal = GlobalLock(hOriginal))
                    {
                    if ( lpCopy = GlobalLock (hCopy))
                        {

                        for ( i=(int)GlobalSize(hOriginal); i--; )
                            *lpCopy++ = *lpOriginal++;
                        GlobalUnlock(hCopy);

                        #ifdef DEBUG
                         lpCopy = GlobalLock(hCopy);
                         GlobalUnlock(hCopy);
                        #endif

                        ret = ( SetClipboardData ( ufmtCopy, hCopy ) != NULL );
                        }
                    GlobalUnlock(hOriginal);
                    }
                }
            }
        SyncCloseClipboard();
        }


    if ( !ret )
        {
        PERROR(TEXT("AddCopiedFormat returning FALSE!\n\r"));
        if ( hCopy )
            GlobalFree (hCopy);
        }

    return ret;


}







/*
 *      AddNetInfoToClipboard
 */

BOOL AddNetInfoToClipboard (
    TCHAR   *szShareName )
{
HANDLE  hData;
HANDLE  hNewData;
TCHAR   szServerName[MAX_COMPUTERNAME_LENGTH + 1];
DWORD   dwNameLen;
LPTSTR  src;
LPTSTR  dst;



    cf_link           = RegisterClipboardFormat (SZLINK);
    cf_linkcopy       = RegisterClipboardFormat (SZLINKCOPY);
    cf_objectlink     = RegisterClipboardFormat (SZOBJECTLINK);
    cf_objectlinkcopy = RegisterClipboardFormat (SZOBJECTLINKCOPY);



    // check to see if this info already added:
    if (IsClipboardFormatAvailable (cf_linkcopy))
        {
        PINFO(TEXT("AddNetInfo: Already added\n\r"));
        return FALSE;
        }




    if (IsClipboardFormatAvailable (cf_link))
        {
        AddCopiedFormat (cf_link, cf_linkcopy);

        if (!SyncOpenClipboard (hwndApp))
           return (FALSE);

        dwNameLen = MAX_COMPUTERNAME_LENGTH+1;
        GetComputerName (szServerName, &dwNameLen);

        PINFO(TEXT("link data found\n\r"));


        if (hData = GetClipboardData (cf_link))
            {
            if (src = GlobalLock (hData))
                {
                hNewData = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        GlobalSize(hData)
                                        +lstrlen (szServerName)
                                        +lstrlen (szShareName)
                                        +3);

                dst = GlobalLock (hNewData);

                wsprintf (dst, TEXT("\\\\%s\\%s"), szServerName, TEXT("NDDE$"));
                dst += lstrlen(dst) + 1;

                lstrcpy ( dst, szShareName );
                *dst = SHR_CHAR;
                lstrcat ( dst, TEXT(".dde") );
                //lstrcat (dst, TEXT(".ole"));
                dst += lstrlen(dst) + 1;

                src += lstrlen(src) + 1;
                src += lstrlen(src) + 1;

                lstrcpy ( dst, src );

                GlobalUnlock (hData);
                GlobalUnlock (hNewData);

                SetClipboardData (cf_link, hNewData);
                }
            }

        SyncCloseClipboard ();
        }



    if (IsClipboardFormatAvailable (cf_objectlink))
        {
        AddCopiedFormat (cf_objectlink, cf_objectlinkcopy);

        if (!SyncOpenClipboard (hwndApp))
            return (FALSE);

        dwNameLen = MAX_COMPUTERNAME_LENGTH+1;
        GetComputerName (szServerName, &dwNameLen);

        PINFO(TEXT("objectlink data found\n\r"));

        if (hData = GetClipboardData (cf_objectlink))
            {
            if (src = GlobalLock (hData))
                {
                hNewData = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        GlobalSize (hData)
                                        +lstrlen (szServerName)
                                        +lstrlen (szShareName)
                                        +3);

                dst = GlobalLock (hNewData);

                wsprintf(dst, TEXT("\\\\%s\\%s"), szServerName, TEXT("NDDE$"));
                dst += lstrlen(dst) + 1;

                lstrcpy (dst, szShareName);

                *dst = SHR_CHAR;
                lstrcat (dst, TEXT(".ole"));
                dst += lstrlen(dst) + 1;

                src += lstrlen(src) + 1;
                src += lstrlen(src) + 1;

                lstrcpy (dst, src);

                GlobalUnlock (hData);
                GlobalUnlock (hNewData);

                SetClipboardData (cf_objectlink, hNewData);
                }
            }

        SyncCloseClipboard ();
        }


    return TRUE;

}
