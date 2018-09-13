#include "ctlspriv.h"
#include "image.h"

#if defined(UNIX)
#include "unixstuff.h"
#endif

#ifndef ILC_COLORMASK
#define ILC_COLORMASK   0x00FE
#define ILD_BLENDMASK   0x000E
#endif
#undef ILC_COLOR
#undef ILC_BLEND

#define CLR_WHITE   0x00FFFFFFL
#define CLR_BLACK   0x00000000L

#define HICOLOR_DRAG

#ifndef _WIN32
extern HPALETTE WINAPI CreateHalftonePalette(HDC hdc);
#endif

#ifdef _WIN32
    #define SetBrushOrg(hdc, x, y)  SetBrushOrgEx(hdc, x, y, NULL)
#endif

//#ifdef WINNT
//#define USE_MASKBLT
//#endif

typedef struct _IMAGELIST
{
    DWORD       wMagic;     //
    int         cImage;     // count of images in image list
    int         cAlloc;     // # of images we have space for
    int         cGrow;      // # of images to grow bitmaps by
    int         cx;         // width of each image
    int         cy;         // height
    int         cStrip;     // # images in horizontal strip
    UINT        flags;      // ILC_* flags
    COLORREF    clrBlend;   // last blend color
    COLORREF    clrBk;      // bk color or CLR_NONE for transparent.
    HBRUSH      hbrBk;      // bk brush or black
    BOOL        fSolidBk;   // is the bkcolor a solid color (in hbmImage)
    HBITMAP     hbmImage;   // all images are in here
    HBITMAP     hbmMask;    // all image masks are in here.
    HDC         hdcImage;
    HDC         hdcMask;
    int         aOverlayIndexes[NUM_OVERLAY_IMAGES];    // array of special images
    int         aOverlayX[NUM_OVERLAY_IMAGES];          // x offset of image
    int         aOverlayY[NUM_OVERLAY_IMAGES];          // y offset of image
    int         aOverlayDX[NUM_OVERLAY_IMAGES];         // cx offset of image
    int         aOverlayDY[NUM_OVERLAY_IMAGES];         // cy offset of image
    int         aOverlayF[NUM_OVERLAY_IMAGES];          // ILD_ flags for image
    BOOL        fColorsSet;  // The DIB colors have been set with ImageList_SetColorTable()
    struct _IMAGELIST *pimlMirror;  // Set only when another mirrored imagelist is needed (ILC_MIRROR)    

#ifdef _WIN32
    //
    // virtual imagelist support
    //
    PFNIMLFILTER    pfnFilter;
    LPARAM          lParamFilter;

    //
    // used for "blending" effects on a HiColor display.
    // assumes layout of a DIBSECTION.
    //
    // BUGBUG on a non-hicolor system we dont need to allocate this!!
    //
    struct {
        BITMAP              bm;
        BITMAPINFOHEADER    bi;
        DWORD               ct[256];
    }   dib;
#endif

} IMAGELIST, NEAR *HIMAGELIST;

#define IsImageListIndex(piml, i) ((i) >= 0 && (i) < piml->cImage)

#define V_HIMAGELIST(himl)  V_HIMAGELISTERR(himl, 0)

#define V_HIMAGELISTVOID(himl)  \
        if (!IsImageList(himl)) {   \
            RIPMSG(0, "Invalid imagelist handle passed to API"); \
            return;                 \
        }

#define V_HIMAGELISTERR(himl, err)  \
        if (!IsImageList(himl)) {   \
            RIPMSG(0, "Invalid imagelist handle passed to API"); \
            return err;             \
        }

#define IMAGELIST_SIG   mmioFOURCC('H','I','M','L') // in memory magic
#define IMAGELIST_MAGIC ('I' + ('L' * 256))         // file format magic
// Version has to stay 0x0101 if we want both back ward and forward compatibility for
// our imagelist_read code
#define IMAGELIST_VER0  0x0101                      // file format ver
// #define IMAGELIST_VER1  0x0102                      // Image list version 2 -- this one has 15 overlay slots

#define BFTYPE_BITMAP   0x4D42      // "BM"

#define CBDIBBUF        4096

// Define this structure such that it will read and write the same
// format for both 16 and 32 bit applications...
#pragma pack(2)
typedef struct _ILFILEHEADER
{
    WORD    magic;
    WORD    version;
    SHORT   cImage;
    SHORT   cAlloc;
    SHORT   cGrow;
    SHORT   cx;
    SHORT   cy;
    COLORREF clrBk;
    SHORT    flags;
    SHORT       aOverlayIndexes[NUM_OVERLAY_IMAGES];  // array of special images
} ILFILEHEADER;

// This is the old size which has only 4 overlay slots
#define ILFILEHEADER_SIZE0 (SIZEOF(ILFILEHEADER) - SIZEOF(SHORT) * (NUM_OVERLAY_IMAGES - NUM_OVERLAY_IMAGES_0)) 

#pragma pack()

#ifdef IMAGELIST_TRACE
    #define DM  DebugMsg
#else
    #define DM     /* ; / ## / */
#endif

HDC g_hdcSrc = NULL;
HBITMAP g_hbmSrc = NULL;
HBITMAP g_hbmDcDeselect = NULL;

HDC g_hdcDst = NULL;
HBITMAP g_hbmDst = NULL;
int g_iILRefCount = 0;

//
// global work buffer, this buffer is always a DDB never a DIBSection
//
HBITMAP g_hbmWork = NULL;                   // work buffer.
BITMAP  g_bmWork;                           // work buffer size

HBRUSH g_hbrMonoDither = NULL;              // gray dither brush for dragging
HBRUSH g_hbrStripe = NULL;

void NEAR PASCAL ImageList_Terminate(void);

BOOL NEAR PASCAL ImageList_Replace2(IMAGELIST* piml, int i, int cImage, HBITMAP hbmImage, HBITMAP hbmMask, int xStart, int yStart);
void NEAR PASCAL ImageList_DeleteBitmap(HBITMAP hbm);
void NEAR PASCAL ImageList_SelectDstBitmap(HBITMAP hbmDst);
void NEAR PASCAL ImageList_SelectSrcBitmap(HBITMAP hbmSrc);
BOOL NEAR PASCAL ImageList_ReAllocBitmaps(IMAGELIST* piml, int cAlloc);
BOOL NEAR PASCAL ImageList_SetIconBitmaps(IMAGELIST* piml, HICON hicon);
void NEAR PASCAL ImageList_ResetBkColor(IMAGELIST* piml, int iFirst, int iLast, COLORREF clrBk);
void NEAR PASCAL ImageList_Merge2(IMAGELIST* piml, IMAGELIST* pimlMerge, int i, int dx, int dy);
void NEAR PASCAL ImageList_DeleteDragBitmaps();
void NEAR PASCAL ImageList_CopyOneImage(IMAGELIST* pimlDst, int iDst, int x, int y, IMAGELIST* pimlSrc, int iSrc);
IMAGELIST * NEAR PASCAL ImageList_Create2(int cx, int cy, UINT flags, int cGrow);
void NEAR PASCAL ImageList_SetOwners(IMAGELIST* piml);
BOOL WINAPI ImageList_SetDragImage(IMAGELIST* piml, int i, int dxHotspot, int dyHotspot);
BOOL NEAR PASCAL ImageList_GetSpareImageRect(IMAGELIST* piml, RECT FAR* prcImage);

BOOL NEAR PASCAL ImageList_IGetImageRect(IMAGELIST* piml, int i, RECT FAR* prcImage);
#define ImageList_GetImageRect ImageList_IGetImageRect

#define NOTSRCAND       0x00220326L
#define ROP_PSo         0x00FC008A
#define ROP_DPo         0x00FA0089
#define ROP_DPna        0x000A0329
#define ROP_DPSona      0x00020c89
#define ROP_SDPSanax    0x00E61ce8
#define ROP_DSna        0x00220326
#define ROP_PSDPxax     0x00b8074a

#define ROP_PatNotMask  0x00b8074a      // D <- S==0 ? P : D
#define ROP_PatMask     0x00E20746      // D <- S==1 ? P : D
#define ROP_MaskPat     0x00AC0744      // D <- P==1 ? D : S

#define ROP_DSo         0x00EE0086L
#define ROP_DSno        0x00BB0226L
#define ROP_DSa         0x008800C6L

BOOL NEAR PASCAL IsImageList(IMAGELIST *piml)
{
    return (piml && !IsBadWritePtr(piml, sizeof(IMAGELIST)) &&
        (piml->wMagic == IMAGELIST_SIG));
}

BOOL NEAR PASCAL ImageList_Filter(IMAGELIST **ppiml, VOID *pvData, BOOL fWrite)
{
    return ((*ppiml)->pfnFilter?
        !(*(*ppiml)->pfnFilter)(ppiml, pvData, (*ppiml)->lParamFilter, fWrite) :
        FALSE);
}

static int g_iDither = 0;

#pragma code_seg(CODESEG_INIT)

void FAR PASCAL InitDitherBrush()
{
    HBITMAP hbmTemp;
#ifdef UNIX
    char graybits[16];
    int i;
    for (i=0; i< 8; i++) {
        graybits[2*i] = ( i % 2) ? 0x55 : 0xAA;
    }
#else
    WORD graybits[] = {0xAAAA, 0x5555, 0xAAAA, 0x5555,
                       0xAAAA, 0x5555, 0xAAAA, 0x5555};
#endif


    if (g_iDither) {
        g_iDither++;
    } else {
        // build the dither brush.  this is a fixed 8x8 bitmap
        hbmTemp = CreateBitmap(8, 8, 1, 1, graybits);
        if (hbmTemp)
        {
            // now use the bitmap for what it was really intended...
            g_hbrMonoDither = CreatePatternBrush(hbmTemp);
#ifndef UNIX
#ifndef WINNT
            SetObjectOwner(g_hbrMonoDither, HINST_THISDLL);
#endif
#endif /* !UNIX */
            DeleteObject(hbmTemp);
            g_iDither++;
        }
    }
}

#pragma code_seg()

void FAR PASCAL TerminateDitherBrush()
{
    g_iDither--;
    if (g_iDither == 0) {
        DeleteObject(g_hbrMonoDither);
        g_hbrMonoDither = NULL;
    }
}

/*
** GetScreenDepth()
*/
int GetScreenDepth()
{
    int i;
    HDC hdc = GetDC(NULL);
    i = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);
    return i;
}

//
// should we use a DIB section on the current device?
//
// the main goal of using DS is to save memory, but they draw slow
// on some devices.
//
// 4bpp Device (ie 16 color VGA)    dont use DS
// 8bpp Device (ie 256 color SVGA)  use DS if DIBENG based.
// >8bpp Device (ie 16bpp 24bpp)    always use DS, saves memory
//

#define CAPS1           94          /* other caps */
#define C1_DIBENGINE    0x0010      /* DIB Engine compliant driver          */

BOOL UseDS(HDC hdc)
{
#ifdef WINNT
    return TRUE;
#else
    BOOL f;

    int ScreenDepth = GetDeviceCaps(hdc, BITSPIXEL) *
                      GetDeviceCaps(hdc, PLANES);

    f = (ScreenDepth > 8) ||
        (ScreenDepth > 4 && (GetDeviceCaps(hdc, CAPS1) & C1_DIBENGINE));

#ifdef DEBUG
    f = GetProfileInt(TEXT("windows"), TEXT("UseDIBSection"), f);
#endif

    return f;
#endif
}

//
// create a bitmap compatible with the given ImageList
//
HBITMAP ImageList_CreateBitmap(IMAGELIST *piml, int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;
    LPVOID lpBits;
    UINT flags;

    struct {
        BITMAPINFOHEADER bi;
        DWORD            ct[256];
    } dib;

    //
    // create a compatible bitmap if the imagelist has a bitmap already.
    //
    if (piml && piml->hbmImage && piml->hdcImage)
    {
        return CreateCompatibleBitmap(piml->hdcImage, cx, cy);
    }

    if (piml)
        flags = piml->flags;
    else
        flags = 0;

    hdc = GetDC(NULL);

    // no color depth was specifed
    //
    // if we are on a DIBENG based DISPLAY, we use 4bit DIBSections to save
    // memory.
    //
    if ((flags & ILC_COLORMASK) == 0)
    {
        if (UseDS(hdc))
            flags |= ILC_COLOR4;
        else
            flags |= ILC_COLORDDB;
    }

    if ((flags & ILC_COLORMASK) != ILC_COLORDDB)
    {
        dib.bi.biSize            = sizeof(BITMAPINFOHEADER);
        dib.bi.biWidth           = cx;
        dib.bi.biHeight          = cy;
        dib.bi.biPlanes          = 1;
        dib.bi.biBitCount        = (flags & ILC_COLORMASK);
        dib.bi.biCompression     = BI_RGB;
        dib.bi.biSizeImage       = 0;
        dib.bi.biXPelsPerMeter   = 0;
        dib.bi.biYPelsPerMeter   = 0;
        dib.bi.biClrUsed         = 16;
        dib.bi.biClrImportant    = 0;
        dib.ct[0]                = 0x00000000;    // 0000  black
        dib.ct[1]                = 0x00800000;    // 0001  dark red
        dib.ct[2]                = 0x00008000;    // 0010  dark green
        dib.ct[3]                = 0x00808000;    // 0011  mustard
        dib.ct[4]                = 0x00000080;    // 0100  dark blue
        dib.ct[5]                = 0x00800080;    // 0101  purple
        dib.ct[6]                = 0x00008080;    // 0110  dark turquoise
        dib.ct[7]                = 0x00C0C0C0;    // 1000  gray
        dib.ct[8]                = 0x00808080;    // 0111  dark gray
        dib.ct[9]                = 0x00FF0000;    // 1001  red
        dib.ct[10]               = 0x0000FF00;    // 1010  green
        dib.ct[11]               = 0x00FFFF00;    // 1011  yellow
        dib.ct[12]               = 0x000000FF;    // 1100  blue
        dib.ct[13]               = 0x00FF00FF;    // 1101  pink (magenta)
        dib.ct[14]               = 0x0000FFFF;    // 1110  cyan
        dib.ct[15]               = 0x00FFFFFF;    // 1111  white

        if (dib.bi.biBitCount == 8)
        {
            HPALETTE hpal;
            int i;

            if (hpal = CreateHalftonePalette(NULL))
            {
                i = GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)&dib.ct[0]);
                DeleteObject(hpal);

                if (i > 64)
                {
                    dib.bi.biClrUsed = i;
                    for (i=0; i<(int)dib.bi.biClrUsed; i++)
                        dib.ct[i] = RGB(GetBValue(dib.ct[i]),GetGValue(dib.ct[i]),GetRValue(dib.ct[i]));
                }
            }
            else
            {
                dib.bi.biBitCount = (flags & ILC_COLORMASK);
                dib.bi.biClrUsed = 256;
            }

            if (dib.bi.biClrUsed <= 16)
                dib.bi.biBitCount = 4;
        }

        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, &lpBits, NULL, 0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdc, cx, cy);
    }

    ReleaseDC(NULL, hdc);

    return hbm;
}

HBITMAP FAR PASCAL CreateColorBitmap(int cx, int cy)
{
    HBITMAP hbm;
    HDC hdc;

    hdc = GetDC(NULL);

    //
    // on a multimonitor system with mixed bitdepths
    // always use a 32bit bitmap for our work buffer
    // this will prevent us from losing colors when
    // blting to and from the screen.  this is mainly
    // important for the drag & drop offscreen buffers.
    //
    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
        GetSystemMetrics(SM_CMONITORS) > 1 &&
        GetSystemMetrics(SM_SAMEDISPLAYFORMAT) == 0)
    {
        LPVOID p;
#ifndef UNIX
        BITMAPINFO bi = {sizeof(BITMAPINFOHEADER), cx, cy, 1, 32};
#else
        BITMAPINFO bi;
        bi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER); 
        bi.bmiHeader.biWidth    = cx;
        bi.bmiHeader.biHeight   = cy;
        bi.bmiHeader.biPlanes   = 1 ;
        bi.bmiHeader.biBitCount = 32;
#endif
        hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &p, NULL, 0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdc, cx, cy);
    }

    ReleaseDC(NULL, hdc);
    return hbm;
}

HBITMAP FAR PASCAL CreateMonoBitmap(int cx, int cy)
{
    return CreateBitmap(cx, cy, 1, 1, NULL);
}

//============================================================================

BOOL NEAR PASCAL ImageList_Init(void)
{
    HDC hdcScreen;
    WORD stripebits[] = {0x7777, 0xdddd, 0x7777, 0xdddd,
                         0x7777, 0xdddd, 0x7777, 0xdddd};
    HBITMAP hbmTemp;

    DM(DM_TRACE, TEXT("ImageList_Init"));

    // if already initialized, there is nothing to do
    if (g_hdcDst)
        return TRUE;

    hdcScreen = GetDC(HWND_DESKTOP);

    g_hdcSrc = CreateCompatibleDC(hdcScreen);
    g_hdcDst = CreateCompatibleDC(hdcScreen);

#ifndef UNIX
#ifndef WINNT
    SetObjectOwner(g_hdcSrc, HINST_THISDLL);
    SetObjectOwner(g_hdcDst, HINST_THISDLL);
#endif
#endif /* !UNIX */

    InitDitherBrush();

    hbmTemp = CreateBitmap(8, 8, 1, 1, stripebits);
    if (hbmTemp)
    {
        // initialize the deselect 1x1 bitmap
        g_hbmDcDeselect = SelectBitmap(g_hdcDst, hbmTemp);
        SelectBitmap(g_hdcDst, g_hbmDcDeselect);

        g_hbrStripe = CreatePatternBrush(hbmTemp);
#ifndef UNIX
#ifndef WINNT
        SetObjectOwner(g_hbrStripe, HINST_THISDLL);
#endif
#endif /* !UNIX */

        DeleteObject(hbmTemp);
    }

    ReleaseDC(HWND_DESKTOP, hdcScreen);

    if (!g_hdcSrc || !g_hdcDst || !g_hbrMonoDither)
    {
        ImageList_Terminate();
        DebugMsg(DM_ERROR, TEXT("ImageList: Unable to initialize"));
        return FALSE;
    }
    return TRUE;
}

