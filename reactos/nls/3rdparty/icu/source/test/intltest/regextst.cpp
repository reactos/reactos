/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   regextst.cpp
//
//      ICU Regular Expressions test, part of intltest.
//

#include "intltest.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uchar.h"
#include "unicode/ucnv.h"
#include "regextst.h"
#include "uvector.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
RegexTest::RegexTest()
{
}


RegexTest::~RegexTest()
{
}



void RegexTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite RegexTest: ");
    switch (index) {

        case 0: name = "Basic";
            if (exec) Basic();
            break;
        case 1: name = "API_Match";
            if (exec) API_Match();
            break;
        case 2: name = "API_Replace";
            if (exec) API_Replace();
            break;
        case 3: name = "API_Pattern";
            if (exec) API_Pattern();
            break;
        case 4: name = "Extended";
            if (exec) Extended();
            break;
        case 5: name = "Errors";
            if (exec) Errors();
            break;
        case 6: name = "PerlTests";
            if (exec) PerlTests();
            break;


        default: name = "";
            break; //needed to end loop
    }
}


//---------------------------------------------------------------------------
//
//   Error Checking / Reporting macros used in all of the tests.
//
//---------------------------------------------------------------------------
#define REGEX_CHECK_STATUS {if (U_FAILURE(status)) {errln("RegexTest failure at line %d.  status=%s\n", \
__LINE__, u_errorName(status)); return;}}

#define REGEX_ASSERT(expr) {if ((expr)==FALSE) {errln("RegexTest failure at line %d.\n", __LINE__);};}

#define REGEX_ASSERT_FAIL(expr, errcode) {UErrorCode status=U_ZERO_ERROR; (expr);\
if (status!=errcode) {errln("RegexTest failure at line %d.  Expected status=%s, got %s\n", \
    __LINE__, u_errorName(errcode), u_errorName(status));};}

#define REGEX_CHECK_STATUS_L(line) {if (U_FAILURE(status)) {errln( \
    "RegexTest failure at line %d, from %d.  status=%d\n",__LINE__, (line), status); }}

#define REGEX_ASSERT_L(expr, line) {if ((expr)==FALSE) { \
    errln("RegexTest failure at line %d, from %d.", __LINE__, (line)); return;}}



//---------------------------------------------------------------------------
//
//    REGEX_TESTLM       Macro + invocation function to simplify writing quick tests
//                       for the LookingAt() and  Match() functions.
//
//       usage:
//          REGEX_TESTLM("pattern",  "input text",  lookingAt expected, matches expected);
//
//          The expected results are UBool - TRUE or FALSE.
//          The input text is unescaped.  The pattern is not.
//
//
//---------------------------------------------------------------------------

#define REGEX_TESTLM(pat, text, looking, match) doRegexLMTest(pat, text, looking, match, __LINE__);

UBool RegexTest::doRegexLMTest(const char *pat, const char *text, UBool looking, UBool match, int32_t line) {
    const UnicodeString pattern(pat);
    const UnicodeString inputText(text);
    UErrorCode          status  = U_ZERO_ERROR;
    UParseError         pe;
    RegexPattern        *REPattern = NULL;
    RegexMatcher        *REMatcher = NULL;
    UBool               retVal     = TRUE;

    UnicodeString patString(pat);
    REPattern = RegexPattern::compile(patString, 0, pe, status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in RegexPattern::compile() at line %d.  Status = %s\n",
            line, u_errorName(status));
        return FALSE;
    }
    if (line==376) { RegexPatternDump(REPattern);}

    UnicodeString inputString(inputText);
    UnicodeString unEscapedInput = inputString.unescape();
    REMatcher = REPattern->matcher(unEscapedInput, status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in REPattern::matcher() at line %d.  Status = %s\n",
            line, u_errorName(status));
        return FALSE;
    }

    UBool actualmatch;
    actualmatch = REMatcher->lookingAt(status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in lookingAt() at line %d.  Status = %s\n",
            line, u_errorName(status));
        retVal =  FALSE;
    }
    if (actualmatch != looking) {
        errln("RegexTest: wrong return from lookingAt() at line %d.\n", line);
        retVal = FALSE;
    }

    status = U_ZERO_ERROR;
    actualmatch = REMatcher->matches(status);
    if (U_FAILURE(status)) {
        errln("RegexTest failure in matches() at line %d.  Status = %s\n",
            line, u_errorName(status));
        retVal = FALSE;
    }
    if (actualmatch != match) {
        errln("RegexTest: wrong return from matches() at line %d.\n", line);
        retVal = FALSE;
    }

    if (retVal == FALSE) {
        RegexPatternDump(REPattern);
    }

    delete REPattern;
    delete REMatcher;
    return retVal;
}




//---------------------------------------------------------------------------
//
//    regex_find(pattern, inputString, lineNumber)
//
//         function to simplify writing tests regex tests.
//
//          The input text is unescaped.  The pattern is not.
//          The input text is marked with the expected match positions
//              <0>text  <1> more text </1>   </0>
//          The <n> </n> tags are removed before trying the match.
//          The tags mark the start and end of the match and of any capture groups.
//
//
//---------------------------------------------------------------------------


//  Set a value into a UVector at position specified by a decimal number in
//   a UnicodeString.   This is a utility function needed by the actual test function,
//   which follows.
static void set(UVector &vec, int32_t val, UnicodeString index) {
    UErrorCode  status=U_ZERO_ERROR;
    int32_t  idx = 0;
    for (int32_t i=0; i<index.length(); i++) {
        int32_t d=u_charDigitValue(index.charAt(i));
        if (d<0) {return;}
        idx = idx*10 + d;
    }
    while (vec.size()<idx+1) {vec.addElement(-1, status);}
    vec.setElementAt(val, idx);
}

