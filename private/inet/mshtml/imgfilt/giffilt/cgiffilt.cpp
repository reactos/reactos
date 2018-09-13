// CGIFFilt.Cpp : Implementation of CGIFFilterApp and DLL registration.

#include "StdAfx.H"
#include "Resource.H"
#include "Include\GIFFilt.H"
#include "CGIFFilt.H"

#undef  DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID( IID_IDirectDrawSurface,		0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,
0x20,0xAF,0x0B,0xE5,0x60 );

/////////////////////////////////////////////////////////////////////////////
// GIF reader

#define Assert(x)
#define Verify(x)   x
#define TraceTag(x)

#define MemAlloc(cb)    ((void *)malloc(cb))
#define MemFree(p)  free(p)

void * MemAllocClear(size_t cb)
{
    void *p = MemAlloc(cb);

    if (p)
        ZeroMemory(p, cb);

    return (void *)p;
}

HRESULT MemRealloc(void **pp, size_t cb)
{
    void *p = realloc(*(pp), cb);

    if (p)
    {
        *(pp) = p;
        return S_OK;
    }
    else
        return E_FAIL;
}

void CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb,
    UINT uCount)
{
    while (uCount--)
    {
        ppe->peRed   = prgb->rgbRed;
        ppe->peGreen = prgb->rgbGreen;
        ppe->peBlue  = prgb->rgbBlue;
        ppe->peFlags = 0;

        prgb++;
        ppe++;
    }
}

BOOL IsGifHdr(BYTE * pb)
{
    return(pb[0] == 'G' && pb[1] == 'I' && pb[2] == 'F'
        && pb[3] == '8' && (pb[4] == '7' || pb[4] == '9') && pb[5] == 'a');
}

BOOL CGIFFilter::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    ULONG cbRead;
    HRESULT hr;

    if (pcbRead == NULL)
        pcbRead = &cbRead;

    hr = m_pStream->Read(pv, cb, pcbRead);

    return((hr == S_OK) && (*pcbRead > 0));
}


void getPassInfo(int logicalRowX, int height, int *pPassX, int *pRowX, int *pBandX)
{
    int passLow, passHigh, passBand;
    int pass = 0;
    int step = 8;
    int ypos = 0;

    if (logicalRowX >= height)
        logicalRowX = height - 1;
    passBand = 8;
    passLow = 0;
    while (step > 1)
    {
        if (pass == 3)
            passHigh = height - 1;
        else
            passHigh = (height - 1 - ypos) / step + passLow;
        if (logicalRowX >= passLow && logicalRowX <= passHigh)
        {
            *pPassX = pass;
            *pRowX = ypos + (logicalRowX - passLow) * step;
            *pBandX = passBand;
            return;
        }
        if (pass++ > 0)
            step /= 2;
        ypos = step / 2;
        passBand /= 2;
        passLow = passHigh + 1;
    }
}

