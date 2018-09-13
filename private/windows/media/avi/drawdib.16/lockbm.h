#define BM_NULL         0
#define BM_TYPE         0x0F        // type mask
#define BM_8BIT         0x01        // all SVGA, and other 256 color
#define BM_16555        0x02        // some HiDAC cards
#define BM_24BGR        0x03        // just like a DIB
#define BM_32BGR        0x04        // 32 bit BGR
#define BM_VGA          0x05        // VGA style bitmap.
#define BM_16565        0x06        // most HiDAC cards
#define BM_24RGB        0x07        // 24 bit RGB
#define BM_32RGB        0x08        // 32 bit RGB
#define BM_1BIT         0x09        // mono bitmap
#define BM_4BIT         0x0A        // 4 bit packed pixel.

#define BM_CANLOCK      0x10        // can lock these bitmaps
#define BM_MAPFLAT      0x20        // can make bitmaps flat
#define BM_HUGE         0x40        // with FillBytes != 0 (for > 64k)
#define BM_BOTTOMTOTOP  0x80        // like a DIB

//
//  this is a physical BITMAP in memory, this is just like a  BITMAP
//  structure, but with extra fields staring at bmWidthPlanes
//
typedef struct {
    short  bmType;
    short  bmWidth;
    short  bmHeight;
    short  bmWidthBytes;
    BYTE   bmPlanes;
    BYTE   bmBitsPixel;
    LPVOID bmBits;
    long   bmWidthPlanes;
    long   bmlpPDevice;
    short  bmSegmentIndex;
    short  bmScanSegment;
    short  bmFillBytes;
    short  reserved1;
    short  reserved2;
} IBITMAP;

#define bmBitmapInfo bmWidthPlanes
#define bmOffset     bmlpPDevice
#define bmNextScan   reserved1

BOOL   FAR CanLockBitmaps(void);
UINT   FAR GetBitmapType(void);
LPVOID FAR LockBitmap(HBITMAP hbm);
LPVOID FAR GetBitmap(HBITMAP hbm, LPVOID p, int cb);
LPVOID FAR GetBitmapDIB(LPBITMAPINFOHEADER lpbi, LPVOID lpBits, LPVOID p, int cb);
LPVOID FAR GetDIBBitmap(HBITMAP hbm, LPBITMAPINFOHEADER lpbi);
LPVOID FAR GetPDevice(HDC hdc);
void   FAR TestSurfaceType(HDC hdc, int x, int y);
UINT   FAR GetSurfaceType(LPVOID lpBits);

BOOL   FAR MakeBitmapFlat(HBITMAP hbm);

//void   FAR BitmapXY(IBITMAP FAR *pbm, int x, int y);
