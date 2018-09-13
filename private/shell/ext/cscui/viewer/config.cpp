//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       config.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shlwapi.h>
#include "regstr.h"
#include "config.h"
#include "util.h"

//
// Determine if a character is a DBCS lead byte.
// If build is UNICODE, always returns false.
//
inline bool DBCSLEADBYTE(TCHAR ch)
{
    if (sizeof(ch) == sizeof(char))
        return boolify(IsDBCSLeadByte((BYTE)ch));
    return false;
}


LPCTSTR CConfig::s_rgpszSubkeys[] = { REGSTR_KEY_OFFLINEFILES,
                                      REGSTR_KEY_OFFLINEFILESPOLICY };

LPCTSTR CConfig::s_rgpszValues[]  = { REGSTR_VAL_DEFCACHESIZE,
                                      REGSTR_VAL_CSCENABLED,
                                      REGSTR_VAL_GOOFFLINEACTION,
                                      REGSTR_VAL_NOCONFIGCACHE,
                                      REGSTR_VAL_NOCACHEVIEWER,
                                      REGSTR_VAL_NOMAKEAVAILABLEOFFLINE,
                                      REGSTR_VAL_SYNCATLOGOFF,
                                      REGSTR_VAL_NOREMINDERS,
                                      REGSTR_VAL_REMINDERFREQMINUTES,
                                      REGSTR_VAL_INITIALBALLOONTIMEOUTSECONDS,
                                      REGSTR_VAL_REMINDERBALLOONTIMEOUTSECONDS,
                                      REGSTR_VAL_EVENTLOGGINGLEVEL,
                                      REGSTR_VAL_PURGEATLOGOFF,
                                      REGSTR_VAL_FIRSTPINWIZARDSHOWN,
                                      REGSTR_VAL_SLOWLINKSPEED,
                                      REGSTR_VAL_ALWAYSPINSUBFOLDERS};

//
// Returns the single instance of the CConfig class.
// Note that by making the singleton instance a function static 
// object it is not created until the first call to GetSingleton.
//
CConfig& CConfig::GetSingleton(
    void
    )
{
    static CConfig TheConfig;
    return TheConfig;
}