HRESULT
CGIFFilter::ReadGIFMaster()
{
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    long useGlobalColormap;
    long imageCount = 0;
    long imageNumber = 1;
    unsigned long i;
    GIFSCREEN GifScreen;
    long bitPixel;
    GIFFRAME * pgfLast = NULL;
    GIFFRAME * pgfNew = NULL;
    RGBQUAD argbLocal[MAXCOLORMAPSIZE];
    RGBQUAD * prgbColors;
    int crgbColors;
    HRESULT hr = S_OK;
    
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
        int scale = 65536 / MAXCOLORMAPSIZE;

        if (ReadColorMap(GifScreen.BitPixel, GifScreen.ColorMap))
        {
            XX_DMsg(DBG_IMAGE, ("error reading global colormap\n"));
            goto exitPoint;
        }
        m_nColors = GifScreen.BitPixel;
        for (i = 0; i < m_nColors; i++)
        {
            m_argbPalette[i].rgbRed = (BYTE) (GifScreen.ColorMap[0][i]);
            m_argbPalette[i].rgbGreen = (BYTE) (GifScreen.ColorMap[1][i]);
            m_argbPalette[i].rgbBlue = (BYTE) (GifScreen.ColorMap[2][i]);
            m_argbPalette[i].rgbReserved = (BYTE) 0;
        }
        for (i = GifScreen.BitPixel; i < MAXCOLORMAPSIZE; i++)
        {
            m_argbPalette[i].rgbRed = (BYTE) 0;
            m_argbPalette[i].rgbGreen = (BYTE) 0;
            m_argbPalette[i].rgbBlue = (BYTE) 0;
            m_argbPalette[i].rgbReserved = (BYTE) 0;
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
        if (!Read(&c, 1))
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
            if (!Read(&c, 1))
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

        // BUGBUG: Disabling animated GIFs for now
        
        if (imageCount > 1)
            goto exitPoint;
            
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
                m_nWidth = GifScreen.Width;
                m_nHeight = GifScreen.Height;
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

                m_nWidth = LM_to_uint(buf[4], buf[5]);
                m_nHeight = LM_to_uint(buf[6], buf[7]);
            }

            _lTrans = _gifinfo.Gif89.transparent;

        }

        if (!useGlobalColormap)
        {
            if (ReadColorMap(bitPixel, localColorMap))
            {
                XX_DMsg(DBG_IMAGE, ("error reading local colormap\n"));
                goto exitPoint;
            }
        }

        // We allocate a frame record for each imag in the GIF stream, including
        // the first/primary image.
        pgfNew = (GIFFRAME *) MemAllocClear(sizeof(GIFFRAME));

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

            pgfNew->bDisposalMethod =  (unsigned char)_gifinfo.Gif89.disposal;

            if (_gifinfo.Gif89.transparent != -1)
                pgfNew->bTransFlags |= TRANSF_TRANSPARENT;

            pgfNew->bTransIndex = (unsigned char)_gifinfo.Gif89.transparent;
        }
        else
        {   // fake one up s.t. GIFs that rely solely on Netscape's delay to time their animations will play
            // The spec says that the scope of one of these blocks is the image after the block.
            // Netscape says 'until further notice'. So we play it their way up to a point. We
            // propagate the disposal method and transparency. Since Netscape doesn't honor the timing
            // we use our default timing for these images.
            pgfNew->uiDelayTime = 100;
            pgfNew->bDisposalMethod =  (unsigned char)_gifinfo.Gif89.disposal;

            if (_gifinfo.Gif89.transparent != -1)
                pgfNew->bTransFlags |= TRANSF_TRANSPARENT;

            pgfNew->bTransIndex = (unsigned char)_gifinfo.Gif89.transparent;
        }

        pgfNew->top = LM_to_uint(buf[2], buf[3]);       // bounds relative to the GIF logical screen
        pgfNew->left = LM_to_uint(buf[0], buf[1]);
        pgfNew->width = LM_to_uint(buf[4], buf[5]);
        pgfNew->height = LM_to_uint(buf[6], buf[7]);

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
            // remember that we saw a local color table and only map two-color images
            // if we have a homogenous color environment
            _gad.fNoBwMap = (unsigned char)(_gad.fNoBwMap || bitPixel > 2);

            // CALLOC will set unused colors to <0,0,0,0>
            for (i = 0; i < (ULONG)bitPixel; ++i)
            {
                argbLocal[i].rgbRed = localColorMap[CM_RED][i];
                argbLocal[i].rgbGreen = localColorMap[CM_GREEN][i];
                argbLocal[i].rgbBlue = localColorMap[CM_BLUE][i];
            }

            crgbColors = bitPixel;
            prgbColors = argbLocal;
        }
        else
        {
            crgbColors = GifScreen.BitPixel;
            prgbColors = m_argbPalette;

            if (GifScreen.BitPixel > 2)
                _gad.fNoBwMap = TRUE;
        }

        // Get this in here so that GifStrectchDIBits can use it during progressive
        // rendering.
        if (_gad.pgf == NULL)
        {
            _gad.pgf = pgfNew;

            // Remember the color table for the first frame so that
            // progressive dithering can be done on it.

            memcpy(_argbFirst, prgbColors, crgbColors * sizeof(RGBQUAD));
        }

        hr = ReadImage(LM_to_uint(buf[4], buf[5]), // width
                       LM_to_uint(buf[6], buf[7]), // height
                       BitSet(buf[8], INTERLACE),
                       imageCount != imageNumber,
                       crgbColors, prgbColors);

        if ( SUCCEEDED(hr) )
        {
            // Oh JOY of JOYS! We got the pixels!
            if (pgfLast != NULL)
            {
                int transparent = (pgfNew->bTransFlags & TRANSF_TRANSPARENT) ? (int) pgfNew->bTransIndex : -1;

                _gad.fAnimated = TRUE; // say multi-image == animated

                pgfNew->hbmDib = NULL;

                pgfLast->pgfNext = pgfNew;

            }
            else
            { // first frame
                _gad.pgf = pgfNew;

                Assert(_gad.pgf->hbmDib == hbmDib);

                // set up a temporary animation state for use in progressive draw
                _gas.fAnimating = TRUE;
                _gas.dwLoopIter = 0;
                _gas.pgfDraw = pgfNew;
                _gad.dwLastProgTimeMS = 0;
                _gad.pgfLastProg = pgfNew;
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
    if (FAILED(hr) || (pgfNew && _gad.pgf != pgfNew))
    {
        MemFree(pgfNew);
        return hr;
    }
    
    return S_OK;
}

long CGIFFilter::ReadColorMap(long number, unsigned char buffer[3][MAXCOLORMAPSIZE])
{
    long i;
    unsigned char rgb[3];

    for (i = 0; i < number; ++i)
    {
        if (!Read(rgb, sizeof(rgb)))
        {
            XX_DMsg(DBG_IMAGE, ("bad colormap\n"));
            return (TRUE);
        }
        buffer[CM_RED][i] = rgb[0];
        buffer[CM_GREEN][i] = rgb[1];
        buffer[CM_BLUE][i] = rgb[2];
    }
    return FALSE;
}

long CGIFFilter::DoExtension(long label)
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


long CGIFFilter::GetDataBlock(unsigned char *buf)
{
    unsigned char count;

    count = 0;

    if (!Read(&count, 1))
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

BOOL CGIFFilter::initLWZ(long input_code_size)
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

    _gifinfo.pstack = (unsigned short *)MemAlloc((_gifinfo.stacksize)*sizeof(unsigned short));
    if(_gifinfo.pstack == 0){
        goto ErrorExit;
    }
    _gifinfo.sp = _gifinfo.pstack;

    // Initialize the two tables.
    _gifinfo.tablesize = (_gifinfo.max_code_size);
    _gifinfo.table[0] = (unsigned short *)MemAlloc((_gifinfo.tablesize)*sizeof(unsigned short));
    _gifinfo.table[1] = (unsigned short *)MemAlloc((_gifinfo.tablesize)*sizeof(unsigned short));
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

long CGIFFilter::nextCode(long code_size)
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
CGIFFilter::growStack()
{
    long index;
    unsigned short *lp;

    if (_gifinfo.stacksize >= MAX_STACK_SIZE)
        return 0;

    index = (_gifinfo.sp - _gifinfo.pstack);
    if (MemRealloc((void **)&_gifinfo.pstack, (_gifinfo.stacksize)*2*sizeof(unsigned short)))
        return 0;

    _gifinfo.sp = &(_gifinfo.pstack[index]);
    _gifinfo.stacksize = (_gifinfo.stacksize)*2;
    lp = &(_gifinfo.pstack[_gifinfo.stacksize]);
    return lp;
}

BOOL
CGIFFilter::growTables()
{
    if (MemRealloc((void **)&_gifinfo.table[0], (_gifinfo.max_code_size)*sizeof(unsigned short)))
        return FALSE;

    if (MemRealloc((void **)&_gifinfo.table[1], (_gifinfo.max_code_size)*sizeof(unsigned short)))
        return FALSE;

    return TRUE;
}

inline
long CGIFFilter::readLWZ()
{
    return((_gifinfo.sp > _gifinfo.pstack) ? *--(_gifinfo.sp) : nextLWZ());
}

#define CODE_MASK 0xffff

long CGIFFilter::nextLWZ()
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
            table0[code] = (unsigned short)((_gifinfo.oldcode) & CODE_MASK);
            table1[code] = (unsigned short)((_gifinfo.firstcode) & CODE_MASK);
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
CGIFFilter::OnProg(BOOL fLast)
{
    CalculateUpdateRect(_yLogRow - PROG_INTERVAL + 1, _yLogRow, &_fInvalidateAll, &_yBottom);

#if 0
    if (fLast || (GetTickCount() - _pImgLoadCtx->_dwTickProg > 250))
    {
        ProgressiveDithering(_yLogRowDraw, _yLogRow);
        _yLogRowDraw = _yLogRow;
    }
#endif

   FireOnProgressEvent(fLast);
}

HRESULT
CGIFFilter::ReadImage(long len, long height, BOOL fInterlace, BOOL fGIFFrame,
    int crgbColors, RGBQUAD * prgbColors)
{
    unsigned char *dp, c;
    long v;
    long xpos = 0, ypos = 0, pass = 0;
    long padlen = ((len + 3) / 4) * 4;
    char buf[256]; // need a buffer to read trailing blocks ( up to terminator ) into
    BYTE * pbBits;
    RECT rect;
    HRESULT hResult;
    LONG nPitch;

	// unused parameters
	prgbColors;
	crgbColors;
	
    /*
       **  Initialize the Compression routines
     */
    if (!Read(&c, 1))
    {
        goto abort;
    }

    if (FAILED(FireGetSurfaceEvent()))
    {
        goto abort;
    }

    _cbImage = padlen * height * sizeof(char);

    if (_cbImage > dwMaxGIFBits)
       goto abort;

#if 0
    if (!fGIFFrame)
    {
        _hbmDib = hbmDib;
        _gad.pgf->hbmDib = hbmDib;
        _pbDst = pbBits;

        if (fInterlace && _colorMode == 8)
        {
            _pbSrcAlloc = (BYTE *)MemAlloc(_cbImage);
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

    if (_colorMode == 8)
    {
        if (_pImgLoadCtx->_wDownloadFlags & DWN_FORCEDITHER)
            _fDither = TRUE;
        else
        {
            int nDifferent;
        
            nDifferent = x_ComputeConstrainMap(cpeColors, ppeColors,
                (_gad.pgf->bTransFlags & TRANSF_TRANSPARENT) ? 
                    _gad.pgf->bTransIndex : -1, 
                _mapConstrained);
            if (cpeColors > 16 && nDifferent > 15)
                _fDither = TRUE;
            else
                TraceTag((tagImgFiltGif, "No dithering needed for '%ls'", GetUrl()));
        }                
    }
#endif

    if (c == 1)
    {
        // Netscape seems to field these bogus GIFs by filling treating them
        // as transparent. While not the optimal way to simulate this effect,
        // we'll fake it by pushing the initial code size up to a safe value,
        // consuming the input, and returning a buffer full of the transparent
        // color or zero, if no transparency is indicated.
        if (initLWZ(MINIMUM_CODE_SIZE))
            while (readLWZ() >= 0);

		rect.left = rect.top = 0;
		rect.right = len;
		rect.bottom = height;

		hResult = LockBits( &rect, SURFACE_LOCK_EXCLUSIVE, (void **)&pbBits,
                  		&nPitch );

        if (_gifinfo.Gif89.transparent != -1)
            FillMemory(pbBits, _cbImage, _gifinfo.Gif89.transparent);
        else // fall back on the background color
            FillMemory(pbBits, _cbImage, 0);

		UnlockBits( &rect, pbBits );
		
        if (!fGIFFrame)
        {
            _yLogRow = height - 1;
            OnProg(TRUE);
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

        rect.left = 0;
        rect.right = m_nWidth;
        for (i = 0; i < height; i++)
        {
//          XX_DMsg(DBG_IMAGE, ("readimage, logical=%d, offset=%d\n", i, padlen * ((height-1) - ypos)));
            rect.top = ypos;
            rect.bottom = rect.top + 1;
            hResult = LockBits( &rect, SURFACE_LOCK_EXCLUSIVE, (void **)&pbBits,
                        &nPitch );
            dp = pbBits;
            for (xpos = 0; xpos < len; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                *dp++ = (unsigned char) v;
            }
            ypos += step;
            while (ypos >= height)
            {
                if (pass++ > 0)
                    step /= 2;
                ypos = step / 2;
            }
            if (!fGIFFrame)
            {
                _yLogRow = i;

                if ((i & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE);
                }
            }

            UnlockBits(&rect, pbBits);
        }

        if (!fGIFFrame)
        {
            OnProg(TRUE);
        }
    }
    else
    {
        rect.left = 0;
        rect.right = m_nWidth;
        
        for (ypos = 0; ypos < height; ++ypos)
        {
            rect.top = ypos;
            rect.bottom = rect.top + 1;
            hResult = LockBits( &rect, SURFACE_LOCK_EXCLUSIVE, (void **)&pbBits,
                        &nPitch );
            dp = pbBits;
            for (xpos = 0; xpos < len; xpos++)
            {
                if ((v = readLWZ()) < 0)
                    goto abort;

                *dp++ = (unsigned char) v;
            }
            if (!fGIFFrame)
            {
                _yLogRow++;
//              XX_DMsg(DBG_IMAGE, ("readimage, logical=%d, offset=%d\n", _yLogRow, padlen * ypos));
                if ((_yLogRow & PROG_INTERVAL) == 0)
                {
                    // Post ProgDraw (IE code has delay-logic)
                    OnProg(FALSE);
                }
            }
            
            UnlockBits(&rect, pbBits);
        }

        if (!fGIFFrame)
        {
            OnProg(TRUE);
        }
    }

    // consume blocks up to image block terminator so we can proceed to the next image
    while (GetDataBlock((unsigned char *) buf) > 0)
                ;

done:

    return S_OK;

abort:
    return E_FAIL;
}


void
CGIFFilter::ProgressiveDithering(int logicalFill, int logicalRow)
{
    BOOL bitbltNeeded = TRUE;
    int i;
    // Note: We only show the primary frame during prog draw, so we use the first GIFFRAME
    int padXSize;
    int err = 0;
    int row = logicalRow;
    int pass;
    int band;
    int band2;
    int offset;
    int passFill;
    int rowFill;
    int bandFill;
    int step;
    int j;

    if (logicalFill < 0)
        logicalFill = 0;

    padXSize = ((_gad.pgf->width + 3) / 4) * 4;

    if (_fInterleaved)
    {
        getPassInfo(logicalFill,_gad.pgf->height,&passFill,&rowFill,&bandFill);
        getPassInfo(logicalRow,_gad.pgf->height,&pass,&row,&band);
        step = passFill == 0 ? 8 : bandFill*2;
        for (i = logicalFill; i <= logicalRow;i++)
        {
            offset = padXSize*(_gad.pgf->height - rowFill - 1) ;    /* the DIB is stored upside down */

            band2 = rowFill <= row ? band : band*2;
            if (band2 != 1)
            {
                memcpy(_pbDst+offset,_pbSrc+offset,padXSize);
                if (rowFill+band2 > _gad.pgf->height)
                    band2 = _gad.pgf->height-rowFill;

                for (j = 1; j < band2; j++)
                    memcpy(_pbDst+(offset-j*padXSize),_pbDst+offset,padXSize);
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

    }
}

void
CGIFFilter::CalculateUpdateRect(int logicalRow0, int logicalRowN, BOOL *pfInvalidateAll, LONG *pyBottom)
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

        getPassInfo(logicalRow0, m_nHeight, &pass0, &row0, &band0);
        getPassInfo(logicalRowN, m_nHeight, &passN, &rowN, &bandN);

        if (passN > pass0 + 1)
            *pfInvalidateAll = TRUE;

        *pyBottom = rowN + bandN;

        // We must special case the last row of last pass to deal with dithering
        // the possibly even numbered last row
        if (logicalRowN == (int)(m_nHeight - 1))
            *pyBottom = m_nHeight;
    }
    else
        *pyBottom = logicalRowN + 1;
}

/////////////////////////////////////////////////////////////////////////////
//

CGIFFilter::CGIFFilter()
{
}

CGIFFilter::~CGIFFilter()
{
}

HRESULT CGIFFilter::FireGetSurfaceEvent()
{
   CComPtr< IRGBColorTable > pColorTable;
   CComPtr< IUnknown > pSurface;
   HRESULT hResult;

    hResult = m_pEventSink->GetSurface(m_nWidth, m_nHeight, BFID_INDEXED_RGB_8, 
                        0, IMGDECODE_HINT_TOPDOWN|IMGDECODE_HINT_FULLWIDTH, 
                        &pSurface);

    if (FAILED(hResult))
        return hResult;

    if (m_dwEvents & IMGDECODE_EVENT_USEDDRAW)
        pSurface->QueryInterface(IID_IDirectDrawSurface, 
                                 (void **)&m_pDDrawSurface);

    if (m_pDDrawSurface)
    {
        LPDIRECTDRAWPALETTE pDDPalette;
        PALETTEENTRY        ape[256];

		hResult = m_pDDrawSurface->GetPalette(&pDDPalette);
		if (SUCCEEDED(hResult))
		{
            CopyPaletteEntriesFromColors(ape, m_argbPalette, m_nColors);
		    pDDPalette->SetEntries(0, 0, m_nColors, ape);
		    pDDPalette->Release();
		}
    }
    else
    {
        hResult = pSurface->QueryInterface(IID_IBitmapSurface, 
                                           (void **)&m_pBitmapSurface);
        if( FAILED( hResult ) )
        {
            return( hResult );
        }
    
        hResult = m_pBitmapSurface->QueryInterface( IID_IRGBColorTable, 
                        (void**)&pColorTable );
        if( FAILED( hResult ) )
        {
            return( hResult );
        }

        hResult = pColorTable->SetColors( 0, m_nColors, m_argbPalette );
        if( FAILED( hResult ) )
        {
            return( hResult );
        }
    }

    if( m_dwEvents & IMGDECODE_EVENT_PALETTE )
    {
        hResult = m_pEventSink->OnPalette();
        if( FAILED( hResult ) )
        {
            return( hResult );
        }
    }

   return( S_OK );
}

HRESULT CGIFFilter::FireOnBeginDecodeEvent()
{
   HRESULT hResult;
   BOOL bFound;
   ULONG iFormat;
   GUID* pFormats;
   ULONG nFormats;

   hResult = m_pEventSink->OnBeginDecode( &m_dwEvents, &nFormats, &pFormats );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   bFound = FALSE;
   for( iFormat = 0; (iFormat < nFormats) && !bFound; iFormat++ )
   {
      if( IsEqualGUID( pFormats[iFormat], BFID_INDEXED_RGB_8 ) )
      {
         bFound = TRUE;
      }
   }
   CoTaskMemFree( pFormats );
   if( !bFound )
   {
      return( E_FAIL );
   }

   return( S_OK );
}

HRESULT CGIFFilter::FireOnBitsCompleteEvent()
{
   HRESULT hResult;

   if( !(m_dwEvents & IMGDECODE_EVENT_BITSCOMPLETE) )
   {
      return( S_OK );
   }

   hResult = m_pEventSink->OnBitsComplete();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

HRESULT CGIFFilter::FireOnProgressEvent(BOOL fLast)
{
   HRESULT hResult;
   RECT rect;

   if( !(m_dwEvents & IMGDECODE_EVENT_PROGRESS) )
   {
      return( S_OK );
   }

   rect.left = 0;
   rect.top = 0;
   rect.right = m_nWidth;
   rect.bottom = _yBottom;

   hResult = m_pEventSink->OnProgress( &rect, fLast );
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   return( S_OK );
}

STDMETHODIMP CGIFFilter::Initialize( IImageDecodeEventSink* pEventSink )
{
   if( pEventSink == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pEventSink = pEventSink;

    _yLogRow = -1;
    _yLogRowDraw = -1;

   _fMustBeTransparent = 0;
   _fInvalidateAll = 0;
   _fDither = 0;
   _cbImage = 0;
   _fInterleaved = 0;
   _fProgAni = 0;
   _yLogRow = 0;
   _yLogRowDraw = 0;
   _yBottom = 0;
   _ySrcBot = 0;
   _yDithRow = 0;
   _pvDithData = NULL;
   _lTrans = -1;
   _pbSrcAlloc = NULL;
   _pbSrc = NULL;
   _pbDst = NULL;
   
   ZeroMemory(&_gifinfo, sizeof(_gifinfo));
   ZeroMemory(&_gas, sizeof(_gas));
   ZeroMemory(&_gad, sizeof(_gad));
   ZeroMemory(_argbFirst, sizeof(_argbFirst));
   ZeroMemory(_mapConstrained, sizeof(_mapConstrained));

   return( S_OK );
}

STDMETHODIMP CGIFFilter::Process( IStream* pStream )
{
   HRESULT hResult;

   if( pStream == NULL )
   {
      return( E_INVALIDARG );
   }

   m_pStream = pStream;

   // KENSY: What do I do about the color table?
	// KENSY: scale according to DPI of screen

   hResult = FireOnBeginDecodeEvent();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

    hResult = ReadGIFMaster();
    if( FAILED( hResult ) )
    {
        return( hResult );
    }
    

    if (_gad.pgf && _yLogRowDraw >= 0)
    {
        if (_yLogRowDraw + 1 >= _gad.pgf->height)
        {
            _ySrcBot = -1;
        }
        else if (_fInterleaved)
        {
            _ySrcBot = min(_yLogRowDraw * 8, _gad.pgf->height);

            if (_ySrcBot == _gad.pgf->height)
                _ySrcBot = -1;
        }
        else if (_yLogRowDraw > 31)
        {
            _ySrcBot = _yLogRowDraw + 1;
        }
    }


	if (_lTrans != -1)
	{
	    if (m_pDDrawSurface)
	    {
	        DDCOLORKEY  ddKey;

            // Set the transparent index
            ddKey.dwColorSpaceLowValue = _lTrans;
            ddKey.dwColorSpaceHighValue = _lTrans;
            hResult = m_pDDrawSurface->SetColorKey(DDCKEY_SRCBLT, &ddKey);
	    }
	    else
	    {
  		    CComPtr< IRGBColorTable > pColorTable;
	
   		    hResult = m_pBitmapSurface->QueryInterface( IID_IRGBColorTable, 
      					    (void**)&pColorTable );
   		    if( SUCCEEDED( hResult ) )
   		    {
   			    pColorTable->SetTransparentIndex(_lTrans);
   		    }
	    }
	}

   hResult = FireOnBitsCompleteEvent();
   if( FAILED( hResult ) )
   {
      return( hResult );
   }

   m_pStream = NULL;

   return( S_OK );
}

STDMETHODIMP CGIFFilter::Terminate( HRESULT hrStatus )
{
   ATLTRACE( "Image decode terminated.  Status: %x\n", hrStatus );

   if( m_pBitmapSurface != NULL )
   {
      m_pBitmapSurface.Release();
   }

    if (m_pDDrawSurface != NULL)
    {
        m_pDDrawSurface.Release();
    }
        
   if( m_pEventSink != NULL )
   {
      m_pEventSink->OnDecodeComplete( hrStatus );
      m_pEventSink.Release();
   }

    MemFree(_pbSrcAlloc);
    MemFree(_pvDithData);
    MemFree(_gifinfo.pstack);
    MemFree(_gifinfo.table[0]);
    MemFree(_gifinfo.table[1]);

   return( S_OK );
}

HRESULT CGIFFilter::LockBits(RECT *prcBounds, DWORD dwLockFlags, void **ppBits, long *pPitch)
{
    HRESULT hResult;
    
    if (m_pDDrawSurface)
    {
        DDSURFACEDESC   ddsd;

        ddsd.dwSize = sizeof(ddsd);
        hResult = m_pDDrawSurface->Lock(prcBounds, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        if (FAILED(hResult))
            return hResult;

        *ppBits = ddsd.lpSurface;
        *pPitch = ddsd.lPitch;

        return S_OK;
    }
    else
    {
        return m_pBitmapSurface->LockBits(prcBounds, dwLockFlags, ppBits, pPitch);
    }
}

HRESULT CGIFFilter::UnlockBits(RECT *prcBounds, void *pBits)
{
    if (m_pDDrawSurface)
    {
        return m_pDDrawSurface->Unlock(pBits);
    }
    else
    {
        return m_pBitmapSurface->UnlockBits(prcBounds, pBits);
    }
}


