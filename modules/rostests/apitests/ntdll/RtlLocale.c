/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for Rtl LCID functions
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <winnls.h>

typedef
NTSTATUS
NTAPI
FN_RtlConvertLCIDToString(
    _In_ LCID LcidValue,
    _In_ ULONG Base,
    _In_ ULONG Padding,
    _Out_writes_(Size) PWSTR pResultBuf,
    _In_ ULONG Size);

typedef
BOOLEAN
NTAPI
FN_RtlCultureNameToLCID(
    _In_ PUNICODE_STRING String,
    _Out_ PLCID Lcid);

typedef
BOOLEAN
NTAPI
FN_RtlLCIDToCultureName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING String);

typedef
NTSTATUS
NTAPI
FN_RtlLcidToLocaleName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING LocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString);

typedef
NTSTATUS
NTAPI
FN_RtlLocaleNameToLcid(
    _In_ PWSTR LocaleName,
    _Out_ PLCID Lcid,
    _In_ ULONG Flags);

FN_RtlConvertLCIDToString* pRtlConvertLCIDToString;
FN_RtlCultureNameToLCID* pRtlCultureNameToLCID;
FN_RtlLCIDToCultureName* pRtlLCIDToCultureName;
FN_RtlLcidToLocaleName* pRtlLcidToLocaleName;
FN_RtlLocaleNameToLcid* pRtlLocaleNameToLcid;

#define FL_LOCALE 0x00000001
#define FL_CULTURE 0x00000002
#define FL_BOTH 0x00000003
#define FL_ALT_NAME 0x00000004

typedef struct _LCID_TEST_ENTRY
{
    ULONG Line;
    LCID Lcid;
    PCWSTR CultureName;
    ULONG Flags;
} LCID_TEST_ENTRY;

#define TEST(Lcid, CultureName, Flags) { __LINE__, Lcid, CultureName, Flags }

