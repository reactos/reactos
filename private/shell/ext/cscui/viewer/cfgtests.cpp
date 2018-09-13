#include "pch.h"
#pragma hdrstop
//
// This module is for testing that the CSettings class member functions properly
// reflect the state of the registry back to the caller.  CSCUI settings can 
// exist as one or more of the following:
//
// 1. Machine policy.
// 2. User policy.
// 3. User preference.
//
// When a setting can exist as both machine and user policy, machine policy
// usually takes precedence or a merging operation occurs when applicable.
// When a setting can exist as policy and preference, user preference takes
// precedence unless an applicable restrictive policy is in place or merging
// is appropriate.  These behaviors are setting-specific.
//
// The class CSettingsTest is designed so you just instantiate an instance
// and tell it to run in either verbose or non-verbose mode.  Verbose mode will
// dump out the test results and any failures to the debugger terminal.  Non-verbose
// mode doesn't.   Both modes display a final MessageBox indicating the number
// of test failures.
//
// To activate this code in the build, simply define the macro TEST_CONFIG_SETTINGS
// and build a checked version.  Currently, I have a CSettingsTest instance
// defined in the WM_INITDIALOG handler of the CSCUI Options prop page.
// 
// CAVEAT:  This code does not verify that MMC/Poledit write the correct policy
//          values nor does it verify that the CSCUI respond correctly to those
//          values.  It also does not verify that the CSCUI correctly records
//          user preferences into the registry.
//
//
// [brianau - 7/24/98]
//
// Only compile this code in debug builds and when testing configuration settings.
//
#if DBG
#if TEST_CONFIG_SETTINGS

#include "cfgtests.h"

//
// Reg value name strings.
// These must remain in the same order as the eRegValue enumeration constants.
//
LPCTSTR CSettingsTest::m_rgpszRegValues[] = { 
            TEXT(""),
            REGSTR_VAL_DEFCACHESIZE,
            REGSTR_VAL_CSCENABLED,
            REGSTR_VAL_CSCENCRYPTED,
            REGSTR_VAL_GOOFFLINEACTION,
            REGSTR_VAL_NOCACHEVIEWER,
            REGSTR_VAL_NOCONFIGCACHE,
            REGSTR_VAL_NOCONFIGCACHESIZE,
            REGSTR_VAL_NOCONFIGGOOFFLINEACTION,
            REGSTR_VAL_NOCONFIGSYNCATLOGOFF,
            REGSTR_VAL_NOCUSTOMIZEGOOFFLINEACTION,
            REGSTR_VAL_NOMAKEAVAILABLEOFFLINE,
            REGSTR_VAL_NOMANUALSYNC,
            REGSTR_VAL_SYNCATLOGOFF,
            REGSTR_VAL_EXTEXCLUSIONLIST };


CSettingsTest::CSettingsTest(
    void
    ) : m_bVerbose(false),
        m_iTest(0),
        m_keyPolicyCU(HKEY_CURRENT_USER,     REGSTR_KEY_POLICY),
        m_keyPolicyLM(HKEY_LOCAL_MACHINE,    REGSTR_KEY_POLICY),
        m_keyPrefCU(HKEY_CURRENT_USER,       REGSTR_KEY_NETCACHE),
        m_keyPolicyCUGOA(HKEY_CURRENT_USER,  CString(REGSTR_KEY_POLICY) + CString(TEXT("\\")) + CString(REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS)),
        m_keyPolicyLMGOA(HKEY_LOCAL_MACHINE, CString(REGSTR_KEY_POLICY) + CString(TEXT("\\")) + CString(REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS)),
        m_keyPrefCUGOA(HKEY_CURRENT_USER,    CString(REGSTR_KEY_NETCACHE) + CString(TEXT("\\")) + CString(REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS))
{
    m_rgpRegKeys[KEY_MACHINEPOLICY]     = &m_keyPolicyLM;
    m_rgpRegKeys[KEY_USERPOLICY]        = &m_keyPolicyCU;
    m_rgpRegKeys[KEY_USERPREF]          = &m_keyPrefCU;
    m_rgpRegKeys[KEY_MACHINEPOLICY_GOA] = &m_keyPolicyLMGOA;
    m_rgpRegKeys[KEY_USERPOLICY_GOA]    = &m_keyPolicyCUGOA;
    m_rgpRegKeys[KEY_USERPREF_GOA]      = &m_keyPrefCUGOA;
}

