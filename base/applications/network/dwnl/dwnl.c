#include <stdio.h>
#define COBJMACROS
#include <urlmon.h>
#include <wininet.h>
#include <tchar.h>

/* FIXME: add correct definitions to urlmon.idl */
#ifdef UNICODE
#define URLDownloadToFile URLDownloadToFileW
#else
#define URLDownloadToFile URLDownloadToFileA
#endif

#define DWNL_E_LASTERROR    0
#define DWNL_E_NEEDTARGETFILENAME   -1
#define DWNL_E_UNSUPPORTEDSCHEME    -2

typedef struct
{
    const IBindStatusCallbackVtbl* lpIBindStatusCallbackVtbl;
    LONG ref;
    TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    TCHAR szMimeType[128];
    UINT64 Size;
    UINT64 Progress;
    UINT bResolving : 1;
    UINT bConnecting : 1;
    UINT bSendingReq : 1;
    UINT bBeginTransfer : 1;
} CBindStatusCallback;

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)
#define interface_to_impl(instance,iface) ((CBindStatusCallback*)((ULONG_PTR)instance - FIELD_OFFSET(CBindStatusCallback,lp##iface##Vtbl)))

static void
CBindStatusCallback_Destroy(CBindStatusCallback *This)
{
    return;
}

static void
write_status(LPCTSTR lpFmt, ...)
{
    va_list args;

    /* FIXME: Determine line length! */
    TCHAR szTxt[80];
    int c;

    va_start(args, lpFmt);
    _vstprintf(szTxt, lpFmt, args);
    va_end(args);

    c = _tcslen(szTxt);
    while (c < (sizeof(szTxt) / sizeof(szTxt[0])) - 1)
        szTxt[c++] = _T(' ');
    szTxt[c] = _T('\0');

    _tprintf(_T("\r%.79s"), szTxt);
}

static void
CBindStatusCallback_UpdateProgress(CBindStatusCallback *This)
{
    /* FIXME: better output */
    if (This->Size != 0)
    {
        UINT Percentage;

        Percentage = (UINT)((This->Progress * 100) / This->Size);
        if (Percentage > 99)
            Percentage = 99;

        write_status(_T("%2d%% (%I64u bytes downloaded)"), Percentage, This->Progress);
    }
    else
    {
        /* Unknown size */
        write_status(_T("%I64u bytes downloaded"), This->Progress);
    }
}

static ULONG STDMETHODCALLTYPE
CBindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    CBindStatusCallback *This = interface_to_impl(iface, IBindStatusCallback);
    ULONG ret;

    ret = InterlockedIncrement((PLONG)&This->ref);
    return ret;
}

