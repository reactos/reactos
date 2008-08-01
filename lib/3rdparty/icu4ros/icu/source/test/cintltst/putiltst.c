/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1998-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
* File putiltst.c (Tests the API in putil)
*
* Modification History:
*
*   Date          Name        Description
*   07/12/2000    Madhu       Creation 
*******************************************************************************
*/

#include "unicode/utypes.h"
#include "cintltst.h"
#include "cmemory.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "putilimp.h"

static UBool compareWithNAN(double x, double y);
static void doAssert(double expect, double got, const char *message);

static void TestPUtilAPI(void){

    double  n1=0.0, y1=0.0, expn1, expy1;
    double  value1 = 0.021;
    UVersionInfo versionArray = {0x01, 0x00, 0x02, 0x02};
    char versionString[17]; /* xxx.xxx.xxx.xxx\0 */
    char *str=0;
    UBool isTrue=FALSE;

    log_verbose("Testing the API uprv_modf()\n");
    y1 = uprv_modf(value1, &n1);
    expn1=0;
    expy1=0.021;
    if(y1 != expy1   || n1 != expn1){
        log_err("Error in uprv_modf.  Expected IntegralValue=%f, Got=%f, \n Expected FractionalValue=%f, Got=%f\n",
             expn1, n1, expy1, y1);
    }
    if(VERBOSITY){
        log_verbose("[float]  x = %f  n = %f y = %f\n", value1, n1, y1);
    }
    log_verbose("Testing the API uprv_fmod()\n");
    expn1=uprv_fmod(30.50, 15.00);
    doAssert(expn1, 0.5, "uprv_fmod(30.50, 15.00) failed.");

    log_verbose("Testing the API uprv_ceil()\n");
    expn1=uprv_ceil(value1);
    doAssert(expn1, 1, "uprv_ceil(0.021) failed.");

    log_verbose("Testing the API uprv_floor()\n");
    expn1=uprv_floor(value1);
    doAssert(expn1, 0, "uprv_floor(0.021) failed.");

    log_verbose("Testing the API uprv_fabs()\n");
    expn1=uprv_fabs((2.02-1.345));
    doAssert(expn1, 0.675, "uprv_fabs(2.02-1.345) failed.");
    
    log_verbose("Testing the API uprv_fmax()\n");
    doAssert(uprv_fmax(2.4, 1.2), 2.4, "uprv_fmax(2.4, 1.2) failed.");

    log_verbose("Testing the API uprv_fmax() with x value= NaN\n");
    expn1=uprv_fmax(uprv_getNaN(), 1.2);
    doAssert(expn1, uprv_getNaN(), "uprv_fmax(uprv_getNaN(), 1.2) failed. when one parameter is NaN");

    log_verbose("Testing the API uprv_fmin()\n");
    doAssert(uprv_fmin(2.4, 1.2), 1.2, "uprv_fmin(2.4, 1.2) failed.");

    log_verbose("Testing the API uprv_fmin() with x value= NaN\n");
    expn1=uprv_fmin(uprv_getNaN(), 1.2);
    doAssert(expn1, uprv_getNaN(), "uprv_fmin(uprv_getNaN(), 1.2) failed. when one parameter is NaN");

    log_verbose("Testing the API uprv_max()\n");
    doAssert(uprv_max(4, 2), 4, "uprv_max(4, 2) failed.");

    log_verbose("Testing the API uprv_min()\n");
    doAssert(uprv_min(-4, 2), -4, "uprv_min(-4, 2) failed.");

    log_verbose("Testing the API uprv_trunc()\n");
    doAssert(uprv_trunc(12.3456), 12, "uprv_trunc(12.3456) failed.");
    doAssert(uprv_trunc(12.234E2), 1223, "uprv_trunc(12.234E2) failed.");
    doAssert(uprv_trunc(uprv_getNaN()), uprv_getNaN(), "uprv_trunc(uprv_getNaN()) failed. with parameter=NaN");
    doAssert(uprv_trunc(uprv_getInfinity()), uprv_getInfinity(), "uprv_trunc(uprv_getInfinity()) failed. with parameter=Infinity");


    log_verbose("Testing the API uprv_pow10()\n");
    doAssert(uprv_pow10(4), 10000, "uprv_pow10(4) failed.");

    log_verbose("Testing the API uprv_isNegativeInfinity()\n");
    isTrue=uprv_isNegativeInfinity(uprv_getInfinity() * -1);
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isNegativeInfinity failed.\n");
    }
    log_verbose("Testing the API uprv_isPositiveInfinity()\n");
    isTrue=uprv_isPositiveInfinity(uprv_getInfinity());
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isPositiveInfinity failed.\n");
    }
    log_verbose("Testing the API uprv_isInfinite()\n");
    isTrue=uprv_isInfinite(uprv_getInfinity());
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isInfinite failed.\n");
    }