void NEAR PASCAL ImageList_Terminate()
{
    TerminateDitherBrush();

    if (g_hbrStripe)
    {
        DeleteObject(g_hbrStripe);
        g_hbrStripe = NULL;
    }

#ifdef _WIN32
    ImageList_DeleteDragBitmaps();
#endif

    if (g_hdcDst)
    {
        ImageList_SelectDstBitmap(NULL);
        DeleteDC(g_hdcDst);
        g_hdcDst = NULL;
    }

    if (g_hdcSrc)
    {
        ImageList_SelectSrcBitmap(NULL);
        DeleteDC(g_hdcSrc);
        g_hdcSrc = NULL;
    }

    if (g_hbmWork)
    {
        DeleteBitmap(g_hbmWork);
        g_hbmWork = NULL;
    }
}

#ifdef DEBUG
void NEAR PASCAL ImageList_SelectFailed(HBITMAP hbm)
{
    DM(DM_TRACE, TEXT("Bitmap select has failed"));
}
#else
#define ImageList_SelectFailed(hbm)
#endif

void NEAR PASCAL ImageList_SelectDstBitmap(HBITMAP hbmDst)
{
    ASSERTCRITICAL;

    if (hbmDst != g_hbmDst)
    {
        // If it's selected in the source DC, then deselect it first
        //
        if (hbmDst && hbmDst == g_hbmSrc)
            ImageList_SelectSrcBitmap(NULL);

#ifndef UNIX
        if (!SelectBitmap(g_hdcDst, hbmDst ? hbmDst : g_hbmDcDeselect))
#else
        if (!SelectBitmap(g_hdcDst, hbmDst ? hbmDst : MwGetDCInitialBitmap(g_hdcDst)))
#endif
            ImageList_SelectFailed(hbmDst);
        g_hbmDst = hbmDst;
    }
}

void NEAR PASCAL ImageList_SelectSrcBitmap(HBITMAP hbmSrc)
{
    ASSERTCRITICAL;

    if (hbmSrc != g_hbmSrc)
    {
        // If it's selected in the dest DC, then deselect it first
        //
        if (hbmSrc && hbmSrc == g_hbmDst)
            ImageList_SelectDstBitmap(NULL);

#ifndef UNIX
        if (!SelectBitmap(g_hdcSrc, hbmSrc ? hbmSrc : g_hbmDcDeselect))
#else
        if (!SelectBitmap(g_hdcSrc, hbmSrc ? hbmSrc : MwGetDCInitialBitmap(g_hdcSrc)))
#endif
            ImageList_SelectFailed(hbmSrc);
        g_hbmSrc = hbmSrc;
    }
}

HDC NEAR PASCAL ImageList_GetWorkDC(HDC hdc, int dx, int dy)
{
    ASSERTCRITICAL;

    if (g_hbmWork == NULL ||
        GetDeviceCaps(hdc, BITSPIXEL) != g_bmWork.bmBitsPixel ||
        g_bmWork.bmWidth  < dx || g_bmWork.bmHeight < dy)
    {
        ImageList_DeleteBitmap(g_hbmWork);
        g_hbmWork = NULL;

        if (dx == 0 || dy == 0)
            return NULL;

        if (g_hbmWork = CreateCompatibleBitmap(hdc, dx, dy))
        {
#ifndef UNIX
#ifndef WINNT
            SetObjectOwner(g_hbmWork, HINST_THISDLL);
#endif
#endif /* !UNIX */
            GetObject(g_hbmWork, sizeof(g_bmWork), &g_bmWork);
        }
    }

    ImageList_SelectSrcBitmap(g_hbmWork);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        HPALETTE hpal;
        hpal = SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);
        SelectPalette(g_hdcSrc, hpal, TRUE);
    }

    return g_hdcSrc;
}

void NEAR PASCAL ImageList_ReleaseWorkDC(HDC hdc)
{
    ASSERTCRITICAL;
    ASSERT(hdc == g_hdcSrc);

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), TRUE);
    }
}

void NEAR PASCAL ImageList_DeleteBitmap(HBITMAP hbm)
{
    ASSERTCRITICAL;
    if (hbm)
    {
        if (g_hbmDst == hbm)
            ImageList_SelectDstBitmap(NULL);
        if (g_hbmSrc == hbm)
            ImageList_SelectSrcBitmap(NULL);
        DeleteBitmap(hbm);
    }
}


#define ILC_WIN95   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE)

//============================================================================
// ImageList_Clone - clone a image list
//
// create a new imagelist with the same properties as the given
// imagelist, except mabey a new icon size
//
//      piml    - imagelist to clone
//      cx,cy   - new icon size (0,0) to use clone icon size.
//      flags   - new flags (used if no clone)
//      cInitial- initial size
//      cGrow   - grow value (used if no clone)
//============================================================================

HIMAGELIST WINAPI ImageList_Clone(HIMAGELIST himl, int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    // This is not a public API, so we shouldn't RIP -- just cope
    if (himl && IsImageList(himl))
    {
        if (cx == 0)            // use the clone size if cx==0
            cx = himl->cx;
        if (cy == 0)            // use the clone size if cy==0
            cy = himl->cy;
        flags = himl->flags;    // always use the clone flags
        if (himl->pimlMirror)
        {
            flags |= ILC_MIRROR;
        }        
    }

    return ImageList_Create(cx,cy,flags,cInitial,cGrow);
}

//============================================================================

IMAGELIST* WINAPI ImageList_CreateHelper(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    IMAGELIST* piml = NULL;

    if (cx < 0 || cy < 0)
        return NULL;

    ENTERCRITICAL;
    if (!g_iILRefCount)
    {
        if (!ImageList_Init())
            goto Error;
    }
    g_iILRefCount++;
    
    if (flags == (DWORD)-1) {
        // some morons (visual test)
        // pass -1 for this....
        // map this to all win95 flags
        flags = ILC_WIN95;
    }

    piml = ImageList_Create2(cx, cy, flags, cGrow);

    // allocate the bitmap PLUS one re-usable entry
    if (piml)
    {
        if (flags & ILC_VIRTUAL)
        {
            piml->hdcImage = (HDC)-1;
            piml->hbmImage = (HBITMAP)-1;

            if (piml->flags & ILC_MASK)
            {
                piml->hdcMask = (HDC)-1;
                piml->hbmMask = (HBITMAP)-1;
            }
        }
        else
        {
            // make the hdc's
    
            piml->hdcImage = CreateCompatibleDC(NULL);
    
            if (piml->hdcImage == NULL)
                goto Error;
    
            if (piml->flags & ILC_MASK)
            {
                piml->hdcMask = CreateCompatibleDC(NULL);
    
                // were they both made ok?
                if (!piml->hdcMask)
                    goto Error;
            }
    
            ImageList_SetOwners(piml);
    
            if (!ImageList_ReAllocBitmaps(piml, cInitial + 1) &&
                !ImageList_ReAllocBitmaps(piml, 1))
            {
                goto Error;
            }
        }
    }

    LEAVECRITICAL;
    return piml;

Error:
    if (piml)
    ImageList_Destroy(piml);
    piml = NULL;

    LEAVECRITICAL;
    return piml;

}

IMAGELIST* WINAPI ImageList_Create(int cx, int cy, UINT flags, int cInitial, 
int cGrow)
{
    IMAGELIST* piml=NULL;
    IMAGELIST* pimlMirror=NULL;

    piml = ImageList_CreateHelper(cx, cy, flags, cInitial, cGrow);

    //
    // Let's create a mirrored imagelist, if requested.
    //
    if (piml && (piml->flags & ILC_MIRROR))
    {
        piml->flags &= ~ILC_MIRROR;
        pimlMirror = ImageList_CreateHelper(cx, cy, flags, cInitial, cGrow);
        if (pimlMirror)
        {
            pimlMirror->flags &= ~ILC_MIRROR;
            piml->pimlMirror = pimlMirror;
        }
    }

    return piml;
}

#ifdef _WIN32
#ifdef UNICODE
//
// When this code is compiled Unicode, this implements the
// ANSI version of the ImageList_LoadImage api.
//

IMAGELIST* WINAPI ImageList_LoadImageA(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
   IMAGELIST* lpResult;
   LPWSTR   lpBmpW;

   if (!IS_INTRESOURCE(lpbmp)) {
       lpBmpW = ProduceWFromA(CP_ACP, lpbmp);

       if (!lpBmpW) {
           return NULL;
       }

   }  else {
       lpBmpW = (LPWSTR)lpbmp;
   }

   lpResult = ImageList_LoadImageW(hi, lpBmpW, cx, cGrow, crMask, uType, uFlags);

   if (!IS_INTRESOURCE(lpbmp))
       FreeProducedString(lpBmpW);

   return lpResult;
}

#else

//
// When this code is compiled ANSI, this stubs the
// Unicode version of the ImageList_LoadImage api.
//

IMAGELIST* WINAPI ImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
   SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
   return NULL;
}

#endif

IMAGELIST* WINAPI ImageList_LoadImage(HINSTANCE hi, LPCTSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
    HBITMAP hbmImage;
    IMAGELIST* piml = NULL;
    BITMAP bm;
    int cy, cInitial;
    UINT flags;

    hbmImage = LoadImage(hi, lpbmp, uType, 0, 0, uFlags);
    if (!hbmImage || (sizeof(bm) != GetObject(hbmImage, sizeof(bm), &bm)))
        goto cleanup;

    // If cx is not stated assume it is the same as cy.
    // ASSERT(cx);
    cy = bm.bmHeight;

    if (cx == 0)
        cx = cy;

    cInitial = bm.bmWidth / cx;

    ENTERCRITICAL;

    flags = 0;
    if (crMask != CLR_NONE)
        flags |= ILC_MASK;
    if (bm.bmBits)
        flags |= (bm.bmBitsPixel & ILC_COLORMASK);

    piml = ImageList_Create(cx, cy, flags, cInitial, cGrow);
    if (piml)
    {
        int added;

        if (crMask == CLR_NONE)
            added = ImageList_Add(piml, hbmImage, NULL);
        else
            added = ImageList_AddMasked(piml, hbmImage, crMask);

        if (added < 0)
        {
            ImageList_Destroy(piml);
            piml = NULL;
        }
    }
    LEAVECRITICAL;

cleanup:
    if (hbmImage)
        DeleteObject(hbmImage);

    return piml;
}
#endif

IMAGELIST* NEAR PASCAL ImageList_Create2(int cx, int cy, UINT flags, int cGrow)
{
    IMAGELIST* piml;
    int i;

    if (flags & ~ILC_VALID)
    {
        // don't let bozos create imagelists with undefined flags
        RIPMSG(0, "ImageList_Create: Invalid flags %08x", flags);
        return NULL;
    }

#ifdef _WIN32
    if (flags & ILC_SHARED)
        piml = Alloc(sizeof(IMAGELIST));        // shared alloc
    else
        piml = (IMAGELIST*)LocalAlloc(LPTR, sizeof(IMAGELIST));    // non-shared
#else
    piml = (IMAGELIST*)LocalAlloc(LPTR, sizeof(IMAGELIST));    // non-shared
#endif
    if (piml)
    {
        if (cGrow < 4)
            cGrow = 4;
        else {
            // round up by 4's
            cGrow = (cGrow + 3) & ~3;
        }

        //piml->cImage = 0;
        //piml->cAlloc = 0;
        piml->cStrip = 4;
        piml->cGrow = cGrow;
        piml->cx = cx;
        piml->cy = cy;
        piml->clrBlend = CLR_NONE;
        piml->clrBk = CLR_NONE;
        piml->hbrBk = GetStockObject(BLACK_BRUSH);
        piml->fSolidBk = TRUE;
        piml->flags = flags;
        piml->pimlMirror = NULL;        
        //piml->hbmImage = NULL;
        //piml->hbmMask = NULL;
        //piml->hInstOwner = NULL;
        //piml->hdcImage = NULL;
        //piml->hdcMask = NULL;

        piml->wMagic = IMAGELIST_SIG;

        //
        // Initialize the overlay indexes to -1 since 0 is a valid index.
        //

        for (i = 0; i < NUM_OVERLAY_IMAGES; i++) {
            piml->aOverlayIndexes[i] = -1;
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("ImageList: Out of near memory"));
    }
    return piml;
}

BOOL WINAPI ImageList_SetFilter(IMAGELIST *piml, PFNIMLFILTER pfnFilter, LPARAM lParamFilter)
{
    V_HIMAGELIST(piml);

    if (!(piml->flags & ILC_VIRTUAL) || piml->pfnFilter)
    {
        // a filter can only be installed for ILC_VIRTUAL imagelists
        // once somebody sets it it can never be changed!
        ASSERT(FALSE);
        return FALSE;
    }

    piml->pfnFilter = pfnFilter;
    piml->lParamFilter = lParamFilter;
    return TRUE;
}

BOOL WINAPI ImageList_DestroyHelper(IMAGELIST* piml)
{
    ENTERCRITICAL;
    // nuke dc's
    if (piml->hdcImage && (piml->hdcImage != (HDC)-1))
    {
#ifndef UNIX
        SelectObject(piml->hdcImage, g_hbmDcDeselect);
#else
        SelectObject(piml->hdcImage, MwGetDCInitialBitmap(piml->hdcImage));
#endif /* !UNIX */
        DeleteDC(piml->hdcImage);
    }
    if (piml->hdcMask && (piml->hdcMask != (HDC)-1))
    {
#ifndef UNIX
        SelectObject(piml->hdcMask, g_hbmDcDeselect);
#else
        SelectObject(piml->hdcMask, MwGetDCInitialBitmap(piml->hdcMask));
#endif /* !UNIX */
        DeleteDC(piml->hdcMask);
    }

    // nuke bitmaps
    if (piml->hbmImage && (piml->hbmImage != (HBITMAP)-1))
        ImageList_DeleteBitmap(piml->hbmImage);

    if (piml->hbmMask && (piml->hbmMask != (HBITMAP)-1))
        ImageList_DeleteBitmap(piml->hbmMask);

    if (piml->hbrBk)
        DeleteObject(piml->hbrBk);

    // one less use of imagelists.  if it's the last, terminate the imagelist
    g_iILRefCount--;
    if (!g_iILRefCount)
        ImageList_Terminate();
    LEAVECRITICAL;

    piml->wMagic = 0;

#ifdef _WIN32
    if (piml->flags & ILC_SHARED)
        Free(piml);
    else
        LocalFree((HLOCAL)piml);
#else
    LocalFree((HLOCAL)piml);
#endif
    return TRUE;
}

BOOL WINAPI ImageList_Destroy(IMAGELIST* piml)
{
    if (!piml)
        return FALSE;
    
    V_HIMAGELIST(piml);

    //
    // Let's destroy the mirrored imagelist first, if one exists. [samera]
    // 
    if (piml->pimlMirror)
    {
        ImageList_DestroyHelper(piml->pimlMirror);
    }

    return ImageList_DestroyHelper(piml);
}

void NEAR PASCAL ImageList_SetOwners(IMAGELIST* piml)
{
#ifndef WINNT

#ifndef UNIX
    if (IsImageList(piml) &&
        ((piml->flags & (ILC_SHARED | ILC_VIRTUAL)) == ILC_SHARED))
    {
        if (piml->hbmImage) SetObjectOwner(piml->hbmImage, HINST_THISDLL);
        if (piml->hbmMask)  SetObjectOwner(piml->hbmMask,  HINST_THISDLL);
        if (piml->hbrBk)    SetObjectOwner(piml->hbrBk,    HINST_THISDLL);
        if (piml->hdcImage) SetObjectOwner(piml->hdcImage, HINST_THISDLL);
        if (piml->hdcMask)  SetObjectOwner(piml->hdcMask,  HINST_THISDLL);
    }
#endif

#endif
}

int WINAPI ImageList_GetImageCount(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);

    return piml->cImage;
}

BOOL WINAPI ImageList_SetImageCount(IMAGELIST* piml, UINT uAlloc)
{
    BOOL fResult = FALSE;

    V_HIMAGELIST(piml);

    ENTERCRITICAL;
    if (ImageList_ReAllocBitmaps(piml, -((int)uAlloc + 1)))
    {
        piml->cImage = (int)uAlloc;
        fResult = TRUE;
    }
    LEAVECRITICAL;

    return fResult;
}

BOOL WINAPI ImageList_GetIconSize(IMAGELIST *piml, int FAR *cx, int FAR *cy)
{
    V_HIMAGELIST(piml);

    if (!cx || !cy)
        return FALSE;

    *cx = piml->cx;
    *cy = piml->cy;
    return TRUE;
}

//
//  change the size of a existing image list
//  also removes all items
//
BOOL WINAPI ImageList_SetIconSizeHelper(IMAGELIST *piml, int cx, int cy)
{
    if (piml->cx == cx && piml->cy == cy)
        return FALSE;       // no change

    if (cx < 0 || cy < 0)
        return FALSE;       // invalid dimensions

    piml->cx = cx;
    piml->cy = cy;

    ImageList_Remove(piml, -1);
    return TRUE;
}

BOOL WINAPI ImageList_SetIconSize(IMAGELIST *piml, int cx, int cy)
{
    V_HIMAGELIST(piml);

   if (piml->pimlMirror)
   {
       ImageList_SetIconSizeHelper(piml->pimlMirror, cx, cy);
   }

   return ImageList_SetIconSizeHelper(piml, cx, cy);
}

