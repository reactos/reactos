/****************************************************************************
 *
 *   dibmap.c
 *
 *   Histrogram and optimal palette processing module.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include "dibmap.h"

#ifndef WIN32
extern NEAR PASCAL MemCopy(LPVOID,LPVOID,DWORD);
#else
#define HUGE
#endif

extern NEAR PASCAL MemFill(LPVOID,DWORD,BYTE);

void Histogram24(BYTE huge *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram);
void Histogram16(BYTE huge *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram);
void Histogram8(BYTE huge *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors);
void Histogram4(BYTE huge *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors);
void Histogram1(BYTE huge *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors);

void Reduce24(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp16to8);
void Reduce16(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp16to8);
void Reduce8(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp8to8);
void Reduce4(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp8to8);
void Reduce1(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp8to8);

//
//  InitHistogram
//
//  create a zero'ed histogram table
//
LPHISTOGRAM InitHistogram(LPHISTOGRAM lpHistogram)
{
    if (lpHistogram == NULL)
        lpHistogram = (LPVOID)GlobalLock(GlobalAlloc(GPTR,32768l*sizeof(DWORD)));

#if 0
    if (lpHistogram)
        MemFill(lpHistogram, 32768l * sizeof(DWORD), 0);
#endif

    return lpHistogram;
}

//
//  FreeHistogram
//
//  free a histogram table
//
void FreeHistogram(LPHISTOGRAM lpHistogram)
{
    GlobalFree(GlobalHandle(lpHistogram));
}

//
//  DibHistogram
//
//  take all colors in a dib and increment its entry in the Histogram table
//
//  supports the following DIB formats: 1,4,8,16,24
//
BOOL DibHistogram(LPBITMAPINFOHEADER lpbi, LPBYTE lpBits, int x, int y, int dx, int dy, LPHISTOGRAM lpHistogram)
{
    int             i;
    WORD            WidthBytes;
    RGBQUAD FAR *   prgbq;
    WORD            argb16[256];

    if (lpbi == NULL || lpHistogram == NULL)
        return FALSE;

    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);

    if (lpBits == NULL)
        lpBits = (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed*sizeof(RGBQUAD);

    WidthBytes = (WORD)((lpbi->biBitCount * lpbi->biWidth + 7) / 8 + 3) & ~3;

    ((BYTE huge *)lpBits) += (DWORD)y*WidthBytes + ((x*(int)lpbi->biBitCount)/8);

    if (dx < 0 || dx > (int)lpbi->biWidth)
        dx = (int)lpbi->biWidth;

    if (dy < 0 || dy > (int)lpbi->biHeight)
        dy = (int)lpbi->biHeight;

    if ((int)lpbi->biBitCount <= 8)
    {
        prgbq = (LPVOID)((LPBYTE)lpbi + lpbi->biSize);

        for (i=0; i<(int)lpbi->biClrUsed; i++)
        {
            argb16[i] = RGB16(prgbq[i].rgbRed,prgbq[i].rgbGreen,prgbq[i].rgbBlue);
        }

        for (i=(int)lpbi->biClrUsed; i<256; i++)
        {
            argb16[i] = 0x0000;     // just in case!
        }
    }

    switch ((int)lpbi->biBitCount)
    {
        case 24:
            Histogram24(lpBits, dx, dy, WidthBytes, lpHistogram);
            break;

        case 16:
            Histogram16(lpBits, dx, dy, WidthBytes, lpHistogram);
            break;

        case 8:
            Histogram8(lpBits, dx, dy, WidthBytes, lpHistogram, argb16);
            break;

        case 4:
            Histogram4(lpBits, dx, dy, WidthBytes, lpHistogram, argb16);
            break;

        case 1:
            Histogram1(lpBits, dx, dy, WidthBytes, lpHistogram, argb16);
            break;
    }
}

//
// will convert the given DIB to a 8bit DIB with the specifed palette
//
HANDLE DibReduce(LPBITMAPINFOHEADER lpbiIn, LPBYTE pbIn, HPALETTE hpal, LPBYTE lp16to8)
{
    HANDLE              hdib;
    short               nPalColors;
    int                 nDibColors;
    WORD                cbOut;
    WORD                cbIn;
    BYTE                xlat[256];
    BYTE HUGE *         pbOut;
    RGBQUAD FAR *       prgb;
    DWORD               dwSize;
    int                 i;
    int                 dx;
    int                 dy;
    PALETTEENTRY        pe;
    LPBITMAPINFOHEADER  lpbiOut;

    dx    = (int)lpbiIn->biWidth;
    dy    = (int)lpbiIn->biHeight;
    cbIn  = ((lpbiIn->biBitCount*dx+7)/8+3)&~3;
    cbOut = (dx+3)&~3;

    //
    // careful with GetObject in Win32: this (counter-intuitively) writes
    // a short not an INT for the number of colours
    //
    GetObject(hpal, sizeof(short), (LPVOID)&nPalColors);
    nDibColors = (int)lpbiIn->biClrUsed;

    if (nDibColors == 0 && lpbiIn->biBitCount <= 8)
        nDibColors = (1 << (int)lpbiIn->biBitCount);

    if (pbIn == NULL)
        pbIn = (LPBYTE)lpbiIn + (int)lpbiIn->biSize + nDibColors*sizeof(RGBQUAD);

    dwSize = (DWORD)cbOut * dy;

    hdib = GlobalAlloc(GMEM_MOVEABLE,sizeof(BITMAPINFOHEADER)
        + nPalColors*sizeof(RGBQUAD) + dwSize);

    if (!hdib)
        return NULL;

    lpbiOut = (LPVOID)GlobalLock(hdib);
    lpbiOut->biSize         = sizeof(BITMAPINFOHEADER);
    lpbiOut->biWidth        = lpbiIn->biWidth;
    lpbiOut->biHeight       = lpbiIn->biHeight;
    lpbiOut->biPlanes       = 1;
    lpbiOut->biBitCount     = 8;
    lpbiOut->biCompression  = BI_RGB;
    lpbiOut->biSizeImage    = dwSize;
    lpbiOut->biXPelsPerMeter= 0;
    lpbiOut->biYPelsPerMeter= 0;
    lpbiOut->biClrUsed      = nPalColors;
    lpbiOut->biClrImportant = 0;

    pbOut = (LPBYTE)lpbiOut + (int)lpbiOut->biSize + nPalColors*sizeof(RGBQUAD);
    prgb  = (LPVOID)((LPBYTE)lpbiOut + (int)lpbiOut->biSize);

    for (i=0; i<nPalColors; i++)
    {
        GetPaletteEntries(hpal, i, 1, &pe);

        prgb[i].rgbRed      = pe.peRed;
        prgb[i].rgbGreen    = pe.peGreen;
        prgb[i].rgbBlue     = pe.peBlue;
        prgb[i].rgbReserved = 0;
    }

    if ((int)lpbiIn->biBitCount <= 8)
    {
        prgb = (LPVOID)((LPBYTE)lpbiIn + lpbiIn->biSize);

        for (i=0; i<nDibColors; i++)
            xlat[i] = lp16to8[RGB16(prgb[i].rgbRed,prgb[i].rgbGreen,prgb[i].rgbBlue)];

        for (; i<256; i++)
            xlat[i] = 0;
    }

    switch ((int)lpbiIn->biBitCount)
    {
        case 24:
            Reduce24(pbIn, dx, dy, cbIn, pbOut, cbOut, lp16to8);
            break;

        case 16:
            Reduce16(pbIn, dx, dy, cbIn, pbOut, cbOut, lp16to8);
            break;

        case 8:
            Reduce8(pbIn, dx, dy, cbIn, pbOut, cbOut, xlat);
            break;

        case 4:
            Reduce4(pbIn, dx, dy, cbIn, pbOut, cbOut, xlat);
            break;

        case 1:
            Reduce1(pbIn, dx, dy, cbIn, pbOut, cbOut, xlat);
            break;
    }

    return hdib;
}

///////////////////////////////////////////////////////////////////////////////
//  cluster.c
///////////////////////////////////////////////////////////////////////////////

#define  IN_DEPTH    5               // # bits/component kept from input
#define  IN_SIZE     (1 << IN_DEPTH) // max value of a color component

typedef enum { red, green, blue } color;

typedef struct tagCut {
   long lvariance;              // for int version
   int cutpoint;
   unsigned long rem;           // for experimental fixed point
   color cutaxis;
   long w1, w2;
   double variance;
   } Cut;

typedef struct tagColorBox {    // from cluster.c
   struct tagColorBox *next;                /* pointer to next box */
   int   rmin, rmax, gmin, gmax, bmin, bmax;    /* bounding box */
   long variance, wt;                           /* weighted variance */
   long sum[3];                                 /* sum of values */
   } ColorBox;

