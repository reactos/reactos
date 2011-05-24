/*
*******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  unistr_props.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:2
*
*   created on: 2004aug25
*   created by: Markus W. Scherer
*
*   Character property dependent functions moved here from unistr.cpp
*/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

UnicodeString& 
UnicodeString::trim()
{
  if(isBogus()) {
    return *this;
  }

  UChar32 c;
  int32_t i = fLength, length;

  // first cut off trailing white space
  for(;;) {
    length = i;
    if(i <= 0) {
      break;
    }
    UTF_PREV_CHAR(fArray, 0, i, c);
    if(!(c == 0x20 || u_isWhitespace(c))) {
      break;
    }
  }
  if(length < fLength) {
    fLength = length;
  }

  // find leading white space
  int32_t start;
  i = 0;
  for(;;) {
    start = i;
    if(i >= length) {
      break;
    }
    UTF_NEXT_CHAR(fArray, i, length, c);
    if(!(c == 0x20 || u_isWhitespace(c))) {
      break;
    }
  }

  // move string forward over leading white space
  if(start > 0) {
    doReplace(0, start, 0, 0, 0);
  }

  return *this;
}

U_NAMESPACE_END
