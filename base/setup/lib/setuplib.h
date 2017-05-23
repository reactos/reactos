/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/setuplib.h
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

extern HANDLE ProcessHeap;

#include "errorcode.h"
#include "linklist.h"
#include "ntverrsrc.h"
// #include "arcname.h"
#include "filesup.h"
#include "fsutil.h"
#include "genlist.h"
#include "inicache.h"
#include "partlist.h"
#include "arcname.h"
#include "osdetect.h"

/* EOF */
