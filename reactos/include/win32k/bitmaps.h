

BOOL  W32kBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP);
HBITMAP  W32kCreateBitmap(INT  Width,
                          INT  Height,
                          UINT  Planes,
                          UINT  BitsPerPel,
                          CONST VOID *Bits);
HBITMAP  W32kCreateBitmapIndirect(CONST BITMAP  *BM);
HBITMAP  W32kCreateDIBitmap(HDC  hDC,
                            CONST BITMAPINFOHEADER  *bmih,
                            DWORD  Init,
                            CONST VOID  *bInit,
                            CONST BITMAPINFO  *bmi,
                            UINT  Usage);
