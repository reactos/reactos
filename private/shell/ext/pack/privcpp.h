#ifndef pack2cpp_h__
#define pack2cpp_h__

#ifdef __cplusplus

#undef DebugMsg
#define DebugMsg TraceMsg


////////////////////////////////
// Forward declarations
//
class CPackage_IOleObject;
class CPackage_IViewObject2;
class CPackage_IDataObject;
class CPackage_IPersistStorage;
class CPackage_IAdviseSink;
class CPackage_IRunnableObject;
class CPackage_IPersistFile;


////////////////////////////////
// CPackage Definition
//
class CPackage : public IEnumOLEVERB
{

// CPackage interfaces
    friend CPackage_IOleObject;
    friend CPackage_IViewObject2;
    friend CPackage_IDataObject;
    friend CPackage_IPersistStorage;
    friend CPackage_IAdviseSink;
    friend CPackage_IRunnableObject;
    friend CPackage_IPersistFile;
    
    friend DWORD CALLBACK MainWaitOnChildThreadProc(void *);   // used when we shellexec a package
    
public:
    CPackage();                 // constructor
   ~CPackage();                 // destructor
   
    HRESULT Init();             // used to initialze fields that could fail
    BOOL        RunWizard();

    // IUnknown methods...
    STDMETHODIMP            QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    // IEnumOLEVERB methods...
    STDMETHODIMP            Next(ULONG celt, OLEVERB* rgVerbs, ULONG* pceltFetched);
    STDMETHODIMP            Skip(ULONG celt);
    STDMETHODIMP            Reset();
    STDMETHODIMP            Clone(IEnumOLEVERB** ppEnum);
    
protected:
    UINT        _cRef;          // package reference count
    UINT        _cf;            // package clipboard format

    LPIC	_lpic;		// icon for the packaged object
    PANETYPE	_panetype;	// tells us whether we have a cmdlink or embed

    // These are mutually exclusive, so should probably be made into a union,
    // but that's a minor point.
    LPEMBED     _pEmbed;        // embedded file structure
    LPCML	_pCml;		// command line structure

    BOOL        _fLoaded;       // true if data from persistent storage
    
    // IOleObject vars from SetHostNames
    LPOLESTR    _lpszContainerApp;
    LPOLESTR    _lpszContainerObj;
    
    BOOL        _fIsDirty;      // dirty flag used by IPersistStorage
    DWORD       _dwCookie;      // connection value for AdviseSink
        
    // Package Storages and streams
    IStorage*   _pIStorage;             // storage used to save the package
    IStream*    _pstm;                  // stream used to save package
    IStream*    _pstmFileContents;      // stream used to get file contents
            
    // CPackage Interfaces...
    CPackage_IPersistStorage*   _pIPersistStorage;
    CPackage_IDataObject*       _pIDataObject;
    CPackage_IOleObject*        _pIOleObject;
    CPackage_IViewObject2*      _pIViewObject2;
    CPackage_IAdviseSink*       _pIAdviseSink;
    CPackage_IRunnableObject*   _pIRunnableObject;
    CPackage_IPersistFile*      _pIPersistFile;
    
    // Advise interfaces
    LPDATAADVISEHOLDER          _pIDataAdviseHolder;
    LPOLEADVISEHOLDER           _pIOleAdviseHolder;
    LPOLECLIENTSITE             _pIOleClientSite;
    
    // to be able to send view change notifications we need these vars
    IAdviseSink                     *_pViewSink;
    DWORD                            _dwViewAspects;
    DWORD                            _dwViewAdvf;

    // IEnumOLEVERB variables:
    ULONG       _cVerbs;
    ULONG       _nCurVerb;
    OLEVERB*    _pVerbs;
    IContextMenu* _pcm;

    // IEnumOLEVERB helper methods:
    HRESULT InitVerbEnum(OLEVERB* pVerbs, ULONG cVerbs);
    HRESULT GetContextMenu(IContextMenu** ppcm);
    VOID ReleaseContextMenu();

    // if fInitFile is TRUE, then we will totally initialize ourselves
    // from the given filename.  In other words, all our structures will be
    // initialized after calling this is fInitFile = TRUE.  On the other hand,
    // if it's FALSE, then we'll just reinit our data and not update icon
    // and filename information.
    //
    HRESULT EmbedInitFromFile(LPTSTR lpFileName, BOOL fInitFile);
    HRESULT CmlInitFromFile(LPTSTR lpFilename, BOOL fUpdateIcon);
    HRESULT InitFromPackInfo(LPPACKAGER_INFO lppi);
    
