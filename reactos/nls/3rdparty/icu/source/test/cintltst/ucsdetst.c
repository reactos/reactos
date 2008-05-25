/*
 ****************************************************************************
 * Copyright (c) 2005-2006, International Business Machines Corporation and *
 * others. All Rights Reserved.                                             *
 ****************************************************************************
 */

#include "unicode/utypes.h"

#include "unicode/ucsdet.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"

#include "cintltst.h"

#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define NEW_ARRAY(type,count) (type *) malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) free(array)

static void TestConstruction(void);
static void TestUTF8(void);
static void TestUTF16(void);
static void TestC1Bytes(void);
static void TestInputFilter(void);
static void TestChaining(void);

void addUCsdetTest(TestNode** root);

void addUCsdetTest(TestNode** root)
{
    addTest(root, &TestConstruction, "ucsdetst/TestConstruction");
    addTest(root, &TestUTF8, "ucsdetst/TestUTF8");
    addTest(root, &TestUTF16, "ucsdetst/TestUTF16");
    addTest(root, &TestC1Bytes, "ucsdetst/TestC1Bytes");
    addTest(root, &TestInputFilter, "ucsdetst/TestInputFilter");
    addTest(root, &TestChaining, "ucsdetst/TestErrorChaining");
}

static int32_t preflight(const UChar *src, int32_t length, UConverter *cnv)
{
    UErrorCode status;
    char buffer[1024];
    char *dest, *destLimit = buffer + sizeof(buffer);
    const UChar *srcLimit = src + length;
    int32_t result = 0;

    do {
        dest = buffer;
        status = U_ZERO_ERROR;
        ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, &status);
        result += (int32_t) (dest - buffer);
    } while (status == U_BUFFER_OVERFLOW_ERROR);

    return result;
}

static char *extractBytes(const UChar *src, int32_t length, const char *codepage, int32_t *byteLength)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *cnv = ucnv_open(codepage, &status);
    int32_t byteCount = preflight(src, length, cnv);
    const UChar *srcLimit = src + length;
    char *bytes = NEW_ARRAY(char, byteCount + 1);
    char *dest = bytes, *destLimit = bytes + byteCount + 1;

    ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, &status);
    ucnv_close(cnv);

    *byteLength = byteCount;
    return bytes;
}

static void freeBytes(char *bytes)
{
    DELETE_ARRAY(bytes);
}

static void TestConstruction(void)
{
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *csd = ucsdet_open(&status);
    UEnumeration *e = ucsdet_getAllDetectableCharsets(csd, &status);
    const char *name;
    int32_t count = uenum_count(e, &status);
    int32_t i, length;

    for(i = 0; i < count; i += 1) {
        name = uenum_next(e, &length, &status);

        if(name == NULL || length <= 0) {
            log_err("ucsdet_getAllDetectableCharsets() returned a null or empty name!\n");
        }
    }
    /* one past the list of all names must return NULL */
    name = uenum_next(e, &length, &status);
    if(name != NULL || length != 0 || U_FAILURE(status)) {
        log_err("ucsdet_getAllDetectableCharsets(past the list) returned a non-null name!\n");
    }

    uenum_close(e);
    ucsdet_close(csd);
}

static void TestUTF8(void)
{
    UErrorCode status = U_ZERO_ERROR;
    static const char ss[] = "This is a string with some non-ascii characters that will "
               "be converted to UTF-8, then shoved through the detection process.  "
               "\\u0391\\u0392\\u0393\\u0394\\u0395"
               "Sure would be nice if our source could contain Unicode directly!";
    int32_t byteLength = 0, sLength = 0, dLength = 0;
    UChar s[sizeof(ss)];
    char *bytes;
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    UChar detected[sizeof(ss)];

    sLength = u_unescape(ss, s, sizeof(ss));
    bytes = extractBytes(s, sLength, "UTF-8", &byteLength);

    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("Detection failure for UTF-8: got no matches.\n");
        goto bail;
    }

    dLength = ucsdet_getUChars(match, detected, sLength, &status);

    if (u_strCompare(detected, dLength, s, sLength, FALSE) != 0) {
        log_err("Round-trip test failed!\n");
    }

    ucsdet_setDeclaredEncoding(csd, "UTF-8", 5, &status); /* for coverage */

bail:
    freeBytes(bytes);
    ucsdet_close(csd);
}

