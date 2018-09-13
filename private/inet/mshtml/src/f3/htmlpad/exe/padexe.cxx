#include <windows.h>
#include <w4warn.h>
#include <mshtmdbg.h>

#if defined(_M_IX86)
    #define F3DebugBreak() _asm { int 3 }
#else
    #define F3DebugBreak() DebugBreak()
#endif

void NonCrtStrCat(LPSTR pszDst, LPCSTR pszSrc)
{
    while (*pszDst)
        ++pszDst;

    while (*pszSrc)
        *pszDst++ = *pszSrc++;

    *pszDst = 0;
}

class CPadMallocSpy : public IMallocSpy
{
public:

    CPadMallocSpy() { _pSpyFwd = 0; }
    void * __cdecl operator new(size_t cb) { return(LocalAlloc(LPTR, cb)); } \
    void __cdecl operator delete(void * pv) { LocalFree(pv); }

    // IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IMallocSpy methods

    STDMETHOD_(SIZE_T,  PreAlloc)(SIZE_T cbRequest);
    STDMETHOD_(void *, PostAlloc)(void *pvActual);
    STDMETHOD_(void *, PreFree)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(void,   PostFree)(BOOL fSpyed);
    STDMETHOD_(SIZE_T,  PreRealloc)(void *pvRequest, SIZE_T cbRequest, void **ppvActual, BOOL fSpyed);
    STDMETHOD_(void *, PostRealloc)(void *pvActual, BOOL fSpyed);
    STDMETHOD_(void *, PreGetSize)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(SIZE_T,  PostGetSize)(SIZE_T cbActual, BOOL fSpyed);
    STDMETHOD_(void *, PreDidAlloc)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(BOOL,   PostDidAlloc)(void *pvRequest, BOOL fSpyed, BOOL fActual);
    STDMETHOD_(void,   PreHeapMinimize)();
    STDMETHOD_(void,   PostHeapMinimize)();

    IMallocSpy *    _pSpyFwd;
};

STDMETHODIMP
CPadMallocSpy::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IMallocSpy)
    {
        *ppv = (IMallocSpy *)this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CPadMallocSpy::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG)
CPadMallocSpy::Release()
{
    return 1;
}

STDMETHODIMP_(SIZE_T)
CPadMallocSpy::PreAlloc(SIZE_T cbRequest)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PreAlloc(cbRequest));
    else
        return(cbRequest);
}

STDMETHODIMP_(void *)
CPadMallocSpy::PostAlloc(void *pvActual)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PostAlloc(pvActual));
    else
        return(pvActual);
}

STDMETHODIMP_(void *)
CPadMallocSpy::PreFree(void *pvRequest, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PreFree(pvRequest, fSpyed));
    else
        return(NULL);
}

STDMETHODIMP_(void)
CPadMallocSpy::PostFree(BOOL fSpyed)
{
    if (_pSpyFwd)
        _pSpyFwd->PostFree(fSpyed);
}

STDMETHODIMP_(SIZE_T)
CPadMallocSpy::PreRealloc(void *pvRequest, SIZE_T cbRequest, void **ppvActual, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PreRealloc(pvRequest, cbRequest, ppvActual, fSpyed));
    else
    {
        *ppvActual = pvRequest;
        return(cbRequest);
    }
}

STDMETHODIMP_(void *)
CPadMallocSpy::PostRealloc(void *pvActual, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PostRealloc(pvActual, fSpyed));
    else
        return(NULL);
}

STDMETHODIMP_(void *)
CPadMallocSpy::PreGetSize(void *pvRequest, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PreGetSize(pvRequest, fSpyed));
    else
        return(NULL);
}

STDMETHODIMP_(SIZE_T)
CPadMallocSpy::PostGetSize(SIZE_T cbActual, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PostGetSize(cbActual, fSpyed));
    else
        return(0);
}

STDMETHODIMP_(void *)
CPadMallocSpy::PreDidAlloc(void *pvRequest, BOOL fSpyed)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PreDidAlloc(pvRequest, fSpyed));
    else
        return(NULL);
}

STDMETHODIMP_(BOOL)
CPadMallocSpy::PostDidAlloc(void *pvRequest, BOOL fSpyed, BOOL fActual)
{
    if (_pSpyFwd)
        return(_pSpyFwd->PostDidAlloc(pvRequest, fSpyed, fActual));
    else
        return(FALSE);
}

STDMETHODIMP_(void)
CPadMallocSpy::PreHeapMinimize()
{
}

STDMETHODIMP_(void)
CPadMallocSpy::PostHeapMinimize()
{
}

