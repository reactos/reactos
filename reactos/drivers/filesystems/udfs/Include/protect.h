////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __UDF_PROTECT__H__
#define __UDF_PROTECT__H__

//#include "udf_eval_time.h"
//#include "product.h"

//#define EVALUATION_TIME_LIMIT !!! // this must be defined in compiler options

#define TIME_JAN_1_2003      0x23d8b
#define EVALUATION_TERM      30
#define EVALUATION_TERM_XOR  0x75fd63c8
#define EVALUATION_TERM_DIE  ((-1) ^ EVALUATION_TERM_XOR)

//#define UDF_CURRENT_VERSION  0x00015002
//                           xx.1.50.02
//#define UDF_CURRENT_BUILD    110

#define REG_TRIAL_DATE1_KEY_NAME  L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define REG_TRIAL_DATE1_VAL_NAME  L"AlwaysOnTop"

#define REG_TRIAL_DATE2_KEY_NAME  L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"
#define REG_TRIAL_DATE2_VAL_NAME  L"AlwaysOnTop"

#define REG_TRIAL_DATE3_KEY_NAME_A  "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Compatibility32"
#define REG_TRIAL_DATE3_KEY_NAME_W  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Compatibility32"
#define REG_TRIAL_DATE3_VAL_NAME_A  "DWGUIFMTUDF32"
#define REG_TRIAL_DATE3_VAL_NAME_W  L"DWGUIFMTUDF32"

// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Compatibility, RUN.EXE               Date
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ModuleCompatibility, EXPLORER.EXE    Date
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\WOW\Compatibility, CMD32.EXE         Version
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion, InstallTime                         Version
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer, SmallIcons                    TrialEnd
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Nls\LocaleMapIDs, CurrentLocaleID       TrialEnd
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved, 62ad7960-e634-11d8-b031-00024451f90c not used now

/*
#define XOR_STR_DECL(n, val) \
    ULONG  UdfXorStr##n = val;
*/

#define UDF_INSTALL_INFO_LOCATIONS  2

#define XOR_VAR(suff, n) UdfXorVal##suff##n
#define XOR_VAR_DECL(suff, n, val) \
    ULONG  XOR_VAR(suff, n) = val;
#define XOR_VAR_DECLH(suff, n) \
    extern ULONG  XOR_VAR(suff, n);

extern ULONG PresentDateMask;
extern BOOLEAN TrialEndOnStart;

XOR_VAR_DECLH(Date,      0);
XOR_VAR_DECLH(Date,      1);
XOR_VAR_DECLH(Version,   0);
XOR_VAR_DECLH(Version,   1);
XOR_VAR_DECLH(TrialEnd,  0);
XOR_VAR_DECLH(TrialEnd,  1);
//XOR_VAR_DECL(Date, 0, 0xff7765cc);
//XOR_VAR_DECL(Date, 1, 0xf76caa82);

//#include "protect_reg.h"


#define GET_XXX_REG_VAL_TYPE(str, n)   (REG_##str##_TYPE_##n)
#define GET_XXX_REG_XOR(xxx, n)        (REG_##xxx##_XOR_##n)

//#define GET_DATE_REG_XOR(n)    (UdfXorVal##Date##n)

// str pointer to the buffer to extract name to
#define GET_XXX_REG_YYY_NAME(xxx, yyy, _str, n) /* Kernel */ \
{ \
    ULONG i; \
    PCHAR str = (PCHAR)(_str); \
    PCHAR tmp = REG_##xxx##_##yyy##_NAME_##n; \
    for(i=0; i< sizeof(REG_##xxx##_##yyy##_NAME_##n); i++) { \
        str[i] = tmp[i] ^ (UCHAR)GET_##xxx##_REG_XOR(n) ; \
    } \
}

#define GET_DATE_REG_XOR(n)               GET_XXX_REG_XOR(DATE, n)
// str pointer to the buffer to extract name to
#define GET_DATE_REG_KEY_NAME(_str, n)    GET_XXX_REG_YYY_NAME(DATE, KEY, _str, n)
#define GET_DATE_REG_VAL_NAME(_str, n)    GET_XXX_REG_YYY_NAME(DATE, VAL, _str, n)


#define GET_VERSION_REG_XOR(n)            GET_XXX_REG_XOR(VERSION, n)
//#define GET_VERSION_REG_XOR(n)    (UdfXorVal##Version##n)
#define GET_VERSION_REG_KEY_NAME(_str, n) GET_XXX_REG_YYY_NAME(VERSION, KEY, _str, n)
#define GET_VERSION_REG_VAL_NAME(_str, n) GET_XXX_REG_YYY_NAME(VERSION, VAL, _str, n)

#define GET_TRIAL_REG_XOR(n)              GET_XXX_REG_XOR(TRIAL, n)
//#define GET_TRIAL_REG_XOR(n)    (UdfXorVal##Trial##n)
#define GET_TRIAL_REG_KEY_NAME(_str, n)   GET_XXX_REG_YYY_NAME(TRIAL, KEY, _str, n)
#define GET_TRIAL_REG_VAL_NAME(_str, n)   GET_XXX_REG_YYY_NAME(TRIAL, VAL, _str, n)

//////////////

#define UDF_LICENSE_KEY        L"LicenseKey"
#define UDF_LICENSE_KEY_USER    "LicenseKey"

#define UDF_USER_KEY           L"UserName"
#define UDF_USER_KEY_USER       "UserName"

#endif // __UDF_PROTECT__H__

#ifdef INCLUDE_XOR_DECL_ONLY
        XOR_VAR_DECL(Date,      0, 0xf826fab2);
        XOR_VAR_DECL(Date,      1, 0x12fcb245);
        XOR_VAR_DECL(Version,   0, 0x8c36acf3);
        XOR_VAR_DECL(Version,   1, 0x9437cfa4);
        XOR_VAR_DECL(TrialEnd,  0, 0xfc9387a6);
        XOR_VAR_DECL(TrialEnd,  1, 0x287cfbde);
#endif // INCLUDE_XOR_DECL_ONLY
