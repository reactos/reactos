/****************************Module*Header******************************\
* Module Name: loadimg.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include <windows.h>
#include <port1632.h>
#include <memory.h>
#include "pbrush.h"
#include "dlgs.h"

extern TCHAR *namePtr;
extern int defaultWid, defaultHgt;
extern BYTE *fileBuff;
extern int imageByteWid;
extern HBITMAP fileBitmap;
extern imagePlanes;
extern BOOL gfDirty;
extern HDC fileDC, pickDC;
extern TCHAR filePath[];
extern int DlgCaptionNo;
extern int pickWid, pickHgt;
extern HWND pbrushWnd[];
extern HPALETTE hPalette;

BOOL InitFileBuffer(void);
void DeleteFileBuffer(void);
BOOL LoadImg24(HWND hWnd, HANDLE fh, DHDR *phdr);

typedef struct tagMSPHeader {
   SHORT id;
   SHORT id2;
   SHORT wid;
   SHORT hgt;
   TCHAR garbg[24]; /* add to 32 bytes total */
} MSPHeader;

static RGBQUAD DefColors[] = { {   0,   0,   0, 0 },
                               { 128,   0,   0, 0 },
                               {   0, 128,   0, 0 },
                               { 128, 128,   0, 0 },
                               {   0,   0, 128, 0 },
                               { 128,   0, 128, 0 },
                               {   0, 128, 128, 0 },
                               { 128, 128, 128, 0 },
                               { 192, 192, 192, 0 },
                               { 255,   0,   0, 0 },
                               {   0, 255,   0, 0 },
                               { 255, 255,   0, 0 },
                               {   0,   0, 255, 0 },
                               { 255,   0, 255, 0 },
                               {   0, 255, 255, 0 },
                               { 255, 255, 255, 0 }
                             };


