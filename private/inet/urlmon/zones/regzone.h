//  File:       regzone.h
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

#ifndef _REGZONE_H_
#define _REGZONE_H_

// Constants corresponding to the registry.
#define MAX_REG_ZONE_CACHE      20
#define URLZONE_INVALID         URLZONE_USER_MAX+1

#define MAX_ZONE_NAME           240
#define MAX_ZONE_PATH           256    // This is "Standard\\ZoneName"
#define MAX_VALUE_NAME          256

#define ZONEID_INVALID          0xFFFFFFFF
#define URLZONE_FINDCACHEENTRY  0xFFFFFFFF

// There are two registry keys under which the information is copied. 
// One of them is "Zones" which holds the actual information and the other
// one is "TemplatePolicies" which holds the high medium and low policies. 
// This enumeration indicates which part of the registry to read. 

enum REGZONEUSE { REGZONEUSEZONES, REGZONEUSETEMPLATE };

class CRegZone 
{
public:
    CRegZone();
    // Seperate init function to allow for failure on return. 
    BOOL Init(LPCTSTR lpStr, BOOL bCreate = TRUE, REGZONEUSE regZoneUse = REGZONEUSEZONES, BOOL bSystem = TRUE);

    ~CRegZone();

    // Attributes
    DWORD GetZoneId() const  { return m_dwZoneId; }
    LPTSTR GetZoneName() const { return m_lpZoneName; }

    // Returns NULL terminated string, free using CoTaskFree
    STDMETHODIMP  GetZoneAttributes (ZONEATTRIBUTES& zoneAttrib);
    STDMETHODIMP  SetZoneAttributes (const ZONEATTRIBUTES& zoneAttrib);

    STDMETHODIMP  GetActionPolicy (DWORD dwAction, URLZONEREG urlZone, DWORD& dwPolicy) const;
    STDMETHODIMP  SetActionPolicy (DWORD dwAction, URLZONEREG urlZone, DWORD dwPolicy);

    STDMETHODIMP  GetCustomPolicy (REFGUID guid, URLZONEREG urlZone, BYTE** ppByte, DWORD *pcb) const; 
    STDMETHODIMP  SetCustomPolicy (REFGUID guid, URLZONEREG urlZone, BYTE* pByte, DWORD cb);

    STDMETHODIMP  CopyTemplatePolicies(DWORD dwTemplateIndex);

    BOOL  UpdateZoneMapFlags( );

    // Should we display this zone in the UI. 
    // Note that Zones not displayed in UI are not included in the zone enumeration.
    inline BOOL IsUIZone() const;
   
protected:
    inline IsValid() const { return (m_dwZoneId != ZONEID_INVALID); }
    inline BOOL UseHKLM(URLZONEREG urlZoneReg) const;
 
    // Helper functions
    inline BOOL IsHardCodedZone() const { return FALSE; }
    inline BOOL GetHardCodedZonePolicy(DWORD dwAction, DWORD& dwPolicy) const;


    inline BOOL  QueryTemplatePolicyIndex(CRegKey& regKey, LPCTSTR psz, LPDWORD pdw) const;
    inline BOOL  SetTemplatePolicyIndex(CRegKey& regKey, LPCTSTR psz, DWORD dw);
        
    static BOOL     IsAttributeName(LPCTSTR psz);
    static LPCTSTR  GetTemplateNameFromIndex ( URLTEMPLATE urlTemplateIndex);
    inline static BOOL     IsValidTemplateIndex( DWORD dwTemplateIndex ); 
    static BOOL     GetAggregateAction(DWORD dwAction, LPDWORD dwAggregateAction);
    static void     KludgeMapAggregatePolicy(DWORD dwAction, LPDWORD pdwAction);
    static VOID     IncrementGlobalCounter( );

// Methods/members to support caching
protected:

    class CRegZoneCache {
    public:
        CRegZoneCache(void);
        ~CRegZoneCache(void);

        BOOL Lookup(DWORD dwZone, LPTSTR lpZonePath, DWORD dwAction, BOOL fUseHKLM, DWORD *pdwPolicy);
        void Add(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, DWORD dwPolicy, int iEntry = URLZONE_FINDCACHEENTRY);
        void Flush(void);

        static VOID IncrementGlobalCounter( );

    protected:

        // Counters to flag cross-process cache invalidation.
        DWORD         m_dwPrevCounter ; // Global counter so we can correctly invalidate the cache if 
                                        // user changes options.
        static HANDLE s_hMutexCounter;  // mutex controlling access to shared memory counter.
 
        BOOL IsCounterEqual() const;
        VOID SetToCurrentCounter();

        // The body of the cache is this array of cache entries.
        // Cross-thread access control for the array is by critical section.

        CRITICAL_SECTION m_csectZoneCache; // assumes only one, static instance of the cache 


        struct CRegZoneCacheEntry {
            CRegZoneCacheEntry(void) :
                m_dwZone(ZONEID_INVALID),
                m_dwAction(0),
                m_fUseHKLM(FALSE),
                m_dwPolicy(0) {};
            ~CRegZoneCacheEntry(void) { Flush(); };

            void Set(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, DWORD dwPolicy);
            void Flush(void);
            
            DWORD      m_dwZone;
            DWORD      m_dwAction;
            BOOL       m_fUseHKLM;
            DWORD      m_dwPolicy;
        }; // CRegZoneCacheEntry

        CRegZoneCacheEntry   m_arzce[MAX_REG_ZONE_CACHE];
        int                  m_iAdd;

