#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

extern void x_DitherRow (BYTE HUGEP * from, BYTE HUGEP * to, PALETTEENTRY *pcolors,
                long row, long ncols, int transparent);

//+------------------------------------------------------------------------
//  Tags
//-------------------------------------------------------------------------

PerfDbgExtern(tagImgTaskIO);

DeclareTag(tagImgTaskGif,           "Dwn",          "Img: Trace Gif optimizations")
PerfDbgTag(tagImgTaskGifAbort,      "Dwn",          "Img: Zap invalid Gif scanlines")
MtDefine(CImgTaskGif, Dwn, "CImgTaskGif")
MtDefine(CImgTaskGifStack, CImgTaskGif, "CImgTaskGif Decode Stack")
MtDefine(CImgTaskGifTable0, CImgTaskGif, "CImgTaskGif Decode Table 0")
MtDefine(CImgTaskGifTable1, CImgTaskGif, "CImgTaskGif Decode Table 1")
MtDefine(CImgTaskGifBits, CImgTaskGif, "CImgTaskGif Decode Image")
MtDefine(CImgTaskGifFrame, CImgInfo, "CImgTaskGif GIFFRAME")

#define XX_DMsg(x, y)

#define PROG_INTERVAL   0x3

#define NUM_IMPORTANT_COLORS 256

#define MAXCOLORMAPSIZE     256

#define TRUE    1
#define FALSE   0

#define MAX_LWZ_BITS        12

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)   (((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)         ((((unsigned int) b)<<8)|((unsigned int)a))

#define dwIndefiniteGIFThreshold 300    // 300 seconds == 5 minutes
                                        // If the GIF runs longer than
                                        // this, we will assume the author
                                        // intended an indefinite run.
#define dwMaxGIFBits 13107200           // keep corrupted GIFs from causing
                                        // us to allocate _too_ big a buffer.
                                        // This one is 1280 X 1024 X 10.
typedef struct _GIFSCREEN
{
    unsigned long Width;
    unsigned long Height;
    unsigned long BitPixel;
    unsigned long ColorResolution;
    unsigned long Background;
    unsigned long AspectRatio;
}
GIFSCREEN;

typedef struct _GIF89
{
    long transparent;
    long delayTime;
    long inputFlag;
    long disposal;
}
GIF89;

#define MAX_STACK_SIZE  ((1 << (MAX_LWZ_BITS)) * 2)
#define MAX_TABLE_SIZE  (1 << MAX_LWZ_BITS)
typedef struct _GIFINFO
{
    unsigned char *src;
    GIF89 Gif89;
    long lGifLoc;
    long ZeroDataBlock;

/*
 **  Pulled out of nextCode
 */
    long curbit, lastbit, get_done;
    long last_byte;
    long return_clear;
/*
 **  Out of nextLWZ
 */
    unsigned short *pstack, *sp;
    long stacksize;
    long code_size, set_code_size;
    long max_code, max_code_size;
    long clear_code, end_code;

/*
 *   Were statics in procedures
 */
    unsigned char buf[280];
    unsigned short *table[2];
    long tablesize;
    long firstcode, oldcode;

} GIFINFO;

class CImgTaskGif : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskGif))

    CImgTaskGif();
    virtual ~CImgTaskGif();

    // CImgTask methods

    virtual void Decode(BOOL *pfNonProgressive);

    // CImgTaskGif methods

    void OnProg(BOOL fLast, ULONG ulCoversImg);
    BOOL Read(void * pv, ULONG cb);
    BOOL ReadByte(BYTE * pb);
    void ReadGIFMaster();
    BOOL ReadColorMap(long number, PALETTEENTRY * ppe);
    long DoExtension(long label);
    long GetDataBlock(unsigned char *buf);
    BOOL initLWZ(long input_code_size);
    long nextCode(long code_size);
    unsigned short * growStack();
    BOOL growTables();
    long readLWZ();
    long nextLWZ();
    BOOL ReadScanline(unsigned char *pb, int cb);
    void CalculateUpdateRect(int logicalRow0, int logicalRowN, BOOL *pfInvalidateAll, LONG *pyBottom);
    void ProgressiveDithering(int logicalFill, int logicalRow);

    typedef BYTE HUGEP * HLPBYTE;
    CImgBitsDIB *ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame,
        int cpeColors, PALETTEENTRY * ppeColors, HLPBYTE * ppbBits, LONG lTrans);
    virtual void BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags);

    // Data members

    LONG                _cbBuf;
    BYTE *              _pbBuf;
    BYTE                _abBuf[512];
    BOOL                _fEof;
    BOOL                _fMustBeTransparent;
    BOOL                _fDither;
    DWORD               _cbImage;
    BOOL                _fInterleaved;
    LONG                _yLogRow;
    LONG                _yLogRowDraw;
    LONG                _yDithRow;
    void *              _pvDithData;
    GIFINFO             _gifinfo;
    PALETTEENTRY        _apeFirst[256];
    int                 _mapConstrained[256];
    HLPBYTE             _pbSrcAlloc;
    HLPBYTE             _pbSrc;
    HLPBYTE             _pbDst;

};

CImgTaskGif::CImgTaskGif()
{
    _yLogRow = -1;
    _yLogRowDraw = -1;
}

CImgTaskGif::~CImgTaskGif()
{
    MemFree(_pbSrcAlloc);
    MemFree(_pvDithData);
    MemFree(_gifinfo.pstack);
    MemFree(_gifinfo.table[0]);
    MemFree(_gifinfo.table[1]);
}

void CImgTaskGif::Decode(BOOL *pfNonProgressive)
{
    ReadGIFMaster();

    if (_pImgBits && _gad.pgf && _yLogRowDraw >= 0)
    {
        if (_yLogRowDraw + 1 >= _gad.pgf->height)
        {
            _ySrcBot = -1;
        }
        else if (_fInterleaved)
        {
            _ySrcBot = (long)min((long)((_yLogRowDraw + 1) * 8), (long)_gad.pgf->height);

            if (_ySrcBot == _gad.pgf->height)
                _ySrcBot = -1;
        }
        else
        {
            _ySrcBot = _yLogRowDraw + 1;
        }

        _gad.pgf->pibd->SetValidLines(_ySrcBot);
    }
}

