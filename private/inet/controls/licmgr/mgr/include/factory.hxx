//+----------------------------------------------------------------------------
//  File:   factory.hxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


#ifndef	_FACTORY_HXX
#define	_FACTORY_HXX


// Routines -------------------------------------------------------------------
HRESULT LicenseManagerFactory(IUnknown * pUnkOuter, REFIID riid, void ** ppvObj);


//+----------------------------------------------------------------------------
//
//  Class:  CLicenseManager
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CLicenseManager : public CComponent,
                        public IObjectWithSite,
                        public IOleObject,
                        public ILicensedClassManager,
                        public ILocalRegistry,
                        public IRequireClasses,
                        public IPersistStream,
                        public IPersistStreamInit,
                        public IPersistPropertyBag
{
    friend HRESULT LicenseManagerFactory(IUnknown * pUnkOuter, REFIID riid, void ** ppvObj);
    typedef CComponent parent;

public:
    CLicenseManager(IUnknown * pUnkOuter);
    ~CLicenseManager();

    // IUnknown methods
    DEFINE_IUNKNOWN_METHODS;

    // IObjectWithSite methods
    STDMETHOD(SetSite)(IUnknown * pUnkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

    // IOleObject methods
    STDMETHOD(SetClientSite)(IOleClientSite * pClientSite);
    STDMETHOD(GetClientSite)(IOleClientSite ** ppClientSite);
    STDMETHOD(SetHostNames)(LPCOLESTR, LPCOLESTR)   { return E_NOTIMPL; }
    STDMETHOD(Close)(DWORD)                         { return E_NOTIMPL; }
    STDMETHOD(SetMoniker)(DWORD, IMoniker *)        { return E_NOTIMPL; }
    STDMETHOD(GetMoniker)(DWORD, DWORD, IMoniker **)    { return E_NOTIMPL; }
    STDMETHOD(InitFromData)(IDataObject *, BOOL, DWORD) { return E_NOTIMPL; }
    STDMETHOD(GetClipboardData)(DWORD, IDataObject **)  { return E_NOTIMPL; }
    STDMETHOD(DoVerb)(LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT)   { return E_NOTIMPL; }
    STDMETHOD(EnumVerbs)(IEnumOLEVERB **)           { return E_NOTIMPL; }
    STDMETHOD(Update)()                             { return E_NOTIMPL; }
    STDMETHOD(IsUpToDate)()                         { return E_NOTIMPL; }
    STDMETHOD(GetUserClassID)(CLSID *)              { return E_NOTIMPL; }
    STDMETHOD(GetUserType)(DWORD, LPOLESTR *)       { return E_NOTIMPL; }
    STDMETHOD(SetExtent)(DWORD, SIZEL *)            { return E_NOTIMPL; }
    STDMETHOD(GetExtent)(DWORD, SIZEL *)            { return E_NOTIMPL; }
    STDMETHOD(Advise)(IAdviseSink *, DWORD *)       { return E_NOTIMPL; }
    STDMETHOD(Unadvise)(DWORD)                      { return E_NOTIMPL; }
    STDMETHOD(EnumAdvise)(IEnumSTATDATA **)         { return E_NOTIMPL; }

    STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus)
	{
		if (!pdwStatus)
			return E_INVALIDARG;

		*pdwStatus = OLEMISC_SETCLIENTSITEFIRST | OLEMISC_INVISIBLEATRUNTIME;

		return S_OK;
	}

    STDMETHOD(SetColorScheme)(LOGPALETTE *)         { return E_NOTIMPL; }

    // ILicensedClassManager methods
    STDMETHOD(OnChangeInRequiredClasses)(IRequireClasses * pRequireClasses);

    // ILocalRegistry methods
    STDMETHOD(CreateInstance)(CLSID clsid,
                                IUnknown * punkOuter,
                                REFIID riid,
                                DWORD dwClsCtx,
                                void ** ppvObj);
    STDMETHOD(GetTypeLibOfClsid)(CLSID clsid, ITypeLib ** ptlib);
    STDMETHOD(GetClassObjectOfClsid)(REFCLSID rclsid,
                                DWORD dwClsCtx,
                                LPVOID lpReserved,
                                REFIID riid,
                                void ** ppcClassObject);

    // IRequireClasses methods
    STDMETHOD(CountRequiredClasses)(ULONG * pcClasses);
    STDMETHOD(GetRequiredClasses)(ULONG iClass, CLSID * pclsid);

    // IPersist methods
    STDMETHOD(GetClassID)(CLSID * pclsid);

    // IPersistStream methods
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream * pstm);
    STDMETHOD(Save)(IStream * pstm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER * pcbSize);

    // IPersistStreamInit methods
    STDMETHOD(InitNew)();

    // IPersistPropertyBag methods
	STDMETHOD(Load)(IPropertyBag * pPropBag, IErrorLog * pErrorLog);
	STDMETHOD(Save)(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

protected:
    struct LICENSE                  // CLSID-License structure
    {
        CLSID               clsid;      // CLSID of the object
        BSTR                bstrLic;    // License (as a BSTR)
        IClassFactory2 *    pcf2;       // Cached class factory (not persisted)
    };
    DEFINE_ARY(LICENSE);

    IUnknown *  _pUnkSite;          // Site object

    BOOL        _fDirty:1;          // Object is dirty
    BOOL        _fLoaded:1;         // Object has been loaded
    BOOL        _fPersistPBag:1;    // IPersistPropertyBag is being used
    BOOL        _fPersistStream:1;  // IPersistStream is being used

    GUID        _guidLPK;           // Identifying GUID of the .LPK

    CAryLICENSE _aryLic;            // Array of CLSID-License pairs

    HRESULT PrivateQueryInterface(REFIID riid, void ** ppvObj);

    HRESULT FindInStream(IStream * pstm, BYTE * pbData, ULONG cbData);
    HRESULT Load(IStream * pstm, ULONG cbSize);

    HRESULT AddClass(REFCLSID rclsid, int * piLic);
    BOOL    FindClass(REFCLSID rclsid, int * piLic);
};


#endif // _FACTORY_HXX