typedef int (WINAPI * PFNPADMAIN)(int argc, char ** argv, IMallocSpy * pSpy);
typedef DWORD (WINAPI * PFNDBGEXGETVERSION)();
typedef TRACETAG (WINAPI * PFNTAGREGISTEROTHER)(char * szOwner, char * szDesc, BOOL fEnabled);
typedef void (WINAPI * PFNDBGCOMEMORYTRACKDISABLE)(BOOL fDisable);
typedef void * (WINAPI * PFNDBGGETMALLOCSPY)();
typedef void (WINAPI * PFNRESTOREDEFAULTDEBUGSTATE)();
typedef BOOL (WINAPI * PFNISTAGENABLED)(TRACETAG tag);
typedef BOOL (WINAPI * PFNASSERTIMPL)(char const * szFile, int iLine, char const * szMessage);
typedef BOOL (WINAPI * PFNENUMPROCESSMODULES)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef DWORD (WINAPI * PFNGETMODULEBASENAMEA)(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize);

#ifdef _M_IA64
//$ WIN64: Why is there unreachable code in the retail build of this next function for IA64?
#pragma warning(disable:4702) /* unreachable code */
#endif

extern "C"
int WINAPI mainCRTStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    CPadMallocSpy *             g_pSpy = new CPadMallocSpy;
    HINSTANCE                   hInstPad    = NULL;
    HINSTANCE                   hInstDbg    = NULL;
    HINSTANCE                   hInstPSAPI  = NULL;
    PFNPADMAIN                  pfnPadMain;
    PFNDBGEXGETVERSION          pfnDbgExGetVersion = NULL;
    PFNTAGREGISTEROTHER         pfnTagRegisterOther = NULL;
    PFNDBGCOMEMORYTRACKDISABLE  pfnDbgCoMemoryTrackDisable = NULL;
    PFNDBGGETMALLOCSPY          pfnDbgGetMallocSpy = NULL;
    PFNRESTOREDEFAULTDEBUGSTATE pfnRestoreDefaultDebugState = NULL;
    PFNISTAGENABLED             pfnIsTagEnabled = NULL;
    PFNASSERTIMPL               pfnAssertImpl = NULL;
    PFNENUMPROCESSMODULES       pfnEnumProcessModules = NULL;
    PFNGETMODULEBASENAMEA       pfnGetModuleBaseNameA = NULL;
    TRACETAG                    tagCoTrack = 0;
    TRACETAG                    tagCoTrackDisable = 0;
    TRACETAG                    tagModuleLeaks = 0;
    HMODULE                     ahModuleBeg[64];
    UINT                        chModuleBeg = 0;
    HMODULE                     ahModuleEnd[64];
    UINT                        chModuleEnd = 0;
    HMODULE                     ahModuleLeak[64];
    UINT                        chModuleLeak = 0;
    DWORD                       cb;
    HMODULE                     hModule;
    int                         argc = 0;
    char *                      argv[64] = { 0 };
    char *                      pch = GetCommandLineA();
    char                        chDelim;

    // Parse command line manually

    while (*pch)
    {
        while (*pch == ' ') ++pch;
		
		if (*pch == '\0')
			break;

        if (*pch == '"')
        {
            chDelim = '"';
            ++pch;
        }
        else
        {
            chDelim = ' ';
        }

        if (argc == 64)
            break;

        argv[argc++] = pch;

        while (*pch != chDelim && *pch != '\0') ++pch;

        if (*pch == chDelim)
            *pch++ = '\0';
    }

    hInstPSAPI = LoadLibraryA("PSAPI.DLL");

    if (hInstPSAPI)
    {
        pfnEnumProcessModules = (PFNENUMPROCESSMODULES)GetProcAddress(hInstPSAPI, "EnumProcessModules");
        pfnGetModuleBaseNameA = (PFNGETMODULEBASENAMEA)GetProcAddress(hInstPSAPI, "GetModuleBaseNameA");

        if (pfnEnumProcessModules && pfnEnumProcessModules(GetCurrentProcess(), ahModuleBeg, sizeof(ahModuleBeg), &cb))
        {
            chModuleBeg = cb / sizeof(HMODULE);
        }
    }

    hInstDbg = LoadLibraryA("mshtmdbg.dll");

    if (hInstDbg)
    {
        pfnDbgExGetVersion          = (PFNDBGEXGETVERSION)GetProcAddress(hInstDbg, "DbgExGetVersion");
        pfnTagRegisterOther         = (PFNTAGREGISTEROTHER)GetProcAddress(hInstDbg, "DbgExTagRegisterOther");
        pfnDbgCoMemoryTrackDisable  = (PFNDBGCOMEMORYTRACKDISABLE)GetProcAddress(hInstDbg, "DbgExCoMemoryTrackDisable");
        pfnDbgGetMallocSpy          = (PFNDBGGETMALLOCSPY)GetProcAddress(hInstDbg, "DbgExGetMallocSpy");
        pfnRestoreDefaultDebugState = (PFNRESTOREDEFAULTDEBUGSTATE)GetProcAddress(hInstDbg, "DbgExRestoreDefaultDebugState");
        pfnIsTagEnabled             = (PFNISTAGENABLED)GetProcAddress(hInstDbg, "DbgExIsTagEnabled");
        pfnAssertImpl               = (PFNASSERTIMPL)GetProcAddress(hInstDbg, "DbgExAssertImpl");

        DWORD dwVer = pfnDbgExGetVersion ? pfnDbgExGetVersion() : 0;

        if (pfnDbgExGetVersion)
        {
            if (dwVer != MSHTMDBG_API_VERSION)
            {
                OutputDebugStringA("MSHTMPAD: Version mismatch for MSHTMDBG.DLL.  Continuing without it.\r\n");
                FreeLibrary(hInstDbg);
                hInstDbg = NULL;
            }
        }

        if (    hInstDbg
            &&  (   !pfnTagRegisterOther
                ||  !pfnDbgCoMemoryTrackDisable
                ||  !pfnDbgGetMallocSpy
                ||  !pfnRestoreDefaultDebugState
                ||  !pfnIsTagEnabled))
        {
            OutputDebugStringA("MSHTMPAD: Can't find one or more entrypoints in MSHTMDBG.DLL.  Continuing without it.\r\n");
            FreeLibrary(hInstDbg);
            hInstDbg = NULL;
        }

        if (hInstDbg)
        {
            tagCoTrack = pfnTagRegisterOther("!Memory", "Track all CoMemory leaks", FALSE);
            tagCoTrackDisable = pfnTagRegisterOther("!Memory", "Do not register CoMemory spy (MUST RESTART EXE)", FALSE);
            tagModuleLeaks = pfnTagRegisterOther("!Memory", "Leaks: Assert on module leaks", FALSE);
            pfnRestoreDefaultDebugState();
        }
    }
    else
    {
        OutputDebugStringA("MSHTMPAD: Can't find MSHTMDBG.DLL.  Continuing without it.\r\n");
    }
    
