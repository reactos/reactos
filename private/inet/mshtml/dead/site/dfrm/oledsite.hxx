//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       olecfrm.hxx
//
//  Contents:   Definition of datadoc control frame site
//
//  Classes:    COleDataSite,
//              COleDataSiteInstance,
//              COleDataSiteTemplate
//
//  Maintained by IstvanC
//
//----------------------------------------------------------------------------


#ifndef _OLEDSITE_HXX_
#   define _OLEDSITE_HXX_    1

//#   ifndef _OLESITE_HXX_
//#       include "OleSite.hxx"
//#   endif

#   ifndef _BINDING_HXX_
#       include "binding.hxx"
#   endif

#   ifndef _CSTR_HXX_
#       include "cstr.hxx"
#   endif

#   ifndef _DATAFRM_HXX_
#       include "datafrm.hxx"
#   endif

#   ifndef _OLESITE_HXX_
#       include "olesite.hxx"
#   endif

#   include  "propchg.hxx"


#define PAINTTEST 0


class COleDataSite : public COleSite
{
typedef COleSite super;

public:

    //+-----------------------------------------------------------------------
    //  Constructors and destructors
    //------------------------------------------------------------------------

    COleDataSite (CSite * pParent);

    // for normal creation (no cloning)
    void * operator new (size_t cb)  { return MemAllocClear(cb); }

    ~COleDataSite ();

    virtual void Detach ();

    //+-----------------------------------------------------------------------
    //  Instance creation API
    //------------------------------------------------------------------------

    // Create an instance (template method)
    HRESULT CreateInstance (CDoc * pDoc, CSite * pParent,
            CSite **ppFrame, CCreateInfo * pcinfo);

    // Build created control frame instance.
    HRESULT BuildInstance (COleDataSite * pNewInstance, CCreateInfo * pcinfo);

    inline COleDataSite * getTemplate ()
        {
            return _pTemplate;
        }

    // Populate the view rectangle (virtual overwrite).
    HRESULT CreateToFit (const CRectl * prclView, DWORD dwFlags);

    //+-----------------------------------------------------------------------
    //  DataDoc selection tree builder methods
    //------------------------------------------------------------------------

#if DBG == 1 || defined(PRODUCT_97)
    virtual HRESULT SelectSite(CSite * pSite, DWORD dwFlags);
#endif

    //+-----------------------------------------------------------------------
    //  Core method overrides
    //------------------------------------------------------------------------

    virtual HRESULT Notify(SITE_NOTIFICATION, DWORD);
    STDMETHOD(SetWidth) (long cx);
    virtual HRESULT Draw(CFormDrawInfo *pDI);
    virtual HRESULT TransitionTo(OLE_SERVER_STATE state, LPMSG pMsg = NULL);
    virtual HRESULT InsertedAt(
            CSite * pParent,
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations);
    virtual HRESULT ConnectControl1(IUnknown **ppUnkCreate, DWORD *pdwInitFlags);

    //  Note that the Flag access methods for this class allow both
    //    the OLESITE_FLAG values introduced by the base COleSite class and
    //    the OLEDATASITE_FLAG values introduced by this class

#define OLEDATASITE_FLAG_DEFAULTVALUE   ((OLESITE_FLAG) OLESITE_FLAG_DEFAULTVALUE)

    //+-----------------------------------------------------------------------
    //  Keyboard handling overrides
    //------------------------------------------------------------------------

    virtual HRESULT BUGCALL HandleMessage(CMessage *pMessage, CSite *pChild);

    // implementation of data and header/footer property access

    STDMETHOD(GetControlSource) (BSTR * pbstrControlSource);
    STDMETHOD(SetControlSource) (LPTSTR bstrControlSource);

    // this one returns the column name, independent of the
    //  creation of binding due to fieldname or number
    HRESULT GetColumnName(LPTSTR *pplptstrName);
    HRESULT BindIndirect(void);


    // overwritten load/save methods, due to pointer fixup

    #if defined(PRODUCT_97)
    virtual HRESULT WriteProps(IStream * pStm, ULONG * pulObjSize);
    virtual HRESULT ReadProps(USHORT  usPropsVer,
                              USHORT  usFormVer,
                              BYTE *  pb,
                              USHORT  cb,
                              ULONG * pulObjSize);


    HRESULT BUGCALL AfterLoad(DWORD dw);

    virtual HRESULT SetProposed(CSite * pSite, const CRectl * prcl);
    HRESULT ProposedDelta(CRectl *rclDelta, BOOL fMove);
    #else

