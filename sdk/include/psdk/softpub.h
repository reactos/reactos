/*
 * Copyright (C) 2006 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SOFTPUB_H
#define __WINE_SOFTPUB_H

#include <wintrust.h>

#define WINTRUST_ACTION_GENERIC_CERT_VERIFY \
    { 0x189a3842, 0x3041, 0x11d1, { 0x85,0xe1,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define SP_GENERIC_CERT_INIT_FUNCTION (const WCHAR []) \
    {'S','o','f','t','p','u','b','D','e','f','C','e','r','t','I','n','i','t', 0}
#elif defined(_MSC_VER)
#define SP_GENERIC_CERT_INIT_FUNCTION L"SoftpubDefCertInit"
#else
static const WCHAR SP_GENERIC_CERT_INIT_FUNCTION[] =
    {'S','o','f','t','p','u','b','D','e','f','C','e','r','t','I','n','i','t', 0};
#endif

#define WINTRUST_ACTION_GENERIC_CHAIN_VERIFY \
    { 0xfc451c16, 0xac75, 0x11d1, { 0xb4,0xb8,0x00,0xc0,0x4f,0xb6,0x6e,0xa0 }}

#if defined(__GNUC__)
#define GENERIC_CHAIN_FINALPOLICY_FUNCTION (const WCHAR []) \
    {'G','e','n','e','r','i','c','C','h','a','i','n','F','i','n','a','l','P','r','o','v', 0}
#define GENERIC_CHAIN_CERTTRUST_FUNCTION (const WCHAR []) \
    {'G','e','n','e','r','i','c','C','h','a','i','n','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0}
#elif defined(_MSC_VER)
#define GENERIC_CHAIN_FINALPOLICY_FUNCTION L"GenericChainFinalProv"
#define GENERIC_CHAIN_CERTTRUST_FUNCTION   L"GenericChainCertificateTrust"
#else
static const WCHAR GENERIC_CHAIN_FINALPOLICY_FUNCTION[] =
    {'G','e','n','e','r','i','c','C','h','a','i','n','F','i','n','a','l','P','r','o','v', 0};
static const WCHAR GENERIC_CHAIN_CERTTRUST_FUNCTION[] =
    {'G','e','n','e','r','i','c','C','h','a','i','n','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0};
#endif

typedef struct _WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO
{
    union {
        DWORD cbStruct;
        DWORD cbSize;
    } DUMMYUNIONNAME;
    PCCERT_CHAIN_CONTEXT pChainContext;
    DWORD                dwSignerType;
    PCMSG_SIGNER_INFO    pMsgSignerInfo;
    DWORD                dwError;
    DWORD                cCounterSigner;
    struct _WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *rgpCounterSigner;
} WTD_GENERIC_CHAIN_POLICY_SIGNER_INFO, *PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO;

typedef HRESULT (WINAPI *PFN_WTD_GENERIC_CHAIN_POLICY_CALLBACK)(
 PCRYPT_PROVIDER_DATA pProvData, DWORD dwStepError, DWORD dwRegPolicySettings,
 DWORD cSigner, PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO rgpSigner,
 void *pvPolicyArg);

typedef struct _WTD_GENERIC_CHAIN_POLICY_CREATE_INFO
{
    union {
        DWORD cbStruct;
        DWORD cbSize;
    } DUMMYUNIONNAME;
    HCERTCHAINENGINE hChainEngine;
    PCERT_CHAIN_PARA pChainPara;
    DWORD            dwFlags;
    void            *pvReserved;
} WTD_GENERIC_CHAIN_POLICY_CREATE_INFO, *PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO;

typedef struct _WTD_GENERIC_CHAIN_POLICY_DATA
{
    union {
        DWORD cbStruct;
        DWORD cbSize;
    } DUMMYUNIONNAME;
    PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO pSignerChainInfo;
    PWTD_GENERIC_CHAIN_POLICY_CREATE_INFO pCounterSignerChainInfo;
    PFN_WTD_GENERIC_CHAIN_POLICY_CALLBACK pfnPolicyCallback;
    void                                 *pvPolicyArg;
} WTD_GENERIC_CHAIN_POLICY_DATA, *PWTD_GENERIC_CHAIN_POLICY_DATA;

#if defined(__GNUC__)
#define SP_POLICY_PROVIDER_DLL_NAME (const WCHAR []) \
    {'W','I','N','T','R','U','S','T','.','D','L','L' ,0}
#elif defined(_MSC_VER)
#define SP_POLICY_PROVIDER_DLL_NAME L"WINTRUST.DLL"
#else
static const WCHAR SP_POLICY_PROVIDER_DLL_NAME[] =
    {'W','I','N','T','R','U','S','T','.','D','L','L', 0};
#endif

#define WINTRUST_ACTION_GENERIC_VERIFY_V2 \
    { 0xaac56b,   0xcd44, 0x11d0, { 0x8c,0xc2,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define SP_INIT_FUNCTION          (const WCHAR []) \
    {'S','o','f','t','p','u','b','I','n','i','t','i','a','l','i','z','e', 0}
#define SP_OBJTRUST_FUNCTION      (const WCHAR []) \
    {'S','o','f','t','p','u','b','L','o','a','d','M','e','s','s','a','g','e', 0}
#define SP_SIGTRUST_FUNCTION      (const WCHAR []) \
    {'S','o','f','t','p','u','b','L','o','a','d','S','i','g','n','a','t','u','r','e', 0}
#define SP_CHKCERT_FUNCTION       (const WCHAR []) \
    {'S','o','f','t','p','u','b','C','h','e','c','k','C','e','r','t', 0}
#define SP_FINALPOLICY_FUNCTION   (const WCHAR []) \
    {'S','o','f','t','p','u','b','A','u','t','h','e','n','t','i','c','o','d','e', 0}
#define SP_CLEANUPPOLICY_FUNCTION (const WCHAR []) \
    {'S','o','f','t','p','u','b','C','l','e','a','n','u','p', 0}
#elif defined(_MSC_VER)
#define SP_INIT_FUNCTION          L"SoftpubInitialize"
#define SP_OBJTRUST_FUNCTION      L"SoftpubLoadMessage"
#define SP_SIGTRUST_FUNCTION      L"SoftpubLoadSignature"
#define SP_CHKCERT_FUNCTION       L"SoftpubCheckCert"
#define SP_FINALPOLICY_FUNCTION   L"SoftpubAuthenticode"
#define SP_CLEANUPPOLICY_FUNCTION L"SoftpubCleanup"
#else
static const WCHAR SP_INIT_FUNCTION[]          =
    {'S','o','f','t','p','u','b','I','n','i','t','i','a','l','i','z','e', 0};
static const WCHAR SP_OBJTRUST_FUNCTION[]      =
    {'S','o','f','t','p','u','b','L','o','a','d','M','e','s','s','a','g','e', 0};
static const WCHAR SP_SIGTRUST_FUNCTION[]      =
    {'S','o','f','t','p','u','b','L','o','a','d','S','i','g','n','a','t','u','r','e', 0};
static const WCHAR SP_CHKCERT_FUNCTION[]       =
    {'S','o','f','t','p','u','b','C','h','e','c','k','C','e','r','t', 0};
static const WCHAR SP_FINALPOLICY_FUNCTION[]   =
    {'S','o','f','t','p','u','b','A','u','t','h','e','n','t','i','c','o','d','e', 0};
static const WCHAR SP_CLEANUPPOLICY_FUNCTION[] =
    {'S','o','f','t','p','u','b','C','l','e','a','n','u','p', 0};
#endif

#define WINTRUST_ACTION_TRUSTPROVIDER_TEST \
    { 0x573e31f8, 0xddba, 0x11d0, { 0x8c,0xcb,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define SP_TESTDUMPPOLICY_FUNCTION_TEST (const WCHAR []) \
    {'S','o','f','t','p','u','b','D','u','m','p','S','t','r','u','c','t','u','r','e', 0}
#elif defined(_MSC_VER)
#define SP_TESTDUMPPOLICY_FUNCTION_TEST L"SoftpubDumpStructure"
#else
static const WCHAR SP_TESTDUMPPOLICY_FUNCTION_TEST[] =
    {'S','o','f','t','p','u','b','D','u','m','p','S','t','r','u','c','t','u','r','e', 0};
#endif

#define HTTPSPROV_ACTION \
    { 0x573e31f8, 0xaaba, 0x11d0, { 0x8c,0xcb,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define HTTPS_CERTTRUST_FUNCTION (const WCHAR []) \
    {'H','T','T','P','S','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0}
#define HTTPS_FINALPOLICY_FUNCTION (const WCHAR []) \
    {'H','T','T','P','S','F','i','n','a','l','P','r','o','v', 0}
#elif defined(_MSC_VER)
#define HTTPS_FINALPOLICY_FUNCTION L"HTTPSFinalProv"
#define HTTPS_CERTTRUST_FUNCTION   L"HTTPSCertificateTrust"
#else
static const WCHAR HTTPS_CERTTRUST_FUNCTION[] =
    {'H','T','T','P','S','C','e','r','t','i','f','i','c','a','t','e','T','r','u','s','t', 0};
static const WCHAR HTTPS_FINALPOLICY_FUNCTION[] =
    {'H','T','T','P','S','F','i','n','a','l','P','r','o','v', 0};
#endif

#define OFFICESIGN_ACTION_VERIFY \
    { 0x5555c2cd, 0x17fb, 0x11d1, { 0x85,0xc4,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define OFFICE_POLICY_PROVIDER_DLL_NAME (const WCHAR []) \
    {'W','I','N','T','R','U','S','T','.','D','L','L' ,0}
#define OFFICE_INITPROV_FUNCTION (const WCHAR []) \
    {'O','f','f','i','c','e','I','n','i','t','i','a','l','i','z','e','P','o','l','i','c','y', 0}
#define OFFICE_CLEANUPPOLICY_FUNCTION (const WCHAR []) \
    {'O','f','f','i','c','e','C','l','e','a','n','u','p','P','o','l','i','c','y', 0}
#elif defined(_MSC_VER)
#define     OFFICE_POLICY_PROVIDER_DLL_NAME SP_POLICY_PROVIDER_DLL_NAME
#define     OFFICE_INITPROV_FUNCTION        L"OfficeInitializePolicy"
#define     OFFICE_CLEANUPPOLICY_FUNCTION   L"OfficeCleanupPolicy"
#else
static const WCHAR OFFICE_POLICY_PROVIDER_DLL_NAME[] =
    {'W','I','N','T','R','U','S','T','.','D','L','L', 0};
static const WCHAR OFFICE_INITPROV_FUNCTION[] =
    {'O','f','f','i','c','e','I','n','i','t','i','a','l','i','z','e','P','o','l','i','c','y', 0};
static const WCHAR OFFICE_CLEANUPPOLICY_FUNCTION[] =
    {'O','f','f','i','c','e','C','l','e','a','n','u','p','P','o','l','i','c','y', 0};
#endif

#define DRIVER_ACTION_VERIFY \
    { 0xf750e6c3, 0x38ee, 0x11d1, { 0x85,0xe5,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#if defined(__GNUC__)
#define DRIVER_INITPROV_FUNCTION (const WCHAR []) \
    {'D','r','i','v','e','r','I','n','i','t','i','a','l','i','z','e','P','o','l','i','c','y', 0}
#define DRIVER_FINALPOLPROV_FUNCTION (const WCHAR []) \
    {'D','r','i','v','e','r','F','i','n','a','l','P','o','l','i','c','y', 0}
#define DRIVER_CLEANUPPOLICY_FUNCTION (const WCHAR []) \
    {'D','r','i','v','e','r','C','l','e','a','n','u','p','P','o','l','i','c','y', 0}
#elif defined(_MSC_VER)
#define     DRIVER_INITPROV_FUNCTION      L"DriverInitializePolicy"
#define     DRIVER_FINALPOLPROV_FUNCTION  L"DriverFinalPolicy"
#define     DRIVER_CLEANUPPOLICY_FUNCTION L"DriverCleanupPolicy"
#else
static const WCHAR DRIVER_INITPROV_FUNCTION[] =
    {'D','r','i','v','e','r','I','n','i','t','i','a','l','i','z','e','P','o','l','i','c','y', 0};
static const WCHAR DRIVER_FINALPOLPROV_FUNCTION[] =
    {'D','r','i','v','e','r','F','i','n','a','l','P','o','l','i','c','y', 0};
static const WCHAR DRIVER_CLEANUPPOLICY_FUNCTION[] =
    {'D','r','i','v','e','r','C','l','e','a','n','u','p','P','o','l','i','c','y', 0};
#endif

typedef struct DRIVER_VER_MAJORMINOR_
{
    DWORD dwMajor;
    DWORD dwMinor;
} DRIVER_VER_MAJORMINOR;

typedef struct DRIVER_VER_INFO_
{
    DWORD                 cbStruct;
    ULONG_PTR             dwReserved1;
    ULONG_PTR             dwReserved2;
    DWORD                 dwPlatform;
    DWORD                 dwVersion;
    WCHAR                 wszVersion[MAX_PATH];
    WCHAR                 wszSignedBy[MAX_PATH];
    PCCERT_CONTEXT        pcSignerCertContext;
    DRIVER_VER_MAJORMINOR sOSVersionLow;
    DRIVER_VER_MAJORMINOR sOSVersionHigh;
    DWORD                 dwBuildNumberLow;
    DWORD                 dwBuildNumberHigh;
} DRIVER_VER_INFO, *PDRIVER_VER_INFO;

#endif /* __WINE_SOFTPUB_H */
