/*
*******************************************************************************
*
*   Copyright (C) 2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  udatpg_test.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2007aug01
*   created by: Markus W. Scherer
*
*   Test of the C wrapper for the DateTimePatternGenerator.
*   Calls each C API function and exercises code paths in the wrapper,
*   but the full functionality is tested in the C++ intltest.
*
*   One item to note: C API functions which return a const UChar *
*   should return a NUL-terminated string.
*   (The C++ implementation needs to use getTerminatedBuffer()
*   on UnicodeString objects which end up being returned this way.)
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/udatpg.h"
#include "unicode/ustring.h"
#include "cintltst.h"

void addDateTimePatternGeneratorTest(TestNode** root);

#define TESTCASE(x) addTest(root, &x, "tsformat/udatpg_test/" #x)

static void TestOpenClose(void);
static void TestUsage(void);
static void TestBuilder(void);

void addDateTimePatternGeneratorTest(TestNode** root) {
    TESTCASE(TestOpenClose);
    TESTCASE(TestUsage);
    TESTCASE(TestBuilder);
}

/*
 * Pipe symbol '|'. We pass only the first UChar without NUL-termination.
 * The second UChar is just to verify that the API does not pick that up.
 */
static const UChar pipeString[]={ 0x7c, 0x0a };

static const UChar testSkeleton1[]={ 0x48, 0x48, 0x6d, 0x6d, 0 }; /* HHmm */
static const UChar expectingBestPattern[]={ 0x48, 0x48, 0x2e, 0x6d, 0x6d, 0 }; /* HH.mm */
static const UChar testPattern[]={ 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0 }; /* HH:mm */
static const UChar expectingSkeleton[]= { 0x48, 0x48, 0x6d, 0x6d, 0 }; /* HHmm */
static const UChar expectingBaseSkeleton[]= { 0x48, 0x6d, 0 }; /* HHmm */
static const UChar redundantPattern[]={ 0x79, 0x79, 0x4d, 0x4d, 0x4d, 0 }; /* yyMMM */
static const UChar testFormat[]= {0x7B, 0x31, 0x7D, 0x20, 0x7B, 0x30, 0x7D, 0};  /* {1} {0} */
static const UChar appendItemName[]= {0x68, 0x72, 0};  /* hr */
static const UChar testPattern2[]={ 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0x20, 0x76, 0 }; /* HH:mm v */
static const UChar replacedStr[]={ 0x76, 0x76, 0x76, 0x76, 0 }; /* vvvv */
/* results for getBaseSkeletons() - {Hmv}, {yMMM} */
static const UChar resultBaseSkeletons[2][10] = {{0x48,0x6d, 0x76, 0}, {0x79, 0x4d, 0x4d, 0x4d, 0 } };


static void TestOpenClose() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg, *dtpg2;
    const UChar *s;
    int32_t length;

    /* Open a DateTimePatternGenerator for the default locale. */
    dtpg=udatpg_open(NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_open(NULL) failed - %s\n", u_errorName(errorCode));
        return;
    }
    udatpg_close(dtpg);

    /* Now one for German. */
    dtpg=udatpg_open("de", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_open(de) failed - %s\n", u_errorName(errorCode));
        return;
    }

    /* Make some modification which we verify gets passed on to the clone. */
    udatpg_setDecimal(dtpg, pipeString, 1);

    /* Clone the generator. */
    dtpg2=udatpg_clone(dtpg, &errorCode);
    if(U_FAILURE(errorCode) || dtpg2==NULL) {
        log_err("udatpg_clone() failed - %s\n", u_errorName(errorCode));
        return;
    }

    /* Verify that the clone has the custom decimal symbol. */
    s=udatpg_getDecimal(dtpg2, &length);
    if(s==pipeString || length!=1 || 0!=u_memcmp(s, pipeString, length) || s[length]!=0) { 
        log_err("udatpg_getDecimal(cloned object) did not return the expected string\n");
        return;
    }

    udatpg_close(dtpg);
    udatpg_close(dtpg2);
}