        BOOL FindCacheEntry(DWORD dwZone, DWORD dwAction, BOOL fUseHKLM, int& riEntry ); // must be called under critical section.

    }; // CRegZoneCache

    static CRegZoneCache s_rzcache;

private:

    DWORD  m_dwZoneId;
    DWORD  m_dwZoneFlags;
    LPTSTR m_lpZoneName;
    LPTSTR m_lpZonePath;

    BOOL m_bHKLMOnly;
    BOOL m_bStandard;         
    BOOL m_bZoneLockOut;         // Is the whole zone locked out. 
    REGZONEUSE m_regZoneUse;
};

typedef CRegZone *LPREGZONE;

BOOL CRegZone::UseHKLM(URLZONEREG urlZoneReg) const
{
    BOOL bReturn;

    switch(urlZoneReg)
    {
        case URLZONEREG_HKLM:
            bReturn = TRUE;
            break;
        case URLZONEREG_HKCU:
            bReturn = FALSE;
            break;
        case URLZONEREG_DEFAULT:
            bReturn = m_bHKLMOnly;
            break;
        default:
            TransAssert(FALSE);
    }

    return bReturn;
}

BOOL CRegZone::IsValidTemplateIndex(DWORD dwTemplateIndex)
{
    BOOL bReturn = FALSE;

    switch (dwTemplateIndex)    
    {
        case URLTEMPLATE_CUSTOM:
        case URLTEMPLATE_LOW:
        case URLTEMPLATE_MEDLOW:
        case URLTEMPLATE_MEDIUM:
        case URLTEMPLATE_HIGH:
            bReturn = TRUE;
            break;
    }
    return bReturn;
}

BOOL CRegZone::QueryTemplatePolicyIndex(CRegKey& regKey, LPCTSTR psz, LPDWORD pdw) const
{
    LONG lRet;

    lRet = regKey.QueryValue(pdw, psz);

    if (NO_ERROR != lRet)
    {
        *pdw = URLTEMPLATE_CUSTOM;
    }
    else if (*pdw < URLTEMPLATE_PREDEFINED_MIN || *pdw > URLTEMPLATE_PREDEFINED_MAX)
    {
        // Invalid value, just return back default.
        *pdw = URLTEMPLATE_CUSTOM;
    }

    return TRUE;
}

BOOL CRegZone::SetTemplatePolicyIndex(CRegKey& regKey, LPCTSTR psz, DWORD dwIndex)
{   
    // Write this only if it is a valid template index. 
    if (IsValidTemplateIndex(dwIndex))
    {
        if (regKey.SetValue(dwIndex, psz) == NO_ERROR)
            return TRUE;
    }
    else 
    {
        TransAssert(FALSE); 
    }

    return FALSE;
}


BOOL CRegZone::GetHardCodedZonePolicy(DWORD dwAction, DWORD& dwPolicy) const
{
    TransAssert(IsHardCodedZone());

    if (!IsHardCodedZone())
        return FALSE;

    switch(dwAction)
    {
        case URLACTION_JAVA_PERMISSIONS:
            dwPolicy = URLPOLICY_JAVA_HIGH;
            break;
        case URLACTION_CREDENTIALS_USE:
            dwPolicy = URLPOLICY_CREDENTIALS_SILENT_LOGON_OK;
            break;
        case URLACTION_AUTHENTICATE_CLIENT:
            dwPolicy = URLPOLICY_AUTHENTICATE_CLEARTEXT_OK;
            break; 
        case URLACTION_ACTIVEX_OVERRIDE_OBJECT_SAFETY:
            dwPolicy = URLPOLICY_QUERY;
            break;          
        default:
            dwPolicy = 0;
            break;
    }

    return TRUE;
}

BOOL CRegZone::IsUIZone() const
{
    return (m_dwZoneFlags & ZAFLAGS_NO_UI) ? FALSE : TRUE;
}

// This is the class that maintains the list of the RegZones currently in action

class CRegZoneContainer
{
public:

    CRegZoneContainer();
    ~CRegZoneContainer();

public:
    BOOL Attach(BOOL bUseHKLM, REGZONEUSE regZoneUse = REGZONEUSEZONES);
    BOOL Detach();
    BOOL SelfHeal(BOOL bUseHKLM);

    CRegZone * GetRegZoneByName(LPCTSTR lpszZoneName) const;
    CRegZone * GetRegZoneById(DWORD dwZoneId) const;
    DWORD  GetZoneCount() const { return m_cZones; };

    STDMETHODIMP CreateZoneEnumerator(DWORD *pdwEnum, DWORD *pdwCount);
    STDMETHODIMP GetZoneAt(DWORD dwEnum, DWORD dwIndex, DWORD *pdwZone);
    STDMETHODIMP DestroyZoneEnumerator(DWORD dwEnum);

protected:
   // Used internally only.
   struct CRegListElem {
        CRegListElem * next;
        CRegZone     * pRegZone;
        DWORD   dwZoneIndex;
    };

    struct CZoneEnumList {
        DWORD dwEnum;   // Cookie indicating which enum this corresponds to.
        CZoneEnumList * next;
    };

    CZoneEnumList * m_pZoneEnumList;
    DWORD m_dwNextEnum;

    // Is this enumerator a valid enumerator.
    BOOL VerifyZoneEnum(DWORD dwEnum) const;

private:

    CRegZone**  m_ppRegZones;        // Array of RegZones.
    DWORD       m_cZones;             // # of Zones.
    BOOL        m_bHKLMOnly;
    CRITICAL_SECTION m_csect;
};
                                    
#endif  // _REGZONE_H_
        

