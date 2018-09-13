//
// loadimag.cpp
//
// implementation of loading a file from disk via an installed graphic filter
//

#include <windows.h>
#include <windowsx.h>
#include "stdafx.h"
#include "pbrush.h"
#include "imgwnd.h"
#include "imgsuprt.h"
#include "loadimag.h"

// must define one of the following:
//#define _USE_FLT_API
#ifdef _X86_
#define _USE_IFL_API
#endif

#ifdef _USE_FLT_API
#include "filtapi.h"
#endif

#ifdef _USE_IFL_API
#include "image.h"
#include "interlac.h"
#define MAX_PAL_SIZE 256

#ifdef PNG_SUPPORT // for Portable Network Graphics. As of 12/10/1996 the support was broken

//----------------------------------------------------------------------------
//    Places a line of image data from an ADAM 7 interlaced file (i.e., currently
//    a PNG file) into its correct position in a memory buffer: this memory
//    buffer is essentially an array of pointers to the rows of the image in
//    which the pixel data is to be set.
//----------------------------------------------------------------------------
 IFLERROR ReadADAM7InterlacedImage(LPBYTE apbImageBuffer[], IFLHANDLE pfpbFROM,
                                         int ImageHeight, int ImageWidth, int cbPixelSize,
                                         IFLCLASS ImageClass)
{


        int cRasterLines = iflGetRasterLineCount(pfpbFROM);

        ADAM7_STRUCT stAdam7;
        stAdam7.iImageHeight = ImageHeight;
        stAdam7.iImageWidth = ImageWidth;
        stAdam7.Class = ImageClass;
        stAdam7.cbPixelSize = iflGetBitsPerPixel (pfpbFROM)/8;//cbPixelSize;
        stAdam7.iPassLine = 0;
        LPBYTE pbScanLine = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, ImageWidth * stAdam7.cbPixelSize);
        wsprintf (buf, TEXT("Pixel size: %d, Size of a scan line: %d\n"), stAdam7.cbPixelSize,
                          ImageWidth*stAdam7.cbPixelSize);

        if (pbScanLine == NULL)
                return IFLERR_MEMORY;

        int cTotalScanLines = iADAM7CalculateNumberOfScanLines(&stAdam7);

        int iLine;
        IFLERROR idErr;
        BOOL fEmptyLine;

        for (iLine = 0, idErr = IFLERR_NONE, fEmptyLine = FALSE;
                idErr == IFLERR_NONE && iLine < (int)cRasterLines;
                iLine++)
        {
                if (!fEmptyLine)
                        idErr = iflRead(pfpbFROM, pbScanLine, 1);

                stAdam7.iScanLine = iLine;
                fEmptyLine = ADAM7AddRowToImageBuffer(apbImageBuffer, pbScanLine, &stAdam7);
        }

        HeapFree(GetProcessHeap(), 0, pbScanLine);
        return idErr;
}

//----------------------------------------------------------------------------
//    Deallocates the image space allocated in the function AllocateImageSpace()
//----------------------------------------------------------------------------
 LPBYTE *FreeImageSpace(HANDLE hHeap, LPBYTE ppImageSpace[], int height)
{
        if (ppImageSpace != NULL)
        {
                for (int i = 0; i < height; i++)
                {
                        if (ppImageSpace[i] != NULL)
                        {
                                HeapFree(hHeap, 0, ppImageSpace[i]);
                                ppImageSpace[i] = NULL;
                        }
                }

                HeapFree(hHeap, 0, ppImageSpace);
                ppImageSpace = NULL;
        }

        return ppImageSpace;
}

