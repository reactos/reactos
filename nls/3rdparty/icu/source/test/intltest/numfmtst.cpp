/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/* Modification History:
*   Date        Name        Description
*   07/15/99    helena      Ported to HPUX 10/11 CC.
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "numfmtst.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/measfmt.h"
#include "unicode/curramt.h"
#include "digitlst.h"
#include "textfile.h"
#include "tokiter.h"
#include "charstr.h"
#include "putilimp.h"
#include "winnmtst.h"
#include <float.h>
#include <string.h>

static const UChar EUR[] = {69,85,82,0}; // "EUR"
 
// *****************************************************************************
// class NumberFormatTest
// *****************************************************************************

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break

#define CHECK(status,str) if (U_FAILURE(status)) { errln(UnicodeString("FAIL: ") + str); return; }

void NumberFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite DateFormatTest");
    switch (index) {
        CASE(0,TestCurrencySign);
        CASE(1,TestCurrency);
        CASE(2,TestParse);
        CASE(3,TestRounding487);
        CASE(4,TestQuotes);
        CASE(5,TestExponential);
        CASE(6,TestPatterns);

        // Upgrade to alphaWorks - liu 5/99
        CASE(7,TestExponent);
        CASE(8,TestScientific);
        CASE(9,TestPad);
        CASE(10,TestPatterns2);
        CASE(11,TestSecondaryGrouping);
        CASE(12,TestSurrogateSupport);
        CASE(13,TestAPI);

        CASE(14,TestCurrencyObject);
        CASE(15,TestCurrencyPatterns);
        //CASE(16,TestDigitList);
        CASE(16,TestWhiteSpaceParsing);
        CASE(17,TestComplexCurrency);
        CASE(18,TestRegCurrency);
        CASE(19,TestSymbolsWithBadLocale);
        CASE(20,TestAdoptDecimalFormatSymbols);

        CASE(21,TestScientific2);
        CASE(22,TestScientificGrouping);
        CASE(23,TestInt64);

        CASE(24,TestPerMill);
        CASE(25,TestIllegalPatterns);
        CASE(26,TestCases);

        CASE(27,TestCurrencyNames);
        CASE(28,TestCurrencyAmount);
        CASE(29,TestCurrencyUnit);
        CASE(30,TestCoverage);
        CASE(31,TestJB3832);
        CASE(32,TestHost);
        CASE(33,TestHostClone);
        CASE(34,TestCurrencyFormat);
        CASE(35,TestRounding);
        default: name = ""; break;
    }
}
 
// -------------------------------------

// Test API (increase code coverage)
void
NumberFormatTest::TestAPI(void)
{
  logln("Test API");
  UErrorCode status = U_ZERO_ERROR;
  NumberFormat *test = NumberFormat::createInstance("root", status);
  if(U_FAILURE(status)) {
    errln("unable to create format object");
  }
  if(test != NULL) {
    test->setMinimumIntegerDigits(10);
    test->setMaximumIntegerDigits(2);

    test->setMinimumFractionDigits(10);
    test->setMaximumFractionDigits(2);

    UnicodeString result;
    FieldPosition pos;
    Formattable bla("Paja Patak"); // Donald Duck for non Serbian speakers
    test->format(bla, result, pos, status);
    if(U_SUCCESS(status)) {
      errln("Yuck... Formatted a duck... As a number!");
    } else {
      status = U_ZERO_ERROR;
    }

    result.remove();
    int64_t ll = 12;
    test->format(ll, result);
    if (result != "12.00"){
        errln("format int64_t error");
    }

    delete test;  
  }
}

class StubNumberForamt :public NumberFormat{
public:
    StubNumberForamt(){};
    virtual UnicodeString& format(double ,UnicodeString& appendTo,FieldPosition& ) const {
        return appendTo;
    }
    virtual UnicodeString& format(int32_t ,UnicodeString& appendTo,FieldPosition& ) const {
        return appendTo.append((UChar)0x0033);
    }
    virtual UnicodeString& format(int64_t number,UnicodeString& appendTo,FieldPosition& pos) const {
        return NumberFormat::format(number, appendTo, pos);
    }
    virtual UnicodeString& format(const Formattable& , UnicodeString& appendTo, FieldPosition& , UErrorCode& ) const {
        return appendTo;
    }
    virtual void parse(const UnicodeString& ,
                    Formattable& ,
                    ParsePosition& ) const {}
    virtual void parse( const UnicodeString& ,
                        Formattable& ,
                        UErrorCode& ) const {}
    virtual UClassID getDynamicClassID(void) const {
        static char classID = 0;
        return (UClassID)&classID; 
    }
    virtual Format* clone() const {return NULL;}
};

void 
NumberFormatTest::TestCoverage(void){
    StubNumberForamt stub;
    UnicodeString agent("agent");
    FieldPosition pos;
    int64_t num = 4;
    if (stub.format(num, agent, pos) != UnicodeString("agent3")){
        errln("NumberFormat::format(int64, UnicodString&, FieldPosition&) should delegate to (int32, ,)");
    };
}

// Test various patterns
void
NumberFormatTest::TestPatterns(void)
{
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols sym(Locale::getUS(), status);
    if (U_FAILURE(status)) { errln("FAIL: Could not construct DecimalFormatSymbols"); return; }

    const char* pat[]    = { "#.#", "#.", ".#", "#" };
    int32_t pat_length = (int32_t)(sizeof(pat) / sizeof(pat[0]));
    const char* newpat[] = { "#0.#", "#0.", "#.0", "#" };
    const char* num[]    = { "0",   "0.", ".0", "0" };
    for (int32_t i=0; i<pat_length; ++i)
    {
        status = U_ZERO_ERROR;
        DecimalFormat fmt(pat[i], sym, status);
        if (U_FAILURE(status)) { errln((UnicodeString)"FAIL: DecimalFormat constructor failed for " + pat[i]); continue; }
        UnicodeString newp; fmt.toPattern(newp);
        if (!(newp == newpat[i]))
            errln((UnicodeString)"FAIL: Pattern " + pat[i] + " should transmute to " + newpat[i] +
                  "; " + newp + " seen instead");

        UnicodeString s; (*(NumberFormat*)&fmt).format((int32_t)0, s);
        if (!(s == num[i]))
        {
            errln((UnicodeString)"FAIL: Pattern " + pat[i] + " should format zero as " + num[i] +
                  "; " + s + " seen instead");
            logln((UnicodeString)"Min integer digits = " + fmt.getMinimumIntegerDigits());
        }
    }
}

/*
icu_2_4::DigitList::operator== 0 0 2 icuuc24d.dll digitlst.cpp Doug  
icu_2_4::DigitList::append 0 0 4 icuin24d.dll digitlst.h Doug  
icu_2_4::DigitList::operator!= 0 0 1 icuuc24d.dll digitlst.h Doug 
*/
/*
void 
NumberFormatTest::TestDigitList(void)
{
  // API coverage for DigitList
  DigitList list1;
  list1.append('1');
  list1.fDecimalAt = 1;
  DigitList list2;
  list2.set((int32_t)1);
  if (list1 != list2) {
    errln("digitlist append, operator!= or set failed ");
  }
  if (!(list1 == list2)) {
    errln("digitlist append, operator== or set failed ");
  }
}
*/

// -------------------------------------

// Test exponential pattern
void
NumberFormatTest::TestExponential(void)
{
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols sym(Locale::getUS(), status);
    if (U_FAILURE(status)) { errln("FAIL: Bad status returned by DecimalFormatSymbols ct"); return; }
    const char* pat[] = { "0.####E0", "00.000E00", "##0.######E000", "0.###E0;[0.###E0]"  };
    int32_t pat_length = (int32_t)(sizeof(pat) / sizeof(pat[0]));

// The following #if statements allow this test to be built and run on
// platforms that do not have standard IEEE numerics.  For example,
// S/390 doubles have an exponent range of -78 to +75.  For the
// following #if statements to work, float.h must define
// DBL_MAX_10_EXP to be a compile-time constant.

// This section may be expanded as needed.

#if DBL_MAX_10_EXP > 300
    double val[] = { 0.01234, 123456789, 1.23e300, -3.141592653e-271 };
    int32_t val_length = (int32_t)(sizeof(val) / sizeof(val[0]));
    const char* valFormat[] =
    {
        // 0.####E0
        "1.234E-2", "1.2346E8", "1.23E300", "-3.1416E-271",
        // 00.000E00
        "12.340E-03", "12.346E07", "12.300E299", "-31.416E-272",
        // ##0.######E000
        "12.34E-003", "123.4568E006", "1.23E300", "-314.1593E-273",
        // 0.###E0;[0.###E0]
        "1.234E-2", "1.235E8", "1.23E300", "[3.142E-271]"
    };
    double valParse[] =
    {
        0.01234, 123460000, 1.23E300, -3.1416E-271,
        0.01234, 123460000, 1.23E300, -3.1416E-271,
        0.01234, 123456800, 1.23E300, -3.141593E-271,
        0.01234, 123500000, 1.23E300, -3.142E-271,
    };
#elif DBL_MAX_10_EXP > 70
    double val[] = { 0.01234, 123456789, 1.23e70, -3.141592653e-71 };
    int32_t val_length = sizeof(val) / sizeof(val[0]);
    char* valFormat[] =
    {
        // 0.####E0
        "1.234E-2", "1.2346E8", "1.23E70", "-3.1416E-71",
        // 00.000E00
        "12.340E-03", "12.346E07", "12.300E69", "-31.416E-72",
        // ##0.######E000
        "12.34E-003", "123.4568E006", "12.3E069", "-31.41593E-072",
        // 0.###E0;[0.###E0]
        "1.234E-2", "1.235E8", "1.23E70", "[3.142E-71]"
    };
    double valParse[] =
    {
        0.01234, 123460000, 1.23E70, -3.1416E-71,
        0.01234, 123460000, 1.23E70, -3.1416E-71,
        0.01234, 123456800, 1.23E70, -3.141593E-71,
        0.01234, 123500000, 1.23E70, -3.142E-71,
    };
#else
    // Don't test double conversion
    double* val = 0;
    int32_t val_length = 0;
    char** valFormat = 0;
    double* valParse = 0;
    logln("Warning: Skipping double conversion tests");
#endif

    int32_t lval[] = { 0, -1, 1, 123456789 };
    int32_t lval_length = (int32_t)(sizeof(lval) / sizeof(lval[0]));
    const char* lvalFormat[] =
    {
        // 0.####E0
        "0E0", "-1E0", "1E0", "1.2346E8",
        // 00.000E00
        "00.000E00", "-10.000E-01", "10.000E-01", "12.346E07",
        // ##0.######E000
        "0E000", "-1E000", "1E000", "123.4568E006",
        // 0.###E0;[0.###E0]
        "0E0", "[1E0]", "1E0", "1.235E8"
    };
    int32_t lvalParse[] =
    {
        0, -1, 1, 123460000,
        0, -1, 1, 123460000,
        0, -1, 1, 123456800,
        0, -1, 1, 123500000,
    };
    int32_t ival = 0, ilval = 0;
    for (int32_t p=0; p<pat_length; ++p)
    {
        DecimalFormat fmt(pat[p], sym, status);
        if (U_FAILURE(status)) { errln("FAIL: Bad status returned by DecimalFormat ct"); continue; }
        UnicodeString pattern;
        logln((UnicodeString)"Pattern \"" + pat[p] + "\" -toPattern-> \"" +
          fmt.toPattern(pattern) + "\"");
        int32_t v;
        for (v=0; v<val_length; ++v)
        {
            UnicodeString s; (*(NumberFormat*)&fmt).format(val[v], s);
            logln((UnicodeString)" " + val[v] + " -format-> " + s);
            if (s != valFormat[v+ival])
                errln((UnicodeString)"FAIL: Expected " + valFormat[v+ival]);

            ParsePosition pos(0);
            Formattable af;
            fmt.parse(s, af, pos);
            double a;
            UBool useEpsilon = FALSE;
            if (af.getType() == Formattable::kLong)
                a = af.getLong();
            else if (af.getType() == Formattable::kDouble) {
                a = af.getDouble();
#if defined(OS390) || defined(OS400)
                // S/390 will show a failure like this:
                //| -3.141592652999999e-271 -format-> -3.1416E-271
                //|                          -parse-> -3.1416e-271
                //| FAIL: Expected -3.141599999999999e-271
                // To compensate, we use an epsilon-based equality
                // test on S/390 only.  We don't want to do this in
                // general because it's less exacting.
                useEpsilon = TRUE;
#endif
            }
            else {
                errln((UnicodeString)"FAIL: Non-numeric Formattable returned");
                continue;
            }
            if (pos.getIndex() == s.length())
            {
                logln((UnicodeString)"  -parse-> " + a);
                // Use epsilon comparison as necessary
                if ((useEpsilon &&
                    (uprv_fabs(a - valParse[v+ival]) / a > (2*DBL_EPSILON))) ||
                    (!useEpsilon && a != valParse[v+ival]))
                {
                    errln((UnicodeString)"FAIL: Expected " + valParse[v+ival]);
                }
            }
            else {
                errln((UnicodeString)"FAIL: Partial parse (" + pos.getIndex() + " chars) -> " + a);
                errln((UnicodeString)"  should be (" + s.length() + " chars) -> " + valParse[v+ival]);
            }
        }
        for (v=0; v<lval_length; ++v)
        {
            UnicodeString s;
            (*(NumberFormat*)&fmt).format(lval[v], s);
            logln((UnicodeString)" " + lval[v] + "L -format-> " + s);
            if (s != lvalFormat[v+ilval])
                errln((UnicodeString)"ERROR: Expected " + lvalFormat[v+ilval] + " Got: " + s);

            ParsePosition pos(0);
            Formattable af;
            fmt.parse(s, af, pos);
            if (af.getType() == Formattable::kLong ||
                af.getType() == Formattable::kInt64) {
                UErrorCode status = U_ZERO_ERROR;
                int32_t a = af.getLong(status);
                if (pos.getIndex() == s.length())
                {
                    logln((UnicodeString)"  -parse-> " + a);
                    if (a != lvalParse[v+ilval])
                        errln((UnicodeString)"FAIL: Expected " + lvalParse[v+ilval]);
                }
                else
                    errln((UnicodeString)"FAIL: Partial parse (" + pos.getIndex() + " chars) -> " + a);
            }
            else
                errln((UnicodeString)"FAIL: Non-long Formattable returned for " + s
                    + " Double: " + af.getDouble()
                    + ", Long: " + af.getLong());
        }
        ival += val_length;
        ilval += lval_length;
    }
}

