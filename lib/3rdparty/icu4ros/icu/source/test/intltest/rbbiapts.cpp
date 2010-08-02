/********************************************************************
 * Copyright (c) 1999-2007, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 *   Date        Name        Description
 *   12/14/99    Madhu        Creation.
 *   01/12/2000  Madhu        updated for changed API
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/uchar.h"
#include "intltest.h"
#include "unicode/rbbi.h"
#include "unicode/schriter.h"
#include "rbbiapts.h"
#include "rbbidata.h"
#include "cstring.h"
#include "ubrkimpl.h"
#include "unicode/ustring.h"
#include "unicode/utext.h"

/**
 * API Test the RuleBasedBreakIterator class
 */


#define TEST_ASSERT_SUCCESS(status) {if (U_FAILURE(status)) {\
errln("Failure at file %s, line %d, error = %s", __FILE__, __LINE__, u_errorName(status));}}

#define TEST_ASSERT(expr) {if ((expr)==FALSE) { \
errln("Test Failure at file %s, line %d", __FILE__, __LINE__);}}

void RBBIAPITest::TestCloneEquals()
{

    UErrorCode status=U_ZERO_ERROR;
    RuleBasedBreakIterator* bi1     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* biequal = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* bi3     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* bi2     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createWordInstance(Locale::getDefault(), status);
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL : in construction");
        return;
    }


    UnicodeString testString="Testing word break iterators's clone() and equals()";
    bi1->setText(testString);
    bi2->setText(testString);
    biequal->setText(testString);

    bi3->setText("hello");

    logln((UnicodeString)"Testing equals()");

    logln((UnicodeString)"Testing == and !=");
    UBool b = (*bi1 != *biequal);
    b |= *bi1 == *bi2;
    b |= *bi1 == *bi3;
    if (b) {
        errln((UnicodeString)"ERROR:1 RBBI's == and != operator failed.");
    }

    if(*bi2 == *biequal || *bi2 == *bi1  || *biequal == *bi3)
        errln((UnicodeString)"ERROR:2 RBBI's == and != operator  failed.");


    // Quick test of RulesBasedBreakIterator assignment -
    // Check that
    //    two different iterators are !=
    //    they are == after assignment
    //    source and dest iterator produce the same next() after assignment.
    //    deleting one doesn't disable the other.
    logln("Testing assignment");
    RuleBasedBreakIterator *bix = (RuleBasedBreakIterator *)BreakIterator::createLineInstance(Locale::getDefault(), status);
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL : in construction");
        return;
    }

    RuleBasedBreakIterator biDefault, biDefault2;
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL : in construction of default iterator");
        return;
    }
    if (biDefault == *bix) {
        errln((UnicodeString)"ERROR: iterators should not compare ==");
        return;
    }
    if (biDefault != biDefault2) {
        errln((UnicodeString)"ERROR: iterators should compare ==");
        return;
    }


    UnicodeString   HelloString("Hello Kitty");
    bix->setText(HelloString);
    if (*bix == *bi2) {
        errln(UnicodeString("ERROR: strings should not be equal before assignment."));
    }
    *bix = *bi2;
    if (*bix != *bi2) {
        errln(UnicodeString("ERROR: strings should be equal before assignment."));
    }

    int bixnext = bix->next();
    int bi2next = bi2->next();
    if (! (bixnext == bi2next && bixnext == 7)) {
        errln(UnicodeString("ERROR: iterators behaved differently after assignment."));
    }
    delete bix;
    if (bi2->next() != 8) {
        errln(UnicodeString("ERROR: iterator.next() failed after deleting copy."));
    }



    logln((UnicodeString)"Testing clone()");
    RuleBasedBreakIterator* bi1clone=(RuleBasedBreakIterator*)bi1->clone();
    RuleBasedBreakIterator* bi2clone=(RuleBasedBreakIterator*)bi2->clone();

    if(*bi1clone != *bi1 || *bi1clone  != *biequal  ||
      *bi1clone == *bi3 || *bi1clone == *bi2)
        errln((UnicodeString)"ERROR:1 RBBI's clone() method failed");

    if(*bi2clone == *bi1 || *bi2clone == *biequal ||
       *bi2clone == *bi3 || *bi2clone != *bi2)
        errln((UnicodeString)"ERROR:2 RBBI's clone() method failed");

    if(bi1->getText() != bi1clone->getText()   ||
       bi2clone->getText() != bi2->getText()   ||
       *bi2clone == *bi1clone )
        errln((UnicodeString)"ERROR: RBBI's clone() method failed");

    delete bi1clone;
    delete bi2clone;
    delete bi1;
    delete bi3;
    delete bi2;
    delete biequal;
}

