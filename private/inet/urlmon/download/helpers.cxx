//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************
//
//    HELPERS.CPP
//
//    HISTORY:
//    
//    9/01/95        philco        Created.
//
//

#include <cdlpch.h>

BOOL GetEXEName(LPSTR szCmdLine);


///////***********       Helper functions        *********/////////////


/*******************************************************************

    NAME:        Unicode2Ansi
        
    SYNOPSIS:    Converts a unicode widechar string to ansi (MBCS)

    NOTES:        Caller must free out parameter using delete
                    
********************************************************************/
HRESULT Unicode2Ansi(const wchar_t *src, char ** dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it.
    int len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
    *dest = new char [len*sizeof(char)];
    if (!*dest)
        return E_OUTOFMEMORY;

    // Now do the actual conversion
    if ((WideCharToMultiByte(CP_ACP, 0, src, -1, *dest, len*sizeof(char), 
                                                            NULL, NULL)) != 0)
        return S_OK; 
    else
        return E_FAIL;
}


/*******************************************************************

    NAME:        Ansi2Unicode
        
    SYNOPSIS:    Converts an ansi (MBCS) string to unicode.

    Notes:        Caller must free out parameter using delete
                        
********************************************************************/
HRESULT Ansi2Unicode(const char * src, wchar_t **dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it
    int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, NULL, 0);
    *dest = new WCHAR [len*sizeof(WCHAR)];
    if (!*dest)
        return E_OUTOFMEMORY;

    // Do the actual conversion.
    if ((MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, *dest, 
                                                    len*sizeof(wchar_t))) != 0)
        return S_OK; 
    else
        return E_FAIL;
}


/*******************************************************************

    NAME:        ConvertANSItoCLSID
        
    SYNOPSIS:    Converts an ANSI string to a CLSID structure (address
                of which is passed by caller as an [out] parameter)
                    
********************************************************************/
HRESULT ConvertANSItoCLSID(const char *pszCLSID, CLSID * clsid)
{
    ASSERT(pszCLSID != NULL);
    ASSERT(clsid != NULL);

    HRESULT hr = S_OK;
    LPOLESTR wcstr = NULL;

    // Since OLE is Unicode only, we need to convert pszClsid to Unicode.
    hr = Ansi2Unicode(pszCLSID, &wcstr);
    if (FAILED(hr))
        goto cleanup;

    // Get CLSID from string
    hr = CLSIDFromString(wcstr, clsid);
    if (FAILED(hr))
        goto cleanup;

cleanup:
    if (wcstr != NULL) 
        delete wcstr;   // Delete unicode string.  We're done.
    return hr;
}

/*******************************************************************

    NAME:        ConvertFriendlyANSItoCLSID
        
    SYNOPSIS:    Works like ConvertANSItoCLSID but allows prefix
                 of "clsid:" CLSID is passed as [out]
                 parameter.
   
********************************************************************/
HRESULT ConvertFriendlyANSItoCLSID(char *pszCLSID, CLSID * clsid)
{
    static const char *szClsid = "clsid:";
    ASSERT(pszCLSID != NULL);
    ASSERT(clsid != NULL);
    
    HRESULT hr = S_OK;

    // try OLE form.
    hr = ConvertANSItoCLSID(pszCLSID,clsid);
    if (SUCCEEDED(hr))
        goto cleanup;

    // try for prefix "clsid:"

    if (StrCmpNA(pszCLSID, szClsid, lstrlenA(szClsid)) == 0) {

        hr = ConvertANSItoCLSID(pszCLSID + lstrlenA(szClsid), clsid);
        if (SUCCEEDED(hr))
            goto cleanup;

        // construct COM form of clsid:
        LPSTR szTmp = NULL;
        szTmp = new char[lstrlenA(pszCLSID) - lstrlenA(szClsid) + 2 + 1];
        if (!szTmp) {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        lstrcpyA(szTmp,"{");
        lstrcatA(szTmp,pszCLSID + lstrlenA(szClsid));
        lstrcatA(szTmp,"}");

        hr = ConvertANSItoCLSID(szTmp,clsid);
        delete [] szTmp;
        if (SUCCEEDED(hr))
            goto cleanup;
    }
    
    // error code passes through
    
cleanup:

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: GetExtnAndBaseFileName
//
// Parameters:
//    in szName: a filename or URL
//  out update pbasefilename to point into this szName
//
//    Returns:
//    out: extn
// ---------------------------------------------------------------------------
FILEXTN GetExtnAndBaseFileName( char *szName, char **plpBaseFileName)
{
    char *pCur = szName;
    char *pExt = NULL;
    char szExtCopy[4];
    FILEXTN extn = FILEXTN_UNKNOWN;

    const static char *szCAB = "CAB";
    const static char *szINF = "INF";
    const static char *szDLL = "DLL";
    const static char *szOCX = "OCX";
    const static char *szEXE = "EXE";
    const static char *szOSD = "OSD";
    const static char *szCAT = "CAT";

    *plpBaseFileName = szName;

    // find location of last '.' and '/' in name
    for (;*pCur; *pCur++) {
        if (*pCur == '.')
            pExt = pCur+1;
        if ((*pCur == '/') || (*pCur == '\\'))
            *plpBaseFileName = pCur+1;
    }

    if (!pExt || !*pExt) { // if no '.' or as last char
        extn = FILEXTN_NONE;
        goto Exit;
    }

    StrNCpy(szExtCopy, pExt, 4);

    CharUpperBuff(szExtCopy, 3);

    if (lstrcmp(szExtCopy, szCAB)     == 0) {
        extn = FILEXTN_CAB;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szOCX)     == 0) {
        extn = FILEXTN_OCX;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szDLL)     == 0) {
        extn = FILEXTN_DLL;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szEXE)     == 0) {
        extn = FILEXTN_EXE;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szINF)     == 0) {
        extn = FILEXTN_INF;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szOSD)     == 0) {
        extn = FILEXTN_OSD;
        goto Exit;
    }

    if (lstrcmp(szExtCopy, szCAT)     == 0) {
        extn = FILEXTN_CAT;
        goto Exit;
    }

