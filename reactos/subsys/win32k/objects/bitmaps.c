

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
//#include <win32k/debug.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL STDCALL W32kBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP)
{
  UNIMPLEMENTED;
}

HBITMAP STDCALL W32kCreateBitmap(INT  Width,
                          INT  Height,
                          UINT  Planes,
                          UINT  BitsPerPel,
                          CONST VOID *Bits)
{
  PBITMAPOBJ  bmp;
  HBITMAP  hBitmap;

  Planes = (BYTE) Planes;
  BitsPerPel = (BYTE) BitsPerPel;

  /* Check parameters */
  if (!Height || !Width) 
    {
      return 0;
    }
  if (Planes != 1) 
    {
      UNIMPLEMENTED;
      return  0;
    }
  if (Height < 0) 
    {
      Height = -Height;
    }
  if (Width < 0) 
    {
      Width = -Width;
    }

  /* Create the BITMAPOBJ */
  bmp = BITMAPOBJ_AllocBitmap ();
  if (!bmp) 
    {
      return 0;
    }

  DPRINT("%dx%d, %d colors returning %08x\n", Width, Height,
         1 << (Planes * BitsPerPel), bmp);

  bmp->size.cx = 0;
  bmp->size.cy = 0;
  bmp->bitmap.bmType = 0;
  bmp->bitmap.bmWidth = Width;
  bmp->bitmap.bmHeight = Height;
  bmp->bitmap.bmPlanes = Planes;
  bmp->bitmap.bmBitsPixel = BitsPerPel;
  bmp->bitmap.bmWidthBytes = BITMAPOBJ_GetWidthBytes (Width, BitsPerPel);
  bmp->bitmap.bmBits = NULL;
  bmp->DDBitmap = NULL;
  bmp->dib = NULL;
  hBitmap = BITMAPOBJ_PtrToHandle (bmp);
  if (Bits) /* Set bitmap bits */
    {
      W32kSetBitmapBits(hBitmap, 
                        Height * bmp->bitmap.bmWidthBytes,
                        Bits);
    }
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return  hBitmap;
}

HBITMAP STDCALL W32kCreateCompatibleBitmap(HDC hDC,
                                    INT  Width,
                                    INT  Height)
{
  HBITMAP  hbmpRet;
  PDC  dc;

  hbmpRet = 0;
  DPRINT("(%04x,%d,%d) = \n", hDC, Width, Height);
  dc = DC_PtrToHandle (hDC);
  if (!dc)
    {
      return 0;
    }
  if ((Width >= 0x10000) || (Height >= 0x10000)) 
    {
      DPRINT("got bad width %d or height %d, please look for reason\n",
             Width, Height);
    } 
  else 
    {
      /* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
      if (!Width || !Height)
        {
          hbmpRet = W32kCreateBitmap (1, 1, 1, 1, NULL);
        }
      else 
        {
          hbmpRet = W32kCreateBitmap (Width, 
                                      Height, 
                                      1, 
                                      dc->w.bitsPerPixel, 
                                      NULL);
        }
    }
  DPRINT ("\t\t%04x\n", hbmpRet);
  DC_UnlockDC (hDC);
  
  return hbmpRet;
}

HBITMAP STDCALL W32kCreateBitmapIndirect(CONST BITMAP  *BM)
{
  return W32kCreateBitmap (BM->bmWidth, 
                           BM->bmHeight, 
                           BM->bmPlanes,
                           BM->bmBitsPixel, 
                           BM->bmBits);
}

HBITMAP STDCALL W32kCreateDIBitmap(HDC  hDC,
                            CONST BITMAPINFOHEADER  *bmih,
                            DWORD  Init,
                            CONST VOID  *bInit,
                            CONST BITMAPINFO  *bmi,
                            UINT  Usage)
{
  UNIMPLEMENTED;
}

HBITMAP STDCALL W32kCreateDIBSection(HDC hDC,
                              CONST BITMAPINFO  *bmi,
                              UINT  Usage,
                              VOID  *Bits,
                              HANDLE  hSection,
                              DWORD  dwOffset)
{
  UNIMPLEMENTED;
}

HBITMAP STDCALL W32kCreateDiscardableBitmap(HDC  hDC,
                                     INT  Width,
                                     INT  Height)
{
  /* FIXME: this probably should do something else */
  return  W32kCreateCompatibleBitmap(hDC, Width, Height);
}

BOOL STDCALL W32kExtFloodFill(HDC  hDC,
                      INT  XStart,
                      INT  YStart,
                      COLORREF  Color, 
                      UINT  FillType)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kFloodFill(HDC  hDC,
                    INT  XStart,
                    INT  YStart,
                    COLORREF  Fill)
{
  UNIMPLEMENTED;
}

LONG STDCALL W32kGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Count,
                        LPVOID  Bits)
{
  PBITMAPOBJ  bmp;
  LONG  height, ret;
  
  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (!bmp) 
    {
      return 0;
    }
    
  /* If the bits vector is null, the function should return the read size */
  if (Bits == NULL)
    {
      return bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;
    }

  if (Count < 0) 
    {
      DPRINT ("(%ld): Negative number of bytes passed???\n", Count);
      Count = -Count;
    }

  /* Only get entire lines */
  height = Count / bmp->bitmap.bmWidthBytes;
  if (height > bmp->bitmap.bmHeight) 
    {
      height = bmp->bitmap.bmHeight;
    }
  Count = height * bmp->bitmap.bmWidthBytes;
  if (Count == 0)
    {
      DPRINT("Less then one entire line requested\n");
      BITMAPOBJ_UnlockBitmap (hBitmap);
      return  0;
    }

  DPRINT("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
         hBitmap, Count, Bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
         1 << bmp->bitmap.bmBitsPixel, height );
#if 0
  /* FIXME: Call DDI CopyBits here if available  */
  if(bmp->DDBitmap) 
    {
      DPRINT("Calling device specific BitmapBits\n");
      if(bmp->DDBitmap->funcs->pBitmapBits)
        {
          ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count,
                                                  DDB_GET);
        }
      else 
        {
          ERR_(bitmap)("BitmapBits == NULL??\n");
          ret = 0;
        }
    } 
  else 