void
NumberFormatTest::TestScientific2() {
    // jb 2552
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat* fmt = (DecimalFormat*)NumberFormat::createCurrencyInstance("en_US", status);
    if (U_SUCCESS(status)) {
        double num = 12.34;
        expect(*fmt, num, "$12.34");
        fmt->setScientificNotation(TRUE);
        expect(*fmt, num, "$1.23E1");
        fmt->setScientificNotation(FALSE);
        expect(*fmt, num, "$12.34");
    }
    delete fmt;
}

void 
NumberFormatTest::TestScientificGrouping() {
    // jb 2552
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat fmt("##0.00E0",status);
    if (U_SUCCESS(status)) {
        expect(fmt, .01234, "12.3E-3");
        expect(fmt, .1234, "123E-3");
        expect(fmt, 1.234, "1.23E0");
        expect(fmt, 12.34, "12.3E0");
        expect(fmt, 123.4, "123E0");
        expect(fmt, 1234., "1.23E3");
    }
}

/*static void setFromString(DigitList& dl, const char* str) {
    char c;
    UBool decimalSet = FALSE;
    dl.clear();
    while ((c = *str++)) {
        if (c == '-') {
            dl.fIsPositive = FALSE;
        } else if (c == '+') {
            dl.fIsPositive = TRUE;
        } else if (c == '.') {
            dl.fDecimalAt = dl.fCount;
            decimalSet = TRUE;
        } else {
            dl.append(c);
        }
    }
    if (!decimalSet) {
        dl.fDecimalAt = dl.fCount;
    }
}*/

void
NumberFormatTest::TestInt64() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat fmt("#.#E0",status);
    fmt.setMaximumFractionDigits(20);
    if (U_SUCCESS(status)) {
        expect(fmt, (Formattable)(int64_t)0, "0E0");
        expect(fmt, (Formattable)(int64_t)-1, "-1E0");
        expect(fmt, (Formattable)(int64_t)1, "1E0");
        expect(fmt, (Formattable)(int64_t)2147483647, "2.147483647E9");
        expect(fmt, (Formattable)((int64_t)-2147483647-1), "-2.147483648E9");
        expect(fmt, (Formattable)(int64_t)U_INT64_MAX, "9.223372036854775807E18");
        expect(fmt, (Formattable)(int64_t)U_INT64_MIN, "-9.223372036854775808E18");
    }

    // also test digitlist
/*    int64_t int64max = U_INT64_MAX;
    int64_t int64min = U_INT64_MIN;
    const char* int64maxstr = "9223372036854775807";
    const char* int64minstr = "-9223372036854775808";
    UnicodeString fail("fail: ");

    // test max int64 value
    DigitList dl;
    setFromString(dl, int64maxstr);
    {
        if (!dl.fitsIntoInt64(FALSE)) {
            errln(fail + int64maxstr + " didn't fit");
        }
        int64_t int64Value = dl.getInt64();
        if (int64Value != int64max) {
            errln(fail + int64maxstr);
        }
        dl.set(int64Value);
        int64Value = dl.getInt64();
        if (int64Value != int64max) {
            errln(fail + int64maxstr);
        }
    }
    // test negative of max int64 value (1 shy of min int64 value)
    dl.fIsPositive = FALSE;
    {
        if (!dl.fitsIntoInt64(FALSE)) {
            errln(fail + "-" + int64maxstr + " didn't fit");
        }
        int64_t int64Value = dl.getInt64();
        if (int64Value != -int64max) {
            errln(fail + "-" + int64maxstr);
        }
        dl.set(int64Value);
        int64Value = dl.getInt64();
        if (int64Value != -int64max) {
            errln(fail + "-" + int64maxstr);
        }
    }
    // test min int64 value
    setFromString(dl, int64minstr);
    {
        if (!dl.fitsIntoInt64(FALSE)) {
            errln(fail + "-" + int64minstr + " didn't fit");
        }
        int64_t int64Value = dl.getInt64();
        if (int64Value != int64min) {
            errln(fail + int64minstr);
        }
        dl.set(int64Value);
        int64Value = dl.getInt64();
        if (int64Value != int64min) {
            errln(fail + int64minstr);
        }
    }
    // test negative of min int 64 value (1 more than max int64 value)
    dl.fIsPositive = TRUE; // won't fit
    {
        if (dl.fitsIntoInt64(FALSE)) {
            errln(fail + "-(" + int64minstr + ") didn't fit");
        }
    }*/
}

// -------------------------------------

// Test the handling of quotes
void
NumberFormatTest::TestQuotes(void)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString *pat;
    DecimalFormatSymbols *sym = new DecimalFormatSymbols(Locale::getUS(), status);
    pat = new UnicodeString("a'fo''o'b#");
    DecimalFormat *fmt = new DecimalFormat(*pat, *sym, status);
    UnicodeString s; 
    ((NumberFormat*)fmt)->format((int32_t)123, s);
    logln((UnicodeString)"Pattern \"" + *pat + "\"");
    logln((UnicodeString)" Format 123 -> " + escape(s));
    if (!(s=="afo'ob123")) 
        errln((UnicodeString)"FAIL: Expected afo'ob123");
    
    s.truncate(0);
    delete fmt;
    delete pat;

    pat = new UnicodeString("a''b#");
    fmt = new DecimalFormat(*pat, *sym, status);
    ((NumberFormat*)fmt)->format((int32_t)123, s);
    logln((UnicodeString)"Pattern \"" + *pat + "\"");
    logln((UnicodeString)" Format 123 -> " + escape(s));
    if (!(s=="a'b123")) 
        errln((UnicodeString)"FAIL: Expected a'b123");
    delete fmt;
    delete pat;
    delete sym;
}

/**
 * Test the handling of the currency symbol in patterns.
 */
void
NumberFormatTest::TestCurrencySign(void)
{
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols* sym = new DecimalFormatSymbols(Locale::getUS(), status);
    UnicodeString pat;
    UChar currency = 0x00A4;
    // "\xA4#,##0.00;-\xA4#,##0.00"
    pat.append(currency).append("#,##0.00;-").
        append(currency).append("#,##0.00");
    DecimalFormat *fmt = new DecimalFormat(pat, *sym, status);
    UnicodeString s; ((NumberFormat*)fmt)->format(1234.56, s);
    pat.truncate(0);
    logln((UnicodeString)"Pattern \"" + fmt->toPattern(pat) + "\"");
    logln((UnicodeString)" Format " + 1234.56 + " -> " + escape(s));
    if (s != "$1,234.56") errln((UnicodeString)"FAIL: Expected $1,234.56");
    s.truncate(0);
    ((NumberFormat*)fmt)->format(- 1234.56, s);
    logln((UnicodeString)" Format " + (-1234.56) + " -> " + escape(s));
    if (s != "-$1,234.56") errln((UnicodeString)"FAIL: Expected -$1,234.56");
    delete fmt;
    pat.truncate(0);
    // "\xA4\xA4 #,##0.00;\xA4\xA4 -#,##0.00"
    pat.append(currency).append(currency).
        append(" #,##0.00;").
        append(currency).append(currency).
        append(" -#,##0.00");
    fmt = new DecimalFormat(pat, *sym, status);
    s.truncate(0);
    ((NumberFormat*)fmt)->format(1234.56, s);
    logln((UnicodeString)"Pattern \"" + fmt->toPattern(pat) + "\"");
    logln((UnicodeString)" Format " + 1234.56 + " -> " + escape(s));
    if (s != "USD 1,234.56") errln((UnicodeString)"FAIL: Expected USD 1,234.56");
    s.truncate(0);
    ((NumberFormat*)fmt)->format(-1234.56, s);
    logln((UnicodeString)" Format " + (-1234.56) + " -> " + escape(s));
    if (s != "USD -1,234.56") errln((UnicodeString)"FAIL: Expected USD -1,234.56");
    delete fmt;
    delete sym;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
}
 
// -------------------------------------