static void TestUsage() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg;
    UChar bestPattern[20];
    UChar result[20];
    int32_t length;    
    UChar *s;
    const UChar *r;
    
    dtpg=udatpg_open("fi", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_open(fi) failed - %s\n", u_errorName(errorCode));
        return;
    }
    length = udatpg_getBestPattern(dtpg, testSkeleton1, 4,
                                   bestPattern, 20, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getBestPattern failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(bestPattern, expectingBestPattern, length)!=0) || bestPattern[length]!=0) { 
        log_err("udatpg_getBestPattern did not return the expected string\n");
        return;
    }
    
    
    /* Test skeleton == NULL */
    s=NULL;
    length = udatpg_getBestPattern(dtpg, s, 0, bestPattern, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getBestPattern failed in illegal argument - skeleton is NULL.\n");
        return;
    }
    
    /* Test udatpg_getSkeleton */
    length = udatpg_getSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getSkeleton failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(result, expectingSkeleton, length)!=0) || result[length]!=0) { 
        log_err("udatpg_getSkeleton did not return the expected string\n");
        return;
    }
    
    /* Test pattern == NULL */
    s=NULL;
    length = udatpg_getSkeleton(dtpg, s, 0, result, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getSkeleton failed in illegal argument - pattern is NULL.\n");
        return;
    }    
    
    /* Test udatpg_getBaseSkeleton */
    length = udatpg_getBaseSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getBaseSkeleton failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(result, expectingBaseSkeleton, length)!=0) || result[length]!=0) { 
        log_err("udatpg_getBaseSkeleton did not return the expected string\n");
        return;
    }
    
    /* Test pattern == NULL */
    s=NULL;
    length = udatpg_getBaseSkeleton(dtpg, s, 0, result, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getBaseSkeleton failed in illegal argument - pattern is NULL.\n");
        return;
    }
    
    /* set append format to {1}{0} */
    udatpg_setAppendItemFormat( dtpg, UDATPG_MONTH_FIELD, testFormat, 7 );
    r = udatpg_getAppendItemFormat(dtpg, UDATPG_MONTH_FIELD, &length);
    
    
    if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) { 
        log_err("udatpg_setAppendItemFormat did not return the expected string\n");
        return;
    }
    
    /* set append name to hr */
    udatpg_setAppendItemName( dtpg, UDATPG_HOUR_FIELD, appendItemName, 7 );
    r = udatpg_getAppendItemName(dtpg, UDATPG_HOUR_FIELD, &length);
    
    if(length!=7 || 0!=u_memcmp(r, appendItemName, length) || r[length]!=0) { 
        log_err("udatpg_setAppendItemName did not return the expected string\n");
        return;
    }
    
    /* set date time format to {1}{0} */
    udatpg_setDateTimeFormat( dtpg, testFormat, 7 );
    r = udatpg_getDateTimeFormat(dtpg, &length);
    
    if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) { 
        log_err("udatpg_setDateTimeFormat did not return the expected string\n");
        return;
    }
    udatpg_close(dtpg);
}

static void TestBuilder() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg;
    UDateTimePatternConflict conflict;
    UEnumeration *en;
    UChar result[20];
    int32_t length, pLength;  
    const UChar *s, *p;
    const UChar* ptrResult[2]; 
    int32_t count=0;
    
    /* test create an empty DateTimePatternGenerator */
    dtpg=udatpg_openEmpty(&errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_openEmpty() failed - %s\n", u_errorName(errorCode));
        return;
    }
    
    /* Add a pattern */
    conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20, 
                                 &length, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_addPattern() failed - %s\n", u_errorName(errorCode));
        return;
    }
    /* Add a redundant pattern */
    conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20,
                                 &length, &errorCode);
    if(conflict == UDATPG_NO_CONFLICT) {
        log_err("udatpg_addPattern() failed to find the duplicate pattern.\n");
        return;
    }
    /* Test pattern == NULL */
    s=NULL;
    length = udatpg_addPattern(dtpg, s, 0, FALSE, result, 20,
                               &length, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_addPattern failed in illegal argument - pattern is NULL.\n");
        return;
    }

    /* replace field type */
    errorCode=U_ZERO_ERROR;
    conflict = udatpg_addPattern(dtpg, testPattern2, 7, FALSE, result, 20,
                                 &length, &errorCode);
    if((conflict != UDATPG_NO_CONFLICT)||U_FAILURE(errorCode)) {
        log_err("udatpg_addPattern() failed to add HH:mm v. - %s\n", u_errorName(errorCode));
        return;
    }
    length = udatpg_replaceFieldTypes(dtpg, testPattern2, 7, replacedStr, 4,
                                      result, 20, &errorCode);
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_replaceFieldTypes failed!\n");
        return;
    }
    
    /* Get all skeletons and the crroespong pattern for each skeleton. */
    ptrResult[0] = testPattern2;
    ptrResult[1] = redundantPattern; 
    count=0;
    en = udatpg_openSkeletons(dtpg, &errorCode);  
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_openSkeletons failed!\n");
        return;
    }
    while ( (s=uenum_unext(en, &length, &errorCode))!= NULL) {
        p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
        if (U_FAILURE(errorCode) || p==NULL || u_memcmp(p, ptrResult[count], pLength)!=0 ) {
            log_err("udatpg_getPatternForSkeleton failed!\n");
            return;
        }
        count++;
    }
    uenum_close(en);
    
    /* Get all baseSkeletons */
    en = udatpg_openBaseSkeletons(dtpg, &errorCode);
    count=0;
    while ( (s=uenum_unext(en, &length, &errorCode))!= NULL) {
        p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
        if (U_FAILURE(errorCode) || p==NULL || u_memcmp(p, resultBaseSkeletons[count], pLength)!=0 ) {
            log_err("udatpg_getPatternForSkeleton failed!\n");
            return;
        }
        count++;
    }
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_openSkeletons failed!\n");
        return;
    }
    uenum_close(en);
    
    udatpg_close(dtpg);
}

#endif
