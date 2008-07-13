/*
**********************************************************************
*   Copyright (C) 2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/

#ifndef ULOCIMP_H
#define ULOCIMP_H

#include "unicode/uloc.h"

/**
 * Create an iterator over the specified keywords list
 * @param keywordList double-null terminated list. Will be copied.
 * @param keywordListSize size in bytes of keywordList
 * @param status err code
 * @return enumeration (owned by caller) of the keyword list.
 * @internal ICU 3.0
 */
U_CAPI UEnumeration* U_EXPORT2
uloc_openKeywordList(const char *keywordList, int32_t keywordListSize, UErrorCode* status);

#endif
