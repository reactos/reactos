// NetClipServer.cpp : CNetClipServer implementation file
//
// Implements the remote clipboard object (IClipboard)
//
#include "stdafx.h"
#include "NetClip.h"
#include "Server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CNetClipObjectFactory::CNetClipObjectFactory(REFCLSID clsid, CRuntimeClass* pRuntimeClass, BOOL bMultiInstance, LPCTSTR lpszProgID) :
        COleObjectFactory(clsid, pRuntimeClass, bMultiInstance, lpszProgID)
{
}

// We override OnCreateObject so we can create a window that we register
// in the clipboard viewer chain
CCmdTarget* CNetClipObjectFactory::OnCreateObject()
{
    CNetClipServer* p =(CNetClipServer*)COleObjectFactory::OnCreateObject();

	// make sure it is a CNetClipServer
	ASSERT_KINDOF(CNetClipServer, p);
	ASSERT_VALID(p);

    // Create our window
    //
    if (0 == p->CreateEx(0, AfxRegisterWndClass(0), _T("Serving Clipboard"), WS_OVERLAPPEDWINDOW, 0,0,0,0, NULL, 0))
    	AfxThrowMemoryException();

	// return the new CCmdTarget object
	return p;
}

BOOL CNetClipObjectFactory::Register()
{
	ASSERT_VALID(this);
	ASSERT(!m_bRegistered);  // registering server/factory twice?
	ASSERT(m_clsid != CLSID_NULL);

	SCODE sc = ::CoRegisterClassObject(m_clsid, &m_xClassFactory,
		CLSCTX_LOCAL_SERVER,
		m_bMultiInstance ? REGCLS_SINGLEUSE : REGCLS_MULTIPLEUSE,
		&m_dwRegister);
	if (sc != S_OK)
	{
#ifdef _DEBUG
		TRACE1("Warning: CoRegisterClassObject failed scode = %s.\n",
			::AfxGetFullScodeString(sc));
#endif
		// registration failed.
		return FALSE;
	}
	ASSERT(m_dwRegister != 0);

	++m_bRegistered;

    // On Win95 we act as a daemon...that is, once started via "netclip /server"
    // we never shut down until killed.
    //
    // On NT only "netclip /Embedding" will cause us to stay around, and we'll
    // exit as soon as the last client releases us.
    //
    // We accomplish this by forcing an additional "App lock". MFC will not
    // revoke the class factory unless the app lock goes to zero...
    //
    if (g_osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        AfxOleLockApp();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetClipServer

IMPLEMENT_DYNCREATE(CNetClipServer, CWnd)

CNetClipServer::CNetClipServer()
{
    // We need to enable MFC IConnectionPointContainer support
    EnableConnections();

	// To keep the application running as long as an OLE
	//	object is active, the constructor calls AfxOleLockApp	
	AfxOleLockApp();

    m_hwndNextCB = NULL;
}

CNetClipServer::~CNetClipServer()
{
	// To terminate the application when all objects created with
	// 	with OLE, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void CNetClipServer::OnFinalRelease()
{
	// When the last reference for an object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

    // OnFinalRelease will destroy our window for us.
	CWnd::OnFinalRelease();

    // CWnd does not automatically delete the object so we have
    // to do it ourselves.
    delete this;
}

#define WM_SENDONCLPBOARDCHANGED (WM_USER+1)
BEGIN_MESSAGE_MAP(CNetClipServer, CWnd)
	//{{AFX_MSG_MAP(CNetClipServer)
	ON_WM_DRAWCLIPBOARD()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CHANGECBCHAIN()
	//}}AFX_MSG_MAP
    ON_MESSAGE( WM_SENDONCLPBOARDCHANGED, OnSendOnClipboardChanged)
END_MESSAGE_MAP()

int CNetClipServer::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    // Register as a clipboard viewer
    m_hwndNextCB = SetClipboardViewer();
	return 0;
}

void CNetClipServer::OnDestroy()
{
    //if (m_hwndNextCB)
        ChangeClipboardChain(m_hwndNextCB);
    m_hwndNextCB = NULL;
    CWnd::OnDestroy();
}

void CNetClipServer::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
    if (m_hwndNextCB)
        ::SendMessage(m_hwndNextCB, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter);
}

void CNetClipServer::OnDrawClipboard()
{
    if (m_hwndNextCB)
        ::SendMessage(m_hwndNextCB, WM_DRAWCLIPBOARD, 0, 0);

    // Attempting to call out while handling a WM_DRAWCLIPBOARD causes
    // RPC_E_CANTCALLOUT_ININPUTSYNCCALL.  We work around this
    // by posting a WM_USER message to ourselves and calling
    // SendOnClipboardChanged from there
    //
    PostMessage(WM_SENDONCLPBOARDCHANGED);
}

LRESULT CNetClipServer::OnSendOnClipboardChanged(WPARAM, LPARAM)
{
    SendOnClipboardChanged();
    return 0;
}

BEGIN_INTERFACE_MAP(CNetClipServer, CWnd)
	INTERFACE_PART(CNetClipServer, IID_IClipboard, Clipboard)
	INTERFACE_PART(CNetClipServer, IID_IConnectionPointContainer, ConnPtContainer)
END_INTERFACE_MAP()

// {F7565504-4B54-11CF-B63C-0080C792B782}
IMPLEMENT_NETCLIPCREATE(CNetClipServer, "Remote Clipboard", 0xf7565504, 0x4b54, 0x11cf, 0xb6, 0x3c, 0x0, 0x80, 0xc7, 0x92, 0xb7, 0x82)

/////////////////////////////////////////////////////////////////////////////
// CNetClipServer message handlers

ULONG CNetClipServer::XClipboard::AddRef()
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)
    return pThis->ExternalAddRef();
}

