/**************************************************************************
*
*   Copyright (C) 2000-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
***************************************************************************
*   file name:  makefile.h
*   encoding:   ANSI X3.4 (1968)
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000may17
*   created by: Steven \u24C7 Loomis
*
*  definition for code to create a makefile.
*  implementation is OS dependent (i.e. gmake.c, nmake.c, .. )
*/

#ifndef _MAKEFILE
#define _MAKEFILE

/* headers */
#include "unicode/utypes.h"
#include "pkgtypes.h"


/* Write any setup/initialization stuff */
void
pkg_mak_writeHeader(FileStream *f, const UPKGOptions *o);

/* Write a stanza in the makefile, with specified   "target: parents...  \n\n\tcommands" [etc] */
void
pkg_mak_writeStanza(FileStream *f, const UPKGOptions *o, 
                    const char *target,
                    CharList* parents,
                    CharList* commands);

/* write any cleanup/post stuff */
void
pkg_mak_writeFooter(FileStream *f, const UPKGOptions *o);



#ifdef U_MAKE_IS_NMAKE
extern void pkg_mode_windows(UPKGOptions *o, FileStream *makefile, UErrorCode *status);
#else /*#ifdef U_MAKE_IS_NMAKE*/
/**
 * Write stanzas for generating .o (and .c) files for each data file in 'o->filePaths'.
 * @param o Package options struct
 * @param makefile Current makefile being written
 * @param objects On output, list of object files
 * @param objSuffix Suffix of object files including dot, typically OBJ_SUFFIX or ".o" or ".obj"
 */
extern void
pkg_mak_writeObjRules(UPKGOptions *o,  FileStream *makefile, CharList **objects, const char* objSuffix);
#ifdef UDATA_SO_SUFFIX
extern void pkg_mode_dll(UPKGOptions* o, FileStream *stream, UErrorCode *status);
extern void pkg_mode_static(UPKGOptions* o, FileStream *stream, UErrorCode *status);
#endif /*#ifdef UDATA_SO_SUFFIX*/
extern void pkg_mode_common(UPKGOptions* o, FileStream *stream, UErrorCode *status);
#endif /*#ifdef U_MAKE_IS_NMAKE*/

extern void pkg_mode_files(UPKGOptions* o, FileStream *stream, UErrorCode *status);


extern void
pkg_mak_writeAssemblyHeader(FileStream *f, const UPKGOptions *o);
extern void
pkg_mak_writeAssemblyFooter(FileStream *f, const UPKGOptions *o);

#endif