static UChar toHexString(int32_t i) { return (UChar)(i + (i < 10 ? 0x30 : (0x41 - 10))); }

UnicodeString&
NumberFormatTest::escape(UnicodeString& s)
{
    UnicodeString buf;
    for (int32_t i=0; i<s.length(); ++i)
    {
        UChar c = s[(int32_t)i];
        if (c <= (UChar)0x7F) buf += c;
        else {
            buf += (UChar)0x5c; buf += (UChar)0x55;
            buf += toHexString((c & 0xF000) >> 12);
            buf += toHexString((c & 0x0F00) >> 8);
            buf += toHexString((c & 0x00F0) >> 4);
            buf += toHexString(c & 0x000F);
        }
    }
    return (s = buf);
}

 
// -------------------------------------
static const char* testCases[][2]= {
     /* locale ID */  /* expected */
    {"ca_ES_PREEURO", "1.150 \\u20A7" },
    {"de_LU_PREEURO", "1,150 F" },
    {"el_GR_PREEURO", "1.150,50\\u0394\\u03C1\\u03C7" },
    {"en_BE_PREEURO", "1.150,50 BF" },
    {"es_ES_PREEURO", "1.150 \\u20A7" },
    {"eu_ES_PREEURO", "1.150 \\u20A7" }, 
    {"gl_ES_PREEURO", "1.150 \\u20A7" },
    {"it_IT_PREEURO", "\\u20A4 1.150" },
    {"pt_PT_PREEURO", "1,150$50 Esc."},
    {"en_US@currency=JPY", "\\u00A51,150"}
};
/**
 * Test localized currency patterns.
 */
void
NumberFormatTest::TestCurrency(void)
{
    UErrorCode status = U_ZERO_ERROR;
    NumberFormat* currencyFmt = NumberFormat::createCurrencyInstance(Locale::getCanadaFrench(), status);
    if (U_FAILURE(status)) {
        dataerrln("Error calling NumberFormat::createCurrencyInstance()");
        return;
    }

    UnicodeString s; currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre ici a..........." + s);
    if (!(s=="1,50 $"))
        errln((UnicodeString)"FAIL: Expected 1,50 $");
    delete currencyFmt;
    s.truncate(0);
    char loc[256]={0};
    int len = uloc_canonicalize("de_DE_PREEURO", loc, 256, &status);
    currencyFmt = NumberFormat::createCurrencyInstance(Locale(loc),status);
    currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre en Allemagne a.." + s);
    if (!(s=="1,50 DM"))
        errln((UnicodeString)"FAIL: Expected 1,50 DM");
    delete currencyFmt;
    s.truncate(0);
    len = uloc_canonicalize("fr_FR_PREEURO", loc, 256, &status);
    currencyFmt = NumberFormat::createCurrencyInstance(Locale(loc), status);
    currencyFmt->format(1.50, s);
    logln((UnicodeString)"Un pauvre en France a....." + s);
    if (!(s=="1,50 F"))
        errln((UnicodeString)"FAIL: Expected 1,50 F");
    delete currencyFmt;
    if (U_FAILURE(status))
        errln((UnicodeString)"FAIL: Status " + (int32_t)status);
    
    for(int i=0; i < (int)(sizeof(testCases)/sizeof(testCases[i])); i++){
        status = U_ZERO_ERROR;
        const char *localeID = testCases[i][0];
        UnicodeString expected(testCases[i][1]);
        expected = expected.unescape();
        s.truncate(0);
        char loc[256]={0};
        uloc_canonicalize(localeID, loc, 256, &status);
        currencyFmt = NumberFormat::createCurrencyInstance(Locale(loc), status);
        if(U_FAILURE(status)){
            errln("Could not create currency formatter for locale %s",localeID);
            continue;
        }
        currencyFmt->format(1150.50, s);
        if(s!=expected){
            errln(UnicodeString("FAIL: Expected: ")+expected 
                    + UnicodeString(" Got: ") + s 
                    + UnicodeString( " for locale: ")+ UnicodeString(localeID) );
        }
        if (U_FAILURE(status)){
            errln((UnicodeString)"FAIL: Status " + (int32_t)status);
        }
        delete currencyFmt;
    }
}
 
// -------------------------------------

/**
 * Test the Currency object handling, new as of ICU 2.2.
 */
void NumberFormatTest::TestCurrencyObject() {
    UErrorCode ec = U_ZERO_ERROR;
    NumberFormat* fmt = 
        NumberFormat::createCurrencyInstance(Locale::getUS(), ec);

    if (U_FAILURE(ec)) {
        errln("FAIL: getCurrencyInstance(US)");
        delete fmt;
        return;
    }

    Locale null("", "", "");
        
    expectCurrency(*fmt, null, 1234.56, "$1,234.56");

    expectCurrency(*fmt, Locale::getFrance(),
                   1234.56, CharsToUnicodeString("\\u20AC1,234.56")); // Euro

    expectCurrency(*fmt, Locale::getJapan(),
                   1234.56, CharsToUnicodeString("\\u00A51,235")); // Yen

    expectCurrency(*fmt, Locale("fr", "CH", ""),
                   1234.56, "SwF1,234.55"); // 0.05 rounding

    expectCurrency(*fmt, Locale::getUS(),
                   1234.56, "$1,234.56");

    delete fmt;
    fmt = NumberFormat::createCurrencyInstance(Locale::getFrance(), ec);

    if (U_FAILURE(ec)) {
        errln("FAIL: getCurrencyInstance(FRANCE)");
        delete fmt;
        return;
    }
        
    expectCurrency(*fmt, null, 1234.56, CharsToUnicodeString("1 234,56 \\u20AC"));

    expectCurrency(*fmt, Locale::getJapan(),
                   1234.56, CharsToUnicodeString("1 235 \\u00A5JP")); // Yen

    expectCurrency(*fmt, Locale("fr", "CH", ""),
                   1234.56, "1 234,55 sFr."); // 0.05 rounding

    expectCurrency(*fmt, Locale::getUS(),
                   1234.56, "1 234,56 $US");

    expectCurrency(*fmt, Locale::getFrance(),
                   1234.56, CharsToUnicodeString("1 234,56 \\u20AC")); // Euro

    delete fmt;
}
    
// -------------------------------------

/**
 * Do rudimentary testing of parsing.
 */
void
NumberFormatTest::TestParse(void)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString arg("0");
    DecimalFormat* format = new DecimalFormat("00", status);
    //try {
        Formattable n; format->parse(arg, n, status);
        logln((UnicodeString)"parse(" + arg + ") = " + n.getLong());
        if (n.getType() != Formattable::kLong ||
            n.getLong() != 0) errln((UnicodeString)"FAIL: Expected 0");
    delete format;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
    //}
    //catch(Exception e) {
    //    errln((UnicodeString)"Exception caught: " + e);
    //}
}
 
// -------------------------------------

/**
 * Test proper rounding by the format method.
 */
void
NumberFormatTest::TestRounding487(void)
{
    UErrorCode status = U_ZERO_ERROR;
    NumberFormat *nf = NumberFormat::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("Error calling NumberFormat::createInstance()");
        return;
    }

    roundingTest(*nf, 0.00159999, 4, "0.0016");
    roundingTest(*nf, 0.00995, 4, "0.01");

    roundingTest(*nf, 12.3995, 3, "12.4");

    roundingTest(*nf, 12.4999, 0, "12");
    roundingTest(*nf, - 19.5, 0, "-20");
    delete nf;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: Status " + (int32_t)status);
}

/**
 * Test the functioning of the secondary grouping value.
 */
void NumberFormatTest::TestSecondaryGrouping(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols ct");

    DecimalFormat f("#,##,###", US, status);
    CHECK(status, "DecimalFormat ct");

    expect2(f, (int32_t)123456789L, "12,34,56,789");
    expectPat(f, "#,##,###");
    f.applyPattern("#,###", status);
    CHECK(status, "applyPattern");

    f.setSecondaryGroupingSize(4);
    expect2(f, (int32_t)123456789L, "12,3456,789");
    expectPat(f, "#,####,###");
    NumberFormat *g = NumberFormat::createInstance(Locale("hi", "IN"), status);
    CHECK(status, "createInstance(hi_IN)");

    UnicodeString out;
    int32_t l = (int32_t)1876543210L;
    g->format(l, out);
    delete g;
    // expect "1,87,65,43,210", but with Hindi digits
    //         01234567890123
    UBool ok = TRUE;
    if (out.length() != 14) {
        ok = FALSE;
    } else {
        for (int32_t i=0; i<out.length(); ++i) {
            UBool expectGroup = FALSE;
            switch (i) {
            case 1:
            case 4:
            case 7:
            case 10:
                expectGroup = TRUE;
                break;
            }
            // Later -- fix this to get the actual grouping
            // character from the resource bundle.
            UBool isGroup = (out.charAt(i) == 0x002C);
            if (isGroup != expectGroup) {
                ok = FALSE;
                break;
            }
        }
    }
    if (!ok) {
        errln((UnicodeString)"FAIL  Expected " + l +
              " x hi_IN -> \"1,87,65,43,210\" (with Hindi digits), got \"" +
              escape(out) + "\"");
    } else {
        logln((UnicodeString)"Ok    " + l +
              " x hi_IN -> \"" +
              escape(out) + "\"");
    }
}

void NumberFormatTest::TestWhiteSpaceParsing(void) {
    UErrorCode ec = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), ec);
    DecimalFormat fmt("a  b#0c  ", US, ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: Constructor");
        return;
    }
    int32_t n = 1234;
    expect(fmt, "a b1234c ", n);
    expect(fmt, "a   b1234c   ", n);
}

/**
 * Test currencies whose display name is a ChoiceFormat.
 */
void NumberFormatTest::TestComplexCurrency() {
    UErrorCode ec = U_ZERO_ERROR;
    Locale loc("en", "IN", "");
    NumberFormat* fmt = NumberFormat::createCurrencyInstance(loc, ec);
    if (U_SUCCESS(ec)) {
        expect2(*fmt, 1.0, "Re. 1.00");
        // Use .00392625 because that's 2^-8.  Any value less than 0.005 is fine.
        expect(*fmt, 1.00390625, "Re. 1.00"); // tricky
        expect2(*fmt, 12345678.0, "Rs. 1,23,45,678.00");
        expect2(*fmt, 0.5, "Rs. 0.50");
        expect2(*fmt, -1.0, "-Re. 1.00");
        expect2(*fmt, -10.0, "-Rs. 10.00");
    } else {
        errln("FAIL: getCurrencyInstance(en_IN)");
    }
    delete fmt;
}
    
// -------------------------------------
 
