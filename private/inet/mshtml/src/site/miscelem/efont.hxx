//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       efont.hxx
//
//  Contents:   CFontElement class
//
//----------------------------------------------------------------------------

#ifndef I_EFONT_HXX_
#define I_EFONT_HXX_
#pragma INCMSG("--- Beg 'efont.hxx'")

#define _hxx_
#include "font.hdl"

#define _hxx_
#include "basefont.hdl"

MtExtern(CFontElement)
MtExtern(CBaseFontElement)

//+---------------------------------------------------------------------------
//
// CFontElement
//
//----------------------------------------------------------------------------

class CFontElement : public CElement
{
    DECLARE_CLASS_TYPES(CFontElement, CElement)
    
public:
    
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFontElement))

    enum { MIN_FONT_SIZE=-7, 
           MAX_FONT_SIZE=7 };

    CFontElement(CDoc *pDoc)
      : CElement(ETAG_FONT, pDoc) {}

    ~CFontElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    HRESULT ApplyDefaultFormat(CFormatInfo * pCFI);
    HRESULT CombineAttributes( CFontElement * pElement );
    HRESULT RemoveAttributes( CFontElement * pElement, BOOL * pfAttrBagEmpty );
    

    #define _CFontElement_
    #include "font.hdl"

    CUnitValue GetFontSize( void);
    
protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CFontElement);
};


//+---------------------------------------------------------------------------
//
// CBaseFontElement
//
//----------------------------------------------------------------------------

class CBaseFontElement : public CElement
{
    DECLARE_CLASS_TYPES(CBaseFontElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBaseFontElement))

    enum { MIN_BASEFONT=1, 
           MAX_BASEFONT=7 };

    CBaseFontElement(CDoc *pDoc)
      : CElement(ETAG_BASEFONT, pDoc) {}

    ~CBaseFontElement() {}

    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);


    #define _CBaseFontElement_
    #include "basefont.hdl"

    long GetFontSize ( void );

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CBaseFontElement);
};


#pragma INCMSG("--- End 'efont.hxx'")
#else
#pragma INCMSG("*** Dup 'efont.hxx'")
#endif
