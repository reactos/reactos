//  File:       regzone.cxx
//
//  Contents:   Registry management for a single zone. 
//
//  Classes:    CRegZone
//
//  Functions:
//
//  History: 
//
//----------------------------------------------------------------------------

#include "zonepch.h"

// Max # of chars to the root of the zones tree (SZZONES, SZTEMPLATE,...)

#define MAX_REGZONE_ROOT        100

// Value names in the registry
#define SZZONEINDEX             __TEXT("ZoneIndex")
#define SZTEMPLATEINDEX         __TEXT("TemplateIndex")
#define SZDISPLAYNAME           __TEXT("DisplayName")
#define SZICON                  __TEXT("Icon")
#define SZDESCRIPTION           __TEXT("Description")
#define SZFLAGS                 __TEXT("Flags")

// Registry key names for the template policies


#define SZLOW                   __TEXT("Low")
#define SZMEDLOW                __TEXT("MedLow")
#define SZMEDIUM                __TEXT("Medium")
#define SZHIGH                  __TEXT("High")


CRegZone::CRegZoneCache CRegZone::s_rzcache;
HANDLE CRegZone::CRegZoneCache::s_hMutexCounter;

// Array of Value Names corresponding to zone attributes.
// These values will not be copied when doing a mass copy 
// from a template zone (HIGH, MED, LOW) to a zone. 
static LPCTSTR rgszAttributeNames [ ] = 
{
    __TEXT(""),            // The default value is excluded as well. 
    SZZONEINDEX,
    SZTEMPLATEINDEX,
    SZDISPLAYNAME,
    SZDESCRIPTION,
    SZICON,
    SZFLAGS,
    SZMINLEVEL,
    SZRECLEVEL,
    SZCURRLEVEL,
};

struct templateNameIdxMap
{
    URLTEMPLATE index;
    LPCTSTR    pszName;
};

static templateNameIdxMap 
TemplateNameIdxMap [ ] = 
{    
    { URLTEMPLATE_LOW,          SZLOW },
    { URLTEMPLATE_MEDLOW,       SZMEDLOW},
    { URLTEMPLATE_MEDIUM,       SZMEDIUM},
    { URLTEMPLATE_HIGH,         SZHIGH}
};     
     
 


// CRegZone implementation.

CRegZone::CRegZone()
{
    // defaults
    m_dwZoneId  =  ZONEID_INVALID;
    m_dwZoneFlags = ZAFLAGS_ADD_SITES;  // BUGBUG: what is the right default here. 
    m_lpZoneName = NULL;
    m_lpZonePath = NULL;

    m_bStandard = TRUE;    
    m_bZoneLockOut = FALSE;
    m_bHKLMOnly = TRUE;
}     

CRegZone::~CRegZone()
{
    LocalFree((HLOCAL)m_lpZoneName);    
    LocalFree((HLOCAL)m_lpZonePath);
}

// Sets up the CRegZone object a given string. 
// If the setting is in the Zones key the string passed in is the actual
// zone index. Otherwise it is one of the "High", "Medium", "Low" strings which indicates
// a template policy. 

BOOL CRegZone::Init(LPCWSTR lpwStr, BOOL bUseHKLMOnly, REGZONEUSE regZoneUse, BOOL bCreate /*=TRUE*/)
{
    TransAssert(lpwStr != NULL);
    if (lpwStr == NULL)
    {
        return FALSE;
    }

    m_bHKLMOnly = bUseHKLMOnly;
    m_regZoneUse = regZoneUse;

    TCHAR szTemp[MAX_REGZONE_ROOT + MAX_ZONE_NAME];

    StrCpyW(szTemp, (regZoneUse == REGZONEUSEZONES? SZZONES : SZTEMPLATE));
    StrCatW(szTemp, lpwStr);

    m_lpZonePath = StrDup(szTemp);
    m_lpZoneName = StrDup(lpwStr);

    CRegKey regKey(bUseHKLMOnly);

    if (regKey.Open(NULL, m_lpZonePath, KEY_READ) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return FALSE;
    }


    // Add code here to 
    DWORD dwZoneId = ZONEID_INVALID;

    // Get the Zone Index.
    if ( regZoneUse == REGZONEUSEZONES )
    {
        // The Zone Id for the string is the same as the key name.
        // Just convert the string 
        m_dwZoneId = StrToInt(m_lpZoneName);
    }
    else if (regZoneUse == REGZONEUSETEMPLATE )
    {
        if (regKey.QueryValue(&dwZoneId, SZTEMPLATEINDEX) == ERROR_SUCCESS)
            m_dwZoneId = dwZoneId;
        else
        {
            // Could happen if the registry is messed up. 
            TransAssert(FALSE);
        }
    }
    else 
    {
        TransAssert(FALSE);
    }       

    // Get the zone flags
    if (regKey.QueryValue(&m_dwZoneFlags, SZFLAGS) != ERROR_SUCCESS)
    {
        m_dwZoneFlags = ZAFLAGS_ADD_SITES;      // What is the right value here.
    }            
    else
    {
        // return value from UpdateZoneMapFlags ignored.  
        UpdateZoneMapFlags( );
    }                
    // Check and make sure the zone Id's are within range.             
    // Assert that the zone ID's are in the appropriate user or standard range.
                    
    return TRUE;
}