void RBBIAPITest::TestBoilerPlate()
{
    UErrorCode status = U_ZERO_ERROR;
    BreakIterator* a = BreakIterator::createWordInstance(Locale("hi"), status);
    BreakIterator* b = BreakIterator::createWordInstance(Locale("hi_IN"),status);
    if (U_FAILURE(status)) {
        errln("Creation of break iterator failed %s", u_errorName(status));
        return;
    }
    if(*a!=*b){
        errln("Failed: boilerplate method operator!= does not return correct results");
    }
    BreakIterator* c = BreakIterator::createWordInstance(Locale("ja"),status);
    if(a && c){
        if(*c==*a){
            errln("Failed: boilerplate method opertator== does not return correct results");
        }
    }else{
        errln("creation of break iterator failed");
    }
    delete a;
    delete b;
    delete c;
}

void RBBIAPITest::TestgetRules()
{
    UErrorCode status=U_ZERO_ERROR;

    RuleBasedBreakIterator* bi1=(RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* bi2=(RuleBasedBreakIterator*)RuleBasedBreakIterator::createWordInstance(Locale::getDefault(), status);
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL: in construction");
        delete bi1;
        delete bi2;
        return;
    }



    logln((UnicodeString)"Testing toString()");

    bi1->setText((UnicodeString)"Hello there");

    RuleBasedBreakIterator* bi3 =(RuleBasedBreakIterator*)bi1->clone();

    UnicodeString temp=bi1->getRules();
    UnicodeString temp2=bi2->getRules();
    UnicodeString temp3=bi3->getRules();
    if( temp2.compare(temp3) ==0 || temp.compare(temp2) == 0 || temp.compare(temp3) != 0)
        errln((UnicodeString)"ERROR: error in getRules() method");

    delete bi1;
    delete bi2;
    delete bi3;
}
void RBBIAPITest::TestHashCode()
{
    UErrorCode status=U_ZERO_ERROR;
    RuleBasedBreakIterator* bi1     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* bi3     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* bi2     = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createWordInstance(Locale::getDefault(), status);
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL : in construction");
        delete bi1;
        delete bi2;
        delete bi3;
        return;
    }


    logln((UnicodeString)"Testing hashCode()");

    bi1->setText((UnicodeString)"Hash code");
    bi2->setText((UnicodeString)"Hash code");
    bi3->setText((UnicodeString)"Hash code");

    RuleBasedBreakIterator* bi1clone= (RuleBasedBreakIterator*)bi1->clone();
    RuleBasedBreakIterator* bi2clone= (RuleBasedBreakIterator*)bi2->clone();

    if(bi1->hashCode() != bi1clone->hashCode() ||  bi1->hashCode() != bi3->hashCode() ||
        bi1clone->hashCode() != bi3->hashCode() || bi2->hashCode() != bi2clone->hashCode())
        errln((UnicodeString)"ERROR: identical objects have different hashcodes");

    if(bi1->hashCode() == bi2->hashCode() ||  bi2->hashCode() == bi3->hashCode() ||
        bi1clone->hashCode() == bi2clone->hashCode() || bi1clone->hashCode() == bi2->hashCode())
        errln((UnicodeString)"ERROR: different objects have same hashcodes");

    delete bi1clone;
    delete bi2clone;
    delete bi1;
    delete bi2;
    delete bi3;

}
void RBBIAPITest::TestGetSetAdoptText()
{
    logln((UnicodeString)"Testing getText setText ");
    UErrorCode status=U_ZERO_ERROR;
    UnicodeString str1="first string.";
    UnicodeString str2="Second string.";
    RuleBasedBreakIterator* charIter1 = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    RuleBasedBreakIterator* wordIter1 = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createWordInstance(Locale::getDefault(), status);
    if(U_FAILURE(status)){
        errln((UnicodeString)"FAIL : in construction");
            return;
    }


    CharacterIterator* text1= new StringCharacterIterator(str1);
    CharacterIterator* text1Clone = text1->clone();
    CharacterIterator* text2= new StringCharacterIterator(str2);
    CharacterIterator* text3= new StringCharacterIterator(str2, 3, 10, 3); //  "ond str"

    wordIter1->setText(str1);
    CharacterIterator *tci = &wordIter1->getText();
    UnicodeString      tstr;
    tci->getText(tstr);
    TEST_ASSERT(tstr == str1);
    if(wordIter1->current() != 0)
        errln((UnicodeString)"ERROR:1 setText did not set the iteration position to the beginning of the text, it is" + wordIter1->current() + (UnicodeString)"\n");

    wordIter1->next(2);

    wordIter1->setText(str2);
    if(wordIter1->current() != 0)
        errln((UnicodeString)"ERROR:2 setText did not reset the iteration position to the beginning of the text, it is" + wordIter1->current() + (UnicodeString)"\n");


    charIter1->adoptText(text1Clone);
    TEST_ASSERT(wordIter1->getText() != charIter1->getText());
    tci = &wordIter1->getText();
    tci->getText(tstr);
    TEST_ASSERT(tstr == str2);
    tci = &charIter1->getText();
    tci->getText(tstr);
    TEST_ASSERT(tstr == str1);


    RuleBasedBreakIterator* rb=(RuleBasedBreakIterator*)wordIter1->clone();
    rb->adoptText(text1);
    if(rb->getText() != *text1)
        errln((UnicodeString)"ERROR:1 error in adoptText ");
    rb->adoptText(text2);
    if(rb->getText() != *text2)
        errln((UnicodeString)"ERROR:2 error in adoptText ");

    // Adopt where iterator range is less than the entire orignal source string.
    //   (With the change of the break engine to working with UText internally,
    //    CharacterIterators starting at positions other than zero are not supported)
    rb->adoptText(text3);
    TEST_ASSERT(rb->preceding(2) == 0);
    TEST_ASSERT(rb->following(11) == BreakIterator::DONE);
    //if(rb->preceding(2) != 3) {
    //    errln((UnicodeString)"ERROR:3 error in adoptText ");
    //}
    //if(rb->following(11) != BreakIterator::DONE) {
    //    errln((UnicodeString)"ERROR:4 error in adoptText ");
    //}

    // UText API
    //
    //   Quick test to see if UText is working at all.
    //
    const char *s1 = "\x68\x65\x6C\x6C\x6F\x20\x77\x6F\x72\x6C\x64"; /* "hello world" in UTF-8 */
    const char *s2 = "\x73\x65\x65\x20\x79\x61"; /* "see ya" in UTF-8 */
    //                012345678901

    status = U_ZERO_ERROR;
    UText *ut = utext_openUTF8(NULL, s1, -1, &status);
    wordIter1->setText(ut, status);
    TEST_ASSERT_SUCCESS(status);

    int32_t pos;
    pos = wordIter1->first();
    TEST_ASSERT(pos==0);
    pos = wordIter1->next();
    TEST_ASSERT(pos==5);
    pos = wordIter1->next();
    TEST_ASSERT(pos==6);
    pos = wordIter1->next();
    TEST_ASSERT(pos==11);
    pos = wordIter1->next();
    TEST_ASSERT(pos==UBRK_DONE);

    status = U_ZERO_ERROR;
    UText *ut2 = utext_openUTF8(NULL, s2, -1, &status);
    TEST_ASSERT_SUCCESS(status);
    wordIter1->setText(ut2, status);
    TEST_ASSERT_SUCCESS(status);

    pos = wordIter1->first();
    TEST_ASSERT(pos==0);
    pos = wordIter1->next();
    TEST_ASSERT(pos==3);
    pos = wordIter1->next();
    TEST_ASSERT(pos==4);

    pos = wordIter1->last();
    TEST_ASSERT(pos==6);
    pos = wordIter1->previous();
    TEST_ASSERT(pos==4);
    pos = wordIter1->previous();
    TEST_ASSERT(pos==3);
    pos = wordIter1->previous();
    TEST_ASSERT(pos==0);
    pos = wordIter1->previous();
    TEST_ASSERT(pos==UBRK_DONE);

    status = U_ZERO_ERROR;
    UnicodeString sEmpty;
    UText *gut2 = utext_openUnicodeString(NULL, &sEmpty, &status);
    wordIter1->getUText(gut2, status);
    TEST_ASSERT_SUCCESS(status);
    utext_close(gut2);

    utext_close(ut);
    utext_close(ut2);

    delete wordIter1;
    delete charIter1;
    delete rb;

 }


