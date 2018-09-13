#include "priv.h"
#include "unicwrap.h"

/*****************************************************************************\
    FUNCTION: SHLoadRegUIString

    DESCRIPTION:
        loads the data from the value given the hkey and
        pszValue. if the data is of the form:

        @[path\]<dllname>,-<strId>

        the string with id <strId> from <dllname> will be
        loaded. if not explicit path is provided then the
        dll will be chosen according to pluggable UI
        specifications, if possible.

        if the value's data doesn't yield a successful
        string load, then the data itself is returned

    NOTE:
        These strings are always loaded with cross codepage support.

    WARNING:
        This function can end up calling LoadLibrary and FreeLibrary.
        Therefore, you must not call SHLoadRegUIString during process
        attach or process detach.

    PARAMETERS:
        hkey        - hkey of where to look for pszValue
        pszValue    - value with text string or indirector (see above) to use
        pszOutBuf   - buffer in which to return the data or indirected string
        cchOutBuf   - size of pszOutBuf
\*****************************************************************************/

LANGID GetNormalizedLangId(DWORD dwFlag);
HRESULT _LoadIndirectString(LPWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf, LPWSTR * ppszUpdate);

STDAPI
SHLoadRegUIStringW(HKEY     hkey,
                   LPCWSTR  pszValue,
                   LPWSTR   pszOutBuf,
                   UINT     cchOutBuf)
{
    HRESULT hr;

    RIP(hkey != NULL);
    RIP(hkey != INVALID_HANDLE_VALUE);
    RIP(IS_VALID_STRING_PTRW(pszValue, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, WCHAR, cchOutBuf));

    DEBUGWhackPathBufferW(pszOutBuf, cchOutBuf);

    // Lots of people (regfldr.cpp, for example)
    // assume they'll get back an empty string on failure,
    // so let's give the public what it wants
    if (cchOutBuf)
        pszOutBuf[0] = 0;

    hr = E_INVALIDARG;

    if (hkey != INVALID_HANDLE_VALUE &&
        hkey != NULL &&
        pszValue != NULL &&
        pszOutBuf != NULL)
    {
        DWORD   cb;
        DWORD   dwRet;
        WCHAR * pszValueDataBuf;

        hr = E_FAIL;

        // first try to get the indirected text which will
        // point to a string id in a dll somewhere... this
        // allows plugUI enabled registry UI strings

        pszValueDataBuf = pszOutBuf;
        cb = cchOutBuf * sizeof(pszOutBuf[0]);

        dwRet = SHQueryValueExW(hkey, pszValue, NULL, NULL, (LPBYTE)pszValueDataBuf, &cb);
        if (dwRet == ERROR_SUCCESS || dwRet == ERROR_MORE_DATA)
        {
            BOOL fAlloc;

            fAlloc = (dwRet == ERROR_MORE_DATA);

            // if we didn't have space, this is where we correct the problem.
            // we create a buffer big enough, load the data, and leave
            // ourselves with pszValueDataBuf pointing at a valid buffer
            // containing valid data, exactly what we hoped for in the
            // SHQueryValueExW above

            if (fAlloc)
            {
                pszValueDataBuf = new WCHAR[(cb+1)/2];
                
                if (pszValueDataBuf != NULL)
                {
                    // try to load again... overwriting dwRet on purpose
                    // because we only need to know whether we successfully filled
                    // the buffer at some point (whether then or now)
                    
                    dwRet = SHQueryValueExW(hkey, pszValue, NULL, NULL, (LPBYTE)pszValueDataBuf, &cb);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }                
            }

            // proceed if we succesfully loaded something via one of the
            // two SHQueryValueExW calls.
            // we should have the data we want in a buffer pointed
            // to by pszValueDataBuf.
            
            if (dwRet == ERROR_SUCCESS)
            {
                LPWSTR pszUpdate;

                hr = _LoadIndirectString(pszValueDataBuf, pszOutBuf, cchOutBuf, &pszUpdate);
                if (SUCCEEDED(hr))
                {
                    if (pszUpdate)
                    {
                        SHSetValueW(hkey, NULL, pszValue, REG_SZ, pszUpdate, (lstrlenW(pszUpdate)+1)*2);

                        delete [] pszUpdate;
                    }
                }
                else
                {
                    // the indirect load failed, so we use
                    // the text of the reg string directly

                    if (pszValueDataBuf != pszOutBuf)
                    {
                        StrCpyNW(pszOutBuf, pszValueDataBuf, cchOutBuf);
                    }

                    hr = S_OK;
                }
            }

            if (fAlloc && pszValueDataBuf != NULL)
            {
                delete [] pszValueDataBuf;
            }
        }
    }

    return hr;
}

STDAPI
SHLoadRegUIStringA(HKEY     hkey,
                   LPCSTR   pszValue,
                   LPSTR    pszOutBuf,
                   UINT     cchOutBuf)
{
    HRESULT     hr;

    RIP(hkey != NULL);
    RIP(hkey != INVALID_HANDLE_VALUE);
    RIP(IS_VALID_STRING_PTRA(pszValue, -1));
    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, char, cchOutBuf));

    CStrInW     strV(pszValue);
    CStrOutW    strOut(pszOutBuf, cchOutBuf);

    hr = SHLoadRegUIStringW(hkey, strV, strOut, strOut.BufSize());

    return hr;
}