static RGBQUAD Def256Colors[] = {
{   0,   0,   0, 0},{   1,   0,   0, 0},{   2,   0,   0, 0},{   3,   0,   0, 0},
{   4,   0,   0, 0},{   5,   0,   0, 0},{   6,   0,  51, 0},{   7,   0,  51, 0},
{   8,   0,  51, 0},{   9,   0,  51, 0},{  10,   0,  51, 0},{  11,   0,  51, 0},
{  12,   0, 102, 0},{  13,   0, 102, 0},{  14,   0, 102, 0},{  15,   0, 102, 0},
{  16,   0, 102, 0},{  17,   0, 102, 0},{  18,   0, 153, 0},{  19,   0, 153, 0},
{  20,   0, 153, 0},{  21,   0, 153, 0},{  22,   0, 153, 0},{  23,   0, 153, 0},
{  24,   0, 204, 0},{  25,   0, 204, 0},{  26,   0, 204, 0},{  27,   0, 204, 0},
{  28,   0, 204, 0},{  29,   0, 204, 0},{  30,   0, 255, 0},{  31,   0, 255, 0},
{  32,   0, 255, 0},{  33,   0, 255, 0},{  34,   0, 255, 0},{  35,   0, 255, 0},
{  36,  51,   0, 0},{  37,  51,   0, 0},{  38,  51,   0, 0},{  39,  51,   0, 0},
{  40,  51,   0, 0},{  41,  51,   0, 0},{  42,  51,  51, 0},{  43,  51,  51, 0},
{  44,  51,  51, 0},{  45,  51,  51, 0},{  46,  51,  51, 0},{  47,  51,  51, 0},
{  48,  51, 102, 0},{  49,  51, 102, 0},{  50,  51, 102, 0},{  51,  51, 102, 0},
{  52,  51, 102, 0},{  53,  51, 102, 0},{  54,  51, 153, 0},{  55,  51, 153, 0},
{  56,  51, 153, 0},{  57,  51, 153, 0},{  58,  51, 153, 0},{  59,  51, 153, 0},
{  60,  51, 204, 0},{  61,  51, 204, 0},{  62,  51, 204, 0},{  63,  51, 204, 0},
{  64,  51, 204, 0},{  65,  51, 204, 0},{  66,  51, 255, 0},{  67,  51, 255, 0},
{  68,  51, 255, 0},{  69,  51, 255, 0},{  70,  51, 255, 0},{  71,  51, 255, 0},
{  72, 102,   0, 0},{  73, 102,   0, 0},{  74, 102,   0, 0},{  75, 102,   0, 0},
{  76, 102,   0, 0},{  77, 102,   0, 0},{  78, 102,  51, 0},{  79, 102,  51, 0},
{  80, 102,  51, 0},{  81, 102,  51, 0},{  82, 102,  51, 0},{  83, 102,  51, 0},
{  84, 102, 102, 0},{  85, 102, 102, 0},{  86, 102, 102, 0},{  87, 102, 102, 0},
{  88, 102, 102, 0},{  89, 102, 102, 0},{  90, 102, 153, 0},{  91, 102, 153, 0},
{  92, 102, 153, 0},{  93, 102, 153, 0},{  94, 102, 153, 0},{  95, 102, 153, 0},
{  96, 102, 204, 0},{  97, 102, 204, 0},{  98, 102, 204, 0},{  99, 102, 204, 0},
{ 100, 102, 204, 0},{ 101, 102, 204, 0},{ 102, 102, 255, 0},{ 103, 102, 255, 0},
{ 104, 102, 255, 0},{ 105, 102, 255, 0},{ 106, 102, 255, 0},{ 107, 102, 255, 0},
{ 108, 153,   0, 0},{ 109, 153,   0, 0},{ 110, 153,   0, 0},{ 111, 153,   0, 0},
{ 112, 153,   0, 0},{ 113, 153,   0, 0},{ 114, 153,  51, 0},{ 115, 153,  51, 0},
{ 116, 153,  51, 0},{ 117, 153,  51, 0},{ 118, 153,  51, 0},{ 119, 153,  51, 0},
{ 120, 153, 102, 0},{ 121, 153, 102, 0},{ 122, 153, 102, 0},{ 123, 153, 102, 0},
{ 124, 153, 102, 0},{ 125, 153, 102, 0},{ 126, 153, 153, 0},{ 127, 153, 153, 0},
{ 128, 153, 153, 0},{ 129, 153, 153, 0},{ 130, 153, 153, 0},{ 131, 153, 153, 0},
{ 132, 153, 204, 0},{ 133, 153, 204, 0},{ 134, 153, 204, 0},{ 135, 153, 204, 0},
{ 136, 153, 204, 0},{ 137, 153, 204, 0},{ 138, 153, 255, 0},{ 139, 153, 255, 0},
{ 140, 153, 255, 0},{ 141, 153, 255, 0},{ 142, 153, 255, 0},{ 143, 153, 255, 0},
{ 144, 204,   0, 0},{ 145, 204,   0, 0},{ 146, 204,   0, 0},{ 147, 204,   0, 0},
{ 148, 204,   0, 0},{ 149, 204,   0, 0},{ 150, 204,  51, 0},{ 151, 204,  51, 0},
{ 152, 204,  51, 0},{ 153, 204,  51, 0},{ 154, 204,  51, 0},{ 155, 204,  51, 0},
{ 156, 204, 102, 0},{ 157, 204, 102, 0},{ 158, 204, 102, 0},{ 159, 204, 102, 0},
{ 160, 204, 102, 0},{ 161, 204, 102, 0},{ 162, 204, 153, 0},{ 163, 204, 153, 0},
{ 164, 204, 153, 0},{ 165, 204, 153, 0},{ 166, 204, 153, 0},{ 167, 204, 153, 0},
{ 168, 204, 204, 0},{ 169, 204, 204, 0},{ 170, 204, 204, 0},{ 171, 204, 204, 0},
{ 172, 204, 204, 0},{ 173, 204, 204, 0},{ 174, 204, 255, 0},{ 175, 204, 255, 0},
{ 176, 204, 255, 0},{ 177, 204, 255, 0},{ 178, 204, 255, 0},{ 179, 204, 255, 0},
{ 180, 255,   0, 0},{ 181, 255,   0, 0},{ 182, 255,   0, 0},{ 183, 255,   0, 0},
{ 184, 255,   0, 0},{ 185, 255,   0, 0},{ 186, 255,  51, 0},{ 187, 255,  51, 0},
{ 188, 255,  51, 0},{ 189, 255,  51, 0},{ 190, 255,  51, 0},{ 191, 255,  51, 0},
{ 192, 255, 102, 0},{ 193, 255, 102, 0},{ 194, 255, 102, 0},{ 195, 255, 102, 0},
{ 196, 255, 102, 0},{ 197, 255, 102, 0},{ 198, 255, 153, 0},{ 199, 255, 153, 0},
{ 200, 255, 153, 0},{ 201, 255, 153, 0},{ 202, 255, 153, 0},{ 203, 255, 153, 0},
{ 204, 255, 204, 0},{ 205, 255, 204, 0},{ 206, 255, 204, 0},{ 207, 255, 204, 0},
{ 208, 255, 204, 0},{ 209, 255, 204, 0},{ 210, 255, 255, 0},{ 211, 255, 255, 0},
{ 212, 255, 255, 0},{ 213, 255, 255, 0},{ 214, 255, 255, 0},{ 215, 238, 238, 0},
{ 216, 221, 221, 0},{ 217, 187, 187, 0},{ 218, 170, 170, 0},{ 219, 136, 136, 0},
{ 220, 119, 119, 0},{ 221,  85,  85, 0},{ 222,  68,  68, 0},{ 223,  34,  34, 0},
{ 224,  17,  17, 0},{ 225, 255, 251, 0},{ 226, 255, 251, 0},{ 227, 255, 251, 0},
{ 228, 255, 251, 0},{ 229, 255, 251, 0},{ 230, 255, 251, 0},{ 231, 255, 251, 0},
{ 232, 255, 251, 0},{ 233, 255, 251, 0},{ 234, 160, 160, 0},{ 235, 128, 128, 0},
{ 236,   0,   0, 0},{ 237,   0,   0, 0},{ 238,   0,   0, 0},{ 239,   0,   0, 0},
{ 240,   0,   0, 0},{ 241,   0,   0, 0},{ 242,   0,   0, 0},{ 243,   0,   0, 0},
{ 244,   0,   0, 0},{ 245,   0,   0, 0},{ 246,   0,   0, 0},{ 247,   0,   0, 0},
{ 248,   0,   0, 0},{ 249,   0,   0, 0},{ 250,   0,   0, 0},{ 251,   0,   0, 0},
{ 252,   0,   0, 0},{ 253,   0,   0, 0},{ 254,   0,   0, 0},{ 255,   0,   0, 0},
};

/* Build palette based on the color resolution */
BOOL BuildPalette(HANDLE fh, LPBITMAPINFO lpDIBinfo, DHDR hdr, int nPlanes)
{
   RGBQUAD FAR *lpPalette = lpDIBinfo->bmiColors;
   int     i;
   BOOL    bConvert, bError;
   DWORD   dwFilePos;
   BYTE    byID;

   bError = FALSE;

   // Is there a palette?
   //
   // Version (hdr.hard) 0, 3, and 2(1bpp) need default VGA palette.
   // Version 2(!1bpp), 5(<8bpp) have 16color RGBTRIPLEs.
   // Version 5(8bpp) has 256color RGBTRIPLEs at end of file.
   //
   if (hdr.hard == 0 || hdr.hard == 3 ||
       (hdr.hard == 2 && lpDIBinfo->bmiHeader.biBitCount == 1)) {
       /* no palette in file - use a "standard" VGA palette */

       switch (lpDIBinfo->bmiHeader.biBitCount) {
           case 1:
               lpPalette[0] = DefColors[0];
               lpPalette[1] = DefColors[15];
               break;

           case 4:
               for (i = 0; i < 16; ++i)
                   lpPalette[i] = DefColors[i];
               if (nPlanes == 3)
                   lpPalette[8] = DefColors[0];
               break;
           case 8:
               for (i = 0; i < 256; i++)
                   lpPalette[i] = Def256Colors[i];
               lpDIBinfo->bmiHeader.biClrUsed = 256;
               break;
       }

       bConvert = FALSE;

   } else if ( hdr.hard == 2 ||
               (hdr.hard == 5 && lpDIBinfo->bmiHeader.biBitCount < 8)) {

       RepeatMove((LPBYTE) lpPalette, hdr.clrma,
                  (WORD)(3 * (1 << lpDIBinfo->bmiHeader.biBitCount)));
       bConvert = TRUE;
   } else if (hdr.hard == 5 && lpDIBinfo->bmiHeader.biBitCount == 8) {
       /* 256 color palette at end of file */

       /* seek to the marker / palette */
       if ((dwFilePos = MyFileSeek(fh, 0L, 1)) == -1 ||
           MyFileSeek(fh, 0L, 2) == -1 ||
           MyFileSeek(fh, -769L, 1) == -1 ||
           !MyByteReadFile(fh, &byID, sizeof(byID)) ||
           byID != 12 ||
           !MyByteReadFile(fh, lpDIBinfo->bmiColors, 768)) {

           SimpleMessage(IDSBadData, namePtr, MB_OK | MB_ICONEXCLAMATION);
           bError = TRUE;
       }

       bConvert = TRUE;

       /* move file pointer back to where it was */
       MyFileSeek(fh, dwFilePos, 0);
   }

   if (bConvert && !bError)
       TripleToQuad(lpDIBinfo, TRUE);

   return bError;
}

