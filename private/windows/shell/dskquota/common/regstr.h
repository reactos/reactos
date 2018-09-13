#ifndef _INC_DSKQUOTA_REGSTR_H
#define _INC_DSKQUOTA_REGSTR_H

//
// Registry strings associated with disk quota policy.
//
#define REGSTR_KEY_POLICYDATA   TEXT("Software\\Policies\\Microsoft\\Windows NT\\DiskQuota")
#ifdef POLICY_MMC_SNAPIN
    #define REGSTR_VAL_POLICYDATA   TEXT("PolicyData")
#endif
#define REGSTR_VAL_POLICY_ENABLE            TEXT("Enable")
#define REGSTR_VAL_POLICY_ENFORCE           TEXT("Enforce")
#define REGSTR_VAL_POLICY_LIMIT             TEXT("Limit")
#define REGSTR_VAL_POLICY_THRESHOLD         TEXT("Threshold")
#define REGSTR_VAL_POLICY_LIMITUNITS        TEXT("LimitUnits")
#define REGSTR_VAL_POLICY_THRESHOLDUNITS    TEXT("ThresholdUnits")
#define REGSTR_VAL_POLICY_REMOVABLEMEDIA    TEXT("ApplyToRemovableMedia")
#define REGSTR_VAL_POLICY_LOGLIMIT          TEXT("LogEventOverLimit")
#define REGSTR_VAL_POLICY_LOGTHRESHOLD      TEXT("LogEventOverThreshold")

//
// This is the subkey we store data under in the registry (HKCU).
// If you want to change the location in HKCU, this is all you have to change.
//
#define REGSTR_KEY_DISKQUOTA    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\DiskQuota")
#define REGSTR_VAL_PREFERENCES  TEXT("Preferences")
#define REGSTR_VAL_FINDMRU      TEXT("FindMRU")
#define REGSTR_VAL_DEBUGPARAMS  TEXT("DebugParams")

//
// Shell extension registry keys.
//
#define REGSTR_KEY_APPROVEDSHELLEXT TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved")
#define REGSTR_KEY_DRIVEPROPSHEETS  TEXT("Drive\\shellex\\PropertySheetHandlers")

#endif
