//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       config.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CONFIG_H
#define _INC_CSCVIEW_CONFIG_H

#ifndef _INC_CSCVIEW_REGISTRY_H
#   include "registry.h"
#endif
#ifndef _INC_CSCVIEW_REGSTR_H
#   include "regstr.h"
#endif
#ifndef _INC_CSCVIEW_UTILS_H
#   include "utils.h"
#endif
#ifndef _INC_CSCVIEW_THDSYNC_H
#   include "thdsync.h"
#endif

#ifdef _USE_EXT_EXCLUSION_LIST
/*
#ifndef _INC_CSCVIEW_AUTOPTR_H
#   include "autoptr.h"
#endif
*/
#endif // _USE_EXT_EXCLUSION_LIST


class CConfig
{
    public:
        ~CConfig(void) { }

        enum SyncAction 
        { 
            eSyncPartial = 0,    // Sync only transient files at logoff.
            eSyncFull,           // Sync all files at logoff.
            eNumSyncActions
        };

        enum OfflineAction 
        { 
            //
            // These MUST match the order of the IDS_GOOFFLINE_ACTION_XXXXX
            // string resource IDs.
            //
            eGoOfflineSilent = 0, // Silently transition share to offline mode.
            eGoOfflineFail,       // Fail the share (NT4 behavior).
            eNumOfflineActions
        };

        //
        // Represents one custom go-offline action.
        //
        struct OfflineActionInfo
        {
            TCHAR szServer[MAX_PATH];   // Name of the associated server.
            int iAction;                // Action code.  One of enum OfflineAction.
        };

        //
        // Represents one entry in the customized server list.
        // "GOA" is "GoOfflineAction".
        //
        class CustomGOA
        {
            public:
                CustomGOA(void)
                    : m_action(eGoOfflineSilent),
                      m_bSetByPolicy(false) { }

                CustomGOA(LPCTSTR pszServer, OfflineAction action, bool bSetByPolicy)
                    : m_strServer(pszServer),
                      m_action(action),
                      m_bSetByPolicy(bSetByPolicy) { }

                bool operator == (const CustomGOA& rhs) const
                    { return (m_action == rhs.m_action &&
                              0 == CompareByServer(rhs)); }

                bool operator != (const CustomGOA& rhs) const
                    { return !(*this == rhs); } 

                bool operator < (const CustomGOA& rhs) const;

                int CompareByServer(const CustomGOA& rhs) const;

                void SetServerName(LPCTSTR pszServer)
                    { m_strServer = pszServer; }

                void SetAction(OfflineAction action)
                    { m_action = action; }

                void GetServerName(CString *pstrServer) const
                    { *pstrServer = m_strServer; }

                const CString& GetServerName(void) const
                    { return m_strServer; }

                OfflineAction GetAction(void) const
                    { return m_action; }

                bool SetByPolicy(void) const
                    { return m_bSetByPolicy; }

            private:
                CString       m_strServer;      // The name of the server.
                OfflineAction m_action;         // The action code.
                bool          m_bSetByPolicy;   // Was action set by policy?
        };

        //
        // Iterator for enumerating custom go-offline actions.
        //
        class OfflineActionIter
        {
            public:
                OfflineActionIter(const CConfig *pConfig = NULL)
                    : m_pConfig(const_cast<CConfig *>(pConfig)),
                      m_iAction(-1) { }

                bool Next(OfflineActionInfo *pInfo);

                void Reset(void)
                    { m_iAction = 0; }

            private:
                CConfig         *m_pConfig;
                CArray<CustomGOA> m_rgGOA;
                int               m_iAction;
        };

#ifdef _USE_EXT_EXCLUSION_LIST
/*
        //
        // Subclass to encapsulate the list of filename extensions excluded
        // from automatic caching.  The semicolon-delmited list of extensions
        // is loaded from the registry by CPolicy and transformed to a 
        // CPolicy::ExcludedExtIter object through the
        // CPolicy::CreateExcludedExtIter() member.  The client accesses
        // each of the extensions using ExcludedExtIter::Next().
        //
        class ExcludedExtIter
        {
            public:
                ExcludedExtIter(void)
                    : m_pszExts(NULL) { }

                ExcludedExtIter(LPCTSTR pszExts)
                    : m_pszExts(CopyDblNulList(pszExts)),
                      m_iter(m_pszExts) { }

                ExcludedExtIter(const ExcludedExtIter& rhs);
                ExcludedExtIter& operator = (const ExcludedExtIter& rhs);

                ~ExcludedExtIter(void) { delete[] m_pszExts; }

                bool Next(LPCTSTR *ppszExt)
                    { return m_iter.Next(ppszExt); }

                void Reset(void)
                    { m_iter.Reset(); }

            private:
                LPTSTR              m_pszExts; // The extensions.
                DblNulTermListIter  m_iter;    // The extention iterator.

                LPTSTR CopyDblNulList(LPCTSTR psz);
                int DblNulListLen(LPCTSTR psz);
        };
*/
#endif // _USE_EXT_EXCLUSION_LIST

        static CConfig& GetSingleton(void);

