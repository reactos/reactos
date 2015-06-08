////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

//#define EVALUATION_TIME_LIMIT

#ifdef EVALUATION_TIME_LIMIT

#define VAR_SAVED(preff, n) Var##preff##n##Saved

#define KEY(preff, n)   preff##n##Key
#define VAL(preff, n)   preff##n##Val

#define CheckAndSave(preff, n, val, type) \
    if (!VAR_SAVED(##preff, ##n)) { \
    if (##type==1)           \
     RegisterDword(KEY(##preff,##n),VAL(##preff,##n),##val ^ XOR_VAR(##preff, ##n) );  \
     else {          \
     szTemp[16];  \
     sprintf(szTemp,"0x%08x", ##val ^ XOR_VAR(##preff, ##n));   \
     RegisterString(KEY(##preff,##n),VAL(##preff,##n), &szTemp[0], FALSE, 0);   \
    }\
        VAR_SAVED(##preff, ##n) = TRUE; \
    }

#define RegisterTrialEnd() \
    CheckAndSave(TrialEnd, 0, 1, 1); \
    CheckAndSave(TrialEnd, 1, 1, 1);

#define ClearTrialEnd() \
    CheckAndSave(TrialEnd, 0, 0, 1); \
    CheckAndSave(TrialEnd, 1, 0, 1);

#define RegisterDate() \
    CheckAndSave(Date, 0, dwDaysSince2003, 0); \
    CheckAndSave(Date, 1, dwDaysSince2003, 0);

#define RegisterVersion() \
    CheckAndSave(Version, 0, UDF_CURRENT_BUILD, 0); \
    CheckAndSave(Version, 1, UDF_CURRENT_BUILD, 1);

#define PATH_VAR_DECL(preff, n) \
CHAR   KEY(preff, n)[256], VAL(preff, n)[64]; \
BOOL   VAR_SAVED(preff, n) = FALSE;


#define GET_KEY_DATE(n)     GET_DATE_REG_KEY_NAME(Date##n##Key, ##n)
#define GET_VAL_DATE(n)     GET_DATE_REG_VAL_NAME(Date##n##Val, ##n)

#define GET_KEY_VERSION(n)  GET_VERSION_REG_KEY_NAME(Version##n##Key, ##n)
#define GET_VAL_VERSION(n)  GET_VERSION_REG_VAL_NAME(Version##n##Val, ##n)

#define GET_KEY_TRIAL(n)    GET_TRIAL_REG_KEY_NAME(TrialEnd##n##Key, ##n)
#define GET_VAL_TRIAL(n)    GET_TRIAL_REG_VAL_NAME(TrialEnd##n##Val, ##n)

        CHAR            szTemp[16];
        ULONG           dwDaysSince2003, dwDaysSince2003fromReg0 = 0, dwDaysSince2003fromReg1 = 0;
        ULONG           TrialEnd0, TrialEnd1;
        ULONG           Version0 = 0, Version1 = 0;
        SYSTEMTIME      SystemTime, SystemTime2003;
        FILETIME        FileTime,   FileTime2003;
        ULARGE_INTEGER  Time,       Time2003;


#define INCLUDE_XOR_DECL_ONLY
#include "protect.h"
#undef INCLUDE_XOR_DECL_ONLY

        PATH_VAR_DECL(Date,      0);
        PATH_VAR_DECL(Date,      1);
        PATH_VAR_DECL(Version,   0);
        PATH_VAR_DECL(Version,   1);
        PATH_VAR_DECL(TrialEnd,  0);
        PATH_VAR_DECL(TrialEnd,  1);

        GET_KEY_TRIAL(0);
        GET_VAL_TRIAL(1);

#ifndef NO_KEY_PRESENCE_CHECK
        CHAR            Key[17];
        if (GetRegString(UDF_SERVICE_PATH,UDF_LICENSE_KEY_USER,Key, sizeof(Key))) {
            goto LicenseKeyPresent;
        }
#endif // NO_KEY_PRESENCE_CHECK

        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &FileTime);
        memset(&SystemTime2003, 0, sizeof(SystemTime2003));
        SystemTime2003.wYear    = 2003;

        GET_KEY_DATE(0);
        GET_VAL_DATE(0);
        GET_KEY_VERSION(0);
        
        SystemTime2003.wMonth   = 1;
        SystemTime2003.wDay     = 1;

        GET_VAL_VERSION(1);

        SystemTimeToFileTime(&SystemTime2003, &FileTime2003);

        memcpy(&Time    , &FileTime,     sizeof(ULARGE_INTEGER));
        memcpy(&Time2003, &FileTime2003, sizeof(ULARGE_INTEGER));
        Time.QuadPart -= Time2003.QuadPart;

        GET_KEY_TRIAL(1);
        GET_VAL_TRIAL(0);
        
        dwDaysSince2003 = (ULONG)(Time.QuadPart/10000000I64/3600/24);

        GET_KEY_DATE(1);
        GET_VAL_VERSION(0);
        GET_KEY_VERSION(1);

        if (GetRegString(KEY(Version, 0), VAL(Version, 0),&szTemp[0], sizeof(szTemp))) {
            sscanf(szTemp,"0x%08x", &Version0);
            Version0 ^= XOR_VAR(Version,0);
        } 

        GET_VAL_DATE(1);

        if (GetRegUlong(KEY(Version, 1), VAL(Version, 1),&Version1)) {
            Version1 ^= XOR_VAR(Version,1);
        } 

        if (Version0 < UDF_CURRENT_BUILD && Version1 < UDF_CURRENT_BUILD) {
            RegisterVersion();
            ClearTrialEnd();
            RegisterDate();
            return 1;
        }

        if ((LONGLONG)(Time.QuadPart) < 0 ||
            GetRegUlong(KEY(TrialEnd, 0), VAL(TrialEnd, 0),&TrialEnd0) && (TrialEnd0 ^ XOR_VAR(TrialEnd, 0)) != 0 ||
            GetRegUlong(KEY(TrialEnd, 1), VAL(TrialEnd, 1),&TrialEnd1) && (TrialEnd1 ^ XOR_VAR(TrialEnd, 1)) != 0) {
            RegisterTrialEnd();
#ifndef NO_MESSAGE_BOXES
            if (MyMessageBox(hInst, 
                             GetDesktopWindow(),
                             MAKEINTRESOURCE(IDS_EXPIRED_TEXT),
                             MAKEINTRESOURCE(IDS_EXPIRED), MB_YESNO | MB_ICONQUESTION) == IDYES) goto EnterRegistration;

#endif  // NO_MESSAGE_BOXES
            return 0;
        }
                
        if (GetRegString(KEY(Date, 0), VAL(Date, 0),&szTemp[0], sizeof(szTemp))) {
            sscanf(szTemp,"0x%08x", &dwDaysSince2003fromReg0);
            dwDaysSince2003fromReg0 ^= XOR_VAR(Date,0);
        } 

        if (GetRegString(KEY(Date, 1), VAL(Date, 1),&szTemp[0], sizeof(szTemp))) {
            sscanf(szTemp,"0x%08x", &dwDaysSince2003fromReg1);
            dwDaysSince2003fromReg1 ^= XOR_VAR(Date,1);
        } 
        
        if (dwDaysSince2003fromReg1 > dwDaysSince2003fromReg0) dwDaysSince2003fromReg0 = dwDaysSince2003fromReg1;
        
        if (!dwDaysSince2003fromReg0 && !dwDaysSince2003fromReg1) {
            RegisterDate();
            dwDaysSince2003fromReg0 = dwDaysSince2003;
        } else if (!dwDaysSince2003fromReg0) {
            CheckAndSave(Date, 0, dwDaysSince2003fromReg1, 0);
            dwDaysSince2003fromReg0 = dwDaysSince2003fromReg1;
        } else if (!dwDaysSince2003fromReg1) {
            CheckAndSave(Date, 1, dwDaysSince2003fromReg0, 0);
        }

        if(dwDaysSince2003 - dwDaysSince2003fromReg0 > EVALUATION_TERM || dwDaysSince2003 < dwDaysSince2003fromReg0 ||
           dwDaysSince2003 > UDF_MAX_DATE ||
           dwDaysSince2003 < UDF_MIN_DATE) {
            RegisterTrialEnd();
#ifndef NO_MESSAGE_BOXES
            if (MyMessageBox(hInst, 
                             GetDesktopWindow(),
                             MAKEINTRESOURCE(IDS_EXPIRED_TEXT),
                             MAKEINTRESOURCE(IDS_EXPIRED), MB_YESNO | MB_ICONQUESTION) == IDYES) goto EnterRegistration;

#endif  // NO_MESSAGE_BOXES
            return 0;
        }

#endif EVALUATION_TIME_LIMIT