//
// For documentation see https://winprotocoldoc.z19.web.core.windows.net/MS-LCID/%5bMS-LCID%5d.pdf
// The comments are based on the comments in the document, they might not apply in all cases.
//
static LCID_TEST_ENTRY Tests[] =
{
    TEST(0x0000, NULL, 0),
    TEST(0x0001, L"ar", FL_CULTURE),
    TEST(0x0002, L"bg", FL_CULTURE),
    TEST(0x0003, L"ca", FL_CULTURE),
    TEST(0x0004, L"zh-Hans", FL_CULTURE),
    TEST(0x0005, L"cs", FL_CULTURE),
    TEST(0x0006, L"da", FL_CULTURE),
    TEST(0x0007, L"de", FL_CULTURE),
    TEST(0x0008, L"el", FL_CULTURE),
    TEST(0x0009, L"en", FL_CULTURE),
    TEST(0x000A, L"es", FL_CULTURE),
    TEST(0x000B, L"fi", FL_CULTURE),
    TEST(0x000C, L"fr", FL_CULTURE),
    TEST(0x000D, L"he", FL_CULTURE),
    TEST(0x000E, L"hu", FL_CULTURE),
    TEST(0x000F, L"is", FL_CULTURE),
    TEST(0x0010, L"it", FL_CULTURE),
    TEST(0x0011, L"ja", FL_CULTURE),
    TEST(0x0012, L"ko", FL_CULTURE),
    TEST(0x0013, L"nl", FL_CULTURE),
    TEST(0x0014, L"no", FL_CULTURE),
    TEST(0x0015, L"pl", FL_CULTURE),
    TEST(0x0016, L"pt", FL_CULTURE),
    TEST(0x0017, L"rm", FL_CULTURE),
    TEST(0x0018, L"ro", FL_CULTURE),
    TEST(0x0019, L"ru", FL_CULTURE),
    TEST(0x001A, L"hr", FL_CULTURE),
    TEST(0x001B, L"sk", FL_CULTURE),
    TEST(0x001C, L"sq", FL_CULTURE),
    TEST(0x001D, L"sv", FL_CULTURE),
    TEST(0x001E, L"th", FL_CULTURE),
    TEST(0x001F, L"tr", FL_CULTURE),
    TEST(0x0020, L"ur", FL_CULTURE),
    TEST(0x0021, L"id", FL_CULTURE),
    TEST(0x0022, L"uk", FL_CULTURE),
    TEST(0x0023, L"be", FL_CULTURE),
    TEST(0x0024, L"sl", FL_CULTURE),
    TEST(0x0025, L"et", FL_CULTURE),
    TEST(0x0026, L"lv", FL_CULTURE),
    TEST(0x0027, L"lt", FL_CULTURE),
    TEST(0x0028, L"tg", FL_CULTURE),
    TEST(0x0029, L"fa", FL_CULTURE),
    TEST(0x002A, L"vi", FL_CULTURE),
    TEST(0x002B, L"hy", FL_CULTURE),
    TEST(0x002C, L"az", FL_CULTURE),
    TEST(0x002D, L"eu", FL_CULTURE),
    TEST(0x002E, L"hsb", FL_CULTURE),
    TEST(0x002F, L"mk", FL_CULTURE),
    TEST(0x0030, L"st", FL_CULTURE),
    TEST(0x0031, L"ts", FL_CULTURE),
    TEST(0x0032, L"tn", FL_CULTURE),
    TEST(0x0033, L"ve", FL_CULTURE),
    TEST(0x0034, L"xh", FL_CULTURE),
    TEST(0x0035, L"zu", FL_CULTURE),
    TEST(0x0036, L"af", FL_CULTURE),
    TEST(0x0037, L"ka", FL_CULTURE),
    TEST(0x0038, L"fo", FL_CULTURE),
    TEST(0x0039, L"hi", FL_CULTURE),
    TEST(0x003A, L"mt", FL_CULTURE),
    TEST(0x003B, L"se", FL_CULTURE),
    TEST(0x003C, L"ga", FL_CULTURE),
    TEST(0x003D, L"yi", FL_CULTURE), // reserved
    TEST(0x003E, L"ms", FL_CULTURE),
    TEST(0x003F, L"kk", FL_CULTURE),
    TEST(0x003F, L"Kk-Cyrl", FL_CULTURE | FL_ALT_NAME), // reserved, see LCID 0x783F
    TEST(0x0040, L"ky", FL_CULTURE),
    TEST(0x0041, L"sw", FL_CULTURE),
    TEST(0x0042, L"tk", FL_CULTURE),
    TEST(0x0043, L"uz", FL_CULTURE),
    TEST(0x0044, L"tt", FL_CULTURE),
    TEST(0x0045, L"bn", FL_CULTURE),
    TEST(0x0046, L"pa", FL_CULTURE),
    TEST(0x0047, L"gu", FL_CULTURE),
    TEST(0x0048, L"or", FL_CULTURE),
    TEST(0x0049, L"ta", FL_CULTURE),
    TEST(0x004A, L"te", FL_CULTURE),
    TEST(0x004B, L"kn", FL_CULTURE),
    TEST(0x004C, L"ml", FL_CULTURE),
    TEST(0x004D, L"as", FL_CULTURE),
    TEST(0x004E, L"mr", FL_CULTURE),
    TEST(0x004F, L"sa", FL_CULTURE),
    TEST(0x0050, L"mn", FL_CULTURE),
    TEST(0x0051, L"bo", FL_CULTURE),
    TEST(0x0052, L"cy", FL_CULTURE),
    TEST(0x0053, L"km", FL_CULTURE),
    TEST(0x0054, L"lo", FL_CULTURE),
    TEST(0x0055, L"my", FL_CULTURE),
    TEST(0x0056, L"gl", FL_CULTURE),
    TEST(0x0057, L"kok", FL_CULTURE),
    TEST(0x0058, L"mni", FL_CULTURE), // reserved
    TEST(0x0059, L"sd", FL_CULTURE),
    TEST(0x005A, L"syr", FL_CULTURE),
    TEST(0x005B, L"si", FL_CULTURE),
    TEST(0x005C, L"chr", FL_CULTURE),
    TEST(0x005D, L"iu", FL_CULTURE),
    TEST(0x005E, L"am", FL_CULTURE),
    TEST(0x005F, L"tzm", FL_CULTURE),
    TEST(0x0060, L"ks", FL_CULTURE),
    TEST(0x0061, L"ne", FL_CULTURE),
    TEST(0x0062, L"fy", FL_CULTURE),
    TEST(0x0063, L"ps", FL_CULTURE),
    TEST(0x0064, L"fil", FL_CULTURE),
    TEST(0x0065, L"dv", FL_CULTURE),
    TEST(0x0066, L"bin", FL_CULTURE), // reserved
    TEST(0x0067, L"ff", FL_CULTURE),
    TEST(0x0068, L"ha", FL_CULTURE),
    TEST(0x0069, L"ibb", FL_CULTURE), // reserved
    TEST(0x006A, L"yo", FL_CULTURE),
    TEST(0x006B, L"quz", FL_CULTURE),
    TEST(0x006C, L"nso", FL_CULTURE),
    TEST(0x006D, L"ba", FL_CULTURE),
    TEST(0x006E, L"lb", FL_CULTURE),
    TEST(0x006F, L"kl", FL_CULTURE),
    TEST(0x0070, L"ig", FL_CULTURE),
    TEST(0x0071, L"kr", FL_CULTURE), // reserved
    TEST(0x0072, L"om", FL_CULTURE),
    TEST(0x0073, L"ti", FL_CULTURE),
    TEST(0x0074, L"gn", FL_CULTURE),
    TEST(0x0075, L"haw", FL_CULTURE),
    TEST(0x0076, L"la", FL_CULTURE), // reserved
    TEST(0x0077, L"so", FL_CULTURE), // reserved
    TEST(0x0078, L"ii", FL_CULTURE),
    TEST(0x0079, L"pap", FL_CULTURE), // reserved
    TEST(0x007A, L"arn", FL_CULTURE),
    TEST(0x007B, NULL, 0), // Neither defined nor reserved
    TEST(0x007C, L"moh", FL_CULTURE),
    TEST(0x007D, NULL, 0), // Neither defined nor reserved
    TEST(0x007E, L"br", FL_CULTURE),
    //TEST(0x007F, L"", FL_BOTH), // CULTURE_INVARIANT
    TEST(0x0080, L"ug", FL_CULTURE),
    TEST(0x0081, L"mi", FL_CULTURE),
    TEST(0x0082, L"oc", FL_CULTURE),
    TEST(0x0083, L"co", FL_CULTURE),
    TEST(0x0084, L"gsw", FL_CULTURE),
    TEST(0x0085, L"sah", FL_CULTURE),
    TEST(0x0086, L"quc", FL_CULTURE), // Doc says qut, see LCID 0x0093
    TEST(0x0086, L"qut", FL_CULTURE | FL_ALT_NAME),
    TEST(0x0087, L"rw", FL_CULTURE),
    TEST(0x0088, L"wo", FL_CULTURE),
    TEST(0x0089, NULL, 0), // Neither defined nor reserved
    TEST(0x008A, NULL, 0), // Neither defined nor reserved
    TEST(0x008B, NULL, 0), // Neither defined nor reserved
    TEST(0x008C, L"prs", FL_CULTURE),
    TEST(0x008D, NULL, 0), // Neither defined nor reserved
    TEST(0x008E, NULL, 0), // Neither defined nor reserved
    TEST(0x008F, NULL, 0), // Neither defined nor reserved
    TEST(0x0090, NULL, 0), // Neither defined nor reserved
    TEST(0x0091, L"gd", FL_CULTURE),
    TEST(0x0092, L"ku", FL_CULTURE),
    TEST(0x0093, NULL, 0), // reserved, quc, see LCID 0x0086
    TEST(0x0401, L"ar-SA", FL_BOTH),
    TEST(0x0402, L"bg-BG", FL_BOTH),
    TEST(0x0403, L"ca-ES", FL_BOTH),
    TEST(0x0404, L"zh-TW", FL_BOTH),
    TEST(0x0405, L"cs-CZ", FL_BOTH),
    TEST(0x0406, L"da-DK", FL_BOTH),
    TEST(0x0407, L"de-DE", FL_BOTH),
    TEST(0x0408, L"el-GR", FL_BOTH),
    TEST(0x0409, L"en-US", FL_BOTH),
    TEST(0x040A, L"es-ES_tradnl", FL_BOTH),
    TEST(0x040B, L"fi-FI", FL_BOTH),
    TEST(0x040C, L"fr-FR", FL_BOTH),
    TEST(0x040D, L"he-IL", FL_BOTH),
    TEST(0x040E, L"hu-HU", FL_BOTH),
    TEST(0x040F, L"is-IS", FL_BOTH),
    TEST(0x0410, L"it-IT", FL_BOTH),
    TEST(0x0411, L"ja-JP", FL_BOTH),
    TEST(0x0412, L"ko-KR", FL_BOTH),
    TEST(0x0413, L"nl-NL", FL_BOTH),
    TEST(0x0414, L"nb-NO", FL_BOTH),
    TEST(0x0415, L"pl-PL", FL_BOTH),
    TEST(0x0416, L"pt-BR", FL_BOTH),
    TEST(0x0417, L"rm-CH", FL_BOTH),
    TEST(0x0418, L"ro-RO", FL_BOTH),
    TEST(0x0419, L"ru-RU", FL_BOTH),
    TEST(0x041A, L"hr-HR", FL_BOTH),
    TEST(0x041B, L"sk-SK", FL_BOTH),
    TEST(0x041C, L"sq-AL", FL_BOTH),
    TEST(0x041D, L"sv-SE", FL_BOTH),
    TEST(0x041E, L"th-TH", FL_BOTH),
    TEST(0x041F, L"tr-TR", FL_BOTH),
    TEST(0x0420, L"ur-PK", FL_BOTH),
    TEST(0x0421, L"id-ID", FL_BOTH),
    TEST(0x0422, L"uk-UA", FL_BOTH),
    TEST(0x0423, L"be-BY", FL_BOTH),
    TEST(0x0424, L"sl-SI", FL_BOTH),
    TEST(0x0425, L"et-EE", FL_BOTH),
    TEST(0x0426, L"lv-LV", FL_BOTH),
    TEST(0x0427, L"lt-LT", FL_BOTH),
    TEST(0x0428, L"tg-Cyrl-TJ", FL_BOTH),
    TEST(0x0429, L"fa-IR", FL_BOTH),
    TEST(0x042A, L"vi-VN", FL_BOTH),
    TEST(0x042B, L"hy-AM", FL_BOTH),
    TEST(0x042C, L"az-Latn-AZ", FL_BOTH),
    TEST(0x042D, L"eu-ES", FL_BOTH),
    TEST(0x042E, L"hsb-DE", FL_BOTH),
    TEST(0x042F, L"mk-MK", FL_BOTH),
    TEST(0x0430, L"st-ZA", FL_BOTH),
    TEST(0x0431, L"ts-ZA", FL_BOTH),
    TEST(0x0432, L"tn-ZA", FL_BOTH),
    TEST(0x0433, L"ve-ZA", FL_BOTH),
    TEST(0x0434, L"xh-ZA", FL_BOTH),
    TEST(0x0435, L"zu-ZA", FL_BOTH),
    TEST(0x0436, L"af-ZA", FL_BOTH),
    TEST(0x0437, L"ka-GE", FL_BOTH),
    TEST(0x0438, L"fo-FO", FL_BOTH),
    TEST(0x0439, L"hi-IN", FL_BOTH),
    TEST(0x043A, L"mt-MT", FL_BOTH),
    TEST(0x043B, L"se-NO", FL_BOTH),
    TEST(0x043D, L"yi-001", FL_BOTH),
    TEST(0x043E, L"ms-MY", FL_BOTH),
    TEST(0x043F, L"kk-KZ", FL_BOTH),
    TEST(0x0440, L"ky-KG", FL_BOTH),
    TEST(0x0441, L"sw-KE", FL_BOTH),
    TEST(0x0442, L"tk-TM", FL_BOTH),
    TEST(0x0443, L"uz-Latn-UZ", FL_BOTH),
    TEST(0x0444, L"tt-RU", FL_BOTH),
    TEST(0x0445, L"bn-IN", FL_BOTH),
    TEST(0x0446, L"pa-IN", FL_BOTH),
    TEST(0x0447, L"gu-IN", FL_BOTH),
    TEST(0x0448, L"or-IN", FL_BOTH),
    TEST(0x0449, L"ta-IN", FL_BOTH),
    TEST(0x044A, L"te-IN", FL_BOTH),
    TEST(0x044B, L"kn-IN", FL_BOTH),
    TEST(0x044C, L"ml-IN", FL_BOTH),
    TEST(0x044D, L"as-IN", FL_BOTH),
    TEST(0x044E, L"mr-IN", FL_BOTH),
    TEST(0x044F, L"sa-IN", FL_BOTH),
    TEST(0x0450, L"mn-MN", FL_BOTH),
    TEST(0x0451, L"bo-CN", FL_BOTH),
    TEST(0x0452, L"cy-GB", FL_BOTH),
    TEST(0x0453, L"km-KH", FL_BOTH),
    TEST(0x0454, L"lo-LA", FL_BOTH),
    TEST(0x0455, L"my-MM", FL_BOTH),
    TEST(0x0456, L"gl-ES", FL_BOTH),
    TEST(0x0457, L"kok-IN", FL_BOTH),
    TEST(0x0458, L"mni-IN", FL_BOTH), // reserved
    TEST(0x0459, L"sd-Deva-IN", FL_BOTH), // reserved
    TEST(0x045A, L"syr-SY", FL_BOTH),
    TEST(0x045B, L"si-LK", FL_BOTH),
    TEST(0x045C, L"chr-Cher-US", FL_BOTH),
    TEST(0x045D, L"iu-Cans-CA", FL_BOTH),
    TEST(0x045E, L"am-ET", FL_BOTH),
    TEST(0x045F, L"tzm-Arab-MA", FL_BOTH),
    TEST(0x0460, L"ks-Arab", FL_CULTURE),
    TEST(0x0461, L"ne-NP", FL_BOTH),
    TEST(0x0462, L"fy-NL", FL_BOTH),
    TEST(0x0463, L"ps-AF", FL_BOTH),
    TEST(0x0464, L"fil-PH", FL_BOTH),
    TEST(0x0465, L"dv-MV", FL_BOTH),
    TEST(0x0466, L"bin-NG", FL_BOTH), // reserved
    TEST(0x0467, L"ff-Latn-NG", FL_BOTH),
    TEST(0x0467, L"ff-NG", FL_BOTH | FL_ALT_NAME),
    TEST(0x0468, L"ha-Latn-NG", FL_BOTH),
    TEST(0x0469, L"ibb-NG", FL_BOTH), // reserved
    TEST(0x046A, L"yo-NG", FL_BOTH),
    TEST(0x046B, L"quz-BO", FL_BOTH),
    TEST(0x046C, L"nso-ZA", FL_BOTH),
    TEST(0x046D, L"ba-RU", FL_BOTH),
    TEST(0x046E, L"lb-LU", FL_BOTH),
    TEST(0x046F, L"kl-GL", FL_BOTH),
    TEST(0x0470, L"ig-NG", FL_BOTH),
    TEST(0x0471, L"kr-Latn-NG", FL_BOTH),
    TEST(0x0472, L"om-ET", FL_BOTH),
    TEST(0x0473, L"ti-ET", FL_BOTH),
    TEST(0x0474, L"gn-PY", FL_BOTH),
    TEST(0x0475, L"haw-US", FL_BOTH),
    TEST(0x0476, L"la-001", FL_BOTH), // Doc says la-VA
    TEST(0x0477, L"so-SO", FL_BOTH),
    TEST(0x0478, L"ii-CN", FL_BOTH),
    TEST(0x0479, L"pap-029", FL_BOTH), // reserved
    TEST(0x047A, L"arn-CL", FL_BOTH),
    TEST(0x047C, L"moh-CA", FL_BOTH),
    TEST(0x047E, L"br-FR", FL_BOTH),
    TEST(0x0480, L"ug-CN", FL_BOTH),
    TEST(0x0481, L"mi-NZ", FL_BOTH),
    TEST(0x0482, L"oc-FR", FL_BOTH),
    TEST(0x0483, L"co-FR", FL_BOTH),
    TEST(0x0484, L"gsw-FR", FL_BOTH),
    TEST(0x0485, L"sah-RU", FL_BOTH),
    TEST(0x0486, L"quc-Latn-GT", FL_BOTH), // reserved, Doc says qut-GT
    TEST(0x0486, L"qut-GT", FL_BOTH | FL_ALT_NAME),
    TEST(0x0487, L"rw-RW", FL_BOTH),
    TEST(0x0488, L"wo-SN", FL_BOTH),
    TEST(0x048C, L"prs-AF", FL_BOTH),
    TEST(0x048D, L"plt-MG", 0), // reserved, plt-MG
    TEST(0x048E, L"zh-yue-HK", 0), // reserved, zh-yue-HK
    TEST(0x048F, L"tdd-Tale-CN", 0), // reserved, tdd-Tale-CN
    TEST(0x0490, L"khb-Talu-CN", 0), // reserved, khb-Talu-CN
    TEST(0x0491, L"gd-GB", FL_BOTH),
    TEST(0x0492, L"ku-Arab-IQ", FL_BOTH),
    TEST(0x0493, L"khb-Talu-CN", 0), // reserved, quc-CO
    //TEST(0x0501, L"qps-ploc", FL_BOTH), // Needs special handling
    TEST(0x05FE, L"qps-ploca", FL_BOTH),
    TEST(0x0801, L"ar-IQ", FL_BOTH),
    TEST(0x0803, L"ca-ES-valencia", FL_BOTH),
    TEST(0x0804, L"zh-CN", FL_BOTH),
    TEST(0x0807, L"de-CH", FL_BOTH),
    TEST(0x0809, L"en-GB", FL_BOTH),
    TEST(0x080A, L"es-MX", FL_BOTH),
    TEST(0x080C, L"fr-BE", FL_BOTH),
    TEST(0x0810, L"it-CH", FL_BOTH),
    TEST(0x0811, L"ja-Ploc-JP", 0), // reserved, ja-Ploc-JP
    TEST(0x0813, L"nl-BE", FL_BOTH),
    TEST(0x0814, L"nn-NO", FL_BOTH),
    TEST(0x0816, L"pt-PT", FL_BOTH),
    TEST(0x0818, L"ro-MD", FL_BOTH),
    TEST(0x0819, L"ru-MD", FL_BOTH),
    TEST(0x081A, L"sr-Latn-CS", FL_BOTH),
    TEST(0x081D, L"sv-FI", FL_BOTH),
    TEST(0x0820, L"ur-IN", FL_BOTH),
    TEST(0x0827, NULL, 0), // Neither defined nor reserved
    TEST(0x082C, L"az-Cyrl-AZ", FL_BOTH), // reserved
    TEST(0x082E, L"dsb-DE", FL_BOTH),
    TEST(0x0832, L"tn-BW", FL_BOTH),
    TEST(0x083B, L"se-SE", FL_BOTH),
    TEST(0x083C, L"ga-IE", FL_BOTH),
    TEST(0x083E, L"ms-BN", FL_BOTH),
    TEST(0x083F, NULL, 0), // reserved, kk-Latn-KZ
    TEST(0x0843, L"uz-Cyrl-UZ", FL_BOTH), // reserved
    TEST(0x0845, L"bn-BD", FL_BOTH),
    TEST(0x0846, L"pa-Arab-PK", FL_BOTH),
    TEST(0x0849, L"ta-LK", FL_BOTH),
    TEST(0x0850, L"mn-Mong-CN", FL_BOTH), // reserved
    TEST(0x0851, NULL, 0), // reserved, bo-BT,
    TEST(0x0859, L"sd-Arab-PK", FL_BOTH),
    TEST(0x085D, L"iu-Latn-CA", FL_BOTH),
    TEST(0x085F, L"tzm-Latn-DZ", FL_BOTH),
    TEST(0x0860, L"ks-Deva-IN", FL_BOTH),
    TEST(0x0861, L"ne-IN", FL_BOTH),
    TEST(0x0867, L"ff-Latn-SN", FL_BOTH),
    TEST(0x086B, L"quz-EC", FL_BOTH),
    TEST(0x0873, L"ti-ER", FL_BOTH),
    TEST(0x09FF, L"qps-plocm", FL_BOTH),
    //TEST(0x0C00, NULL, 0),// Locale without assigned LCID if the current user default locale. See section 2.2.1.
    TEST(0x0C01, L"ar-EG", FL_BOTH),
    TEST(0x0C04, L"zh-HK", FL_BOTH),
    TEST(0x0C07, L"de-AT", FL_BOTH),
    TEST(0x0C09, L"en-AU", FL_BOTH),
    TEST(0x0C0A, L"es-ES", FL_BOTH),
    TEST(0x0C0C, L"fr-CA", FL_BOTH),
    TEST(0x0C1A, L"sr-Cyrl-CS", FL_BOTH),
    TEST(0x0C3B, L"se-FI", FL_BOTH),
    TEST(0x0C50, L"mn-Mong-MN", FL_BOTH),
    TEST(0x0C51, L"dz-BT", FL_BOTH),
    TEST(0x0C5F, L"Tmz-MA", 0), // reserved, Tmz-MA
    TEST(0x0C6b, L"quz-PE", FL_BOTH),
    TEST(0x1000, NULL, 0), // Locale without assigned LCID if the current user default locale. See section 2.2.1.
    TEST(0x1001, L"ar-LY", FL_BOTH),
    TEST(0x1004, L"zh-SG", FL_BOTH),
    TEST(0x1007, L"de-LU", FL_BOTH),
    TEST(0x1009, L"en-CA", FL_BOTH),
    TEST(0x100A, L"es-GT", FL_BOTH),
    TEST(0x100C, L"fr-CH", FL_BOTH),
    TEST(0x101A, L"hr-BA", FL_BOTH),
    TEST(0x103B, L"smj-NO", FL_BOTH),
    TEST(0x105F, L"tzm-Tfng-MA", FL_BOTH),
    //TEST(0x1400, NULL, 0),
    TEST(0x1401, L"ar-DZ", FL_BOTH),
    TEST(0x1404, L"zh-MO", FL_BOTH),
    TEST(0x1407, L"de-LI", FL_BOTH),
    TEST(0x1409, L"en-NZ", FL_BOTH),
    TEST(0x140A, L"es-CR", FL_BOTH),
    TEST(0x140C, L"fr-LU", FL_BOTH),
    TEST(0x141A, L"bs-Latn-BA", FL_BOTH),
    TEST(0x143B, L"smj-SE", FL_BOTH),
    TEST(0x1801, L"ar-MA", FL_BOTH),
    TEST(0x1809, L"en-IE", FL_BOTH),
    TEST(0x180A, L"es-PA", FL_BOTH),
    TEST(0x180C, L"fr-MC", FL_BOTH),
    TEST(0x181A, L"sr-Latn-BA", FL_BOTH),
    TEST(0x183B, L"sma-NO", FL_BOTH),
    TEST(0x1C01, L"ar-TN", FL_BOTH),
    TEST(0x1C09, L"en-ZA", FL_BOTH),
    TEST(0x1C0A, L"es-DO", FL_BOTH),
    TEST(0x1C0C, L"fr-029", FL_BOTH),
    TEST(0x1C1A, L"sr-Cyrl-BA", FL_BOTH),
    TEST(0x1C3B, L"sma-SE", FL_BOTH),
    TEST(0x2000, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x2001, L"ar-OM", FL_BOTH),
    TEST(0x2008, NULL, 0), // Neither defined nor reserved
    TEST(0x2009, L"en-JM", FL_BOTH),
    TEST(0x200A, L"es-VE", FL_BOTH),
    TEST(0x200C, L"fr-RE", FL_BOTH),
    TEST(0x201A, L"bs-Cyrl-BA", FL_BOTH),
    TEST(0x203B, L"sms-FI", FL_BOTH),
    TEST(0x2400, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x2401, L"ar-YE", FL_BOTH),
    TEST(0x2409, L"en-029", FL_BOTH), // reserved
    TEST(0x240A, L"es-CO", FL_BOTH),
    TEST(0x240C, L"fr-CD", FL_BOTH),
    TEST(0x241A, L"sr-Latn-RS", FL_BOTH),
    TEST(0x243B, L"smn-FI", FL_BOTH),
    TEST(0x2800, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x2801, L"ar-SY", FL_BOTH),
    TEST(0x2809, L"en-BZ", FL_BOTH),
    TEST(0x280A, L"es-PE", FL_BOTH),
    TEST(0x280C, L"fr-SN", FL_BOTH),
    TEST(0x281A, L"sr-Cyrl-RS", FL_BOTH),
    TEST(0x2C00, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x2C01, L"ar-JO", FL_BOTH),
    TEST(0x2C09, L"en-TT", FL_BOTH),
    TEST(0x2C0A, L"es-AR", FL_BOTH),
    TEST(0x2C0C, L"fr-CM", FL_BOTH),
    TEST(0x2C1A, L"sr-Latn-ME", FL_BOTH),
    TEST(0x3000, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x3001, L"ar-LB", FL_BOTH),
    TEST(0x3009, L"en-ZW", FL_BOTH),
    TEST(0x300A, L"es-EC", FL_BOTH),
    TEST(0x300C, L"fr-CI", FL_BOTH),
    TEST(0x301A, L"sr-Cyrl-ME", FL_BOTH),
    TEST(0x3400, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x3401, L"ar-KW", FL_BOTH),
    TEST(0x3409, L"en-PH", FL_BOTH),
    TEST(0x340A, L"es-CL", FL_BOTH),
    TEST(0x340C, L"fr-ML", FL_BOTH),
    TEST(0x3800, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x3801, L"ar-AE", FL_BOTH),
    TEST(0x3809, L"en-ID", FL_BOTH), // reserved
    TEST(0x380A, L"es-UY", FL_BOTH),
    TEST(0x380C, L"fr-MA", FL_BOTH),
    TEST(0x3C00, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x3C01, L"ar-BH", FL_BOTH),
    TEST(0x3C09, L"en-HK", FL_BOTH),
    TEST(0x3C0A, L"es-PY", FL_BOTH),
    TEST(0x3C0C, L"fr-HT", FL_BOTH),
    TEST(0x4000, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x4001, L"ar-QA", FL_BOTH),
    TEST(0x4009, L"en-IN", FL_BOTH),
    TEST(0x400A, L"es-BO", FL_BOTH),
    TEST(0x4400, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x4401, L"ar-Ploc-SA", 0), // reserved, ar-Ploc-SA
    TEST(0x4409, L"en-MY", FL_BOTH),
    TEST(0x440A, L"es-SV", FL_BOTH),
    TEST(0x4800, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x4801, L"ar-145", 0), // reserved, ar-145
    TEST(0x4809, L"en-SG", FL_BOTH),
    TEST(0x480A, L"es-HN", FL_BOTH),
    TEST(0x4C00, NULL, 0), // Unassigned LCID locale temporarily assigned to LCID 0x3000. See section 2.2.1.
    TEST(0x4C09, L"en-AE", FL_BOTH),
    TEST(0x4C0A, L"es-NI", FL_BOTH),
    TEST(0x5009, L"en-BH", 0), // reserved
    TEST(0x500A, L"es-PR", FL_BOTH),
    TEST(0x5409, L"en-EG", 0), // reserved
    TEST(0x540A, L"es-US", FL_BOTH),
    TEST(0x5809, L"en-JO", 0), // reserved
    TEST(0x580A, L"es-419", FL_BOTH),// reserved
    TEST(0x5C09, L"en-KW", 0), // reserved
    TEST(0x5C0A, L"es-CU", FL_BOTH),
    TEST(0x6009, L"en-TR", 0), // reserved
    TEST(0x6409, L"en-YE", 0), // reserved
    TEST(0x641A, L"bs-Cyrl", FL_CULTURE),
    TEST(0x681A, L"bs-Latn", FL_CULTURE),
    TEST(0x6C1A, L"sr-Cyrl", FL_CULTURE),
    TEST(0x701A, L"sr-Latn", FL_CULTURE),
    TEST(0x703B, L"smn", FL_CULTURE),
    TEST(0x742C, L"az-Cyrl", FL_CULTURE),
    TEST(0x743B, L"sms", FL_CULTURE),
    TEST(0x7804, L"zh", FL_CULTURE),
    TEST(0x7814, L"nn", FL_CULTURE),
    TEST(0x781A, L"bs", FL_CULTURE),
    TEST(0x782C, L"az-Latn", FL_CULTURE),
    TEST(0x783B, L"sma", FL_CULTURE),
    //TEST(0x783F, L"kk-Cyrl", FL_BOTH), // reserved, see LCID 0x003F
    TEST(0x7843, L"uz-Cyrl", FL_CULTURE),
    TEST(0x7850, L"mn-Cyrl", FL_CULTURE),
    TEST(0x785D, L"iu-Cans", FL_CULTURE),
    TEST(0x785F, L"tzm-Tfng", FL_CULTURE),
    TEST(0x7C04, L"zh-Hant", FL_CULTURE),
    TEST(0x7C14, L"nb", FL_CULTURE),
    TEST(0x7C1A, L"sr", FL_CULTURE),
    TEST(0x7C28, L"tg-Cyrl", FL_CULTURE),
    TEST(0x7C2E, L"dsb", FL_CULTURE),
    TEST(0x7C3B, L"smj", FL_CULTURE),
    TEST(0x7C3F, L"kk-Latn", 0), // reserved
    TEST(0x7C43, L"uz-Latn", FL_CULTURE),
    TEST(0x7C46, L"pa-Arab", FL_CULTURE),
    TEST(0x7C50, L"mn-Mong", FL_CULTURE),
    TEST(0x7C59, L"sd-Arab", FL_CULTURE),
    TEST(0x7C5C, L"chr-Cher", FL_CULTURE),
    TEST(0x7C5D, L"iu-Latn", FL_CULTURE),
    TEST(0x7C5F, L"tzm-Latn", FL_CULTURE),
    TEST(0x7C67, L"ff-Latn", FL_CULTURE),
    TEST(0x7C68, L"ha-Latn", FL_CULTURE),
    TEST(0x7C92, L"ku-Arab", FL_CULTURE),
    TEST(0xF2EE, NULL, 0), // reserved
    TEST(0xE40C, L"fr-015", 0), // reserved
    TEST(0xEEEE, NULL, 0), // reserved

    // Alternative sorting
    TEST(0x10407, L"de-DE_phoneb", FL_BOTH),
    TEST(0x1040E, L"hu-HU_technl", FL_BOTH),
    TEST(0x10437, L"ka-GE_modern", FL_BOTH),
    TEST(0x20804, L"zh-CN_stroke", FL_BOTH),
    TEST(0x21004, L"zh-SG_stroke", FL_BOTH),
    TEST(0x30404, L"zh-TW_pronun", FL_BOTH),
    TEST(0x40404, L"zh-TW_radstr", FL_BOTH),
    TEST(0x40411, L"ja-JP_radstr", FL_BOTH),
    TEST(0x40C04, L"zh-HK_radstr", FL_BOTH),
    TEST(0x41404, L"zh-MO_radstr", FL_BOTH),

    // Some invalid sorting flags
    TEST(0x10804, NULL, 0),
    TEST(0x20404, NULL, 0),

};