//
// This is the workhorse of the CSCUI policy code for scalar values.  
// The caller passes in a value (iVAL_XXXXXX) identifier from the eValues 
// enumeration to identify the policy/preference value of interest.  
// Known keys in the registry are scanned until a value is found.  
// The scanning order enforces the precedence of policy vs. default vs.
// preference and machine vs. user.
//
DWORD CConfig::GetValue(
    eValues iValue,
    bool *pbSetByPolicy
    ) const
{
    //
    // This table identifies each DWORD policy/preference item used by CSCUI.  
    // The entries MUST be ordered the same as the eValues enumeration.
    // Each entry describes the possible sources for data and a default value
    // to be used if no registry entries are present or if there's a problem reading
    // the registry.
    //
    static const struct Item
    {
        DWORD fSrc;      // Mask indicating the reg locations to read.
        DWORD dwDefault; // Hard-coded default.

    } rgItems[] =  {

//  Value ID                               eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM   Default value
// --------------------------------------  ------------   ------------   -----------   ----------- -------------------
/* iVAL_DEFCACHESIZE                  */ {                                             eSRC_POL_LM, 1000             },
/* iVAL_CSCENABLED                    */ {                                             eSRC_POL_LM, 1                },  
/* iVAL_GOOFFLINEACTION               */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, eGoOfflineSilent },
/* iVAL_NOCONFIGCACHE                 */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_NOCACHEVIEWER                 */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_NOMAKEAVAILABLEOFFLINE        */ {                               eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_SYNCATLOGOFF                  */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, eSyncFull        },
/* iVAL_NOREMINDERS                   */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_REMINDERFREQMINUTES           */ { eSRC_PREF_CU |                eSRC_POL_CU | eSRC_POL_LM, 60               },
/* iVAL_INITIALBALLOONTIMEOUTSECONDS  */ {                               eSRC_POL_CU | eSRC_POL_LM, 30               },
/* iVAL_REMINDERBALLOONTIMEOUTSECONDS */ {                               eSRC_POL_CU | eSRC_POL_LM, 15               },
/* iVAL_EVENTLOGGINGLEVEL             */ { eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM, 0                },
/* iVAL_PURGEATLOGOFF                 */ {                                             eSRC_POL_LM, 0                },
/* iVAL_FIRSTPINWIZARDSHOWN           */ { eSRC_PREF_CU                                           , 0                },
/* iVAL_SLOWLINKSPEED                 */ { eSRC_PREF_CU | eSRC_PREF_LM | eSRC_POL_CU | eSRC_POL_LM, 640              },
/* iVAL_ALWAYSPINSUBFOLDERS           */ {                                             eSRC_POL_LM, 0                }
                                         };

    //
    // This table maps registry keys and subkey names to our 
    // source mask values.  The array is ordered with the highest
    // precedence sources first.  A policy level mask is also
    // associated with each entry so that we honor the "big switch"
    // for enabling/disabling CSCUI policies.
    // 
    static const struct Source
    {
        eSources    fSrc;         // Source for reg data.
        HKEY        hkeyRoot;     // Root key in registry (hkcu, hklm).
        eSubkeys    iSubkey;      // Index into s_rgpszSubkeys[]

    } rgSrcs[] = { { eSRC_POL_LM,  HKEY_LOCAL_MACHINE, iSUBKEY_POL  },
                   { eSRC_POL_CU,  HKEY_CURRENT_USER,  iSUBKEY_POL  },
                   { eSRC_PREF_CU, HKEY_CURRENT_USER,  iSUBKEY_PREF },
                   { eSRC_PREF_LM, HKEY_LOCAL_MACHINE, iSUBKEY_PREF }
                 };


    const Item& item  = rgItems[iValue];
    DWORD dwResult    = item.dwDefault;    // Set default return value.
    bool bSetByPolicy = false;

    //
    // Iterate over all of the sources until we find one that is specified
    // for this item.  For each iteration, if we're able to read the value, 
    // that's the one we return.  If not we drop down to the next source
    // in the precedence order (rgSrcs[]) and try to read it's value.  If 
    // we've tried all of the sources without a successful read we return the 
    // hard-coded default.
    //
    for (int i = 0; i < ARRAYSIZE(rgSrcs); i++)
    {
        const Source& src = rgSrcs[i];

        //
        // Is this source valid for this item?
        //
        if (0 != (src.fSrc & item.fSrc))
        {
            //
            // This source is valid for this item.  Read it.
            //
            DWORD cbResult = sizeof(dwResult);
            DWORD dwType;
    
            if (ERROR_SUCCESS == SHGetValue(src.hkeyRoot,
                                            s_rgpszSubkeys[src.iSubkey],
                                            s_rgpszValues[iValue],
                                            &dwType,
                                            &dwResult,
                                            &cbResult))
            {
                //
                // We read a value from the registry so we're done.
                //
                bSetByPolicy = (0 != (eSRC_POL & src.fSrc));
                break;
            }
        }
    }
    if (NULL != pbSetByPolicy)
        *pbSetByPolicy = bSetByPolicy;

    return dwResult;
}


//
// Save a custom GoOfflineAction list to the registry.
// See comments for LoadCustomGoOfflineActions for formatting details.
//
HRESULT 
CConfig::SaveCustomGoOfflineActions(
    RegKey& key,
    const CArray<CConfig::CustomGOA>& rgCustomGOA
    )
{
    DBGTRACE((DM_CONFIG, DL_MID, TEXT("CConfig::SaveCustomGoOfflineActions")));
    DBGPRINT((DM_CONFIG, DL_LOW, TEXT("Saving %d actions"), rgCustomGOA.Count()));
    HRESULT hr = NOERROR;
    int cValuesNotDeleted = 0;
    key.DeleteAllValues(&cValuesNotDeleted);
    if (0 != cValuesNotDeleted)
    {
        DBGERROR((TEXT("%d GoOfflineAction values not deleted from registry"), 
                  cValuesNotDeleted));
    }

    CString strServer;
    TCHAR szAction[20];
    int cGOA = rgCustomGOA.Count();
    for (int i = 0; i < cGOA; i++)
    {
        //
        // Write each sharename-action pair to the registry.
        // The action value must be converted to ASCII to be
        // compatible with the values generated by poledit.
        //
        wsprintf(szAction, TEXT("%d"), DWORD(rgCustomGOA[i].GetAction()));
        rgCustomGOA[i].GetServerName(&strServer);
        hr = key.SetValue(strServer, szAction);
        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X saving GoOfflineAction for \"%s\" to registry."), 
                     hr, strServer.Cstr()));
            break;
        }
    }
    return hr;
}


