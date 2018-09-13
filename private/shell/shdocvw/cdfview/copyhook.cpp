//
// Copy Hook Handler for CDFView shell extension
//
// Description:
//     This object installs an ICopyHook handler that traps all
//     copies/movies/renames in the shell so that we can special 
//     case links to channel objects and unsubscribe them when
//     the link is deleted. The implementation is in shdocvw for
//     speed.
//
// julianj 6/16/97
// 

#include "priv.h"
#include "sccls.h"
#include "chanmgr.h"
#include "channel.h"
#include "../resource.h"

#include <mluisupp.h>

class CCDFCopyHook 
                        : public ICopyHookA
                        , public ICopyHookW
{
public:
    
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** ICopyHookA method ***
    STDMETHODIMP_(UINT) CopyCallback(HWND hwnd, 
        UINT wFunc, UINT wFlags, LPCSTR pszSrcFile, DWORD dwSrcAttribs, LPCSTR pszDestFile, DWORD dwDestAttribs);

    // *** ICopyHookW method ***
    STDMETHODIMP_(UINT) CopyCallback(HWND hwnd, 
        UINT wFunc, UINT wFlags, LPCWSTR pwzSrcFile, DWORD dwSrcAttribs, LPCWSTR pwzDestFile, DWORD dwDestAttribs);

private:
    CCDFCopyHook( HRESULT * pHr);
    ~CCDFCopyHook();

    BOOL IsSubscriptionFolder(LPCTSTR pszPath);

    LONG m_cRef;

    friend HRESULT CCDFCopyHook_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);    // for ctor
};

//The copyhook handler uses this function

HRESULT Channel_GetFolder(LPTSTR pszPath, int cchPath)
{
    TCHAR szChannel[MAX_PATH];
    TCHAR szFav[MAX_PATH];
    ULONG cbChannel = sizeof(szChannel);
    
    if (SHGetSpecialFolderPath(NULL, szFav, CSIDL_FAVORITES, TRUE))
    {
        //
        // Get the potentially localized name of the Channel folder from the
        // registry if it is there.  Otherwise just read it from the resource.
        // Then tack this on the favorites path.
        //

        if (ERROR_SUCCESS != SHRegGetUSValue(L"Software\\Microsoft\\Windows\\CurrentVersion",
                                             L"ChannelFolderName", NULL, (void*)szChannel,
                                             &cbChannel, TRUE, NULL, 0))
        {
            MLLoadString(IDS_CHANNEL, szChannel, ARRAYSIZE(szChannel));
        }

        PathCombine(pszPath, szFav, szChannel);

        //
        // For IE5+ use the channels dir if it exists - else use favorites
        //
        if (!PathFileExists(pszPath))
            StrCpyN(pszPath, szFav, cchPath);

        return S_OK;
    }    
    
    return E_FAIL;
}


//
// Basic CreateInstance for this object
//
HRESULT CCDFCopyHook_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    if ( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    HRESULT hr = NOERROR;
    CCDFCopyHook * pCDFCopyHook = new CCDFCopyHook( & hr );
    if ( !pCDFCopyHook )
    {
        return E_OUTOFMEMORY;
    }
    if ( FAILED( hr ))
    {
        delete pCDFCopyHook;
        return hr;
    }
    
    *ppunk = SAFECAST(pCDFCopyHook, ICopyHookA *);
    return NOERROR;
}

