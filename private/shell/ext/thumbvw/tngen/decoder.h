#ifndef _DECODER_H_

#define _DECODER_H_

struct FILTERINFO
{
    int            _colorMode;
    LONG        _lTrans;
    ULONG        m_nBitsPerPixel;
    LONG        _xWidth;
    LONG        _yHeight;
    LONG        m_nPitch;
    HBITMAP        _hbmDib;
    HPALETTE    hpal;
    BYTE *        m_pbBits;
    BYTE *        m_pbFirstScanLine;
    DWORD        dwEvents;
    IDirectDrawSurface * m_pDDrawSurface;
};



#endif