#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <tchar.h>

#ifdef _FEATURE_SHELLEX

#include <shlobj.h>

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#pragma data_seg()

#include "shellex.h"

CShellEx::CShellEx()
{
#ifdef _DEBUG
    OutputDebugString(_T("CShellEx::CShellEx\r\n"));
#endif

    m_dwRefCount=0;
	m_punkInner=NULL;
	m_punkOuter=NULL;
}

HRESULT CShellEx::Initialize(IUnknown* punkOuter) {
	try
	{
		m_punkInner=new CInnerUnk(this);
	}
	catch(...)
	{
		m_punkInner=NULL;
		return E_OUTOFMEMORY;
	}

	if (punkOuter) 
	{
		m_punkOuter=punkOuter;
	}
	else
	{
		m_punkOuter=(IUnknown*) m_punkInner;
	}

	return S_OK;
}

CShellEx::~CShellEx() 
{
#ifdef _DEBUG
    OutputDebugString(_T("CShellEx::~CShellEx\r\n"));
#endif

	if (m_punkInner)
	{
		delete m_punkInner;
	}
}

HRESULT CShellEx::QueryInterface(REFIID riid, void** ppObject) {
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
		return E_POINTER;
	}
	return m_punkOuter->QueryInterface(riid, ppObject);
}

ULONG CShellEx::AddRef() 
{
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
		return 0;
	}
	return m_punkOuter->AddRef();
}

ULONG CShellEx::Release()
{
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
		return 0;
	}
	return m_punkOuter->Release();
}

CShellEx::CInnerUnk::CInnerUnk(CShellEx* pObj)
{
	m_pObj=pObj;
}

HRESULT CShellEx::CInnerUnk::QueryInterface(REFIID riid, void** ppObject) 
{
#ifdef _DEBUG
    OutputDebugString(_T("CShellEx::CInnerUnk::QueryInterface\r\n"));
#endif
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
#ifdef _DEBUG
        OutputDebugString(_T("CShellEx::CInnerUnk::QueryInterface failed E_POINTER\r\n"));
#endif
		return E_POINTER;
	}
    else if (riid==IID_IUnknown || riid== IID_IShellExtInit) 
    {
		*ppObject=(IShellExtInit*) m_pObj;
    }
    else if (riid==IID_IDropTarget)
	{
		*ppObject=(IDropTarget*) m_pObj;
	}
    else if (riid==IID_IExtractIcon)
	{
		*ppObject=(IExtractIcon*) m_pObj;
	}
    else if (riid==IID_IContextMenu)
	{
		*ppObject=(IContextMenu*) m_pObj;
	}
	else 
	{
#ifdef _DEBUG
        WCHAR sz[256];
        StringFromGUID2(riid, sz, 256);
        OutputDebugString(_T("CShellEx::CInnerUnk::QueryInterface failed E_NOINTERFACE: "));
        OutputDebugStringW(sz);
        OutputDebugString(_T("\r\n"));
#endif
		return E_NOINTERFACE;
	}
	m_pObj->AddRef();
	return S_OK;
}

ULONG CShellEx::CInnerUnk::AddRef() 
{
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
		return 0;
	}
	InterlockedIncrement((long*) &g_dwRefCount);
	InterlockedIncrement((long*) &m_pObj->m_dwRefCount);
	return m_pObj->m_dwRefCount;
}

ULONG CShellEx::CInnerUnk::Release()
{
	if (IsBadWritePtr(this, sizeof(*this))) 
	{
		return 0;
	}
	ULONG dwRefCount=(m_pObj->m_dwRefCount)-1;
	InterlockedDecrement((long*) &g_dwRefCount);
	if (InterlockedDecrement((long*) &m_pObj->m_dwRefCount)==0) 
	{
		delete m_pObj;
		return 0;
	}
	return dwRefCount;
}

// IShellExtInit::Initialize
HRESULT CShellEx::Initialize(LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
#ifdef _DEBUG
    OutputDebugString(_T("CShellEx::Initialize\r\n"));
#endif
    return S_OK;
}

// IDropTarget::DragEnter
HRESULT CShellEx::DragEnter( 
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY ;
    return S_OK;
}

// IDropTarget::DragOver
HRESULT CShellEx::DragOver( 
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
    // Prevent d&d'ing on self

    // TODO: We should check to make sure the dataobject passed in
    // has a format we can support.
    //

    // check for force link
    if ((grfKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
        *pdwEffect= DROPEFFECT_LINK;
    // check for force copy
    else if ((grfKeyState & MK_CONTROL) == MK_CONTROL)
        *pdwEffect= DROPEFFECT_COPY;
    // check for force move
    else if ((grfKeyState & MK_ALT) == MK_ALT)
        *pdwEffect= DROPEFFECT_MOVE;
    // default -- recommended action is move
    else
        *pdwEffect= DROPEFFECT_MOVE;
    return S_OK;
}

// IDropTarget::DragLeave
HRESULT CShellEx::DragLeave( void)
{
    return S_OK;
}

// IDropTarget::Drop
HRESULT CShellEx::Drop( 
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
    return S_OK;
}

// IExtractIcon::GetIconLocation
HRESULT CShellEx::GetIconLocation(UINT uFlags,LPSTR  szIconFile,UINT   cchMax,int   * piIndex,UINT  * pwFlags)
{
#ifdef _DEBUG
    OutputDebugString(_T("CShellEx::GetIconLocation\n\r"));
#endif
	HMODULE hModule;
	hModule=GetModuleHandle(_T("nclipps.DLL"));
	if (!hModule) 
	{
#ifdef _DEBUG
        OutputDebugString(_T("GetModuleHandle failed in CShellEx::GetIconLocation\n\r"));
#endif
        return E_UNEXPECTED;
    }

	if (GetModuleFileName(hModule, szIconFile, cchMax)==0) 
	{
#ifdef _DEBUG
        OutputDebugString(_T("GetModuleFileName failed in CShellEx::GetIconLocation\n\r"));
#endif
        return E_UNEXPECTED;
    }

    *piIndex = 0;
    *pwFlags = GIL_PERINSTANCE;
    
    return S_OK;
}

// IContextMenu::GetCommandString
HRESULT CShellEx::GetCommandString(UINT idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

// IContextMenu::InvokeCommand
HRESULT CShellEx::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    return E_NOTIMPL;
}

// IContextMenu::QueryContextMenu
HRESULT CShellEx::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) 
{ 
    return E_NOTIMPL;
}


#endif