static BOOLEAN Init(void)
{
    HMODULE hmod = GetModuleHandleA("ntdll.dll");

    // Initialize function pointers
    pRtlConvertLCIDToString = (FN_RtlConvertLCIDToString*)GetProcAddress(hmod, "RtlConvertLCIDToString");
    pRtlCultureNameToLCID = (FN_RtlCultureNameToLCID*)GetProcAddress(hmod, "RtlCultureNameToLCID");
    pRtlLCIDToCultureName = (FN_RtlLCIDToCultureName*)GetProcAddress(hmod, "RtlLCIDToCultureName");
    pRtlLcidToLocaleName = (FN_RtlLcidToLocaleName*)GetProcAddress(hmod, "RtlLcidToLocaleName");
    pRtlLocaleNameToLcid = (FN_RtlLocaleNameToLcid*)GetProcAddress(hmod, "RtlLocaleNameToLcid");
    if (!pRtlConvertLCIDToString ||
        !pRtlCultureNameToLCID ||
        !pRtlLCIDToCultureName ||
        !pRtlLcidToLocaleName ||
        !pRtlLocaleNameToLcid)
    {
        skip("Function not found\n");
        return FALSE;
    }

    return TRUE;
}

static void Test_RtlCultureNameToLCID(void)
{
    UNICODE_STRING CultureName;
    WCHAR Buffer[100];
    BOOLEAN Result, ExpectedResult;;
    LCID Lcid, ExpectedLcid;

    // Test valid CultureName
    Lcid = 0xDEADBEEF;
    RtlInitUnicodeString(&CultureName, L"en-US");
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, TRUE);
    ok_eq_hex(Lcid, 0x0409);

    // Test wrongly capatalized CultureName
    Lcid = 0xDEADBEEF;
    RtlInitUnicodeString(&CultureName, L"eN-uS");
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, TRUE);
    ok_eq_hex(Lcid, 0x0409);

    // Test not nullterminated buffer
    Lcid = 0xDEADBEEF;
    wcscpy(Buffer, L"en-US");
    RtlInitUnicodeString(&CultureName, Buffer);
    Buffer[5] = L'X';
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, TRUE);
    ok_eq_hex(Lcid, 0x0409);

    // Test NULL Lcid
    Lcid = 0xDEADBEEF;
    RtlInitUnicodeString(&CultureName, L"en-US");
    Result = pRtlCultureNameToLCID(&CultureName, NULL);
    ok_eq_bool(Result, FALSE);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test NULL CultureName
    Lcid = 0xDEADBEEF;
    Result = pRtlCultureNameToLCID(NULL, &Lcid);
    ok_eq_bool(Result, FALSE);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test NULL CultureName buffer
    Lcid = 0xDEADBEEF;
    RtlInitEmptyUnicodeString(&CultureName, NULL, 0);
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, FALSE);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test empty CultureName
    Lcid = 0xDEADBEEF;
    RtlInitUnicodeString(&CultureName, L"");
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, FALSE);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test invalid CultureName
    Lcid = 0xDEADBEEF;
    RtlInitUnicodeString(&CultureName, L"en-UX");
    Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
    ok_eq_bool(Result, FALSE);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Process the test entries
    for (ULONG i = 0; i < ARRAYSIZE(Tests); i++)
    {
        ExpectedResult = (Tests[i].Flags & FL_CULTURE) ? TRUE : FALSE;
        ExpectedLcid = (Tests[i].Flags & FL_CULTURE) ? Tests[i].Lcid : 0xDEADBEEF;

        RtlInitUnicodeString(&CultureName, Tests[i].CultureName);
        Lcid = 0xDEADBEEF;
        Result = pRtlCultureNameToLCID(&CultureName, &Lcid);
        ok(Result == ExpectedResult,
           "Line %lu: Result = %u, expected %u failed\n",
           Tests[i].Line,
           Result,
           ExpectedResult);
        ok(Lcid == ExpectedLcid,
           "Line %lu: Lcid = 0x%08lX, expected 0x%08lX\n",
           Tests[i].Line, Lcid, Tests[i].Lcid);
    }
}

