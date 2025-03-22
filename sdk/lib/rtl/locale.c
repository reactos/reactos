/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Rtl locale functions
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtl.h>

#define NDEBUG
#include <debug.h>

// See https://winprotocoldoc.z19.web.core.windows.net/MS-LCID/%5bMS-LCID%5d.pdf
// For special LCIDs see https://learn.microsoft.com/en-us/windows/win32/intl/locale-custom-constants
// and https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/926e694f-1797-4418-a922-343d1c5e91a6
// and https://learn.microsoft.com/en-us/windows/win32/intl/locale-identifiers

#define MAX_PRIMARY_LANGUAGE 0x3FF
#define MAX_BASIC_LCID 0x5FFF

#define LCID_ALT_NAME 0x00100000 // Alternative name, OR'ed with LCID

LCID RtlpUserDefaultLcid;
LCID RtlpSystemDefaultLcid;

typedef struct _LOCALE_ENTRY
{
    const CHAR Locale[16];
    LCID Lcid : 24;
    ULONG Flags : 8;
} LOCALE_ENTRY;

// This table is sorted alphabetically to allow binary search
static const LOCALE_ENTRY RtlpLocaleTable[] =
{
    { "",               0x0007F }, // LOCALE_INVARIANT
    { "af",             0x00036 },
    { "af-ZA",          0x00436 },
    { "am",             0x0005E },
    { "am-ET",          0x0045E },
    { "ar",             0x00001 },
    // { "ar-145",         0x04801 }, // reserved
    { "ar-AE",          0x03801 },
    { "ar-BH",          0x03C01 },
    { "ar-DZ",          0x01401 },
    { "ar-EG",          0x00C01 },
    { "ar-IQ",          0x00801 },
    { "ar-JO",          0x02C01 },
    { "ar-KW",          0x03401 },
    { "ar-LB",          0x03001 },
    { "ar-LY",          0x01001 },
    { "ar-MA",          0x01801 },
    { "ar-OM",          0x02001 },
    // { "ar-Ploc-SA",     0x04401 }, // reserved
    { "ar-QA",          0x04001 },
    { "ar-SA",          0x00401 },
    { "ar-SY",          0x02801 },
    { "ar-TN",          0x01C01 },
    { "ar-YE",          0x02401 },
    { "arn",            0x0007A },
    { "arn-CL",         0x0047A },
    { "as",             0x0004D },
    { "as-IN",          0x0044D },
    { "az",             0x0002C },
    { "az-Cyrl",        0x0742C },
    { "az-Cyrl-AZ",     0x0082C }, // Doc: reserved
    { "az-Latn",        0x0782C },
    { "az-Latn-AZ",     0x0042C },
    { "ba",             0x0006D },
    { "ba-RU",          0x0046D },
    { "be",             0x00023 },
    { "be-BY",          0x00423 },
    { "bg",             0x00002 },
    { "bg-BG",          0x00402 },
    { "bin",            0x00066 }, // Doc: reserved
    { "bin-NG",         0x00466 }, // Doc: reserved
    { "bn",             0x00045 },
    { "bn-BD",          0x00845 },
    { "bn-IN",          0x00445 },
    { "bo",             0x00051 },
    // { "bo-BT",          0x00851 }, // reserved
    { "bo-CN",          0x00451 },
    { "br",             0x0007E },
    { "br-FR",          0x0047E },
    { "bs",             0x0781A },
    { "bs-Cyrl",        0x0641A },
    { "bs-Cyrl-BA",     0x0201A },
    { "bs-Latn",        0x0681A },
    { "bs-Latn-BA",     0x0141A },
    { "ca",             0x00003 },
    { "ca-ES",          0x00403 },
    { "ca-ES-valencia", 0x00803 },
    { "chr",            0x0005C },
    { "chr-Cher",       0x07C5C },
    { "chr-Cher-US",    0x0045C },
    { "co",             0x00083 },
    { "co-FR",          0x00483 },
    { "cs",             0x00005 },
    { "cs-CZ",          0x00405 },
    { "cy",             0x00052 },
    { "cy-GB",          0x00452 },
    { "da",             0x00006 },
    { "da-DK",          0x00406 },
    { "de",             0x00007 },
    { "de-AT",          0x00C07 },
    { "de-CH",          0x00807 },
    { "de-DE",          0x00407 },
    { "de-DE_phoneb",   0x10407 },
    { "de-LI",          0x01407 },
    { "de-LU",          0x01007 },
    { "dsb",            0x07C2E },
    { "dsb-DE",         0x0082E },
    { "dv",             0x00065 },
    { "dv-MV",          0x00465 },
    { "dz-BT",          0x00C51 },
    { "el",             0x00008 },
    { "el-GR",          0x00408 },
    { "en",             0x00009 },
    { "en-029",         0x02409 }, // Doc: reserved
    { "en-AE",          0x04C09 },
    { "en-AU",          0x00C09 },
    // { "en-BH",          0x05009 }, // reserved
    { "en-BZ",          0x02809 },
    { "en-CA",          0x01009 },
    // { "en-EG",          0x05409 }, // reserved
    { "en-GB",          0x00809 },
    { "en-HK",          0x03C09 },
    { "en-ID",          0x03809 }, // reserved
    { "en-IE",          0x01809 },
    { "en-IN",          0x04009 },
    { "en-JM",          0x02009 },
    // { "en-JO",          0x05809 }, // reserved
    // { "en-KW",          0x05C09 }, // reserved
    { "en-MY",          0x04409 },
    { "en-NZ",          0x01409 },
    { "en-PH",          0x03409 },
    { "en-SG",          0x04809 },
    // { "en-TR",          0x6009 }, // reserved
    { "en-TT",          0x02C09 },
    { "en-US",          0x00409 },
    // { "en-YE",          0x6409 }, // reserved
    { "en-ZA",          0x01C09 },
    { "en-ZW",          0x03009 },
    { "es",             0x0000A },
    { "es-419",         0x0580A }, // Doc: reserved
    { "es-AR",          0x02C0A },
    { "es-BO",          0x0400A },
    { "es-CL",          0x0340A },
    { "es-CO",          0x0240A },
    { "es-CR",          0x0140A },
    { "es-CU",          0x05C0A },
    { "es-DO",          0x01C0A },
    { "es-EC",          0x0300A },
    { "es-ES",          0x00C0A },
    { "es-ES_tradnl",   0x0040A },
    { "es-GT",          0x0100A },
    { "es-HN",          0x0480A },
    { "es-MX",          0x0080A },
    { "es-NI",          0x04C0A },
    { "es-PA",          0x0180A },
    { "es-PE",          0x0280A },
    { "es-PR",          0x0500A },
    { "es-PY",          0x03C0A },
    { "es-SV",          0x0440A },
    { "es-US",          0x0540A },
    { "es-UY",          0x0380A },
    { "es-VE",          0x0200A },
    { "et",             0x00025 },
    { "et-EE",          0x00425 },
    { "eu",             0x0002D },
    { "eu-ES",          0x0042D },
    { "fa",             0x00029 },
    { "fa-IR",          0x00429 },
    { "ff",             0x00067 },
    { "ff-Latn",        0x07C67 },
    { "ff-Latn-NG",     0x00467 },
    { "ff-Latn-SN",     0x00867 },
    { "ff-NG",          0x00467 | LCID_ALT_NAME },
    { "fi",             0x0000B },
    { "fi-FI",          0x0040B },
    { "fil",            0x00064 },
    { "fil-PH",         0x00464 },
    { "fo",             0x00038 },
    { "fo-FO",          0x00438 },
    { "fr",             0x0000C },
    // { "fr-015",         0x0E40C }, // reserved
    { "fr-029",         0x01C0C },
    { "fr-BE",          0x0080C },
    { "fr-CA",          0x00C0C },
    { "fr-CD",          0x0240C },
    { "fr-CH",          0x0100C },
    { "fr-CI",          0x0300C },
    { "fr-CM",          0x02C0C },
    { "fr-FR",          0x0040C },
    { "fr-HT",          0x03C0C },
    { "fr-LU",          0x0140C },
    { "fr-MA",          0x0380C },
    { "fr-MC",          0x0180C },
    { "fr-ML",          0x0340C },
    { "fr-RE",          0x0200C },
    { "fr-SN",          0x0280C },
    { "fy",             0x00062 },
    { "fy-NL",          0x00462 },
    { "ga",             0x0003C },
    { "ga-IE",          0x0083C },
    { "gd",             0x00091 },
    { "gd-GB",          0x00491 },
    { "gl",             0x00056 },
    { "gl-ES",          0x00456 },
    { "gn",             0x00074 },
    { "gn-PY",          0x00474 },
    { "gsw",            0x00084 },
    { "gsw-FR",         0x00484 },
    { "gu",             0x00047 },
    { "gu-IN",          0x00447 },
    { "ha",             0x00068 },
    { "ha-Latn",        0x07C68 },
    { "ha-Latn-NG",     0x00468 },
    { "haw",            0x00075 },
    { "haw-US",         0x00475 },
    { "he",             0x0000D },
    { "he-IL",          0x0040D },
    { "hi",             0x00039 },
    { "hi-IN",          0x00439 },
    { "hr",             0x0001A },
    { "hr-BA",          0x0101A },
    { "hr-HR",          0x0041A },
    { "hsb",            0x0002E },
    { "hsb-DE",         0x0042E },
    { "hu",             0x0000E },
    { "hu-HU",          0x0040E },
    { "hu-HU_technl",   0x1040E },
    { "hy",             0x0002B },
    { "hy-AM",          0x0042B },
    { "ibb",            0x00069 }, // Doc: reserved
    { "ibb-NG",         0x00469 }, // Doc: reserved
    { "id",             0x00021 },
    { "id-ID",          0x00421 },
    { "ig",             0x00070 },
    { "ig-NG",          0x00470 },
    { "ii",             0x00078 },
    { "ii-CN",          0x00478 },
    { "is",             0x0000F },
    { "is-IS",          0x0040F },
    { "it",             0x00010 },
    { "it-CH",          0x00810 },
    { "it-IT",          0x00410 },
    { "iu",             0x0005D },
    { "iu-Cans",        0x0785D },
    { "iu-Cans-CA",     0x0045D },
    { "iu-Latn",        0x07C5D },
    { "iu-Latn-CA",     0x0085D },
    { "ja",             0x00011 },
    { "ja-JP",          0x00411 },
    { "ja-JP_radstr",   0x040411 },
    // { "ja-Ploc-JP",     0x00811 }, // reserved
    { "ka",             0x00037 },
    { "ka-GE",          0x00437 },
    { "ka-GE_modern",   0x10437 },
    // { "khb-Talu-CN",    0x00490 }, // reserved
    { "kk",             0x0003F },
    { "kk-Cyrl",        0x0003F | LCID_ALT_NAME }, // Doc: 0x0783F, reserved
    { "kk-KZ",          0x0043F },
    // { "kk-Latn",        0x07C3F }, // reserved
    // { "kk-Latn-KZ",     0x0083F }, // reserved
    { "kl",             0x0006F },
    { "kl-GL",          0x0046F },
    { "km",             0x00053 },
    { "km-KH",          0x00453 },
    { "kn",             0x0004B },
    { "kn-IN",          0x0044B },
    { "ko",             0x00012 },
    { "ko-KR",          0x00412 },
    { "kok",            0x00057 },
    { "kok-IN",         0x00457 },
    { "kr",             0x00071 }, // Doc: reserved
    { "kr-Latn-NG",     0x00471 },
    { "ks",             0x00060 },
    { "ks-Arab",        0x00460 }, // extended
    { "ks-Deva-IN",     0x00860 },
    { "ku",             0x00092 },
    { "ku-Arab",        0x07C92 },
    { "ku-Arab-IQ",     0x00492 },
    { "ky",             0x00040 },
    { "ky-KG",          0x00440 },
    { "la",             0x00076 }, // reserved
    { "la-001",         0x00476 }, // Doc: la-VA
    { "lb",             0x0006E },
    { "lb-LU",          0x0046E },
    { "lo",             0x00054 },
    { "lo-LA",          0x00454 },
    { "lt",             0x00027 },
    { "lt-LT",          0x00427 },
    { "lv",             0x00026 },
    { "lv-LV",          0x00426 },
    { "mi",             0x00081 },
    { "mi-NZ",          0x00481 },
    { "mk",             0x0002F },
    { "mk-MK",          0x0042F },
    { "ml",             0x0004C },
    { "ml-IN",          0x0044C },
    { "mn",             0x00050 },
    { "mn-Cyrl",        0x07850 },
    { "mn-MN",          0x00450 },
    { "mn-Mong",        0x07C50 },
    { "mn-Mong-CN",     0x00850 }, // Doc: reserved
    { "mn-Mong-MN",     0x00C50 },
    { "mni",            0x00058 }, // Doc: reserved
    { "mni-IN",         0x00458 }, // Doc: reserved
    { "moh",            0x0007C },
    { "moh-CA",         0x0047C },
    { "mr",             0x0004E },
    { "mr-IN",          0x0044E },
    { "ms",             0x0003E },
    { "ms-BN",          0x0083E },
    { "ms-MY",          0x0043E },
    { "mt",             0x0003A },
    { "mt-MT",          0x0043A },
    { "my",             0x00055 },
    { "my-MM",          0x00455 },
    { "nb",             0x07C14 },
    { "nb-NO",          0x00414 },
    { "ne",             0x00061 },
    { "ne-IN",          0x00861 },
    { "ne-NP",          0x00461 },
    { "nl",             0x00013 },
    { "nl-BE",          0x00813 },
    { "nl-NL",          0x00413 },
    { "nn",             0x07814 },
    { "nn-NO",          0x00814 },
    { "no",             0x00014 },
    { "nso",            0x0006C },
    { "nso-ZA",         0x0046C },
    { "oc",             0x00082 },
    { "oc-FR",          0x00482 },
    { "om",             0x00072 },
    { "om-ET",          0x00472 },
    { "or",             0x00048 },
    { "or-IN",          0x00448 },
    { "pa",             0x00046 },
    { "pa-Arab",        0x07C46 },
    { "pa-Arab-PK",     0x00846 },
    { "pa-IN",          0x00446 },
    { "pap",            0x00079 }, // Doc: reserved
    { "pap-029",        0x00479 }, // Doc: reserved
    { "pl",             0x00015 },
    { "pl-PL",          0x00415 },
    // { "plt-MG",         0x0048D }, // reserved
    { "prs",            0x0008C },
    { "prs-AF",         0x0048C },
    { "ps",             0x00063 },
    { "ps-AF",          0x00463 },
    { "pt",             0x00016 },
    { "pt-BR",          0x00416 },
    { "pt-PT",          0x00816 },
    { "qps-ploc",       0x00501 },
    { "qps-ploca",      0x005FE },
    { "qps-plocm",      0x009FF },
    { "quc",            0x00086 }, // Doc: 0x00093, reserved
    // { "quc-CO",         0x00493 }, // reserved
    { "quc-Latn-GT",    0x00486 }, // Doc: qut-GT, reserved
    { "qut",            0x00086 | LCID_ALT_NAME },
    { "qut-GT",         0x00486 | LCID_ALT_NAME }, // reserved
    { "quz",            0x0006B },
    { "quz-BO",         0x0046B },
    { "quz-EC",         0x0086B },
    { "quz-PE",         0x00C6b },
    { "rm",             0x00017 },
    { "rm-CH",          0x00417 },
    { "ro",             0x00018 },
    { "ro-MD",          0x00818 },
    { "ro-RO",          0x00418 },
    { "ru",             0x00019 },
    { "ru-MD",          0x00819 },
    { "ru-RU",          0x00419 },
    { "rw",             0x00087 },
    { "rw-RW",          0x00487 },
    { "sa",             0x0004F },
    { "sa-IN",          0x0044F },
    { "sah",            0x00085 },
    { "sah-RU",         0x00485 },
    { "sd",             0x00059 },
    { "sd-Arab",        0x07C59 },
    { "sd-Arab-PK",     0x00859 },
    { "sd-Deva-IN",     0x00459 }, // Doc: reserved
    { "se",             0x0003B },
    { "se-FI",          0x00C3B },
    { "se-NO",          0x0043B },
    { "se-SE",          0x0083B },
    { "si",             0x0005B },
    { "si-LK",          0x0045B },
    { "sk",             0x0001B },
    { "sk-SK",          0x0041B },
    { "sl",             0x00024 },
    { "sl-SI",          0x00424 },
    { "sma",            0x0783B },
    { "sma-NO",         0x0183B },
    { "sma-SE",         0x01C3B },
    { "smj",            0x07C3B },
    { "smj-NO",         0x0103B },
    { "smj-SE",         0x0143B },
    { "smn",            0x0703B },
    { "smn-FI",         0x0243B },
    { "sms",            0x0743B },
    { "sms-FI",         0x0203B },
    { "so",             0x00077 }, // Doc: reserved
    { "so-SO",          0x00477 },
    { "sq",             0x0001C },
    { "sq-AL",          0x0041C },
    { "sr",             0x07C1A },
    { "sr-Cyrl",        0x06C1A },
    { "sr-Cyrl-BA",     0x01C1A },
    { "sr-Cyrl-CS",     0x00C1A },
    { "sr-Cyrl-ME",     0x0301A },
    { "sr-Cyrl-RS",     0x0281A },
    { "sr-Latn",        0x0701A },
    { "sr-Latn-BA",     0x0181A },
    { "sr-Latn-CS",     0x0081A },
    { "sr-Latn-ME",     0x02C1A },
    { "sr-Latn-RS",     0x0241A },
    { "st",             0x00030 },
    { "st-ZA",          0x00430 },
    { "sv",             0x0001D },
    { "sv-FI",          0x0081D },
    { "sv-SE",          0x0041D },
    { "sw",             0x00041 },
    { "sw-KE",          0x00441 },
    { "syr",            0x0005A },
    { "syr-SY",         0x0045A },
    { "ta",             0x00049 },
    { "ta-IN",          0x00449 },
    { "ta-LK",          0x00849 },
    // { "tdd-Tale-CN",    0x0048F }, // reserved
    { "te",             0x0004A },
    { "te-IN",          0x0044A },
    { "tg",             0x00028 },
    { "tg-Cyrl",        0x07C28 },
    { "tg-Cyrl-TJ",     0x00428 },
    { "th",             0x0001E },
    { "th-TH",          0x0041E },
    { "ti",             0x00073 },
    { "ti-ER",          0x00873 },
    { "ti-ET",          0x00473 },
    { "tk",             0x00042 },
    { "tk-TM",          0x00442 },
    // { "tmz-MA",         0x00C5F }, // reserved
    { "tn",             0x00032 },
    { "tn-BW",          0x00832 },
    { "tn-ZA",          0x00432 },
    { "tr",             0x0001F },
    { "tr-TR",          0x0041F },
    { "ts",             0x00031 },
    { "ts-ZA",          0x00431 },
    { "tt",             0x00044 },
    { "tt-RU",          0x00444 },
    { "tzm",            0x0005F },
    { "tzm-Arab-MA",    0x0045F },
    { "tzm-Latn",       0x07C5F },
    { "tzm-Latn-DZ",    0x0085F },
    { "tzm-Tfng",       0x0785F },
    { "tzm-Tfng-MA",    0x0105F },
    { "ug",             0x00080 },
    { "ug-CN",          0x00480 },
    { "uk",             0x00022 },
    { "uk-UA",          0x00422 },
    { "ur",             0x00020 },
    { "ur-IN",          0x00820 },
    { "ur-PK",          0x00420 },
    { "uz",             0x00043 },
    { "uz-Cyrl",        0x07843 },
    { "uz-Cyrl-UZ",     0x00843 }, // Doc: reserved
    { "uz-Latn",        0x07C43 },
    { "uz-Latn-UZ",     0x00443 },
    { "ve",             0x00033 },
    { "ve-ZA",          0x00433 },
    { "vi",             0x0002A },
    { "vi-VN",          0x0042A },
    { "wo",             0x00088 },
    { "wo-SN",          0x00488 },
    { "xh",             0x00034 },
    { "xh-ZA",          0x00434 },
    { "yi",             0x0003D }, // Doc: reserved
    { "yi-001",         0x0043D },
    { "yo",             0x0006A },
    { "yo-NG",          0x0046A },
    { "zh",             0x07804 },
    { "zh-CN",          0x00804 },
    { "zh-CN_stroke",   0x20804 },
    { "zh-Hans",        0x00004 },
    { "zh-Hant",        0x07C04 },
    { "zh-HK",          0x00C04 },
    { "zh-HK_radstr",   0x40C04 },
    { "zh-MO",          0x01404 },
    { "zh-MO_radstr",   0x41404 },
    { "zh-SG",          0x01004 },
    { "zh-SG_stroke",   0x21004 },
    { "zh-TW",          0x00404 },
    { "zh-TW_pronun",   0x30404 },
    { "zh-TW_radstr",   0x40404 },
    // { "zh-yue-HK",      0x0048E }, // reserved
    { "zu",             0x00035 },
    { "zu-ZA",          0x00435 },
};