//
//  ImageList_SetFlags
//
//  change the image list flags, then rebuilds the bitmaps.
//
//  the only reason to call this function is to change the
//  color depth of the image list, the shell needs to do this
//  when the screen depth changes and it wants to use HiColor icons.
//
BOOL WINAPI ImageList_SetFlags(IMAGELIST *piml, UINT flags)
{
    HBITMAP hOldImage;
    V_HIMAGELIST(piml);

    // check for valid input flags
    if (flags & ~ILC_VALID)
        return FALSE;

    // you cant change these flags.
    if ((flags ^ piml->flags) & ILC_SHARED)
        return FALSE;

    // now change the flags and rebuild the bitmaps.
    piml->flags = flags;

    // set the old bitmap to NULL, so when Imagelist_remove calls
    // ImageList_createBitmap, it will not call CreatecomptibleBitmap,
    // it will create the spec for the bitmap from scratch..
    hOldImage = piml->hbmImage;
    piml->hbmImage = NULL;
    
    ImageList_Remove(piml, -1);

    // imagelist_remove will have ensured that the old image is no longer selected
    // thus we can now delete it...
    if ( hOldImage )
        DeleteObject( hOldImage );
        
    return TRUE;
}

UINT WINAPI ImageList_GetFlags(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);
    return (piml->flags & ILC_VALID) | (piml->pimlMirror ? ILC_MIRROR : 0);
}

// reset the background color of images iFirst through iLast

void NEAR PASCAL ImageList_ResetBkColor(IMAGELIST* piml, int iFirst, int iLast, COLORREF clr)
{
    HBRUSH hbrT=NULL;
    DWORD  rop;

    if (!IsImageList(piml) || piml->hdcMask == NULL)
        return;

    if (clr == CLR_BLACK || clr == CLR_NONE)
    {
        rop = ROP_DSna;
    }
    else if (clr == CLR_WHITE)
    {
        rop = ROP_DSo;
    }
    else
    {
        ASSERT(piml->hbrBk);
        ASSERT(piml->clrBk == clr);

        rop = ROP_PatMask;
        hbrT = SelectBrush(piml->hdcImage, piml->hbrBk);
    }

    for ( ;iFirst <= iLast; iFirst++)
    {
        RECT rc;

        ImageList_GetImageRect(piml, iFirst, &rc);

        BitBlt(piml->hdcImage, rc.left, rc.top, piml->cx, piml->cy,
        piml->hdcMask, rc.left, rc.top, rop);
    }

    if (hbrT)
        SelectBrush(piml->hdcImage, hbrT);
}

//
//  GetNearestColor sucks.  If you have a 32-bit HDC with a 16-bit bitmap
//  selected into it, and you call GetNearestColor, GDI ignores the
//  color-depth of the bitmap and thinks you have a 32-bit bitmap inside,
//  so of course it returns the same color unchanged.
//
//  So instead, we have to emulate GetNearestColor with SetPixel.
//
COLORREF GetNearestColor32(HDC hdc, COLORREF rgb)
{
    COLORREF rgbT;

    rgbT = GetPixel(hdc, 0, 0);
    rgb = SetPixel(hdc, 0, 0, rgb);
    SetPixelV(hdc, 0, 0, rgbT);

    return rgb;
}

COLORREF WINAPI ImageList_SetBkColorHelper(IMAGELIST* piml, COLORREF clrBk)
{
    COLORREF clrBkOld;

    // Quick out if there is no change in color
    if (piml->clrBk == clrBk)
    {
        return clrBk;
    }

    // The following code deletes the brush, resets the background color etc.,
    // so, protect it with a critical section.
    ENTERCRITICAL;
    
    if (piml->hbrBk)
    {
        DeleteBrush(piml->hbrBk);
    }

    clrBkOld = piml->clrBk;
    piml->clrBk = clrBk;

    if ((clrBk == CLR_NONE) || (piml->flags & ILC_VIRTUAL))
    {
        piml->hbrBk = GetStockObject(BLACK_BRUSH);
        piml->fSolidBk = TRUE;
    }
    else
    {
        piml->hbrBk = CreateSolidBrush(clrBk);
        piml->fSolidBk = GetNearestColor32(piml->hdcImage, clrBk) == clrBk;
    }

    ASSERT(piml->hbrBk);

    if (piml->cImage > 0)
    {
        ImageList_ResetBkColor(piml, 0, piml->cImage - 1, clrBk);
    }

    LEAVECRITICAL;
    
    return clrBkOld;
}

COLORREF WINAPI ImageList_SetBkColor(IMAGELIST* piml, COLORREF clrBk)
{
    V_HIMAGELISTERR(piml, CLR_NONE);

   if (piml->pimlMirror)
   {
       ImageList_SetBkColorHelper(piml->pimlMirror, clrBk);
   }    

   return ImageList_SetBkColorHelper(piml, clrBk);
   
}

COLORREF WINAPI ImageList_GetBkColor(IMAGELIST* piml)
{
    V_HIMAGELISTERR(piml, CLR_NONE);

    return piml->clrBk;
}

BOOL NEAR PASCAL ImageList_ReAllocBitmaps(IMAGELIST* piml, int cAlloc)
{
    HBITMAP hbmImageNew;
    HBITMAP hbmMaskNew;
    int cx, cy;

    V_HIMAGELIST(piml);

    // HACK: don't shrink unless the caller passes a negative count
    if (cAlloc > 0)
    {
        if (piml->cAlloc >= cAlloc)
            return TRUE;
    }
    else
        cAlloc *= -1;

    hbmMaskNew = NULL;
    hbmImageNew = NULL;

    cx = piml->cx * piml->cStrip;
    cy = piml->cy * ((cAlloc + piml->cStrip - 1) / piml->cStrip);
    if (cAlloc > 0)
    {
        if (piml->flags & ILC_MASK)
        {
            hbmMaskNew = CreateMonoBitmap(cx, cy);
            if (!hbmMaskNew)
            {
                DebugMsg(DM_ERROR, TEXT("ImageList: Can't create bitmap"));
                return FALSE;
            }
        }
        hbmImageNew = ImageList_CreateBitmap(piml, cx, cy);
        if (!hbmImageNew)
        {
            if (hbmMaskNew)
                ImageList_DeleteBitmap(hbmMaskNew);
            DebugMsg(DM_ERROR, TEXT("ImageList: Can't create bitmap"));
            return FALSE;
        }
    }

    if (piml->cImage > 0)
    {
        int cyCopy = piml->cy * ((min(cAlloc, piml->cImage) + piml->cStrip - 1) / piml->cStrip);

        if (piml->flags & ILC_MASK)
        {
            ImageList_SelectDstBitmap(hbmMaskNew);
            BitBlt(g_hdcDst, 0, 0, cx, cyCopy, piml->hdcMask, 0, 0, SRCCOPY);
        }

        ImageList_SelectDstBitmap(hbmImageNew);
        BitBlt(g_hdcDst, 0, 0, cx, cyCopy, piml->hdcImage, 0, 0, SRCCOPY);
    }

    // select into DC's, delete then assign
    ImageList_SelectDstBitmap(NULL);
    ImageList_SelectSrcBitmap(NULL);
    SelectObject(piml->hdcImage, hbmImageNew);

    if (piml->hdcMask)
        SelectObject(piml->hdcMask, hbmMaskNew);

    if (piml->hbmMask)
        ImageList_DeleteBitmap(piml->hbmMask);

    if (piml->hbmImage)
        ImageList_DeleteBitmap(piml->hbmImage);

    piml->hbmMask = hbmMaskNew;
    piml->hbmImage = hbmImageNew;
    piml->clrBlend = CLR_NONE;

    ImageList_SetOwners(piml);

    piml->cAlloc = cAlloc;

    return TRUE;
}

int WINAPI ImageList_SetColorTable(IMAGELIST* piml, int start, int len, RGBQUAD *prgb)
{
    // mark it that we have set the color table so that it won't be overwritten 
    // by the first bitmap add....
    piml->fColorsSet = TRUE;
    return SetDIBColorTable(piml->hdcImage, start, len, prgb);
}

HBITMAP WINAPI ImageList_CreateMirroredBitmap( HBITMAP hbmOrig)
{
    HDC     hdc, hdcMem1, hdcMem2;
    HBITMAP hbm = NULL, hOld_bm1, hOld_bm2;
    BITMAP  bm;
    int     IncOne = 0;


    if (!hbmOrig)
        return NULL;

    if (!GetObject(hbmOrig, sizeof(BITMAP), &bm))
        return NULL;

    // Grab the screen DC
    hdc = GetDC(NULL);

    hdcMem1 = CreateCompatibleDC(hdc);

    if (!hdcMem1)
    {
        ReleaseDC(NULL, hdc);
        return NULL;
    }
    
    hdcMem2 = CreateCompatibleDC(hdc);
    if (!hdcMem2)
    {
        DeleteDC(hdcMem1);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    hbm = CreateColorBitmap(bm.bmWidth, bm.bmHeight);

    if (!hbm)
    {
        DeleteDC(hdcMem2);
        DeleteDC(hdcMem1);        
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    //
    // Flip the bitmap
    //
    hOld_bm1 = (HBITMAP)SelectObject(hdcMem1, hbmOrig);
    hOld_bm2 = (HBITMAP)SelectObject(hdcMem2 , hbm );

    SET_DC_RTL_MIRRORED(hdcMem2);

#ifndef WINNT
    // off-by-one on win98 or higher copying from non-mirrored to mirrored DC
    IncOne++;
#endif // WINNT

    BitBlt(hdcMem2, IncOne, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

    SelectObject(hdcMem1, hOld_bm1 );
    SelectObject(hdcMem1, hOld_bm2 );
    
    DeleteDC(hdcMem2);
    DeleteDC(hdcMem1);
    ReleaseDC(NULL, hdc);

    return hbm;
}

// in:
//    piml            image list to add to
//    hbmImage & hbmMask  the new image(s) to add, if multiple pass in horizontal strip
//    cImage            number of images to add in hbmImage and hbmMask
//
// returns:
//    index of new item, if more than one starting index of new items

int WINAPI ImageList_Add2(IMAGELIST* piml, HBITMAP hbmImage, HBITMAP hbmMask,
        int cImage, int xStart, int yStart)
{
    int i = -1;

    V_HIMAGELISTERR(piml, -1);

    ENTERCRITICAL;

#ifdef _WIN32
    //
    // if the ImageList is empty clone the color table of the first
    // bitmap you add to the imagelist.
    //
    // the ImageList needs to be a 8bpp image list
    // the bitmap being added needs to be a 8bpp DIBSection
    //
    if (hbmImage && piml->cImage == 0 &&
        (piml->flags & ILC_COLORMASK) != ILC_COLORDDB)
    {
        if ( !piml->fColorsSet )
        {
            int n;
            RGBQUAD argb[256];

            ImageList_SelectDstBitmap(hbmImage);

            if (n = GetDIBColorTable(g_hdcDst, 0, 256, argb))
            {
                ImageList_SetColorTable(piml, 0, n, argb);
            }

            ImageList_SelectDstBitmap(NULL);
        }
        
        piml->clrBlend = CLR_NONE;
    }
#endif

    if (piml->cImage + cImage + 1 > piml->cAlloc)
    {
        if (!ImageList_ReAllocBitmaps(piml, piml->cAlloc + max(cImage, piml->cGrow) + 1))
            goto Cleanup;
    }

    i = piml->cImage;
    piml->cImage += cImage;

    if (hbmImage && !ImageList_Replace2(piml, i, cImage, hbmImage, hbmMask, xStart, yStart))
    {
        piml->cImage -= cImage;
        i = -1;
        goto Cleanup;
    }

  Cleanup:
    LEAVECRITICAL;
    return i;
}


int WINAPI ImageList_AddHelper(IMAGELIST* piml, HBITMAP hbmImage, HBITMAP hbmMask)
{
    BITMAP bm;
    int cImage;

    ASSERT(piml);

    if (GetObject(hbmImage, sizeof(bm), &bm) != sizeof(bm) || bm.bmWidth < piml->cx)
        return -1;

    ASSERT(hbmImage);
    ASSERT(piml->cx);

    cImage = bm.bmWidth / piml->cx;     // # of images in source

    // serialization handled within Add2.
    return(ImageList_Add2(piml, hbmImage, hbmMask, cImage, 0, 0));
}

int WINAPI ImageList_Add(IMAGELIST* piml, HBITMAP hbmImage, HBITMAP hbmMask)
{
    if (!piml)
        return -1;

   if (piml->pimlMirror)
   {
       HBITMAP hbmMirroredImage = ImageList_CreateMirroredBitmap(hbmImage);
       HBITMAP hbmMirroredMask = ImageList_CreateMirroredBitmap(hbmMask);

       ImageList_AddHelper(piml->pimlMirror, hbmMirroredImage, hbmMirroredMask);

       // The caller will take care of deleting hbmImage, hbmMask
       // He knows nothing about hbmMirroredImage, hbmMirroredMask
       DeleteObject(hbmMirroredImage);
       DeleteObject(hbmMirroredMask);
   }    

   return ImageList_AddHelper(piml, hbmImage, hbmMask);
}

#ifdef _WIN32
int WINAPI ImageList_AddMaskedHelper(IMAGELIST* piml, HBITMAP hbmImage, COLORREF crMask)
{
    COLORREF crbO, crtO;
    HBITMAP hbmMask;
    int cImage;
    int retval;
    int n,i;
    BITMAP bm;
    DWORD ColorTableSave[256];
    DWORD ColorTable[256];

    if (GetObject(hbmImage, sizeof(bm), &bm) != sizeof(bm))
        return -1;

    hbmMask = CreateMonoBitmap(bm.bmWidth, bm.bmHeight);
    if (!hbmMask)
        return -1;

    ENTERCRITICAL;

    // copy color to mono, with crMask turning 1 and all others 0, then
    // punch all crMask pixels in color to 0
    ImageList_SelectSrcBitmap(hbmImage);
    ImageList_SelectDstBitmap(hbmMask);

    // crMask == CLR_DEFAULT, means use the pixel in the upper left
    //
    if (crMask == CLR_DEFAULT)
        crMask = GetPixel(g_hdcSrc, 0, 0);

    // DIBSections dont do color->mono like DDBs do, so we have to do it.
    // this only works for <=8bpp DIBSections, this method does not work
    // for HiColor DIBSections.
#ifndef MAINWIN
    //
    // This code is a workaround for a problem in Win32 when a DIB is converted to 
    // monochrome. The conversion is done according to closeness to white or black
    // and without regard to the background color. This workaround is is not required 
    // under MainWin. 
    //
    // Please note, this code has an endianship problems the comparision in the if statement
    // below is sensitive to endianship
    // ----> if (ColorTableSave[i] == RGB(GetBValue(crMask),GetGValue(crMask),GetRValue(crMask))
    //
    if (bm.bmBits != NULL && bm.bmBitsPixel <= 8)
    {
        n = GetDIBColorTable(g_hdcSrc, 0, 256, (RGBQUAD*)ColorTableSave);

        for (i=0; i<n; i++)
        {
            if (ColorTableSave[i] == RGB(GetBValue(crMask),GetGValue(crMask),GetRValue(crMask)))
                ColorTable[i] = 0x00FFFFFF;
            else
                ColorTable[i] = 0x00000000;
        }

        SetDIBColorTable(g_hdcSrc, 0, n, (RGBQUAD*)ColorTable);
    }
    else if (bm.bmBits != NULL && bm.bmBitsPixel > 8)
    {
        RIPMSG(0, "ImageList_AddMask on a bmp with more than 256 colors is not supported\r\nFix your bmp to use fewer colors");
    }
#endif
    crbO = SetBkColor(g_hdcSrc, crMask);
    BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcSrc, 0, 0, SRCCOPY);
    SetBkColor(g_hdcSrc, 0x00FFFFFFL);
    crtO = SetTextColor(g_hdcSrc, 0x00L);
    BitBlt(g_hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcDst, 0, 0, ROP_DSna);
    SetBkColor(g_hdcSrc, crbO);
    SetTextColor(g_hdcSrc, crtO);

#ifndef MAINWIN
    if (bm.bmBits != NULL && bm.bmBitsPixel <= 8)
    {
        SetDIBColorTable(g_hdcSrc, 0, n, (RGBQUAD*)ColorTableSave);
    }
#endif

    ImageList_SelectSrcBitmap(NULL);
    ImageList_SelectDstBitmap(NULL);

    ASSERT(piml->cx);
    cImage = bm.bmWidth / piml->cx;    // # of images in source

    retval = ImageList_Add2(piml, hbmImage, hbmMask, cImage, 0, 0);

    DeleteObject(hbmMask);
    LEAVECRITICAL;
    return retval;
}

int WINAPI ImageList_AddMasked(IMAGELIST* piml, HBITMAP hbmImage, COLORREF crMask)
{
    V_HIMAGELISTERR(piml, -1);

   if (piml->pimlMirror)
   {
       HBITMAP hbmMirroredImage = ImageList_CreateMirroredBitmap(hbmImage);

       ImageList_AddMaskedHelper(piml->pimlMirror, hbmMirroredImage, crMask);

       // The caller will take care of deleting hbmImage
       // He knows nothing about hbmMirroredImage
       DeleteObject(hbmMirroredImage);

   }    

   return ImageList_AddMaskedHelper(piml, hbmImage, crMask);
}

#endif

#ifdef _WIN32
BOOL WINAPI ImageList_ReplaceHelper(IMAGELIST* piml, int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
    BOOL fRet;

    if (ImageList_Filter(&piml, &i, TRUE))
        return FALSE;

    if (!IsImageListIndex(piml, i))
        return FALSE;

    ENTERCRITICAL;
    fRet = ImageList_Replace2(piml, i, 1, hbmImage, hbmMask, 0, 0);
    LEAVECRITICAL;

    return fRet;
}
#endif

BOOL WINAPI ImageList_Replace(IMAGELIST* piml, int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
    V_HIMAGELIST(piml);

   if (piml->pimlMirror)
   {
       HBITMAP hbmMirroredImage = ImageList_CreateMirroredBitmap(hbmImage);
       HBITMAP hbmMirroredMask = ImageList_CreateMirroredBitmap(hbmMask);

       ImageList_ReplaceHelper(piml->pimlMirror, i, hbmMirroredImage, hbmMirroredMask);

       // The caller will take care of deleting hbmImage, hbmMask
       // He knows nothing about hbmMirroredImage, hbmMirroredMask
       DeleteObject(hbmMirroredImage);
       DeleteObject(hbmMirroredMask);
       
   }    

   return ImageList_ReplaceHelper(piml, i, hbmImage, hbmMask);
}


// replaces images in piml with images from bitmaps
//
// in:
//    piml
//    i    index in image list to start at (replace)
//    cImage    count of images in source (hbmImage, hbmMask)
//

BOOL NEAR PASCAL ImageList_Replace2(IMAGELIST* piml, int i, int cImage, HBITMAP hbmImage, HBITMAP hbmMask,
    int xStart, int yStart)
{
    RECT rcImage;
    int x, iImage;

    V_HIMAGELIST(piml);
    ASSERT(hbmImage);

    ImageList_SelectSrcBitmap(hbmImage);
    if (piml->hdcMask) ImageList_SelectDstBitmap(hbmMask); // using as just a second source hdc

    for (x = xStart, iImage = 0; iImage < cImage; iImage++, x += piml->cx) {
    
        ImageList_GetImageRect(piml, i + iImage, &rcImage);

        if (piml->hdcMask)
        {
            BitBlt(piml->hdcMask, rcImage.left, rcImage.top, piml->cx, piml->cy,
                    g_hdcDst, x, yStart, SRCCOPY);
        }

        BitBlt(piml->hdcImage, rcImage.left, rcImage.top, piml->cx, piml->cy,
                g_hdcSrc, x, yStart, SRCCOPY);
    }

    ImageList_ResetBkColor(piml, i, i + cImage - 1, piml->clrBk);

    //
    // Bug fix : We should unselect hbmImage, so that the client can play with
    //           it. (SatoNa)
    //
    ImageList_SelectSrcBitmap(NULL);
    if (piml->hdcMask) ImageList_SelectDstBitmap(NULL);

    return TRUE;
}

#ifdef _WIN32

HICON WINAPI ImageList_GetIcon(IMAGELIST* piml, int i, UINT flags)
{
    UINT cx, cy;
    HICON hIcon = NULL;
    HBITMAP hbmMask, hbmColor;
    ICONINFO ii;

    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, &i, FALSE))
        return NULL;

    if (!IsImageListIndex(piml, i))
        return NULL;

    cx = piml->cx;
    cy = piml->cy;

    hbmColor = CreateColorBitmap(cx,cy);
    if (!hbmColor)
    {
        goto Error1;
    }
    hbmMask  = CreateMonoBitmap(cx,cy);
    if (!hbmMask)
    {
        goto Error2;
    }

    ENTERCRITICAL;
    ImageList_SelectDstBitmap(hbmMask);
    PatBlt(g_hdcDst, 0, 0, cx, cy, WHITENESS);
    ImageList_Draw(piml, i, g_hdcDst, 0, 0, ILD_MASK | flags);

    ImageList_SelectDstBitmap(hbmColor);
    PatBlt(g_hdcDst, 0, 0, cx, cy, BLACKNESS);
    ImageList_Draw(piml, i, g_hdcDst, 0, 0, ILD_TRANSPARENT | flags);

    ImageList_SelectDstBitmap(NULL);
    LEAVECRITICAL;

    ii.fIcon    = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmColor = hbmColor;
    ii.hbmMask  = hbmMask;
    hIcon = CreateIconIndirect(&ii);
    DeleteObject(hbmMask);
Error2:;
    DeleteObject(hbmColor);
Error1:;
    return hIcon;
}