//----------------------------------------------------------------------------
//    Allocate some image space: this function will create a dynamic array
//    of "height" pointers which each point to an allocated row of bytes of
//    size "width".
//----------------------------------------------------------------------------
 LPBYTE *AllocateImageSpace(HANDLE hHeap, int height, int width, int cbSize)
{
        LPBYTE *ppImageSpace = (LPBYTE *)HeapAlloc(hHeap, 0, height * sizeof(void *));
        TCHAR buf[200];
        wsprintf (buf, TEXT("Size of image line: %d\n"), width*cbSize);


        if (ppImageSpace != NULL)
        {
                // Init the pointers to NULL: this makes error recovery easier
                for (int i = 0; i < height; i++)
                        ppImageSpace[i] = NULL;

                // NOW allocate the pointer space for the image
                for (i = 0; i < height; i++)
                {
                        ppImageSpace[i] = (LPBYTE)HeapAlloc(hHeap, 0, width * cbSize);
                        if (ppImageSpace[i] == NULL)
                                break;
                }

                if (i < height) // We weren't able to allocate the required space
                        ppImageSpace = FreeImageSpace(hHeap, ppImageSpace, height);
        }

        return ppImageSpace;
}
#endif // PNG_SUPPORT

#endif // _USE_IFL_API



// returns a pointer to the extension of a file.
//
// in:
//      qualified or unqualfied file name
//
// returns:
//      pointer to the extension of this file.  if there is no extension
//      as in "foo" we return a pointer to the NULL at the end
//      of the file
//
//      foo.txt     ==> ".txt"
//      foo         ==> ""
//      foo.        ==> "."
//

LPCTSTR FindExtension(LPCTSTR pszPath)
{
        for (LPCTSTR pszDot = NULL; *pszPath; pszPath = CharNext(pszPath))
        {
                switch (*pszPath)
                {
                        case TEXT('.'):
                                pszDot = pszPath;       // remember the last dot
                                break;
                        case TEXT('\\'):
                        case TEXT(' '):                               // extensions can't have spaces
                                pszDot = NULL;          // forget last dot, it was in a directory
                                break;
                }
        }

        // if we found the extension, return ptr to the dot, else
        // ptr to end of the string (NULL extension)
        return pszDot ? pszDot : pszPath;
}

//
// GetFilterInfo
//
//  32-bit import filters are listed in the registry...
//
//  HKLM\SOFTWARE\Microsoft\Shared Tools\Graphics Filters\Import\XXX
//      Path        = filename
//      Name        = friendly name
//      Extenstions = file extenstion list
//
#pragma data_seg(".text")
static const TCHAR c_szImpHandlerKey[] = TEXT("SOFTWARE\\Microsoft\\Shared Tools\\Graphics Filters\\Import");
static const TCHAR c_szExpHandlerKey[] = TEXT("SOFTWARE\\Microsoft\\Shared Tools\\Graphics Filters\\Export");
static const TCHAR c_szName[] = TEXT("Name");
static const TCHAR c_szPath[] = TEXT("Path");
static const TCHAR c_szExts[] = TEXT("Extensions");
static const TCHAR c_szImageAPI[] = TEXT("Image API Enabled Filters");
#pragma data_seg()

BOOL GetInstalledFilters(BOOL bOpenFileDialog, int i, LPTSTR szName, UINT cbName,
                LPTSTR szExt, UINT cbExt, LPTSTR szHandler, UINT cbHandler, BOOL& bImageAPI)
{
        HKEY hkey;
        HKEY hkeyT;
        TCHAR ach[80];
        BOOL rc = FALSE;        // return code

        bImageAPI = FALSE;

        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                bOpenFileDialog ? c_szImpHandlerKey : c_szExpHandlerKey, &hkey) == 0)
        {
                if (RegEnumKey(hkey, i, ach, sizeof(ach))==0)
                {
                        if (RegOpenKey(hkey, ach, &hkeyT) == 0)
                        {
                                if (szName)
                                {
                                        szName[0] = 0;
                                        RegQueryValueEx(hkeyT, c_szName, NULL, NULL,
                                                (LPBYTE)szName, (LPDWORD)&cbName);
                                }
                                if (szExt)
                                {
                                        szExt[0] = 0;
                                        RegQueryValueEx(hkeyT, c_szExts, NULL, NULL,
                                                (LPBYTE)szExt, (LPDWORD)&cbExt);
                                }
                                if (szHandler)
                                {
                                        szHandler[0] = 0;
                                        RegQueryValueEx(hkeyT, c_szPath, NULL, NULL,
                                                (LPBYTE)szHandler, (LPDWORD)&cbHandler);
                                }

                                RegCloseKey(hkeyT);
                                rc = TRUE;
                        }

                        TCHAR szEnabledFilters[1024];
                        DWORD dwEnabledFiltersSize = sizeof(szEnabledFilters);

                        // Does the filter support Image Library Files API ?

                        if (RegQueryValueEx(hkey, c_szImageAPI, NULL, NULL,
                                (LPBYTE)szEnabledFilters, &dwEnabledFiltersSize) == 0)
                        {
                            for (
                                PCTSTR pExt = _tcstok(szEnabledFilters, _T(" "));
                                pExt != NULL && bImageAPI != TRUE;
                                pExt = _tcstok(NULL, _T(" "))) 
                            {
                                if (_tcsicmp(pExt, ach) == 0) 
                                {
                                    bImageAPI = TRUE;
                                }
                            }
                        }
                }
                RegCloseKey(hkey);
        }

        return rc;
}

