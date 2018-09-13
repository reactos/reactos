#include "private.h"
#include "multiutl.h"
#include <wtypes.h>
#include "strconst.h"
#include <platform.h>

LPMALLOC    g_pMalloc = NULL;
extern      HINSTANCE   g_hInst;

// Pstore related variables.
static PST_KEY s_Key = PST_KEY_CURRENT_USER;

// {89C39569-6841-11d2-9F59-0000F8085266}
static const GUID GUID_PStoreType = { 0x89c39569, 0x6841, 0x11d2, { 0x9f, 0x59, 0x0, 0x0, 0xf8, 0x8, 0x52, 0x66 } };
static WCHAR c_szIdentityMgr[] = L"IdentityMgr";
static WCHAR c_szIdentities[] = L"Identities";
static WCHAR c_szIdentityPass[] = L"IdentitiesPass";

// --------------------------------------------------------------------------
// FIsSpaceA
// --------------------------------------------------------------------------
BOOL FIsSpaceA(LPSTR psz)
{
#ifdef MAC
    return (isspace(*psz));
#else	// !MAC
    WORD wType;

    if (IsDBCSLeadByte(*psz))
        GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 2, &wType);
    else
        GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType);
    return (wType & C1_SPACE);
#endif	// MAC
}

// --------------------------------------------------------------------------
// FIsSpaceW
// --------------------------------------------------------------------------
BOOL FIsSpaceW(LPWSTR psz)
{
#ifdef MAC
    // Maybe we should convert to ANSI before checking??
    return (isspace(*( ( (TCHAR *) psz ) + 1 ) ));
#else	// !MAC
    WORD wType;
    GetStringTypeExW(LOCALE_USER_DEFAULT, CT_CTYPE1, psz, 1, &wType);
    return (wType & C1_SPACE);
#endif	// !MAC
}


ULONG UlStripWhitespace(LPTSTR lpsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb)
{
    // Locals
    ULONG           cb;
    LPTSTR          psz;
    
    Assert(lpsz != NULL);
    Assert(fLeading || fTrailing);
    
    // Did the user pass in the length
    if (pcb)
        cb = *pcb;
    else
        cb = lstrlen (lpsz);
    
    if (cb == 0)
        return cb;
    
    if (fLeading)
    {
        psz = lpsz;
        
        while (FIsSpace(psz))
        {
            psz++;
            cb--;
        }
        
        if (psz != lpsz)
            // get the NULL at the end too
            MoveMemory(lpsz, psz, (cb + 1) * sizeof(TCHAR));
    }
    
    if (fTrailing)
    {
        psz = lpsz + cb;
        
        while (cb > 0)
        {
            if (!FIsSpace(psz-1))
                break;
            psz--;
            cb--;
        }    
        
        // NULL Term
        *psz = '\0';
    }
    
    // Set String Size
    if (pcb)
        *pcb = cb;
    
    // Done
    return cb;
}

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap)
{
    if (uMsg == WM_HELP)
    {
        LPHELPINFO lphi = (LPHELPINFO) lParam;
        if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
        {
            WinHelp ((HWND)lphi->hItemHandle,
                        c_szCtxHelpFile,
                        HELP_WM_HELP,
                        (DWORD_PTR)(LPVOID)rgCtxMap);
        }
        return (TRUE);
    }
    else if (uMsg == WM_CONTEXTMENU)
    {
        WinHelp ((HWND) wParam,
                    c_szCtxHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)rgCtxMap);
        return (TRUE);
    }

    Assert(0);

    return FALSE;
}



#define OBFUSCATOR              0x14151875;

#define PROT_SIZEOF_HEADER      0x02    // 2 bytes in the header
#define PROT_SIZEOF_XORHEADER   (PROT_SIZEOF_HEADER+sizeof(DWORD))

#define PROT_VERSION_1          0x01

#define PROT_PASS_XOR           0x01
#define PROT_PASS_PST           0x02

static BOOL FDataIsValidV1(BYTE *pb)
{ return pb && pb[0] == PROT_VERSION_1 && (pb[1] == PROT_PASS_XOR || pb[1] == PROT_PASS_PST); }

