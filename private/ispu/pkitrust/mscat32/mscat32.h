//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mscat32.h
//
//  History:    25-Apr-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef MSCAT32_H
#define MSCAT32_H

#ifdef __cplusplus
extern "C"
{
#endif

extern CRITICAL_SECTION     MSCAT_CriticalSection;
extern HINSTANCE            hInst;

extern BOOL     CatalogFreeMember(CRYPTCATMEMBER *pCatMember);
extern BOOL     CatalogFreeAttribute(CRYPTCATATTRIBUTE *pCatMember);

extern void     *CatalogNew(DWORD cbSize);
extern BOOL     CatalogCheckForDuplicateMember(Stack_ *pMembers, WCHAR *pwszReferenceTag);

extern BOOL     CatalogSaveP7UData(CRYPTCATSTORE *pCat);
extern BOOL     CatalogSaveP7SData(CRYPTCATSTORE *pCat, CTL_CONTEXT *pCTLContext);
extern BOOL     CatalogLoadFileData(CRYPTCATSTORE *pCat);


extern BOOL     CatalogEncodeNameValue(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr,
                                       PCRYPT_ATTRIBUTE pCryptAttr);
extern BOOL     CatalogDecodeNameValue(CRYPTCATSTORE *pCatStore, PCRYPT_ATTRIBUTE pCryptAttr,
                                       CRYPTCATATTRIBUTE *pCatAttr);

extern BOOL     CatalogEncodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                        PCRYPT_ATTRIBUTE pCryptAttr);
extern BOOL     CatalogDecodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                        CRYPT_ATTRIBUTE *pAttr);
extern BOOL     CatalogReallyDecodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                              CRYPT_ATTR_BLOB *pAttr);

extern BOOL     CatalogEncodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                          PCRYPT_ATTRIBUTE pCryptAttr);
extern BOOL     CatalogDecodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                          CRYPT_ATTRIBUTE *pAttr);
extern BOOL     CatalogReallyDecodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember,
                                                CRYPT_ATTR_BLOB *pAttr);


extern BOOL     CatAdminDllMain(HANDLE hInstDLL,DWORD fdwReason,LPVOID lpvReserved);

extern void     CatalogCertExt2CryptAttr(CERT_EXTENSION *pCertExt, CRYPT_ATTRIBUTE *pCryptAttr);
extern void     CatalogCryptAttr2CertExt(CRYPT_ATTRIBUTE *pCryptAttr, CERT_EXTENSION *pCertExt);


LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember);

LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember, BOOL fContinueOnError,
                                       LPVOID pvReserved);

CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumAttributesWithCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszMemberTag, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);

BOOL MsCatConstructHashTag (IN DWORD cbDigest, IN LPBYTE pbDigest, OUT LPWSTR* ppwszHashTag);

VOID MsCatFreeHashTag (IN LPWSTR pwszHashTag);


#ifdef __cplusplus
}
#endif


#endif // MSCAT32_H