void RBBIAPITest::TestIteration()
{
    // This test just verifies that the API is present.
    // Testing for correct operation of the break rules happens elsewhere.

    UErrorCode status=U_ZERO_ERROR;
    RuleBasedBreakIterator* bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating character break iterator.  Status = %s", u_errorName(status));
    }
    delete bi;

    status=U_ZERO_ERROR;
    bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createWordInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating Word break iterator.  Status = %s", u_errorName(status));
    }
    delete bi;

    status=U_ZERO_ERROR;
    bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createLineInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating Line break iterator.  Status = %s", u_errorName(status));
    }
    delete bi;

    status=U_ZERO_ERROR;
    bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createSentenceInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating Sentence break iterator.  Status = %s", u_errorName(status));
    }
    delete bi;

    status=U_ZERO_ERROR;
    bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createTitleInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating Title break iterator.  Status = %s", u_errorName(status));
    }
    delete bi;

    status=U_ZERO_ERROR;
    bi  = (RuleBasedBreakIterator*)RuleBasedBreakIterator::createCharacterInstance(Locale::getDefault(), status);
    if (U_FAILURE(status) || bi == NULL)  {
        errln("Failure creating character break iterator.  Status = %s", u_errorName(status));
        return;   // Skip the rest of these tests.
    }


    UnicodeString testString="0123456789";
    bi->setText(testString);

    int32_t i;
    i = bi->first();
    if (i != 0) {
        errln("Incorrect value from bi->first().  Expected 0, got %d.", i);
    }

    i = bi->last();
    if (i != 10) {
        errln("Incorrect value from bi->last().  Expected 10, got %d", i);
    }

    //
    // Previous
    //
    bi->last();
    i = bi->previous();
    if (i != 9) {
        errln("Incorrect value from bi->last() at line %d.  Expected 9, got %d", __LINE__, i);
    }


    bi->first();
    i = bi->previous();
    if (i != BreakIterator::DONE) {
        errln("Incorrect value from bi->previous() at line %d.  Expected DONE, got %d", __LINE__, i);
    }

    //
    // next()
    //
    bi->first();
    i = bi->next();
    if (i != 1) {
        errln("Incorrect value from bi->next() at line %d.  Expected 1, got %d", __LINE__, i);
    }

    bi->last();
    i = bi->next();
    if (i != BreakIterator::DONE) {
        errln("Incorrect value from bi->next() at line %d.  Expected DONE, got %d", __LINE__, i);
    }


    //
    //  current()
    //
    bi->first();
    i = bi->current();
    if (i != 0) {
        errln("Incorrect value from bi->previous() at line %d.  Expected 0, got %d", __LINE__, i);
    }

    bi->next();
    i = bi->current();
    if (i != 1) {
        errln("Incorrect value from bi->previous() at line %d.  Expected 1, got %d", __LINE__, i);
    }

    bi->last();
    bi->next();
    i = bi->current();
    if (i != 10) {
        errln("Incorrect value from bi->previous() at line %d.  Expected 10, got %d", __LINE__, i);
    }

    bi->first();
    bi->previous();
    i = bi->current();
    if (i != 0) {
        errln("Incorrect value from bi->previous() at line %d.  Expected 0, got %d", __LINE__, i);
    }


    //
    // Following()
    //
    i = bi->following(4);
    if (i != 5) {
        errln("Incorrect value from bi->following() at line %d.  Expected 5, got %d", __LINE__, i);
    }

    i = bi->following(9);
    if (i != 10) {
        errln("Incorrect value from bi->following() at line %d.  Expected 10, got %d", __LINE__, i);
    }

    i = bi->following(10);
    if (i != BreakIterator::DONE) {
        errln("Incorrect value from bi->following() at line %d.  Expected DONE, got %d", __LINE__, i);
    }


    //
    // Preceding
    //
    i = bi->preceding(4);
    if (i != 3) {
        errln("Incorrect value from bi->preceding() at line %d.  Expected 3, got %d", __LINE__, i);
    }

    i = bi->preceding(10);
    if (i != 9) {
        errln("Incorrect value from bi->preceding() at line %d.  Expected 9, got %d", __LINE__, i);
    }

    i = bi->preceding(1);
    if (i != 0) {
        errln("Incorrect value from bi->preceding() at line %d.  Expected 0, got %d", __LINE__, i);
    }

    i = bi->preceding(0);
    if (i != BreakIterator::DONE) {
        errln("Incorrect value from bi->preceding() at line %d.  Expected DONE, got %d", __LINE__, i);
    }


    //
    // isBoundary()
    //
    bi->first();
    if (bi->isBoundary(3) != TRUE) {
        errln("Incorrect value from bi->isBoudary() at line %d.  Expected TRUE, got FALSE", __LINE__, i);
    }
    i = bi->current();
    if (i != 3) {
        errln("Incorrect value from bi->current() at line %d.  Expected 3, got %d", __LINE__, i);
    }


    if (bi->isBoundary(11) != FALSE) {
        errln("Incorrect value from bi->isBoudary() at line %d.  Expected FALSE, got TRUE", __LINE__, i);
    }
    i = bi->current();
    if (i != 10) {
        errln("Incorrect value from bi->current() at line %d.  Expected 10, got %d", __LINE__, i);
    }

    //
    // next(n)
    //
    bi->first();
    i = bi->next(4);
    if (i != 4) {
        errln("Incorrect value from bi->next() at line %d.  Expected 4, got %d", __LINE__, i);
    }

    i = bi->next(6);
    if (i != 10) {
        errln("Incorrect value from bi->next() at line %d.  Expected 10, got %d", __LINE__, i);
    }

    bi->first();
    i = bi->next(11);
    if (i != BreakIterator::DONE) {
        errln("Incorrect value from bi->next() at line %d.  Expected BreakIterator::DONE, got %d", __LINE__, i);
    }

    delete bi;

}






