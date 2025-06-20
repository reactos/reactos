/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of LocaleNameToLCID
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "k32_vista.h"
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

LCID
WINAPI
LocaleNameToLCID(
    _In_ LPCWSTR lpName,
    _In_ DWORD dwFlags)
{
    NTSTATUS Status;
    LCID Lcid;

    /* Validate flags */
    if (dwFlags & ~LOCALE_ALLOW_NEUTRAL_NAMES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Check for LOCALE_NAME_USER_DEFAULT (NULL) */
    if (lpName == NULL)
    {
        return GetUserDefaultLCID();
    }

    /* Check for LOCALE_NAME_INVARIANT (L"") */
    if (lpName[0] == L'\0')
    {
        return MAKELCID(MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL), SORT_DEFAULT);
    }

    /* Call the RTL function (include neutral names) */
    Status = RtlLocaleNameToLcid(lpName, &Lcid, RTL_LOCALE_ALLOW_NEUTRAL_NAMES);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Are neutral locales allowed? */
    if ((dwFlags & LOCALE_ALLOW_NEUTRAL_NAMES) == 0)
    {
        /* Check if we got a neutral locale */
        LANGID LangId = LANGIDFROMLCID(Lcid);
        USHORT SubLangId = SUBLANGID(LangId);
        if (SubLangId == SUBLANG_NEUTRAL)
        {
            /* Adjust it to be the default locale */
            Lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LangId), SUBLANG_DEFAULT), SORT_DEFAULT);
        }
        else if (SubLangId >= 0x18)
        {
           /* Handle special neutral LCIDs */
            switch (Lcid)
            {
                case 0x0742C: return 0x0082C; // "az-Cyrl" -> "az-Cyrl-AZ"
                case 0x0782C: return 0x0042C; // "az-Latn" -> "az-Latn-AZ"
                case 0x07C67: return 0x00867; // "ff-Latn" -> "ff-Latn-SN"
                case 0x0781A: return 0x0141A; // "bs" -> "bs-Latn-BA"
                case 0x0641A: return 0x0201A; // "bs-Cyrl" -> "bs-Cyrl-BA"
                case 0x0681A: return 0x0141A; // "bs-Latn" -> "bs-Latn-BA"
                case 0x07C5C: return 0x0045C; // "chr-Cher" -> "chr-Cher-US"
                case 0x07C2E: return 0x0082E; // "dsb" -> "dsb-DE"
                case 0x07C68: return 0x00468; // "ha-Latn" -> "ha-Latn-NG"
                case 0x0785D: return 0x0045D; // "iu-Cans" -> "iu-Cans-CA"
                case 0x07C5D: return 0x0085D; // "iu-Latn" -> "iu-Latn-CA"
                case 0x07C92: return 0x00492; // "ku-Arab" -> "ku-Arab-IQ"
                case 0x07850: return 0x00450; // "mn-Cyrl" -> "mn-MN"
                case 0x07C50: return 0x00850; // "mn-Mong" -> "mn-Mong-CN"
                case 0x07C14: return 0x00414; // "nb" -> "nb-NO"
                case 0x07814: return 0x00814; // "nn" -> "nn-NO"
                case 0x07C46: return 0x00846; // "pa-Arab" -> "pa-Arab-PK"
                case 0x0703B: return 0x0243B; // "smn" -> "smn-FI"
                case 0x07C59: return 0x00859; // "sd-Arab" -> "sd-Arab-PK"
                case 0x0783B: return 0x01C3B; // "sma" -> "sma-SE"
                case 0x07C3B: return 0x0143B; // "smj" -> "smj-SE"
                case 0x0743B: return 0x0203B; // "sms" -> "sms-FI"
                case 0x06C1A: return 0x0281A; // "sr-Cyrl" -> "sr-Cyrl-RS"
                case 0x0701A: return 0x0241A; // "sr-Latn" -> "sr-Latn-RS"
                case 0x07C28: return 0x00428; // "tg-Cyrl" -> "tg-Cyrl-TJ"
                case 0x07C5F: return 0x0085F; // "tzm-Latn" -> "tzm-Latn-DZ"
                case 0x0785F: return 0x0105F; // "tzm-Tfng" -> "tzm-Tfng-MA"
                case 0x07843: return 0x00843; // "uz-Cyrl" -> "uz-Cyrl-UZ"
                case 0x07C43: return 0x00443; // "uz-Latn" -> "uz-Latn-UZ"
                case 0x07804: return 0x00804; // "zh" -> "zh-CN"
                case 0x07C04: return 0x00C04; // "zh-Hant" -> "zh-HK"
            }

            /* Should not happen */
            DPRINT1("Unandled neutral LCID %x\n", Lcid);
            ASSERT(FALSE);
            return 0;
        }
    }

    return Lcid;
}
