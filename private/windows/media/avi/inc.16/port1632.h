/***************************************************************************\
* Module Name: 1632PORT.H
*
* Copyright (c) 1985-1994, Microsoft Corporation
*
* Master include file for Portable Windows applications.
*
\***************************************************************************/

/*
 * This file maps a Meta-API for Windows to specific 16-bit or 32-bit forms
 * allowing a single portable C source for windows to work on multiple
 * versions of Windows.
 */

#ifndef _PORT1632_
#define _PORT1632_

#if defined(WIN16)
/* ---------------- Maps to windows 3.0 and 3.1 16-bit APIs ----------------*/
#include "ptypes16.h"
#include "pwin16.h"
#include "plan16.h"
/* -------------------------------------------------------------------------*/

#elif defined(WIN32)
/* ---------------- Maps to windows 3.2 and 4.0 32-bit APIs ----------------*/
#include "ptypes32.h"
#include "pcrt32.h"
#include "pwin32.h"
#include "plan32.h"
/* -------------------------------------------------------------------------*/
#else
#error You must define either WIN32 or WIN16
#endif /* WIN32 or WIN16 */
#endif /* ndef _PORT1632_ */
