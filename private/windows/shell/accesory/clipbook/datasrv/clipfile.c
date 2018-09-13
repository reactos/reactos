/*
 * CLIPFILE.C - Windows Clipboard File I/O Routines
 */

/* NOTE:
 *   When saving the contents of the clipboard we SetClipboardData(fmt, NULL)
 *   to free up the memory associated with each clipboard format.  Then
 *   after we are done saving we take over as the clipboard owner.  This
 *   causes OWNERDRAW formats to be lost in the save process.
 */

#include "clipshr.h"
#include "clipsrv.h"
#include <string.h>
#include <windowsx.h>
#include "..\common\dib.h"
#include "..\common\common.h"

BOOL fAnythingToRender;

TCHAR       szFileSpecifier[] = TEXT("*.CLP");
TCHAR       szFileName[MAX_PATH];
TCHAR       szSaveFileName[MAX_PATH]; // Saved filename for delayed render

BOOL        fNTReadFileFormat;
BOOL        fNTSaveFileFormat;

extern TCHAR szCaptionName[];

UINT cf_link;
UINT cf_objectlink;
UINT cf_linkcopy;
UINT cf_objectlinkcopy;

extern HANDLE RenderFormat(FORMATHEADER *, register HANDLE);

// winball additions
extern BOOL AddNetInfoToClipboard ( TCHAR * );
extern BOOL AddPreviewFormat ( VOID );
extern BOOL AddCopiedFormat ( UINT ufmtOriginal, UINT ufmtCopy );
BOOL AddDIBtoDDB(VOID);
// end winball

/*-----------------------------------------------------------------------
 * File read routines
 *-----------------------------------------------------------------------*/

//
// Purpose: Read the file header in the given .clp file, and get the number
//    of formats. Also sets the fNTReadFileFormat flag appropriately.
//
// Parameters:
//    fh - Handle to the file.
//
// Returns:
//    The number of formats, or 0 if it isn't a valid .clp file.
//
unsigned ReadFileHeader(
HANDLE fh)
{
FILEHEADER FileHeader;
DWORD dwBytesRead;

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

BOOL ReadFormatHeader(
HANDLE fh,
FORMATHEADER *pfh,
unsigned iFormat)
{
DWORD dwMrPibb;
OLDFORMATHEADER OldFormatHeader;

// PINFO(TEXT("ClSrv\\RdFmtHdr"));

if (NULL == pfh || NULL == fh)
   {
   PERROR("RdFmtHdr got NULL pointer\r\n");
   return FALSE;
   }

SetFilePointer(fh, sizeof(FILEHEADER) + iFormat *
      (fNTReadFileFormat ? sizeof(FORMATHEADER) : sizeof(OLDFORMATHEADER)),
      NULL, FILE_BEGIN);

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

   pfh->FormatID = OldFormatHeader.FormatID;
   pfh->DataLen = OldFormatHeader.DataLen;
   pfh->DataOffset = OldFormatHeader.DataOffset;
   MultiByteToWideChar(
         CP_ACP, MB_PRECOMPOSED, OldFormatHeader.Name, -1,
         pfh->Name, CCHFMTNAMEMAX);
   }

// PINFO(TEXT("\r\n"));
return TRUE;
}

/*
 *  ReadClipboardFromFile()
 *
 * Read in a clipboard file and register all the formats in delayed mode.
 * to render things for real reopen the file specified by ofStruct.
 *
 * NOTE:
 *    This makes us the clipboard owner.
 *
 * Bug 14564:  Changed return value to a short integer noting why the
 * reading failed.
 * Return Value:  0  Success
 *                1  Improper format
 *                2  SyncOpenClipboard failed
 */
#define READFILE_SUCCESS         0
#define READFILE_IMPROPERFORMAT  1
#define READFILE_OPENCLIPBRDFAIL 2

