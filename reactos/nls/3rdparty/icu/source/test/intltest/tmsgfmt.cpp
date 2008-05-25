/********************************************************************
 * Copyright (c) 1997-2007, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 * File TMSGFMT.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   03/24/97    helena      Converted from Java.
 *   07/11/97    helena      Updated to work on AIX.
 *   08/04/97    jfitz       Updated to intltest
 *******************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tmsgfmt.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/gregocal.h"
#include <stdio.h>

void
TestMessageFormat::runIndexedTest(int32_t index, UBool exec,
                                  const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,testBug1);
        TESTCASE(1,testBug2);
        TESTCASE(2,sample);
        TESTCASE(3,PatternTest);
        TESTCASE(4,testStaticFormat);
        TESTCASE(5,testSimpleFormat);
        TESTCASE(6,testMsgFormatChoice);
        TESTCASE(7,testCopyConstructor);
        TESTCASE(8,testAssignment);
        TESTCASE(9,testClone);
        TESTCASE(10,testEquals);
        TESTCASE(11,testNotEquals);
        TESTCASE(12,testSetLocale);
        TESTCASE(13,testFormat);
        TESTCASE(14,testParse);
        TESTCASE(15,testAdopt);
        TESTCASE(16,testCopyConstructor2);
        TESTCASE(17,TestUnlimitedArgsAndSubformats);
        TESTCASE(18,TestRBNF);
        TESTCASE(19,TestTurkishCasing);
        TESTCASE(20,testAutoQuoteApostrophe);
        default: name = ""; break;
    }
}

void TestMessageFormat::testBug3()
{
    double myNumber = -123456;
    DecimalFormat *form = 0;
    Locale locale[] = {
        Locale("ar", "", ""),
        Locale("be", "", ""),
        Locale("bg", "", ""),
        Locale("ca", "", ""),
        Locale("cs", "", ""),
        Locale("da", "", ""),
        Locale("de", "", ""),
        Locale("de", "AT", ""),
        Locale("de", "CH", ""),
        Locale("el", "", ""),       // 10
        Locale("en", "CA", ""),
        Locale("en", "GB", ""),
        Locale("en", "IE", ""),
        Locale("en", "US", ""),
        Locale("es", "", ""),
        Locale("et", "", ""),
        Locale("fi", "", ""),
        Locale("fr", "", ""),
        Locale("fr", "BE", ""),
        Locale("fr", "CA", ""),     // 20
        Locale("fr", "CH", ""),
        Locale("he", "", ""),
        Locale("hr", "", ""),
        Locale("hu", "", ""),
        Locale("is", "", ""),
        Locale("it", "", ""),
        Locale("it", "CH", ""),
        Locale("ja", "", ""),
        Locale("ko", "", ""),
        Locale("lt", "", ""),       // 30
        Locale("lv", "", ""),
        Locale("mk", "", ""),
        Locale("nl", "", ""),
        Locale("nl", "BE", ""),
        Locale("no", "", ""),
        Locale("pl", "", ""),
        Locale("pt", "", ""),
        Locale("ro", "", ""),
        Locale("ru", "", ""),
        Locale("sh", "", ""),       // 40
        Locale("sk", "", ""),
        Locale("sl", "", ""),
        Locale("sq", "", ""),
        Locale("sr", "", ""),
        Locale("sv", "", ""),
        Locale("tr", "", ""),
        Locale("uk", "", ""),
        Locale("zh", "", ""),
        Locale("zh", "TW", "")      // 49
    };
    int32_t i;
    for (i= 0; i < 49; i++) {
        UnicodeString buffer;
        logln(locale[i].getDisplayName(buffer));
        UErrorCode success = U_ZERO_ERROR;
//        form = (DecimalFormat*)NumberFormat::createCurrencyInstance(locale[i], success);
        form = (DecimalFormat*)NumberFormat::createInstance(locale[i], success);
        if (U_FAILURE(success)) {
            errln("Err: Number Format ");
            logln("Number format creation failed.");
            continue;
        }
        Formattable result;
        FieldPosition pos(0);
        buffer.remove();
        form->format(myNumber, buffer, pos);
        success = U_ZERO_ERROR;
        ParsePosition parsePos;
        form->parse(buffer, result, parsePos);
        logln(UnicodeString(" -> ") /* + << dec*/ + toString(result) + UnicodeString("[supposed output for result]"));
        if (U_FAILURE(success)) {
            errln("Err: Number Format parse");
            logln("Number format parse failed.");
        }
        delete form;
    }
}

void TestMessageFormat::testBug1()
{
    const double limit[] = {0.0, 1.0, 2.0};
    const UnicodeString formats[] = {"0.0<=Arg<1.0",
                               "1.0<=Arg<2.0",
                               "2.0<-Arg"};
    ChoiceFormat *cf = new ChoiceFormat(limit, formats, 3);
    FieldPosition status(0);
    UnicodeString toAppendTo;
    cf->format((int32_t)1, toAppendTo, status);
    if (toAppendTo != "1.0<=Arg<2.0") {
        errln("ChoiceFormat cmp in testBug1");
    }
    logln(toAppendTo);
    delete cf;
}