// This updates the flags in the ZoneMap part of the registry which correspond to the 
// ZAFLAGS_. For convenience the UI will only update the ZAFLAGS.


BOOL CRegZone::UpdateZoneMapFlags( )
{
    // ProxyByPass is current controlled by the ProxyByPass flag, not the zoneAttrib.
    if (m_dwZoneId == URLZONE_INTRANET)
    {
        // If we are updating zonemap flags we have to invalidate any url to zone caches
        CSecurityManager::IncrementGlobalCounter( );
                
        CRegKey regZoneMap;
        if (ERROR_SUCCESS == regZoneMap.Open(NULL, SZZONEMAP, KEY_READ | KEY_WRITE))
        {
            if (m_dwZoneFlags & ZAFLAGS_INCLUDE_PROXY_OVERRIDE)
            {
                // We will succeed even if this fails.
                regZoneMap.SetValue(m_dwZoneId, SZPROXYBYPASS);
            }
            else 
            {
                regZoneMap.DeleteValue(SZPROXYBYPASS);
            }

            if (m_dwZoneFlags & ZAFLAGS_INCLUDE_INTRANET_SITES)
            {
                // We will succeed even if this fails.
                regZoneMap.SetValue(m_dwZoneId, SZINTRANETNAME);
            }
            else 
            {
                regZoneMap.DeleteValue(SZINTRANETNAME);
            }

            DWORD dwUncAsIntranet = (m_dwZoneFlags & ZAFLAGS_UNC_AS_INTRANET) ? 1 : 0 ;
            regZoneMap.SetValue(dwUncAsIntranet, SZUNCASINTRANET);

        }
    }
    return TRUE;
}

// Static functions.

VOID
CRegZone::IncrementGlobalCounter( )
{
    CRegZone::CRegZoneCache::IncrementGlobalCounter( );
}

BOOL CRegZone::IsAttributeName(LPCTSTR psz)
{
    DWORD dwMaxIndex = sizeof(rgszAttributeNames)/sizeof(rgszAttributeNames[0]);

    for ( DWORD dwIndex = 0 ; dwIndex < dwMaxIndex ; dwIndex++ )
    {
#ifndef UNIX
        if (0 == StrCmpW(psz, rgszAttributeNames[dwIndex]))
#else
        if (0 == lstrcmpi(psz, rgszAttributeNames[dwIndex]))
#endif
            return TRUE;
    }

    return FALSE;
}


LPCTSTR CRegZone::GetTemplateNameFromIndex(URLTEMPLATE urlTemplateIndex)
{
    DWORD dwMaxIndex = sizeof(TemplateNameIdxMap) / sizeof(TemplateNameIdxMap[0]);

    for (DWORD dwIndex = 0 ; dwIndex < dwMaxIndex ; dwIndex++ )
    {
        if (TemplateNameIdxMap[dwIndex].index == urlTemplateIndex)
            return TemplateNameIdxMap[dwIndex].pszName;
    }
    
    return NULL;
}
             
// These are static functions to deal with aggregate policies. 

// Because of the discrepancy between the UI and the actions defined,
// there are cases where the security manager munges the policies for certain actions.

inline void CRegZone::KludgeMapAggregatePolicy(DWORD dwAction, LPDWORD pdwPolicy)
{
    TransAssert(pdwPolicy != NULL);

    switch (dwAction)
    {
        case URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY:
        case URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY:
        case URLACTION_SCRIPT_OVERRIDE_SAFETY:
        {
            if (GetUrlPolicyPermissions(*pdwPolicy) == URLPOLICY_QUERY)
                SetUrlPolicyPermissions(*pdwPolicy, URLPOLICY_DISALLOW);
            break;
        }
    }
}
             

// Call this function to determine if an action is aggregated by some
// other action. 
// RETURNS : TRUE if there is an aggregate action corr to dwAction. 
//           also returns the action in pdwAggregate.
//           FALSE: if this action is not aggregated by some other action.
//           pdwAggregate is unchanged in this case.

inline BOOL CRegZone::GetAggregateAction(DWORD dwAction, LPDWORD pdwAggregate)
{
    DWORD dwAggregate = 0;
    BOOL bReturn = FALSE;
       
    TransAssert(dwAction >= URLACTION_MIN);
    
    switch(dwAction)
    {
        case URLACTION_ACTIVEX_OVERRIDE_DATA_SAFETY:
        case URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY:
        case URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY:
        case URLACTION_SCRIPT_OVERRIDE_SAFETY:
            bReturn = TRUE;
            dwAggregate = URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY;
            break;
        case URLACTION_HTML_SUBMIT_FORMS_FROM:
        case URLACTION_HTML_SUBMIT_FORMS_TO:
            bReturn = TRUE;
            dwAggregate = URLACTION_HTML_SUBMIT_FORMS;
            break;
    }

    if (bReturn && pdwAggregate)
        *pdwAggregate = dwAggregate;

    return bReturn;
}