#if 0
    log_verbose("Testing the API uprv_digitsAfterDecimal()....\n");
    doAssert(uprv_digitsAfterDecimal(value1), 3, "uprv_digitsAfterDecimal() failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E2), 2, "uprv_digitsAfterDecimal(1.2345E2) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E-2), 6, "uprv_digitsAfterDecimal(1.2345E-2) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E2), 2, "uprv_digitsAfterDecimal(1.2345E2) failed.");
    doAssert(uprv_digitsAfterDecimal(-1.2345E-20), 24, "uprv_digitsAfterDecimal(1.2345E-20) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E20), 0, "uprv_digitsAfterDecimal(1.2345E20) failed.");
    doAssert(uprv_digitsAfterDecimal(-0.021), 3, "uprv_digitsAfterDecimal(-0.021) failed.");
    doAssert(uprv_digitsAfterDecimal(23.0), 0, "uprv_digitsAfterDecimal(23.0) failed.");
    doAssert(uprv_digitsAfterDecimal(0.022223333321), 9, "uprv_digitsAfterDecimal(0.022223333321) failed.");
#endif


    log_verbose("Testing the API u_versionToString().....\n");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 1.0.2.2, Got=%s\n", versionString);
    }
    log_verbose("Testing the API u_versionToString().....with versionArray=NULL\n");
    u_versionToString(NULL, versionString);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed. with versionArray=NULL. It should just return\n");
    }
    log_verbose("Testing the API u_versionToString().....with versionArray=NULL\n");
    u_versionToString(NULL, versionString);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed . It should just return\n");
    }
    log_verbose("Testing the API u_versionToString().....with versionString=NULL\n");
    u_versionToString(versionArray, NULL);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed. with versionArray=NULL  It should just return\n");
    }
    versionArray[0] = 0x0a;
    log_verbose("Testing the API u_versionToString().....\n");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "10.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 10.0.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0xa0;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "160.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.0.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0xa0;
    versionArray[1] = 0xa0;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "160.160.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.160.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0x01;
    versionArray[1] = 0x0a;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.10.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.160.2.2, Got=%s\n", versionString);
    }

    log_verbose("Testing the API u_versionFromString() ....\n");
    u_versionFromString(versionArray, "1.3.5.6");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.3.5.6") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: 1.3.5.6, Got=%s\n",  versionString);
    }
    log_verbose("Testing the API u_versionFromString() where versionArray=NULL....\n");
    u_versionFromString(NULL, "1.3.5.6");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.3.5.6") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: 1.3.5.6, Got=%s\n",  versionString);
    }

    log_verbose("Testing the API u_getVersion().....\n");
    u_getVersion(versionArray);
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, U_ICU_VERSION) != 0){
        log_err("ERROR: u_getVersion() failed. Got=%s, expected %s\n",  versionString, U_ICU_VERSION);
    }
    log_verbose("Testing the API u_errorName()...\n");
    str=(char*)u_errorName((UErrorCode)0);
    if(strcmp(str, "U_ZERO_ERROR") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: U_ZERO_ERROR Got=%s\n",  str);
    }
    log_verbose("Testing the API u_errorName()...\n");
    str=(char*)u_errorName((UErrorCode)-127);
    if(strcmp(str, "U_USING_DEFAULT_WARNING") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: U_USING_DEFAULT_WARNING Got=%s\n",  str);
    }
    log_verbose("Testing the API u_errorName().. with BOGUS ERRORCODE...\n");
    str=(char*)u_errorName((UErrorCode)200);
    if(strcmp(str, "[BOGUS UErrorCode]") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: [BOGUS UErrorCode] Got=%s\n",  str);
    }

    {
        const char* dataDirectory;
        int32_t dataDirectoryLen;
        UChar *udataDir=0;
        UChar temp[100];
        char *charvalue=0;
        log_verbose("Testing chars to UChars\n");
        
         /* This cannot really work on a japanese system. u_uastrcpy will have different results than */
        /* u_charsToUChars when there is a backslash in the string! */
        /*dataDirectory=u_getDataDirectory();*/

        dataDirectory="directory1";  /*no backslashes*/
        dataDirectoryLen=(int32_t)strlen(dataDirectory);
        udataDir=(UChar*)malloc(sizeof(UChar) * (dataDirectoryLen + 1));
        u_charsToUChars(dataDirectory, udataDir, (dataDirectoryLen + 1));
        u_uastrcpy(temp, dataDirectory);
       
        if(u_strcmp(temp, udataDir) != 0){
            log_err("ERROR: u_charsToUChars failed. Expected %s, Got %s\n", austrdup(temp), austrdup(udataDir));
        }
        log_verbose("Testing UChars to chars\n");
        charvalue=(char*)malloc(sizeof(char) * (u_strlen(udataDir) + 1));

        u_UCharsToChars(udataDir, charvalue, (u_strlen(udataDir)+1));
        if(strcmp(charvalue, dataDirectory) != 0){
            log_err("ERROR: u_UCharsToChars failed. Expected %s, Got %s\n", charvalue, dataDirectory);
        }
        free(charvalue);
        free(udataDir);
    }
   
    log_verbose("Testing uprv_timezone()....\n");
    {
        int32_t tzoffset = uprv_timezone();
        log_verbose("Value returned from uprv_timezone = %d\n",  tzoffset);
        if (tzoffset != 28800) {
            log_verbose("***** WARNING: If testing in the PST timezone, t_timezone should return 28800! *****");
        }
        if ((tzoffset % 1800 != 0)) {
            log_err("FAIL: t_timezone may be incorrect. It is not a multiple of 30min.");
        }
        /*tzoffset=uprv_getUTCtime();*/

    }
}