//
// Run all the tests.
//
int 
CSettingsTest::Run(
    bool bVerbose
    )
{
    typedef int (CSettingsTest::*PTF)(void);  // Ptr to test function.

    //
    // To add a new test, just add it's function name here.
    //
    PTF rgpfnTests[] = { 
                         TestSyncAtLogoff,
                         TestGoOfflineAction,
                         TestNoConfigCache,
                         TestNoConfigCacheSize,
                         TestNoManualSync,
                         TestNoCustomizeGoOfflineAction,
                         TestNoCacheViewer,
                         TestDefCacheSize,
                         TestCscEnabled,
                         TestCscEncrypted,
                         TestCustomGoOfflineActions,
                         TestExcludedExtensions 
                       };

    m_bVerbose = bVerbose;
    m_iTest    = 0;

    int cErrors = 0;
    for (int i = 0; i < ARRAYSIZE(rgpfnTests); i++)
    {
        PTF pfn = rgpfnTests[i];
        cErrors += (this->*pfn)();  // Run the test function.
    }
    //
    // Report final test results.
    //
    CString s;
    s.Format(TEXT("Tests completed with %1!d! %2"), 
             cErrors, 1 == cErrors ? TEXT("error") : TEXT("errors"));
    m_bVerbose = true; // Force output.
    Report(s);
    MessageBox(NULL, s.Cstr(), TEXT("CSCUI Config Settings Test Complete"), MB_OK | MB_ICONINFORMATION);
    return cErrors;
}

//
// Perform a single test.
//
int
CSettingsTest::Test(
    int iValue,
    int iExpected
    )
{
    if (m_bVerbose)
    {
        if (iValue != iExpected)
        {
            //
            // Report failure.
            //
            static CString s;
            s.Format(TEXT("Test %1!d! \"%2\" failed, was %3!d! expected %4!d!"),
                           m_iTest, m_strCurrentTest.Cstr(), iValue, iExpected);
            Report(s);
        }
    }
    m_iTest++;
    return iValue == iExpected ? 0 : 1;
}


//
// Set up CSettingsTest object for a new test.
//
void
CSettingsTest::SetCurrentTest(
    LPCTSTR pszTest
    )
{
    m_strCurrentTest = pszTest;
    m_iTest = 0;
    if (m_bVerbose)
    {
        static CString s;
        s.Format(TEXT("Test: \"%1\""), pszTest);
        Report(s);
    }
}

//
// Append "\n\r" to a string and write the debugger.
//
void
CSettingsTest::Report(
    CString& s
    )
{
    static const CString strNL(TEXT("\n\r"));
    s += strNL;
    OutputDebugString(s);
}


//
// Things to verify (as applicable)
//
// 1. HKLM policy can be set.
// 2. HKCU policy can be set.
// 3. HKLM policy takes precedence over HKCU policy.
// 4. User preference can take precedence over certain policies.
// 5. Policy restrictions can prevent use of user preference settings in 
//    favor of policy values.
//


int
CSettingsTest::TestSyncAtLogoff(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("SyncAtLogoff"));
    DelAllRegValues();

    //
    // Test default value.
    //
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncFull);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncFull);
    SetRegValue(KEY_MACHINEPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncPartial);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    DelRegValue(KEY_MACHINEPOLICY, VAL_SYNCATLOGOFF);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncFull);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncFull);
    SetRegValue(KEY_USERPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncPartial);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    DelRegValue(KEY_USERPOLICY, VAL_SYNCATLOGOFF);
    //
    // Test HKCU-only user pref setting.
    //
    SetRegValue(KEY_USERPREF, VAL_SYNCATLOGOFF, CConfig::eSyncPartial);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    SetRegValue(KEY_USERPREF, VAL_SYNCATLOGOFF, CConfig::eSyncFull);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncFull);
    DelRegValue(KEY_USERPREF, VAL_SYNCATLOGOFF);
    //
    // HKLM policy takes precedence over HKCU policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncPartial);
    SetRegValue(KEY_USERPOLICY, VAL_SYNCATLOGOFF, CConfig::eSyncFull);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    //
    // HKCU user pref takes precedence over policy.
    //
    SetRegValue(KEY_USERPREF, VAL_SYNCATLOGOFF, CConfig::eSyncFull);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncFull);
    //
    // Restriction prevents user preference overriding policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGSYNCATLOGOFF, 1);
    cErrors += Test(m_Settings.SyncAtLogoff(), CConfig::eSyncPartial);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}



