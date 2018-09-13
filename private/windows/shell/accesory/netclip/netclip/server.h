// NetClipServer.h : header file
//

#define DECLARE_NETCLIPCREATE(class_name) \
public: \
	static AFX_DATA CNetClipObjectFactory factory; \
	static AFX_DATA const GUID guid; \

// Specify that we're multi-instance
//
#define IMPLEMENT_NETCLIPCREATE(class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	AFX_DATADEF CNetClipObjectFactory class_name::factory(class_name::guid, \
		RUNTIME_CLASS(class_name), FALSE, _T(external_name)); \
	const AFX_DATADEF GUID class_name::guid = \
		{ l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }; \

class CNetClipObjectFactory : public COleObjectFactory
{
public:
    CNetClipObjectFactory(REFCLSID clsid, CRuntimeClass* pRuntimeClass, BOOL bMultiInstance, LPCTSTR lpszProgID);
    virtual ~CNetClipObjectFactory() {};
    virtual CCmdTarget* OnCreateObject();
    virtual BOOL Register();
};

/////////////////////////////////////////////////////////////////////////////
// CNetClipServer command target

class CNetClipServer : public CWnd
{
	DECLARE_DYNCREATE(CNetClipServer)

	CNetClipServer();           // protected constructor used by dynamic creation

// Attributes
public:
    HWND m_hwndNextCB ;

// Operations
public:
    HRESULT SendOnClipboardChanged();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetClipServer)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CNetClipServer();

	// Generated message map functions
	//{{AFX_MSG(CNetClipServer)
	afx_msg void OnDrawClipboard();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	//}}AFX_MSG
    afx_msg LRESULT OnSendOnClipboardChanged(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
	DECLARE_NETCLIPCREATE(CNetClipServer)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CNetClipServer)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
//  DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

    BEGIN_INTERFACE_PART(Clipboard, IClipboard)
        virtual HRESULT __stdcall GetClipboardFormatName( 
            /* [in] */ CLIPFORMAT cf,
            /* [out] */ LPOLESTR __RPC_FAR *ppsz);
        
        virtual HRESULT __stdcall GetClipboard( 
            /* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDataObject);
        
        virtual HRESULT __stdcall SetClipboard( 
            /* [in] */ IDataObject __RPC_FAR *pDataObject);
        
        virtual HRESULT __stdcall IsCurrentClipboard( 
            /* [in] */ IDataObject __RPC_FAR *pDataObject);
        
        virtual HRESULT __stdcall FlushClipboard( void);
    END_INTERFACE_PART(Clipboard)

    DECLARE_CONNECTION_MAP()

    // Connection point for ISample interface
    BEGIN_CONNECTION_PART(CNetClipServer, ClipboardNotifyCP)
        CONNECTION_IID(IID_IClipboardNotify)
    END_CONNECTION_PART(ClipboardNotifyCP)
};

/////////////////////////////////////////////////////////////////////////////
