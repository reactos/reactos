#ifndef I_XMLNS_HXX_
#define I_XMLNS_HXX_
#pragma INCMSG("--- Beg 'xmlns.hxx'")

///////////////////////////////////////////////////////////////////////////
//
//  misc
//
///////////////////////////////////////////////////////////////////////////

#define COMPONENT_NAMESPACE _T("PUBLIC")
#define COMPONENT_URN       _T("URN:COMPONENT")

MtExtern(CXmlNamespaceTable);
MtExtern(CXmlUrnAtomTable);

MtExtern(CXmlNamespaceTable_CItemsArray);

enum XMLNAMESPACETYPE
{
    XMLNAMESPACETYPE_NULL = 0,
    XMLNAMESPACETYPE_TAG,
    XMLNAMESPACETYPE_ATTR
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CXmlNamespaceTable
//
///////////////////////////////////////////////////////////////////////////

class CXmlNamespaceTable : public IOleCommandTarget, public CVoid
{
public:

    DECLARE_CLASS_TYPES(CXmlNamespaceTable, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CXmlNamespaceTable))

    DECLARE_FORMS_STANDARD_IUNKNOWN(CXmlNamespaceTable)

    //
    // data item
    //

    class CItem
    {
    public:

        void    Clear();

        CStr                _cstr;
        LONG                _atom;
        XMLNAMESPACETYPE    _type;
    };

    DECLARE_CDataAry(CItemsArray, CItem, Mt(Mem), Mt(CXmlNamespaceTable_CItemsArray));

    //
    // methods
    //

    CXmlNamespaceTable(CDoc * pDoc);
    ~CXmlNamespaceTable();

    HRESULT     Init();

    HRESULT     RegisterNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType, BOOL * pfChangeDetected = NULL);
    HRESULT     GetUrnAtom       (LPTSTR pchNamespace, LONG * pAtom);
    HRESULT     GetUrn           (LPTSTR pchNamespace, LPTSTR * ppchUrn);
    HRESULT     GetUrn           (LONG   atom,         LPTSTR * ppchUrn);

    ELEMENT_TAG IsXmlSprinkle(LPTSTR pchNamespace);

    CItem *     FindItem  (LPTSTR pch);
    HRESULT     EnsureItem(LPTSTR pch, CItem ** ppItem, BOOL * pfNew = NULL);

    HRESULT     SaveNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff);

    //
    // IOleCommandTarget
    //

    STDMETHOD(QueryStatus)(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText) { RRETURN (E_NOTIMPL); }
    STDMETHOD(Exec)       (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut);

    //
    // data
    //

    CItemsArray     _aryItems;
    CDoc *          _pDoc;
    CItem *         _pLastItem;
    LONG            _urnAtomComponent;
};

///////////////////////////////////////////////////////////////////////////
//
//  Class:      CXmlUrnAtomTable
//
///////////////////////////////////////////////////////////////////////////

class CXmlUrnAtomTable : protected CAtomTable
{
public:

    DECLARE_CLASS_TYPES(CXmlUrnAtomTable, CAtomTable)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CXmlUrnAtomTable))

    //
    // methods
    //

    CXmlUrnAtomTable( );
    ~CXmlUrnAtomTable();

    HRESULT EnsureUrnAtom(LPTSTR pchUrn, LONG * pAtom);
    inline HRESULT GetUrn(LONG atom, LPTSTR * ppchUrn)
    {
        HRESULT     hr;

        hr = THR(GetNameFromAtom (atom, (LPCTSTR*)ppchUrn));

        Assert (DISP_E_MEMBERNOTFOUND != hr);   // should never fail to get urn from atom

        RRETURN (hr);
    }

    //
    // data
    //
};

///////////////////////////////////////////////////////////////////////////
//
//  eof
//
///////////////////////////////////////////////////////////////////////////

#pragma INCMSG("--- End 'xmlns.hxx'")
#else
#pragma INCMSG("*** Dup 'xmlns.hxx'")
#endif
 