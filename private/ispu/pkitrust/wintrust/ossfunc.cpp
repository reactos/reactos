//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ossfunc.cpp
//
//--------------------------------------------------------------------------


#include "global.hxx"

extern "C"
{
#ifdef OSS_CRYPT_ASN1
#   include "wtoss.h"
#else
#   include "wtasn.h"
#endif
}
#include "crypttls.h"
#include "unicode.h"
#include "pkiasn1.h"

#include <dbgdef.h>

#include "locals.h"

#define SpcAsnAlloc         WVTNew
#define SpcAsnFree          WVTDelete


// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

static const BYTE NullDer[2] = {0x05, 0x00};
static const CRYPT_OBJID_BLOB NullDerBlob = {2, (BYTE *)&NullDer[0]};

static HCRYPTASN1MODULE hAsn1Module;


extern "C"
{
BOOL WINAPI WVTAsn1SpcLinkEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_LINK pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcLinkDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_LINK pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcIndirectDataContentEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_INDIRECT_DATA_CONTENT pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcIndirectDataContentDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_INDIRECT_DATA_CONTENT pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcSpAgencyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_SP_AGENCY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcSpAgencyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_SP_AGENCY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcMinimalCriteriaInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BOOL *pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcMinimalCriteriaInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BOOL *pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcFinancialCriteriaInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_FINANCIAL_CRITERIA pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcFinancialCriteriaInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_FINANCIAL_CRITERIA pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcStatementTypeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_STATEMENT_TYPE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcStatementTypeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_STATEMENT_TYPE pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcSpOpusInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_SP_OPUS_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcSpOpusInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_SP_OPUS_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcPeImageDataEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_PE_IMAGE_DATA pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcPeImageDataDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_PE_IMAGE_DATA pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1SpcSigInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_SIGINFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI WVTAsn1SpcSigInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_SIGINFO pInfo,
        IN OUT DWORD *pcbInfo
        );
BOOL WINAPI WVTAsn1UtcTimeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT FILETIME * pFileTime,
        IN OUT DWORD *pcbFileTime
        );

BOOL WINAPI WVTAsn1CatNameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCAT_NAMEVALUE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded);

BOOL WINAPI WVTAsn1CatMemberInfoEncode(
        IN DWORD dwEncoding,
        IN LPCSTR lpszStructType,
        IN PCAT_MEMBERINFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded);

BOOL WINAPI WVTAsn1CatNameValueDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCAT_NAMEVALUE pInfo,
        IN OUT DWORD *pcbInfo);

BOOL WINAPI WVTAsn1CatMemberInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCAT_MEMBERINFO pInfo,
        IN OUT DWORD *pcbInfo);
};


static inline ASN1encoding_t GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(hAsn1Module);
}
static inline ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hAsn1Module);
}

//+-------------------------------------------------------------------------
//  SPC ASN allocation and free functions
//--------------------------------------------------------------------------
HRESULT HError()
{
    DWORD dw = GetLastError();

    HRESULT hr;
    if ( dw <= 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}

typedef struct _OID_REG_ENTRY {
    LPCSTR   pszOID;
    LPCSTR   pszOverrideFuncName;
} OID_REG_ENTRY, *POID_REG_ENTRY;

static const OID_REG_ENTRY SpcRegEncodeTable[] =
{
    SPC_PE_IMAGE_DATA_OBJID,            "WVTAsn1SpcPeImageDataEncode",
    SPC_PE_IMAGE_DATA_STRUCT,           "WVTAsn1SpcPeImageDataEncode",

    SPC_CAB_DATA_OBJID,                 "WVTAsn1SpcLinkEncode",
    SPC_CAB_DATA_STRUCT,                "WVTAsn1SpcLinkEncode",

    SPC_JAVA_CLASS_DATA_OBJID,          "WVTAsn1SpcLinkEncode",
    SPC_JAVA_CLASS_DATA_STRUCT,         "WVTAsn1SpcLinkEncode",

    SPC_LINK_OBJID,                     "WVTAsn1SpcLinkEncode",
    SPC_LINK_STRUCT,                    "WVTAsn1SpcLinkEncode",

    SPC_SIGINFO_OBJID,                  "WVTAsn1SpcSigInfoEncode",
    SPC_SIGINFO_STRUCT,                 "WVTAsn1SpcSigInfoEncode",

    SPC_INDIRECT_DATA_OBJID,            "WVTAsn1SpcIndirectDataContentEncode",
    SPC_INDIRECT_DATA_CONTENT_STRUCT,   "WVTAsn1SpcIndirectDataContentEncode",

    SPC_SP_AGENCY_INFO_OBJID,           "WVTAsn1SpcSpAgencyInfoEncode",
    SPC_SP_AGENCY_INFO_STRUCT,          "WVTAsn1SpcSpAgencyInfoEncode",

    SPC_MINIMAL_CRITERIA_OBJID,         "WVTAsn1SpcMinimalCriteriaInfoEncode",
    SPC_MINIMAL_CRITERIA_STRUCT,        "WVTAsn1SpcMinimalCriteriaInfoEncode",

    SPC_FINANCIAL_CRITERIA_OBJID,       "WVTAsn1SpcFinancialCriteriaInfoEncode",
    SPC_FINANCIAL_CRITERIA_STRUCT,      "WVTAsn1SpcFinancialCriteriaInfoEncode",

    SPC_STATEMENT_TYPE_OBJID,           "WVTAsn1SpcStatementTypeEncode",
    SPC_STATEMENT_TYPE_STRUCT,          "WVTAsn1SpcStatementTypeEncode",

    CAT_NAMEVALUE_OBJID,                "WVTAsn1CatNameValueEncode",
    CAT_NAMEVALUE_STRUCT,               "WVTAsn1CatNameValueEncode",

    CAT_MEMBERINFO_OBJID,               "WVTAsn1CatMemberInfoEncode",
    CAT_MEMBERINFO_STRUCT,              "WVTAsn1CatMemberInfoEncode",

    SPC_SP_OPUS_INFO_OBJID,             "WVTAsn1SpcSpOpusInfoEncode",
    SPC_SP_OPUS_INFO_STRUCT,            "WVTAsn1SpcSpOpusInfoEncode"

};
#define SPC_REG_ENCODE_COUNT (sizeof(SpcRegEncodeTable) / sizeof(SpcRegEncodeTable[0]))

static const OID_REG_ENTRY SpcRegDecodeTable[] =
{
    SPC_PE_IMAGE_DATA_OBJID,            "WVTAsn1SpcPeImageDataDecode",
    SPC_PE_IMAGE_DATA_STRUCT,           "WVTAsn1SpcPeImageDataDecode",

    SPC_CAB_DATA_OBJID,                 "WVTAsn1SpcLinkDecode",
    SPC_CAB_DATA_STRUCT,                "WVTAsn1SpcLinkDecode",

    SPC_JAVA_CLASS_DATA_OBJID,          "WVTAsn1SpcLinkDecode",
    SPC_JAVA_CLASS_DATA_STRUCT,         "WVTAsn1SpcLinkDecode",

    SPC_LINK_OBJID,                     "WVTAsn1SpcLinkDecode",
    SPC_LINK_STRUCT,                    "WVTAsn1SpcLinkDecode",

    SPC_SIGINFO_OBJID,                  "WVTAsn1SpcSigInfoDecode",
    SPC_SIGINFO_STRUCT,                 "WVTAsn1SpcSigInfoDecode",

    SPC_INDIRECT_DATA_OBJID,            "WVTAsn1SpcIndirectDataContentDecode",
    SPC_INDIRECT_DATA_CONTENT_STRUCT,   "WVTAsn1SpcIndirectDataContentDecode",

    SPC_SP_AGENCY_INFO_OBJID,           "WVTAsn1SpcSpAgencyInfoDecode",
    SPC_SP_AGENCY_INFO_STRUCT,          "WVTAsn1SpcSpAgencyInfoDecode",

    SPC_MINIMAL_CRITERIA_OBJID,         "WVTAsn1SpcMinimalCriteriaInfoDecode",
    SPC_MINIMAL_CRITERIA_STRUCT,        "WVTAsn1SpcMinimalCriteriaInfoDecode",

    SPC_FINANCIAL_CRITERIA_OBJID,       "WVTAsn1SpcFinancialCriteriaInfoDecode",
    SPC_FINANCIAL_CRITERIA_STRUCT,      "WVTAsn1SpcFinancialCriteriaInfoDecode",

    SPC_STATEMENT_TYPE_OBJID,           "WVTAsn1SpcStatementTypeDecode",
    SPC_STATEMENT_TYPE_STRUCT,          "WVTAsn1SpcStatementTypeDecode",

    CAT_NAMEVALUE_OBJID,                "WVTAsn1CatNameValueDecode",
    CAT_NAMEVALUE_STRUCT,               "WVTAsn1CatNameValueDecode",

    CAT_MEMBERINFO_OBJID,               "WVTAsn1CatMemberInfoDecode",
    CAT_MEMBERINFO_STRUCT,              "WVTAsn1CatMemberInfoDecode",

    SPC_SP_OPUS_INFO_OBJID,             "WVTAsn1SpcSpOpusInfoDecode",
    SPC_SP_OPUS_INFO_STRUCT,            "WVTAsn1SpcSpOpusInfoDecode"

};
#define SPC_REG_DECODE_COUNT (sizeof(SpcRegDecodeTable) / sizeof(SpcRegDecodeTable[0]))

#ifdef OSS_CRYPT_ASN1
#define ASN1_OID_OFFSET         10000 +
#define ASN1_OID_PREFIX         "OssCryptAsn1."
#else
#define ASN1_OID_OFFSET
#define ASN1_OID_PREFIX
#endif  // OSS_CRYPT_ASN1

static const CRYPT_OID_FUNC_ENTRY SpcEncodeFuncTable[] =
{
    ASN1_OID_OFFSET SPC_PE_IMAGE_DATA_STRUCT,           WVTAsn1SpcPeImageDataEncode,
    ASN1_OID_PREFIX SPC_PE_IMAGE_DATA_OBJID,            WVTAsn1SpcPeImageDataEncode,

    ASN1_OID_PREFIX SPC_CAB_DATA_OBJID,                 WVTAsn1SpcLinkEncode,
    ASN1_OID_OFFSET SPC_CAB_DATA_STRUCT,                WVTAsn1SpcLinkEncode,

    ASN1_OID_OFFSET SPC_LINK_STRUCT,                    WVTAsn1SpcLinkEncode,
    ASN1_OID_PREFIX SPC_LINK_OBJID,                     WVTAsn1SpcLinkEncode,

    ASN1_OID_OFFSET SPC_SIGINFO_STRUCT,                 WVTAsn1SpcSigInfoEncode,
    ASN1_OID_PREFIX SPC_SIGINFO_OBJID,                  WVTAsn1SpcSigInfoEncode,

    ASN1_OID_PREFIX SPC_INDIRECT_DATA_OBJID,            WVTAsn1SpcIndirectDataContentEncode,
    ASN1_OID_OFFSET SPC_INDIRECT_DATA_CONTENT_STRUCT,   WVTAsn1SpcIndirectDataContentEncode,

    ASN1_OID_OFFSET SPC_SP_AGENCY_INFO_STRUCT,          WVTAsn1SpcSpAgencyInfoEncode,
    ASN1_OID_PREFIX SPC_SP_AGENCY_INFO_OBJID,           WVTAsn1SpcSpAgencyInfoEncode,

    ASN1_OID_OFFSET SPC_MINIMAL_CRITERIA_STRUCT,        WVTAsn1SpcMinimalCriteriaInfoEncode,
    ASN1_OID_PREFIX SPC_MINIMAL_CRITERIA_OBJID,         WVTAsn1SpcMinimalCriteriaInfoEncode,

    ASN1_OID_OFFSET SPC_FINANCIAL_CRITERIA_STRUCT,      WVTAsn1SpcFinancialCriteriaInfoEncode,
    ASN1_OID_PREFIX SPC_FINANCIAL_CRITERIA_OBJID,       WVTAsn1SpcFinancialCriteriaInfoEncode,

    ASN1_OID_OFFSET SPC_STATEMENT_TYPE_STRUCT,          WVTAsn1SpcStatementTypeEncode,
    ASN1_OID_PREFIX SPC_STATEMENT_TYPE_OBJID,           WVTAsn1SpcStatementTypeEncode,

    ASN1_OID_PREFIX CAT_NAMEVALUE_OBJID,                WVTAsn1CatNameValueEncode,
    ASN1_OID_OFFSET CAT_NAMEVALUE_STRUCT,               WVTAsn1CatNameValueEncode,

    ASN1_OID_PREFIX CAT_MEMBERINFO_OBJID,               WVTAsn1CatMemberInfoEncode,
    ASN1_OID_OFFSET CAT_MEMBERINFO_STRUCT,              WVTAsn1CatMemberInfoEncode,

    ASN1_OID_OFFSET SPC_SP_OPUS_INFO_STRUCT,            WVTAsn1SpcSpOpusInfoEncode,
    ASN1_OID_PREFIX SPC_SP_OPUS_INFO_OBJID,             WVTAsn1SpcSpOpusInfoEncode
};

#define SPC_ENCODE_FUNC_COUNT (sizeof(SpcEncodeFuncTable) / \
                                    sizeof(SpcEncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY SpcDecodeFuncTable[] =
{
    ASN1_OID_OFFSET SPC_PE_IMAGE_DATA_STRUCT,           WVTAsn1SpcPeImageDataDecode,
    ASN1_OID_PREFIX SPC_PE_IMAGE_DATA_OBJID,            WVTAsn1SpcPeImageDataDecode,

    ASN1_OID_OFFSET SPC_CAB_DATA_STRUCT,                WVTAsn1SpcLinkDecode,
    ASN1_OID_PREFIX SPC_CAB_DATA_OBJID,                 WVTAsn1SpcLinkDecode,

    ASN1_OID_OFFSET SPC_LINK_STRUCT,                    WVTAsn1SpcLinkDecode,
    ASN1_OID_PREFIX SPC_LINK_OBJID,                     WVTAsn1SpcLinkDecode,

    ASN1_OID_OFFSET SPC_SIGINFO_STRUCT,                 WVTAsn1SpcSigInfoDecode,
    ASN1_OID_PREFIX SPC_SIGINFO_OBJID,                  WVTAsn1SpcSigInfoDecode,

    ASN1_OID_OFFSET PKCS_UTC_TIME,                      WVTAsn1UtcTimeDecode,
    ASN1_OID_PREFIX szOID_RSA_signingTime,              WVTAsn1UtcTimeDecode,

    ASN1_OID_OFFSET SPC_SP_AGENCY_INFO_STRUCT,          WVTAsn1SpcSpAgencyInfoDecode,
    ASN1_OID_PREFIX SPC_SP_AGENCY_INFO_OBJID,           WVTAsn1SpcSpAgencyInfoDecode,

    ASN1_OID_OFFSET SPC_SP_OPUS_INFO_STRUCT,            WVTAsn1SpcSpOpusInfoDecode,
    ASN1_OID_PREFIX SPC_SP_OPUS_INFO_OBJID,             WVTAsn1SpcSpOpusInfoDecode,

    ASN1_OID_OFFSET SPC_INDIRECT_DATA_CONTENT_STRUCT,   WVTAsn1SpcIndirectDataContentDecode,
    ASN1_OID_PREFIX SPC_INDIRECT_DATA_OBJID,            WVTAsn1SpcIndirectDataContentDecode,

    ASN1_OID_OFFSET SPC_SP_AGENCY_INFO_STRUCT,          WVTAsn1SpcSpAgencyInfoDecode,
    ASN1_OID_PREFIX SPC_SP_AGENCY_INFO_OBJID,           WVTAsn1SpcSpAgencyInfoDecode,

    ASN1_OID_OFFSET SPC_MINIMAL_CRITERIA_STRUCT,        WVTAsn1SpcMinimalCriteriaInfoDecode,
    ASN1_OID_PREFIX SPC_MINIMAL_CRITERIA_OBJID,         WVTAsn1SpcMinimalCriteriaInfoDecode,

    ASN1_OID_OFFSET SPC_FINANCIAL_CRITERIA_STRUCT,      WVTAsn1SpcFinancialCriteriaInfoDecode,
    ASN1_OID_PREFIX SPC_FINANCIAL_CRITERIA_OBJID,       WVTAsn1SpcFinancialCriteriaInfoDecode,

    ASN1_OID_OFFSET SPC_STATEMENT_TYPE_STRUCT,          WVTAsn1SpcStatementTypeDecode,
    ASN1_OID_PREFIX SPC_STATEMENT_TYPE_OBJID,           WVTAsn1SpcStatementTypeDecode,

    ASN1_OID_OFFSET CAT_NAMEVALUE_STRUCT,               WVTAsn1CatNameValueDecode,
    ASN1_OID_PREFIX CAT_NAMEVALUE_OBJID,                WVTAsn1CatNameValueDecode,

    ASN1_OID_OFFSET CAT_MEMBERINFO_STRUCT,              WVTAsn1CatMemberInfoDecode,
    ASN1_OID_PREFIX CAT_MEMBERINFO_OBJID,               WVTAsn1CatMemberInfoDecode,

    ASN1_OID_OFFSET SPC_FINANCIAL_CRITERIA_STRUCT,      WVTAsn1SpcFinancialCriteriaInfoDecode,
    ASN1_OID_PREFIX SPC_FINANCIAL_CRITERIA_OBJID,       WVTAsn1SpcFinancialCriteriaInfoDecode
};

#define SPC_DECODE_FUNC_COUNT (sizeof(SpcDecodeFuncTable) / \
                                    sizeof(SpcDecodeFuncTable[0]))

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
HRESULT WINAPI ASNRegisterServer(LPCWSTR dllName)
{
    int i;

    for (i = 0; i < SPC_REG_ENCODE_COUNT; i++)
    {
        if (!(CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
                                       SpcRegEncodeTable[i].pszOID, dllName,
                                       SpcRegEncodeTable[i].pszOverrideFuncName)))
        {
            return(HError());
        }
    }

    for (i = 0; i < SPC_REG_DECODE_COUNT; i++)
    {
        if (!(CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
                                       SpcRegDecodeTable[i].pszOID, dllName,
                                       SpcRegDecodeTable[i].pszOverrideFuncName)))
        {
            return(HError());
        }
    }

    return S_OK;
}