// This table will be sorted by LCID at runtime
static USHORT RtlpLocaleIndexTable[_ARRAYSIZE(RtlpLocaleTable)];

typedef struct _SORT_ENTRY
{
    LCID Lcid;
    USHORT Index;
} SORT_ENTRY, *PSORT_ENTRY;

// Callback function for qsort to sort the SORT_ENTRY table by LCID
static int __cdecl LcidSortEntryCompare(const void* a, const void* b)
{
    PSORT_ENTRY SortEntryA = (PSORT_ENTRY)a;
    PSORT_ENTRY SortEntryB = (PSORT_ENTRY)b;
    return SortEntryA->Lcid - SortEntryB->Lcid;
}

//
// Creates a temporary table, that maps the LCIDs to the index in the
// alphabetical table. This table is then sorted by LCID and the indices
// are used to create the final table.
//
NTSTATUS
NTAPI
RtlpInitializeLocaleTable(VOID)
{
    PSORT_ENTRY SortTable;
    SIZE_T SortTableSize;

    NtQueryDefaultLocale(TRUE, &RtlpUserDefaultLcid);
    NtQueryDefaultLocale(FALSE, &RtlpSystemDefaultLcid);

    SortTableSize = sizeof(SortTable[0]) * ARRAYSIZE(RtlpLocaleTable);
    SortTable = RtlAllocateHeap(RtlGetProcessHeap(), 0, SortTableSize);
    if (SortTable == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the LCIDs and the index */
    for (USHORT i = 0; i < ARRAYSIZE(RtlpLocaleTable); i++)
    {
        SortTable[i].Lcid = RtlpLocaleTable[i].Lcid;
        SortTable[i].Index = i;
    }

    /* Sort the table by LCID */
    qsort(SortTable,
          ARRAYSIZE(RtlpLocaleTable),
          sizeof(SortTable[0]),
          LcidSortEntryCompare);

    /* Copy the sorted indices to the global table */
    for (USHORT i = 0; i < ARRAYSIZE(RtlpLocaleTable); i++)
    {
        RtlpLocaleIndexTable[i] = SortTable[i].Index;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, SortTable);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
static
ULONG
FindIndexByLcid(
    _In_ LCID Lcid)
{
    USHORT TableIndex;
    LONG Low = 0;
    LONG High = ARRAYSIZE(RtlpLocaleTable) - 1;
    LONG Middle;
    LCID CurrentLcid;

    while (Low <= High)
    {
        Middle = (Low + High) / 2;

        /* Use the indirection table to get the real table entry */
        TableIndex = RtlpLocaleIndexTable[Middle];
        ASSERT(TableIndex < ARRAYSIZE(RtlpLocaleTable));

        /* Compare the LCID (including the alternative name flag!) */
        CurrentLcid = RtlpLocaleTable[TableIndex].Lcid;
        if (CurrentLcid < Lcid)
        {
            Low = Middle + 1;
        }
        else if (CurrentLcid > Lcid)
        {
            High = Middle - 1;
        }
        else /* CurrentLcid == Lcid */
        {
            return RtlpLocaleIndexTable[Middle];
        }
    }

    return MAXULONG;
}

#define LOWERCASE_CHAR(Char) \
    (((Char) >= 'A' && (Char) <= 'Z') ? ((Char) + ('a' - 'A')) : (Char))

_Must_inspect_result_
static
INT
CompareLocaleNames(
    _In_ PCSTR AsciiLocaleName,
    _In_ PCUNICODE_STRING UnicodeLocaleName)
{
    ULONG i;

    /* Do a case-insensitive comparison. */
    for (i = 0; i < UnicodeLocaleName->Length / sizeof(WCHAR); i++)
    {
        /* Make the 2 chars lower case for comparison */
        CHAR Char1 = LOWERCASE_CHAR(AsciiLocaleName[i]);
        WCHAR Char2 = LOWERCASE_CHAR(UnicodeLocaleName->Buffer[i]);

        /* Keep comparing, while they are equal */
        if (Char1 == Char2)
        {
            continue;
        }

        /* Check if the ASCII string ends first */
        if (AsciiLocaleName[i] == '\0')
        {
            /* The 1st string is shorter than the 2nd, i.e. smaller */
            return -1;
        }

        /* Return the difference between the two characters */
        return (INT)Char1 - (INT)Char2;
    }

    /* The strings match up to the lenth of the unicode string.
       If the ASCII string ends here, they are equal. */
    if (AsciiLocaleName[i] == '\0')
    {
        return 0;
    }

    /* The 1st string is longer than the 2nd, i.e. larger */
    return 1;
}

_Must_inspect_result_
static
ULONG
FindIndexByLocaleName(
    _In_ PCUNICODE_STRING LocaleName)
{
    LONG Low = 0;
    LONG High = ARRAYSIZE(RtlpLocaleTable) - 1;
    LONG Middle;
    PCSTR CurrentLocaleName;
    INT CompareResult;

    while (Low <= High)
    {
        Middle = (Low + High) / 2;

        CurrentLocaleName = RtlpLocaleTable[Middle].Locale;
        CompareResult = CompareLocaleNames(CurrentLocaleName, LocaleName);
        if (CompareResult < 0)
        {
            Low = Middle + 1;
        }
        else if (CompareResult > 0)
        {
            High = Middle - 1;
        }
        else /* CompareResult == 0 */
        {
            return Middle;
        }
    }

    return MAXULONG;
}

_Must_inspect_result_
static
BOOLEAN
CopyAsciizToUnicodeString(
    _Inout_ PUNICODE_STRING UnicodeString,
    _In_ PCSTR AsciiString)
{
    SIZE_T AsciiLength = strlen(AsciiString);

    /* Make sure we can copy the full string, including the terminating 0 */
    if (UnicodeString->MaximumLength < (AsciiLength + 1) * sizeof(WCHAR))
    {
        return FALSE;
    }

    /* Copy the string manually */
    for (SIZE_T i = 0; i < AsciiLength; i++)
    {
        UnicodeString->Buffer[i] = (WCHAR)AsciiString[i];
    }

    /* Add the terminating 0 and update the Length */
    UnicodeString->Buffer[AsciiLength] = UNICODE_NULL;
    UnicodeString->Length = (USHORT)(AsciiLength * sizeof(WCHAR));

    return TRUE;
}

static
BOOLEAN
IsNeutralLocale(
    _In_ LCID Lcid)
{
    /* Check if the LCID is within the neutral locale range */
    if (((Lcid <= MAX_PRIMARY_LANGUAGE) && (Lcid != LOCALE_INVARIANT)) ||
        (Lcid == 0x0460) /* ks-Arab */ ||
        ((Lcid & 0xFFFF) > MAX_BASIC_LCID))
    {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NTAPI
RtlLcidToLocaleName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING LocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString)
{
    ULONG LocaleIndex;

    /* Check for invalid flags */
    if (Flags & ~0x2)
    {
        DPRINT1("RtlLcidToLocaleName: Invalid flags: 0x%lx\n", Flags);
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check if the LocaleName buffer is valid */
    if ((LocaleName == NULL) || (LocaleName->Buffer == NULL))
    {
        DPRINT1("RtlLcidToLocaleName: Invalid buffer\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Validate LCID */
    if (Lcid & ~NLS_VALID_LOCALE_MASK)
    {
        DPRINT1("RtlLcidToLocaleName: Invalid LCID: 0x%lx\n", Lcid);
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check if neutral locales were requested */
    if ((Flags & RTL_LOCALE_ALLOW_NEUTRAL_NAMES) == 0)
    {
        /* Check if this is a neutral locale */
        if (IsNeutralLocale(Lcid))
        {
            DPRINT("RtlLcidToLocaleName: Neutral LCID: 0x%lx\n", Lcid);
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    /* Handle special LCIDs */
    switch (Lcid)
    {
        case LOCALE_USER_DEFAULT:
            Lcid = RtlpUserDefaultLcid;
            break;

        case LOCALE_SYSTEM_DEFAULT:
        case LOCALE_CUSTOM_DEFAULT:
            Lcid = RtlpSystemDefaultLcid;
            break;

        case LOCALE_CUSTOM_UI_DEFAULT:
            return STATUS_UNSUCCESSFUL;
    }

    /* Try to find the locale by LCID */
    LocaleIndex = FindIndexByLcid(Lcid);
    if (LocaleIndex == MAXULONG)
    {
        DPRINT("RtlLcidToLocaleName: LCID 0x%lx not found\n", Lcid);
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Copy the locale name to the buffer */
    if (!CopyAsciizToUnicodeString(LocaleName, RtlpLocaleTable[LocaleIndex].Locale))
    {
        DPRINT("RtlLcidToLocaleName: Buffer too small\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
static
NTSTATUS
RtlpLocaleNameToLcidInternal(
    _In_ PCUNICODE_STRING LocaleName,
    _Out_ PLCID Lcid,
    _In_ ULONG Flags)
{
    ULONG LocaleIndex;
    LCID FoundLcid;

    /* Check if LocaleName points to a valid unicode string */
    if ((LocaleName == NULL) || (LocaleName->Buffer == NULL))
    {
        DPRINT1("RtlpLocaleNameToLcidInternal: Invalid buffer\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Check if the Lcid pointer is valid */
    if (Lcid == NULL)
    {
        DPRINT1("RtlpLocaleNameToLcidInternal: Lcid is NULL\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Check for invalid flags */
    if (Flags & ~0x3)
    {
        DPRINT1("RtlpLocaleNameToLcidInternal: Invalid flags: 0x%lx\n", Flags);
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Try to find the locale */
    LocaleIndex = FindIndexByLocaleName(LocaleName);
    if (LocaleIndex == MAXULONG)
    {
        DPRINT("RtlpLocaleNameToLcidInternal: Locale name not found\n");
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Extract the LCID without the flags */
    FoundLcid = RtlpLocaleTable[LocaleIndex].Lcid & NLS_VALID_LOCALE_MASK;

    /* Check if neutral locales were requested */
    if ((Flags & RTL_LOCALE_ALLOW_NEUTRAL_NAMES) == 0)
    {
        /* Check if this is a neutral locale */
        if (IsNeutralLocale(FoundLcid))
        {
            DPRINT("RtlpLocaleNameToLcidInternal: Neutral LCID: 0x%lx\n", FoundLcid);
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    /* Copy the LCID to the output buffer */
    *Lcid = FoundLcid;

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
RtlLocaleNameToLcid(
    _In_ PCWSTR LocaleName,
    _Out_ PLCID Lcid,
    _In_ ULONG Flags)
{
    UNICODE_STRING LocaleNameString;

    /* Convert the string to a UNICODE_STRING */
    RtlInitUnicodeString(&LocaleNameString, LocaleName);

    /* Forward to internal function */
    return RtlpLocaleNameToLcidInternal(&LocaleNameString, Lcid, Flags);
}

_Success_(return != FALSE)
BOOLEAN
NTAPI
RtlLCIDToCultureName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING String)
{
    NTSTATUS Status;

    /* Forward to RtlLcidToLocaleName, passing flag 2 to get the extended stuff */
    Status = RtlLcidToLocaleName(Lcid, String, 0x2, FALSE);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

_Success_(return != FALSE)
BOOLEAN
NTAPI
RtlCultureNameToLCID(
    _In_ PCUNICODE_STRING String,
    _Out_ PLCID Lcid)
{
    NTSTATUS Status;

    if ((String == NULL) ||
        (String->Buffer == NULL) ||
        (String->Buffer[0] == UNICODE_NULL))
    {
        return FALSE;
    }

    /* Forward to internal function with flag 0x2 */
    Status = RtlpLocaleNameToLcidInternal(String, Lcid, 0x2);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
RtlIsValidLocaleName(
    _In_ LPCWSTR LocaleName,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
    return TRUE;
}

NTSTATUS
NTAPI
RtlConvertLCIDToString(
    _In_ LCID LcidValue,
    _In_ ULONG Base,
    _In_ ULONG Padding,
    _Out_writes_(Size) PWSTR pResultBuf,
    _In_ ULONG Size)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