#endif

#ifdef DISABLE
// this is essentially a BitBlt from one ImageList to another
//
int WINAPI ImageList_CopyImageListBlock(IMAGELIST* pimlDest, IMAGELIST* pimlSrc, int iSrc)
{
    RECT rcImage;

    V_HIMAGELISTERR(pimlDest, -1);
    V_HIMAGELISTERR(pimlSrc, -1);

    // Can't copy to itself (I'm lazy)

    if (pimlSrc == pimlDest)
    {
        return(-1);
    }

    // Check that the two image lists are "compatible"
    // BUGBUG: Should I check for the bitmaps being in the same format?
    if (pimlDest->cx!=pimlSrc->cx || pimlDest->cy!=pimlSrc->cy)
    {
        return(-1);
    }

    // Check that the source image is not out of bounds
    if (!ImageList_GetImageRect(pimlSrc, iSrc, &rcImage))
    {
        return(-1);
    }

    // Go ahead and add it
    return(ImageList_Add2(pimlDest, pimlSrc->hbmImage, pimlSrc->hbmMask, pimlSrc->cImage - iSrc,
        rcImage.left, rcImage.top));
}
#endif

// this removes an image from the bitmap but doing all the
// proper shuffling.
//
//   this does the following:
//    if the bitmap being removed is not the last in the row
//        it blts the images to the right of the one being deleted
//        to the location of the one being deleted (covering it up)
//
//    for all rows until the last row (where the last image is)
//        move the image from the next row up to the last position
//        in the current row.  then slide over all images in that
//        row to the left.

void NEAR PASCAL ImageList_RemoveItemBitmap(IMAGELIST* piml, int i)
{
    RECT rc1;
    RECT rc2;
    int dx, y;
    int x;
    
    ImageList_GetImageRect(piml, i, &rc1);
    ImageList_GetImageRect(piml, piml->cImage - 1, &rc2);

    // the row with the image being deleted, do we need to shuffle?
    // amount of stuff to shuffle
    dx = piml->cStrip * piml->cx - rc1.right;

    if (dx) {
        // yes, shuffle things left
        BitBlt(piml->hdcImage, rc1.left, rc1.top, dx, piml->cy, piml->hdcImage, rc1.right, rc1.top, SRCCOPY);
        if (piml->hdcMask)  BitBlt(piml->hdcMask,  rc1.left, rc1.top, dx, piml->cy, piml->hdcMask,  rc1.right, rc1.top, SRCCOPY);
    }

    y = rc1.top;    // top of row we are working on
    x = piml->cx * (piml->cStrip - 1); // x coord of last bitmaps in each row
    while (y < rc2.top) {
    
        // copy first from row below to last image position on this row
        BitBlt(piml->hdcImage, x, y,
                   piml->cx, piml->cy, piml->hdcImage, 0, y + piml->cy, SRCCOPY);

            if (piml->hdcMask)
                BitBlt(piml->hdcMask, x, y,
                   piml->cx, piml->cy, piml->hdcMask, 0, y + piml->cy, SRCCOPY);

        y += piml->cy;    // jump to row to slide left

        if (y <= rc2.top) {

            // slide the rest over to the left
            BitBlt(piml->hdcImage, 0, y, x, piml->cy,
                       piml->hdcImage, piml->cx, y, SRCCOPY);

            // slide the rest over to the left
            if (piml->hdcMask)
            {
                BitBlt(piml->hdcMask, 0, y, x, piml->cy,
                       piml->hdcMask, piml->cx, y, SRCCOPY);
            }
        }
    }
}

//
//  ImageList_Remove - remove a image from the image list
//
//  i - image to remove, or -1 to remove all images.
//
//  NOTE all images are "shifted" down, ie all image index's
//  above the one deleted are changed by 1
//
BOOL WINAPI ImageList_RemoveHelper(IMAGELIST* piml, int i)
{
    BOOL bRet = TRUE;

    if (ImageList_Filter(&piml, &i, TRUE))
        return FALSE;

    ENTERCRITICAL;

    if (i == -1)
    {
        piml->cImage = 0;
        piml->cAlloc = 0;

        for (i=0; i<NUM_OVERLAY_IMAGES; i++)
            piml->aOverlayIndexes[i] = -1;

        ImageList_ReAllocBitmaps(piml, -piml->cGrow);
    }
    else
    {
        if (!IsImageListIndex(piml, i))
        {
            bRet = FALSE;
        }
        else
        {
            ImageList_RemoveItemBitmap(piml, i);

            --piml->cImage;

            if (piml->cAlloc - (piml->cImage + 1) > piml->cGrow)
            ImageList_ReAllocBitmaps(piml, piml->cAlloc - piml->cGrow);
        }
    }
    LEAVECRITICAL;

    return bRet;
}

BOOL WINAPI ImageList_Remove(IMAGELIST* piml, int i)
{
    V_HIMAGELIST(piml);

    if (piml->pimlMirror)
    {
        ImageList_RemoveHelper(piml->pimlMirror, i);
    }

    return ImageList_RemoveHelper(piml, i);
}

//
//  ImageList_Copy - move an image in the image list
//
BOOL WINAPI ImageList_Copy(IMAGELIST* pimlDst, int iDst,
    IMAGELIST* pimlSrc, int iSrc, UINT uFlags)
{
    RECT rcDst, rcSrc, rcTmp;
    IMAGELIST *pimlTmp;
    BOOL bRet = FALSE;

    if (uFlags & ~ILCF_VALID)
    {
        // don't let hosers pass bogus flags
        RIPMSG(0, "ImageList_Copy: Invalid flags %08x", uFlags);
        return FALSE;
    }

    V_HIMAGELIST(pimlSrc);
    if (ImageList_Filter(&pimlDst, &iDst, TRUE) ||
        ImageList_Filter(&pimlSrc, &iSrc, (uFlags & ILCF_SWAP)))
    {
        return FALSE;
    }

    // Not supported 
    if (pimlDst != pimlSrc)
    {
        return FALSE;
    }

    ENTERCRITICAL;
    pimlTmp = (uFlags & ILCF_SWAP)? pimlSrc : NULL;

    if (ImageList_GetImageRect(pimlDst, iDst, &rcDst) &&
        ImageList_GetImageRect(pimlSrc, iSrc, &rcSrc) &&
        (!pimlTmp || ImageList_GetSpareImageRect(pimlTmp, &rcTmp)))
    {
        int cx = pimlSrc->cx;
        int cy = pimlSrc->cy;

        //
        // iff we are swapping we need to save the destination image
        //
        if (pimlTmp)
        {
            BitBlt(pimlTmp->hdcImage, rcTmp.left, rcTmp.top, cx, cy,
                   pimlDst->hdcImage, rcDst.left, rcDst.top, SRCCOPY);

            if (pimlTmp->hdcMask /*&& pimlDst->hdcMask*/)
            {
                BitBlt(pimlTmp->hdcMask, rcTmp.left, rcTmp.top, cx, cy,
                       pimlDst->hdcMask, rcDst.left, rcDst.top, SRCCOPY);
            }
        }

        //
        // copy the image
        //
        BitBlt(pimlDst->hdcImage, rcDst.left, rcDst.top, cx, cy,
           pimlSrc->hdcImage, rcSrc.left, rcSrc.top, SRCCOPY);

        if (pimlSrc->hdcMask /*&& pimlDst->hdcMask*/)
        {
            BitBlt(pimlDst->hdcMask, rcDst.left, rcDst.top, cx, cy,
                   pimlSrc->hdcMask, rcSrc.left, rcSrc.top, SRCCOPY);
        }

        //
        // iff we are swapping we need to copy the saved image too
        //
        if (pimlTmp)
        {
            BitBlt(pimlSrc->hdcImage, rcSrc.left, rcSrc.top, cx, cy,
                   pimlTmp->hdcImage, rcTmp.left, rcTmp.top, SRCCOPY);

            if (pimlSrc->hdcMask /*&& pimlTmp->hdcMask*/)
            {
                BitBlt(pimlSrc->hdcMask, rcSrc.left, rcSrc.top, cx, cy,
                       pimlTmp->hdcMask, rcTmp.left, rcTmp.top, SRCCOPY);
            }
        }

        bRet = TRUE;
    }

    LEAVECRITICAL;
    return bRet;
}

// IS_WHITE_PIXEL, BITS_ALL_WHITE are macros for looking at monochrome bits
// to determine if certain pixels are white or black.  Note that within a byte
// the most significant bit represents the left most pixel.
//
#define IS_WHITE_PIXEL(pj,x,y,cScan) \
    ((pj)[((y) * (cScan)) + ((x) >> 3)] & (1 << (7 - ((x) & 7))))

#define BITS_ALL_WHITE(b) (b == 0xff)

// Set the image iImage as one of the special images for us in combine
// drawing.  to draw with these specify the index of this
// in:
//      piml    imagelist
//      iImage  image index to use in speical drawing
//      iOverlay        index of special image, values 1-4

BOOL WINAPI ImageList_SetOverlayImageHelper(IMAGELIST* piml, int iImage, int iOverlay)
{
    RECT    rcImage;
    RECT    rc;
    int     x,y;
    int     cx,cy;
    ULONG   cScan;
    ULONG   cBits;
    HBITMAP hbmMem;
    BOOL    bRet = FALSE;

    if (ImageList_Filter(&piml, &iImage, TRUE))
        return FALSE;

    iOverlay--;         // make zero based
    if (piml->hdcMask == NULL ||
        iImage < 0 || iImage >= piml->cImage ||
        iOverlay < 0 || iOverlay >= NUM_OVERLAY_IMAGES)
        return FALSE;

    if (piml->aOverlayIndexes[iOverlay] == (SHORT)iImage)
        return TRUE;

    piml->aOverlayIndexes[iOverlay] = (SHORT)iImage;

    //
    // find minimal rect that bounds the image
    //
    ImageList_GetImageRect(piml, iImage, &rcImage);
    SetRect(&rc, 0x7FFF, 0x7FFF, 0, 0);

    //
    // now compute the black box.  This is much faster than GetPixel but
    // could still be improved by doing more operations looking at entire
    // bytes.  We basicaly get the bits in monochrome form and then use
    // a private GetPixel.  This decreased time on NT from 50 milliseconds to
    // 1 millisecond for a 32X32 image.
    //
    cx     = rcImage.right  - rcImage.left;
    cy     = rcImage.bottom - rcImage.top;

    // compute the number of bytes in a scan.  Note that they are WORD alligned
    cScan  = (((cx + (sizeof(SHORT)*8 - 1)) / 16) * 2);
    cBits  = cScan * cy;

    hbmMem = CreateBitmap(cx,cy,1,1,NULL);

    if (hbmMem)
    {
        HDC     hdcMem = CreateCompatibleDC(piml->hdcMask);

        if (hdcMem)
        {
            PBYTE   pBits  = LocalAlloc(LMEM_FIXED,cBits);
            PBYTE   pScan;

            if (pBits)
            {
                SelectObject(hdcMem,hbmMem);

                //
                // map black pixels to 0, white to 1
                //
                BitBlt(hdcMem,0,0,cx,cy,piml->hdcMask,rcImage.left,rcImage.top,SRCCOPY);

                //
                // fill in the bits
                //
                GetBitmapBits(hbmMem,cBits,pBits);

                //
                // for each scan, find the bounds
                //
                for (y = 0, pScan = pBits; y < cy; ++y,pScan += cScan)
                {
                    int i;

                    //
                    // first go byte by byte through white space
                    //
                    for (x = 0, i = 0; (i < (cx >> 3)) && BITS_ALL_WHITE(pScan[i]); ++i)
                    {
                        x += 8;
                    }

                    //
                    // now finish the scan bit by bit
                    //
                    for (; x < cx; ++x)
                    {
                        if (!IS_WHITE_PIXEL(pBits, x,y,cScan))
                        {
                            rc.left   = min(rc.left, x);
                            rc.right  = max(rc.right, x+1);
                            rc.top    = min(rc.top, y);
                            rc.bottom = max(rc.bottom, y+1);

                            // now that we found one, quickly jump to the known right edge

                            if ((x >= rc.left) && (x < rc.right))
                            {
                                x = rc.right-1;
                            }
                        }
                    }
                }

                if (rc.left == 0x7FFF) {
                    rc.left = 0;
                    ASSERT(0);
                }

                if (rc.top == 0x7FFF) {
                    rc.top = 0;
                    ASSERT(0);
                }

                piml->aOverlayDX[iOverlay] = (SHORT)(rc.right - rc.left);
                piml->aOverlayDY[iOverlay] = (SHORT)(rc.bottom- rc.top);
                piml->aOverlayX[iOverlay]  = (SHORT)(rc.left);
                piml->aOverlayY[iOverlay]  = (SHORT)(rc.top);
                piml->aOverlayF[iOverlay]  = 0;

                //
                // see if the image is non-rectanglar
                //
                // if the overlay does not require a mask to be drawn set the
                // ILD_IMAGE flag, this causes ImageList_DrawEx to just
                // draw the image, ignoring the mask.
                //
                for (y=rc.top; y<rc.bottom; y++)
                {
                    for (x=rc.left; x<rc.right; x++)
                    {
                        if (IS_WHITE_PIXEL(pBits, x, y,cScan))
                            break;
                    }

                    if (x != rc.right)
                        break;
                }

                if (y == rc.bottom)
                    piml->aOverlayF[iOverlay] = ILD_IMAGE;

                LocalFree(pBits);

                bRet = TRUE;
            }

            DeleteDC(hdcMem);
        }

        DeleteObject(hbmMem);
    }


    DebugMsg(DM_TRACE, TEXT("overlay #%d index=%d (%d,%d,%d,%d) mask:%d"), iOverlay, iImage, piml->aOverlayX[iOverlay], piml->aOverlayY[iOverlay], piml->aOverlayDX[iOverlay], piml->aOverlayDY[iOverlay],  piml->aOverlayF[iOverlay]);

    return bRet;
}

