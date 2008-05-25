/*
**********************************************************************
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
*
*  ucnv_imp.h:
*  Contains all internal and external data structure definitions
* Created & Maitained by Bertrand A. Damiba
*
*
*
* ATTENTION:
* ---------
* Although the data structures in this file are open and stack allocatable
* we reserve the right to hide them in further releases.
*/

#ifndef UCNV_IMP_H
#define UCNV_IMP_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/uloc.h"
#include "ucnv_bld.h"

/* figures out if we need to go to file to read in the data tables.
 * @param converterName The name of the converter
 * @param err The error code
 * @return the newly created converter
 */
UConverter *ucnv_createConverter (UConverter *myUConverter, const char *converterName, UErrorCode * err);

/*
 * Open a purely algorithmic converter, specified by a type constant.
 * @param myUConverter  NULL, or pre-allocated UConverter structure to avoid
 *                      a memory allocation
 * @param type          requested converter type
 * @param locale        locale parameter, or ""
 * @param options       converter options bit set (default 0)
 * @param err           ICU error code, not tested for U_FAILURE on input
 *                      because this is an internal function
 * @internal
 */
U_CFUNC UConverter *
ucnv_createAlgorithmicConverter(UConverter *myUConverter,
                                UConverterType type,
                                const char *locale, uint32_t options,
                                UErrorCode *err);

/* Creates a converter from shared data 
 */
UConverter*
ucnv_createConverterFromSharedData(UConverter *myUConverter, UConverterSharedData *mySharedConverterData, const char *realName, const char *locale, uint32_t options, UErrorCode *err);

UConverter* ucnv_createConverterFromPackage(const char *packageName, const char *converterName,  
                                            UErrorCode *err);

typedef struct {
    char cnvName[UCNV_MAX_CONVERTER_NAME_LENGTH], locale[ULOC_FULLNAME_CAPACITY];
    const char *realName;
    uint32_t options;
} UConverterLookupData;

/**
 * Load a converter but do not create a UConverter object.
 * Simply return the UConverterSharedData.
 * Performs alias lookup etc.
 * @internal
 */
UConverterSharedData *
ucnv_loadSharedData(const char *converterName, UConverterLookupData *lookup, UErrorCode * err);

/**
 * This may unload the shared data in a thread safe manner.
 * This will only unload the data if no other converters are sharing it.
 */
void
ucnv_unloadSharedDataIfReady(UConverterSharedData *sharedData);

/**
 * This is a thread safe way to increment the reference count.
 */
void
ucnv_incrementRefCount(UConverterSharedData *sharedData);

#define UCNV_TO_U_DEFAULT_CALLBACK ((UConverterToUCallback) UCNV_TO_U_CALLBACK_SUBSTITUTE)
#define UCNV_FROM_U_DEFAULT_CALLBACK ((UConverterFromUCallback) UCNV_FROM_U_CALLBACK_SUBSTITUTE)

#endif

#endif /* _UCNV_IMP */