HRESULT WINAPI ASNUnregisterServer()
{
    HRESULT hr = S_OK;
    int     i;

    for (i = 0; i < SPC_REG_ENCODE_COUNT; i++)
    {
        if (!(CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC,
                                         SpcRegEncodeTable[i].pszOID)))
        {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
            {
                hr = HError();
            }
        }
    }

    for (i = 0; i < SPC_REG_DECODE_COUNT; i++)
    {
        if (!(CryptUnregisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC,
                                         SpcRegDecodeTable[i].pszOID)))
        {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
            {
                hr = HError();
            }
        }
    }
    return(hr);
}



BOOL WINAPI ASNDllMain(HMODULE hInst, ULONG ulReason, LPVOID lpReserved)
{
    BOOL    fRet;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef OSS_CRYPT_ASN1
        if (0 == (hAsn1Module = I_CryptInstallAsn1Module(wtoss, 0, NULL)))
#else
        WTASN_Module_Startup();
        if (0 == (hAsn1Module = I_CryptInstallAsn1Module(
                WTASN_Module, 0, NULL)))
#endif  // OSS_CRYPT_ASN1
        {
            goto CryptInstallAsn1ModuleError;
        }

        if (!(CryptInstallOIDFunctionAddress(
#ifdef OSS_CRYPT_ASN1
                NULL, // for testing purposes, osscrypt.dll explicitly unloaded
#else
                hInst,
#endif  // OSS_CRYPT_ASN1
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                SPC_ENCODE_FUNC_COUNT,
                SpcEncodeFuncTable,
                0)))
        {
            goto CryptInstallOIDFunctionAddressError;
        }

        if (!(CryptInstallOIDFunctionAddress(
#ifdef OSS_CRYPT_ASN1
                NULL, // for testing purposes, osscrypt.dll explicitly unloaded
#else
                hInst,
#endif  // OSS_CRYPT_ASN1
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_FUNC,
                SPC_DECODE_FUNC_COUNT,
                SpcDecodeFuncTable,
                CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG)))
        {
            goto CryptInstallOIDFunctionAddressError;
        }

        break;

    case DLL_PROCESS_DETACH:
        I_CryptUninstallAsn1Module(hAsn1Module);
#ifndef OSS_CRYPT_ASN1
        WTASN_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS,CryptInstallAsn1ModuleError)
TRACE_ERROR_EX(DBG_SS,CryptInstallOIDFunctionAddressError)
}

#ifdef OSS_CRYPT_ASN1
typedef struct BITSTRING
{
    unsigned int    length;  /* number of significant bits */
    unsigned char   *value;
} BITSTRING;
#endif  // OSS_CRYPT_ASN1


//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
inline void WVTAsn1SetAny(IN PCRYPT_OBJID_BLOB pInfo, OUT NOCOPYANY *pOss)
{
    PkiAsn1SetAny(pInfo, pOss);
}

inline void WVTAsn1GetAny(IN NOCOPYANY *pOss, IN DWORD dwFlags, OUT PCRYPT_OBJID_BLOB pInfo,
                                 IN OUT BYTE **ppbExtra, IN OUT LONG *plRemainExtra)
{
    PkiAsn1GetAny(pOss, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
inline void WVTAsn1SetOctetString(IN PCRYPT_DATA_BLOB pInfo, OUT OCTETSTRING *pOss)
{
    pOss->value = pInfo->pbData;
    pOss->length = pInfo->cbData;
}

inline void WVTAsn1GetOctetString(
        IN OCTETSTRING *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pOss->length, pOss->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

inline void WVTAsn1SetBit(IN PCRYPT_BIT_BLOB pInfo, OUT BITSTRING *pOss)
{
    PkiAsn1SetBitString(pInfo, &pOss->length, &pOss->value);
}

inline void WVTAsn1GetBit(IN BITSTRING *pOss, IN DWORD dwFlags,
                                 OUT PCRYPT_BIT_BLOB pInfo,
                                 IN OUT BYTE **ppbExtra,
                                 IN OUT LONG *plRemainExtra)
{
    PkiAsn1GetBitString(pOss->length, pOss->value, dwFlags,
                        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Unicode mapped to IA5 String
//--------------------------------------------------------------------------
inline BOOL WVTAsn1SetUnicodeConvertedToIA5(
        IN LPWSTR pwsz,
        OUT IA5STRING *pOss
        )
{
    return PkiAsn1SetUnicodeConvertedToIA5String(pwsz,
        &pOss->length, &pOss->value);
}
inline void WVTAsn1FreeUnicodeConvertedToIA5(IN IA5STRING *pOss)
{
    PkiAsn1FreeUnicodeConvertedToIA5String(pOss->value);
    pOss->value = NULL;
}
inline void WVTAsn1GetIA5ConvertedToUnicode(
        IN IA5STRING *pOss,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetIA5StringConvertedToUnicode(pOss->length, pOss->value, dwFlags,
        ppwsz, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get LPWSTR (BMP String)
//--------------------------------------------------------------------------
inline void WVTAsn1SetBMP(
        IN LPWSTR pwsz,
        OUT BMPSTRING *pOss
        )
{
    pOss->value = pwsz;
    pOss->length = wcslen(pwsz);
}
inline void WVTAsn1GetBMP(
        IN BMPSTRING *pOss,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetBMPString(pOss->length, pOss->value, dwFlags,
        ppwsz, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Get Spc String
//--------------------------------------------------------------------------
void WVTAsn1SetSpcString(
        IN LPWSTR pwsz,
        OUT SpcString *pOss
        )
{
    pOss->choice = unicode_chosen;
    WVTAsn1SetBMP(pwsz, &pOss->u.unicode);
}

void WVTAsn1GetSpcString(
        IN SpcString *pOss,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    switch (pOss->choice) {
    case unicode_chosen:
        WVTAsn1GetBMP(&pOss->u.unicode, dwFlags,
            ppwsz, ppbExtra, plRemainExtra);
        break;
    case ascii_chosen:
        WVTAsn1GetIA5ConvertedToUnicode(&pOss->u.ascii, dwFlags,
            ppwsz, ppbExtra, plRemainExtra);
        break;
    default:
        if (*plRemainExtra >= 0)
            *ppwsz = NULL;
    }
}

//+-------------------------------------------------------------------------
//  Set/Get Spc Link
//--------------------------------------------------------------------------
BOOL WVTAsn1SetSpcLink(
        IN PSPC_LINK pInfo,
        OUT SpcLink *pOss
        )
{
    BOOL fRet = TRUE;

    memset(pOss, 0, sizeof(*pOss));

    // Assumption: OSS choice == dwLinkChoice
    // WVTAsn1GetSpcLink has asserts to verify
    pOss->choice = (unsigned short) pInfo->dwLinkChoice;

    switch (pInfo->dwLinkChoice) {
    case SPC_URL_LINK_CHOICE:
        fRet = WVTAsn1SetUnicodeConvertedToIA5(pInfo->pwszUrl, &pOss->u.url);
        break;
    case SPC_MONIKER_LINK_CHOICE:
        pOss->u.moniker.classId.length = sizeof(pInfo->Moniker.ClassId);
#ifdef OSS_CRYPT_ASN1
        memcpy(pOss->u.moniker.classId.value, pInfo->Moniker.ClassId,
            sizeof(pInfo->Moniker.ClassId));
#else
        pOss->u.moniker.classId.value = pInfo->Moniker.ClassId;
#endif  // OSS_CRYPT_ASN1
        WVTAsn1SetOctetString(&pInfo->Moniker.SerializedData,
            &pOss->u.moniker.serializedData);
        break;
    case SPC_FILE_LINK_CHOICE:
        WVTAsn1SetSpcString(pInfo->pwszFile, &pOss->u.file);
        break;
    default:
        SetLastError((DWORD) E_INVALIDARG);
        fRet = FALSE;
    }

    return fRet;
}

void WVTAsn1FreeSpcLink(
        IN SpcLink *pOss
        )
{
    if (pOss->choice == url_chosen)
        WVTAsn1FreeUnicodeConvertedToIA5(&pOss->u.url);
}

BOOL WVTAsn1GetSpcLink(
        IN SpcLink *pOss,
        IN DWORD dwFlags,
        OUT PSPC_LINK pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    DWORD dwLinkChoice;

    assert(url_chosen == SPC_URL_LINK_CHOICE);
    assert(moniker_chosen == SPC_MONIKER_LINK_CHOICE);
    assert(file_chosen == SPC_FILE_LINK_CHOICE);
#ifdef OSS_CRYPT_ASN1
    assert(sizeof(pInfo->Moniker.ClassId) ==
        sizeof(pOss->u.moniker.classId.value));
#endif  // OSS_CRYPT_ASN1

    dwLinkChoice = pOss->choice;

    if (*plRemainExtra >= 0) {
        memset(pInfo, 0, sizeof(*pInfo));
        pInfo->dwLinkChoice = dwLinkChoice;
    }

    switch (dwLinkChoice) {
    case SPC_URL_LINK_CHOICE:
        WVTAsn1GetIA5ConvertedToUnicode(&pOss->u.url, dwFlags,
            &pInfo->pwszUrl, ppbExtra, plRemainExtra);
        break;
    case SPC_MONIKER_LINK_CHOICE:
#ifndef OSS_CRYPT_ASN1
        if (sizeof(pInfo->Moniker.ClassId) != pOss->u.moniker.classId.length) {
            SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
            return FALSE;
        }
#endif  // OSS_CRYPT_ASN1

        if (*plRemainExtra >= 0) {
            memcpy(pInfo->Moniker.ClassId, pOss->u.moniker.classId.value,
                sizeof(pInfo->Moniker.ClassId));
        }
        WVTAsn1GetOctetString(&pOss->u.moniker.serializedData, dwFlags,
            &pInfo->Moniker.SerializedData, ppbExtra, plRemainExtra);
        break;
    case SPC_FILE_LINK_CHOICE:
        WVTAsn1GetSpcString(&pOss->u.file, dwFlags,
            &pInfo->pwszFile, ppbExtra, plRemainExtra);
        break;
    default:
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }

    return TRUE;
}

BOOL WVTAsn1GetSpcLinkPointer(
        IN SpcLink *pOss,
        IN DWORD dwFlags,
        OUT PSPC_LINK *pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    PSPC_LINK pLink;

    lAlignExtra = INFO_LEN_ALIGN(sizeof(SPC_LINK));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        pLink = (PSPC_LINK) *ppbExtra;
        *pInfo = pLink;
        *ppbExtra += lAlignExtra;
    } else
        pLink = NULL;

    return WVTAsn1GetSpcLink(
        pOss,
        dwFlags,
        pLink,
        ppbExtra,
        plRemainExtra
        );
}

BOOL WVTAsn1SetSpcSigInfo(IN PSPC_SIGINFO pInfo, OUT SpcSigInfo *pOss)
{
    memset(pOss, 0x00, sizeof(*pOss));

    pOss->dwSIPversion      = pInfo->dwSipVersion;

    pOss->gSIPguid.length   = sizeof(GUID);
#ifdef OSS_CRYPT_ASN1
    memcpy(&pOss->gSIPguid.value[0], &pInfo->gSIPGuid, sizeof(GUID));
#else
    pOss->gSIPguid.value = (BYTE *) &pInfo->gSIPGuid;
#endif  // OSS_CRYPT_ASN1

    pOss->dwReserved1       = pInfo->dwReserved1;
    pOss->dwReserved2       = pInfo->dwReserved2;
    pOss->dwReserved3       = pInfo->dwReserved3;
    pOss->dwReserved4       = pInfo->dwReserved4;
    pOss->dwReserved5       = pInfo->dwReserved5;

    return(TRUE);
}

BOOL WVTAsn1GetSpcSigInfo(IN SpcSigInfo *pOss, IN DWORD dwFlags,
                              OUT PSPC_SIGINFO pInfo, IN OUT BYTE **ppbExtra,
                              IN OUT LONG *plRemainExtra)
{

    if (!(pInfo))
    {
        return(TRUE);
    }

    pInfo->dwSipVersion     = pOss->dwSIPversion;
#ifdef OSS_CRYPT_ASN1
    memcpy(&pInfo->gSIPGuid, &pOss->gSIPguid.value[0], sizeof(GUID));
#else
    if (sizeof(GUID) != pOss->gSIPguid.length) {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
    memcpy(&pInfo->gSIPGuid, pOss->gSIPguid.value, sizeof(GUID));
#endif  // OSS_CRYPT_ASN1

    pInfo->dwReserved1      = pOss->dwReserved1;
    pInfo->dwReserved2      = pOss->dwReserved2;
    pInfo->dwReserved3      = pOss->dwReserved3;
    pInfo->dwReserved4      = pOss->dwReserved4;
    pInfo->dwReserved5      = pOss->dwReserved5;

    return(TRUE);
}


//+-------------------------------------------------------------------------
//  Set/Get Object Identifier string
//--------------------------------------------------------------------------
BOOL WVTAsn1SetObjId(
        IN LPSTR pszObjId,
        OUT ObjectID *pOss
        )
{
    pOss->count = sizeof(pOss->value) / sizeof(pOss->value[0]);
    if (PkiAsn1ToObjectIdentifier(pszObjId, &pOss->count, pOss->value))
        return TRUE;
    else {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }
}

void WVTAsn1GetObjId(
        IN ObjectID *pOss,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;
    DWORD cbObjId;

    cbObjId = lRemainExtra > 0 ? lRemainExtra : 0;
    PkiAsn1FromObjectIdentifier(
        pOss->count,
        pOss->value,
        (LPSTR) pbExtra,
        &cbObjId
        );

    lAlignExtra = INFO_LEN_ALIGN(cbObjId);
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        if(cbObjId) {
            *ppszObjId = (LPSTR) pbExtra;
        } else
            *ppszObjId = NULL;
        pbExtra += lAlignExtra;
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}


//+-------------------------------------------------------------------------
//  Set/Get CRYPT_ALGORITHM_IDENTIFIER
//--------------------------------------------------------------------------
BOOL WVTAsn1SetAlgorithm(
        IN PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        OUT AlgorithmIdentifier *pOss
        )
{
    memset(pOss, 0, sizeof(*pOss));
    if (pInfo->pszObjId) {
        if (!WVTAsn1SetObjId(pInfo->pszObjId, &pOss->algorithm))
            return FALSE;
        if (pInfo->Parameters.cbData)
            WVTAsn1SetAny(&pInfo->Parameters, &pOss->parameters);
        else
            // Per PKCS #1: default to the ASN.1 type NULL.
            WVTAsn1SetAny((PCRYPT_OBJID_BLOB) &NullDerBlob, &pOss->parameters);
        pOss->bit_mask |= parameters_present;
    }
    return TRUE;
}

void WVTAsn1GetAlgorithm(
        IN AlgorithmIdentifier *pOss,
        IN DWORD dwFlags,
        OUT PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    if (*plRemainExtra >= 0)
        memset(pInfo, 0, sizeof(*pInfo));
    WVTAsn1GetObjId(&pOss->algorithm, dwFlags, &pInfo->pszObjId,
            ppbExtra, plRemainExtra);
    if (pOss->bit_mask & parameters_present)
        WVTAsn1GetAny(&pOss->parameters, dwFlags, &pInfo->Parameters,
            ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Encode an OSS formatted info structure
//
//  Called by the WVTAsn1*Encode() functions.
//--------------------------------------------------------------------------
BOOL WVTAsn1InfoEncode(
        IN int pdunum,
        IN void *pOssInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfo(
        GetEncoder(),
        pdunum,
        pOssInfo,
        pbEncoded,
        pcbEncoded);
}


//+-------------------------------------------------------------------------
//  Decode into an allocated, OSS formatted info structure
//
//  Called by the WVTAsn1*Decode() functions.
//--------------------------------------------------------------------------
BOOL WVTAsn1InfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppOssInfo
        )
{
    return PkiAsn1DecodeAndAllocInfo(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppOssInfo);
}

//+-------------------------------------------------------------------------
//  Free an allocated, OSS formatted info structure
//
//  Called by the WVTAsn1*Decode() functions.
//--------------------------------------------------------------------------
void WVTAsn1InfoFree(
        IN int pdunum,
        IN void *pOssInfo
        )
{
    if (pOssInfo) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(GetDecoder(), pdunum, pOssInfo);

        SetLastError(dwErr);
    }
}

//+-------------------------------------------------------------------------
//  SPC PKCS #7 Indirect Data Content Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcIndirectDataContentEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_INDIRECT_DATA_CONTENT pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SpcIndirectDataContent OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));
    if (!WVTAsn1SetObjId(pInfo->Data.pszObjId, &OssInfo.data.type))
        goto ErrorReturn;

    if (pInfo->Data.Value.cbData) {
        WVTAsn1SetAny(&pInfo->Data.Value, &OssInfo.data.value);
        OssInfo.data.bit_mask |= value_present;
    }

    if (!WVTAsn1SetAlgorithm(&pInfo->DigestAlgorithm,
            &OssInfo.messageDigest.digestAlgorithm))
        goto ErrorReturn;
    WVTAsn1SetOctetString(&pInfo->Digest, &OssInfo.messageDigest.digest);

    fResult = WVTAsn1InfoEncode(
        SpcIndirectDataContent_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC PKCS #7 Indirect Data Content Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcIndirectDataContentDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_INDIRECT_DATA_CONTENT pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SpcIndirectDataContent *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!WVTAsn1InfoDecodeAndAlloc(
            SpcIndirectDataContent_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_INDIRECT_DATA_CONTENT);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(SPC_INDIRECT_DATA_CONTENT));

        pbExtra = (BYTE *) pInfo + sizeof(SPC_INDIRECT_DATA_CONTENT);
    }

    WVTAsn1GetObjId(&pOssInfo->data.type, dwFlags, &pInfo->Data.pszObjId,
            &pbExtra, &lRemainExtra);

    if (pOssInfo->data.bit_mask & value_present)
        WVTAsn1GetAny(&pOssInfo->data.value, dwFlags, &pInfo->Data.Value,
            &pbExtra, &lRemainExtra);

    WVTAsn1GetAlgorithm(&pOssInfo->messageDigest.digestAlgorithm, dwFlags,
            &pInfo->DigestAlgorithm, &pbExtra, &lRemainExtra);
    WVTAsn1GetOctetString(&pOssInfo->messageDigest.digest, dwFlags,
                &pInfo->Digest, &pbExtra, &lRemainExtra);

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1InfoFree(SpcIndirectDataContent_PDU, pOssInfo);
    return fResult;
}


BOOL WINAPI WVTAsn1UtcTimeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT FILETIME * pFileTime,
        IN OUT DWORD *pcbFileTime
        ) {


    BOOL fResult;
    UtcTime * putcTime = NULL;

    assert(pcbFileTime != NULL);

    if(pFileTime == NULL) {
            *pcbFileTime = sizeof(FILETIME);
            return(TRUE);
    }

    if (*pcbFileTime < sizeof(FILETIME)) {
            *pcbFileTime = sizeof(FILETIME);
            SetLastError((DWORD) ERROR_MORE_DATA);
            return(FALSE);
    }

    *pcbFileTime = sizeof(FILETIME);

    if (!WVTAsn1InfoDecodeAndAlloc(
            UtcTime_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &putcTime))
        goto WVTAsn1InfoDecodeAndAllocError;

    if( !PkiAsn1FromUTCTime(putcTime, pFileTime) )
            goto  PkiAsn1FromUTCTimeError;

    fResult = TRUE;

CommonReturn:
    WVTAsn1InfoFree(UtcTime_PDU, putcTime);
    return fResult;

ErrorReturn:
    *pcbFileTime = 0;
    fResult = FALSE;
        goto CommonReturn;

TRACE_ERROR_EX(DBG_SS,WVTAsn1InfoDecodeAndAllocError);
TRACE_ERROR_EX(DBG_SS,PkiAsn1FromUTCTimeError);
}

//+-------------------------------------------------------------------------
//  SPC SP Agency Info Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcSpAgencyInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_SP_AGENCY_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SpcSpAgencyInformation OssInfo;
    memset(&OssInfo, 0, sizeof(OssInfo));

    if (pInfo->pPolicyInformation) {
        if (!WVTAsn1SetSpcLink(pInfo->pPolicyInformation,
                &OssInfo.policyInformation))
            goto ErrorReturn;
        OssInfo.bit_mask |= policyInformation_present;
    }

    if (pInfo->pwszPolicyDisplayText) {
        WVTAsn1SetSpcString(pInfo->pwszPolicyDisplayText,
            &OssInfo.policyDisplayText);
        OssInfo.bit_mask |= policyDisplayText_present;
    }

    if (pInfo->pLogoImage) {
        PSPC_IMAGE pImage = pInfo->pLogoImage;
        if (pImage->pImageLink) {
            if (!WVTAsn1SetSpcLink(pImage->pImageLink,
                    &OssInfo.logoImage.imageLink))
                goto ErrorReturn;
            OssInfo.logoImage.bit_mask |= imageLink_present;
        }

        if (pImage->Bitmap.cbData != 0) {
            WVTAsn1SetOctetString(&pImage->Bitmap, &OssInfo.logoImage.bitmap);
            OssInfo.logoImage.bit_mask |= bitmap_present;
        }
        if (pImage->Metafile.cbData != 0) {
            WVTAsn1SetOctetString(&pImage->Metafile,
                &OssInfo.logoImage.metafile);
            OssInfo.logoImage.bit_mask |= metafile_present;
        }
        if (pImage->EnhancedMetafile.cbData != 0) {
            WVTAsn1SetOctetString(&pImage->EnhancedMetafile,
                &OssInfo.logoImage.enhancedMetafile);
            OssInfo.logoImage.bit_mask |= enhancedMetafile_present;
        }
        if (pImage->GifFile.cbData != 0) {
            WVTAsn1SetOctetString(&pImage->GifFile,
                &OssInfo.logoImage.gifFile);
            OssInfo.logoImage.bit_mask |= gifFile_present;
        }

        OssInfo.bit_mask |= logoImage_present;
    }

    if (pInfo->pLogoLink) {
        if (!WVTAsn1SetSpcLink(pInfo->pLogoLink, &OssInfo.logoLink))
            goto ErrorReturn;
        OssInfo.bit_mask |= logoLink_present;
    }

    fResult = WVTAsn1InfoEncode(
        SpcSpAgencyInformation_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1FreeSpcLink(&OssInfo.policyInformation);
    WVTAsn1FreeSpcLink(&OssInfo.logoImage.imageLink);
    WVTAsn1FreeSpcLink(&OssInfo.logoLink);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC SP Agency Info Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcSpAgencyInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_SP_AGENCY_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SpcSpAgencyInformation *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;
    LONG lAlignExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!WVTAsn1InfoDecodeAndAlloc(
            SpcSpAgencyInformation_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_SP_AGENCY_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(SPC_SP_AGENCY_INFO));

        pbExtra = (BYTE *) pInfo + sizeof(SPC_SP_AGENCY_INFO);
    }

    if (pOssInfo->bit_mask & policyInformation_present) {
        if (!WVTAsn1GetSpcLinkPointer(&pOssInfo->policyInformation, dwFlags,
                &pInfo->pPolicyInformation, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
    }

    if (pOssInfo->bit_mask & policyDisplayText_present) {
        WVTAsn1GetSpcString(&pOssInfo->policyDisplayText, dwFlags,
            &pInfo->pwszPolicyDisplayText, &pbExtra, &lRemainExtra);
    }

    if (pOssInfo->bit_mask & logoImage_present) {
        PSPC_IMAGE pImage;
        SpcImage *pOssImage = &pOssInfo->logoImage;

        lAlignExtra = INFO_LEN_ALIGN(sizeof(SPC_IMAGE));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pImage = (PSPC_IMAGE) pbExtra;
            memset(pImage, 0, sizeof(SPC_IMAGE));
            pInfo->pLogoImage = pImage;
            pbExtra += lAlignExtra;
        } else
            pImage = NULL;

        if (pOssImage->bit_mask & imageLink_present) {
            if (!WVTAsn1GetSpcLinkPointer(&pOssImage->imageLink, dwFlags,
                    &pImage->pImageLink, &pbExtra, &lRemainExtra))
                goto ErrorReturn;
        }
        if (pOssImage->bit_mask & bitmap_present) {
            WVTAsn1GetOctetString(&pOssImage->bitmap, dwFlags,
                &pImage->Bitmap, &pbExtra, &lRemainExtra);
        }
        if (pOssImage->bit_mask & metafile_present) {
            WVTAsn1GetOctetString(&pOssImage->metafile, dwFlags,
                &pImage->Metafile, &pbExtra, &lRemainExtra);
        }
        if (pOssImage->bit_mask & enhancedMetafile_present) {
            WVTAsn1GetOctetString(&pOssImage->enhancedMetafile, dwFlags,
                &pImage->EnhancedMetafile, &pbExtra, &lRemainExtra);
        }
        if (pOssImage->bit_mask & gifFile_present) {
            WVTAsn1GetOctetString(&pOssImage->gifFile, dwFlags,
                &pImage->GifFile, &pbExtra, &lRemainExtra);
        }

    }

    if (pOssInfo->bit_mask & logoLink_present) {
        if (!WVTAsn1GetSpcLinkPointer(&pOssInfo->logoLink, dwFlags,
                &pInfo->pLogoLink, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1InfoFree(SpcSpAgencyInformation_PDU, pOssInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC Minimal Criteria Info Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcMinimalCriteriaInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BOOL *pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ossBoolean OssInfo = (ossBoolean) *pInfo;
    return WVTAsn1InfoEncode(
        SpcMinimalCriteria_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  SPC Minimal Criteria Info Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcMinimalCriteriaInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT BOOL *pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    ossBoolean *pOssInfo = NULL;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if ((fResult = WVTAsn1InfoDecodeAndAlloc(
            SpcMinimalCriteria_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))) {
        if (*pcbInfo < sizeof(BOOL)) {
            if (pInfo) {
                fResult = FALSE;
                SetLastError((DWORD) ERROR_MORE_DATA);
            }
        } else
            *pInfo = (BOOL) *pOssInfo;
        *pcbInfo = sizeof(BOOL);
    } else {
        if (*pcbInfo >= sizeof(BOOL))
            *pInfo = FALSE;
        *pcbInfo = 0;
    }

    WVTAsn1InfoFree(SpcMinimalCriteria_PDU, pOssInfo);

    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC Financial Criteria Info Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcFinancialCriteriaInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_FINANCIAL_CRITERIA pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    SpcFinancialCriteria OssInfo;
    OssInfo.financialInfoAvailable =
        (ossBoolean) pInfo->fFinancialInfoAvailable;
    OssInfo.meetsCriteria = (ossBoolean) pInfo->fMeetsCriteria;

    return WVTAsn1InfoEncode(
        SpcFinancialCriteria_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  SPC Financial Criteria Info Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcFinancialCriteriaInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_FINANCIAL_CRITERIA pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SpcFinancialCriteria *pOssInfo = NULL;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if ((fResult = WVTAsn1InfoDecodeAndAlloc(
            SpcFinancialCriteria_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))) {
        if (*pcbInfo < sizeof(SPC_FINANCIAL_CRITERIA)) {
            if (pInfo) {
                fResult = FALSE;
                SetLastError((DWORD) ERROR_MORE_DATA);
            }
        } else {
            pInfo->fFinancialInfoAvailable =
                (BOOL) pOssInfo->financialInfoAvailable;
            pInfo->fMeetsCriteria = (BOOL) pOssInfo->meetsCriteria;
        }
        *pcbInfo = sizeof(SPC_FINANCIAL_CRITERIA);
    } else
        *pcbInfo = 0;

    WVTAsn1InfoFree(SpcFinancialCriteria_PDU, pOssInfo);

    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC statement type attribute value Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcStatementTypeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_STATEMENT_TYPE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD cId;
    LPSTR *ppszId;
    SpcStatementType OssInfo;
    ObjectID *pOssId;

    cId = pInfo->cKeyPurposeId;
    ppszId = pInfo->rgpszKeyPurposeId;
    OssInfo.count = cId;
    OssInfo.value = NULL;

    if (cId > 0) {
        pOssId = (ObjectID *) SpcAsnAlloc(cId * sizeof(ObjectID));
        if (pOssId == NULL)
            goto ErrorReturn;
        memset(pOssId, 0, cId * sizeof(ObjectID));
        OssInfo.value = pOssId;
    }

    // Array of Object Ids
    for ( ; cId > 0; cId--, ppszId++, pOssId++) {
        if (!WVTAsn1SetObjId(*ppszId, pOssId))
            goto ErrorReturn;
    }

    fResult = WVTAsn1InfoEncode(
        SpcStatementType_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (OssInfo.value)
        SpcAsnFree(OssInfo.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC statement type attribute value Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcStatementTypeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_STATEMENT_TYPE pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SpcStatementType *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;
    LONG lAlignExtra;

    DWORD cId;
    LPSTR *ppszId;
    ObjectID *pOssId;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!WVTAsn1InfoDecodeAndAlloc(
            SpcStatementType_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_STATEMENT_TYPE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(SPC_STATEMENT_TYPE);

    cId = pOssInfo->count;
    pOssId = pOssInfo->value;
    lAlignExtra = INFO_LEN_ALIGN(cId * sizeof(LPSTR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pInfo->cKeyPurposeId = cId;
        ppszId = (LPSTR *) pbExtra;
        pInfo->rgpszKeyPurposeId = ppszId;
        pbExtra += lAlignExtra;
    } else
        ppszId = NULL;

    // Array of Object Ids
    for ( ; cId > 0; cId--, ppszId++, pOssId++) {
        WVTAsn1GetObjId(pOssId, dwFlags, ppszId, &pbExtra, &lRemainExtra);
    }

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1InfoFree(SpcStatementType_PDU, pOssInfo);
    return fResult;
}


//+-------------------------------------------------------------------------
//  SPC SP Opus info attribute value Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcSpOpusInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PSPC_SP_OPUS_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SpcSpOpusInfo OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    if (pInfo->pwszProgramName) {
        WVTAsn1SetSpcString((LPWSTR) pInfo->pwszProgramName, &OssInfo.programName);
        OssInfo.bit_mask |= programName_present;
    }

    if (pInfo->pMoreInfo) {
        if (!WVTAsn1SetSpcLink(pInfo->pMoreInfo, &OssInfo.moreInfo))
            goto ErrorReturn;
        OssInfo.bit_mask |= moreInfo_present;
    }
    if (pInfo->pPublisherInfo) {
        if (!WVTAsn1SetSpcLink(pInfo->pPublisherInfo, &OssInfo.publisherInfo))
            goto ErrorReturn;
        OssInfo.bit_mask |= publisherInfo_present;
    }

    fResult = WVTAsn1InfoEncode(
        SpcSpOpusInfo_PDU,
        &OssInfo,
        pbEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1FreeSpcLink(&OssInfo.moreInfo);
    WVTAsn1FreeSpcLink(&OssInfo.publisherInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SPC SP Opus info attribute value Encode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcSpOpusInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PSPC_SP_OPUS_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    BOOL fResult;
    SpcSpOpusInfo *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
        *pcbInfo = 0;

    if (!WVTAsn1InfoDecodeAndAlloc(
            SpcSpOpusInfo_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pOssInfo))
        goto ErrorReturn;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_SP_OPUS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(SPC_SP_OPUS_INFO));

        pbExtra = (BYTE *) pInfo + sizeof(SPC_SP_OPUS_INFO);
    }

    if (pOssInfo->bit_mask & programName_present) {
        WVTAsn1GetSpcString(&pOssInfo->programName, dwFlags,
            (LPWSTR*) &pInfo->pwszProgramName, &pbExtra, &lRemainExtra);
    }

    if (pOssInfo->bit_mask & moreInfo_present) {
        if (!WVTAsn1GetSpcLinkPointer(&pOssInfo->moreInfo, dwFlags,
                &pInfo->pMoreInfo, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
    }
    if (pOssInfo->bit_mask & publisherInfo_present) {
        if (!WVTAsn1GetSpcLinkPointer(&pOssInfo->publisherInfo, dwFlags,
                &pInfo->pPublisherInfo, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    else {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;
ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;
CommonReturn:
    WVTAsn1InfoFree(SpcSpOpusInfo_PDU, pOssInfo);
    return fResult;
}

BOOL WINAPI WVTAsn1SpcLinkEncode(   IN DWORD dwCertEncodingType,
                                    IN LPCSTR lpszStructType,
                                    IN PSPC_LINK pInfo,
                                    OUT BYTE *pbEncoded,
                                    IN OUT DWORD *pcbEncoded)
{
    BOOL fResult;
    SpcLink OssSpcLink;

    if (!(WVTAsn1SetSpcLink(pInfo, &OssSpcLink)))
    {
        goto ErrorReturn;
    }

    fResult = WVTAsn1InfoEncode(SpcLink_PDU, &OssSpcLink, pbEncoded,
                            pcbEncoded);

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    return(fResult);
}

BOOL WINAPI WVTAsn1SpcLinkDecode(IN DWORD dwCertEncodingType,
                                 IN LPCSTR lpszStructType,
                                 IN const BYTE *pbEncoded,
                                 IN DWORD cbEncoded,
                                 IN DWORD dwFlags,
                                 OUT PSPC_LINK pInfo,
                                 IN OUT DWORD *pcbInfo)
{
    BOOL fResult;
    SpcLink *pSpcLink = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
    {
        *pcbInfo = 0;
    }

    if (!(WVTAsn1InfoDecodeAndAlloc(SpcLink_PDU, pbEncoded, cbEncoded,
                                (void **)&pSpcLink)))
    {
        goto ErrorReturn;
    }

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_LINK);
    if (lRemainExtra < 0)
    {
        pbExtra = NULL;
    }
    else
    {
        pbExtra = (BYTE *) pInfo + sizeof(SPC_LINK);
    }

    if (!(WVTAsn1GetSpcLink(pSpcLink, dwFlags, pInfo, &pbExtra, &lRemainExtra)))
    {
        goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
    {
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    }
    else
    {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo)
        {
            goto LengthError;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD)ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1InfoFree(SpcLink_PDU, pSpcLink);
    return fResult;
}


BOOL WINAPI WVTAsn1SpcPeImageDataEncode(IN DWORD dwCertEncodingType,
                                        IN LPCSTR lpszStructType,
                                        IN PSPC_PE_IMAGE_DATA pInfo,
                                        OUT BYTE *pbEncoded,
                                        IN OUT DWORD *pcbEncoded)
{
    BOOL fResult;
    SpcPeImageData OssInfo;

    memset(&OssInfo, 0, sizeof(OssInfo));

    if (pInfo->Flags.cbData)
    {
        // SpcPeImageFlags has its own definition. It has a default
        // bit (includeResources). Therefore, can't use the default BITSTRING.
        // Note: BITSTRING's length is an unsigned int, while SpcPeImageFlags's
        // length is an unsigned short.
        BITSTRING OssBitString;
        WVTAsn1SetBit(&pInfo->Flags, &OssBitString);
        OssInfo.flags.length = (WORD)OssBitString.length;
        OssInfo.flags.value = OssBitString.value;
        OssInfo.bit_mask |= flags_present;
    }

    if (pInfo->pFile)
    {
        if (!WVTAsn1SetSpcLink(pInfo->pFile, &OssInfo.file))
        {
            goto ErrorReturn;
        }

        OssInfo.bit_mask |= file_present;
    }

    fResult = WVTAsn1InfoEncode(SpcPeImageData_PDU, &OssInfo, pbEncoded,
                            pcbEncoded);

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1FreeSpcLink(&OssInfo.file);
    return(fResult);
}

//+-------------------------------------------------------------------------
//  SPC Portable Executable (PE) Image Attribute Value Decode (OSS X509)
//--------------------------------------------------------------------------
BOOL WINAPI WVTAsn1SpcPeImageDataDecode(IN DWORD dwCertEncodingType,
                                        IN LPCSTR lpszStructType,
                                        IN const BYTE *pbEncoded,
                                        IN DWORD cbEncoded,
                                        IN DWORD dwFlags,
                                        OUT PSPC_PE_IMAGE_DATA pInfo,
                                        IN OUT DWORD *pcbInfo)
{
    BOOL fResult;
    SpcPeImageData *pOssInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
    {
        *pcbInfo = 0;
    }

    if (!WVTAsn1InfoDecodeAndAlloc(SpcPeImageData_PDU, pbEncoded, cbEncoded,
                                (void **)&pOssInfo))
    {
        goto ErrorReturn;
    }

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_PE_IMAGE_DATA);
    if (lRemainExtra < 0)
    {
        pbExtra = NULL;
    }
    else
    {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(SPC_PE_IMAGE_DATA));

        pbExtra = (BYTE *) pInfo + sizeof(SPC_PE_IMAGE_DATA);
    }

    if (pOssInfo->bit_mask & flags_present)
    {
        // See above encode for why we need to do this extra indirect step
        BITSTRING OssBitString;
        OssBitString.length = pOssInfo->flags.length;
        OssBitString.value = pOssInfo->flags.value;
        WVTAsn1GetBit(&OssBitString, dwFlags,
            &pInfo->Flags, &pbExtra, &lRemainExtra);
    }

    if (pOssInfo->bit_mask & file_present)
    {
        if (!WVTAsn1GetSpcLinkPointer(&pOssInfo->file, dwFlags,
                &pInfo->pFile, &pbExtra, &lRemainExtra))
        {
            goto ErrorReturn;
        }
    }

    if (lRemainExtra >= 0)
    {
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    }
    else
    {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo)
        {
            goto LengthError;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1InfoFree(SpcPeImageData_PDU, pOssInfo);
    return(fResult);
}

BOOL WINAPI WVTAsn1SpcSigInfoEncode(DWORD dwCertEncodingType, LPCSTR lpszStructType,
                                    PSPC_SIGINFO pInfo, BYTE *pbEncoded,
                                    DWORD *pcbEncoded)
{
    BOOL fResult;
    SpcSigInfo OssSpcSigInfo;

    if (!(WVTAsn1SetSpcSigInfo(pInfo, &OssSpcSigInfo)))
    {
        goto ErrorReturn;
    }

    fResult = WVTAsn1InfoEncode(SpcSigInfo_PDU, &OssSpcSigInfo, pbEncoded,
                            pcbEncoded);

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    return(fResult);
}

BOOL WINAPI WVTAsn1SpcSigInfoDecode(DWORD dwCertEncodingType, LPCSTR lpszStructType,
                                    const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags,
                                    PSPC_SIGINFO pInfo, OUT DWORD *pcbInfo)
{
    BOOL        fResult;
    SpcSigInfo  *pSpcSigInfo = NULL;
    BYTE        *pbExtra;
    LONG        lRemainExtra;

    if (pInfo == NULL)
    {
        *pcbInfo = 0;
    }

    if (!(WVTAsn1InfoDecodeAndAlloc(SpcSigInfo_PDU, pbEncoded, cbEncoded, (void **)&pSpcSigInfo)))
    {
        goto ErrorReturn;
    }

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(SPC_SIGINFO);
    if (lRemainExtra < 0)
    {
        pbExtra = NULL;
    }
    else
    {
        pbExtra = (BYTE *) pInfo + sizeof(SPC_SIGINFO);
    }

    if (!(WVTAsn1GetSpcSigInfo(pSpcSigInfo, dwFlags, pInfo, &pbExtra, &lRemainExtra)))
    {
        goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
    {
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    }
    else
    {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo)
        {
            goto LengthError;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD)ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1InfoFree(SpcSigInfo_PDU, pSpcSigInfo);
    return fResult;
}

BOOL WVTAsn1SetCatNameValue(IN PCAT_NAMEVALUE pInfo, OUT NameValue *pOss)
{
    memset(pOss, 0x00, sizeof(*pOss));


    //  tag!
    WVTAsn1SetBMP(pInfo->pwszTag, &pOss->refname);

    //  flags
    pOss->typeaction = (int)pInfo->fdwFlags;

    //  value
    WVTAsn1SetOctetString(&pInfo->Value, &pOss->value);

    return(TRUE);
}

BOOL WVTAsn1SetCatMemberInfo(IN PCAT_MEMBERINFO pInfo, OUT MemberInfo *pOss)
{
    memset(pOss, 0x00, sizeof(*pOss));


    //  subject guid (wide text)
    WVTAsn1SetBMP(pInfo->pwszSubjGuid, &pOss->subguid);

    // cert version
    pOss->certversion = (int)pInfo->dwCertVersion;


    return(TRUE);
}


BOOL WVTAsn1GetCatNameValue(IN NameValue *pOss, IN DWORD dwFlags,
                                    OUT PCAT_NAMEVALUE pInfo, IN OUT BYTE **ppbExtra,
                                    IN OUT LONG *plRemainExtra)
{
    if (*plRemainExtra >= 0)
    {
        memset(pInfo, 0, sizeof(*pInfo));
    }

    WVTAsn1GetOctetString(&pOss->value, dwFlags,
                          &pInfo->Value, ppbExtra, plRemainExtra);

    if (*plRemainExtra >= 0)
    {
        pInfo->fdwFlags = (DWORD)pOss->typeaction;
    }

    WVTAsn1GetBMP(&pOss->refname, dwFlags, &pInfo->pwszTag, ppbExtra, plRemainExtra);

    return(TRUE);
}

BOOL WVTAsn1GetCatMemberInfo(IN MemberInfo *pOss, IN DWORD dwFlags,
                                    OUT PCAT_MEMBERINFO pInfo, IN OUT BYTE **ppbExtra,
                                    IN OUT LONG *plRemainExtra)
{
    if (*plRemainExtra >= 0)
    {
        memset(pInfo, 0, sizeof(*pInfo));
    }

    WVTAsn1GetBMP(&pOss->subguid, dwFlags, &pInfo->pwszSubjGuid, ppbExtra, plRemainExtra);

    if (*plRemainExtra >= 0)
    {
        pInfo->dwCertVersion = pOss->certversion;
    }


    return(TRUE);
}


BOOL WINAPI WVTAsn1CatNameValueEncode(   IN DWORD dwCertEncodingType,
                                    IN LPCSTR lpszStructType,
                                    IN PCAT_NAMEVALUE pInfo,
                                    OUT BYTE *pbEncoded,
                                    IN OUT DWORD *pcbEncoded)
{
    BOOL fResult;
    NameValue   OssNameValue;

    if (!(WVTAsn1SetCatNameValue(pInfo, &OssNameValue)))
    {
        goto ErrorReturn;
    }

    fResult = WVTAsn1InfoEncode(NameValue_PDU, &OssNameValue, pbEncoded,
                            pcbEncoded);

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    return(fResult);
}

BOOL WINAPI WVTAsn1CatMemberInfoEncode( IN DWORD dwCertEncodingType,
                                        IN LPCSTR lpszStructType,
                                        IN PCAT_MEMBERINFO pInfo,
                                        OUT BYTE *pbEncoded,
                                        IN OUT DWORD *pcbEncoded)
{
    BOOL fResult;
    MemberInfo OssMemberInfo;

    if (!(WVTAsn1SetCatMemberInfo(pInfo, &OssMemberInfo)))
    {
        goto ErrorReturn;
    }

    fResult = WVTAsn1InfoEncode(MemberInfo_PDU, &OssMemberInfo, pbEncoded,
                            pcbEncoded);

    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;

CommonReturn:
    return(fResult);
}


BOOL WINAPI WVTAsn1CatNameValueDecode(IN DWORD dwCertEncodingType,
                                 IN LPCSTR lpszStructType,
                                 IN const BYTE *pbEncoded,
                                 IN DWORD cbEncoded,
                                 IN DWORD dwFlags,
                                 OUT PCAT_NAMEVALUE pInfo,
                                 IN OUT DWORD *pcbInfo)
{
    BOOL fResult;
    NameValue   *pNameValue = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
    {
        *pcbInfo = 0;
    }

    if (!(WVTAsn1InfoDecodeAndAlloc(NameValue_PDU, pbEncoded, cbEncoded,
                                (void **)&pNameValue)))
    {
        goto ErrorReturn;
    }

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CAT_NAMEVALUE);
    if (lRemainExtra < 0)
    {
        pbExtra = NULL;
    }
    else
    {
        pbExtra = (BYTE *) pInfo + sizeof(CAT_NAMEVALUE);
    }

    if (!(WVTAsn1GetCatNameValue(pNameValue, dwFlags, pInfo, &pbExtra, &lRemainExtra)))
    {
        goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
    {
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    }
    else
    {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo)
        {
            goto LengthError;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD)ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1InfoFree(NameValue_PDU, pNameValue);
    return fResult;
}


BOOL WINAPI WVTAsn1CatMemberInfoDecode( IN DWORD dwCertEncodingType,
                                        IN LPCSTR lpszStructType,
                                        IN const BYTE *pbEncoded,
                                        IN DWORD cbEncoded,
                                        IN DWORD dwFlags,
                                        OUT PCAT_MEMBERINFO pInfo,
                                        IN OUT DWORD *pcbInfo)
{
    BOOL fResult;
    MemberInfo   *pMemberInfo = NULL;
    BYTE *pbExtra;
    LONG lRemainExtra;

    if (pInfo == NULL)
    {
        *pcbInfo = 0;
    }

    if (!(WVTAsn1InfoDecodeAndAlloc(MemberInfo_PDU, pbEncoded, cbEncoded,
                                (void **)&pMemberInfo)))
    {
        goto ErrorReturn;
    }

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbInfo - sizeof(CAT_MEMBERINFO);
    if (lRemainExtra < 0)
    {
        pbExtra = NULL;
    }
    else
    {
        pbExtra = (BYTE *) pInfo + sizeof(CAT_MEMBERINFO);
    }

    if (!(WVTAsn1GetCatMemberInfo(pMemberInfo, dwFlags, pInfo, &pbExtra, &lRemainExtra)))
    {
        goto ErrorReturn;
    }

    if (lRemainExtra >= 0)
    {
        *pcbInfo = *pcbInfo - (DWORD) lRemainExtra;
    }
    else
    {
        *pcbInfo = *pcbInfo + (DWORD) -lRemainExtra;
        if (pInfo)
        {
            goto LengthError;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD)ERROR_MORE_DATA);
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbInfo = 0;
    fResult = FALSE;

CommonReturn:
    WVTAsn1InfoFree(MemberInfo_PDU, pMemberInfo);
    return fResult;
}
