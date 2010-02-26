/*
*******************************************************************************
*
*   Copyright (C) 2001-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module reads a tailoring rule string and produces a list of 
* tokens that will be turned into collation elements
* 
*/

#ifndef UCOL_TOKENS_H
#define UCOL_TOKENS_H

#include "unicode/utypes.h"
#include "unicode/uset.h"

#if !UCONFIG_NO_COLLATION

#include "ucol_imp.h"
#include "uhash.h"
#include "unicode/parseerr.h"

#define UCOL_TOK_UNSET 0xFFFFFFFF
#define UCOL_TOK_RESET 0xDEADBEEF

#define UCOL_TOK_POLARITY_NEGATIVE 0
#define UCOL_TOK_POLARITY_POSITIVE 1

#define UCOL_TOK_TOP 0x04
#define UCOL_TOK_VARIABLE_TOP 0x08
#define UCOL_TOK_BEFORE 0x03
#define UCOL_TOK_SUCCESS 0x10

/* this is space for the extra strings that need to be unquoted */
/* during the parsing of the rules */
#define UCOL_TOK_EXTRA_RULE_SPACE_SIZE 4096
typedef struct UColToken UColToken;

typedef struct  {
  UColToken* first;
  UColToken* last;
  UColToken* reset;
  UBool indirect;
  uint32_t baseCE;
  uint32_t baseContCE;
  uint32_t nextCE;
  uint32_t nextContCE;
  uint32_t previousCE;
  uint32_t previousContCE;
  int32_t pos[UCOL_STRENGTH_LIMIT];
  uint32_t gapsLo[3*UCOL_CE_STRENGTH_LIMIT];
  uint32_t gapsHi[3*UCOL_CE_STRENGTH_LIMIT];
  uint32_t numStr[UCOL_CE_STRENGTH_LIMIT];
  UColToken* fStrToken[UCOL_CE_STRENGTH_LIMIT];
  UColToken* lStrToken[UCOL_CE_STRENGTH_LIMIT];
} UColTokListHeader;

struct UColToken {
  UChar debugSource;
  UChar debugExpansion;
  UChar debugPrefix;
  uint32_t CEs[128];
  uint32_t noOfCEs;
  uint32_t expCEs[128];
  uint32_t noOfExpCEs;
  uint32_t source;
  uint32_t expansion;
  uint32_t prefix;
  uint32_t strength;
  uint32_t toInsert;
  uint32_t polarity; /* 1 for <, <<, <<<, , ; and -1 for >, >>, >>> */
  UColTokListHeader *listHeader;
  UColToken* previous;
  UColToken* next;
  UChar *rulesToParse;
  uint16_t flags;
};

/* 
 * This is a token that has been parsed
 * but not yet processed. Used to reduce
 * the number of arguments in the parser
 */
typedef struct {
  uint32_t strength;
  uint32_t charsOffset;
  uint32_t charsLen;
  uint32_t extensionOffset;
  uint32_t extensionLen;
  uint32_t prefixOffset;
  uint32_t prefixLen;
  uint16_t flags;
  uint16_t indirectIndex;
} UColParsedToken;


typedef struct {
  UColParsedToken parsedToken;
  UChar *source;
  UChar *end;
  const UChar *current;
  UChar *sourceCurrent;
  UChar *extraCurrent;
  UChar *extraEnd;
  const InverseUCATableHeader *invUCA;
  const UCollator *UCA;
  UHashtable *tailored;
  UColOptionSet *opts;
  uint32_t resultLen;
  uint32_t listCapacity;
  UColTokListHeader *lh;
  UColToken *varTop;
  USet *copySet;
  USet *removeSet;
  UBool buildCCTabFlag;  /* Tailoring rule requirs building combining class table. */
} UColTokenParser;

typedef struct {
  const UChar *subName;
  int32_t subLen;
  UColAttributeValue attrVal;
} ucolTokSuboption;

typedef struct {
   const UChar *optionName;
   int32_t optionLen;
   const ucolTokSuboption *subopts;
   int32_t subSize;
   UColAttribute attr;
} ucolTokOption;

#define ucol_tok_isSpecialChar(ch)              \
    (((((ch) <= 0x002F) && ((ch) >= 0x0020)) || \
      (((ch) <= 0x003F) && ((ch) >= 0x003A)) || \
      (((ch) <= 0x0060) && ((ch) >= 0x005B)) || \
      (((ch) <= 0x007E) && ((ch) >= 0x007D)) || \
      (ch) == 0x007B))


U_CFUNC 
uint32_t ucol_tok_assembleTokenList(UColTokenParser *src,
                                    UParseError *parseError, 
                                    UErrorCode *status);

U_CFUNC
void ucol_tok_initTokenList(UColTokenParser *src, const UChar *rules, const uint32_t rulesLength, const UCollator *UCA, UErrorCode *status);

U_CFUNC void ucol_tok_closeTokenList(UColTokenParser *src);

U_CAPI const UChar* U_EXPORT2 ucol_tok_parseNextToken(UColTokenParser *src, 
                        UBool startOfRules,
                        UParseError *parseError,
                        UErrorCode *status);

U_CAPI const UChar * U_EXPORT2
ucol_tok_getNextArgument(const UChar *start, const UChar *end, 
                               UColAttribute *attrib, UColAttributeValue *value, 
                               UErrorCode *status);
U_CAPI int32_t U_EXPORT2 ucol_inv_getNextCE(const UColTokenParser *src,
                                            uint32_t CE, uint32_t contCE,
                                            uint32_t *nextCE, uint32_t *nextContCE,
                                            uint32_t strength);
U_CAPI int32_t U_EXPORT2 ucol_inv_getPrevCE(const UColTokenParser *src,
                                            uint32_t CE, uint32_t contCE,
                                            uint32_t *prevCE, uint32_t *prevContCE,
                                            uint32_t strength);

U_CAPI uint32_t U_EXPORT2 ucol_getCEStrengthDifference(uint32_t CE, uint32_t contCE, 
                                            uint32_t prevCE, uint32_t prevContCE);


#endif /* #if !UCONFIG_NO_COLLATION */

#endif