BOOL IsGifHdr(BYTE * pb)
{
    return(pb[0] == 'G' && pb[1] == 'I' && pb[2] == 'F'
        && pb[3] == '8' && (pb[4] == '7' || pb[4] == '9') && pb[5] == 'a');
}

CImgTask * NewImgTaskGif()
{
    return(new CImgTaskGif);
}

BOOL
CImgTaskGif::Read(void * pv, ULONG cb)
{
    PerfDbgLog1(tagImgTaskIO, this, "+CImgTaskGif::Read (req %ld)", cb);

    LONG cbReq = (LONG)cb, cbGot, cbTot = 0;

    while (cbReq > 0)
    {
        cbGot = cbReq < _cbBuf ? cbReq : _cbBuf;

        if (cbGot > 0)
        {
            memcpy(pv, _pbBuf, cbGot);
            _pbBuf += cbGot;
            _cbBuf -= cbGot;
            cbTot  += cbGot;

            if (cbTot == (LONG)cb)
                break;
        }

        if (_pDwnBindData->IsEof() || _fTerminate)
            break;

        if (cbGot > 0)
        {
            pv     = (BYTE *)pv + cbGot;
            cbReq -= cbGot;
        }

        cbGot  = 0;
        super::Read(_abBuf, ARRAY_SIZE(_abBuf), (ULONG *)&cbGot, cbReq);
        _pbBuf = _abBuf;
        _cbBuf = cbGot;
    }

    PerfDbgLog3(tagImgTaskIO, this, "-CImgTaskGif::Read (got %ld) %c%c",
        cbTot, _pDwnBindData->IsEof() ? 'E' : ' ',
        cbTot > 0 ? 'T' : 'F');

    return(cbTot > 0);
}

inline BOOL
CImgTaskGif::ReadByte(BYTE * pb)
{
    if (--_cbBuf >= 0)
    {
        *pb = *_pbBuf++;
        return(TRUE);
    }
    else
    {
        return(Read(pb, 1));
    }
}

