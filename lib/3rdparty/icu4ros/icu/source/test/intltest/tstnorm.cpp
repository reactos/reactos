/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/uniset.h"
#include "unicode/usetiter.h"
#include "unicode/schriter.h"
#include "cstring.h"
#include "unormimp.h"
#include "tstnorm.h"

#define LENGTHOF(array) ((int32_t)(sizeof(array)/sizeof((array)[0])))
#define ARRAY_LENGTH(array) LENGTHOF(array)

#define CASE(id,test) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              test();                   \
                          }                             \
                          break

static UErrorCode status = U_ZERO_ERROR;

void BasicNormalizerTest::runIndexedTest(int32_t index, UBool exec,
                                         const char* &name, char* /*par*/) {
    switch (index) {
        CASE(0,TestDecomp);
        CASE(1,TestCompatDecomp);
        CASE(2,TestCanonCompose);
        CASE(3,TestCompatCompose);
        CASE(4,TestPrevious);
        CASE(5,TestHangulDecomp);
        CASE(6,TestHangulCompose);
        CASE(7,TestTibetan);
        CASE(8,TestCompositionExclusion);
        CASE(9,TestZeroIndex);
        CASE(10,TestVerisign);
        CASE(11,TestPreviousNext);
        CASE(12,TestNormalizerAPI);
        CASE(13,TestConcatenate);
        CASE(14,FindFoldFCDExceptions);
        CASE(15,TestCompare);
        CASE(16,TestSkippable);
        default: name = ""; break;
    }
}

/**
 * Convert Java-style strings with \u Unicode escapes into UnicodeString objects
 */
static UnicodeString str(const char *input)
{
    UnicodeString str(input, ""); // Invariant conversion
    return str.unescape();
}


BasicNormalizerTest::BasicNormalizerTest()
{
  // canonTest
  // Input                    Decomposed                    Composed

    canonTests[0][0] = str("cat");  canonTests[0][1] = str("cat"); canonTests[0][2] =  str("cat");

    canonTests[1][0] = str("\\u00e0ardvark");    canonTests[1][1] = str("a\\u0300ardvark");  canonTests[1][2] = str("\\u00e0ardvark"); 

    canonTests[2][0] = str("\\u1e0a"); canonTests[2][1] = str("D\\u0307"); canonTests[2][2] = str("\\u1e0a");                 // D-dot_above

    canonTests[3][0] = str("D\\u0307");  canonTests[3][1] = str("D\\u0307"); canonTests[3][2] = str("\\u1e0a");            // D dot_above

    canonTests[4][0] = str("\\u1e0c\\u0307"); canonTests[4][1] = str("D\\u0323\\u0307");  canonTests[4][2] = str("\\u1e0c\\u0307");         // D-dot_below dot_above

    canonTests[5][0] = str("\\u1e0a\\u0323"); canonTests[5][1] = str("D\\u0323\\u0307");  canonTests[5][2] = str("\\u1e0c\\u0307");        // D-dot_above dot_below 

    canonTests[6][0] = str("D\\u0307\\u0323"); canonTests[6][1] = str("D\\u0323\\u0307");  canonTests[6][2] = str("\\u1e0c\\u0307");         // D dot_below dot_above 

    canonTests[7][0] = str("\\u1e10\\u0307\\u0323");  canonTests[7][1] = str("D\\u0327\\u0323\\u0307"); canonTests[7][2] = str("\\u1e10\\u0323\\u0307");     // D dot_below cedilla dot_above

    canonTests[8][0] = str("D\\u0307\\u0328\\u0323"); canonTests[8][1] = str("D\\u0328\\u0323\\u0307"); canonTests[8][2] = str("\\u1e0c\\u0328\\u0307");     // D dot_above ogonek dot_below

    canonTests[9][0] = str("\\u1E14"); canonTests[9][1] = str("E\\u0304\\u0300"); canonTests[9][2] = str("\\u1E14");         // E-macron-grave

    canonTests[10][0] = str("\\u0112\\u0300"); canonTests[10][1] = str("E\\u0304\\u0300");  canonTests[10][2] = str("\\u1E14");            // E-macron + grave

    canonTests[11][0] = str("\\u00c8\\u0304"); canonTests[11][1] = str("E\\u0300\\u0304");  canonTests[11][2] = str("\\u00c8\\u0304");         // E-grave + macron
  
    canonTests[12][0] = str("\\u212b"); canonTests[12][1] = str("A\\u030a"); canonTests[12][2] = str("\\u00c5");             // angstrom_sign

    canonTests[13][0] = str("\\u00c5");      canonTests[13][1] = str("A\\u030a");  canonTests[13][2] = str("\\u00c5");            // A-ring
  
    canonTests[14][0] = str("\\u00C4ffin");  canonTests[14][1] = str("A\\u0308ffin");  canonTests[14][2] = str("\\u00C4ffin");

    canonTests[15][0] = str("\\u00C4\\uFB03n"); canonTests[15][1] = str("A\\u0308\\uFB03n"); canonTests[15][2] = str("\\u00C4\\uFB03n");
  
    canonTests[16][0] = str("Henry IV"); canonTests[16][1] = str("Henry IV"); canonTests[16][2] = str("Henry IV");

    canonTests[17][0] = str("Henry \\u2163");  canonTests[17][1] = str("Henry \\u2163");  canonTests[17][2] = str("Henry \\u2163");
  
    canonTests[18][0] = str("\\u30AC");  canonTests[18][1] = str("\\u30AB\\u3099");  canonTests[18][2] = str("\\u30AC");              // ga (Katakana)

    canonTests[19][0] = str("\\u30AB\\u3099"); canonTests[19][1] = str("\\u30AB\\u3099");  canonTests[19][2] = str("\\u30AC");            // ka + ten

    canonTests[20][0] = str("\\uFF76\\uFF9E"); canonTests[20][1] = str("\\uFF76\\uFF9E");  canonTests[20][2] = str("\\uFF76\\uFF9E");       // hw_ka + hw_ten

    canonTests[21][0] = str("\\u30AB\\uFF9E"); canonTests[21][1] = str("\\u30AB\\uFF9E");  canonTests[21][2] = str("\\u30AB\\uFF9E");         // ka + hw_ten

    canonTests[22][0] = str("\\uFF76\\u3099"); canonTests[22][1] = str("\\uFF76\\u3099");  canonTests[22][2] = str("\\uFF76\\u3099");         // hw_ka + ten

    canonTests[23][0] = str("A\\u0300\\u0316"); canonTests[23][1] = str("A\\u0316\\u0300");  canonTests[23][2] = str("\\u00C0\\u0316");     

    /* compatTest */
  // Input                        Decomposed                        Composed
  compatTests[0][0] = str("cat"); compatTests[0][1] = str("cat"); compatTests[0][2] = str("cat") ;
  
  compatTests[1][0] = str("\\uFB4f");  compatTests[1][1] = str("\\u05D0\\u05DC"); compatTests[1][2] = str("\\u05D0\\u05DC");  // Alef-Lamed vs. Alef, Lamed
  
  compatTests[2][0] = str("\\u00C4ffin"); compatTests[2][1] = str("A\\u0308ffin"); compatTests[2][2] = str("\\u00C4ffin") ;

  compatTests[3][0] = str("\\u00C4\\uFB03n"); compatTests[3][1] = str("A\\u0308ffin"); compatTests[3][2] = str("\\u00C4ffin") ; // ffi ligature -> f + f + i
  
  compatTests[4][0] = str("Henry IV"); compatTests[4][1] = str("Henry IV"); compatTests[4][2] = str("Henry IV") ;

  compatTests[5][0] = str("Henry \\u2163"); compatTests[5][1] = str("Henry IV");  compatTests[5][2] = str("Henry IV") ;
  
  compatTests[6][0] = str("\\u30AC"); compatTests[6][1] = str("\\u30AB\\u3099"); compatTests[6][2] = str("\\u30AC") ; // ga (Katakana)

  compatTests[7][0] = str("\\u30AB\\u3099"); compatTests[7][1] = str("\\u30AB\\u3099"); compatTests[7][2] = str("\\u30AC") ; // ka + ten
  
  compatTests[8][0] = str("\\uFF76\\u3099"); compatTests[8][1] = str("\\u30AB\\u3099"); compatTests[8][2] = str("\\u30AC") ; // hw_ka + ten
  
  /* These two are broken in Unicode 2.1.2 but fixed in 2.1.5 and later */
  compatTests[9][0] = str("\\uFF76\\uFF9E"); compatTests[9][1] = str("\\u30AB\\u3099"); compatTests[9][2] = str("\\u30AC") ; // hw_ka + hw_ten

  compatTests[10][0] = str("\\u30AB\\uFF9E"); compatTests[10][1] = str("\\u30AB\\u3099"); compatTests[10][2] = str("\\u30AC") ; // ka + hw_ten

  /* Hangul Canonical */
  // Input                        Decomposed                        Composed
  hangulCanon[0][0] = str("\\ud4db"); hangulCanon[0][1] = str("\\u1111\\u1171\\u11b6"); hangulCanon[0][2] = str("\\ud4db") ;

  hangulCanon[1][0] = str("\\u1111\\u1171\\u11b6"), hangulCanon[1][1] = str("\\u1111\\u1171\\u11b6"),   hangulCanon[1][2] = str("\\ud4db");
}

BasicNormalizerTest::~BasicNormalizerTest()
{
}

void BasicNormalizerTest::TestPrevious() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  
  logln("testing decomp...");
  uint32_t i;
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }
  
  logln("testing compose...");
  norm->setMode(UNORM_NFC);
  for (i = 0; i < ARRAY_LENGTH(canonTests); i++) {
    backAndForth(norm, canonTests[i][0]);
  }

  delete norm;
}

void BasicNormalizerTest::TestDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 1);
  staticTest(UNORM_NFD, 0, canonTests, ARRAY_LENGTH(canonTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCompatDecomp() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKD);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 1);
  
  staticTest(UNORM_NFKD, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 1);
  delete norm;
}

void BasicNormalizerTest::TestCanonCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, canonTests, ARRAY_LENGTH(canonTests), 2);
  
  staticTest(UNORM_NFC, 0, canonTests,
         ARRAY_LENGTH(canonTests), 2);
  delete norm;
}

void BasicNormalizerTest::TestCompatCompose() 
{
  Normalizer* norm = new Normalizer("", UNORM_NFKC);
  iterateTest(norm, compatTests, ARRAY_LENGTH(compatTests), 2);
  
  staticTest(UNORM_NFKC, 0, 
         compatTests, ARRAY_LENGTH(compatTests), 2);
  delete norm;
}


//-------------------------------------------------------------------------------

void BasicNormalizerTest::TestHangulCompose() 
{
  // Make sure that the static composition methods work
  logln("Canonical composition...");
  staticTest(UNORM_NFC, 0,                    hangulCanon,  ARRAY_LENGTH(hangulCanon),  2);
  logln("Compatibility composition...");
  
  // Now try iterative composition....
  logln("Static composition...");
  Normalizer* norm = new Normalizer("", UNORM_NFC);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 2);
  norm->setMode(UNORM_NFKC);
  
  // And finally, make sure you can do it in reverse too
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFC);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}

void BasicNormalizerTest::TestHangulDecomp() 
{
  // Make sure that the static decomposition methods work
  logln("Canonical decomposition...");
  staticTest(UNORM_NFD, 0,                     hangulCanon,  ARRAY_LENGTH(hangulCanon),  1);
  logln("Compatibility decomposition...");
  
  // Now the iterative decomposition methods...
  logln("Iterative decomposition...");
  Normalizer* norm = new Normalizer("", UNORM_NFD);
  iterateTest(norm, hangulCanon, ARRAY_LENGTH(hangulCanon), 1);
  norm->setMode(UNORM_NFKD);
  
  // And finally, make sure you can do it in reverse too
  logln("Reverse iteration...");
  norm->setMode(UNORM_NFD);
  for (uint32_t i = 0; i < ARRAY_LENGTH(hangulCanon); i++) {
    backAndForth(norm, hangulCanon[i][0]);
  }
  delete norm;
}

/**
 * The Tibetan vowel sign AA, 0f71, was messed up prior to Unicode version 2.1.9.
 */
void BasicNormalizerTest::TestTibetan(void) {
    UnicodeString decomp[1][3];
    decomp[0][0] = str("\\u0f77");
    decomp[0][1] = str("\\u0f77");
    decomp[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    UnicodeString compose[1][3];
    compose[0][0] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][1] = str("\\u0fb2\\u0f71\\u0f80");
    compose[0][2] = str("\\u0fb2\\u0f71\\u0f80");

    staticTest(UNORM_NFD,         0, decomp, ARRAY_LENGTH(decomp), 1);
    staticTest(UNORM_NFKD,  0, decomp, ARRAY_LENGTH(decomp), 2);
    staticTest(UNORM_NFC,        0, compose, ARRAY_LENGTH(compose), 1);
    staticTest(UNORM_NFKC, 0, compose, ARRAY_LENGTH(compose), 2);
}