/* Convert a 4 planar bitmap to chunky format */
void PlanarToChunky(LPBYTE lpDIBits, LPBYTE lpRedBuf, LPBYTE lpGreenBuf,
                    LPBYTE lpBlueBuf, LPBYTE lpIntBuf, int bpline)
{
   int x, i, index;
   BYTE m;

   for (x = index = 0; x < bpline; x++, index += 4) {
       /* Each Byte from RGBI buffer set will make 8 nibbles, 4 bytes */

       for (i=0, m = 0x80; i < 4; i++, m >>= 1) {

           /* Upper Nibble */

           lpDIBits[index+i] = (BYTE)
                      ((((lpIntBuf[x]   & m) == m) ? 0x80 : 0) |
                       (((lpRedBuf[x]   & m) == m) ? 0x40 : 0) |
                       (((lpGreenBuf[x] & m) == m) ? 0x20 : 0) |
                       (((lpBlueBuf[x]  & m) == m) ? 0x10 : 0));

           /* Lower Nibble */

           m >>= 1;

           lpDIBits[index+i] |=
                           (((lpIntBuf[x]   & m) == m) ? 0x08 : 0) |
                           (((lpRedBuf[x]   & m) == m) ? 0x04 : 0) |
                           (((lpGreenBuf[x] & m) == m) ? 0x02 : 0) |
                           (((lpBlueBuf[x]  & m) == m) ? 0x01 : 0) ;
       }
   }
}

void Decode2Buf(LPBYTE where, LPBYTE src, int many)
{
   if (many <= 0)
       return;

   do {
       if (!*src) {
           RepeatFill(where, src[2], src[1]);
           many -= 3;
           where += src[1];
           src += 3;
       } else {
           RepeatMove(where, 1 + src, *src);
           many -= 1 + *src;
           where += *src;
           src += 1 + *src;
       }
   } while (many > 0);
}