#endif
    {
      if(!bmp->bitmap.bmBits) 
        {
          DPRINT ("Bitmap is empty\n");
          ret = 0;
        } 
      else 
        {
          memcpy(Bits, bmp->bitmap.bmBits, Count);
          ret = Count;
        }
    }
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return  ret;
}

BOOL STDCALL W32kGetBitmapDimensionEx(HBITMAP  hBitmap,
                               LPSIZE  Dimension)
{
  PBITMAPOBJ  bmp;
  
  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL) 
    {
      return FALSE;
    }
  
  *Dimension = bmp->size;
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return  TRUE;
}

UINT STDCALL W32kGetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO   bi,
                   UINT  Usage)
{
  UNIMPLEMENTED;
}

COLORREF STDCALL W32kGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kMaskBlt(HDC  hDCDest,
                  INT  XDest,
                  INT  YDest,
                  INT  Width,
                  INT  Height,
                  HDC  hDCSrc,
                  INT  XSrc, 
                  INT  YSrc,
                  HBITMAP  hMaskBitmap,
                  INT  xMask,
                  INT  yMask,
                  DWORD  ROP)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kPlgBlt(HDC  hDCDest,
                CONST POINT  *Point,
                HDC  hDCSrc, 
                INT  XSrc,  
                INT  YSrc,  
                INT  Width, 
                INT  Height,
                HBITMAP  hMaskBitmap,
                INT  xMask,      
                INT  yMask)
{
  UNIMPLEMENTED;
}

LONG STDCALL W32kSetBitmapBits(HBITMAP  hBitmap,
                        DWORD  Bytes,
                        CONST VOID *Bits)
{
  DWORD  height, ret;
  PBITMAPOBJ  bmp;
  
  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL || Bits == NULL)
    {
      return 0;
    }

  if (Bytes < 0) 
    {
      DPRINT ("(%ld): Negative number of bytes passed???\n", Bytes );
      Bytes = -Bytes;
    }

  /* Only get entire lines */
  height = Bytes / bmp->bitmap.bmWidthBytes;
  if (height > bmp->bitmap.bmHeight) 
    {
      height = bmp->bitmap.bmHeight;
    }
  Bytes = height * bmp->bitmap.bmWidthBytes;
  DPRINT ("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
          hBitmap, 
          Bytes, 
          Bits, 
          bmp->bitmap.bmWidth, 
          bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel, 
          height);

#if 0
  /* FIXME: call DDI specific function here if available  */
  if(bmp->DDBitmap) 
    {
      DPRINT ("Calling device specific BitmapBits\n");
      if (bmp->DDBitmap->funcs->pBitmapBits)
        {
          ret = bmp->DDBitmap->funcs->pBitmapBits(hBitmap, 
                                                  (void *) Bits,
                                                  Bytes, 
                                                  DDB_SET);
        } 
      else 
        {
          DPRINT ("BitmapBits == NULL??\n");
          ret = 0;
        }
    } 
  else
