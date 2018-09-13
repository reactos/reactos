//  CCtl.CPP
//
//      Implementation of the control
//
//  Created 17-Apr-98 [JonT]

#include "priv.h"
#include "resource.h"
#include "cctl.h"
#include "util.h"
#include "dump.h"       // for Dbg_* functions


//--------------------------------------------------------------------
//
//
//  CARPCtl class
//
//
//--------------------------------------------------------------------


// ARPCtlEnumCB
//      Callback for EnumAppItems that adds the appdata item to
//      the array.

void CALLBACK ARPCtlEnumCB(CAppData * pcad, LPARAM lParam)
{
    ((CARPCtl *)lParam)->EnumCallback(pcad);
}


//--------------------------------------------------------------------
// Methods


// CARPCtl::EnumCallback
//      EnumAppItems callback for the ARPCtl.

void CARPCtl::EnumCallback(CAppData * pcad)
{
    switch (_dwEnum)
    {
    case ENUM_INSTALLED:
    case ENUM_PUBLISHED:    
    case ENUM_OCSETUP:    
        if (S_OK == _pmtxarray->AddItem(pcad, NULL))
        {
            _dwcItems++;
            TraceMsg(TF_CTL, "ARP: Added item \"%s\" to list", pcad->GetData()->pszDisplayName);

            // Fire the event saying the new row is ready.
            // Note that this has to be AFTER we increment so that
            // the ItemCount property will be correct at this point.
            Fire_OnRowReady(_dwcItems - 1);
        }
        break;

    case ENUM_CATEGORIES:
        if (S_OK == _pmtxarray->AddItem(pcad, NULL))
        {
            _dwcItems++;
            TraceMsg(TF_CTL, "ARP: Added item \"%s\" to list", pcad->GetCategory());

            // Fire the event saying the new row is ready.
            // Note that this has to be AFTER we increment so that
            // the ItemCount property will be correct at this point.
            Fire_OnRowReady(_dwcItems - 1);
        }
        break;
    }
}


//  CARPCtl::InitData
//      Script tells the control to get data from the app management object
//      and which order to sort in.
//
//      bstrEnum can be:
//          "installed" - enumerate currently installed apps.
//          ""          - ditto.
//          "published" - enumerate published apps.
//          "categories" - enumerate published categories.

STDMETHODIMP
CARPCtl::InitData(
    BSTR bstrEnum,
    DWORD dwSortOrder
    )
{
    HRESULT hres = S_FALSE;

    // Determine what we're enumerating
    _dwEnum = ENUM_INSTALLED;               // default value
    if (0 == lstrcmpiW(bstrEnum, L"published"))
        _dwEnum = ENUM_PUBLISHED;
    else if (0 == lstrcmpiW(bstrEnum, L"categories"))
        _dwEnum = ENUM_CATEGORIES;
    else if (0 == lstrcmpiW(bstrEnum, L"ocsetup"))
        _dwEnum = ENUM_OCSETUP;

        
    // Load the app manager if we haven't already
    if (_pam == NULL)
    {
        if (FAILED(CoCreateInstance(CLSID_ShellAppManager, NULL,
            CLSCTX_INPROC, IID_IShellAppManager, (void**)&_pam)))
        {
            TraceMsg(TF_ERROR, "Couldn't instantiate ShellAppManager object");
            return S_FALSE;
        }
    }

    // Make sure the worker thread isn't already running. Stop it if it is.
    _workerthread.Kill();

    // If we already have a list, nuke it
    _FreeAppData();

    // Now create the new list
    _pmtxarray = new CMtxArray2(_dwEnum);
    if (_pmtxarray)
    {
        // Start enumerating items
        hres = EnumAppItems(_dwEnum, _pam, ARPCtlEnumCB, (LPARAM)this);

        // Did the enumeration succeed?
        if (SUCCEEDED(hres))
        {
            // Yes; tell the script we're done getting the synchronous data
            
            // This call is synchronous and does not return until the script 
            // is finished responding.  This seems like a bug in Trident.
            Fire_OnSyncDataReady();

            // We only get slow info for the installed apps
            if (ENUM_INSTALLED == _dwEnum)
            {
                // Create and kick off the worker thread
                hres = _workerthread.Initialize(SAFECAST(this, IWorkerEvent *), _pmtxarray);
            }
        }
    }

    // Can't return failure to script
    if (FAILED(hres))
        hres = S_FALSE;
        
    return hres;
}