    HRESULT CreateTempFile();
    HRESULT CreateTempFileName();
    HRESULT IconRefresh();
    void    DestroyIC();
    
    // Data Transfer functions...
    HRESULT GetFileDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    HRESULT GetFileContents(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    HRESULT GetMetafilePict(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    HRESULT GetObjectDescriptor(LPFORMATETC pFE, LPSTGMEDIUM pSTM) ;

    HRESULT CreateShortcutOnStream(IStream* pstm); 

    // Packager Read/Write Functions...
    HRESULT PackageReadFromStream(IStream* pstm);
    HRESULT IconReadFromStream(IStream* pstm);
    HRESULT EmbedReadFromStream(IStream* pstm);
    HRESULT CmlReadFromStream(IStream* pstm);
    HRESULT PackageWriteToStream(IStream* pstm);
    HRESULT IconWriteToStream(IStream* pstm, DWORD *pdw);
    HRESULT EmbedWriteToStream(IStream* pstm, DWORD *pdw);
    HRESULT CmlWriteToStream(IStream* pstm, DWORD *pdw);
};


////////////////////////////////////////////
//
// CPackage_IPersistStorage Interface
//
class CPackage_IPersistStorage : public IPersistStorage
{
public:
    CPackage_IPersistStorage(CPackage *pPackage);
   ~CPackage_IPersistStorage();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IPersistStorage Methods...
    STDMETHODIMP        GetClassID(LPCLSID pClassID);	
    STDMETHODIMP        IsDirty(void);
    STDMETHODIMP        InitNew(IStorage* pstg);
    STDMETHODIMP        Load(IStorage* pstg);
    STDMETHODIMP        Save(IStorage* pstg, BOOL fSameAsLoad);
    STDMETHODIMP        SaveCompleted(IStorage* pstg);
    STDMETHODIMP        HandsOffStorage(void);
    
protected:
    UINT        _cRef;                  // interface ref count
    CPackage*   _pPackage;              // back pointer to object
    PSSTATE     _psState;               // persistent storage state
};

////////////////////////////////////////////
//
// CPackage_IPersistFile Interface
//
class CPackage_IPersistFile : public IPersistFile
{
public:
    CPackage_IPersistFile(CPackage *pPackage);
   ~CPackage_IPersistFile();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IPersistStorage Methods...
    STDMETHODIMP        GetClassID(LPCLSID pClassID);	
    STDMETHODIMP        IsDirty(void);
    STDMETHODIMP        Load(LPCOLESTR pszFileName, DWORD dwdMode);
    STDMETHODIMP        Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP        SaveCompleted(LPCOLESTR pszFileName);
    STDMETHODIMP        GetCurFile(LPOLESTR *ppszFileName);
    
protected:
    UINT        _cRef;                  // interface ref count
    CPackage*   _pPackage;              // back pointer to object
};





////////////////////////////////////////////
//
// CPackage_IDataObject Interface
//
class CPackage_IDataObject : public IDataObject
{
public:
    CPackage_IDataObject(CPackage *pPackage);
   ~CPackage_IDataObject();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IDataObject Methods...
    STDMETHODIMP GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM);
    STDMETHODIMP GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM);
    STDMETHODIMP QueryGetData(LPFORMATETC pFE);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut);
    STDMETHODIMP SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
    STDMETHODIMP DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
                            DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);
    
protected:
    UINT        _cRef;
    CPackage*   _pPackage;
};





////////////////////////////////////////////
//
// CPackage_IOleObject Interface
//

class CPackage_IOleObject : public IOleObject
{
    