BOOL WINAPI ImageList_SetOverlayImage(IMAGELIST* piml, int iImage, int iOverlay)
{
    V_HIMAGELIST(piml);

    if (piml->pimlMirror)
    {
        ImageList_SetOverlayImageHelper(piml->pimlMirror, iImage, iOverlay);
    }

    return ImageList_SetOverlayImageHelper(piml, iImage, iOverlay);
}

/*
**  BlendCT
**
*/
void BlendCT(DWORD *pdw, DWORD rgb, UINT n, UINT count)
{
    UINT i;

    for (i=0; i<count; i++)
    {
        pdw[i] = RGB(
            ((UINT)GetRValue(pdw[i]) * (100-n) + (UINT)GetBValue(rgb) * (n)) / 100,
            ((UINT)GetGValue(pdw[i]) * (100-n) + (UINT)GetGValue(rgb) * (n)) / 100,
            ((UINT)GetBValue(pdw[i]) * (100-n) + (UINT)GetRValue(rgb) * (n)) / 100);
    }
}

/*
** ImageList_BlendDither
**
**  copy the source to the dest blended with the given color.
**
**  simulate a blend with a dither pattern.
**
*/
void ImageList_BlendDither(HDC hdcDst, int xDst, int yDst, IMAGELIST *piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    HBRUSH hbr;
    HBRUSH hbrT;
    HBRUSH hbrMask;
    HBRUSH hbrFree = NULL;         // free if non-null

    ASSERT(GetTextColor(hdcDst) == CLR_BLACK);
    ASSERT(GetBkColor(hdcDst) == CLR_WHITE);

    // choose a dither/blend brush

    switch (fStyle & ILD_BLENDMASK)
    {
        default:
        case ILD_BLEND50:
            hbrMask = g_hbrMonoDither;
            break;

        case ILD_BLEND25:
            hbrMask = g_hbrStripe;
            break;
    }

    // create (or use a existing) brush for the blend color

    switch (rgb)
    {
        case CLR_DEFAULT:
            hbr = g_hbrHighlight;
            break;

        case CLR_NONE:
            hbr = piml->hbrBk;
            break;

        default:
            if (rgb == piml->clrBk)
                hbr = piml->hbrBk;
            else
                hbr = hbrFree = CreateSolidBrush(rgb);
            break;
    }

    hbrT = SelectObject(hdcDst, hbr);
    PatBlt(hdcDst, xDst, yDst, cx, cy, PATCOPY);
    SelectObject(hdcDst, hbrT);

    hbrT = SelectObject(hdcDst, hbrMask);
    BitBlt(hdcDst, xDst, yDst, cx, cy, piml->hdcImage, x, y, ROP_MaskPat);
    SelectObject(hdcDst, hbrT);

    if (hbrFree)
        DeleteBrush(hbrFree);
}

/*
** ImageList_BlendCT
**
**  copy the source to the dest blended with the given color.
**
*/
void ImageList_BlendCT(HDC hdcDst, int xDst, int yDst, IMAGELIST *piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;

    GetObject(piml->hbmImage, sizeof(bm), &bm);

    if (rgb == CLR_DEFAULT)
        rgb = GetSysColor(COLOR_HIGHLIGHT);

    ASSERT(rgb != CLR_NONE);

    //
    // get the DIB color table and blend it, only do this when the
    // blend color changes
    //
    if (piml->clrBlend != rgb)
    {
        int n,cnt;

        piml->clrBlend = rgb;

#ifndef UNIX
        GetObject(piml->hbmImage, sizeof(piml->dib), &piml->dib.bm);
#else
        GetObject(piml->hbmImage, sizeof(DIBSECTION), &piml->dib.bm);
#endif /* !UNIX */
        cnt = GetDIBColorTable(piml->hdcImage, 0, 256, (LPRGBQUAD)&piml->dib.ct);

        if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50)
            n = 50;
        else
            n = 25;

        BlendCT(piml->dib.ct, rgb, n, cnt);
    }

    //
    // draw the image with a different color table
    //
    StretchDIBits(hdcDst, xDst, yDst, cx, cy,
        x, piml->dib.bi.biHeight-(y+cy), cx, cy,
        bm.bmBits, (LPBITMAPINFO)&piml->dib.bi, DIB_RGB_COLORS, SRCCOPY);
}


/*
**  RGB555 macros
*/
#define RGB555(r,g,b)       (((((r)>>3)&0x1F)<<10) | ((((g)>>3)&0x1F)<<5) | (((b)>>3)&0x1F))
#define R_555(w)            (int)(((w) >> 7) & 0xF8)
#define G_555(w)            (int)(((w) >> 2) & 0xF8)
#define B_555(w)            (int)(((w) << 3) & 0xF8)

/*
**  DIBXY16() macro - compute a pointer to a pixel given a (x,y)
*/
#define DIBXY16(bm,x,y) \
    (WORD*)((BYTE*)bm.bmBits + (bm.bmHeight-1-(y))*bm.bmWidthBytes + (x)*2)

/*
**  Blend16
**
**  dest.r = source.r * (1-a) + (rgb.r * a)
*/
void Blend16(
    WORD*   dst,        // destination RGB 555 bits
    int     dst_pitch,  // width in bytes of a dest scanline
    WORD*   src,        // source RGB 555 bits
    int     src_pitch,  // width in bytes of a source scanline
    int     cx,         // width in pixels
    int     cy,         // height in pixels
    DWORD   rgb,        // color to blend
    int     a)          // alpha value
{
    int i,x,y,r,g,b,sr,sg,sb;

    // subtract off width from pitch
    dst_pitch = dst_pitch - cx*2;
    src_pitch = src_pitch - cx*2;

    if (rgb == CLR_NONE)
    {
        // blending with the destination, we ignore the alpha and always
        // do 50% (this is what the old dither mask code did)

        for (y=0; y<cy; y++)
        {
            for (x=0; x<cx; x++)
            {
                *dst++ = ((*dst & 0x7BDE) >> 1) + ((*src++ & 0x7BDE) >> 1);
            }
            dst = (WORD *)((BYTE *)dst + dst_pitch);
            src = (WORD *)((BYTE *)src + src_pitch);
        }
    }
    else
    {
        // blending with a solid color

        // pre multiply source (constant) rgb by alpha
        sr = GetRValue(rgb) * a;
        sg = GetGValue(rgb) * a;
        sb = GetBValue(rgb) * a;

        // compute inverse alpha for inner loop
        a = 256 - a;

        // special case a 50% blend, to avoid a multiply

        if (a == 128)
        {
            sr = RGB555(sr>>8,sg>>8,sb>>8);

            for (y=0; y<cy; y++)
            {
                for (x=0; x<cx; x++)
                {
                    i = *src++;
                    i = sr + ((i & 0x7BDE) >> 1);
                    *dst++ = (WORD) i;
                }
                dst = (WORD *)((BYTE *)dst + dst_pitch);
                src = (WORD *)((BYTE *)src + src_pitch);
            }
        }
        else
        {
            for (y=0; y<cy; y++)
            {
                for (x=0; x<cx; x++)
                {
                    i = *src++;
                    r = (R_555(i) * a + sr) >> 8;
                    g = (G_555(i) * a + sg) >> 8;
                    b = (B_555(i) * a + sb) >> 8;
                    *dst++ = RGB555(r,g,b);
                }
                dst = (WORD *)((BYTE *)dst + dst_pitch);
                src = (WORD *)((BYTE *)src + src_pitch);
            }
        }
    }
}

/*
** ImageList_Blend16
**
**  copy the source to the dest blended with the given color.
**
**  source is assumed to be a 16 bit (RGB 555) bottom-up DIBSection
**  (this is the only kind of DIBSection we create)
*/
void ImageList_Blend16(HDC hdcDst, int xDst, int yDst, IMAGELIST *piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    RECT rc;
    int  a;

    // get bitmap info for source bitmap
    GetObject(piml->hbmImage, sizeof(bm), &bm);
    ASSERT(bm.bmBitsPixel==16);

    // get blend RGB
    if (rgb == CLR_DEFAULT)
        rgb = GetSysColor(COLOR_HIGHLIGHT);

    // get blend factor as a fraction of 256
    // only 50% or 25% is currently used.
    if ((fStyle & ILD_BLENDMASK) == ILD_BLEND50)
        a = 128;
    else
        a = 64;

    // blend the image with the specified color and place at end of image list
    ImageList_GetSpareImageRect(piml, &rc);

    // if blending with the destination, copy the dest to our work buffer
    if (rgb == CLR_NONE)
        BitBlt(piml->hdcImage, rc.left, rc.top, cx, cy, hdcDst, xDst, yDst, SRCCOPY);

    // sometimes the user can change the icon size (via plustab) between 32x32 and 48x48,
    // thus the values we have might be bigger than the actual bitmap. To prevent us from
    // crashing in Blend16 when this happens we do some bounds checks here
    if (rc.left + cx <= bm.bmWidth  &&
        rc.top  + cy <= bm.bmHeight &&
        x + cx       <= bm.bmWidth  &&
        y + cy       <= bm.bmHeight)
    {
        Blend16(DIBXY16(bm,rc.left,rc.top), -(int)bm.bmWidthBytes,
                DIBXY16(bm,x,y), -(int)bm.bmWidthBytes, cx, cy, rgb, a);
    }

    // blt blended image to the dest DC
    BitBlt(hdcDst, xDst, yDst, cx, cy, piml->hdcImage, rc.left, rc.top, SRCCOPY);
}

/*
** ImageList_Blend
**
**  copy the source to the dest blended with the given color.
**  top level function to decide what blend function to call
*/
void ImageList_Blend(HDC hdcDst, int xDst, int yDst, IMAGELIST *piml, int x, int y, int cx, int cy, COLORREF rgb, UINT fStyle)
{
    BITMAP bm;
    int bpp = GetDeviceCaps(hdcDst, BITSPIXEL);

    GetObject(piml->hbmImage, sizeof(bm), &bm);

#ifndef UNIX
    /* IEUNIX - must QA with MAINWIN before we enable it */
    //
    // if hbmImage is a DIBSection and we are on a HiColor device
    // the do a "real" blend
    //
    if (bm.bmBits && bm.bmBitsPixel <= 8 && (bpp > 8 || bm.bmBitsPixel==8))
    {
        // blend from a 4bit or 8bit DIB
        ImageList_BlendCT(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
    else if (bm.bmBits && bm.bmBitsPixel == 16 && bpp > 8)
    {
        // blend from a 16bit 555 DIB
        ImageList_Blend16(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
    else
#endif
    {
        // simulate a blend with a dither pattern.
        ImageList_BlendDither(hdcDst, xDst, yDst, piml, x, y, cx, cy, rgb, fStyle);
    }
}

/*
** Draw the image, either selected, transparent, or just a blt
**
** For the selected case, a new highlighted image is generated
** and used for the final output.
**
**      piml    ImageList to get image from.
**      i       the image to get.
**      hdc     DC to draw image to
**      x,y     where to draw image (upper left corner)
**      cx,cy   size of image to draw (0,0 means normal size)
**
**      rgbBk   background color
**              CLR_NONE            - draw tansparent
**              CLR_DEFAULT         - use bk color of the image list
**
**      rgbFg   foreground (blend) color (only used if ILD_BLENDMASK set)
**              CLR_NONE            - blend with destination (transparent)
**              CLR_DEFAULT         - use windows hilight color
**
**  if blend
**      if blend with color
**          copy image, and blend it with color.
**      else if blend with dst
**          copy image, copy mask, blend mask 50%
##
**  if ILD_TRANSPARENT
**      draw transparent (two blts) special case black or white background
**      unless we copied the mask or image
**  else if (rgbBk == piml->rgbBk && fSolidBk)
**      just blt it
**  else if mask
**      copy image
**      replace bk color
**      blt it.
**  else
**      just blt it
*/

BOOL WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp) {
    RECT rcImage;
    RECT rc;
    HBRUSH  hbrT;

    BOOL    fImage;
    HDC     hdcMask;
    HDC     hdcImage;
    int     xMask, yMask;
    int     xImage, yImage;

    V_HIMAGELIST(pimldp->himl);

    if (pimldp->cbSize != sizeof(IMAGELISTDRAWPARAMS))
        return FALSE;
    
    if (ImageList_Filter(&pimldp->himl, &pimldp->i, FALSE))
        return FALSE;

    if (!IsImageListIndex(pimldp->himl, pimldp->i))
        return FALSE;

    ENTERCRITICAL;

    //
    // If we need to use the mirrored imagelist, then let's set it.
    //
    if (pimldp->himl->pimlMirror &&
        (IS_DC_RTL_MIRRORED(pimldp->hdcDst)))
    {
        pimldp->himl = pimldp->himl->pimlMirror;
    }

    ImageList_GetImageRect(pimldp->himl, pimldp->i, &rcImage);
    rcImage.left += pimldp->xBitmap;
    rcImage.top += pimldp->yBitmap;
        
    if (pimldp->rgbBk == CLR_DEFAULT)
        pimldp->rgbBk = pimldp->himl->clrBk;

    if (pimldp->rgbBk == CLR_NONE)
        pimldp->fStyle |= ILD_TRANSPARENT;

    if (pimldp->cx == 0)
        pimldp->cx = rcImage.right  - rcImage.left;

    if (pimldp->cy == 0)
        pimldp->cy = rcImage.bottom - rcImage.top;

again:
    hdcMask = pimldp->himl->hdcMask;
    xMask = rcImage.left;
    yMask = rcImage.top;

    hdcImage = pimldp->himl->hdcImage;
    xImage = rcImage.left;
    yImage = rcImage.top;

    if (pimldp->fStyle & ILD_BLENDMASK)
    {
        // make a copy of the image, because we will have to modify it
        hdcImage = ImageList_GetWorkDC(pimldp->hdcDst, pimldp->cx, pimldp->cy);
        xImage = 0;
        yImage = 0;

        //
        //  blend with the destination
        //  by "oring" the mask with a 50% dither mask
        //
        if (pimldp->rgbFg == CLR_NONE && hdcMask)
        {
            if ((pimldp->himl->flags & ILC_COLORMASK) == ILC_COLOR16 &&
                !(pimldp->fStyle & ILD_MASK))
            {
                // copy dest to our work buffer
                BitBlt(hdcImage, 0, 0, pimldp->cx, pimldp->cy, pimldp->hdcDst, pimldp->x, pimldp->y, SRCCOPY);

                // blend source into our work buffer
                ImageList_Blend16(hdcImage, 0, 0,
                    pimldp->himl, rcImage.left, rcImage.top, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
            }
            else
            {
                ImageList_GetSpareImageRect(pimldp->himl, &rc);
                xMask = rc.left;
                yMask = rc.top;

                // copy the source image
                BitBlt(hdcImage, 0, 0, pimldp->cx, pimldp->cy,
                       pimldp->himl->hdcImage, rcImage.left, rcImage.top, SRCCOPY);

                // make a dithered copy of the mask
                hbrT = SelectObject(hdcMask, g_hbrMonoDither);
                BitBlt(hdcMask, rc.left, rc.top, pimldp->cx, pimldp->cy,
                       hdcMask, rcImage.left, rcImage.top, ROP_PSo);
                SelectObject(hdcMask, hbrT);
            }

            pimldp->fStyle |= ILD_TRANSPARENT;
        }
        else
        {
            // blend source into our work buffer
            ImageList_Blend(hdcImage, 0, 0,
                pimldp->himl, rcImage.left, rcImage.top, pimldp->cx, pimldp->cy, pimldp->rgbFg, pimldp->fStyle);
        }
    }

    // is the source image from the image list (not hdcWork)
    fImage = hdcImage == pimldp->himl->hdcImage;

    //
    // ILD_MASK means draw the mask only
    //
    if ((pimldp->fStyle & ILD_MASK) && hdcMask)
    {
        DWORD dwRop;
        
        ASSERT(GetTextColor(pimldp->hdcDst) == CLR_BLACK);
        ASSERT(GetBkColor(pimldp->hdcDst) == CLR_WHITE);
        
        if (pimldp->fStyle & ILD_ROP)
            dwRop = pimldp->dwRop;
        else if (pimldp->fStyle & ILD_TRANSPARENT)
            dwRop = SRCAND;
        else 
            dwRop = SRCCOPY;
        
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMask, xMask, yMask, dwRop);
    }
    else if (pimldp->fStyle & ILD_IMAGE)
    {
        COLORREF clrBk = GetBkColor(hdcImage);
        DWORD dwRop;
        
        if (pimldp->rgbBk != CLR_DEFAULT) {
            SetBkColor(hdcImage, pimldp->rgbBk);
        }
        
        if (pimldp->fStyle & ILD_ROP)
            dwRop = pimldp->dwRop;
        else
            dwRop = SRCCOPY;
        
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, dwRop);
        
        SetBkColor(hdcImage, clrBk);
    }
    //
    // if there is a mask and the drawing is to be transparent,
    // use the mask for the drawing.
    //
    else if ((pimldp->fStyle & ILD_TRANSPARENT) && hdcMask)
    {
//
// on NT dont dork around, just call MaskBlt
//
#if defined(USE_MASKBLT) && !defined(MAINWIN)
        MaskBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, pimldp->himl->hbmMask, xMask, yMask, 0xCCAA0000);
#else
        COLORREF clrTextSave;
        COLORREF clrBkSave;

        //
        //  we have some special cases:
        //
        //  if the background color is black, we just do a AND then OR
        //  if the background color is white, we just do a OR then AND
        //  otherwise change source, then AND then OR
        //

        clrTextSave = SetTextColor(pimldp->hdcDst, CLR_BLACK);
        clrBkSave = SetBkColor(pimldp->hdcDst, CLR_WHITE);

        // we cant do white/black special cases if we munged the mask or image

        if (fImage && pimldp->himl->clrBk == CLR_WHITE)
        {
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMask,  xMask, yMask,   ROP_DSno);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, ROP_DSa);
        }
        else if (fImage && (pimldp->himl->clrBk == CLR_BLACK || pimldp->himl->clrBk == CLR_NONE))
        {
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMask,  xMask, yMask,   ROP_DSa);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, ROP_DSo);
        }
        else
        {
            ASSERT(GetTextColor(hdcImage) == CLR_BLACK);
            ASSERT(GetBkColor(hdcImage) == CLR_WHITE);

            // black out the source image.
            BitBlt(hdcImage, xImage, yImage, pimldp->cx, pimldp->cy, hdcMask, xMask, yMask, ROP_DSna);

            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcMask,  xMask,  yMask,  ROP_DSa);
            BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, ROP_DSo);

            // restore the bkcolor, if it came from the image list
            if (fImage)
                ImageList_ResetBkColor(pimldp->himl, pimldp->i, pimldp->i, pimldp->himl->clrBk);
        }

        SetTextColor(pimldp->hdcDst, clrTextSave);
        SetBkColor(pimldp->hdcDst, clrBkSave);