ULONG CNetClipServer::XClipboard::Release()
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)
    return pThis->ExternalRelease();
}

HRESULT CNetClipServer::XClipboard::QueryInterface(
    REFIID iid, void** ppvObj)
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)
    return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}


HRESULT CNetClipServer::XClipboard::GetClipboardFormatName(
            /* [in] */ CLIPFORMAT cf,
            /* [out] */ LPOLESTR __RPC_FAR *ppsz)
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)

    *ppsz=NULL;
    TCHAR sz[256];
    int n = ::GetClipboardFormatName(cf, sz, 255);
    if (n == 0)
        return S_FALSE;

    *ppsz = (OLECHAR*)CoTaskMemAlloc((n+1)*sizeof(OLECHAR));
    if (*ppsz==NULL)
        return E_OUTOFMEMORY;

#ifdef _UNICODE
    wcscpy(*ppsz, sz);
#else
    MultiByteToWideChar(CP_ACP, 0, sz, -1, *ppsz, n+1);
#endif

    return S_OK;
}

HRESULT CNetClipServer::XClipboard::GetClipboard(
    /* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDataObject)
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)

    /*
    if (caller does not have write access)
    {
        IDataObject* pdo = NULL;
        HRESULT hr;
        if (SUCCEEDED(hr = OleGetClipboard(&pdo)))
        {
            CGenericDataObject* pgen = new CGenericDataObject(pdo);
            *ppDataObject = (IDataObject*)pgen->GetInterface(IID_IDataObject);
            pdo->Release();
        }
        return hr;
    }
    */

    return OleGetClipboard(ppDataObject);
}

HRESULT CNetClipServer::XClipboard::SetClipboard(
    /* [in] */ IDataObject __RPC_FAR *pDataObject)
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)

    /*
    if (caller does not have write access)
        return E_ACCESSDENIED;
    */

    return OleSetClipboard(pDataObject);
}

HRESULT CNetClipServer::XClipboard::IsCurrentClipboard(
    /* [in] */ IDataObject __RPC_FAR *pDataObject)
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)

    return OleIsCurrentClipboard(pDataObject);
}

HRESULT CNetClipServer::XClipboard::FlushClipboard()
{
    METHOD_PROLOGUE(CNetClipServer, Clipboard)

    return OleFlushClipboard();
}

BEGIN_CONNECTION_MAP(CNetClipServer, CWnd)
    CONNECTION_PART(CNetClipServer, IID_IClipboardNotify, ClipboardNotifyCP)
END_CONNECTION_MAP()

HRESULT CNetClipServer::SendOnClipboardChanged()
{
    const CPtrArray* pConnections = m_xClipboardNotifyCP.GetConnections();
    ASSERT(pConnections != NULL);

    int cConnections = (int)pConnections->GetSize();
    IClipboardNotify* pSink;
    for (int i = 0; i < cConnections; i++)
    {
        pSink = (IClipboardNotify*)(pConnections->GetAt(i));
        ASSERT(pSink != NULL);
        pSink->OnClipboardChanged();
    }
    return S_OK;
}