int
CSettingsTest::TestGoOfflineAction(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("GoOfflineAction"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineFail);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineFail);
    SetRegValue(KEY_MACHINEPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineSilent);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    DelRegValue(KEY_MACHINEPOLICY, VAL_GOOFFLINEACTION);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineFail);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineFail);
    SetRegValue(KEY_USERPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineSilent);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    DelRegValue(KEY_USERPOLICY, VAL_GOOFFLINEACTION);
    //
    // Test HKCU-only user pref setting.
    //
    SetRegValue(KEY_USERPREF, VAL_GOOFFLINEACTION, CConfig::eGoOfflineSilent);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    SetRegValue(KEY_USERPREF, VAL_GOOFFLINEACTION, CConfig::eGoOfflineFail);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineFail);
    DelRegValue(KEY_USERPREF, VAL_GOOFFLINEACTION);
    //
    // HKLM policy takes precedence over HKCU policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineSilent);
    SetRegValue(KEY_USERPOLICY, VAL_GOOFFLINEACTION, CConfig::eGoOfflineFail);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    //
    // HKCU user pref takes precedence over policy.
    //
    SetRegValue(KEY_USERPREF, VAL_GOOFFLINEACTION, CConfig::eGoOfflineFail);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineFail);
    //
    // Restriction prevents user preference overriding policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGGOOFFLINEACTION, 1);
    cErrors += Test(m_Settings.GoOfflineAction(), CConfig::eGoOfflineSilent);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestNoCustomizeGoOfflineAction(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("NoCustomizeGoOfflineAction"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, 1);
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, DWORD(0));
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 0);
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, 1);
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, DWORD(0));
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 0);
    DelRegValue(KEY_USERPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION);
    //
    // HKLM policy takes precedence over HKCU policy.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, DWORD(0));
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 0);
    //
    // Restricting cache configuration also restricts customization of 
    // go-offline actions.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, DWORD(0));
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, 1);
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 1);
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE);
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE, 1);
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 1);
    DelRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE);
    //
    // NoCustomizeGoOfflineAction is not settable via user prefrence.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, DWORD(0));
    SetRegValue(KEY_USERPREF, VAL_NOCUSTOMIZEGOOFFLINEACTION, 1);
    cErrors += Test(m_Settings.NoCustomizeGoOfflineAction(), 0);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestNoConfigCache(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("NoConfigCache"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.NoConfigCache(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, 1);
    cErrors += Test(m_Settings.NoConfigCache(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCache(), 0);
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE, 1);
    cErrors += Test(m_Settings.NoConfigCache(), 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCache(), 0);
    DelRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE);
    //
    // Test HKLM policy preference over HKCU policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCache(), 1);
    //
    // NoConfigCache is not settable via user prefrence.
    //
    DelRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHE);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, DWORD(0));
    SetRegValue(KEY_USERPREF, VAL_NOCONFIGCACHE, 1);
    cErrors += Test(m_Settings.NoConfigCache(), 0);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestNoConfigCacheSize(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("NoConfigCacheSize"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.NoConfigCacheSize(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHESIZE, 1);
    cErrors += Test(m_Settings.NoConfigCacheSize(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHESIZE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCacheSize(), 0);
    //
    // Test HKCU-only policy setting.  NoConfigCacheSize is HKLM only
    // so HKCU policy setting should have no effect.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHESIZE, 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHESIZE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCacheSize(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHESIZE, DWORD(0));
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHESIZE, 1);
    cErrors += Test(m_Settings.NoConfigCacheSize(), DWORD(0));
    //
    // Restricting cache configuration also restricts cache size
    // configuration.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE, 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHESIZE, DWORD(0));
    cErrors += Test(m_Settings.NoConfigCacheSize(), 1);
    //
    // NoConfigCacheSize is not settable via user prefrence.
    //
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHE);
    DelRegValue(KEY_USERPOLICY, VAL_NOCONFIGCACHESIZE);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCONFIGCACHESIZE, DWORD(0));
    SetRegValue(KEY_USERPREF, VAL_NOCONFIGCACHESIZE, 1);
    cErrors += Test(m_Settings.NoConfigCacheSize(), 0);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestNoManualSync(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("NoManualSync"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.NoManualSync(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOMANUALSYNC, 1);
    cErrors += Test(m_Settings.NoManualSync(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOMANUALSYNC, DWORD(0));
    cErrors += Test(m_Settings.NoManualSync(), 0);
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOMANUALSYNC);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOMANUALSYNC, 1);
    cErrors += Test(m_Settings.NoManualSync(), 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOMANUALSYNC, DWORD(0));
    cErrors += Test(m_Settings.NoManualSync(), 0);
    DelRegValue(KEY_USERPOLICY, VAL_NOMANUALSYNC);
    //
    // HKLM policy takes preference over HKCU policy.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOMANUALSYNC, 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOMANUALSYNC, DWORD(0));
    cErrors += Test(m_Settings.NoManualSync(), 1);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestNoCacheViewer(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("NoCacheViewer"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.NoCacheViewer(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCACHEVIEWER, 1);
    cErrors += Test(m_Settings.NoCacheViewer(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCACHEVIEWER, 0);
    cErrors += Test(m_Settings.NoCacheViewer(), 0);
    DelRegValue(KEY_MACHINEPOLICY, VAL_NOCACHEVIEWER);
    //
    // Test HKCU-only policy setting.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCACHEVIEWER, 1);
    cErrors += Test(m_Settings.NoCacheViewer(), 1);
    SetRegValue(KEY_USERPOLICY, VAL_NOCACHEVIEWER, 0);
    cErrors += Test(m_Settings.NoCacheViewer(), 0);
    DelRegValue(KEY_USERPOLICY, VAL_NOCACHEVIEWER);
    //
    // HKLM policy takes precedence over HKCU policy.
    //
    SetRegValue(KEY_USERPOLICY, VAL_NOCACHEVIEWER, 0);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCACHEVIEWER, 1);
    cErrors += Test(m_Settings.NoCacheViewer, 1);
    //
    // NoCacheViewer is not settable via user prefrence.
    //
    DelRegValue(KEY_USERPOLICY, VAL_NOCACHEVIEWER);
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCACHEVIEWER, 1);
    SetRegValue(KEY_USERPREF, VAL_NOCACHEVIEWER, 0);
    cErrors += Test(m_Settings.NoCacheViewer, 1);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestDefCacheSize(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("DefaultCacheSize"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.DefaultCacheSize(), 1000);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_DEFCACHESIZE, 2000);
    cErrors += Test(m_Settings.DefaultCacheSize(), 2000);
    //
    // DefCacheSize is HKLM policy only.
    //
    SetRegValue(KEY_USERPOLICY, VAL_DEFCACHESIZE, 3000);
    cErrors += Test(m_Settings.DefaultCacheSize(), 2000);
    //
    // DefCacheSize is not settable via user prefrence.
    //
    DelRegValue(KEY_USERPOLICY, VAL_DEFCACHESIZE);
    SetRegValue(KEY_USERPREF, VAL_DEFCACHESIZE, 5000);
    cErrors += Test(m_Settings.DefaultCacheSize(), 2000);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


int
CSettingsTest::TestCscEnabled(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("CscEnabled"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.CscEnabled(), 1);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_CSCENABLED, DWORD(0));
    cErrors += Test(m_Settings.CscEnabled(), 0);
    SetRegValue(KEY_MACHINEPOLICY, VAL_CSCENABLED, 1);
    cErrors += Test(m_Settings.CscEnabled(), 1);
    //
    // CscEnabled is HKLM policy only.
    //
    SetRegValue(KEY_USERPOLICY, VAL_CSCENABLED, DWORD(0));
    cErrors += Test(m_Settings.CscEnabled(), 1);
    //
    // CscEnabled is settable via user preference but it's not
    // stored in the user-preference section of the registry.
    // It's stored under the HKLM CSC settings data.  This test
    // merely verifies that a user preference reg entry can't
    // override the current policy setting.
    //
    DelRegValue(KEY_USERPOLICY, VAL_CSCENABLED);
    SetRegValue(KEY_USERPREF, VAL_CSCENABLED, DWORD(0));
    cErrors += Test(m_Settings.CscEnabled(), 1);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}

int
CSettingsTest::TestCscEncrypted(
    void
    )
{
    int cErrors = 0;   // Running error count

    SetCurrentTest(TEXT("CscEncrypted"));
    DelAllRegValues();
    //
    // Test default value.
    //
    cErrors += Test(m_Settings.CscEncrypted(), 0);
    //
    // Test HKLM-only policy setting.
    //
    SetRegValue(KEY_MACHINEPOLICY, VAL_CSCENCRYPTED, 1);
    cErrors += Test(m_Settings.CscEncrypted(), 1);
    SetRegValue(KEY_MACHINEPOLICY, VAL_CSCENCRYPTED, DWORD(0));
    cErrors += Test(m_Settings.CscEncrypted(), 0);
    //
    // CscEncrypted is HKLM policy only.
    //
    SetRegValue(KEY_USERPOLICY, VAL_CSCENCRYPTED, 1);
    cErrors += Test(m_Settings.CscEncrypted(), DWORD(0));
    //
    // CscEncrypted is settable via user preference but it's not
    // stored in the user-preference section of the registry.
    // It's stored under the HKLM CSC settings data.  This test
    // merely verifies that a user preference reg entry can't
    // override the current policy setting.
    //
    DelRegValue(KEY_USERPOLICY, VAL_CSCENCRYPTED);
    SetRegValue(KEY_USERPREF, VAL_CSCENCRYPTED, 1);
    cErrors += Test(m_Settings.CscEncrypted(), 0);
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}

//
// Things to test for:
//
// 1. Leading spaces and "\\" prefix in server name.
// 2. Merging of policy and user-preference values.
//    Precedence in decending order is:
//        a. Machine policy.
//        b. User policy.
//        c. User preference.
//
// 3. Removal of duplicate entries (key is server name).
//
int
CSettingsTest::TestCustomGoOfflineActions(
    void
    )
{
    int cErrors = 0;   // Running error count
    int i;
    GOATestData rgtd[] = {
               { KEY_MACHINEPOLICY_GOA, TEXT("kernel"),      CConfig::eGoOfflineSilent },
               { KEY_MACHINEPOLICY_GOA, TEXT("  rastaman"),  CConfig::eGoOfflineFail   },
               { KEY_MACHINEPOLICY_GOA, TEXT("\\\\orville"), CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("orville"),     CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("  \\\\worf"),  CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("scratch"),     CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("products1"),   CConfig::eGoOfflineFail   },
               { KEY_USERPOLICY_GOA,    TEXT("phoenix"),     CConfig::eGoOfflineSilent },
               { KEY_USERPREF_GOA,      TEXT("phoenix"),     CConfig::eGoOfflineFail   },
               { KEY_USERPREF_GOA,      TEXT("\\trango"),    CConfig::eGoOfflineSilent },
               { KEY_USERPREF_GOA,      TEXT("brianau1"),    CConfig::eGoOfflineFail   },
               { KEY_USERPREF_GOA,      TEXT("popcorn"),     CConfig::eGoOfflineSilent },
               { KEY_USERPREF_GOA,      TEXT("scratch"),     CConfig::eGoOfflineFail   }
               };

    SetCurrentTest(TEXT("CustomGoOfflineActions"));
    DelAllRegValues();
    //
    // Test HKLM-only policy setting.
    //
    for (i = 0; i < ARRAYSIZE(rgtd); i++)
    {
        TCHAR szValue[10];
        wsprintf(szValue, TEXT("%d"), rgtd[i].action);
        HRESULT hr = SetRegValue(rgtd[i].eKey, rgtd[i].pszServer, szValue);
        if (FAILED(hr))
        {
            CString s;
            s.Format(TEXT("Error 0x%1!08X! calling SetRegValue"), hr);
            Report(s);
        }
    }

    //
    // Results are sorted alphabetically by server name as that's how
    // CSettings::GetCustomGoOfflineActions() returns them.  Pre-ordering of
    // expected test results makes comparison easier.
    //
    GOATestData rgResults1[] = {
               { KEY_USERPREF_GOA,      TEXT("brianau1"),  CConfig::eGoOfflineFail   },
               { KEY_MACHINEPOLICY_GOA, TEXT("kernel"),    CConfig::eGoOfflineSilent },
               { KEY_MACHINEPOLICY_GOA, TEXT("orville"),   CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("phoenix"),   CConfig::eGoOfflineSilent },
               { KEY_USERPREF_GOA,      TEXT("popcorn"),   CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("products1"), CConfig::eGoOfflineFail   },
               { KEY_MACHINEPOLICY_GOA, TEXT("rastaman"),  CConfig::eGoOfflineFail   },
               { KEY_USERPOLICY_GOA,    TEXT("scratch"),   CConfig::eGoOfflineSilent },
               { KEY_USERPREF_GOA,      TEXT("trango"),    CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("worf"),      CConfig::eGoOfflineSilent },
               };

    cErrors += TestGOA(rgResults1, ARRAYSIZE(rgResults1));

    //
    // Restrict custom actions to policy-only and retest.  Shouldn't see any
    // user preferences in resulting GOA set.
    // 
    SetRegValue(KEY_MACHINEPOLICY, VAL_NOCUSTOMIZEGOOFFLINEACTION, 1);
    GOATestData rgResults2[] = {
               { KEY_MACHINEPOLICY_GOA, TEXT("kernel"),    CConfig::eGoOfflineSilent },
               { KEY_MACHINEPOLICY_GOA, TEXT("orville"),   CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("phoenix"),   CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("products1"), CConfig::eGoOfflineFail   },
               { KEY_MACHINEPOLICY_GOA, TEXT("rastaman"),  CConfig::eGoOfflineFail   },
               { KEY_USERPOLICY_GOA,    TEXT("scratch"),   CConfig::eGoOfflineSilent },
               { KEY_USERPOLICY_GOA,    TEXT("worf"),      CConfig::eGoOfflineSilent },
               };

    cErrors += TestGOA(rgResults2, ARRAYSIZE(rgResults2));

    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}

//
// Compare the current custom go-offline actions with an array of "expected"
// results.
//
int
CSettingsTest::TestGOA(
    const GOATestData *pTestData,
    int n
    )
{
    const CArray<CConfig::CustomGOA>& rgGOA = m_Settings.GetCustomGoOfflineActions();
    int iActions = rgGOA.Count();
    int cErrors = 0;
    CString s;

    if (iActions != n)
    {
        s.Format(TEXT("Item count is %1!d!, expected %2!d!"), iActions, n);
        Report(s);
        return 1;  // 1 error.
    }

    const GOATestData *ptd = pTestData;
    for (int i = 0; i < n; i++)
    {
        
        CConfig::CustomGOA goa(ptd->pszServer, ptd->action, GOATestDataSetByPolicy(*ptd));
        if (goa != rgGOA[i])
        {
            cErrors++;
            s.Format(TEXT("GOA failed: Server = \"%1\", expected \"%2\"\nAction = %3!d!, expected %4!d!"),
                     rgGOA[i].GetServerName().Cstr(), ptd->pszServer,
                     rgGOA[i].GetAction(), ptd->action);
            Report(s);
        }
        ptd++;
    }
    m_iTest++;
    return cErrors;
}


//
// Helper for creating temporary CConfig::CustomGOA object.
// See use in CSettingsTest::TestGOA().
//
bool
CSettingsTest::GOATestDataSetByPolicy(
    const GOATestData& td
    )
{
    return KEY_MACHINEPOLICY_GOA == td.eKey ||
           KEY_USERPOLICY_GOA == td.eKey;
}

    
int
CSettingsTest::TestExcludedExtensions(
    void
    )
{
    int cErrors = 0;   // Running error count
    int i;
    CString s;

    struct TestData
    {
        eRegKey eKey;
        LPCTSTR pszExts;
    } rgtd[] = {
        { KEY_MACHINEPOLICY, TEXT(".dbf, cfg, .ABC;xyz;def") },
        { KEY_USERPOLICY,    TEXT("pch, PDB;COM; my ext") }
        };

    SetCurrentTest(TEXT("ExcludedExtensions"));
    DelAllRegValues();
    for (i = 0; i < ARRAYSIZE(rgtd); i++)
    {
        SetRegValue(rgtd[i].eKey, VAL_EXTEXCLUSIONLIST, rgtd[i].pszExts);
    }

    struct TestResults
    {
        bool bExcluded;  // Expected state.
        LPCTSTR pszExt;  // Test this extension.
 
    } rgtr[] = { 
               { true,  TEXT("dbf")    },
               { true,  TEXT("cfg")    },
               { true,  TEXT("abc")    },
               { true,  TEXT("xyz")    },
               { true,  TEXT("def")    },
               { true,  TEXT("PCH")    },
               { true,  TEXT("PDB")    },
               { true,  TEXT("COM")    },
               { true,  TEXT("my ext") },
               { false, TEXT("123")    },
               { false, TEXT("dbx")    },
               { false, TEXT("xbf")    },
               { false, TEXT("dbfx")   }
               };

    for (i = 0; i < ARRAYSIZE(rgtr); i++)
    {
        bool bExcluded = m_Settings.ExtensionExcluded(rgtr[i].pszExt);
        if ( bExcluded != rgtr[i].bExcluded)
        {
            cErrors++;
            s.Format(TEXT("Extension \"%1\" %2 excluded.  Should %3 excluded."),
                     rgtr[i].pszExt,
                     bExcluded ? TEXT("was") : TEXT("was not"),
                     bExcluded ? TEXT("not be") : TEXT("be"));
            Report(s);
        }
    }
    //
    // Post test Clean up.
    //
    DelAllRegValues();
    return cErrors;
}


//
// Open reg key, set a value then close reg key.
//
HRESULT
CSettingsTest::SetRegValue(
    eRegKey eKey, 
    LPCTSTR pszValueName, 
    DWORD dwValue
    )
{ 
    HRESULT hr = OpenRegKey(eKey, KEY_WRITE);
    
    if (SUCCEEDED(hr))
    {
        hr = m_rgpRegKeys[eKey]->SetValue(pszValueName, dwValue);
        CloseRegKey(eKey);
    }
    return hr;
}


HRESULT
CSettingsTest::SetRegValue(
    eRegKey eKey, 
    LPCTSTR pszValueName, 
    LPCTSTR pszValue
    )
{ 
    HRESULT hr = OpenRegKey(eKey, KEY_WRITE);
    
    if (SUCCEEDED(hr))
    {
        hr = m_rgpRegKeys[eKey]->SetValue(pszValueName, pszValue);
        CloseRegKey(eKey);
    }
    return hr;
}


//
// Open reg key, delete a value then close reg key.
//
HRESULT
CSettingsTest::DelRegValue(
    eRegKey eKey, 
    eRegValue eValue
    )
{ 
    HRESULT hr = OpenRegKey(eKey, KEY_WRITE);
    
    if (SUCCEEDED(hr))
    {
        hr = m_rgpRegKeys[eKey]->DeleteValue(m_rgpszRegValues[eValue]);
        CloseRegKey(eKey);
    }
    return hr;
}

HRESULT
CSettingsTest::DelAllRegValues(
    eRegKey eKey
    )
{ 
    HRESULT hr = OpenRegKey(eKey, KEY_ALL_ACCESS);
    
    if (SUCCEEDED(hr))
    {
        hr = m_rgpRegKeys[eKey]->DeleteAllValues(NULL);
        CloseRegKey(eKey);
    }
    return hr;
}

//
// Delete all reg values associated with CSC settings.
//
void
CSettingsTest::DelAllRegValues(
    void
    )
{
    HRESULT hr;
    for (int i = 0; i < ARRAYSIZE(m_rgpRegKeys); i++)
    {
        hr = DelAllRegValues(eRegKey(i));
        if (FAILED(hr))
        {
            DBGERROR((TEXT("DelAllRegValues failed with error 0x%08X\n"), hr));
        }
    }
}

#endif // TEST_CONFIG_SETTINGS
#endif // DBG



