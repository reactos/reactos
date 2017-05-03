/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/concfg.h
 * PURPOSE:         Console settings management - Public header
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

/* Needed PSDK headers when using this library */
#if 0

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <wingdi.h> // For LF_FACESIZE and TranslateCharsetInfo()
#include <wincon.h>
#include <winnls.h> // For code page support
#include <winreg.h>

#endif

/* NOTE: Please keep the header inclusion order! */

#include "settings.h"
#include "font.h"

/* EOF */
