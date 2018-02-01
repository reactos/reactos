/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides basic abstraction wrappers around CRT streams or
 *              Win32 console API I/O functions, to deal with i18n + Unicode
 *              related problems.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    instream.c
 * @ingroup ConUtils
 *
 * @brief   Console I/O utility API -- Input
 **/

/*
 * Enable this define if you want to only use CRT functions to output
 * UNICODE stream to the console, as in the way explained by
 * http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
 */
/** NOTE: Experimental! Don't use USE_CRT yet because output to console is a bit broken **/
// #define USE_CRT

/* FIXME: Temporary HACK before we cleanly support UNICODE functions */
#define UNICODE
#define _UNICODE

#ifdef USE_CRT
#include <fcntl.h>
#include <io.h>
#endif /* USE_CRT */

#include <stdlib.h> // limits.h // For MB_LEN_MAX

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h> // MAKEINTRESOURCEW, RT_STRING
#include <wincon.h>  // Console APIs (only if kernel32 support included)
#include <strsafe.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include "conutils.h"
#include "stream.h"
#include "stream_private.h"


/* EOF */