void TestMessageFormat::testBug2()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString result;
    // {sfb} use double format in pattern, so result will match (not strictly necessary)
    const UnicodeString pattern = "There {0,choice,0.0#are no files|1.0#is one file|1.0<are {0, number} files} on disk {1}. ";
    logln("The input pattern : " + pattern);
    MessageFormat *fmt = new MessageFormat(pattern, status);
    if (U_FAILURE(status)) {
        errln("MessageFormat pattern creation failed.");
        return;
    }
    logln("The output pattern is : " + fmt->toPattern(result));
    if (pattern != result) {
        errln("MessageFormat::toPattern() failed.");
    }
    delete fmt;
}

#if 0
#if defined(_DEBUG) && U_IOSTREAM_SOURCE!=0
//----------------------------------------------------
// console I/O
//----------------------------------------------------

#if U_IOSTREAM_SOURCE >= 199711
#   include <iostream>
    std::ostream& operator<<(std::ostream& stream,  const Formattable&   obj);
#elif U_IOSTREAM_SOURCE >= 198506
#   include <iostream.h>
    ostream& operator<<(ostream& stream,  const Formattable&   obj);
#endif

#include "unicode/datefmt.h"
#include <stdlib.h>
#include <string.h>

IntlTest&
operator<<( IntlTest&           stream,
            const Formattable&  obj)
{
    static DateFormat *defDateFormat = 0;

    UnicodeString buffer;
    switch(obj.getType()) {
        case Formattable::kDate : 
            if (defDateFormat == 0) {
                defDateFormat = DateFormat::createInstance();
            }
            defDateFormat->format(obj.getDate(), buffer);
            stream << buffer;
            break;
        case Formattable::kDouble :
            char convert[20];
            sprintf( convert, "%lf", obj.getDouble() );
            stream << convert << "D";
            break;
        case Formattable::kLong :
            stream << obj.getLong() << "L";
            break;
        case Formattable::kString:
            stream << "\"" << obj.getString(buffer) << "\"";
            break;
        case Formattable::kArray:
            int32_t i, count;
            const Formattable* array;
            array = obj.getArray(count);
            stream << "[";
            for (i=0; i<count; ++i) stream << array[i] << ( (i==(count-1)) ? "" : ", " );
            stream << "]";
            break;
        default:
            stream << "INVALID_Formattable";
    }
    return stream;
}
#endif /* defined(_DEBUG) && U_IOSTREAM_SOURCE!=0 */
#endif

void TestMessageFormat::PatternTest() 
{
    Formattable testArgs[] = {
        Formattable(double(1)), Formattable(double(3456)),
            Formattable("Disk"), Formattable(UDate((int32_t)1000000000L), Formattable::kIsDate)
    };
    UnicodeString testCases[] = {
       "Quotes '', '{', 'a' {0} '{0}'",
       "Quotes '', '{', 'a' {0,number} '{0}'",
       "'{'1,number,'#',##} {1,number,'#',##}",
       "There are {1} files on {2} at {3}.",
       "On {2}, there are {1} files, with {0,number,currency}.",
       "'{1,number,percent}', {1,number,percent},",
       "'{1,date,full}', {1,date,full},",
       "'{3,date,full}', {3,date,full},",
       "'{1,number,#,##}' {1,number,#,##}",
    };

    UnicodeString testResultPatterns[] = {
        "Quotes '', '{', a {0} '{'0}",
        "Quotes '', '{', a {0,number} '{'0}",
        "'{'1,number,#,##} {1,number,'#'#,##}",
        "There are {1} files on {2} at {3}.",
        "On {2}, there are {1} files, with {0,number,currency}.",
        "'{'1,number,percent}, {1,number,percent},",
        "'{'1,date,full}, {1,date,full},",
        "'{'3,date,full}, {3,date,full},",
        "'{'1,number,#,##} {1,number,#,##}"
    };

    UnicodeString testResultStrings[] = {
        "Quotes ', {, a 1 {0}",
        "Quotes ', {, a 1 {0}",
        "{1,number,#,##} #34,56",
        "There are 3,456 files on Disk at 1/12/70 5:46 AM.",
        "On Disk, there are 3,456 files, with $1.00.",
        "{1,number,percent}, 345,600%,",
        "{1,date,full}, Wednesday, December 31, 1969,",
        "{3,date,full}, Monday, January 12, 1970,",
        "{1,number,#,##} 34,56"
    };


    for (int32_t i = 0; i < 9; ++i) {
        //it_out << "\nPat in:  " << testCases[i]);

        MessageFormat *form = 0;
        UErrorCode success = U_ZERO_ERROR;
        UnicodeString buffer;
        form = new MessageFormat(testCases[i], Locale::getUS(), success);
        if (U_FAILURE(success)) {
            errln("MessageFormat creation failed.#1");
            logln(((UnicodeString)"MessageFormat for ") + testCases[i] + " creation failed.\n");
            continue;
        }
        if (form->toPattern(buffer) != testResultPatterns[i]) {
            errln(UnicodeString("TestMessageFormat::PatternTest failed test #2, i = ") + i);
            //form->toPattern(buffer);
            errln(((UnicodeString)" Orig: ") + testCases[i]);
            errln(((UnicodeString)" Exp:  ") + testResultPatterns[i]);
            errln(((UnicodeString)" Got:  ") + buffer);
        }

        //it_out << "Pat out: " << form->toPattern(buffer));
        UnicodeString result;
        int32_t count = 4;
        FieldPosition fieldpos(0);
        form->format(testArgs, count, result, fieldpos, success);
        if (U_FAILURE(success)) {
            errln("MessageFormat failed test #3");
            logln("TestMessageFormat::PatternTest failed test #3");
            continue;
        }
        if (result != testResultStrings[i]) {
            errln("TestMessageFormat::PatternTest failed test #4");
            logln("TestMessageFormat::PatternTest failed #4.");
            logln(UnicodeString("    Result: ") + result );
            logln(UnicodeString("  Expected: ") + testResultStrings[i] );
        }
        

        //it_out << "Result:  " << result);
#if 0
        /* TODO: Look at this test and see if this is still a valid test */
        logln("---------------- test parse ----------------");

        form->toPattern(buffer);
        logln("MSG pattern for parse: " + buffer);

        int32_t parseCount = 0;
        Formattable* values = form->parse(result, parseCount, success);
        if (U_FAILURE(success)) {
            errln("MessageFormat failed test #5");
            logln(UnicodeString("MessageFormat failed test #5 with error code ")+(int32_t)success);
        } else if (parseCount != count) {
            errln("MSG count not %d as expected. Got %d", count, parseCount);
        }
        UBool failed = FALSE;
        for (int32_t j = 0; j < parseCount; ++j) {
             if (values == 0 || testArgs[j] != values[j]) {
                errln(((UnicodeString)"MSG testargs[") + j + "]: " + toString(testArgs[j]));
                errln(((UnicodeString)"MSG values[") + j + "]  : " + toString(values[j]));
                failed = TRUE;
             }
        }
        if (failed)
            errln("MessageFormat failed test #6");
#endif
        delete form;
    }
}