/**
 * Make sure characters in the CompositionExclusion.txt list do not get
 * composed to.
 */
void BasicNormalizerTest::TestCompositionExclusion(void) {
    // This list is generated from CompositionExclusion.txt.
    // Update whenever the normalizer tables are updated.  Note
    // that we test all characters listed, even those that can be
    // derived from the Unicode DB and are therefore commented
    // out.
    // ### TODO read composition exclusion from source/data/unidata file
    // and test against that
    UnicodeString EXCLUDED = str(
        "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958"
        "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC"
        "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E"
        "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69"
        "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2"
        "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79"
        "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u1FDB"
        "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF9\\u1FFB\\u1FFD\\u2000"
        "\\u2001\\u2126\\u212A\\u212B\\u2329\\u232A\\uF900\\uFA10"
        "\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A\\uFB1F"
        "\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E\\uFB2F\\uFB30\\uFB31"
        "\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36\\uFB38\\uFB39\\uFB3A"
        "\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46"
        "\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B\\uFB4C\\uFB4D\\uFB4E"
        );
    for (int32_t i=0; i<EXCLUDED.length(); ++i) {
        UnicodeString a(EXCLUDED.charAt(i));
        UnicodeString b;
        UnicodeString c;
        Normalizer::normalize(a, UNORM_NFKD, 0, b, status);
        Normalizer::normalize(b, UNORM_NFC, 0, c, status);
        if (c == a) {
            errln("FAIL: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));
        } else if (verbose) {
            logln("Ok: " + hex(a) + " x DECOMP_COMPAT => " +
                  hex(b) + " x COMPOSE => " +
                  hex(c));                
        }
    }
}

/**
 * Test for a problem that showed up just before ICU 1.6 release
 * having to do with combining characters with an index of zero.
 * Such characters do not participate in any canonical
 * decompositions.  However, having an index of zero means that
 * they all share one typeMask[] entry, that is, they all have to
 * map to the same canonical class, which is not the case, in
 * reality.
 */
void BasicNormalizerTest::TestZeroIndex(void) {
    const char* DATA[] = {
        // Expect col1 x COMPOSE_COMPAT => col2
        // Expect col2 x DECOMP => col3
        "A\\u0316\\u0300", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0300\\u0316", "\\u00C0\\u0316", "A\\u0316\\u0300",
        "A\\u0327\\u0300", "\\u00C0\\u0327", "A\\u0327\\u0300",
        "c\\u0321\\u0327", "c\\u0321\\u0327", "c\\u0321\\u0327",
        "c\\u0327\\u0321", "\\u00E7\\u0321", "c\\u0327\\u0321",
    };
    int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    for (int32_t i=0; i<DATA_length; i+=3) {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString a(DATA[i], "");
        a = a.unescape();
        UnicodeString b;
        Normalizer::normalize(a, UNORM_NFKC, 0, b, status);
        UnicodeString exp(DATA[i+1], "");
        exp = exp.unescape();
        if (b == exp) {
            logln((UnicodeString)"Ok: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b));
        } else {
            errln((UnicodeString)"FAIL: " + hex(a) + " x COMPOSE_COMPAT => " + hex(b) +
                  ", expect " + hex(exp));
        }
        Normalizer::normalize(b, UNORM_NFD, 0, a, status);
        exp = UnicodeString(DATA[i+2], "").unescape();
        if (a == exp) {
            logln((UnicodeString)"Ok: " + hex(b) + " x DECOMP => " + hex(a));
        } else {
            errln((UnicodeString)"FAIL: " + hex(b) + " x DECOMP => " + hex(a) +
                  ", expect " + hex(exp));
        }
    }
}

/**
 * Run a few specific cases that are failing for Verisign.
 */
void BasicNormalizerTest::TestVerisign(void) {
    /*
      > Their input:
      > 05B8 05B9 05B1 0591 05C3 05B0 05AC 059F
      > Their output (supposedly from ICU):
      > 05B8 05B1 05B9 0591 05C3 05B0 05AC 059F
      > My output from charlint:
      > 05B1 05B8 05B9 0591 05C3 05B0 05AC 059F
      
      05B8 05B9 05B1 0591 05C3 05B0 05AC 059F => 05B1 05B8 05B9 0591 05C3 05B0
      05AC 059F
      
      U+05B8  18  E HEBREW POINT QAMATS
      U+05B9  19  F HEBREW POINT HOLAM
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA
      
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+05B8  18 HEBREW POINT QAMATS
      U+05B9  19 HEBREW POINT HOLAM
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA
      
      Wrong result:
      U+05B8  18 HEBREW POINT QAMATS
      U+05B1  11 HEBREW POINT HATAF SEGOL
      U+05B9  19 HEBREW POINT HOLAM
      U+0591 220 HEBREW ACCENT ETNAHTA
      U+05C3   0 HEBREW PUNCTUATION SOF PASUQ
      U+05B0  10 HEBREW POINT SHEVA
      U+05AC 230 HEBREW ACCENT ILUY
      U+059F 230 HEBREW ACCENT QARNEY PARA

      
      > Their input:
      >0592 05B7 05BC 05A5 05B0 05C0 05C4 05AD
      >Their output (supposedly from ICU):
      >0592 05B0 05B7 05BC 05A5 05C0 05AD 05C4
      >My output from charlint:
      >05B0 05B7 05BC 05A5 0592 05C0 05AD 05C4
      
      0592 05B7 05BC 05A5 05B0 05C0 05C4 05AD => 05B0 05B7 05BC 05A5 0592 05C0
      05AD 05C4
      
      U+0592 230 HEBREW ACCENT SEGOL
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+05B0  10 HEBREW POINT SHEVA
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05C4 230 HEBREW MARK UPPER DOT
      U+05AD 222 HEBREW ACCENT DEHI
      
      U+05B0  10 HEBREW POINT SHEVA
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+0592 230 HEBREW ACCENT SEGOL
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05AD 222 HEBREW ACCENT DEHI
      U+05C4 230 HEBREW MARK UPPER DOT

      Wrong result:
      U+0592 230 HEBREW ACCENT SEGOL
      U+05B0  10 HEBREW POINT SHEVA
      U+05B7  17 HEBREW POINT PATAH
      U+05BC  21 HEBREW POINT DAGESH OR MAPIQ
      U+05A5 220 HEBREW ACCENT MERKHA
      U+05C0   0 HEBREW PUNCTUATION PASEQ
      U+05AD 222 HEBREW ACCENT DEHI
      U+05C4 230 HEBREW MARK UPPER DOT
    */
    UnicodeString data[2][3];
    data[0][0] = str("\\u05B8\\u05B9\\u05B1\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][1] = str("\\u05B1\\u05B8\\u05B9\\u0591\\u05C3\\u05B0\\u05AC\\u059F");
    data[0][2] = str("");
    data[1][0] = str("\\u0592\\u05B7\\u05BC\\u05A5\\u05B0\\u05C0\\u05C4\\u05AD");
    data[1][1] = str("\\u05B0\\u05B7\\u05BC\\u05A5\\u0592\\u05C0\\u05AD\\u05C4");
    data[1][2] = str("");

    staticTest(UNORM_NFD, 0, data, ARRAY_LENGTH(data), 1);
    staticTest(UNORM_NFC, 0, data, ARRAY_LENGTH(data), 1);
}

//------------------------------------------------------------------------
// Internal utilities
//

UnicodeString BasicNormalizerTest::hex(UChar ch) {
    UnicodeString result;
    return appendHex(ch, 4, result);
}

UnicodeString BasicNormalizerTest::hex(const UnicodeString& s) {
    UnicodeString result;
    for (int i = 0; i < s.length(); ++i) {
        if (i != 0) result += (UChar)0x2c/*,*/;
        appendHex(s[i], 4, result);
    }
    return result;
}


inline static void insert(UnicodeString& dest, int pos, UChar32 ch)
{
    dest.replace(pos, 0, ch);
}

void BasicNormalizerTest::backAndForth(Normalizer* iter, const UnicodeString& input)
{
    UChar32 ch;
    iter->setText(input, status);

    // Run through the iterator forwards and stick it into a StringBuffer
    UnicodeString forward;
    for (ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        forward += ch;
    }

    // Now do it backwards
    UnicodeString reverse;
    for (ch = iter->last(); ch != iter->DONE; ch = iter->previous()) {
        insert(reverse, 0, ch);
    }
    
    if (forward != reverse) {
        errln("Forward/reverse mismatch for input " + hex(input)
              + ", forward: " + hex(forward) + ", backward: " + hex(reverse));
    }
}

void BasicNormalizerTest::staticTest(UNormalizationMode mode, int options,
                     UnicodeString tests[][3], int length,
                     int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        UnicodeString output;
        Normalizer::normalize(input, mode, options, output, status);
        
        if (output != expect) {
            errln(UnicodeString("ERROR: case ") + i + " normalized " + hex(input) + "\n"
                + "                expected " + hex(expect) + "\n"
                + "              static got " + hex(output) );
        }
    }
}

void BasicNormalizerTest::iterateTest(Normalizer* iter,
                                      UnicodeString tests[][3], int length,
                                      int outCol)
{
    for (int i = 0; i < length; i++)
    {
        UnicodeString& input = tests[i][0];
        UnicodeString& expect = tests[i][outCol];
        
        logln("Normalizing '" + input + "' (" + hex(input) + ")" );
        
        iter->setText(input, status);
        assertEqual(input, expect, iter, UnicodeString("ERROR: case ") + i + " ");
    }
}

void BasicNormalizerTest::assertEqual(const UnicodeString&    input,
                      const UnicodeString&    expected,
                      Normalizer*        iter,
                      const UnicodeString&    errPrefix)
{
    UnicodeString result;

    for (UChar32 ch = iter->first(); ch != iter->DONE; ch = iter->next()) {
        result += ch;
    }
    if (result != expected) {
        errln(errPrefix + "normalized " + hex(input) + "\n"
            + "                expected " + hex(expected) + "\n"
            + "             iterate got " + hex(result) );
    }
}

// helper class for TestPreviousNext()
// simple UTF-32 character iterator
class UChar32Iterator {
public:
    UChar32Iterator(const UChar32 *text, int32_t len, int32_t index) :
        s(text), length(len), i(index) {}

    UChar32 current() {
        if(i<length) {
            return s[i];
        } else {
            return 0xffff;
        }
    }

    UChar32 next() {
        if(i<length) {
            return s[i++];
        } else {
            return 0xffff;
        }
    }

    UChar32 previous() {
        if(i>0) {
            return s[--i];
        } else {
            return 0xffff;
        }
    }

    int32_t getIndex() {
        return i;
    }
private:
    const UChar32 *s;
    int32_t length, i;
};

void
BasicNormalizerTest::TestPreviousNext(const UChar *src, int32_t srcLength,
                                      const UChar32 *expect, int32_t expectLength,
                                      const int32_t *expectIndex, // its length=expectLength+1
                                      int32_t srcMiddle, int32_t expectMiddle,
                                      const char *moves,
                                      UNormalizationMode mode,
                                      const char *name) {
    // iterators
    Normalizer iter(src, srcLength, mode);

    // test getStaticClassID and getDynamicClassID
    if(iter.getDynamicClassID() != Normalizer::getStaticClassID()) {
        errln("getStaticClassID != getDynamicClassID for Normalizer.");
    }

    UChar32Iterator iter32(expect, expectLength, expectMiddle);

    UChar32 c1, c2;
    char m;

    // initially set the indexes into the middle of the strings
    iter.setIndexOnly(srcMiddle);

    // move around and compare the iteration code points with
    // the expected ones
    const char *move=moves;
    while((m=*move++)!=0) {
        if(m=='-') {
            c1=iter.previous();
            c2=iter32.previous();
        } else if(m=='0') {
            c1=iter.current();
            c2=iter32.current();
        } else /* m=='+' */ {
            c1=iter.next();
            c2=iter32.next();
        }

        // compare results
        if(c1!=c2) {
            // copy the moves until the current (m) move, and terminate
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            errln("error: mismatch in Normalizer iteration (%s) at %s: "
                  "got c1=U+%04lx != expected c2=U+%04lx\n",
                  name, history, c1, c2);
            break;
        }

        // compare indexes
        if(iter.getIndex()!=expectIndex[iter32.getIndex()]) {
            // copy the moves until the current (m) move, and terminate
            char history[64];
            uprv_strcpy(history, moves);
            history[move-moves]=0;
            errln("error: index mismatch in Normalizer iteration (%s) at %s: "
                  "Normalizer index %ld expected %ld\n",
                  name, history, iter.getIndex(), expectIndex[iter32.getIndex()]);
            break;
        }
    }
}

void
BasicNormalizerTest::TestPreviousNext() {
    // src and expect strings
    static const UChar src[]={
        UTF16_LEAD(0x2f999), UTF16_TRAIL(0x2f999),
        UTF16_LEAD(0x1d15f), UTF16_TRAIL(0x1d15f),
        0xc4,
        0x1ed0
    };
    static const UChar32 expect[]={
        0x831d,
        0x1d158, 0x1d165,
        0x41, 0x308,
        0x4f, 0x302, 0x301
    };

    // expected src indexes corresponding to expect indexes
    static const int32_t expectIndex[]={
        0,
        2, 2,
        4, 4,
        5, 5, 5,
        6 // behind last character
    };

    // src and expect strings for regression test for j2911
    static const UChar src_j2911[]={
        UTF16_LEAD(0x2f999), UTF16_TRAIL(0x2f999),
        0xdd00, 0xd900, // unpaired surrogates - regression test for j2911
        0xc4,
        0x4f, 0x302, 0x301
    };
    static const UChar32 expect_j2911[]={
        0x831d,
        0xdd00, 0xd900, // unpaired surrogates - regression test for j2911
        0xc4,
        0x1ed0
    };

    // expected src indexes corresponding to expect indexes
    static const int32_t expectIndex_j2911[]={
        0,
        2, 3,
        4,
        5,
        8 // behind last character
    };

    // initial indexes into the src and expect strings
    // for both sets of test data
    enum {
        SRC_MIDDLE=4,
        EXPECT_MIDDLE=3,
        SRC_MIDDLE_2=2,
        EXPECT_MIDDLE_2=1
    };

    // movement vector
    // - for previous(), 0 for current(), + for next()
    // for both sets of test data
    static const char *const moves="0+0+0--0-0-+++0--+++++++0--------";

    TestPreviousNext(src, LENGTHOF(src),
                     expect, LENGTHOF(expect),
                     expectIndex,
                     SRC_MIDDLE, EXPECT_MIDDLE,
                     moves, UNORM_NFD, "basic");

    TestPreviousNext(src_j2911, LENGTHOF(src_j2911),
                     expect_j2911, LENGTHOF(expect_j2911),
                     expectIndex_j2911,
                     SRC_MIDDLE, EXPECT_MIDDLE,
                     moves, UNORM_NFKC, "j2911");

    // try again from different "middle" indexes
    TestPreviousNext(src, LENGTHOF(src),
                     expect, LENGTHOF(expect),
                     expectIndex,
                     SRC_MIDDLE_2, EXPECT_MIDDLE_2,
                     moves, UNORM_NFD, "basic_2");

    TestPreviousNext(src_j2911, LENGTHOF(src_j2911),
                     expect_j2911, LENGTHOF(expect_j2911),
                     expectIndex_j2911,
                     SRC_MIDDLE_2, EXPECT_MIDDLE_2,
                     moves, UNORM_NFKC, "j2911_2");
}

void BasicNormalizerTest::TestConcatenate() {
    static const char *const
    cases[][4]={
        /* mode, left, right, result */
        {
            "C",
            "re",
            "\\u0301sum\\u00e9",
            "r\\u00e9sum\\u00e9"
        },
        {
            "C",
            "a\\u1100",
            "\\u1161bcdefghijk",
            "a\\uac00bcdefghijk"
        },
        /* ### TODO: add more interesting cases */
        {
            "D", 
            "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958" 
            "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC" 
            "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E" 
            "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69" 
            "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2" 
            "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79" 
            "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u1FDB",
            
            "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF9\\u1FFB\\u1FFD\\u2000" 
            "\\u2001\\u2126\\u212A\\u212B\\u2329\\u232A\\uF900\\uFA10" 
            "\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A\\uFB1F" 
            "\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E\\uFB2F\\uFB30\\uFB31" 
            "\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36\\uFB38\\uFB39\\uFB3A" 
            "\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46" 
            "\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B\\uFB4C\\uFB4D\\uFB4E",
           
            "\\u0340\\u0341\\u0343\\u0344\\u0374\\u037E\\u0387\\u0958"
            "\\u0959\\u095A\\u095B\\u095C\\u095D\\u095E\\u095F\\u09DC"
            "\\u09DD\\u09DF\\u0A33\\u0A36\\u0A59\\u0A5A\\u0A5B\\u0A5E"
            "\\u0B5C\\u0B5D\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69"
            "\\u0F73\\u0F75\\u0F76\\u0F78\\u0F81\\u0F93\\u0F9D\\u0FA2"
            "\\u0FA7\\u0FAC\\u0FB9\\u1F71\\u1F73\\u1F75\\u1F77\\u1F79"
            "\\u1F7B\\u1F7D\\u1FBB\\u1FBE\\u1FC9\\u1FCB\\u1FD3\\u0399"
            "\\u0301\\u03C5\\u0308\\u0301\\u1FEB\\u1FEE\\u1FEF\\u1FF9"
            "\\u1FFB\\u1FFD\\u2000\\u2001\\u2126\\u212A\\u212B\\u2329"
            "\\u232A\\uF900\\uFA10\\uFA12\\uFA15\\uFA20\\uFA22\\uFA25"
            "\\uFA26\\uFA2A\\uFB1F\\uFB2A\\uFB2B\\uFB2C\\uFB2D\\uFB2E"
            "\\uFB2F\\uFB30\\uFB31\\uFB32\\uFB33\\uFB34\\uFB35\\uFB36"
            "\\uFB38\\uFB39\\uFB3A\\uFB3B\\uFB3C\\uFB3E\\uFB40\\uFB41"
            "\\uFB43\\uFB44\\uFB46\\uFB47\\uFB48\\uFB49\\uFB4A\\uFB4B"
            "\\uFB4C\\uFB4D\\uFB4E"
        }
    };

    UnicodeString left, right, expect, result, r;
    UErrorCode errorCode;
    UNormalizationMode mode;
    int32_t i;

    /* test concatenation */
    for(i=0; i<(int32_t)(sizeof(cases)/sizeof(cases[0])); ++i) {
        switch(*cases[i][0]) {
        case 'C': mode=UNORM_NFC; break;
        case 'D': mode=UNORM_NFD; break;
        case 'c': mode=UNORM_NFKC; break;
        case 'd': mode=UNORM_NFKD; break;
        default: mode=UNORM_NONE; break;
        }

        left=UnicodeString(cases[i][1], "").unescape();
        right=UnicodeString(cases[i][2], "").unescape();
        expect=UnicodeString(cases[i][3], "").unescape();

        //result=r=UnicodeString();
        errorCode=U_ZERO_ERROR;

        r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
        if(U_FAILURE(errorCode) || /*result!=r ||*/ result!=expect) {
            errln("error in Normalizer::concatenate(), cases[] fails with "+
                UnicodeString(u_errorName(errorCode))+", result==expect: expected: "+
                hex(expect)+" =========> got: " + hex(result));
        }
    }

    /* test error cases */

    /* left.getBuffer()==result.getBuffer() */
    result=r=expect=UnicodeString("zz", "");
    errorCode=U_UNEXPECTED_TOKEN;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_UNEXPECTED_TOKEN || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), violates UErrorCode protocol");
    }

    left.setToBogus();
    errorCode=U_ZERO_ERROR;
    r=Normalizer::concatenate(left, right, result, mode, 0, errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || result!=r || !result.isBogus()) {
        errln("error in Normalizer::concatenate(), does not detect left.isBogus()");
    }
}