//  CARPCtl::MoveFirst
//      Script tells control to move to the first item in the list.
//      Returns false if there are no items.

STDMETHODIMP
CARPCtl::MoveFirst(
    BOOL* pbool
    )
{
    // Set our current index to the start
    _dwCurrentIndex = 0;

    // Return TRUE iff we are pointing to a valid item
    *pbool = _dwCurrentIndex >= _dwcItems ? FALSE : TRUE;

    return S_OK;
}


//  CARPCtl::MoveNext
//      Script tells control to move to the next item in the curren tlist.
//      Returns false if off the end of the list.

STDMETHODIMP
CARPCtl::MoveNext(
    BOOL* pbool
    )
{
    _dwCurrentIndex++;

    // Return TRUE iff we are pointing to a valid item
    *pbool = _dwCurrentIndex >= _dwcItems ? FALSE : TRUE;

    return S_OK;
}


//  CARPCtl::MoveTo
//      Tells the control to move to a specific item

STDMETHODIMP
CARPCtl::MoveTo(
    DWORD dwRecNum,
    BOOL* pbool
    )
{
    // If they want to go past the end, fail it and don't move the pointer
    if (dwRecNum >= _dwcItems)
    {
        *pbool = FALSE;
        return S_OK;
    }

    // Update the pointer and return success
    _dwCurrentIndex = dwRecNum;
    *pbool = TRUE;
    return S_OK;
}


//  CARPCtl::Exec
//      Tells the control to exec a command.  The command may act
//      upon the current record.

STDMETHODIMP
CARPCtl::Exec(
    BSTR bstrCmd
    )
{
    TraceMsg(TF_CTL, "(Ctl) Command (%ls) called", bstrCmd);

#ifdef NOTYET    
    // security check must pass before we could exec anything. 
    if (!_fSecure)
    {
        TraceMsg(TF_CTL, "(Ctl) Security blocked Exec call");
        return S_FALSE;        // scripting methods cannot return failure
    }
#endif    
    
    const static struct {
        LPCWSTR pszCmd;
        APPCMD  appcmd;
    } s_rgCmds[] = {
        { L"install",           APPCMD_INSTALL },
        { L"uninstall",         APPCMD_UNINSTALL },
        { L"modify",            APPCMD_MODIFY },
        { L"upgrade",           APPCMD_UPGRADE },
        { L"repair",            APPCMD_REPAIR },
        { L"generic install",   APPCMD_GENERICINSTALL },
        { L"ntoptions",         APPCMD_NTOPTIONS },
        { L"winupdate",         APPCMD_WINUPDATE },
    };

    int i;
    APPCMD appcmd = APPCMD_UNKNOWN;

    for (i = 0; i < ARRAYSIZE(s_rgCmds); i++)
    {
        if (0 == StrCmpIW(bstrCmd, s_rgCmds[i].pszCmd))
        {
            appcmd = s_rgCmds[i].appcmd;
            break;
        }
    }

    switch (appcmd)
    {
        case APPCMD_INSTALL:
        case APPCMD_UNINSTALL:
        case APPCMD_MODIFY:
        case APPCMD_UPGRADE:
        case APPCMD_REPAIR:
            {
                CAppData * pappdata = _GetAppData(_dwCurrentIndex);
                if (pappdata)
                    pappdata->DoCommand(appcmd);
            }
            break;

        case APPCMD_GENERICINSTALL:
            InstallAppFromFloppyOrCDROM(NULL);
            break;

        case APPCMD_NTOPTIONS:
            // command to invoke and OCMgr
            // "sysocmgr /x /i:%systemroot%\system32\sysoc.inf"
            TCHAR szInf[MAX_PATH];
            if (GetSystemDirectory(szInf, ARRAYSIZE(szInf)) && PathCombine(szInf, szInf, TEXT("sysoc.inf")))
            {
                TCHAR szParam[MAX_PATH];
                wsprintf(szParam, TEXT("/x /i:%s"), szInf);
                ShellExecute(NULL, NULL, TEXT("sysocmgr"), szParam, NULL, SW_SHOWDEFAULT);
            }
            break;

        case APPCMD_WINUPDATE:
            break;

        case APPCMD_UNKNOWN:
            TraceMsg(TF_ERROR, "(Ctl) Received invalid appcmd %ls", bstrCmd);
            break;
    }
        
    return S_OK;
}