void RegexTest::regex_find(const UnicodeString &pattern,
                           const UnicodeString &flags,
                           const UnicodeString &inputString,
                           int32_t line) {
    UnicodeString       unEscapedInput;
    UnicodeString       deTaggedInput;

    UErrorCode          status         = U_ZERO_ERROR;
    UParseError         pe;
    RegexPattern        *parsePat      = NULL;
    RegexMatcher        *parseMatcher  = NULL;
    RegexPattern        *callerPattern = NULL;
    RegexMatcher        *matcher       = NULL;
    UVector             groupStarts(status);
    UVector             groupEnds(status);
    UBool               isMatch        = FALSE;
    UBool               failed         = FALSE;
    int32_t                 numFinds;
    int32_t                 i;

    //
    //  Compile the caller's pattern
    //
    uint32_t bflags = 0;
    if (flags.indexOf((UChar)0x69) >= 0)  { // 'i' flag
        bflags |= UREGEX_CASE_INSENSITIVE;
    }
    if (flags.indexOf((UChar)0x78) >= 0)  { // 'x' flag
        bflags |= UREGEX_COMMENTS;
    }
    if (flags.indexOf((UChar)0x73) >= 0)  { // 's' flag
        bflags |= UREGEX_DOTALL;
    }
    if (flags.indexOf((UChar)0x6d) >= 0)  { // 'm' flag
        bflags |= UREGEX_MULTILINE;
    }


    callerPattern = RegexPattern::compile(pattern, bflags, pe, status);
    if (status != U_ZERO_ERROR) {
        #if UCONFIG_NO_BREAK_ITERATION==1
        // 'v' test flag means that the test pattern should not compile if ICU was configured
        //     to not include break iteration.  RBBI is needed for Unicode word boundaries.
        if (flags.indexOf((UChar)0x76) >= 0 /*'v'*/ && status == U_UNSUPPORTED_ERROR) {
            goto cleanupAndReturn;
        }
        #endif
        errln("Line %d: error %s compiling pattern.", line, u_errorName(status));
        goto cleanupAndReturn;
    }

    if (flags.indexOf((UChar)'d') >= 0) {
        RegexPatternDump(callerPattern);
    }

    //
    // Number of times find() should be called on the test string, default to 1
    //
    numFinds = 1;
    for (i=2; i<=9; i++) {
        if (flags.indexOf((UChar)(0x30 + i)) >= 0) {   // digit flag
            if (numFinds != 1) {
                errln("Line %d: more than one digit flag.  Scanning %d.", line, i);
                goto cleanupAndReturn;
            }
            numFinds = i;
        }
    }

    //
    //  Find the tags in the input data, remove them, and record the group boundary
    //    positions.
    //
    parsePat = RegexPattern::compile("<(/?)([0-9]+)>", 0, pe, status);
    REGEX_CHECK_STATUS_L(line);

    unEscapedInput = inputString.unescape();
    parseMatcher = parsePat->matcher(unEscapedInput, status);
    REGEX_CHECK_STATUS_L(line);
    while(parseMatcher->find()) {
        parseMatcher->appendReplacement(deTaggedInput, "", status);
        REGEX_CHECK_STATUS;
        UnicodeString groupNum = parseMatcher->group(2, status);
        if (parseMatcher->group(1, status) == "/") {
            // close tag
            set(groupEnds, deTaggedInput.length(), groupNum);
        } else {
            set(groupStarts, deTaggedInput.length(), groupNum);
        }
    }
    parseMatcher->appendTail(deTaggedInput);
    REGEX_ASSERT_L(groupStarts.size() == groupEnds.size(), line);


    //
    // Do a find on the de-tagged input using the caller's pattern
    //
    matcher = callerPattern->matcher(deTaggedInput, status);
    REGEX_CHECK_STATUS_L(line);
    if (flags.indexOf((UChar)'t') >= 0) {
        matcher->setTrace(TRUE);
    }

    for (i=0; i<numFinds; i++) {
        isMatch = matcher->find();
    }
    matcher->setTrace(FALSE);

    //
    // Match up the groups from the find() with the groups from the tags
    //

    // number of tags should match number of groups from find operation.
    // matcher->groupCount does not include group 0, the entire match, hence the +1.
    //   G option in test means that capture group data is not available in the
    //     expected results, so the check needs to be suppressed.
    if (isMatch == FALSE && groupStarts.size() != 0) {
        errln("Error at line %d:  Match expected, but none found.\n", line);
        failed = TRUE;
        goto cleanupAndReturn;
    }

    if (flags.indexOf((UChar)0x47 /*G*/) >= 0) {
        // Only check for match / no match.  Don't check capture groups.
        if (isMatch && groupStarts.size() == 0) {
            errln("Error at line %d:  No match expected, but one found.\n", line);
            failed = TRUE;
        }
        goto cleanupAndReturn;
    }

    for (i=0; i<=matcher->groupCount(); i++) {
        int32_t  expectedStart = (i >= groupStarts.size()? -1 : groupStarts.elementAti(i));
        if (matcher->start(i, status) != expectedStart) {
            errln("Error at line %d: incorrect start position for group %d.  Expected %d, got %d",
                line, i, expectedStart, matcher->start(i, status));
            failed = TRUE;
            goto cleanupAndReturn;  // Good chance of subsequent bogus errors.  Stop now.
        }
        int32_t  expectedEnd = (i >= groupEnds.size()? -1 : groupEnds.elementAti(i));
        if (matcher->end(i, status) != expectedEnd) {
            errln("Error at line %d: incorrect end position for group %d.  Expected %d, got %d",
                line, i, expectedEnd, matcher->end(i, status));
            failed = TRUE;
            // Error on end position;  keep going; real error is probably yet to come as group
            //   end positions work from end of the input data towards the front.
        }
    }
    if ( matcher->groupCount()+1 < groupStarts.size()) {
        errln("Error at line %d: Expected %d capture groups, found %d.",
            line, groupStarts.size()-1, matcher->groupCount());
        failed = TRUE;
        }

cleanupAndReturn:
    if (failed) {
        errln((UnicodeString)"\""+pattern+(UnicodeString)"\"  "
            +flags+(UnicodeString)"  \""+inputString+(UnicodeString)"\"");
        // callerPattern->dump();
    }
    delete parseMatcher;
    delete parsePat;
    delete matcher;
    delete callerPattern;
}








//---------------------------------------------------------------------------
//
//    REGEX_ERR       Macro + invocation function to simplify writing tests
//                       regex tests for incorrect patterns
//
//       usage:
//          REGEX_ERR("pattern",   expected error line, column, expected status);
//
//---------------------------------------------------------------------------
#define REGEX_ERR(pat, line, col, status) regex_err(pat, line, col, status, __LINE__);

void RegexTest::regex_err(const char *pat, int32_t errLine, int32_t errCol,
                          UErrorCode expectedStatus, int32_t line) {
    UnicodeString       pattern(pat);

    UErrorCode          status         = U_ZERO_ERROR;
    UParseError         pe;
    RegexPattern        *callerPattern = NULL;

    //
    //  Compile the caller's pattern
    //
    UnicodeString patString(pat);
    callerPattern = RegexPattern::compile(patString, 0, pe, status);
    if (status != expectedStatus) {
        errln("Line %d: unexpected error %s compiling pattern.", line, u_errorName(status));
    } else {
        if (status != U_ZERO_ERROR) {
            if (pe.line != errLine || pe.offset != errCol) {
                errln("Line %d: incorrect line/offset from UParseError.  Expected %d/%d; got %d/%d.\n",
                    line, errLine, errCol, pe.line, pe.offset);
            }
        }
    }

    delete callerPattern;
}



