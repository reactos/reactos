#ifdef __cplusplus

// VERSION.DLL delay load

class CVersion
{
    public:
#define DELAYVERAPI(_fn, _args, _nargs) \
    DWORD _fn _args { \
        HRESULT hres = Init(); \
        DWORD dwRet; \
        if (SUCCEEDED(hres)) { \
            dwRet = _pfn##_fn _nargs; \
        } \
        return dwRet;    } \
    DWORD (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT     Init(void);
    CVersion();
    ~CVersion();

    BOOL    m_fInited;
    HMODULE m_hMod;

    DELAYVERAPI( VerInstallFileA,
        (DWORD uFlags,
        LPSTR szSrcFileName,
        LPSTR szDestFileName,
        LPSTR szSrcDir,
        LPSTR szDestDir,
        LPSTR szCurDir,
        LPSTR szTmpFile,
        PUINT lpuTmpFileLen),
        (uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir,
        szCurDir, szTmpFile, lpuTmpFileLen));


    DELAYVERAPI( VerQueryValueA,
        (const LPVOID pBlock,
        LPSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen),
        (pBlock, lpSubBlock, lplpBuffer, puLen));

    DELAYVERAPI( GetFileVersionInfoA,
        (LPSTR lptstrFilename, 
        DWORD dwHandle,         
        DWORD dwLen,            
        LPVOID lpData),
        (lptstrFilename, dwHandle, dwLen, lpData));                      

    DELAYVERAPI( GetFileVersionInfoSizeA,
        (LPSTR lptstrFilename,
        LPDWORD lpdwHandle),
        (lptstrFilename, lpdwHandle));


};

inline
CVersion::CVersion()
{
    m_fInited = FALSE;
}

inline
CVersion::~CVersion()
{
    if (m_fInited) {
        FreeLibrary(m_hMod);
    }
}

inline
HRESULT 
CVersion::Init(void)
{
    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary( "VERSION.DLL" );

    if (!m_hMod) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) return E_UNEXPECTED;

    CHECKAPI(VerInstallFileA);
    CHECKAPI(VerQueryValueA);
    CHECKAPI(GetFileVersionInfoSizeA);
    CHECKAPI(GetFileVersionInfoA);

    m_fInited = TRUE;
    return S_OK;
}

#endif
