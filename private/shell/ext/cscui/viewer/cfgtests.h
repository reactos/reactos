#ifndef _INC_CSCVIEW_CFGTESTS_H
//
// See header in cfgtests.cpp for module description.
//
// Only compile this code in debug builds and when testing configuration settings.
//
#if DBG
#ifdef TEST_CONFIG_SETTINGS

#ifndef _INC_CSCVIEW_CONFIG_H
#   include "config.h"
#endif


class CSettingsTest
{
    public:
        CSettingsTest(void);
        int Run(bool bVerbose = false); // Run the tests.  Return error count.

    private:
        //
        // These must correspond to the order of the associates strings in m_rgpszRegValues[].
        //
        enum eRegValue { VAL_KEYDEFAULT,
                         VAL_DEFCACHESIZE,
                         VAL_CSCENABLED,
                         VAL_CSCENCRYPTED,
                         VAL_GOOFFLINEACTION,
                         VAL_NOCACHEVIEWER,
                         VAL_NOCONFIGCACHE,
                         VAL_NOCONFIGCACHESIZE,
                         VAL_NOCONFIGGOOFFLINEACTION,
                         VAL_NOCONFIGSYNCATLOGOFF,
                         VAL_NOCUSTOMIZEGOOFFLINEACTION,
                         VAL_NOMAKEAVAILABLEOFFLINE,
                         VAL_NOMANUALSYNC,
                         VAL_SYNCATLOGOFF,
                         VAL_EXTEXCLUSIONLIST };

        CSettings m_Settings;       // The "test subject".
        CString   m_strCurrentTest; // Name of current test (for output).
        bool      m_bVerbose;       // Output to debugger?
        int       m_iTest;          // Current test number.
        RegKey    m_keyPolicyCU;    // User policy key.
        RegKey    m_keyPolicyLM;    // Machine policy key.
        RegKey    m_keyPrefCU;      // User preference key.
        RegKey    m_keyPolicyCUGOA; // User policy custom go-offline actions.
        RegKey    m_keyPolicyLMGOA; // Machine policy custom go-offline actions.
        RegKey    m_keyPrefCUGOA;   // User preference custom go-offline actions.
                
        enum eRegKey  { KEY_MACHINEPOLICY, 
                        KEY_USERPOLICY, 
                        KEY_USERPREF, 
                        KEY_MACHINEPOLICY_GOA,
                        KEY_USERPOLICY_GOA,
                        KEY_USERPREF_GOA,
                        NUMKEYS };

        RegKey   *m_rgpRegKeys[NUMKEYS];     // Ptrs to keys for convenience.

        static   LPCTSTR m_rgpszRegValues[]; // Reg value name strings.

        //
        // Registry helper functions.
        //
        HRESULT SetRegValue(eRegKey eKey, eRegValue eValue, DWORD dwValue)
            { return SetRegValue(eKey, m_rgpszRegValues[eValue], dwValue); }

        HRESULT SetRegValue(eRegKey eKey, LPCTSTR pszValueName, DWORD dwValue);

        HRESULT SetRegValue(eRegKey eKey, eRegValue eValue, LPCTSTR pszValue)
            { return SetRegValue(eKey, m_rgpszRegValues[eValue], pszValue); }

        HRESULT SetRegValue(eRegKey eKey, LPCTSTR pszValueName, LPCTSTR pszValue);

        HRESULT DelRegValue(eRegKey eKey, eRegValue eValue);
        HRESULT DelAllRegValues(eRegKey eKey);

        void DelAllRegValues(void);

        HRESULT OpenRegKey(eRegKey eKey, DWORD dwAccess)
            { return m_rgpRegKeys[eKey]->Open(dwAccess, true); }

        void CloseRegKey(eRegKey eKey)
            { m_rgpRegKeys[eKey]->Close(); }

        struct GOATestData
        {
            eRegKey eKey;
            LPCTSTR pszServer;
            CConfig::OfflineAction action;
        };

        //
        // Test and reporting helper functions.
        //
        int Test(int iValue, int iExpected);
        void SetCurrentTest(LPCTSTR pszTest);
        void Report(CString& s);
        bool GOATestDataSetByPolicy(const GOATestData& td);
        int TestGOA(const GOATestData *pTestData, int n);

        //
        // Test procedures.
        //
        int TestSyncAtLogoff(void);
        int TestGoOfflineAction(void);
        int TestNoConfigCache(void);
        int TestNoConfigCacheSize(void);
        int TestNoManualSync(void);
        int TestNoCustomizeGoOfflineAction(void);
        int TestNoCacheViewer(void);
        int TestDefCacheSize(void);
        int TestCscEnabled(void);
        int TestCscEncrypted(void);
        int TestCustomGoOfflineActions(void);
        int TestExcludedExtensions(void);
};


#endif // TEST_CONFIG_SETTINGS

#endif // DBG

#endif // _INC_CSCVIEW_CFGTESTS_H
