#include "headers.hxx"
#include "img.hxx"

#if 0

extern "C" {
#include "pic.h"
#include "errors.h"
}

typedef struct tagBITMAPINFO_256 {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD				bmiColors[256];
} BITMAPINFO_256;

class CImgFiltPeg : public CImgFilt
{
public:
    typedef CImgFilt super;

    CImgFiltPeg(CRITICAL_SECTION * pcs) : CImgFilt(pcs) {}

    // CImgFilt methods

    virtual BITMAPINFO * Decode();

    // CImgFiltPeg methods

	void GetColors();
    BITMAPINFO * FinishDithering();
	void ReadImage();
	void MakePalette();
	
    virtual void VStretchDIBits(
        HDC  hdc,           // handle of device context
        int  XDest,         // x-coordinate of upper-left corner of dest. rect.
        int  YDest,         // y-coordinate of upper-left corner of dest. rect.
        int  nDestWidth,    // width of destination rectangle
        int  nDestHeight,   // height of destination rectangle
        int  XSrc,          // x-coordinate of upper-left corner of source rect.
        int  YSrc,          // y-coordinate of upper-left corner of source rect.
        int  nSrcWidth,     // width of source rectangle
        int  nSrcHeight,    // height of source rectangle
        UINT  iUsage,       // usage
        DWORD  dwRop        // raster operation code
        );

    // Data members

    UINT _nColors;
    UINT _nPasses;

	BITMAPINFO *_pbmi;

	PIC_PARM pic;
};

void CImgFiltPeg::GetColors()
{
		_nColors = g_cHalftoneEntries;
		memcpy(_ape, g_peHalftone, sizeof(_ape));
}

#define LINEBYTES(_wid,_bits) ((((_wid)*(_bits) + 31) / 32) * 4)

void CImgFiltPeg::ReadImage()
{
	// assume the worst for now...
	_pbBits = NULL;
}

BITMAPINFO * CImgFiltPeg::Decode()
{
	BYTE byBuf[4096];
	ULONG ulSize, cbRead, ulLine;
	int i;
	
	// setup to read JPEG
	memset(&pic, 0, sizeof(pic));
	pic.ParmSize = sizeof(pic);
	pic.ParmVer = CURRENT_PARMVER;
	pic.ParmVerMinor = 1;

	// fill in Get queue

	pic.Get.Start = pic.Get.Front = byBuf;
	pic.Get.End = pic.Get.Rear = byBuf + sizeof(byBuf);
	if (!Read((unsigned char *)&byBuf, sizeof(byBuf), &cbRead))
		return NULL;
	
	pic.u.QRY.BitFlagsReq = QBIT_BIWIDTH | QBIT_BIHEIGHT;
	
	if (!PegasusQuery(&pic)) 
		return NULL;

	_xWidth = pic.Head.biWidth;
	_yHeight = abs(pic.Head.biHeight);
	_lTrans = -1;

    // Post WHKNOWN
    OnSize(_xWidth, _yHeight, -1 /* lTrans */);

    // Do the color stuff
	GetColors();
	MakePalette();

	// allocate space for the bitmap
	ulLine = LINEBYTES(_xWidth, _pbmi->bmiHeader.biBitCount);
	ulSize = ulLine * _yHeight;
	_pbBits = (BYTE *)MemAlloc(ulSize);
	if (!_pbBits)
		return NULL;

	// setup for the decode phase
	memset(&pic, 0, sizeof(pic));
	pic.ParmSize = sizeof(pic);
	pic.ParmVer = CURRENT_PARMVER;
	pic.ParmVerMinor = 1;
	pic.Get.Start = pic.Get.Front = byBuf;
	pic.Get.End = pic.Get.Rear = byBuf + cbRead;
	pic.Put.QFlags = Q_REVERSE;
	pic.Put.Start = _pbBits;
	pic.Put.End = _pbBits + ulSize;
	pic.Put.Front = pic.Put.Rear = pic.Put.End - 1;
	pic.Op = OP_EXPJ;
	pic.u.S2D.DibSize = _pbmi->bmiHeader.biBitCount;
    pic.u.S2D.PicFlags = PF_Dither | PF_NoCrossBlockSmoothing;
//    pic.u.S2D.PicFlags = PF_Dither;
	pic.Head.biClrUsed = _nColors;
	pic.Head.biClrImportant = 0;
	for (i = 0; i < (int)_nColors; ++i) {
		pic.ColorTable[i].rgbRed = _ape[i].peRed;
		pic.ColorTable[i].rgbGreen = _ape[i].peGreen;
		pic.ColorTable[i].rgbBlue = _ape[i].peBlue;
		pic.ColorTable[i].rgbReserved = 0;
	}
	
	// do the decode loop
	RESPONSE Response;
	LONG lStatus = ERR_NONE;
	
	Response = Pegasus(&pic, REQ_INIT);
	if (Response != RES_DONE) {
		DebugBreak();
		MemFree(_pbBits);
		_pbBits = NULL;
		return NULL;
	}

	Response = Pegasus(&pic, REQ_EXEC);

	while ((lStatus == ERR_NONE) && (Response != RES_DONE))
	{
		switch (Response) 
		{
			case RES_ERR:
				lStatus = pic.Status;
				break;

			case RES_GET_NEED_DATA:
				if (pic.Get.Rear == pic.Get.End
					&& pic.Get.Front > pic.Get.Start) 
				{
					if (!Read((unsigned char *)pic.Get.Start,
								pic.Get.Front - pic.Get.Start - 1)) 
						lStatus = ERR_BAD_READ;

					pic.Get.Rear = pic.Get.Front - 1;
				} 
				else if (pic.Get.Rear < pic.Get.End)
				{
					if (!Read((unsigned char *)pic.Get.Rear,
						pic.Get.End - pic.Get.Rear))
						lStatus = ERR_BAD_READ;

					pic.Get.Rear = pic.Get.End;

					if (pic.Get.Start != pic.Get.Front)
					{
						if (!Read((unsigned char *)pic.Get.Start,
								pic.Get.Front - pic.Get.Start - 1))
							lStatus = ERR_BAD_READ;

						pic.Get.Rear = pic.Get.Front - 1;
					}
				}
				break;
				
			default:
				DebugBreak();
				break;
		}

		// send a progressive render message
		_yBottom = (pic.Put.End - pic.Put.Rear) / ulLine;
		OnProg(FALSE, IMGBITS_PARTIAL);

		if (lStatus == ERR_NONE)
			Response = Pegasus(&pic, REQ_CONT);
	}


	if (lStatus != ERR_NONE) 
	{
		MemFree(_pbBits);
		_pbBits = NULL;
		return NULL;
	}

	OnProg(TRUE, IMGBITS_TOTAL);

	// return the info ptr
    return _pbmi;
}

