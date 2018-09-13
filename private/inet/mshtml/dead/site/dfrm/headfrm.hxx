
//+---------------------------------------------------------------------------
//
//  Maintained by: frankman
//
//  Copyright: (C) Microsoft Corporation, 1994-1995.
//
//  File:       headfrm.hxx
//
//  Contents:   this file contains the CHeaderFrame class definition.
//
//----------------------------------------------------------------------------

#ifndef _HEADERFRM_HXX_
#   define _HEADERFRM_HXX_ 1

#   ifndef _BASEFRM_HXX_
#       include "detail.hxx"
#   endif

#include    "propchg.hxx"



//
//  forward declaration
//

class CHeaderFrameTemplate;

//+---------------------------------------------------------------------------
//
//  Class:      CHeaderFrame
//
//  Purpose:    Frame class implementing header/footer behaviour
//              this class is comparable to a detail in use and purpose
//
//----------------------------------------------------------------------------
class CHeaderFrame: public CDetailFrame
{
typedef CDetailFrame super;

public:
    //+-----------------------------------------------------------------------
    //  Template construction
    //------------------------------------------------------------------------

    // Template constructor.
    CHeaderFrame(CDoc * pDoc, CSite * pParent) : super(pDoc, pParent) {_fPaintBackground=TRUE;};

    // allocate size and memcpy it from original to create clone
    // operator new for the instance construction.
    void * operator new(size_t cb) { return MemAllocClear(cb); }

    inline CHeaderFrameTemplate * getTemplate()
        {
            return (CHeaderFrameTemplate *) _pTemplate;
        }


    virtual HRESULT Notify(SITE_NOTIFICATION, DWORD);


    // We have to override pure virtual for PULL model creation.
    HRESULT CreateInstance (CDoc * pDoc,
                            CSite * pParent,
                            CSite **ppFrame,
                            CCreateInfo * pcinfo);

    virtual void OptimizeBackgroundPainting(void) { _fPaintBackground = TRUE; }

    // header needs to overwrite draw for borders in listbox/combobox cases
    virtual HRESULT Draw(CFormDrawInfo *pDI);
    virtual void PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo);

    //  HeaderFrame property overrides

    //  Return the qualifier / bookmark of the layout's row
    virtual HRESULT GetQualifier (QUALIFIER * q);
    virtual BOOL IsVisible(void);      // TRUE if this control is visible

    virtual HRESULT ProposedDelta(CRectl *rclDelta);

    #ifdef PRODUCT_97
    virtual HRESULT CalcControlPositions(DWORD dwFlags);
    virtual HRESULT ProposedFriendFrames(void);
    #endif

    HRESULT RemoveRelated(CSite *pSite, DWORD dwFlags);
    HRESULT DeleteSite(CSite * pSite, DWORD dwFlags);

    // Core methods override
    virtual void GetClientRectl(RECTL *prcl);

protected:

    //+-----------------------------------------------------------------------
    //  (Protected) Instance creation API
    //------------------------------------------------------------------------

    // Instance constructor.
    CHeaderFrame(CDoc * pDoc, CSite * pParent, CHeaderFrame * pTemplate);

    // operator new for the template construction.
    void * operator new (size_t s, CHeaderFrame * pOriginal);


};
//---- end of class declaration--------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Class:      CHeaderFrameTemplate
//
//  Purpose:    Frame class implementing header/footer behaviour
//              this class is comparable to a detail in use and purpose
//
//----------------------------------------------------------------------------
class CHeaderFrameTemplate : public CHeaderFrame
{
typedef CHeaderFrame super;
friend class CHeaderFrame;

public:
    CHeaderFrameTemplate(CDoc * pDoc, CSite * pParent, BOOL fFooter=FALSE) : super(pDoc, pParent) {_fIsFooter = fFooter;};

    virtual CSite::CTBag * GetTBag() { return &_TBag; }

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}
    void    SetFooter(BOOL flag) { _fIsFooter = flag; };
    BOOL    IsFooter(void) { return _fIsFooter; };

protected:
    // DATA
    CTBag _TBag;

    static PROP_DESC    s_apropdesc[];
    static CLASSDESC   s_classdesc;

};
//---- end of class declaration--------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Class:      CHeaderFrameInstance
//
//  Purpose:    Frame class implementing header/footer behaviour
//              this class is comparable to a detail in use and purpose
//
//----------------------------------------------------------------------------
class CHeaderFrameInstance : public CHeaderFrame
{
typedef CHeaderFrame super;
friend class CHeaderFrame;

public:
    virtual CSite::CTBag * GetTBag() { return getTemplate()->GetTBag(); }
    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}


protected:
    // cloning constructor of CHeaderFrame from the Template
    CHeaderFrameInstance(CDoc * pDoc, CSite * pParent, CHeaderFrame * pTemplate);

protected:
    // data

    static CLASSDESC   s_classdesc;
};
//---- end of class declaration--------------------------------------------------

#endif // end of header file