short ReadClipboardFromFile(
HWND hwnd,
HANDLE fh)
{
register unsigned i;
unsigned          cFormats;
FORMATHEADER      FormatHeader;
OLDFORMATHEADER   OldFormatHeader;
// DWORD             dwMrPibb; // Dummy var used for ReadFile

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
   ReadFormatHeader(fh, &FormatHeader, i);

   if (PRIVATE_FORMAT(FormatHeader.FormatID))
      {
      FormatHeader.FormatID = RegisterClipboardFormatW((LPWSTR)FormatHeader.Name);
      }

   /*Delayed Render. */
   PINFO(TEXT("Set up delayed render for format %d .\r\n"), FormatHeader.FormatID);
   SetClipboardData(FormatHeader.FormatID, NULL);
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


BOOL OpenClipboardFile(
HWND hwnd,
LPTSTR szName)
{
HANDLE     fh;
BOOL nReadError = TRUE;

PINFO(TEXT("OpenClipboardFile: %s \r\n"),szName);

fh = CreateFile((LPCTSTR)szName, GENERIC_READ,
     FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

if (fh != INVALID_HANDLE_VALUE)
   {
   // Store file name for delayed rendering stuff.
   lstrcpy(szSaveFileName, szName);

   // Read the sucker.
   nReadError = ReadClipboardFromFile(hwnd, fh);
   CloseHandle(fh);
   }
else
   {
   PERROR(TEXT("ClSrv\\OpClpFile: can't open file!"));
   }

return !nReadError;
}


//
// Purpose: Go get the given format from the given file.
//
/////////////////////////////////////////////////////////////////
HANDLE RenderFormatFromFile(
LPTSTR szFile,
WORD   wFormat)
{
HANDLE          fh;
FORMATHEADER    FormatHeader;
HANDLE          hData = NULL;
DWORD           dwBytesRead; // Dummy var for ReadFile
unsigned        cFormats;
unsigned        i;

PINFO(TEXT("ClSrv\\RndrFmtFromFile: Opening file %s.\r\n"),szSaveFileName);

fh = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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
         FormatHeader.FormatID = RegisterClipboardFormatW(FormatHeader.Name);
         }
      if (FormatHeader.FormatID == wFormat)
         {
         hData = RenderFormat(&FormatHeader, fh);
         }
      }
   CloseHandle(fh);
   }
else
   {
   PERROR(TEXT("Can't open file\r\n"));
   }
return(hData);
}


