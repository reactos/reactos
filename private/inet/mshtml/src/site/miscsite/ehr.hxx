//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ehr.hxx
//
//  Contents:   CHRElement class
//
//----------------------------------------------------------------------------

#ifndef I_EHR_HXX_
#define I_EHR_HXX_
#pragma INCMSG("--- Beg 'ehr.hxx'")

#define _hxx_
#include "hr.hdl"

class CHRLayout;

MtExtern(CHRElement)

// Maximum height of an HR in pixels

class CHRElement : public CSite
{
    DECLARE_CLASS_TYPES(CHRElement, CSite)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHRElement))

    CHRElement(CDoc *pDoc)
      : super(ETAG_HR, pDoc)
    {
#ifdef WIN16
        m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
        m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }

    ~CHRElement() {}
    
    static  HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd);

    DECLARE_LAYOUT_FNS(CHRLayout)

    HRESULT ApplyDefaultFormat ( CFormatInfo * pCFI );
    HRESULT STDMETHODCALLTYPE QueryStatus(GUID * pguidCmdGroup,
                                          ULONG cCmds,
                                          MSOCMD rgCmds[],
                                          MSOCMDTEXT * pcmdtext);
    HRESULT STDMETHODCALLTYPE Exec( GUID * pguidCmdGroup,
                                    DWORD  nCmdID,
                                    DWORD  nCmdexecopt,
                                    VARIANTARG * pvarargIn,
                                    VARIANTARG * pvarargOut);

    //--------------------------------------------------------------
    // Property bag and class descriptor
    //--------------------------------------------------------------

    #define _CHRElement_
    #include "hr.hdl"

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    NO_COPY(CHRElement);
};

#pragma INCMSG("--- End 'ehr.hxx'")
#else
#pragma INCMSG("*** Dup 'ehr.hxx'")
#endif
