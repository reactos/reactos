//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       earea.hxx
//
//  Contents:   Definition of the Area Element class
//
//  Classes:    EAreaElement
//
//----------------------------------------------------------------------------

#ifndef I_EAREA_HXX_
#define I_EAREA_HXX_
#pragma INCMSG("--- Beg 'earea.hxx'")

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_CSIMUTIL_HXX_
#define X_CSIMUTIL_HXX_
#include "csimutil.hxx"
#endif

#define _hxx_
#include "area.hdl"

MtExtern(CAreaElement)
MtExtern(CImgAreaStub)

class CMapElement;
class CImgAreaStub;

class CAreaElement : public CHyperlink
{
    DECLARE_CLASS_TYPES(CAreaElement, CHyperlink)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAreaElement))

    // Creation and Initialization
    CAreaElement(CDoc *pDoc) :
        super(ETAG_AREA, pDoc) 
    {
#ifdef WIN16
	    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }

    static HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);
    virtual HRESULT Init2(CInit2Context * pContext);

    // Destructor
    void Passivate();

    virtual void    Notify(CNotification *pNF);

    virtual HRESULT ClickAction (CMessage *pmsg);

    // Drawing related
    HRESULT Draw(CFormDrawInfo * pDI, CElement * pImg);

    // Bounding rectangle
    void GetBoundingRect(RECT *prc);

    // Modification/Update
    HRESULT UpdatePolygon();                        // Updates polygon region
    HRESULT UpdateRectangle();                      // Corrects rectangle
    HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags);

    // Misc
    HRESULT ParseCoords();
    NV_DECLARE_PROPERTY_METHOD(GetcoordsHelper, GETcoordsHelper, (CStr *pstrCOORDS));
    NV_DECLARE_PROPERTY_METHOD(SetcoordsHelper, SETcoordsHelper, (CStr *pstrCOORDS));
    NV_DECLARE_PROPERTY_METHOD(GetshapeHelper, GETshapeHelper, (CStr *pstrSHAPE));
    NV_DECLARE_PROPERTY_METHOD(SetshapeHelper, SETshapeHelper, (CStr *pstrSHAPE));

    // URL accessors - CHyperlink overrides
    virtual HRESULT SetUrl(BSTR bstrUrl);
    virtual LPCTSTR GetUrl() const;
    virtual LPCTSTR GetTarget() const;
    virtual HRESULT GetUrlTitle(CStr *pstr);

    // Helpers for adding/removing areas through automation
    HRESULT InsertIntoElemTree(CMapElement * pMap, long lItemIndex);
    HRESULT RemoveFromElemTree();

    // Stub helper
    CImgAreaStub* FindFirstStub();

    static const CLSID *            s_apclsidPages[];

    
    #define _CAreaElement_
    #include "area.hdl"

    // interface baseimplementation property prototypes
    // DECLARE_TEAROFF_METHOD(put_href, PUT_href, (BSTR v));
    // DECLARE_TEAROFF_METHOD(get_href, GET_href, (BSTR * p));


protected:
    DECLARE_CLASSDESC_MEMBERS;


private:
    NO_COPY(CAreaElement);


public:
    union CoordinateUnion _coords;

    CPointAry       _ptList;

    unsigned _nShapeType:2;
    unsigned _fShapeSet:1;

private:
    CStr     _strCoords;

};

class CImgElement;


#pragma INCMSG("--- End 'earea.hxx'")
#else
#pragma INCMSG("*** Dup 'earea.hxx'")
#endif