static int InitBoxes(int nBoxes);
static void DeleteBoxes(void);
static int SplitBoxAxis(ColorBox *box, Cut cutaxis);
static void ShrinkBox(ColorBox *box);
static int ComputePalette(LPHISTOGRAM lpHistogram, LPBYTE lp16to8, LPPALETTEENTRY palette);
static COLORREF DetermineRepresentative(ColorBox *box, int palIndex);
static Cut FindSplitAxis(ColorBox *box);
static void SplitBox(ColorBox *box);
static void SortBoxes(void);

HANDLE hBoxes;
ColorBox *UsedBoxes;
ColorBox *FreeBoxes;
LPBYTE   glp16to8;

#ifdef WIN32

/*
 * to avoid all this 16 bit assembler with minimal changes to the
 * rest of the code the Win32 version will use a global pointer set by
 * UseHistogram and accessed by the hist() and IncHistogram macros.
 */
DWORD HUGE* glpHistogram;

#define UseHistogram(p)	(glpHistogram = (p))

#define hist(r,g,b)  ((DWORD HUGE *)glpHistogram)[(WORD)(b) | ((WORD)(g)<<IN_DEPTH) | ((WORD)(r)<<(IN_DEPTH*2))]

#define IncHistogram(w) if (lpHistogram[(WORD)(w)] < 0xFFFFFFFF) {  \
			    lpHistogram[(WORD)(w)]++;\
			}

