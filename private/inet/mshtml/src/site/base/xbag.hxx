//+---------------------------------------------------------------------------
//
//  File:       xbag.hxx
//
//  Contents:   Xfer object for arbitrary selections on Form
//
//  Classes:    CXBag
//
//  History:    18-Nov-93   CliffG      Created
//              5-22-95     kfl         converted WCHAR to TCHAR
//              24-Aug-95   t-AnandR    Added mouse button state info
//
//----------------------------------------------------------------------------

#ifndef I_XBAG_HXX_
#define I_XBAG_HXX_
#pragma INCMSG("--- Beg 'xbag.hxx'")

MtExtern(CDummyDropSource)
MtExtern(CGenDataObject)
MtExtern(CEnumFormatEtc)

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CDropSource
// Implements IDropSource interface.

class CDropSource : public IDropSource
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    STDMETHOD(QueryContinueDrag) (BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback) (DWORD dwEffect);
protected:
    DWORD        _dwButton;     // Mouse btn state info for dragging
    CDoc *       _pDoc;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//
// CDummyDropSource
// Wraps a trivial instantiable class around CDropSource

class CDummyDropSource : public CDropSource
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDummyDropSource))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CDummyDropSource);
    static HRESULT Create(DWORD dwKeyState, CDoc * pDoc, IDropSource ** ppDropSrc);
private:
    CDummyDropSource() {_ulRefs = 1; }
};

// command IDs used by IOleCommandTarget interface, for programatic paste
// security check

#define IDM_SETSECURITYDOMAIN   1000
#define IDM_CHECKSECURITYDOMAIN 1001

// An abstract class for all the classes that implement
// IDataObject (the text xbag,  the 2d xbag and the
// generic data object)
class CBaseBag : public CDropSource,
                 public IDataObject,
                 public IOleCommandTarget
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_FORMS_STANDARD_IUNKNOWN(CBaseBag);

    //
    //IDataObject
    //
    STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc,
            DWORD advf,
            LPADVISESINK pAdvSink,
            DWORD FAR* pdwConnection) { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP DUnadvise( DWORD dwConnection)
            { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise)
            { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP EnumFormatEtc(
                DWORD dwDirection,
                LPENUMFORMATETC FAR* ppenumFormatEtc)
            { return E_NOTIMPL; }

    STDMETHODIMP GetCanonicalFormatEtc(
                LPFORMATETC pformatetc,
                LPFORMATETC pformatetcOut)
            { pformatetcOut->ptd = NULL; return E_NOTIMPL; }

    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
            { return E_NOTIMPL; }
    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
            { return E_NOTIMPL; }
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc )
            { return E_NOTIMPL; }
    STDMETHODIMP SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
            { return E_NOTIMPL; }

    //
    // IOleCommandTarget
    //
    
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(
                    const GUID * pguidCmdGroup,
                    ULONG cCmds,
                    OLECMD rgCmds[],
                    OLECMDTEXT * pcmdtext);
    virtual HRESULT STDMETHODCALLTYPE Exec(
                    const GUID * pguidCmdGroup,
                    DWORD nCmdID,
                    DWORD nCmdexecopt,
                    VARIANTARG * pvarargIn,
                    VARIANTARG * pvarargOut);

    //
    // Other
    //

    virtual HRESULT DeleteObject()
            { return E_FAIL; }

    IDataObject * _pLinkDataObj;
    BOOL _fLinkFormatAppended;

    virtual     ~CBaseBag()
    {
        if (_pLinkDataObj)
        {
            _pLinkDataObj->Release();
            _pLinkDataObj = NULL;
        }
    }
    BYTE        _abSID[MAX_SIZE_SECURITY_ID];
};

class CGenDataObject : public CBaseBag
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CGenDataObject))
    CGenDataObject(CDoc * pDoc);
    ~CGenDataObject();

    void SetPreferredEffect(DWORD dwPreferredEffect)
        { _dwPreferredEffect = dwPreferredEffect; }

    DWORD GetPreferredEffect() const
        { return _dwPreferredEffect; }

    HRESULT AppendFormatData(CLIPFORMAT cfFormat, HGLOBAL hGlobal);
    HRESULT DeleteFormatData(CLIPFORMAT cfFormat);

    //IDataObject
    //
    STDMETHODIMP EnumFormatEtc(DWORD                    dwDirection,
                               LPENUMFORMATETC FAR *    ppenumFormatEtc);
    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc);
    STDMETHODIMP SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);

    void SetBtnState(DWORD dwKeyState);

    CDataAry<FORMATETC>     _rgfmtc;
    CDataAry<STGMEDIUM>     _rgstgmed;

private:
    FORMATETC   _fmtcPreferredEffect;
    DWORD       _dwPreferredEffect;
};

/*
 *  class CEnumFormatEtc
 *
 *  Purpose:
 *      implements a generic format enumerator for IDataObject
 */

class CEnumFormatEtc : public IEnumFORMATETC
{
public:
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    STDMETHOD(Next) (ULONG celt, FORMATETC *rgelt,
            ULONG *pceltFetched);
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) (void);
    STDMETHOD(Clone) (IEnumFORMATETC **ppenum);

    static HRESULT Create(FORMATETC *prgFormats, DWORD cFormats, 
                IEnumFORMATETC **ppenum);

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEnumFormatEtc))
    CEnumFormatEtc();
    ~CEnumFormatEtc();

    ULONG       _crefs;
    ULONG       _iCurrent;  // current clipboard format
    ULONG       _cTotal;    // total number of formats
    FORMATETC * _prgFormats; // array of available formats
};

#pragma INCMSG("--- End 'xbag.hxx'")
#else
#pragma INCMSG("*** Dup 'xbag.hxx'")
#endif
