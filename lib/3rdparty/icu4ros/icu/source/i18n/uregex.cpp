/*
*******************************************************************************
*   Copyright (C) 2004-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  regex.cpp
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uregex.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uobject.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"

U_NAMESPACE_USE

struct URegularExpression: public UMemory {
public:
    URegularExpression();
    ~URegularExpression();
    int32_t           fMagic;
    RegexPattern     *fPat;
    int32_t          *fPatRefCount;
    UChar            *fPatString;
    int32_t           fPatStringLen;
    RegexMatcher     *fMatcher;
    const UChar      *fText;         // Text from setText()
    int32_t           fTextLength;   // Length provided by user with setText(), which
                                     //  may be -1.

    UnicodeString     fTextString;   // The setText(text) is wrapped into a UnicodeString.
                                     // TODO: regexp engine should not depend on UnicodeString.
};

static const int32_t REXP_MAGIC = 0x72657870; // "rexp" in ASCII

URegularExpression::URegularExpression() {
    fMagic        = REXP_MAGIC;
    fPat          = NULL;
    fPatRefCount  = NULL;
    fPatString    = NULL;
    fPatStringLen = 0;
    fMatcher      = NULL;
    fText         = NULL;
    fTextLength   = 0;
}

URegularExpression::~URegularExpression() {
    delete fMatcher;
    fMatcher = NULL;
    if (fPatRefCount!=NULL && umtx_atomic_dec(fPatRefCount)==0) {
        delete fPat;
        uprv_free(fPatString);
        uprv_free(fPatRefCount);
    }
    fMagic = 0;
}

//----------------------------------------------------------------------------------------
//
//   validateRE    Do boilerplate style checks on API function parameters.
//                 Return TRUE if they look OK.
//----------------------------------------------------------------------------------------
static UBool validateRE(const URegularExpression *re, UErrorCode *status, UBool requiresText = TRUE) {
    if (U_FAILURE(*status)) {
        return FALSE;
    }
    if (re == NULL || re->fMagic != REXP_MAGIC) {
        // U_ASSERT(FALSE);
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    if (requiresText && re->fText == NULL) {
        *status = U_REGEX_INVALID_STATE;
        return FALSE;
    }
    return TRUE;
}

//----------------------------------------------------------------------------------------
//
//    uregex_open
//
//----------------------------------------------------------------------------------------
U_CAPI URegularExpression *  U_EXPORT2
uregex_open( const  UChar          *pattern,
                    int32_t         patternLength,
                    uint32_t        flags,
                    UParseError    *pe,
                    UErrorCode     *status) {

    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (pattern == NULL || patternLength < -1 || patternLength == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    int32_t actualPatLen = patternLength;
    if (actualPatLen == -1) {
        actualPatLen = u_strlen(pattern);
    }

    URegularExpression *re     = new URegularExpression;
    int32_t            *refC   = (int32_t *)uprv_malloc(sizeof(int32_t));
    UChar              *patBuf = (UChar *)uprv_malloc(sizeof(UChar)*(actualPatLen+1));
    if (re == NULL || refC == NULL || patBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        delete re;
        uprv_free(refC);
        uprv_free(patBuf);
        return NULL;
    }
    re->fPatRefCount = refC;
    *re->fPatRefCount = 1;

    //
    // Make a copy of the pattern string, so we can return it later if asked.
    //    For compiling the pattern, we will use a read-only-aliased UnicodeString
    //    of this local copy, to avoid making even more copies.
    //
    re->fPatString    = patBuf;
    re->fPatStringLen = patternLength;
    u_memcpy(patBuf, pattern, actualPatLen);
    patBuf[actualPatLen] = 0;
    UnicodeString  patString(patternLength==-1, patBuf, patternLength);                 

    //
    // Compile the pattern
    //
    if (pe != NULL) {
        re->fPat = RegexPattern::compile(patString, flags, *pe, *status);
    } else {
        re->fPat = RegexPattern::compile(patString, flags, *status);
    }
    if (U_FAILURE(*status)) {
        goto ErrorExit;
    }

    //
    // Create the matcher object
    //
    re->fMatcher = re->fPat->matcher(*status);
    if (U_SUCCESS(*status)) {
        return re;
    }

ErrorExit:
    delete re;
    return NULL;

}

//----------------------------------------------------------------------------------------
//
//    uregex_close
//
//----------------------------------------------------------------------------------------
U_CAPI void  U_EXPORT2
uregex_close(URegularExpression  *re) {
    UErrorCode  status = U_ZERO_ERROR;
    if (validateRE(re, &status, FALSE) == FALSE) {
        return;
    }
    delete re;
}


//----------------------------------------------------------------------------------------
//
//    uregex_clone
//
//----------------------------------------------------------------------------------------
U_CAPI URegularExpression * U_EXPORT2 
uregex_clone(const URegularExpression *source, UErrorCode *status)  {
    if (validateRE(source, status, FALSE) == FALSE) {
        return NULL;
    }

    URegularExpression *clone = new URegularExpression;
    if (clone == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    clone->fMatcher = source->fPat->matcher(*status);
    if (U_FAILURE(*status)) {
        delete clone;
        return NULL;
    }
    if (clone == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    clone->fPat          = source->fPat;
    clone->fPatRefCount  = source->fPatRefCount; 
    clone->fPatString    = source->fPatString;
    clone->fPatStringLen = source->fPatStringLen;
    umtx_atomic_inc(source->fPatRefCount);
    // Note:  fText is not cloned.

    return clone;
}




//------------------------------------------------------------------------------
//
//    uregex_pattern
//
//------------------------------------------------------------------------------
U_CAPI const UChar * U_EXPORT2 
uregex_pattern(const  URegularExpression *regexp,
               int32_t            *patLength,
               UErrorCode         *status)  {
    
    if (validateRE(regexp, status, FALSE) == FALSE) {
        return NULL;
    }
    if (patLength != NULL) {
        *patLength = regexp->fPatStringLen;
    }
    return regexp->fPatString;
}


//------------------------------------------------------------------------------
//
//    uregex_flags
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_flags(const URegularExpression *regexp, UErrorCode *status)  {
    if (validateRE(regexp, status, FALSE) == FALSE) {
        return 0;
    }
    int32_t flags = regexp->fPat->flags();
    return flags;
}


//------------------------------------------------------------------------------
//
//    uregex_setText
//
//------------------------------------------------------------------------------
U_CAPI void U_EXPORT2 
uregex_setText(URegularExpression *regexp,
               const UChar        *text,
               int32_t             textLength,
               UErrorCode         *status)  {
    if (validateRE(regexp, status, FALSE) == FALSE) {
        return;
    }
    if (text == NULL || textLength < -1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    regexp->fText       = text;
    regexp->fTextLength = textLength;
    UBool isTerminated  = (textLength == -1);

    regexp->fTextString.setTo(isTerminated, text, textLength);
    regexp->fMatcher->reset(regexp->fTextString);
}



//------------------------------------------------------------------------------
//
//    uregex_getText
//
//------------------------------------------------------------------------------
U_CAPI const UChar * U_EXPORT2 
uregex_getText(URegularExpression *regexp,
               int32_t            *textLength,
               UErrorCode         *status)  {
    if (validateRE(regexp, status, FALSE) == FALSE) {
        return NULL;
    }
    if (textLength != NULL) {
        *textLength = regexp->fTextLength;
    }
    return regexp->fText;
}


//------------------------------------------------------------------------------
//
//    uregex_matches
//
//------------------------------------------------------------------------------
U_CAPI UBool U_EXPORT2 
uregex_matches(URegularExpression *regexp,
                int32_t            startIndex,
                UErrorCode        *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return FALSE;
    }
    UBool result = regexp->fMatcher->matches(startIndex, *status);
    return result;
}



//------------------------------------------------------------------------------
//
//    uregex_lookingAt
//
//------------------------------------------------------------------------------
U_CAPI UBool U_EXPORT2 
uregex_lookingAt(URegularExpression *regexp,
                 int32_t             startIndex,
                 UErrorCode         *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return FALSE;
    }
    UBool result = regexp->fMatcher->lookingAt(startIndex, *status);
    return result;
}



//------------------------------------------------------------------------------
//
//    uregex_find
//
//------------------------------------------------------------------------------
U_CAPI UBool U_EXPORT2 
uregex_find(URegularExpression *regexp,
            int32_t             startIndex, 
            UErrorCode         *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return FALSE;
    }
    UBool result = regexp->fMatcher->find(startIndex, *status);
    return result;
}

//------------------------------------------------------------------------------
//
//    uregex_findNext
//
//------------------------------------------------------------------------------
U_CAPI UBool U_EXPORT2 
uregex_findNext(URegularExpression *regexp,
                UErrorCode         *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return FALSE;
    }
    UBool result = regexp->fMatcher->find();
    return result;
}

//------------------------------------------------------------------------------
//
//    uregex_groupCount
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_groupCount(URegularExpression *regexp,
                  UErrorCode         *status)  {
    if (validateRE(regexp, status, FALSE) == FALSE) {
        return 0;
    }
    int32_t  result = regexp->fMatcher->groupCount();
    return result;
}


//------------------------------------------------------------------------------
//
//    uregex_group
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_group(URegularExpression *regexp,
             int32_t             groupNum,
             UChar              *dest,
             int32_t             destCapacity,
             UErrorCode          *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (destCapacity < 0 || (destCapacity > 0 && dest == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    // Pick up the range of characters from the matcher
    //
    int32_t  startIx = regexp->fMatcher->start(groupNum, *status);
    int32_t  endIx   = regexp->fMatcher->end  (groupNum, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }

    //
    // Trim length based on buffer capacity
    // 
    int32_t fullLength = endIx - startIx;
    int32_t copyLength = fullLength;
    if (copyLength < destCapacity) {
        dest[copyLength] = 0;
    } else  if (copyLength == destCapacity) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        copyLength = destCapacity;
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    //
    // Copy capture group to user's buffer
    //
    if (copyLength > 0) {
        u_memcpy(dest, &regexp->fText[startIx], copyLength);
    }
    return fullLength;
}


//------------------------------------------------------------------------------
//
//    uregex_start
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_start(URegularExpression *regexp,
             int32_t             groupNum,
             UErrorCode          *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    int32_t result = regexp->fMatcher->start(groupNum, *status);
    return result;
}


//------------------------------------------------------------------------------
//
//    uregex_end
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_end(URegularExpression   *regexp,
           int32_t               groupNum,
           UErrorCode           *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    int32_t result = regexp->fMatcher->end(groupNum, *status);
    return result;
}

//------------------------------------------------------------------------------
//
//    uregex_reset
//
//------------------------------------------------------------------------------
U_CAPI void U_EXPORT2 
uregex_reset(URegularExpression    *regexp,
             int32_t               index,
             UErrorCode            *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return;
    }
    regexp->fMatcher->reset(index, *status);
}


//------------------------------------------------------------------------------
//
//    uregex_replaceAll
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_replaceAll(URegularExpression    *regexp,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                 *destBuf,
                  int32_t                destCapacity,
                  UErrorCode            *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        destBuf == NULL && destCapacity > 0 ||
        destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t   len = 0;
    uregex_reset(regexp, 0, status);
    while (uregex_findNext(regexp, status)) {
        len += uregex_appendReplacement(regexp, replacementText, replacementLength, 
                                        &destBuf, &destCapacity, status);
    }
    len += uregex_appendTail(regexp, &destBuf, &destCapacity, status);

    return len;
}


//------------------------------------------------------------------------------
//
//    uregex_replaceFirst
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_replaceFirst(URegularExpression  *regexp,
                    const UChar         *replacementText,
                    int32_t              replacementLength,
                    UChar               *destBuf,
                    int32_t              destCapacity,
                    UErrorCode          *status)  {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        destBuf == NULL && destCapacity > 0 ||
        destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t   len = 0;
    UBool     findSucceeded;
    uregex_reset(regexp, 0, status);
    findSucceeded = uregex_find(regexp, 0, status);
    if (findSucceeded) {
        len = uregex_appendReplacement(regexp, replacementText, replacementLength, 
                                       &destBuf, &destCapacity, status);
    }
    len += uregex_appendTail(regexp, &destBuf, &destCapacity, status);

    return len;
}


//------------------------------------------------------------------------------
//
//    uregex_appendReplacement
//
//------------------------------------------------------------------------------


//
//  Dummy class, because these functions need to be friends of class RegexMatcher,
//               and stand-alone C functions don't work as friends
//
U_NAMESPACE_BEGIN
class RegexCImpl {
 public:
   inline static  int32_t appendReplacement(URegularExpression    *regexp,
                      const UChar           *replacementText,
                      int32_t                replacementLength,
                      UChar                **destBuf,
                      int32_t               *destCapacity,
                      UErrorCode            *status);

   inline static int32_t appendTail(URegularExpression    *regexp,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status);
};
U_NAMESPACE_END


//
//  Call-back function for u_unescapeAt(), used when we encounter
//    \uxxxx or \Uxxxxxxxxx escapes in the replacement text.
//
U_CDECL_BEGIN
static UChar U_CALLCONV
unescape_charAt(int32_t offset, void *context) {
    UChar c16 = ((UChar *)context)[offset];
    return c16;
}
U_CDECL_END


static const UChar BACKSLASH  = 0x5c;
static const UChar DOLLARSIGN = 0x24;

//
//  Move a character to an output buffer, with bounds checking on the index.
//      Index advances even if capacity is exceeded, for preflight size computations.
//      This little sequence is used a LOT.
//
static inline void appendToBuf(UChar c, int32_t *idx, UChar *buf, int32_t bufCapacity) {
    if (*idx < bufCapacity) {
        buf[*idx] = c;
    }
    (*idx)++;
}


//
//  appendReplacement, the actual implementation.
//
int32_t RegexCImpl::appendReplacement(URegularExpression    *regexp,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status)  {

    // If we come in with a buffer overflow error, don't suppress the operation.
    //  A series of appendReplacements, appendTail need to correctly preflight
    //  the buffer size when an overflow happens somewhere in the middle.
    UBool pendingBufferOverflow = FALSE;
    if (*status == U_BUFFER_OVERFLOW_ERROR && destCapacity == 0) {
        pendingBufferOverflow = TRUE;
        *status = U_ZERO_ERROR;
    }

    //
    // Validate all paramters
    //
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        destCapacity == NULL || destBuf == NULL || 
        *destBuf == NULL && *destCapacity > 0 ||
        *destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    RegexMatcher *m = regexp->fMatcher;
    if (m->fMatch == FALSE) {
        *status = U_REGEX_INVALID_STATE;
        return 0;
    }

    UChar    *dest             = *destBuf;
    int32_t   capacity         = *destCapacity;
    int32_t   destIdx          =  0;
    int32_t   i;
    
    // If it wasn't supplied by the caller,  get the length of the replacement text.
    //   TODO:  slightly smarter logic in the copy loop could watch for the NUL on
    //          the fly and avoid this step.
    if (replacementLength == -1) {
        replacementLength = u_strlen(replacementText);
    }

    // Copy input string from the end of previous match to start of current match
    for (i=m->fLastMatchEnd; i<m->fMatchStart; i++) {
        appendToBuf(regexp->fText[i], &destIdx, dest, capacity);
    }

    

    // scan the replacement text, looking for substitutions ($n) and \escapes.
    int32_t  replIdx = 0;
    while (replIdx < replacementLength) {
        UChar  c = replacementText[replIdx];
        replIdx++;
        if (c != DOLLARSIGN && c != BACKSLASH) {
            // Common case, no substitution, no escaping, 
            //  just copy the char to the dest buf.
            appendToBuf(c, &destIdx, dest, capacity);
            continue;
        }

        if (c == BACKSLASH) {
            // Backslash Escape.  Copy the following char out without further checks.
            //                    Note:  Surrogate pairs don't need any special handling
            //                           The second half wont be a '$' or a '\', and
            //                           will move to the dest normally on the next
            //                           loop iteration.
            if (replIdx >= replacementLength) {
                break;
            }
            c = replacementText[replIdx];

            if (c==0x55/*U*/ || c==0x75/*u*/) {
                // We have a \udddd or \Udddddddd escape sequence.
                UChar32 escapedChar = 
                    u_unescapeAt(unescape_charAt,
                       &replIdx,                   // Index is updated by unescapeAt 
                       replacementLength,          // Length of replacement text
                       (void *)replacementText);

                if (escapedChar != (UChar32)0xFFFFFFFF) {
                    if (escapedChar <= 0xffff) {
                        appendToBuf((UChar)escapedChar, &destIdx, dest, capacity);
                    } else {
                        appendToBuf(U16_LEAD(escapedChar), &destIdx, dest, capacity);
                        appendToBuf(U16_TRAIL(escapedChar), &destIdx, dest, capacity);
                    }
                    continue;
                }
                // Note:  if the \u escape was invalid, just fall through and
                //        treat it as a plain \<anything> escape.
            }

            // Plain backslash escape.  Just put out the escaped character.
            appendToBuf(c, &destIdx, dest, capacity);

            replIdx++;
            continue;
        }



        // We've got a $.  Pick up a capture group number if one follows.
        // Consume at most the number of digits necessary for the largest capture
        // number that is valid for this pattern.

        int32_t numDigits = 0;
        int32_t groupNum  = 0;
        UChar32 digitC;
        for (;;) {
            if (replIdx >= replacementLength) {
                break;
            }
            U16_GET(replacementText, 0, replIdx, replacementLength, digitC);
            if (u_isdigit(digitC) == FALSE) {
                break;
            }

            U16_FWD_1(replacementText, replIdx, replacementLength);
            groupNum=groupNum*10 + u_charDigitValue(digitC);
            numDigits++;
            if (numDigits >= m->fPattern->fMaxCaptureDigits) {
                break;
            }
        }


        if (numDigits == 0) {
            // The $ didn't introduce a group number at all.
            // Treat it as just part of the substitution text.
            appendToBuf(DOLLARSIGN, &destIdx, dest, capacity);
            continue;
        }

        // Finally, append the capture group data to the destination.
        int32_t  capacityRemaining = capacity - destIdx;
        if (capacityRemaining < 0) {
            capacityRemaining = 0;
        }
        destIdx += uregex_group(regexp, groupNum, dest+destIdx, capacityRemaining, status);
        if (*status == U_BUFFER_OVERFLOW_ERROR) {
            // Ignore buffer overflow when extracting the group.  We need to
            //   continue on to get full size of the untruncated result.  We will
            //   raise our own buffer overflow error at the end.
            *status = U_ZERO_ERROR;
        }

        if (U_FAILURE(*status)) {
            // Can fail if group number is out of range.
            break;
        }

    }

    //
    //  Nul Terminate the dest buffer if possible.
    //  Set the appropriate buffer overflow or not terminated error, if needed.
    //
    if (destIdx < capacity) {
        dest[destIdx] = 0;
    } else if (destIdx == *destCapacity) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    
    //
    // Return an updated dest buffer and capacity to the caller.
    //
    if (destIdx > 0 &&  *destCapacity > 0) {
        if (destIdx < capacity) {
            *destBuf      += destIdx;
            *destCapacity -= destIdx;
        } else {
            *destBuf      += capacity;
            *destCapacity =  0;
        }
    }

    // If we came in with a buffer overflow, make sure we go out with one also.
    //   (A zero length match right at the end of the previous match could
    //    make this function succeed even though a previous call had overflowed the buf)
    if (pendingBufferOverflow && U_SUCCESS(*status)) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    return destIdx;
}