void
CImgTaskGif::ReadGIFMaster()
{
    unsigned char buf[16];
    unsigned char c;
    long useGlobalColormap;
    long imageCount = 0;
    long imageNumber = 1;
    GIFSCREEN GifScreen;
    long bitPixel;
    GIFFRAME * pgfLast = NULL;
    GIFFRAME * pgfNew = NULL;
    HLPBYTE pbBits;
    PALETTEENTRY apeLocal[MAXCOLORMAPSIZE];
    PALETTEENTRY * ppeColors;
    int cpeColors;
    CImgBitsDIB *pibd = NULL;

    _gifinfo.ZeroDataBlock = 0;

    /*
     * Initialize GIF89 extensions
     */
    _gifinfo.Gif89.transparent = -1;
    _gifinfo.Gif89.delayTime = 5;
    _gifinfo.Gif89.inputFlag = -1;

    // initialize our animation fields
    _gad.dwGIFVer = dwGIFVerUnknown;

    if (!Read(buf, 6))
    {
        XX_DMsg(DBG_IMAGE, ("GIF: error reading magic number\n"));
        goto exitPoint;
    }

    if (!IsGifHdr(buf))
        goto exitPoint;

    _gad.dwGIFVer = (buf[4] == '7') ? dwGIFVer87a : dwGIFVer89a;

    if (!Read(buf, 7))
    {
        XX_DMsg(DBG_IMAGE, ("GIF: failed to read screen descriptor\n"));
        goto exitPoint;
    }

    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP))
    {                           /* Global Colormap */
        if (!ReadColorMap(GifScreen.BitPixel, _ape))
        {
            XX_DMsg(DBG_IMAGE, ("error reading global colormap\n"));
            goto exitPoint;
        }
    }

    if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
        float r;
        r = ((float) (GifScreen.AspectRatio) + (float) 15.0) / (float) 64.0;
        XX_DMsg(DBG_IMAGE, ("Warning: non-square pixels!\n"));
    }

    for (;; ) // our appetite now knows no bounds save termination or error
    {
        if (!ReadByte(&c))
        {
            XX_DMsg(DBG_IMAGE, ("EOF / read error on image data\n"));
            goto exitPoint;
        }

        if (c == ';')
        {                       /* GIF terminator */
            if (imageCount < imageNumber)
            {
                XX_DMsg(DBG_IMAGE, ("No images found in file\n"));
                goto exitPoint;
            }
            break;
        }

        if (c == '!')
        {                       /* Extension */
            if (!ReadByte(&c))
            {
                XX_DMsg(DBG_IMAGE, ("EOF / read error on extension function code\n"));
                goto exitPoint;
            }
            DoExtension(c);
            continue;
        }

        if (c != ',')
        {                       /* Not a valid start character */
            break;
        }

        ++imageCount;

        if (!Read(buf, 9))
        {
            XX_DMsg(DBG_IMAGE, ("couldn't read left/top/width/height\n"));
            goto exitPoint;
        }

        useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

        bitPixel = 1 << ((buf[8] & 0x07) + 1);

        /*
         * We only want to set width and height for the imageNumber
         * we are requesting.
         */
        if (imageCount == imageNumber)
        {
            // Replicate some of Netscape's special cases:
            // Don't use the logical screen if it's a GIF87a and the topLeft of the first image is at the origin.
            // Don't use the logical screen if the first image spills out of the logical screen.
            // These are artifacts of primitive authoring tools falling into the hands of hapless users.
            RECT    rectImage;  // rect defining bounds of GIF
            RECT    rectLS;     // rect defining bounds of GIF logical screen.
            RECT    rectSect;   // intersection of image an logical screen
            BOOL    fNoSpill;   // True if the image doesn't spill out of the logical screen
            BOOL    fGoofy87a;  // TRUE if its one of the 87a pathologies that Netscape special cases

            rectImage.left = LM_to_uint(buf[0], buf[1]);
            rectImage.top = LM_to_uint(buf[2], buf[3]);
            rectImage.right = rectImage.left + LM_to_uint(buf[4], buf[5]);
            rectImage.bottom = rectImage.top + LM_to_uint(buf[6], buf[7]);
            rectLS.left = rectLS.top = 0;
            rectLS.right = GifScreen.Width;
            rectLS.bottom = GifScreen.Height;
            IntersectRect( &rectSect, &rectImage, &rectLS );
            fNoSpill = EqualRect( &rectImage, &rectSect );
            fGoofy87a = FALSE;
            if (_gad.dwGIFVer == dwGIFVer87a)
            {
                // netscape ignores the logical screen if the image is flush against
                // either the upper left or lower right corner
                fGoofy87a = (rectImage.top == 0 && rectImage.left == 0) ||
                            (rectImage.bottom == rectLS.bottom &&
                             rectImage.right == rectLS.right);
            }

            if (!fGoofy87a && fNoSpill)
            {
                _xWid = GifScreen.Width;
                _yHei = GifScreen.Height;
            }
            else
            {
                // Something is amiss. Fall back to the image's dimensions.

                // If the sizes match, but the image is offset, or we're ignoring
                // the logical screen cuz it's a goofy 87a, then pull it back to
                // to the origin
                if ((LM_to_uint(buf[4], buf[5]) == GifScreen.Width &&
                      LM_to_uint(buf[6], buf[7]) == GifScreen.Height) ||
                     fGoofy87a)
                {
                    buf[0] = buf[1] = 0; // left corner to zero
                    buf[2] = buf[3] = 0; // top to zero.
                }

                _xWid = LM_to_uint(buf[4], buf[5]);
                _yHei = LM_to_uint(buf[6], buf[7]);
            }

            _lTrans = _gifinfo.Gif89.transparent;

            // Post WHKNOWN
            OnSize(_xWid, _yHei, _lTrans);
        }

        if (!useGlobalColormap)
        {
            if (!ReadColorMap(bitPixel, apeLocal))
            {
                XX_DMsg(DBG_IMAGE, ("error reading local colormap\n"));
                goto exitPoint;
            }
        }

        // We allocate a frame record for each imag in the GIF stream, including
        // the first/primary image.
        pgfNew = (GIFFRAME *) MemAllocClear(Mt(CImgTaskGifFrame), sizeof(GIFFRAME));

        if ( pgfNew == NULL )
        {
            XX_DMsg(DBG_IMAGE, ("not enough memory for GIF frame\n"));
            goto exitPoint;
        }

        if ( _gifinfo.Gif89.delayTime != -1 )
        {
            // we have a fresh control extension for this block

            // convert to milliseconds
            pgfNew->uiDelayTime = _gifinfo.Gif89.delayTime * 10;


            //REVIEW(seanf): crude hack to cope with 'degenerate animations' whose timing is set to some
            //               small value becaue of the delays imposed by Netscape's animation process
            if ( pgfNew->uiDelayTime <= 50 ) // assume these small values imply Netscape encoding delay
                pgfNew->uiDelayTime = 100;   // pick a larger value s.t. the frame will be visible

            pgfNew->bDisposalMethod =  _gifinfo.Gif89.disposal;

            if (_gifinfo.Gif89.transparent != -1)
                pgfNew->bTransFlags |= TRANSF_TRANSPARENT;
        }
        else
        {   // fake one up s.t. GIFs that rely solely on Netscape's delay to time their animations will play
            // The spec says that the scope of one of these blocks is the image after the block.
            // Netscape says 'until further notice'. So we play it their way up to a point. We
            // propagate the disposal method and transparency. Since Netscape doesn't honor the timing
            // we use our default timing for these images.
            pgfNew->uiDelayTime = 100;
            pgfNew->bDisposalMethod =  _gifinfo.Gif89.disposal;
        }

        pgfNew->top = LM_to_uint(buf[2], buf[3]);       // bounds relative to the GIF logical screen
        pgfNew->left = LM_to_uint(buf[0], buf[1]);
        pgfNew->width = LM_to_uint(buf[4], buf[5]);
        pgfNew->height = LM_to_uint(buf[6], buf[7]);

        if (_gifinfo.Gif89.transparent != -1)
            pgfNew->bTransFlags |= TRANSF_TRANSPARENT;
            
        // Images that are offset, or do not cover the full logical screen are 'transparent' in the
        // sense that they require us to matte the frame onto the background.

        if (!_gad.fHasTransparency)
        {
                if (pgfNew->top != 0 ||
                    pgfNew->left != 0 ||
                    (UINT)pgfNew->width != (UINT)GifScreen.Width ||
                    (UINT)pgfNew->height != (UINT)GifScreen.Height)
                    _fMustBeTransparent = TRUE;

            if ((pgfNew->bTransFlags & TRANSF_TRANSPARENT) || _fMustBeTransparent)
            {
                _gad.fHasTransparency = TRUE;
                if (_lTrans == -1)
                {
                    _lTrans = 0;
                    OnTrans(_lTrans);
                }
            }
        }

        // We don't need to allocate a handle for the simple region case.
        // FrancisH says Windows is too much of a cheapskate to allow us the simplicity
        // of allocating the region once and modifying as needed. Well, okay, he didn't
        // put it that way...
        pgfNew->hrgnVis = NULL;
        pgfNew->bRgnKind = NULLREGION;

        if (!useGlobalColormap)
        {
            cpeColors = bitPixel;
            ppeColors = apeLocal;
        }
        else
        {
            cpeColors = GifScreen.BitPixel;
            ppeColors = _ape;
        }

        // First frame: must be able to progressively render, so stick it in _gad now.
        if (_gad.pgf == NULL)
        {
            _gad.pgf = pgfNew;

            // Remember the color table for the first frame so that
            // progressive dithering can be done on it.

            memcpy(_apeFirst, ppeColors, cpeColors * sizeof(PALETTEENTRY));
        }

        pibd = ReadImage(LM_to_uint(buf[4], buf[5]), // width
                            LM_to_uint(buf[6], buf[7]), // height
                            BitSet(buf[8], INTERLACE),
                            imageCount != imageNumber,
                            cpeColors, ppeColors, &pbBits,
                            _gifinfo.Gif89.transparent);

        if (pibd != NULL)
        {
            if (pgfLast != NULL)
            {
                // Set up pgfNew if not the first frame
                
                int transparent = (pgfNew->bTransFlags & TRANSF_TRANSPARENT) ? _gifinfo.Gif89.transparent : -1;

                _gad.fAnimated = TRUE; // say multi-image == animated

#ifdef _MAC
                pibd->ComputeTransMask(0, pgfNew->height, transparent, transparent);
#else
                if (_colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE)) // palettized, use DIB_PAL_COLORS
                {   // This will also dither the bits to the screen palette

                    if (x_Dither(pbBits, ppeColors, pgfNew->width, pgfNew->height, transparent))
                        goto exitPoint;
                        
                    pibd->ComputeTransMask(0, pgfNew->height, g_wIdxTrans, 255);
                }
#endif

                pgfNew->pibd = pibd;
                pibd = NULL;

                pgfLast->pgfNext = pgfNew;

                OnAnim();
            }
            else
            {
                // first frame: already been set up
                
                Assert(_gad.pgf == pgfNew);
                Assert(_gad.pgf->pibd == pibd);
                
                pibd = NULL;

            }
            pgfLast = pgfNew;
            pgfNew = NULL;
        }

        // make the _gifinfo.Gif89.delayTime stale, so we know if we got a new
        // GCE for the next image
        _gifinfo.Gif89.delayTime = -1;
    }

    if ( imageCount > imageNumber )
        _gad.fAnimated = TRUE; // say multi-image == animated

exitPoint:
    if (pibd)
        delete pibd;
    if (pgfNew && _gad.pgf != pgfNew)
        MemFree(pgfNew);
    return;
}

BOOL CImgTaskGif::ReadColorMap(long number, PALETTEENTRY * ppe)
{
    if (!Read(ppe, number * 3))
        return(FALSE);

    if (number)
    {
#ifdef _MAC
        DWORD UNALIGNED * pdwSrc = (DWORD *)((BYTE *)ppe + (number - 1) * 3);
 
        DWORD * pdwDst = (DWORD *)&ppe[number - 1];

        for (; number > 0; --number)
        {
            *pdwDst-- = (*pdwSrc & 0xFFFFFF00);
            pdwSrc = (DWORD *)((BYTE *)pdwSrc - 3);
        }
#else
#ifndef UNIX
        DWORD UNALIGNED * pdwSrc = (DWORD *)((BYTE *)ppe + (number - 1) * 3);
 
        DWORD * pdwDst = (DWORD *)&ppe[number - 1];

        for (; number > 0; --number)
        {
            *pdwDst-- = (*pdwSrc & 0xFFFFFF);
            pdwSrc = (DWORD *)((BYTE *)pdwSrc - 3);
        }
#else
        BYTE * pdwSrc = (BYTE *)ppe + (number - 1) * 3;
        DWORD * pdwDst = (DWORD *)&ppe[number - 1];

        for (; number > 0; --number)
        {
            *pdwDst-- = ((((DWORD)pdwSrc[2] << 8) | pdwSrc[1]) << 8) | pdwSrc[0];
            pdwSrc -= 3;
        }
#endif
#endif // _MAC
    }

    return(TRUE);
}

long CImgTaskGif::DoExtension(long label)
{
    unsigned char buf[256];
    int count;

    switch (label)
    {
        case 0x01:              /* Plain Text Extension */
            break;
        case 0xff:              /* Application Extension */
            // Is it the Netscape looping extension
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 11)
            {
                char *szNSExt = "NETSCAPE2.0";

                if ( memcmp( buf, szNSExt, strlen( szNSExt ) ) == 0 )
                { // if it has their signature, get the data subblock with the iter count
                    count = GetDataBlock((unsigned char *) buf);
                    if ( count >= 3 )
                    {
                        _gad.fLooped = TRUE;
                        _gad.cLoops = (buf[2] << 8) | buf[1];
                    }
                }
            }
            while (GetDataBlock((unsigned char *) buf) > 0)
                ;
            return FALSE;
            break;
        case 0xfe:              /* Comment Extension */
            while (GetDataBlock((unsigned char *) buf) > 0)
            {
                XX_DMsg(DBG_IMAGE, ("GIF comment: %s\n", buf));
            }
            return FALSE;
        case 0xf9:              /* Graphic Control Extension */
            count = GetDataBlock((unsigned char *) buf);
            if (count >= 3)
            {
                _gifinfo.Gif89.disposal = (buf[0] >> 2) & 0x7;
                _gifinfo.Gif89.inputFlag = (buf[0] >> 1) & 0x1;
                _gifinfo.Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
                if ((buf[0] & 0x1) != 0)
                    _gifinfo.Gif89.transparent = buf[3];
                else
                    _gifinfo.Gif89.transparent = -1;
            }
            while (GetDataBlock((unsigned char *) buf) > 0)
                ;
            return FALSE;
        default:
            break;
    }

    while (GetDataBlock((unsigned char *) buf) > 0)
        ;

    return FALSE;
}


long CImgTaskGif::GetDataBlock(unsigned char *buf)
{
    unsigned char count;

    count = 0;

    if (!ReadByte(&count))
    {
        return -1;
    }

    _gifinfo.ZeroDataBlock = count == 0;

    if ((count != 0) && (!Read(buf, count)))
    {
        return -1;
    }

    return ((long) count);
}

#define MIN_CODE_BITS 5
#define MIN_STACK_SIZE 64
#define MINIMUM_CODE_SIZE 2

BOOL CImgTaskGif::initLWZ(long input_code_size)
{
    if (input_code_size < MINIMUM_CODE_SIZE)
        return FALSE;

    _gifinfo.set_code_size = input_code_size;
    _gifinfo.code_size = _gifinfo.set_code_size + 1;
    _gifinfo.clear_code = 1 << _gifinfo.set_code_size;
    _gifinfo.end_code = _gifinfo.clear_code + 1;
    _gifinfo.max_code_size = 2 * _gifinfo.clear_code;
    _gifinfo.max_code = _gifinfo.clear_code + 2;

    _gifinfo.curbit = _gifinfo.lastbit = 0;
    _gifinfo.last_byte = 2;
    _gifinfo.get_done = FALSE;

    _gifinfo.return_clear = TRUE;

    if(input_code_size >= MIN_CODE_BITS)
        _gifinfo.stacksize = ((1 << (input_code_size)) * 2);
    else
        _gifinfo.stacksize = MIN_STACK_SIZE;

    if ( _gifinfo.pstack != NULL )
        MemFree( _gifinfo.pstack );
    if ( _gifinfo.table[0] != NULL  )
        MemFree( _gifinfo.table[0] );
    if ( _gifinfo.table[1] != NULL  )
        MemFree( _gifinfo.table[1] );

    _gifinfo.table[0] = 0;
    _gifinfo.table[1] = 0;
    _gifinfo.pstack = 0;

    _gifinfo.pstack = (unsigned short *)MemAlloc(Mt(CImgTaskGifStack), (_gifinfo.stacksize)*sizeof(unsigned short));
    if(_gifinfo.pstack == 0){
        goto ErrorExit;
    }
    _gifinfo.sp = _gifinfo.pstack;

    // Initialize the two tables.
    _gifinfo.tablesize = (_gifinfo.max_code_size);
    _gifinfo.table[0] = (unsigned short *)MemAlloc(Mt(CImgTaskGifTable0), (_gifinfo.tablesize)*sizeof(unsigned short));
    _gifinfo.table[1] = (unsigned short *)MemAlloc(Mt(CImgTaskGifTable1), (_gifinfo.tablesize)*sizeof(unsigned short));
    if((_gifinfo.table[0] == 0) || (_gifinfo.table[1] == 0)){
        Assert(0);
        goto ErrorExit;
    }

    return TRUE;

ErrorExit:
    if(_gifinfo.pstack){
        MemFree(_gifinfo.pstack);
        _gifinfo.pstack = 0;
    }

    if(_gifinfo.table[0]){
        MemFree(_gifinfo.table[0]);
        _gifinfo.table[0] = 0;
    }

    if(_gifinfo.table[1]){
        MemFree(_gifinfo.table[1]);
        _gifinfo.table[1] = 0;
    }

    return FALSE;

}

long CImgTaskGif::nextCode(long code_size)
{
    static const long maskTbl[16] =
    {
        0x0000, 0x0001, 0x0003, 0x0007,
        0x000f, 0x001f, 0x003f, 0x007f,
        0x00ff, 0x01ff, 0x03ff, 0x07ff,
        0x0fff, 0x1fff, 0x3fff, 0x7fff,
    };
    long i, j, ret, end;
    unsigned char *buf = &_gifinfo.buf[0];

    if (_gifinfo.return_clear)
    {
        _gifinfo.return_clear = FALSE;
        return _gifinfo.clear_code;
    }

    end = _gifinfo.curbit + code_size;

    if (end >= _gifinfo.lastbit)
    {
        long count;

        if (_gifinfo.get_done)
        {
            return -1;
        }
        buf[0] = buf[_gifinfo.last_byte - 2];
        buf[1] = buf[_gifinfo.last_byte - 1];

        if ((count = GetDataBlock(&buf[2])) == 0)
            _gifinfo.get_done = TRUE;
        if (count < 0)
        {
            return -1;
        }
        _gifinfo.last_byte = 2 + count;
        _gifinfo.curbit = (_gifinfo.curbit - _gifinfo.lastbit) + 16;
        _gifinfo.lastbit = (2 + count) * 8;

        end = _gifinfo.curbit + code_size;

        // Okay, bug 30784 time. It's possible that we only got 1
        // measly byte in the last data block. Rare, but it does happen.
        // In that case, the additional byte may still not supply us with
        // enough bits for the next code, so, as Mars Needs Women, IE
        // Needs Data.
        if (end >= _gifinfo.lastbit && !_gifinfo.get_done)
        {
            // protect ourselve from the ( theoretically impossible )
            // case where between the last data block, the 2 bytes from
            // the block preceding that, and the potential 0xFF bytes in
            // the next block, we overflow the buffer.
            // Since count should always be 1,
            Assert(count == 1);
            // there should be enough room in the buffer, so long as someone
            // doesn't shrink it.
            if (count + 0x101 >= sizeof(_gifinfo.buf))
            {
                Assert(FALSE);
                return -1;
            }

            if ((count = GetDataBlock(&buf[2 + count])) == 0)
                _gifinfo.get_done = TRUE;
            if (count < 0)
            {
                return -1;
            }
            _gifinfo.last_byte += count;
            _gifinfo.lastbit = _gifinfo.last_byte * 8;

            end = _gifinfo.curbit + code_size;
        }
    }

    j = end / 8;
    i = _gifinfo.curbit / 8;

    if (i == j)
        ret = buf[i];
    else if (i + 1 == j)
        ret = buf[i] | (((long) buf[i + 1]) << 8);
    else
        ret = buf[i] | (((long) buf[i + 1]) << 8) | (((long) buf[i + 2]) << 16);

    ret = (ret >> (_gifinfo.curbit % 8)) & maskTbl[code_size];

    _gifinfo.curbit += code_size;

    return ret;
}

// Grows the stack and returns the top of the stack.
unsigned short *
CImgTaskGif::growStack()
{
    long index;
    unsigned short *lp;

    if (_gifinfo.stacksize >= MAX_STACK_SIZE)
        return 0;

    index = (_gifinfo.sp - _gifinfo.pstack);
    if (MemRealloc(Mt(CImgTaskGifStack), (void **)&_gifinfo.pstack, (_gifinfo.stacksize)*2*sizeof(unsigned short)))
        return 0;

    _gifinfo.sp = &(_gifinfo.pstack[index]);
    _gifinfo.stacksize = (_gifinfo.stacksize)*2;
    lp = &(_gifinfo.pstack[_gifinfo.stacksize]);
    return lp;
}

BOOL
CImgTaskGif::growTables()
{
    if (MemRealloc(Mt(CImgTaskGifTable0), (void **)&_gifinfo.table[0], (_gifinfo.max_code_size)*sizeof(unsigned short)))
        return FALSE;

    if (MemRealloc(Mt(CImgTaskGifTable1), (void **)&_gifinfo.table[1], (_gifinfo.max_code_size)*sizeof(unsigned short)))
        return FALSE;

    return TRUE;
}

inline
long CImgTaskGif::readLWZ()
{
    return((_gifinfo.sp > _gifinfo.pstack) ? *--(_gifinfo.sp) : nextLWZ());
}

#define CODE_MASK 0xffff

long CImgTaskGif::nextLWZ()
{
    long code, incode;
    unsigned short usi;
    unsigned short *table0 = _gifinfo.table[0];
    unsigned short *table1 = _gifinfo.table[1];
    unsigned short *pstacktop = &(_gifinfo.pstack[_gifinfo.stacksize]);

    while ((code = nextCode(_gifinfo.code_size)) >= 0)
    {
        if (code == _gifinfo.clear_code)
        {

            /* corrupt GIFs can make this happen */
            if (_gifinfo.clear_code >= (1 << MAX_LWZ_BITS))
            {
                return -2;
            }


            _gifinfo.code_size = _gifinfo.set_code_size + 1;
            _gifinfo.max_code_size = 2 * _gifinfo.clear_code;
            _gifinfo.max_code = _gifinfo.clear_code + 2;

            if(!growTables())
                return -2;

            table0 = _gifinfo.table[0];
            table1 = _gifinfo.table[1];

            _gifinfo.tablesize = _gifinfo.max_code_size;


            for (usi = 0; usi < _gifinfo.clear_code; ++usi)
            {
                table1[usi] = usi;
            }
            memset(table0,0,sizeof(unsigned short )*(_gifinfo.tablesize));
            memset(&table1[_gifinfo.clear_code],0,sizeof(unsigned short)*((_gifinfo.tablesize)-_gifinfo.clear_code));
            _gifinfo.sp = _gifinfo.pstack;
            do
            {
                _gifinfo.firstcode = _gifinfo.oldcode = nextCode(_gifinfo.code_size);
            }
            while (_gifinfo.firstcode == _gifinfo.clear_code);

            return _gifinfo.firstcode;
        }
        if (code == _gifinfo.end_code)
        {
            long count;
            unsigned char buf[260];

            if (_gifinfo.ZeroDataBlock)
            {
                return -2;
            }

            while ((count = GetDataBlock(buf)) > 0)
                ;

            if (count != 0)
            return -2;
        }

        incode = code;

        if (code >= MAX_TABLE_SIZE)
            return -2;
        
        if (code >= _gifinfo.max_code)
        {
            if (_gifinfo.sp >= pstacktop){
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
            }
            *(_gifinfo.sp)++ = (unsigned short)((CODE_MASK ) & (_gifinfo.firstcode));
            code = _gifinfo.oldcode;
        }

        while (code >= _gifinfo.clear_code)
        {
            if (_gifinfo.sp >= pstacktop){
                pstacktop = growStack();
                if(pstacktop == 0)
                    return -2;
            }
            *(_gifinfo.sp)++ = table1[code];
            if (code == (long)(table0[code]))
            {
                return (code);
            }
            code = (long)(table0[code]);
        }

        if (_gifinfo.sp >= pstacktop){
            pstacktop = growStack();
            if(pstacktop == 0)
                return -2;
        }
        _gifinfo.firstcode = (long)table1[code];
        *(_gifinfo.sp)++ = table1[code];

        if ((code = _gifinfo.max_code) < (1 << MAX_LWZ_BITS))
        {
            table0[code] = (_gifinfo.oldcode) & CODE_MASK;
            table1[code] = (_gifinfo.firstcode) & CODE_MASK;
            ++_gifinfo.max_code;
            if ((_gifinfo.max_code >= _gifinfo.max_code_size) && (_gifinfo.max_code_size < ((1 << MAX_LWZ_BITS))))
            {
                _gifinfo.max_code_size *= 2;
                ++_gifinfo.code_size;
                if(!growTables())
                    return -2;

                table0 = _gifinfo.table[0];
                table1 = _gifinfo.table[1];

                // Tables have been reallocated to the correct size but initialization
                // still remains to be done. This initialization is different from
                // the first time initialization of these tables.
                memset(&(table0[_gifinfo.tablesize]),0,
                        sizeof(unsigned short )*(_gifinfo.max_code_size - _gifinfo.tablesize));

                memset(&(table1[_gifinfo.tablesize]),0,
                        sizeof(unsigned short )*(_gifinfo.max_code_size - _gifinfo.tablesize));

                _gifinfo.tablesize = (_gifinfo.max_code_size);


            }
        }

        _gifinfo.oldcode = incode;

        if (_gifinfo.sp > _gifinfo.pstack)
            return ((long)(*--(_gifinfo.sp)));
    }
    return code;
}

void
CImgTaskGif::OnProg(BOOL fLast, ULONG ulCoversImg)
{
    BOOL fInvalAll;

    CalculateUpdateRect(_yLogRow - PROG_INTERVAL + 1, _yLogRow, &fInvalAll, &_yBot);

    if (fLast || (GetTickCount() - _dwTickProg > 1000))
    {
#ifndef _MAC
        ProgressiveDithering(_yLogRowDraw, _yLogRow);
#endif
        _yLogRowDraw = _yLogRow;
    }

    super::OnProg(fLast, ulCoversImg, fInvalAll, _yBot);
}