// reference implementation of Normalizer::compare
static int32_t
ref_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    UnicodeString r1, r2, t1, t2;
    int32_t normOptions=(int32_t)(options>>UNORM_COMPARE_NORM_OPTIONS_SHIFT);

    if(options&U_COMPARE_IGNORE_CASE) {
        Normalizer::decompose(s1, FALSE, normOptions, r1, errorCode);
        Normalizer::decompose(s2, FALSE, normOptions, r2, errorCode);

        r1.foldCase(options);
        r2.foldCase(options);
    } else {
        r1=s1;
        r2=s2;
    }

    Normalizer::decompose(r1, FALSE, normOptions, t1, errorCode);
    Normalizer::decompose(r2, FALSE, normOptions, t2, errorCode);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}

// test wrapper for Normalizer::compare, sets UNORM_INPUT_IS_FCD appropriately
static int32_t
_norm_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options, UErrorCode &errorCode) {
    int32_t normOptions=(int32_t)(options>>UNORM_COMPARE_NORM_OPTIONS_SHIFT);

    if( UNORM_YES==Normalizer::quickCheck(s1, UNORM_FCD, normOptions, errorCode) &&
        UNORM_YES==Normalizer::quickCheck(s2, UNORM_FCD, normOptions, errorCode)) {
        options|=UNORM_INPUT_IS_FCD;
    }

    return Normalizer::compare(s1, s2, options, errorCode);
}

// reference implementation of UnicodeString::caseCompare
static int32_t
ref_case_compare(const UnicodeString &s1, const UnicodeString &s2, uint32_t options) {
    UnicodeString t1, t2;

    t1=s1;
    t2=s2;

    t1.foldCase(options);
    t2.foldCase(options);

    if(options&U_COMPARE_CODE_POINT_ORDER) {
        return t1.compareCodePointOrder(t2);
    } else {
        return t1.compare(t2);
    }
}

// reduce an integer to -1/0/1
static inline int32_t
_sign(int32_t value) {
    if(value==0) {
        return 0;
    } else {
        return (value>>31)|1;
    }
}

static const char *
_signString(int32_t value) {
    if(value<0) {
        return "<0";
    } else if(value==0) {
        return "=0";
    } else /* value>0 */ {
        return ">0";
    }
}