void TestMessageFormat::sample() 
{
    MessageFormat *form = 0;
    UnicodeString buffer1, buffer2;
    UErrorCode success = U_ZERO_ERROR;
    form = new MessageFormat("There are {0} files on {1}", success);
    if (U_FAILURE(success)) {
        errln("Err: Message format creation failed");
        logln("Sample message format creation failed.");
        return;
    }
    UnicodeString abc("abc");
    UnicodeString def("def");
    Formattable testArgs1[] = { abc, def };
    FieldPosition fieldpos(0);
    assertEquals("format",
                 "There are abc files on def",
                 form->format(testArgs1, 2, buffer2, fieldpos, success));
    assertSuccess("format", success);
    delete form;
}

void TestMessageFormat::testStaticFormat()
{
    UErrorCode err = U_ZERO_ERROR;
    Formattable arguments[] = {
        (int32_t)7,
        Formattable(UDate(8.71068e+011), Formattable::kIsDate),
        "a disturbance in the Force"
        };

    UnicodeString result;
    result = MessageFormat::format(
        "At {1,time} on {1,date}, there was {2} on planet {0,number,integer}.",
        arguments,
        3,
        result,
        err);

    if (U_FAILURE(err)) {
        errln("TestMessageFormat::testStaticFormat #1");
        logln(UnicodeString("TestMessageFormat::testStaticFormat failed test #1 with error code ")+(int32_t)err);
        return;
    }

    const UnicodeString expected(
            "At 12:20:00 PM on Aug 8, 1997, there was a disturbance in the Force on planet 7.", "");
    if (result != expected) {
        errln("TestMessageFormat::testStaticFormat failed on test");
        logln( UnicodeString("     Result: ") + result );
        logln( UnicodeString("   Expected: ") + expected );
    }
}

/* When the default locale is tr, make sure that the pattern can still be parsed. */
void TestMessageFormat::TestTurkishCasing()
{
    UErrorCode err = U_ZERO_ERROR;
    Locale  saveDefaultLocale;
    Locale::setDefault( Locale("tr"), err );

    Formattable arguments[] = {
        (int32_t)7,
        Formattable(UDate(8.71068e+011), Formattable::kIsDate),
        "a disturbance in the Force"
        };

    UnicodeString result;
    result = MessageFormat::format(
        "At {1,TIME} on {1,DATE,SHORT}, there was {2} on planet {0,NUMBER,INTEGER}.",
        arguments,
        3,
        result,
        err);

    if (U_FAILURE(err)) {
        errln("TestTurkishCasing #1 with error code %s", u_errorName(err));
        return;
    }

    const UnicodeString expected(
            "At 12:20:00 on 08.08.1997, there was a disturbance in the Force on planet 7.", "");
    if (result != expected) {
        errln("TestTurkishCasing failed on test");
        errln( UnicodeString("     Result: ") + result );
        errln( UnicodeString("   Expected: ") + expected );
    }
    Locale::setDefault( saveDefaultLocale, err );
}

void TestMessageFormat::testSimpleFormat(/* char* par */)
{
    logln("running TestMessageFormat::testSimpleFormat");

    UErrorCode err = U_ZERO_ERROR;

    Formattable testArgs1[] = {(int32_t)0, "MyDisk"};
    Formattable testArgs2[] = {(int32_t)1, "MyDisk"};
    Formattable testArgs3[] = {(int32_t)12, "MyDisk"};
   
    MessageFormat* form = new MessageFormat(
        "The disk \"{1}\" contains {0} file(s).", err);
    
    UnicodeString string;
    FieldPosition ignore(FieldPosition::DONT_CARE);
    form->format(testArgs1, 2, string, ignore, err);
    if (U_FAILURE(err) || string != "The disk \"MyDisk\" contains 0 file(s).") {
        errln(UnicodeString("TestMessageFormat::testSimpleFormat failed on test #1"));
    }
 
    ignore.setField(FieldPosition::DONT_CARE);
    string.remove();
    form->format(testArgs2, 2, string, ignore, err);
    if (U_FAILURE(err) || string != "The disk \"MyDisk\" contains 1 file(s).") {
        logln(string);
        errln(UnicodeString("TestMessageFormat::testSimpleFormat failed on test #2")+string);
    }
 
    ignore.setField(FieldPosition::DONT_CARE);
    string.remove();
    form->format(testArgs3, 2, string, ignore, err);
    if (U_FAILURE(err) || string != "The disk \"MyDisk\" contains 12 file(s).") {
        errln(UnicodeString("TestMessageFormat::testSimpleFormat failed on test #3")+string);
    }

    delete form;
 }

