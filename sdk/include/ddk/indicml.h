/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     DDK-compatible <indicml.h> for the Indicator
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#ifndef _INDICML_
#define _INDICML_

#ifdef __cplusplus
extern "C" {
#endif

// Indicator window class name
#ifdef _WIN32
    #define INDICATOR_CLASSA  "Indicator"
    #define INDICATOR_CLASSW L"Indicator"

    #ifdef UNICODE
        #define INDICATOR_CLASS  INDICATOR_CLASSW
    #else
        #define INDICATOR_CLASS  INDICATOR_CLASSA
    #endif
#else
    #define INDICATOR_CLASS  "Indicator"
#endif

// Indicator messages
#define INDICM_SETIMEICON             (WM_USER + 100)
#define INDICM_SETIMETOOLTIPS         (WM_USER + 101)
#define INDICM_REMOVEDEFAULTMENUITEMS (WM_USER + 102)

// wParam for INDICM_REMOVEDEFAULTMEUITEMS
#define RDMI_LEFT  0x0001
#define RDMI_RIGHT 0x0002

#ifdef __cplusplus
}
#endif

#endif // _INDICML_
