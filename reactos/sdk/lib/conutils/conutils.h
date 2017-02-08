/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/conutils.h
 * PURPOSE:         Provides simple abstraction wrappers around CRT streams or
 *                  Win32 console API I/O functions, to deal with i18n + Unicode
 *                  related problems.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __CONUTILS_H__
#define __CONUTILS_H__

#ifndef _UNICODE
#error The ConUtils library only supports compilation with _UNICODE defined, at the moment!
#endif

#include "utils.h"
#include "stream.h"
#include "screen.h"
#include "pager.h"

#endif  /* __CONUTILS_H__ */
