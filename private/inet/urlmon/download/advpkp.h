#ifdef __cplusplus

// ADVPACK.DLL delay load

class CRunSetupHook
{
    public:

#ifdef WX86
#define DELAYRSCAPI(_fn, _args, _nargs) \
    DWORD _fn _args { \
        HRESULT hres = Init(); \
        DWORD dwRet = (DWORD)hres; \
        if (SUCCEEDED(hres)) { \
            dwRet = _pfn##_fn _nargs; \
        } \
        return dwRet;    } \
    DWORD (STDAPICALLTYPE* _pfn##_fn) _args; \
    DWORD _fn##X86 _args { \
        HRESULT hres = InitX86(); \
        DWORD dwRet = (DWORD)hres; \
        if (SUCCEEDED(hres)) { \
            dwRet = _pfn##_fn##X86 _nargs; \
        } \
        return dwRet;    } \
    DWORD (STDAPICALLTYPE* _pfn##_fn##X86) _args;
#else
#define DELAYRSCAPI(_fn, _args, _nargs) \
    DWORD _fn _args { \
        HRESULT hres = Init(); \
        DWORD dwRet = (DWORD)hres; \
        if (SUCCEEDED(hres)) { \
            dwRet = _pfn##_fn _nargs; \
        } \
        return dwRet;    } \
    DWORD (STDAPICALLTYPE* _pfn##_fn) _args;
#endif

    HRESULT     Init(void);
#ifdef WX86
    HRESULT     InitX86(void);

    typedef BOOL (*pfnUnloadFn)(HMODULE hMod);
    pfnUnloadFn pfnUnload;
#endif
    CRunSetupHook();
    ~CRunSetupHook();

    BOOL    m_fInited;
    HMODULE m_hMod;
#ifdef WX86
    BOOL    m_fInitedX86;
    HMODULE m_hModX86;
    HMODULE m_hModWx86;
#endif

    DELAYRSCAPI( RunSetupCommand,
    (HWND    hWnd,
    LPSTR   szCmdName,
    LPSTR   szInfSection,
    LPSTR   szDir,
    LPSTR   szTitle,
    HANDLE *phEXE,
    DWORD   dwFlags,
    LPVOID  pvReserved),
    (hWnd, szCmdName, szInfSection, szDir, szTitle, phEXE, dwFlags,pvReserved));

    DELAYRSCAPI( NeedReboot,
    (DWORD dwRebootCheck),
    (dwRebootCheck));

    DELAYRSCAPI( NeedRebootInit, (), ());

    DELAYRSCAPI( TranslateInfString,
    (PCSTR  pszInfFilename,
    PCSTR  pszInstallSection,
    PCSTR  pszTranslateSection,
    PCSTR  pszTranslateKey,
    PSTR   pszBuffer,
    DWORD  dwBufferSize,
    PDWORD pdwRequiredSize,
    PVOID  pvReserved),
    (pszInfFilename, pszInstallSection, pszTranslateSection, pszTranslateKey,
    pszBuffer, dwBufferSize, pdwRequiredSize, pvReserved));


    DELAYRSCAPI( AdvInstallFile,
    (HWND hwnd,
    LPCSTR lpszSourceDir,
    LPCSTR lpszSourceFile,
    LPCSTR lpszDestDir,
    LPCSTR lpszDestFile,
    DWORD dwFlags,
    DWORD dwReserved),
    (hwnd, lpszSourceDir, lpszSourceFile, lpszDestDir, lpszDestFile, dwFlags,
    dwReserved));

};

inline
CRunSetupHook::CRunSetupHook()
{
    m_fInited = FALSE;
    m_hMod = NULL;
#ifdef WX86
    m_hModX86 = m_hModWx86 = NULL;
#endif
}

inline
CRunSetupHook::~CRunSetupHook()
{
    if (m_fInited) {
        if (m_hMod) {
            FreeLibrary(m_hMod);
        }
#ifdef WX86
        if (m_hModX86) {
            if (pfnUnload) {
                (*pfnUnload)(m_hModX86);    // free x86 advpack.dll
            }
        }
        
        if (m_hModWx86) {
            FreeLibrary(m_hModWx86);    // free wx86.dll
        }
#endif
    }
}

inline
HRESULT 
CRunSetupHook::Init(void)
{
    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary( "ADVPACK.DLL" );

    if (!m_hMod) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) return E_UNEXPECTED;

    CHECKAPI(RunSetupCommand);
    CHECKAPI(NeedReboot);
    CHECKAPI(NeedRebootInit);

    CHECKAPI(TranslateInfString);
    CHECKAPI(AdvInstallFile);

    m_fInited = TRUE;
    return S_OK;
}

#ifdef WX86
inline
HRESULT 
CRunSetupHook::InitX86(void)
{
    typedef HMODULE (*pfnLoadFn)(LPCWSTR name, DWORD dwFlags);
    typedef PVOID (*pfnThunkFn)(PVOID pvAddress, PVOID pvCbDispatch, BOOLEAN fNativeToX86);
    pfnLoadFn pfnLoad;
    pfnThunkFn pfnThunk;

    if (m_fInitedX86) {
        return S_OK;
    }

    // No need to check the reg key to see if Wx86 is installed and enabled.
    // By the time this routine runs, that has already been done.
    m_hModWx86 = LoadLibrary("wx86.dll");
    if (!m_hModWx86) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    pfnLoad = (pfnLoadFn)GetProcAddress(m_hModWx86, "Wx86LoadX86Dll");
    pfnUnload = (pfnUnloadFn)GetProcAddress(m_hModWx86, "Wx86FreeX86Dll");
    pfnThunk = (pfnThunkFn)GetProcAddress(m_hModWx86, "Wx86ThunkProc");
    if (!pfnLoad || !pfnThunk || !pfnUnload) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        FreeLibrary(m_hModWx86);
        return hr;
    }

    // Load x86 advpack.dll
    m_hModX86 = (*pfnLoad)( L"ADVPACK.DLL", 0 );

    if (!m_hModX86) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#undef CHECKAPI
#define CHECKAPI(_fn, n) \
    *(FARPROC*)&(_pfn##_fn##X86) = GetProcAddress(m_hModX86, #_fn); \
    if (!(_pfn##_fn##X86)) return E_UNEXPECTED;                  \
    *(FARPROC*)&(_pfn##_fn##X86) = (FARPROC)(*pfnThunk)(_pfn##_fn##X86, (PVOID)n, TRUE);

    CHECKAPI(RunSetupCommand, 8);
    CHECKAPI(NeedReboot, 1);
    CHECKAPI(NeedRebootInit, 0);

    CHECKAPI(TranslateInfString, 8);
    CHECKAPI(AdvInstallFile, 7);

    m_fInitedX86 = TRUE;
    return S_OK;
}
#endif


#endif