#else

#define hist(r,g,b)  GetHistogram((BYTE)(r),(BYTE)(g),(BYTE)(b))



#pragma optimize ("", off)
//
//  set FS == lpHistogram.sel, so we can get at it quickly!
//
void NEAR PASCAL UseHistogram(LPHISTOGRAM lpHistogram)
{
    _asm {
        mov     ax,word ptr lpHistogram[2]

        _emit   08Eh                     ; mov  fs,ax
        _emit   0E0h
    }
}



//
//  get the DWORD histogram count of a RGB
//
DWORD NEAR _FASTCALL GetHistogram(BYTE r, BYTE g, BYTE b)
{

    if (0)              // avoid compiler warning NO RETURN VALUE
        return 0;

    _asm {
        ;
        ; on entry al=r, dl=g, bl=b  [0-31]
        ;
        ; map to a RGB16
        ;
        xor     ah,ah
        shl     ax,5
        or      al,dl
        shl     ax,5
        or      al,bl

        ; now ax = RGB16

        _emit 66h _asm xor bx,bx           ; xor ebx,ebx
                  _asm mov bx,ax           ; mov  bx,ax
        _emit 66h _asm shl bx,2            ; shl ebx,2

        _emit 64h _asm _emit 67h           ; mov dx,fs:[ebx][2]
        _emit 8Bh _asm _emit 53h
        _emit 02h

        _emit 64h _asm _emit 67h           ; mov ax,fs:[ebx][0]
        _emit 8Bh _asm _emit 03h
    }
}

//
//  increment the histogram count of a RGB16
//
//
//  #define IncHistogram(w) if (lpHistogram[(WORD)(w)] < 0xFFFFFFFF)
//                              lpHistogram[(WORD)(w)]++;
//
void NEAR _FASTCALL IncHistogram(WORD rgb16)
{
    _asm {
        ;
        ; on entry ax = rgb16
        ;
        _emit 66h _asm xor bx,bx           ; xor ebx,ebx
                  _asm mov bx,ax           ; mov bx,ax
        _emit 66h _asm shl bx,2            ; shl ebx,2

        _emit 64h _asm _emit 67h           ; cmp dword ptr fs:[ebx], -1
        _emit 66h _asm _emit 83h
        _emit 3Bh _asm _emit 0FFh

        _emit 74h _asm _emit 05h           ; je  short @f

        _emit 64h _asm _emit 67h           ; inc dword ptr fs:[ebx]
        _emit 66h _asm _emit 0FFh
        _emit 03h
    }
}

#pragma optimize ("", on)

// !!! C8 generates a Jump into the middle of a 2 byte instruction
// !!! Stupid C8!
#pragma optimize ("", off)

#endif  //WIN32

