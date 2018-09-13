//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       eform.hxx
//
//  Contents:   CFormElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_EFORM_HXX_
#define I_EFORM_HXX_
#pragma INCMSG("--- Beg 'eform.hxx'")

#define _hxx_
#include "eform.hdl"

MtExtern(CFormElement)

class CInput;

// reserve first half of collection range for DISPID-s exposed in typeinfo
#define DISPID_COLLECTION_TI_MIN        DISPID_COLLECTION_MIN
#define DISPID_COLLECTION_TI_MAX        (DISPID_COLLECTION_MIN + (DISPID_COLLECTION_MAX + 1 - DISPID_COLLECTION_MIN) / 2 - 1)
// reserve second half for DISPID-s exposed through GetIDsOfNames
#define DISPID_COLLECTION_GN_MIN        (DISPID_COLLECTION_TI_MAX + 1)
#define DISPID_COLLECTION_GN_MAX        DISPID_COLLECTION_MAX

// define mappings between the ranges
#define DISPID_COLLECTION_TI_TO_GN(x)   (x - DISPID_COLLECTION_TI_MIN + DISPID_COLLECTION_GN_MIN)
#define DISPID_COLLECTION_GN_TO_TI(x)   (x - DISPID_COLLECTION_GN_MIN + DISPID_COLLECTION_TI_MIN)

// reserve typeinfo ranges for form element collection and form named img collection
#define DISPID_FORM_ELEMENT_TI_MIN      DISPID_COLLECTION_TI_MIN
#define DISPID_FORM_ELEMENT_TI_MAX      (DISPID_COLLECTION_TI_MIN + (DISPID_COLLECTION_TI_MAX + 1 - DISPID_COLLECTION_TI_MIN) / 2 - 1)
#define DISPID_FORM_NAMED_IMG_TI_MIN    (DISPID_FORM_ELEMENT_TI_MAX + 1)
#define DISPID_FORM_NAMED_IMG_TI_MAX    DISPID_COLLECTION_TI_MAX

// define GetIdsOfNames ranges for form element collection and form named img collection
#define DISPID_FORM_ELEMENT_GN_MIN      DISPID_COLLECTION_TI_TO_GN(DISPID_FORM_ELEMENT_TI_MIN)
#define DISPID_FORM_ELEMENT_GN_MAX      DISPID_COLLECTION_TI_TO_GN(DISPID_FORM_ELEMENT_TI_MAX)
#define DISPID_FORM_NAMED_IMG_GN_MIN    DISPID_COLLECTION_TI_TO_GN(DISPID_FORM_NAMED_IMG_TI_MIN)
#define DISPID_FORM_NAMED_IMG_GN_MAX    DISPID_COLLECTION_TI_TO_GN(DISPID_FORM_NAMED_IMG_TI_MAX)

//+---------------------------------------------------------------------------
//
// CFormElement
//
//----------------------------------------------------------------------------

class CFormElement : public CElement
{
    DECLARE_CLASS_TYPES(CFormElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFormElement))

    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)

    CFormElement(CDoc *pDoc)
        : CElement(ETAG_FORM, pDoc) 
    {
#ifdef WIN16
	    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
            m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    }

    ~CFormElement() { delete _pCollectionCache; }

    static  HRESULT CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElementResult);

    // CBase overrides
    virtual HRESULT Init2(CInit2Context * pContext);
    void            Passivate();

    // strictly for in-this-file use
    enum 
    {
        FORM_ELEMENT_COLLECTION = 0,
        FORM_NAMED_IMG_COLLECTION = 1,
        FORM_SUBMIT_COLLECTION = 2,
        FORM_NUM_COLLECTIONS 
    };

    //
    // IPrivateUnknown members
    //

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // The following 4 methods are IDispatchEx:
    HRESULT STDMETHODCALLTYPE GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid);

    NV_DECLARE_TEAROFF_METHOD (ContextThunk_InvokeEx, contextthunk_invokeex, (
            DISPID dispidMember,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            IServiceProvider *pSrvProvider));

    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id,
                                            BSTR *pbstrName));

    // IProvideMultiClassInfo methods
    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti, 
            DWORD dwFlags, 
            ITypeInfo** pptiCoClass, 
            DWORD* pdwTIFlags, 
            ULONG* pcdispidReserved, 
            IID* piidPrimary, 
            IID* piidSource));

    #define _CFormElement_
    #include "eform.hdl"

    // interface baseimplementation property prototypes
    NV_DECLARE_TEAROFF_METHOD(put_method, PUT_method, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(get_method, GET_method, (BSTR*p));

    DECLARE_CLASSDESC_MEMBERS;

    NV_DECLARE_ENSURE_METHOD(EnsureCollections, ensurecollections, (long, long * plCollectionVersion));
    HRESULT         EnsureCollectionCache();
    HRESULT         FormTraverseGroup(
        LPCTSTR bstrGroupName, 
        PFN_VISIT pfn,
        DWORD_PTR dw,
        BOOL fForward);

    CElement * FindDefaultElem(BOOL fDefault, BOOL fCurrent=FALSE);

    HRESULT FOnlyTextbox(CInput * pTextbox, BOOL * pfOnly);

    HRESULT BUGCALL ValidateElement(CElement *pElement, CStr **ppstr);

    HRESULT DoSubmit(CElement *pSubmitSite, BOOL fFireEvent);
    HRESULT DoReset(BOOL fFireEvent);

    HRESULT CallGetSubmitInfo(
        CElement * pSubmitSite,
        CElement** ppInputImg,
        CPostData* pSubmitData,
        int      * pnMultiLines,
        int      * pnFieldsChanged,
        LPCTSTR  * pchAction,
        BOOL     * pfSendAsPost,
        BOOL       fUseUtf8 = FALSE,
        CODEPAGE   cp = NULL);

    //
    // CElement overrides.
    //
    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    //
    // Helper functions
    //
    BOOL Utf8InAcceptCharset();

    //
    // Data members
    //
    
    CCollectionCache *  _pCollectionCache;
    ITypeInfo *         _pTypeInfoElements;         // type info of elements collection
    ITypeInfo *         _pTypeInfoCoClassElements;  // coclass for elements collection
    ITypeInfo *         _pTypeInfoImgs;             // type info of images collection
    ITypeInfo *         _pTypeInfoCoClassImgs;      // coclass for images collection
    CElement            *_pElemDefault;             // Default element

    RADIOGRPNAME        *_pRadioGrpName;    // names of radio groups having checked radio

    unsigned            _fInTraverseGroup:1;        // TRUE if inside TraverseGroup of radio buttons

protected:
    static const CLSID *                s_apclsidPages[];


private:
    NO_COPY(CFormElement);
};

#pragma INCMSG("--- End 'eform.hxx'")
#else
#pragma INCMSG("*** Dup 'eform.hxx'")
#endif