void TestMessageFormat::testMsgFormatChoice(/* char* par */)
{
    logln("running TestMessageFormat::testMsgFormatChoice");

    UErrorCode err = U_ZERO_ERROR;

    MessageFormat* form = new MessageFormat("The disk \"{1}\" contains {0}.", err);
    double filelimits[] = {0,1,2};
    UnicodeString filepart[] = {"no files","one file","{0,number} files"};
    ChoiceFormat* fileform = new ChoiceFormat(filelimits, filepart, 3);
    form->setFormat(1,*fileform); // NOT zero, see below
        //is the format adopted?

    FieldPosition ignore(FieldPosition::DONT_CARE);
    UnicodeString string;
    Formattable testArgs1[] = {(int32_t)0, "MyDisk"};    
    form->format(testArgs1, 2, string, ignore, err);
    if (string != "The disk \"MyDisk\" contains no files.") {
        errln("TestMessageFormat::testMsgFormatChoice failed on test #1");
    }
 
    ignore.setField(FieldPosition::DONT_CARE);
    string.remove();
    Formattable testArgs2[] = {(int32_t)1, "MyDisk"};    
    form->format(testArgs2, 2, string, ignore, err);
    if (string != "The disk \"MyDisk\" contains one file.") {
        errln("TestMessageFormat::testMsgFormatChoice failed on test #2");
    }

    ignore.setField(FieldPosition::DONT_CARE);
    string.remove();
    Formattable testArgs3[] = {(int32_t)1273, "MyDisk"};    
    form->format(testArgs3, 2, string, ignore, err);
    if (string != "The disk \"MyDisk\" contains 1,273 files.") {
        errln("TestMessageFormat::testMsgFormatChoice failed on test #3");
    }

    delete form;
    delete fileform;
}


//---------------------------------
//  API Tests
//---------------------------------

void TestMessageFormat::testCopyConstructor() 
{
    UErrorCode success = U_ZERO_ERROR;
    MessageFormat *x = new MessageFormat("There are {0} files on {1}", success);
    MessageFormat *z = new MessageFormat("There are {0} files on {1} created", success);
    MessageFormat *y = 0;
    y = new MessageFormat(*x);
    if ( (*x == *y) && 
         (*x != *z) && 
         (*y != *z) )
         logln("First test (operator ==): Passed!");
    else {
        errln("TestMessageFormat::testCopyConstructor failed #1");
        logln("First test (operator ==): Failed!");
    }
    if ( ((*x == *y) && (*y == *x)) &&
         ((*x != *z) && (*z != *x)) &&
         ((*y != *z) && (*z != *y)) )
        logln("Second test (equals): Passed!");
    else {
        errln("TestMessageFormat::testCopyConstructor failed #2");
        logln("Second test (equals): Failed!");
    }

    delete x;
    delete y;
    delete z;
}


void TestMessageFormat::testAssignment() 
{
    UErrorCode success = U_ZERO_ERROR;
    MessageFormat *x = new MessageFormat("There are {0} files on {1}", success);
    MessageFormat *z = new MessageFormat("There are {0} files on {1} created", success);
    MessageFormat *y = new MessageFormat("There are {0} files on {1} created", success);
    *y = *x;
    if ( (*x == *y) && 
         (*x != *z) && 
         (*y != *z) )
        logln("First test (operator ==): Passed!");
    else {
        errln( "TestMessageFormat::testAssignment failed #1");
        logln("First test (operator ==): Failed!");
    }
    if ( ((*x == *y) && (*y == *x)) &&
         ((*x != *z) && (*z != *x)) &&
         ((*y != *z) && (*z != *y)) )
        logln("Second test (equals): Passed!");
    else {
        errln("TestMessageFormat::testAssignment failed #2");
        logln("Second test (equals): Failed!");
    }

    delete x;
    delete y;
    delete z;
}

void TestMessageFormat::testClone() 
{
    UErrorCode success = U_ZERO_ERROR;
    MessageFormat *x = new MessageFormat("There are {0} files on {1}", success);
    MessageFormat *z = new MessageFormat("There are {0} files on {1} created", success);
    MessageFormat *y = 0;
    y = (MessageFormat*)x->clone();
    if ( (*x == *y) && 
         (*x != *z) && 
         (*y != *z) )
        logln("First test (operator ==): Passed!");
    else {
        errln("TestMessageFormat::testClone failed #1");
        logln("First test (operator ==): Failed!");
    }
    if ( ((*x == *y) && (*y == *x)) &&
         ((*x != *z) && (*z != *x)) &&
         ((*y != *z) && (*z != *y)) )
        logln("Second test (equals): Passed!");
    else {
        errln("TestMessageFormat::testClone failed #2");
        logln("Second test (equals): Failed!");
    }

    delete x;
    delete y;
    delete z;
}

void TestMessageFormat::testEquals() 
{
    UErrorCode success = U_ZERO_ERROR;
    MessageFormat x("There are {0} files on {1}", success);
    MessageFormat y("There are {0} files on {1}", success);
    if (!(x == y)) {
        errln( "TestMessageFormat::testEquals failed #1");
        logln("First test (operator ==): Failed!");
    }

}

void TestMessageFormat::testNotEquals() 
{
    UErrorCode success = U_ZERO_ERROR;
    MessageFormat x("There are {0} files on {1}", success);
    MessageFormat y(x);
    y.setLocale(Locale("fr"));
    if (!(x != y)) {
        errln( "TestMessageFormat::testEquals failed #1");
        logln("First test (operator !=): Failed!");
    }
    y = x;
    y.applyPattern("There are {0} files on {1} the disk", success);
    if (!(x != y)) {
        errln( "TestMessageFormat::testEquals failed #1");
        logln("Second test (operator !=): Failed!");
    }
}


void TestMessageFormat::testSetLocale()
{
    UErrorCode err = U_ZERO_ERROR;
    GregorianCalendar cal(err);   
    Formattable arguments[] = {
        456.83,
        Formattable(UDate(8.71068e+011), Formattable::kIsDate),
        "deposit"
        };
   
    UnicodeString result;

    //UnicodeString formatStr = "At {1,time} on {1,date}, you made a {2} of {0,number,currency}.";
    UnicodeString formatStr = "At <time> on {1,date}, you made a {2} of {0,number,currency}.";
    // {sfb} to get $, would need Locale::US, not Locale::ENGLISH
    // Just use unlocalized currency symbol.
    //UnicodeString compareStrEng = "At <time> on Aug 8, 1997, you made a deposit of $456.83.";
    UnicodeString compareStrEng = "At <time> on Aug 8, 1997, you made a deposit of ";
    compareStrEng += (UChar) 0x00a4;
    compareStrEng += "456.83.";
    // {sfb} to get DM, would need Locale::GERMANY, not Locale::GERMAN
    // Just use unlocalized currency symbol.
    //UnicodeString compareStrGer = "At <time> on 08.08.1997, you made a deposit of 456,83 DM.";
    UnicodeString compareStrGer = "At <time> on 08.08.1997, you made a deposit of ";
    compareStrGer += "456,83 ";
    compareStrGer += (UChar) 0x00a4;
    compareStrGer += ".";

    MessageFormat msg( formatStr, err);
    result = "";
    FieldPosition pos(0);
    result = msg.format(
        arguments,
        3,
        result,
        pos,
        err);

    logln(result);
    if (result != compareStrEng) {
        errln("***  MSG format err.");
    }

    msg.setLocale(Locale::getEnglish());
    UBool getLocale_ok = TRUE;
    if (msg.getLocale() != Locale::getEnglish()) {
        errln("*** MSG getLocal err.");
        getLocale_ok = FALSE;
    }

    msg.setLocale(Locale::getGerman());

    if (msg.getLocale() != Locale::getGerman()) {
        errln("*** MSG getLocal err.");
        getLocale_ok = FALSE;
    }

    msg.applyPattern( formatStr, err);

    pos.setField(0);
    result = "";
    result = msg.format(
        arguments,
        3,
        result,
        pos,
        err);

    logln(result);
    if (result == compareStrGer) {
        logln("MSG setLocale tested.");
    }else{
        errln( "*** MSG setLocale err.");
    }

    if (getLocale_ok) { 
        logln("MSG getLocale tested.");
    }
}

void TestMessageFormat::testFormat()
{
    UErrorCode err = U_ZERO_ERROR;
    GregorianCalendar cal(err);   

    const Formattable ftarray[] = 
    {
        Formattable( UDate(8.71068e+011), Formattable::kIsDate )
    };
    const int32_t ft_cnt = sizeof(ftarray) / sizeof(Formattable);
    Formattable ft_arr( ftarray, ft_cnt );

    Formattable* fmt = new Formattable(UDate(8.71068e+011), Formattable::kIsDate);
   
    UnicodeString result;

    //UnicodeString formatStr = "At {1,time} on {1,date}, you made a {2} of {0,number,currency}.";
    UnicodeString formatStr = "On {0,date}, it began.";
    UnicodeString compareStr = "On Aug 8, 1997, it began.";

    err = U_ZERO_ERROR;
    MessageFormat msg( formatStr, err);
    FieldPosition fp(0);

    result = "";
    fp = 0;
    result = msg.format(
        *fmt,
        result,
        //FieldPosition(0),
        fp,
        err);

    if (err != U_ILLEGAL_ARGUMENT_ERROR) {
        errln("*** MSG format without expected error code.");
    }
    err = U_ZERO_ERROR;

    result = "";
    fp = 0;
    result = msg.format(
        ft_arr,
        result,
        //FieldPosition(0),
        fp,
        err);

    logln("MSG format( Formattable&, ... ) expected:" + compareStr);
    logln("MSG format( Formattable&, ... )   result:" + result);
    if (result != compareStr) {
        errln("***  MSG format( Formattable&, .... ) err.");
    }else{
        logln("MSG format( Formattable&, ... ) tested.");
    }

    delete fmt;

}

