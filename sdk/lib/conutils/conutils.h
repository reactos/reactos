/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides simple abstraction wrappers around CRT streams or
 *              Win32 console API I/O functions, to deal with i18n + Unicode
 *              related problems.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    conutils.h
 * @ingroup ConUtils
 *
 * @defgroup ConUtils   ReactOS Console Utilities Library
 *
 * @brief   This library contains common functions used in many places inside
 *          the ReactOS console utilities and the ReactOS Command-Line Interpreter.
 *          Most of these functions are related with internationalisation and
 *          the problem of correctly displaying Unicode text on the console.
 *          Besides those, helpful functions for retrieving strings and messages
 *          from application resources are provided, together with printf-like
 *          functionality.
 **/

#ifndef __CONUTILS_H__
#define __CONUTILS_H__

#pragma once

#ifndef _UNICODE
#error The ConUtils library only supports compilation with _UNICODE defined, at the moment!
#endif

#include "utils.h"
#include "stream.h"
// #include "instream.h"
#include "outstream.h"
#include "screen.h"
#include "pager.h"

#endif  /* __CONUTILS_H__ */

/* EOF */