void RBBIAPITest::TestBuilder() {
     UnicodeString rulesString1 = "$Letters = [:L:];\n"
                                  "$Numbers = [:N:];\n"
                                  "$Letters+;\n"
                                  "$Numbers+;\n"
                                  "[^$Letters $Numbers];\n"
                                  "!.*;\n";
     UnicodeString testString1  = "abc123..abc";
                                // 01234567890
     int32_t bounds1[] = {0, 3, 6, 7, 8, 11};
     UErrorCode status=U_ZERO_ERROR;
     UParseError    parseError;

     RuleBasedBreakIterator *bi = new RuleBasedBreakIterator(rulesString1, parseError, status);
     if(U_FAILURE(status)) {
         errln("FAIL : in construction");
     } else {
         bi->setText(testString1);
         doBoundaryTest(*bi, testString1, bounds1);
     }
     delete bi;
}


//
//  TestQuoteGrouping
//       Single quotes within rules imply a grouping, so that a modifier
//       following the quoted text (* or +) applies to all of the quoted chars.
//
void RBBIAPITest::TestQuoteGrouping() {
     UnicodeString rulesString1 = "#Here comes the rule...\n"
                                  "'$@!'*;\n"   //  (\$\@\!)*
                                  ".;\n";

     UnicodeString testString1  = "$@!$@!X$@!!X";
                                // 0123456789012
     int32_t bounds1[] = {0, 6, 7, 10, 11, 12};
     UErrorCode status=U_ZERO_ERROR;
     UParseError    parseError;

     RuleBasedBreakIterator *bi = new RuleBasedBreakIterator(rulesString1, parseError, status);
     if(U_FAILURE(status)) {
         errln("FAIL : in construction");
     } else {
         bi->setText(testString1);
         doBoundaryTest(*bi, testString1, bounds1);
     }
     delete bi;
}