//---------------------------------------------------------------------------
//
//      Basic      Check for basic functionality of regex pattern matching.
//                 Avoid the use of REGEX_FIND test macro, which has
//                 substantial dependencies on basic Regex functionality.
//
//---------------------------------------------------------------------------
void RegexTest::Basic() {


//
// Debug - slide failing test cases early
//
#if 0
    {
        // REGEX_TESTLM("a\N{LATIN SMALL LETTER B}c", "abc", FALSE, FALSE);
        UParseError pe;
        UErrorCode  status = U_ZERO_ERROR;
        RegexPattern::compile("^(?:a?b?)*$", 0, pe, status);
        // REGEX_FIND("(?>(abc{2,4}?))(c*)", "<0>ab<1>cc</1><2>ccc</2></0>ddd");
        // REGEX_FIND("(X([abc=X]+)+X)|(y[abc=]+)", "=XX====================");
    }
    exit(1);
#endif


    //
    // Pattern with parentheses
    //
    REGEX_TESTLM("st(abc)ring", "stabcring thing", TRUE,  FALSE);
    REGEX_TESTLM("st(abc)ring", "stabcring",       TRUE,  TRUE);
    REGEX_TESTLM("st(abc)ring", "stabcrung",       FALSE, FALSE);

    //
    // Patterns with *
    //
    REGEX_TESTLM("st(abc)*ring", "string", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcring", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcring", TRUE, TRUE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcdring", FALSE, FALSE);
    REGEX_TESTLM("st(abc)*ring", "stabcabcabcring etc.", TRUE, FALSE);

    REGEX_TESTLM("a*", "",  TRUE, TRUE);
    REGEX_TESTLM("a*", "b", TRUE, FALSE);


    //
    //  Patterns with "."
    //
    REGEX_TESTLM(".", "abc", TRUE, FALSE);
    REGEX_TESTLM("...", "abc", TRUE, TRUE);
    REGEX_TESTLM("....", "abc", FALSE, FALSE);
    REGEX_TESTLM(".*", "abcxyz123", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcdefghij", FALSE, FALSE);
    REGEX_TESTLM("ab.*xyz", "abcdefg...wxyz", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcde...wxyz...abc..xyz", TRUE, TRUE);
    REGEX_TESTLM("ab.*xyz", "abcde...wxyz...abc..xyz...", TRUE, FALSE);

    //
    //  Patterns with * applied to chars at end of literal string
    //
    REGEX_TESTLM("abc*", "ab", TRUE, TRUE);
    REGEX_TESTLM("abc*", "abccccc", TRUE, TRUE);

    //
    //  Supplemental chars match as single chars, not a pair of surrogates.
    //
    REGEX_TESTLM(".", "\\U00011000", TRUE, TRUE);
    REGEX_TESTLM("...", "\\U00011000x\\U00012002", TRUE, TRUE);
    REGEX_TESTLM("...", "\\U00011000x\\U00012002y", TRUE, FALSE);


    //
    //  UnicodeSets in the pattern
    //
    REGEX_TESTLM("[1-6]", "1", TRUE, TRUE);
    REGEX_TESTLM("[1-6]", "3", TRUE, TRUE);
    REGEX_TESTLM("[1-6]", "7", FALSE, FALSE);
    REGEX_TESTLM("a[1-6]", "a3", TRUE, TRUE);
    REGEX_TESTLM("a[1-6]", "a3", TRUE, TRUE);
    REGEX_TESTLM("a[1-6]b", "a3b", TRUE, TRUE);

    REGEX_TESTLM("a[0-9]*b", "a123b", TRUE, TRUE);
    REGEX_TESTLM("a[0-9]*b", "abc", TRUE, FALSE);
    REGEX_TESTLM("[\\p{Nd}]*", "123456", TRUE, TRUE);
    REGEX_TESTLM("[\\p{Nd}]*", "a123456", TRUE, FALSE);   // note that * matches 0 occurences.
    REGEX_TESTLM("[a][b][[:Zs:]]*", "ab   ", TRUE, TRUE);

    //
    //   OR operator in patterns
    //
    REGEX_TESTLM("(a|b)", "a", TRUE, TRUE);
    REGEX_TESTLM("(a|b)", "b", TRUE, TRUE);
    REGEX_TESTLM("(a|b)", "c", FALSE, FALSE);
    REGEX_TESTLM("a|b", "b", TRUE, TRUE);

    REGEX_TESTLM("(a|b|c)*", "aabcaaccbcabc", TRUE, TRUE);
    REGEX_TESTLM("(a|b|c)*", "aabcaaccbcabdc", TRUE, FALSE);
    REGEX_TESTLM("(a(b|c|d)(x|y|z)*|123)", "ac", TRUE, TRUE);
    REGEX_TESTLM("(a(b|c|d)(x|y|z)*|123)", "123", TRUE, TRUE);
    REGEX_TESTLM("(a|(1|2)*)(b|c|d)(x|y|z)*|123", "123", TRUE, TRUE);
    REGEX_TESTLM("(a|(1|2)*)(b|c|d)(x|y|z)*|123", "222211111czzzzw", TRUE, FALSE);

    //
    //  +
    //
    REGEX_TESTLM("ab+", "abbc", TRUE, FALSE);
    REGEX_TESTLM("ab+c", "ac", FALSE, FALSE);
    REGEX_TESTLM("b+", "", FALSE, FALSE);
    REGEX_TESTLM("(abc|def)+", "defabc", TRUE, TRUE);
    REGEX_TESTLM(".+y", "zippity dooy dah ", TRUE, FALSE);
    REGEX_TESTLM(".+y", "zippity dooy", TRUE, TRUE);

    //
    //   ?
    //
    REGEX_TESTLM("ab?", "ab", TRUE, TRUE);
    REGEX_TESTLM("ab?", "a", TRUE, TRUE);
    REGEX_TESTLM("ab?", "ac", TRUE, FALSE);
    REGEX_TESTLM("ab?", "abb", TRUE, FALSE);
    REGEX_TESTLM("a(b|c)?d", "abd", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "acd", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "ad", TRUE, TRUE);
    REGEX_TESTLM("a(b|c)?d", "abcd", FALSE, FALSE);
    REGEX_TESTLM("a(b|c)?d", "ab", FALSE, FALSE);

    //
    //  Escape sequences that become single literal chars, handled internally
    //   by ICU's Unescape.
    //

    // REGEX_TESTLM("\101\142", "Ab", TRUE, TRUE);      // Octal     TODO: not implemented yet.
    REGEX_TESTLM("\\a", "\\u0007", TRUE, TRUE);        // BEL
    REGEX_TESTLM("\\cL", "\\u000c", TRUE, TRUE);       // Control-L
    REGEX_TESTLM("\\e", "\\u001b", TRUE, TRUE);        // Escape
    REGEX_TESTLM("\\f", "\\u000c", TRUE, TRUE);        // Form Feed
    REGEX_TESTLM("\\n", "\\u000a", TRUE, TRUE);        // new line
    REGEX_TESTLM("\\r", "\\u000d", TRUE, TRUE);        //  CR
    REGEX_TESTLM("\\t", "\\u0009", TRUE, TRUE);        // Tab
    REGEX_TESTLM("\\u1234", "\\u1234", TRUE, TRUE);
    REGEX_TESTLM("\\U00001234", "\\u1234", TRUE, TRUE);

    REGEX_TESTLM(".*\\Ax", "xyz", TRUE, FALSE);  //  \A matches only at the beginning of input
    REGEX_TESTLM(".*\\Ax", " xyz", FALSE, FALSE);  //  \A matches only at the beginning of input

    // Escape of special chars in patterns
    REGEX_TESTLM("\\\\\\|\\(\\)\\[\\{\\~\\$\\*\\+\\?\\.", "\\\\|()[{~$*+?.", TRUE, TRUE);


}


//---------------------------------------------------------------------------
//
//      API_Match   Test that the API for class RegexMatcher
//                  is present and nominally working, but excluding functions
//                  implementing replace operations.
//
//---------------------------------------------------------------------------
void RegexTest::API_Match() {
    UParseError         pe;
    UErrorCode          status=U_ZERO_ERROR;
    int32_t             flags = 0;

    //
    // Debug - slide failing test cases early
    //
#if 0
    {
    }
    return;
#endif

    //
    // Simple pattern compilation
    //
    {
        UnicodeString       re("abc");
        RegexPattern        *pat2;
        pat2 = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;

        UnicodeString inStr1 = "abcdef this is a test";
        UnicodeString instr2 = "not abc";
        UnicodeString empty  = "";


        //
        // Matcher creation and reset.
        //
        RegexMatcher *m1 = pat2->matcher(inStr1, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(m1->lookingAt(status) == TRUE);
        REGEX_ASSERT(m1->input() == inStr1);
        m1->reset(instr2);
        REGEX_ASSERT(m1->lookingAt(status) == FALSE);
        REGEX_ASSERT(m1->input() == instr2);
        m1->reset(inStr1);
        REGEX_ASSERT(m1->input() == inStr1);
        REGEX_ASSERT(m1->lookingAt(status) == TRUE);
        m1->reset(empty);
        REGEX_ASSERT(m1->lookingAt(status) == FALSE);
        REGEX_ASSERT(m1->input() == empty);
        REGEX_ASSERT(&m1->pattern() == pat2);

        //
        //  reset(pos, status)
        //
        m1->reset(inStr1);
        m1->reset(4, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(m1->input() == inStr1);
        REGEX_ASSERT(m1->lookingAt(status) == TRUE);

        m1->reset(-1, status);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);
        status = U_ZERO_ERROR;

        m1->reset(0, status);
        REGEX_CHECK_STATUS;
        status = U_ZERO_ERROR;

        int32_t len = m1->input().length();
        m1->reset(len-1, status);
        REGEX_CHECK_STATUS;
        status = U_ZERO_ERROR;

        m1->reset(len, status);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);
        status = U_ZERO_ERROR;

        //
        // match(pos, status)
        //
        m1->reset(instr2);
        REGEX_ASSERT(m1->matches(4, status) == TRUE);
        m1->reset();
        REGEX_ASSERT(m1->matches(3, status) == FALSE);
        m1->reset();
        REGEX_ASSERT(m1->matches(5, status) == FALSE);
        REGEX_ASSERT(m1->matches(4, status) == TRUE);
        REGEX_ASSERT(m1->matches(-1, status) == FALSE);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);

        // Match() at end of string should fail, but should not
        //  be an error.
        status = U_ZERO_ERROR;
        len = m1->input().length();
        REGEX_ASSERT(m1->matches(len, status) == FALSE);
        REGEX_CHECK_STATUS;

        // Match beyond end of string should fail with an error.
        status = U_ZERO_ERROR;
        REGEX_ASSERT(m1->matches(len+1, status) == FALSE);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);

        // Successful match at end of string.
        {
            status = U_ZERO_ERROR;
            RegexMatcher m("A?", 0, status);  // will match zero length string.
            REGEX_CHECK_STATUS;
            m.reset(inStr1);
            len = inStr1.length();
            REGEX_ASSERT(m.matches(len, status) == TRUE);
            REGEX_CHECK_STATUS;
            m.reset(empty);
            REGEX_ASSERT(m.matches(0, status) == TRUE);
            REGEX_CHECK_STATUS;
        }


        //
        // lookingAt(pos, status)
        //
        status = U_ZERO_ERROR;
        m1->reset(instr2);  // "not abc"
        REGEX_ASSERT(m1->lookingAt(4, status) == TRUE);
        REGEX_ASSERT(m1->lookingAt(5, status) == FALSE);
        REGEX_ASSERT(m1->lookingAt(3, status) == FALSE);
        REGEX_ASSERT(m1->lookingAt(4, status) == TRUE);
        REGEX_ASSERT(m1->lookingAt(-1, status) == FALSE);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);
        status = U_ZERO_ERROR;
        len = m1->input().length();
        REGEX_ASSERT(m1->lookingAt(len, status) == FALSE);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(m1->lookingAt(len+1, status) == FALSE);
        REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);

        delete m1;
        delete pat2;
    }


    //
    // Capture Group.
    //     RegexMatcher::start();
    //     RegexMatcher::end();
    //     RegexMatcher::groupCount();
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re("01(23(45)67)(.*)");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = "0123456789";

        RegexMatcher *matcher = pat->matcher(data, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(matcher->lookingAt(status) == TRUE);
        static const int32_t matchStarts[] = {0,  2, 4, 8};
        static const int32_t matchEnds[]   = {10, 8, 6, 10};
        int32_t i;
        for (i=0; i<4; i++) {
            int32_t actualStart = matcher->start(i, status);
            REGEX_CHECK_STATUS;
            if (actualStart != matchStarts[i]) {
                errln("RegexTest failure at line %d, index %d.  Expected %d, got %d\n",
                    __LINE__, i, matchStarts[i], actualStart);
            }
            int32_t actualEnd = matcher->end(i, status);
            REGEX_CHECK_STATUS;
            if (actualEnd != matchEnds[i]) {
                errln("RegexTest failure at line %d index %d.  Expected %d, got %d\n",
                    __LINE__, i, matchEnds[i], actualEnd);
            }
        }

        REGEX_ASSERT(matcher->start(0, status) == matcher->start(status));
        REGEX_ASSERT(matcher->end(0, status) == matcher->end(status));

        REGEX_ASSERT_FAIL(matcher->start(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        REGEX_ASSERT_FAIL(matcher->start( 4, status), U_INDEX_OUTOFBOUNDS_ERROR);
        matcher->reset();
        REGEX_ASSERT_FAIL(matcher->start( 0, status), U_REGEX_INVALID_STATE);

        matcher->lookingAt(status);
        REGEX_ASSERT(matcher->group(status)    == "0123456789");
        REGEX_ASSERT(matcher->group(0, status) == "0123456789");
        REGEX_ASSERT(matcher->group(1, status) == "234567"    );
        REGEX_ASSERT(matcher->group(2, status) == "45"        );
        REGEX_ASSERT(matcher->group(3, status) == "89"        );
        REGEX_CHECK_STATUS;
        REGEX_ASSERT_FAIL(matcher->group(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        REGEX_ASSERT_FAIL(matcher->group( 4, status), U_INDEX_OUTOFBOUNDS_ERROR);
        matcher->reset();
        REGEX_ASSERT_FAIL(matcher->group( 0, status), U_REGEX_INVALID_STATE);

        delete matcher;
        delete pat;

    }

    //
    //  find
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re("abc");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = ".abc..abc...abc..";
        //                    012345678901234567

        RegexMatcher *matcher = pat->matcher(data, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 6);
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 12);
        REGEX_ASSERT(matcher->find() == FALSE);
        REGEX_ASSERT(matcher->find() == FALSE);

        matcher->reset();
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 1);

        REGEX_ASSERT(matcher->find(0, status));
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find(1, status));
        REGEX_ASSERT(matcher->start(status) == 1);
        REGEX_ASSERT(matcher->find(2, status));
        REGEX_ASSERT(matcher->start(status) == 6);
        REGEX_ASSERT(matcher->find(12, status));
        REGEX_ASSERT(matcher->start(status) == 12);
        REGEX_ASSERT(matcher->find(13, status) == FALSE);
        REGEX_ASSERT(matcher->find(16, status) == FALSE);
        REGEX_ASSERT(matcher->find(17, status) == FALSE);
        REGEX_ASSERT_FAIL(matcher->start(status), U_REGEX_INVALID_STATE);

        status = U_ZERO_ERROR;
        REGEX_ASSERT_FAIL(matcher->find(-1, status), U_INDEX_OUTOFBOUNDS_ERROR);
        status = U_ZERO_ERROR;
        REGEX_ASSERT_FAIL(matcher->find(18, status), U_INDEX_OUTOFBOUNDS_ERROR);

        REGEX_ASSERT(matcher->groupCount() == 0);

        delete matcher;
        delete pat;
    }


    //
    //  find, with \G in pattern (true if at the end of a previous match).
    //
    {
        int32_t             flags=0;
        UParseError         pe;
        UErrorCode          status=U_ZERO_ERROR;

        UnicodeString       re(".*?(?:(\\Gabc)|(abc))");
        RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
        REGEX_CHECK_STATUS;
        UnicodeString data = ".abcabc.abc..";
        //                    012345678901234567

        RegexMatcher *matcher = pat->matcher(data, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 0);
        REGEX_ASSERT(matcher->start(1, status) == -1);
        REGEX_ASSERT(matcher->start(2, status) == 1);

        REGEX_ASSERT(matcher->find());
        REGEX_ASSERT(matcher->start(status) == 4);
        REGEX_ASSERT(matcher->start(1, status) == 4);
        REGEX_ASSERT(matcher->start(2, status) == -1);
        REGEX_CHECK_STATUS;

        delete matcher;
        delete pat;
    }

    //
    //   find with zero length matches, match position should bump ahead
    //     to prevent loops.
    //
    {
        int32_t                 i;
        UErrorCode          status=U_ZERO_ERROR;
        RegexMatcher        m("(?= ?)", 0, status);   // This pattern will zero-length matches anywhere,
                                                      //   using an always-true look-ahead.
        REGEX_CHECK_STATUS;
        UnicodeString s("    ");
        m.reset(s);
        for (i=0; ; i++) {
            if (m.find() == FALSE) {
                break;
            }
            REGEX_ASSERT(m.start(status) == i);
            REGEX_ASSERT(m.end(status) == i);
        }
        REGEX_ASSERT(i==5);

        // Check that the bump goes over surrogate pairs OK
        s = "\\U00010001\\U00010002\\U00010003\\U00010004";
        s = s.unescape();
        m.reset(s);
        for (i=0; ; i+=2) {
            if (m.find() == FALSE) {
                break;
            }
            REGEX_ASSERT(m.start(status) == i);
            REGEX_ASSERT(m.end(status) == i);
        }
        REGEX_ASSERT(i==10);
    }
    {
        // find() loop breaking test.
        //        with pattern of /.?/, should see a series of one char matches, then a single
        //        match of zero length at the end of the input string.
        int32_t                 i;
        UErrorCode          status=U_ZERO_ERROR;
        RegexMatcher        m(".?", 0, status);
        REGEX_CHECK_STATUS;
        UnicodeString s("    ");
        m.reset(s);
        for (i=0; ; i++) {
            if (m.find() == FALSE) {
                break;
            }
            REGEX_ASSERT(m.start(status) == i);
            REGEX_ASSERT(m.end(status) == (i<4 ? i+1 : i));
        }
        REGEX_ASSERT(i==5);
    }


    //
    // Matchers with no input string behave as if they had an empty input string.
    //

    {
        UErrorCode status = U_ZERO_ERROR;
        RegexMatcher  m(".?", 0, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(m.find());
        REGEX_ASSERT(m.start(status) == 0);
        REGEX_ASSERT(m.input() == "");
    }
    {
        UErrorCode status = U_ZERO_ERROR;
        RegexPattern  *p = RegexPattern::compile(".", 0, status);
        RegexMatcher  *m = p->matcher(status);
        REGEX_CHECK_STATUS;

        REGEX_ASSERT(m->find() == FALSE);
        REGEX_ASSERT(m->input() == "");
        delete m;
        delete p;
    }

    //
    // Compilation error on reset with UChar *
    //   These were a hazard that people were stumbling over with runtime errors.
    //   Changed them to compiler errors by adding private methods that more closely
    //   matched the incorrect use of the functions.
    //
#if 0
    {
        UErrorCode status = U_ZERO_ERROR;
        UChar ucharString[20];
        RegexMatcher m(".", 0, status);
        m.reset(ucharString);  // should not compile.

        RegexPattern *p = RegexPattern::compile(".", 0, status);
        RegexMatcher *m2 = p->matcher(ucharString, status);    //  should not compile.

        RegexMatcher m3(".", ucharString, 0, status);  //  Should not compile
    }
#endif

}