void TestMessageFormat::testParse()
{
    UErrorCode err = U_ZERO_ERROR;
    int32_t count;
    UnicodeString msgFormatString = "{0} =sep= {1}";
    MessageFormat msg( msgFormatString, err);
    UnicodeString source = "abc =sep= def";
    UnicodeString tmp1, tmp2;

    Formattable* fmt_arr = msg.parse( source, count, err );
    if (U_FAILURE(err) || (!fmt_arr)) {
        errln("*** MSG parse (ustring, count, err) error.");
    }else{
        logln("MSG parse -- count: %d", count);
        if (count != 2) {
            errln("*** MSG parse (ustring, count, err) count err.");
        }else{
            if ((fmt_arr[0].getType() == Formattable::kString)
             && (fmt_arr[1].getType() == Formattable::kString)
             && (fmt_arr[0].getString(tmp1) == "abc")
             && (fmt_arr[1].getString(tmp2) == "def")) {
                logln("MSG parse (ustring, count, err) tested.");
            }else{
                errln("*** MSG parse (ustring, count, err) result err.");
            }
        }
    }
    delete[] fmt_arr;

    ParsePosition pp(0);

    fmt_arr = msg.parse( source, pp, count );
    if ((pp == 0) || (!fmt_arr)) {
        errln("*** MSG parse (ustring, parsepos., count) error.");
    }else{
        logln("MSG parse -- count: %d", count);
        if (count != 2) {
            errln("*** MSG parse (ustring, parsepos., count) count err.");
        }else{
            if ((fmt_arr[0].getType() == Formattable::kString)
             && (fmt_arr[1].getType() == Formattable::kString)
             && (fmt_arr[0].getString(tmp1) == "abc")
             && (fmt_arr[1].getString(tmp2) == "def")) {
                logln("MSG parse (ustring, parsepos., count) tested.");
            }else{
                errln("*** MSG parse (ustring, parsepos., count) result err.");
            }
        }
    }
    delete[] fmt_arr;

    pp = 0;
    Formattable fmta;

    msg.parseObject( source, fmta, pp );
    if (pp == 0) {
        errln("*** MSG parse (ustring, Formattable, parsepos ) error.");
    }else{
        logln("MSG parse -- count: %d", count);
        fmta.getArray(count);
        if (count != 2) {
            errln("*** MSG parse (ustring, Formattable, parsepos ) count err.");
        }else{
            if ((fmta[0].getType() == Formattable::kString)
             && (fmta[1].getType() == Formattable::kString)
             && (fmta[0].getString(tmp1) == "abc")
             && (fmta[1].getString(tmp2) == "def")) {
                logln("MSG parse (ustring, Formattable, parsepos ) tested.");
            }else{
                errln("*** MSG parse (ustring, Formattable, parsepos ) result err.");
            }
        }
    }
}


void TestMessageFormat::testAdopt()
{
    UErrorCode err = U_ZERO_ERROR;

    UnicodeString formatStr("{0,date},{1},{2,number}", "");
    UnicodeString formatStrChange("{0,number},{1,number},{2,date}", "");
    err = U_ZERO_ERROR;
    MessageFormat msg( formatStr, err);
    MessageFormat msgCmp( formatStr, err);
    int32_t count, countCmp;
    const Format** formats = msg.getFormats(count);
    const Format** formatsCmp = msgCmp.getFormats(countCmp);
    const Format** formatsChg = 0;
    const Format** formatsAct = 0;
    int32_t countAct;
    const Format* a;
    const Format* b;
    UnicodeString patCmp;
    UnicodeString patAct;
    Format** formatsToAdopt;

    if (!formats || !formatsCmp || (count <= 0) || (count != countCmp)) {
        errln("Error getting Formats");
        return;
    }

    int32_t i;

    for (i = 0; i < count; i++) {
        a = formats[i];
        b = formatsCmp[i];
        if ((a != NULL) && (b != NULL)) {
            if (*a != *b) {
                errln("a != b");
                return;
            }
        }else if ((a != NULL) || (b != NULL)) {
            errln("(a != NULL) || (b != NULL)");
            return;
        }
    }

    msg.applyPattern( formatStrChange, err ); //set msg formats to something different
    int32_t countChg;
    formatsChg = msg.getFormats(countChg); // tested function
    if (!formatsChg || (countChg != count)) {
        errln("Error getting Formats");
        return;
    }

    UBool diff;
    diff = TRUE;
    for (i = 0; i < count; i++) {
        a = formatsChg[i];
        b = formatsCmp[i];
        if ((a != NULL) && (b != NULL)) {
            if (*a == *b) {
                logln("formatsChg == formatsCmp at index %d", i);
                diff = FALSE;
            }
        }
    }
    if (!diff) {
        errln("*** MSG getFormats diff err.");
        return;
    }

    logln("MSG getFormats tested.");

    msg.setFormats( formatsCmp, countCmp ); //tested function

    formatsAct = msg.getFormats(countAct);
    if (!formatsAct || (countAct <=0) || (countAct != countCmp)) {
        errln("Error getting Formats");
        return;
    }

    assertEquals("msgCmp.toPattern()", formatStr, msgCmp.toPattern(patCmp.remove()));
    assertEquals("msg.toPattern()", formatStr, msg.toPattern(patAct.remove()));

    for (i = 0; i < countAct; i++) {
        a = formatsAct[i];
        b = formatsCmp[i];
        if ((a != NULL) && (b != NULL)) {
            if (*a != *b) {
                logln("formatsAct != formatsCmp at index %d", i);
                errln("a != b");
                return;
            }
        }else if ((a != NULL) || (b != NULL)) {
            errln("(a != NULL) || (b != NULL)");
            return;
        }
    }
    logln("MSG setFormats tested.");

    //----

    msg.applyPattern( formatStrChange, err ); //set msg formats to something different

    formatsToAdopt = new Format* [countCmp];
    if (!formatsToAdopt) {
        errln("memory allocation error");
        return;
    }

    for (i = 0; i < countCmp; i++) {
        if (formatsCmp[i] == NULL) {
            formatsToAdopt[i] = NULL;
        }else{
            formatsToAdopt[i] = formatsCmp[i]->clone();
            if (!formatsToAdopt[i]) {
                errln("Can't clone format at index %d", i);
                return;
            }
        }
    }
    msg.adoptFormats( formatsToAdopt, countCmp ); // function to test
    delete[] formatsToAdopt;

    assertEquals("msgCmp.toPattern()", formatStr, msgCmp.toPattern(patCmp.remove()));
    assertEquals("msg.toPattern()", formatStr, msg.toPattern(patAct.remove()));

    formatsAct = msg.getFormats(countAct);
    if (!formatsAct || (countAct <=0) || (countAct != countCmp)) {
        errln("Error getting Formats");
        return;
    }

    for (i = 0; i < countAct; i++) {
        a = formatsAct[i];
        b = formatsCmp[i];
        if ((a != NULL) && (b != NULL)) {
            if (*a != *b) {
                errln("a != b");
                return;
            }
        }else if ((a != NULL) || (b != NULL)) {
            errln("(a != NULL) || (b != NULL)");
            return;
        }
    }
    logln("MSG adoptFormats tested.");

    //---- adoptFormat

    msg.applyPattern( formatStrChange, err ); //set msg formats to something different

    formatsToAdopt = new Format* [countCmp];
    if (!formatsToAdopt) {
        errln("memory allocation error");
        return;
    }

    for (i = 0; i < countCmp; i++) {
        if (formatsCmp[i] == NULL) {
            formatsToAdopt[i] = NULL;
        }else{
            formatsToAdopt[i] = formatsCmp[i]->clone();
            if (!formatsToAdopt[i]) {
                errln("Can't clone format at index %d", i);
                return;
            }
        }
    }

    for ( i = 0; i < countCmp; i++ ) {
        msg.adoptFormat( i, formatsToAdopt[i] ); // function to test
    }
    delete[] formatsToAdopt; // array itself not needed in this case;

    assertEquals("msgCmp.toPattern()", formatStr, msgCmp.toPattern(patCmp.remove()));
    assertEquals("msg.toPattern()", formatStr, msg.toPattern(patAct.remove()));

    formatsAct = msg.getFormats(countAct);
    if (!formatsAct || (countAct <=0) || (countAct != countCmp)) {
        errln("Error getting Formats");
        return;
    }

    for (i = 0; i < countAct; i++) {
        a = formatsAct[i];
        b = formatsCmp[i];
        if ((a != NULL) && (b != NULL)) {
            if (*a != *b) {
                errln("a != b");
                return;
            }
        }else if ((a != NULL) || (b != NULL)) {
            errln("(a != NULL) || (b != NULL)");
            return;
        }
    }
    logln("MSG adoptFormat tested.");
}