#endif
    }
    else if (fImage && pimldp->rgbBk == pimldp->himl->clrBk && pimldp->himl->fSolidBk)
    {
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, SRCCOPY);
    }
    else if (hdcMask)
    {
        if (fImage && 
            ((pimldp->rgbBk == pimldp->himl->clrBk && 
               !pimldp->himl->fSolidBk) || 
              GetNearestColor32(hdcImage, pimldp->rgbBk) != pimldp->rgbBk))
        {
            // make a copy of the image, because we will have to modify it
            hdcImage = ImageList_GetWorkDC(pimldp->hdcDst, pimldp->cx, pimldp->cy);
            xImage = 0;
            yImage = 0;
            fImage = FALSE;

            BitBlt(hdcImage, 0, 0, pimldp->cx, pimldp->cy, pimldp->himl->hdcImage, rcImage.left, rcImage.top, SRCCOPY);
        }

        SetBrushOrg(hdcImage, xImage-pimldp->x, yImage-pimldp->y);
        hbrT = SelectBrush(hdcImage, CreateSolidBrush(pimldp->rgbBk));
        BitBlt(hdcImage, xImage, yImage, pimldp->cx, pimldp->cy, hdcMask, xMask, yMask, ROP_PatMask);
        DeleteObject(SelectBrush(hdcImage, hbrT));
        SetBrushOrg(hdcImage, 0, 0);

        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, SRCCOPY);

        if (fImage)
            ImageList_ResetBkColor(pimldp->himl, pimldp->i, pimldp->i, pimldp->himl->clrBk);
    }
    else
    {
        BitBlt(pimldp->hdcDst, pimldp->x, pimldp->y, pimldp->cx, pimldp->cy, hdcImage, xImage, yImage, SRCCOPY);
    }

    //
    // now deal with a overlay image, use the minimal bounding rect (and flags)
    // we computed in ImageList_SetOverlayImage()
    //
    if (pimldp->fStyle & ILD_OVERLAYMASK)
    {
        int n = OVERLAYMASKTOINDEX(pimldp->fStyle);

        if (n < NUM_OVERLAY_IMAGES) {
            pimldp->i = pimldp->himl->aOverlayIndexes[n];
            ImageList_GetImageRect(pimldp->himl, pimldp->i, &rcImage);

            pimldp->cx = pimldp->himl->aOverlayDX[n];
            pimldp->cy = pimldp->himl->aOverlayDY[n];
            pimldp->x += pimldp->himl->aOverlayX[n];
            pimldp->y += pimldp->himl->aOverlayY[n];
            rcImage.left += pimldp->himl->aOverlayX[n]+pimldp->xBitmap;
            rcImage.top  += pimldp->himl->aOverlayY[n]+pimldp->yBitmap;

            pimldp->fStyle &= ILD_MASK;
            pimldp->fStyle |= ILD_TRANSPARENT;
            pimldp->fStyle |= pimldp->himl->aOverlayF[n];

            if (pimldp->cx > 0 && pimldp->cy > 0)
                goto again;  // ImageList_DrawEx(piml, i, hdcDst, x, y, 0, 0, CLR_DEFAULT, CLR_NONE, fStyle);
        }
    }

    if (!fImage)
    {
        ImageList_ReleaseWorkDC(hdcImage);
    }

    LEAVECRITICAL;

    return TRUE;
}


BOOL WINAPI ImageList_DrawEx(IMAGELIST* piml, int i, HDC hdcDst, int x, int y, int cx, int cy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    imldp.cbSize = sizeof(imldp);
    imldp.himl   = piml;
    imldp.i      = i;
    imldp.hdcDst = hdcDst;
    imldp.x      = x;
    imldp.y      = y;
    imldp.cx     = cx;
    imldp.cy     = cy;
    imldp.xBitmap= 0;
    imldp.yBitmap= 0;
    imldp.rgbBk  = rgbBk;
    imldp.rgbFg  = rgbFg;
    imldp.fStyle = fStyle;
    
    return ImageList_DrawIndirect(&imldp);

}

BOOL WINAPI ImageList_Draw(IMAGELIST* piml, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
    IMAGELISTDRAWPARAMS imldp;

    imldp.cbSize = sizeof(imldp);
    imldp.himl   = piml;
    imldp.i      = i;
    imldp.hdcDst = hdcDst;
    imldp.x      = x;
    imldp.y      = y;
    imldp.cx     = 0;
    imldp.cy     = 0;
    imldp.xBitmap= 0;
    imldp.yBitmap= 0;
    imldp.rgbBk  = CLR_DEFAULT;
    imldp.rgbFg  = CLR_DEFAULT;
    imldp.fStyle = fStyle;
    
    return ImageList_DrawIndirect(&imldp);
}

#ifdef _WIN32
BOOL WINAPI ImageList_GetImageInfo(IMAGELIST* piml, int i, IMAGEINFO FAR* pImageInfo)
{
    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, &i, TRUE))
        return FALSE;

    RIPMSG(pImageInfo != NULL, "ImageList_GetImageInfo: Invalid NULL pointer");
    RIPMSG(IsImageListIndex(piml, i), "ImageList_GetImageInfo: Invalid image index %d", i);
    if (!pImageInfo || !IsImageListIndex(piml, i))
        return FALSE;

    pImageInfo->hbmImage      = piml->hbmImage;
    pImageInfo->hbmMask       = piml->hbmMask;

    return ImageList_GetImageRect(piml, i, &pImageInfo->rcImage);
}
#endif

//
// Parameter:
//  i -- -1 to add
//
int WINAPI ImageList_ReplaceIconHelper(IMAGELIST* piml, int i, HICON hIcon)
{
    HICON hIconT = hIcon;
    RECT rc;

    if (ImageList_Filter(&piml, &i, TRUE))
        return FALSE;

    DM(DM_TRACE, TEXT("ImageList_ReplaceIcon"));
    
    
    // be win95 compatible
    if (i < -1)
        return -1;
    

    //
    //  re-size the icon (iff needed) by calling CopyImage
    //
#if defined(_WIN32) || defined(UNIX)
    hIcon = CopyImage(hIconT, IMAGE_ICON, piml->cx, piml->cy,LR_COPYFROMRESOURCE | LR_COPYRETURNORG);
#else
    hIcon = CopyImage(HINST_THISDLL,hIconT, IMAGE_ICON, piml->cx, piml->cy,LR_COPYFROMRESOURCE | LR_COPYRETURNORG);
#endif

    if (hIcon == NULL)
        return -1;

    //
    //  alocate a slot for the icon
    //
    if (i == -1)
        i = ImageList_Add2(piml,NULL,NULL,1,0,0);

    if (i == -1)
        return -1;

    //
    //  now draw it into the image bitmaps
    //
    if (!ImageList_GetImageRect(piml, i, &rc))
        return -1;

    FillRect(piml->hdcImage, &rc, piml->hbrBk);
    DrawIconEx(piml->hdcImage, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_NORMAL);

    if (piml->hdcMask)
        DrawIconEx(piml->hdcMask, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_MASK);

    //
    // if we had user size a new icon, delete it.
    //
    if (hIcon != hIconT)
        DestroyIcon(hIcon);

    return i;
}

int WINAPI ImageList_ReplaceIcon(IMAGELIST* piml, int i, HICON hIcon)
{
    V_HIMAGELISTERR(piml, -1);

    // Let's add it first to the mirrored image list, if one exists
    if (piml->pimlMirror)
    {
        HICON hIconT = CopyIcon(hIcon);
        if (hIconT)
        {
            MirrorIcon(&hIconT, NULL);
            ImageList_ReplaceIconHelper(piml->pimlMirror, i, hIconT);
            DestroyIcon(hIconT);
        }
    }

    return ImageList_ReplaceIconHelper(piml, i, hIcon);
}

//
//
#undef ImageList_AddIcon
int WINAPI ImageList_AddIcon(IMAGELIST* piml, HICON hIcon)
{
    return ImageList_ReplaceIcon(piml, -1, hIcon);
}

/*--------------------------------------------------------------------
** make a dithered copy of the source image in the destination image.
** allows placing of the final image in the destination.
**--------------------------------------------------------------------*/
void WINAPI ImageList_CopyDitherImage(IMAGELIST *pimlDst, WORD iDst,
    int xDst, int yDst, IMAGELIST *pimlSrc, int iSrc, UINT fStyle)
{
    RECT rc;
    int x, y;

    V_HIMAGELISTVOID(pimlDst);
    V_HIMAGELISTVOID(pimlSrc);
    
    if (ImageList_Filter(&pimlDst, &iDst, TRUE) ||
        ImageList_Filter(&pimlSrc, &iSrc, FALSE))
    {
        return;
    }

    //
    // Let's get the right one for the mirrored process.
    //
    if (pimlSrc->pimlMirror && (fStyle & ILD_MIRROR))
    {
        pimlSrc = pimlSrc->pimlMirror;
        fStyle &= ~ILD_MIRROR;
    }

    ImageList_GetImageRect(pimlDst, iDst, &rc);

    // coordinates in destination image list
    x = xDst + rc.left;
    y = yDst + rc.top;

    fStyle &= ILD_OVERLAYMASK;
    ImageList_DrawEx(pimlSrc, iSrc, pimlDst->hdcImage, x, y, 0, 0, CLR_DEFAULT, CLR_NONE, ILD_IMAGE | fStyle);

#ifdef HICOLOR_DRAG
    //
    // dont dither the mask on a hicolor device, we will draw the image
    // with blending while dragging.
    //
    if (pimlDst->hdcMask && GetScreenDepth() > 8)
        ImageList_DrawEx(pimlSrc, iSrc, pimlDst->hdcMask, x, y, 0, 0, CLR_NONE, CLR_NONE, ILD_MASK | fStyle);
    else
#endif
    if (pimlDst->hdcMask)
        ImageList_DrawEx(pimlSrc, iSrc, pimlDst->hdcMask,  x, y, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50|ILD_MASK | fStyle);

    ImageList_ResetBkColor(pimlDst, iDst, iDst+1, pimlDst->clrBk);
}

/////////////////////////////////////////////////////////////////////////////
// drag stuff is only WIN32
/////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

//
//  Cached bitmaps that we use during drag&drop. We re-use those bitmaps
// across multiple drag session as far as the image size is the same.
//
struct DRAGRESTOREBMP {
    int     BitsPixel;
    HBITMAP hbmOffScreen;
    HBITMAP hbmRestore;
    SIZE    sizeRestore;
} g_drb = {
    0, NULL, NULL, {-1,-1}
};

BOOL NEAR PASCAL ImageList_CreateDragBitmaps(IMAGELIST* piml)
{
    HDC hdc;

    V_HIMAGELIST(piml);

    hdc = GetDC(NULL);

    if (piml->cx != g_drb.sizeRestore.cx ||
        piml->cy != g_drb.sizeRestore.cy ||
        GetDeviceCaps(hdc, BITSPIXEL) != g_drb.BitsPixel)
    {
        ImageList_DeleteDragBitmaps();

        g_drb.BitsPixel      = GetDeviceCaps(hdc, BITSPIXEL);
        g_drb.sizeRestore.cx = piml->cx;
        g_drb.sizeRestore.cy = piml->cy;

#ifndef WINNT
        // Win98 BiDi localized, Off-by one when you bitblt on a mirrored DC (later when you use this bitmap),
        // if you have the bitmap with different color depth than the DC.
        if(IS_BIDI_LOCALIZED_SYSTEM())
        {
            g_drb.hbmRestore   = CreateCompatibleBitmap(hdc, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy);
            g_drb.hbmOffScreen = CreateCompatibleBitmap(hdc, g_drb.sizeRestore.cx * 2 - 1, g_drb.sizeRestore.cy * 2 - 1);
        }
        else
        {
            g_drb.hbmRestore   = CreateColorBitmap(g_drb.sizeRestore.cx, g_drb.sizeRestore.cy);
            g_drb.hbmOffScreen = CreateColorBitmap(g_drb.sizeRestore.cx * 2 - 1, g_drb.sizeRestore.cy * 2 - 1);            
        }
        
#else
        g_drb.hbmRestore   = CreateColorBitmap(g_drb.sizeRestore.cx, g_drb.sizeRestore.cy);
        g_drb.hbmOffScreen = CreateColorBitmap(g_drb.sizeRestore.cx * 2 - 1, g_drb.sizeRestore.cy * 2 - 1);

#endif

        if (!g_drb.hbmRestore || !g_drb.hbmOffScreen)
        {
            ImageList_DeleteDragBitmaps();
            ReleaseDC(NULL, hdc);
            return FALSE;
        }
#ifndef UNIX
#ifndef WINNT
        SetObjectOwner(g_drb.hbmRestore, HINST_THISDLL);
        SetObjectOwner(g_drb.hbmOffScreen, HINST_THISDLL);
#endif
#endif /* !UNIX */
    }
    ReleaseDC(NULL, hdc);
    return TRUE;
}

void NEAR PASCAL ImageList_DeleteDragBitmaps()
{
    if (g_drb.hbmRestore)
    {
        ImageList_DeleteBitmap(g_drb.hbmRestore);
        g_drb.hbmRestore = NULL;
    }
    if (g_drb.hbmOffScreen)
    {
        ImageList_DeleteBitmap(g_drb.hbmOffScreen);
        g_drb.hbmOffScreen = NULL;
    }

    g_drb.sizeRestore.cx = -1;
    g_drb.sizeRestore.cy = -1;
}

//
//  Drag context. We don't reuse none of them across two different
// drag sessions. I'm planning to allocate it for each session
// to minimize critical sections.
//
struct DRAGCONTEXT {
    IMAGELIST* pimlDrag;    // Image to be drawin while dragging
    IMAGELIST* pimlCursor;  // Overlap cursor image
    IMAGELIST* pimlDither;  // Dithered image
    int        iCursor;     // Image index of the cursor
    POINT      ptDrag;      // current drag position (hwndDC coords)
    POINT      ptDragHotspot;
    POINT      ptCursor;
    BOOL       fDragShow;
    BOOL       fHiColor;
    HWND       hwndDC;
} g_dctx = {
    (IMAGELIST*)NULL, (IMAGELIST*)NULL, (IMAGELIST*)NULL,
    -1,
    {0, 0}, {0, 0}, {0, 0},
    FALSE,
    FALSE,
    (HWND)NULL
};

#ifndef UNIX
HDC ImageList_GetDragDC()
{
    HDC hdc = GetDCEx(g_dctx.hwndDC, NULL, DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE);
    //
    // If hdc is mirrored then mirror the 2 globals DCs.
    //
    if (IS_DC_RTL_MIRRORED(hdc)) {
        SET_DC_RTL_MIRRORED(g_hdcDst);
        SET_DC_RTL_MIRRORED(g_hdcSrc);
    }
    return hdc;
}
#else
#define ImageList_GetDragDC()           GetDCEx(g_dctx.hwndDC, NULL, DCX_CACHE | DCX_LOCKWINDOWUPDATE)
#endif /* !UNIX */

void ImageList_ReleaseDragDC(HDC hdc)
{
    //
    // If the hdc is mirrored then unmirror the 2 globals DCs.
    //
    if (IS_DC_RTL_MIRRORED(hdc)) {
        SET_DC_LAYOUT(g_hdcDst, 0);
        SET_DC_LAYOUT(g_hdcSrc, 0);
    }

    ReleaseDC(g_dctx.hwndDC, hdc);
}