bool
CConfig::CustomGOAExists(
    const CArray<CustomGOA>& rgGOA,
    const CustomGOA& goa
    )
{
    int cEntries = rgGOA.Count();
    CString strServer;
    for (int i = 0; i < cEntries; i++)
    {
        if (0 == goa.CompareByServer(rgGOA[i]))
            return true;
    }
    return false;
}
        

//
// Builds an array of Go-offline actions.
// Each entry is a server-action pair.
//
void
CConfig::GetCustomGoOfflineActions(
    CArray<CustomGOA> *prgGOA,
    bool *pbSetByPolicy         // optional.  Can be NULL.
    )
{
    static const struct Source
    {
        eSources    fSrc;         // Source for reg data.
        HKEY        hkeyRoot;     // Root key in registry (hkcu, hklm).
        eSubkeys    iSubkey;      // Index into s_rgpszSubkeys[]

    } rgSrcs[] = { { eSRC_POL_LM,   HKEY_LOCAL_MACHINE, iSUBKEY_POL  },
                   { eSRC_POL_CU,   HKEY_CURRENT_USER,  iSUBKEY_POL  },
                   { eSRC_PREF_CU,  HKEY_CURRENT_USER,  iSUBKEY_PREF }
                 };

    prgGOA->Clear();

    CString strName;
    HRESULT hr;
    bool bSetByPolicyAny = false;
    bool bSetByPolicy    = false;

    //
    // Iterate over all of the possible sources.
    //
    for (int i = 0; i < ARRAYSIZE(rgSrcs); i++)
    {
        const Source& src = rgSrcs[i];
        //
        // Is this source valid for the current policy level?
        // Note that policy level is ignored if source is preference.
        //
        if (0 != (eSRC_PREF & src.fSrc))
        {
            RegKey key(src.hkeyRoot, s_rgpszSubkeys[src.iSubkey]);

            if (SUCCEEDED(key.Open(KEY_READ)))
            {
                RegKey keyGOA(key, REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS);
                if (SUCCEEDED(keyGOA.Open(KEY_READ)))
                {
                    TCHAR szValue[20];
                    DWORD dwType;
                    DWORD cbValue = sizeof(szValue);
                    RegKey::ValueIterator iter = keyGOA.CreateValueIterator();


                    while(S_OK == (hr = iter.Next(&strName, &dwType, (LPBYTE)szValue, &cbValue)))
                    {
                        if (REG_SZ == dwType)
                        {
                            //
                            // Convert from "0","1","2" to 0,1,2
                            //
                            DWORD dwValue = szValue[0] - TEXT('0');
                            if (IsValidGoOfflineAction(dwValue))
                            {
                                //
                                // Only add if value is of proper type and value.
                                // Protects against someone manually adding garbage
                                // to the registry.
                                //
                                // Server names can also be entered into the registry
                                // using poledit (and winnt.adm).  This entry mechanism
                                // can't validate format so we need to ensure the entry
                                // doesn't have leading '\' or space characters.
                                //
                                LPCTSTR pszServer = strName.Cstr();
                                while(*pszServer && (TEXT('\\') == *pszServer || TEXT(' ') == *pszServer))
                                    pszServer++;

                                bSetByPolicy    = (0 != (src.fSrc & eSRC_POL));
                                bSetByPolicyAny = bSetByPolicyAny || bSetByPolicy;
                                CustomGOA goa = CustomGOA(pszServer,
                                                          (CConfig::OfflineAction)dwValue,
                                                          bSetByPolicy);

                                if (!CustomGOAExists(*prgGOA, goa))
                                {
                                    prgGOA->Append(goa);
                                }
                            }
                            else
                            {
                                DBGERROR((TEXT("GoOfflineAction value %d invalid for \"%s\""),
                                          dwValue, strName.Cstr()));
                            }
                        }
                        else
                        {
                            DBGERROR((TEXT("GoOfflineAction for \"%s\" has invalid reg type %d"),
                                      strName.Cstr(), dwType));
                        }
                    }
                }
            }
        }
    }
    if (NULL != pbSetByPolicy)
        *pbSetByPolicy = bSetByPolicyAny;
}   


