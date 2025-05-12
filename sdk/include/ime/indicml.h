/*
 * PROJECT:     ReactOS headers
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Indicator (internat.exe) window class
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifndef _INDICML_
#define _INDICML_

#pragma once

#define INDICATOR_CLASSW L"Indicator"
#define INDICATOR_CLASSA  "Indicator"

#ifdef UNICODE
#define INDICATOR_CLASS INDICATOR_CLASSW
#else
#define INDICATOR_CLASS INDICATOR_CLASSA
#endif

#define INDICM_SETIMEICON              (WM_USER + 100)
#define INDICM_SETIMETOOLTIPS          (WM_USER + 101)
#define INDICM_REMOVEDEFAULTMENUITEMS  (WM_USER + 102)

/* wParam flags for INDICM_REMOVEDEFAULTMENUITEMS */
#define RDMI_LEFT  1
#define RDMI_RIGHT 2

#endif /* ndef _INDICML_ */
