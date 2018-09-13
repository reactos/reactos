/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    PKCSlib

Abstract:

    This header file describes the services and definitions necessary to use the
    Crypto Certificate API.

Author:

    Doug Barlow (dbarlow) 8/17/1995

Environment:

    Win32, Crypto API

Notes:

    Current X.509 Support Level : V3
    Current PKCS Support Level  : V1

--*/

#ifndef _PKCSLIB_H_
#define _PKCSLIB_H_

#include <wincrypt.h>

#ifdef _cplusplus
extern "C" {
#endif

#ifndef PKCSDLLAPI
#define PKCSDLLAPI
#endif

typedef const void *
    PKCSHANDLE;


//
//==============================================================================
//
//  Attribute List services.
//
//  Also see the list of standard Attribute types, below.
//

typedef PKCSHANDLE
    ATTRIBLISTHANDLE;               // Reference handle type.
typedef ATTRIBLISTHANDLE
    *PATTRIBLISTHANDLE,             // Pointers to reference handles.
    *LPATTRIBLISTHANDLE;

extern PKCSDLLAPI BOOL WINAPI
PkcsAttributeListCreate(
    OUT LPATTRIBLISTHANDLE hAtrList); // Handle for future reference.

extern PKCSDLLAPI BOOL WINAPI
PkcsAttributeListClose(
    IN ATTRIBLISTHANDLE hAtrList);  // The handle to the attrList to discard

extern PKCSDLLAPI BOOL WINAPI
PkcsAttributeListAdd(
    IN ATTRIBLISTHANDLE hAtrList,   // The reference handle to the List.
    IN LPCTSTR szAtrType,           // The Object Identifier of the attribute
    IN const BYTE * pbAtrValue);    // The Value of the ASN.1 encoded attribute

extern PKCSDLLAPI BOOL WINAPI
PkcsAttributeListLookup(
    IN ATTRIBLISTHANDLE hAtrList,   // The reference handle to the List.
    IN LPCTSTR szAtrType,           // The Object Identifier of the attribute
    OUT LPBYTE pbAtrValue,          // The value of the attribute
    IN OUT LPDWORD pcbAtrValLen);   // The length of the pbAtrValue buffer

extern PKCSDLLAPI BOOL WINAPI
PkcsAttributeListContents(
    IN ATTRIBLISTHANDLE hAtrList,   // The reference handle to the List.
    OUT LPTSTR mszAtrTypes,         // The Object Identifier list
    IN OUT LPDWORD pcbAtrTypesLen); // The length of the mszAtrTypes buffer

    //
    // ?Q? - Is there any need for a remove service?
    //


//
//==============================================================================
//
//  X.509 v3 Certificate Extension List services.
//
// ?TODO?
//

typedef PKCSHANDLE
    EXTENSIONLISTHANDLE;            // Reference handle type.
typedef EXTENSIONLISTHANDLE
    *PEXTENSIONLISTHANDLE,          // Pointers to reference handles.
    *LPEXTENSIONLISTHANDLE;


//
//==============================================================================
//
//  Subject services.  To use these services, you must have created a
//      key, either directly via the CryptoAPI, or via the
//      convenience service PkcsCreateSubject.
//

typedef PKCSHANDLE
    SUBJECTHANDLE;                  // Reference handle type.
typedef SUBJECTHANDLE
    *PSUBJECTHANDLE,                // Pointers to reference handles.
    *LPSUBJECTHANDLE;

extern PKCSDLLAPI BOOL WINAPI
PkcsSetDNamePrefix(
    IN DWORD dwStore,               // The Certificate Store.
    IN LPCTSTR szPrefix);           // The Prefix to set.

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectCreate(
    OUT LPSUBJECTHANDLE phSubject,  // Handle for future reference
    IN LPCTSTR szKeySet,            // What to name the new Subject keyset
    IN LPCTSTR szProvider,          // The specific name of the CSP, or Blank
    IN DWORD dwKeyType,             // Specifies the type of key
    IN DWORD dwProvType,            // Should be PROV_RSA_FULL
    IN ALG_ID algPref,              // Suggest optional algorithm preferences
    IN DWORD dwStore);              // Store Id or zero.

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectOpen(
    OUT LPSUBJECTHANDLE phSubject,  // Handle for future reference
    IN LPCTSTR szKeySet,            // The name of the Subject keyset
    IN LPCTSTR szProvider,          // The specific name of the CSP, or Blank
    IN DWORD dwKeyType,             // Specifies the type of key
    IN DWORD dwProvType,            // Should be PROV_RSA_FULL
    IN ALG_ID algPref,              // Suggest optional algorithm preferences
    IN DWORD dwStore);              // Store Id or zero.

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectSign(
    IN SUBJECTHANDLE hSubject,      // The reference handle to the Subject
    IN const BYTE *pbData,          // The data to be signed
    IN DWORD cbDataLen,             // The length of the data to be signed
    IN LPCTSTR szComment,           // Comment string associated with signature
    OUT LPBYTE pbSignature,         // Buffer to receive the signature
    IN OUT LPDWORD pcbSigLen);      // Length of the pbSignature buffer.

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectClose(
    IN SUBJECTHANDLE hSubject);     // The reference handle to the Subject

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectDelete(
    IN SUBJECTHANDLE hSubject);     // The handle to the Subject to remove

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectRequestCertification(
    IN SUBJECTHANDLE hSubject,      // The reference handle to the Subject
    IN ATTRIBLISTHANDLE hAtrList,   // reference to subject attributes, if any
    OUT LPBYTE pbCertReq,           // Buffer to receive certificate request
    IN OUT LPDWORD pcbCertReqLen);  // Length of pbCertReq buffer

extern PKCSDLLAPI BOOL WINAPI
PkcsSubjectDistinguishedName(
    IN SUBJECTHANDLE hSubject,      // The reference handle to the Subject
    OUT LPTSTR szDname,             // Buffer to receive the distinguished name
    IN OUT LPDWORD pcbDnameLen);    // Length of pbCertReq buffer

    //
    // ?TODO? - Need to attach an X.509 v2 UniqueIdentifier to the Subject.
    // ?HOW? -  Can we attach an X.509 v2 UniqueIdentifier to the request?
    //


//
//==============================================================================
//
//  Issuer services.  To use these services, you must have created an
//      AT_SIGNATURE key, either directly via the CryptoAPI, or via the
//      convienience service PkcsCreateIssuer, and you will be certifying
//      other's keys.
//

typedef PKCSHANDLE
    ISSUERHANDLE;                   // Reference handle type.
typedef ISSUERHANDLE
    *PISSUERHANDLE,                 // Pointers to reference handles.
    *LPISSUERHANDLE;

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerCreate(
    OUT LPISSUERHANDLE phIssuer,    // Handle for future reference
    IN LPCTSTR szKeySet,            // What to name the new Issuer keyset
    IN LPCTSTR szProvider,          // The specific name of the CSP, or Blank
    IN DWORD dwProvType,            // Should be PROV_RSA_FULL
    IN ALG_ID algPref,              // Suggest optional algorithm preferences
    IN DWORD dwStore);              // Store Id or zero.

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerOpen(
    OUT LPISSUERHANDLE phIssuer,    // Handle for future reference
    IN LPCTSTR szKeySet,            // The name of the Issuer keyset
    IN LPCTSTR szProvider,          // The specific name of the CSP, or Blank
    IN DWORD dwProvType,            // Should be PROV_RSA_FULL
    IN ALG_ID algPref,              // Suggest optional algorithm preferences
    IN DWORD dwStore);              // Store Id or zero.

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerClose(
    IN ISSUERHANDLE hIssuer);       // The reference handle to the Issuer

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerDelete(
    IN ISSUERHANDLE hIssuer);       // The handle to the Issuer to remove

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerRequestCertification(
    IN ISSUERHANDLE hIssuer,        // The reference handle to the Issuer
    IN ATTRIBLISTHANDLE hAtrList,   // reference to issuer attributes, if any
    OUT LPBYTE pbCertReq,           // Buffer to receive the certificate request
    IN OUT LPDWORD pcbCertReqLen);  // Length of the pbCertReq buffer

    //
    // ?TODO? - Need to attach an X.509 v2 UniqueIdentifier to the Issuer.
    //          It would be nice to get at the X.509 name.  Other info?
    // ?HOW? -  How can we attach an X.509 v2 UniqueIdentifier to the request?
    //

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerIssueLocalCA(
    IN ISSUERHANDLE hIssuer,        // The reference handle to the Issuer
    OUT LPBYTE pbCert,              // Buffer to receive certificate
    IN OUT LPDWORD pcbCertLen);     // Length of the pbCert buffer.

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerCertify(
    IN ISSUERHANDLE hIssuer,        // The reference handle to the Issuer
    IN const BYTE *pbCertReq,       // Buffer containing the certificate request
    IN const BYTE *pbSerialNo,      // Serial number to assign to certificate
    IN DWORD cbSerialNoLen,         // Length of the serial number
    IN LPFILETIME pftStartDate,     // Effective date of the certificate
    IN LPFILETIME pftEndDate,       // Termination date of the certificate
    OUT LPBYTE pbCert,              // Buffer to receive the certificate
    IN OUT LPDWORD pcbCertLen);     // Length of the pbCert buffer.

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerRecertify(
    IN ISSUERHANDLE hIssuer,        // The reference handle to the Issuer
    IN const BYTE *pbInCert,        // Buffer containing the old certificate
    IN const BYTE *pbSerialNo,      // Serial number to assign to certificate
    IN DWORD cbSerialNoLen,         // Length of the serial number
    IN LPFILETIME pftStartDate,     // Effective date of the certificate
    IN LPFILETIME pftEndDate,       // Termination date of the certificate
    OUT LPBYTE pbOutCert,           // Buffer to receive the certificate
    IN OUT LPDWORD pcbCertLen);     // Length of the pbCert buffer.

extern PKCSDLLAPI BOOL WINAPI
PkcsIssuerDistinguishedName(
    IN ISSUERHANDLE hIssuer,        // The reference handle to the Issuer
    OUT LPTSTR szDname,             // Buffer to receive the distinguished name
    IN OUT LPDWORD pcbDnameLen);    // Length of pbCertReq buffer

    //
    // ?HOW? -  How do we get the Issuer's UniqueIdentifier?
    //          How do we attach X.509 v3 Extensions to the certificate?
    //


//
//==============================================================================
//
//  CRL services.
//

typedef PKCSHANDLE
    CRLHANDLE;                      // Reference handle type.
typedef CRLHANDLE
    *PCRLHANDLE,                    // Pointers to reference handles.
    *LPCRLHANDLE;

extern PKCSDLLAPI BOOL WINAPI
PkcsCrlCreate(
    OUT LPCRLHANDLE phCrl,          // Handle for future reference
    IN ISSUERHANDLE hIssuer);       // Handle of controlling issuer

extern PKCSDLLAPI BOOL WINAPI
PkcsCrlLoad(
    OUT LPCRLHANDLE phCrl,          // Handle for future reference
    IN ISSUERHANDLE hIssuer,        // Handle of controlling issuer
    IN const BYTE *pbCrl);          // Buffer containing the CRL.

extern PKCSDLLAPI BOOL WINAPI
PkcsCrlRevoke(
    IN CRLHANDLE hCrl,              // The reference handle to the CRL
    IN const BYTE *pbSerialNo,      // Serial number of certificate to revoke
    IN DWORD cbSerialNoLen,         // Length of the serial number
    IN LPFILETIME pfmStartDate);    // Effective date of revokation

extern PKCSDLLAPI BOOL WINAPI
PkcsCrlIssue(
    IN CRLHANDLE hCrl,              // The reference handle to the CRL
    IN LPFILETIME pftEndDate,       // Termination date of the CRL
    OUT LPBYTE pbCrl,               // Buffer to receive the CRL
    IN OUT LPDWORD pcbCrlLen);      // Length of the pbCrl buffer

extern PKCSDLLAPI BOOL WINAPI
PkcsCrlClose(
    IN CRLHANDLE hCrl);             // The reference handle to the Crl

    //
    // ?HOW? -  How do we attach X.509 CRL v2 Extensions to the revokee?
    //


//
//==============================================================================
//
//  Certificate services.
//

typedef PKCSHANDLE
    CERTIFICATEHANDLE;              // Reference handle type.
typedef CERTIFICATEHANDLE
    *PCERTIFICATEHANDLE,            // Pointers to reference handles.
    *LPCERTIFICATEHANDLE;

#define CERT_PKCSV1_INFO 1          // The type of Cert Info Struct following:

//
// Supported Certificate Types.
//

#define CERTYPE_UNKNOWN         0   // Unknown Certificate Type.
#define CERTYPE_LOCAL_CA        1   // A local CA pointer.
#define CERTYPE_X509            2   // An X.509 certificate.
#define CERTYPE_PKCS_X509       3   // A PKCS & imbedded X.509 Certificate.
#define CERTYPE_PKCS_REQUEST    4   // A PKCS Certificate Request (internal use)


//
// Supported Certificate Types.
//

// Local CA Specifics

#define LCA_VERSION_1 0             // This Local CA is version 1.
#define LCA_MAX_VERSION LCA_VERSION_1 // Max version supported.

typedef struct {
    DWORD   dwVersion;              // The version of the local CA
    LPTSTR  szSubject;              // Address for Subject name
    DWORD   cbSubjectLen;           // Length of szSubject buffer
    LPTSTR  szProvider;             // Address for the provider name
    DWORD   cbProviderLen;          // Length of szProvider buffer
    DWORD   dwProvType;             // The type of Provider
    LPTSTR  szKeyset;               // Address for the keyset name
    DWORD   cbKeysetLen;            // Length of the szKeyset buffer
    DWORD   dwKeySpec;              // The specific key identifier
} LOCALCACERTINFO, *PLOCALCACERTINFO, *LPLOCALCACERTINFO;


// X.509 Certificate specifics

#define X509_VERSION_1 0            // This certificate is X.509 version 1
#define X509_VERSION_2 1            // This certificate is X.509 version 2
#define X509_VERSION_3 2            // This certificate is X.509 version 3
#define X509_MAX_VERSION X509_VERSION_1 // Max version supported.

typedef struct {
    DWORD   dwX509Version;          // The version of the certificate
    LPBYTE  pbSerialNumber;         // Address for serial number.
    DWORD   cbSerialNumLen;         // Length of pbSerialNumber buffer.
    ALG_ID  algId;                  // Algorithm Id.
    LPTSTR  szIssuer;               // Address for Issuer name
    DWORD   cbIssuerLen;            // Length of szIssuer buffer
    FILETIME ftNotBefore;           // Certificate effective date
    FILETIME ftNotAfter;            // Certificate expiration date
    LPTSTR  szSubject;              // Address for Subject name
    DWORD   cbSubjectLen;           // Length of szSubject buffer
    LPVOID  pvIssuerUid;            // Address for Issuer Id ?q?
    DWORD   cbIssuerUidLen;         // Length of pvIssuerUid buffer
    LPVOID  pvSubjectUid;           // Address for Subject Id ?q?
    DWORD   cbSubjectUidLen;        // Length of pvSubjectUid buffer
    EXTENSIONLISTHANDLE
            hExtensions;            // Extension List handle
} X509CERTINFO, *PX509CERTINFO, *LPX509CERTINFO;


// PKCS-6 with embedded X.509 Certificate specifics

#define PKCS_NOTUSED   0xffff       // PKCS isn't used on this certificate
#define PKCS_VERSION_1 0            // This certificate is PKCS version 1
#define PKCS_MAX_VERSION PKCS_VERSION_1 // Max version supported.

typedef struct {
    DWORD   dwPKCSVersion;          // The version of the certificate
    ATTRIBLISTHANDLE hAttributes;   // Attribute list handle
    X509CERTINFO x509Info;          // Info from the X.509 Certificate
} PKCSX509CERTINFO, *PPKCSX509CERTINFO, *LPPKCSX509CERTINFO;


// PKCS-10 Certificate Request Specifics
typedef struct {
    DWORD   dwPKCSVersion;          // The version of the certificate request
    LPTSTR  szSubject;              // Address for Subject name
    DWORD   cbSubjectLen;           // Length of szSubject buffer
    ATTRIBLISTHANDLE hAttributes;   // Attribute list handle
} PKCSREQCERTINFO, *PPKCSREQCERTINFO, *LPPKCSREQCERTINFO;


// Common Certificate Info Header.

typedef struct {
    // This part is common to all certificate info structure types.  (?Q?)
    DWORD   cbStructLen;            // Length of this structure
    WORD    wCertInfoVersion;       // The version (CERT_PKCSV1_INFO)
    WORD    wCertInfoType;          // The type of the following structure
    union {
        LOCALCACERTINFO localCA;    // Local CA Characteristics
        X509CERTINFO x509;          // X.509 Characteristics
        PKCSX509CERTINFO pkcs;      // PKCS-6 Characteristics
        PKCSREQCERTINFO req;        // PKCS-10 Request Characteristics
    } certInfo;
} CERTIFICATEINFO, *PCERTIFICATEINFO, *LPCERTIFICATEINFO;


// Crypto API Definitions
#define CAPI_MAX_VERSION 2          // Supported version of CAPI.


// Certificate Store Definitions
#define CERTSTORE_NONE          0   // No store to be used.
#define CERTSTORE_APPLICATION   1   // Store in application volatile memory
#define CERTSTORE_CURRENT_USER  3   // Store in Registry under current user
#define CERTSTORE_LOCAL_MACHINE 5   // Store in Registry under local machine


// Certificate Warning Definitions
#define CERTWARN_NOCRL       0x01   // At least one of the signing CAs didn't
                                    // have an associated CRL.
#define CERTWARN_EARLYCRL    0x02   // At least one of the signing CAs had an
                                    // associated CRL who's issuing date was
                                    // in the future.
#define CERTWARN_LATECRL     0x04   // At least one of the signing CAs had an
                                    // expired CRL.
#define CERTWARN_TOBEREVOKED 0x08   // At least one of the signing CAs contained
                                    // a revocation for a certificate, but its
                                    // effective date has not yet been reached.

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateLoad(
    OUT LPCERTIFICATEHANDLE phCert, // Handle for future reference
    IN const BYTE *pbCert,          // Buffer containing the certificate
    IN const BYTE *pbCrl,           // Buffer containing any associated CRL
    IN OUT LPDWORD pdwType,         // Certificate Type
    IN DWORD dwStore,               // Which certificate store to load
    IN LPCTSTR szKeySet,            // The name of the keyset to use
    IN LPCTSTR szProvider,          // The specific name of the CSP to use
    IN DWORD dwProvType,            // Provider type hint
    OUT LPBYTE szIssuerName,        // The root or missing issuer
    IN OUT LPDWORD pcbIssuerLen,    // Length of the szIssuerName buffer
    OUT LPDWORD pdwWarnings);       // Receives warning flags.

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateOpen(
    OUT LPCERTIFICATEHANDLE phCert, // Handle for future reference
    IN LPCTSTR szSubjName,          // Name of subject of existing certificate
    IN LPCTSTR szKeySet,            // The name of the keyset to use
    IN LPCTSTR szProvider,          // The specific name of the CSP to use
    IN DWORD dwProvType,            // Provider type hint
    OUT LPDWORD pdwCertType,        // Certificate Type
    IN OUT LPDWORD pfStore,         // Certificate store search/found limits
    OUT LPTSTR szIssuerName,        // The root or missing issuer
    IN OUT LPDWORD pcbIssuerLen,    // Length of the szIssuerName buffer
    OUT LPDWORD pdwWarnings);       // Receives warning flags.

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateUpdateCrl(
    IN CERTIFICATEHANDLE hCert,     // The reference handle to the Certificate
    IN const BYTE *pbCrl);          // Buffer containing the associated CRL

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateVerify(
    IN CERTIFICATEHANDLE hCert,     // The reference handle to the Certificate
    IN const BYTE *pbData,          // The data to be verified
    IN DWORD cbDataLen,             // The length of the data to be signed
    IN LPCTSTR szComment,           // Comment string associated with signature
    IN ALG_ID algId,                // Algorithm suggestion
    IN const BYTE *pbSignature,     // The supplied signature
    IN DWORD cbSigLen);             // Length of the pbSignature buffer.

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateGetInfo(
    IN CERTIFICATEHANDLE hCert,     // The reference handle to the Certificate
    IN OUT LPCERTIFICATEINFO pCertInfo); // The info structure to fill in

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateClose(
    IN CERTIFICATEHANDLE hCert);    // The reference handle to the Certificate

extern PKCSDLLAPI BOOL WINAPI
PkcsCertificateDelete(
    IN CERTIFICATEHANDLE hCert);    // The handle to the Certificate to remove


#if defined(_MSVC) && defined(_DEBUG)
//
//==============================================================================
//
//  Debugging extensions
//

extern PKCSDLLAPI void WINAPI
PkcsMemoryClean(
    void);
#endif


//
//==============================================================================
//
//  Attribute Type definitions
//

#define X500_commonName                     TEXT("2.5.4.3")
#define X500_surname                        TEXT("2.5.4.4")
#define X500_serialNumber                   TEXT("2.5.4.5")
#define X500_countryName                    TEXT("2.5.4.6")
#define X500_locality                       TEXT("2.5.4.7")
#define X500_stateOrProvinceName            TEXT("2.5.4.8")
#define X500_streetAddress                  TEXT("2.5.4.9")
#define X500_organizationName               TEXT("2.5.4.10")
#define X500_orginazationalUnitName         TEXT("2.5.4.11")
#define X500_title                          TEXT("2.5.4.12")
#define X500_description                    TEXT("2.5.4.13")
#define X500_businessCategory               TEXT("2.5.4.15")
#define X500_postalCode                     TEXT("2.5.4.17")
#define X500_postOfficeBox                  TEXT("2.5.4.18")
#define X500_physicalDeliveryOfficeName     TEXT("2.5.4.19")
#define X500_telephoneNumber                TEXT("2.5.4.20")
#define X500_x121Address                    TEXT("2.5.4.24")
#define X500_internationalISDNNumber        TEXT("2.5.4.25")
#define X500_destinationIndicator           TEXT("2.5.4.27")

#define PKCS1_md2                           TEXT("1.2.840.113549.2.2")
#define PKCS1_md4                           TEXT("1.2.840.113549.2.4")
#define PKCS1_md5                           TEXT("1.2.840.113549.2.5")
#define PKCS1_rsaEncryption                 TEXT("1.2.840.113549.1.1.1")
#define PKCS1_md2WithRSAEncryption          TEXT("1.2.840.113549.1.1.2")
#define PKCS1_md4WithRSAEncryption          TEXT("1.2.840.113549.1.1.3")
#define PKCS1_md5WithRSAEncryption          TEXT("1.2.840.113549.1.1.4")

#define PKCS3_dhKeyAgreement                TEXT("1.2.840.113549.1.3.1")

#define PKCS5_pbeWithMD2AndDES_CBC          TEXT("1.2.840.113549.1.5.1")
#define PKCS5_pbeWithMD5AndDES_CBC          TEXT("1.2.840.113549.1.5.3")

#define PKCS7_data                          TEXT("1.2.840.113549.1.7.1")
#define PKCS7_signedData                    TEXT("1.2.840.113549.1.7.2")
#define PKCS7_envelopedData                 TEXT("1.2.840.113549.1.7.3")
#define PKCS7_signedAndEnvelopedData        TEXT("1.2.840.113549.1.7.4")
#define PKCS7_digestedData                  TEXT("1.2.840.113549.1.7.5")
#define PKCS7_encryptedData                 TEXT("1.2.840.113549.1.7.6")

#define PKCS9_emailAddress                  TEXT("1.2.840.113549.1.9.1")
#define PKCS9_unstructuredName              TEXT("1.2.840.113549.1.9.2")
#define PKCS9_contentType                   TEXT("1.2.840.113549.1.9.3")
#define PKCS9_messageDigest                 TEXT("1.2.840.113549.1.9.4")
#define PKCS9_signingTime                   TEXT("1.2.840.113549.1.9.5")
#define PKCS9_countersignature              TEXT("1.2.840.113549.1.9.6")
#define PKCS9_challengePassword             TEXT("1.2.840.113549.1.9.7")
#define PKCS9_unstructuredAddress           TEXT("1.2.840.113549.1.9.8")
#define PKCS9_extendedCertificateAttributes TEXT("1.2.840.113549.1.9.9")
#define PKCS9_description                   TEXT("1.2.840.113549.1.9.10")

#ifdef _cplusplus
}
#endif
#endif // _PKCSLIB_H_