void
NumberFormatTest::roundingTest(NumberFormat& nf, double x, int32_t maxFractionDigits, const char* expected)
{
    nf.setMaximumFractionDigits(maxFractionDigits);
    UnicodeString out; nf.format(x, out);
    logln((UnicodeString)"" + x + " formats with " + maxFractionDigits + " fractional digits to " + out);
    if (!(out==expected)) errln((UnicodeString)"FAIL: Expected " + expected);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestExponent(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols constructor");
    DecimalFormat fmt1(UnicodeString("0.###E0"), US, status);
    CHECK(status, "DecimalFormat(0.###E0)");
    DecimalFormat fmt2(UnicodeString("0.###E+0"), US, status);
    CHECK(status, "DecimalFormat(0.###E+0)");
    int32_t n = 1234;
    expect2(fmt1, n, "1.234E3");
    expect2(fmt2, n, "1.234E+3");
    expect(fmt1, "1.234E+3", n); // Either format should parse "E+3"
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestScientific(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols constructor");

    // Test pattern round-trip
    const char* PAT[] = { "#E0", "0.####E0", "00.000E00", "##0.####E000",
                          "0.###E0;[0.###E0]" };
    int32_t PAT_length = (int32_t)(sizeof(PAT) / sizeof(PAT[0]));
    int32_t DIGITS[] = {
        // min int, max int, min frac, max frac
        0, 1, 0, 0, // "#E0"
        1, 1, 0, 4, // "0.####E0"
        2, 2, 3, 3, // "00.000E00"
        1, 3, 0, 4, // "##0.####E000"
        1, 1, 0, 3, // "0.###E0;[0.###E0]"
    };
    for (int32_t i=0; i<PAT_length; ++i) {
        UnicodeString pat(PAT[i]);
        DecimalFormat df(pat, US, status);
        CHECK(status, "DecimalFormat constructor");
        UnicodeString pat2;
        df.toPattern(pat2);
        if (pat == pat2) {
            logln(UnicodeString("Ok   Pattern rt \"") +
                  pat + "\" -> \"" +
                  pat2 + "\"");
        } else {
            errln(UnicodeString("FAIL Pattern rt \"") +
                  pat + "\" -> \"" +
                  pat2 + "\"");
        }
        // Make sure digit counts match what we expect
        if (df.getMinimumIntegerDigits() != DIGITS[4*i] ||
            df.getMaximumIntegerDigits() != DIGITS[4*i+1] ||
            df.getMinimumFractionDigits() != DIGITS[4*i+2] ||
            df.getMaximumFractionDigits() != DIGITS[4*i+3]) {
            errln(UnicodeString("FAIL \"" + pat +
                                "\" min/max int; min/max frac = ") +
                  df.getMinimumIntegerDigits() + "/" +
                  df.getMaximumIntegerDigits() + ";" +
                  df.getMinimumFractionDigits() + "/" +
                  df.getMaximumFractionDigits() + ", expect " +
                  DIGITS[4*i] + "/" +
                  DIGITS[4*i+1] + ";" +
                  DIGITS[4*i+2] + "/" +
                  DIGITS[4*i+3]);
        }
    }


    // Test the constructor for default locale. We have to
    // manually set the default locale, as there is no 
    // guarantee that the default locale has the same 
    // scientific format.
    Locale def = Locale::getDefault();
    Locale::setDefault(Locale::getUS(), status);
    expect2(NumberFormat::createScientificInstance(status),
           12345.678901,
           "1.2345678901E4", status);
    Locale::setDefault(def, status);

    expect2(new DecimalFormat("#E0", US, status),
           12345.0,
           "1.2345E4", status);
    expect(new DecimalFormat("0E0", US, status),
           12345.0,
           "1E4", status);
    expect2(NumberFormat::createScientificInstance(Locale::getUS(), status),
           12345.678901,
           "1.2345678901E4", status);
    expect(new DecimalFormat("##0.###E0", US, status),
           12345.0,
           "12.34E3", status);
    expect(new DecimalFormat("##0.###E0", US, status),
           12345.00001,
           "12.35E3", status);
    expect2(new DecimalFormat("##0.####E0", US, status),
           (int32_t) 12345,
           "12.345E3", status);
    expect2(NumberFormat::createScientificInstance(Locale::getFrance(), status),
           12345.678901,
           "1,2345678901E4", status);
    expect(new DecimalFormat("##0.####E0", US, status),
           789.12345e-9,
           "789.12E-9", status);
    expect2(new DecimalFormat("##0.####E0", US, status),
           780.e-9,
           "780E-9", status);
    expect(new DecimalFormat(".###E0", US, status),
           45678.0,
           ".457E5", status);
    expect2(new DecimalFormat(".###E0", US, status),
           (int32_t) 0,
           ".0E0", status);
    /*
    expect(new DecimalFormat[] { new DecimalFormat("#E0", US),
                                 new DecimalFormat("##E0", US),
                                 new DecimalFormat("####E0", US),
                                 new DecimalFormat("0E0", US),    
                                 new DecimalFormat("00E0", US),   
                                 new DecimalFormat("000E0", US), 
                               },
           new Long(45678000),
           new String[] { "4.5678E7",
                          "45.678E6",
                          "4567.8E4",
                          "5E7",
                          "46E6",  
                          "457E5",
                        }
           );
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect2(new DecimalFormat("#E0", US, status),
           (int32_t) 45678000, "4.5678E7", status);
    expect2(new DecimalFormat("##E0", US, status),
           (int32_t) 45678000, "45.678E6", status);
    expect2(new DecimalFormat("####E0", US, status),
           (int32_t) 45678000, "4567.8E4", status);
    expect(new DecimalFormat("0E0", US, status),
           (int32_t) 45678000, "5E7", status);
    expect(new DecimalFormat("00E0", US, status),
           (int32_t) 45678000, "46E6", status);
    expect(new DecimalFormat("000E0", US, status),
           (int32_t) 45678000, "457E5", status);
    /*
    expect(new DecimalFormat("###E0", US, status),
           new Object[] { new Double(0.0000123), "12.3E-6",
                          new Double(0.000123), "123E-6",
                          new Double(0.00123), "1.23E-3",
                          new Double(0.0123), "12.3E-3",
                          new Double(0.123), "123E-3",
                          new Double(1.23), "1.23E0",
                          new Double(12.3), "12.3E0",
                          new Double(123), "123E0",
                          new Double(1230), "1.23E3",
                         });
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect2(new DecimalFormat("###E0", US, status),
           0.0000123, "12.3E-6", status);
    expect2(new DecimalFormat("###E0", US, status),
           0.000123, "123E-6", status);
    expect2(new DecimalFormat("###E0", US, status),
           0.00123, "1.23E-3", status);
    expect2(new DecimalFormat("###E0", US, status),
           0.0123, "12.3E-3", status);
    expect2(new DecimalFormat("###E0", US, status),
           0.123, "123E-3", status);
    expect2(new DecimalFormat("###E0", US, status),
           1.23, "1.23E0", status);
    expect2(new DecimalFormat("###E0", US, status),
           12.3, "12.3E0", status);
    expect2(new DecimalFormat("###E0", US, status),
           123.0, "123E0", status);
    expect2(new DecimalFormat("###E0", US, status),
           1230.0, "1.23E3", status);
    /*
    expect(new DecimalFormat("0.#E+00", US, status),
           new Object[] { new Double(0.00012), "1.2E-04",
                          new Long(12000),     "1.2E+04",
                         });
    !
    ! Unroll this test into individual tests below...
    !
    */
    expect2(new DecimalFormat("0.#E+00", US, status),
           0.00012, "1.2E-04", status);
    expect2(new DecimalFormat("0.#E+00", US, status),
           (int32_t) 12000, "1.2E+04", status);
}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestPad(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols constructor");

    expect2(new DecimalFormat("*^##.##", US, status),
           int32_t(0), "^^^^0", status);
    expect2(new DecimalFormat("*^##.##", US, status),
           -1.3, "^-1.3", status);
    expect2(new DecimalFormat("##0.0####E0*_ 'g-m/s^2'", US, status),
           int32_t(0), "0.0E0______ g-m/s^2", status);
    expect(new DecimalFormat("##0.0####E0*_ 'g-m/s^2'", US, status),
           1.0/3, "333.333E-3_ g-m/s^2", status);
    expect2(new DecimalFormat("##0.0####*_ 'g-m/s^2'", US, status),
           int32_t(0), "0.0______ g-m/s^2", status);
    expect(new DecimalFormat("##0.0####*_ 'g-m/s^2'", US, status),
           1.0/3, "0.33333__ g-m/s^2", status);

    // Test padding before a sign
    const char *formatStr = "*x#,###,###,##0.0#;*x(###,###,##0.0#)";
    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(-10),  "xxxxxxxxxx(10.0)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(-1000),"xxxxxxx(1,000.0)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(-1000000),"xxx(1,000,000.0)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           -100.37,       "xxxxxxxx(100.37)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           -10456.37,     "xxxxx(10,456.37)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           -1120456.37,   "xx(1,120,456.37)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           -112045600.37, "(112,045,600.37)", status);
    expect2(new DecimalFormat(formatStr, US, status),
           -1252045600.37,"(1,252,045,600.37)", status);

    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(10),  "xxxxxxxxxxxx10.0", status);
    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(1000),"xxxxxxxxx1,000.0", status);
    expect2(new DecimalFormat(formatStr, US, status),
           int32_t(1000000),"xxxxx1,000,000.0", status);
    expect2(new DecimalFormat(formatStr, US, status),
           100.37,       "xxxxxxxxxx100.37", status);
    expect2(new DecimalFormat(formatStr, US, status),
           10456.37,     "xxxxxxx10,456.37", status);
    expect2(new DecimalFormat(formatStr, US, status),
           1120456.37,   "xxxx1,120,456.37", status);
    expect2(new DecimalFormat(formatStr, US, status),
           112045600.37, "xx112,045,600.37", status);
    expect2(new DecimalFormat(formatStr, US, status),
           10252045600.37,"10,252,045,600.37", status);


    // Test padding between a sign and a number
    const char *formatStr2 = "#,###,###,##0.0#*x;(###,###,##0.0#*x)";
    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(-10),  "(10.0xxxxxxxxxx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(-1000),"(1,000.0xxxxxxx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(-1000000),"(1,000,000.0xxx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           -100.37,       "(100.37xxxxxxxx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           -10456.37,     "(10,456.37xxxxx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           -1120456.37,   "(1,120,456.37xx)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           -112045600.37, "(112,045,600.37)", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           -1252045600.37,"(1,252,045,600.37)", status);

    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(10),  "10.0xxxxxxxxxxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(1000),"1,000.0xxxxxxxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           int32_t(1000000),"1,000,000.0xxxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           100.37,       "100.37xxxxxxxxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           10456.37,     "10,456.37xxxxxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           1120456.37,   "1,120,456.37xxxx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           112045600.37, "112,045,600.37xx", status);
    expect2(new DecimalFormat(formatStr2, US, status),
           10252045600.37,"10,252,045,600.37", status);

    //testing the setPadCharacter(UnicodeString) and getPadCharacterString()
    DecimalFormat fmt("#", US, status);
    CHECK(status, "DecimalFormat constructor");
    UnicodeString padString("P");
    fmt.setPadCharacter(padString);
    expectPad(fmt, "*P##.##", DecimalFormat::kPadBeforePrefix, 5, padString);
    fmt.setPadCharacter((UnicodeString)"^");
    expectPad(fmt, "*^#", DecimalFormat::kPadBeforePrefix, 1, (UnicodeString)"^");
    //commented untill implementation is complete
  /*  fmt.setPadCharacter((UnicodeString)"^^^");
    expectPad(fmt, "*^^^#", DecimalFormat::kPadBeforePrefix, 3, (UnicodeString)"^^^");
    padString.remove();
    padString.append((UChar)0x0061);
    padString.append((UChar)0x0302);
    fmt.setPadCharacter(padString);
    UChar patternChars[]={0x002a, 0x0061, 0x0302, 0x0061, 0x0302, 0x0023, 0x0000};
    UnicodeString pattern(patternChars);
    expectPad(fmt, pattern , DecimalFormat::kPadBeforePrefix, 4, padString);
 */

}