static void Test_RtlLCIDToCultureName(void)
{
    UNICODE_STRING CultureName, ExpectedCultureName;
    WCHAR Buffer[100];
    BOOLEAN Result, ExpectedResult, Equal;

    // Test NULL CultureName
    Result = pRtlLCIDToCultureName(0x0409, NULL);
    ok_eq_bool(Result, FALSE);

    // Test NULL CultureName buffer
    RtlInitEmptyUnicodeString(&CultureName, NULL, 0);
    Result = pRtlLCIDToCultureName(0x0409, &CultureName);
    ok_eq_bool(Result, FALSE);

    // Test 0 sized CultureName buffer
    RtlInitEmptyUnicodeString(&CultureName, Buffer, 0);
    Result = pRtlLCIDToCultureName(0x0409, &CultureName);
    ok_eq_bool(Result, FALSE);

    // Test too small CultureName buffer
    RtlInitEmptyUnicodeString(&CultureName, Buffer, 10);
    Result = pRtlLCIDToCultureName(0x0409, &CultureName);
    ok_eq_bool(Result, FALSE);

    // Test valid CultureName buffer
    RtlInitEmptyUnicodeString(&CultureName, Buffer, 12);
    Result = pRtlLCIDToCultureName(0x0409, &CultureName);
    ok_eq_bool(Result, TRUE);
    ok_eq_wstr(CultureName.Buffer, L"en-US");

    // Test invalid LCID
    RtlInitEmptyUnicodeString(&CultureName, Buffer, 12);
    Result = pRtlLCIDToCultureName(0x0469, &CultureName);
    ok_eq_bool(Result, FALSE);

    // Process the test entries
    for (ULONG i = 0; i < ARRAYSIZE(Tests); i++)
    {
        if (Tests[i].Flags & FL_ALT_NAME)
        {
            continue;
        }

        ExpectedResult = (Tests[i].Flags & FL_CULTURE) ? TRUE : FALSE;
        RtlInitEmptyUnicodeString(&CultureName, Buffer, sizeof(Buffer));
        RtlInitUnicodeString(&ExpectedCultureName, Tests[i].CultureName);
        Result = pRtlLCIDToCultureName(Tests[i].Lcid, &CultureName);
        ok(Result == ExpectedResult,
           "Line %lu, Lcid 0x%lx: Result == %u, expected %u\n",
           Tests[i].Line, Tests[i].Lcid, Result, ExpectedResult);
        if (Result)
        {
            Equal = RtlEqualUnicodeString(&CultureName, &ExpectedCultureName, FALSE);
            ok(Equal, "Line %lu, Lcid 0x%lx: CultureName = '%wZ', expected '%wZ'\n",
               Tests[i].Line, Tests[i].Lcid, &CultureName, &ExpectedCultureName);
        }
    }
}