STDMETHODIMP CCDFCopyHook::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IShellCopyHookA))
    {
        *ppvObj = SAFECAST(this, ICopyHookA *);
    }
    else if (IsEqualIID(riid, IID_IShellCopyHookW))
    {
        *ppvObj = SAFECAST(this, ICopyHookW *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

STDMETHODIMP_ (ULONG) CCDFCopyHook::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_ (ULONG) CCDFCopyHook::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

CCDFCopyHook::CCDFCopyHook( HRESULT * pHr) : m_cRef(1)
{
    *pHr = S_OK;
    DllAddRef();
}

CCDFCopyHook::~CCDFCopyHook()
{
    DllRelease();
}

////////////////////////////////////////////////////////////////////////////////
//
// ICopyHookA::CopyCallback
//
// Either allows the shell to move, copy, delete, or rename a folder or printer
// object, or disallows the shell from carrying out the operation. The shell 
// calls each copy hook handler registered for a folder or printer object until
// either all the handlers have been called or one of them returns IDCANCEL.
//
// RETURNS:
//
//  IDYES    - Allows the operation.
//  IDNO     - Prevents the operation on this file, but continues with any other
//             operations (for example, a batch copy operation).
//  IDCANCEL - Prevents the current operation and cancels any pending operations
//
////////////////////////////////////////////////////////////////////////////////
UINT CCDFCopyHook::CopyCallback(
    HWND hwnd,          // Handle of the parent window for displaying UI objects
    UINT wFunc,         // Operation to perform. 
    UINT wFlags,        // Flags that control the operation 
    LPCSTR pszSrcFile,  // Pointer to the source file 
    DWORD dwSrcAttribs, // Source file attributes 
    LPCSTR pszDestFile, // Pointer to the destination file 
    DWORD dwDestAttribs // Destination file attributes 
)
{
    WCHAR szSrcFile[MAX_PATH];
    WCHAR szDestFile[MAX_PATH];

    AnsiToUnicode(pszSrcFile, szSrcFile, ARRAYSIZE(szSrcFile));

    if (pszDestFile)    // shell32.dll can call with NULL for pszDestFile
        AnsiToUnicode(pszDestFile, szDestFile, ARRAYSIZE(szDestFile));
    else
        szDestFile[0] = L'\0';

    return CopyCallback(hwnd, wFunc, wFlags, szSrcFile, dwSrcAttribs, szSrcFile, dwDestAttribs);
}


CCDFCopyHook::IsSubscriptionFolder(LPCTSTR pszPath)
{
    TCHAR szSubsPath[MAX_PATH] = {0};
    DWORD dwSize = SIZEOF(szSubsPath);

    if (SHGetValueGoodBoot(HKEY_LOCAL_MACHINE, REGSTR_PATH_SUBSCRIPTION,
                       REGSTR_VAL_DIRECTORY, NULL, (LPBYTE)szSubsPath, &dwSize) != ERROR_SUCCESS)
    {
        TCHAR szWindows[MAX_PATH];

        GetWindowsDirectory(szWindows, ARRAYSIZE(szWindows));
        PathCombine(szSubsPath, szWindows, TEXT("Offline Web Pages"));
    }

    return 0 == StrCmpI(pszPath, szSubsPath);
}

////////////////////////////////////////////////////////////////////////////////
//
// ICopyHookW::CopyCallback
//
// Currently we just thunk to the ansi version.
//
////////////////////////////////////////////////////////////////////////////////
UINT CCDFCopyHook::CopyCallback(
    HWND hwnd,          // Handle of the parent window for displaying UI objects
    UINT wFunc,         // Operation to perform. 
    UINT wFlags,        // Flags that control the operation 
    LPCWSTR pszSrcFile,  // Pointer to the source file 
    DWORD dwSrcAttribs, // Source file attributes 
    LPCWSTR pszDestFile, // Pointer to the destination file 
    DWORD dwDestAttribs // Destination file attributes 
)
{
    HRESULT hr;
    ICopyHookA * pCDFViewCopyHookA;
    TCHAR   szPath[MAX_PATH];

    //
    // Return immediately if this isn't a system folder or if isn't a delete or
    // rename operation
    //
    if (!(wFunc == FO_DELETE || wFunc == FO_RENAME))
    {
        return IDYES;
    }

    // no rename of channels folder allowed.
    if ((wFunc == FO_RENAME) 
            && (Channel_GetFolder(szPath, ARRAYSIZE(szPath)) == S_OK) 
            && (StrCmpI(szPath, pszSrcFile) ==  0))
    {
        MessageBeep(MB_OK);
        return IDNO;
    }

    if (SHRestricted2W(REST_NoRemovingSubscriptions, NULL, 0) &&
        IsSubscriptionFolder(pszSrcFile))
    {
        MessageBeep(MB_OK);
        return IDNO;
    }

    if (!(dwSrcAttribs & FILE_ATTRIBUTE_SYSTEM))
        return IDYES;
    //
    // REVIEW could check for guid in desktop.ini matching CDFVIEW but its 
    // cleaner to have the ChannelMgr know about that
    //

    //
    // Create the channel manager object and ask it for the copy hook iface
    //
    hr = CoCreateInstance(CLSID_ChannelMgr, NULL,  CLSCTX_INPROC_SERVER, 
                          IID_IShellCopyHookA, (void**)&pCDFViewCopyHookA);
    if (SUCCEEDED(hr))
    {
        //
        // Delegate to the Copy hook handler in the channel mgr
        //
        CHAR szSrcFile[MAX_PATH];
        CHAR szDestFile[MAX_PATH] = {'\0'};

        SHUnicodeToAnsi(pszSrcFile, szSrcFile, ARRAYSIZE(szSrcFile));

        if (pszDestFile)
            SHUnicodeToAnsi(pszDestFile, szDestFile, ARRAYSIZE(szDestFile));

        UINT retValue = pCDFViewCopyHookA->CopyCallback(
                hwnd, wFunc, wFlags, szSrcFile, 
                dwSrcAttribs, szDestFile, dwDestAttribs);

        pCDFViewCopyHookA->Release();

        return retValue;
    }
    else
    {
        // Couldn't create ChannelMgr object for some reason 
        TraceMsg(TF_ERROR, "Could not CoCreateInstance CLSID_ChannelMgr");
        return IDYES;
    }
}