HRESULT _LoadDllString(LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf)
{
    HRESULT hr = E_FAIL;
    WCHAR * szParseBuf;
    int     nStrId;

    UINT cchSource = lstrlenW(pszSource)+1;

    szParseBuf = new WCHAR[cchSource];
    if (szParseBuf != NULL)
    {
        StrCpyW(szParseBuf, pszSource);

        // see if this is a special string reference.
        // such strings take the form [path\]dllname.dll,-123
        // where 123 is the id of the string resource
        // note that reference by index is not permitted

        nStrId = PathParseIconLocationW(szParseBuf);
        nStrId *= -1;

        if (nStrId > 0)
        {
            LPWSTR      pszDllName;
            HINSTANCE   hinst;
            BOOL        fUsedMLLoadLibrary;

            pszDllName = PathFindFileNameW(szParseBuf);
            ASSERT(pszDllName >= szParseBuf);

            // try loading the dll with MLLoadLibrary, but
            // only if an explicit path was not provided.
            // we assume an explicit path means that
            // the caller knows precisely which dll is needed
            // use MLLoadLibrary first, otherwise we'll miss
            // out chance to have plugUI behavior

            hinst = NULL;
            if (pszDllName == szParseBuf)
            {
                // note: using HINST_THISDLL (below) is sort of a hack because that's
                // techinically supposed to be the *parent* dll's hinstance...
                // however we get called from lots of places and therefore
                // don't know the parent dll, and the hinst for browseui.dll
                // is good enough since all the hinst is really used for is to
                // find the path to check if the install language is the
                // currently selected UI language. this will usually be
                // something like "\winnt\system32"

                hinst = MLLoadLibraryW(pszDllName, HINST_THISDLL, ML_CROSSCODEPAGE);
            }

            if (hinst != NULL)
            {
                fUsedMLLoadLibrary = TRUE;
            }
            else
            {
                fUsedMLLoadLibrary = FALSE;

                // our last chance to load something is if a full
                // path was provided... if there's a full path it
                // will start at the beginning of the szParseBuf buffer

                if (pszDllName > szParseBuf)
                {
                    BOOL    fFileExists;

                    // don't bother if the file isn't there
                    // failling in LoadLibrary is slow
                    fFileExists = PathFileExistsW(szParseBuf);
                    if (fFileExists)
                    {
                        hinst = LoadLibraryWrapW(szParseBuf);
                    }
                }
            }

            if (hinst != NULL)
            {
                int nLSRet;

                // dll found, so load the string

                nLSRet = LoadStringWrapW(hinst, nStrId, pszOutBuf, cchOutBuf);
                if (nLSRet != 0)
                {
                    hr = S_OK;
                }
                else
                {
                    TraceMsg(TF_WARNING,
                             "SHLoadRegUIString(): Failure loading string %d from module %ws for valid load request %ws.",
                             nStrId,
                             szParseBuf,
                             pszSource);
                }

                // BUGBUG
                // This is bad. Since these dlls aren't
                // loaded as data files, loading them and
                // not freeing them (precisely what you
                // are about to witness) is really scrungy.
                // We *NEED* to load these resource dlls
                // as data files
                
                if (fUsedMLLoadLibrary)
                {
                    MLClearMLHInstance(hinst);
                }
            }
        }

        delete [] szParseBuf;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// Note: pszSource and pszOutBuf may be the same buffer
// *ppszUpdate is the [OUT] LocalAlloc()d return buffer if we need to update the registry value
HRESULT _LoadIndirectString(LPWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf, LPWSTR *ppszUpdate)
{
    HRESULT hr;

    RIP(IS_VALID_WRITE_BUFFER(pszOutBuf, WCHAR, cchOutBuf));

    hr = E_FAIL;
    *ppszUpdate = NULL;

    if (pszSource[0] == L'@') // "@dllname,-id" or "@dllname,-id@lid,string"
    {
        LPWSTR pszLidString = StrChrW(pszSource+1, L'@');
        LANGID lidUI = MLGetUILanguage();
        WCHAR wszDllId[MAX_PATH + 1 + 6]; // path + comma + -65536

        StrCpyNW(wszDllId, pszSource+1, ARRAYSIZE(wszDllId));

        if (pszLidString)
        {
            // NULL terminate the dll,id just in case we need to actually load
            wszDllId[pszLidString-(pszSource+1)] = L'\0';

            // If the langid matches the ML language, then the string is valid
            //
            pszLidString++;
            LANGID lid = (UINT)StrToInt(pszLidString);
            if (lid == lidUI)
            {
                pszLidString = StrChrW(pszLidString, L',');
                if (pszLidString)
                {
                    StrCpyNW(pszOutBuf, pszLidString+1, cchOutBuf);
                    return S_OK;
                }
                else
                {
                    TraceMsg(TF_WARNING, "Invalid string in registry, we're needlessly hosing perf");
                }
            }
            else
            {
                TraceMsg(TF_GENERAL, "SHLoadRegUIString mis-matched lidUI, loading new string");
            }
        }

        hr = _LoadDllString(wszDllId, pszOutBuf, cchOutBuf);

        // Might as well write the new string out so we don't have to load the DLL next time through
        // but we don't write cross codepage string on Win9x
        if (SUCCEEDED(hr) && (GetNormalizedLangId(ML_CROSSCODEPAGE_NT) == lidUI))
        {
            int cch = 1 + lstrlen(wszDllId) + 7 + lstrlen(pszOutBuf) + 1; // 7 for @#####,
            *ppszUpdate = new WCHAR[cch];
            if (*ppszUpdate)
            {
                wnsprintfW(*ppszUpdate, cch, L"@%s@%d,%s", wszDllId, lidUI, pszOutBuf);
            }
        }
    }

    return hr;
}