static void Test_RtlLocaleNameToLcid(void)
{
    LCID Lcid;
    NTSTATUS Status, ExpectedStatus;
    LCID ExpectedLcid;

    // Test valid LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, 0x0409);

    // Test wrongly capatalized LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"eN-uS", &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, 0x0409);

    // Test extended unicode chars
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"\x1165n-US", &Lcid, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test NULL LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(NULL, &Lcid, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test empty LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"", &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, LOCALE_INVARIANT);

    // Test invalid LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"en-UX", &Lcid, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test NULL Lcid
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"en-US", NULL, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test flags
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0x1);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0x2);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0x3);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0x4);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
    Status = pRtlLocaleNameToLcid(L"en-US", &Lcid, 0x8);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

    // Test NULL LocaleName and NULL Lcid
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(NULL, NULL, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test invalid LocaleName and NULL Lcid
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"en-UX", NULL, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test NULL Lcid and invalid flags
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"en-US", NULL, 0x8);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);
    ok_eq_hex(Lcid, 0xDEADBEEF);

    // Test empty LocaleName
    Lcid = 0xDEADBEEF;
    Status = pRtlLocaleNameToLcid(L"", &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, 0x7F);

    // Process the test entries
    for (ULONG i = 0; i < ARRAYSIZE(Tests); i++)
    {
        // No flags
        ExpectedStatus = (Tests[i].Flags & FL_LOCALE) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER_1;
        ExpectedLcid = (Tests[i].Flags & FL_LOCALE) ? Tests[i].Lcid : 0xDEADBEEF;
        Lcid = 0xDEADBEEF;
        Status = pRtlLocaleNameToLcid((PWSTR)Tests[i].CultureName, &Lcid, 0);
        ok(Status == ExpectedStatus,
           "Line %lu: Status == 0x%lx, expected 0x%lx\n",
           Tests[i].Line, Status, ExpectedStatus);
        ok(Lcid == ExpectedLcid,
           "Line %lu: Lcid == 0x%08X, expected 0x%08X\n",
           Tests[i].Line, Lcid, ExpectedLcid);

        // Flags = 0x2
        ExpectedStatus = (Tests[i].Flags & FL_BOTH) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER_1;
        ExpectedLcid = (Tests[i].Flags & FL_BOTH) ? Tests[i].Lcid : 0xDEADBEEF;
        Lcid = 0xDEADBEEF;
        Status = pRtlLocaleNameToLcid((PWSTR)Tests[i].CultureName, &Lcid, 0x2);
        ok(Status == ExpectedStatus,
           "Line %lu: Status == 0x%lx, expected 0x%lx\n",
           Tests[i].Line, Status, ExpectedStatus);
        ok(Lcid == ExpectedLcid,
           "Line %lu: Lcid == 0x%08X, expected 0x%08X\n",
           Tests[i].Line, Lcid, ExpectedLcid);
    }
}