    friend DWORD CALLBACK MainWaitOnChildThreadProc(void *);
    
public:
    CPackage_IOleObject(CPackage *pPackage);
   ~CPackage_IOleObject();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    
    // IOleObject Methods...
    STDMETHODIMP SetClientSite(LPOLECLIENTSITE pClientSite);
    STDMETHODIMP GetClientSite(LPOLECLIENTSITE *ppClientSite);
    STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    STDMETHODIMP Close(DWORD dwSaveOption);
    STDMETHODIMP SetMoniker(DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMonkier,LPMONIKER *ppmk);
    STDMETHODIMP InitFromData(LPDATAOBJECT pDataObject, BOOL fCreation, 
                                 DWORD dwReserved);
    STDMETHODIMP GetClipboardData(DWORD dwReserved, LPDATAOBJECT *ppDataObject);
    STDMETHODIMP DoVerb(LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, 
                           LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    STDMETHODIMP EnumVerbs(LPENUMOLEVERB *ppEnumOleVerb);
    STDMETHODIMP Update(void);
    STDMETHODIMP IsUpToDate(void);
    STDMETHODIMP GetUserClassID(LPCLSID pClsid);
    STDMETHODIMP GetUserType(DWORD dwFromOfType, LPOLESTR *pszUserType);
    STDMETHODIMP SetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    STDMETHODIMP GetExtent(DWORD dwDrawAspect, LPSIZEL psizel);
    STDMETHODIMP Advise(LPADVISESINK pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP Unadvise(DWORD dwConnection);
    STDMETHODIMP EnumAdvise(LPENUMSTATDATA *ppenumAdvise);
    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);
    STDMETHODIMP SetColorScheme(LPLOGPALETTE pLogpal);
    
protected:
    UINT        _cRef;
    CPackage*   _pPackage;
    
};





////////////////////////////////////////////
//
// CPackage_IViewObject2 Interface
//
class CPackage_IViewObject2 : public IViewObject2
{
public:
    CPackage_IViewObject2(CPackage *pPackage);
   ~CPackage_IViewObject2();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IViewObject2 Methods...
    STDMETHODIMP Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
                         DVTARGETDEVICE *ptd, HDC hdcTargetDev,
                         HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                         BOOL (CALLBACK *pfnContinue)(DWORD), DWORD dwContinue);
    STDMETHODIMP GetColorSet(DWORD dwAspect, LONG lindex, void *pvAspect,
                                DVTARGETDEVICE *ptd, HDC hdcTargetDev,
                                LPLOGPALETTE *ppColorSet);
    STDMETHODIMP Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, 
                           DWORD *pdwFreeze);
    STDMETHODIMP Unfreeze(DWORD dwFreeze);
    STDMETHODIMP SetAdvise(DWORD dwAspects, DWORD dwAdvf,
                              LPADVISESINK pAdvSink);
    STDMETHODIMP GetAdvise(DWORD *pdwAspects, DWORD *pdwAdvf,
                              LPADVISESINK *ppAdvSink);
    STDMETHODIMP GetExtent(DWORD dwAspect, LONG lindex, DVTARGETDEVICE *ptd,
                              LPSIZEL pszl);
                           
protected:
    UINT                _cRef;
    CPackage*           _pPackage;
    BOOL                _fFrozen;
};


////////////////////////////////////////////
//
// CPackage_IAdviseSink Interface
//
class CPackage_IAdviseSink : public IAdviseSink
{
public:
    CPackage_IAdviseSink(CPackage *pPackage);
   ~CPackage_IAdviseSink();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IAdviseSink Methods...
    STDMETHODIMP_(void)  OnDataChange(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP_(void)  OnViewChange(DWORD, LONG);
    STDMETHODIMP_(void)  OnRename(LPMONIKER);
    STDMETHODIMP_(void)  OnSave(void);
    STDMETHODIMP_(void)  OnClose(void);
                           
protected:
    UINT                _cRef;
    CPackage*           _pPackage;
    
};

////////////////////////////////////////////
//
// CPackage_IRunnableObject Interface
//
class CPackage_IRunnableObject : public IRunnableObject
{
public:
    CPackage_IRunnableObject(CPackage *pPackage);
   ~CPackage_IRunnableObject();
    
    // IUnknown Methods...
    STDMETHODIMP         QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IRunnable Object methods...
    STDMETHODIMP        GetRunningClass(LPCLSID);
    STDMETHODIMP        Run(LPBC);
    STDMETHODIMP_(BOOL) IsRunning();
    STDMETHODIMP        LockRunning(BOOL,BOOL);
    STDMETHODIMP        SetContainedObject(BOOL);
    
protected:
    UINT                _cRef;
    CPackage*           _pPackage;
    
};



////////////////////////////////////////////
//
// Package Wizard and Edit Package Dialog Procs and functions
//

// Pages for Wizard
BOOL APIENTRY PackWiz_CreatePackageDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY PackWiz_SelectFileDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY PackWiz_SelectIconDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY PackWiz_SelectLabelDlgProc(HWND, UINT, WPARAM, LPARAM);

// Edit dialog procs
BOOL APIENTRY PackWiz_EditEmbedPackageDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY PackWiz_EditCmdPackakgeDlgProc(HWND, UINT, WPARAM, LPARAM);

// functions
int	PackWiz_CreateWizard(HWND,LPPACKAGER_INFO);
int	PackWiz_EditPackage(HWND,WORD,LPPACKAGER_INFO);
VOID	PackWiz_FillInPropertyPage(PROPSHEETPAGE *, INT, DLGPROC);


#endif  // __cplusplus

#endif