//
//  HistogramPalette
//
//  given a histogram, will reduce it to 'nColors' number of colors.
//  returns a optimal palette.  if specifed lp16to8 will contain the
//  translate table from RGB16 to the palette index.
//
//  you can specify lpHistogram as lp16to8
//
HPALETTE HistogramPalette(LPHISTOGRAM lpHistogram, LPBYTE lp16to8, int nColors)
{
    WORD     w;
    DWORD    dwMax;
    COLORREF rgb;
    ColorBox *box;
    int i;
    // Had to make this global to prevent VB 2.0 stack explosion
    static struct {
        WORD         palVersion;
        WORD         palNumEntries;
        PALETTEENTRY palPalEntry[256];
    }   pal;

    //
    //  the 'C' code cant handle >64k histogram counts.
    //  !!!fix this
    //
    for (dwMax=0,w=0; w<0x8000; w++)
        dwMax = max(dwMax,lpHistogram[w]);

    while (dwMax > 0xFFFFl)
    {
        for (w=0; w<0x8000; w++)
            lpHistogram[w] /= 2;

        dwMax /= 2;
    }

    if (!InitBoxes(min(nColors, 236)))
        return NULL;

    UseHistogram(lpHistogram);
    glp16to8 = lp16to8;

    /* while there are free boxes left, split the largest */

    i = 0;

    do {
       i++;
       SplitBox(UsedBoxes);
       }
    while (FreeBoxes && UsedBoxes->variance);

    SortBoxes();

    i=0;

    //
    // add some standard colors to the histogram
    //
    if (nColors > 236)
    {
        HDC hdc;
	HPALETTE hpal;

        hdc = GetDC(NULL);

        if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        {
        GetSystemPaletteEntries(hdc, 0,   10, &pal.palPalEntry[0]);
        GetSystemPaletteEntries(hdc, 246, 10, &pal.palPalEntry[246]);

        i = 10;
        } else {	// we're a true colour device, so get the system
			// colours from the default palette.
	    hpal = GetStockObject(DEFAULT_PALETTE);
            GetPaletteEntries(hpal, 0, 10, &pal.palPalEntry[0]);
            GetPaletteEntries(hpal, 10, 10, &pal.palPalEntry[246]);
            i = 10;
	}

        ReleaseDC(NULL, hdc);
    }

    /* Generate the representitives and the associated Palette mapping */
    /* NOTE:  Might loop less than nColors times.                      */
    for (box = UsedBoxes; box; box = box->next, i++)
    {
        rgb = DetermineRepresentative(box, i);
        pal.palPalEntry[i].peRed   = GetRValue(rgb);
        pal.palPalEntry[i].peGreen = GetGValue(rgb);
        pal.palPalEntry[i].peBlue  = GetBValue(rgb);
        pal.palPalEntry[i].peFlags = 0;
    }

    DeleteBoxes();

    if (nColors > 236)
    {
        for (; i<246; i++)
        {
            pal.palPalEntry[i].peRed   = 0;
            pal.palPalEntry[i].peGreen = 0;
            pal.palPalEntry[i].peBlue  = 0;
            pal.palPalEntry[i].peFlags = 0;
        }

        i = 256;
    }

    glp16to8 = NULL;

    pal.palVersion    = 0x300;
    pal.palNumEntries = i;
    return CreatePalette((LPLOGPALETTE)&pal);
}

#pragma optimize ("", on)

static void SortBoxes()
{
    ColorBox *box;
    ColorBox *newList;
    ColorBox *insBox;
    ColorBox *nextBox;

    newList = UsedBoxes;
    nextBox = newList->next;
    newList->next = NULL;

    for (box = nextBox; box; box = nextBox) { // just an insertion sort...
            nextBox = box->next;
            if (box->wt > newList->wt) {
                    box->next = newList;
                    newList = box;
            } else {
                    for (insBox = newList;
                            insBox->next && (box->wt < insBox->next->wt);
                            insBox = insBox->next) ;
                    box->next = insBox->next;
                    insBox->next = box;
            }
    }

    UsedBoxes = newList;
}


/*
   allocate space for nBoxes boxes, set up links.  On exit UsedBoxes
   points to one box, FreeBoxes points to remaining (nBoxes-1) boxes.
   return 0 if successful.
*/

static BOOL InitBoxes(int nBoxes)
{
    int i;

    hBoxes = LocalAlloc(LHND, nBoxes*sizeof(ColorBox));
    if (!hBoxes)
        return FALSE;

    UsedBoxes = (ColorBox*)LocalLock(hBoxes);
    FreeBoxes = UsedBoxes + 1;
    UsedBoxes->next = NULL;

    for (i = 0; i < nBoxes - 1; ++i)
    {
        FreeBoxes[i].next = FreeBoxes + i + 1;
    }
    FreeBoxes[nBoxes-2].next = NULL;

    /* save the bounding box */
    UsedBoxes->rmin = UsedBoxes->gmin = UsedBoxes->bmin = 0;
    UsedBoxes->rmax = UsedBoxes->gmax = UsedBoxes->bmax = IN_SIZE - 1;
    UsedBoxes->variance = 9999999;    /* arbitrary large # */

    return TRUE;
}

static void DeleteBoxes()
{
   LocalUnlock(hBoxes);
   LocalFree(hBoxes);
   hBoxes = NULL;
}

static void SplitBox(ColorBox *box)
{
   /*
      split box into two roughly equal halves and update the data structures
      appropriately.
   */
   Cut cutaxis;
   ColorBox *temp, *temp2, *prev;

   cutaxis = FindSplitAxis(box);

   /* split the box along that axis.  If rc != 0 then the box contains
      one color, and should not be split */
   if (SplitBoxAxis(box, cutaxis))
      return;

   /* shrink each of the boxes to fit the points they enclose */
   ShrinkBox(box);
   ShrinkBox(FreeBoxes);

   /* move old box down in list, if necessary */
   if (box->next && box->variance < box->next->variance)
   {
      UsedBoxes = box->next;
      temp = box;
      do {
         prev = temp;
         temp = temp->next;
         } while (temp && temp->variance > box->variance);
      box->next = temp;
      prev->next = box;
   }

   /* insert the new box in sorted order (descending), removing it
      from the free list. */
   if (FreeBoxes->variance >= UsedBoxes->variance)
   {
      temp = FreeBoxes;
      FreeBoxes = FreeBoxes->next;
      temp->next = UsedBoxes;
      UsedBoxes = temp;
   }
   else
   {
      temp = UsedBoxes;
      do {
         prev = temp;
         temp = temp->next;
         } while (temp && temp->variance > FreeBoxes->variance);
      temp2 = FreeBoxes->next;
      FreeBoxes->next = temp;
      prev->next = FreeBoxes;
      FreeBoxes = temp2;
   }
}

static Cut FindSplitAxis(ColorBox *box)
{
        unsigned long   proj_r[IN_SIZE],proj_g[IN_SIZE],proj_b[IN_SIZE];
        unsigned long   f;
        double          currentMax,mean;
        unsigned long   w,w1,m,m1;
        short           r,g,b;
        short           bestCut;
        color           bestAxis;
        Cut             cutRet;
        double          temp1,temp2;

        for (r = 0; r < IN_SIZE; r++) {
                proj_r[r] = proj_g[r] = proj_b[r] = 0;
        }

        w = 0;

        // Project contents of box down onto axes
        for (r = box->rmin; r <= box->rmax; r++) {
                for (g = box->gmin; g <= box->gmax; ++g) {
                        for (b = box->bmin; b <= box->bmax; ++b) {
                                f = hist(r,g,b);
                                proj_r[r] += f;
                                proj_g[g] += f;
                                proj_b[b] += f;
                        }
                }
                w += proj_r[r];
        }

        currentMax = 0.0f;

#define Check_Axis(l,color)                                     \
        m = 0;                                                  \
        for (l = box->l##min; l <= box->l##max; (l)++) {        \
                m += l * proj_##l[l];                           \
        }                                                       \
        mean = ((double) m) / ((double) w);                     \
                                                                \
        w1 = 0;                                                 \
        m1 = 0;                                                 \
        for (l = box->l##min; l <= box->l##max; l++) {          \
                w1 += proj_##l[l];                              \
                if (w1 == 0)                                    \
                        continue;                               \
                if (w1 == w)                                    \
                        break;                                  \
                m1 += l * proj_##l[l];                          \
                temp1 = mean - (((double) m1) / ((double) w1)); \
                temp2 = (((double) w1) / ((double) (w-w1))) * temp1 * temp1; \
                if (temp2 > currentMax) {                       \
                        bestCut = l;                            \
                        bestAxis = color;                       \
                        currentMax = temp2;                     \
                }                                               \
        }

        Check_Axis(r,red);
        Check_Axis(g,green);
        Check_Axis(b,blue);

        cutRet.cutaxis = bestAxis;
        cutRet.cutpoint = bestCut;

        return cutRet;
}