void
BasicNormalizerTest::TestCompare() {
    // test Normalizer::compare and unorm_compare (thinly wrapped by the former)
    // by comparing it with its semantic equivalent
    // since we trust the pieces, this is sufficient

    // test each string with itself and each other
    // each time with all options
    static const char *const
    strings[]={
        // some cases from NormalizationTest.txt
        // 0..3
        "D\\u031B\\u0307\\u0323",
        "\\u1E0C\\u031B\\u0307",
        "D\\u031B\\u0323\\u0307",
        "d\\u031B\\u0323\\u0307",

        // 4..6
        "\\u00E4",
        "a\\u0308",
        "A\\u0308",

        // Angstrom sign = A ring
        // 7..10
        "\\u212B",
        "\\u00C5",
        "A\\u030A",
        "a\\u030A",

        // 11.14
        "a\\u059A\\u0316\\u302A\\u032Fb",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "a\\u302A\\u0316\\u032F\\u059Ab",
        "A\\u059A\\u0316\\u302A\\u032Fb",

        // from ICU case folding tests
        // 15..20
        "A\\u00df\\u00b5\\ufb03\\U0001040c\\u0131",
        "ass\\u03bcffi\\U00010434i",
        "\\u0061\\u0042\\u0131\\u03a3\\u00df\\ufb03\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udfff",
        "\\u0041\\u0062\\u0131\\u03c3\\u0053\\u0073\\u0066\\u0046\\u0069\\ud93f\\udfff",
        "\\u0041\\u0062\\u0069\\u03c3\\u0073\\u0053\\u0046\\u0066\\u0049\\ud93f\\udffd",

        //     U+d800 U+10001   see implementation comment in unorm_cmpEquivFold
        // vs. U+10000          at bottom - code point order
        // 21..22
        "\\ud800\\ud800\\udc01",
        "\\ud800\\udc00",

        // other code point order tests from ustrtest.cpp
        // 23..31
        "\\u20ac\\ud801",
        "\\u20ac\\ud800\\udc00",
        "\\ud800",
        "\\ud800\\uff61",
        "\\udfff",
        "\\uff61\\udfff",
        "\\uff61\\ud800\\udc02",
        "\\ud800\\udc02",
        "\\ud84d\\udc56",

        // long strings, see cnormtst.c/TestNormCoverage()
        // equivalent if case-insensitive
        // 32..33
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaaaaaaaaazzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\uAD8B\\uAD8B\\uAD8B\\uAD8B"
        "d\\u031B\\u0307\\u0323",

        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "\\U0001d15e\\U0001d157\\U0001d165\\U0001d15e\\U0001d15e\\U0001d15e\\U0001d15e"
        "aaaaaaaaaaAAAAAAAAZZZZZZZZZZZZZZZZzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        "\\u1100\\u116f\\u11aa\\uAD8B\\uAD8B\\u1100\\u116f\\u11aa"
        "\\u1E0C\\u031B\\u0307",

        // some strings that may make a difference whether the compare function
        // case-folds or decomposes first
        // 34..41
        "\\u0360\\u0345\\u0334",
        "\\u0360\\u03b9\\u0334",

        "\\u0360\\u1f80\\u0334",
        "\\u0360\\u03b1\\u0313\\u03b9\\u0334",

        "\\u0360\\u1ffc\\u0334",
        "\\u0360\\u03c9\\u03b9\\u0334",

        "a\\u0360\\u0345\\u0360\\u0345b",
        "a\\u0345\\u0360\\u0345\\u0360b",

        // interesting cases for canonical caseless match with turkic i handling
        // 42..43
        "\\u00cc",
        "\\u0069\\u0300",

        // strings with post-Unicode 3.2 normalization or normalization corrections
        // 44..45
        "\\u00e4\\u193b\\U0002f868",
        "\\u0061\\u193b\\u0308\\u36fc",

        // empty string
        // 46
        ""
    };

    UnicodeString s[100]; // at least as many items as in strings[] !

    // all combinations of options
    // UNORM_INPUT_IS_FCD is set automatically if both input strings fulfill FCD conditions
    // set UNORM_UNICODE_3_2 in one additional combination
    static const struct {
        uint32_t options;
        const char *name;
    } opt[]={
        { 0, "default" },
        { U_COMPARE_CODE_POINT_ORDER, "c.p. order" },
        { U_COMPARE_IGNORE_CASE, "ignore case" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE, "c.p. order & ignore case" },
        { U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "ignore case & special i" },
        { U_COMPARE_CODE_POINT_ORDER|U_COMPARE_IGNORE_CASE|U_FOLD_CASE_EXCLUDE_SPECIAL_I, "c.p. order & ignore case & special i" },
        { UNORM_UNICODE_3_2<<UNORM_COMPARE_NORM_OPTIONS_SHIFT, "Unicode 3.2" }
    };

    int32_t i, j, k, count=LENGTHOF(strings);
    int32_t result, refResult;

    UErrorCode errorCode;

    // create the UnicodeStrings
    for(i=0; i<count; ++i) {
        s[i]=UnicodeString(strings[i], "").unescape();
    }

    // test them each with each other
    for(i=0; i<count; ++i) {
        for(j=i; j<count; ++j) {
            for(k=0; k<LENGTHOF(opt); ++k) {
                // test Normalizer::compare
                errorCode=U_ZERO_ERROR;
                result=_norm_compare(s[i], s[j], opt[k].options, errorCode);
                refResult=ref_norm_compare(s[i], s[j], opt[k].options, errorCode);
                if(_sign(result)!=_sign(refResult)) {
                    errln("Normalizer::compare(%d, %d, %s)%s should be %s %s",
                        i, j, opt[k].name, _signString(result), _signString(refResult),
                        U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                }

                // test UnicodeString::caseCompare - same internal implementation function
                if(opt[k].options&U_COMPARE_IGNORE_CASE) {
                    errorCode=U_ZERO_ERROR;
                    result=s[i].caseCompare(s[j], opt[k].options);
                    refResult=ref_case_compare(s[i], s[j], opt[k].options);
                    if(_sign(result)!=_sign(refResult)) {
                        errln("UniStr::caseCompare(%d, %d, %s)%s should be %s %s",
                            i, j, opt[k].name, _signString(result), _signString(refResult),
                            U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                    }
                }
            }
        }
    }

    // test cases with i and I to make sure Turkic works
    static const UChar iI[]={ 0x49, 0x69, 0x130, 0x131 };
    USerializedSet sset;
    UnicodeSet set;

    UnicodeString s1, s2;
    UChar32 start, end;

    // collect all sets into one for contiguous output
    for(i=0; i<LENGTHOF(iI); ++i) {
        if(unorm_getCanonStartSet(iI[i], &sset)) {
            count=uset_getSerializedRangeCount(&sset);
            for(j=0; j<count; ++j) {
                uset_getSerializedRange(&sset, j, &start, &end);
                set.add(start, end);
            }
        }
    }

    // test all of these precomposed characters
    UnicodeSetIterator it(set);
    while(it.nextRange() && !it.isString()) {
        start=it.getCodepoint();
        end=it.getCodepointEnd();
        while(start<=end) {
            s1.setTo(start);
            errorCode=U_ZERO_ERROR;
            Normalizer::decompose(s1, FALSE, 0, s2, errorCode);
            if(U_FAILURE(errorCode)) {
                errln("Normalizer::decompose(U+%04x) failed: %s", start, u_errorName(errorCode));
                return;
            }

            for(k=0; k<LENGTHOF(opt); ++k) {
                // test Normalizer::compare
                errorCode=U_ZERO_ERROR;
                result=_norm_compare(s1, s2, opt[k].options, errorCode);
                refResult=ref_norm_compare(s1, s2, opt[k].options, errorCode);
                if(_sign(result)!=_sign(refResult)) {
                    errln("Normalizer::compare(U+%04x with its NFD, %s)%s should be %s %s",
                        start, opt[k].name, _signString(result), _signString(refResult),
                        U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                }

                // test UnicodeString::caseCompare - same internal implementation function
                if(opt[k].options&U_COMPARE_IGNORE_CASE) {
                    errorCode=U_ZERO_ERROR;
                    result=s1.caseCompare(s2, opt[k].options);
                    refResult=ref_case_compare(s1, s2, opt[k].options);
                    if(_sign(result)!=_sign(refResult)) {
                        errln("UniStr::caseCompare(U+%04x with its NFD, %s)%s should be %s %s",
                            start, opt[k].name, _signString(result), _signString(refResult),
                            U_SUCCESS(errorCode) ? "" : u_errorName(errorCode));
                    }
                }
            }

            ++start;
        }
    }
}

// verify that case-folding does not un-FCD strings
int32_t
BasicNormalizerTest::countFoldFCDExceptions(uint32_t foldingOptions) {
    UnicodeString s, fold, d;
    UChar32 c;
    int32_t count;
    uint8_t cc, trailCC, foldCC, foldTrailCC;
    UNormalizationCheckResult qcResult;
    int8_t category;
    UBool isNFD;
    UErrorCode errorCode;

    logln("Test if case folding may un-FCD a string (folding options %04lx)", foldingOptions);

    count=0;
    for(c=0; c<=0x10ffff; ++c) {
        errorCode = U_ZERO_ERROR;
        category=u_charType(c);
        if(category==U_UNASSIGNED) {
            continue; // skip unassigned code points
        }
        if(c==0xac00) {
            c=0xd7a3; // skip Hangul - no case folding there
            continue;
        }
        // skip Han blocks - no case folding there either
        if(c==0x3400) {
            c=0x4db5;
            continue;
        }
        if(c==0x4e00) {
            c=0x9fa5;
            continue;
        }
        if(c==0x20000) {
            c=0x2a6d6;
            continue;
        }

        s.setTo(c);

        // get leading and trailing cc for c
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        isNFD= s==d;
        cc=u_getCombiningClass(d.char32At(0));
        trailCC=u_getCombiningClass(d.char32At(d.length()-1));

        // get leading and trailing cc for the case-folding of c
        s.foldCase(foldingOptions);
        Normalizer::decompose(s, FALSE, 0, d, errorCode);
        foldCC=u_getCombiningClass(d.char32At(0));
        foldTrailCC=u_getCombiningClass(d.char32At(d.length()-1));

        qcResult=Normalizer::quickCheck(s, UNORM_FCD, errorCode);

        if (U_FAILURE(errorCode)) {
            ++count;
            errln("U+%04lx: Failed with error %s", u_errorName(errorCode));
        }

        // bad:
        // - character maps to empty string: adjacent characters may then need reordering
        // - folding has different leading/trailing cc's, and they don't become just 0
        // - folding itself is not FCD
        if( qcResult!=UNORM_YES ||
            s.isEmpty() ||
            (cc!=foldCC && foldCC!=0) || (trailCC!=foldTrailCC && foldTrailCC!=0)
        ) {
            ++count;
            errln("U+%04lx: case-folding may un-FCD a string (folding options %04lx)", c, foldingOptions);
            errln("  cc %02x trailCC %02x    foldCC(U+%04lx) %02x foldTrailCC(U+%04lx) %02x   quickCheck(folded)=%d", cc, trailCC, d.char32At(0), foldCC, d.char32At(d.length()-1), foldTrailCC, qcResult);
            continue;
        }

        // also bad:
        // if a code point is in NFD but its case folding is not, then
        // unorm_compare will also fail
        if(isNFD && UNORM_YES!=Normalizer::quickCheck(s, UNORM_NFD, errorCode)) {
            ++count;
            errln("U+%04lx: case-folding un-NFDs this character (folding options %04lx)", c, foldingOptions);
        }
    }

    logln("There are %ld code points for which case-folding may un-FCD a string (folding options %04lx)", count, foldingOptions);
    return count;
}

void
BasicNormalizerTest::FindFoldFCDExceptions() {
    int32_t count;

    count=countFoldFCDExceptions(0);
    count+=countFoldFCDExceptions(U_FOLD_CASE_EXCLUDE_SPECIAL_I);
    if(count>0) {
        /*
         * If case-folding un-FCDs any strings, then unorm_compare() must be
         * re-implemented.
         * It currently assumes that one can check for FCD then case-fold
         * and then still have FCD strings for raw decomposition without reordering.
         */
        errln("error: There are %ld code points for which case-folding may un-FCD a string for all folding options.\n"
              "See comment in BasicNormalizerTest::FindFoldFCDExceptions()!", count);
    }
}

/*
 * Hardcoded "NF* Skippable" sets, generated from
 * Mark Davis' com.ibm.text.UCD.NFSkippable (see ICU4J CVS, module unicodetools).
 * Run com.ibm.text.UCD.Main with the option NFSkippable.
 *
 * Must be updated for each Unicode version.
 */
static void
initExpectedSkippables(UnicodeSet skipSets[UNORM_MODE_COUNT]) {
    UErrorCode errorCode=U_ZERO_ERROR;

    skipSets[UNORM_NFD].applyPattern(UnicodeString(
        "[^\\u00C0-\\u00C5\\u00C7-\\u00CF\\u00D1-\\u00D6\\u00D9-\\u00DD"
        "\\u00E0-\\u00E5\\u00E7-\\u00EF\\u00F1-\\u00F6\\u00F9-\\u00FD"
        "\\u00FF-\\u010F\\u0112-\\u0125\\u0128-\\u0130\\u0134-\\u0137"
        "\\u0139-\\u013E\\u0143-\\u0148\\u014C-\\u0151\\u0154-\\u0165"
        "\\u0168-\\u017E\\u01A0\\u01A1\\u01AF\\u01B0\\u01CD-\\u01DC"
        "\\u01DE-\\u01E3\\u01E6-\\u01F0\\u01F4\\u01F5\\u01F8-\\u021B"
        "\\u021E\\u021F\\u0226-\\u0233\\u0300-\\u034E\\u0350-\\u036F"
        "\\u0374\\u037E\\u0385-\\u038A\\u038C\\u038E-\\u0390\\u03AA-"
        "\\u03B0\\u03CA-\\u03CE\\u03D3\\u03D4\\u0400\\u0401\\u0403\\u0407"
        "\\u040C-\\u040E\\u0419\\u0439\\u0450\\u0451\\u0453\\u0457\\u045C"
        "-\\u045E\\u0476\\u0477\\u0483-\\u0486\\u04C1\\u04C2\\u04D0-"
        "\\u04D3\\u04D6\\u04D7\\u04DA-\\u04DF\\u04E2-\\u04E7\\u04EA-"
        "\\u04F5\\u04F8\\u04F9\\u0591-\\u05BD\\u05BF\\u05C1\\u05C2\\u05C4"
        "\\u05C5\\u05C7\\u0610-\\u0615\\u0622-\\u0626\\u064B-\\u065E"
        "\\u0670\\u06C0\\u06C2\\u06D3\\u06D6-\\u06DC\\u06DF-\\u06E4"
        "\\u06E7\\u06E8\\u06EA-\\u06ED\\u0711\\u0730-\\u074A\\u07EB-"
        "\\u07F3\\u0929\\u0931\\u0934\\u093C\\u094D\\u0951-\\u0954\\u0958"
        "-\\u095F\\u09BC\\u09CB-\\u09CD\\u09DC\\u09DD\\u09DF\\u0A33"
        "\\u0A36\\u0A3C\\u0A4D\\u0A59-\\u0A5B\\u0A5E\\u0ABC\\u0ACD\\u0B3C"
        "\\u0B48\\u0B4B-\\u0B4D\\u0B5C\\u0B5D\\u0B94\\u0BCA-\\u0BCD"
        "\\u0C48\\u0C4D\\u0C55\\u0C56\\u0CBC\\u0CC0\\u0CC7\\u0CC8\\u0CCA"
        "\\u0CCB\\u0CCD\\u0D4A-\\u0D4D\\u0DCA\\u0DDA\\u0DDC-\\u0DDE"
        "\\u0E38-\\u0E3A\\u0E48-\\u0E4B\\u0EB8\\u0EB9\\u0EC8-\\u0ECB"
        "\\u0F18\\u0F19\\u0F35\\u0F37\\u0F39\\u0F43\\u0F4D\\u0F52\\u0F57"
        "\\u0F5C\\u0F69\\u0F71-\\u0F76\\u0F78\\u0F7A-\\u0F7D\\u0F80-"
        "\\u0F84\\u0F86\\u0F87\\u0F93\\u0F9D\\u0FA2\\u0FA7\\u0FAC\\u0FB9"
        "\\u0FC6\\u1026\\u1037\\u1039\\u135F\\u1714\\u1734\\u17D2\\u17DD"
        "\\u18A9\\u1939-\\u193B\\u1A17\\u1A18\\u1B06\\u1B08\\u1B0A\\u1B0C"
        "\\u1B0E\\u1B12\\u1B34\\u1B3B\\u1B3D\\u1B40\\u1B41\\u1B43\\u1B44"
        "\\u1B6B-\\u1B73\\u1DC0-\\u1DCA\\u1DFE-\\u1E99\\u1E9B\\u1EA0-"
        "\\u1EF9\\u1F00-\\u1F15\\u1F18-\\u1F1D\\u1F20-\\u1F45\\u1F48-"
        "\\u1F4D\\u1F50-\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F-\\u1F7D"
        "\\u1F80-\\u1FB4\\u1FB6-\\u1FBC\\u1FBE\\u1FC1-\\u1FC4\\u1FC6-"
        "\\u1FD3\\u1FD6-\\u1FDB\\u1FDD-\\u1FEF\\u1FF2-\\u1FF4\\u1FF6-"
        "\\u1FFD\\u2000\\u2001\\u20D0-\\u20DC\\u20E1\\u20E5-\\u20EF"
        "\\u2126\\u212A\\u212B\\u219A\\u219B\\u21AE\\u21CD-\\u21CF\\u2204"
        "\\u2209\\u220C\\u2224\\u2226\\u2241\\u2244\\u2247\\u2249\\u2260"
        "\\u2262\\u226D-\\u2271\\u2274\\u2275\\u2278\\u2279\\u2280\\u2281"
        "\\u2284\\u2285\\u2288\\u2289\\u22AC-\\u22AF\\u22E0-\\u22E3"
        "\\u22EA-\\u22ED\\u2329\\u232A\\u2ADC\\u302A-\\u302F\\u304C"
        "\\u304E\\u3050\\u3052\\u3054\\u3056\\u3058\\u305A\\u305C\\u305E"
        "\\u3060\\u3062\\u3065\\u3067\\u3069\\u3070\\u3071\\u3073\\u3074"
        "\\u3076\\u3077\\u3079\\u307A\\u307C\\u307D\\u3094\\u3099\\u309A"
        "\\u309E\\u30AC\\u30AE\\u30B0\\u30B2\\u30B4\\u30B6\\u30B8\\u30BA"
        "\\u30BC\\u30BE\\u30C0\\u30C2\\u30C5\\u30C7\\u30C9\\u30D0\\u30D1"
        "\\u30D3\\u30D4\\u30D6\\u30D7\\u30D9\\u30DA\\u30DC\\u30DD\\u30F4"
        "\\u30F7-\\u30FA\\u30FE\\uA806\\uAC00-\\uD7A3\\uF900-\\uFA0D"
        "\\uFA10\\uFA12\\uFA15-\\uFA1E\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A"
        "-\\uFA2D\\uFA30-\\uFA6A\\uFA70-\\uFAD9\\uFB1D-\\uFB1F\\uFB2A-"
        "\\uFB36\\uFB38-\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46"
        "-\\uFB4E\\uFE20-\\uFE23\\U00010A0D\\U00010A0F\\U00010A38-\\U0001"
        "0A3A\\U00010A3F\\U0001D15E-\\U0001D169\\U0001D16D-\\U0001D172"
        "\\U0001D17B-\\U0001D182\\U0001D185-\\U0001D18B\\U0001D1AA-"
        "\\U0001D1AD\\U0001D1BB-\\U0001D1C0\\U0001D242-\\U0001D244\\U0002"
        "F800-\\U0002FA1D]"
        , ""), errorCode);

    skipSets[UNORM_NFC].applyPattern(UnicodeString(
        "[^<->A-PR-Za-pr-z\\u00A8\\u00C0-\\u00CF\\u00D1-\\u00D6\\u00D8-"
        "\\u00DD\\u00E0-\\u00EF\\u00F1-\\u00F6\\u00F8-\\u00FD\\u00FF-"
        "\\u0103\\u0106-\\u010F\\u0112-\\u0117\\u011A-\\u0121\\u0124"
        "\\u0125\\u0128-\\u012D\\u0130\\u0139\\u013A\\u013D\\u013E\\u0143"
        "\\u0144\\u0147\\u0148\\u014C-\\u0151\\u0154\\u0155\\u0158-"
        "\\u015D\\u0160\\u0161\\u0164\\u0165\\u0168-\\u0171\\u0174-"
        "\\u017F\\u01A0\\u01A1\\u01AF\\u01B0\\u01B7\\u01CD-\\u01DC\\u01DE"
        "-\\u01E1\\u01E6-\\u01EB\\u01F4\\u01F5\\u01F8-\\u01FB\\u0200-"
        "\\u021B\\u021E\\u021F\\u0226-\\u0233\\u0292\\u0300-\\u034E"
        "\\u0350-\\u036F\\u0374\\u037E\\u0387\\u0391\\u0395\\u0397\\u0399"
        "\\u039F\\u03A1\\u03A5\\u03A9\\u03AC\\u03AE\\u03B1\\u03B5\\u03B7"
        "\\u03B9\\u03BF\\u03C1\\u03C5\\u03C9-\\u03CB\\u03CE\\u03D2\\u0406"
        "\\u0410\\u0413\\u0415-\\u0418\\u041A\\u041E\\u0423\\u0427\\u042B"
        "\\u042D\\u0430\\u0433\\u0435-\\u0438\\u043A\\u043E\\u0443\\u0447"
        "\\u044B\\u044D\\u0456\\u0474\\u0475\\u0483-\\u0486\\u04D8\\u04D9"
        "\\u04E8\\u04E9\\u0591-\\u05BD\\u05BF\\u05C1\\u05C2\\u05C4\\u05C5"
        "\\u05C7\\u0610-\\u0615\\u0622\\u0623\\u0627\\u0648\\u064A-"
        "\\u065E\\u0670\\u06C1\\u06D2\\u06D5-\\u06DC\\u06DF-\\u06E4"
        "\\u06E7\\u06E8\\u06EA-\\u06ED\\u0711\\u0730-\\u074A\\u07EB-"
        "\\u07F3\\u0928\\u0930\\u0933\\u093C\\u094D\\u0951-\\u0954\\u0958"
        "-\\u095F\\u09BC\\u09BE\\u09C7\\u09CD\\u09D7\\u09DC\\u09DD\\u09DF"
        "\\u0A33\\u0A36\\u0A3C\\u0A4D\\u0A59-\\u0A5B\\u0A5E\\u0ABC\\u0ACD"
        "\\u0B3C\\u0B3E\\u0B47\\u0B4D\\u0B56\\u0B57\\u0B5C\\u0B5D\\u0B92"
        "\\u0BBE\\u0BC6\\u0BC7\\u0BCD\\u0BD7\\u0C46\\u0C4D\\u0C55\\u0C56"
        "\\u0CBC\\u0CBF\\u0CC2\\u0CC6\\u0CCA\\u0CCD\\u0CD5\\u0CD6\\u0D3E"
        "\\u0D46\\u0D47\\u0D4D\\u0D57\\u0DCA\\u0DCF\\u0DD9\\u0DDC\\u0DDF"
        "\\u0E38-\\u0E3A\\u0E48-\\u0E4B\\u0EB8\\u0EB9\\u0EC8-\\u0ECB"
        "\\u0F18\\u0F19\\u0F35\\u0F37\\u0F39\\u0F43\\u0F4D\\u0F52\\u0F57"
        "\\u0F5C\\u0F69\\u0F71-\\u0F76\\u0F78\\u0F7A-\\u0F7D\\u0F80-"
        "\\u0F84\\u0F86\\u0F87\\u0F93\\u0F9D\\u0FA2\\u0FA7\\u0FAC\\u0FB9"
        "\\u0FC6\\u1025\\u102E\\u1037\\u1039\\u1100-\\u1112\\u1161-"
        "\\u1175\\u11A8-\\u11C2\\u135F\\u1714\\u1734\\u17D2\\u17DD\\u18A9"
        "\\u1939-\\u193B\\u1A17\\u1A18\\u1B05\\u1B07\\u1B09\\u1B0B\\u1B0D"
        "\\u1B11\\u1B34\\u1B35\\u1B3A\\u1B3C\\u1B3E\\u1B3F\\u1B42\\u1B44"
        "\\u1B6B-\\u1B73\\u1DC0-\\u1DCA\\u1DFE-\\u1E03\\u1E0A-\\u1E0F"
        "\\u1E12-\\u1E1B\\u1E20-\\u1E27\\u1E2A-\\u1E41\\u1E44-\\u1E53"
        "\\u1E58-\\u1E7D\\u1E80-\\u1E87\\u1E8E-\\u1E91\\u1E96-\\u1E99"
        "\\u1EA0-\\u1EF3\\u1EF6-\\u1EF9\\u1F00-\\u1F11\\u1F18\\u1F19"
        "\\u1F20-\\u1F31\\u1F38\\u1F39\\u1F40\\u1F41\\u1F48\\u1F49\\u1F50"
        "\\u1F51\\u1F59\\u1F60-\\u1F71\\u1F73-\\u1F75\\u1F77\\u1F79"
        "\\u1F7B-\\u1F7D\\u1F80\\u1F81\\u1F88\\u1F89\\u1F90\\u1F91\\u1F98"
        "\\u1F99\\u1FA0\\u1FA1\\u1FA8\\u1FA9\\u1FB3\\u1FB6\\u1FBB\\u1FBC"
        "\\u1FBE\\u1FBF\\u1FC3\\u1FC6\\u1FC9\\u1FCB\\u1FCC\\u1FD3\\u1FDB"
        "\\u1FE3\\u1FEB\\u1FEE\\u1FEF\\u1FF3\\u1FF6\\u1FF9\\u1FFB-\\u1FFE"
        "\\u2000\\u2001\\u20D0-\\u20DC\\u20E1\\u20E5-\\u20EF\\u2126"
        "\\u212A\\u212B\\u2190\\u2192\\u2194\\u21D0\\u21D2\\u21D4\\u2203"
        "\\u2208\\u220B\\u2223\\u2225\\u223C\\u2243\\u2245\\u2248\\u224D"
        "\\u2261\\u2264\\u2265\\u2272\\u2273\\u2276\\u2277\\u227A-\\u227D"
        "\\u2282\\u2283\\u2286\\u2287\\u2291\\u2292\\u22A2\\u22A8\\u22A9"
        "\\u22AB\\u22B2-\\u22B5\\u2329\\u232A\\u2ADC\\u302A-\\u302F"
        "\\u3046\\u304B\\u304D\\u304F\\u3051\\u3053\\u3055\\u3057\\u3059"
        "\\u305B\\u305D\\u305F\\u3061\\u3064\\u3066\\u3068\\u306F\\u3072"
        "\\u3075\\u3078\\u307B\\u3099\\u309A\\u309D\\u30A6\\u30AB\\u30AD"
        "\\u30AF\\u30B1\\u30B3\\u30B5\\u30B7\\u30B9\\u30BB\\u30BD\\u30BF"
        "\\u30C1\\u30C4\\u30C6\\u30C8\\u30CF\\u30D2\\u30D5\\u30D8\\u30DB"
        "\\u30EF-\\u30F2\\u30FD\\uA806\\uAC00\\uAC1C\\uAC38\\uAC54\\uAC70"
        "\\uAC8C\\uACA8\\uACC4\\uACE0\\uACFC\\uAD18\\uAD34\\uAD50\\uAD6C"
        "\\uAD88\\uADA4\\uADC0\\uADDC\\uADF8\\uAE14\\uAE30\\uAE4C\\uAE68"
        "\\uAE84\\uAEA0\\uAEBC\\uAED8\\uAEF4\\uAF10\\uAF2C\\uAF48\\uAF64"
        "\\uAF80\\uAF9C\\uAFB8\\uAFD4\\uAFF0\\uB00C\\uB028\\uB044\\uB060"
        "\\uB07C\\uB098\\uB0B4\\uB0D0\\uB0EC\\uB108\\uB124\\uB140\\uB15C"
        "\\uB178\\uB194\\uB1B0\\uB1CC\\uB1E8\\uB204\\uB220\\uB23C\\uB258"
        "\\uB274\\uB290\\uB2AC\\uB2C8\\uB2E4\\uB300\\uB31C\\uB338\\uB354"
        "\\uB370\\uB38C\\uB3A8\\uB3C4\\uB3E0\\uB3FC\\uB418\\uB434\\uB450"
        "\\uB46C\\uB488\\uB4A4\\uB4C0\\uB4DC\\uB4F8\\uB514\\uB530\\uB54C"
        "\\uB568\\uB584\\uB5A0\\uB5BC\\uB5D8\\uB5F4\\uB610\\uB62C\\uB648"
        "\\uB664\\uB680\\uB69C\\uB6B8\\uB6D4\\uB6F0\\uB70C\\uB728\\uB744"
        "\\uB760\\uB77C\\uB798\\uB7B4\\uB7D0\\uB7EC\\uB808\\uB824\\uB840"
        "\\uB85C\\uB878\\uB894\\uB8B0\\uB8CC\\uB8E8\\uB904\\uB920\\uB93C"
        "\\uB958\\uB974\\uB990\\uB9AC\\uB9C8\\uB9E4\\uBA00\\uBA1C\\uBA38"
        "\\uBA54\\uBA70\\uBA8C\\uBAA8\\uBAC4\\uBAE0\\uBAFC\\uBB18\\uBB34"
        "\\uBB50\\uBB6C\\uBB88\\uBBA4\\uBBC0\\uBBDC\\uBBF8\\uBC14\\uBC30"
        "\\uBC4C\\uBC68\\uBC84\\uBCA0\\uBCBC\\uBCD8\\uBCF4\\uBD10\\uBD2C"
        "\\uBD48\\uBD64\\uBD80\\uBD9C\\uBDB8\\uBDD4\\uBDF0\\uBE0C\\uBE28"
        "\\uBE44\\uBE60\\uBE7C\\uBE98\\uBEB4\\uBED0\\uBEEC\\uBF08\\uBF24"
        "\\uBF40\\uBF5C\\uBF78\\uBF94\\uBFB0\\uBFCC\\uBFE8\\uC004\\uC020"
        "\\uC03C\\uC058\\uC074\\uC090\\uC0AC\\uC0C8\\uC0E4\\uC100\\uC11C"
        "\\uC138\\uC154\\uC170\\uC18C\\uC1A8\\uC1C4\\uC1E0\\uC1FC\\uC218"
        "\\uC234\\uC250\\uC26C\\uC288\\uC2A4\\uC2C0\\uC2DC\\uC2F8\\uC314"
        "\\uC330\\uC34C\\uC368\\uC384\\uC3A0\\uC3BC\\uC3D8\\uC3F4\\uC410"
        "\\uC42C\\uC448\\uC464\\uC480\\uC49C\\uC4B8\\uC4D4\\uC4F0\\uC50C"
        "\\uC528\\uC544\\uC560\\uC57C\\uC598\\uC5B4\\uC5D0\\uC5EC\\uC608"
        "\\uC624\\uC640\\uC65C\\uC678\\uC694\\uC6B0\\uC6CC\\uC6E8\\uC704"
        "\\uC720\\uC73C\\uC758\\uC774\\uC790\\uC7AC\\uC7C8\\uC7E4\\uC800"
        "\\uC81C\\uC838\\uC854\\uC870\\uC88C\\uC8A8\\uC8C4\\uC8E0\\uC8FC"
        "\\uC918\\uC934\\uC950\\uC96C\\uC988\\uC9A4\\uC9C0\\uC9DC\\uC9F8"
        "\\uCA14\\uCA30\\uCA4C\\uCA68\\uCA84\\uCAA0\\uCABC\\uCAD8\\uCAF4"
        "\\uCB10\\uCB2C\\uCB48\\uCB64\\uCB80\\uCB9C\\uCBB8\\uCBD4\\uCBF0"
        "\\uCC0C\\uCC28\\uCC44\\uCC60\\uCC7C\\uCC98\\uCCB4\\uCCD0\\uCCEC"
        "\\uCD08\\uCD24\\uCD40\\uCD5C\\uCD78\\uCD94\\uCDB0\\uCDCC\\uCDE8"
        "\\uCE04\\uCE20\\uCE3C\\uCE58\\uCE74\\uCE90\\uCEAC\\uCEC8\\uCEE4"
        "\\uCF00\\uCF1C\\uCF38\\uCF54\\uCF70\\uCF8C\\uCFA8\\uCFC4\\uCFE0"
        "\\uCFFC\\uD018\\uD034\\uD050\\uD06C\\uD088\\uD0A4\\uD0C0\\uD0DC"
        "\\uD0F8\\uD114\\uD130\\uD14C\\uD168\\uD184\\uD1A0\\uD1BC\\uD1D8"
        "\\uD1F4\\uD210\\uD22C\\uD248\\uD264\\uD280\\uD29C\\uD2B8\\uD2D4"
        "\\uD2F0\\uD30C\\uD328\\uD344\\uD360\\uD37C\\uD398\\uD3B4\\uD3D0"
        "\\uD3EC\\uD408\\uD424\\uD440\\uD45C\\uD478\\uD494\\uD4B0\\uD4CC"
        "\\uD4E8\\uD504\\uD520\\uD53C\\uD558\\uD574\\uD590\\uD5AC\\uD5C8"
        "\\uD5E4\\uD600\\uD61C\\uD638\\uD654\\uD670\\uD68C\\uD6A8\\uD6C4"
        "\\uD6E0\\uD6FC\\uD718\\uD734\\uD750\\uD76C\\uD788\\uF900-\\uFA0D"
        "\\uFA10\\uFA12\\uFA15-\\uFA1E\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A"
        "-\\uFA2D\\uFA30-\\uFA6A\\uFA70-\\uFAD9\\uFB1D-\\uFB1F\\uFB2A-"
        "\\uFB36\\uFB38-\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46"
        "-\\uFB4E\\uFE20-\\uFE23\\U00010A0D\\U00010A0F\\U00010A38-\\U0001"
        "0A3A\\U00010A3F\\U0001D15E-\\U0001D169\\U0001D16D-\\U0001D172"
        "\\U0001D17B-\\U0001D182\\U0001D185-\\U0001D18B\\U0001D1AA-"
        "\\U0001D1AD\\U0001D1BB-\\U0001D1C0\\U0001D242-\\U0001D244\\U0002"
        "F800-\\U0002FA1D]"
        , ""), errorCode);

    skipSets[UNORM_NFKD].applyPattern(UnicodeString(
        "[^\\u00A0\\u00A8\\u00AA\\u00AF\\u00B2-\\u00B5\\u00B8-\\u00BA"
        "\\u00BC-\\u00BE\\u00C0-\\u00C5\\u00C7-\\u00CF\\u00D1-\\u00D6"
        "\\u00D9-\\u00DD\\u00E0-\\u00E5\\u00E7-\\u00EF\\u00F1-\\u00F6"
        "\\u00F9-\\u00FD\\u00FF-\\u010F\\u0112-\\u0125\\u0128-\\u0130"
        "\\u0132-\\u0137\\u0139-\\u0140\\u0143-\\u0149\\u014C-\\u0151"
        "\\u0154-\\u0165\\u0168-\\u017F\\u01A0\\u01A1\\u01AF\\u01B0"
        "\\u01C4-\\u01DC\\u01DE-\\u01E3\\u01E6-\\u01F5\\u01F8-\\u021B"
        "\\u021E\\u021F\\u0226-\\u0233\\u02B0-\\u02B8\\u02D8-\\u02DD"
        "\\u02E0-\\u02E4\\u0300-\\u034E\\u0350-\\u036F\\u0374\\u037A"
        "\\u037E\\u0384-\\u038A\\u038C\\u038E-\\u0390\\u03AA-\\u03B0"
        "\\u03CA-\\u03CE\\u03D0-\\u03D6\\u03F0-\\u03F2\\u03F4\\u03F5"
        "\\u03F9\\u0400\\u0401\\u0403\\u0407\\u040C-\\u040E\\u0419\\u0439"
        "\\u0450\\u0451\\u0453\\u0457\\u045C-\\u045E\\u0476\\u0477\\u0483"
        "-\\u0486\\u04C1\\u04C2\\u04D0-\\u04D3\\u04D6\\u04D7\\u04DA-"
        "\\u04DF\\u04E2-\\u04E7\\u04EA-\\u04F5\\u04F8\\u04F9\\u0587"
        "\\u0591-\\u05BD\\u05BF\\u05C1\\u05C2\\u05C4\\u05C5\\u05C7\\u0610"
        "-\\u0615\\u0622-\\u0626\\u064B-\\u065E\\u0670\\u0675-\\u0678"
        "\\u06C0\\u06C2\\u06D3\\u06D6-\\u06DC\\u06DF-\\u06E4\\u06E7"
        "\\u06E8\\u06EA-\\u06ED\\u0711\\u0730-\\u074A\\u07EB-\\u07F3"
        "\\u0929\\u0931\\u0934\\u093C\\u094D\\u0951-\\u0954\\u0958-"
        "\\u095F\\u09BC\\u09CB-\\u09CD\\u09DC\\u09DD\\u09DF\\u0A33\\u0A36"
        "\\u0A3C\\u0A4D\\u0A59-\\u0A5B\\u0A5E\\u0ABC\\u0ACD\\u0B3C\\u0B48"
        "\\u0B4B-\\u0B4D\\u0B5C\\u0B5D\\u0B94\\u0BCA-\\u0BCD\\u0C48"
        "\\u0C4D\\u0C55\\u0C56\\u0CBC\\u0CC0\\u0CC7\\u0CC8\\u0CCA\\u0CCB"
        "\\u0CCD\\u0D4A-\\u0D4D\\u0DCA\\u0DDA\\u0DDC-\\u0DDE\\u0E33"
        "\\u0E38-\\u0E3A\\u0E48-\\u0E4B\\u0EB3\\u0EB8\\u0EB9\\u0EC8-"
        "\\u0ECB\\u0EDC\\u0EDD\\u0F0C\\u0F18\\u0F19\\u0F35\\u0F37\\u0F39"
        "\\u0F43\\u0F4D\\u0F52\\u0F57\\u0F5C\\u0F69\\u0F71-\\u0F7D\\u0F80"
        "-\\u0F84\\u0F86\\u0F87\\u0F93\\u0F9D\\u0FA2\\u0FA7\\u0FAC\\u0FB9"
        "\\u0FC6\\u1026\\u1037\\u1039\\u10FC\\u135F\\u1714\\u1734\\u17D2"
        "\\u17DD\\u18A9\\u1939-\\u193B\\u1A17\\u1A18\\u1B06\\u1B08\\u1B0A"
        "\\u1B0C\\u1B0E\\u1B12\\u1B34\\u1B3B\\u1B3D\\u1B40\\u1B41\\u1B43"
        "\\u1B44\\u1B6B-\\u1B73\\u1D2C-\\u1D2E\\u1D30-\\u1D3A\\u1D3C-"
        "\\u1D4D\\u1D4F-\\u1D6A\\u1D78\\u1D9B-\\u1DCA\\u1DFE-\\u1E9B"
        "\\u1EA0-\\u1EF9\\u1F00-\\u1F15\\u1F18-\\u1F1D\\u1F20-\\u1F45"
        "\\u1F48-\\u1F4D\\u1F50-\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F-"
        "\\u1F7D\\u1F80-\\u1FB4\\u1FB6-\\u1FC4\\u1FC6-\\u1FD3\\u1FD6-"
        "\\u1FDB\\u1FDD-\\u1FEF\\u1FF2-\\u1FF4\\u1FF6-\\u1FFE\\u2000-"
        "\\u200A\\u2011\\u2017\\u2024-\\u2026\\u202F\\u2033\\u2034\\u2036"
        "\\u2037\\u203C\\u203E\\u2047-\\u2049\\u2057\\u205F\\u2070\\u2071"
        "\\u2074-\\u208E\\u2090-\\u2094\\u20A8\\u20D0-\\u20DC\\u20E1"
        "\\u20E5-\\u20EF\\u2100-\\u2103\\u2105-\\u2107\\u2109-\\u2113"
        "\\u2115\\u2116\\u2119-\\u211D\\u2120-\\u2122\\u2124\\u2126"
        "\\u2128\\u212A-\\u212D\\u212F-\\u2131\\u2133-\\u2139\\u213B-"
        "\\u2140\\u2145-\\u2149\\u2153-\\u217F\\u219A\\u219B\\u21AE"
        "\\u21CD-\\u21CF\\u2204\\u2209\\u220C\\u2224\\u2226\\u222C\\u222D"
        "\\u222F\\u2230\\u2241\\u2244\\u2247\\u2249\\u2260\\u2262\\u226D-"
        "\\u2271\\u2274\\u2275\\u2278\\u2279\\u2280\\u2281\\u2284\\u2285"
        "\\u2288\\u2289\\u22AC-\\u22AF\\u22E0-\\u22E3\\u22EA-\\u22ED"
        "\\u2329\\u232A\\u2460-\\u24EA\\u2A0C\\u2A74-\\u2A76\\u2ADC"
        "\\u2D6F\\u2E9F\\u2EF3\\u2F00-\\u2FD5\\u3000\\u302A-\\u302F"
        "\\u3036\\u3038-\\u303A\\u304C\\u304E\\u3050\\u3052\\u3054\\u3056"
        "\\u3058\\u305A\\u305C\\u305E\\u3060\\u3062\\u3065\\u3067\\u3069"
        "\\u3070\\u3071\\u3073\\u3074\\u3076\\u3077\\u3079\\u307A\\u307C"
        "\\u307D\\u3094\\u3099-\\u309C\\u309E\\u309F\\u30AC\\u30AE\\u30B0"
        "\\u30B2\\u30B4\\u30B6\\u30B8\\u30BA\\u30BC\\u30BE\\u30C0\\u30C2"
        "\\u30C5\\u30C7\\u30C9\\u30D0\\u30D1\\u30D3\\u30D4\\u30D6\\u30D7"
        "\\u30D9\\u30DA\\u30DC\\u30DD\\u30F4\\u30F7-\\u30FA\\u30FE\\u30FF"
        "\\u3131-\\u318E\\u3192-\\u319F\\u3200-\\u321E\\u3220-\\u3243"
        "\\u3250-\\u327E\\u3280-\\u32FE\\u3300-\\u33FF\\uA806\\uAC00-"
        "\\uD7A3\\uF900-\\uFA0D\\uFA10\\uFA12\\uFA15-\\uFA1E\\uFA20"
        "\\uFA22\\uFA25\\uFA26\\uFA2A-\\uFA2D\\uFA30-\\uFA6A\\uFA70-"
        "\\uFAD9\\uFB00-\\uFB06\\uFB13-\\uFB17\\uFB1D-\\uFB36\\uFB38-"
        "\\uFB3C\\uFB3E\\uFB40\\uFB41\\uFB43\\uFB44\\uFB46-\\uFBB1\\uFBD3"
        "-\\uFD3D\\uFD50-\\uFD8F\\uFD92-\\uFDC7\\uFDF0-\\uFDFC\\uFE10-"
        "\\uFE19\\uFE20-\\uFE23\\uFE30-\\uFE44\\uFE47-\\uFE52\\uFE54-"
        "\\uFE66\\uFE68-\\uFE6B\\uFE70-\\uFE72\\uFE74\\uFE76-\\uFEFC"
        "\\uFF01-\\uFFBE\\uFFC2-\\uFFC7\\uFFCA-\\uFFCF\\uFFD2-\\uFFD7"
        "\\uFFDA-\\uFFDC\\uFFE0-\\uFFE6\\uFFE8-\\uFFEE\\U00010A0D\\U00010"
        "A0F\\U00010A38-\\U00010A3A\\U00010A3F\\U0001D15E-\\U0001D169"
        "\\U0001D16D-\\U0001D172\\U0001D17B-\\U0001D182\\U0001D185-"
        "\\U0001D18B\\U0001D1AA-\\U0001D1AD\\U0001D1BB-\\U0001D1C0\\U0001"
        "D242-\\U0001D244\\U0001D400-\\U0001D454\\U0001D456-\\U0001D49C"
        "\\U0001D49E\\U0001D49F\\U0001D4A2\\U0001D4A5\\U0001D4A6\\U0001D4"
        "A9-\\U0001D4AC\\U0001D4AE-\\U0001D4B9\\U0001D4BB\\U0001D4BD-"
        "\\U0001D4C3\\U0001D4C5-\\U0001D505\\U0001D507-\\U0001D50A\\U0001"
        "D50D-\\U0001D514\\U0001D516-\\U0001D51C\\U0001D51E-\\U0001D539"
        "\\U0001D53B-\\U0001D53E\\U0001D540-\\U0001D544\\U0001D546\\U0001"
        "D54A-\\U0001D550\\U0001D552-\\U0001D6A5\\U0001D6A8-\\U0001D7CB"
        "\\U0001D7CE-\\U0001D7FF\\U0002F800-\\U0002FA1D]"
        , ""), errorCode);

    skipSets[UNORM_NFKC].applyPattern(UnicodeString(
        "[^<->A-PR-Za-pr-z\\u00A0\\u00A8\\u00AA\\u00AF\\u00B2-\\u00B5"
        "\\u00B8-\\u00BA\\u00BC-\\u00BE\\u00C0-\\u00CF\\u00D1-\\u00D6"
        "\\u00D8-\\u00DD\\u00E0-\\u00EF\\u00F1-\\u00F6\\u00F8-\\u00FD"
        "\\u00FF-\\u0103\\u0106-\\u010F\\u0112-\\u0117\\u011A-\\u0121"
        "\\u0124\\u0125\\u0128-\\u012D\\u0130\\u0132\\u0133\\u0139\\u013A"
        "\\u013D-\\u0140\\u0143\\u0144\\u0147-\\u0149\\u014C-\\u0151"
        "\\u0154\\u0155\\u0158-\\u015D\\u0160\\u0161\\u0164\\u0165\\u0168"
        "-\\u0171\\u0174-\\u017F\\u01A0\\u01A1\\u01AF\\u01B0\\u01B7"
        "\\u01C4-\\u01DC\\u01DE-\\u01E1\\u01E6-\\u01EB\\u01F1-\\u01F5"
        "\\u01F8-\\u01FB\\u0200-\\u021B\\u021E\\u021F\\u0226-\\u0233"
        "\\u0292\\u02B0-\\u02B8\\u02D8-\\u02DD\\u02E0-\\u02E4\\u0300-"
        "\\u034E\\u0350-\\u036F\\u0374\\u037A\\u037E\\u0384\\u0385\\u0387"
        "\\u0391\\u0395\\u0397\\u0399\\u039F\\u03A1\\u03A5\\u03A9\\u03AC"
        "\\u03AE\\u03B1\\u03B5\\u03B7\\u03B9\\u03BF\\u03C1\\u03C5\\u03C9-"
        "\\u03CB\\u03CE\\u03D0-\\u03D6\\u03F0-\\u03F2\\u03F4\\u03F5"
        "\\u03F9\\u0406\\u0410\\u0413\\u0415-\\u0418\\u041A\\u041E\\u0423"
        "\\u0427\\u042B\\u042D\\u0430\\u0433\\u0435-\\u0438\\u043A\\u043E"
        "\\u0443\\u0447\\u044B\\u044D\\u0456\\u0474\\u0475\\u0483-\\u0486"
        "\\u04D8\\u04D9\\u04E8\\u04E9\\u0587\\u0591-\\u05BD\\u05BF\\u05C1"
        "\\u05C2\\u05C4\\u05C5\\u05C7\\u0610-\\u0615\\u0622\\u0623\\u0627"
        "\\u0648\\u064A-\\u065E\\u0670\\u0675-\\u0678\\u06C1\\u06D2"
        "\\u06D5-\\u06DC\\u06DF-\\u06E4\\u06E7\\u06E8\\u06EA-\\u06ED"
        "\\u0711\\u0730-\\u074A\\u07EB-\\u07F3\\u0928\\u0930\\u0933"
        "\\u093C\\u094D\\u0951-\\u0954\\u0958-\\u095F\\u09BC\\u09BE"
        "\\u09C7\\u09CD\\u09D7\\u09DC\\u09DD\\u09DF\\u0A33\\u0A36\\u0A3C"
        "\\u0A4D\\u0A59-\\u0A5B\\u0A5E\\u0ABC\\u0ACD\\u0B3C\\u0B3E\\u0B47"
        "\\u0B4D\\u0B56\\u0B57\\u0B5C\\u0B5D\\u0B92\\u0BBE\\u0BC6\\u0BC7"
        "\\u0BCD\\u0BD7\\u0C46\\u0C4D\\u0C55\\u0C56\\u0CBC\\u0CBF\\u0CC2"
        "\\u0CC6\\u0CCA\\u0CCD\\u0CD5\\u0CD6\\u0D3E\\u0D46\\u0D47\\u0D4D"
        "\\u0D57\\u0DCA\\u0DCF\\u0DD9\\u0DDC\\u0DDF\\u0E33\\u0E38-\\u0E3A"
        "\\u0E48-\\u0E4B\\u0EB3\\u0EB8\\u0EB9\\u0EC8-\\u0ECB\\u0EDC"
        "\\u0EDD\\u0F0C\\u0F18\\u0F19\\u0F35\\u0F37\\u0F39\\u0F43\\u0F4D"
        "\\u0F52\\u0F57\\u0F5C\\u0F69\\u0F71-\\u0F7D\\u0F80-\\u0F84"
        "\\u0F86\\u0F87\\u0F93\\u0F9D\\u0FA2\\u0FA7\\u0FAC\\u0FB9\\u0FC6"
        "\\u1025\\u102E\\u1037\\u1039\\u10FC\\u1100-\\u1112\\u1161-"
        "\\u1175\\u11A8-\\u11C2\\u135F\\u1714\\u1734\\u17D2\\u17DD\\u18A9"
        "\\u1939-\\u193B\\u1A17\\u1A18\\u1B05\\u1B07\\u1B09\\u1B0B\\u1B0D"
        "\\u1B11\\u1B34\\u1B35\\u1B3A\\u1B3C\\u1B3E\\u1B3F\\u1B42\\u1B44"
        "\\u1B6B-\\u1B73\\u1D2C-\\u1D2E\\u1D30-\\u1D3A\\u1D3C-\\u1D4D"
        "\\u1D4F-\\u1D6A\\u1D78\\u1D9B-\\u1DCA\\u1DFE-\\u1E03\\u1E0A-"
        "\\u1E0F\\u1E12-\\u1E1B\\u1E20-\\u1E27\\u1E2A-\\u1E41\\u1E44-"
        "\\u1E53\\u1E58-\\u1E7D\\u1E80-\\u1E87\\u1E8E-\\u1E91\\u1E96-"
        "\\u1E9B\\u1EA0-\\u1EF3\\u1EF6-\\u1EF9\\u1F00-\\u1F11\\u1F18"
        "\\u1F19\\u1F20-\\u1F31\\u1F38\\u1F39\\u1F40\\u1F41\\u1F48\\u1F49"
        "\\u1F50\\u1F51\\u1F59\\u1F60-\\u1F71\\u1F73-\\u1F75\\u1F77"
        "\\u1F79\\u1F7B-\\u1F7D\\u1F80\\u1F81\\u1F88\\u1F89\\u1F90\\u1F91"
        "\\u1F98\\u1F99\\u1FA0\\u1FA1\\u1FA8\\u1FA9\\u1FB3\\u1FB6\\u1FBB-"
        "\\u1FC1\\u1FC3\\u1FC6\\u1FC9\\u1FCB-\\u1FCF\\u1FD3\\u1FDB\\u1FDD"
        "-\\u1FDF\\u1FE3\\u1FEB\\u1FED-\\u1FEF\\u1FF3\\u1FF6\\u1FF9"
        "\\u1FFB-\\u1FFE\\u2000-\\u200A\\u2011\\u2017\\u2024-\\u2026"
        "\\u202F\\u2033\\u2034\\u2036\\u2037\\u203C\\u203E\\u2047-\\u2049"
        "\\u2057\\u205F\\u2070\\u2071\\u2074-\\u208E\\u2090-\\u2094"
        "\\u20A8\\u20D0-\\u20DC\\u20E1\\u20E5-\\u20EF\\u2100-\\u2103"
        "\\u2105-\\u2107\\u2109-\\u2113\\u2115\\u2116\\u2119-\\u211D"
        "\\u2120-\\u2122\\u2124\\u2126\\u2128\\u212A-\\u212D\\u212F-"
        "\\u2131\\u2133-\\u2139\\u213B-\\u2140\\u2145-\\u2149\\u2153-"
        "\\u217F\\u2190\\u2192\\u2194\\u21D0\\u21D2\\u21D4\\u2203\\u2208"
        "\\u220B\\u2223\\u2225\\u222C\\u222D\\u222F\\u2230\\u223C\\u2243"
        "\\u2245\\u2248\\u224D\\u2261\\u2264\\u2265\\u2272\\u2273\\u2276"
        "\\u2277\\u227A-\\u227D\\u2282\\u2283\\u2286\\u2287\\u2291\\u2292"
        "\\u22A2\\u22A8\\u22A9\\u22AB\\u22B2-\\u22B5\\u2329\\u232A\\u2460"
        "-\\u24EA\\u2A0C\\u2A74-\\u2A76\\u2ADC\\u2D6F\\u2E9F\\u2EF3"
        "\\u2F00-\\u2FD5\\u3000\\u302A-\\u302F\\u3036\\u3038-\\u303A"
        "\\u3046\\u304B\\u304D\\u304F\\u3051\\u3053\\u3055\\u3057\\u3059"
        "\\u305B\\u305D\\u305F\\u3061\\u3064\\u3066\\u3068\\u306F\\u3072"
        "\\u3075\\u3078\\u307B\\u3099-\\u309D\\u309F\\u30A6\\u30AB\\u30AD"
        "\\u30AF\\u30B1\\u30B3\\u30B5\\u30B7\\u30B9\\u30BB\\u30BD\\u30BF"
        "\\u30C1\\u30C4\\u30C6\\u30C8\\u30CF\\u30D2\\u30D5\\u30D8\\u30DB"
        "\\u30EF-\\u30F2\\u30FD\\u30FF\\u3131-\\u318E\\u3192-\\u319F"
        "\\u3200-\\u321E\\u3220-\\u3243\\u3250-\\u327E\\u3280-\\u32FE"
        "\\u3300-\\u33FF\\uA806\\uAC00\\uAC1C\\uAC38\\uAC54\\uAC70\\uAC8C"
        "\\uACA8\\uACC4\\uACE0\\uACFC\\uAD18\\uAD34\\uAD50\\uAD6C\\uAD88"
        "\\uADA4\\uADC0\\uADDC\\uADF8\\uAE14\\uAE30\\uAE4C\\uAE68\\uAE84"
        "\\uAEA0\\uAEBC\\uAED8\\uAEF4\\uAF10\\uAF2C\\uAF48\\uAF64\\uAF80"
        "\\uAF9C\\uAFB8\\uAFD4\\uAFF0\\uB00C\\uB028\\uB044\\uB060\\uB07C"
        "\\uB098\\uB0B4\\uB0D0\\uB0EC\\uB108\\uB124\\uB140\\uB15C\\uB178"
        "\\uB194\\uB1B0\\uB1CC\\uB1E8\\uB204\\uB220\\uB23C\\uB258\\uB274"
        "\\uB290\\uB2AC\\uB2C8\\uB2E4\\uB300\\uB31C\\uB338\\uB354\\uB370"
        "\\uB38C\\uB3A8\\uB3C4\\uB3E0\\uB3FC\\uB418\\uB434\\uB450\\uB46C"
        "\\uB488\\uB4A4\\uB4C0\\uB4DC\\uB4F8\\uB514\\uB530\\uB54C\\uB568"
        "\\uB584\\uB5A0\\uB5BC\\uB5D8\\uB5F4\\uB610\\uB62C\\uB648\\uB664"
        "\\uB680\\uB69C\\uB6B8\\uB6D4\\uB6F0\\uB70C\\uB728\\uB744\\uB760"
        "\\uB77C\\uB798\\uB7B4\\uB7D0\\uB7EC\\uB808\\uB824\\uB840\\uB85C"
        "\\uB878\\uB894\\uB8B0\\uB8CC\\uB8E8\\uB904\\uB920\\uB93C\\uB958"
        "\\uB974\\uB990\\uB9AC\\uB9C8\\uB9E4\\uBA00\\uBA1C\\uBA38\\uBA54"
        "\\uBA70\\uBA8C\\uBAA8\\uBAC4\\uBAE0\\uBAFC\\uBB18\\uBB34\\uBB50"
        "\\uBB6C\\uBB88\\uBBA4\\uBBC0\\uBBDC\\uBBF8\\uBC14\\uBC30\\uBC4C"
        "\\uBC68\\uBC84\\uBCA0\\uBCBC\\uBCD8\\uBCF4\\uBD10\\uBD2C\\uBD48"
        "\\uBD64\\uBD80\\uBD9C\\uBDB8\\uBDD4\\uBDF0\\uBE0C\\uBE28\\uBE44"
        "\\uBE60\\uBE7C\\uBE98\\uBEB4\\uBED0\\uBEEC\\uBF08\\uBF24\\uBF40"
        "\\uBF5C\\uBF78\\uBF94\\uBFB0\\uBFCC\\uBFE8\\uC004\\uC020\\uC03C"
        "\\uC058\\uC074\\uC090\\uC0AC\\uC0C8\\uC0E4\\uC100\\uC11C\\uC138"
        "\\uC154\\uC170\\uC18C\\uC1A8\\uC1C4\\uC1E0\\uC1FC\\uC218\\uC234"
        "\\uC250\\uC26C\\uC288\\uC2A4\\uC2C0\\uC2DC\\uC2F8\\uC314\\uC330"
        "\\uC34C\\uC368\\uC384\\uC3A0\\uC3BC\\uC3D8\\uC3F4\\uC410\\uC42C"
        "\\uC448\\uC464\\uC480\\uC49C\\uC4B8\\uC4D4\\uC4F0\\uC50C\\uC528"
        "\\uC544\\uC560\\uC57C\\uC598\\uC5B4\\uC5D0\\uC5EC\\uC608\\uC624"
        "\\uC640\\uC65C\\uC678\\uC694\\uC6B0\\uC6CC\\uC6E8\\uC704\\uC720"
        "\\uC73C\\uC758\\uC774\\uC790\\uC7AC\\uC7C8\\uC7E4\\uC800\\uC81C"
        "\\uC838\\uC854\\uC870\\uC88C\\uC8A8\\uC8C4\\uC8E0\\uC8FC\\uC918"
        "\\uC934\\uC950\\uC96C\\uC988\\uC9A4\\uC9C0\\uC9DC\\uC9F8\\uCA14"
        "\\uCA30\\uCA4C\\uCA68\\uCA84\\uCAA0\\uCABC\\uCAD8\\uCAF4\\uCB10"
        "\\uCB2C\\uCB48\\uCB64\\uCB80\\uCB9C\\uCBB8\\uCBD4\\uCBF0\\uCC0C"
        "\\uCC28\\uCC44\\uCC60\\uCC7C\\uCC98\\uCCB4\\uCCD0\\uCCEC\\uCD08"
        "\\uCD24\\uCD40\\uCD5C\\uCD78\\uCD94\\uCDB0\\uCDCC\\uCDE8\\uCE04"
        "\\uCE20\\uCE3C\\uCE58\\uCE74\\uCE90\\uCEAC\\uCEC8\\uCEE4\\uCF00"
        "\\uCF1C\\uCF38\\uCF54\\uCF70\\uCF8C\\uCFA8\\uCFC4\\uCFE0\\uCFFC"
        "\\uD018\\uD034\\uD050\\uD06C\\uD088\\uD0A4\\uD0C0\\uD0DC\\uD0F8"
        "\\uD114\\uD130\\uD14C\\uD168\\uD184\\uD1A0\\uD1BC\\uD1D8\\uD1F4"
        "\\uD210\\uD22C\\uD248\\uD264\\uD280\\uD29C\\uD2B8\\uD2D4\\uD2F0"
        "\\uD30C\\uD328\\uD344\\uD360\\uD37C\\uD398\\uD3B4\\uD3D0\\uD3EC"
        "\\uD408\\uD424\\uD440\\uD45C\\uD478\\uD494\\uD4B0\\uD4CC\\uD4E8"
        "\\uD504\\uD520\\uD53C\\uD558\\uD574\\uD590\\uD5AC\\uD5C8\\uD5E4"
        "\\uD600\\uD61C\\uD638\\uD654\\uD670\\uD68C\\uD6A8\\uD6C4\\uD6E0"
        "\\uD6FC\\uD718\\uD734\\uD750\\uD76C\\uD788\\uF900-\\uFA0D\\uFA10"
        "\\uFA12\\uFA15-\\uFA1E\\uFA20\\uFA22\\uFA25\\uFA26\\uFA2A-"
        "\\uFA2D\\uFA30-\\uFA6A\\uFA70-\\uFAD9\\uFB00-\\uFB06\\uFB13-"
        "\\uFB17\\uFB1D-\\uFB36\\uFB38-\\uFB3C\\uFB3E\\uFB40\\uFB41"
        "\\uFB43\\uFB44\\uFB46-\\uFBB1\\uFBD3-\\uFD3D\\uFD50-\\uFD8F"
        "\\uFD92-\\uFDC7\\uFDF0-\\uFDFC\\uFE10-\\uFE19\\uFE20-\\uFE23"
        "\\uFE30-\\uFE44\\uFE47-\\uFE52\\uFE54-\\uFE66\\uFE68-\\uFE6B"
        "\\uFE70-\\uFE72\\uFE74\\uFE76-\\uFEFC\\uFF01-\\uFFBE\\uFFC2-"
        "\\uFFC7\\uFFCA-\\uFFCF\\uFFD2-\\uFFD7\\uFFDA-\\uFFDC\\uFFE0-"
        "\\uFFE6\\uFFE8-\\uFFEE\\U00010A0D\\U00010A0F\\U00010A38-\\U00010"
        "A3A\\U00010A3F\\U0001D15E-\\U0001D169\\U0001D16D-\\U0001D172"
        "\\U0001D17B-\\U0001D182\\U0001D185-\\U0001D18B\\U0001D1AA-"
        "\\U0001D1AD\\U0001D1BB-\\U0001D1C0\\U0001D242-\\U0001D244\\U0001"
        "D400-\\U0001D454\\U0001D456-\\U0001D49C\\U0001D49E\\U0001D49F"
        "\\U0001D4A2\\U0001D4A5\\U0001D4A6\\U0001D4A9-\\U0001D4AC\\U0001D"
        "4AE-\\U0001D4B9\\U0001D4BB\\U0001D4BD-\\U0001D4C3\\U0001D4C5-"
        "\\U0001D505\\U0001D507-\\U0001D50A\\U0001D50D-\\U0001D514\\U0001"
        "D516-\\U0001D51C\\U0001D51E-\\U0001D539\\U0001D53B-\\U0001D53E"
        "\\U0001D540-\\U0001D544\\U0001D546\\U0001D54A-\\U0001D550\\U0001"
        "D552-\\U0001D6A5\\U0001D6A8-\\U0001D7CB\\U0001D7CE-\\U0001D7FF"
        "\\U0002F800-\\U0002FA1D]"
        , ""), errorCode);
}

U_CDECL_BEGIN

// USetAdder implementation
// Does not use uset.h to reduce code dependencies
static void U_CALLCONV
_set_add(USet *set, UChar32 c) {
    uset_add(set, c);
}

static void U_CALLCONV
_set_addRange(USet *set, UChar32 start, UChar32 end) {
    uset_addRange(set, start, end);
}

static void U_CALLCONV
_set_addString(USet *set, const UChar *str, int32_t length) {
    uset_addString(set, str, length);
}

U_CDECL_END

void
BasicNormalizerTest::TestSkippable() {
    UnicodeSet starts, diff, skipSets[UNORM_MODE_COUNT], expectSets[UNORM_MODE_COUNT];
    UnicodeSet *startsPtr = &starts;
    UnicodeString s, pattern;
    UChar32 start, limit, rangeStart, rangeEnd;
    int32_t i, range, count;

    UErrorCode status;

    /* build NF*Skippable sets from runtime data */
    status=U_ZERO_ERROR;
    USetAdder sa = {
        (USet *)startsPtr,
        _set_add,
        _set_addRange,
        _set_addString,
        NULL // don't need remove()
    };
    unorm_addPropertyStarts(&sa, &status);
    if(U_FAILURE(status)) {
        errln("unable to load normalization data for unorm_addPropertyStarts(() - %s\n", u_errorName(status));
        return;
    }
    count=starts.getRangeCount();

    start=limit=0;
    rangeStart=rangeEnd=0;
    range=0;
    for(;;) {
        if(start<limit) {
            /* get properties for start and apply them to [start..limit[ */
            if(unorm_isNFSkippable(start, UNORM_NFD)) {
                skipSets[UNORM_NFD].add(start, limit-1);
            }
            if(unorm_isNFSkippable(start, UNORM_NFKD)) {
                skipSets[UNORM_NFKD].add(start, limit-1);
            }
            if(unorm_isNFSkippable(start, UNORM_NFC)) {
                skipSets[UNORM_NFC].add(start, limit-1);
            }
            if(unorm_isNFSkippable(start, UNORM_NFKC)) {
                skipSets[UNORM_NFKC].add(start, limit-1);
            }
        }

        /* go to next range of same properties */
        start=limit;
        if(++limit>rangeEnd) {
            if(range<count) {
                limit=rangeStart=starts.getRangeStart(range);
                rangeEnd=starts.getRangeEnd(range);
                ++range;
            } else if(range==count) {
                /* additional range to complete the Unicode code space */
                limit=rangeStart=rangeEnd=0x110000;
                ++range;
            } else {
                break;
            }
        }
    }

    /* get expected sets from hardcoded patterns */
    initExpectedSkippables(expectSets);

    for(i=UNORM_NONE; i<UNORM_MODE_COUNT; ++i) {
        if(skipSets[i]!=expectSets[i]) {
            errln("error: TestSkippable skipSets[%d]!=expectedSets[%d]\n"
                  "may need to update hardcoded UnicodeSet patterns in\n"
                  "tstnorm.cpp/initExpectedSkippables(),\n"
                  "see ICU4J - unicodetools.com.ibm.text.UCD.NFSkippable\n",
                  i, i);

            s=UNICODE_STRING_SIMPLE("skip-expect=");
            (diff=skipSets[i]).removeAll(expectSets[i]).toPattern(pattern, TRUE);
            s.append(pattern);

            pattern.remove();
            s.append(UNICODE_STRING_SIMPLE("\n\nexpect-skip="));
            (diff=expectSets[i]).removeAll(skipSets[i]).toPattern(pattern, TRUE);
            s.append(pattern);
            s.append(UNICODE_STRING_SIMPLE("\n\n"));

            errln(s);
        }
    }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */
