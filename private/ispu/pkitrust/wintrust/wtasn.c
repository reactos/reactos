/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for Indirect data contents */

#include <windows.h>
#include "wtasn.h"

ASN1module_t WTASN_Module = NULL;

static int ASN1CALL ASN1Enc_ObjectID(ASN1encoding_t enc, ASN1uint32_t tag, ObjectID *val);
static int ASN1CALL ASN1Enc_SpcMinimalCriteria(ASN1encoding_t enc, ASN1uint32_t tag, SpcMinimalCriteria *val);
static int ASN1CALL ASN1Enc_UtcTime(ASN1encoding_t enc, ASN1uint32_t tag, UtcTime *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Enc_SpcAttributeTypeAndOptionalValue(ASN1encoding_t enc, ASN1uint32_t tag, SpcAttributeTypeAndOptionalValue *val);
static int ASN1CALL ASN1Enc_SpcString(ASN1encoding_t enc, ASN1uint32_t tag, SpcString *val);
static int ASN1CALL ASN1Enc_SpcSerializedObject(ASN1encoding_t enc, ASN1uint32_t tag, SpcSerializedObject *val);
static int ASN1CALL ASN1Enc_SpcLink(ASN1encoding_t enc, ASN1uint32_t tag, SpcLink *val);
static int ASN1CALL ASN1Enc_SpcPeImageData(ASN1encoding_t enc, ASN1uint32_t tag, SpcPeImageData *val);
static int ASN1CALL ASN1Enc_SpcSigInfo(ASN1encoding_t enc, ASN1uint32_t tag, SpcSigInfo *val);
static int ASN1CALL ASN1Enc_SpcImage(ASN1encoding_t enc, ASN1uint32_t tag, SpcImage *val);
static int ASN1CALL ASN1Enc_SpcFinancialCriteria(ASN1encoding_t enc, ASN1uint32_t tag, SpcFinancialCriteria *val);
static int ASN1CALL ASN1Enc_SpcStatementType(ASN1encoding_t enc, ASN1uint32_t tag, SpcStatementType *val);
static int ASN1CALL ASN1Enc_SpcSpOpusInfo(ASN1encoding_t enc, ASN1uint32_t tag, SpcSpOpusInfo *val);
static int ASN1CALL ASN1Enc_NameValue(ASN1encoding_t enc, ASN1uint32_t tag, NameValue *val);
static int ASN1CALL ASN1Enc_NameValues(ASN1encoding_t enc, ASN1uint32_t tag, NameValues *val);
static int ASN1CALL ASN1Enc_MemberInfo(ASN1encoding_t enc, ASN1uint32_t tag, MemberInfo *val);
static int ASN1CALL ASN1Enc_SpcIndirectDataContent(ASN1encoding_t enc, ASN1uint32_t tag, SpcIndirectDataContent *val);
static int ASN1CALL ASN1Enc_SpcSpAgencyInformation(ASN1encoding_t enc, ASN1uint32_t tag, SpcSpAgencyInformation *val);
static int ASN1CALL ASN1Dec_ObjectID(ASN1decoding_t dec, ASN1uint32_t tag, ObjectID *val);
static int ASN1CALL ASN1Dec_SpcMinimalCriteria(ASN1decoding_t dec, ASN1uint32_t tag, SpcMinimalCriteria *val);
static int ASN1CALL ASN1Dec_UtcTime(ASN1decoding_t dec, ASN1uint32_t tag, UtcTime *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val);
static int ASN1CALL ASN1Dec_SpcAttributeTypeAndOptionalValue(ASN1decoding_t dec, ASN1uint32_t tag, SpcAttributeTypeAndOptionalValue *val);
static int ASN1CALL ASN1Dec_SpcString(ASN1decoding_t dec, ASN1uint32_t tag, SpcString *val);
static int ASN1CALL ASN1Dec_SpcSerializedObject(ASN1decoding_t dec, ASN1uint32_t tag, SpcSerializedObject *val);
static int ASN1CALL ASN1Dec_SpcLink(ASN1decoding_t dec, ASN1uint32_t tag, SpcLink *val);
static int ASN1CALL ASN1Dec_SpcPeImageData(ASN1decoding_t dec, ASN1uint32_t tag, SpcPeImageData *val);
static int ASN1CALL ASN1Dec_SpcSigInfo(ASN1decoding_t dec, ASN1uint32_t tag, SpcSigInfo *val);
static int ASN1CALL ASN1Dec_SpcImage(ASN1decoding_t dec, ASN1uint32_t tag, SpcImage *val);
static int ASN1CALL ASN1Dec_SpcFinancialCriteria(ASN1decoding_t dec, ASN1uint32_t tag, SpcFinancialCriteria *val);
static int ASN1CALL ASN1Dec_SpcStatementType(ASN1decoding_t dec, ASN1uint32_t tag, SpcStatementType *val);
static int ASN1CALL ASN1Dec_SpcSpOpusInfo(ASN1decoding_t dec, ASN1uint32_t tag, SpcSpOpusInfo *val);
static int ASN1CALL ASN1Dec_NameValue(ASN1decoding_t dec, ASN1uint32_t tag, NameValue *val);
static int ASN1CALL ASN1Dec_NameValues(ASN1decoding_t dec, ASN1uint32_t tag, NameValues *val);
static int ASN1CALL ASN1Dec_MemberInfo(ASN1decoding_t dec, ASN1uint32_t tag, MemberInfo *val);
static int ASN1CALL ASN1Dec_SpcIndirectDataContent(ASN1decoding_t dec, ASN1uint32_t tag, SpcIndirectDataContent *val);
static int ASN1CALL ASN1Dec_SpcSpAgencyInformation(ASN1decoding_t dec, ASN1uint32_t tag, SpcSpAgencyInformation *val);
static void ASN1CALL ASN1Free_ObjectID(ObjectID *val);
static void ASN1CALL ASN1Free_UtcTime(UtcTime *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val);
static void ASN1CALL ASN1Free_SpcAttributeTypeAndOptionalValue(SpcAttributeTypeAndOptionalValue *val);
static void ASN1CALL ASN1Free_SpcString(SpcString *val);
static void ASN1CALL ASN1Free_SpcSerializedObject(SpcSerializedObject *val);
static void ASN1CALL ASN1Free_SpcLink(SpcLink *val);
static void ASN1CALL ASN1Free_SpcPeImageData(SpcPeImageData *val);
static void ASN1CALL ASN1Free_SpcSigInfo(SpcSigInfo *val);
static void ASN1CALL ASN1Free_SpcImage(SpcImage *val);
static void ASN1CALL ASN1Free_SpcStatementType(SpcStatementType *val);
static void ASN1CALL ASN1Free_SpcSpOpusInfo(SpcSpOpusInfo *val);
static void ASN1CALL ASN1Free_NameValue(NameValue *val);
static void ASN1CALL ASN1Free_NameValues(NameValues *val);
static void ASN1CALL ASN1Free_MemberInfo(MemberInfo *val);
static void ASN1CALL ASN1Free_SpcIndirectDataContent(SpcIndirectDataContent *val);
static void ASN1CALL ASN1Free_SpcSpAgencyInformation(SpcSpAgencyInformation *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[14] = {
    (ASN1EncFun_t) ASN1Enc_ObjectID,
    (ASN1EncFun_t) ASN1Enc_SpcMinimalCriteria,
    (ASN1EncFun_t) ASN1Enc_UtcTime,
    (ASN1EncFun_t) ASN1Enc_SpcLink,
    (ASN1EncFun_t) ASN1Enc_SpcPeImageData,
    (ASN1EncFun_t) ASN1Enc_SpcSigInfo,
    (ASN1EncFun_t) ASN1Enc_SpcFinancialCriteria,
    (ASN1EncFun_t) ASN1Enc_SpcStatementType,
    (ASN1EncFun_t) ASN1Enc_SpcSpOpusInfo,
    (ASN1EncFun_t) ASN1Enc_NameValue,
    (ASN1EncFun_t) ASN1Enc_NameValues,
    (ASN1EncFun_t) ASN1Enc_MemberInfo,
    (ASN1EncFun_t) ASN1Enc_SpcIndirectDataContent,
    (ASN1EncFun_t) ASN1Enc_SpcSpAgencyInformation,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[14] = {
    (ASN1DecFun_t) ASN1Dec_ObjectID,
    (ASN1DecFun_t) ASN1Dec_SpcMinimalCriteria,
    (ASN1DecFun_t) ASN1Dec_UtcTime,
    (ASN1DecFun_t) ASN1Dec_SpcLink,
    (ASN1DecFun_t) ASN1Dec_SpcPeImageData,
    (ASN1DecFun_t) ASN1Dec_SpcSigInfo,
    (ASN1DecFun_t) ASN1Dec_SpcFinancialCriteria,
    (ASN1DecFun_t) ASN1Dec_SpcStatementType,
    (ASN1DecFun_t) ASN1Dec_SpcSpOpusInfo,
    (ASN1DecFun_t) ASN1Dec_NameValue,
    (ASN1DecFun_t) ASN1Dec_NameValues,
    (ASN1DecFun_t) ASN1Dec_MemberInfo,
    (ASN1DecFun_t) ASN1Dec_SpcIndirectDataContent,
    (ASN1DecFun_t) ASN1Dec_SpcSpAgencyInformation,
};
static const ASN1FreeFun_t freefntab[14] = {
    (ASN1FreeFun_t) ASN1Free_ObjectID,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_UtcTime,
    (ASN1FreeFun_t) ASN1Free_SpcLink,
    (ASN1FreeFun_t) ASN1Free_SpcPeImageData,
    (ASN1FreeFun_t) ASN1Free_SpcSigInfo,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_SpcStatementType,
    (ASN1FreeFun_t) ASN1Free_SpcSpOpusInfo,
    (ASN1FreeFun_t) ASN1Free_NameValue,
    (ASN1FreeFun_t) ASN1Free_NameValues,
    (ASN1FreeFun_t) ASN1Free_MemberInfo,
    (ASN1FreeFun_t) ASN1Free_SpcIndirectDataContent,
    (ASN1FreeFun_t) ASN1Free_SpcSpAgencyInformation,
};
static const ULONG sizetab[14] = {
    SIZE_WTASN_Module_PDU_0,
    SIZE_WTASN_Module_PDU_1,
    SIZE_WTASN_Module_PDU_2,
    SIZE_WTASN_Module_PDU_3,
    SIZE_WTASN_Module_PDU_4,
    SIZE_WTASN_Module_PDU_5,
    SIZE_WTASN_Module_PDU_6,
    SIZE_WTASN_Module_PDU_7,
    SIZE_WTASN_Module_PDU_8,
    SIZE_WTASN_Module_PDU_9,
    SIZE_WTASN_Module_PDU_10,
    SIZE_WTASN_Module_PDU_11,
    SIZE_WTASN_Module_PDU_12,
    SIZE_WTASN_Module_PDU_13,
};

/* forward declarations of values: */
extern ASN1octet_t SpcPeImageData_flags_default_octets[1];
/* definitions of value components: */
static ASN1octet_t SpcPeImageData_flags_default_octets[1] = { 0x80 };
/* definitions of values: */
SpcPeImageFlags SpcPeImageData_flags_default = { 1, SpcPeImageData_flags_default_octets };

void ASN1CALL WTASN_Module_Startup(void)
{
    WTASN_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 14, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x7477);
}

void ASN1CALL WTASN_Module_Cleanup(void)
{
    ASN1_CloseModule(WTASN_Module);
    WTASN_Module = NULL;
}

static int ASN1CALL ASN1Enc_ObjectID(ASN1encoding_t enc, ASN1uint32_t tag, ObjectID *val)
{
    if (!ASN1BEREncObjectIdentifier2(enc, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ObjectID(ASN1decoding_t dec, ASN1uint32_t tag, ObjectID *val)
{
    if (!ASN1BERDecObjectIdentifier2(dec, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ObjectID(ObjectID *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SpcMinimalCriteria(ASN1encoding_t enc, ASN1uint32_t tag, SpcMinimalCriteria *val)
{
    if (!ASN1BEREncBool(enc, tag ? tag : 0x1, *val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcMinimalCriteria(ASN1decoding_t dec, ASN1uint32_t tag, SpcMinimalCriteria *val)
{
    if (!ASN1BERDecBool(dec, tag ? tag : 0x1, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UtcTime(ASN1encoding_t enc, ASN1uint32_t tag, UtcTime *val)
{
    if (!ASN1DEREncUTCTime(enc, tag ? tag : 0x17, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UtcTime(ASN1decoding_t dec, ASN1uint32_t tag, UtcTime *val)
{
    if (!ASN1BERDecUTCTime(dec, tag ? tag : 0x17, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UtcTime(UtcTime *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->algorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->algorithm))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_DigestInfo(ASN1encoding_t enc, ASN1uint32_t tag, DigestInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->digest).length, ((val)->digest).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DigestInfo(ASN1decoding_t dec, ASN1uint32_t tag, DigestInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->digestAlgorithm))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->digest))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DigestInfo(DigestInfo *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->digestAlgorithm);
    }
}

static int ASN1CALL ASN1Enc_SpcAttributeTypeAndOptionalValue(ASN1encoding_t enc, ASN1uint32_t tag, SpcAttributeTypeAndOptionalValue *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &(val)->type))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcAttributeTypeAndOptionalValue(ASN1decoding_t dec, ASN1uint32_t tag, SpcAttributeTypeAndOptionalValue *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &(val)->type))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->value))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcAttributeTypeAndOptionalValue(SpcAttributeTypeAndOptionalValue *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_SpcString(ASN1encoding_t enc, ASN1uint32_t tag, SpcString *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncChar16String(enc, 0x80000000, ((val)->u.unicode).length, ((val)->u.unicode).value))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncCharString(enc, 0x80000001, ((val)->u.ascii).length, ((val)->u.ascii).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SpcString(ASN1decoding_t dec, ASN1uint32_t tag, SpcString *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecChar16String(dec, 0x80000000, &(val)->u.unicode))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1BERDecCharString(dec, 0x80000001, &(val)->u.ascii))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SpcString(SpcString *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1char16string_free(&(val)->u.unicode);
	    break;
	case 2:
	    ASN1charstring_free(&(val)->u.ascii);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SpcSerializedObject(ASN1encoding_t enc, ASN1uint32_t tag, SpcSerializedObject *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->classId).length, ((val)->classId).value))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->serializedData).length, ((val)->serializedData).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcSerializedObject(ASN1decoding_t dec, ASN1uint32_t tag, SpcSerializedObject *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->classId))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->serializedData))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcSerializedObject(SpcSerializedObject *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SpcLink(ASN1encoding_t enc, ASN1uint32_t tag, SpcLink *val)
{
    ASN1uint32_t nLenOff0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncCharString(enc, 0x80000000, ((val)->u.url).length, ((val)->u.url).value))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_SpcSerializedObject(enc, 0x80000001, &(val)->u.moniker))
	    return 0;
	break;
    case 3:
	if (!ASN1BEREncExplicitTag(enc, 0x80000002, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcString(enc, 0, &(val)->u.file))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SpcLink(ASN1decoding_t dec, ASN1uint32_t tag, SpcLink *val)
{
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1BERDecCharString(dec, 0x80000000, &(val)->u.url))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_SpcSerializedObject(dec, 0x80000001, &(val)->u.moniker))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1BERDecExplicitTag(dec, 0x80000002, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcString(dd0, 0, &(val)->u.file))
	    return 0;
	if (!ASN1BERDecEndOfContents(dec, dd0, di0))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SpcLink(SpcLink *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1charstring_free(&(val)->u.url);
	    break;
	case 2:
	    ASN1Free_SpcSerializedObject(&(val)->u.moniker);
	    break;
	case 3:
	    ASN1Free_SpcString(&(val)->u.file);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SpcPeImageData(ASN1encoding_t enc, ASN1uint32_t tag, SpcPeImageData *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    ASN1uint32_t r;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if (!ASN1bitstring_cmp(&val->flags, &SpcPeImageData_flags_default, 1))
	o[0] &= ~0x80;
    if (o[0] & 0x80) {
	r = ((val)->flags).length;
	ASN1BEREncRemoveZeroBits(&r, ((val)->flags).value);
	if (!ASN1DEREncBitString(enc, 0x3, r, ((val)->flags).value))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->file))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcPeImageData(ASN1decoding_t dec, ASN1uint32_t tag, SpcPeImageData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x3) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecBitString(dd, 0x3, &(val)->flags))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->file))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcPeImageData(SpcPeImageData *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1bitstring_free(&(val)->flags);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SpcLink(&(val)->file);
	}
    }
}

static int ASN1CALL ASN1Enc_SpcSigInfo(ASN1encoding_t enc, ASN1uint32_t tag, SpcSigInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwSIPversion))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->gSIPguid).length, ((val)->gSIPguid).value))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwReserved1))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwReserved2))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwReserved3))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwReserved4))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->dwReserved5))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcSigInfo(ASN1decoding_t dec, ASN1uint32_t tag, SpcSigInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwSIPversion))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->gSIPguid))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwReserved1))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwReserved2))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwReserved3))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwReserved4))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->dwReserved5))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcSigInfo(SpcSigInfo *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_SpcImage(ASN1encoding_t enc, ASN1uint32_t tag, SpcImage *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->imageLink))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncOctetString(enc, 0x80000001, ((val)->bitmap).length, ((val)->bitmap).value))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1DEREncOctetString(enc, 0x80000002, ((val)->metafile).length, ((val)->metafile).value))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1DEREncOctetString(enc, 0x80000003, ((val)->enhancedMetafile).length, ((val)->enhancedMetafile).value))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1DEREncOctetString(enc, 0x80000004, ((val)->gifFile).length, ((val)->gifFile).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcImage(ASN1decoding_t dec, ASN1uint32_t tag, SpcImage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->imageLink))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecOctetString2(dd, 0x80000001, &(val)->bitmap))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecOctetString2(dd, 0x80000002, &(val)->metafile))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000003) {
	(val)->o[0] |= 0x10;
	if (!ASN1BERDecOctetString2(dd, 0x80000003, &(val)->enhancedMetafile))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000004) {
	(val)->o[0] |= 0x8;
	if (!ASN1BERDecOctetString2(dd, 0x80000004, &(val)->gifFile))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcImage(SpcImage *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SpcLink(&(val)->imageLink);
	}
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	}
	if ((val)->o[0] & 0x10) {
	}
	if ((val)->o[0] & 0x8) {
	}
    }
}