static void Test_RtlLcidToLocaleName(void)
{
    UNICODE_STRING LocaleName, ExpectedLocaleName;
    WCHAR Buffer[100];
    NTSTATUS Status, ExpectedStatus;
    BOOLEAN Equal;
    LCID Lcid;

    // Test valid parameters
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, sizeof(Buffer));
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(memcmp(Buffer, L"en-US", 12) == 0, "Expected null-terminated 'en-US', got %wZ\n", &LocaleName);

    // Test invalid LCIDs
    Status = pRtlLcidToLocaleName(0xF2EE, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    Status = pRtlLcidToLocaleName(0x100409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);

    // Test reserved LCID
    Status = pRtlLcidToLocaleName(0x048E, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);

    // Test NULL LocaleName
    Status = pRtlLcidToLocaleName(0x0409, NULL, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    // Test NULL LocaleName buffer
    RtlInitEmptyUnicodeString(&LocaleName, NULL, 0);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    // Test invalid buffer size
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, sizeof(Buffer));
    LocaleName.Length = sizeof(Buffer) + 8;
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    // Test flags
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x1, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x2, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x4, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x8, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x08000000, FALSE); // LOCALE_ALLOW_NEUTRAL_NAMES
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

    // Test invalid LCID and NULL buffer
    RtlInitEmptyUnicodeString(&LocaleName, NULL, 0);
    Status = pRtlLcidToLocaleName(0xF2EE, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    // Test reserved LCID and NULL buffer
    Status = pRtlLcidToLocaleName(0x048E, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    // Test reserved LCID and invalid flags
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, sizeof(Buffer));
    Status = pRtlLcidToLocaleName(0x048E, &LocaleName, 0x8, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

    // Test invalid flags and NULL buffer
    RtlInitEmptyUnicodeString(&LocaleName, NULL, 0);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0x8, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

    // Test 0 sized LocaleName buffer
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, 0);
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);

    // Test too small LocaleName buffer
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, 10);
    Buffer[0] = L'@';
    Status = pRtlLcidToLocaleName(0x0409, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok(Buffer[0] == L'@', "Buffer should not be modified\n");

    // Test LOCALE_INVARIANT (0x007F)
    RtlInitEmptyUnicodeString(&LocaleName, Buffer, sizeof(Buffer));
    Status = pRtlLcidToLocaleName(LOCALE_INVARIANT, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(Buffer[0] == L'\0', "Buffer should be empty\n");

    // Test LOCALE_USER_DEFAULT (0x0400)
    Status = pRtlLcidToLocaleName(LOCALE_USER_DEFAULT, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLocaleNameToLcid(Buffer, &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, GetUserDefaultLCID());

    // Test LOCALE_SYSTEM_DEFAULT (0x0800)
    Status = pRtlLcidToLocaleName(LOCALE_SYSTEM_DEFAULT, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = pRtlLocaleNameToLcid(Buffer, &Lcid, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(Lcid, GetSystemDefaultLCID());

    // Test LOCALE_CUSTOM_DEFAULT (0x0C00)
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_DEFAULT, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    // Test LOCALE_CUSTOM_UNSPECIFIED (0x1000)
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_UNSPECIFIED, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_UNSPECIFIED, &LocaleName, 0x2, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);

    // Test LOCALE_CUSTOM_UI_DEFAULT (0x1400)
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_UI_DEFAULT, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_UNSUCCESSFUL);
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_UI_DEFAULT, &LocaleName, 0x2, FALSE);
    ok_ntstatus(Status, STATUS_UNSUCCESSFUL);

    // Test LOCALE_CUSTOM_UNSPECIFIED (0x1000)
    Status = pRtlLcidToLocaleName(LOCALE_CUSTOM_UNSPECIFIED, &LocaleName, 0, FALSE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_1);

    // Process the test entries
    for (ULONG i = 0; i < ARRAYSIZE(Tests); i++)
    {
        if (Tests[i].Flags & FL_ALT_NAME)
        {
            continue;
        }

        RtlInitEmptyUnicodeString(&LocaleName, Buffer, sizeof(Buffer));
        RtlInitUnicodeString(&ExpectedLocaleName, Tests[i].CultureName);

        /* First test with Flags == 0 */
        ExpectedStatus = (Tests[i].Flags & FL_LOCALE) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER_1;
        Status = pRtlLcidToLocaleName(Tests[i].Lcid, &LocaleName, 0, FALSE);
        ok(Status == ExpectedStatus,
           "Line %lu, Lcid 0x%lx: Status == 0x%lx, expected 0x%lx\n",
           Tests[i].Line, Tests[i].Lcid, Status, ExpectedStatus);
        if (NT_SUCCESS(Status))
        {
            Equal = RtlEqualUnicodeString(&LocaleName, &ExpectedLocaleName, TRUE);
            ok(Equal, "Line %lu, Lcid 0x%lx: LocaleName == '%wZ', expected '%wZ'\n",
               Tests[i].Line, Tests[i].Lcid, &LocaleName, &ExpectedLocaleName);
        }

        /* Test with Flags == 2 */
        ExpectedStatus = (Tests[i].Flags & FL_BOTH) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER_1;
        Status = pRtlLcidToLocaleName(Tests[i].Lcid, &LocaleName, 2, FALSE);
        ok(Status == ExpectedStatus,
           "Line %lu, Lcid 0x%lx: Status == 0x%lx, expected 0x%lx\n",
           Tests[i].Line, Tests[i].Lcid, Status, ExpectedStatus);
        if (NT_SUCCESS(Status))
        {
            Equal = RtlEqualUnicodeString(&LocaleName, &ExpectedLocaleName, FALSE);
            ok(Equal, "Line %lu, Lcid 0x%lx: LocaleName == '%wZ', expected '%wZ'\n",
               Tests[i].Line, Tests[i].Lcid, &LocaleName, &ExpectedLocaleName);
        }
    }
}

START_TEST(RtlLocale)
{
    if (!Init())
    {
        return;
    }

    Test_RtlCultureNameToLCID();
    Test_RtlLCIDToCultureName();
    Test_RtlLocaleNameToLcid();
    Test_RtlLcidToLocaleName();
}