#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

BOOL
CImgTaskGif::ReadScanline(unsigned char *pb, int cb)
{
    int i;
    long b;

    for (i = cb; --i >= 0;)
    {
        b = readLWZ();
        if (b < 0)
            return FALSE;
        *pb++ = (unsigned char)b;
    }
    return TRUE;
}

#pragma optimize("", on)

#ifdef WIN16
#undef FillMemory
#define FillMemory( _pv, _cb, ch) hmemset( (_pv), ch, (_cb) )
#endif

CImgBitsDIB *
CImgTaskGif::ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame,
    int cpeColors, PALETTEENTRY * ppeColors, HLPBYTE * ppbBits, long lTrans)
{
    unsigned char c;
    long ypos = 0;
    long padlen = ((len + 3) / 4) * 4;
    char buf[256]; // need a buffer to read trailing blocks ( up to terminator ) into
    ULONG ulCoversImg = IMGBITS_PARTIAL;
    HLPBYTE pbBits;
    BOOL fAbort = FALSE;
    HRESULT hr;
    CImgBitsDIB *pibd = NULL;
    BOOL fColorTable;
    

    /*
       **  Initialize the Compression routines
     */
    if (!ReadByte(&c))
    {
        goto abort;
    }

    _cbImage = padlen * height * sizeof(char);

    if (_cbImage > dwMaxGIFBits)
       goto abort;

    pibd = new CImgBitsDIB();
    if (!pibd)
        goto abort;

    // don't bother with allocating a color table if we're going to dither
    // to our standard palette
    
    fColorTable = (_colorMode != 8 || _pImgInfo->TstFlags(DWNF_RAWIMAGE));

    if (!fColorTable)
    {
        hr = THR(pibd->AllocDIB(8, len, height, NULL, 0, -1, lTrans == -1));
        if (hr)
            goto abort;
    }
    else
    {
        RGBQUAD argbTable[256];

        if (cpeColors > 256)
            cpeColors = 256;
            
        CopyColorsFromPaletteEntries(argbTable, ppeColors, cpeColors);
        
        hr = THR(pibd->AllocDIB(8, len, height, argbTable, cpeColors, lTrans, lTrans == -1));
        if (hr)
            goto abort;
    }

    pbBits = (BYTE *)pibd->GetBits();

    if (!fGIFFrame)
    {
        _pImgBits = pibd;
        _gad.pgf->pibd = pibd;
        _pbDst = pbBits;

        if (fInterlace && _colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
        {
            _pbSrcAlloc = (HLPBYTE) MemAlloc(Mt(CImgTaskGifBits), _cbImage);
            if (_pbSrcAlloc == NULL)
                goto abort;
            _pbSrc = _pbSrcAlloc;
            pbBits = _pbSrc;
        }
        else
        {
            _pbSrc = pbBits;
        }
    }

 #ifndef _MAC
   if (_colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
    {
        int nDifferent;

        nDifferent = x_ComputeConstrainMap(cpeColors, ppeColors, lTrans, _mapConstrained);

        if (_pImgInfo->TstFlags(DWNF_FORCEDITHER)
            ||  (cpeColors > 16 && nDifferent > 15))
            _fDither = TRUE;
        else
            TraceTag((tagImgTaskGif, "No dithering needed for '%ls'", GetUrl()));
    }
#endif // _MAC

    if (c == 1)
    {
        // Netscape seems to field these bogus GIFs by filling treating them
        // as transparent. While not the optimal way to simulate this effect,
        // we'll fake it by pushing the initial code size up to a safe value,
        // consuming the input, and returning a buffer full of the transparent
        // color or zero, if no transparency is indicated.
        if (initLWZ(MINIMUM_CODE_SIZE))
            while (readLWZ() >= 0);

        if (lTrans != -1)
            FillMemory(_pbSrc, _cbImage, lTrans);
        else // fall back on the background color
            FillMemory(_pbSrc, _cbImage, 0);

        if (!fGIFFrame)
        {
            _yLogRow = height - 1;
            OnProg(TRUE, IMGBITS_TOTAL);
        }

        goto done;
    }


    if (initLWZ(c) == FALSE)
        goto abort;

    if (fInterlace)
    {
        long i;
        long pass = 0, step = 8;

        if (!fGIFFrame && (height > 4))
            _fInterleaved = TRUE;

        for (i = 0; i < height; i++)
        {
//          XX_DMsg(DBG_IMAGE, ("readimage, logical=%d, offset=%d\n", i, padlen * ((height-1) - ypos)));
            if (    fAbort
#ifdef _MAC
                ||  !ReadScanline(&pbBits[padlen * ypos], len))
#else
                ||  !ReadScanline(&pbBits[padlen * ((height-1) - ypos)], len))
#endif
            {
            #if DBG==1 || defined(PERFTAGS)
                if (IsPerfDbgEnabled(tagImgTaskGifAbort))
                {
                    fAbort = TRUE;
                    memset(&pbBits[padlen * ((height-1) - ypos)], 0, len);
                }
                else
            #endif
                    break;
            }

            ypos += step;
            while (ypos >= height)
            {
                if (pass++ > 0)
                    step /= 2;
                ypos = step / 2;
                if (!fGIFFrame && pass == 1)
                {
                    ulCoversImg = IMGBITS_TOTAL;
                }
            }
            if (!fGIFFrame)
            {
                _yLogRow = i;

                if ((i & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE, ulCoversImg);
                }
            }
        }

        if (!fGIFFrame)
        {
            OnProg(TRUE, IMGBITS_TOTAL);
        }
    }
    else
    {
#ifdef _MAC
        for (ypos = 0; ypos < height; ypos++)
#else
        for (ypos = height-1; ypos >= 0; ypos--)
#endif
        {
            if (!ReadScanline(&pbBits[padlen * ypos], len))
                break;

            if (!fGIFFrame)
            {
                _yLogRow++;
//              XX_DMsg(DBG_IMAGE, ("readimage, logical=%d, offset=%d\n", _yLogRow, padlen * ypos));
                if ((_yLogRow & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE, IMGBITS_PARTIAL);
                }
            }
        }

        if (!fGIFFrame)
        {
            OnProg(TRUE, IMGBITS_TOTAL);
        }
    }

    // consume blocks up to image block terminator so we can proceed to the next image
    while (GetDataBlock((unsigned char *) buf) > 0)
                ;

done:
    *ppbBits = pbBits;

    return (pibd);

abort:
    delete pibd;
    
    return NULL;
}


void
CImgTaskGif::ProgressiveDithering(int logicalFill, int logicalRow)
{
    BOOL bitbltNeeded = TRUE;
    int i;
    // Note: We only show the primary frame during prog draw, so we use the first GIFFRAME
    int padXSize;
    int row = logicalRow;
    int pass;
    int band;
    int band2;
    long offset;
    int passFill;
    int rowFill;
    int bandFill;
    int step;
    int j;

    if (logicalFill < 0)
        logicalFill = 0;

    if (_fDither && _pvDithData == NULL)
    {
        _pvDithData = pCreateDitherData(_gad.pgf->width);
        if (_pvDithData == NULL)
            return;
    }

    padXSize = ((_gad.pgf->width + 3) / 4) * 4;

    if (_fInterleaved)
    {
        getPassInfo(logicalFill,_gad.pgf->height,&passFill,&rowFill,&bandFill);
        getPassInfo(logicalRow,_gad.pgf->height,&pass,&row,&band);
        step = passFill == 0 ? 8 : bandFill*2;
        for (i = logicalFill; i <= logicalRow;i++)
        {
            offset = (long)padXSize*(long)(_gad.pgf->height - rowFill - 1) ;    /* the DIB is stored upside down */

            band2 = rowFill <= row ? band : band*2;
            if (band2 != 1)
            {
                if (_colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
                {
                    x_ColorConstrain(_pbSrc+offset,
                                     _pbDst+offset,
                                     _mapConstrained,
                                     _gad.pgf->width);
                }
                else
                    hmemcpy(_pbDst+offset,_pbSrc+offset,padXSize);
                if (rowFill+band2 > _gad.pgf->height)
                    band2 = _gad.pgf->height-rowFill;

                for (j = 1; j < band2; j++)
                    hmemcpy(_pbDst+(offset-j*padXSize),_pbDst+offset,padXSize);

                if (_colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
                {
                    _gad.pgf->pibd->ComputeTransMask(rowFill, band2, g_wIdxTrans, 255);
                }
            }

            if ((rowFill += step) >= _gad.pgf->height)
            {
                if (passFill++ > 0)
                    step /= 2;
                rowFill = step / 2;
            }

        }
        switch (pass)
        {
            case 0:
                band += row;
                break;
            case 3:
                band = _gad.pgf->height-row-1;
                break;
            default:
                band = _gad.pgf->height;
                break;
        }
        if (band > _gad.pgf->height)
            band = _gad.pgf->height;

        if (band > 0)
        {
            bitbltNeeded = (pass == 3);
        }
        if (bitbltNeeded)
        {
            if ((row >= _gad.pgf->height) ||
                (logicalRow == _gad.pgf->height - 1))
                row = _gad.pgf->height - 1;
        }
    }

    if (bitbltNeeded)
    {
        band = row + 1;
        if (_colorMode == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
        {
            if (_yDithRow <= row)
            {
                if (_fDither)
                {
                    x_DitherRelative(_pbSrc,
                                     _pbDst,
                                     _apeFirst,
                                     _gad.pgf->width,
                                     _gad.pgf->height,
                                     (_gad.pgf->bTransFlags & TRANSF_TRANSPARENT) ? _gifinfo.Gif89.transparent : -1,
                                     (int *)_pvDithData,
                                     _yDithRow,
                                     row);
                }
                else
                {
                    ULONG cb = (long)padXSize * (long)(_gad.pgf->height - row - 1);
                    x_ColorConstrain(_pbSrc + cb, _pbDst + cb, _mapConstrained,
                        padXSize * (row - _yDithRow + 1));
                }

                _gad.pgf->pibd->ComputeTransMask(_yDithRow, row - _yDithRow + 1, g_wIdxTrans, 255);
                _yDithRow = band;
            }
        }
    }
}

void CImgTaskGif::BltDib(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags)
{
    int ySrcBot;

    if (_yLogRowDraw < 0 || _gad.pgf == NULL)
        return;

    if (_fInterleaved)
        ySrcBot = min(_yHei, (_yLogRowDraw + 1L) * 8L);
    else
        ySrcBot = _yLogRowDraw + 1;

    _gad.pgf->pibd->SetValidLines(ySrcBot);

    _gad.pgf->pibd->StretchBltOffset(hdc, prcDst, prcSrc, _gad.pgf->left, _gad.pgf->top, dwRop, dwFlags);
}

void
CImgTaskGif::CalculateUpdateRect(int logicalRow0, int logicalRowN, BOOL *pfInvalidateAll, LONG *pyBottom)
{
    *pfInvalidateAll = FALSE;

    if (logicalRowN == 0)
    {
        *pyBottom = 0;
        return;
    }

    if (_gad.pgf)
    {
        logicalRow0 += _gad.pgf->top;
        logicalRowN += _gad.pgf->top;
    }

    if (_fInterleaved)
    {
        int pass0, passN;
        int row0, rowN;
        int band0, bandN;

        getPassInfo(logicalRow0, _yHei, &pass0, &row0, &band0);
        getPassInfo(logicalRowN, _yHei, &passN, &rowN, &bandN);

        if (passN > pass0 + 1)
            *pfInvalidateAll = TRUE;

        *pyBottom = rowN + bandN;

        // We must special case the last row of last pass to deal with dithering
        // the possibly even numbered last row
        if (logicalRowN == _yHei - 1)
            *pyBottom = _yHei;
    }
    else
        *pyBottom = logicalRowN + 1;
}