//
//  x, y     -- Specifies the initial cursor position in the coords of hwndLock,
//              which is specified by the previous ImageList_StartDrag call.
//
BOOL WINAPI ImageList_DragMove(int x, int y)
{
    int IncOne = 0;
    ENTERCRITICAL;
    if (g_dctx.fDragShow)
    {
        RECT rcOld, rcNew, rcBounds;
        int dx, dy;

        dx = x - g_dctx.ptDrag.x;
        dy = y - g_dctx.ptDrag.y;
        rcOld.left = g_dctx.ptDrag.x - g_dctx.ptDragHotspot.x;
        rcOld.top = g_dctx.ptDrag.y - g_dctx.ptDragHotspot.y;
        rcOld.right = rcOld.left + g_drb.sizeRestore.cx;
        rcOld.bottom = rcOld.top + g_drb.sizeRestore.cy;
        rcNew = rcOld;
        OffsetRect(&rcNew, dx, dy);

        if (!IntersectRect(&rcBounds, &rcOld, &rcNew))
        {
            //
            // No intersection. Simply hide the old one and show the new one.
            //
            ImageList_DragShowNolock(FALSE);
            g_dctx.ptDrag.x = x;
            g_dctx.ptDrag.y = y;
            ImageList_DragShowNolock(TRUE);
        }
        else
        {
            //
            // Some intersection.
            //
            HDC hdcScreen;
            int cx, cy;

            UnionRect(&rcBounds, &rcOld, &rcNew);

            hdcScreen = ImageList_GetDragDC();

            //
            // If the DC is RTL mirrored, then restrict the
            // screen bitmap  not to go beyond the screen since 
            // we will end up copying the wrong bits from the
            // hdcScreen to the hbmOffScreen when the DC is mirrored.
            // GDI will skip invalid screen coord from the screen into
            // the destination bitmap. This will result in copying un-init
            // bits back to the screen (since the screen is mirrored).
            // [samera]
            //
            if (IS_DC_RTL_MIRRORED(hdcScreen))
            {
                RECT rcWindow;
#ifndef WINNT                
                // Fix off by one mirroring bug on Win98
                ++IncOne;                
#endif // WINNT
                GetWindowRect(g_dctx.hwndDC, &rcWindow);
                rcWindow.right -= rcWindow.left;

                if (rcBounds.right > rcWindow.right)
                {
                    rcBounds.right = rcWindow.right;
                }

                if (rcBounds.left < 0)
                {
                    rcBounds.left = 0;
                }
            }

            cx = rcBounds.right - rcBounds.left;
            cy = rcBounds.bottom - rcBounds.top;

            //
            // Copy the union rect from the screen to hbmOffScreen.
            //
            ImageList_SelectDstBitmap(g_drb.hbmOffScreen);
            BitBlt(g_hdcDst, 0, 0, cx, cy,
                    hdcScreen, rcBounds.left, rcBounds.top, SRCCOPY);

            //
            // Hide the cursor on the hbmOffScreen by copying hbmRestore.
            //
            ImageList_SelectSrcBitmap(g_drb.hbmRestore);
            BitBlt(g_hdcDst,
                    rcOld.left - rcBounds.left,
                    rcOld.top - rcBounds.top,
                    g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                    g_hdcSrc, 0, 0, SRCCOPY);

            //
            // Copy the original screen bits to hbmRestore
            //
            BitBlt(g_hdcSrc, 0, 0, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                    g_hdcDst,
                    rcNew.left - rcBounds.left,
                    rcNew.top - rcBounds.top,
                    SRCCOPY);

            //
            // Draw the image on hbmOffScreen
            //
#ifdef HICOLOR_DRAG
            if (g_dctx.fHiColor)
            {
                ImageList_DrawEx(g_dctx.pimlDrag, 0, g_hdcDst,
                        rcNew.left - rcBounds.left + IncOne,
                        rcNew.top - rcBounds.top, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50);

                if (g_dctx.pimlCursor)
                {
                    ImageList_Draw(g_dctx.pimlCursor, g_dctx.iCursor, g_hdcDst,
                            rcNew.left - rcBounds.left + g_dctx.ptCursor.x + IncOne,
                            rcNew.top - rcBounds.top + g_dctx.ptCursor.y,
                            ILD_NORMAL);
                            
                }
            }
            else
#endif
            {
                ImageList_Draw(g_dctx.pimlDrag, 0, g_hdcDst,
                        rcNew.left - rcBounds.left + IncOne,
                        rcNew.top - rcBounds.top, ILD_NORMAL);
            }

            //
            // Copy the hbmOffScreen back to the screen.
            //
            BitBlt(hdcScreen, rcBounds.left, rcBounds.top, cx, cy,
                    g_hdcDst, 0, 0, SRCCOPY);

            ImageList_ReleaseDragDC(hdcScreen);

            g_dctx.ptDrag.x = x;
            g_dctx.ptDrag.y = y;
        }
    }
    LEAVECRITICAL;
    return TRUE;
}

BOOL WINAPI ImageList_BeginDrag(IMAGELIST* pimlTrack, int iTrack, int dxHotspot, int dyHotspot)
{
    BOOL fRet = FALSE;;
    V_HIMAGELIST(pimlTrack);

    if (ImageList_Filter(&pimlTrack, &iTrack, FALSE))
        return FALSE;

    ENTERCRITICAL;
    if (!g_dctx.pimlDrag)
    {
        UINT flags;

        g_dctx.fDragShow = FALSE;
        g_dctx.hwndDC = NULL;

#ifdef HICOLOR_DRAG
        /*
        ** is this a HiColor drag?
        */
        g_dctx.fHiColor = GetScreenDepth() > 8;
#endif

        /*
        ** make a copy of the drag image
        */
        flags = pimlTrack->flags|ILC_SHARED;

        if (g_dctx.fHiColor)
            flags = (flags & ~ILC_COLORMASK) | ILC_COLOR16;

        g_dctx.pimlDither = ImageList_Create(
            pimlTrack->cx, pimlTrack->cy, flags, 1, 0);

        if (g_dctx.pimlDither)
        {
            g_dctx.pimlDither->cImage++;
            g_dctx.ptDragHotspot.x = dxHotspot;
            g_dctx.ptDragHotspot.y = dyHotspot;

            ImageList_CopyOneImage(g_dctx.pimlDither, 0, 0, 0, pimlTrack, iTrack);
            fRet = ImageList_SetDragImage(g_dctx.pimlDither, 0, dxHotspot, dyHotspot);
        }
    }
    LEAVECRITICAL;

    return fRet;
}

BOOL WINAPI ImageList_DragEnter(HWND hwndLock, int x, int y)
{
    BOOL fRet = FALSE;

    hwndLock = hwndLock ? hwndLock : GetDesktopWindow();

    ENTERCRITICAL;
    if (!g_dctx.hwndDC)
    {
        g_dctx.hwndDC = hwndLock;

        g_dctx.ptDrag.x = x;
        g_dctx.ptDrag.y = y;

        ImageList_DragShowNolock(TRUE);
        fRet = TRUE;
    }
    LEAVECRITICAL;

    return fRet;
}

BOOL WINAPI ImageList_DragLeave(HWND hwndLock)
{
    BOOL fRet = FALSE;

    hwndLock = hwndLock ? hwndLock : GetDesktopWindow();

    ENTERCRITICAL;
    if (g_dctx.hwndDC == hwndLock)
    {
        ImageList_DragShowNolock(FALSE);
        g_dctx.hwndDC = NULL;
        fRet = TRUE;
    }
    LEAVECRITICAL;

    return fRet;
}

#if 0
//
//  hwndLock -- Specifies the window to be used to draw destination feedback.
//              NULL indicates the whole screen.
//  x, y     -- Specifies the initial cursor position in hwndLock coords.
//
BOOL WINAPI ImageList_StartDrag(IMAGELIST* pimlTrack, HWND hwndLock, int iTrack, int x, int y, int dxHotspot, int dyHotspot)
{
    BOOL fRet = FALSE;
    V_HIMAGELIST(pimlTrack);

    if (fRet = ImageList_BeginDrag(pimlTrack, iTrack, dxHotspot, dyHotspot))
    {
        fRet = ImageList_DragEnter(hwndLock, x, y);
    }

    return fRet;
}
#endif


BOOL WINAPI ImageList_DragShowNolock(BOOL fShow)
{
    HDC hdcScreen;
    int x, y;
    int IncOne = 0;

    x = g_dctx.ptDrag.x - g_dctx.ptDragHotspot.x;
    y = g_dctx.ptDrag.y - g_dctx.ptDragHotspot.y;

    if (!g_dctx.pimlDrag)
        return FALSE;

    //
    // REVIEW: Why this block is in the critical section? We are supposed
    //  to have only one dragging at a time, aren't we?
    //
    ENTERCRITICAL;
    if (fShow && !g_dctx.fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

#ifndef WINNT 
        if (IS_DC_RTL_MIRRORED(hdcScreen)) {
            // Fix off by one mirroring bug on Win98
            ++IncOne;                
        }        
#endif // WINNT        

        ImageList_SelectSrcBitmap(g_drb.hbmRestore);

        BitBlt(g_hdcSrc, 0, 0, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                hdcScreen, x, y, SRCCOPY);

#ifdef HICOLOR_DRAG
        if (g_dctx.fHiColor)
        {
            ImageList_DrawEx(g_dctx.pimlDrag, 0, hdcScreen, x + IncOne, y, 0, 0, CLR_NONE, CLR_NONE, ILD_BLEND50);
            
            if (g_dctx.pimlCursor)
            {
                ImageList_Draw(g_dctx.pimlCursor, g_dctx.iCursor, hdcScreen,
                    x + g_dctx.ptCursor.x + IncOne, y + g_dctx.ptCursor.y, ILD_NORMAL);
            }
        }
        else
#endif
        {
            ImageList_Draw(g_dctx.pimlDrag, 0, hdcScreen, x + IncOne, y, ILD_NORMAL);
        }

        ImageList_ReleaseDragDC(hdcScreen);
    }
    else if (!fShow && g_dctx.fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

        ImageList_SelectSrcBitmap(g_drb.hbmRestore);

        BitBlt(hdcScreen, x, y, g_drb.sizeRestore.cx, g_drb.sizeRestore.cy,
                g_hdcSrc, 0, 0, SRCCOPY);

        ImageList_ReleaseDragDC(hdcScreen);
    }

    g_dctx.fDragShow = fShow;
    LEAVECRITICAL;

    return TRUE;
}

IMAGELIST* WINAPI ImageList_GetDragImage(POINT FAR* ppt, POINT FAR* pptHotspot)
{
    if (ppt)
    {
        ppt->x = g_dctx.ptDrag.x;
        ppt->y = g_dctx.ptDrag.y;
    }
    if (pptHotspot)
    {
        pptHotspot->x = g_dctx.ptDragHotspot.x;
        pptHotspot->y = g_dctx.ptDragHotspot.y;
    }
    return g_dctx.pimlDrag;
}


// BUGBUG: this hotspot stuff is broken in design
BOOL ImageList_MergeDragImages(int dxHotspot, int dyHotspot)
{
    IMAGELIST* pimlNew;

    if (g_dctx.pimlDither)
    {
        if (g_dctx.pimlCursor)
        {
            // If the cursor list has a mirrored list, let's use that.
            if(g_dctx.pimlCursor->pimlMirror)
            {
                pimlNew = ImageList_Merge(g_dctx.pimlDither, 0, g_dctx.pimlCursor->pimlMirror, g_dctx.iCursor, dxHotspot, dyHotspot);            
            }
            else
            {
                pimlNew = ImageList_Merge(g_dctx.pimlDither, 0, g_dctx.pimlCursor, g_dctx.iCursor, dxHotspot, dyHotspot);
            }    

            if (pimlNew && ImageList_CreateDragBitmaps(pimlNew))
            {
                // WARNING: Don't destroy pimlDrag if it is pimlDither.
                if (g_dctx.pimlDrag && (g_dctx.pimlDrag != g_dctx.pimlDither))
                {
                    ImageList_Destroy(g_dctx.pimlDrag);
                }

                g_dctx.pimlDrag = pimlNew;
                return TRUE;
            }
            return FALSE;
        }
        else
        {
            if (ImageList_CreateDragBitmaps(g_dctx.pimlDither))
            {
                g_dctx.pimlDrag = g_dctx.pimlDither;
                return TRUE;
            }
            return FALSE;
        }
    } else {
        // not an error case if both aren't set yet
        // only an error if we actually tried the merge and failed
        return TRUE;
    }
}

BOOL WINAPI ImageList_SetDragImage(IMAGELIST* piml, int i, int dxHotspot, int dyHotspot)
{
    BOOL fVisible = g_dctx.fDragShow;
    BOOL fRet;

    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, &i, FALSE))
        return FALSE;

    ENTERCRITICAL;
    if (fVisible)
        ImageList_DragShowNolock(FALSE);

    // only do this last step if everything is there.
    fRet = ImageList_MergeDragImages(dxHotspot, dyHotspot);

    if (fVisible)
        ImageList_DragShowNolock(TRUE);

    LEAVECRITICAL;
    return fRet;
}

BOOL WINAPI ImageList_SetDragCursorImage(IMAGELIST * piml, int i, int dxHotspot, int dyHotspot)
{
    BOOL fVisible = g_dctx.fDragShow;
    BOOL fRet = TRUE;
#ifndef UNIX
    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, &i, FALSE))
        return FALSE;

    ENTERCRITICAL;
    // do work only if something has changed
    if ((g_dctx.pimlCursor != piml) || (g_dctx.iCursor != i)) {

        if (fVisible)
            ImageList_DragShowNolock(FALSE);

        g_dctx.pimlCursor = piml;
        g_dctx.iCursor = i;
        g_dctx.ptCursor.x = dxHotspot;
        g_dctx.ptCursor.y = dyHotspot;

        fRet = ImageList_MergeDragImages(dxHotspot, dyHotspot);

        if (fVisible)
            ImageList_DragShowNolock(TRUE);
    }
    LEAVECRITICAL;
#endif
    return fRet;
}

void WINAPI ImageList_EndDrag()
{
    IMAGELIST* piml = g_dctx.pimlDrag;

    ENTERCRITICAL;
    if (IsImageList(piml))
    {
        ImageList_DragShowNolock(FALSE);

        // WARNING: Don't destroy pimlDrag if it is pimlDither.
        if (g_dctx.pimlDrag && (g_dctx.pimlDrag != g_dctx.pimlDither))
        {
            ImageList_Destroy(g_dctx.pimlDrag);
        }

        if (g_dctx.pimlDither)
        {
            ImageList_Destroy(g_dctx.pimlDither);
            g_dctx.pimlDither = NULL;
        }

        g_dctx.pimlCursor = NULL;
        g_dctx.iCursor = -1;
        g_dctx.pimlDrag = NULL;
        g_dctx.hwndDC = NULL;
    }
    LEAVECRITICAL;
}
#endif // _WIN32
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//
// ImageList_CopyBitmap
//
// Worker function for ImageList_Duplicate.
//
// Given a bitmap and an hdc, creates and returns a copy of the passed in bitmap.
//
HBITMAP WINAPI ImageList_CopyBitmap(HBITMAP hbm, HDC hdc)
{
    HBITMAP hbmCopy;
    BITMAP bm;

    ASSERT(hbm);

    hbmCopy = NULL;
    if (GetObject(hbm, sizeof(bm), &bm) == sizeof(bm))
    {
        ENTERCRITICAL;
        if (hbmCopy = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight))
        {
            ImageList_SelectDstBitmap(hbmCopy);

            BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight,
                    hdc, 0, 0, SRCCOPY);

            ImageList_SelectDstBitmap(NULL);
        }
        LEAVECRITICAL;
    }
    return hbmCopy;
}

//
// ImageList_Duplicate
//
// Makes a copy of the passed in imagelist.
//
IMAGELIST* WINAPI ImageList_Duplicate(IMAGELIST* piml)
{
    IMAGELIST* pimlCopy;
    HBITMAP hbmImage;
    HBITMAP hbmMask = NULL;

    V_HIMAGELIST(piml);

    ENTERCRITICAL;

    hbmImage = ImageList_CopyBitmap(piml->hbmImage, piml->hdcImage);
    if (!hbmImage)
        goto Error;

    if (piml->hdcMask)
    {
        hbmMask = ImageList_CopyBitmap(piml->hbmMask, piml->hdcMask);
        if (!hbmMask)
            goto Error;
    }

    pimlCopy = ImageList_Create(piml->cx, piml->cy, piml->flags, 0, piml->cGrow);

    if (pimlCopy) {

        // Slam in our bitmap copies and delete the old ones
        SelectObject(pimlCopy->hdcImage, hbmImage);
        ImageList_DeleteBitmap(pimlCopy->hbmImage);
        if (pimlCopy->hdcMask) {
            SelectObject(pimlCopy->hdcMask, hbmMask);
            ImageList_DeleteBitmap(pimlCopy->hbmMask);
        }
        pimlCopy->hbmImage = hbmImage;
        pimlCopy->hbmMask = hbmMask;

        // Make sure other info is correct
        pimlCopy->cImage = piml->cImage;
        pimlCopy->cAlloc = piml->cAlloc;
        pimlCopy->cStrip = piml->cStrip;
        pimlCopy->clrBlend = piml->clrBlend;
        pimlCopy->clrBk = piml->clrBk;

        // Delete the old brush and create the correct one
        if (pimlCopy->hbrBk)
            DeleteObject(pimlCopy->hbrBk);
        if ((pimlCopy->clrBk == CLR_NONE) || (pimlCopy->flags & ILC_VIRTUAL))
        {
            pimlCopy->hbrBk = GetStockObject(BLACK_BRUSH);
            pimlCopy->fSolidBk = TRUE;
        }
        else
        {
            pimlCopy->hbrBk = CreateSolidBrush(pimlCopy->clrBk);
            pimlCopy->fSolidBk = GetNearestColor32(pimlCopy->hdcImage, pimlCopy->clrBk) == pimlCopy->clrBk;
        }
    } else {
Error:
        if (hbmImage)
            ImageList_DeleteBitmap(hbmImage);
        if (hbmMask)
            ImageList_DeleteBitmap(hbmMask);
    }

    LEAVECRITICAL;

    return pimlCopy;
}


#ifdef _WIN32
// REVIEW, make this public, this is useful.

// copy an image from one imagelist to another at x,y within iDst in pimlDst.
// pimlDst's image size should be larger than pimlSrc
void NEAR PASCAL ImageList_CopyOneImage(IMAGELIST* pimlDst, int iDst, int x, int y, IMAGELIST* pimlSrc, int iSrc)
{
    RECT rcSrc, rcDst;

    ImageList_GetImageRect(pimlSrc, iSrc, &rcSrc);
    ImageList_GetImageRect(pimlDst, iDst, &rcDst);

    if (pimlSrc->hdcMask && pimlDst->hdcMask)
    {
        BitBlt(pimlDst->hdcMask, rcDst.left + x, rcDst.top + y, pimlSrc->cx, pimlSrc->cy,
               pimlSrc->hdcMask, rcSrc.left, rcSrc.top, SRCCOPY);

    }

    BitBlt(pimlDst->hdcImage, rcDst.left + x, rcDst.top + y, pimlSrc->cx, pimlSrc->cy,
           pimlSrc->hdcImage, rcSrc.left, rcSrc.top, SRCCOPY);
}