//
// Retrieve the go-offline action for a specific server.  If the server
// has a "customized" action defined by either system policy or user
// setting, that action is used.  Otherwise, the "default" action is
// used.
//
int
CConfig::GoOfflineAction(
    LPCTSTR pszServer
    ) const
{
    DBGTRACE((DM_CONFIG, DL_HIGH, TEXT("CConfig::GoOfflineAction(server)")));
    DBGPRINT((DM_CONFIG, DL_HIGH, TEXT("\tServer = \"%s\""), pszServer ? pszServer : TEXT("<null>")));
    int iAction = GoOfflineAction(); // Get default action.

    if (NULL == pszServer)
        return iAction;

    DBGASSERT((NULL != pszServer));

    //
    // Skip passed any leading backslashes for comparison.
    // The values we store in the registry don't have a leading "\\".
    //
    while(*pszServer && TEXT('\\') == *pszServer)
        pszServer++;

    CConfig::OfflineActionInfo info;
    CConfig::OfflineActionIter iter = CreateOfflineActionIter();
    while(iter.Next(&info))
    {
        if (0 == lstrcmpi(pszServer, info.szServer))
        {
            DBGPRINT((DM_CONFIG, DL_HIGH, TEXT("Action is %d for share \"%s\""), info.iAction, info.szServer));
            iAction = info.iAction;  // Return custom action.
            break;
        }
    }
   
    //
    // Guard against bogus reg data.
    //
    if (eNumOfflineActions <= iAction || 0 > iAction)
        iAction = eGoOfflineSilent;

    DBGPRINT((DM_CONFIG, DL_HIGH, TEXT("Action is %d (default)"), iAction));
    return iAction;
}


//-----------------------------------------------------------------------------
// CConfig::CustomGOA
// "GOA" is "Go Offline Action"
//-----------------------------------------------------------------------------
bool 
CConfig::CustomGOA::operator < (
    const CustomGOA& rhs
    ) const
{
    int diff = CompareByServer(rhs);
    if (0 == diff)
        diff = m_action - rhs.m_action;

    return diff < 0;
}


//
// Compare two CustomGoOfflineAction objects by their
// server names.  Comparison is case-insensitive.
// Returns:  <0 = *this < rhs
//            0 = *this == rhs
//           >0 = *this > rhs
//
int 
CConfig::CustomGOA::CompareByServer(
    const CustomGOA& rhs
    ) const
{
    return GetServerName().CompareNoCase(rhs.GetServerName());
}


//-----------------------------------------------------------------------------
// CConfig::OfflineActionIter
//-----------------------------------------------------------------------------
bool
CConfig::OfflineActionIter::Next(
    OfflineActionInfo *pInfo
    )
{
    bool bResult = false;
    //
    // Exception-phobic code may be calling this.
    //
    try
    {
        if (-1 == m_iAction)
        {
            m_pConfig->GetCustomGoOfflineActions(&m_rgGOA);
            m_iAction = 0;
        }
        if (m_iAction < m_rgGOA.Count())
        {
            CString s;
            m_rgGOA[m_iAction].GetServerName(&s);
            lstrcpyn(pInfo->szServer, s, ARRAYSIZE(pInfo->szServer));
            pInfo->iAction = (DWORD)m_rgGOA[m_iAction++].GetAction();
            bResult = true;
        }
    }
    catch(...)
    {

    }
    return bResult;
}