    virtual HRESULT WriteProps(IStream * pStm, ULONG * pulObjSize) { return E_NOTIMPL; };
    virtual HRESULT ReadProps(USHORT  usPropsVer,
                              USHORT  usFormVer,
                              BYTE *  pb,
                              USHORT  cb,
                              ULONG * pulObjSize) { return E_NOTIMPL; };


    #endif

    //
    //  overwritten move functionallity
    //      for providing related oledsites syncronisation
    //
    virtual HRESULT MoveToProposed (DWORD dwFlags);


    //+-----------------------------------------------------------------------
    //  property notification and template propagation
    //      owner:frankman
    //------------------------------------------------------------------------

    virtual HRESULT OnPropertyChange(DISPID, IDispatch *pDisp);
    virtual HRESULT UpdatePropertyChanges(UPDATEPROPS updFlag=UpdatePropsPrepareTemplates);
    HRESULT UpdateProperties(void);


    //+-----------------------------------------------------------------------
    // DLA stuff
    //------------------------------------------------------------------------

    HRESULT SetDLASource(CDataFrame* pdfr);
    HRESULT SetControl ();
    HRESULT CreateAccessorByColumnNumber (UINT uiColumnID);
    HRESULT CreateAccessorByColumnName (LPTSTR pstrName);

    // hooks for databinding, overridden from CSite

    virtual HRESULT OnControlRequestEdit (DISPID dispid);
    virtual HRESULT OnControlChanged (DISPID dispid);
    virtual HRESULT SaveData();
    virtual HRESULT RefreshData();
    virtual void DataSourceChanged();

    // overridden from COleSite

    //  so that we can return success without creating a control for OLESITE_FLAG_FAKECONTROL
    virtual HRESULT CreateControl();
    virtual HRESULT ConnectControl2(IUnknown *pUnkCreate, DWORD dwInitFlags);

    enum BindIType {BIT_None, BIT_Disp, BIT_Morph};

    IMorphDataControl * GetMorphInterface(void)
    {
        Assert(TBag()->_fDispatchType == BIT_Morph);
        return _invokeIntf.pMorphData;
    }

    //+-----------------------------------------------------------------------
    //
    //  CRSControl   (fake embedded record selector control)
    //
    //------------------------------------------------------------------------

    class CRSControl : public CVoid, public IOleObject
    {
    public:

    //  MFC-style pointer fixup to the embedding COleDataSite object
    #define PREPARE_THIS() \
        COleDataSite * pThis = (COleDataSite*)((char*)this-offsetof(COleDataSite, _RSControl));

        DECLARE_TEAROFF_TABLE(IPersistStreamInit)
        //
        //  IUnknown methods
        //

        // IUnknown methods
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObj);


        //+-----------------------------------------------------------------------
        //  IOleObject implementation
        //------------------------------------------------------------------------

        //  IOleObject interface methods