static int SplitBoxAxis(ColorBox *box, Cut cutaxis)
{
   /*
      Split box along splitaxis into two boxes, one of which is placed
      back in box, the other going in the first free box (FreeBoxes)
      If the box only contains one color, return non-zero, else return 0.
   */
   ColorBox *next;

   if ( box->variance == 0)
      return 1;

   /* copy all non-link information to new box */
   next = FreeBoxes->next;
   *FreeBoxes = *box;
   FreeBoxes->next = next;

   switch (cutaxis.cutaxis)
   {
      case red:
         box->rmax = cutaxis.cutpoint;
         FreeBoxes->rmin = cutaxis.cutpoint+1;
         break;
      case green:
         box->gmax = cutaxis.cutpoint;
         FreeBoxes->gmin = cutaxis.cutpoint+1;
         break;
      case blue:
         box->bmax = cutaxis.cutpoint;
         FreeBoxes->bmin = cutaxis.cutpoint+1;
         break;
   }

   return 0;
}

static void ShrinkBox(ColorBox *box)
{
        unsigned long n, sxx, sx2, var, quotient, remainder;
        int r,g,b;
        unsigned long   f;
        unsigned long   proj_r[IN_SIZE],proj_g[IN_SIZE],proj_b[IN_SIZE];

        n = 0;

        for (r = 0; r < IN_SIZE; r++) {
                proj_r[r] = proj_g[r] = proj_b[r] = 0;
        }

        // Project contents of box down onto axes
        for (r = box->rmin; r <= box->rmax; r++) {
                for (g = box->gmin; g <= box->gmax; ++g) {
                        for (b = box->bmin; b <= box->bmax; ++b) {
                                f = hist(r,g,b);
                                proj_r[r] += f;
                                proj_g[g] += f;
                                proj_b[b] += f;
                        }
                }
                n += proj_r[r];
        }

        box->wt = n;
        var = 0;

#define AddAxisVariance(c)                                              \
        sxx = 0; sx2 = 0;                                               \
        for (c = box->c##min; c <= box->c##max; c++) {                  \
                sxx += proj_##c[c] * c * c;                             \
                sx2 += proj_##c[c] * c;                                 \
        }                                                               \
        quotient = sx2 / n; /* This stuff avoids overflow */            \
        remainder = sx2 % n;                                            \
        var += sxx - quotient * sx2 - ((remainder * sx2)/n);

        AddAxisVariance(r);
        AddAxisVariance(g);
        AddAxisVariance(b);

        box->variance = var;
}

static COLORREF DetermineRepresentative(ColorBox *box, int palIndex)
{
   /*
      determines the rgb value to represent the pixels contained in
      box.  nbits is the # bits/component we're allowed to return.
   */
   long f;
   long Rval, Gval, Bval;
   unsigned long total;
   int r, g, b;
   WORD w;

   /* compute the weighted sum of the elements in the box */
   Rval = Gval = Bval = total = 0;
   for (r = box->rmin; r <= box->rmax; ++r)
   {
      for (g = box->gmin; g <= box->gmax; ++g)
      {
         for (b = box->bmin; b <= box->bmax; ++b)
         {
            if (glp16to8)
            {
                w = (WORD)(b) | ((WORD)(g)<<IN_DEPTH) | ((WORD)(r)<<(IN_DEPTH*2));
                glp16to8[w] = (BYTE)palIndex;
            }

            f = hist(r,g,b);
            if (f == 0L)
               continue;

            Rval += f * (long) r;
            Gval += f * (long) g;
            Bval += f * (long) b;

            total += f;
         }
      }
   }

   /* Bias the sum so that we round up at .5 */
   Rval += total / 2;
   Gval += total / 2;
   Bval += total / 2;

   return RGB(Rval*255/total/IN_SIZE, Gval*255/total/IN_SIZE, Bval*255/total/IN_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  write this stuff in ASM!
//
///////////////////////////////////////////////////////////////////////////////

void Histogram24(BYTE HUGE *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram)
{
    int x,y;
    BYTE r,g,b;
    WORD w;

    UseHistogram(lpHistogram);

    WidthBytes -= dx*3;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            b = *pb++;
            g = *pb++;
            r = *pb++;
            w = RGB16(r,g,b);
            IncHistogram(w);
        }
        pb += WidthBytes;
    }
}

