//
// event.cxx - simple eventing mechanism for ras/offline/logon events
//
#include "wininetp.h"
#include <docobj.h>

//
// Globals
//
static const TCHAR szEventKey[] = REGSTR_PATH_INETEVENTS;

CLSID clsidEventGroup = { /* ab8ed004-b86a-11d1-b1f8-00c04fa357fa */
    0xab8ed004,
    0xb86a,
    0x11d1,
    {0xb1, 0xf8, 0x00, 0xc0, 0x4f, 0xa3, 0x57, 0xfa}
  };

//
// remember events so we don't repeat them
//
static DWORD g_dwOffline = 0;

//
// SendEvent - deliver an event to a single client
//
BOOL SendEvent(DWORD dwEvent, VARIANTARG *pva, LPTSTR pszValue)
{
    CLSID               clsid;
    IOleCommandTarget   *poct;
    HRESULT             hr = E_FAIL;

#ifdef UNICODE
    if(FAILED(CLSIDFromString(pszValue, &clsid)))
        return FALSE;
#else
    WCHAR wszCLSID[80];
    MultiByteToWideChar(CP_ACP, 0, pszValue, -1, wszCLSID, sizeof(wszCLSID) / sizeof(WCHAR));
    if(FAILED(CLSIDFromString(wszCLSID, &clsid)))
        return FALSE;
#endif


    hr = (CoCreateInstance(clsid, NULL, CLSCTX_ALL,
                            IID_IOleCommandTarget, (void **)&poct));
    if(SUCCEEDED(hr))
    {
        // ensure client likes our group
        hr = poct->Exec(&clsidEventGroup, dwEvent, 0, pva, NULL);
        poct->Release();
    }

    return SUCCEEDED(hr);
}


//
// EnumClients - send all events to clients in a reg key
//
DWORD EnumClients(HKEY hkey, DWORD dwEvent, LPWSTR pwsEventDesc, DWORD dwEventData)
{
    DWORD cbData, cbValue, dwType, i, dwMask;
    TCHAR szValueName[80];
    VARIANTARG  va;

    va.vt = VT_EMPTY;

    //
    // Enumerate everyone
    //
    for (i = 0; ; i++)
    {
        LONG lEnum;

        cbValue = sizeof(szValueName) / sizeof(TCHAR);
        cbData = sizeof(DWORD);

        // BUGBUG (Unicode, Davepl) I'm assuming that the data is UNICODE,
        // but I'm not sure who put it there yet... double check.

        if( ( lEnum = RegEnumValue( hkey, i, szValueName, &cbValue, NULL,
                                    &dwType, (LPBYTE)&dwMask, &cbData ) ) == ERROR_MORE_DATA )
        {
            // ERROR_MORE_DATA means the value name or data was too large
            // skip to the next item
            continue;
        }
        else if( lEnum != ERROR_SUCCESS )
        {
            // could be ERROR_NO_MORE_ENTRIES, or some kind of failure
            // we can't recover from any other registry problem, anyway
            break;
        }

        if(0 != (dwMask & dwEvent)) {
            // this guy wants this event
            SendEvent(dwEvent, &va, szValueName);
        }
    }

    return 0;
}



//
// DispatchEvent - enumerate all clients and deliver the event to them
//

DWORD InternetDispatchEvent(DWORD dwEvent, LPWSTR pwsEventDesc, DWORD dwEventData)
{
    HKEY hkey;

    // get rid of repeated events here
    switch(dwEvent) {
    case INETEVT_OFFLINE:
        if(g_dwOffline == dwEvent)
            return 0;
        g_dwOffline = dwEvent;
        break;
    case INETEVT_ONLINE:
        if(g_dwOffline == dwEvent)
            return 0;
        g_dwOffline = dwEvent;
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    // fire up com
    if(SUCCEEDED(CoInitialize(NULL))) {

        if (REGOPENKEY(HKEY_CURRENT_USER, szEventKey, &hkey) == ERROR_SUCCESS) {
            EnumClients(hkey, dwEvent, pwsEventDesc, dwEventData);
            REGCLOSEKEY(hkey);
        }

        if (REGOPENKEY(HKEY_LOCAL_MACHINE, szEventKey, &hkey) == ERROR_SUCCESS) {
            EnumClients(hkey, dwEvent, pwsEventDesc, dwEventData);
            REGCLOSEKEY(hkey);
        }

        CoUninitialize();
    }

    return 0;
}