static int ASN1CALL ASN1Enc_SpcFinancialCriteria(ASN1encoding_t enc, ASN1uint32_t tag, SpcFinancialCriteria *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->financialInfoAvailable))
	return 0;
    if (!ASN1BEREncBool(enc, 0x1, (val)->meetsCriteria))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcFinancialCriteria(ASN1decoding_t dec, ASN1uint32_t tag, SpcFinancialCriteria *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->financialInfoAvailable))
	return 0;
    if (!ASN1BERDecBool(dd, 0x1, &(val)->meetsCriteria))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_SpcStatementType(ASN1encoding_t enc, ASN1uint32_t tag, SpcStatementType *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncObjectIdentifier2(enc, 0x6, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcStatementType(ASN1decoding_t dec, ASN1uint32_t tag, SpcStatementType *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (ObjectID *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecObjectIdentifier2(dd, 0x6, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcStatementType(SpcStatementType *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SpcSpOpusInfo(ASN1encoding_t enc, ASN1uint32_t tag, SpcSpOpusInfo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcString(enc, 0, &(val)->programName))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->moreInfo))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000002, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->publisherInfo))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcSpOpusInfo(ASN1decoding_t dec, ASN1uint32_t tag, SpcSpOpusInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcString(dd0, 0, &(val)->programName))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->moreInfo))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecExplicitTag(dd, 0x80000002, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->publisherInfo))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcSpOpusInfo(SpcSpOpusInfo *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SpcString(&(val)->programName);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SpcLink(&(val)->moreInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SpcLink(&(val)->publisherInfo);
	}
    }
}

