/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Public header
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
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

extern HANDLE ProcessHeap;

#include "errorcode.h"
#include "linklist.h"
#include "fsutil.h"
#include "genlist.h"
#include "partlist.h"

/* EOF */