Exit:

    return extn;
}

// ---------------------------------------------------------------------------
// %%Function: GetVersionFromString
//
//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
// ---------------------------------------------------------------------------
HRESULT
GetVersionFromString(const char *szBuf, LPDWORD pdwFileVersionMS, LPDWORD pdwFileVersionLS, char cSeperator)
{
    // cdl.h defines cSeperator as a default parameter: ','
    const char *pch = szBuf;
    char ch;
    char szMaxString[30];

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;
    wsprintf(szMaxString, "-1%c-1%c-1%c-1", cSeperator, cSeperator, cSeperator);

    if (lstrcmp(pch, szMaxString) == 0) {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
        return S_OK;
    }

    USHORT n = 0;

    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;


    for (ch = *pch++;;ch = *pch++) {

        if ((ch == cSeperator) || (ch == '\0')) {

            switch (have) {

            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == '\0') {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        } else if ( (ch < '0') || (ch > '9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - '0');


    } /* end forever */

    // NEVERREACHED
}

#ifdef _ZEROIMPACT
LPSTR GetStringFromVersion(char *szBuf, DWORD dwFileVersionMS, DWORD dwFileVersionLS, char cSeperator)
{
    SetLastError(NOERROR);
    wsprintf(szBuf, "%lu%c%lu%c%lu%c%lu", dwFileVersionMS >> 16, cSeperator, dwFileVersionMS & 0x0000FFFF, cSeperator, 
                                          dwFileVersionLS >> 16, cSeperator, dwFileVersionLS & 0x0000FFFF);
    if(GetLastError() == NOERROR)
        return szBuf;
    else
        return NULL;
}
#endif


//************************************************************
// CheckFileImplementsCLSID
//
// checks if a clsid is implemented by the given file
// Return S_OK on success.
//
// pszFileName:  Full path to the file whose clsid we
//               desire.
//  rClsid:    clsid to check
//

HRESULT CheckFileImplementsCLSID(const char *pszFileName, REFCLSID rClsid)
{
    LPTYPELIB ptlib = NULL;
    LPTYPEINFO lpTypeInfo = NULL;
    LPTYPEATTR lpTypeAttr = NULL;
    WCHAR puszFileName[MAX_PATH];  // unicode string
    HRESULT hr = S_OK;

    //
    // LoadTypeLib() needs the string in unicode.
    //
    MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, puszFileName, sizeof(puszFileName) / sizeof(wchar_t));

    hr = LoadTypeLib(puszFileName, &ptlib);

    if (FAILED(hr))
        return hr;

    hr = ptlib->GetTypeInfoOfGuid(rClsid, &lpTypeInfo);

    if (SUCCEEDED(hr)) {

        if (S_OK != (hr = lpTypeInfo->GetTypeAttr(&lpTypeAttr))) {
            goto Exit;
        }

        if (TKIND_COCLASS == lpTypeAttr->typekind) {

            Assert(IsEqualGUID( lpTypeAttr->guid, rClsid));

            // success
            goto Exit;
        }

    
    }


Exit:

    if (lpTypeAttr != NULL) lpTypeInfo->ReleaseTypeAttr(lpTypeAttr);
    if (lpTypeInfo != NULL) lpTypeInfo->Release();
    if (ptlib != NULL)      ptlib->Release();

    return hr;
}

HRESULT CDLDupWStr( LPWSTR *pszwstrDst, LPCWSTR szwSrc )
{
    HRESULT hr = S_OK;

    *pszwstrDst = new WCHAR[lstrlenW(szwSrc)+1];
    if (*pszwstrDst)
        StrCpyW( *pszwstrDst, szwSrc );
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

extern CMutexSem g_mxsCodeDownloadGlobals;

HRESULT 
MakeUniqueTempDirectory(LPCSTR szTempDir, LPSTR szUniqueTempDir)
{
    int n = 1;
    HRESULT hr = S_OK;

    //execute entire function under critical section
    CLock lck(g_mxsCodeDownloadGlobals);

    do {

        if (n > 100)    // avoid infinite loop!
            break;

        wsprintf(szUniqueTempDir,"%s%s%d.tmp", szTempDir, "ICD", n++);


    } while (GetFileAttributes(szUniqueTempDir) != -1);

    if (!CreateDirectory(szUniqueTempDir, NULL)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT
ComposeHackClsidFromMime(LPSTR szHackMimeType, LPCSTR szClsid)
{
    HRESULT hr = S_OK;
    char szID[MAX_PATH];
    LPSTR pchDest = szID;

    for (LPCSTR pchSrc = szClsid; *pchDest = *pchSrc; pchSrc++,pchDest++) {

        if (*pchSrc == '/') {
            *pchDest++ = '_';   
            *pchDest++ = '2';   
            *pchDest++ = 'F';   
            *pchDest = '_'; 
            
        }
    }

    wsprintf(szHackMimeType, "&CLSID=%s", szID);

    return hr;
}


HRESULT
RemoveDirectoryAndChildren(LPCSTR szDir)
{
    HRESULT hr = S_OK;
    HANDLE hf = INVALID_HANDLE_VALUE;
    char szBuf[MAX_PATH];
    WIN32_FIND_DATA fd;

    if (RemoveDirectory(szDir))
        goto Exit;

    // ha! we have a case where the directory is probbaly not empty

    lstrcpy(szBuf, szDir);
    lstrcat(szBuf, "\\*");

    if ((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    do {

        if ( (lstrcmp(fd.cFileName, ".") == 0) || 
             (lstrcmp(fd.cFileName, "..") == 0))
            continue;

        wsprintf(szBuf, "%s\\%s", szDir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            SetFileAttributes(szBuf, 
                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

            if (FAILED((hr=RemoveDirectoryAndChildren(szBuf)))) {
                goto Exit;
            }

        } else {

            SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(szBuf)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }
        }


    } while (FindNextFile(hf, &fd));


    if (GetLastError() != ERROR_NO_MORE_FILES) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (hf != INVALID_HANDLE_VALUE) {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    // here if all subdirs/children removed
    /// re-attempt to remove the main dir
    if (!RemoveDirectory(szDir)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:

    if (hf != INVALID_HANDLE_VALUE)
        FindClose(hf);

    return hr;
}


HRESULT IsExtnHandled(LPSTR pszExt)
{
    HRESULT hr = REGDB_E_CLASSNOTREG;

    HKEY        hkRoot = HKEY_CLASSES_ROOT;
    char        szFileExt[MAX_PATH];
    DWORD       cbFileExt = sizeof(szFileExt);

    char szCmdLine[2*MAX_PATH];
    char szProgID[2*MAX_PATH];
    LONG  cbProgID = sizeof(szProgID);
    DWORD dwType;
    DWORD dwLen = sizeof(szCmdLine);
    HKEY hkeyCmd = 0;

    if (pszExt[0] == '\0')
    {
        goto Exit;
    }

    strcpy(szFileExt,pszExt);

    Assert((szFileExt[0] == '.'));

    // the entry begins with '.' so it may be a file extension
    // query the value (which is the ProgID)

    if (RegQueryValue(hkRoot, szFileExt, szProgID, &cbProgID) == ERROR_SUCCESS)
    {
        // we got the value (ProgID), now query for the CLSID
        // string and convert it to a CLSID
        strcat(szProgID, "\\Shell\\Open\\Command");

        if ( (RegOpenKeyEx(HKEY_CLASSES_ROOT, szProgID, 0, KEY_QUERY_VALUE, &hkeyCmd) == ERROR_SUCCESS) &&
             (SHQueryValueEx(hkeyCmd, NULL, NULL, &dwType, &szCmdLine, &dwLen) == ERROR_SUCCESS) && dwLen)

        {
            if (GetEXEName(szCmdLine)) {
                hr = S_OK;
            }
        }
    }

Exit:

    if (hkeyCmd)
        RegCloseKey(hkeyCmd);

    return hr;
}

#define SZMIMESIZE_MAX  128

HRESULT IsMimeHandled(LPCWSTR pwszMime)
{
    HRESULT hr = S_OK;
    HKEY hMimeKey = NULL;
    DWORD dwType;
    char szValue[MAX_PATH];
    DWORD dwValueLen = MAX_PATH;
    static char szMimeKey[]     = "MIME\\Database\\Content Type\\";
    static char szExtension[]     = "Extension";
    const ULONG ulMimeKeyLen    = ((sizeof(szMimeKey)/sizeof(char))-1);
    char szKey[SZMIMESIZE_MAX + ulMimeKeyLen];
    LPSTR pszMime = NULL;

    if (SUCCEEDED((hr=::Unicode2Ansi(pwszMime, &pszMime)))) {

        strcpy(szKey, szMimeKey);
        strcat(szKey, pszMime);

        hr = REGDB_E_READREGDB;

        if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_QUERY_VALUE, &hMimeKey) == ERROR_SUCCESS) {

            if (RegQueryValueEx(hMimeKey
                    , szExtension
                    , NULL
                    , &dwType
                    , (LPBYTE)szValue
                    , &dwValueLen) == ERROR_SUCCESS) {

                hr = IsExtnHandled(szValue);
            }
        }
    }

    if (hMimeKey) {
        RegCloseKey(hMimeKey);
    }

    SAFEDELETE(pszMime);

    return hr;
}


#ifdef _ZEROIMPACT
// GetClassObjectFromDll
// Use the DllGetClassObject (or the DllGetClassObjectFromString) entrypoint
// in the passed in dll to get an instance of the passed in class
// Returns S_OK on success,
//         HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) on class object not found
typedef HRESULT (__stdcall *PFNDLLGETCLASSOBJECTCLSID)(REFCLSID rclsid, REFIID riid, void **ppv);
typedef HRESULT (__stdcall *PFNDLLGETCLASSOBJECTSTRING)(LPWSTR szClassString, REFIID riid, void **ppv);

HRESULT
GetClassObjectFromDll(LPCWSTR wszClass, LPWSTR wszDll, REFIID riid,
                      DWORD grfBINDF, void **ppv)
{    
    HMODULE hmod = NULL;
    HRESULT hres = NULL;
    HRESULT hr = S_OK;
    PFNDLLGETCLASSOBJECTCLSID pfnGetClassObject = NULL;
    PFNDLLGETCLASSOBJECTSTRING pfnString = NULL;
    TCHAR szClsidRequest[] = TEXT("DllGetClassObject");
    TCHAR szClassStringRequest[] = TEXT("DllGetClassObjectFromString");
    CLSID clsid;
    void * pvClassFactory = NULL;

    hmod = CoLoadLibrary((LPOLESTR) wszDll, TRUE);

    if (!hmod) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    // Get the clsid to use in DllGetClassObject.  If the string does not translate
    // to a class id, set pszDllRequest to use the DllGetClassObjectFromString entrypoint
    // If it does translate to a non-null clsid, use the DllGetClassObject entrypoint

    hr = CLSIDFromString(const_cast<LPOLESTR>(wszClass), &clsid);

    if(SUCCEEDED(hr)) {   // Have a clsid

        if(IsEqualGUID(clsid, CLSID_NULL)) {
            hr = E_INVALIDARG;
            goto Exit;
        }

        pfnGetClassObject = (PFNDLLGETCLASSOBJECTCLSID)GetProcAddress(hmod, szClsidRequest);

        if(pfnGetClassObject) {

            if (grfBINDF & BINDF_GETCLASSOBJECT) {
                hr = pfnGetClassObject(clsid, riid, ppv);
            }
            else {
                IUnknown * pUnknown = NULL;
                IClassFactory * pClassFactory = NULL;
                hr = pfnGetClassObject(clsid, riid, (void **) &pUnknown);
    
                if(SUCCEEDED(hr)) {
                    hr = pUnknown->QueryInterface(IID_IClassFactory, (void **) &pClassFactory);
                    if(SUCCEEDED(hr)) {
                        hr = pClassFactory->CreateInstance(NULL, riid, ppv);
                    }
                }
    
            }
        }
        else { // could not get the proc address
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }
    else           // No clsid, try the classstring entrypoint
    {
        pfnString = (PFNDLLGETCLASSOBJECTSTRING)GetProcAddress(hmod, szClassStringRequest);

        if(pfnString) {
            hr = pfnString(const_cast<LPOLESTR>(wszClass), riid, ppv);
        }
        else { // could not get the proc address
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }


Exit:
    if(FAILED(hr) && hmod)
        FreeLibrary(hmod);

    return hr;
}

#endif


