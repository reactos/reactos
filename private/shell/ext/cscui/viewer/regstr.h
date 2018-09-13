//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       regstr.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_REGSTR_H
#define _INC_CSCVIEW_REGSTR_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

//
// The following table lists all of the registry parameters associated with CSC.
// Parameters can be broken into two groups.  
//    a. Operational values
//    b. Restrictions
//
// Operational values provide operational control for CSC.  Values may exist as
// system policy (per-user or per-machine) or they may be user-configured.
// The policy value serves as the default with HKLM taking precedence.
// If there is no corresponding restriction and a user-defined value exists, it is
// used in place of the policy value.  If there is a restriction or if only the policy
// value exists, the policy value is used.  In the case where there is no policy value
// or no user-defined value, a hard-coded default is used.
//
// Restrictions are policy-rules preventing users from performing some action.
// In general, this means controlling the ability for users to change an operational 
// value.  Restrictions are only present under the CSC "policy" registry key.  All of 
// the restriction values are prefixed with "No".  If a restriction value is not present,
// it is assumed there is no restriction.
//
//
//                                - User pref-    -- Policy --
//  Parameter Name                HKCU    HKLM    HKCU    HKLM    Values
//  ----------------------------- ----    ----    ----    ------  --------------------------------------
//  CustomGoOfflineActions        X               X       X       ShareName-OfflineAction pairs.
//  DefCacheSize                                          X       (Pct disk * 10000) 5025 = 50.25%
//  Enabled                                               X       0 = Disabled,1 = Enabled
//  ExtExclusionList                              X       X       List of semicolon-delimited file exts.
//  GoOfflineAction               X               X       X       0 = Silent, 1 = Fail
//  NoConfigCache                                 X       X       0 = No restriction, 1 = restricted
//  NoCacheViewer                                 X       X       0 = No restriction, 1 = restricted
//  NoMakeAvailableOffline                        X       X       0 = No restriction, 1 = restricted
//  SyncAtLogoff                  X               X       X       0 = Partial (quick), 1 = Full
//  NoReminders                   X               X       X       0 = Show reminders.
//  NoConfigReminders                             X       X       0 = No restriction. 1 = restricted.
//  ReminderFreqMinutes           X               X       X       Frequency of reminder balloons in min.
//  InitialBalloonTimeoutSeconds  X               X       X       Seconds before initial balloon auto-pops.
//  ReminderBalloonTimeoutSeconds X               X       X       Seconds before reminder balloon auto-pops.
//  EventLoggingLevel                     X               X       0 = No logging, (1) minimal -> (3) verbose.
//  PurgeAtLogoff                                 X       X       1 = Purge, 0 = Don't purge users's files
//  AlwaysPinSubFolders                                   X       1 = Always recursively pin.
//

const TCHAR REGSTR_KEY_OFFLINEFILESPOLICY[]            = TEXT("Software\\Policies\\Microsoft\\Windows\\NetCache");
const TCHAR REGSTR_KEY_OFFLINEFILES[]                  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache");
const TCHAR REGSTR_SUBKEY_CUSTOMGOOFFLINEACTIONS[]     = TEXT("CustomGoOfflineActions");
const TCHAR REGSTR_VAL_DEFCACHESIZE[]                  = TEXT("DefCacheSize");
const TCHAR REGSTR_VAL_CSCENABLED[]                    = TEXT("Enabled");
const TCHAR REGSTR_VAL_EXTEXCLUSIONLIST[]              = TEXT("ExcludedExtensions");
const TCHAR REGSTR_VAL_GOOFFLINEACTION[]               = TEXT("GoOfflineAction");
const TCHAR REGSTR_VAL_NOCONFIGCACHE[]                 = TEXT("NoConfigCache");
const TCHAR REGSTR_VAL_NOCACHEVIEWER[]                 = TEXT("NoCacheViewer");
const TCHAR REGSTR_VAL_NOMAKEAVAILABLEOFFLINE[]        = TEXT("NoMakeAvailableOffline");
const TCHAR REGSTR_VAL_SYNCATLOGOFF[]                  = TEXT("SyncAtLogoff");
const TCHAR REGSTR_VAL_NOREMINDERS[]                   = TEXT("NoReminders");
const TCHAR REGSTR_VAL_NOCONFIGREMINDERS[]             = TEXT("NoConfigReminders");
const TCHAR REGSTR_VAL_REMINDERFREQMINUTES[]           = TEXT("ReminderFreqMinutes");
const TCHAR REGSTR_VAL_INITIALBALLOONTIMEOUTSECONDS[]  = TEXT("InitialBalloonTimeoutSeconds");
const TCHAR REGSTR_VAL_REMINDERBALLOONTIMEOUTSECONDS[] = TEXT("ReminderBalloonTimeoutSeconds");
const TCHAR REGSTR_VAL_FIRSTPINWIZARDSHOWN[]           = TEXT("FirstPinWizardShown");
const TCHAR REGSTR_VAL_EXPANDSTATUSDLG[]               = TEXT("ExpandStatusDlg");
const TCHAR REGSTR_VAL_FORMATCSCDB[]                   = TEXT("FormatDatabase");
const TCHAR REGSTR_VAL_EVENTLOGGINGLEVEL[]             = TEXT("EventLoggingLevel");
const TCHAR REGSTR_VAL_PURGEATLOGOFF[]                 = TEXT("PurgeAtLogoff");
const TCHAR REGSTR_VAL_SLOWLINKSPEED[]                 = TEXT("SlowLinkSpeed");
const TCHAR REGSTR_VAL_ALWAYSPINSUBFOLDERS[]           = TEXT("AlwaysPinSubFolders");
const TCHAR REGSTR_VAL_WINDOWSTATE_MAIN[]              = TEXT("0");
const TCHAR REGSTR_VAL_VIEWSTATE_SHARE[]               = TEXT("1");
const TCHAR REGSTR_VAL_VIEWSTATE_DETAILS[]             = TEXT("2");
const TCHAR REGSTR_VAL_VIEWSTATE_STALE[]               = TEXT("3");


#endif // _INC_CSCVIEW_REGSTR_H