static void TestUTF16(void)
{
    UErrorCode status = U_ZERO_ERROR;
    /* Notice the BOM on the start of this string */
    static const UChar chars[] = {
        0xFEFF, 0x0623, 0x0648, 0x0631, 0x0648, 0x0628, 0x0627, 0x002C,
        0x0020, 0x0628, 0x0631, 0x0645, 0x062c, 0x064a, 0x0627, 0x062a,
        0x0020, 0x0627, 0x0644, 0x062d, 0x0627, 0x0633, 0x0648, 0x0628,
        0x0020, 0x002b, 0x0020, 0x0627, 0x0646, 0x062a, 0x0631, 0x0646,
        0x064a, 0x062a, 0x0000};
    int32_t beLength = 0, leLength = 0, cLength = ARRAY_SIZE(chars);
    char *beBytes = extractBytes(chars, cLength, "UTF-16BE", &beLength);
    char *leBytes = extractBytes(chars, cLength, "UTF-16LE", &leLength);
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *name;
    int32_t conf;

    ucsdet_setText(csd, beBytes, beLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("Encoding detection failure for UTF-16BE: got no matches.\n");
        goto try_le;
    }

    name  = ucsdet_getName(match, &status);
    conf  = ucsdet_getConfidence(match, &status);

    if (strcmp(name, "UTF-16BE") != 0) {
        log_err("Encoding detection failure for UTF-16BE: got %s\n", name);
    }

    if (conf != 100) {
        log_err("Did not get 100%% confidence for UTF-16BE: got %d\n", conf);
    }

try_le:
    ucsdet_setText(csd, leBytes, leLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("Encoding detection failure for UTF-16LE: got no matches.\n");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    conf = ucsdet_getConfidence(match, &status);


    if (strcmp(name, "UTF-16LE") != 0) {
        log_err("Enconding detection failure for UTF-16LE: got %s\n", name);
    }

    if (conf != 100) {
        log_err("Did not get 100%% confidence for UTF-16LE: got %d\n", conf);
    }

bail:
    freeBytes(leBytes);
    freeBytes(beBytes);
    ucsdet_close(csd);
}

static void TestC1Bytes(void)
{
#if !UCONFIG_NO_LEGACY_CONVERSION
    UErrorCode status = U_ZERO_ERROR;
    static const char ssISO[] = "This is a small sample of some English text. Just enough to be sure that it detects correctly.";
    static const char ssWindows[] = "This is another small sample of some English text. Just enough to be sure that it detects correctly. It also includes some \\u201CC1\\u201D bytes.";
    int32_t sISOLength = 0, sWindowsLength = 0;
    UChar sISO[sizeof(ssISO)];
    UChar sWindows[sizeof(ssWindows)];
    int32_t lISO = 0, lWindows = 0;
    char *bISO;
    char *bWindows;
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *name;

    sISOLength = u_unescape(ssISO, sISO, sizeof(ssISO));
    sWindowsLength = u_unescape(ssWindows, sWindows, sizeof(ssWindows));
    bISO = extractBytes(sISO, sISOLength, "ISO-8859-1", &lISO);
    bWindows = extractBytes(sWindows, sWindowsLength, "windows-1252", &lWindows);

    ucsdet_setText(csd, bWindows, lWindows, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("English test with C1 bytes got no matches.\n");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);

    if (strcmp(name, "windows-1252") != 0) {
        log_err("English text with C1 bytes does not detect as windows-1252, but as %s\n", name);
    }

    ucsdet_setText(csd, bISO, lISO, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("English text without C1 bytes got no matches.\n");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);

    if (strcmp(name, "ISO-8859-1") != 0) {
        log_err("English text without C1 bytes does not detect as ISO-8859-1, but as %s\n", name);
    }

bail:
    freeBytes(bWindows);
    freeBytes(bISO);

    ucsdet_close(csd);
#endif
}

static void TestInputFilter(void)
{
    UErrorCode status = U_ZERO_ERROR;
    static const char ss[] = "<a> <lot> <of> <English> <inside> <the> <markup> Un tr\\u00E8s petit peu de Fran\\u00E7ais. <to> <confuse> <the> <detector>";
    int32_t sLength = 0;
    UChar s[sizeof(ss)];
    int32_t byteLength = 0;
    char *bytes;
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *lang, *name;

    sLength = u_unescape(ss, s, sizeof(ss));
    bytes = extractBytes(s, sLength, "ISO-8859-1", &byteLength);

    ucsdet_enableInputFilter(csd, TRUE);

    if (!ucsdet_isInputFilterEnabled(csd)) {
        log_err("ucsdet_enableInputFilter(csd, TRUE) did not enable input filter!\n");
    }


    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("Turning on the input filter resulted in no matches.\n");
        goto turn_off;
    }

    name = ucsdet_getName(match, &status);

    if (name == NULL || strcmp(name, "ISO-8859-1") != 0) {
        log_err("Turning on the input filter resulted in %s rather than ISO-8859-1\n", name);
    } else {
        lang = ucsdet_getLanguage(match, &status);

        if (lang == NULL || strcmp(lang, "fr") != 0) {
            log_err("Input filter did not strip markup!\n");
        }
    }

turn_off:
    ucsdet_enableInputFilter(csd, FALSE);
    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        log_err("Turning off the input filter resulted in no matches.\n");
        goto bail;
    }

    name = ucsdet_getName(match, &status);

    if (name == NULL || strcmp(name, "ISO-8859-1") != 0) {
        log_err("Turning off the input filter resulted in %s rather than ISO-8859-1\n", name);
    } else {
        lang = ucsdet_getLanguage(match, &status);

        if (lang == NULL || strcmp(lang, "en") != 0) {
            log_err("Unfiltered input did not detect as English!\n");
        }
    }

bail:
    freeBytes(bytes);
    ucsdet_close(csd);
}

static void TestChaining(void) {
    UErrorCode status = U_USELESS_COLLATOR_ERROR;

    ucsdet_open(&status);
    ucsdet_setText(NULL, NULL, 0, &status);
    ucsdet_getName(NULL, &status);
    ucsdet_getConfidence(NULL, &status);
    ucsdet_getLanguage(NULL, &status);
    ucsdet_detect(NULL, &status);
    ucsdet_setDeclaredEncoding(NULL, NULL, 0, &status);
    ucsdet_detectAll(NULL, NULL, &status);
    ucsdet_getUChars(NULL, NULL, 0, &status);
    ucsdet_getUChars(NULL, NULL, 0, &status);
    ucsdet_close(NULL);

    /* All of this code should have done nothing. */
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Status got changed to %s\n", u_errorName(status));
    }
}