//
//  TestRuleStatus
//      Test word break rule status constants.
//
void RBBIAPITest::TestRuleStatus() {
     UChar str[30];
     u_unescape("plain word 123.45 \\u9160\\u9161 \\u30a1\\u30a2 \\u3041\\u3094",
              // 012345678901234567  8      9    0  1      2    3  4      5    6
              //                    Ideographic    Katakana       Hiragana
                str, 30);
     UnicodeString testString1(str);
     int32_t bounds1[] = {0, 5, 6, 10, 11, 17, 18, 19, 20, 21, 23, 24, 25, 26};
     int32_t tag_lo[]  = {UBRK_WORD_NONE,     UBRK_WORD_LETTER, UBRK_WORD_NONE,    UBRK_WORD_LETTER,
                          UBRK_WORD_NONE,     UBRK_WORD_NUMBER, UBRK_WORD_NONE,
                          UBRK_WORD_IDEO,     UBRK_WORD_IDEO,   UBRK_WORD_NONE,
                          UBRK_WORD_KANA,     UBRK_WORD_NONE,   UBRK_WORD_KANA,    UBRK_WORD_KANA};

     int32_t tag_hi[]  = {UBRK_WORD_NONE_LIMIT, UBRK_WORD_LETTER_LIMIT, UBRK_WORD_NONE_LIMIT, UBRK_WORD_LETTER_LIMIT,
                          UBRK_WORD_NONE_LIMIT, UBRK_WORD_NUMBER_LIMIT, UBRK_WORD_NONE_LIMIT,
                          UBRK_WORD_IDEO_LIMIT, UBRK_WORD_IDEO_LIMIT,   UBRK_WORD_NONE_LIMIT,
                          UBRK_WORD_KANA_LIMIT, UBRK_WORD_NONE_LIMIT,   UBRK_WORD_KANA_LIMIT, UBRK_WORD_KANA_LIMIT};

     UErrorCode status=U_ZERO_ERROR;

     RuleBasedBreakIterator *bi = (RuleBasedBreakIterator *)BreakIterator::createWordInstance(Locale::getEnglish(), status);
     if(U_FAILURE(status)) {
         errln("FAIL : in construction");
     } else {
         bi->setText(testString1);
         // First test that the breaks are in the right spots.
         doBoundaryTest(*bi, testString1, bounds1);

         // Then go back and check tag values
         int32_t i = 0;
         int32_t pos, tag;
         for (pos = bi->first(); pos != BreakIterator::DONE; pos = bi->next(), i++) {
             if (pos != bounds1[i]) {
                 errln("FAIL: unexpected word break at postion %d", pos);
                 break;
             }
             tag = bi->getRuleStatus();
             if (tag < tag_lo[i] || tag >= tag_hi[i]) {
                 errln("FAIL: incorrect tag value %d at position %d", tag, pos);
                 break;
             }

             // Check that we get the same tag values from getRuleStatusVec()
             int32_t vec[10];
             int t = bi->getRuleStatusVec(vec, 10, status);
             TEST_ASSERT_SUCCESS(status);
             TEST_ASSERT(t==1);
             TEST_ASSERT(vec[0] == tag);
         }
     }
     delete bi;

     // Now test line break status.  This test mostly is to confirm that the status constants
     //                              are correctly declared in the header.
     testString1 =   "test line. \n";
     // break type    s    s     h

     bi = (RuleBasedBreakIterator *)
         BreakIterator::createLineInstance(Locale::getEnglish(), status);
     if(U_FAILURE(status)) {
         errln("failed to create word break iterator.");
     } else {
         int32_t i = 0;
         int32_t pos, tag;
         UBool   success;

         bi->setText(testString1);
         pos = bi->current();
         tag = bi->getRuleStatus();
         for (i=0; i<3; i++) {
             switch (i) {
             case 0:
                 success = pos==0  && tag==UBRK_LINE_SOFT; break;
             case 1:
                 success = pos==5  && tag==UBRK_LINE_SOFT; break;
             case 2:
                 success = pos==12 && tag==UBRK_LINE_HARD; break;
             default:
                 success = FALSE; break;
             }
             if (success == FALSE) {
                 errln("Fail: incorrect word break status or position.  i=%d, pos=%d, tag=%d",
                     i, pos, tag);
                 break;
             }
             pos = bi->next();
             tag = bi->getRuleStatus();
         }
         if (UBRK_LINE_SOFT >= UBRK_LINE_SOFT_LIMIT ||
             UBRK_LINE_HARD >= UBRK_LINE_HARD_LIMIT ||
             UBRK_LINE_HARD > UBRK_LINE_SOFT && UBRK_LINE_HARD < UBRK_LINE_SOFT_LIMIT ) {
             errln("UBRK_LINE_* constants from header are inconsistent.");
         }
     }
     delete bi;

}