#if !defined(PRODUCT_PROF)
    if (g_pSpy && hInstDbg && !pfnIsTagEnabled(tagCoTrackDisable))
    {
        g_pSpy->_pSpyFwd = (IMallocSpy *)pfnDbgGetMallocSpy();
    }
#endif

    if (hInstDbg && !pfnIsTagEnabled(tagCoTrack))
    {
        pfnDbgCoMemoryTrackDisable(TRUE);

        //$BUGBUG Not ready for prime-time yet
        pfnDbgCoMemoryTrackDisable(TRUE);
    }

    hInstPad = LoadLibraryA("htmlpad.dll");

    if (hInstPad == NULL)
    {
        OutputDebugStringA("MSHTMPAD: Unable to load HTMLPAD.DLL\r\n");
        ExitProcess(1);
    }

    pfnPadMain = (PFNPADMAIN)GetProcAddress(hInstPad, "PadMain");

    if (!pfnPadMain)
    {
        OutputDebugStringA("MSHTMPAD: Can't find PadMain entrypoint in HTMLPAD.DLL\r\n");
        ExitProcess(1);
    }

    pfnPadMain(argc, argv, (g_pSpy && g_pSpy->_pSpyFwd) ? g_pSpy : NULL);

    FreeLibrary(hInstPad);

    if (hInstDbg && pfnIsTagEnabled(tagModuleLeaks))
    {
        if (pfnEnumProcessModules && pfnEnumProcessModules(GetCurrentProcess(), ahModuleEnd, sizeof(ahModuleEnd), &cb))
        {
            chModuleEnd = cb / sizeof(HMODULE);
        }

        if (chModuleEnd)
        {
            while (chModuleEnd > 0)
            {
                chModuleEnd -= 1;

                hModule = ahModuleEnd[chModuleEnd];

                for (cb = 0; cb < chModuleBeg; ++cb)
                {
                    if (ahModuleBeg[cb] == hModule)
                        break;
                }

                if (cb == chModuleBeg && hModule != (HMODULE)hInstDbg)
                {
                    ahModuleLeak[chModuleLeak++] = hModule;
                }
            }

            if (chModuleLeak)
            {
                char achLeakMsg[2048];
                char achModule[128];

                achLeakMsg[0] = 0;

                NonCrtStrCat(achLeakMsg, "The following modules were leaked: ");

                for (cb = 0; cb < chModuleLeak; ++cb)
                {
                    if (!pfnGetModuleBaseNameA(GetCurrentProcess(), ahModuleLeak[cb], achModule, sizeof(achModule)))
                        NonCrtStrCat(achLeakMsg, "<unknown>");
                    if (cb > 0)
                        NonCrtStrCat(achLeakMsg, ", ");
                    NonCrtStrCat(achLeakMsg, achModule);
                }

                if (pfnAssertImpl(__FILE__, __LINE__, achLeakMsg))
                    F3DebugBreak();
            }
        }
    }

	if (hInstPSAPI)
	{
		FreeLibrary(hInstPSAPI);
	}

    if (hInstDbg)
    {
        FreeLibrary(hInstDbg);
    }

    g_pSpy->_pSpyFwd = NULL;

    ExitProcess(0);
    return(0);
}

#ifdef _M_IA64
#pragma warning(default:4702) /* unreachable code */
#endif

#ifdef UNIX
// IEUNIX uses WinMain as program entry.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return mainCRTStartup(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#endif
