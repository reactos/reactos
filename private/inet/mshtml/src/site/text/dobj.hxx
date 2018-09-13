#ifndef I_DOBJ_HXX_
#define I_DOBJ_HXX_
#pragma INCMSG("--- Beg 'dobj.hxx'")

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

MtExtern(CDataObject)

class CDataObject : public CBaseBag
{
    typedef CBaseBag super;

//BUGBUG (alexz): rumors are that CBaseBag is about to go away,
// so either it's functionality should be moved to CDataObject,
// or it should be preserved

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDataObject))

    CDataObject (LPSTR lpszText);
    ~CDataObject();

    // IDataObject interfaces

    STDMETHOD(EnumFormatEtc)    (DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc);
    STDMETHOD(GetData)          (FORMATETC * pformatetcIn, STGMEDIUM * pmedium);
    STDMETHOD(QueryGetData)     (FORMATETC * pformatetcs);

    // misc

    static HRESULT Create (LPSTR lpszText, CDataObject ** ppObj);
    static HRESULT CreateFromStr (LPSTR lpszText, IDataObject ** ppDataObject);

    // implementation

    HRESULT InitObj (LPSTR lpszText);
    void    DoneObj ();

    HRESULT InitFormats ();
    void    DoneFormats ();

protected:

    ULONG       _cFormats;      // total number of formats supported
    FORMATETC   * _pFormats;    // the array of supported formats

    HGLOBAL     _hObj;         // handle to block containing text
};

#pragma INCMSG("--- End 'dobj.hxx'")
#else
#pragma INCMSG("*** Dup 'dobj.hxx'")
#endif
