#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Misc data
/----------------------------------------------------------------------------*/

//
// mapping of class to resource ID's
//

typedef struct
{
    LPCWSTR pObjectClass;
    INT iResource;
} CLASSTORESOURCE, * LPCLASSTORESOURCE;

CLASSTORESOURCE normalIcons[] =
{
    L"builtInDomain",               IDI_BUILTINDOMAIN,
    L"computer",                    IDI_COMPUTER,
    L"configuration",               IDI_CONFIGURATION,
    L"rpcContainer",                IDI_CONFIGURATION,
    L"contact",                     IDI_CONTACT,
    L"container",                   IDI_CONTAINER,
    L"domainDNS",                   IDI_DOMAINDNS,
    L"domainPolicy",                IDI_DOMAINPOLICY,
    L"group",                       IDI_GROUP,
    L"localGroup",                  IDI_GROUP,
    L"localPolicy",                 IDI_LOCALPOLICY,
    L"nTDSConnection",              IDI_NTDSCONNECTION,
    L"nTDSDSA",                     IDI_NTDSDSA,
    L"nTDSSettings",                IDI_NTDSSETTINGS,
    L"organizationalPerson",        IDI_ORGANIZATIONALPERSON,
    L"organizationalUnit",          IDI_ORGANIZATIONALUNIT,
    L"person",                      IDI_PERSON,
    L"printQueue",                  IDI_PRINTQUEUE,
    L"remoteMailRecipient",         IDI_REMOTEMAILRECIPIENT,
    L"server",                      IDI_SERVER,
    L"serverConnection",            IDI_SERVERCONNECTION,
    L"site",                        IDI_SITE,
    L"sitesContainer",              IDI_SITESCONTAINER,
    L"storage",                     IDI_STORAGE,
    L"subnet",                      IDI_SUBNET,
    L"subnetContainer",             IDI_CONTAINER,
    L"user",                        IDI_USER,
    L"volume",                      IDI_VOLUME,
    L"workStationAccount",          IDI_WORKSTATIONACCOUNT,
// added daviddv (05jun98) for jonn
    L"licensingSiteSettings",       IDI_LICENSING,
    L"nTDSSiteSettings",            IDI_NTDSSITESETTINGS,
    L"siteLink",                    IDI_SITELINK,
    L"siteLinkBridge",              IDI_SITELINK,
// added daviddv (19jun98) for jonn
    L"nTFRSSettings",               IDI_NTFRS,    
    L"nTFRSReplicaSet",             IDI_NTFRS,
    L"nTFRSSubscriptions",          IDI_NTFRS,
    L"nTFRSSubscriber",             IDI_NTFRS,
    L"nTFRSMember",                 IDI_NTFRS,
// added daviddv (23jun98) for ericb
    L"foreignSecurityPrincipal",    IDI_USER,
// added daviddv (29oct98) for jonn
    L"interSiteTransport",          IDI_CONTAINER,
    L"interSiteTransportContainer", IDI_CONTAINER, 
    L"serversContainer",            IDI_CONTAINER,
    NULL, NULL,
};

CLASSTORESOURCE openIcons[] =
{
    L"container",                   IDI_CONTAINER_OPEN,
    L"subnetContainer",             IDI_CONTAINER_OPEN,
    L"interSiteTransport",          IDI_CONTAINER_OPEN,
    L"interSiteTransportContainer", IDI_CONTAINER_OPEN, 
    L"serversContainer",            IDI_CONTAINER_OPEN,
    NULL, NULL,
};

CLASSTORESOURCE disabledIcons[] =
{
    L"user",                        IDI_USER_DISABLED,
    L"computer",                    IDI_COMPUTER_DISABLED,
    NULL, NULL,
};


//
// mapping of states to icon tables
//

LPCLASSTORESOURCE state_to_icons[] =
{
    normalIcons,            // DSGIF_ISNORMAL
    openIcons,              // DSGIF_ISOPEN
    disabledIcons,          // DSGIF_ISDISABLED
};


//
// Look up a locally stored icon given its class and state.
//

BOOL _GetIconForState(LPWSTR pObjectClass, INT iState, INT* pindex)
{
    BOOL fFound = FALSE;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_ICON, "_GetIconForState");
    Trace(TEXT("Find icon for class: %s, state: %d"), W2T(pObjectClass), iState);

    if ( iState < ARRAYSIZE(state_to_icons) )
    {
        LPCLASSTORESOURCE pTable = state_to_icons[iState];
        
        for ( i = 0 ; !fFound && pTable[i].pObjectClass ; i++ )
        {
            if ( !StrCmpIW(pTable[i].pObjectClass, pObjectClass) )
            {
                Trace(TEXT("Found icon at index %d"), i);
                *pindex = -pTable[i].iResource;
                fFound = TRUE;
            }
        }        
    }        

    TraceLeaveValue(fFound);
}


#if DOWNLEVEL_SHELL

//
// Win9x user doesn't support PrivateExtractIcons, therefore we use shell32's ExtractIconEx
// with suitable parameter munging.
//