#if 0
static void testIEEEremainder()
{
    double    pinf        = uprv_getInfinity();
    double    ninf        = -uprv_getInfinity();
    double    nan         = uprv_getNaN();
/*    double    pzero       = 0.0;*/
/*    double    nzero       = 0.0;
    nzero *= -1;*/

     /* simple remainder checks*/
    remainderTest(7.0, 2.5, -0.5);
    remainderTest(7.0, -2.5, -0.5);
     /* this should work
     remainderTest(43.7, 2.5, 1.2);
     */

    /* infinity and real*/
    remainderTest(1.0, pinf, 1.0);
    remainderTest(1.0, ninf, 1.0);

    /*test infinity and real*/
    remainderTest(nan, 1.0, nan);
    remainderTest(1.0, nan, nan);
    /*test infinity and nan*/
    remainderTest(ninf, nan, nan);
    remainderTest(pinf, nan, nan);

    /* test infinity and zero */
/*    remainderTest(pinf, pzero, 1.25);
    remainderTest(pinf, nzero, 1.25);
    remainderTest(ninf, pzero, 1.25);
    remainderTest(ninf, nzero, 1.25); */
}

static void remainderTest(double x, double y, double exp)
{
    double result = uprv_IEEEremainder(x,y);

    if(        uprv_isNaN(result) && 
        ! ( uprv_isNaN(x) || uprv_isNaN(y))) {
        log_err("FAIL: got NaN as result without NaN as argument");
        log_err("      IEEEremainder(%f, %f) is %f, expected %f\n", x, y, result, exp);
    }
    else if(!compareWithNAN(result, exp)) {
        log_err("FAIL:  IEEEremainder(%f, %f) is %f, expected %f\n", x, y, result, exp);
    } else{
        log_verbose("OK: IEEEremainder(%f, %f) is %f\n", x, y, result);
    }

}
#endif

static UBool compareWithNAN(double x, double y)
{
  if( uprv_isNaN(x) || uprv_isNaN(y) ) {
    if(!uprv_isNaN(x) || !uprv_isNaN(y) ) {
      return FALSE;
    }
  }
  else if (y != x) { /* no NaN's involved */
    return FALSE;
  }

  return TRUE;
}

static void doAssert(double got, double expect, const char *message)
{
  if(! compareWithNAN(expect, got) ) {
    log_err("ERROR :  %s. Expected : %lf, Got: %lf\n", message, expect, got);
  }
}


#define _CODE_ARR_LEN 8
static const UErrorCode errorCode[_CODE_ARR_LEN] = {
    U_USING_FALLBACK_WARNING,
    U_STRING_NOT_TERMINATED_WARNING,
    U_ILLEGAL_ARGUMENT_ERROR,
    U_STATE_TOO_OLD_ERROR,
    U_BAD_VARIABLE_DEFINITION,
    U_RULE_MASK_ERROR,
    U_UNEXPECTED_TOKEN,
    U_UNSUPPORTED_ATTRIBUTE
};

static const char* str[] = {
    "U_USING_FALLBACK_WARNING",
    "U_STRING_NOT_TERMINATED_WARNING",
    "U_ILLEGAL_ARGUMENT_ERROR",
    "U_STATE_TOO_OLD_ERROR",
    "U_BAD_VARIABLE_DEFINITION",
    "U_RULE_MASK_ERROR",
    "U_UNEXPECTED_TOKEN",
    "U_UNSUPPORTED_ATTRIBUTE"
};

static void TestErrorName(void){
    int32_t code=0;
    const char* errorName ;
    for(;code<U_ERROR_LIMIT;code++){
        errorName = u_errorName((UErrorCode)code);
    }

    for(code=0;code<_CODE_ARR_LEN; code++){
        errorName = u_errorName(errorCode[code]);
        if(uprv_strcmp(str[code],errorName )!=0){
            log_err("Error : u_errorName failed. Expected: %s Got: %s \n",str[code],errorName);
        }
    }
}

void addPUtilTest(TestNode** root);

void
addPUtilTest(TestNode** root)
{
    addTest(root, &TestPUtilAPI,       "putiltst/TestPUtilAPI");
/*    addTest(root, &testIEEEremainder,  "putiltst/testIEEEremainder"); */
    addTest(root, &TestErrorName, "putiltst/TestErrorName");
}