// This test is a regression test for a fixed bug in the copy constructor.
// It is kept as a global function rather than as a method since the test depends on memory values.
// (At least before the bug was fixed, whether it showed up or not depended on memory contents,
// which is probably why it didn't show up in the regular test for the copy constructor.)
// For this reason, the test isn't changed even though it contains function calls whose results are
// not tested and had no problems. Actually, the test failed by *crashing*.
static void _testCopyConstructor2()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString formatStr("Hello World on {0,date,full}", "");
    UnicodeString resultStr(" ", "");
    UnicodeString result;
    FieldPosition fp(0);
    UDate d = Calendar::getNow();
    const Formattable fargs( d, Formattable::kIsDate );

    MessageFormat* fmt1 = new MessageFormat( formatStr, status );
    MessageFormat* fmt2 = new MessageFormat( *fmt1 );
    MessageFormat* fmt3;
    MessageFormat* fmt4;

    if (fmt1 == NULL) it_err("testCopyConstructor2: (fmt1 != NULL)");

    result = fmt1->format( &fargs, 1, resultStr, fp, status );

    if (fmt2 == NULL) it_err("testCopyConstructor2: (fmt2 != NULL)");

    fmt3 = (MessageFormat*) fmt1->clone();
    fmt4 = (MessageFormat*) fmt2->clone();

    if (fmt3 == NULL) it_err("testCopyConstructor2: (fmt3 != NULL)");
    if (fmt4 == NULL) it_err("testCopyConstructor2: (fmt4 != NULL)");

    result = fmt1->format( &fargs, 1, resultStr, fp, status );
    result = fmt2->format( &fargs, 1, resultStr, fp, status );
    result = fmt3->format( &fargs, 1, resultStr, fp, status );
    result = fmt4->format( &fargs, 1, resultStr, fp, status );
    delete fmt1;
    delete fmt2;
    delete fmt3;
    delete fmt4;
}

void TestMessageFormat::testCopyConstructor2() {
    _testCopyConstructor2();
}

/**
 * Verify that MessageFormat accomodates more than 10 arguments and
 * more than 10 subformats.
 */