static ULONG STDMETHODCALLTYPE
CBindStatusCallback_Release(IBindStatusCallback *iface)
{
    CBindStatusCallback *This = interface_to_impl(iface, IBindStatusCallback);
    ULONG ret;

    ret = InterlockedDecrement((PLONG)&This->ref);
    if (ret == 0)
    {
        CBindStatusCallback_Destroy(This);

        HeapFree(GetProcessHeap(),
                 0,
                 This);
    }

    return ret;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_QueryInterface(IBindStatusCallback *iface,
                                   REFIID iid,
                                   PVOID *pvObject)
{
    CBindStatusCallback *This = interface_to_impl(iface, IBindStatusCallback);

    *pvObject = NULL;

    if (IsEqualIID(iid,
                   &IID_IBindStatusCallback) ||
        IsEqualIID(iid,
                   &IID_IUnknown))
    {
        *pvObject = impl_to_interface(This, IBindStatusCallback);
    }
    else
        return E_NOINTERFACE;

    CBindStatusCallback_AddRef(iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
                                   DWORD dwReserved,
                                   IBinding* pib)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_GetPriority(IBindStatusCallback *iface,
                                LONG* pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnLowResource(IBindStatusCallback *iface,
                                  DWORD reserved)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnProgress(IBindStatusCallback *iface,
                               ULONG ulProgress,
                               ULONG ulProgressMax,
                               ULONG ulStatusCode,
                               LPCWSTR szStatusText)
{
    CBindStatusCallback *This = interface_to_impl(iface, IBindStatusCallback);

    switch (ulStatusCode)
    {
        case BINDSTATUS_FINDINGRESOURCE:
            if (!This->bResolving)
            {
                _tcscpy(This->szHostName, szStatusText);
                This->bResolving = TRUE;

                _tprintf(_T("Resolving %s... "), This->szHostName);
            }
            break;

        case BINDSTATUS_CONNECTING:
            This->bConnecting = TRUE;
            This->bSendingReq = FALSE;
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = _T('\0');
            if (This->bResolving)
            {
                _tprintf(_T("done.\n"));
                _tprintf(_T("Connecting to %s[%s]... "), This->szHostName, szStatusText);
            }
            else
                _tprintf(_T("Connecting to %s... "), szStatusText);
            break;

        case BINDSTATUS_REDIRECTING:
            This->bResolving = FALSE;
            This->bConnecting = FALSE;
            This->bSendingReq = FALSE;
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = _T('\0');
            _tprintf(_T("Redirecting to %s... "), szStatusText);
            break;

        case BINDSTATUS_SENDINGREQUEST:
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = _T('\0');
            if (This->bResolving || This->bConnecting)
                _tprintf(_T("done.\n"));

            if (!This->bSendingReq)
                _tprintf(_T("Sending request... "));

            This->bSendingReq = TRUE;
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            _tcscpy(This->szMimeType, szStatusText);
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
            This->Progress = (UINT64)ulProgress;
            This->Size = (UINT64)ulProgressMax;

            if (This->bSendingReq)
                _tprintf(_T("done.\n"));

            if (!This->bBeginTransfer && This->Size != 0)
            {
                if (This->szMimeType[0] != _T('\0'))
                    _tprintf(_T("Length: %I64u [%s]\n"), This->Size, This->szMimeType);
                else
                    _tprintf(_T("Length: %ull\n"), This->Size);
            }

            _tprintf(_T("\n"));

            This->bBeginTransfer = TRUE;
            break;

        case BINDSTATUS_ENDDOWNLOADDATA:
            write_status(_T("File saved."));
            _tprintf(_T("\n"));
            break;

        case BINDSTATUS_DOWNLOADINGDATA:
            This->Progress = (UINT64)ulProgress;
            This->Size = (UINT64)ulProgressMax;

            CBindStatusCallback_UpdateProgress(This);
            break;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
                                  HRESULT hresult,
                                  LPCWSTR szError)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
                                DWORD* grfBINDF,
                                BINDINFO* pbindinfo)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
                                    DWORD grfBSCF,
                                    DWORD dwSize,
                                    FORMATETC* pformatetc,
                                    STGMEDIUM* pstgmed)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
CBindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
                                      REFIID riid,
                                      IUnknown* punk)
{
    return E_NOTIMPL;
}

static const struct IBindStatusCallbackVtbl vtblIBindStatusCallback =
{
    CBindStatusCallback_QueryInterface,
    CBindStatusCallback_AddRef,
    CBindStatusCallback_Release,
    CBindStatusCallback_OnStartBinding,
    CBindStatusCallback_GetPriority,
    CBindStatusCallback_OnLowResource,
    CBindStatusCallback_OnProgress,
    CBindStatusCallback_OnStopBinding,
    CBindStatusCallback_GetBindInfo,
    CBindStatusCallback_OnDataAvailable,
    CBindStatusCallback_OnObjectAvailable,
};

static IBindStatusCallback *
CreateBindStatusCallback(void)
{
    CBindStatusCallback *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (This == NULL)
        return NULL;

    This->lpIBindStatusCallbackVtbl = &vtblIBindStatusCallback;
    This->ref = 1;

    return impl_to_interface(This, IBindStatusCallback);
}


// ToDo: Show status, get file name from webserver, better error reporting

static int
get_display_url(IN LPURL_COMPONENTS purl,
                OUT TCHAR *szBuffer,
                IN PDWORD pdwBufferSize)
{
    URL_COMPONENTS urlc;

    /* Hide the password */
    urlc = *purl;
    urlc.lpszPassword = NULL;
    urlc.dwPasswordLength = 0;

    if (!InternetCreateUrl(&urlc, ICU_ESCAPE, szBuffer, pdwBufferSize))
        return DWNL_E_LASTERROR;

    return 1;
}

static int
download_file(IN LPCTSTR pszUrl,
              IN LPCTSTR pszFile  OPTIONAL)
{
    TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH + 1];
    TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    TCHAR szPassWord[INTERNET_MAX_PASSWORD_LENGTH + 1];
    TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH + 1];
    TCHAR szExtraInfo[INTERNET_MAX_PATH_LENGTH + 1];
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwUrlLen;
    LPTSTR pszFilePart;
    URL_COMPONENTS urlc;
    IBindStatusCallback *pbsc;
    int iRet;

    if (pszFile != NULL && pszFile[0] == _T('\0'))
        pszFile = NULL;

    urlc.dwStructSize = sizeof(urlc);
    urlc.lpszScheme = szScheme;
    urlc.dwSchemeLength = sizeof(szScheme) / sizeof(szScheme[0]);
    urlc.lpszHostName = szHostName;
    urlc.dwHostNameLength = sizeof(szHostName) / sizeof(szHostName[0]);
    urlc.lpszUserName = szUserName;
    urlc.dwUserNameLength = sizeof(szUserName) / sizeof(szUserName[0]);
    urlc.lpszPassword = szPassWord;
    urlc.dwPasswordLength = sizeof(szPassWord) / sizeof(szPassWord[0]);
    urlc.lpszUrlPath = szUrlPath;
    urlc.dwUrlPathLength = sizeof(szUrlPath) / sizeof(szUrlPath[0]);
    urlc.lpszExtraInfo = szExtraInfo;
    urlc.dwExtraInfoLength = sizeof(szExtraInfo) / sizeof(szExtraInfo[0]);
    if (!InternetCrackUrl(pszUrl, _tcslen(pszUrl), ICU_ESCAPE, &urlc))
        return DWNL_E_LASTERROR;

    if (urlc.nScheme != INTERNET_SCHEME_FTP &&
        urlc.nScheme != INTERNET_SCHEME_GOPHER &&
        urlc.nScheme != INTERNET_SCHEME_HTTP &&
        urlc.nScheme != INTERNET_SCHEME_HTTPS)
    {
        return DWNL_E_UNSUPPORTEDSCHEME;
    }

    if (urlc.nScheme == INTERNET_SCHEME_FTP && urlc.dwUserNameLength == 0 && urlc.dwPasswordLength == 0)
    {
        _tcscpy(szUserName, _T("anonymous"));
        urlc.dwUserNameLength = _tcslen(szUserName);
    }

    /* FIXME: Get file name from server */
    if (urlc.dwUrlPathLength == 0 && pszFile == NULL)
        return DWNL_E_NEEDTARGETFILENAME;

    pszFilePart = _tcsrchr(szUrlPath, _T('/'));
    if (pszFilePart != NULL)
        pszFilePart++;

    if (pszFilePart == NULL && pszFile == NULL)
        return DWNL_E_NEEDTARGETFILENAME;

    if (pszFile == NULL)
        pszFile = pszFilePart;

    if (urlc.dwUserNameLength == 0)
        urlc.lpszUserName = NULL;

    if (urlc.dwPasswordLength == 0)
        urlc.lpszPassword = NULL;

    /* Generate the URL to be displayed (without a password) */
    dwUrlLen = sizeof(szUrl) / sizeof(szUrl[0]);
    iRet = get_display_url(&urlc, szUrl, &dwUrlLen);
    if (iRet <= 0)
        return iRet;

    _tprintf(_T("Download `%s\'\n\t=> `%s\'\n"), szUrl, pszFile);

    /* Generate the URL to download */
    dwUrlLen = sizeof(szUrl) / sizeof(szUrl[0]);
    if (!InternetCreateUrl(&urlc, ICU_ESCAPE, szUrl, &dwUrlLen))
        return DWNL_E_LASTERROR;

    pbsc = CreateBindStatusCallback();
    if (pbsc == NULL)
        return DWNL_E_LASTERROR;

    if(!SUCCEEDED(URLDownloadToFile(NULL, szUrl, pszFile, 0, pbsc)))
    {
        IBindStatusCallback_Release(pbsc);
        return DWNL_E_LASTERROR; /* FIXME */
    }

    IBindStatusCallback_Release(pbsc);
    return 1;
}

static int
print_err(int iErr)
{
    write_status(_T(""));

    if (iErr == DWNL_E_LASTERROR)
    {
        if (GetLastError() == ERROR_SUCCESS)
        {
            /* File not found */
            _ftprintf(stderr, _T("\nERROR: Download failed.\n"));
        }
        else
        {
            /* Display last error code */
            _ftprintf(stderr, _T("\nERROR: %u\n"), GetLastError());
        }
    }
    else
    {
        switch (iErr)
        {
            case DWNL_E_NEEDTARGETFILENAME:
                _ftprintf(stderr, _T("\nERROR: Cannot determine filename, please specify a destination file name.\n"));
                break;

            case DWNL_E_UNSUPPORTEDSCHEME:
                _ftprintf(stderr, _T("\nERROR: Unsupported protocol.\n"));
                break;
        }
    }

    return 1;
}

int _tmain(int argc, TCHAR **argv)
{
    int iErr, iRet = 0;

    if(argc != 2 && argc != 3)
    {
        _tprintf(_T("Usage: dwnl URL [DESTINATION]"));
        return 2;
    }

    iErr = download_file(argv[1], argc == 3 ? argv[2] : NULL);
    if (iErr <= 0)
        iRet = print_err(iErr);

    return iRet;
}