static int ASN1CALL ASN1Enc_NameValue(ASN1encoding_t enc, ASN1uint32_t tag, NameValue *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->refname).length, ((val)->refname).value))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->typeaction))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->value).length, ((val)->value).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NameValue(ASN1decoding_t dec, ASN1uint32_t tag, NameValue *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->refname))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->typeaction))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NameValue(NameValue *val)
{
    if (val) {
	ASN1char16string_free(&(val)->refname);
    }
}

static int ASN1CALL ASN1Enc_NameValues(ASN1encoding_t enc, ASN1uint32_t tag, NameValues *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if (!ASN1DEREncBeginBlk(enc, ASN1_DER_SET_OF_BLOCK, &pBlk))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1DEREncNewBlkElement(pBlk, &enc2))
	    return 0;
	if (!ASN1Enc_NameValue(enc2, 0, &((val)->value)[i]))
	    return 0;
	if (!ASN1DEREncFlushBlkElement(pBlk))
	    return 0;
    }
    if (!ASN1DEREncEndBlk(pBlk))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NameValues(ASN1decoding_t dec, ASN1uint32_t tag, NameValues *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (NameValue *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_NameValue(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NameValues(NameValues *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_NameValue(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_NameValue(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_MemberInfo(ASN1encoding_t enc, ASN1uint32_t tag, MemberInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->subguid).length, ((val)->subguid).value))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->certversion))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_MemberInfo(ASN1decoding_t dec, ASN1uint32_t tag, MemberInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->subguid))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->certversion))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_MemberInfo(MemberInfo *val)
{
    if (val) {
	ASN1char16string_free(&(val)->subguid);
    }
}

static int ASN1CALL ASN1Enc_SpcIndirectDataContent(ASN1encoding_t enc, ASN1uint32_t tag, SpcIndirectDataContent *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_SpcAttributeTypeAndOptionalValue(enc, 0, &(val)->data))
	return 0;
    if (!ASN1Enc_DigestInfo(enc, 0, &(val)->messageDigest))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcIndirectDataContent(ASN1decoding_t dec, ASN1uint32_t tag, SpcIndirectDataContent *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_SpcAttributeTypeAndOptionalValue(dd, 0, &(val)->data))
	return 0;
    if (!ASN1Dec_DigestInfo(dd, 0, &(val)->messageDigest))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcIndirectDataContent(SpcIndirectDataContent *val)
{
    if (val) {
	ASN1Free_SpcAttributeTypeAndOptionalValue(&(val)->data);
	ASN1Free_DigestInfo(&(val)->messageDigest);
    }
}

static int ASN1CALL ASN1Enc_SpcSpAgencyInformation(ASN1encoding_t enc, ASN1uint32_t tag, SpcSpAgencyInformation *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->policyInformation))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcString(enc, 0, &(val)->policyDisplayText))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_SpcImage(enc, 0x80000002, &(val)->logoImage))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000003, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SpcLink(enc, 0, &(val)->logoLink))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SpcSpAgencyInformation(ASN1decoding_t dec, ASN1uint32_t tag, SpcSpAgencyInformation *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->policyInformation))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcString(dd0, 0, &(val)->policyDisplayText))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1Dec_SpcImage(dd, 0x80000002, &(val)->logoImage))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000003) {
	(val)->o[0] |= 0x10;
	if (!ASN1BERDecExplicitTag(dd, 0x80000003, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SpcLink(dd0, 0, &(val)->logoLink))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SpcSpAgencyInformation(SpcSpAgencyInformation *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SpcLink(&(val)->policyInformation);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SpcString(&(val)->policyDisplayText);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_SpcImage(&(val)->logoImage);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_SpcLink(&(val)->logoLink);
	}
    }
}