//
//  TestRuleStatusVec
//      Test the vector form of  break rule status.
//
void RBBIAPITest::TestRuleStatusVec() {
    UnicodeString rulesString  = "[A-N]{100}; \n"
                                 "[a-w]{200}; \n"
                                 "[\\p{L}]{300}; \n"
                                 "[\\p{N}]{400}; \n"
                                 "[0-5]{500}; \n"
                                  "!.*;\n";
     UnicodeString testString1  = "Aapz5?";
     int32_t  statusVals[10];
     int32_t  numStatuses;
     int32_t  pos;

     UErrorCode status=U_ZERO_ERROR;
     UParseError    parseError;

     RuleBasedBreakIterator *bi = new RuleBasedBreakIterator(rulesString, parseError, status);
     TEST_ASSERT_SUCCESS(status);
     if (U_SUCCESS(status)) {
         bi->setText(testString1);

         // A
         pos = bi->next();
         TEST_ASSERT(pos==1);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 100);
         TEST_ASSERT(statusVals[1] == 300);

         // a
         pos = bi->next();
         TEST_ASSERT(pos==2);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 200);
         TEST_ASSERT(statusVals[1] == 300);

         // p
         pos = bi->next();
         TEST_ASSERT(pos==3);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 200);
         TEST_ASSERT(statusVals[1] == 300);

         // z
         pos = bi->next();
         TEST_ASSERT(pos==4);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 1);
         TEST_ASSERT(statusVals[0] == 300);

         // 5
         pos = bi->next();
         TEST_ASSERT(pos==5);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 400);
         TEST_ASSERT(statusVals[1] == 500);

         // ?
         pos = bi->next();
         TEST_ASSERT(pos==6);
         numStatuses = bi->getRuleStatusVec(statusVals, 10, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 1);
         TEST_ASSERT(statusVals[0] == 0);

         //
         //  Check buffer overflow error handling.   Char == A
         //
         bi->first();
         pos = bi->next();
         TEST_ASSERT(pos==1);
         memset(statusVals, -1, sizeof(statusVals));
         numStatuses = bi->getRuleStatusVec(statusVals, 0, status);
         TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == -1);

         status = U_ZERO_ERROR;
         memset(statusVals, -1, sizeof(statusVals));
         numStatuses = bi->getRuleStatusVec(statusVals, 1, status);
         TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 100);
         TEST_ASSERT(statusVals[1] == -1);

         status = U_ZERO_ERROR;
         memset(statusVals, -1, sizeof(statusVals));
         numStatuses = bi->getRuleStatusVec(statusVals, 2, status);
         TEST_ASSERT_SUCCESS(status);
         TEST_ASSERT(numStatuses == 2);
         TEST_ASSERT(statusVals[0] == 100);
         TEST_ASSERT(statusVals[1] == 300);
         TEST_ASSERT(statusVals[2] == -1);
     }
     delete bi;

}