// Functions corresponding to IInternetZoneManager functionality. 
STDMETHODIMP CRegZone::GetZoneAttributes(ZONEATTRIBUTES& zoneAttrib)
{
    if (!IsValid())
    {
        return E_FAIL;
    }

    CRegKey regKey(m_bHKLMOnly);

    if (regKey.Open(NULL, m_lpZonePath, KEY_READ) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return E_FAIL;
    }

    // Since this is the first rev, we should have enough memory 
    // to fill in the ZONEATTRIBUTES structure. If we need to extend
    // the structure this code will have to be modified.
    TransAssert(zoneAttrib.cbSize >= sizeof(ZONEATTRIBUTES));

    // Amount of information we will copy.
    zoneAttrib.cbSize = sizeof(ZONEATTRIBUTES);

    TransAssert(regKey!= NULL);

    DWORD dwCount;
    LONG lRet;

    // BUGBUG deal with values exceeding size limit. 
    // We would have to allocate memory ourself and 
    // truncate the resulting string down.   

    // Read DisplayName.
    dwCount = sizeof(zoneAttrib.szDisplayName);
    lRet = regKey.QueryValue(zoneAttrib.szDisplayName, SZDISPLAYNAME, &dwCount);   
    TransAssert(ERROR_MORE_DATA != lRet);

    if (NO_ERROR != lRet)
        zoneAttrib.szDisplayName[0] = __TEXT('\0');    

    // Read Description
    dwCount = sizeof(zoneAttrib.szDescription);
    regKey.QueryValue(zoneAttrib.szDescription, SZDESCRIPTION, &dwCount);
    TransAssert(ERROR_MORE_DATA != lRet);

    if (NO_ERROR != lRet)
        zoneAttrib.szDescription[0] = __TEXT('\0');
    
    // Read Icon. 
    dwCount = sizeof(zoneAttrib.szIconPath);
    regKey.QueryValue(zoneAttrib.szIconPath, SZICON, &dwCount);
    TransAssert(ERROR_MORE_DATA != lRet);

    if (NO_ERROR != lRet)
        zoneAttrib.szIconPath[0] = __TEXT('\0');

    // Read Current, Recommended and Min Settings. 
    QueryTemplatePolicyIndex(regKey, SZMINLEVEL, &zoneAttrib.dwTemplateMinLevel);
    QueryTemplatePolicyIndex(regKey, SZRECLEVEL, &zoneAttrib.dwTemplateRecommended);
    QueryTemplatePolicyIndex(regKey, SZCURRLEVEL, &zoneAttrib.dwTemplateCurrentLevel);

    // Re-read the flags in case someone else updated it in an independent process. 
    DWORD dwZoneFlags;
    if (regKey.QueryValue(&dwZoneFlags, SZFLAGS) == ERROR_SUCCESS)
    {
        m_dwZoneFlags = dwZoneFlags;
        UpdateZoneMapFlags();
    }

    zoneAttrib.dwFlags = m_dwZoneFlags;

    return S_OK;
}     
      
STDMETHODIMP CRegZone::SetZoneAttributes(const ZONEATTRIBUTES& zoneAttrib)
{
    if (!IsValid())
    {
        return E_FAIL;
    }

    // Check if the attributes we are trying to set are valid.

    if (!IsValidTemplateIndex(zoneAttrib.dwTemplateMinLevel) ||
        !IsValidTemplateIndex(zoneAttrib.dwTemplateCurrentLevel) ||
        !IsValidTemplateIndex(zoneAttrib.dwTemplateRecommended))
    {
        return E_INVALIDARG;
    }

    CRegKey regKey(m_bHKLMOnly);

    if (regKey.Open(NULL, m_lpZonePath, KEY_WRITE | KEY_READ) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return E_FAIL;
    }

    
    // Write the descriptive strings. 
    // These should almost never be changed by this call.   
    if (zoneAttrib.szDisplayName[0] != TEXT('\0'))
        regKey.SetValue(zoneAttrib.szDisplayName, SZDISPLAYNAME);   

    if (zoneAttrib.szDescription[0] != TEXT('\0'))
        regKey.SetValue(zoneAttrib.szDescription, SZDESCRIPTION);

    if (zoneAttrib.szIconPath[0] != TEXT('\0'))
        regKey.SetValue(zoneAttrib.szIconPath, SZICON);

    // Write the Template Indicies. 
    SetTemplatePolicyIndex(regKey, SZMINLEVEL, zoneAttrib.dwTemplateMinLevel);
    SetTemplatePolicyIndex(regKey, SZRECLEVEL, zoneAttrib.dwTemplateRecommended);

    DWORD dwTemplateCurrentLevel;

    // When the caller is setting the "CurrentLevel" to "Custom" it is assumed
    // that the caller has already changed the underlying policies. 
    if (zoneAttrib.dwTemplateCurrentLevel == URLTEMPLATE_CUSTOM)
    {
        SetTemplatePolicyIndex(regKey, SZCURRLEVEL, zoneAttrib.dwTemplateCurrentLevel);
    }
    else 
    {
        CopyTemplatePolicies(zoneAttrib.dwTemplateCurrentLevel);
    }


    // Finally write the flags value.
    regKey.SetValue(zoneAttrib.dwFlags, SZFLAGS);
    m_dwZoneFlags = zoneAttrib.dwFlags;
    UpdateZoneMapFlags();

    IncrementGlobalCounter();   // increment the count to invalidate the zone policy cache
    
    return S_OK;
}