#ifdef _USE_EXT_EXCLUSION_LIST
/*
//
// I originally wrote this code assuming our UI code would want to 
// know about excluded extensions.  As it turns out, only the CSC agent
// cares about these and ShishirP has his own code for reading these
// entries.  I've left the code more as documentation for how the list
// is implemented and how one might merge multiple lists into a single
// list.  It's also been left in case the UI code does eventually need
// access to excluded extensions. [brianau - 3/15/99]
//
//
// Refresh the "excluded file extension" list from the registry.
// The resulting list is a double-nul terminated list of filename extensions.
// The lists from HKLM and HKCU are merged together to form one list.
// Duplicates are removed.
// Leading spaces and periods are removed from each entry.
// Trailing whitespace is removed from each entry.
// Embedded whitespace is preserved for each entry.
// On exit, m_ptrExclExt points to the new list or contains NULL if there is no list.
//
// i.e.:  ".PDB;  DBF  ,CPP, Html File\0" -> "PDB\0DBF\0CPP\0Html File\0\0"
//
// Note that code must be DBCS aware.
// 
void
CConfig::RefreshExclusionList(
    RegKey& keyLM,
    RegKey& keyCU
    ) const
{
    DBGTRACE((DM_CONFIG, DL_MID, TEXT("CConfig::RefreshExclusionList")));

    HRESULT hr = NOERROR;
    m_ptrExclExt = NULL;

    CArray<CString> rgExtsLM;
    CArray<CString> rgExtsCU;
    CArray<CString> rgExts;

    //
    // Get each exclusion array (LM and CU) then merge them together
    // into one (removing duplicates).
    //
    GetOneExclusionArray(keyLM, &rgExtsLM);
    GetOneExclusionArray(keyCU, &rgExtsCU);
    MergeStringArrays(rgExtsLM, rgExtsCU, &rgExts);
    //
    // Calculate how many characters are needed to create a single
    // double-nul term list of the extensions.  We could use the
    // merged array as the final holding place for the extensions
    // however that's not very efficient for storing lot's of very
    // short strings (i.e. extensions).  This way the array objects
    // are only temporary.
    //
    int i;
    int cch = 0;
    int n = rgExts.Count();
    for (i = 0; i < n; i++)
    {
        cch += rgExts[i].Length() + 1; // +1 for nul term.
    }
    if (0 < cch)
    {
        cch++; // Final nul term.
        LPTSTR s0;
        LPTSTR s = s0 = new TCHAR[cch];

        for (i = 0; i < n; i++)
        {
            lstrcpy(s, rgExts[i]);
            s += rgExts[i].Length() + 1;
        }
        *s = TEXT('\0'); // Final nul term.
        m_ptrExclExt = s0;
    }
}


//
// Advance a character pointer past any space or dot characters.
//
LPCTSTR
CConfig::SkipDotsAndSpaces(  // [ static ]
    LPCTSTR s
    )
{
    while(s && *s && (TEXT('.') == *s || TEXT(' ') == *s))
        s++;
    return s;
}



//
// Removes any duplicate extension strings from a CString array.
// This function assumes the array is sorted.
//
void
CConfig::RemoveDuplicateStrings(
    CArray<CString> *prgExt
    ) const
{
    DBGTRACE((DM_CONFIG, DL_LOW, TEXT("CConfig::RemoveDuplicateStrings")));

    CArray<CString>& rg = *prgExt;
    int n = rg.Count();
    while(0 < --n)
    {
        if (rg[n] == rg[n-1])
            rg.Delete(n);
    }
}


//
// Merge two arrays of CString objects into a single
// array containing no duplicates.  The returned array is
// in sorted order.
//
void
CConfig::MergeStringArrays(
    CArray<CString>& rgExtsA,
    CArray<CString>& rgExtsB,
    CArray<CString> *prgMerged
    ) const
{
    DBGTRACE((DM_CONFIG, DL_LOW, TEXT("CConfig::MergeStringArrays")));
    int nA = rgExtsA.Count();
    int nB = rgExtsB.Count();

    if (0 == nA || 0 == nB)
    {
        //
        // A quick optimization if one of the arrays is empty.
        // We can just copy the non-empty one.
        //
        if (0 == nA)
            *prgMerged = rgExtsB;
        else
            *prgMerged = rgExtsA;

        prgMerged->BubbleSort();
    }
    else
    {
        //
        // Both arrays have content so we must merge.
        //
        rgExtsA.BubbleSort();
        rgExtsB.BubbleSort();
        prgMerged->Clear();
        int iA = 0;
        int iB = 0;
        for (int i = 0; i < nA+nB; i++)
        {
            if (iA >= nA)
            {
                prgMerged->Append(rgExtsB[iB++]); // 'A' list exhausted.
            }
            else if (iB >= nB)
            {
                prgMerged->Append(rgExtsA[iA++]); // 'B' list exhausted.
            }
            else
            {
                int diff = rgExtsA[iA].CompareNoCase(rgExtsB[iB]);
                prgMerged->Append((0 >= diff ? rgExtsA[iA++] : rgExtsB[iB++]));
            }
        }
    }
    RemoveDuplicateStrings(prgMerged);
}


//
// Read one extension exclusion list from the registry.
// Break it up into the individual extensions, skipping any 
// leading spaces and periods.  Each extension is appended
// to an array of extension strings.
//
void
CConfig::GetOneExclusionArray(
    RegKey& key,
    CArray<CString> *prgExts
    ) const
{
    DBGTRACE((DM_CONFIG, DL_LOW, TEXT("CConfig::GetOneExclusionArray")));
    prgExts->Clear();

    if (key.IsOpen() && S_OK == key.ValueExists(REGSTR_VAL_EXTEXCLUSIONLIST))
    {
        AutoLockCs lock(m_cs);
        CString s;
        HRESULT hr = key.GetValue(REGSTR_VAL_EXTEXCLUSIONLIST, &s);
        if (SUCCEEDED(hr))
        {
            //
            // Build an array of CStrings.  One element for each of the
            // extensions in the policy value.
            //
            LPTSTR ps0;
            LPTSTR ps = ps0 = s.GetBuffer(); // No need for Release() later.
            LPTSTR psEnd = ps + s.Length();
            while(ps <= psEnd)
            {
                bool bAddThis = false;
                if (!DBCSLEADBYTE(*ps))
                {
                    if (TEXT('.') != *ps)
                    {
                        //
                        // Skip leading periods.
                        // Replace semicolons and commas with nul.
                        //
                        if (TEXT(';') == *ps || TEXT(',') == *ps)
                        {
                            *ps = TEXT('\0');
                            bAddThis = true;
                        }
                    }
                }
                if (ps == psEnd)
                {
                    //
                    // Pick up last item in list.
                    //
                    bAddThis = true;
                } 
                if (bAddThis)
                {
                    CString sAdd(SkipDotsAndSpaces(ps0));
                    sAdd.Rtrim();
                    if (0 < sAdd.Length()) // Don't add a blank string.
                        prgExts->Append(sAdd);
                    ps0 = ps + 1;
                }
                ps++;
            }
        }
        else
        {
            DBGERROR((TEXT("Error 0x%08X reading reg value \"%s\""), 
                       hr, REGSTR_VAL_EXTEXCLUSIONLIST));
        }
    }
}


//
// Determine if a particular file extension is included in the
// exclusion list.
//
bool
CConfig::ExtensionExcluded(
    LPCTSTR pszExt
    ) const
{
    bool bExcluded = false;
    ExcludedExtIter iter = CreateExcludedExtIter();
    LPCTSTR psz;
    while(iter.Next(&psz))
    {
        if (0 == lstrcmpi(psz, pszExt))
        {
            bExcluded = true;
            break;
        }
    }
    return bExcluded;
}



//-----------------------------------------------------------------------------
// CConfig::ExcludedExtIter
//-----------------------------------------------------------------------------

CConfig::ExcludedExtIter::ExcludedExtIter(
    const CConfig::ExcludedExtIter& rhs
    ) : m_pszExts(NULL)
{
    *this = rhs;
}

CConfig::ExcludedExtIter&
CConfig::ExcludedExtIter::operator = (
    const CConfig::ExcludedExtIter& rhs
    )
{
    if (&rhs != this)
    {
        delete[] m_pszExts;
        m_pszExts = NULL;
        if (NULL != rhs.m_pszExts)
            m_pszExts = CopyDblNulList(rhs.m_pszExts);
        m_iter.Attach(m_pszExts);
    }
    return *this;
}

//
// Returns length of required buffer in characters
// including the final nul terminator.
//
int
CConfig::ExcludedExtIter::DblNulListLen(
    LPCTSTR psz
    )
{
    int len = 0;
    while(psz && (*psz || *(psz+1)))
    {
        len++;
        psz++;
    }
    return psz ? len + 2 : 0;
}

//
// Copies a double-nul terminated list.  Returns address
// of new copy.  Caller must free new list using delete[].
//
LPTSTR 
CConfig::ExcludedExtIter::CopyDblNulList(
    LPCTSTR psz
    )
{
    LPTSTR s0 = NULL;
    if (NULL != psz)
    {
        int cch = DblNulListLen(psz);  // Incl final nul.
        LPTSTR s = s0 = new TCHAR[cch];
        while(0 < cch--)
            *s++ = *psz++;
    }
    return s0;
}

*/
#endif // _USE_EXT_EXCLUSION_LIST