BOOL LoadMSP2Img(HWND hWnd)
{
   register    i, j;
   int         hgt;
   HANDLE      fh;
   BOOL        error = TRUE;
   HCURSOR     oldCsr;
   int         bpline;
   int         iBufSize;
   LPINT       lpLengths, lpBuffer;
   HANDLE      hLength, hBuffer;
   WORD        wSize;
   MSPHeader   hdr;
   WORD        errmsg;

   if ((fh = MyOpenFile(namePtr, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error1;
   }

   wSize = sizeof(hdr);
   if (!MyByteReadFile(fh, &hdr, wSize))
   {
       errmsg = IDSUnableHdr;
       goto error2;
   }

   if ((hdr.id != 0x694c) || (hdr.id2 != 0x536e)) {
       errmsg = IDSBadHeader;
       goto error2;
   }

   if (errmsg = AllocImg(hdr.wid, hgt = hdr.hgt, 1, 1, FALSE))
       goto error2;

   if (!AllocTemp(hdr.wid, 1, 1, 1, FALSE)) {
       errmsg = IDSNotEnufMem;
       goto error2;
   }

   if (!(hLength = GlobalAlloc(GHND, (DWORD) hdr.hgt * sizeof(WORD)))) {
       errmsg = IDSNotEnufMem;
       goto error3;
   }

   if (!(lpLengths = (LPINT) GlobalLock(hLength))) {
       errmsg = IDSNotEnufMem;
       goto error4;
   }

   wSize = (WORD)(sizeof(WORD) * hdr.hgt);
   if (!MyByteReadFile(fh, lpLengths, wSize)) {
       errmsg = IDSBadHeader;
       goto error5;
   }

   bpline = (7 + hdr.wid) >> 3;      /* 1 bit per pixel */

   oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   iBufSize = 0;
   hBuffer = NULL;

   for (i = 0; i < hgt; ++i) {
       j = lpLengths[i];

       if (iBufSize < j) {
           iBufSize = j;

           if (hBuffer) {
               GlobalUnlock(hBuffer);
               GlobalFree(hBuffer);
           }

           if (!(hBuffer = GlobalAlloc(GHND, (DWORD) iBufSize))) {
               errmsg = IDSBadHeader;
               goto error6;
           }

           if (!(lpBuffer = (LPINT) GlobalLock(hBuffer))) {
               errmsg = IDSBadHeader;
               goto error7;
           }
       }

       if (j)  {
           if (!MyByteReadFile(fh, lpBuffer, j)) {
               errmsg = IDSBadData;
               goto error7;
           } else
               Decode2Buf(fileBuff, (LPBYTE) lpBuffer, j);
       } else                        /* no data, all FF's */
           RepeatFill(fileBuff, 0xFF, (WORD)imageByteWid);

       SetBitmapBits(fileBitmap, (long) imageByteWid * imagePlanes,
                     (LPBYTE) fileBuff);
       BitBlt(hdcWork, 0, i, hdr.wid, 1, fileDC, 0, 0,
               SRCCOPY);
   }

   BitBlt(hdcImage, 0, 0, hdr.wid, hdr.hgt, hdcWork, 0, 0, SRCCOPY);

   GetCurrentDirectory(PATHlen, filePath);
   ReplaceExtension(namePtr, BITMAPFILE);

   if (bFileExists(namePtr))
       SimpleMessage(IDSNameConflict, namePtr, MB_OK);

   error = FALSE;

error7:
   GlobalFree(hBuffer);

error6:
   SetCursor(oldCsr);
   if (error)
       ClearImg();
   gfDirty = FALSE;

error5:
   GlobalUnlock(hLength);

error4:
   GlobalFree(hLength);

error3:
   FreeTemp();

error2:
   MyCloseFile(fh);

error1:
   return error ? errmsg : 0;
}

BOOL LoadMSPImg(HWND hWnd)
{
   int         i, hgt;
   HANDLE      fh;
   BOOL        error = TRUE;
   WORD        errmsg;
   HCURSOR     oldCsr;
   int         bpline;
   MSPHeader   hdr;
   WORD        wSize;

   if ((fh = MyOpenFile(namePtr, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error2;
   }

   wSize = sizeof(hdr);
   if (!MyByteReadFile(fh, &hdr, wSize))
   {
       errmsg = IDSUnableHdr;
       goto error3;
   }

   if ((hdr.id != 0x6144) || (hdr.id2 != 0x4d6e)) {
       MyCloseFile(fh);
       return LoadMSP2Img(hWnd);
   }

   if (errmsg = AllocImg(hdr.wid, hgt = hdr.hgt, 1, 1, FALSE))
       goto error3;

   if (!AllocTemp(hdr.wid, 1, 1, 1, FALSE)) {
       errmsg = IDSNotEnufMem;
       goto error4;
   }

   bpline = (hdr.wid + 7) >> 3;      /* 1 bit per pixel */

   oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   for (i = 0; i < hgt; ++i) {
       if (!MyByteReadFile(fh, fileBuff, bpline)) {
           errmsg = IDSBadData;
           goto error5;
       }

       SetBitmapBits(fileBitmap, (long) imageByteWid * imagePlanes,
                     (LPBYTE) fileBuff);

       BitBlt(hdcWork, 0, i, hdr.wid, 1, fileDC, 0, 0,
               SRCCOPY);
   }

   BitBlt(hdcImage, 0, 0, hdr.wid, hdr.hgt, hdcWork, 0, 0, SRCCOPY);

   GetCurrentDirectory(PATHlen, filePath);
   ReplaceExtension(namePtr, BITMAPFILE);

   if (bFileExists(namePtr))
       SimpleMessage(IDSNameConflict, namePtr, MB_OK);

   error = FALSE;

error5:
   SetCursor(oldCsr);
   FreeTemp();

error4:
   if (error)
       ClearImg();
   gfDirty = FALSE;

error3:
   MyCloseFile(fh);

error2:
   return error ? errmsg : 0;

}

BOOL GetMSPInfo(HWND hWnd)
{
   TCHAR       szFileName[25];
   int         rc;
   HANDLE      fh;
   MSPHeader   hdr;

   if (!GetDlgItemText(hWnd, edt1, szFileName, 25))
       return FALSE;

   if ((fh = MyOpenFile(szFileName, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
       return FALSE;

   rc = MyByteReadFile(fh, &hdr, sizeof(hdr));
   MyCloseFile(fh);

   if (rc) {
       BitmapHeader.wid = hdr.wid;
       BitmapHeader.hgt = hdr.hgt;
       BitmapHeader.planes   =
       BitmapHeader.bitcount = 1;
   }

   return rc;
}

BOOL LoadImg(HWND hWnd, LPTSTR lpFilename)
{
   LPBITMAPINFO lpDIBinfo;
   SHORT       i, wid, hgt;
   WORD        wplanes, wbitpx;
   HANDLE      fh;
   BOOL        error = TRUE;
   WORD        errmsg;
   DHDR        hdr;
   HCURSOR     oldCsr;
   HDC         hdc;
   LPBYTE      lpDIBits;
   HANDLE      hDIBits, hDIBinfo;
   HPALETTE    hOldPalette = NULL, hNewPal;
   HBITMAP     htempbit;
   int         alloc;
   int         nFilePlanes;
   WORD        wSize;
   UINT        wUsage;
   DWORD       dwSize;


   if ((fh = MyOpenFile(lpFilename, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
       errmsg = IDSCantOpen;
       goto error0;
   }

   oldCsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   wSize = sizeof(DHDR);
   if (!MyByteReadFile(fh, &hdr, wSize)) {
       errmsg = IDSUnableHdr;
       goto error1;
   }

   if (!ValidHdr(hWnd, &hdr, lpFilename)) {
       error = TRUE;
       errmsg = IDSUnableHdr;
       goto error1;
   }

   // If it is a 24 bpp .pcx file, then call special case code to read it
   if (hdr.nPlanes == 3 && hdr.bitpx == 8)
   {
        if (hdr.hard == 5) {
            errmsg = LoadImg24(hWnd, fh, &hdr);
            if (errmsg != 0)
                error = TRUE;
            else
                error = FALSE;
            goto error1;
        } else {
            error = TRUE;
            errmsg = IDSUnknownFmt;
            goto error1;
        }
   }

   if (hdr.nPlanes == 3) {
       hdr.nPlanes = 4;
       nFilePlanes = 3;
   } else
       nFilePlanes = hdr.nPlanes;

   wid = (SHORT)(1 + hdr.x2 - hdr.x1);
   hgt = (SHORT)(1 + hdr.y2 - hdr.y1);

   wplanes = wbitpx = (WORD)((1 == hdr.nPlanes && 1 == hdr.bitpx) ? 1 : 0);

   if (DlgCaptionNo == PASTEFROM) {
      pickWid = wid;
      pickHgt = hgt;

      if (!(hdc = GetDC(NULL)) ||
          !(alloc = AlocPick(hdc))) {
           if (hdc)
               ReleaseDC(NULL, hdc);
           errmsg = IDSCantAlloc;
           goto error1;
      }
      ReleaseDC(NULL, hdc);
   } else {
       if (errmsg = AllocImg(wid, hgt, wplanes, wbitpx, FALSE))
           goto error1;
   }

   if (!AllocTemp(max(wid, hdr.bplin), 1, -hdr.nPlanes, -hdr.bitpx, FALSE)) {
       errmsg = IDSNotEnufMem;
       goto error1;
   }

   if (!InitFileBuffer()) {
       errmsg = IDSCantAlloc;
       goto error1;
   }

   htempbit = CreateBitmap(1, 1, 1, 1, NULL);
   if (!htempbit) {
       errmsg = IDSCantAlloc;
       goto error1a;
   }

   /* allocate dib info header */
   dwSize = sizeof(BITMAPINFOHEADER) + (1L << (hdr.nPlanes * hdr.bitpx))
                                     * sizeof(PALETTEENTRY);
   if (!(hDIBinfo = GlobalAlloc(GHND, dwSize))) {
       errmsg = IDSCantAlloc;
       goto error2;
   }

   /* lock it down */
   if (!(lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo))) {
       errmsg = IDSCantAlloc;
       goto error3;
   }

   /* Setup our dib header */
   lpDIBinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   lpDIBinfo->bmiHeader.biWidth = wid;
   lpDIBinfo->bmiHeader.biHeight = 1;
   lpDIBinfo->bmiHeader.biPlanes = 1;
   lpDIBinfo->bmiHeader.biBitCount = hdr.nPlanes * hdr.bitpx;
   lpDIBinfo->bmiHeader.biSizeImage = 0;
   lpDIBinfo->bmiHeader.biCompression = 0;
   lpDIBinfo->bmiHeader.biXPelsPerMeter = 0;
   lpDIBinfo->bmiHeader.biYPelsPerMeter = 0;
   lpDIBinfo->bmiHeader.biClrUsed = 0;
   lpDIBinfo->bmiHeader.biClrImportant = 0;

   /* and build the logical palette */
   if (BuildPalette(fh, lpDIBinfo, hdr, nFilePlanes)) {
       errmsg = IDSCantAlloc;
       goto error4;
   }

   hNewPal = MakeImagePalette(hPalette, hDIBinfo, &wUsage);
   if (hNewPal && hNewPal != hPalette) {
       hPalette = hNewPal;
       hdc = (DlgCaptionNo == PASTEFROM) ? pickDC : hdcWork;

       SelectPalette(hdc, hPalette, 0);
       RealizePalette(hdc);

       if (DlgCaptionNo != PASTEFROM) {
               SelectPalette(hdcImage, hPalette, 0);
               RealizePalette(hdcImage);
       }

   }

   /* if we have a 4bpp image allocate space for the DIB data */
   if (hdr.nPlanes == 4) {
       /* allocate space for the DIB */
       dwSize = 4L * ((hdr.bplin * hdr.nPlanes + 3) / 4);
       if (!(hDIBits = GlobalAlloc(GHND, dwSize))) {
           errmsg = IDSCantAlloc;
           goto error4;
       }

       /* lock it down */
       if (!(lpDIBits = GlobalLock(hDIBits))) {
           errmsg = IDSCantAlloc;
           goto error5;
       }
   } else /* the file buffer IS the DIB data */
       lpDIBits = fileBuff;

   hOldPalette = SelectPalette(fileDC, hPalette, 0);
   RealizePalette(fileDC);

   for (i = 0; i < hgt; ++i) {
       if (!UnpkBuff(fileBuff, 0, hdr, nFilePlanes, fh)) {
           errmsg = IDSBadData;
           goto error6;
       }

       /* If PCX is 4 planar convert to DIB */
       if (hdr.nPlanes == 4) {
           PlanarToChunky(lpDIBits, fileBuff + 2 * hdr.bplin,
                          fileBuff + hdr.bplin, fileBuff,
                          fileBuff + 3 * hdr.bplin,
                          hdr.bplin);
       }

       SelectObject(fileDC, htempbit);
       SetDIBits(fileDC, fileBitmap, 0, 1, lpDIBits, lpDIBinfo, wUsage);
       SelectObject(fileDC, fileBitmap);

       if (DlgCaptionNo != PASTEFROM)
           BitBlt(hdcWork, 0, i, wid, 1, fileDC, 0, 0, SRCCOPY);
       else
           BitBlt(pickDC, 0, i, wid, 1, fileDC, 0, 0, SRCCOPY);
   }

   if (DlgCaptionNo != PASTEFROM)
       BitBlt(hdcImage, 0, 0, wid, hgt, hdcWork, 0, 0, SRCCOPY);

   error = FALSE;

error6:
   if (hOldPalette)
       SelectPalette(fileDC, hOldPalette, 0);

   if (hdr.nPlanes == 4)
       GlobalUnlock(hDIBits);

error5:
   if (hdr.nPlanes == 4)
       GlobalFree(hDIBits);

error4:
   GlobalUnlock(hDIBinfo);

error3:
   GlobalFree(hDIBinfo);

error2:
   DeleteFileBuffer();

error1a:
   DeleteObject(htempbit);

error1:
   MyCloseFile(fh);

   SetCursor(oldCsr);
   FreeTemp();

   if (DlgCaptionNo != PASTEFROM)
       gfDirty = FALSE;

   if (error) {
       if (DlgCaptionNo == PASTEFROM)
           FreePick();
       else
           ClearImg();
   } else
       GetCurrentDirectory(PATHlen, filePath);

error0:
   return error ? errmsg : 0;
}


BOOL LoadImg24(HWND hWnd, HANDLE fh, DHDR *phdr) {
    BOOL fError = TRUE;
    WORD errmsg;
    LPBYTE lpPCXBits = NULL;
    LPBYTE lpDIBits = NULL;
    LPBITMAPINFO lpDIBinfo = NULL;
    HANDLE hDIBinfo = NULL;
    SHORT       i, wid, hgt;
    HDC         hdc;
    WORD        wplanes, wbitpx;
    HPALETTE    hNewPal;
    int         alloc;

    UINT        wUsage;
    DWORD       dwSize;
    int cbScan;
    HBITMAP htempbit = NULL;

    /*
     * Create the image device bitmap
     */
    wid = (SHORT)(1 + phdr->x2 - phdr->x1);
    hgt = (SHORT)(1 + phdr->y2 - phdr->y1);

    wplanes = wbitpx = 0;

    if (DlgCaptionNo == PASTEFROM) {
        pickWid = wid;
        pickHgt = hgt;

        if (!(hdc = GetDC(NULL)) ||
            !(alloc = AlocPick(hdc))) {
            if (hdc)
                ReleaseDC(NULL, hdc);
            errmsg = IDSCantAlloc;
            goto error1;
        }
        ReleaseDC(NULL, hdc);
    } else if (errmsg = AllocImg(wid, hgt, wplanes, wbitpx, FALSE)) {
        goto error1;
    }

    /*
     * hbmImage and hbmWork are now created.  Width and Height == file bitmap;
     * however, planes and bpp are for the display.
     */


    /*
     * Create DIB header for 24 bpp bmp with no palette
     */

    // allocate dib info header
    if (!(hDIBinfo = GlobalAlloc(GHND, sizeof(BITMAPINFOHEADER)))) {
        errmsg = IDSCantAlloc;
        goto error;
    }

    /* lock it down */
    if (!(lpDIBinfo = (LPBITMAPINFO) GlobalLock(hDIBinfo))) {
        errmsg = IDSCantAlloc;
        goto error;
    }

    // Setup our dib header
    lpDIBinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    lpDIBinfo->bmiHeader.biWidth = wid;
    lpDIBinfo->bmiHeader.biHeight = 1;
    lpDIBinfo->bmiHeader.biPlanes = 1;
    lpDIBinfo->bmiHeader.biBitCount = 24;
    lpDIBinfo->bmiHeader.biSizeImage = 0;
    lpDIBinfo->bmiHeader.biCompression = 0;
    lpDIBinfo->bmiHeader.biXPelsPerMeter = 0;
    lpDIBinfo->bmiHeader.biYPelsPerMeter = 0;
    lpDIBinfo->bmiHeader.biClrUsed = 0;
    lpDIBinfo->bmiHeader.biClrImportant = 0;

    /*
     * Create a basic palette
     */
    hNewPal = MakeImagePalette(hPalette, hDIBinfo, &wUsage);
    if (hNewPal && hNewPal != hPalette) {
        hPalette = hNewPal;
        hdc = (DlgCaptionNo == PASTEFROM) ? pickDC : hdcWork;

        SelectPalette(hdc, hPalette, 0);
        RealizePalette(hdc);

        if (DlgCaptionNo != PASTEFROM) {
            SelectPalette(hdcImage, hPalette, 0);
            RealizePalette(hdcImage);
        }
    }

    /*
     * Create buffer for entire bitmap
     */
    cbScan = (wid * sizeof(RGBTRIPLE) + (sizeof(DWORD)-1)) & ~(sizeof(DWORD)-1);
    lpDIBits = LocalAlloc( LMEM_FIXED, cbScan);

    if (lpDIBits == NULL) {
        errmsg = IDSCantAlloc;
        goto error;
    }

    lpPCXBits = LocalAlloc( LMEM_FIXED, phdr->bplin * phdr->nPlanes);
    if (lpPCXBits == NULL) {
        errmsg = IDSCantAlloc;
        goto error;
    }

    /*
     * Load bitmap bits and translate
     */
    // Init vars for UnpkBuffer()
    if (!InitFileBuffer()) {
        errmsg = IDSCantAlloc;
        goto error;
    }

    hdc = (DlgCaptionNo == PASTEFROM) ? pickDC : hdcWork;

    for( i = 0; i < hgt; i++ ) {
        DWORD cbPlane;
        LPBYTE pRed, pGreen, pBlue, pDst;

        if (!UnpkBuff(lpPCXBits, 0, *phdr, phdr->nPlanes, fh)) {
            errmsg = IDSBadData;
            goto error;
        }

        /* Convert the bytes from RGB to BGR */
        cbPlane = phdr->bplin;
        pDst = lpDIBits;
        pRed = lpPCXBits;
        pGreen = pRed + cbPlane;
        pBlue = pGreen + cbPlane;

        for (; cbPlane > 0; cbPlane--) {
             *pDst++ = *pBlue++;
             *pDst++ = *pGreen++;
             *pDst++ = *pRed++;
        }

        StretchDIBits(hdc, 0, i, wid, 1, 0, 0, wid, 1, lpDIBits,
            lpDIBinfo, wUsage, SRCCOPY);
    }

    /*
     * If this is the image bitmap, then copy hdcWork to hdcImage
     */
    if (DlgCaptionNo != PASTEFROM)
        BitBlt(hdcImage, 0, 0, wid, hgt, hdcWork, 0, 0, SRCCOPY);

    /*
     * Cleanup and return
     */
    fError = FALSE;

error:

    if (lpPCXBits != NULL )
        LocalFree(lpPCXBits);

    if (lpDIBits != NULL)
        LocalFree(lpDIBits);

    if (lpDIBinfo != NULL)
        GlobalUnlock(lpDIBinfo);


    if (hDIBinfo != NULL)
        GlobalFree(hDIBinfo);

error1:

    return fError ? errmsg : 0;
}