//
//   Bug 2190 Regression test.   Builder crash on rule consisting of only a
//                               $variable reference
void RBBIAPITest::TestBug2190() {
     UnicodeString rulesString1 = "$aaa = abcd;\n"
                                  "$bbb = $aaa;\n"
                                  "$bbb;\n";
     UnicodeString testString1  = "abcdabcd";
                                // 01234567890
     int32_t bounds1[] = {0, 4, 8};
     UErrorCode status=U_ZERO_ERROR;
     UParseError    parseError;

     RuleBasedBreakIterator *bi = new RuleBasedBreakIterator(rulesString1, parseError, status);
     if(U_FAILURE(status)) {
         errln("FAIL : in construction");
     } else {
         bi->setText(testString1);
         doBoundaryTest(*bi, testString1, bounds1);
     }
     delete bi;
}


void RBBIAPITest::TestRegistration() {
#if !UCONFIG_NO_SERVICE
    UErrorCode status = U_ZERO_ERROR;
    BreakIterator* ja_word = BreakIterator::createWordInstance("ja_JP", status);

    // ok to not delete these if we exit because of error?
    BreakIterator* ja_char = BreakIterator::createCharacterInstance("ja_JP", status);
    BreakIterator* root_word = BreakIterator::createWordInstance("", status);
    BreakIterator* root_char = BreakIterator::createCharacterInstance("", status);

    URegistryKey key = BreakIterator::registerInstance(ja_word, "xx", UBRK_WORD, status);
    {
        if (ja_word && *ja_word == *root_word) {
            errln("japan not different from root");
        }
    }

    {
        BreakIterator* result = BreakIterator::createWordInstance("xx_XX", status);
        UBool fail = TRUE;
        if(result){
            fail = *result != *ja_word;
        }
        delete result;
        if (fail) {
            errln("bad result for xx_XX/word");
        }
    }

    {
        BreakIterator* result = BreakIterator::createCharacterInstance("ja_JP", status);
        UBool fail = TRUE;
        if(result){
            fail = *result != *ja_char;
        }
        delete result;
        if (fail) {
            errln("bad result for ja_JP/char");
        }
    }

    {
        BreakIterator* result = BreakIterator::createCharacterInstance("xx_XX", status);
        UBool fail = TRUE;
        if(result){
            fail = *result != *root_char;
        }
        delete result;
        if (fail) {
            errln("bad result for xx_XX/char");
        }
    }

    {
        StringEnumeration* avail = BreakIterator::getAvailableLocales();
        UBool found = FALSE;
        const UnicodeString* p;
        while ((p = avail->snext(status))) {
            if (p->compare("xx") == 0) {
                found = TRUE;
                break;
            }
        }
        delete avail;
        if (!found) {
            errln("did not find test locale");
        }
    }

    {
        UBool unreg = BreakIterator::unregister(key, status);
        if (!unreg) {
            errln("unable to unregister");
        }
    }

    {
        BreakIterator* result = BreakIterator::createWordInstance("en_US", status);
        BreakIterator* root = BreakIterator::createWordInstance("", status);
        UBool fail = TRUE;
        if(root){
          fail = *root != *result;
        }
        delete root;
        delete result;
        if (fail) {
            errln("did not get root break");
        }
    }

    {
        StringEnumeration* avail = BreakIterator::getAvailableLocales();
        UBool found = FALSE;
        const UnicodeString* p;
        while ((p = avail->snext(status))) {
            if (p->compare("xx") == 0) {
                found = TRUE;
                break;
            }
        }
        delete avail;
        if (found) {
            errln("found test locale");
        }
    }

    {
        int32_t count;
        UBool   foundLocale = FALSE;
        const Locale *avail = BreakIterator::getAvailableLocales(count);
        for (int i=0; i<count; i++) {
            if (avail[i] == Locale::getEnglish()) {
                foundLocale = TRUE;
                break;
            }
        }
        if (foundLocale == FALSE) {
            errln("BreakIterator::getAvailableLocales(&count), failed to find EN.");
        }
    }


    // ja_word was adopted by factory
    delete ja_char;
    delete root_word;
    delete root_char;
#endif
}