CImgFilt * ImgFiltCreatePeg(CRITICAL_SECTION * pcs)
{
    return(new CImgFiltPeg(pcs));
}

BITMAPINFO *
CImgFiltPeg::FinishDithering()
{
#if 0
    if (GetColorMode() == 8)
    {
        if (x_Dither(_pbBits, _ape, _xWidth, _yHeight, _lTrans))
            return NULL;
    }
#endif
    return _pbmi;
}

void
CImgFiltPeg::MakePalette()
{
    if (GetColorMode() == 8)
    {
    	_pbmi = BIT_Make_DIB_PAL_Header(_xWidth, _yHeight);
    }
    else
    {
        if (GetColorMode() == 4)
        {
            _pbmi = BIT_Make_DIB_RGB_Header_VGA(_xWidth, _yHeight);
        }
        else
        {
        /* true color display */
            _pbmi = BIT_Make_DIB_RGB_Header_24BIT(_xWidth, _yHeight);
        }
    }
}

void
CImgFiltPeg::VStretchDIBits(
    HDC  hdc,   // handle of device context
    int  XDest, // x-coordinate of upper-left corner of dest. rect.
    int  YDest, // y-coordinate of upper-left corner of dest. rect.
    int  nDestWidth,    // width of destination rectangle
    int  nDestHeight,   // height of destination rectangle
    int  XSrc,  // x-coordinate of upper-left corner of source rect.
    int  YSrc,  // y-coordinate of upper-left corner of source rect.
    int  nSrcWidth, // width of source rectangle
    int  nSrcHeight,    // height of source rectangle
    UINT  iUsage,   // usage
    DWORD  dwRop    // raster operation code
   )
{
    if ((nSrcWidth == 0) || (nSrcHeight == 0))
        return;

    ImgBlt(hdc, XDest, YDest, nDestWidth, nDestHeight, 0, 0,
        _xWidth, _yHeight, _pbBits, _pbmi, NULL, -1, iUsage, dwRop, FALSE);
}

#endif