void Histogram16(BYTE HUGE *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram)
{
    int x,y;
    WORD w;

    UseHistogram(lpHistogram);

    WidthBytes -= dx*2;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            w = *((WORD HUGE *)pb)++;
            w &= 0x7FFF;
            IncHistogram(w);
        }
        pb += WidthBytes;
    }
}

void Histogram8(BYTE HUGE *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors)
{
    int x,y;
    WORD w;

    UseHistogram(lpHistogram);

    WidthBytes -= dx;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            w = lpColors[*pb++];
            IncHistogram(w);
        }
        pb += WidthBytes;
    }
}

void Histogram4(BYTE HUGE *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors)
{
    int x,y;
    BYTE b;
    WORD w;

    UseHistogram(lpHistogram);

    WidthBytes -= (dx+1)/2;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<(dx+1)/2; x++)
        {
            b = *pb++;

            w = lpColors[b>>4];
            IncHistogram(w);

            w = lpColors[b&0x0F];
            IncHistogram(w);
        }
        pb += WidthBytes;
    }
}

void Histogram1(BYTE HUGE *pb, int dx, int dy, WORD WidthBytes, LPHISTOGRAM lpHistogram, LPWORD lpColors)
{
    int x,y,i;
    BYTE b;
    WORD w;

    UseHistogram(lpHistogram);

    WidthBytes -= (dx+7)/8;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<(dx+7)/8; x++)
        {
            b = *pb++;

            for (i=0; i<8; i++)
            {
                w = lpColors[b>>7];
                IncHistogram(w);
                b<<=1;
            }
        }
        pb += WidthBytes;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  write this stuff in ASM! too
//  -- if you do - please leave the C version #ifdef WIN32
//
///////////////////////////////////////////////////////////////////////////////

void Reduce24(BYTE HUGE *pbIn, int dx, int dy, WORD cbIn, BYTE HUGE *pbOut, WORD cbOut, LPBYTE lp16to8)
{
    int x,y;
    BYTE r,g,b;

    cbOut -= dx;
    cbIn  -= dx*3;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            b = *pbIn++;
            g = *pbIn++;
            r = *pbIn++;
            *pbOut++ = lp16to8[RGB16(r,g,b)];
        }
        pbIn += cbIn;
        pbOut+= cbOut;
    }
}

void Reduce16(BYTE huge *pbIn, int dx, int dy, WORD cbIn, BYTE huge *pbOut, WORD cbOut, LPBYTE lp16to8)
{
    int x,y;
    WORD w;

    cbOut -= dx;
    cbIn  -= dx*2;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            w = *((WORD HUGE *)pbIn)++;
            *pbOut++ = lp16to8[w&0x7FFF];
        }
        pbIn += cbIn;
        pbOut+= cbOut;
    }
}

void Reduce8(BYTE HUGE *pbIn, int dx, int dy, WORD cbIn, BYTE HUGE *pbOut, WORD cbOut, LPBYTE lp8to8)
{
    int x,y;

    cbIn  -= dx;
    cbOut -= dx;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            *pbOut++ = lp8to8[*pbIn++];
        }
        pbIn  += cbIn;
        pbOut += cbOut;
    }
}

void Reduce4(BYTE HUGE *pbIn, int dx, int dy, WORD cbIn, BYTE HUGE *pbOut, WORD cbOut, LPBYTE lp8to8)
{
    int x,y;
    BYTE b;

    cbIn  -= (dx+1)/2;
    cbOut -= (dx+1)&~1;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<(dx+1)/2; x++)
        {
            b = *pbIn++;
            *pbOut++ = lp8to8[b>>4];
            *pbOut++ = lp8to8[b&0x0F];
        }
        pbIn  += cbIn;
        pbOut += cbOut;
    }
}

void Reduce1(BYTE HUGE *pbIn, int dx, int dy, WORD cbIn, BYTE HUGE *pbOut, WORD cbOut, LPBYTE lp8to8)
{
    int x,y;
    BYTE b;

    cbIn  -= (dx+7)/8;
    cbOut -= dx;

    for (y=0; y<dy; y++)
    {
        for (x=0; x<dx; x++)
        {
            if (x%8 == 0)
                b = *pbIn++;

            *pbOut++ = lp8to8[b>>7];
            b<<=1;
        }
        pbIn  += cbIn;
        pbOut += cbOut;
    }
}