//---------------------------------------------------------------------------
//
//      API_Replace        API test for class RegexMatcher, testing the
//                         Replace family of functions.
//
//---------------------------------------------------------------------------
void RegexTest::API_Replace() {
    //
    //  Replace
    //
    int32_t             flags=0;
    UParseError         pe;
    UErrorCode          status=U_ZERO_ERROR;

    UnicodeString       re("abc");
    RegexPattern *pat = RegexPattern::compile(re, flags, pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString data = ".abc..abc...abc..";
    //                    012345678901234567
    RegexMatcher *matcher = pat->matcher(data, status);

    //
    //  Plain vanilla matches.
    //
    UnicodeString  dest;
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".yz..abc...abc..");

    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".yz..yz...yz..");

    //
    //  Plain vanilla non-matches.
    //
    UnicodeString d2 = ".abx..abx...abx..";
    matcher->reset(d2);
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".abx..abx...abx..");

    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == ".abx..abx...abx..");

    //
    // Empty source string
    //
    UnicodeString d3 = "";
    matcher->reset(d3);
    dest = matcher->replaceFirst("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "");

    dest = matcher->replaceAll("yz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "");

    //
    // Empty substitution string
    //
    matcher->reset(data);              // ".abc..abc...abc.."
    dest = matcher->replaceFirst("", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "...abc...abc..");

    dest = matcher->replaceAll("", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "........");

    //
    // match whole string
    //
    UnicodeString d4 = "abc";
    matcher->reset(d4);
    dest = matcher->replaceFirst("xyz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "xyz");

    dest = matcher->replaceAll("xyz", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "xyz");

    //
    // Capture Group, simple case
    //
    UnicodeString       re2("a(..)");
    RegexPattern *pat2 = RegexPattern::compile(re2, flags, pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString d5 = "abcdefg";
    RegexMatcher *matcher2 = pat2->matcher(d5, status);
    REGEX_CHECK_STATUS;
    dest = matcher2->replaceFirst("$1$1", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "bcbcdefg");

    dest = matcher2->replaceFirst("The value of \\$1 is $1.", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "The value of $1 is bc.defg");

    dest = matcher2->replaceFirst("$ by itself, no group number $$$", status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "$ by itself, no group number $$$defg");

    UnicodeString replacement = "Supplemental Digit 1 $\\U0001D7CF.";
    replacement = replacement.unescape();
    dest = matcher2->replaceFirst(replacement, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(dest == "Supplemental Digit 1 bc.defg");

    REGEX_ASSERT_FAIL(matcher2->replaceFirst("bad capture group number $5...",status), U_INDEX_OUTOFBOUNDS_ERROR);


    //
    // Replacement String with \u hex escapes
    //
    {
        UnicodeString  src = "abc 1 abc 2 abc 3";
        UnicodeString  substitute = "--\\u0043--";
        matcher->reset(src);
        UnicodeString  result = matcher->replaceAll(substitute, status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(result == "--C-- 1 --C-- 2 --C-- 3");
    }
    {
        UnicodeString  src = "abc !";
        UnicodeString  substitute = "--\\U00010000--";
        matcher->reset(src);
        UnicodeString  result = matcher->replaceAll(substitute, status);
        REGEX_CHECK_STATUS;
        UnicodeString expected = UnicodeString("--");
        expected.append((UChar32)0x10000);
        expected.append("-- !");
        REGEX_ASSERT(result == expected);
    }
    // TODO:  need more through testing of capture substitutions.

    // Bug 4057
    //
    {
        status = U_ZERO_ERROR;
        UnicodeString s = "The matches start with ss and end with ee ss stuff ee fin";
        RegexMatcher m("ss(.*?)ee", 0, status);
        REGEX_CHECK_STATUS;
        UnicodeString result;

        // Multiple finds do NOT bump up the previous appendReplacement postion.
        m.reset(s);
        m.find();
        m.find();
        m.appendReplacement(result, "ooh", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(result == "The matches start with ss and end with ee ooh");

        // After a reset into the interior of a string, appendReplacemnt still starts at beginning.
        status = U_ZERO_ERROR;
        result.truncate(0);
        m.reset(10, status);
        m.find();
        m.find();
        m.appendReplacement(result, "ooh", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(result == "The matches start with ss and end with ee ooh");

        // find() at interior of string, appendReplacemnt still starts at beginning.
        status = U_ZERO_ERROR;
        result.truncate(0);
        m.reset();
        m.find(10, status);
        m.find();
        m.appendReplacement(result, "ooh", status);
        REGEX_CHECK_STATUS;
        REGEX_ASSERT(result == "The matches start with ss and end with ee ooh");

        m.appendTail(result);
        REGEX_ASSERT(result == "The matches start with ss and end with ee ooh fin");

    }

    delete matcher2;
    delete pat2;
    delete matcher;
    delete pat;
}


//---------------------------------------------------------------------------
//
//      API_Pattern       Test that the API for class RegexPattern is
//                        present and nominally working.
//
//---------------------------------------------------------------------------
void RegexTest::API_Pattern() {
    RegexPattern        pata;    // Test default constructor to not crash.
    RegexPattern        patb;

    REGEX_ASSERT(pata == patb);
    REGEX_ASSERT(pata == pata);

    UnicodeString re1("abc[a-l][m-z]");
    UnicodeString re2("def");
    UErrorCode    status = U_ZERO_ERROR;
    UParseError   pe;

    RegexPattern        *pat1 = RegexPattern::compile(re1, 0, pe, status);
    RegexPattern        *pat2 = RegexPattern::compile(re2, 0, pe, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(*pat1 == *pat1);
    REGEX_ASSERT(*pat1 != pata);

    // Assign
    patb = *pat1;
    REGEX_ASSERT(patb == *pat1);

    // Copy Construct
    RegexPattern patc(*pat1);
    REGEX_ASSERT(patc == *pat1);
    REGEX_ASSERT(patb == patc);
    REGEX_ASSERT(pat1 != pat2);
    patb = *pat2;
    REGEX_ASSERT(patb != patc);
    REGEX_ASSERT(patb == *pat2);

    // Compile with no flags.
    RegexPattern         *pat1a = RegexPattern::compile(re1, pe, status);
    REGEX_ASSERT(*pat1a == *pat1);

    REGEX_ASSERT(pat1a->flags() == 0);

    // Compile with different flags should be not equal
    RegexPattern        *pat1b = RegexPattern::compile(re1, UREGEX_CASE_INSENSITIVE, pe, status);
    REGEX_CHECK_STATUS;

    REGEX_ASSERT(*pat1b != *pat1a);
    REGEX_ASSERT(pat1b->flags() == UREGEX_CASE_INSENSITIVE);
    REGEX_ASSERT(pat1a->flags() == 0);
    delete pat1b;

    // clone
    RegexPattern *pat1c = pat1->clone();
    REGEX_ASSERT(*pat1c == *pat1);
    REGEX_ASSERT(*pat1c != *pat2);

    delete pat1c;
    delete pat1a;
    delete pat1;
    delete pat2;


    //
    //   Verify that a matcher created from a cloned pattern works.
    //     (Jitterbug 3423)
    //
    {
        UErrorCode     status     = U_ZERO_ERROR;
        RegexPattern  *pSource    = RegexPattern::compile("\\p{L}+", 0, status);
        RegexPattern  *pClone     = pSource->clone();
        delete         pSource;
        RegexMatcher  *mFromClone = pClone->matcher(status);
        REGEX_CHECK_STATUS;
        UnicodeString s = "Hello World";
        mFromClone->reset(s);
        REGEX_ASSERT(mFromClone->find() == TRUE);
        REGEX_ASSERT(mFromClone->group(status) == "Hello");
        REGEX_ASSERT(mFromClone->find() == TRUE);
        REGEX_ASSERT(mFromClone->group(status) == "World");
        REGEX_ASSERT(mFromClone->find() == FALSE);
        delete mFromClone;
        delete pClone;
    }

    //
    //   matches convenience API
    //
    REGEX_ASSERT(RegexPattern::matches(".*", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches("abc", "random input", pe, status) == FALSE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches(".*nput", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches("random input", "random input", pe, status) == TRUE);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(RegexPattern::matches(".*u", "random input", pe, status) == FALSE);
    REGEX_CHECK_STATUS;
    status = U_INDEX_OUTOFBOUNDS_ERROR;
    REGEX_ASSERT(RegexPattern::matches("abc", "abc", pe, status) == FALSE);
    REGEX_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);


    //
    // Split()
    //
    status = U_ZERO_ERROR;
    pat1 = RegexPattern::compile(" +",  pe, status);
    REGEX_CHECK_STATUS;
    UnicodeString  fields[10];

    int32_t n;
    n = pat1->split("Now is the time", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==4);
    REGEX_ASSERT(fields[0]=="Now");
    REGEX_ASSERT(fields[1]=="is");
    REGEX_ASSERT(fields[2]=="the");
    REGEX_ASSERT(fields[3]=="time");
    REGEX_ASSERT(fields[4]=="");

    n = pat1->split("Now is the time", fields, 2, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==2);
    REGEX_ASSERT(fields[0]=="Now");
    REGEX_ASSERT(fields[1]=="is the time");
    REGEX_ASSERT(fields[2]=="the");   // left over from previous test

    fields[1] = "*";
    status = U_ZERO_ERROR;
    n = pat1->split("Now is the time", fields, 1, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==1);
    REGEX_ASSERT(fields[0]=="Now is the time");
    REGEX_ASSERT(fields[1]=="*");
    status = U_ZERO_ERROR;

    n = pat1->split("    Now       is the time   ", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==5);
    REGEX_ASSERT(fields[0]=="");
    REGEX_ASSERT(fields[1]=="Now");
    REGEX_ASSERT(fields[2]=="is");
    REGEX_ASSERT(fields[3]=="the");
    REGEX_ASSERT(fields[4]=="time");
    REGEX_ASSERT(fields[5]=="");

    n = pat1->split("     ", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==1);
    REGEX_ASSERT(fields[0]=="");

    fields[0] = "foo";
    n = pat1->split("", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==0);
    REGEX_ASSERT(fields[0]=="foo");

    delete pat1;

    //  split, with a pattern with (capture)
    pat1 = RegexPattern::compile("<(\\w*)>",  pe, status);
    REGEX_CHECK_STATUS;

    status = U_ZERO_ERROR;
    n = pat1->split("<a>Now is <b>the time<c>", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==6);
    REGEX_ASSERT(fields[0]=="");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="b");
    REGEX_ASSERT(fields[4]=="the time");
    REGEX_ASSERT(fields[5]=="c");
    REGEX_ASSERT(fields[6]=="");
    REGEX_ASSERT(status==U_ZERO_ERROR);

    n = pat1->split("  <a>Now is <b>the time<c>", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==6);
    REGEX_ASSERT(fields[0]=="  ");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="b");
    REGEX_ASSERT(fields[4]=="the time");
    REGEX_ASSERT(fields[5]=="c");
    REGEX_ASSERT(fields[6]=="");

    status = U_ZERO_ERROR;
    fields[6] = "foo";
    n = pat1->split("  <a>Now is <b>the time<c>", fields, 6, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==6);
    REGEX_ASSERT(fields[0]=="  ");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="b");
    REGEX_ASSERT(fields[4]=="the time");
    REGEX_ASSERT(fields[5]=="c");
    REGEX_ASSERT(fields[6]=="foo");

    status = U_ZERO_ERROR;
    fields[5] = "foo";
    n = pat1->split("  <a>Now is <b>the time<c>", fields, 5, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==5);
    REGEX_ASSERT(fields[0]=="  ");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="b");
    REGEX_ASSERT(fields[4]=="the time<c>");
    REGEX_ASSERT(fields[5]=="foo");

    status = U_ZERO_ERROR;
    fields[5] = "foo";
    n = pat1->split("  <a>Now is <b>the time", fields, 5, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==5);
    REGEX_ASSERT(fields[0]=="  ");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="b");
    REGEX_ASSERT(fields[4]=="the time");
    REGEX_ASSERT(fields[5]=="foo");

    status = U_ZERO_ERROR;
    n = pat1->split("  <a>Now is <b>the time<c>", fields, 4, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==4);
    REGEX_ASSERT(fields[0]=="  ");
    REGEX_ASSERT(fields[1]=="a");
    REGEX_ASSERT(fields[2]=="Now is ");
    REGEX_ASSERT(fields[3]=="the time<c>");
    status = U_ZERO_ERROR;
    delete pat1;

    pat1 = RegexPattern::compile("([-,])",  pe, status);
    REGEX_CHECK_STATUS;
    n = pat1->split("1-10,20", fields, 10, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(n==5);
    REGEX_ASSERT(fields[0]=="1");
    REGEX_ASSERT(fields[1]=="-");
    REGEX_ASSERT(fields[2]=="10");
    REGEX_ASSERT(fields[3]==",");
    REGEX_ASSERT(fields[4]=="20");
    delete pat1;


    //
    // RegexPattern::pattern()
    //
    pat1 = new RegexPattern();
    REGEX_ASSERT(pat1->pattern() == "");
    delete pat1;

    pat1 = RegexPattern::compile("(Hello, world)*",  pe, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(pat1->pattern() == "(Hello, world)*");
    delete pat1;


    //
    // classID functions
    //
    pat1 = RegexPattern::compile("(Hello, world)*",  pe, status);
    REGEX_CHECK_STATUS;
    REGEX_ASSERT(pat1->getDynamicClassID() == RegexPattern::getStaticClassID());
    REGEX_ASSERT(pat1->getDynamicClassID() != NULL);
    UnicodeString Hello("Hello, world.");
    RegexMatcher *m = pat1->matcher(Hello, status);
    REGEX_ASSERT(pat1->getDynamicClassID() != m->getDynamicClassID());
    REGEX_ASSERT(m->getDynamicClassID() == RegexMatcher::getStaticClassID());
    REGEX_ASSERT(m->getDynamicClassID() != NULL);
    delete m;
    delete pat1;

}

//---------------------------------------------------------------------------
//
//      Extended       A more thorough check for features of regex patterns
//                     The test cases are in a separate data file,
//                       source/tests/testdata/regextst.txt
//                     A description of the test data format is included in that file.
//
//---------------------------------------------------------------------------

const char *
RegexTest::getPath(char buffer[2048], const char *filename) {
    UErrorCode status=U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);
    if (U_FAILURE(status)) {
        errln("ERROR: loadTestData() failed - %s", u_errorName(status));
        return NULL;
    }

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}

void RegexTest::Extended() {
    char tdd[2048];
    const char *srcPath;
    UErrorCode  status  = U_ZERO_ERROR;
    int32_t     lineNum = 0;

    //
    //  Open and read the test data file.
    //
    srcPath=getPath(tdd, "regextst.txt");
    if(srcPath==NULL) {
        return; /* something went wrong, error already output */
    }

    int32_t    len;
    UChar *testData = ReadAndConvertFile(srcPath, len, status);
    if (U_FAILURE(status)) {
        return; /* something went wrong, error already output */
    }

    //
    //  Put the test data into a UnicodeString
    //
    UnicodeString testString(FALSE, testData, len);

    RegexMatcher    quotedStuffMat("\\s*([\\'\\\"/])(.*?)\\1", 0, status);
    RegexMatcher    commentMat    ("\\s*(#.*)?$", 0, status);
    RegexMatcher    flagsMat      ("\\s*([ixsmdtGv2-9]*)([:letter:]*)", 0, status);

    RegexMatcher    lineMat("(.*?)\\r?\\n", testString, 0, status);
    UnicodeString   testPattern;   // The pattern for test from the test file.
    UnicodeString   testFlags;     // the flags   for a test.
    UnicodeString   matchString;   // The marked up string to be used as input

    if (U_FAILURE(status)){
        dataerrln("Construct RegexMatcher() error.");
        delete [] testData;
        return;
    }

    //
    //  Loop over the test data file, once per line.
    //
    while (lineMat.find()) {
        lineNum++;
        if (U_FAILURE(status)) {
            errln("line %d: ICU Error \"%s\"", lineNum, u_errorName(status));
        }

        status = U_ZERO_ERROR;
        UnicodeString testLine = lineMat.group(1, status);
        if (testLine.length() == 0) {
            continue;
        }

        //
        // Parse the test line.  Skip blank and comment only lines.
        // Separate out the three main fields - pattern, flags, target.
        //

        commentMat.reset(testLine);
        if (commentMat.lookingAt(status)) {
            // This line is a comment, or blank.
            continue;
        }

        //
        //  Pull out the pattern field, remove it from the test file line.
        //
        quotedStuffMat.reset(testLine);
        if (quotedStuffMat.lookingAt(status)) {
            testPattern = quotedStuffMat.group(2, status);
            testLine.remove(0, quotedStuffMat.end(0, status));
        } else {
            errln("Bad pattern (missing quotes?) at test file line %d", lineNum);
            continue;
        }


        //
        //  Pull out the flags from the test file line.
        //
        flagsMat.reset(testLine);
        flagsMat.lookingAt(status);                  // Will always match, possibly an empty string.
        testFlags = flagsMat.group(1, status);
        if (flagsMat.group(2, status).length() > 0) {
            errln("Bad Match flag at line %d. Scanning %c\n",
                lineNum, flagsMat.group(2, status).charAt(0));
            continue;
        }
        testLine.remove(0, flagsMat.end(0, status));

        //
        //  Pull out the match string, as a whole.
        //    We'll process the <tags> later.
        //
        quotedStuffMat.reset(testLine);
        if (quotedStuffMat.lookingAt(status)) {
            matchString = quotedStuffMat.group(2, status);
            testLine.remove(0, quotedStuffMat.end(0, status));
        } else {
            errln("Bad match string at test file line %d", lineNum);
            continue;
        }

        //
        //  The only thing left from the input line should be an optional trailing comment.
        //
        commentMat.reset(testLine);
        if (commentMat.lookingAt(status) == FALSE) {
            errln("Line %d: unexpected characters at end of test line.", lineNum);
            continue;
        }

        //
        //  Run the test
        //
        regex_find(testPattern, testFlags, matchString, lineNum);
    }

    delete [] testData;

}



//---------------------------------------------------------------------------
//
//      Errors     Check for error handling in patterns.
//
//---------------------------------------------------------------------------
void RegexTest::Errors() {
    // \escape sequences that aren't implemented yet.
    //REGEX_ERR("hex format \\x{abcd} not implemented", 1, 13, U_REGEX_UNIMPLEMENTED);

    // Missing close parentheses
    REGEX_ERR("Comment (?# with no close", 1, 25, U_REGEX_MISMATCHED_PAREN);
    REGEX_ERR("Capturing Parenthesis(...", 1, 25, U_REGEX_MISMATCHED_PAREN);
    REGEX_ERR("Grouping only parens (?: blah blah", 1, 34, U_REGEX_MISMATCHED_PAREN);

    // Extra close paren
    REGEX_ERR("Grouping only parens (?: blah)) blah", 1, 31, U_REGEX_MISMATCHED_PAREN);
    REGEX_ERR(")))))))", 1, 1, U_REGEX_MISMATCHED_PAREN);
    REGEX_ERR("(((((((", 1, 7, U_REGEX_MISMATCHED_PAREN);

    // Look-ahead, Look-behind
    //  TODO:  add tests for unbounded length look-behinds.
    REGEX_ERR("abc(?<@xyz).*", 1, 7, U_REGEX_RULE_SYNTAX);       // illegal construct

    // Attempt to use non-default flags
    {
        UParseError   pe;
        UErrorCode    status = U_ZERO_ERROR;
        int32_t       flags  = UREGEX_CANON_EQ |
                               UREGEX_COMMENTS         | UREGEX_DOTALL   |
                               UREGEX_MULTILINE;
        RegexPattern *pat1= RegexPattern::compile(".*", flags, pe, status);
        REGEX_ASSERT(status == U_REGEX_UNIMPLEMENTED);
        delete pat1;
    }


    // Quantifiers are allowed only after something that can be quantified.
    REGEX_ERR("+", 1, 1, U_REGEX_RULE_SYNTAX);
    REGEX_ERR("abc\ndef(*2)", 2, 5, U_REGEX_RULE_SYNTAX);
    REGEX_ERR("abc**", 1, 5, U_REGEX_RULE_SYNTAX);

    // Mal-formed {min,max} quantifiers
    REGEX_ERR("abc{a,2}",1,5, U_REGEX_BAD_INTERVAL);
    REGEX_ERR("abc{4,2}",1,8, U_REGEX_MAX_LT_MIN);
    REGEX_ERR("abc{1,b}",1,7, U_REGEX_BAD_INTERVAL);
    REGEX_ERR("abc{1,,2}",1,7, U_REGEX_BAD_INTERVAL);
    REGEX_ERR("abc{1,2a}",1,8, U_REGEX_BAD_INTERVAL);
    REGEX_ERR("abc{222222222222222222222}",1,14, U_REGEX_NUMBER_TOO_BIG);
    REGEX_ERR("abc{5,50000000000}", 1, 17, U_REGEX_NUMBER_TOO_BIG);        // Overflows int during scan
    REGEX_ERR("abc{5,687865858}", 1, 16, U_REGEX_NUMBER_TOO_BIG);          // Overflows regex binary format
    REGEX_ERR("abc{687865858,687865859}", 1, 24, U_REGEX_NUMBER_TOO_BIG);


    // UnicodeSet containing a string
    REGEX_ERR("abc[{def}]xyz", 1, 10, U_REGEX_SET_CONTAINS_STRING);

    // Ticket 5389
    REGEX_ERR("*c", 1, 1, U_REGEX_RULE_SYNTAX);

}


//-------------------------------------------------------------------------------
//
//  Read a text data file, convert it to UChars, and return the data
//    in one big UChar * buffer, which the caller must delete.
//
//--------------------------------------------------------------------------------
UChar *RegexTest::ReadAndConvertFile(const char *fileName, int32_t &ulen, UErrorCode &status) {
    UChar       *retPtr  = NULL;
    char        *fileBuf = NULL;
    UConverter* conv     = NULL;
    FILE        *f       = NULL;

    ulen = 0;
    if (U_FAILURE(status)) {
        return retPtr;
    }

    //
    //  Open the file.
    //
    f = fopen(fileName, "rb");
    if (f == 0) {
        errln("Error opening test data file %s\n", fileName);
        status = U_FILE_ACCESS_ERROR;
        return NULL;
    }
    //
    //  Read it in
    //
    int32_t            fileSize;
    int32_t            amt_read;

    fseek( f, 0, SEEK_END);
    fileSize = ftell(f);
    fileBuf = new char[fileSize];
    fseek(f, 0, SEEK_SET);
    amt_read = fread(fileBuf, 1, fileSize, f);
    if (amt_read != fileSize || fileSize <= 0) {
        errln("Error reading test data file.");
        goto cleanUpAndReturn;
    }

    //
    // Look for a Unicode Signature (BOM) on the data just read
    //
    int32_t        signatureLength;
    const char *   fileBufC;
    const char*    encoding;

    fileBufC = fileBuf;
    encoding = ucnv_detectUnicodeSignature(
        fileBuf, fileSize, &signatureLength, &status);
    if(encoding!=NULL ){
        fileBufC  += signatureLength;
        fileSize  -= signatureLength;
    }

    //
    // Open a converter to take the rule file to UTF-16
    //
    conv = ucnv_open(encoding, &status);
    if (U_FAILURE(status)) {
        goto cleanUpAndReturn;
    }

    //
    // Convert the rules to UChar.
    //  Preflight first to determine required buffer size.
    //
    ulen = ucnv_toUChars(conv,
        NULL,           //  dest,
        0,              //  destCapacity,
        fileBufC,
        fileSize,
        &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        // Buffer Overflow is expected from the preflight operation.
        status = U_ZERO_ERROR;

        retPtr = new UChar[ulen+1];
        ucnv_toUChars(conv,
            retPtr,       //  dest,
            ulen+1,
            fileBufC,
            fileSize,
            &status);
    }

cleanUpAndReturn:
    fclose(f);
    delete[] fileBuf;
    ucnv_close(conv);
    if (U_FAILURE(status)) {
        errln("ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
        delete retPtr;
        retPtr = 0;
        ulen   = 0;
    };
    return retPtr;
}


//-------------------------------------------------------------------------------
//
//   PerlTests  - Run Perl's regular expression tests
//                The input file for this test is re_tests, the standard regular
//                expression test data distributed with the Perl source code.
//
//                Here is Perl's description of the test data file:
//
//        # The tests are in a separate file 't/op/re_tests'.
//        # Each line in that file is a separate test.
//        # There are five columns, separated by tabs.
//        #
//        # Column 1 contains the pattern, optionally enclosed in C<''>.
//        # Modifiers can be put after the closing C<'>.
//        #
//        # Column 2 contains the string to be matched.
//        #
//        # Column 3 contains the expected result:
//        #     y   expect a match
//        #     n   expect no match
//        #     c   expect an error
//        # B   test exposes a known bug in Perl, should be skipped
//        # b   test exposes a known bug in Perl, should be skipped if noamp
//        #
//        # Columns 4 and 5 are used only if column 3 contains C<y> or C<c>.
//        #
//        # Column 4 contains a string, usually C<$&>.
//        #
//        # Column 5 contains the expected result of double-quote
//        # interpolating that string after the match, or start of error message.
//        #
//        # Column 6, if present, contains a reason why the test is skipped.
//        # This is printed with "skipped", for harness to pick up.
//        #
//        # \n in the tests are interpolated, as are variables of the form ${\w+}.
//        #
//        # If you want to add a regular expression test that can't be expressed
//        # in this format, don't add it here: put it in op/pat.t instead.
//
//        For ICU, if field 3 contains an 'i', the test will be skipped.
//        The test exposes is some known incompatibility between ICU and Perl regexps.
//        (The i is in addition to whatever was there before.)
//
//-------------------------------------------------------------------------------
void RegexTest::PerlTests() {
    char tdd[2048];
    const char *srcPath;
    UErrorCode  status = U_ZERO_ERROR;
    UParseError pe;

    //
    //  Open and read the test data file.
    //
    srcPath=getPath(tdd, "re_tests.txt");
    if(srcPath==NULL) {
        return; /* something went wrong, error already output */
    }

    int32_t    len;
    UChar *testData = ReadAndConvertFile(srcPath, len, status);
    if (U_FAILURE(status)) {
        return; /* something went wrong, error already output */
    }

    //
    //  Put the test data into a UnicodeString
    //
    UnicodeString testDataString(FALSE, testData, len);

    //
    //  Regex to break the input file into lines, and strip the new lines.
    //     One line per match, capture group one is the desired data.
    //
    RegexPattern* linePat = RegexPattern::compile("(.+?)[\\r\\n]+", 0, pe, status);
    if (U_FAILURE(status)) {
        dataerrln("RegexPattern::compile() error");
        return;
    }
    RegexMatcher* lineMat = linePat->matcher(testDataString, status);

    //
    //  Regex to split a test file line into fields.
    //    There are six fields, separated by tabs.
    //
    RegexPattern* fieldPat = RegexPattern::compile("\\t", 0, pe, status);

    //
    //  Regex to identify test patterns with flag settings, and to separate them.
    //    Test patterns with flags look like 'pattern'i
    //    Test patterns without flags are not quoted:   pattern
    //   Coming out, capture group 2 is the pattern, capture group 3 is the flags.
    //
    RegexPattern *flagPat = RegexPattern::compile("('?)(.*)\\1(.*)", 0, pe, status);
    RegexMatcher* flagMat = flagPat->matcher(status);

    //
    // The Perl tests reference several perl-isms, which are evaluated/substituted
    //   in the test data.  Not being perl, this must be done explicitly.  Here
    //   are string constants and REs for these constructs.
    //
    UnicodeString nulnulSrc("${nulnul}");
    UnicodeString nulnul("\\u0000\\u0000");
    nulnul = nulnul.unescape();

    UnicodeString ffffSrc("${ffff}");
    UnicodeString ffff("\\uffff");
    ffff = ffff.unescape();

    //  regexp for $-[0], $+[2], etc.
    RegexPattern *groupsPat = RegexPattern::compile("\\$([+\\-])\\[(\\d+)\\]", 0, pe, status);
    RegexMatcher *groupsMat = groupsPat->matcher(status);

    //  regexp for $0, $1, $2, etc.
    RegexPattern *cgPat = RegexPattern::compile("\\$(\\d+)", 0, pe, status);
    RegexMatcher *cgMat = cgPat->matcher(status);


    //
    // Main Loop for the Perl Tests, runs once per line from the
    //   test data file.
    //
    int32_t  lineNum = 0;
    int32_t  skippedUnimplementedCount = 0;
    while (lineMat->find()) {
        lineNum++;

        //
        //  Get a line, break it into its fields, do the Perl
        //    variable substitutions.
        //
        UnicodeString line = lineMat->group(1, status);
        UnicodeString fields[7];
        fieldPat->split(line, fields, 7, status);

        flagMat->reset(fields[0]);
        flagMat->matches(status);
        UnicodeString pattern  = flagMat->group(2, status);
        pattern.findAndReplace("${bang}", "!");
        pattern.findAndReplace(nulnulSrc, "\\u0000\\u0000");
        pattern.findAndReplace(ffffSrc, ffff);

        //
        //  Identify patterns that include match flag settings,
        //    split off the flags, remove the extra quotes.
        //
        UnicodeString flagStr = flagMat->group(3, status);
        if (U_FAILURE(status)) {
            errln("ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
            return;
        }
        int32_t flags = 0;
        const UChar UChar_c = 0x63;  // Char constants for the flag letters.
        const UChar UChar_i = 0x69;  //   (Damn the lack of Unicode support in C)
        const UChar UChar_m = 0x6d;
        const UChar UChar_x = 0x78;
        const UChar UChar_y = 0x79;
        if (flagStr.indexOf(UChar_i) != -1) {
            flags |= UREGEX_CASE_INSENSITIVE;
        }
        if (flagStr.indexOf(UChar_m) != -1) {
            flags |= UREGEX_MULTILINE;
        }
        if (flagStr.indexOf(UChar_x) != -1) {
            flags |= UREGEX_COMMENTS;
        }

        //
        // Compile the test pattern.
        //
        status = U_ZERO_ERROR;
        RegexPattern *testPat = RegexPattern::compile(pattern, flags, pe, status);
        if (status == U_REGEX_UNIMPLEMENTED) {
            //
            // Test of a feature that is planned for ICU, but not yet implemented.
            //   skip the test.
            skippedUnimplementedCount++;
            delete testPat;
            status = U_ZERO_ERROR;
            continue;
        }

        if (U_FAILURE(status)) {
            // Some tests are supposed to generate errors.
            //   Only report an error for tests that are supposed to succeed.
            if (fields[2].indexOf(UChar_c) == -1  &&  // Compilation is not supposed to fail AND
                fields[2].indexOf(UChar_i) == -1)     //   it's not an accepted ICU incompatibility
            {
                errln("line %d: ICU Error \"%s\"\n", lineNum, u_errorName(status));
            }
            status = U_ZERO_ERROR;
            delete testPat;
            continue;
        }

        if (fields[2].indexOf(UChar_i) >= 0) {
            // ICU should skip this test.
            delete testPat;
            continue;
        }

        if (fields[2].indexOf(UChar_c) >= 0) {
            // This pattern should have caused a compilation error, but didn't/
            errln("line %d: Expected a pattern compile error, got success.", lineNum);
            delete testPat;
            continue;
        }

        //
        // replace the Perl variables that appear in some of the
        //   match data strings.
        //
        UnicodeString matchString = fields[1];
        matchString.findAndReplace(nulnulSrc, nulnul);
        matchString.findAndReplace(ffffSrc,   ffff);

        // Replace any \n in the match string with an actual new-line char.
        //  Don't do full unescape, as this unescapes more than Perl does, which
        //  causes other spurious failures in the tests.
        matchString.findAndReplace("\\n", "\n");



        //
        // Run the test, check for expected match/don't match result.
        //
        RegexMatcher *testMat = testPat->matcher(matchString, status);
        UBool found = testMat->find();
        UBool expected = FALSE;
        if (fields[2].indexOf(UChar_y) >=0) {
            expected = TRUE;
        }
        if (expected != found) {
            errln("line %d: Expected %smatch, got %smatch",
                lineNum, expected?"":"no ", found?"":"no " );
            continue;
        }

        //
        // Interpret the Perl expression from the fourth field of the data file,
        // building up an ICU string from the results of the ICU match.
        //   The Perl expression will contain references to the results of
        //     a regex match, including the matched string, capture group strings,
        //     group starting and ending indicies, etc.
        //
        UnicodeString resultString;
        UnicodeString perlExpr = fields[3];
        groupsMat->reset(perlExpr);
        cgMat->reset(perlExpr);

        while (perlExpr.length() > 0) {
            if (perlExpr.startsWith("$&")) {
                resultString.append(testMat->group(status));
                perlExpr.remove(0, 2);
            }

            else if (groupsMat->lookingAt(status)) {
                // $-[0]   $+[2]  etc.
                UnicodeString digitString = groupsMat->group(2, status);
                int32_t t = 0;
                int32_t groupNum = ICU_Utility::parseNumber(digitString, t, 10);
                UnicodeString plusOrMinus = groupsMat->group(1, status);
                int32_t matchPosition;
                if (plusOrMinus.compare("+") == 0) {
                    matchPosition = testMat->end(groupNum, status);
                } else {
                    matchPosition = testMat->start(groupNum, status);
                }
                if (matchPosition != -1) {
                    ICU_Utility::appendNumber(resultString, matchPosition);
                }
                perlExpr.remove(0, groupsMat->end(status));
            }

            else if (cgMat->lookingAt(status)) {
                // $1, $2, $3, etc.
                UnicodeString digitString = cgMat->group(1, status);
                int32_t t = 0;
                int32_t groupNum = ICU_Utility::parseNumber(digitString, t, 10);
                if (U_SUCCESS(status)) {
                    resultString.append(testMat->group(groupNum, status));
                    status = U_ZERO_ERROR;
                }
                perlExpr.remove(0, cgMat->end(status));
            }

            else if (perlExpr.startsWith("@-")) {
                int32_t i;
                for (i=0; i<=testMat->groupCount(); i++) {
                    if (i>0) {
                        resultString.append(" ");
                    }
                    ICU_Utility::appendNumber(resultString, testMat->start(i, status));
                }
                perlExpr.remove(0, 2);
            }

            else if (perlExpr.startsWith("@+")) {
                int32_t i;
                for (i=0; i<=testMat->groupCount(); i++) {
                    if (i>0) {
                        resultString.append(" ");
                    }
                    ICU_Utility::appendNumber(resultString, testMat->end(i, status));
                }
                perlExpr.remove(0, 2);
            }

            else if (perlExpr.startsWith("\\")) {    // \Escape.  Take following char as a literal.
                                                     //           or as an escaped sequence (e.g. \n)
                if (perlExpr.length() > 1) {
                    perlExpr.remove(0, 1);  // Remove the '\', but only if not last char.
                }
                UChar c = perlExpr.charAt(0);
                switch (c) {
                case 'n':   c = '\n'; break;
                // add any other escape sequences that show up in the test expected results.
                }
                resultString.append(c);
                perlExpr.remove(0, 1);
            }

            else  {
                // Any characters from the perl expression that we don't explicitly
                //  recognize before here are assumed to be literals and copied
                //  as-is to the expected results.
                resultString.append(perlExpr.charAt(0));
                perlExpr.remove(0, 1);
            }

            if (U_FAILURE(status)) {
                errln("Line %d: ICU Error \"%s\"", lineNum, u_errorName(status));
                break;
            }
        }

        //
        // Expected Results Compare
        //
        UnicodeString expectedS(fields[4]);
        expectedS.findAndReplace(nulnulSrc, nulnul);
        expectedS.findAndReplace(ffffSrc,   ffff);
        expectedS.findAndReplace("\\n", "\n");


        if (expectedS.compare(resultString) != 0) {
            err("Line %d: Incorrect perl expression results.", lineNum);
            errln((UnicodeString)"Expected \""+expectedS+(UnicodeString)"\"; got \""+resultString+(UnicodeString)"\"");
        }

        delete testMat;
        delete testPat;
    }

    //
    // All done.  Clean up allocated stuff.
    //
    delete cgMat;
    delete cgPat;

    delete groupsMat;
    delete groupsPat;

    delete flagMat;
    delete flagPat;

    delete lineMat;
    delete linePat;

    delete fieldPat;
    delete [] testData;


    logln("%d tests skipped because of unimplemented regexp features.", skippedUnimplementedCount);

}



#endif  /* !UCONFIG_NO_REGULAR_EXPRESSIONS  */