//
// Purpose: Go get all formats from the given file.
//
/////////////////////////////////////////////////////////////////
HANDLE RenderAllFromFile(
LPTSTR szFile)
{
HANDLE          fh;
FORMATHEADER    FormatHeader;
HANDLE          hData;
DWORD           dwBytesRead; // Dummy var for ReadFile
unsigned        cFormats;
unsigned        i;

/* Check if the clipbrd viewer has done any File I/O before.
 * If it has not, then it has nothing to render!  Sankar
 */
if (CountClipboardFormats() && fAnythingToRender)
   {
   /* Empty the clipboard */
   if (!SyncOpenClipboard(hwndServer))
      {
      PERROR("Couldn't open clipboard!\r\n");
      }
   else
      {
      EmptyClipboard();

      fh = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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


/*-------------------------------------------------------------------------
 * File write routines
 *-------------------------------------------------------------------------*/
/*
 *  IsWriteable()
 *
 * Test if a clipboard format is writeable(i.e. if it makes sense to write it)
 * OWNERDRAW and others can't be written because we (CLIPBRD) will become the
 * owner when the files are reopened.
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
* Purpose: Writes the format header for a single data format.
*
* Parameters:
*  fh - File handle to write to.
*  offset - Position in the file to write the format block.
*  DataOffset - Position in the file where the data for this format will be.
*  DataLen    - Length of the data for this format.
*  Format     - The format number.
*  szName     - Name of the format.
*
* Returns:
*  The number of bytes written to the file.
*/
DWORD WriteFormatBlock(
HANDLE fh,
DWORD offset,
DWORD DataOffset,
DWORD DataLen,
UINT Format,
LPWSTR wszName)
{
DWORD           dwBytesWritten = 0;

SetFilePointer(fh, offset, NULL, FILE_BEGIN);

if (fNTSaveFileFormat)
   {
   FORMATHEADER    FormatHeader;

   memset(&FormatHeader,0, sizeof(FormatHeader));

   FormatHeader.FormatID = Format;
   FormatHeader.DataLen = DataLen;
   FormatHeader.DataOffset = DataOffset;
   lstrcpyW(FormatHeader.Name, wszName);
   WriteFile(fh, &FormatHeader, sizeof(FormatHeader), &dwBytesWritten, NULL);
   }
else
   {
   OLDFORMATHEADER OldFormatHeader;

   memset(&OldFormatHeader,0, sizeof(OldFormatHeader));

   OldFormatHeader.FormatID = Format;
   OldFormatHeader.DataLen = DataLen;
   OldFormatHeader.DataOffset = DataOffset;
   WideCharToMultiByte(CP_ACP, 0, wszName, -1,
         OldFormatHeader.Name, CCHFMTNAMEMAX, NULL, NULL);
   WriteFile(fh, &OldFormatHeader, sizeof(OldFormatHeader),
         &dwBytesWritten, NULL);
   }
return(dwBytesWritten);
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
DWORD WriteDataBlock(
register HANDLE hFile,
DWORD offset,
WORD Format)
{
WORD            wPalEntries;
LPSTR           lpData;
DWORD           dwSize = 0;
BITMAP          bitmap;
HANDLE          hMF;
HANDLE          hBitmap;
HANDLE          hLogPalette;
register HANDLE hData;
LPLOGPALETTE    lpLogPalette;
LPMETAFILEPICT  lpMFP;
DWORD           dwMFPSize;
BOOL            fOK = FALSE;

if (!(hData = GetClipboardData(Format)) ||
    SetFilePointer(hFile, offset, NULL, FILE_BEGIN) != offset)
   {
   PERROR(TEXT("WriteDataBlock: couldn't get format data\n\r"));
   }
else
   {

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

            w31mfp.mm = lpMFP->mm;
            w31mfp.xExt = lpMFP->xExt;
            w31mfp.yExt = lpMFP->yExt;
            w31mfp.hMF  = 0;

            WriteFile(hFile, &w31mfp, sizeof(WIN31METAFILEPICT),
                &dwMFPSize, NULL);
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
                  WriteFile(hFile, lpData, dwSize,
                       &dwSize, NULL);

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

      if (hBitmap = DibFromBitmap((HBITMAP)hData, BI_RGB,
               bitmap.bmBitsPixel * bitmap.bmPlanes,
               IsClipboardFormatAvailable(CF_PALETTE) ?
                  GetClipboardData(CF_PALETTE) : NULL))
         {
         if (lpData = GlobalLock(hBitmap))
            {
            // dwSize might be too big, but we can live with that.
            dwSize = GlobalSize(lpData);

            WriteFile(hFile, lpData, dwSize,
                &dwSize, NULL);

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
      dwSize = GlobalSize(hData);

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
   }

/* Return the number of bytes written. */
return(dwSize);
}


/*--------------------------------------------------------------------------*/
/*                               */
/*  SendOwnerMessage() -                      */
/*                               */
/*--------------------------------------------------------------------------*/
void SendOwnerMessage(
UINT message,
WPARAM wParam,
LPARAM lParam)
{
register HWND hwndOwner;

/* Send a message to the clipboard owner, if there is one */
hwndOwner = GetClipboardOwner();

if (hwndOwner != NULL)
    SendMessage(hwndOwner, message, wParam, lParam);
}


/*--------------------------------------------------------------------------*/
/*                               */
/*  GetClipboardNameW() -                      */
/*                               */
/*--------------------------------------------------------------------------*/
void GetClipboardNameW(
register int fmt,
LPWSTR wszName,
register int iSize)
{
LPWSTR  lprgch;
HANDLE  hrgch;

*wszName = '\0';

/* Get global memory that everyone can get to */
if (hrgch = GlobalAlloc(GMEM_MOVEABLE, (LONG)(iSize + 1)*sizeof(WCHAR)))
   {
   if ((lprgch = (LPWSTR)GlobalLock(hrgch)))
      {
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

      GlobalUnlock(hrgch);
      }
   else
      {
      PERROR(TEXT("GetClipboardNameW: bad lock\r\n"));
      }
   GlobalFree(hrgch);
   }
else
   {
   PERROR(TEXT("GetClipboardNameW: bad alloc\r\n"));
   }
}

extern PSECURITY_DESCRIPTOR CurrentUserOnlySD(void);

/*
 *  SaveClipboardData() - Writes a clipboard file.
 *
 * In:
 *    hwnd        handle of wnd that becomes the clipboard owner
 *    szFileName  file handle to read from
 *    fPage       TRUE if this is a clipbook page (which means we secure it)
 *
 * NOTE:
 *    When done we call ReadClipboardFromFile(). this makes us the
 *    clipboard owner.
 */
BOOL SaveClipboardData(
HWND   hwnd,
LPTSTR szFileName,
BOOL   fPage)
{
register HANDLE  fh;
register WORD   Format;
DWORD     HeaderPos;
DWORD     DataPos;
DWORD     datasize;
BOOL      fComplain = TRUE;
HCURSOR   hCursor;
FILEHEADER FileHeader;
// Must be WCHAR... it be format name!
WCHAR     wszName[CCHFMTNAMEMAX];
UINT      wHeaderSize;
UINT      uiSizeHeaderToWrite;
BOOL      fDIBUsed = FALSE;
DWORD     dwQuaker100PercentNatural; // What I was eating when I created this meaningless temp variable
SECURITY_ATTRIBUTES sa;
PSECURITY_DESCRIPTOR pSDUser;


/* First open the clipboard */
if (SyncOpenClipboard(hwndServer))
   {
   sa.nLength = sizeof(sa);
   sa.lpSecurityDescriptor = (fPage ? CurrentUserOnlySD() : NULL);
   sa.bInheritHandle = FALSE;

   fh = CreateFile((LPCTSTR)szFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

   if (sa.lpSecurityDescriptor)
      {
      GlobalFree((HGLOBAL)sa.lpSecurityDescriptor);
      }

   if (fh != INVALID_HANDLE_VALUE)
      {
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
         DataPos = HeaderPos + (uiSizeHeaderToWrite *
               CountClipboardFormats());
         }
      else
         {
         DataPos = HeaderPos + (uiSizeHeaderToWrite *
               Count16BitClipboardFormats());
         }

      /* Now loop throught the data, one format at a time, and write out the data. */
      hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      ShowCursor(TRUE);
      fComplain = FALSE;

      /* Enumerate all formats */
      Format = 0;
      while (Format = EnumClipboardFormats(Format))
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

             GetClipboardNameW(Format == CF_BITMAP ? CF_DIB : Format,
                   wszName, sizeof(wszName));

             PINFO(TEXT("SClipboardData: writing %ls (#)%d\r\n"),
                  wszName,Format);

             if (datasize = WriteDataBlock(fh, DataPos, Format))
                {
                /* Create a Format header and write it to the file */
                wHeaderSize = (WORD)WriteFormatBlock(
                     fh,
                     HeaderPos,
                     DataPos, datasize,
                     Format == CF_BITMAP ? CF_DIB : Format,
                     wszName);

                if (wHeaderSize < uiSizeHeaderToWrite)
                   {
                   PERROR(TEXT("SaveClipboardData: error writing format block\n\r"));
                   fComplain = TRUE;
                   break;
                   }
                HeaderPos += wHeaderSize;

                /* Update the data pos for the next block */
                DataPos += datasize;

                FileHeader.FormatCount++;   /* this format has been written */
                }
             else
                {
                PERROR(TEXT("SaveClipboardData: error writing data block\n\r"));
                fComplain = TRUE;
                break;
                }
             }
          }

      ShowCursor(FALSE);
      SetCursor(hCursor);

      SyncCloseClipboard();      /* we are done looking at this */

      // Go back and write the file header at the front of the file
      SetFilePointer(fh, 0L, NULL, FILE_BEGIN);
      WriteFile(fh, &FileHeader, sizeof(FileHeader),
           &dwQuaker100PercentNatural, NULL);

      /* Now we open the clipboard and become the owner.  this places
       * all the things we just saved in the clipboard (and throws out
       * those things we didn't save)
       */

      // Set us back to the beginning
      SetFilePointer(fh, 0L, NULL, FILE_BEGIN);

      /* Under NT, the save filename will be used to get the file back */
      lstrcpy(szSaveFileName, szFileName);

      PINFO(TEXT("sAVEcLIPBOARDdATA: Copied name %s to name %s\r\n"),
            szSaveFileName, szFileName);
      fNTReadFileFormat = fNTSaveFileFormat;

      if (!fComplain)
         {
         fComplain = ReadClipboardFromFile(hwndServer, fh);
         }

      CloseHandle(fh);

      if (fComplain)
         {
         PERROR(TEXT("SCD: Trouble in ReadClipboardFromFile\r\n"));
         DeleteFile(szFileName);
         }
      }
   else
      {
      PERROR ("Error opening clipboard file!\r\n");
      GetLastError();
      }
   SyncCloseClipboard();
   }
return !fComplain;
}

/*
 *  SaveClipboardToFile() -
 *    Parameters:
 *       hwnd - Passed to SaveClipboardData
 *       szShareName - Clipbook page name
 *       szFileName  - Filename to save to
 *       fPage - TRUE if this is a clbook page, FALSE if a file saved
 *          by the user.
 *
 *    Returns: TRUE on success, FALSE on failure.
 *
 */
BOOL SaveClipboardToFile(
HWND hwnd,
TCHAR *szShareName,
TCHAR *szFileName,
BOOL  fPage)
{

PINFO(TEXT("\r\n Entering SaveClipboardToFile\r\n"));
if (fPage)
   {
   AddNetInfoToClipboard( szShareName );
   AddPreviewFormat();
   }

if (!SaveClipboardData(hwnd, szFileName, fPage))
   {
   /* If Failure, Delete the incomplete file */
   PERROR(TEXT("SaveClipboardData failed!"));
   DeleteFile(szSaveFileName);
   return FALSE;
   }
return TRUE;
}


BOOL AddPreviewFormat (
VOID)
{
HANDLE hClpData, hBmpData;
HBITMAP hBitmap, hClpBmp, hOldDstBmp, hOldSrcBmp;
LPMETAFILEPICT lpMF;
BITMAP Bitmap;
HDC hDC, hDstDC, hSrcDC;
LPBYTE lpClpData, lpBmp;
int ret = FALSE;
RECT rc;
int OldMode;

if (IsClipboardFormatAvailable(CF_TEXT)         ||
    IsClipboardFormatAvailable(CF_BITMAP)       ||
    IsClipboardFormatAvailable(CF_METAFILEPICT) ||
    IsClipboardFormatAvailable(CF_ENHMETAFILE)  ||
    IsClipboardFormatAvailable(CF_UNICODETEXT)  )
   {
   if ( !SyncOpenClipboard(hwndServer))
      return FALSE;

   if ( !(hBmpData = GlobalAlloc ( GHND, 64 * 64 / 8 )) )
      {
      SyncCloseClipboard();
      return FALSE;
      }

   hDC = GetDC ( hwndServer );
   hDstDC = CreateCompatibleDC ( hDC );
   hSrcDC = CreateCompatibleDC ( hDC );
   ReleaseDC ( hwndServer, hDC );

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
   }
return ret;
}

BOOL AddCopiedFormat (
UINT ufmtOriginal,
UINT ufmtCopy )
{
LPBYTE lpOriginal, lpCopy;
HANDLE hOriginal, hCopy = NULL;
BOOL ret = FALSE;
int i;


if (IsClipboardFormatAvailable(ufmtOriginal) && SyncOpenClipboard(hwndServer)) {
   if ( hOriginal = GetClipboardData(ufmtOriginal)) {
      if ( hCopy = GlobalAlloc( GHND, GlobalSize(hOriginal))) {
         if ( lpOriginal = GlobalLock(hOriginal)) {
            if ( lpCopy = GlobalLock (hCopy)) {
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
if ( !ret ) {
   PERROR(TEXT("AddCopiedFormat returning FALSE!\n\r"));
   if ( hCopy )
      GlobalFree ( hCopy );
}
return ret;
}


BOOL AddNetInfoToClipboard (
TCHAR *szShareName )
{
HANDLE hData, hNewData;
TCHAR szServerName[MAX_COMPUTERNAME_LENGTH + 1];
DWORD dwNameLen;
LPTSTR src,dst;

cf_link = RegisterClipboardFormat ( SZLINK );
cf_objectlink = RegisterClipboardFormat ( SZOBJECTLINK );
cf_linkcopy = RegisterClipboardFormat ( SZLINKCOPY );
cf_objectlinkcopy = RegisterClipboardFormat ( SZOBJECTLINKCOPY );

// check to see if this info already added:
if ( IsClipboardFormatAvailable ( cf_linkcopy ) )
   {
   PINFO(TEXT("AddNetInfo: Already added\n\r"));
   return FALSE;
   }
else if ( IsClipboardFormatAvailable ( cf_link ) )
   {
   AddCopiedFormat ( cf_link, cf_linkcopy );

   if (!SyncOpenClipboard(hwndServer))
      return(FALSE);

   dwNameLen = MAX_COMPUTERNAME_LENGTH+1;
   GetComputerName(szServerName, &dwNameLen);

   PINFO(TEXT("link data found\n\r"));
   hData = GetClipboardData ( cf_link );

   src = GlobalLock ( hData );

   hNewData = GlobalAlloc ( GMEM_MOVEABLE | GMEM_ZEROINIT,
      GlobalSize( hData )
      + lstrlen ( szServerName ) + lstrlen(szShareName) + 3 );

   dst = GlobalLock ( hNewData );

   wsprintf( dst, TEXT("\\\\%s\\%s"), szServerName, TEXT("NDDE$")  );
   dst += lstrlen(dst) + 1;

   lstrcpy ( dst, szShareName );
   *dst = SHR_CHAR;
   lstrcat ( dst, TEXT(".dde") );
   // lstrcat ( dst, TEXT(".ole") );
   dst += lstrlen(dst) + 1;

   src += lstrlen(src) + 1;
   src += lstrlen(src) + 1;

   lstrcpy ( dst, src );

   GlobalUnlock ( hData );
   GlobalUnlock ( hNewData );

   SetClipboardData ( cf_link, hNewData );
   SyncCloseClipboard();
   }

if ( IsClipboardFormatAvailable ( cf_objectlink ) )
   {
   AddCopiedFormat ( cf_objectlink, cf_objectlinkcopy );

   if (!SyncOpenClipboard(hwndServer))
      return(FALSE);

   dwNameLen = MAX_COMPUTERNAME_LENGTH+1;
   GetComputerName(szServerName, &dwNameLen);

   PINFO(TEXT("objectlink data found\n\r"));
   hData = GetClipboardData ( cf_objectlink );

   src = GlobalLock ( hData );

   hNewData = GlobalAlloc ( GMEM_MOVEABLE | GMEM_ZEROINIT,
      GlobalSize( hData )
      + lstrlen(szServerName) + lstrlen(szShareName) + 3 );

   dst = GlobalLock ( hNewData );

   wsprintf( dst, TEXT("\\\\%s\\%s"), szServerName, TEXT("NDDE$")  );
   dst += lstrlen(dst) + 1;

   lstrcpy ( dst, szShareName );
   *dst = SHR_CHAR;
   lstrcat ( dst, TEXT(".ole") );
   dst += lstrlen(dst) + 1;

   src += lstrlen(src) + 1;
   src += lstrlen(src) + 1;

   lstrcpy ( dst, src );

   GlobalUnlock ( hData );
   GlobalUnlock ( hNewData );

   SetClipboardData ( cf_objectlink, hNewData );
   SyncCloseClipboard();
   }
return TRUE;
}
