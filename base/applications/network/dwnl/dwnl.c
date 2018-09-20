#define COBJMACROS
#include <urlmon.h>
#include <wininet.h>
#include <wchar.h>
#include <strsafe.h>
#include <conutils.h>

#include "resource.h"

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
    WCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR szMimeType[128];
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
write_status(LPCWSTR lpFmt, ...)
{
    va_list args;
    WCHAR szTxt[128];
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    va_start(args, lpFmt);
    StringCbVPrintf(szTxt, sizeof(szTxt), lpFmt, args);
    va_end(args);

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        ConPrintf(StdOut, L"\r%*.*s", -(csbi.dwSize.X - 1), csbi.dwSize.X - 1, szTxt);
    }
    else
    {
        ConPuts(StdOut, szTxt);
    }
}

static void
CBindStatusCallback_UpdateProgress(CBindStatusCallback *This)
{
    WCHAR szMessage[MAX_PATH];

    /* FIXME: better output */
    if (This->Size != 0)
    {
        UINT Percentage;

        Percentage = (UINT)((This->Progress * 100) / This->Size);
        if (Percentage > 99)
            Percentage = 99;

        LoadStringW(NULL, IDS_BYTES_DOWNLOADED_FULL, szMessage, ARRAYSIZE(szMessage));

        write_status(szMessage, Percentage, This->Progress);
    }
    else
    {
        LoadStringW(NULL, IDS_BYTES_DOWNLOADED, szMessage, ARRAYSIZE(szMessage));

        /* Unknown size */
        write_status(szMessage, This->Progress);
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
                wcscpy(This->szHostName, szStatusText);
                This->bResolving = TRUE;

                ConResPrintf(StdOut, IDS_RESOLVING, This->szHostName);
            }
            break;

        case BINDSTATUS_CONNECTING:
            This->bConnecting = TRUE;
            This->bSendingReq = FALSE;
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = L'\0';
            if (This->bResolving)
            {
                ConResPrintf(StdOut, IDS_DONE);
                ConResPrintf(StdOut, IDS_CONNECTING_TO_FULL, This->szHostName, szStatusText);
            }
            else
            {
                ConResPrintf(StdOut, IDS_CONNECTING_TO, szStatusText);
            }
            break;

        case BINDSTATUS_REDIRECTING:
            This->bResolving = FALSE;
            This->bConnecting = FALSE;
            This->bSendingReq = FALSE;
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = L'\0';
            ConResPrintf(StdOut, IDS_REDIRECTING_TO, szStatusText);
            break;

        case BINDSTATUS_SENDINGREQUEST:
            This->bBeginTransfer = FALSE;
            This->szMimeType[0] = L'\0';
            if (This->bResolving || This->bConnecting)
                ConResPrintf(StdOut, IDS_DONE);

            if (!This->bSendingReq)
                ConResPrintf(StdOut, IDS_SEND_REQUEST);

            This->bSendingReq = TRUE;
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            wcscpy(This->szMimeType, szStatusText);
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
            This->Progress = (UINT64)ulProgress;
            This->Size = (UINT64)ulProgressMax;

            if (This->bSendingReq)
                ConResPrintf(StdOut, IDS_DONE);

            if (!This->bBeginTransfer && This->Size != 0)
            {
                if (This->szMimeType[0] != L'\0')
                    ConResPrintf(StdOut, IDS_LENGTH_FULL, This->Size, This->szMimeType);
                else
                    ConResPrintf(StdOut, IDS_LENGTH, This->Size);
            }

            ConPuts(StdOut, L"\n");

            This->bBeginTransfer = TRUE;
            break;

        case BINDSTATUS_ENDDOWNLOADDATA:
            ConResPrintf(StdOut, IDS_FILE_SAVED);
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
                OUT LPWSTR szBuffer,
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
download_file(IN LPCWSTR pszUrl,
              IN LPCWSTR pszFile OPTIONAL)
{
    WCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH + 1];
    WCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH + 1];
    WCHAR szPassWord[INTERNET_MAX_PASSWORD_LENGTH + 1];
    WCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH + 1];
    WCHAR szExtraInfo[INTERNET_MAX_PATH_LENGTH + 1];
    WCHAR szUrl[INTERNET_MAX_URL_LENGTH + 1];
    DWORD dwUrlLen;
    LPTSTR pszFilePart;
    URL_COMPONENTS urlc;
    IBindStatusCallback *pbsc;
    int iRet;
    SYSTEMTIME sysTime;

    if (pszFile != NULL && pszFile[0] == L'\0')
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
    if (!InternetCrackUrl(pszUrl, wcslen(pszUrl), ICU_ESCAPE, &urlc))
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
        wcscpy(szUserName, L"anonymous");
        urlc.dwUserNameLength = wcslen(szUserName);
    }

    /* FIXME: Get file name from server */
    if (urlc.dwUrlPathLength == 0 && pszFile == NULL)
        return DWNL_E_NEEDTARGETFILENAME;

    pszFilePart = wcsrchr(szUrlPath, L'/');
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

    GetLocalTime(&sysTime);

    ConPrintf(StdOut, L"--%d-%02d-%02d %02d:%02d:%02d-- %s\n\t=> %s\n",
          sysTime.wYear, sysTime.wMonth, sysTime.wDay,
          sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
          szUrl, pszFile);

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
    write_status(L"");

    if (iErr == DWNL_E_LASTERROR)
    {
        if (GetLastError() == ERROR_SUCCESS)
        {
            /* File not found */
            ConResPrintf(StdErr, IDS_ERROR_DOWNLOAD);
        }
        else
        {
            /* Display last error code */
            ConResPrintf(StdErr, IDS_ERROR_CODE, GetLastError());
        }
    }
    else
    {
        switch (iErr)
        {
            case DWNL_E_NEEDTARGETFILENAME:
                ConResPrintf(StdErr, IDS_ERROR_FILENAME);
                break;

            case DWNL_E_UNSUPPORTEDSCHEME:
                ConResPrintf(StdErr, IDS_ERROR_PROTOCOL);
                break;
        }
    }

    return 1;
}

int wmain(int argc, WCHAR **argv)
{
    int iErr, iRet = 0;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if(argc != 2 && argc != 3)
    {
        ConResPrintf(StdOut, IDS_USAGE);
        return 2;
    }

    iErr = download_file(argv[1], argc == 3 ? argv[2] : NULL);
    if (iErr <= 0)
        iRet = print_err(iErr);

    return iRet;
}
