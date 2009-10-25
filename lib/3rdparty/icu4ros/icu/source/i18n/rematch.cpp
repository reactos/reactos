/*
**************************************************************************
*   Copyright (C) 2002-2007 International Business Machines Corporation  *
*   and others. All rights reserved.                                     *
**************************************************************************
*/
//
//  file:  rematch.cpp    
//
//         Contains the implementation of class RegexMatcher,
//         which is one of the main API classes for the ICU regular expression package.
//

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/rbbi.h"
#include "uassert.h"
#include "cmemory.h"
#include "uvector.h"
#include "uvectr32.h"
#include "regeximp.h"
#include "regexst.h"

// #include <malloc.h>        // Needed for heapcheck testing

U_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------
//
//   Constructor and Destructor
//
//-----------------------------------------------------------------------------
RegexMatcher::RegexMatcher(const RegexPattern *pat)  { 
    fPattern           = pat;
    fPatternOwned      = NULL;
    fInput             = NULL;
    fTraceDebug        = FALSE;
    fDeferredStatus    = U_ZERO_ERROR;
    fStack             = new UVector32(fDeferredStatus); 
    fData              = fSmallData;
    fWordBreakItr      = NULL;
    if (pat==NULL) {
        fDeferredStatus = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (pat->fDataSize > (int32_t)(sizeof(fSmallData)/sizeof(int32_t))) {
        fData = (int32_t *)uprv_malloc(pat->fDataSize * sizeof(int32_t)); 
    }
    if (fStack == NULL || fData == NULL) {
        fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
    }
        
    reset(RegexStaticSets::gStaticSets->fEmptyString);
}



RegexMatcher::RegexMatcher(const UnicodeString &regexp, const UnicodeString &input,
                           uint32_t flags, UErrorCode &status) {
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    fPattern           = fPatternOwned;
    fTraceDebug        = FALSE;
    fDeferredStatus    = U_ZERO_ERROR;
    fStack             = new UVector32(status); 
    fData              = fSmallData;
    fWordBreakItr      = NULL;
    if (U_FAILURE(status)) {
        return;
    }
    if (fPattern->fDataSize > (int32_t)(sizeof(fSmallData)/sizeof(int32_t))) {
        fData = (int32_t *)uprv_malloc(fPattern->fDataSize * sizeof(int32_t)); 
    }
    if (fStack == NULL || fData == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    reset(input);
}


RegexMatcher::RegexMatcher(const UnicodeString &regexp, 
                           uint32_t flags, UErrorCode &status) {
    UParseError    pe;
    fTraceDebug        = FALSE;
    fDeferredStatus    = U_ZERO_ERROR;
    fStack             = new UVector32(status); 
    fData              = fSmallData;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    fPattern           = fPatternOwned;
    fWordBreakItr      = NULL;
    if (U_FAILURE(status)) {
        return;
    }

    if (fPattern->fDataSize > (int32_t)(sizeof(fSmallData)/sizeof(int32_t))) {
        fData = (int32_t *)uprv_malloc(fPattern->fDataSize * sizeof(int32_t)); 
    }
    if (fStack == NULL || fData == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    reset(RegexStaticSets::gStaticSets->fEmptyString);
}



RegexMatcher::~RegexMatcher() {
    delete fStack;
    if (fData != fSmallData) {
        uprv_free(fData);
        fData = NULL;
    }
    if (fPatternOwned) {
        delete fPatternOwned;
        fPatternOwned = NULL;
        fPattern = NULL;
    }
    #if UCONFIG_NO_BREAK_ITERATION==0
    delete fWordBreakItr;
    #endif
}



static const UChar BACKSLASH  = 0x5c;
static const UChar DOLLARSIGN = 0x24;
//--------------------------------------------------------------------------------
//
//    appendReplacement
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::appendReplacement(UnicodeString &dest,
                                              const UnicodeString &replacement,
                                              UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return *this;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return *this;
    }

    // Copy input string from the end of previous match to start of current match
    int32_t  len = fMatchStart-fLastReplaceEnd;
    if (len > 0) {
        dest.append(*fInput, fLastReplaceEnd, len);
    }
    fLastReplaceEnd = fMatchEnd;
    

    // scan the replacement text, looking for substitutions ($n) and \escapes.
    //  TODO:  optimize this loop by efficiently scanning for '$' or '\',
    //         move entire ranges not containing substitutions.
    int32_t  replLen = replacement.length();
    int32_t  replIdx = 0;
    while (replIdx<replLen) {
        UChar  c = replacement.charAt(replIdx);
        replIdx++;
        if (c == BACKSLASH) {
            // Backslash Escape.  Copy the following char out without further checks.
            //                    Note:  Surrogate pairs don't need any special handling
            //                           The second half wont be a '$' or a '\', and
            //                           will move to the dest normally on the next
            //                           loop iteration.
            if (replIdx >= replLen) {
                break;
            }
            c = replacement.charAt(replIdx);

            if (c==0x55/*U*/ || c==0x75/*u*/) {
                // We have a \udddd or \Udddddddd escape sequence.
                UChar32 escapedChar = replacement.unescapeAt(replIdx);
                if (escapedChar != (UChar32)0xFFFFFFFF) {
                    dest.append(escapedChar);
                    // TODO:  Report errors for mal-formed \u escapes?
                    //        As this is, the original sequence is output, which may be OK.
                    continue;
                }
            }

            // Plain backslash escape.  Just put out the escaped character.
            dest.append(c);
            replIdx++;
            continue;
        }

        if (c != DOLLARSIGN) {
            // Normal char, not a $.  Copy it out without further checks.
            dest.append(c);
            continue;
        }

        // We've got a $.  Pick up a capture group number if one follows.
        // Consume at most the number of digits necessary for the largest capture
        // number that is valid for this pattern.

        int32_t numDigits = 0;
        int32_t groupNum  = 0;
        UChar32 digitC;
        for (;;) {
            if (replIdx >= replLen) {
                break;
            }
            digitC = replacement.char32At(replIdx);
            if (u_isdigit(digitC) == FALSE) {
                break;
            }
            replIdx = replacement.moveIndex32(replIdx, 1);
            groupNum=groupNum*10 + u_charDigitValue(digitC);
            numDigits++;
            if (numDigits >= fPattern->fMaxCaptureDigits) {
                break;
            }
        }


        if (numDigits == 0) {
            // The $ didn't introduce a group number at all.
            // Treat it as just part of the substitution text.
            dest.append(DOLLARSIGN);
            continue;
        }

        // Finally, append the capture group data to the destination.
        dest.append(group(groupNum, status));
        if (U_FAILURE(status)) {
            // Can fail if group number is out of range.
            break;
        }

    }

    return *this;
}



//--------------------------------------------------------------------------------
//
//    appendTail     Intended to be used in conjunction with appendReplacement()
//                   To the destination string, append everything following
//                   the last match position from the input string.
//
//--------------------------------------------------------------------------------
UnicodeString &RegexMatcher::appendTail(UnicodeString &dest) {
    int32_t  len = fInput->length()-fMatchEnd;
    if (len > 0) {
        dest.append(*fInput, fMatchEnd, len);
    }
    return dest;
}



//--------------------------------------------------------------------------------
//
//   end
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::end(UErrorCode &err) const {
    return end(0, err);
}



int32_t RegexMatcher::end(int32_t group, UErrorCode &err) const {
    if (U_FAILURE(err)) {
        return -1;
    }
    if (fMatch == FALSE) {
        err = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        err = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int32_t e = -1;
    if (group == 0) {
        e = fMatchEnd; 
    } else {
        // Get the position within the stack frame of the variables for
        //    this capture group.
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        e = fFrame->fExtra[groupOffset + 1];
    }
    return e;
}



//--------------------------------------------------------------------------------
//
//   find()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::find() {
    // Start at the position of the last match end.  (Will be zero if the
    //   matcher has been reset.
    //
    if (U_FAILURE(fDeferredStatus)) {
        return FALSE;
    }

    int32_t startPos = fMatchEnd;

    if (fMatch) {
        // Save the position of any previous successful match.
        fLastMatchEnd = fMatchEnd;

        if (fMatchStart == fMatchEnd) {
            // Previous match had zero length.  Move start position up one position
            //  to avoid sending find() into a loop on zero-length matches.
            if (startPos == fInput->length()) {
                fMatch = FALSE;
                return FALSE;
            }
            startPos = fInput->moveIndex32(startPos, 1);
        }
    } else {
        if (fLastMatchEnd >= 0) {
            // A previous find() failed to match.  Don't try again.
            //   (without this test, a pattern with a zero-length match
            //    could match again at the end of an input string.)
            return FALSE;
        }
    }

    int32_t inputLen = fInput->length();

    // Compute the position in the input string beyond which a match can not begin, because
    //   the minimum length match would extend past the end of the input.
    int32_t testLen  = inputLen - fPattern->fMinMatchLen;
    if (startPos > testLen) {
        fMatch = FALSE;
        return FALSE;
    }

    const UChar *inputBuf = fInput->getBuffer();
    UChar32  c;
    U_ASSERT(startPos >= 0);

    switch (fPattern->fStartType) {
    case START_NO_INFO:
        // No optimization was found. 
        //  Try a match at each input position.
        for (;;) {
            MatchAt(startPos, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            if (startPos >= testLen) {
                return FALSE;
            }
            U16_FWD_1(inputBuf, startPos, inputLen);
            // Note that it's perfectly OK for a pattern to have a zero-length
            //   match at the end of a string, so we must make sure that the loop
            //   runs with startPos == testLen the last time through.
        }
        U_ASSERT(FALSE);

    case START_START:
        // Matches are only possible at the start of the input string
        //   (pattern begins with ^ or \A)
        if (startPos > 0) {
            fMatch = FALSE;
            return FALSE;
        }
        MatchAt(startPos, fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        return fMatch;


    case START_SET:
        {
            // Match may start on any char from a pre-computed set.
            U_ASSERT(fPattern->fMinMatchLen > 0);
            for (;;) {
                int32_t pos = startPos;
                U16_NEXT(inputBuf, startPos, inputLen, c);  // like c = inputBuf[startPos++];
                if (c<256 && fPattern->fInitialChars8->contains(c) ||
                    c>=256 && fPattern->fInitialChars->contains(c)) {
                    MatchAt(pos, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (pos >= testLen) {
                    fMatch = FALSE;
                    return FALSE;
                }
            }
        }
        U_ASSERT(FALSE);

    case START_STRING:
    case START_CHAR:
        {
            // Match starts on exactly one char.
            U_ASSERT(fPattern->fMinMatchLen > 0);
            UChar32 theChar = fPattern->fInitialChar;
            for (;;) {
                int32_t pos = startPos;
                U16_NEXT(inputBuf, startPos, inputLen, c);  // like c = inputBuf[startPos++];
                if (c == theChar) {
                    MatchAt(pos, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (pos >= testLen) {
                    fMatch = FALSE;
                    return FALSE;
                }
            }
        }
        U_ASSERT(FALSE);

    case START_LINE:
        {
            UChar32  c;
            if (startPos == 0) {
                MatchAt(startPos, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
                U16_NEXT(inputBuf, startPos, inputLen, c);  // like c = inputBuf[startPos++];
            }

            for (;;) {
                c = inputBuf[startPos-1];
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029 )) {
                        if (c == 0x0d && startPos < inputLen && inputBuf[startPos] == 0x0a) {
                            startPos++;
                        }
                        MatchAt(startPos, fDeferredStatus);
                        if (U_FAILURE(fDeferredStatus)) {
                            return FALSE;
                        }
                        if (fMatch) {
                            return TRUE;
                        }
                }
                if (startPos >= testLen) {
                    fMatch = FALSE;
                    return FALSE;
                }
                U16_NEXT(inputBuf, startPos, inputLen, c);  // like c = inputBuf[startPos++];
                // Note that it's perfectly OK for a pattern to have a zero-length
                //   match at the end of a string, so we must make sure that the loop
                //   runs with startPos == testLen the last time through.
            }
        }

    default:
        U_ASSERT(FALSE);
    }

    U_ASSERT(FALSE);
    return FALSE;
}



UBool RegexMatcher::find(int32_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    int32_t inputLen = fInput->length();
    if (start < 0 || start > inputLen) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    this->reset();
    fMatchEnd = start;
    return find();
}



//--------------------------------------------------------------------------------
//
//  group()
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::group(UErrorCode &status) const {
    return group(0, status);
}



UnicodeString RegexMatcher::group(int32_t groupNum, UErrorCode &status) const {
    int32_t  s = start(groupNum, status);
    int32_t  e = end(groupNum, status);

    // Note:  calling start() and end() above will do all necessary checking that
    //        the group number is OK and that a match exists.  status will be set.
    if (U_FAILURE(status)) {
        return UnicodeString();
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return UnicodeString();
    }

    if (s < 0) {
        // A capture group wasn't part of the match 
        return UnicodeString();
    }
    U_ASSERT(s <= e);
    return UnicodeString(*fInput, s, e-s);
}




int32_t RegexMatcher::groupCount() const {
    return fPattern->fGroupMap->size();
}



const UnicodeString &RegexMatcher::input() const {
    return *fInput;
}




//--------------------------------------------------------------------------------
//
//  lookingAt()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::lookingAt(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    MatchAt(0, status);
    return fMatch;
}


UBool RegexMatcher::lookingAt(int32_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    if (start < 0 || start > fInput->length()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    reset();
    MatchAt(start, status);
    return fMatch;
}



//--------------------------------------------------------------------------------
//
//  matches()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::matches(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    MatchAt(0, status);
    UBool   success  = (fMatch && fMatchEnd==fInput->length());
    return success;
}


UBool RegexMatcher::matches(int32_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    if (start < 0 || start > fInput->length()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    reset();
    MatchAt(start, status);
    UBool   success  = (fMatch && fMatchEnd==fInput->length());
    return success;
}



const RegexPattern &RegexMatcher::pattern() const {
    return *fPattern;
}



//--------------------------------------------------------------------------------
//
//    replaceAll
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::replaceAll(const UnicodeString &replacement, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *fInput;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return *fInput;
    }
    UnicodeString destString;
    for (reset(); find(); ) {
        appendReplacement(destString, replacement, status);
        if (U_FAILURE(status)) {
            break;
        }
    }
    appendTail(destString);
    return destString;
}




//--------------------------------------------------------------------------------
//
//    replaceFirst
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::replaceFirst(const UnicodeString &replacement, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *fInput;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return *fInput;
    }

    reset();
    if (!find()) {
        return *fInput;
    }

    UnicodeString destString;
    appendReplacement(destString, replacement, status);
    appendTail(destString);
    return destString;
}



//--------------------------------------------------------------------------------
//
//     reset
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::reset() {
    fMatchStart     = 0;
    fMatchEnd       = 0;
    fLastMatchEnd   = -1;
    fLastReplaceEnd = 0;
    fMatch          = FALSE;
    resetStack();
    return *this;
}



RegexMatcher &RegexMatcher::reset(const UnicodeString &input) {
    fInput          = &input;
    reset();
    if (fWordBreakItr != NULL) {
        #if UCONFIG_NO_BREAK_ITERATION==0
        fWordBreakItr->setText(input);
        #endif
    }
    return *this;
}

/*RegexMatcher &RegexMatcher::reset(const UChar *) {
    fDeferredStatus = U_INTERNAL_PROGRAM_ERROR;
    return *this;
}*/


RegexMatcher &RegexMatcher::reset(int32_t position, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    reset();
    if (position < 0 || position >= fInput->length()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return *this;
    }
    fMatchEnd = position;
    return *this;
}





//--------------------------------------------------------------------------------
//
//    setTrace
//
//--------------------------------------------------------------------------------
void RegexMatcher::setTrace(UBool state) {
    fTraceDebug = state;
}



//---------------------------------------------------------------------
//
//   split
//
//---------------------------------------------------------------------
int32_t  RegexMatcher::split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode       &status)
{
    //
    // Check arguements for validity
    //
    if (U_FAILURE(status)) {
        return 0;
    };

    if (destCapacity < 1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }


    //
    // Reset for the input text
    //
    reset(input);
    int32_t   inputLen = input.length();
    int32_t   nextOutputStringStart = 0;
    if (inputLen == 0) {
        return 0;
    }


    //
    // Loop through the input text, searching for the delimiter pattern
    //
    int32_t i;
    int32_t numCaptureGroups = fPattern->fGroupMap->size();
    for (i=0; ; i++) {
        if (i>=destCapacity-1) {
            // There is one or zero output string left.
            // Fill the last output string with whatever is left from the input, then exit the loop.
            //  ( i will be == destCapicity if we filled the output array while processing
            //    capture groups of the delimiter expression, in which case we will discard the
            //    last capture group saved in favor of the unprocessed remainder of the
            //    input string.)
            i = destCapacity-1;
            int32_t remainingLength = inputLen-nextOutputStringStart;
            if (remainingLength > 0) {
                dest[i].setTo(input, nextOutputStringStart, remainingLength);
            }
            break;
        }
        if (find()) {
            // We found another delimiter.  Move everything from where we started looking
            //  up until the start of the delimiter into the next output string.
            int32_t fieldLen = fMatchStart - nextOutputStringStart;
            dest[i].setTo(input, nextOutputStringStart, fieldLen);
            nextOutputStringStart = fMatchEnd;

            // If the delimiter pattern has capturing parentheses, the captured
            //  text goes out into the next n destination strings.
            int32_t groupNum;
            for (groupNum=1; groupNum<=numCaptureGroups; groupNum++) {
                if (i==destCapacity-1) {
                    break;
                }
                i++;
                dest[i] = group(groupNum, status);
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
            dest[i].setTo(input, nextOutputStringStart, inputLen-nextOutputStringStart);
            break;
        }
    }
    return i+1;
}



//--------------------------------------------------------------------------------
//
//     start
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::start(UErrorCode &status) const {
    return start(0, status);
}




int32_t RegexMatcher::start(int32_t group, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return -1;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return -1;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int32_t s;
    if (group == 0) {
        s = fMatchStart; 
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
    }
    return s;
}



//================================================================================
//
//    Code following this point in this file is the internal
//    Match Engine Implementation.
//
//================================================================================


//--------------------------------------------------------------------------------
//
//   resetStack
//           Discard any previous contents of the state save stack, and initialize a
//           new stack frame to all -1.  The -1s are needed for capture group limits, 
//           where they indicate that a group has not yet matched anything.
//--------------------------------------------------------------------------------
REStackFrame *RegexMatcher::resetStack() {
    // Discard any previous contents of the state save stack, and initialize a
    //  new stack frame to all -1.  The -1s are needed for capture group limits, where
    //  they indicate that a group has not yet matched anything.
    fStack->removeAllElements();

    int32_t *iFrame = fStack->reserveBlock(fPattern->fFrameSize, fDeferredStatus);
    int32_t i;
    for (i=0; i<fPattern->fFrameSize; i++) {
        iFrame[i] = -1;
    }
    return (REStackFrame *)iFrame;
}



//--------------------------------------------------------------------------------
//
//   isWordBoundary 
//                     in perl, "xab..cd..", \b is true at positions 0,3,5,7
//                     For us,
//                       If the current char is a combining mark,
//                          \b is FALSE.
//                       Else Scan backwards to the first non-combining char.
//                            We are at a boundary if the this char and the original chars are
//                               opposite in membership in \w set
//
//          parameters:   pos   - the current position in the input buffer
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::isWordBoundary(int32_t pos) {
    UBool isBoundary = FALSE;
    UBool cIsWord    = FALSE;
    
    // Determine whether char c at current position is a member of the word set of chars.
    // If we're off the end of the string, behave as though we're not at a word char.
    if (pos < fInput->length()) {
        UChar32  c = fInput->char32At(pos);
        if (u_hasBinaryProperty(c, UCHAR_GRAPHEME_EXTEND) || u_charType(c) == U_FORMAT_CHAR) {
            // Current char is a combining one.  Not a boundary.
            return FALSE;
        }
        cIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(c);
    }
    
    // Back up until we come to a non-combining char, determine whether
    //  that char is a word char.
    UBool prevCIsWord = FALSE;
    int32_t prevPos = pos;
    for (;;) {
        if (prevPos == 0) {
            break;
        }
        prevPos = fInput->moveIndex32(prevPos, -1);
        UChar32 prevChar = fInput->char32At(prevPos);
        if (!(u_hasBinaryProperty(prevChar, UCHAR_GRAPHEME_EXTEND)
              || u_charType(prevChar) == U_FORMAT_CHAR)) {
            prevCIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(prevChar);
            break;
        }
    }
    isBoundary = cIsWord ^ prevCIsWord;
    return isBoundary;
}

//--------------------------------------------------------------------------------
//
//   isUWordBoundary 
//
//         Test for a word boundary using RBBI word break.
//
//          parameters:   pos   - the current position in the input buffer
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::isUWordBoundary(int32_t pos) {
    UBool       returnVal = FALSE;
#if UCONFIG_NO_BREAK_ITERATION==0
    UErrorCode  status    = U_ZERO_ERROR;  
    
    // If we haven't yet created a break iterator for this matcher, do it now.
    if (fWordBreakItr == NULL) {
        fWordBreakItr = 
            (RuleBasedBreakIterator *)BreakIterator::createWordInstance(Locale::getEnglish(), status);
        if (U_FAILURE(status)) {
            // TODO:  reliable error reporting for BI failures.
            return FALSE;
        }
        fWordBreakItr->setText(*fInput);
    }

    returnVal = fWordBreakItr->isBoundary(pos);
#endif
    return   returnVal;
}

//--------------------------------------------------------------------------------
//
//   StateSave      
//       Make a new stack frame, initialized as a copy of the current stack frame.
//       Set the pattern index in the original stack frame from the operand value
//       in the opcode.  Execution of the engine continues with the state in
//       the newly created stack frame
//
//       Note that reserveBlock() may grow the stack, resulting in the
//       whole thing being relocated in memory.  
//
//--------------------------------------------------------------------------------
inline REStackFrame *RegexMatcher::StateSave(REStackFrame *fp, int32_t savePatIdx, int32_t frameSize, UErrorCode &status) {
    // push storage for a new frame. 
    int32_t *newFP = fStack->reserveBlock(frameSize, status);
    fp = (REStackFrame *)(newFP - frameSize);  // in case of realloc of stack.
    
    // New stack frame = copy of old top frame.
    int32_t *source = (int32_t *)fp;
    int32_t *dest   = newFP;
    for (;;) {
        *dest++ = *source++;
        if (source == newFP) {
            break;
        }
    }
    
    fp->fPatIdx = savePatIdx;
    return (REStackFrame *)newFP;
}
    
            
//--------------------------------------------------------------------------------
//
//   MatchAt      This is the actual matching engine.
//
//--------------------------------------------------------------------------------
void RegexMatcher::MatchAt(int32_t startIdx, UErrorCode &status) {
    UBool       isMatch  = FALSE;      // True if the we have a match.

    int32_t     op;                    // Operation from the compiled pattern, split into
    int32_t     opType;                //    the opcode
    int32_t     opValue;               //    and the operand value.

    #ifdef REGEX_RUN_DEBUG
    if (fTraceDebug)
    {
        printf("MatchAt(startIdx=%d)\n", startIdx);
        printf("Original Pattern: ");
        int32_t i;
        for (i=0; i<fPattern->fPattern.length(); i++) {
            printf("%c", fPattern->fPattern.charAt(i));
        }
        printf("\n");
        printf("Input String: ");
        for (i=0; i<fInput->length(); i++) {
            UChar c = fInput->charAt(i);
            if (c<32 || c>256) {
                c = '.';
            }
            printf("%c", c);
        }
        printf("\n");
        printf("\n");
    }
    #endif

    if (U_FAILURE(status)) {
        return;
    }

    //  Cache frequently referenced items from the compiled pattern
    //  in local variables.
    //
    int32_t             *pat           = fPattern->fCompiledPat->getBuffer();

    const UChar         *litText       = fPattern->fLiteralText.getBuffer();
    UVector             *sets          = fPattern->fSets;
    int32_t              inputLen      = fInput->length();
    const UChar         *inputBuf      = fInput->getBuffer();

    REStackFrame        *fp            = resetStack();
    int32_t              frameSize     = fPattern->fFrameSize;

    fp->fPatIdx   = 0;
    fp->fInputIdx = startIdx;

    // Zero out the pattern's static data
    int32_t i;
    for (i = 0; i<fPattern->fDataSize; i++) {
        fData[i] = 0;
    }

    //
    //  Main loop for interpreting the compiled pattern.
    //  One iteration of the loop per pattern operation performed.
    //
    for (;;) {
#if 0
        if (_heapchk() != _HEAPOK) {
            fprintf(stderr, "Heap Trouble\n");
        }
#endif
        op      = pat[fp->fPatIdx];
        opType  = URX_TYPE(op);
        opValue = URX_VAL(op);
        #ifdef REGEX_RUN_DEBUG
        if (fTraceDebug) {
            printf("inputIdx=%d   inputChar=%c   sp=%3d  ", fp->fInputIdx,
                fInput->char32At(fp->fInputIdx), (int32_t *)fp-fStack->getBuffer());
            fPattern->dumpOp(fp->fPatIdx);
        }
        #endif
        fp->fPatIdx++;

        switch (opType) {


        case URX_NOP:
            break;


        case URX_BACKTRACK:
            // Force a backtrack.  In some circumstances, the pattern compiler
            //   will notice that the pattern can't possibly match anything, and will
            //   emit one of these at that point.
            fp = (REStackFrame *)fStack->popFrame(frameSize);
            break;


        case URX_ONECHAR:
            if (fp->fInputIdx < inputLen) {
                UChar32   c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (c == opValue) {           
                    break;
                }
            }
            fp = (REStackFrame *)fStack->popFrame(frameSize);
            break;


        case URX_STRING:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                int32_t   stringStartIdx = opValue;
                int32_t   stringLen;

                op      = pat[fp->fPatIdx];     // Fetch the second operand
                fp->fPatIdx++;
                opType    = URX_TYPE(op);
                stringLen = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                U_ASSERT(stringLen >= 2);

                if (fp->fInputIdx + stringLen > inputLen) {
                    // No match.  String is longer than the remaining input text.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                const UChar * pInp = inputBuf + fp->fInputIdx;
                const UChar * pPat = litText+stringStartIdx;
                const UChar * pEnd = pInp + stringLen;
                for(;;) {
                    if (*pInp == *pPat) {
                        pInp++;
                        pPat++;
                        if (pInp == pEnd) {
                            // Successful Match.
                            fp->fInputIdx += stringLen;
                            break;
                        }
                    } else {
                        // Match failed.
                        fp = (REStackFrame *)fStack->popFrame(frameSize);
                        break;
                    }
                }
            }
            break;



        case URX_STATE_SAVE:
            fp = StateSave(fp, opValue, frameSize, status);
            break;


        case URX_END:
            // The match loop will exit via this path on a successful match,
            //   when we reach the end of the pattern.
            isMatch = TRUE;
            goto  breakFromLoop;

        // Start and End Capture stack frame variables are layout out like this:
            //  fp->fExtra[opValue]  - The start of a completed capture group
            //             opValue+1 - The end   of a completed capture group
            //             opValue+2 - the start of a capture group whose end
            //                          has not yet been reached (and might not ever be).
        case URX_START_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < frameSize-3);
            fp->fExtra[opValue+2] = fp->fInputIdx;
            break;


        case URX_END_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < frameSize-3);
            U_ASSERT(fp->fExtra[opValue+2] >= 0);            // Start pos for this group must be set.
            fp->fExtra[opValue]   = fp->fExtra[opValue+2];   // Tentative start becomes real.
            fp->fExtra[opValue+1] = fp->fInputIdx;           // End position
            U_ASSERT(fp->fExtra[opValue] <= fp->fExtra[opValue+1]);
            break;

            
        case URX_DOLLAR:                   //  $, test for End of line
                                           //     or for position before new line at end of input
            if (fp->fInputIdx < inputLen-2) {
                // We are no where near the end of input.  Fail.
                fp = (REStackFrame *)fStack->popFrame(frameSize);
                break;
            }
            if (fp->fInputIdx >= inputLen) {
                // We really are at the end of input.  Success.
                break;
            }
            // If we are positioned just before a new-line that is located at the
            //   end of input, succeed.
            if (fp->fInputIdx == inputLen-1) {
                UChar32 c = fInput->char32At(fp->fInputIdx);
                if ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029) {
                    // If not in the middle of a CR/LF sequence
                    if ( !(c==0x0a && fp->fInputIdx>0 && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        break;
                        // At new-line at end of input. Success
                    }
                }
            }

            if (fp->fInputIdx == inputLen-2) {
                if (fInput->char32At(fp->fInputIdx) == 0x0d && fInput->char32At(fp->fInputIdx+1) == 0x0a) {
                    break;                         // At CR/LF at end of input.  Success
                }
            }

            fp = (REStackFrame *)fStack->popFrame(frameSize);

            break;


         case URX_DOLLAR_M:                //  $, test for End of line in multi-line mode
             {
                 if (fp->fInputIdx >= inputLen) {
                     // We really are at the end of input.  Success.
                     break;
                 }
                 // If we are positioned just before a new-line, succeed.
                 // It makes no difference where the new-line is within the input.
                 UChar32 c = inputBuf[fp->fInputIdx];
                 if ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029) {
                     // At a line end, except for the odd chance of  being in the middle of a CR/LF sequence
                     if ( !(c==0x0a && fp->fInputIdx>0 && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        break;                         // At new-line at end of input. Success
                     }
                 }
                 
                 // not at a new line.  Fail.
                 fp = (REStackFrame *)fStack->popFrame(frameSize);
             }
             break;


       case URX_CARET:                    //  ^, test for start of line
            if (fp->fInputIdx != 0) {
                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }           
            break;


       case URX_CARET_M:                   //  ^, test for start of line in mulit-line mode
           {
               if (fp->fInputIdx == 0) {
                   // We are at the start input.  Success.
                   break;
               }
               // Check whether character just before the current pos is a new-line
               //   unless we are at the end of input
               UChar  c = inputBuf[fp->fInputIdx - 1]; 
               if ((fp->fInputIdx < inputLen) && 
                   ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                   //  It's a new-line.  ^ is true.  Success.
                   break;                        
               }
               // Not at the start of a line.  Fail.
               fp = (REStackFrame *)fStack->popFrame(frameSize);
           }             
           break;


        case URX_BACKSLASH_B:          // Test for word boundaries
            {
                UBool success = isWordBoundary(fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                }
            }
            break;


        case URX_BACKSLASH_BU:          // Test for word boundaries, Unicode-style
            {
                UBool success = isUWordBoundary(fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                }
            }
            break;


        case URX_BACKSLASH_D:            // Test for decimal digit
            {
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                UChar32 c = fInput->char32At(fp->fInputIdx);   
                int8_t ctype = u_charType(c);
                UBool success = (ctype == U_DECIMAL_DIGIT_NUMBER);
                success ^= (opValue != 0);        // flip sense for \D
                if (success) {
                    fp->fInputIdx = fInput->moveIndex32(fp->fInputIdx, 1);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                }
            }
            break;




        case URX_BACKSLASH_G:          // Test for position at end of previous match
            if (!((fMatch && fp->fInputIdx==fMatchEnd) || fMatch==FALSE && fp->fInputIdx==0)) {
                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }
            break;


        case URX_BACKSLASH_X:     
            //  Match a Grapheme, as defined by Unicode TR 29.
            //  Differs slightly from Perl, which consumes combining marks independently
            //    of context.
            {                  

                // Fail if at end of input
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                // Examine (and consume) the current char.
                //   Dispatch into a little state machine, based on the char.
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                UnicodeSet **sets = fPattern->fStaticSets;
                if (sets[URX_GC_NORMAL]->contains(c))  goto GC_Extend;
                if (sets[URX_GC_CONTROL]->contains(c)) goto GC_Control;
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                goto GC_Extend;



GC_L:
                if (fp->fInputIdx >= inputLen)         goto GC_Done;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                U16_PREV(inputBuf, 0, fp->fInputIdx, c);
                goto GC_Extend;

GC_V:
                if (fp->fInputIdx >= inputLen)         goto GC_Done;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                U16_PREV(inputBuf, 0, fp->fInputIdx, c);
                goto GC_Extend;

GC_T:
                if (fp->fInputIdx >= inputLen)         goto GC_Done;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                U16_PREV(inputBuf, 0, fp->fInputIdx, c);
                goto GC_Extend;

GC_Extend:
                // Combining characters are consumed here
                for (;;) {
                    if (fp->fInputIdx >= inputLen) {
                        break;
                    }
                    U16_GET(inputBuf, 0, fp->fInputIdx, inputLen, c);
                    if (sets[URX_GC_EXTEND]->contains(c) == FALSE) {
                        break;
                    }
                    U16_FWD_1(inputBuf, fp->fInputIdx, inputLen);
                }
                goto GC_Done;

GC_Control:
                // Most control chars stand alone (don't combine with combining chars),  
                //   except for that CR/LF sequence is a single grapheme cluster.
                if (c == 0x0d && fp->fInputIdx < inputLen && inputBuf[fp->fInputIdx] == 0x0a) {
                    fp->fInputIdx++;
                }

GC_Done:
                break;
            }
            



        case URX_BACKSLASH_Z:          // Test for end of line
            if (fp->fInputIdx < inputLen) {
                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }
            break;



        case URX_STATIC_SETREF:
            {
                // Test input character against one of the predefined sets
                //    (Word Characters, for example)
                // The high bit of the op value is a flag for the match polarity.
                //    0:   success if input char is in set.
                //    1:   success if input char is not in set.
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                UBool success = ((opValue & URX_NEG_SET) == URX_NEG_SET);  
                opValue &= ~URX_NEG_SET;
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c)) {
                        success = !success;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c)) {
                        success = !success;
                    }
                }
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                }
            }
            break;
            

        case URX_STAT_SETREF_N:
            {
                // Test input character for NOT being a member of  one of 
                //    the predefined sets (Word Characters, for example)
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c) == FALSE) {
                        break;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c) == FALSE) {
                        break;
                    }
                }

                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }
            break;
            

        case URX_SETREF:
            if (fp->fInputIdx < inputLen) {
                // There is input left.  Pick up one char and test it for set membership.
                UChar32   c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                U_ASSERT(opValue > 0 && opValue < sets->size());
                if (c<256) {
                    Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                    if (s8->contains(c)) {
                        break;
                    }
                } else {
                    
                    UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                    if (s->contains(c)) {
                        // The character is in the set.  A Match.
                        break;
                    }
                }
            } 
            // Either at end of input, or the character wasn't in the set.
            // Either way, we need to back track out.
            fp = (REStackFrame *)fStack->popFrame(frameSize);
            break;
            

        case URX_DOTANY:
            {
                // . matches anything, but stops at end-of-line.
                if (fp->fInputIdx >= inputLen) {
                    // At end of input.  Match failed.  Backtrack out.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }
                // There is input left.  Advance over one char, unless we've hit end-of-line
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    // End of line in normal mode.   . does not match.
                        fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }
            }
            break;
            
            
        case URX_DOTANY_ALL:
            {
                // ., in dot-matches-all (including new lines) mode
                if (fp->fInputIdx >= inputLen) {
                    // At end of input.  Match failed.  Backtrack out.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }
                // There is input left.  Advance over one char, except if we are
                //   at a cr/lf, advance over both of them.
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (c==0x0d) {
                    // In the case of a CR/LF, we need to advance over both.
                    UChar nextc = inputBuf[fp->fInputIdx];
                    if (nextc == 0x0a) {
                        fp->fInputIdx++;
                    }
                }
            }
            break;

        case URX_DOTANY_PL:
            // Match all up to and end-of-line or end-of-input.
            {
                //  Fail if input already exhausted.
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }
                
                // There is input left. Fail if we are  at the end of a line.
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    // End of line in normal mode.   . does not match.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }
                
                // There was input left.  Consume it until we hit the end of a line,
                //   or until it's exhausted.
                while  (fp->fInputIdx < inputLen) {
                    U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                    if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                        ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                        U16_BACK_1(inputBuf, 0, fp->fInputIdx)
                            // Scan has reached a line-end.  We are done.
                            break;
                    }
                }
            }
            break;

        case URX_DOTANY_ALL_PL:
            {
                // Match up to end of input.  Fail if already at end of input.
                if (fp->fInputIdx >= inputLen) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                } else {
                    fp->fInputIdx = inputLen;
                }
            }
            break;


        case URX_JMP:
            fp->fPatIdx = opValue;
            break;

        case URX_FAIL:
            isMatch = FALSE;
            goto breakFromLoop;

        case URX_JMP_SAV:
            U_ASSERT(opValue < fPattern->fCompiledPat->size());
            fp = StateSave(fp, fp->fPatIdx, frameSize, status);  // State save to loc following current
            fp->fPatIdx = opValue;                               // Then JMP.
            break;

        case URX_JMP_SAV_X:
            // This opcode is used with (x)+, when x can match a zero length string.
            // Same as JMP_SAV, except conditional on the match having made forward progress.
            // Destination of the JMP must be a URX_STO_INP_LOC, from which we get the
            //   data address of the input position at the start of the loop.
            {
                U_ASSERT(opValue > 0 && opValue < fPattern->fCompiledPat->size());
                int32_t  stoOp = pat[opValue-1];
                U_ASSERT(URX_TYPE(stoOp) == URX_STO_INP_LOC);
                int32_t  frameLoc = URX_VAL(stoOp);
                U_ASSERT(frameLoc >= 0 && frameLoc < frameSize);
                int32_t prevInputIdx = fp->fExtra[frameLoc];
                U_ASSERT(prevInputIdx <= fp->fInputIdx);
                if (prevInputIdx < fp->fInputIdx) {
                    // The match did make progress.  Repeat the loop.
                    fp = StateSave(fp, fp->fPatIdx, frameSize, status);  // State save to loc following current
                    fp->fPatIdx = opValue;
                    fp->fExtra[frameLoc] = fp->fInputIdx;
                } 
                // If the input position did not advance, we do nothing here,
                //   execution will fall out of the loop.
            }
            break;

        case URX_CTR_INIT:
            {
                U_ASSERT(opValue >= 0 && opValue < frameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero

                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = pat[instrOperandLoc+1];
                int32_t maxCount = pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    fp = StateSave(fp, loopLoc+1, frameSize, status);
                }
                if (maxCount == 0) {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                }
            }
            break;

        case URX_CTR_LOOP:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT);
                int32_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = pat[opValue+2];
                int32_t maxCount = pat[opValue+3];
                // Increment the counter.  Note: we're not worrying about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                if ((uint32_t)*pCounter >= (uint32_t)maxCount) {
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                if (*pCounter >= minCount) {
                    fp = StateSave(fp, fp->fPatIdx, frameSize, status);
                }
                fp->fPatIdx = opValue + 4;    // Loop back.
            }
            break;

        case URX_CTR_INIT_NG:
            {
                U_ASSERT(opValue >= 0 && opValue < frameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero

                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = pat[instrOperandLoc+1];
                int32_t maxCount = pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    if (maxCount != 0) {
                        fp = StateSave(fp, fp->fPatIdx, frameSize, status);
                    }
                    fp->fPatIdx = loopLoc+1;   // Continue with stuff after repeated block
                } 
            }
            break;

        case URX_CTR_LOOP_NG:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT_NG);
                int32_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = pat[opValue+2];
                int32_t maxCount = pat[opValue+3];
                // Increment the counter.  Note: we're not worrying about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);

                if ((uint32_t)*pCounter >= (uint32_t)maxCount) {
                    // The loop has matched the maximum permitted number of times.
                    //   Break out of here with no action.  Matching will
                    //   continue with the following pattern.
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }

                if (*pCounter < minCount) {
                    // We haven't met the minimum number of matches yet.
                    //   Loop back for another one.
                    fp->fPatIdx = opValue + 4;    // Loop back.
                } else {
                    // We do have the minimum number of matches.
                    //   Fall into the following pattern, but first do
                    //   a state save to the top of the loop, so that a failure
                    //   in the following pattern will try another iteration of the loop.
                    fp = StateSave(fp, opValue + 4, frameSize, status);
                }
            }
            break;

            // TODO:  Possessive flavor of loop ops, or take them out if no longer needed.

        case URX_STO_SP:
            U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
            fData[opValue] = fStack->size();
            break;

        case URX_LD_SP:
            {
                U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
                int32_t newStackSize = fData[opValue];
                U_ASSERT(newStackSize <= fStack->size());
                int32_t *newFP = fStack->getBuffer() + newStackSize - frameSize;
                if (newFP == (int32_t *)fp) {
                    break;
                }
                int32_t i;
                for (i=0; i<frameSize; i++) {
                    newFP[i] = ((int32_t *)fp)[i];
                }
                fp = (REStackFrame *)newFP;
                fStack->setSize(newStackSize);
            }
            break;

        case URX_BACKREF:
        case URX_BACKREF_I:
            {
                U_ASSERT(opValue < frameSize);
                int32_t groupStartIdx = fp->fExtra[opValue];
                int32_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                int32_t len = groupEndIdx-groupStartIdx;
                if (groupStartIdx < 0) {
                    // This capture group has not participated in the match thus far,
                    fp = (REStackFrame *)fStack->popFrame(frameSize);   // FAIL, no match.
                }

                if (len == 0) {
                        //   The capture group match was of an empty string.
                        //   Verified by testing:  Perl matches succeed in this case, so
                        //   we do too.
                        break;
                    }

                UBool  haveMatch = FALSE;
                if (fp->fInputIdx + len <= inputLen) {
                    if (opType == URX_BACKREF) {
                        if (u_strncmp(inputBuf+groupStartIdx, inputBuf+fp->fInputIdx, len) == 0) {
                            haveMatch = TRUE;
                        }
                    } else {
                        if (u_strncasecmp(inputBuf+groupStartIdx, inputBuf+fp->fInputIdx,
                                  len, U_FOLD_CASE_DEFAULT) == 0) {
                            haveMatch = TRUE;
                        }
                    }
                }
                if (haveMatch) {
                    fp->fInputIdx += len;     // Match.  Advance current input position.
                } else {
                    fp = (REStackFrame *)fStack->popFrame(frameSize);   // FAIL, no match.
                }
            }
            break;

        case URX_STO_INP_LOC:
            {
                U_ASSERT(opValue >= 0 && opValue < frameSize);
                fp->fExtra[opValue] = fp->fInputIdx;
            }
            break;

        case URX_JMPX:
            {
                int32_t instrOperandLoc = fp->fPatIdx;
                fp->fPatIdx += 1;
                int32_t dataLoc  = URX_VAL(pat[instrOperandLoc]);
                U_ASSERT(dataLoc >= 0 && dataLoc < frameSize);
                int32_t savedInputIdx = fp->fExtra[dataLoc];
                U_ASSERT(savedInputIdx <= fp->fInputIdx);
                if (savedInputIdx < fp->fInputIdx) {
                    fp->fPatIdx = opValue;                               // JMP
                } else {
                     fp = (REStackFrame *)fStack->popFrame(frameSize);   // FAIL, no progress in loop.
                }
            }
            break;

        case URX_LA_START:
            {
                // Entering a lookahead block.
                // Save Stack Ptr, Input Pos.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
            }
            break;

        case URX_LA_END:
            {
                // Leaving a look-ahead block.
                //  restore Stack Ptr, Input Pos to positions they had on entry to block.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t stackSize = fStack->size();
                int32_t newStackSize = fData[opValue];
                U_ASSERT(stackSize >= newStackSize);
                if (stackSize > newStackSize) {
                    int32_t *newFP = fStack->getBuffer() + newStackSize - frameSize;
                    int32_t i;
                    for (i=0; i<frameSize; i++) {
                        newFP[i] = ((int32_t *)fp)[i];
                    }
                    fp = (REStackFrame *)newFP;
                    fStack->setSize(newStackSize);
                }
                fp->fInputIdx = fData[opValue+1];
            }
            break;

        case URX_ONECHAR_I:
            if (fp->fInputIdx < inputLen) {
                UChar32   c;
                U16_NEXT(inputBuf, fp->fInputIdx, inputLen, c);
                if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {           
                    break;
                }
            } 
            fp = (REStackFrame *)fStack->popFrame(frameSize);
            break;

        case URX_STRING_I:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                int32_t stringStartIdx, stringLen;
                stringStartIdx = opValue;

                op      = pat[fp->fPatIdx];
                fp->fPatIdx++;
                opType  = URX_TYPE(op);
                opValue = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                stringLen = opValue;
                
                int32_t stringEndIndex = fp->fInputIdx + stringLen;
                if (stringEndIndex <= inputLen) {
                    if (u_strncasecmp(inputBuf+fp->fInputIdx, litText+stringStartIdx,
                        stringLen, U_FOLD_CASE_DEFAULT) == 0) {
                        // Success.  Advance the current input position.
                        fp->fInputIdx = stringEndIndex;
                        break;
                    }
                }                 
                // No match.  Back up matching to a saved state
                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }
            break;

        case URX_LB_START:
            {
                // Entering a look-behind block.
                // Save Stack Ptr, Input Pos.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                // Init the variable containing the start index for attempted matches.
                fData[opValue+2] = -1;
                // Save input string length, then reset to pin any matches to end at
                //   the current position.
                fData[opValue+3] = inputLen;
                inputLen         = fp->fInputIdx;
            }
            break;


        case URX_LB_CONT:
            {
                // Positive Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.

                // Fetch the min and max possible match lengths.  They are the operands
                //   of this op in the pattern.
                int32_t minML = pat[fp->fPatIdx++];
                int32_t maxML = pat[fp->fPatIdx++];
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);

                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;   // Because U16_BACK is unsafe starting at 0.
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match.  Backtrack out, and out of the
                    //   Look Behind altogether.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    int32_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= inputLen);
                    U_ASSERT(restoreInputLen <= fInput->length());
                    inputLen = restoreInputLen;
                    break;
                }

                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will fall off the end of the loop.)
                fp = StateSave(fp, fp->fPatIdx-3, frameSize, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;

        case URX_LB_END:
            // End of a look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != inputLen) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or fail
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                // Look-behind match is good.  Restore the orignal input string length,
                //   which had been truncated to pin the end of the lookbehind match to the 
                //   position being looked-behind.
                int32_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= inputLen);
                U_ASSERT(originalInputLen <= fInput->length());
                inputLen = originalInputLen;
            }
            break;


        case URX_LBN_CONT:
            {
                // Negative Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.

                // Fetch the extra parameters of this op.
                int32_t minML       = pat[fp->fPatIdx++];
                int32_t maxML       = pat[fp->fPatIdx++];
                int32_t continueLoc = pat[fp->fPatIdx++];
                        continueLoc = URX_VAL(continueLoc);
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                U_ASSERT(continueLoc > fp->fPatIdx);

                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;   // Because U16_BACK is unsafe starting at 0.
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match, which means that the negative lookbehind as
                    //  a whole has succeeded.  Jump forward to the continue location
                    int32_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= inputLen);
                    U_ASSERT(restoreInputLen <= fInput->length());
                    inputLen = restoreInputLen;
                    fp->fPatIdx = continueLoc;
                    break;
                }

                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will cause a FAIL out of the loop altogether.)
                fp = StateSave(fp, fp->fPatIdx-4, frameSize, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;

        case URX_LBN_END:
            // End of a negative look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != inputLen) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or succeed
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(frameSize);
                    break;
                }

                // Look-behind expression matched, which means look-behind test as
                //   a whole Fails
                
                //   Restore the orignal input string length, which had been truncated 
                //   inorder to pin the end of the lookbehind match  
                //   to the position being looked-behind.
                int32_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= inputLen);
                U_ASSERT(originalInputLen <= fInput->length());
                inputLen = originalInputLen;

                // Restore original stack position, discarding any state saved
                //   by the successful pattern match.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t newStackSize = fData[opValue];
                U_ASSERT(fStack->size() > newStackSize);
                fStack->setSize(newStackSize);
                
                //  FAIL, which will take control back to someplace 
                //  prior to entering the look-behind test.
                fp = (REStackFrame *)fStack->popFrame(frameSize);
            }
            break;


        case URX_LOOP_SR_I:
            // Loop Initialization for the optimized implementation of
            //     [some character set]*
            //   This op scans through all matching input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                U_ASSERT(opValue > 0 && opValue < sets->size());
                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                UnicodeSet   *s  = (UnicodeSet *)sets->elementAt(opValue);

                // Loop through input, until either the input is exhausted or
                //   we reach a character that is not a member of the set.
                int32_t ix = fp->fInputIdx;
                for (;;) {
                    if (ix >= inputLen) {
                        break;
                    }
                    UChar32   c;
                    U16_NEXT(inputBuf, ix, inputLen, c);
                    if (c<256) {
                        if (s8->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    } else {
                        if (s->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    }
                }

                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }

                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this [set]*
                int32_t loopcOp = pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < frameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;

                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, frameSize, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_DOT_I:
            // Loop Initialization for the optimized implementation of .*
            //   This op scans through all remaining input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                // Loop through input until the input is exhausted (we reach an end-of-line)
                // In multi-line mode, we can just go straight to the end of the input.
                int32_t ix;
                if (opValue == 1) {
                    // Multi-line mode.
                    ix = inputLen;
                } else {
                    // NOT multi-line mode.  Line endings do not match '.'
                    // Scan forward until a line ending or end of input.
                    ix = fp->fInputIdx;
                    for (;;) {
                        if (ix >= inputLen) {
                            ix = inputLen;
                            break;
                        }
                        UChar32   c;
                        U16_NEXT(inputBuf, ix, inputLen, c);   // c = inputBuf[ix++]
                        if (((c & 0x7f) <= 0x29) &&     
                            ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                            //  char is a line ending.  Put the input pos back to the  
                            //    line ending char, and exit the scanning loop.
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    }
                }
                
                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }

                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this [set]*
                int32_t loopcOp = pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < frameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;

                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, frameSize, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_C:
            {
                U_ASSERT(opValue>=0 && opValue<frameSize);
                int32_t   terminalIdx =  fp->fExtra[opValue];
                U_ASSERT(terminalIdx <= fp->fInputIdx);
                if (terminalIdx == fp->fInputIdx) {
                    // We've backed up the input idx to the point that the loop started.
                    // The loop is done.  Leave here without saving state.   
                    //  Subsequent failures won't come back here.
                    break;
                }
                // Set up for the next iteration of the loop, with input index
                //   backed up by one from the last time through,
                //   and a state save to this instruction in case the following code fails again.
                //   (We're going backwards because this loop emulates stack unwinding, not
                //    the initial scan forward.)
                U_ASSERT(fp->fInputIdx > 0);
                U16_BACK_1(inputBuf, 0, fp->fInputIdx);
                if (inputBuf[fp->fInputIdx] == 0x0a && 
                    fp->fInputIdx > terminalIdx &&
                    inputBuf[fp->fInputIdx-1] == 0x0d) {
                    int32_t prevOp = pat[fp->fPatIdx-2];
                    if (URX_TYPE(prevOp) == URX_LOOP_DOT_I) {
                        // .*, stepping back over CRLF pair.
                        fp->fInputIdx--;
                    }
                }


                fp = StateSave(fp, fp->fPatIdx-1, frameSize, status);
            }
            break;



        default:
            // Trouble.  The compiled pattern contains an entry with an
            //           unrecognized type tag.
            U_ASSERT(FALSE);
        }

        if (U_FAILURE(status)) {
            break;
        }
    }
    
breakFromLoop:
    fMatch = isMatch;
    if (isMatch) {
        fLastMatchEnd = fMatchEnd;
        fMatchStart   = startIdx;
        fMatchEnd     = fp->fInputIdx;
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("Match.  start=%d   end=%d\n\n", fMatchStart, fMatchEnd));
        }
    }
    else
    {
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("No match\n\n"));
        }
    }

    fFrame = fp;                // The active stack frame when the engine stopped.
                                //   Contains the capture group results that we need to
                                //    access later.

    return;
}



UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegexMatcher)

U_NAMESPACE_END

#endif  // !UCONFIG_NO_REGULAR_EXPRESSIONS

