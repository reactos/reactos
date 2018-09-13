// 5.0 Version

// this is an inline function to allow debugger components to
// do the equivalent of theApp.FindInterface. As it is unlikely
// to be actually inlined, the idea is that is one call to this per component.

// You may experience trouble pulling in enough headers to get this
// and its caller to build. I can't give any advice, except to look at
// those components which have been suitably modified [apennell]

// In order for a debugger component to be loaded, the DBG package
// is already loaded (as it is responsible for loading and unloading
// them) so we dynabind to it.

#ifdef __cplusplus
extern "C" {
#endif

__inline HRESULT DebuggerFindInterface( REFIID refid, LPVOID FAR* lpIface )
{
	typedef HRESULT (__cdecl *PFNFINDINT)( REFIID, LPVOID FAR*);
	static HINSTANCE hInst;
	static PFNFINDINT pFindInterface;
	static BOOL bFailed;

	if (bFailed)
		return S_FALSE;

	if (hInst==NULL)
	{
#if defined(DEBUGVER) || defined(_DEBUG) || defined(DEBUG)
		hInst = GetModuleHandle( "DEVDBGD.PKG" );
#else
		hInst = GetModuleHandle( "DEVDBG.PKG" );
#endif
	}
	if (pFindInterface==NULL)
		pFindInterface = (PFNFINDINT) GetProcAddress( hInst, "DbgFindInterface" );
	if (pFindInterface)
		return pFindInterface( refid, lpIface );
	else
	{
		bFailed = TRUE;
		return S_FALSE;
	}
}

#ifdef __cplusplus
} // extern "C" {
#endif