void NEAR PASCAL ImageList_Merge2(IMAGELIST* piml, IMAGELIST* pimlMerge, int i, int dx, int dy)
{
    if (piml->hdcMask && pimlMerge->hdcMask)
    {
        RECT rcMerge;

        ImageList_GetImageRect(pimlMerge, i, &rcMerge);

        BitBlt(piml->hdcMask, dx, dy, pimlMerge->cx, pimlMerge->cy,
               pimlMerge->hdcMask, rcMerge.left, rcMerge.top, SRCAND);
    }

    ImageList_Draw(pimlMerge, i, piml->hdcImage, dx, dy, ILD_TRANSPARENT);
}

IMAGELIST* WINAPI ImageList_Merge(IMAGELIST* piml1, int i1, IMAGELIST* piml2, int i2, int dx, int dy)
{
    IMAGELIST* pimlNew;
    RECT rcNew;
    RECT rc1;
    RECT rc2;
    int cx, cy;
    int c1, c2;
    UINT wFlags;

    V_HIMAGELIST(piml1);
    V_HIMAGELIST(piml2);

    if (ImageList_Filter(&piml1, &i1, TRUE) ||
        ImageList_Filter(&piml2, &i2, FALSE))
    {
        return NULL;
    }

    ENTERCRITICAL;

    SetRect(&rc1, 0, 0, piml1->cx, piml1->cy);
    SetRect(&rc2, dx, dy, piml2->cx + dx, piml2->cy + dy);
    UnionRect(&rcNew, &rc1, &rc2);

    cx = rcNew.right - rcNew.left;
    cy = rcNew.bottom - rcNew.top;

    //
    // If one of images are shared, create a shared image.
    //
    wFlags = (piml1->flags|piml2->flags) & ~ILC_COLORMASK;

    c1 = (piml1->flags & ILC_COLORMASK);
    c2 = (piml2->flags & ILC_COLORMASK);

#ifdef HICOLOR_DRAG
    if (c1 == 16 && c2 == ILC_COLORDDB)
    {
        c2 = 16;
    }
#endif

    wFlags |= max(c1,c2);

    pimlNew = ImageList_Create(cx, cy, ILC_MASK|wFlags, 1, 0);
    if (pimlNew)
    {
        pimlNew->cImage++;

        if (pimlNew->hdcMask) PatBlt(pimlNew->hdcMask,  0, 0, cx, cy, WHITENESS);
        PatBlt(pimlNew->hdcImage, 0, 0, cx, cy, BLACKNESS);

        ImageList_Merge2(pimlNew, piml1, i1, rc1.left - rcNew.left, rc1.top - rcNew.top);
        ImageList_Merge2(pimlNew, piml2, i2, rc2.left - rcNew.left, rc2.top - rcNew.top);
    }

    LEAVECRITICAL;

    return pimlNew;
}

#endif // WIN32

#ifdef _WIN32        // Only support persistence in 32 bits

// helper macros for using a IStream* from "C"
#define Stream_Read(ps, pv, cb)     SUCCEEDED((ps)->lpVtbl->Read(ps, pv, cb, NULL))
#define Stream_Write(ps, pv, cb)    SUCCEEDED((ps)->lpVtbl->Write(ps, pv, cb, NULL))
#define Stream_Flush(ps)            SUCCEEDED((ps)->lpVtbl->Commit(ps, 0))
#define Stream_Seek(ps, li, d, p)   SUCCEEDED((ps)->lpVtbl->Seek(ps, li, d, p))
#define Stream_Close(ps)            (void)(ps)->lpVtbl->Release(ps)

// BUGBUG should these be public?
BOOL    WINAPI Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel);
HBITMAP WINAPI Stream_ReadBitmap(LPSTREAM pstm, BOOL f);

BOOL WINAPI ImageList_MoreOverlaysUsed(IMAGELIST * piml)
{
    int i;
    for (i = NUM_OVERLAY_IMAGES_0; i < NUM_OVERLAY_IMAGES; i++)
        if (piml->aOverlayIndexes[i] != -1)
            return TRUE;
    return FALSE;
}

BOOL WINAPI ImageList_Write(IMAGELIST* piml, LPSTREAM pstm)
{
    int i;
    ILFILEHEADER ilfh;
    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, NULL, FALSE))
        return FALSE;

    ilfh.magic   = IMAGELIST_MAGIC;
    ilfh.version = IMAGELIST_VER0;
    ilfh.cImage  = (SHORT) piml->cImage;
    ilfh.cAlloc  = (SHORT) piml->cAlloc;
    ilfh.cGrow   = (SHORT) piml->cGrow;
    ilfh.cx      = (SHORT) piml->cx;
    ilfh.cy      = (SHORT) piml->cy;
    ilfh.clrBk   = piml->clrBk;
    ilfh.flags   = (SHORT) piml->flags;

    //
    // Store mirror flags
    //
    if (piml->pimlMirror)
        ilfh.flags |= ILC_MIRROR;   

    if (ImageList_MoreOverlaysUsed(piml))
        ilfh.flags |= ILC_MOREOVERLAY;
    
    for (i=0; i<NUM_OVERLAY_IMAGES; i++)
        ilfh.aOverlayIndexes[i] =  (SHORT) piml->aOverlayIndexes[i];

    Stream_Write(pstm, &ilfh, ILFILEHEADER_SIZE0);

    if (!Stream_WriteBitmap(pstm, piml->hbmImage, 0))
        return FALSE;

    if (piml->hdcMask)
    {
        if (!Stream_WriteBitmap(pstm, piml->hbmMask, 1))
            return FALSE;
    }


    if (ilfh.flags & ILC_MOREOVERLAY)
        Stream_Write(pstm, (LPBYTE)&ilfh + ILFILEHEADER_SIZE0, sizeof(ilfh) - ILFILEHEADER_SIZE0);

    if(piml->pimlMirror)
    {
        if (!Stream_WriteBitmap(pstm, piml->pimlMirror->hbmImage, 0))
            return FALSE;

        if (piml->pimlMirror->hdcMask)
        {
            if (!Stream_WriteBitmap(pstm, piml->pimlMirror->hbmMask, 1))
                return FALSE;
        }
            
    }
        
        
    return TRUE;
}


IMAGELIST* WINAPI ImageList_ReadHelper(ILFILEHEADER *pilfh, HBITMAP hbmImage, HBITMAP hbmMask)
{
    IMAGELIST* piml;
    int i;

    piml = ImageList_Create(pilfh->cx, pilfh->cy, pilfh->flags, 1, pilfh->cGrow);

    if (piml)
    {
        // select into DC's before deleting existing bitmaps
        // patch in the bitmaps we loaded
        SelectObject(piml->hdcImage, hbmImage);
        DeleteObject(piml->hbmImage);
        piml->hbmImage = hbmImage;
        piml->clrBlend = CLR_NONE;

        // Same for the mask (if necessary)
        if (piml->hdcMask) {
            SelectObject(piml->hdcMask, hbmMask);
            DeleteObject(piml->hbmMask);
            piml->hbmMask = hbmMask;
        }

        piml->cAlloc = pilfh->cAlloc;

        //
        // Call ImageList_SetBkColor with 0 in piml->cImage to avoid
        // calling expensive ImageList_ResetBkColor
        //
        piml->cImage = 0;
        ImageList_SetBkColor(piml, pilfh->clrBk);
        piml->cImage = pilfh->cImage;

        for (i=0; i<NUM_OVERLAY_IMAGES; i++)
            ImageList_SetOverlayImage(piml, pilfh->aOverlayIndexes[i], i+1);

        ImageList_SetOwners(piml);
    }
    else
    {
        DeleteObject(hbmImage);
        DeleteObject(hbmMask);
    }
    return piml;
}

IMAGELIST* WINAPI ImageList_Read(LPSTREAM pstm)
{
    IMAGELIST* piml;
    ILFILEHEADER ilfh = {0};
    HBITMAP hbmImage;
    HBITMAP hbmMask;

    HBITMAP hbmMirroredImage;
    HBITMAP hbmMirroredMask;
    BOOL bMirroredIL = FALSE;
    piml = NULL;
   
    // fist read in the old struct
    if (!Stream_Read(pstm, &ilfh, ILFILEHEADER_SIZE0))
        return piml;

    if (ilfh.magic != IMAGELIST_MAGIC)
        return piml;

    // unfortunately if we want to be backward compatible, we are stuck with this version. 
    if (ilfh.version != IMAGELIST_VER0)
        return piml;
    
    hbmMask = NULL;
    hbmMirroredMask = NULL;
    hbmImage = Stream_ReadBitmap(pstm, (ilfh.flags&ILC_COLORMASK));
    if (!hbmImage)
        return piml;

    if (hbmImage && (ilfh.flags & ILC_MASK))
    {
        hbmMask = Stream_ReadBitmap(pstm, FALSE);
        if (!hbmMask)
        {
            DeleteBitmap(hbmImage);
            return piml;
        }
    }
    // Read in the rest of the struct, new overlay stuff.
    if (ilfh.flags & ILC_MOREOVERLAY)
    {
        if (Stream_Read(pstm, (LPBYTE)&ilfh + ILFILEHEADER_SIZE0, sizeof(ilfh) - ILFILEHEADER_SIZE0))
            ilfh.flags &= ~ILC_MOREOVERLAY;
        else
            return piml;
    }

    if (ilfh.flags & ILC_MIRROR)
    {
        ilfh.flags &= ~ILC_MIRROR;
        bMirroredIL = TRUE;
        hbmMirroredImage = Stream_ReadBitmap(pstm, (ilfh.flags&ILC_COLORMASK));
        if (!hbmMirroredImage)
            return piml;

        if (hbmMirroredImage && (ilfh.flags & ILC_MASK))
        {
            hbmMirroredMask = Stream_ReadBitmap(pstm, FALSE);
            if (!hbmMirroredMask)
            {
                DeleteBitmap(hbmMirroredImage);
                return piml;
            }
        }        
    }

    
    piml = ImageList_ReadHelper(&ilfh, hbmImage, hbmMask);

    if(piml && bMirroredIL)
    {
        piml->pimlMirror = ImageList_ReadHelper(&ilfh, hbmMirroredImage, hbmMirroredMask);
        if(!piml->pimlMirror)
        {
            // if we failed to read mirrored imagelist, let's force fail.
            DeleteBitmap(hbmImage);
            DeleteBitmap(hbmMask);
            piml = NULL;
        }
    }
    
    return piml;
}

BOOL WINAPI Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel)
{
    BOOL fSuccess;
    BITMAP bm;
    int cx, cy;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER FAR* pbi;
    BYTE FAR* pbuf;
    HDC hdc;
    UINT cbColorTable;
    int cLines;
    int cLinesWritten;

    ASSERT(pstm);

    fSuccess = FALSE;
    hdc = NULL;
    pbi = NULL;
    pbuf = NULL;

    if (GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
        goto Error;

    hdc = GetDC(HWND_DESKTOP);

    cx = bm.bmWidth;
    cy = bm.bmHeight;

    if (cBitsPerPixel == 0)
        cBitsPerPixel = bm.bmPlanes * bm.bmBitsPixel;

    if (cBitsPerPixel <= 8)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);
    else
        cbColorTable = 0;

    bi.biSize           = sizeof(bi);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = (WORD) cBitsPerPixel;
    bi.biCompression    = BI_RGB;       // RLE not supported!
    bi.biSizeImage      = 0;
    bi.biXPelsPerMeter  = 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed        = 0;
    bi.biClrImportant   = 0;

    bf.bfType           = BFTYPE_BITMAP;
    bf.bfOffBits        = sizeof(BITMAPFILEHEADER) +
                          sizeof(BITMAPINFOHEADER) + cbColorTable;
    bf.bfSize           = bf.bfOffBits + bi.biSizeImage;
    bf.bfReserved1      = 0;
    bf.bfReserved2      = 0;

    pbi = (BITMAPINFOHEADER FAR*)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + cbColorTable);

    if (!pbi)
        goto Error;

    // Get the color table and fill in the rest of *pbi
    //
    *pbi = bi;
    if (GetDIBits(hdc, hbm, 0, cy, NULL, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS) == 0)
        goto Error;

    if (cBitsPerPixel == 1)
    {
        ((DWORD *)(pbi+1))[0] = CLR_BLACK;
        ((DWORD *)(pbi+1))[1] = CLR_WHITE;
    }

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    if (!Stream_Write(pstm, &bf, sizeof(bf)))
        goto Error;

    if (!Stream_Write(pstm, pbi, sizeof(bi) + cbColorTable))
        goto Error;

    //
    // if we have a DIBSection just write the bits out
    //
    if (bm.bmBits != NULL)
    {
        if (!Stream_Write(pstm, bm.bmBits, pbi->biSizeImage))
            goto Error;

        goto Done;
    }

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    pbuf = LocalAlloc(LPTR, CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesWritten = 0; cLinesWritten < cy; cLinesWritten += cLines)
    {
        if (cLines > cy - cLinesWritten)
            cLines = cy - cLinesWritten;

        if (GetDIBits(hdc, hbm, cLinesWritten, cLines,
                pbuf, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS) == 0)
            goto Error;

        if (!Stream_Write(pstm, pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines))
            goto Error;
    }

//  if (!Stream_Flush(pstm))
//      goto Error;

Done:
    fSuccess = TRUE;
Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        LocalFree((HLOCAL)pbi);
    if (pbuf)
        LocalFree((HLOCAL)pbuf);
    return fSuccess;
}

HBITMAP WINAPI Stream_ReadBitmap(LPSTREAM pstm, BOOL fDS)
{
    BOOL fSuccess;
    HDC hdc;
    HBITMAP hbm;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER FAR* pbi;
    BYTE FAR* pbuf=NULL;
    int cBitsPerPixel;
    UINT cbColorTable;
    int cx, cy;
    int cLines, cLinesRead;

    ASSERT(pstm);

    fSuccess = FALSE;

    hdc = NULL;
    hbm = NULL;
    pbi = NULL;

    if (!Stream_Read(pstm, &bf, sizeof(bf)))
        goto Error;

    if (bf.bfType != BFTYPE_BITMAP)
        goto Error;

    if (!Stream_Read(pstm, &bi, sizeof(bi)))
        goto Error;

    if (bi.biSize != sizeof(bi))
        goto Error;

    cx = (int)bi.biWidth;
    cy = (int)bi.biHeight;

    cBitsPerPixel = (int)bi.biBitCount * (int)bi.biPlanes;

    if (cBitsPerPixel <= 8)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);
    else
        cbColorTable = 0;

    pbi = (BITMAPINFOHEADER*)LocalAlloc(LPTR, sizeof(bi) + cbColorTable);
    if (!pbi)
        goto Error;
    *pbi = bi;

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    if (cbColorTable)
    {
        if (!Stream_Read(pstm, pbi + 1, cbColorTable))
            goto Error;
    }

    hdc = GetDC(HWND_DESKTOP);

    //
    //  see if we can make a DIBSection
    //
    if ((cBitsPerPixel > 1) && (fDS != ILC_COLORDDB) && (fDS || UseDS(hdc)))
    {
        //
        // create DIBSection and read the bits directly into it!
        //
        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)pbi, DIB_RGB_COLORS, (LPVOID *)&pbuf, NULL, 0);

        if (hbm == NULL)
            goto Error;

        if (!Stream_Read(pstm, pbuf, pbi->biSizeImage))
            goto Error;

        pbuf = NULL;        // dont free this
        goto Done;
    }

    //
    //  cant make a DIBSection make a mono or color bitmap.
    //
    else if (cBitsPerPixel > 1)
        hbm = CreateColorBitmap(cx, cy);
    else
        hbm = CreateMonoBitmap(cx, cy);

    if (!hbm)
        return NULL;

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    pbuf = LocalAlloc(LPTR, CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesRead = 0; cLinesRead < cy; cLinesRead += cLines)
    {
        if (cLines > cy - cLinesRead)
            cLines = cy - cLinesRead;

        if (!Stream_Read(pstm, pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines))
            goto Error;

        if (!SetDIBits(hdc, hbm, cLinesRead, cLines,
                pbuf, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS))
            goto Error;
    }

Done:
    fSuccess = TRUE;

Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        LocalFree((HLOCAL)pbi);
    if (pbuf)
        LocalFree((HLOCAL)pbuf);

    if (!fSuccess && hbm)
    {
        DeleteBitmap(hbm);
        hbm = NULL;
    }
    return hbm;
}

#endif  // WIN32

///////////////////////////////////////////////////////////////////////////////
// keep this block at the end of the file
///////////////////////////////////////////////////////////////////////////////
#undef ImageList_GetImageRect

BOOL NEAR PASCAL ImageList_IGetImageRect(IMAGELIST* piml, int i, RECT FAR* prcImage)
{
    int x, y;
    ASSERT(prcImage);

    if (!piml || !prcImage || !IsImageListIndex(piml, i))
        return FALSE;

    x = piml->cx * (i % piml->cStrip);
    y = piml->cy * (i / piml->cStrip);

    SetRect(prcImage, x, y, x + piml->cx, y + piml->cy);
    return TRUE;
}

BOOL WINAPI ImageList_GetImageRect(IMAGELIST* piml, int i, RECT FAR* prcImage)
{
    V_HIMAGELIST(piml);

    if (ImageList_Filter(&piml, &i, TRUE))
        return FALSE;

    return ImageList_IGetImageRect(piml, i, prcImage);
}

BOOL NEAR PASCAL ImageList_GetSpareImageRect(IMAGELIST* piml, RECT FAR* prcImage)
{
    BOOL fRet;

    // special hacking to use the one scratch image at tail of list :)
    piml->cImage++;
    fRet = ImageList_IGetImageRect(piml, piml->cImage-1, prcImage);
    piml->cImage--;

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// keep this block at the end of the file
///////////////////////////////////////////////////////////////////////////////