void TestMessageFormat::TestUnlimitedArgsAndSubformats() {
    UErrorCode ec = U_ZERO_ERROR;
    const UnicodeString pattern =
        "On {0,date} (aka {0,date,short}, aka {0,date,long}) "
        "at {0,time} (aka {0,time,short}, aka {0,time,long}) "
        "there were {1,number} werjes "
        "(a {3,number,percent} increase over {2,number}) "
        "despite the {4}''s efforts "
        "and to delight of {5}, {6}, {7}, {8}, {9}, and {10} {11}.";
    MessageFormat msg(pattern, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: constructor failed");
        return;
    }

    const Formattable ARGS[] = {
        Formattable(UDate(1e13), Formattable::kIsDate),
        Formattable((int32_t)1303),
        Formattable((int32_t)1202),
        Formattable(1303.0/1202 - 1),
        Formattable("Glimmung"),
        Formattable("the printers"),
        Formattable("Nick"),
        Formattable("his father"),
        Formattable("his mother"),
        Formattable("the spiddles"),
        Formattable("of course"),
        Formattable("Horace"),
    };
    const int32_t ARGS_LENGTH = sizeof(ARGS) / sizeof(ARGS[0]);
    Formattable ARGS_OBJ(ARGS, ARGS_LENGTH);

    UnicodeString expected =
        "On Nov 20, 2286 (aka 11/20/86, aka November 20, 2286) "
        "at 9:46:40 AM (aka 9:46 AM, aka 9:46:40 AM PST) "
        "there were 1,303 werjes "
        "(a 8% increase over 1,202) "
        "despite the Glimmung's efforts "
        "and to delight of the printers, Nick, his father, "
        "his mother, the spiddles, and of course Horace.";
    UnicodeString result;
    msg.format(ARGS_OBJ, result, ec);
    if (result == expected) {
        logln(result);
    } else {
        errln((UnicodeString)"FAIL: Got " + result +
              ", expected " + expected);
    }
}

// test RBNF extensions to message format
void TestMessageFormat::TestRBNF(void) {
    // WARNING: this depends on the RBNF formats for en_US
    Locale locale("en", "US", "");

    UErrorCode ec = U_ZERO_ERROR;

    UnicodeString values[] = {
        // decimal values do not format completely for ordinal or duration, and 
        // do not always parse, so do not include them
        "0", "1", "12", "100", "123", "1001", "123,456", "-17",
    };
    int32_t values_count = sizeof(values)/sizeof(values[0]);

    UnicodeString formats[] = {
        "There are {0,spellout} files to search.",
        "There are {0,spellout,%simplified} files to search.",
        "The bogus spellout {0,spellout,%BOGUS} files behaves like the default.",
        "This is the {0,ordinal} file to search.", // TODO fix bug, ordinal does not parse
        "Searching this file will take {0,duration} to complete.",
        "Searching this file will take {0,duration,%with-words} to complete.",
    };
    int32_t formats_count = sizeof(formats)/sizeof(formats[0]);

    Formattable args[1];

    NumberFormat* numFmt = NumberFormat::createInstance(locale, ec);
    if (U_FAILURE(ec)) {
        dataerrln("Error calling NumberFormat::createInstance()");
        return;
    }

    for (int i = 0; i < formats_count; ++i) {
        MessageFormat* fmt = new MessageFormat(formats[i], locale, ec);
        logln((UnicodeString)"Testing format pattern: '" + formats[i] + "'");

        for (int j = 0; j < values_count; ++j) {
            ec = U_ZERO_ERROR;
            numFmt->parse(values[j], args[0], ec);
            if (U_FAILURE(ec)) {
                errln((UnicodeString)"Failed to parse test argument " + values[j]);
            } else {
                FieldPosition fp(0);
                UnicodeString result;
                fmt->format(args, 1, result, fp, ec);
                logln((UnicodeString)"value: " + toString(args[0]) + " --> " + result + UnicodeString(" ec: ") + u_errorName(ec));
               
                if (i != 3) { // TODO: fix this, for now skip ordinal parsing (format string at index 3)
                    int32_t count = 0;
                    Formattable* parseResult = fmt->parse(result, count, ec);
                    if (count != 1) {
                        errln((UnicodeString)"parse returned " + count + " args");
                    } else if (parseResult[0] != args[0]) {
                        errln((UnicodeString)"parsed argument " + toString(parseResult[0]) + " != " + toString(args[0]));
                    }
                    delete []parseResult;
                }
            }
        }
        delete fmt;
    }
    delete numFmt;
}

void TestMessageFormat::testAutoQuoteApostrophe(void) {
    const char* patterns[] = { // pattern, expected pattern
        "'", "''",
        "''", "''",
        "'{", "'{'",
        "' {", "'' {",
        "'a", "''a",
        "'{'a", "'{'a",
        "'{a'", "'{a'",
        "'{}", "'{}'",
        "{'", "{'",
        "{'a", "{'a",
        "{'a{}'a}'a", "{'a{}'a}''a",
        "'}'", "'}'",
        "'} '{'}'", "'} '{'}''",
        "'} {{{''", "'} {{{'''",
    };
    int32_t pattern_count = sizeof(patterns)/sizeof(patterns[0]);

    for (int i = 0; i < pattern_count; i += 2) {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString result = MessageFormat::autoQuoteApostrophe(patterns[i], status);
        UnicodeString target(patterns[i+1]);
        if (target != result) {
            const int BUF2_LEN = 64;
            char buf[256];
            char buf2[BUF2_LEN];
            int32_t len = result.extract(0, result.length(), buf2, BUF2_LEN);
            if (len >= BUF2_LEN) {
                buf2[BUF2_LEN-1] = 0;
            }
            sprintf(buf, "[%2d] test \"%s\": target (\"%s\") != result (\"%s\")\n", i/2, patterns[i], patterns[i+1], buf2);
            errln(buf);
        }
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