#ifdef _USE_FLT_API
//
//  GetHandlerForFile
//
//  find an import/export filter for the given file.
//
BOOL GetHandlerForFile(BOOL bImport, LPCTSTR szFile, LPTSTR szHandler, UINT cb)
{
        TCHAR    buf[40];
        BOOL    rc = FALSE;     // return code

        *szHandler = 0;

        if (szFile == NULL)
                return FALSE;

        // find the extension
        LPCTSTR ext = FindExtension(szFile);

        for (int i = 0;
                GetInstalledFilters(bImport, i, NULL, 0, buf, sizeof(buf), szHandler, cb);
                i++)
        {
                if (lstrcmpi(ext+1, buf) == 0)
                        break;
                else
                        *szHandler = 0;
        }

        // make sure the handler file does exist
        if (*szHandler && GetFileAttributes(szHandler) != -1)
                rc = TRUE;

        return rc;
}

//
// FindBitmapInfo
//
// find the DIB bitmap in a memory meta file...
//
LPBITMAPINFOHEADER FindBitmapInfo(LPMETAHEADER pmh)
{
        for (LPMETARECORD pmr = (LPMETARECORD)((LPBYTE)pmh + pmh->mtHeaderSize*2);
                pmr < (LPMETARECORD)((LPBYTE)pmh + pmh->mtSize*2);
                pmr = (LPMETARECORD)((LPBYTE)pmr + pmr->rdSize*2))
        {
                switch (pmr->rdFunction)
                {
                        case META_DIBBITBLT:
                                return (LPBITMAPINFOHEADER)&(pmr->rdParm[8]);

                        case META_DIBSTRETCHBLT:
                                return (LPBITMAPINFOHEADER)&(pmr->rdParm[10]);

                        case META_STRETCHDIB:
                                return (LPBITMAPINFOHEADER)&(pmr->rdParm[11]);

                        case META_SETDIBTODEV:
                                return (LPBITMAPINFOHEADER)&(pmr->rdParm[9]);
                }
        }

        return NULL;
}

#endif // _USE_FLT_API

#ifdef _USE_IFL_API
  IFLERROR ReadGIFInterlacedImage(BYTE *ppbImageBuffer,
                                      IFLHANDLE pfpbFROM,
                                      int ImageHeight, DWORD dwWidthInBytes)
{
    int          iLine, iPass, iIntLine, iTempLine;
    IFLERROR   idErr;


    WORD       InterlaceMultiplier[] = { 8, 8, 4, 2 };
    WORD       InterlaceOffset[]     = { 0, 4, 2, 1 };

    idErr = IFLERR_NONE;

    iPass = 0;
    iIntLine = InterlaceOffset[iPass];
    iLine = 0;
    while (idErr == IFLERR_NONE && iLine < ImageHeight)
    {
       iTempLine = InterlaceMultiplier[iPass] * iIntLine + InterlaceOffset[iPass];
       if (iTempLine >= ImageHeight)
       {
           iPass++;
           iIntLine = 0;
           iTempLine = InterlaceOffset[iPass];
       }

       if (iTempLine < ImageHeight)
       {
           idErr = iflRead(pfpbFROM,
                     (LPBYTE)ppbImageBuffer+((ImageHeight-iTempLine-1)*dwWidthInBytes),
                           1);
           iLine++;
       }
       iIntLine++;
    }

    return idErr;
}
#endif // _USE_IFL_API

//
//  LoadDIBFromFile
//
//  load a image file using a image import filter. The filters use ANSI strings.
//


LPBITMAPINFOHEADER LoadDIBFromFileW (LPCWSTR szFileName)
{
   char *szAnsiName = NULL;
   UINT cb;
   LPBITMAPINFOHEADER pbi = NULL;

   cb = WideCharToMultiByte (CP_ACP, 0, (LPWSTR)szFileName, -1, NULL, 0, NULL, NULL);
   if (cb)
   {
      szAnsiName = (LPSTR)GlobalAlloc (GPTR, cb);
   }
   if (szAnsiName)
   {
      WideCharToMultiByte (CP_ACP, 0, (LPWSTR)szFileName, -1, szAnsiName, cb, NULL, NULL);
      pbi = LoadDIBFromFileA (szAnsiName);
      GlobalFree (szAnsiName);
   }
   return pbi;
}

LPBITMAPINFOHEADER LoadDIBFromFileA(LPCSTR szFileName)
{
#ifdef _USE_IFL_API

        IFLTYPE iflType;

        iflImageType((LPSTR)szFileName, &iflType);

        // make sure the image is of a type we know how to import
        if (iflType == IFLT_PNG)
        {
           return NULL;
        }

        IFLHANDLE iflHandle = iflCreateReadHandle(iflType);
        if (!iflHandle)
        {
           //
           // No filter installed for this type
           //
           return NULL;
        }

        LPBYTE lpStart = 0;

    __try 
    {

        IFLERROR iflErr = iflOpen(iflHandle, (LPSTR)szFileName, IFLM_READ);
        if (iflErr != IFLERR_NONE)
        {
                iflFreeHandle(iflHandle);
                return NULL;
        }

        ((CPBApp *)AfxGetApp())->m_nFltTypeUsed = iflType;

        IFLCLASS        iflClass = iflGetClass(iflHandle);
        IFLSEQUENCE     iflSequence = iflGetSequence(iflHandle);
        IFLCOMPRESSION  iflCompression = iflGetCompression(iflHandle);
        int             iBPS = iflGetBitsPerChannel(iflHandle);

        if (iflClass != IFLCL_RGB && iflClass != IFLCL_PALETTE &&
            iflClass != IFLCL_GRAY && iflClass != IFLCL_BILEVEL)
        {
#ifdef _DEBUG
           TRACE(TEXT("LoadDIBFromFile: Not a RGB/PALETTE/GRAY/BW image.\n"));
           MessageBox (NULL, TEXT("Not a RGB/PALETTE/GRAY/BW image."),
                             TEXT("Loadimag.cpp"), MB_OK);
#endif
           iflClose(iflHandle);
           iflFreeHandle(iflHandle);
           return NULL;
        }

        // get the transparent color
        if (iflClass == IFLCL_RGB)
        {
                IFLCOLOR iflTransColor;
                g_bUseTrans = (IFLERR_NONE ==
                        iflControl(iflHandle, IFLCMD_TRANS_RGB, 0, 0, &iflTransColor));
                if (g_bUseTrans)
                        crTrans = RGB(iflTransColor.wRed,
                                                  iflTransColor.wGreen,
                                                  iflTransColor.wBlue);
        }
        else // must be IFLCL_PALETTE or IFLCL_GRAY or IFLCL_BILEVEL
        {
                BYTE byTransIdx;
                g_bUseTrans = (IFLERR_NONE ==
                        iflControl(iflHandle, IFLCMD_TRANS_IDX, 0, 0, &byTransIdx));
                if (g_bUseTrans)
                        crTrans = byTransIdx; // need to convert to COLORREF below
        }

        BITMAPINFOHEADER bi;
        memset(&bi, 0, sizeof(BITMAPINFOHEADER));

        bi.biSize = sizeof(BITMAPINFOHEADER); // should be 0x28 or 40 decimal
        bi.biWidth = iflGetWidth(iflHandle);
        bi.biHeight = iflGetHeight(iflHandle);
        bi.biPlanes = 1;




        if (iflClass == IFLCL_RGB)
        {
#ifdef PNG_SUPPORT
           if (iflType == IFLT_PNG)
           {
              bi.biBitCount = iBPS*3;
           }
           else
#endif // PNG_SUPPORT
           {
              bi.biBitCount = iflGetBitsPerPixel (iflHandle);
           }
        }
        else // must be IFLCL_PALETTE or IFLCL_GRAY or IFLCL_BILEVEL
        {
           bi.biBitCount = 8;
        }



        bi.biCompression = 0;
        // convert width in pixels to bytes after rounding it up first
        DWORD dwWidthInBytes = ((bi.biWidth * bi.biBitCount + 31) & ~31)/8;
        bi.biSizeImage = abs(bi.biHeight) * dwWidthInBytes;
//      bi.biXPelsPerMeter = 0;
//      bi.biYPelsPerMeter = 0;
        if (iflClass == IFLCL_PALETTE || iflClass == IFLCL_GRAY
             || iflClass == IFLCL_BILEVEL)
                bi.biClrUsed = MAX_PAL_SIZE;
//      bi.biClrImportant = 0;

        LPBYTE lpBMP;

        if ((lpBMP = lpStart = (LPBYTE)GlobalAlloc(GPTR, bi.biSize +
                bi.biClrUsed*sizeof(RGBQUAD) + bi.biSizeImage)) == NULL)
                goto exit;

        memcpy(lpBMP, &bi, bi.biSize);
        lpBMP += bi.biSize;

        BYTE    byTemp;
        int             i, j;

        switch (iflSequence)
        {
           case IFLSEQ_TOPDOWN:
              switch (iflClass)
              {
                 case IFLCL_RGB:

                    lpBMP += bi.biClrUsed*sizeof(RGBQUAD) + bi.biSizeImage -
                    dwWidthInBytes;
                    for (i = 0; i < abs(bi.biHeight); lpBMP-=dwWidthInBytes, i++)
                    {
                       // read in one line at a time
                       iflRead(iflHandle, (LPBYTE)lpBMP, 1);
                       // need to swap RED with BLUE for internal DIB display
                       for (j = 0; j < bi.biWidth*3; j+=3)
                       {
                          byTemp = *(lpBMP+j);
                          *(lpBMP+j) = *(lpBMP+j+2);
                          *(lpBMP+j+2) = byTemp;
                       }
                    }
                    break;

                 case IFLCL_PALETTE:

                    // get palette info first...
                    RGBTRIPLE Pal3[MAX_PAL_SIZE];
                    RGBQUAD   Pal4[MAX_PAL_SIZE];
                    ZeroMemory (Pal3, MAX_PAL_SIZE*(sizeof(RGBTRIPLE)));
                    iflErr = iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &Pal3);

                    for (i = 0; i < MAX_PAL_SIZE; i++)
                    {
                       Pal4[i].rgbBlue     = Pal3[i].rgbtRed;
                       Pal4[i].rgbGreen    = Pal3[i].rgbtGreen;
                       Pal4[i].rgbRed      = Pal3[i].rgbtBlue;
                       Pal4[i].rgbReserved = 0;
                    }
                    memcpy(lpBMP, Pal4, sizeof(Pal4));

                    if (g_bUseTrans)
                    // convert the transparent color index to COLORREF
                       crTrans = RGB(Pal4[crTrans].rgbRed,Pal4[crTrans].rgbGreen,
                                             Pal4[crTrans].rgbBlue);

                    lpBMP += sizeof(Pal4) + bi.biSizeImage - dwWidthInBytes;

                    for (i = 0;i < abs(bi.biHeight);lpBMP-=dwWidthInBytes, i++)
                    {
                       // read in one line at a time
                       iflRead(iflHandle, (LPBYTE)lpBMP, 1);
                    }

                    break;

                 case IFLCL_GRAY:

                    // get palette info first...
                    //BYTE PalGray[MAX_PAL_SIZE];
                    //iflErr = iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &PalGray);

                    for (i = 0; i < MAX_PAL_SIZE; i++)
                    {
                       Pal4[i].rgbBlue     = i;//PalGray[i];
                       Pal4[i].rgbGreen    = i;//PalGray[i];
                       Pal4[i].rgbRed      = i;//PalGray[i];
                       Pal4[i].rgbReserved = 0;
                    }
                    memcpy(lpBMP, Pal4, sizeof(Pal4));

                    if (g_bUseTrans)
                    // convert the transparent color index to COLORREF
                       crTrans = RGB(Pal4[crTrans].rgbRed, Pal4[crTrans].rgbGreen,
                                                   Pal4[crTrans].rgbBlue);

                    lpBMP += sizeof(Pal4) + bi.biSizeImage - dwWidthInBytes;

                    for (i = 0;i < abs(bi.biHeight);lpBMP-=dwWidthInBytes, i++)
                    {
                       // read in one line at a time
                       iflRead(iflHandle, (LPBYTE)lpBMP, 1);
                    }

                    break;

                 case IFLCL_BILEVEL:

                    // set color Black
                    Pal4[0].rgbBlue     = 0;
                    Pal4[0].rgbGreen    = 0;
                    Pal4[0].rgbRed      = 0;
                    Pal4[0].rgbReserved = 0;

                    // set color White
                    Pal4[1].rgbBlue     = 255;
                    Pal4[1].rgbGreen    = 255;
                    Pal4[1].rgbRed      = 255;
                    Pal4[1].rgbReserved = 0;

                    memcpy(lpBMP, Pal4, sizeof(Pal4));

                    if (g_bUseTrans)
                       // convert the transparent color index to COLORREF
                       crTrans = RGB(Pal4[crTrans].rgbRed,
                                     Pal4[crTrans].rgbGreen,
                                     Pal4[crTrans].rgbBlue);

                    lpBMP += sizeof(Pal4) + bi.biSizeImage - dwWidthInBytes;

                    for (i = 0;i < abs(bi.biHeight);lpBMP-=dwWidthInBytes, i++)
                    {
                       // read in one line at a time
                       iflRead(iflHandle, (LPBYTE)lpBMP, 1);
                    }
                    break;

                 default:
                 // currently not supported
                    break;
              }
              break;

           case IFLSEQ_BOTTOMUP:

              lpBMP += bi.biClrUsed*sizeof(RGBQUAD) + bi.biSizeImage - dwWidthInBytes;

              for (i = 0;i < abs(bi.biHeight);lpBMP-=dwWidthInBytes, i++)
              {
                 // read in one line at a time
                 iflRead(iflHandle, (LPBYTE)lpBMP, 1);

                 // need to swap RED with BLUE for internal DIB display
                 for (j = 0; j < bi.biWidth*3; j+=3)
                 {
                    byTemp = *(lpBMP+j);
                    *(lpBMP+j) = *(lpBMP+j+2);
                    *(lpBMP+j+2) = byTemp;
                 }
              }
              break;

           case IFLSEQ_GIF_INTERLACED:
           {

              // get color palette info first...
              RGBTRIPLE Pal3[MAX_PAL_SIZE];
              RGBQUAD   Pal4[MAX_PAL_SIZE];
              iflErr = iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &Pal3);

              for (i = 0; i < MAX_PAL_SIZE; i++)
                 {
                    Pal4[i].rgbBlue     = Pal3[i].rgbtRed;
                    Pal4[i].rgbGreen    = Pal3[i].rgbtGreen;
                    Pal4[i].rgbRed      = Pal3[i].rgbtBlue;
                    Pal4[i].rgbReserved = 0;
                 }
                 memcpy(lpBMP, Pal4, sizeof(Pal4));

                 if (g_bUseTrans)
                 // convert the transparent color index to COLORREF
                    crTrans = RGB(Pal4[crTrans].rgbRed,Pal4[crTrans].rgbGreen,
                                                       Pal4[crTrans].rgbBlue);

              LPBYTE lpTemp = lpBMP + sizeof(Pal4);
              ReadGIFInterlacedImage (lpTemp, iflHandle, bi.biHeight, dwWidthInBytes);

           }
           break;
/*         case 1010101:
              {

                 int IM[] = { 8, 8, 4, 2 }; // interlace multiplier
                 //int IO[] = { 1, 5, 3 ,2 }; // interface offset
                 int IO[] = { 0, 4, 2,1 };

                 for (j = 0; j < 4; j++)
                 {
                    lpBMP = lpTemp + bi.biSizeImage - dwWidthInBytes*IO[j];
                    for (i = 0; i < abs(bi.biHeight) && lpBMP >= lpTemp;
                                 lpBMP-=dwWidthInBytes*IM[j], i+=8)
                    {
                       // read in one line at a time
                       iflRead(iflHandle, (LPBYTE)lpBMP, 1);
                    }
                 }

                 break;
              }*/
#ifdef PNG_SUPPORT
           case IFLSEQ_ADAM7_INTERLACED:
           {

              // get color palette info first...
              RGBTRIPLE Pal3[MAX_PAL_SIZE];
              RGBQUAD   Pal4[MAX_PAL_SIZE];
              iflErr = iflControl(iflHandle, IFLCMD_PALETTE, 0, 0, &Pal3);

              for (i = 0; i < MAX_PAL_SIZE; i++)
              {
                 Pal4[i].rgbBlue     = Pal3[i].rgbtRed;
                 Pal4[i].rgbGreen    = Pal3[i].rgbtGreen;
                 Pal4[i].rgbRed      = Pal3[i].rgbtBlue;
                 Pal4[i].rgbReserved = 0;
              }
              memcpy(lpBMP, Pal4, sizeof(Pal4));

              if (g_bUseTrans)
                 // convert the transparent color index to COLORREF
                 crTrans = RGB(Pal4[crTrans].rgbRed,
                               Pal4[crTrans].rgbGreen,
                               Pal4[crTrans].rgbBlue);
/////////////////////////////
                HANDLE hHeap = GetProcessHeap();
                LPBYTE *ppbRGBRowPtrs =(LPBYTE *)AllocateImageSpace(hHeap,
                                      bi.biHeight, dwWidthInBytes, /*bi.biWidth, */sizeof(BYTE));

                if (ppbRGBRowPtrs != NULL)
                {
                // First get the image. This function will de-interlace the image
                // AND any alpha channel information: it will also resize the alpha
                // channel data structure to the image height from the number of
                // raster lines, if necessary.
                   iflErr = ReadADAM7InterlacedImage(ppbRGBRowPtrs, iflHandle,
                                                    bi.biHeight, bi.biWidth,
                                                    sizeof(BYTE)*3, iflClass);
/////////////////////////////

                   if (iflErr == IFLERR_NONE)
                   {
                      lpBMP += bi.biClrUsed*sizeof(RGBQUAD) + bi.biSizeImage -
                                             dwWidthInBytes;
                      for (i = 0;i < abs(bi.biHeight);lpBMP-=dwWidthInBytes, i++)
                      {
                         // read in one line at a time
                         memcpy((LPBYTE)lpBMP, ppbRGBRowPtrs[i], dwWidthInBytes);

                         // need to swap RED with BLUE for internal DIB display
                         for (j = 0; j < bi.biWidth*3; j+=3)
                         {
                            byTemp = *(lpBMP+j);
                            *(lpBMP+j) = *(lpBMP+j+2);
                            *(lpBMP+j+2) = byTemp;
                         }
                     }
                  }

                  ppbRGBRowPtrs = (LPBYTE *)FreeImageSpace(hHeap,
                                                           ppbRGBRowPtrs,
                                                           bi.biHeight);
                }
                break;

             }
#endif // PNG_SUPPORT
             default:
                break;
        }

    } 
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