//  IWorkerEvent::FireOnDataReady
//      Called by worker thread when some data is ready.

STDMETHODIMP 
CARPCtl::FireOnDataReady(
    LONG iRow
    )
{
    Fire_OnAsyncDataReady(iRow);
    return S_OK;
}


//  IWorkerEvent::FireOnFinished
//      Called by worker thread when it is complete.

STDMETHODIMP 
CARPCtl::FireOnFinished(void)
{
    return S_OK;
}





//--------------------------------------------------------------------
//  Properties

// SIMPLE_PROPERTY_GET
//
//  Defines a simple property get method so that we don't type the same
//  code over and over. It only works for strings copied from the APPINFODATA
//  structure.
//
//  This keeps the code cleaned up. but doesn't help
//  with the code bloat, so a better approach would be great.

#define SIMPLE_PROPERTY_GET(PropName)                                       \
STDMETHODIMP                                                                \
CARPCtl::get_##PropName##(BSTR* pbstr)                                      \
{                                                                           \
    USES_CONVERSION;                                                        \
                                                                            \
    if (_dwCurrentIndex >= _dwcItems)                                       \
        return E_FAIL;                                                      \
                                                                            \
    CAppData * pappdata = _GetAppData(_dwCurrentIndex);                     \
    if (pappdata)                                                           \
    {                                                                       \
        APPINFODATA * paidata = pappdata->GetData();                        \
                                                                            \
        ASSERT(NULL == paidata->psz##PropName || IS_VALID_STRING_PTRW(paidata->psz##PropName, -1)); \
                                                                            \
        *pbstr = W2BSTR(paidata->psz##PropName);                            \
    }                                                                       \
    else                                                                    \
        *pbstr = NULL;                                                      \
                                                                            \
    return S_OK;                                                            \
}

// TODO: Since this is big code bloat, make sure we really need all these...

SIMPLE_PROPERTY_GET(Version)
SIMPLE_PROPERTY_GET(Publisher)
SIMPLE_PROPERTY_GET(ProductID)
SIMPLE_PROPERTY_GET(RegisteredOwner)
SIMPLE_PROPERTY_GET(Language)
SIMPLE_PROPERTY_GET(SupportUrl)
SIMPLE_PROPERTY_GET(SupportTelephone)
SIMPLE_PROPERTY_GET(HelpLink)
SIMPLE_PROPERTY_GET(InstallLocation)
SIMPLE_PROPERTY_GET(InstallSource)
SIMPLE_PROPERTY_GET(InstallDate)
SIMPLE_PROPERTY_GET(RequiredByPolicy)
SIMPLE_PROPERTY_GET(Contact)

// DisplayName
//      The display name of the item.

STDMETHODIMP
CARPCtl::get_DisplayName(BSTR* pbstr)
{
    USES_CONVERSION;
    CAppData * pappdata;

    if (_dwCurrentIndex >= _dwcItems)
        return E_FAIL;

    pappdata = _GetAppData(_dwCurrentIndex);
    if (pappdata)
    {
        if (ENUM_CATEGORIES == _dwEnum)
        {
            *pbstr = W2BSTR(pappdata->GetCategory());
        }
        else
        {
            *pbstr = W2BSTR(pappdata->GetData()->pszDisplayName);
        }
    }
    else
        *pbstr = NULL;
    
    return S_OK;
}

//  Size
//      The calculated size of the application. Returns "Unknown" if not available

STDMETHODIMP
CARPCtl::get_Size(BSTR* pbstr)
{
    USES_CONVERSION;
    TCHAR szSize[256];
    ULONG ulSize = 0;
    LPTSTR WINAPI ShortSizeFormat(DWORD dw, LPTSTR szBuf);
    CAppData * pappdata;

    if (_dwCurrentIndex >= _dwcItems)
        return E_FAIL;

    // Get the size and truncate to a ULONG. If the app is bigger than 4G,
    // well, too bad.
    pappdata = _GetAppData(_dwCurrentIndex);
    if (pappdata)
        ulSize = (ULONG)pappdata->GetSlowData()->ullSize;

    // If the size is zero, return unknown, otherwise,
    // Use the shell32 function to make a nicely formatted size string
    if (ulSize == 0)
        LoadString(g_hinst, IDS_UNKNOWN, szSize, ARRAYSIZE(szSize));
    else
        ShortSizeFormat(ulSize, szSize);

    // Return as a BSTR
    *pbstr = W2BSTR(szSize);
    return S_OK;
}


//  TimesUsed
//      Returns the number of times used for this item

STDMETHODIMP
CARPCtl::get_TimesUsed(BSTR* pbstr)
{
    USES_CONVERSION;
    int ncUsed = 0;
    WCHAR szUsed[256];
    CAppData * pappdata;

    if (_dwCurrentIndex >= _dwcItems)
        return E_FAIL;

    pappdata = _GetAppData(_dwCurrentIndex);
    if (pappdata)
        ncUsed = pappdata->GetSlowData()->iTimesUsed;

    // Convert to a BSTR
    wsprintf(szUsed, TEXT("%d"), ncUsed);
    *pbstr = W2BSTR(szUsed);
    return S_OK;
}


//  LastUsed
//      Returns last date the app was used

STDMETHODIMP
CARPCtl::get_LastUsed(BSTR* pbstr)
{
    USES_CONVERSION;
    FILETIME ft = {0};
    WCHAR szDate[256];
    CAppData * pappdata;

    if (_dwCurrentIndex >= _dwcItems)
        return E_FAIL;

    pappdata = _GetAppData(_dwCurrentIndex);
    if (pappdata)
        ft = pappdata->GetSlowData()->ftLastUsed;

    // Convert to a BSTR
    FileTimeToDateTimeString(&ft, szDate, SIZECHARS(szDate));
    *pbstr = W2BSTR(szDate);
    return S_OK;
}


//  Capability
//      Flags that indicate the possible actions that can
//      be performed on the item.  See APPACTION_* flags.

STDMETHODIMP
CARPCtl::get_Capability(long * pVal)
{
    CAppData * pappdata;
    
    if (_dwCurrentIndex >= _dwcItems)
        return E_FAIL;

    pappdata = _GetAppData(_dwCurrentIndex);
    if (pappdata)
        *pVal = pappdata->GetCapability();
    else
        *pVal = 0;

    return S_OK;
}




//  ItemCount
//      Number of items in current list

STDMETHODIMP
CARPCtl::get_ItemCount(long * pVal)
{
    *pVal = _dwcItems;

    return S_OK;
}


//--------------------------------------------------------------------
// Object lifetime stuff

//  CARPCtl constructor

CARPCtl::CARPCtl()
{
    ASSERT(NULL == _pmtxarray);
    ASSERT(NULL == _pam);
}

// CARPCtl destructor

CARPCtl::~CARPCtl()
{
    // Kill the worker thread if it's still around
    _workerthread.Kill();

    // Free our contained object
    ATOMICRELEASE(_pam);

    // Clean up the application list
    _FreeAppData();
}


//--------------------------------------------------------------------
// Private methods


//  CARPCtl::_GetAppData
//      Returns the appdata of the current record, or NULL if there
//      is no such record.

CAppData *
CARPCtl::_GetAppData(DWORD iItem)
{
    CAppData * pappdata = NULL;

    if (_pmtxarray && iItem < _dwcItems)
        pappdata = _pmtxarray->GetAppData(iItem);
        
    return pappdata;
}


//  CARPCtl::_FreeAppData
//      Frees all memory associated with the application

void
CARPCtl::_FreeAppData()
{
    if (_pmtxarray)
    {
        delete _pmtxarray;
        _pmtxarray = NULL;
    }
    _dwcItems = 0;
}