STDMETHODIMP CRegZone::GetActionPolicy(DWORD dwAction, URLZONEREG urlZoneReg, DWORD& dwPolicy) const 
{
    if (!IsValid())
        return E_FAIL;

    DWORD dwActionUse;

    // If the action is aggregated by some other action, then we should 
    // actually check the policy for the aggregate action. If the function 
    if (!GetAggregateAction(dwAction, &dwActionUse))
        dwActionUse = dwAction;

    // If it is a hard-coded zone get the policy from internal tables.
    // Don't look up the registry for these.
    if (IsHardCodedZone() && GetHardCodedZonePolicy(dwActionUse, dwPolicy))
    {
        // dwPolicy should have the policy now.
    }
    else
    {
        if(!s_rzcache.Lookup(m_dwZoneId, m_lpZonePath, dwActionUse, UseHKLM(urlZoneReg), &dwPolicy))
        {
            return E_FAIL;
        }

        // For some special aggregate policies we have to modify the policy value.
        KludgeMapAggregatePolicy(dwAction, &dwPolicy);
    }

    return S_OK;
}                                     

STDMETHODIMP CRegZone::SetActionPolicy(DWORD dwAction, URLZONEREG urlZoneReg, DWORD dwPolicy)
{
    if (!IsValid())
        return E_FAIL;

    CRegKey regKey(UseHKLM(urlZoneReg));

    if (regKey.Open(NULL, m_lpZonePath, KEY_WRITE) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return E_FAIL;
    }

    // Policies cannot be set on Actions that are aggregate's.
    // They can be only be set on the aggregator policy.

    if (IsHardCodedZone())
    {
        TransAssert(FALSE);
        return E_FAIL;
    }
            
    DWORD dwActionUse;

    // If the action is aggregated by some other action, then we should 
    // actually use the policy for the aggregate action. 
    if (!GetAggregateAction(dwAction, &dwActionUse))
        dwActionUse = dwAction;

    // Convert the Action to a string. 
#ifndef unix
    TCHAR wsz[9];  // FFFFFFFF\0
#else
    TCHAR wsz[(sizeof(DWORD)+1)*sizeof(WCHAR)];
#endif /* unix */
    if (!DwToWchar(dwActionUse, wsz, 16))
    {
        TransAssert(FALSE);
        return E_UNEXPECTED;
    }

    regKey.SetValue(dwPolicy, wsz);

    s_rzcache.Add(m_dwZoneId, dwActionUse, UseHKLM(urlZoneReg), dwPolicy, URLZONE_FINDCACHEENTRY);

    return S_OK;
}    

