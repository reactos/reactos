#ifndef __ncshellex_h__
#define __ncshellex_h__

class CShellExFactory;

class CShellEx : public IShellExtInit, IDropTarget, IContextMenu
{
public:
	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	ULONG	_stdcall AddRef();
	ULONG	_stdcall Release();

    // IShellExtInit methods 
    HRESULT _stdcall Initialize(LPCITEMIDLIST pidlFolder,LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    HRESULT __stdcall DragEnter( 
        /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD __RPC_FAR *pdwEffect);
    
    HRESULT __stdcall DragOver( 
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD __RPC_FAR *pdwEffect);
    
    HRESULT __stdcall DragLeave( void);
    
    HRESULT __stdcall Drop( 
        /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
        /* [in] */ DWORD grfKeyState,
        /* [in] */ POINTL pt,
        /* [out][in] */ DWORD __RPC_FAR *pdwEffect);

    // *** IExtractIcon methods ***
    HRESULT __stdcall GetIconLocation(
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags);

	// IContextMenu methods
    HRESULT _stdcall QueryContextMenu(HMENU hMenu,UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    HRESULT _stdcall InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    HRESULT _stdcall GetCommandString(UINT idCmd, UINT uFlags,UINT *pwReserved, LPSTR pszName, UINT cchMax);
    
    friend CShellExFactory;

private:

	CShellEx();
	HRESULT Initialize(IUnknown* punkOuter);
	~CShellEx();

	class CInnerUnk : public IUnknown 
	{
	public:
		// IUnknown methods
		HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
		ULONG	_stdcall AddRef();
		ULONG	_stdcall Release();

		CInnerUnk(CShellEx* pObj);

		CShellEx* m_pObj;
	} *m_punkInner;
	friend CInnerUnk;

	IUnknown* m_punkOuter;
	ULONG m_dwRefCount;
};

extern ULONG g_dwRefCount;

#endif