UINT _MyExtractIcons(LPCWSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, 
                                            HICON *phicon, UINT *piconid, UINT nIcons, UINT flags)
{  
    UINT ures = 0;   
    HICON *phLargeIcon = (GetSystemMetrics(SM_CXICON) == cxIcon) ? phicon:NULL;
    HICON *phSmallIcon = (GetSystemMetrics(SM_CXICON) != cxIcon) ? phicon:NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_ICON, "_MyExtractIcons");

    ures = ExtractIconEx(W2CT(szFileName), nIconIndex, phLargeIcon, phSmallIcon, 1);
    Trace(TEXT("ures is %d from ExtractIcons"), ures);

    TraceLeaveValue(ures);
}

#endif


/*-----------------------------------------------------------------------------
/ _GetIconLocation
/ ----------------
/   Given a cache record for the icon, attempt to fetch the icon location from
/    it.
/
/ In:
/   pCacheEntry -> locked cacherecord
/   dwFlags = flags indicating which icon is required
/   pBuffer -> buffer that receives the name
/   cchBuffer = maximum size of the name buffer
/   piIndex = receives the resource ID of the loaded resource
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

HRESULT _GetModuleLocation(LPWSTR pBuffer, INT cchBuffer)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_ICON,"_GetModuleLocation");

#if UNICODE
    if ( !GetModuleFileName(GLOBAL_HINSTANCE, pBuffer, cchBuffer) )
        ExitGracefully(hr, E_FAIL, "Failed to get module location");
#else
    TCHAR szBuffer[MAX_PATH];

    if ( !GetModuleFileName(GLOBAL_HINSTANCE, szBuffer, ARRAYSIZE(szBuffer)) )
        ExitGracefully(hr, E_FAIL, "Failed to get module location");

    MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, pBuffer, cchBuffer);
#endif        

exit_gracefully:

    TraceLeaveResult(hr);
}

HRESULT _GetIconLocation(LPCLASSCACHEENTRY pCacheEntry, DWORD dwFlags, LPWSTR pBuffer, INT cchBuffer, INT* piIndex)
{
    HRESULT hr;
    INT iState = dwFlags & DSGIF_ISMASK;
    USES_CONVERSION;

    TraceEnter(TRACE_ICON, "_GetIconLocation");

    if ( !pBuffer || !piIndex || (iState >= ARRAYSIZE(pCacheEntry->pIconName)) )
        ExitGracefully(hr, E_INVALIDARG, "No class, buffer or index pointer specified")

    // before we get too involved in looking at the cache records lets see if we have
    // one already, if not then bail out now.

    if ( !pCacheEntry )
        ExitGracefully(hr, S_FALSE, "No cache record, returning S_FALSE");

    // look up the class in the cache, if that works try and get the icon string
    // for the given index, if that yeilds a NULL then try normal.  Once we
    // have a string pointer then lets copy that and parse out the resource ID.

    if ( (pCacheEntry->dwCached & CLASSCACHE_ICONS) &&
           (pCacheEntry->pIconName[iState] || pCacheEntry->pIconName[DSGIF_ISNORMAL]) )
    {
        TraceMsg("Reading icon name from the display specifier strings");

        if ( !pCacheEntry->pIconName[iState] )
            iState = DSGIF_ISNORMAL;

        StrCpyNW(pBuffer, pCacheEntry->pIconName[iState], cchBuffer);
        *piIndex = PathParseIconLocationW(pBuffer);
    }
    else
    {
        TraceMsg("Attempting to find icon in our fixed resource table");

        if ( _GetIconForState(pCacheEntry->pObjectClass, iState, piIndex) ||
                _GetIconForState(pCacheEntry->pObjectClass, DSGIF_ISNORMAL, piIndex) )
        {
            hr = _GetModuleLocation(pBuffer, cchBuffer);
            FailGracefully(hr, "Failed to get the module location for dsuiext");
        }
        else
        {
            ExitGracefully(hr, S_FALSE, "Failed to find icon bound resources");
        }
    }

    Trace(TEXT("Location: %s, Index: %d"), W2T(pBuffer), *piIndex);
    hr = S_OK;

exit_gracefully:

    //
    // if we failed to look up the icon location, and the caller requested the 
    // default document icon then lets return the shell def document image
    //

    if ( (hr == S_FALSE) )      
    {
        if ( dwFlags & DSGIF_DEFAULTISCONTAINER )
        {
            hr = E_FAIL;

            if ( _GetIconForState(L"container", iState, piIndex) ||
                    _GetIconForState(L"container", DSGIF_ISNORMAL, piIndex) )
            {
                hr = _GetModuleLocation(pBuffer, cchBuffer);
            }
            else if ( dwFlags & DSGIF_GETDEFAULTICON )
            {
                StrCpyNW(pBuffer, L"shell32.dll", cchBuffer);
                *piIndex = -1;
            }

            if ( FAILED(hr) )
            {
                dwFlags &= ~DSGIF_DEFAULTISCONTAINER;
                ExitGracefully(hr, S_FALSE, "Failed to look up icon as container");
            }
        }

        hr = S_OK;                  // its OK, we have a location now.
    }

    TraceLeaveResult(hr);
}