        STDMETHOD(SetClientSite)(LPOLECLIENTSITE pClientSite);
        STDMETHOD(GetClientSite)(LPOLECLIENTSITE FAR* ppClientSite);
        STDMETHOD(SetHostNames)(LPCTSTR szContainerApp, LPCTSTR szContainerObj);
        STDMETHOD(Close)(DWORD dwSaveOption);
        STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, LPMONIKER pmk);
        STDMETHOD(GetMoniker)(
                DWORD dwAssign,
                DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk);
        STDMETHOD(InitFromData)(
                LPDATAOBJECT pDataObject,
                BOOL fCreation,
                DWORD dwReserved);
        STDMETHOD(GetClipboardData)(
                DWORD dwReserved,
                LPDATAOBJECT FAR* ppDataObject);
        STDMETHOD(DoVerb)(
                LONG iVerb,
                LPMSG lpmsg,
                LPOLECLIENTSITE pActiveSite,
                LONG lindex,
                HWND hwndParent,
                LPCRECT lprcPosRect);
        STDMETHOD(EnumVerbs)(LPENUMOLEVERB FAR* ppenumOleVerb);
        STDMETHOD(Update)();
        STDMETHOD(IsUpToDate)();
        STDMETHOD(GetUserClassID)(CLSID FAR* pClsid);
        STDMETHOD(GetUserType)(DWORD dwFormOfType, LPTSTR FAR* pszUserType);
        STDMETHOD(SetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);
        STDMETHOD(GetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);

        STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
        STDMETHOD(Unadvise)(DWORD dwConnection);
        STDMETHOD(EnumAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);
        STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD FAR* pdwStatus);
        STDMETHOD(SetColorScheme)(LPLOGPALETTE lpLogpal);

        //+-----------------------------------------------------------------------
        //  IPersistStreamInit implementation
        //------------------------------------------------------------------------
        //  we don't inherit from IPersistStreamInit, it is implemented
        //  as a tear-off
        //------------------------------------------------------------------------

        // *** IPersist methods ***
        STDMETHOD(GetClassID) (LPCLSID lpClassID);

        // *** IPersistStreamInit methods **
        STDMETHOD(IsDirty) (void);
        STDMETHOD(Load) (LPSTREAM pStm);
        STDMETHOD(Save) (LPSTREAM pStm, BOOL fClearDirty);
        STDMETHOD(GetSizeMax) (ULARGE_INTEGER FAR * pcbSize);
        STDMETHOD(InitNew) (void);

        COleDataSite * MyOleDataSite();
    };

    static CServer::CLASSDESC s_sclassdescDummyControl;

    //+-----------------------------------------------------------------------
    //
    //  CTBag (template bag)
    //
    //------------------------------------------------------------------------

    struct COleDataSiteTBag : super::CTBag
    {
    public:

        COleDataSiteTBag();

        ~COleDataSiteTBag();

        void * operator new(size_t cb) { return MemAllocClear(cb); }

        int _iIndex;

        CStr    _cstrControlSource;

        BindIType   _fDispatchType;

        IProvideInstance * _pProvideInstance;

        // property change notification members
        CControlPropertyChange _propertyChanges;    // holds the property change data
        UINT            _uiAccessorColumn;          // needed for autocreation of controlheaders
        COleDataSite    *_pRelated;                 // associated cell pointer
    };

    typedef COleDataSiteTBag CTBag;

    inline CTBag * TBag(); // { return (CTBag *)GetTBag(); }

    virtual int getIndex(void) { return TBag()->_iIndex; };

    virtual void SetIndex(int i) { TBag()->_iIndex = i; }

    COleDataSite *  _pTemplate;
    CRSControl      _RSControl;
    unsigned        _fRecordSelector:1;
    unsigned        _fFakeControl:1;

    CSite * GetTemplate(void) { return _pTemplate;}

protected:

    // cloning constructor
    COleDataSite (CSite * pParent, COleDataSite * pTemplate);

    // allocate size and memcpy it from original to create clone
    void * operator new (size_t s, COleSite * pOriginal);

    // temporary binding info for the 'value' property of a control for now ...
    CBaseFrame *    _pBindSource;

    union {                                 // BUGBUG (ICS) replace it with dual interfaces and use _pDispatch !
        IDispatch *         pDispatch;
        IMorphDataControl * pMorphData;
    } _invokeIntf;

#if PAINTTEST
    CStr _cstrValue;
#endif

    static const CLSID * s_aclsidPages[4];
};


inline COleDataSite *
COleDataSite::CRSControl::MyOleDataSite()
{
    return CONTAINING_RECORD(this, COleDataSite, _RSControl);
};


class COleDataSiteTemplate : public COleDataSite
{
typedef COleDataSite super;

friend class COleDataSite;

public:
    COleDataSiteTemplate(CSite * pParent) : super(pParent) {}

    virtual CSite::CTBag * GetTBag() { return &_TBag; }

    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

protected:

    CTBag _TBag;

    static PROP_DESC    s_apropdesc[];
    static CLASSDESC    s_classdesc;

};


class COleDataSiteInstance : public COleDataSite
{
typedef COleDataSite super;

friend class COleDataSite;

public:

    HRESULT InitInstance ();       // after constructor method

    HRESULT CloneControl(COleDataSite * pTemplate);
    HRESULT CreateControl();
    HRESULT ConnectControl2(IUnknown *pUnkCreate, DWORD dwInitFlags);

    virtual CSite::CTBag * GetTBag() { return getTemplate()->GetTBag(); }
    virtual CBase::CLASSDESC *GetClassDesc() const { return &s_classdesc;}

protected:

    // Cloning Constructor of COleDataSite from the COleDataSiteTemplate.
    COleDataSiteInstance(CSite * pParent, COleDataSite * pTemplate);

    ~COleDataSiteInstance ();

    static CLASSDESC   s_classdesc;
};

inline COleDataSite::CTBag * COleDataSite::TBag()
{
    Assert((CTBag *)GetTBag() == &((COleDataSiteTemplate *)_pTemplate)->_TBag);
    return &((COleDataSiteTemplate *)_pTemplate)->_TBag;
}

#endif _OLEDSITE_HXX_
