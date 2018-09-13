//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

BOOL WINAPI Sz_AllocCopy(LPCTSTR pszSrc, LPTSTR *ppszDst);
HRESULT PASCAL ICoCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);
BOOL Cursor_Wait(HCURSOR *phcur);
BOOL Cursor_UnWait(HCURSOR hcur);

//----------------------------------------------------------------------------
__inline BOOL LAlloc(UINT cb, PVOID *ppv)
{
    *ppv = (PVOID*)LocalAlloc(LPTR, cb);
    return *ppv ? TRUE : FALSE;
}

//----------------------------------------------------------------------------
__inline BOOL LFree(PVOID pv)
{
    return LocalFree(pv) ? FALSE : TRUE;
}

//----------------------------------------------------------------------------
__inline UINT WINAPI Sz_Cb(LPCTSTR psz)
{
    return (lstrlen(psz)+1)*sizeof(TCHAR);
}


#ifdef DEBUG
//----------------------------------------------------------------------------
void WINAPI AssertFailed(LPCTSTR szFile, int line);
void __cdecl _AssertMsg(BOOL f, LPCTSTR pszMsg, ...);
void __cdecl _Dbg(LPCTSTR psz, ...);

#define Dbg             _Dbg
#define Dbg_OpenLog()
#define Dbg_CloseLog()

#define Assert(f)                               \
{                                               \
    if (!(f))                                   \
        AssertFailed(TEXT(__FILE__), __LINE__); \
}

#else
//----------------------------------------------------------------------------
#define Assert(f)
#ifdef NO_LOGGING
#define Dbg        	    1 ? (void)0 : (void)
#define AssertMsg       1 ? (void)0 : (void)
#define DebugBreak()
#define Dbg_OpenLog     1 ? (void)0 : (void)
#define Dbg_CloseLog    1 ? (void)0 : (void)
#else
void __cdecl _Dbg(LPCTSTR psz, ...);
BOOL WINAPI _Dbg_OpenLog(void);
BOOL WINAPI _Dbg_CloseLog(void);
#define Dbg             _Dbg
#define Dbg_OpenLog     _Dbg_OpenLog
#define Dbg_CloseLog    _Dbg_CloseLog
#endif
#endif
