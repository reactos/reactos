/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for Indirect data contents */

#ifndef _WTASN_Module_H_
#define _WTASN_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1intx_t HUGEINTEGER;

typedef ASN1bitstring_t BITSTRING;

typedef ASN1octetstring_t OCTETSTRING;

typedef ASN1open_t NOCOPYANY;

typedef ASN1charstring_t NUMERICSTRING;

typedef ASN1charstring_t PRINTABLESTRING;

typedef ASN1charstring_t TELETEXSTRING;

typedef ASN1charstring_t T61STRING;

typedef ASN1charstring_t VIDEOTEXSTRING;

typedef ASN1charstring_t IA5STRING;

typedef ASN1charstring_t GRAPHICSTRING;

typedef ASN1charstring_t VISIBLESTRING;

typedef ASN1charstring_t ISO646STRING;

typedef ASN1charstring_t GENERALSTRING;

typedef ASN1char32string_t UNIVERSALSTRING;

typedef ASN1char16string_t BMPSTRING;

typedef ASN1objectidentifier2_t ObjectID;
#define ObjectID_PDU 0
#define SIZE_WTASN_Module_PDU_0 sizeof(ObjectID)

typedef OCTETSTRING SpcUuid;

typedef ASN1bitstring_t SpcPeImageFlags;
#define includeResources 0x80
#define includeDebugInfo 0x40
#define includeImportAddressTable 0x20

typedef ASN1bool_t SpcMinimalCriteria;
#define SpcMinimalCriteria_PDU 1
#define SIZE_WTASN_Module_PDU_1 sizeof(SpcMinimalCriteria)

typedef ASN1utctime_t UtcTime;
#define UtcTime_PDU 2
#define SIZE_WTASN_Module_PDU_2 sizeof(UtcTime)

typedef struct AlgorithmIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID algorithm;
#   define parameters_present 0x80
    NOCOPYANY parameters;
} AlgorithmIdentifier;

typedef struct DigestInfo {
    AlgorithmIdentifier digestAlgorithm;
    OCTETSTRING digest;
} DigestInfo;

typedef struct SpcAttributeTypeAndOptionalValue {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID type;
#   define value_present 0x80
    NOCOPYANY value;
} SpcAttributeTypeAndOptionalValue;

typedef struct SpcString {
    ASN1choice_t choice;
    union {
#	define unicode_chosen 1
	BMPSTRING unicode;
#	define ascii_chosen 2
	IA5STRING ascii;
    } u;
} SpcString;

typedef struct SpcSerializedObject {
    SpcUuid classId;
    OCTETSTRING serializedData;
} SpcSerializedObject;

typedef struct SpcLink {
    ASN1choice_t choice;
    union {
#	define url_chosen 1
	IA5STRING url;
#	define moniker_chosen 2
	SpcSerializedObject moniker;
#	define file_chosen 3
	SpcString file;
    } u;
} SpcLink;
#define SpcLink_PDU 3
#define SIZE_WTASN_Module_PDU_3 sizeof(SpcLink)

typedef struct SpcPeImageData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define flags_present 0x80
    SpcPeImageFlags flags;
#   define file_present 0x40
    SpcLink file;
} SpcPeImageData;
#define SpcPeImageData_PDU 4
#define SIZE_WTASN_Module_PDU_4 sizeof(SpcPeImageData)

typedef struct SpcSigInfo {
    ASN1int32_t dwSIPversion;
    SpcUuid gSIPguid;
    ASN1int32_t dwReserved1;
    ASN1int32_t dwReserved2;
    ASN1int32_t dwReserved3;
    ASN1int32_t dwReserved4;
    ASN1int32_t dwReserved5;
} SpcSigInfo;
#define SpcSigInfo_PDU 5
#define SIZE_WTASN_Module_PDU_5 sizeof(SpcSigInfo)

typedef struct SpcImage {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define imageLink_present 0x80
    SpcLink imageLink;
#   define bitmap_present 0x40
    OCTETSTRING bitmap;
#   define metafile_present 0x20
    OCTETSTRING metafile;
#   define enhancedMetafile_present 0x10
    OCTETSTRING enhancedMetafile;
#   define gifFile_present 0x8
    OCTETSTRING gifFile;
} SpcImage;

typedef struct SpcFinancialCriteria {
    ASN1bool_t financialInfoAvailable;
    ASN1bool_t meetsCriteria;
} SpcFinancialCriteria;
#define SpcFinancialCriteria_PDU 6
#define SIZE_WTASN_Module_PDU_6 sizeof(SpcFinancialCriteria)

typedef struct SpcStatementType {
    ASN1uint32_t count;
    ObjectID *value;
} SpcStatementType;
#define SpcStatementType_PDU 7
#define SIZE_WTASN_Module_PDU_7 sizeof(SpcStatementType)

typedef struct SpcSpOpusInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define programName_present 0x80
    SpcString programName;
#   define moreInfo_present 0x40
    SpcLink moreInfo;
#   define publisherInfo_present 0x20
    SpcLink publisherInfo;
} SpcSpOpusInfo;
#define SpcSpOpusInfo_PDU 8
#define SIZE_WTASN_Module_PDU_8 sizeof(SpcSpOpusInfo)

typedef struct NameValue {
    BMPSTRING refname;
    ASN1int32_t typeaction;
    OCTETSTRING value;
} NameValue;
#define NameValue_PDU 9
#define SIZE_WTASN_Module_PDU_9 sizeof(NameValue)

typedef struct NameValues {
    ASN1uint32_t count;
    struct NameValue *value;
} NameValues;
#define NameValues_PDU 10
#define SIZE_WTASN_Module_PDU_10 sizeof(NameValues)

typedef struct MemberInfo {
    BMPSTRING subguid;
    ASN1int32_t certversion;
} MemberInfo;
#define MemberInfo_PDU 11
#define SIZE_WTASN_Module_PDU_11 sizeof(MemberInfo)

typedef struct SpcIndirectDataContent {
    SpcAttributeTypeAndOptionalValue data;
    DigestInfo messageDigest;
} SpcIndirectDataContent;
#define SpcIndirectDataContent_PDU 12
#define SIZE_WTASN_Module_PDU_12 sizeof(SpcIndirectDataContent)

typedef struct SpcSpAgencyInformation {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define policyInformation_present 0x80
    SpcLink policyInformation;
#   define policyDisplayText_present 0x40
    SpcString policyDisplayText;
#   define logoImage_present 0x20
    SpcImage logoImage;
#   define logoLink_present 0x10
    SpcLink logoLink;
} SpcSpAgencyInformation;
#define SpcSpAgencyInformation_PDU 13
#define SIZE_WTASN_Module_PDU_13 sizeof(SpcSpAgencyInformation)

extern SpcPeImageFlags SpcPeImageData_flags_default;

extern ASN1module_t WTASN_Module;
extern void ASN1CALL WTASN_Module_Startup(void);
extern void ASN1CALL WTASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _WTASN_Module_H_ */