void RBBIAPITest::RoundtripRule(const char *dataFile) {
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    parseError.line = 0;
    parseError.offset = 0;
    UDataMemory *data = udata_open(U_ICUDATA_BRKITR, "brk", dataFile, &status);
    uint32_t length;
    const UChar *builtSource;
    const uint8_t *rbbiRules;
    const uint8_t *builtRules;

    if (U_FAILURE(status)) {
        errln("Can't open \"%s\"", dataFile);
        return;
    }

    builtRules = (const uint8_t *)udata_getMemory(data);
    builtSource = (const UChar *)(builtRules + ((RBBIDataHeader*)builtRules)->fRuleSource);
    RuleBasedBreakIterator *brkItr = new RuleBasedBreakIterator(builtSource, parseError, status);
    if (U_FAILURE(status)) {
        errln("createRuleBasedBreakIterator: ICU Error \"%s\"  at line %d, column %d\n",
                u_errorName(status), parseError.line, parseError.offset);
        return;
    };
    rbbiRules = brkItr->getBinaryRules(length);
    logln("Comparing \"%s\" len=%d", dataFile, length);
    if (memcmp(builtRules, rbbiRules, (int32_t)length) != 0) {
        errln("Built rules and rebuilt rules are different %s", dataFile);
        return;
    }
    delete brkItr;
    udata_close(data);
}

void RBBIAPITest::TestRoundtripRules() {
    RoundtripRule("word");
    RoundtripRule("title");
    RoundtripRule("sent");
    RoundtripRule("line");
    RoundtripRule("char");
    if (!quick) {
        RoundtripRule("word_ja");
        RoundtripRule("word_POSIX");
    }
}

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

void RBBIAPITest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln((UnicodeString)"TestSuite RuleBasedBreakIterator API ");
    switch (index) {
     //   case 0: name = "TestConstruction"; if (exec) TestConstruction(); break;
        case  0: name = "TestCloneEquals"; if (exec) TestCloneEquals(); break;
        case  1: name = "TestgetRules"; if (exec) TestgetRules(); break;
        case  2: name = "TestHashCode"; if (exec) TestHashCode(); break;
        case  3: name = "TestGetSetAdoptText"; if (exec) TestGetSetAdoptText(); break;
        case  4: name = "TestIteration"; if (exec) TestIteration(); break;
        case  5: name = "TestBuilder"; if (exec) TestBuilder(); break;
        case  6: name = "TestQuoteGrouping"; if (exec) TestQuoteGrouping(); break;
        case  7: name = "TestRuleStatus"; if (exec) TestRuleStatus(); break;
        case  8: name = "TestRuleStatusVec"; if (exec) TestRuleStatusVec(); break;
        case  9: name = "TestBug2190"; if (exec) TestBug2190(); break;
        case 10: name = "TestRegistration"; if (exec) TestRegistration(); break;
        case 11: name = "TestBoilerPlate"; if (exec) TestBoilerPlate(); break;
        case 12: name = "TestRoundtripRules"; if (exec) TestRoundtripRules(); break;

        default: name = ""; break; // needed to end loop
    }
}

//---------------------------------------------
//Internal subroutines
//---------------------------------------------

void RBBIAPITest::doBoundaryTest(RuleBasedBreakIterator& bi, UnicodeString& text, int32_t *boundaries){
     logln((UnicodeString)"testIsBoundary():");
        int32_t p = 0;
        UBool isB;
        for (int32_t i = 0; i < text.length(); i++) {
            isB = bi.isBoundary(i);
            logln((UnicodeString)"bi.isBoundary(" + i + ") -> " + isB);

            if (i == boundaries[p]) {
                if (!isB)
                    errln((UnicodeString)"Wrong result from isBoundary() for " + i + (UnicodeString)": expected true, got false");
                p++;
            }
            else {
                if (isB)
                    errln((UnicodeString)"Wrong result from isBoundary() for " + i + (UnicodeString)": expected false, got true");
            }
        }
}
void RBBIAPITest::doTest(UnicodeString& testString, int32_t start, int32_t gotoffset, int32_t expectedOffset, const char* expectedString){
    UnicodeString selected;
    UnicodeString expected=CharsToUnicodeString(expectedString);

    if(gotoffset != expectedOffset)
         errln((UnicodeString)"ERROR:****returned #" + gotoffset + (UnicodeString)" instead of #" + expectedOffset);
    if(start <= gotoffset){
        testString.extractBetween(start, gotoffset, selected);
    }
    else{
        testString.extractBetween(gotoffset, start, selected);
    }
    if(selected.compare(expected) != 0)
         errln(prettify((UnicodeString)"ERROR:****selected \"" + selected + "\" instead of \"" + expected + "\""));
    else
        logln(prettify("****selected \"" + selected + "\""));
}

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