STDMETHODIMP  CRegZone::GetCustomPolicy (REFGUID guid, URLZONEREG urlZoneReg, BYTE** ppByte, DWORD *pcb) const 
{
    if (!IsValid())
        return E_FAIL;


    CRegKey regKey(UseHKLM(urlZoneReg));

    if (regKey.Open(NULL, m_lpZonePath, KEY_READ) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return E_FAIL;
    }

    // Convert the Action to a string. 
    TCHAR sz[40];  // {8CC49940-3146-11CF-97A1-00AA00424A9F}\0

    SHStringFromGUID(guid, sz, sizeof(sz));

    *pcb = 0;
    // First figure out the amount of memory required.
    if (regKey.QueryBinaryValue(NULL, sz, pcb) != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    // Memory will be freed by caller.
    *ppByte = (BYTE *)CoTaskMemAlloc(*pcb);

    if ( *ppByte == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Actually query the registry for the value.
    if (regKey.QueryBinaryValue(*ppByte, sz, pcb) != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return S_OK;
}

STDMETHODIMP  CRegZone::SetCustomPolicy (REFGUID guid, URLZONEREG urlZoneReg, BYTE* pByte, DWORD cb)
{
    if (!IsValid())
        return E_FAIL;

    CRegKey regKey(UseHKLM(urlZoneReg));

    if (regKey.Open(NULL, m_lpZonePath, KEY_WRITE) != ERROR_SUCCESS) 
    {
        // BUGBUG:: We have to be able to deal with this. situation and not just bail.
        // Possibilities: Setup defaults here if we can create and write to the key
        return E_FAIL;
    }

    // Convert the Action to a string. 
    TCHAR sz[40];  // {8CC49940-3146-11CF-97A1-00AA00424A9F}\0

    SHStringFromGUID(guid, sz, sizeof(sz));

    DWORD dwError = ERROR_SUCCESS;
    if ((dwError = regKey.SetBinaryValue(pByte, sz, cb)) != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(dwError);
    }

    return S_OK;
}


STDMETHODIMP CRegZone::CopyTemplatePolicies(DWORD dwTemplate)
// Copy the policies from a predefined template into the current zone
{
    HRESULT hr = E_FAIL;

    // First check if we can get a name back for the template.
    LPCTSTR szTemplateName = GetTemplateNameFromIndex((URLTEMPLATE)dwTemplate);

    if (NULL == szTemplateName )
        return E_INVALIDARG;

    // Create a CRegZone for the template.
    CRegZone regTemplate;


    if (regTemplate.Init(szTemplateName, TRUE /* templates are stored in HKLM only */, REGZONEUSETEMPLATE))
    {
        CRegKey regZoneKey(m_bHKLMOnly);
        CRegKey regTemplateKey(TRUE);

        if ((NO_ERROR == regTemplateKey.Open(NULL, regTemplate.m_lpZonePath, KEY_READ)) &&
            (NO_ERROR == regZoneKey.Open(NULL, m_lpZonePath, KEY_WRITE)))
        {
            TCHAR szValueName[MAX_VALUE_NAME];
            DWORD dwNameLen = sizeof(szValueName)/sizeof(TCHAR);
            DWORD dwBufLen = 2048;
            DWORD dwActualLen = 2048;
            BYTE * buffer = new BYTE[dwBufLen];
            DWORD dwEnumIndex = 0;
            DWORD dwType;
            LONG lRet;

            while ((lRet = regTemplateKey.EnumValue (dwEnumIndex, szValueName, &dwNameLen, &dwType, buffer, &dwActualLen))
                      != ERROR_NO_MORE_ITEMS)
            {
                // Need more memory, allocate and re-try.
                if (lRet == ERROR_MORE_DATA && dwActualLen > dwBufLen)
                {
                    dwBufLen = dwActualLen;
                    delete [] buffer;
                    buffer = new BYTE[dwBufLen];

                    dwNameLen = sizeof(szValueName)/sizeof(TCHAR);
                                    
                    // Try with the bigger buffer.
                    lRet = regTemplateKey.EnumValue(dwEnumIndex, szValueName, &dwNameLen, &dwType, buffer, &dwActualLen);
                }                                            
                    
                // dwActualLen contains the actual size of the data to be written. 
                if (lRet == NO_ERROR && !IsAttributeName(szValueName))
                {
                    // Copy the value over.
                    regZoneKey.SetValueOfType(buffer, szValueName, dwActualLen, dwType);
                }

                dwEnumIndex++;
                dwActualLen = dwBufLen;
                dwNameLen = sizeof(szValueName)/sizeof(TCHAR);
            }
            
            // Set the "CurrentLevel" value to the Template Index.
            if (regZoneKey.SetValue(dwTemplate, SZCURRLEVEL) == NO_ERROR)
                hr = S_OK;

            delete [] buffer;
        }
    }

    return hr;
}
                    
                         
// CRegZoneContainer methods.


CRegZoneContainer::CRegZoneContainer()
{
    m_ppRegZones = NULL;
    m_cZones = 0;
    m_bHKLMOnly = FALSE;

    m_pZoneEnumList = NULL;
    m_dwNextEnum = 0;
    InitializeCriticalSection(&m_csect);
}

CRegZoneContainer::~CRegZoneContainer()
{
    Detach();
    DeleteCriticalSection(&m_csect);
};

// This functions goes through the registry and creates the CRegZone objects corresponding 
// to the zones currently in the registry.


BOOL CRegZoneContainer::Attach(BOOL bUseHKLM, REGZONEUSE regZoneUse /* = REGZONEUSEZONES */)                                        
{
    // If this assert fires you probably forgot to call Detach.
    TransAssert(m_cZones == 0);
    TransAssert(m_ppRegZones == NULL);
    // recover if we are screwed up.
    Detach();

    m_bHKLMOnly = bUseHKLM ;


    TCHAR sz[MAX_REGZONE_ROOT];
    StrCpyW(sz, (regZoneUse == REGZONEUSEZONES ? SZZONES : SZTEMPLATE )); 

    // Make sure we have the minimal set of zones required and we self-heal if there is a problem.
    // even if self-heal fails we ignore the error code and try to initialize the zones anyway.
    if (regZoneUse == REGZONEUSEZONES )
        SelfHeal(bUseHKLM);


    CRegKey regKey(m_bHKLMOnly);
    
    if (regKey.Open(NULL, sz, KEY_READ) != ERROR_SUCCESS)
    {
        // Hosed setup right defaults here. 
        return FALSE;
    }
        

    TransAssert(regKey.m_hKey != NULL);

    // For each entry in the registry
    DWORD dwIndex = 0;
    DWORD dwCount = 0;
    DWORD lRes;
    TCHAR szZoneName[MAX_ZONE_NAME];
    DWORD dwSize = MAX_ZONE_NAME;    

    CRegListElem *pElemStart = NULL;

    for (; (lRes = regKey.EnumKey(dwIndex, szZoneName, &dwSize)) != ERROR_NO_MORE_ITEMS; dwIndex++)
    {
        if (lRes != ERROR_SUCCESS)
        {
            break;
        }
        
        dwSize = MAX_ZONE_NAME;

        CRegZone *pRegZone = new CRegZone();
        
        if (pRegZone == NULL)
        {
            m_cZones = 0;
            // Out of memory -- change all error codes to return HRESULT's
            break;
        }
        
        if (!pRegZone->Init(szZoneName, m_bHKLMOnly))
        {
            continue;   // can't create the zone for some reason.
        }

        m_cZones++;
        
        CRegListElem * pRegListElem = new CRegListElem();
        if ( pRegListElem == NULL)
        {
            m_cZones = 0;
            break;  // Out of memory.
        }

        pRegListElem->pRegZone = pRegZone;
        pRegListElem->dwZoneIndex = pRegZone->GetZoneId();

        // Insert list into sorted position.
        if (pElemStart == NULL)
        {
            pElemStart = pRegListElem;
            pRegListElem->next = NULL;
        }
        else if (pElemStart->dwZoneIndex > pRegListElem->dwZoneIndex)
        {
            // Insert to the head of the list.
            pRegListElem->next = pElemStart;
            pElemStart = pRegListElem;
        }
        else 
        {
            // Insert in the correct position. 
            CRegListElem *pElemCurr = pElemStart;
            
            while (pElemCurr->next != NULL &&
                 pElemCurr->next->dwZoneIndex < pRegListElem->dwZoneIndex )
            {
                pElemCurr = pElemCurr->next;
            }

            TransAssert(pElemCurr != NULL);
            pRegListElem->next = pElemCurr->next;
            pElemCurr->next = pRegListElem;
        }
    }

    // Now that we have all the RegZones collected we will just store them in 
    // sorted order in an array.

    if (m_cZones)
        m_ppRegZones = new LPREGZONE[m_cZones];
    else
        m_ppRegZones = NULL;

    if (m_ppRegZones == NULL)
    {
        // Out of memory
        m_cZones = 0;
        return FALSE;
    }

    for (dwIndex = 0; dwIndex < m_cZones ; dwIndex++)
    {
        TransAssert(pElemStart != NULL);

        m_ppRegZones[dwIndex] = pElemStart->pRegZone;
        CRegListElem * pElemDelete = pElemStart;
        pElemStart = pElemStart->next;
        delete pElemDelete;
    }

    return TRUE;

}

BOOL CRegZoneContainer::Detach()
{
    // First free all the CRegZone entries we are holding on to.
    DWORD dwIndex = 0;

    for (; dwIndex < m_cZones; dwIndex++)
    {
        delete m_ppRegZones[dwIndex];
    }

    delete [] m_ppRegZones;
    m_ppRegZones = NULL;
    m_cZones = 0;
    m_bHKLMOnly = TRUE;

    // Zone enumerator cleanup.
    // This ASSERT will fire if you forget to call DestroyZoneEnumerator before 
    // freeing the object.
    TransAssert(m_pZoneEnumList == NULL);
    CZoneEnumList * pNextEnum = m_pZoneEnumList;
    while (pNextEnum != NULL)
    {
        CZoneEnumList * pEnumListDelete = pNextEnum;
        pNextEnum = pNextEnum->next;
        delete pEnumListDelete;
    }

    return TRUE;
}

// This function makes sure that the minimal set of zones are in the registry If things are missing
// it calls the self-registration entry point and re-creates the zone key.

#define WIN2KSETUP  TEXT("System\\Setup")
#define INPROGRESS  TEXT("SystemSetupInProgress")

BOOL CRegZoneContainer::SelfHeal(BOOL bUseHKLM)
{
 
    HKEY hKeyZones = NULL;

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        // If we are called in here as part of Win2K Setup, do not run the SelfHeal code.
        HKEY hWin2kSetup = NULL;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WIN2KSETUP, 0, KEY_READ, &hWin2kSetup)
                == ERROR_SUCCESS)
        {
            DWORD dwValue = 0;  // Default is "Setup not in progress"
            DWORD dwSize = sizeof(dwValue);

            // If we cannnot read the value, fall back to default.
            if (RegQueryValueEx(hWin2kSetup, INPROGRESS, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
                dwValue = 0;
    
            // First close the open handle
            RegCloseKey(hWin2kSetup);
    
            // Now compare the Setup flag. 
            // Anything other than 0 means we are in Win2k Setup. If so, ignore SelfHeal
            if (dwValue != 0)
            {
                return TRUE;
            }
            
        }
    }

    if (RegOpenKeyEx((bUseHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER), SZZONES, 0, KEY_READ, &hKeyZones)
            == ERROR_SUCCESS)
    {
                     
        // Strings corresponding to the five pre-defined zones.
        TCHAR * rgszZones[] = { TEXT("0"), TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4") };
        
        int i;
        for (i = 0 ; i < ARRAYSIZE(rgszZones) ; i++ )
        {
            HKEY hKey;

            DWORD dwError = RegOpenKeyEx(hKeyZones, rgszZones[i], 0, KEY_READ, &hKey);
            
            if (dwError != ERROR_SUCCESS)
                break;
            else
                RegCloseKey(hKey);
        }

        RegCloseKey(hKeyZones);    

        // If we succesfully opened all the zones.
        if (i == ARRAYSIZE(rgszZones))
        {
            return TRUE;
        }

    }


    // If we reached here we were not able to open atleast one of the keys.
    // Note that we use L"" because ZonesDllInstall takes a LPCWSTR  
    BOOL bRet;
    HRESULT hr = ZonesDllInstall(TRUE, bUseHKLM ? L"HKLM" : L"HKCU");
    
    if (SUCCEEDED(hr))
    {
        CRegKey regKey(bUseHKLM);

        // Keep track of how many times we self heal for diagnostic purposes..
        DWORD dwSelfHealCount;
        TCHAR *pszSelfHealCount = TEXT("SelfHealCount");

        if (regKey.Open(NULL, SZZONES, KEY_READ | KEY_WRITE) == ERROR_SUCCESS)
        {
            if (regKey.QueryValue(&dwSelfHealCount, pszSelfHealCount) != ERROR_SUCCESS)
                dwSelfHealCount = 0;

            dwSelfHealCount++;
            regKey.SetValue(dwSelfHealCount, pszSelfHealCount);
        }

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    return bRet;
}

CRegZone * CRegZoneContainer::GetRegZoneByName(LPCTSTR lpName) const
{
    DWORD dwIndex = 0;
    CRegZone *pRegZone = NULL;

    for ( ; dwIndex < m_cZones; dwIndex++ )
    {
        pRegZone = m_ppRegZones[dwIndex] ;

        if (pRegZone && StrCmpIW(pRegZone->GetZoneName(), lpName) == 0)
            break;
    }

    return pRegZone;
}

CRegZone * CRegZoneContainer::GetRegZoneById(DWORD dwZoneId) const
{
    DWORD dwIndex = 0;
    CRegZone * pReturnZone = NULL;

    for (; dwIndex < m_cZones ; dwIndex++ )
    {
        CRegZone * pRegZone = m_ppRegZones[dwIndex];

        
        if (pRegZone == NULL)       
        {
            // This shouldn't happen but a safety check doesn't hurt.
            break;
        }
        else if (pRegZone->GetZoneId() == dwZoneId)
        {
            pReturnZone = pRegZone;
            break;      // Got it
        }
        else if (pRegZone->GetZoneId() > dwZoneId)
        {
            break;
        }
    }

    return pReturnZone;
}

// Zone Enumeration functions. 
BOOL CRegZoneContainer::VerifyZoneEnum(DWORD dwEnum ) const
{
    BOOL bFound = FALSE;

    CZoneEnumList *pNext = m_pZoneEnumList;
    while (pNext)
    {
        if (pNext->dwEnum == dwEnum)
        {
            bFound = TRUE;
            break;
        }

        pNext = pNext->next;
    }
    
    return bFound;
}
          


STDMETHODIMP CRegZoneContainer::CreateZoneEnumerator(DWORD* pdwEnum,  DWORD *pdwCount)
{
    if (pdwEnum == NULL || pdwCount == NULL)
        return E_INVALIDARG;

    if (m_cZones == 0)
    {
        return E_FAIL;
    }

    CZoneEnumList *pEnumListElem = new CZoneEnumList;
    if (pEnumListElem == NULL)
        return E_OUTOFMEMORY;

    pEnumListElem->dwEnum = m_dwNextEnum++;

    EnterCriticalSection(&m_csect);

    if (m_pZoneEnumList == NULL)
    {
        pEnumListElem->next = NULL;
        m_pZoneEnumList = pEnumListElem;
    }
    else 
    {
        pEnumListElem->next = m_pZoneEnumList;
        m_pZoneEnumList = pEnumListElem;
    }

    *pdwEnum = m_pZoneEnumList->dwEnum;
    *pdwCount = m_cZones;
        
    TransAssert(VerifyZoneEnum(*pdwEnum));
    
    LeaveCriticalSection(&m_csect);
        
    return S_OK;
}


STDMETHODIMP CRegZoneContainer::GetZoneAt(DWORD dwEnum, DWORD dwIndex, DWORD *pdwZone)
{
    if (!VerifyZoneEnum(dwEnum) || dwIndex >= m_cZones)
    {
        return E_INVALIDARG;
    }

    if (m_ppRegZones && m_ppRegZones[dwIndex])
    {
        *pdwZone = m_ppRegZones[dwIndex]->GetZoneId();
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CRegZoneContainer::DestroyZoneEnumerator(DWORD dwEnum)
{
    HRESULT hr = S_OK;
    CZoneEnumList *pDelete = NULL;

    EnterCriticalSection(&m_csect);
    if (m_pZoneEnumList == NULL)
    {
    }
    else if (m_pZoneEnumList->dwEnum == dwEnum)
    {
        pDelete = m_pZoneEnumList;
        m_pZoneEnumList = pDelete->next;
    }
    else {
        CZoneEnumList *pCurr = m_pZoneEnumList;

        while (pCurr != NULL)
        {
            if (pCurr->next && pCurr->next->dwEnum == dwEnum)
            {
                pDelete = pCurr->next;
                pCurr->next = pDelete->next;
                break;
            }
            pCurr = pCurr->next;
        }
    }

    if (pDelete == NULL)
    {
        // Didn't find the entry must be an invalid Enumerator.
        hr = E_INVALIDARG;
    }
    else 
    {
        delete pDelete;
        hr = S_OK;
    }
    LeaveCriticalSection(&m_csect);
    return hr;
}

//=============================================================================
// CRegZoneCache methods
//=============================================================================
CRegZone::CRegZoneCache::CRegZoneCache(void)
{
    InitializeCriticalSection(&m_csectZoneCache);
    
    // single static object, so this only gets inited once per
    // process.
    s_hMutexCounter = CreateMutexA(NULL, FALSE, "ZonesCacheCounterMutex");
    m_iAdd = 0;
}

CRegZone::CRegZoneCache::~CRegZoneCache(void)
{
    Flush();
    DeleteCriticalSection(&m_csectZoneCache) ; 

    CloseHandle(s_hMutexCounter);
}

BOOL
CRegZone::CRegZoneCache::Lookup(DWORD dwZone, LPTSTR lpZonePath, DWORD dwAction, BOOL fUseHKLM, DWORD *pdwPolicy)
{
    BOOL fFound = FALSE;
    int iEntry = URLZONE_FINDCACHEENTRY;

    TransAssert(iEntry < MAX_REG_ZONE_CACHE);

    EnterCriticalSection(&m_csectZoneCache);

    if ( !IsCounterEqual() )
        Flush();
    
    fFound = FindCacheEntry(dwZone, dwAction, fUseHKLM, iEntry );
    if (fFound)
    {
        if (pdwPolicy)
        {
            *pdwPolicy = m_arzce[iEntry].m_dwPolicy;
        }
    }
    else
    {
        // Convert the Action to a string. 
#ifndef unix
        TCHAR wsz[9];  // FFFFFFFF\0
#else
        TCHAR wsz[(sizeof(DWORD)+1)*sizeof(WCHAR)];
#endif /* unix */

        if (DwToWchar(dwAction, wsz, 16))
        {
            CRegKey regKey(fUseHKLM);

            if (regKey.Open(NULL, lpZonePath, KEY_READ) == ERROR_SUCCESS) 
            {
                if (regKey.QueryValue(pdwPolicy, wsz) == ERROR_SUCCESS)
                {
                    fFound = TRUE;
                    Add(dwZone, dwAction, fUseHKLM, *pdwPolicy, iEntry);
                }
            }
            else
            {
                // BUGBUG:: We have to be able to deal with this situation and not just bail.
                // Possibilities: Setup defaults here if we can create and write to the key
                TransAssert(FALSE);
            }
        }
        else
        {
            TransAssert(FALSE);
        }
    }

    LeaveCriticalSection(&m_csectZoneCache);

    return fFound;
}

void 
CRegZone::CRegZoneCache::Add(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, DWORD dwPolicy, int iEntry)
{
    BOOL    fFound;

    TransAssert(iEntry < MAX_REG_ZONE_CACHE);
    
    EnterCriticalSection(&m_csectZoneCache);

    if ( !IsCounterEqual() )
        Flush();

    if(iEntry == URLZONE_FINDCACHEENTRY)  // using optional param that indicates the entry we want to add so don't bother doing a find.
        fFound = FindCacheEntry(dwZone, dwAction, fUseHKLM, iEntry ); // found or not, iEntry will be the right place to set it.

    m_arzce[iEntry].Set(dwZone, dwAction, fUseHKLM, dwPolicy);

    SetToCurrentCounter(); // validate this cache.

    LeaveCriticalSection(&m_csectZoneCache);
}

void 
CRegZone::CRegZoneCache::Flush(void)
{
    int i;

    EnterCriticalSection(&m_csectZoneCache);

    for ( i = 0; i < MAX_REG_ZONE_CACHE; i++ )
        m_arzce[i].Flush();

    m_iAdd = 0;
    
    LeaveCriticalSection(&m_csectZoneCache);
}

// Is the counter we saved with the cache entry, equal to the current counter.
BOOL
CRegZone::CRegZoneCache::IsCounterEqual( ) const 
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD)g_SharedMem.GetPtr(SM_REGZONECHANGE_COUNTER);
    // If we couldn't create the shared memory for some reason, we just assume our cache is up to date.
    if (lpdwCounter == NULL)
        return TRUE;

    return (m_dwPrevCounter == *lpdwCounter);
}

VOID
CRegZone::CRegZoneCache::SetToCurrentCounter( ) 
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD)g_SharedMem.GetPtr(SM_REGZONECHANGE_COUNTER);
    if (lpdwCounter == NULL)
        return;

    m_dwPrevCounter = *lpdwCounter;
}

VOID
CRegZone::CRegZoneCache::IncrementGlobalCounter( )
{
    CExclusiveLock lock(s_hMutexCounter);
    LPDWORD lpdwCounter = (LPDWORD)g_SharedMem.GetPtr(SM_REGZONECHANGE_COUNTER);
    if (lpdwCounter == NULL)
        return;

    (*lpdwCounter)++;
}

BOOL
CRegZone::CRegZoneCache::FindCacheEntry(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, int& riEntry )
{
    BOOL fFound = FALSE;

    for ( riEntry = 0; (m_arzce[riEntry].m_dwZone != ZONEID_INVALID) && (riEntry < MAX_REG_ZONE_CACHE); riEntry++ )
    {
        if ( m_arzce[riEntry].m_dwZone == dwZone &&
             m_arzce[riEntry].m_dwAction == dwAction &&
             m_arzce[riEntry].m_fUseHKLM == fUseHKLM )
        {
            fFound = TRUE;
            break;
        }
    }

    if(!fFound)
    {
        riEntry = m_iAdd;
        m_iAdd = (m_iAdd + 1) % MAX_REG_ZONE_CACHE;    // next index to add an entry that's not found
    }
    
    return fFound;
}
  
void 
CRegZone::CRegZoneCache::CRegZoneCacheEntry::Set(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, DWORD dwPolicy)
{
    m_dwZone = dwZone;    
    m_dwAction = dwAction;
    m_fUseHKLM = fUseHKLM;
    m_dwPolicy = dwPolicy;
}

void 
CRegZone::CRegZoneCache::CRegZoneCacheEntry::Flush(void)
{
    m_dwZone = ZONEID_INVALID;
}
          