        bool CscEnabled(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_CSCENABLED, pbSetByPolicy)); }

        DWORD DefaultCacheSize(bool *pbSetByPolicy = NULL) const
            { return GetValue(iVAL_DEFCACHESIZE, pbSetByPolicy); }

        int EventLoggingLevel(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_EVENTLOGGINGLEVEL, pbSetByPolicy)); }

        bool FirstPinWizardShown(void) const
            { return boolify(GetValue(iVAL_FIRSTPINWIZARDSHOWN)); }

        void GetCustomGoOfflineActions(CArray<CustomGOA> *prgGOA, bool *pbSetByPolicy = NULL);

        int GoOfflineAction(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_GOOFFLINEACTION, pbSetByPolicy)); }

        int GoOfflineAction(LPCTSTR pszServer) const;

        int InitialBalloonTimeoutSeconds(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_INITIALBALLOONTIMEOUTSECONDS, pbSetByPolicy)); }

        bool NoCacheViewer(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOCACHEVIEWER, pbSetByPolicy)); }

        bool NoConfigCache(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOCONFIGCACHE, pbSetByPolicy)); }

        bool NoMakeAvailableOffline(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOMAKEAVAILABLEOFFLINE, pbSetByPolicy)); }

        bool NoReminders(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_NOREMINDERS, pbSetByPolicy)); }

        bool PurgeAtLogoff(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_PURGEATLOGOFF, pbSetByPolicy)); }

        bool AlwaysPinSubFolders(bool *pbSetByPolicy = NULL) const
            { return boolify(GetValue(iVAL_ALWAYSPINSUBFOLDERS, pbSetByPolicy)); }

        int ReminderBalloonTimeoutSeconds(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_REMINDERBALLOONTIMEOUTSECONDS, pbSetByPolicy)); }

        int ReminderFreqMinutes(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_REMINDERFREQMINUTES, pbSetByPolicy)); }

        int SyncAtLogoff(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SYNCATLOGOFF, pbSetByPolicy)); }

        int SlowLinkSpeed(bool *pbSetByPolicy = NULL) const
            { return int(GetValue(iVAL_SLOWLINKSPEED, pbSetByPolicy)); }

        OfflineActionIter CreateOfflineActionIter(void) const
            { return OfflineActionIter(this); }

        static HRESULT SaveCustomGoOfflineActions(RegKey& key, const CArray<CustomGOA>& rgCustomGOA);

#ifdef _USE_EXT_EXCLUSION_LIST
/*
        bool ExtensionExcluded(LPCTSTR pszExt) const;

        ExcludedExtIter CreateExcludedExtIter(void) const
            { AutoLockCs lock(m_cs); return ExcludedExtIter(m_ptrExclExt.get()); }
*/
#endif // _USE_EXT_EXCLUSION_LIST


    private:
        //
        // Indexes into s_rgpszSubkeys[].
        //
        enum eSubkeys 
        { 
            iSUBKEY_PREF,
            iSUBKEY_POL,
            MAX_SUBKEYS 
        };
        //
        // Indexes into s_rgpszValues[].
        //
        enum eValues 
        { 
            iVAL_DEFCACHESIZE,
            iVAL_CSCENABLED,
            iVAL_GOOFFLINEACTION,
            iVAL_NOCONFIGCACHE,
            iVAL_NOCACHEVIEWER,
            iVAL_NOMAKEAVAILABLEOFFLINE,
            iVAL_SYNCATLOGOFF,
            iVAL_NOREMINDERS,
            iVAL_REMINDERFREQMINUTES,
            iVAL_INITIALBALLOONTIMEOUTSECONDS,
            iVAL_REMINDERBALLOONTIMEOUTSECONDS,
            iVAL_EVENTLOGGINGLEVEL,
            iVAL_PURGEATLOGOFF,
            iVAL_FIRSTPINWIZARDSHOWN,
            iVAL_SLOWLINKSPEED,
            iVAL_ALWAYSPINSUBFOLDERS,
            MAX_VALUES 
        };
        //
        // Mask to specify source of a config value.
        //
        enum eSources 
        { 
            eSRC_PREF_CU = 0x00000001,
            eSRC_PREF_LM = 0x00000002,
            eSRC_POL_CU  = 0x00000004,
            eSRC_POL_LM  = 0x00000008,
            eSRC_POL     = eSRC_POL_LM  | eSRC_POL_CU,
            eSRC_PREF    = eSRC_PREF_LM | eSRC_PREF_CU 
        };

        static LPCTSTR s_rgpszSubkeys[MAX_SUBKEYS];
        static LPCTSTR s_rgpszValues[MAX_VALUES];
        static CCriticalSection s_csMigration;

        template <class T>
        bool boolify(const T& x) const
            { return !!x; }

        DWORD GetValue(eValues iValue, bool *pbSetByPolicy = NULL) const;

        bool CustomGOAExists(const CArray<CustomGOA>& rgGOA, const CustomGOA& goa);

        static bool IsValidGoOfflineAction(DWORD dwAction)
            { return ((OfflineAction)dwAction == eGoOfflineSilent ||
                      (OfflineAction)dwAction == eGoOfflineFail); }

        static bool IsValidSyncAction(DWORD dwAction)
            { return ((SyncAction)dwAction == eSyncPartial ||
                      (SyncAction)dwAction == eSyncFull); }

        //
        // Enforce use of GetSingleton() for instantiation.
        //
        CConfig(void) { }
        //
        // Prevent copy.
        //
        CConfig(const CConfig& rhs);
        CConfig& operator = (const CConfig& rhs);


#ifdef _USE_EXT_EXCLUSION_LIST
/*
        mutable array_autoptr<TCHAR> m_ptrExclExt;
        mutable CCriticalSection m_cs;

        void RefreshExclusionList(RegKey& keyLM, RegKey& keyCU) const;
        void GetOneExclusionArray(RegKey& key, CArray<CString> *prgExts) const;
        void RemoveDuplicateStrings(CArray<CString> *prgExts) const;
        void MergeStringArrays(CArray<CString>& rgExtsA,
                               CArray<CString>& rgExtsB,
                               CArray<CString> *prgMerged) const;
        static LPCTSTR SkipDotsAndSpaces(LPCTSTR s);
*/
#endif // _USE_EXT_EXCLUSION_LIST


};


#endif // _INC_CSCVIEW_CONFIG_H
