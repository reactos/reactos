#include "precomp.h"
#ifndef SHCIDS_CANONICALONLY
#define SHCIDS_CANONICALONLY 0x10000000L
#endif

void *operator new(size_t size)
{
    return LocalAlloc(LMEM_ZEROINIT, size);
}

void operator delete(void *p)
{
    LocalFree(p);
}

void operator delete(void *p, UINT_PTR)
{
    LocalFree(p);
}

HRESULT SHELL_GetIDListFromObject(IUnknown *punk, PIDLIST_ABSOLUTE *ppidl)
{
#if DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA && 0 // FIXME: SHELL32 not ready yet
    return SHGetIDListFromObject(punk, ppidl);
#else
    HRESULT hr;
    IPersistFolder2 *pf2;
    if (SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IPersistFolder2, &pf2))))
    {
        hr = pf2->GetCurFolder(ppidl);
        pf2->Release();
    }
    IPersistIDList *pil;
    if (FAILED(hr) && SUCCEEDED(hr = punk->QueryInterface(IID_PPV_ARG(IPersistIDList, &pil))))
    {
        hr = pil->GetIDList(ppidl);
        pil->Release();
    }
    return hr;
#endif
}

static HRESULT SHELL_CompareAbsoluteIDs(LPARAM lParam, PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b)
{
    IShellFolder *psf;
    HRESULT hr = SHGetDesktopFolder(&psf);
    if (FAILED(hr))
        return hr;
    hr = psf->CompareIDs(lParam, a, b);
    psf->Release();
    return hr;
}

BOOL SHELL_IsEqualAbsoluteID(PCIDLIST_ABSOLUTE a, PCIDLIST_ABSOLUTE b)
{
    return !SHELL_CompareAbsoluteIDs(SHCIDS_CANONICALONLY, a, b);
}

BOOL SHELL_IsVerb(IContextMenu *pcm, UINT_PTR idCmd, LPCWSTR Verb)
{
    HRESULT hr;
    WCHAR wide[MAX_PATH];
    if (SUCCEEDED(hr = pcm->GetCommandString(idCmd, GCS_VERBW, NULL, (LPSTR)wide, _countof(wide))))
        return !lstrcmpiW(wide, Verb);

    CHAR ansi[_countof(wide)], buf[MAX_PATH];
    if (SHUnicodeToAnsi(Verb, buf, _countof(buf)))
    {
        if (SUCCEEDED(hr = pcm->GetCommandString(idCmd, GCS_VERBA, NULL, ansi, _countof(ansi))))
            return !lstrcmpiA(ansi, buf);
    }
    return FALSE;
}
