/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/precomp.h
 * PURPOSE:         Console settings management - Precompiled header
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

// #pragma once

/* PSDK/NDK Headers */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h> // For LF_FACESIZE and TranslateCharsetInfo()
#include <wincon.h>
#include <winnls.h> // For code page support
#include <winreg.h>
// #include <imm.h>

// /* Undocumented user definitions */
// #include <undocuser.h>

#define NTOS_MODE_USER
// #include <ndk/cmfuncs.h>
// #include <ndk/exfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>

#include <stdio.h> // for swprintf
#include <strsafe.h>

/* EOF */