exit:
        iflClose(iflHandle);
        iflFreeHandle(iflHandle);

        return (LPBITMAPINFOHEADER)lpStart;

#endif // _USE_IFL_API

//////////////////////////////////////////////////////////////////////////////

#ifdef _USE_FLT_API

        HINSTANCE           hLib = NULL;
        FILESPEC            fileSpec;               // file to load
        GRPI                            pict;
        UINT                rc;                     // return code
        HANDLE              hPrefMem = NULL;        // filter-supplied preferences
        UINT                wFilterType;            // 2 = graphics filter
        char                szHandler[128];
        LPBITMAPINFOHEADER  lpbi = NULL;

        PFNGetFilterInfo lpfnGetFilterInfo;
        PFNImportGr lpfnImportGr;

        if (!GetHandlerForFile(TRUE, szFileName, szHandler, sizeof(szHandler)))
        return FALSE;

    if (szHandler[0] == 0)
        return FALSE;

    if ((hLib = LoadLibrary(szHandler)) == NULL)
        goto exit;

    // get a pointer to the ImportGR function
    lpfnGetFilterInfo = (PFNGetFilterInfo)GetProcAddress(hLib, "GetFilterInfo");
    lpfnImportGr = (PFNImportGr)GetProcAddress(hLib, "ImportGr");

    if (lpfnGetFilterInfo == NULL)
        lpfnGetFilterInfo = (PFNGetFilterInfo)GetProcAddress(hLib, "GetFilterInfo@16");

    if (lpfnImportGr == NULL)
        lpfnImportGr = (PFNImportGr)GetProcAddress(hLib, "ImportGr@16");

    if (lpfnImportGr == NULL)
        goto exit;

    if (lpfnGetFilterInfo != NULL)
    {
        wFilterType = (*lpfnGetFilterInfo)
            ((short) 2,                 // interface version no.
            (char *)NULL,               // unused
            (HANDLE *) &hPrefMem,       // fill in: preferences
            (DWORD) 0x00020000);        // unused in Windows

        // the return value is the type of filter: 0=error,
        // 1=text-filter, 2=graphics-filter
        if (wFilterType != 2)
            goto exit;
    }

    fileSpec.slippery = FALSE;      // TRUE if file may disappear
    fileSpec.write = FALSE;         // TRUE if open for write
    fileSpec.unnamed = FALSE;       // TRUE if unnamed
    fileSpec.linked = FALSE;        // Linked to an FS FCB
    fileSpec.mark = FALSE;          // Generic mark bit
    fileSpec.dcbFile = 0L;
    //the converters need a pathname without spaces! silly people
    GetShortPathName(szFileName, fileSpec.szName, sizeof(fileSpec.szName));

    pict.hmf = NULL;

    rc = (*lpfnImportGr)
        (NULL,                      // "the target DC" (printer?)
        (FILESPEC *) &fileSpec,     // file to read
        (GRPI *) &pict,             // fill in: result metafile
        (HANDLE) hPrefMem);         // preferences memory

        if (rc != 0 || pict.hmf == NULL)
        goto exit;

    //
    // find the BITMAPINFO in the returned metafile
    // this saves us from creating a metafile and duplicating
    // all the memory.
    //
    lpbi = FindBitmapInfo((LPMETAHEADER)GlobalLock(pict.hmf));

    if (lpbi == NULL)       // cant find it bail
    {
        GlobalFree(pict.hmf);
    }
    else
    {
        lpbi->biXPelsPerMeter = (DWORD)pict.hmf;
        lpbi->biYPelsPerMeter = 0x12345678;
    }

exit:
    if (hPrefMem != NULL)
        GlobalFree(hPrefMem);

    if (hLib)
        FreeLibrary(hLib);

    return lpbi;

#endif // _USE_FLT_API
   return NULL;
}

//
//  FreeDIB
//
void FreeDIB(LPBITMAPINFOHEADER lpbi)
{
        if (lpbi)
        {
                if (lpbi->biXPelsPerMeter && lpbi->biYPelsPerMeter == 0x12345678)
                        GlobalFree((HANDLE)LongToPtr(lpbi->biXPelsPerMeter));
                else
                        GlobalFree(lpbi);
        }
}