/**
 * Upgrade to alphaWorks
 */
void NumberFormatTest::TestPatterns2(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols US(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols constructor");

    DecimalFormat fmt("#", US, status);
    CHECK(status, "DecimalFormat constructor");
    
    UChar hat = 0x005E; /*^*/

    expectPad(fmt, "*^#", DecimalFormat::kPadBeforePrefix, 1, hat);
    expectPad(fmt, "$*^#", DecimalFormat::kPadAfterPrefix, 2, hat);
    expectPad(fmt, "#*^", DecimalFormat::kPadBeforeSuffix, 1, hat);
    expectPad(fmt, "#$*^", DecimalFormat::kPadAfterSuffix, 2, hat);
    expectPad(fmt, "$*^$#", ILLEGAL);
    expectPad(fmt, "#$*^$", ILLEGAL);
    expectPad(fmt, "'pre'#,##0*x'post'", DecimalFormat::kPadBeforeSuffix,
              12, (UChar)0x0078 /*x*/);
    expectPad(fmt, "''#0*x", DecimalFormat::kPadBeforeSuffix,
              3, (UChar)0x0078 /*x*/);
    expectPad(fmt, "'I''ll'*a###.##", DecimalFormat::kPadAfterPrefix,
              10, (UChar)0x0061 /*a*/);

    fmt.applyPattern("AA#,##0.00ZZ", status);
    CHECK(status, "applyPattern");
    fmt.setPadCharacter(hat);

    fmt.setFormatWidth(10);

    fmt.setPadPosition(DecimalFormat::kPadBeforePrefix);
    expectPat(fmt, "*^AA#,##0.00ZZ");

    fmt.setPadPosition(DecimalFormat::kPadBeforeSuffix);
    expectPat(fmt, "AA#,##0.00*^ZZ");

    fmt.setPadPosition(DecimalFormat::kPadAfterSuffix);
    expectPat(fmt, "AA#,##0.00ZZ*^");

    //            12  3456789012
    UnicodeString exp("AA*^#,##0.00ZZ", "");
    fmt.setFormatWidth(12);
    fmt.setPadPosition(DecimalFormat::kPadAfterPrefix);
    expectPat(fmt, exp);

    fmt.setFormatWidth(13);
    //              12  34567890123
    expectPat(fmt, "AA*^##,##0.00ZZ");

    fmt.setFormatWidth(14);
    //              12  345678901234
    expectPat(fmt, "AA*^###,##0.00ZZ");

    fmt.setFormatWidth(15);
    //              12  3456789012345
    expectPat(fmt, "AA*^####,##0.00ZZ"); // This is the interesting case

    fmt.setFormatWidth(16);
    //              12  34567890123456
    expectPat(fmt, "AA*^#,###,##0.00ZZ");
}

void NumberFormatTest::TestSurrogateSupport(void) {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatSymbols custom(Locale::getUS(), status);
    CHECK(status, "DecimalFormatSymbols constructor");

    custom.setSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol, "decimal");
    custom.setSymbol(DecimalFormatSymbols::kPlusSignSymbol, "plus");
    custom.setSymbol(DecimalFormatSymbols::kMinusSignSymbol, " minus ");
    custom.setSymbol(DecimalFormatSymbols::kExponentialSymbol, "exponent");

    UnicodeString patternStr("*\\U00010000##.##", "");
    patternStr = patternStr.unescape();
    UnicodeString expStr("\\U00010000\\U00010000\\U00010000\\U000100000", "");
    expStr = expStr.unescape();
    expect2(new DecimalFormat(patternStr, custom, status),
           int32_t(0), expStr, status);

    status = U_ZERO_ERROR;
    expect2(new DecimalFormat("*^##.##", custom, status),
           int32_t(0), "^^^^0", status);
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat("##.##", custom, status),
           -1.3, " minus 1decimal3", status);
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat("##0.0####E0 'g-m/s^2'", custom, status),
           int32_t(0), "0decimal0exponent0 g-m/s^2", status);
    status = U_ZERO_ERROR;
    expect(new DecimalFormat("##0.0####E0 'g-m/s^2'", custom, status),
           1.0/3, "333decimal333exponent minus 3 g-m/s^2", status);
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat("##0.0#### 'g-m/s^2'", custom, status),
           int32_t(0), "0decimal0 g-m/s^2", status);
    status = U_ZERO_ERROR;
    expect(new DecimalFormat("##0.0#### 'g-m/s^2'", custom, status),
           1.0/3, "0decimal33333 g-m/s^2", status);

    UnicodeString zero((UChar32)0x10000);
    custom.setSymbol(DecimalFormatSymbols::kZeroDigitSymbol, zero);
    expStr = UnicodeString("\\U00010001decimal\\U00010002\\U00010005\\U00010000", "");
    expStr = expStr.unescape();
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat("##0.000", custom, status),
           1.25, expStr, status);

    custom.setSymbol(DecimalFormatSymbols::kZeroDigitSymbol, (UChar)0x30);
    custom.setSymbol(DecimalFormatSymbols::kCurrencySymbol, "units of money");
    custom.setSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol, "money separator");
    patternStr = "0.00 \\u00A4' in your bank account'";
    patternStr = patternStr.unescape();
    expStr = UnicodeString(" minus 20money separator00 units of money in your bank account", "");
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat(patternStr, custom, status),
           int32_t(-20), expStr, status);

    custom.setSymbol(DecimalFormatSymbols::kPercentSymbol, "percent");
    patternStr = "'You''ve lost ' -0.00 %' of your money today'";
    patternStr = patternStr.unescape();
    expStr = UnicodeString(" minus You've lost   minus 2000decimal00 percent of your money today", "");
    status = U_ZERO_ERROR;
    expect2(new DecimalFormat(patternStr, custom, status),
           int32_t(-20), expStr, status);
}

void NumberFormatTest::TestCurrencyPatterns(void) {
    int32_t i, locCount;
    const Locale* locs = NumberFormat::getAvailableLocales(locCount);
    for (i=0; i<locCount; ++i) {
        UErrorCode ec = U_ZERO_ERROR;
        NumberFormat* nf = NumberFormat::createCurrencyInstance(locs[i], ec);
        if (U_FAILURE(ec)) {
            errln("FAIL: Can't create NumberFormat(%s) - %s", locs[i].getName(), u_errorName(ec));
        } else {
            // Make sure currency formats do not have a variable number
            // of fraction digits
            int32_t min = nf->getMinimumFractionDigits();
            int32_t max = nf->getMaximumFractionDigits();
            if (min != max) {
                UnicodeString a, b;
                nf->format(1.0, a);
                nf->format(1.125, b);
                errln((UnicodeString)"FAIL: " + locs[i].getName() +
                      " min fraction digits != max fraction digits; "
                      "x 1.0 => " + escape(a) +
                      "; x 1.125 => " + escape(b));
            }

            // Make sure EURO currency formats have exactly 2 fraction digits
            if (nf->getDynamicClassID() == DecimalFormat::getStaticClassID()) {
                DecimalFormat* df = (DecimalFormat*) nf;
                if (u_strcmp(EUR, df->getCurrency()) == 0) {
                    if (min != 2 || max != 2) {
                        UnicodeString a;
                        nf->format(1.0, a);
                        errln((UnicodeString)"FAIL: " + locs[i].getName() +
                              " is a EURO format but it does not have 2 fraction digits; "
                              "x 1.0 => " +
                              escape(a));
                    }
                }
            }
        }
        delete nf;
    }
}

void NumberFormatTest::TestRegCurrency(void) {
#if !UCONFIG_NO_SERVICE
    UErrorCode status = U_ZERO_ERROR;
    UChar USD[4];
    ucurr_forLocale("en_US", USD, 4, &status);
    UChar YEN[4];
    ucurr_forLocale("ja_JP", YEN, 4, &status);
    UChar TMP[4];
    static const UChar QQQ[] = {0x51, 0x51, 0x51, 0};
    if(U_FAILURE(status)) {
        errln("Unable to get currency for locale, error %s", u_errorName(status));
        return;
    }
    
    UCurrRegistryKey enkey = ucurr_register(YEN, "en_US", &status);
    UCurrRegistryKey enUSEUROkey = ucurr_register(QQQ, "en_US_EURO", &status);
    
    ucurr_forLocale("en_US", TMP, 4, &status);
    if (u_strcmp(YEN, TMP) != 0) {
        errln("FAIL: didn't return YEN registered for en_US");
    }

    ucurr_forLocale("en_US_EURO", TMP, 4, &status);
    if (u_strcmp(QQQ, TMP) != 0) {
        errln("FAIL: didn't return QQQ for en_US_EURO");
    }
    
    int32_t fallbackLen = ucurr_forLocale("en_XX_BAR", TMP, 4, &status);
    if (fallbackLen) {
        errln("FAIL: tried to fallback en_XX_BAR");
    }
    status = U_ZERO_ERROR; // reset
    
    if (!ucurr_unregister(enkey, &status)) {
        errln("FAIL: couldn't unregister enkey");
    }

    ucurr_forLocale("en_US", TMP, 4, &status);        
    if (u_strcmp(USD, TMP) != 0) {
        errln("FAIL: didn't return USD for en_US after unregister of en_US");
    }
    status = U_ZERO_ERROR; // reset
    
    ucurr_forLocale("en_US_EURO", TMP, 4, &status);
    if (u_strcmp(QQQ, TMP) != 0) {
        errln("FAIL: didn't return QQQ for en_US_EURO after unregister of en_US");
    }
    
    ucurr_forLocale("en_US_BLAH", TMP, 4, &status);
    if (u_strcmp(USD, TMP) != 0) {
        errln("FAIL: could not find USD for en_US_BLAH after unregister of en");
    }
    status = U_ZERO_ERROR; // reset
    
    if (!ucurr_unregister(enUSEUROkey, &status)) {
        errln("FAIL: couldn't unregister enUSEUROkey");
    }
    
    ucurr_forLocale("en_US_EURO", TMP, 4, &status);
    if (u_strcmp(EUR, TMP) != 0) {
        errln("FAIL: didn't return EUR for en_US_EURO after unregister of en_US_EURO");
    }
    status = U_ZERO_ERROR; // reset
#endif
}