//
//   appendReplacement   the acutal API function,
//
U_CAPI int32_t U_EXPORT2 
uregex_appendReplacement(URegularExpression    *regexp,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status) {
    return RegexCImpl::appendReplacement(
        regexp, replacementText, replacementLength,destBuf, destCapacity, status);
}


//------------------------------------------------------------------------------
//
//    uregex_appendTail
//
//------------------------------------------------------------------------------
int32_t RegexCImpl::appendTail(URegularExpression    *regexp,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status)  {

    // If we come in with a buffer overflow error, don't suppress the operation.
    //  A series of appendReplacements, appendTail need to correctly preflight
    //  the buffer size when an overflow happens somewhere in the middle.
    UBool pendingBufferOverflow = FALSE;
    if (*status == U_BUFFER_OVERFLOW_ERROR && *destCapacity == 0) {
        pendingBufferOverflow = TRUE;
        *status = U_ZERO_ERROR;
    }

    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (destCapacity == NULL || destBuf == NULL || 
        *destBuf == NULL && *destCapacity > 0 ||
        *destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    RegexMatcher *m = regexp->fMatcher;

    int32_t  srcIdx;
    if (m->fMatch) {
        // The most recent call to find() succeeded.  
        srcIdx = m->fMatchEnd;
    } else {
        // The last call to find() on this matcher failed().
        //   Look back to the end of the last find() that succeeded for src index.
        srcIdx = m->fLastMatchEnd;
        if (srcIdx == -1)  {
            // There has been no successful match with this matcher.
            //   We want to copy the whole string.
            srcIdx = 0;
        }
    }

    int32_t  destIdx     = 0;
    int32_t  destCap     = *destCapacity;
    UChar    *dest       = *destBuf;

    for (;;) {
        if (srcIdx == regexp->fTextLength) {
            break;
        }
        UChar c = regexp->fText[srcIdx];
        if (c == 0 && regexp->fTextLength == -1) {
            break;
        }
        if (destIdx < destCap) {
            dest[destIdx] = c;
        } else {
            // We've overflowed the dest buffer.
            //  If the total input string length is known, we can
            //    compute the total buffer size needed without scanning through the string.
            if (regexp->fTextLength > 0) {
                destIdx += (regexp->fTextLength - srcIdx);
                break;
            }
        }
        srcIdx++;
        destIdx++;
    }

    //
    //  NUL terminate the output string, if possible, otherwise issue the
    //   appropriate error or warning.
    //
    if (destIdx < destCap) {
        dest[destIdx] = 0;
    } else  if (destIdx == destCap) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    //
    // Update the user's buffer ptr and capacity vars to reflect the
    //   amount used.
    //
    if (destIdx < destCap) {
        *destBuf      += destIdx;
        *destCapacity -= destIdx;
    } else {
        *destBuf      += destCap;
        *destCapacity  = 0;
    }

    if (pendingBufferOverflow && U_SUCCESS(*status)) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    return destIdx;
}


U_CAPI int32_t U_EXPORT2 
uregex_appendTail(URegularExpression    *regexp,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status)  {
    return RegexCImpl::appendTail(regexp, destBuf, destCapacity, status);
}


//------------------------------------------------------------------------------
//
//    copyString     Internal utility to copy a string to an output buffer,
//                   while managing buffer overflow and preflight size
//                   computation.  NUL termination is added to destination,
//                   and the NUL is counted in the output size.
//
//------------------------------------------------------------------------------
static void copyString(UChar        *destBuffer,    //  Destination buffer.
                       int32_t       destCapacity,  //  Total capacity of dest buffer
                       int32_t      *destIndex,     //  Index into dest buffer.  Updated on return.
                                                    //    Update not clipped to destCapacity.
                       const UChar  *srcPtr,        //  Pointer to source string
                       int32_t       srcLen)        //  Source string len.
{
    int32_t  si;
    int32_t  di = *destIndex;
    UChar    c;

    for (si=0; si<srcLen;  si++) {
        c = srcPtr[si];
        if (di < destCapacity) {
            destBuffer[di] = c;
            di++;
        } else {
            di += srcLen - si;
            break;
        }
    }
    if (di<destCapacity) {
        destBuffer[di] = 0;
    }
    di++;
    *destIndex = di;
}


//------------------------------------------------------------------------------
//
//    uregex_split
//
//------------------------------------------------------------------------------
U_CAPI int32_t U_EXPORT2 
uregex_split(   URegularExpression      *regexp,
                  UChar                 *destBuf,
                  int32_t                destCapacity,
                  int32_t               *requiredCapacity,
                  UChar                 *destFields[],
                  int32_t                destFieldsCapacity,
                  UErrorCode            *status) {
    if (validateRE(regexp, status) == FALSE) {
        return 0;
    }
    if (destBuf == NULL && destCapacity > 0 ||
        destCapacity < 0 ||
        destFields == NULL ||
        destFieldsCapacity < 1 ) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    // Reset for the input text
    //
    regexp->fMatcher->reset();
    int32_t   inputLen = regexp->fTextString.length();
    int32_t   nextOutputStringStart = 0;
    if (inputLen == 0) {
        return 0;
    }


    //
    // Loop through the input text, searching for the delimiter pattern
    //
    int32_t   i;             // Index of the field being processed.
    int32_t   destIdx = 0;   // Next available position in destBuf;
    int32_t   numCaptureGroups = regexp->fMatcher->groupCount();
    for (i=0; ; i++) {
        if (i>=destFieldsCapacity-1) {
            // There are one or zero output string left.
            // Fill the last output string with whatever is left from the input, then exit the loop.
            //  ( i will be == destFieldsCapacity if we filled the output array while processing
            //    capture groups of the delimiter expression, in which case we will discard the
            //    last capture group saved in favor of the unprocessed remainder of the
            //    input string.)
            int32_t remainingLength = inputLen-nextOutputStringStart;
            if (remainingLength > 0) {
            }
            if (i >= destFieldsCapacity) {
                // No fields are left.  Recycle the last one for holding the trailing part of
                //   the input string.
                i = destFieldsCapacity-1;
                destIdx = (int32_t)(destFields[i] - destFields[0]);
            }
            
            destFields[i] = &destBuf[destIdx];
            copyString(destBuf, destCapacity, &destIdx, 
                &regexp->fText[nextOutputStringStart], remainingLength);
            break;
        }
        
        if (regexp->fMatcher->find()) {
            // We found another delimiter.  Move everything from where we started looking
            //  up until the start of the delimiter into the next output string.
            int32_t fieldLen = regexp->fMatcher->start(*status) - nextOutputStringStart;
            destFields[i] = &destBuf[destIdx];
            copyString(destBuf, destCapacity, &destIdx, 
                &regexp->fText[nextOutputStringStart], fieldLen);
            nextOutputStringStart =  regexp->fMatcher->end(*status);
            
            // If the delimiter pattern has capturing parentheses, the captured
            //  text goes out into the next n destination strings.
            int32_t groupNum;
            for (groupNum=1; groupNum<=numCaptureGroups; groupNum++) {
                // If we've run out of output string slots, bail out.
                if (i==destFieldsCapacity-1) {
                    break;
                }
                i++;
                
                // Set up to extract the capture group contents into the dest buffer.
                UErrorCode  tStatus = U_ZERO_ERROR;   // Want to ignore any buffer overflow
                                                      //  error while extracting this group.
                int32_t remainingCapacity = destCapacity - destIdx;
                if (remainingCapacity < 0) {
                    remainingCapacity = 0;
                }
                destFields[i] = &destBuf[destIdx];
                int32_t t = uregex_group(regexp, groupNum, destFields[i], remainingCapacity, &tStatus);
                destIdx += t + 1;    // Record the space used in the output string buffer.
                                     //  +1 for the NUL that terminates the string.
            }

            if (nextOutputStringStart == inputLen) {
                // The delimiter was at the end of the string.  We're done.
                break;
            }

        }
        else
        {
            // We ran off the end of the input while looking for the next delimiter.
            // All the remaining text goes into the current output string.
            destFields[i] = &destBuf[destIdx];
            copyString(destBuf, destCapacity, &destIdx, 
                         &regexp->fText[nextOutputStringStart], inputLen-nextOutputStringStart);
            break;
        }
    }

    // Zero out any unused portion of the destFields array
    int j;
    for (j=i+1; j<destFieldsCapacity; j++) {
        destFields[j] = NULL;
    }

    if (requiredCapacity != NULL) {
        *requiredCapacity = destIdx;
    }
    if (destIdx > destCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    return i+1;
}


#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS

