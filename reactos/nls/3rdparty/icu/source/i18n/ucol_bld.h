/*
*******************************************************************************
*
*   Copyright (C) 2001-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module builds a collator based on the rule set.
* 
*/

#ifndef UCOL_BLD_H
#define UCOL_BLD_H

#ifdef UCOL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION
#if !UCONFIG_NO_COLLATION_BUILDER

#include "ucol_imp.h"
#include "ucol_tok.h"
#include "ucol_elm.h"
#include "ucol_wgt.h"

#include "uhash.h"
#include "cpputils.h"

#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/normlzr.h"

U_CFUNC
UCATableHeader *ucol_assembleTailoringTable(UColTokenParser *src, UErrorCode *status);

typedef struct {
  WeightRange ranges[7];
  int32_t noOfRanges;
  uint32_t byteSize; uint32_t start; uint32_t limit;
  int32_t maxCount;
  int32_t count;
  uint32_t current;
  uint32_t fLow; /*forbidden Low */
  uint32_t fHigh; /*forbidden High */
} ucolCEGenerator;

#endif /* #if !UCONFIG_NO_COLLATION_BUILDER */
#endif /* #if !UCONFIG_NO_COLLATION */

#endif
