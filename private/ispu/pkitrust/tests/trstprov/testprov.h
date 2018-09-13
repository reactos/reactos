//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       testprov.h
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  History:    25-Jul-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef TESTPROV_H
#define TESTPROV_H

#ifdef __cplusplus
extern "C" 
{
#endif

#include    "wtoride.h"

// Test Trust Provider: {684D31F8-DDBA-11d0-8CCB-00C04FC295EE}
//
#define TESTPROV_ACTION_TEST                                    \
            { 0x684d31f8,                                       \
              0xddba,                                           \
              0x11d0,                                           \
              { 0x8c, 0xcb, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } \
            }


//////////////////////////////////////////////////////////////////////////////
//
// TESTPROV Policy Provider defines
//----------------------------------------------------------------------------
//  The following are definitions of the Microsoft Test Policy Provider
//  (TESTPROV.DLL's Policy Provider)
//  

#define TP_DLL_NAME                         L"TPROV1.DLL"

#define TP_INIT_FUNCTION                    L"TestprovInitialize"
#define TP_OBJTRUST_FUNCTION                L"TestprovObjectProv"
#define TP_SIGTRUST_FUNCTION                L"TestprovSigProv"
#define TP_CHKCERT_FUNCTION                 L"TestprovCheckCertProv"
#define TP_FINALPOLICY_FUNCTION             L"TestprovFinalProv"
#define TP_TESTDUMPPOLICY_FUNCTION_TEST     L"TestprovTester"
#define TP_CLEANUPPOLICY_FUNCTION           L"TestprovCleanup"

//////////////////////////////////////////////////////////////////////////////
//
// TESTPROV_PRIVATE_DATE
//----------------------------------------------------------------------------
//  This structure defines TESTPROV.DLL's private data that is stored
//  in the CRYPT_PROVIDER_PRIVDATA array.
//  

typedef struct _TESTPROV_PRIVATE_DATA
{
    DWORD                   cbStruct;

    CRYPT_PROVIDER_FUNCTIONS_ORMORE    sAuthenticodePfns;
    CRYPT_PROVIDER_FUNCTIONS_ORLESS    sAuthenticodePfns_less;

} TESTPROV_PRIVATE_DATA, *PTESTPROV_PRIVATE_DATA;


#ifdef __cplusplus
}
#endif

#endif // TESTPROV_H
