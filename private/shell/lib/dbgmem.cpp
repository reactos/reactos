
#ifdef DEBUG

#include <dbgmem.h>

//EXTERN_C BOOL g_bUseNewLeakDetection;
BOOL          g_bRegistered = FALSE;

HINSTANCE g_hinstDBG;
#define   SHELLDBG    "ShellDBG.DLL"

STDAPI_(void) _GetProcFromDLL(HMODULE* phmod, LPCSTR pszDLL, FARPROC* ppfn, LPCSTR pszProc);
// the following are in olestuff.cpp
STDAPI_(void) _DebugMemLeak(UINT flag, LPCTSTR pszFile, UINT iLine);
STDAPI_(void) _add_to_memlist(HMODULE hmod, void *pv, UINT nSize, UINT nType, LPCSTR pszFile, const INT_PTR iLine);
STDAPI_(void) _remove_from_memlist(void *pv);
STDAPI_(void) _remove_from_thread_memlist(DWORD dwThreadId, LPVOID pv);
STDAPI_(void) _transfer_to_thread_memlist(DWORD dwThread, LPVOID pv);
STDAPI_(UINT) _mem_thread_message(void);
STDAPI_(void) _received_for_thread_memlist(DWORD dwFlags, void * pData);
STDAPI_(void) _register_intelli_dump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns);

STDAPI_(void) MemLeakInit(UINT wFlags, LPCTSTR pszFile, UINT iLine);
STDAPI_(void) MemLeakTerminate(DWORD dwFlags);

// should be in olestuff.cpp of each dll
STDAPI_(void) DumpLeakedMemory(PLEAKMEMINFO pmeminfo);
STDAPI_(LPWSTR) GetLeakSymbolicName(PLEAKMEMINFO pmeminfo, LPWSTR pwszBuf, int cchBuf);

static const INTELLILEAKDUMPCBFUNCTIONS c_ildcbf = {
    DumpLeakedMemory,
    GetLeakSymbolicName
};

#define DELAY_LOAD_LEAK_VOID(_fn, _ord, _args, _nargs)  \
void __stdcall _fn _args                                \
{                                                       \
    static void (__stdcall *_pfn##_fn) _args = NULL;    \
    if (g_bUseNewLeakDetection)                         \
    {                                                   \
        if (!g_bRegistered)                             \
        {                                               \
            register_intelli_dump(HINST_THISDLL, &c_ildcbf); \
            g_bRegistered = TRUE;                       \
        }                                               \
        _GetProcFromDLL(&g_hinstDBG, SHELLDBG, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    }                           \
    else                         \
        _pfn##_fn = _##_fn;       \
    if (_pfn##_fn)               \
        _pfn##_fn _nargs;        \
    return;                      \
}

#define DELAY_LOAD_LEAK_FN(_ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                            \
{                                                       \
    static _ret (__stdcall *_pfn##_fn) _args = NULL;    \
    if (g_bUseNewLeakDetection)                         \
    {                                                   \
        if (!g_bRegistered)                             \
        {                                               \
            register_intelli_dump(HINST_THISDLL, &c_ildcbf); \
            g_bRegistered = TRUE;                       \
        }                                               \
        _GetProcFromDLL(&g_hinstDBG, SHELLDBG, (FARPROC*)&_pfn##_fn, (LPCSTR)_ord); \
    }                            \
    else                         \
        _pfn##_fn = _##_fn;       \
    if (_pfn##_fn)               \
        return _pfn##_fn _nargs;        \
    return (_ret)_err;                      \
}

#define _MemLeakInit _DebugMemLeak // for delay load macro to work
DELAY_LOAD_LEAK_VOID(MemLeakInit, 2, 
                     (UINT wFlags, LPCTSTR pszFile, UINT iLine), 
                     (wFlags, pszFile, iLine))

// does nothing for now because MemLeakInit is called to terminate leak detection as well
STDAPI_(void) MemLeakTerminate(DWORD dwFlags)
{
}

DELAY_LOAD_LEAK_VOID(add_to_memlist, 3, 
                     (HMODULE hmod, void *pv, UINT nSize, UINT nType, LPCSTR pszFile, const INT_PTR iLine),
                     (hmod, pv, nSize, nType, pszFile, iLine))
DELAY_LOAD_LEAK_FN(UINT, mem_thread_message, 4, (void), (), 0)
DELAY_LOAD_LEAK_VOID(received_for_thread_memlist, 5, (DWORD dwFlags, void *pData), (dwFlags, pData))
DELAY_LOAD_LEAK_VOID(remove_from_memlist, 6, (void *pv), (pv))
DELAY_LOAD_LEAK_VOID(remove_from_thread_memlist, 7, (DWORD dwThreadId, LPVOID pv), (dwThreadId, pv))
DELAY_LOAD_LEAK_VOID(transfer_to_thread_memlist, 8, (DWORD dwThreadId, LPVOID pv), (dwThreadId, pv))

//DELAY_LOAD_LEAK_VOID(register_intelli_dump, 9, (HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns), (hmod, pfns))
void register_intelli_dump(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns)
{
    static void (__stdcall *pfnregister_intelli_dump)(HMODULE hmod, const INTELLILEAKDUMPCBFUNCTIONS *pfns) = NULL;

    if (!pfnregister_intelli_dump)
    {
        if (g_bUseNewLeakDetection)
            _GetProcFromDLL(&g_hinstDBG, SHELLDBG, (FARPROC*)&pfnregister_intelli_dump, (LPCSTR)9);
        else
            pfnregister_intelli_dump = _register_intelli_dump;
    }

    if (pfnregister_intelli_dump)
        pfnregister_intelli_dump(hmod, pfns);
}

#endif //DEBUG
