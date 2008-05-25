/*
*******************************************************************************
*
*   Copyright (C) 2004-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  genbidi.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004dec30
*   created by: Markus W. Scherer
*/

#ifndef __GENBIDI_H__
#define __GENBIDI_H__

#include "unicode/utypes.h"

U_CDECL_BEGIN

/* genbidi ------------------------------------------------------------------ */

/* global flags */
extern UBool beVerbose, haveCopyright;

/* properties vectors in genbidi.c */
extern uint32_t *pv;

/* prototypes */
U_CFUNC void
writeUCDFilename(char *basename, const char *filename, const char *suffix);

extern void
setUnicodeVersion(const char *v);

extern void
addMirror(UChar32 src, UChar32 mirror);

extern void
generateData(const char *dataDir, UBool csource);

U_CDECL_END

#endif