#endif
    {
      /* FIXME: Alloc enough for entire bitmap */
      if (bmp->bitmap.bmBits == NULL)
        {
          bmp->bitmap.bmBits = ExAllocatePool (NonPagedPool, Bytes);
        }
      if(!bmp->bitmap.bmBits) 
        {
          DPRINT ("Unable to allocate bit buffer\n");
          ret = 0;
        } 
      else 
        {
          memcpy(bmp->bitmap.bmBits, Bits, Bytes);
          ret = Bytes;
        }
    }
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return ret;
}

BOOL STDCALL W32kSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
{
  PBITMAPOBJ  bmp;
  
  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL) 
    {
      return FALSE;
    }
  
  if (Size) 
    {
      *Size = bmp->size;
    }
  bmp->size.cx = Width;
  bmp->size.cy = Height;
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return TRUE;
}

UINT STDCALL W32kSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kSetDIBits(HDC  hDC,
                   HBITMAP  hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   CONST VOID  *Bits,
                   CONST BITMAPINFO  *bmi,
                   UINT  ColorUse)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kSetDIBitsToDevice(HDC  hDC,
                           INT  XDest,
                           INT  YDest,
                           DWORD  Width,
                           DWORD  Height,
                           INT  XSrc,
                           INT  YSrc,
                           UINT  StartScan,
                           UINT  ScanLines,
                           CONST VOID  *Bits,
                           CONST BITMAPINFO  *bmi,
                           UINT  ColorUse)
{
  UNIMPLEMENTED;
}

COLORREF STDCALL W32kSetPixel(HDC  hDC,
                       INT  X,
                       INT  Y,
                       COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kStretchBlt(HDC  hDCDest,
                     INT  XOriginDest,
                     INT  YOriginDest,
                     INT  WidthDest,
                     INT  HeightDest,
                     HDC  hDCSrc,
                     INT  XOriginSrc,
                     INT  YOriginSrc,
                     INT  WidthSrc,    
                     INT  HeightSrc, 
                     DWORD  ROP)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kStretchDIBits(HDC  hDC,
                       INT  XDest,
                       INT  YDest,
                       INT  DestWidth,
                       INT  DestHeight,
                       INT  XSrc,       
                       INT  YSrc,       
                       INT  SrcWidth,  
                       INT  SrcHeight, 
                       CONST VOID  *Bits,
                       CONST BITMAPINFO  *BitsInfo,
                       UINT  Usage,                 
                       DWORD  ROP)
{
  UNIMPLEMENTED;
}

/*  Internal Functions  */

INT 
BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp)
{
  switch(bpp)
    {
    case 1:
      return 2 * ((bmWidth+15) >> 4);
      
    case 24:
      bmWidth *= 3; /* fall through */
    case 8:
      return bmWidth + (bmWidth & 1);
      
    case 32:
      return bmWidth * 4;
      
    case 16:
    case 15:
      return bmWidth * 2;

    case 4:
      return 2 * ((bmWidth+3) >> 2);
      
    default:
      DPRINT ("stub");
    }

  return -1;
}

HBITMAP  BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
{
  PBITMAPOBJ  bmp;
  HBITMAP  res;
  BITMAP  bm;

  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
  if (bmp == NULL) 
    {
      return 0;
    }
  res = 0;
  
  bm = bmp->bitmap;
  bm.bmBits = NULL;
  res = W32kCreateBitmapIndirect(&bm);
  if(res) 
    {
      char *buf;
      
      buf = ExAllocatePool (NonPagedPool, bm.bmWidthBytes * bm.bmHeight);
      W32kGetBitmapBits (hBitmap, bm.bmWidthBytes * bm.bmHeight, buf);
      W32kSetBitmapBits (res, bm.bmWidthBytes * bm.bmHeight, buf);
      ExFreePool (buf);
    }
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return  res;
}

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 * 11/16/1999 (RJJ) lifted from wine
 */
int DIB_GetDIBWidthBytes(int  width, int  depth)
{
  int words;
  
  switch(depth)
    {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;
      
    default:
      DPRINT("(%d): Unsupported depth\n", depth );
      /* fall through */
    case 32:
      words = width;
    }
  return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

int DIB_GetDIBImageBytes (int  width, int  height, int  depth)
{
  return DIB_GetDIBWidthBytes( width, depth ) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

int DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse)
{
  int colors;

  if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
      BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
      colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
      return sizeof(BITMAPCOREHEADER) + colors *
        ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
  else  /* assume BITMAPINFOHEADER */
    {
      colors = info->bmiHeader.biClrUsed;
      if (!colors && (info->bmiHeader.biBitCount <= 8))
        colors = 1 << info->bmiHeader.biBitCount;
        return sizeof(BITMAPINFOHEADER) + colors *
               ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}