void NumberFormatTest::TestCurrencyNames(void) {
    // Do a basic check of getName()
    // USD { "US$", "US Dollar"            } // 04/04/1792-
    UErrorCode ec = U_ZERO_ERROR;
    static const UChar USD[] = {0x55, 0x53, 0x44, 0}; /*USD*/
    static const UChar USX[] = {0x55, 0x53, 0x58, 0}; /*USX*/
    static const UChar CAD[] = {0x43, 0x41, 0x44, 0}; /*CAD*/
    static const UChar ITL[] = {0x49, 0x54, 0x4C, 0}; /*ITL*/
    UBool isChoiceFormat;
    int32_t len;
    // Warning: HARD-CODED LOCALE DATA in this test.  If it fails, CHECK
    // THE LOCALE DATA before diving into the code.
    assertEquals("USD.getName(SYMBOL_NAME)",
                 UnicodeString("$"),
                 UnicodeString(ucurr_getName(USD, "en",
                                             UCURR_SYMBOL_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("USD.getName(LONG_NAME)",
                 UnicodeString("US Dollar"),
                 UnicodeString(ucurr_getName(USD, "en",
                                             UCURR_LONG_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("CAD.getName(SYMBOL_NAME)",
                 UnicodeString("Can$"),
                 UnicodeString(ucurr_getName(CAD, "en",
                                             UCURR_SYMBOL_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("CAD.getName(SYMBOL_NAME)",
                 UnicodeString("$"),
                 UnicodeString(ucurr_getName(CAD, "en_CA",
                                             UCURR_SYMBOL_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("USD.getName(SYMBOL_NAME)",
                 UnicodeString("US$"),
                 UnicodeString(ucurr_getName(USD, "en_AU",
                                             UCURR_SYMBOL_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("CAD.getName(SYMBOL_NAME)",
                 UnicodeString("Can$"),
                 UnicodeString(ucurr_getName(CAD, "en_AU",
                                             UCURR_SYMBOL_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertEquals("USX.getName(LONG_NAME)",
                 UnicodeString("USX"),
                 UnicodeString(ucurr_getName(USX, "en_US",
                                             UCURR_LONG_NAME,
                                             &isChoiceFormat, &len, &ec)));
    assertSuccess("ucurr_getName", ec);
    
    ec = U_ZERO_ERROR;

    // Test that a default or fallback warning is being returned. JB 4239.
    ucurr_getName(CAD, "es_ES", UCURR_LONG_NAME, &isChoiceFormat,
                            &len, &ec);
    assertTrue("ucurr_getName (fallback)",
                    U_USING_FALLBACK_WARNING == ec, TRUE);

    ucurr_getName(CAD, "zh_TW", UCURR_LONG_NAME, &isChoiceFormat,
                            &len, &ec);
    assertTrue("ucurr_getName (fallback)",
                    U_USING_FALLBACK_WARNING == ec, TRUE);

    ucurr_getName(CAD, "en_US", UCURR_LONG_NAME, &isChoiceFormat,
                            &len, &ec);
    assertTrue("ucurr_getName (default)",
                    U_USING_DEFAULT_WARNING == ec, TRUE);
    
    ucurr_getName(CAD, "vi", UCURR_LONG_NAME, &isChoiceFormat,
                            &len, &ec);
    assertTrue("ucurr_getName (default)",
                    U_USING_DEFAULT_WARNING == ec, TRUE);
    
    // Test that a default warning is being returned when falling back to root. JB 4536.
    ucurr_getName(ITL, "cy", UCURR_LONG_NAME, &isChoiceFormat,
                            &len, &ec);
    assertTrue("ucurr_getName (default to root)",
                    U_USING_DEFAULT_WARNING == ec, TRUE);
    
    // TODO add more tests later
}

void NumberFormatTest::TestCurrencyUnit(void){
    UErrorCode ec = U_ZERO_ERROR;
    static const UChar USD[] = {85, 83, 68, 0}; /*USD*/
    CurrencyUnit cu(USD, ec);
    assertSuccess("CurrencyUnit", ec);

    const UChar * r = cu.getISOCurrency(); // who is the buffer owner ?
    assertEquals("getISOCurrency()", USD, r);

    CurrencyUnit cu2(cu);
    if (!(cu2 == cu)){
        errln("CurrencyUnit copy constructed object should be same");
    }

    CurrencyUnit * cu3 = (CurrencyUnit *)cu.clone();
    if (!(*cu3 == cu)){
        errln("CurrencyUnit cloned object should be same");
    }
    delete cu3;
}

void NumberFormatTest::TestCurrencyAmount(void){
    UErrorCode ec = U_ZERO_ERROR;
    static const UChar USD[] = {85, 83, 68, 0}; /*USD*/
    CurrencyAmount ca(9, USD, ec);
    assertSuccess("CurrencyAmount", ec);

    CurrencyAmount ca2(ca);
    if (!(ca2 == ca)){
        errln("CurrencyAmount copy constructed object should be same");
    }

    ca2=ca;
    if (!(ca2 == ca)){
        errln("CurrencyAmount assigned object should be same");
    }
    
    CurrencyAmount *ca3 = (CurrencyAmount *)ca.clone();
    if (!(*ca3 == ca)){
        errln("CurrencyAmount cloned object should be same");
    }
    delete ca3;
}

void NumberFormatTest::TestSymbolsWithBadLocale(void) {
    Locale locDefault;
    Locale locBad("x-crazy_ZZ_MY_SPECIAL_ADMINISTRATION_REGION_NEEDS_A_SPECIAL_VARIANT_WITH_A_REALLY_REALLY_REALLY_REALLY_REALLY_REALLY_REALLY_LONG_NAME");
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString intlCurrencySymbol((UChar)0xa4);

    intlCurrencySymbol.append((UChar)0xa4);

    logln("Current locale is %s", Locale::getDefault().getName());
    Locale::setDefault(locBad, status);
    logln("Current locale is %s", Locale::getDefault().getName());
    DecimalFormatSymbols mySymbols(status);
    if (status != U_USING_FALLBACK_WARNING) {
        errln("DecimalFormatSymbols should returned U_USING_FALLBACK_WARNING.");
    }
    if (strcmp(mySymbols.getLocale().getName(), locBad.getName()) != 0) {
        errln("DecimalFormatSymbols does not have the right locale.");
    }
    int symbolEnum = (int)DecimalFormatSymbols::kDecimalSeparatorSymbol;
    for (; symbolEnum < (int)DecimalFormatSymbols::kFormatSymbolCount; symbolEnum++) {
        logln(UnicodeString("DecimalFormatSymbols[") + symbolEnum + UnicodeString("] = ")
            + prettify(mySymbols.getSymbol((DecimalFormatSymbols::ENumberFormatSymbol)symbolEnum)));

        if (mySymbols.getSymbol((DecimalFormatSymbols::ENumberFormatSymbol)symbolEnum).length() == 0
            && symbolEnum != (int)DecimalFormatSymbols::kGroupingSeparatorSymbol
            && symbolEnum != (int)DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol)
        {
            errln("DecimalFormatSymbols has an empty string at index %d.", symbolEnum);
        }
    }
    status = U_ZERO_ERROR;
    Locale::setDefault(locDefault, status);
    logln("Current locale is %s", Locale::getDefault().getName());
}

/**
 * Check that adoptDecimalFormatSymbols and setDecimalFormatSymbols
 * behave the same, except for memory ownership semantics. (No
 * version of this test on Java, since Java has only one method.)
 */
void NumberFormatTest::TestAdoptDecimalFormatSymbols(void) {
    UErrorCode ec = U_ZERO_ERROR;
    DecimalFormatSymbols *sym = new DecimalFormatSymbols(Locale::getUS(), ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormatSymbols constructor");
        delete sym;
        return;
    }
    UnicodeString pat(" #,##0.00");
    pat.insert(0, (UChar)0x00A4);
    DecimalFormat fmt(pat, sym, ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormat constructor");
        return;
    }

    UnicodeString str;
    fmt.format(2350.75, str);
    if (str == "$ 2,350.75") {
        logln(str);
    } else {
        errln("Fail: " + str + ", expected $ 2,350.75");
    }

    sym = new DecimalFormatSymbols(Locale::getUS(), ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormatSymbols constructor");
        delete sym;
        return;
    }
    sym->setSymbol(DecimalFormatSymbols::kCurrencySymbol, "Q");
    fmt.adoptDecimalFormatSymbols(sym);

    str.truncate(0);
    fmt.format(2350.75, str);
    if (str == "Q 2,350.75") {
        logln(str);
    } else {
        errln("Fail: adoptDecimalFormatSymbols -> " + str + ", expected Q 2,350.75");
    }

    sym = new DecimalFormatSymbols(Locale::getUS(), ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormatSymbols constructor");
        delete sym;
        return;
    }
    DecimalFormat fmt2(pat, sym, ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormat constructor");
        return;
    }
    
    DecimalFormatSymbols sym2(Locale::getUS(), ec);
    if (U_FAILURE(ec)) {
        errln("Fail: DecimalFormatSymbols constructor");
        return;
    }
    sym2.setSymbol(DecimalFormatSymbols::kCurrencySymbol, "Q");
    fmt2.setDecimalFormatSymbols(sym2);

    str.truncate(0);
    fmt2.format(2350.75, str);
    if (str == "Q 2,350.75") {
        logln(str);
    } else {
        errln("Fail: setDecimalFormatSymbols -> " + str + ", expected Q 2,350.75");
    }
}

void NumberFormatTest::TestPerMill() {
    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString str;
    DecimalFormat fmt(ctou("###.###\\u2030"), ec);
    if (!assertSuccess("DecimalFormat ct", ec)) return;
    assertEquals("0.4857 x ###.###\\u2030",
                 ctou("485.7\\u2030"), fmt.format(0.4857, str));
    
    DecimalFormatSymbols sym(Locale::getUS(), ec);
    sym.setSymbol(DecimalFormatSymbols::kPerMillSymbol, ctou("m"));
    DecimalFormat fmt2("", sym, ec);
    fmt2.applyLocalizedPattern("###.###m", ec);
    if (!assertSuccess("setup", ec)) return;
    str.truncate(0);
    assertEquals("0.4857 x ###.###m",
                 "485.7m", fmt2.format(0.4857, str));
}

/**
 * Generic test for patterns that should be legal/illegal.
 */
void NumberFormatTest::TestIllegalPatterns() {
    // Test cases:
    // Prefix with "-:" for illegal patterns
    // Prefix with "+:" for legal patterns
    const char* DATA[] = {
        // Unquoted special characters in the suffix are illegal
        "-:000.000|###",
        "+:000.000'|###'",
        0
    };
    for (int32_t i=0; DATA[i]; ++i) {
        const char* pat=DATA[i];
        UBool valid = (*pat) == '+';
        pat += 2;
        UErrorCode ec = U_ZERO_ERROR;
        DecimalFormat fmt(pat, ec); // locale doesn't matter here
        if (U_SUCCESS(ec) == valid) {
            logln("Ok: pattern \"%s\": %s",
                  pat, u_errorName(ec));
        } else {
            errln("FAIL: pattern \"%s\" should have %s; got %s",
                  pat, (valid?"succeeded":"failed"),
                  u_errorName(ec));
        }
    }
}

//----------------------------------------------------------------------

static const char* KEYWORDS[] = {
    /*0*/ "ref=", // <reference pattern to parse numbers>
    /*1*/ "loc=", // <locale for formats>
    /*2*/ "f:",   // <pattern or '-'> <number> <exp. string>
    /*3*/ "fp:",  // <pattern or '-'> <number> <exp. string> <exp. number>
    /*4*/ "rt:",  // <pattern or '-'> <(exp.) number> <(exp.) string>
    /*5*/ "p:",   // <pattern or '-'> <string> <exp. number>
    /*6*/ "perr:", // <pattern or '-'> <invalid string>
    /*7*/ "pat:", // <pattern or '-'> <exp. toPattern or '-' or 'err'>
    /*8*/ "fpc:", // <pattern or '-'> <curr.amt> <exp. string> <exp. curr.amt>
    0
};

/**
 * Return an integer representing the next token from this
 * iterator.  The integer will be an index into the given list, or
 * -1 if there are no more tokens, or -2 if the token is not on
 * the list.
 */
static int32_t keywordIndex(const UnicodeString& tok) {
    for (int32_t i=0; KEYWORDS[i]!=0; ++i) {
        if (tok==KEYWORDS[i]) {
            return i;
        }
    }
    return -1;
}

/**
 * Parse a CurrencyAmount using the given NumberFormat, with
 * the 'delim' character separating the number and the currency.
 */
static void parseCurrencyAmount(const UnicodeString& str,
                                const NumberFormat& fmt,
                                UChar delim,
                                Formattable& result,
                                UErrorCode& ec) {
    UnicodeString num, cur;
    int32_t i = str.indexOf(delim);
    str.extractBetween(0, i, num);
    str.extractBetween(i+1, INT32_MAX, cur);
    Formattable n;
    fmt.parse(num, n, ec);
    result.adoptObject(new CurrencyAmount(n, cur.getTerminatedBuffer(), ec));
}

void NumberFormatTest::TestCases() {
    UErrorCode ec = U_ZERO_ERROR;
    TextFile reader("NumberFormatTestCases.txt", "UTF8", ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: Couldn't open NumberFormatTestCases.txt");
        return;
    }
    TokenIterator tokens(&reader);

    Locale loc("en", "US", "");
    DecimalFormat *ref = 0, *fmt = 0;
    MeasureFormat *mfmt = 0;
    UnicodeString pat, tok, mloc, str, out, where, currAmt;
    Formattable n;

    for (;;) {
        ec = U_ZERO_ERROR;
        if (!tokens.next(tok, ec)) {
            break;
        }
        where = UnicodeString("(") + tokens.getLineNumber() + ") ";
        int32_t cmd = keywordIndex(tok);
        switch (cmd) {
        case 0:
            // ref= <reference pattern>
            if (!tokens.next(tok, ec)) goto error;
            delete ref;
            ref = new DecimalFormat(tok,
                      new DecimalFormatSymbols(Locale::getUS(), ec), ec);
            if (U_FAILURE(ec)) {
                dataerrln("Error constructing DecimalFormat");
                goto error;
            }
            break;
        case 1:
            // loc= <locale>
            if (!tokens.next(tok, ec)) goto error;
            loc = Locale::createFromName(CharString(tok));
            break;
        case 2: // f:
        case 3: // fp:
        case 4: // rt:
        case 5: // p:
            if (!tokens.next(tok, ec)) goto error;
            if (tok != "-") {
                pat = tok;
                delete fmt;
                fmt = new DecimalFormat(pat, new DecimalFormatSymbols(loc, ec), ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: " + where + "Pattern \"" + pat + "\": " + u_errorName(ec));
                    ec = U_ZERO_ERROR;
                    if (!tokens.next(tok, ec)) goto error;
                    if (!tokens.next(tok, ec)) goto error;
                    if (cmd == 3) {
                        if (!tokens.next(tok, ec)) goto error;
                    }
                    continue;
                }
            }
            if (cmd == 2 || cmd == 3 || cmd == 4) {
                // f: <pattern or '-'> <number> <exp. string>
                // fp: <pattern or '-'> <number> <exp. string> <exp. number>
                // rt: <pattern or '-'> <number> <string>
                UnicodeString num;
                if (!tokens.next(num, ec)) goto error;
                if (!tokens.next(str, ec)) goto error;
                ref->parse(num, n, ec);
                assertSuccess("parse", ec);
                assertEquals(where + "\"" + pat + "\".format(" + num + ")",
                             str, fmt->format(n, out.remove(), ec));
                assertSuccess("format", ec);
                if (cmd == 3) { // fp:
                    if (!tokens.next(num, ec)) goto error;
                    ref->parse(num, n, ec);
                    assertSuccess("parse", ec);
                }
                if (cmd != 2) { // != f:
                    Formattable m;
                    fmt->parse(str, m, ec);
                    assertSuccess("parse", ec);
                    assertEquals(where + "\"" + pat + "\".parse(\"" + str + "\")",
                                 n, m);
                } 
            }
            // p: <pattern or '-'> <string to parse> <exp. number>
            else {
                UnicodeString expstr;
                if (!tokens.next(str, ec)) goto error;
                if (!tokens.next(expstr, ec)) goto error;
                Formattable exp, n;
                ref->parse(expstr, exp, ec);
                assertSuccess("parse", ec);
                fmt->parse(str, n, ec);
                assertSuccess("parse", ec);
                assertEquals(where + "\"" + pat + "\".parse(\"" + str + "\")",
                             exp, n);
            }
            break;
        case 8: // fpc:
            if (!tokens.next(tok, ec)) goto error;
            if (tok != "-") {
                mloc = tok;
                delete mfmt;
                mfmt = MeasureFormat::createCurrencyFormat(
                           Locale::createFromName(CharString(mloc)), ec);
                if (U_FAILURE(ec)) {
                    errln("FAIL: " + where + "Loc \"" + mloc + "\": " + u_errorName(ec));
                    ec = U_ZERO_ERROR;
                    if (!tokens.next(tok, ec)) goto error;
                    if (!tokens.next(tok, ec)) goto error;
                    if (!tokens.next(tok, ec)) goto error;
                    continue;
                }
            }
            // fpc: <loc or '-'> <curr.amt> <exp. string> <exp. curr.amt>
            if (!tokens.next(currAmt, ec)) goto error;
            if (!tokens.next(str, ec)) goto error;
            parseCurrencyAmount(currAmt, *ref, (UChar)0x2F/*'/'*/, n, ec);
            if (assertSuccess("parseCurrencyAmount", ec)) {
                assertEquals(where + "getCurrencyFormat(" + mloc + ").format(" + currAmt + ")",
                             str, mfmt->format(n, out.remove(), ec));
                assertSuccess("format", ec);
            }
            if (!tokens.next(currAmt, ec)) goto error;
            parseCurrencyAmount(currAmt, *ref, (UChar)0x2F/*'/'*/, n, ec);
            if (assertSuccess("parseCurrencyAmount", ec)) {
                Formattable m;
                mfmt->parseObject(str, m, ec);
                if (assertSuccess("parseCurrency", ec)) {
                    assertEquals(where + "getCurrencyFormat(" + mloc + ").parse(\"" + str + "\")",
                                 n, m);
                }
            }
            break;
        case 6:
            // perr: <pattern or '-'> <invalid string>
            errln("FAIL: Under construction");
            goto done;
        case 7: {
            // pat: <pattern> <exp. toPattern, or '-' or 'err'>
            UnicodeString testpat;
            UnicodeString exppat;
            if (!tokens.next(testpat, ec)) goto error;
            if (!tokens.next(exppat, ec)) goto error;
            UBool err = exppat == "err";
            UBool existingPat = FALSE;
            if (testpat == "-") {
                if (err) {
                    errln("FAIL: " + where + "Invalid command \"pat: - err\"");
                    continue;
                }
                existingPat = TRUE;
                testpat = pat;
            }
            if (exppat == "-") exppat = testpat;
            DecimalFormat* f = 0;
            UErrorCode ec2 = U_ZERO_ERROR;
            if (existingPat) {
                f = fmt;
            } else {
                f = new DecimalFormat(testpat, ec2);
            }
            if (U_SUCCESS(ec2)) {
                if (err) {
                    errln("FAIL: " + where + "Invalid pattern \"" + testpat +
                          "\" was accepted");
                } else {
                    UnicodeString pat2;
                    assertEquals(where + "\"" + testpat + "\".toPattern()",
                                 exppat, f->toPattern(pat2));
                }
            } else {
                if (err) {
                    logln("Ok: " + where + "Invalid pattern \"" + testpat +
                          "\" failed: " + u_errorName(ec2));
                } else {
                    errln("FAIL: " + where + "Valid pattern \"" + testpat +
                          "\" failed: " + u_errorName(ec2));
                }
            }
            if (!existingPat) delete f;
            } break;
        case -1:
            errln("FAIL: " + where + "Unknown command \"" + tok + "\"");
            goto done;
        }
    }
    goto done;

 error:
    if (U_SUCCESS(ec)) {
        errln("FAIL: Unexpected EOF");
    } else {
        errln("FAIL: " + where + "Unexpected " + u_errorName(ec));
    }

 done:
    delete mfmt;
    delete fmt;
    delete ref;
}


//----------------------------------------------------------------------
// Support methods
//----------------------------------------------------------------------

UBool NumberFormatTest::equalValue(const Formattable& a, const Formattable& b) {
    if (a.getType() == b.getType()) {
        return a == b;
    }

    if (a.getType() == Formattable::kLong) {
        if (b.getType() == Formattable::kInt64) {
            return a.getLong() == b.getLong();
        } else if (b.getType() == Formattable::kDouble) {
            return (double) a.getLong() == b.getDouble(); // TODO check use of double instead of long 
        }
    } else if (a.getType() == Formattable::kDouble) {
        if (b.getType() == Formattable::kLong) {
            return a.getDouble() == (double) b.getLong();
        } else if (b.getType() == Formattable::kInt64) {
            return a.getDouble() == (double)b.getInt64();
        }
    } else if (a.getType() == Formattable::kInt64) {
        if (b.getType() == Formattable::kLong) {
                return a.getInt64() == (int64_t)b.getLong();
        } else if (b.getType() == Formattable::kDouble) {
            return a.getInt64() == (int64_t)b.getDouble();
        }
    }
    return FALSE;
}

void NumberFormatTest::expect2(NumberFormat& fmt, const Formattable& n, const UnicodeString& str) {
    // Don't round-trip format test, since we explicitly do it
    expect(fmt, n, str, FALSE);
    expect(fmt, str, n);
}

void NumberFormatTest::expect2(NumberFormat* fmt, const Formattable& n,
                               const UnicodeString& exp,
                               UErrorCode status) {
    if (U_FAILURE(status)) {
        errln("FAIL: NumberFormat constructor");
    } else {
        expect2(*fmt, n, exp);
    }
    delete fmt;
}

void NumberFormatTest::expect(NumberFormat& fmt, const UnicodeString& str, const Formattable& n) {
    UErrorCode status = U_ZERO_ERROR;
    Formattable num;
    fmt.parse(str, num, status);
    if (U_FAILURE(status)) {
        errln(UnicodeString("FAIL: Parse failed for \"") + str + "\"");
        return;
    }
    UnicodeString pat;
    ((DecimalFormat*) &fmt)->toPattern(pat);
    if (equalValue(num, n)) {
        logln(UnicodeString("Ok   \"") + str + "\" x " +
              pat + " = " +
              toString(num));
    } else {
        errln(UnicodeString("FAIL \"") + str + "\" x " +
              pat + " = " +
              toString(num) + ", expected " + toString(n));
    }
}

void NumberFormatTest::expect(NumberFormat& fmt, const Formattable& n,
                              const UnicodeString& exp, UBool rt) {
    UnicodeString saw;
    FieldPosition pos;
    UErrorCode status = U_ZERO_ERROR;
    fmt.format(n, saw, pos, status);
    CHECK(status, "NumberFormat::format");
    UnicodeString pat;
    ((DecimalFormat*) &fmt)->toPattern(pat);
    if (saw == exp) {
        logln(UnicodeString("Ok   ") + toString(n) + " x " +
              escape(pat) + " = \"" +
              escape(saw) + "\"");
        // We should be able to round-trip the formatted string =>
        // number => string (but not the other way around: number
        // => string => number2, might have number2 != number):
        if (rt) {
            Formattable n2;
            fmt.parse(exp, n2, status);
            if (U_FAILURE(status)) {
                errln(UnicodeString("FAIL: Parse failed for \"") + exp + "\"");
                return;
            }
            UnicodeString saw2;
            fmt.format(n2, saw2, pos, status);
            CHECK(status, "NumberFormat::format");
            if (saw2 != exp) {
                errln((UnicodeString)"FAIL \"" + exp + "\" => " + toString(n2) +
                      " => \"" + saw2 + "\"");
            }
        }
    } else {
        errln(UnicodeString("FAIL ") + toString(n) + " x " +
              escape(pat) + " = \"" +
              escape(saw) + "\", expected \"" + exp + "\"");
    }
}

void NumberFormatTest::expect(NumberFormat* fmt, const Formattable& n,
                              const UnicodeString& exp,
                              UErrorCode status) {
    if (U_FAILURE(status)) {
        errln("FAIL: NumberFormat constructor");
    } else {
        expect(*fmt, n, exp);
    }
    delete fmt;
}

void NumberFormatTest::expectCurrency(NumberFormat& nf, const Locale& locale,
                                      double value, const UnicodeString& string) {
    UErrorCode ec = U_ZERO_ERROR;
    DecimalFormat& fmt = * (DecimalFormat*) &nf;
    const UChar DEFAULT_CURR[] = {45/*-*/,0};
    UChar curr[4];
    u_strcpy(curr, DEFAULT_CURR);
    if (*locale.getLanguage() != 0) {
        ucurr_forLocale(locale.getName(), curr, 4, &ec);
        assertSuccess("ucurr_forLocale", ec);
        fmt.setCurrency(curr, ec);
        assertSuccess("DecimalFormat::setCurrency", ec);
        fmt.setCurrency(curr); //Deprecated variant, for coverage only
    }
    UnicodeString s;
    fmt.format(value, s);
    s.findAndReplace((UChar32)0x00A0, (UChar32)0x0020);

    // Default display of the number yields "1234.5599999999999"
    // instead of "1234.56".  Use a formatter to fix this.
    NumberFormat* f = 
        NumberFormat::createInstance(Locale::getUS(), ec);
    UnicodeString v;
    if (U_FAILURE(ec)) {
        // Oops; bad formatter.  Use default op+= display.
        v = (UnicodeString)"" + value;
    } else {
        f->setMaximumFractionDigits(4);
        f->setGroupingUsed(FALSE);
        f->format(value, v);
    }
    delete f;

    if (s == string) {
        logln((UnicodeString)"Ok: " + v + " x " + curr + " => " + prettify(s));
    } else {
        errln((UnicodeString)"FAIL: " + v + " x " + curr + " => " + prettify(s) +
              ", expected " + prettify(string));
    }
}

void NumberFormatTest::expectPat(DecimalFormat& fmt, const UnicodeString& exp) {
    UnicodeString pat;
    fmt.toPattern(pat);
    if (pat == exp) {
        logln(UnicodeString("Ok   \"") + pat + "\"");
    } else {
        errln(UnicodeString("FAIL \"") + pat + "\", expected \"" + exp + "\"");
    }
}

void NumberFormatTest::expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                                 int32_t pos) {
    expectPad(fmt, pat, pos, 0, (UnicodeString)"");
}
void NumberFormatTest::expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                                 int32_t pos, int32_t width, UChar pad) {
    expectPad(fmt, pat, pos, width, UnicodeString(pad));
}
void NumberFormatTest::expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                                 int32_t pos, int32_t width, const UnicodeString& pad) {
    int32_t apos = 0, awidth = 0;
    UnicodeString apadStr;
    UErrorCode status = U_ZERO_ERROR;
    fmt.applyPattern(pat, status);
    if (U_SUCCESS(status)) {
        apos = fmt.getPadPosition();
        awidth = fmt.getFormatWidth();
        apadStr=fmt.getPadCharacterString();
    } else {
        apos = -1;
        awidth = width;
        apadStr = pad;
    }
    if (apos == pos && awidth == width && apadStr == pad) {
        UnicodeString infoStr;
        if (pos == ILLEGAL) {
            infoStr = UnicodeString(" width=", "") + awidth + UnicodeString(" pad=", "") + apadStr;
        }
        logln(UnicodeString("Ok   \"") + pat + "\" pos=" + apos + infoStr);
    } else {
        errln(UnicodeString("FAIL \"") + pat + "\" pos=" + apos +
              " width=" + awidth + " pad=" + apadStr +
              ", expected " + pos + " " + width + " " + pad);
    }
}
void NumberFormatTest::TestJB3832(){
    const char* localeID = "pt_PT@currency=PTE";
    Locale loc(localeID);
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString expected("1,150$50 Esc.");
    UnicodeString s;
    NumberFormat* currencyFmt = NumberFormat::createCurrencyInstance(loc, status);
    if(U_FAILURE(status)){
        errln("Could not create currency formatter for locale %s", localeID);
        return;
    }
    currencyFmt->format(1150.50, s);
    if(s!=expected){
        errln(UnicodeString("FAIL: Expected: ")+expected 
                + UnicodeString(" Got: ") + s 
                + UnicodeString( " for locale: ")+ UnicodeString(localeID) );
    }
    if (U_FAILURE(status)){
        errln("FAIL: Status %s", u_errorName(status));
    }
    delete currencyFmt;
}

void NumberFormatTest::TestHost()
{
#ifdef U_WINDOWS
    Win32NumberTest::testLocales(this);
#endif
}

void NumberFormatTest::TestHostClone()
{
    /*
    Verify that a cloned formatter gives the same results
    and is useable after the original has been deleted.
    */
    // This is mainly important on Windows.
    UErrorCode status = U_ZERO_ERROR;
    Locale loc("en_US@compat=host");
    UDate now = Calendar::getNow();
    NumberFormat *full = NumberFormat::createInstance(loc, status);
    if (full == NULL || U_FAILURE(status)) {
        errln("FAIL: Can't create Relative date instance");
        return;
    }
    UnicodeString result1;
    full->format(now, result1, status);
    Format *fullClone = full->clone();
    delete full;
    full = NULL;

    UnicodeString result2;
    fullClone->format(now, result2, status);
    if (U_FAILURE(status)) {
        errln("FAIL: format failure.");
    }
    if (result1 != result2) {
        errln("FAIL: Clone returned different result from non-clone.");
    }
    delete fullClone;
}

void NumberFormatTest::TestCurrencyFormat()
{
    // This test is here to increase code coverage.
    UErrorCode status = U_ZERO_ERROR;
    MeasureFormat *cloneObj;
    UnicodeString str;
    Formattable toFormat, result;
    static const UChar ISO_CODE[4] = {0x0047, 0x0042, 0x0050, 0};

    Locale  saveDefaultLocale = Locale::getDefault();
    Locale::setDefault( Locale::getUK(), status );
    if (U_FAILURE(status)) {
        errln("couldn't set default Locale!");
        return;
    }

    MeasureFormat *measureObj = MeasureFormat::createCurrencyFormat(status);
    Locale::setDefault( saveDefaultLocale, status );
    if (U_FAILURE(status)){
        errln("FAIL: Status %s", u_errorName(status));
        return;
    }
    cloneObj = (MeasureFormat *)measureObj->clone();
    if (cloneObj == NULL) {
        errln("Clone doesn't work");
        return;
    }
    toFormat.adoptObject(new CurrencyAmount(1234.56, ISO_CODE, status));
    measureObj->format(toFormat, str, status);
    measureObj->parseObject(str, result, status);
    if (U_FAILURE(status)){
        errln("FAIL: Status %s", u_errorName(status));
    }
    if (result != toFormat) {
        errln("measureObj does not round trip. Formatted string was \"" + str + "\" Got: " + toString(result) + " Expected: " + toString(toFormat));
    }
    status = U_ZERO_ERROR;
    str.truncate(0);
    cloneObj->format(toFormat, str, status);
    cloneObj->parseObject(str, result, status);
    if (U_FAILURE(status)){
        errln("FAIL: Status %s", u_errorName(status));
    }
    if (result != toFormat) {
        errln("Clone does not round trip. Formatted string was \"" + str + "\" Got: " + toString(result) + " Expected: " + toString(toFormat));
    }
    if (*measureObj != *cloneObj) {
        errln("Cloned object is not equal to the original object");
    }
    delete measureObj;
    delete cloneObj;

    status = U_USELESS_COLLATOR_ERROR;
    if (MeasureFormat::createCurrencyFormat(status) != NULL) {
        errln("createCurrencyFormat should have returned NULL.");
    }
}

/* Port of ICU4J rounding test. */
void NumberFormatTest::TestRounding() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat *df = (DecimalFormat*)NumberFormat::createCurrencyInstance(Locale::getEnglish(), status);

    if (U_FAILURE(status)) {
        errln("Unable to create decimal formatter.");
        return;
    }

    int roundingIncrements[]={1, 2, 5, 20, 50, 100};
    int testValues[]={0, 300};

    for (int j=0; j<2; j++) {
        for (int mode=DecimalFormat::kRoundUp;mode<DecimalFormat::kRoundHalfEven;mode++) {
            df->setRoundingMode((DecimalFormat::ERoundingMode)mode);
            for (int increment=0; increment<6; increment++) {
                double base=testValues[j];
                double rInc=roundingIncrements[increment];
                checkRounding(df, base, 20, rInc);
                rInc=1.000000000/rInc;
                checkRounding(df, base, 20, rInc);
            }
        }
    }

}

void NumberFormatTest::checkRounding(DecimalFormat* df, double base, int iterations, double increment) {
    df->setRoundingIncrement(increment);
    double lastParsed=INT32_MIN; //Intger.MIN_VALUE
    for (int i=-iterations; i<=iterations;i++) {
        double iValue=base+(increment*(i*0.1));
        double smallIncrement=0.00000001;
        if (iValue!=0) {
            smallIncrement*=iValue;
        }
        //we not only test the value, but some values in a small range around it
        lastParsed=checkRound(df, iValue-smallIncrement, lastParsed);
        lastParsed=checkRound(df, iValue, lastParsed);
        lastParsed=checkRound(df, iValue+smallIncrement, lastParsed);
    }
}

double NumberFormatTest::checkRound(DecimalFormat* df, double iValue, double lastParsed) {
    UErrorCode status=U_ZERO_ERROR;
    UnicodeString formattedDecimal;
    double parsed;
    Formattable result;
    df->format(iValue, formattedDecimal, status);

    if (U_FAILURE(status)) {
        errln("Error formatting number.");
    }

    df->parse(formattedDecimal, result, status);

    if (U_FAILURE(status)) {
        errln("Error parsing number.");
    }

    parsed=result.getDouble();

    if (lastParsed>parsed) {
        errln("Rounding wrong direction! %d > %d", lastParsed, parsed);
    }

    return lastParsed;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