static BOOL FDataIsPST(BYTE *pb)
{ return pb && pb[1] == PROT_PASS_PST; }

///////////////////////////////////////////////////////////////////////////
// 
// NOTE - The functions for encoding the user passwords really should not 
//        be here.  Unfortunately, they are not anywhere else so for now,
//        this is where they will stay.  They are defined as static since
//        other code should not rely on them staying here, particularly the 
//        XOR stuff.
//
///////////////////////////////////////////////////////////////////////////
// 
// XOR functions
//
///////////////////////////////////////////////////////////////////////////

static HRESULT _XOREncodeProp(const BLOB *const pClear, BLOB *const pEncoded)
{
    DWORD       dwSize;
    DWORD       last, last2;
    DWORD       *pdwCypher;
    DWORD       dex;

    pEncoded->cbSize = pClear->cbSize+PROT_SIZEOF_XORHEADER;
    if (!MemAlloc((LPVOID *)&pEncoded->pBlobData, pEncoded->cbSize + 6))
        return E_OUTOFMEMORY;
    
    // set up header data
    Assert(2 == PROT_SIZEOF_HEADER);
    pEncoded->pBlobData[0] = PROT_VERSION_1;
    pEncoded->pBlobData[1] = PROT_PASS_XOR;
    *((DWORD *)&(pEncoded->pBlobData[2])) = pClear->cbSize;

    // nevermind that the pointer is offset by the header size, this is
    // where we start to write out the modified password
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;                              // 0' = 0 ^ ob
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            {
            last2 = ((DWORD *)pClear->pBlobData)[dex];  // 1 
            pdwCypher[dex] = last2 ^ last;              // 1' = 1 ^ 0
            last = last2;                   // save 1 for the 2 round
            }
        }

    // if we have bits left over
    // note that dwSize is computed now in bits
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
        pdwCypher[dex] &= ((DWORD)-1) << dwSize;
        pdwCypher[dex] |=
            ((((DWORD *)pClear->pBlobData)[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

static HRESULT _XORDecodeProp(const BLOB *const pEncoded, BLOB *const pClear)
{
    DWORD       dwSize;
    DWORD       last;
    DWORD       *pdwCypher;
    DWORD       dex;

    // we use CoTaskMemAlloc to be in line with the PST implementation
    pClear->cbSize = pEncoded->pBlobData[2];
    MemAlloc((void **)&pClear->pBlobData, pClear->cbSize);
    if (!pClear->pBlobData)
        return E_OUTOFMEMORY;
    
    // should have been tested by now
    Assert(FDataIsValidV1(pEncoded->pBlobData));
    Assert(!FDataIsPST(pEncoded->pBlobData));

    // nevermind that the pointer is offset by the header size, this is
    // where the password starts
    pdwCypher = (DWORD *)&(pEncoded->pBlobData[PROT_SIZEOF_XORHEADER]);

    dex = 0;
    last = OBFUSCATOR;
    if (dwSize = pClear->cbSize / sizeof(DWORD))
        {
        // case where data is >= 4 bytes
        for (; dex < dwSize; dex++)
            last = ((DWORD *)pClear->pBlobData)[dex] = pdwCypher[dex] ^ last;
        }

    // if we have bits left over
    if (dwSize = (pClear->cbSize % sizeof(DWORD))*8)
        {
        // need to not munge memory that isn't ours
        last >>= sizeof(DWORD)*8-dwSize;
        ((DWORD *)pClear->pBlobData)[dex] &= ((DWORD)-1) << dwSize;
        ((DWORD *)pClear->pBlobData)[dex] |=
                ((pdwCypher[dex] & (((DWORD)-1) >> (sizeof(DWORD)*8-dwSize))) ^ last);
        }

    return S_OK;
}

/*
    EncodeUserPassword

    Encrypt the passed in password.  This encryption seems to
    add an extra 6 bytes on to the beginning of the data
    that it passes back, so we need to make sure that the 
    lpszPwd is large enough to hold a few extra characters.
    *cb should be different on return than it was when it 
    was passed in.

    Parameters:
    lpszPwd - on entry, a c string containing the password.
    on exit, it is the encrypted data, plus some header info.

    cb - the size of lpszPwd on entry and exit.  Note that it should
    include the trailing null, so "foo" would enter with *cb == 4.
*/
void EncodeUserPassword(TCHAR *lpszPwd, ULONG *cb)
{
    BLOB            blobClient;
    BLOB            blobProp;

    blobClient.pBlobData= (BYTE *)lpszPwd;
    blobClient.cbSize   = *cb;
    blobProp.pBlobData  = NULL;
    blobProp.cbSize     = 0;
    
    _XOREncodeProp(&blobClient, &blobProp);
    
    if (blobProp.pBlobData)
    {
        memcpy(lpszPwd, blobProp.pBlobData, blobProp.cbSize);
        *cb = blobProp.cbSize;
        MemFree(blobProp.pBlobData);
    }
}

/*
    DecodeUserPassword

    Decrypt the passed in data and return a password.  This 
    encryption seems to add an extra 6 bytes on to the beginning 
    so decrupting will result in a using less of lpszPwd.
    .
    *cb should be different on return than it was when it 
    was passed in.

    Parameters:
    lpszPwd - on entry, the encrypted password plus some 
    header info. 
    on exit, a c string containing the password.

    cb - the size of lpszPwd on entry and exit.  Note that it should
    include the trailing null, so "foo" would leave with *cb == 4.
*/
void DecodeUserPassword(TCHAR *lpszPwd, ULONG *cb)
{
    BLOB            blobClient;
    BLOB            blobProp;

    blobClient.pBlobData= (BYTE *)lpszPwd;
    blobClient.cbSize   = *cb;
    blobProp.pBlobData  = NULL;
    blobProp.cbSize     = 0;
    
    _XORDecodeProp(&blobClient, &blobProp);

    if (blobProp.pBlobData)
    {
        memcpy(lpszPwd, blobProp.pBlobData, blobProp.cbSize);
        lpszPwd[blobProp.cbSize] = 0;
        *cb = blobProp.cbSize;
        MemFree(blobProp.pBlobData);
    }
}


// --------------------------------------------------------------------------------
// MemInit
// --------------------------------------------------------------------------------
VOID MemInit()
{
	// Create OLE Task Memory Allocator
    if (!g_pMalloc)
        CoGetMalloc(1, &g_pMalloc);
    assert(g_pMalloc);
}

// --------------------------------------------------------------------------------
// MemUnInit
// --------------------------------------------------------------------------------
VOID MemUnInit()
{
	// Release OLE Task Memory Allocator
    if (g_pMalloc)
        g_pMalloc->Release();
}


// --------------------------------------------------------------------------------
// ZeroAllocate
// --------------------------------------------------------------------------------
LPVOID ZeroAllocate(DWORD cbSize)
{
    LPVOID pv = g_pMalloc->Alloc(cbSize);
    if (pv)
        ZeroMemory(pv, cbSize);
    return pv;
}

// --------------------------------------------------------------------------------
// MemAlloc
// --------------------------------------------------------------------------------
BOOL MemAlloc(LPVOID* ppv, ULONG cb) 
{
    assert(ppv && cb);
    *ppv = g_pMalloc->Alloc(cb);
    if (NULL == *ppv)
        return FALSE;
    return TRUE;
}

// --------------------------------------------------------------------------------
// MemRealloc
// --------------------------------------------------------------------------------
BOOL MemRealloc(LPVOID *ppv, ULONG cbNew) 
{
    assert(ppv && cbNew);
    LPVOID pv = g_pMalloc->Realloc(*ppv, cbNew);
    if (NULL == pv)
        return FALSE;
    *ppv = pv;
    return TRUE;
}

// --------------------------------------------------------------------------------
//  operator new and operator delete implmented to allocate memory using 
//  the COM allocator
// --------------------------------------------------------------------------------
void * __cdecl operator new(
    size_t size
    )
{
    void *pv;
    
    if (!g_pMalloc)
        MemInit();

    MemAlloc(&pv, size);

    return pv;
}

void __cdecl operator delete(
    void *ptr
    ) throw()
{
    if (NULL != ptr)
        MemFree(ptr);
}


// --------------------------------------------------------------------------------
//  Functions to convert GUIDs to ascii strings
// --------------------------------------------------------------------------------

int AStringFromGUID(GUID *puid,  TCHAR *lpsz, int cch)
{
    WCHAR   wsz[255];
    int     i;

    i = StringFromGUID2(*puid, wsz, 255);

    if (WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpsz, cch, NULL, NULL) == 0)
        return 0;
    
    return (lstrlen(lpsz) + 1);
}


HRESULT GUIDFromAString(TCHAR *lpsz, GUID *puid)
{
    WCHAR   wsz[255];
    HRESULT hr;

    if (MultiByteToWideChar(CP_ACP, 0, lpsz, -1, wsz, 255) == 0)
        return GetLastError();

    hr = CLSIDFromString(wsz, puid);
    
    return hr;
}



// ****************************************************************************************************
//  CNotifierList Class
//
//  A really basic IUnknown list class.  Actually, its a IUnknown array class, but you don't need to know
//  that.
//


CNotifierList::CNotifierList()
{
    m_count = 0;
    m_ptrCount = 0;
    m_entries = NULL;
    m_nextCookie = 1;
    m_cRef = 1;
    InitializeCriticalSection(&m_rCritSect);
}

/*
    CNotifierList::~CNotifierList

    Clean up any memory that was allocated in the CNotifierList object
*/
CNotifierList::~CNotifierList()
{
    if (m_entries)
    {
        for (int i = 0; i < m_count; i++)
        {
            if (m_entries[i].punk)
            {
                m_entries[i].punk->Release();
                m_entries[i].punk = NULL;
                m_entries[i].dwCookie = 0;
            }
        }
        MemFree(m_entries);
        m_entries = NULL;
        m_count = 0;
    }
    DeleteCriticalSection(&m_rCritSect);
}


STDMETHODIMP_(ULONG) CNotifierList::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CNotifierList::Release()
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

/*
    CNotifierList::Add

    Add a IUnknown to the end of the IUnknown list.
*/
HRESULT    CNotifierList::Add(IUnknown *punk, DWORD *pdwCookie)
{
    TraceCall("Identity - CNotifierList::Add");

    EnterCriticalSection(&m_rCritSect);
    // make more room for pointers, if necessary
    if (m_ptrCount == m_count)
    {
        m_ptrCount += 5;
        if (!MemRealloc((void **)&m_entries, sizeof(UNKLIST_ENTRY) * m_ptrCount))
        {
            m_ptrCount -= 5;
            Assert(false);
            LeaveCriticalSection(&m_rCritSect);    
            return E_OUTOFMEMORY;
        }

        // initialize the new IUnknowns to nil
        for (int i = m_count; i < m_ptrCount; i++)
        {
            ZeroMemory(&m_entries[i], sizeof(UNKLIST_ENTRY));
        }
    }
    
    //now put the IUnknown in the next location
    int iNewIndex = m_count++;
    
    punk->AddRef();
    m_entries[iNewIndex].punk = punk;
    m_entries[iNewIndex].bState = NS_NONE;
    m_entries[iNewIndex].dwCookie = ++m_nextCookie;
    m_entries[iNewIndex].dwThreadId = GetCurrentThreadId();
    *pdwCookie = m_entries[iNewIndex].dwCookie;
    LeaveCriticalSection(&m_rCritSect);  
    CreateNotifyWindow();
    return S_OK;
}

/*
    CNotifierList::Remove
    
    Remove a IUnknown at zero based index iIndex 
*/

HRESULT CNotifierList::Remove(int   iIndex)
{
    int     iCopySize;

    TraceCall("Identity - CNotifierList::Remove");

    EnterCriticalSection(&m_rCritSect);
    iCopySize = ((m_count - iIndex) - 1) * sizeof(UNKLIST_ENTRY);

    // free the memory for the IUnknown
    if (m_entries[iIndex].punk)
    {
        ReleaseWindow();
        m_entries[iIndex].punk->Release();
        ZeroMemory(&m_entries[iIndex], sizeof(UNKLIST_ENTRY));
    }

    // move the other IUnknowns down
    if (iCopySize)
    {
        memmove(&(m_entries[iIndex]), &(m_entries[iIndex+1]), iCopySize);
    }

    // null out the last item in the list and decrement the counter.
    m_entries[--m_count].punk = NULL;
    LeaveCriticalSection(&m_rCritSect); 
    return S_OK;
}

/*
    CNotifierList::RemoveCookie
    
    Remove an IUnknown by its cookie
*/

HRESULT    CNotifierList::RemoveCookie(DWORD dwCookie)
{
    int     iIndex;

    for (iIndex = 0; iIndex < m_count; iIndex++)
    {
        if (m_entries[iIndex].dwCookie == dwCookie)
        {
            return Remove(iIndex);
        }
    }
    return E_FAIL;
}

/*
    CNotifierList::GetAtIndex
    
    Return the pointer to the IUnknown at zero based index iIndex.

    Return the IUnknown at the given index.  Note that the object pointer
    is still owned by the IUnknown list and should not be deleted.
*/

HRESULT     CNotifierList::GetAtIndex(int iIndex, IUnknown **ppunk)
{
    HRESULT hr = E_FAIL;

    EnterCriticalSection(&m_rCritSect);
    if (iIndex < m_count && iIndex >= 0 && m_entries[iIndex].punk)
    {
        *ppunk = m_entries[iIndex].punk;
        (*ppunk)->AddRef();
        hr = S_OK;
    }
    else
        *ppunk = NULL;

    LeaveCriticalSection(&m_rCritSect);    
    return hr;
}


HRESULT     CNotifierList::CreateNotifyWindow()
{
    DWORD   dwThreadCount = 0;
    DWORD   dwThreadId = GetCurrentThreadId();
    int     iIndex;
    int     iFound = -1;
    HWND    hwnd = NULL;

    for (iIndex = 0; iIndex < m_count; iIndex++)
    {
        if (m_entries[iIndex].dwThreadId == dwThreadId)
        {
            iFound  = iIndex;
            if (!hwnd)
                hwnd = m_entries[iIndex].hwnd;
            else
            {
                Assert(NULL == m_entries[iIndex].hwnd || hwnd == m_entries[iIndex].hwnd);
                m_entries[iIndex].hwnd = hwnd;
            }
            dwThreadCount++;
        }
    }
    
    if (dwThreadCount == 1 && iFound >= 0)
    {
        hwnd = m_entries[iFound].hwnd = CreateWindow(c_szNotifyWindowClass, c_szNotifyWindowClass, WS_POPUP, 
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, 
                    NULL, g_hInst, this);
        
        if (m_entries[iFound].hwnd)
            SetWindowLongPtr(m_entries[iFound].hwnd, GWLP_USERDATA, (LRESULT)this);
    }
    return (hwnd ? S_OK : E_FAIL);
}


HRESULT     CNotifierList::ReleaseWindow()
{
    DWORD   dwThreadCount = 0;
    DWORD   dwThreadId = GetCurrentThreadId();
    int     iIndex;
    HWND    hwnd = NULL;

    for (iIndex = 0; iIndex < m_count; iIndex++)
    {
        if (m_entries[iIndex].dwThreadId == dwThreadId)
        {
            if (dwThreadCount == 0)
                hwnd = m_entries[iIndex].hwnd;
            dwThreadCount++;
        }
    }
    
    if (dwThreadCount == 1 && hwnd)
    {
        SendMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return S_OK;
}

HRESULT     CNotifierList::PreNotify()
{
    DWORD   dwThreadId = GetCurrentThreadId();
    int     iIndex;

    for (iIndex = m_count -1; iIndex >= 0; iIndex--)
    {
        if (m_entries[iIndex].dwThreadId == dwThreadId && NULL != m_entries[iIndex].punk)
            m_entries[iIndex].bState = NS_PRE_NOTIFY;
//        else          //BUG 47472, this could cause problems during re-entrant calls to SendNotification
//            m_entries[iIndex].bState = NS_NONE;
    }
    return S_OK;
}

int     CNotifierList::GetNextNotify()
{
    DWORD   dwThreadId = GetCurrentThreadId();
    int     iIndex;
    for (iIndex = m_count -1; iIndex >= 0; iIndex--)
    {
        if (m_entries[iIndex].dwThreadId == dwThreadId && NULL != m_entries[iIndex].punk && NS_PRE_NOTIFY == m_entries[iIndex].bState)
            return iIndex;
    }
    return -1;
}

HRESULT     CNotifierList::SendNotification(UINT msg, DWORD dwType)
{
    DWORD   dwThreadCount = 0, dwOldCount;
    DWORD   dwThreadId = GetCurrentThreadId();
    int     iIndex;
    HWND    hwnd = NULL;
    HRESULT hr = S_OK;

#if defined(DEBUG)
    DebugStrf("Identity - CNotifierList::SendNotification %ld\r\n", msg);
#endif

    AddRef();

    PreNotify();

    while ((iIndex = GetNextNotify()) != -1)
    {
        IUnknown    *punk;
        IIdentityChangeNotify    *pICNotify;

        punk = m_entries[iIndex].punk;
        m_entries[iIndex].bState = NS_NOTIFIED;

        punk->AddRef();
        if (SUCCEEDED(punk->QueryInterface(IID_IIdentityChangeNotify, (void **)&pICNotify)) && pICNotify)
        {
            switch (msg)
            {
                case WM_QUERY_IDENTITY_CHANGE:
                    if (FAILED(hr = pICNotify->QuerySwitchIdentities()))
                    {
                        punk->Release();
                        pICNotify->Release();
                        goto exit;
                    }
                    break;

                case WM_IDENTITY_CHANGED:
                    pICNotify->SwitchIdentities();
                    break;

                case WM_IDENTITY_INFO_CHANGED:
                    pICNotify->IdentityInformationChanged(dwType);
                    break;

            }

            pICNotify->Release();
        }
        punk->Release();
    }

exit:
    Release();
    return hr;
}

#ifdef DEBUG

// --------------------------------------------------------------------------------
// DebugStrf
// --------------------------------------------------------------------------------
void DebugStrf(LPTSTR lpszFormat, ...)
{
    static TCHAR szDebugBuff[500];
    va_list arglist;

    va_start(arglist, lpszFormat);
    wvsprintf(szDebugBuff, lpszFormat, arglist);
    va_end(arglist);

    OutputDebugString(szDebugBuff);
}
#endif



// ---------------------------------------------------------------------------------
// Pstore code for storing passwords
// ---------------------------------------------------------------------------------
// Functions related to saving and restoring user passwords from the pstore.


// We have wrappers around Create and Release to allow for future caching of the pstore
// instance within webcheck. 

STDAPI CreatePStore(IPStore **ppIPStore)
{
    HRESULT hr;

    hr = PStoreCreateInstance ( ppIPStore,
                                NULL,
                                NULL,
                                0);
    return hr;
}


STDAPI ReleasePStore(IPStore *pIPStore)
{
    HRESULT hr;

    if (pIPStore)
    {
        pIPStore->Release();
        hr = S_OK;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}


STDAPI  ReadIdentityPassword(GUID *puidIdentity, PASSWORD_STORE  *pPwdStore)
{
    GUID             itemType = GUID_NULL;
    GUID             itemSubtype = GUID_NULL;
    PST_PROMPTINFO   promptInfo = {0};
    IPStore*         pStore = NULL;
    HRESULT          hr ;
     
    if (pPwdStore == NULL)
        return E_POINTER;

    promptInfo.cbSize = sizeof(promptInfo);
    promptInfo.szPrompt = NULL;
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp = NULL;
    
    hr = CreatePStore(&pStore);    

    if (SUCCEEDED(hr))
    {
        Assert(pStore != NULL);

        itemType = GUID_PStoreType;
        itemSubtype = *puidIdentity;

        if (SUCCEEDED(hr))
        {
            DWORD           cbData;
            BYTE           *pbData = NULL;
            

            hr = pStore->ReadItem(
                            s_Key,
                            &itemType,
                            &itemSubtype,
                            c_szIdentityPass,
                            &cbData,
                            &pbData,
                            &promptInfo,
                            0);

            if (SUCCEEDED(hr))
            {
                CopyMemory(pPwdStore, pbData, (cbData <= sizeof(PASSWORD_STORE) ? cbData : sizeof(PASSWORD_STORE)));
                CoTaskMemFree(pbData);

                hr = S_OK;
            }
        }

        ReleasePStore(pStore);
    }

    return hr;
}

STDAPI WriteIdentityPassword(GUID *puidIdentity, PASSWORD_STORE  *pPwdStore)
{
    HRESULT         hr;
    PST_TYPEINFO    typeInfo;
    PST_PROMPTINFO  promptInfo;
    IPStore *       pStore;

    typeInfo.cbSize = sizeof(typeInfo);

    typeInfo.szDisplayName = c_szIdentityMgr;

    promptInfo.cbSize = sizeof(promptInfo);
    promptInfo.dwPromptFlags = 0;
    promptInfo.hwndApp = NULL;
    promptInfo.szPrompt = NULL;

    hr = CreatePStore(&pStore);

    if (SUCCEEDED(hr))
    {
        GUID itemType = GUID_NULL;
        GUID itemSubtype = GUID_NULL;

        Assert(pStore != NULL);

        itemType = GUID_PStoreType;
        itemSubtype = *puidIdentity;
        
        if (SUCCEEDED(hr))
        {
            hr = pStore->CreateType(s_Key, &itemType, &typeInfo, 0);

            // PST_E_TYPE_EXISTS implies type already exists which is just fine
            // by us.
            if (SUCCEEDED(hr) || hr == PST_E_TYPE_EXISTS)
            {
                typeInfo.szDisplayName = c_szIdentities;

                hr = pStore->CreateSubtype(
                                        s_Key,
                                        &itemType,
                                        &itemSubtype,
                                        &typeInfo,
                                        NULL,
                                        0);

                if (SUCCEEDED(hr) || hr == PST_E_TYPE_EXISTS)
                {
                    if (pPwdStore != NULL)
                    {
                        hr = pStore->WriteItem(
                                            s_Key,
                                            &itemType,
                                            &itemSubtype,
                                            c_szIdentityPass,
                                            (sizeof(PASSWORD_STORE)),
                                            (BYTE *)pPwdStore,
                                            &promptInfo,
                                            PST_CF_NONE,
                                            0);
                    }
                    else
                    {
                        hr = pStore->DeleteItem(
                                            s_Key,
                                            &itemType,
                                            &itemSubtype,
                                            c_szIdentityPass,
                                            &promptInfo,
                                            0);
                    }
                }
            }
        }
        
        ReleasePStore(pStore);
    }
    
    return hr;
}              

#define CH_WHACK TEXT(FILENAME_SEPARATOR)
                                                         
// rips the last part of the path off including the backslash
//      C:\foo      -> C:\      ;
//      C:\foo\bar  -> C:\foo
//      C:\foo\     -> C:\foo
//      \\x\y\x     -> \\x\y
//      \\x\y       -> \\x
//      \\x         -> ?? (test this)
//      \foo        -> \  (Just the slash!)
//
// in/out:
//      pFile   fully qualified path name
// returns:
//      TRUE    we stripped something
//      FALSE   didn't strip anything (root directory case)
//
//      Stolen from shlwapi\path.c

STDAPI_(BOOL) _PathRemoveFileSpec(LPTSTR pFile)
{
    LPTSTR pT;
    LPTSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == CH_WHACK)
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == TEXT(':')) {   // skip ":\" so we don't
            if (pT2[1] ==TEXT('\\'))    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything

    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == CH_WHACK)) {
        // Is it just a '\'?
        if (*(pT+1) != TEXT('\0')) {
            // Nope.
            *(pT+1) = TEXT('\0');
            return TRUE;        // stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;    // stripped something
    }
}
