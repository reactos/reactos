/*
*******************************************************************************
*
*   Copyright (C) 2003-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  convtest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003jul15
*   created by: Markus W. Scherer
*
*   Test file for data-driven conversion tests.
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_LEGACY_CONVERSION
/*
 * Note: Turning off all of convtest.cpp if !UCONFIG_NO_LEGACY_CONVERSION
 * is slightly unnecessary - it removes tests for Unicode charsets
 * like UTF-8 that should work.
 * However, there is no easy way for the test to detect whether a test case
 * is for a Unicode charset, so it would be difficult to only exclude those.
 * Also, regular testing of ICU is done with all modules on, therefore
 * not testing conversion for a custom configuration like this should be ok.
 */

#include "unicode/ucnv.h"
#include "unicode/unistr.h"
#include "unicode/parsepos.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/ures.h"
#include "convtest.h"
#include "unicode/tstdtmod.h"
#include <string.h>
#include <stdlib.h>

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

enum {
    // characters used in test data for callbacks
    SUB_CB='?',
    SKIP_CB='0',
    STOP_CB='.',
    ESC_CB='&'
};

ConversionTest::ConversionTest() {
    UErrorCode errorCode=U_ZERO_ERROR;
    utf8Cnv=ucnv_open("UTF-8", &errorCode);
    ucnv_setToUCallBack(utf8Cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("unable to open UTF-8 converter");
    }
}

ConversionTest::~ConversionTest() {
    ucnv_close(utf8Cnv);
}

void
ConversionTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if (exec) logln("TestSuite ConversionTest: ");
    switch (index) {
        case 0: name="TestToUnicode"; if (exec) TestToUnicode(); break;
        case 1: name="TestFromUnicode"; if (exec) TestFromUnicode(); break;
        case 2: name="TestGetUnicodeSet"; if (exec) TestGetUnicodeSet(); break;
        default: name=""; break; //needed to end loop
    }
}

// test data interface ----------------------------------------------------- ***

void
ConversionTest::TestToUnicode() {
    ConversionCase cc;
    char charset[100], cbopt[4];
    const char *option;
    UnicodeString s, unicode;
    int32_t offsetsLength;
    UConverterToUCallback callback;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    UErrorCode errorCode;
    int32_t i;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("toUnicode", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/toUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                cc.caseNr=i;

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");
                cc.charset=charset;

                cc.bytes=testCase->getBinary(cc.bytesLength, "bytes", errorCode);
                unicode=testCase->getString("unicode", errorCode);
                cc.unicode=unicode.getBuffer();
                cc.unicodeLength=unicode.length();

                offsetsLength=0;
                cc.offsets=testCase->getIntVector(offsetsLength, "offsets", errorCode);
                if(offsetsLength==0) {
                    cc.offsets=NULL;
                } else if(offsetsLength!=unicode.length()) {
                    errln("toUnicode[%d] unicode[%d] and offsets[%d] must have the same length",
                            i, unicode.length(), offsetsLength);
                    errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                }

                cc.finalFlush= 0!=testCase->getInt28("flush", errorCode);
                cc.fallbacks= 0!=testCase->getInt28("fallbacks", errorCode);

                s=testCase->getString("errorCode", errorCode);
                if(s==UNICODE_STRING("invalid", 7)) {
                    cc.outErrorCode=U_INVALID_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illegal", 7)) {
                    cc.outErrorCode=U_ILLEGAL_CHAR_FOUND;
                } else if(s==UNICODE_STRING("truncated", 9)) {
                    cc.outErrorCode=U_TRUNCATED_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illesc", 6)) {
                    cc.outErrorCode=U_ILLEGAL_ESCAPE_SEQUENCE;
                } else if(s==UNICODE_STRING("unsuppesc", 9)) {
                    cc.outErrorCode=U_UNSUPPORTED_ESCAPE_SEQUENCE;
                } else {
                    cc.outErrorCode=U_ZERO_ERROR;
                }

                s=testCase->getString("callback", errorCode);
                s.extract(0, 0x7fffffff, cbopt, sizeof(cbopt), "");
                cc.cbopt=cbopt;
                switch(cbopt[0]) {
                case SUB_CB:
                    callback=UCNV_TO_U_CALLBACK_SUBSTITUTE;
                    break;
                case SKIP_CB:
                    callback=UCNV_TO_U_CALLBACK_SKIP;
                    break;
                case STOP_CB:
                    callback=UCNV_TO_U_CALLBACK_STOP;
                    break;
                case ESC_CB:
                    callback=UCNV_TO_U_CALLBACK_ESCAPE;
                    break;
                default:
                    callback=NULL;
                    break;
                }
                option=callback==NULL ? cbopt : cbopt+1;
                if(*option==0) {
                    option=NULL;
                }

                cc.invalidChars=testCase->getBinary(cc.invalidLength, "invalidChars", errorCode);

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/toUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                } else {
                    logln("TestToUnicode[%d] %s", i, charset);
                    ToUnicodeCase(cc, callback, option);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        errln("Failed: could not load test conversion data");
    }
}

void
ConversionTest::TestFromUnicode() {
    ConversionCase cc;
    char charset[100], cbopt[4];
    const char *option;
    UnicodeString s, unicode, invalidUChars;
    int32_t offsetsLength, index;
    UConverterFromUCallback callback;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    const UChar *p;
    UErrorCode errorCode;
    int32_t i, length;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("fromUnicode", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/fromUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                cc.caseNr=i;

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");
                cc.charset=charset;

                unicode=testCase->getString("unicode", errorCode);
                cc.unicode=unicode.getBuffer();
                cc.unicodeLength=unicode.length();
                cc.bytes=testCase->getBinary(cc.bytesLength, "bytes", errorCode);

                offsetsLength=0;
                cc.offsets=testCase->getIntVector(offsetsLength, "offsets", errorCode);
                if(offsetsLength==0) {
                    cc.offsets=NULL;
                } else if(offsetsLength!=cc.bytesLength) {
                    errln("fromUnicode[%d] bytes[%d] and offsets[%d] must have the same length",
                            i, cc.bytesLength, offsetsLength);
                    errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                }

                cc.finalFlush= 0!=testCase->getInt28("flush", errorCode);
                cc.fallbacks= 0!=testCase->getInt28("fallbacks", errorCode);

                s=testCase->getString("errorCode", errorCode);
                if(s==UNICODE_STRING("invalid", 7)) {
                    cc.outErrorCode=U_INVALID_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illegal", 7)) {
                    cc.outErrorCode=U_ILLEGAL_CHAR_FOUND;
                } else if(s==UNICODE_STRING("truncated", 9)) {
                    cc.outErrorCode=U_TRUNCATED_CHAR_FOUND;
                } else {
                    cc.outErrorCode=U_ZERO_ERROR;
                }

                s=testCase->getString("callback", errorCode);
                cc.setSub=0; // default: no subchar

                if((index=s.indexOf((UChar)0))>0) {
                    // read NUL-separated subchar first, if any
                    // copy the subchar from Latin-1 characters
                    // start after the NUL
                    p=s.getTerminatedBuffer();
                    length=index+1;
                    p+=length;
                    length=s.length()-length;
                    if(length<=0 || length>=(int32_t)sizeof(cc.subchar)) {
                        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        int32_t j;

                        for(j=0; j<length; ++j) {
                            cc.subchar[j]=(char)p[j];
                        }
                        // NUL-terminate the subchar
                        cc.subchar[j]=0;
                        cc.setSub=1;
                    }

                    // remove the NUL and subchar from s
                    s.truncate(index);
                } else if((index=s.indexOf((UChar)0x3d))>0) /* '=' */ {
                    // read a substitution string, separated by an equal sign
                    p=s.getBuffer()+index+1;
                    length=s.length()-(index+1);
                    if(length<0 || length>=LENGTHOF(cc.subString)) {
                        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        u_memcpy(cc.subString, p, length);
                        // NUL-terminate the subString
                        cc.subString[length]=0;
                        cc.setSub=-1;
                    }

                    // remove the equal sign and subString from s
                    s.truncate(index);
                }

                s.extract(0, 0x7fffffff, cbopt, sizeof(cbopt), "");
                cc.cbopt=cbopt;
                switch(cbopt[0]) {
                case SUB_CB:
                    callback=UCNV_FROM_U_CALLBACK_SUBSTITUTE;
                    break;
                case SKIP_CB:
                    callback=UCNV_FROM_U_CALLBACK_SKIP;
                    break;
                case STOP_CB:
                    callback=UCNV_FROM_U_CALLBACK_STOP;
                    break;
                case ESC_CB:
                    callback=UCNV_FROM_U_CALLBACK_ESCAPE;
                    break;
                default:
                    callback=NULL;
                    break;
                }
                option=callback==NULL ? cbopt : cbopt+1;
                if(*option==0) {
                    option=NULL;
                }

                invalidUChars=testCase->getString("invalidUChars", errorCode);
                cc.invalidUChars=invalidUChars.getBuffer();
                cc.invalidLength=invalidUChars.length();

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/fromUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                } else {
                    logln("TestFromUnicode[%d] %s", i, charset);
                    FromUnicodeCase(cc, callback, option);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        errln("Failed: could not load test conversion data");
    }
}

static const UChar ellipsis[]={ 0x2e, 0x2e, 0x2e };

void
ConversionTest::TestGetUnicodeSet() {
    char charset[100];
    UnicodeString s, map, mapnot;
    int32_t which;

    ParsePosition pos;
    UnicodeSet cnvSet, mapSet, mapnotSet, diffSet;
    UnicodeSet *cnvSetPtr = &cnvSet;
    UConverter *cnv;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    UErrorCode errorCode;
    int32_t i;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("getUnicodeSet", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/getUnicodeSet test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");

                map=testCase->getString("map", errorCode);
                mapnot=testCase->getString("mapnot", errorCode);

                which=testCase->getInt28("which", errorCode);

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/getUnicodeSet test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                // test this test case
                mapSet.clear();
                mapnotSet.clear();

                pos.setIndex(0);
                mapSet.applyPattern(map, pos, 0, NULL, errorCode);
                if(U_FAILURE(errorCode) || pos.getIndex()!=map.length()) {
                    errln("error creating the map set for conversion/getUnicodeSet test case %d - %s\n"
                          "    error index %d  index %d  U+%04x",
                            i, u_errorName(errorCode), pos.getErrorIndex(), pos.getIndex(), map.char32At(pos.getIndex()));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                pos.setIndex(0);
                mapnotSet.applyPattern(mapnot, pos, 0, NULL, errorCode);
                if(U_FAILURE(errorCode) || pos.getIndex()!=mapnot.length()) {
                    errln("error creating the mapnot set for conversion/getUnicodeSet test case %d - %s\n"
                          "    error index %d  index %d  U+%04x",
                            i, u_errorName(errorCode), pos.getErrorIndex(), pos.getIndex(), mapnot.char32At(pos.getIndex()));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                logln("TestGetUnicodeSet[%d] %s", i, charset);

                cnv=cnv_open(charset, errorCode);
                if(U_FAILURE(errorCode)) {
                    errln("error opening \"%s\" for conversion/getUnicodeSet test case %d - %s",
                            charset, i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                ucnv_getUnicodeSet(cnv, (USet *)cnvSetPtr, (UConverterUnicodeSet)which, &errorCode);
                ucnv_close(cnv);

                if(U_FAILURE(errorCode)) {
                    errln("error in ucnv_getUnicodeSet(\"%s\") for conversion/getUnicodeSet test case %d - %s",
                            charset, i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                // are there items that must be in cnvSet but are not?
                (diffSet=mapSet).removeAll(cnvSet);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(s, TRUE);
                    if(s.length()>100) {
                        s.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") is missing items - conversion/getUnicodeSet test case %d",
                            charset, i);
                    errln(s);
                }

                // are there items that must not be in cnvSet but are?
                (diffSet=mapnotSet).retainAll(cnvSet);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(s, TRUE);
                    if(s.length()>100) {
                        s.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") contains unexpected items - conversion/getUnicodeSet test case %d",
                            charset, i);
                    errln(s);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        errln("Failed: could not load test conversion data");
    }
}

// open testdata or ICU data converter ------------------------------------- ***

UConverter *
ConversionTest::cnv_open(const char *name, UErrorCode &errorCode) {
    if(name!=NULL && *name=='*') {
        /* loadTestData(): set the data directory */
        return ucnv_openPackage(loadTestData(errorCode), name+1, &errorCode);
    } else {
        return ucnv_open(name, &errorCode);
    }
}

// output helpers ---------------------------------------------------------- ***

static inline char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}

static char *
printBytes(const uint8_t *bytes, int32_t length, char *out) {
    uint8_t b;

    if(length>0) {
        b=*bytes++;
        --length;
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
    }

    while(length>0) {
        b=*bytes++;
        --length;
        *out++=' ';
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
    }
    *out++=0;
    return out;
}

static char *
printUnicode(const UChar *unicode, int32_t length, char *out) {
    UChar32 c;
    int32_t i;

    for(i=0; i<length;) {
        if(i>0) {
            *out++=' ';
        }
        U16_NEXT(unicode, i, length, c);
        // write 4..6 digits
        if(c>=0x100000) {
            *out++='1';
        }
        if(c>=0x10000) {
            *out++=hexDigit((uint8_t)((c>>16)&0xf));
        }
        *out++=hexDigit((uint8_t)((c>>12)&0xf));
        *out++=hexDigit((uint8_t)((c>>8)&0xf));
        *out++=hexDigit((uint8_t)((c>>4)&0xf));
        *out++=hexDigit((uint8_t)(c&0xf));
    }
    *out++=0;
    return out;
}

static char *
printOffsets(const int32_t *offsets, int32_t length, char *out) {
    int32_t i, o, d;

    if(offsets==NULL) {
        length=0;
    }

    for(i=0; i<length; ++i) {
        if(i>0) {
            *out++=' ';
        }
        o=offsets[i];

        // print all offsets with 2 characters each (-x, -9..99, xx)
        if(o<-9) {
            *out++='-';
            *out++='x';
        } else if(o<0) {
            *out++='-';
            *out++=(char)('0'-o);
        } else if(o<=99) {
            *out++=(d=o/10)==0 ? ' ' : (char)('0'+d);
            *out++=(char)('0'+o%10);
        } else /* o>99 */ {
            *out++='x';
            *out++='x';
        }
    }
    *out++=0;
    return out;
}

// toUnicode test worker functions ----------------------------------------- ***

static int32_t
stepToUnicode(ConversionCase &cc, UConverter *cnv,
              UChar *result, int32_t resultCapacity,
              int32_t *resultOffsets, /* also resultCapacity */
              int32_t step,
              UErrorCode *pErrorCode) {
    const char *source, *sourceLimit, *bytesLimit;
    UChar *target, *targetLimit, *resultLimit;
    UBool flush;

    source=(const char *)cc.bytes;
    target=result;
    bytesLimit=source+cc.bytesLength;
    resultLimit=result+resultCapacity;

    if(step>=0) {
        // call ucnv_toUnicode() with in/out buffers no larger than (step) at a time
        // move only one buffer (in vs. out) at a time to be extra mean
        // step==0 performs bulk conversion and generates offsets

        // initialize the partial limits for the loop
        if(step==0) {
            // use the entire buffers
            sourceLimit=bytesLimit;
            targetLimit=resultLimit;
            flush=cc.finalFlush;
        } else {
            // start with empty partial buffers
            sourceLimit=source;
            targetLimit=target;
            flush=FALSE;

            // output offsets only for bulk conversion
            resultOffsets=NULL;
        }

        for(;;) {
            // resetting the opposite conversion direction must not affect this one
            ucnv_resetFromUnicode(cnv);

            // convert
            ucnv_toUnicode(cnv,
                &target, targetLimit,
                &source, sourceLimit,
                resultOffsets,
                flush, pErrorCode);

            // check pointers and errors
            if(source>sourceLimit || target>targetLimit) {
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                if(target!=targetLimit) {
                    // buffer overflow must only be set when the target is filled
                    *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    break;
                } else if(targetLimit==resultLimit) {
                    // not just a partial overflow
                    break;
                }

                // the partial target is filled, set a new limit, reset the error and continue
                targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
                *pErrorCode=U_ZERO_ERROR;
            } else if(U_FAILURE(*pErrorCode)) {
                // some other error occurred, done
                break;
            } else {
                if(source!=sourceLimit) {
                    // when no error occurs, then the input must be consumed
                    *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    break;
                }

                if(sourceLimit==bytesLimit) {
                    // we are done
                    break;
                }

                // the partial conversion succeeded, set a new limit and continue
                sourceLimit=(bytesLimit-source)>=step ? source+step : bytesLimit;
                flush=(UBool)(cc.finalFlush && sourceLimit==bytesLimit);
            }
        }
    } else /* step<0 */ {
        /*
         * step==-1: call only ucnv_getNextUChar()
         * otherwise alternate between ucnv_toUnicode() and ucnv_getNextUChar()
         *   if step==-2 or -3, then give ucnv_toUnicode() the whole remaining input,
         *   else give it at most (-step-2)/2 bytes
         */
        UChar32 c;

        // end the loop by getting an index out of bounds error
        for(;;) {
            // resetting the opposite conversion direction must not affect this one
            ucnv_resetFromUnicode(cnv);

            // convert
            if((step&1)!=0 /* odd: -1, -3, -5, ... */) {
                sourceLimit=source; // use sourceLimit not as a real limit
                                    // but to remember the pre-getNextUChar source pointer
                c=ucnv_getNextUChar(cnv, &source, bytesLimit, pErrorCode);

                // check pointers and errors
                if(*pErrorCode==U_INDEX_OUTOFBOUNDS_ERROR) {
                    if(source!=bytesLimit) {
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    } else {
                        *pErrorCode=U_ZERO_ERROR;
                    }
                    break;
                } else if(U_FAILURE(*pErrorCode)) {
                    break;
                }
                // source may not move if c is from previous overflow

                if(target==resultLimit) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
                if(c<=0xffff) {
                    *target++=(UChar)c;
                } else {
                    *target++=U16_LEAD(c);
                    if(target==resultLimit) {
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        break;
                    }
                    *target++=U16_TRAIL(c);
                }

                // alternate between -n-1 and -n but leave -1 alone
                if(step<-1) {
                    ++step;
                }
            } else /* step is even */ {
                // allow only one UChar output
                targetLimit=target<resultLimit ? target+1 : resultLimit;

                // as with ucnv_getNextUChar(), we always flush (if we go to bytesLimit)
                // and never output offsets
                if(step==-2) {
                    sourceLimit=bytesLimit;
                } else {
                    sourceLimit=source+(-step-2)/2;
                    if(sourceLimit>bytesLimit) {
                        sourceLimit=bytesLimit;
                    }
                }

                ucnv_toUnicode(cnv,
                    &target, targetLimit,
                    &source, sourceLimit,
                    NULL, (UBool)(sourceLimit==bytesLimit), pErrorCode);

                // check pointers and errors
                if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                    if(target!=targetLimit) {
                        // buffer overflow must only be set when the target is filled
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                        break;
                    } else if(targetLimit==resultLimit) {
                        // not just a partial overflow
                        break;
                    }

                    // the partial target is filled, set a new limit and continue
                    *pErrorCode=U_ZERO_ERROR;
                } else if(U_FAILURE(*pErrorCode)) {
                    // some other error occurred, done
                    break;
                } else {
                    if(source!=sourceLimit) {
                        // when no error occurs, then the input must be consumed
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                        break;
                    }

                    // we are done (flush==TRUE) but we continue, to get the index out of bounds error above
                }

                --step;
            }
        }
    }

    return (int32_t)(target-result);
}

UBool
ConversionTest::ToUnicodeCase(ConversionCase &cc, UConverterToUCallback callback, const char *option) {
    UConverter *cnv;
    UErrorCode errorCode;

    // open the converter
    errorCode=U_ZERO_ERROR;
    cnv=cnv_open(cc.charset, errorCode);
    if(U_FAILURE(errorCode)) {
        errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_open() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
        return FALSE;
    }

    // set the callback
    if(callback!=NULL) {
        ucnv_setToUCallBack(cnv, callback, option, NULL, NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setToUCallBack() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    }

    int32_t resultOffsets[256];
    UChar result[256];
    int32_t resultLength;
    UBool ok;

    static const struct {
        int32_t step;
        const char *name;
    } steps[]={
        { 0, "bulk" }, // must be first for offsets to be checked
        { 1, "step=1" },
        { 3, "step=3" },
        { 7, "step=7" },
        { -1, "getNext" },
        { -2, "toU(bulk)+getNext" },
        { -3, "getNext+toU(bulk)" },
        { -4, "toU(1)+getNext" },
        { -5, "getNext+toU(1)" },
        { -12, "toU(5)+getNext" },
        { -13, "getNext+toU(5)" },
    };
    int32_t i, step;

    ok=TRUE;
    for(i=0; i<LENGTHOF(steps) && ok; ++i) {
        step=steps[i].step;
        if(step<0 && !cc.finalFlush) {
            // skip ucnv_getNextUChar() if !finalFlush because
            // ucnv_getNextUChar() always implies flush
            continue;
        }
        if(step!=0) {
            // bulk test is first, then offsets are not checked any more
            cc.offsets=NULL;
        }
        else {
            memset(resultOffsets, -1, LENGTHOF(resultOffsets));
        }
        memset(result, -1, LENGTHOF(result));
        errorCode=U_ZERO_ERROR;
        resultLength=stepToUnicode(cc, cnv,
                                result, LENGTHOF(result),
                                step==0 ? resultOffsets : NULL,
                                step, &errorCode);
        ok=checkToUnicode(
                cc, cnv, steps[i].name,
                result, resultLength,
                cc.offsets!=NULL ? resultOffsets : NULL,
                errorCode);
        if(U_FAILURE(errorCode) || !cc.finalFlush) {
            // reset if an error occurred or we did not flush
            // otherwise do nothing to make sure that flushing resets
            ucnv_resetToUnicode(cnv);
        }
        if (resultOffsets[resultLength] != -1) {
            errln("toUnicode[%d](%s) Conversion wrote too much to offsets at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
        if (result[resultLength] != (UChar)-1) {
            errln("toUnicode[%d](%s) Conversion wrote too much to result at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
    }

    // not a real loop, just a convenience for breaking out of the block
    while(ok && cc.finalFlush) {
        // test ucnv_toUChars()
        memset(result, 0, sizeof(result));

        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_toUChars(cnv,
                        result, LENGTHOF(result),
                        (const char *)cc.bytes, cc.bytesLength,
                        &errorCode);
        ok=checkToUnicode(
                cc, cnv, "toUChars",
                result, resultLength,
                NULL,
                errorCode);
        if(!ok) {
            break;
        }

        // test preflighting
        // keep the correct result for simple checking
        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_toUChars(cnv,
                        NULL, 0,
                        (const char *)cc.bytes, cc.bytesLength,
                        &errorCode);
        if(errorCode==U_STRING_NOT_TERMINATED_WARNING || errorCode==U_BUFFER_OVERFLOW_ERROR) {
            errorCode=U_ZERO_ERROR;
        }
        ok=checkToUnicode(
                cc, cnv, "preflight toUChars",
                result, resultLength,
                NULL,
                errorCode);
        break;
    }

    ucnv_close(cnv);
    return ok;
}

UBool
ConversionTest::checkToUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                               const UChar *result, int32_t resultLength,
                               const int32_t *resultOffsets,
                               UErrorCode resultErrorCode) {
    char resultInvalidChars[8];
    int8_t resultInvalidLength;
    UErrorCode errorCode;

    const char *msg;

    // reset the message; NULL will mean "ok"
    msg=NULL;

    errorCode=U_ZERO_ERROR;
    resultInvalidLength=sizeof(resultInvalidChars);
    ucnv_getInvalidChars(cnv, resultInvalidChars, &resultInvalidLength, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) ucnv_getInvalidChars() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, u_errorName(errorCode));
        return FALSE;
    }

    // check everything that might have gone wrong
    if(cc.unicodeLength!=resultLength) {
        msg="wrong result length";
    } else if(0!=u_memcmp(cc.unicode, result, cc.unicodeLength)) {
        msg="wrong result string";
    } else if(cc.offsets!=NULL && 0!=memcmp(cc.offsets, resultOffsets, cc.unicodeLength*sizeof(*cc.offsets))) {
        msg="wrong offsets";
    } else if(cc.outErrorCode!=resultErrorCode) {
        msg="wrong error code";
    } else if(cc.invalidLength!=resultInvalidLength) {
        msg="wrong length of last invalid input";
    } else if(0!=memcmp(cc.invalidChars, resultInvalidChars, cc.invalidLength)) {
        msg="wrong last invalid input";
    }

    if(msg==NULL) {
        return TRUE;
    } else {
        char buffer[2000]; // one buffer for all strings
        char *s, *bytesString, *unicodeString, *resultString,
            *offsetsString, *resultOffsetsString,
            *invalidCharsString, *resultInvalidCharsString;

        bytesString=s=buffer;
        s=printBytes(cc.bytes, cc.bytesLength, bytesString);
        s=printUnicode(cc.unicode, cc.unicodeLength, unicodeString=s);
        s=printUnicode(result, resultLength, resultString=s);
        s=printOffsets(cc.offsets, cc.unicodeLength, offsetsString=s);
        s=printOffsets(resultOffsets, resultLength, resultOffsetsString=s);
        s=printBytes(cc.invalidChars, cc.invalidLength, invalidCharsString=s);
        s=printBytes((uint8_t *)resultInvalidChars, resultInvalidLength, resultInvalidCharsString=s);

        if((s-buffer)>(int32_t)sizeof(buffer)) {
            errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) fatal error: checkToUnicode() test output buffer overflow writing %d chars\n",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, (int)(s-buffer));
            exit(1);
        }

        errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) failed: %s\n"
              "  bytes <%s>[%d]\n"
              " expected <%s>[%d]\n"
              "  result  <%s>[%d]\n"
              " offsets         <%s>\n"
              "  result offsets <%s>\n"
              " error code expected %s got %s\n"
              "  invalidChars expected <%s> got <%s>\n",
              cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, msg,
              bytesString, cc.bytesLength,
              unicodeString, cc.unicodeLength,
              resultString, resultLength,
              offsetsString,
              resultOffsetsString,
              u_errorName(cc.outErrorCode), u_errorName(resultErrorCode),
              invalidCharsString, resultInvalidCharsString);

        return FALSE;
    }
}

// fromUnicode test worker functions --------------------------------------- ***

static int32_t
stepFromUTF8(ConversionCase &cc,
             UConverter *utf8Cnv, UConverter *cnv,
             char *result, int32_t resultCapacity,
             int32_t step,
             UErrorCode *pErrorCode) {
    const char *source, *sourceLimit, *utf8Limit;
    UChar pivotBuffer[32];
    UChar *pivotSource, *pivotTarget, *pivotLimit;
    char *target, *targetLimit, *resultLimit;
    UBool flush;

    source=cc.utf8;
    pivotSource=pivotTarget=pivotBuffer;
    target=result;
    utf8Limit=source+cc.utf8Length;
    resultLimit=result+resultCapacity;

    // call ucnv_convertEx() with in/out buffers no larger than (step) at a time
    // move only one buffer (in vs. out) at a time to be extra mean
    // step==0 performs bulk conversion

    // initialize the partial limits for the loop
    if(step==0) {
        // use the entire buffers
        sourceLimit=utf8Limit;
        targetLimit=resultLimit;
        flush=cc.finalFlush;

        pivotLimit=pivotBuffer+LENGTHOF(pivotBuffer);
    } else {
        // start with empty partial buffers
        sourceLimit=source;
        targetLimit=target;
        flush=FALSE;

        // empty pivot is not allowed, make it of length step
        pivotLimit=pivotBuffer+step;
    }

    for(;;) {
        // resetting the opposite conversion direction must not affect this one
        ucnv_resetFromUnicode(utf8Cnv);
        ucnv_resetToUnicode(cnv);

        // convert
        ucnv_convertEx(cnv, utf8Cnv,
            &target, targetLimit,
            &source, sourceLimit,
            pivotBuffer, &pivotSource, &pivotTarget, pivotLimit,
            FALSE, flush, pErrorCode);

        // check pointers and errors
        if(source>sourceLimit || target>targetLimit) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            break;
        } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            if(target!=targetLimit) {
                // buffer overflow must only be set when the target is filled
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(targetLimit==resultLimit) {
                // not just a partial overflow
                break;
            }

            // the partial target is filled, set a new limit, reset the error and continue
            targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
            *pErrorCode=U_ZERO_ERROR;
        } else if(U_FAILURE(*pErrorCode)) {
            if(pivotSource==pivotBuffer) {
                // toUnicode error, should not occur
                // toUnicode errors are tested in cintltst TestConvertExFromUTF8()
                break;
            } else {
                // fromUnicode error
                // some other error occurred, done
                break;
            }
        } else {
            if(source!=sourceLimit) {
                // when no error occurs, then the input must be consumed
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            }

            if(sourceLimit==utf8Limit) {
                // we are done
                if(*pErrorCode==U_STRING_NOT_TERMINATED_WARNING) {
                    // ucnv_convertEx() warns about not terminating the output
                    // but ucnv_fromUnicode() does not and so
                    // checkFromUnicode() does not expect it
                    *pErrorCode=U_ZERO_ERROR;
                }
                break;
            }

            // the partial conversion succeeded, set a new limit and continue
            sourceLimit=(utf8Limit-source)>=step ? source+step : utf8Limit;
            flush=(UBool)(cc.finalFlush && sourceLimit==utf8Limit);
        }
    }

    return (int32_t)(target-result);
}

static int32_t
stepFromUnicode(ConversionCase &cc, UConverter *cnv,
                char *result, int32_t resultCapacity,
                int32_t *resultOffsets, /* also resultCapacity */
                int32_t step,
                UErrorCode *pErrorCode) {
    const UChar *source, *sourceLimit, *unicodeLimit;
    char *target, *targetLimit, *resultLimit;
    UBool flush;

    source=cc.unicode;
    target=result;
    unicodeLimit=source+cc.unicodeLength;
    resultLimit=result+resultCapacity;

    // call ucnv_fromUnicode() with in/out buffers no larger than (step) at a time
    // move only one buffer (in vs. out) at a time to be extra mean
    // step==0 performs bulk conversion and generates offsets

    // initialize the partial limits for the loop
    if(step==0) {
        // use the entire buffers
        sourceLimit=unicodeLimit;
        targetLimit=resultLimit;
        flush=cc.finalFlush;
    } else {
        // start with empty partial buffers
        sourceLimit=source;
        targetLimit=target;
        flush=FALSE;

        // output offsets only for bulk conversion
        resultOffsets=NULL;
    }

    for(;;) {
        // resetting the opposite conversion direction must not affect this one
        ucnv_resetToUnicode(cnv);

        // convert
        ucnv_fromUnicode(cnv,
            &target, targetLimit,
            &source, sourceLimit,
            resultOffsets,
            flush, pErrorCode);

        // check pointers and errors
        if(source>sourceLimit || target>targetLimit) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            break;
        } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            if(target!=targetLimit) {
                // buffer overflow must only be set when the target is filled
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(targetLimit==resultLimit) {
                // not just a partial overflow
                break;
            }

            // the partial target is filled, set a new limit, reset the error and continue
            targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
            *pErrorCode=U_ZERO_ERROR;
        } else if(U_FAILURE(*pErrorCode)) {
            // some other error occurred, done
            break;
        } else {
            if(source!=sourceLimit) {
                // when no error occurs, then the input must be consumed
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            }

            if(sourceLimit==unicodeLimit) {
                // we are done
                break;
            }

            // the partial conversion succeeded, set a new limit and continue
            sourceLimit=(unicodeLimit-source)>=step ? source+step : unicodeLimit;
            flush=(UBool)(cc.finalFlush && sourceLimit==unicodeLimit);
        }
    }

    return (int32_t)(target-result);
}

UBool
ConversionTest::FromUnicodeCase(ConversionCase &cc, UConverterFromUCallback callback, const char *option) {
    UConverter *cnv;
    UErrorCode errorCode;

    // open the converter
    errorCode=U_ZERO_ERROR;
    cnv=cnv_open(cc.charset, errorCode);
    if(U_FAILURE(errorCode)) {
        errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_open() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
        return FALSE;
    }
    ucnv_resetToUnicode(utf8Cnv);

    // set the callback
    if(callback!=NULL) {
        ucnv_setFromUCallBack(cnv, callback, option, NULL, NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setFromUCallBack() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    }

    // set the fallbacks flag
    // TODO change with Jitterbug 2401, then add a similar call for toUnicode too
    ucnv_setFallback(cnv, cc.fallbacks);

    // set the subchar
    int32_t length;

    if(cc.setSub>0) {
        length=(int32_t)strlen(cc.subchar);
        ucnv_setSubstChars(cnv, cc.subchar, (int8_t)length, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setSubstChars() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    } else if(cc.setSub<0) {
        ucnv_setSubstString(cnv, cc.subString, -1, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setSubstString() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    }

    // convert unicode to utf8
    char utf8[256];
    cc.utf8=utf8;
    u_strToUTF8(utf8, LENGTHOF(utf8), &cc.utf8Length,
                cc.unicode, cc.unicodeLength,
                &errorCode);
    if(U_FAILURE(errorCode)) {
        // skip UTF-8 testing of a string with an unpaired surrogate,
        // or of one that's too long
        // toUnicode errors are tested in cintltst TestConvertExFromUTF8()
        cc.utf8Length=-1;
    }

    int32_t resultOffsets[256];
    char result[256];
    int32_t resultLength;
    UBool ok;

    static const struct {
        int32_t step;
        const char *name, *utf8Name;
    } steps[]={
        { 0, "bulk",   "utf8" }, // must be first for offsets to be checked
        { 1, "step=1", "utf8 step=1" },
        { 3, "step=3", "utf8 step=3" },
        { 7, "step=7", "utf8 step=7" }
    };
    int32_t i, step;

    ok=TRUE;
    for(i=0; i<LENGTHOF(steps) && ok; ++i) {
        step=steps[i].step;
        memset(resultOffsets, -1, LENGTHOF(resultOffsets));
        memset(result, -1, LENGTHOF(result));
        errorCode=U_ZERO_ERROR;
        resultLength=stepFromUnicode(cc, cnv,
                                result, LENGTHOF(result),
                                step==0 ? resultOffsets : NULL,
                                step, &errorCode);
        ok=checkFromUnicode(
                cc, cnv, steps[i].name,
                (uint8_t *)result, resultLength,
                cc.offsets!=NULL ? resultOffsets : NULL,
                errorCode);
        if(U_FAILURE(errorCode) || !cc.finalFlush) {
            // reset if an error occurred or we did not flush
            // otherwise do nothing to make sure that flushing resets
            ucnv_resetFromUnicode(cnv);
        }
        if (resultOffsets[resultLength] != -1) {
            errln("fromUnicode[%d](%s) Conversion wrote too much to offsets at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
        if (result[resultLength] != (char)-1) {
            errln("fromUnicode[%d](%s) Conversion wrote too much to result at index %d",
                cc.caseNr, cc.charset, resultLength);
        }

        // bulk test is first, then offsets are not checked any more
        cc.offsets=NULL;

        // test direct conversion from UTF-8
        if(cc.utf8Length>=0) {
            errorCode=U_ZERO_ERROR;
            resultLength=stepFromUTF8(cc, utf8Cnv, cnv,
                                    result, LENGTHOF(result),
                                    step, &errorCode);
            ok=checkFromUnicode(
                    cc, cnv, steps[i].utf8Name,
                    (uint8_t *)result, resultLength,
                    NULL,
                    errorCode);
            if(U_FAILURE(errorCode) || !cc.finalFlush) {
                // reset if an error occurred or we did not flush
                // otherwise do nothing to make sure that flushing resets
                ucnv_resetToUnicode(utf8Cnv);
                ucnv_resetFromUnicode(cnv);
            }
        }
    }

    // not a real loop, just a convenience for breaking out of the block
    while(ok && cc.finalFlush) {
        // test ucnv_fromUChars()
        memset(result, 0, sizeof(result));

        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_fromUChars(cnv,
                        result, LENGTHOF(result),
                        cc.unicode, cc.unicodeLength,
                        &errorCode);
        ok=checkFromUnicode(
                cc, cnv, "fromUChars",
                (uint8_t *)result, resultLength,
                NULL,
                errorCode);
        if(!ok) {
            break;
        }

        // test preflighting
        // keep the correct result for simple checking
        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_fromUChars(cnv,
                        NULL, 0,
                        cc.unicode, cc.unicodeLength,
                        &errorCode);
        if(errorCode==U_STRING_NOT_TERMINATED_WARNING || errorCode==U_BUFFER_OVERFLOW_ERROR) {
            errorCode=U_ZERO_ERROR;
        }
        ok=checkFromUnicode(
                cc, cnv, "preflight fromUChars",
                (uint8_t *)result, resultLength,
                NULL,
                errorCode);
        break;
    }

    ucnv_close(cnv);
    return ok;
}

UBool
ConversionTest::checkFromUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                                 const uint8_t *result, int32_t resultLength,
                                 const int32_t *resultOffsets,
                                 UErrorCode resultErrorCode) {
    UChar resultInvalidUChars[8];
    int8_t resultInvalidLength;
    UErrorCode errorCode;

    const char *msg;

    // reset the message; NULL will mean "ok"
    msg=NULL;

    errorCode=U_ZERO_ERROR;
    resultInvalidLength=LENGTHOF(resultInvalidUChars);
    ucnv_getInvalidUChars(cnv, resultInvalidUChars, &resultInvalidLength, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) ucnv_getInvalidUChars() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, u_errorName(errorCode));
        return FALSE;
    }

    // check everything that might have gone wrong
    if(cc.bytesLength!=resultLength) {
        msg="wrong result length";
    } else if(0!=memcmp(cc.bytes, result, cc.bytesLength)) {
        msg="wrong result string";
    } else if(cc.offsets!=NULL && 0!=memcmp(cc.offsets, resultOffsets, cc.bytesLength*sizeof(*cc.offsets))) {
        msg="wrong offsets";
    } else if(cc.outErrorCode!=resultErrorCode) {
        msg="wrong error code";
    } else if(cc.invalidLength!=resultInvalidLength) {
        msg="wrong length of last invalid input";
    } else if(0!=u_memcmp(cc.invalidUChars, resultInvalidUChars, cc.invalidLength)) {
        msg="wrong last invalid input";
    }

    if(msg==NULL) {
        return TRUE;
    } else {
        char buffer[2000]; // one buffer for all strings
        char *s, *unicodeString, *bytesString, *resultString,
            *offsetsString, *resultOffsetsString,
            *invalidCharsString, *resultInvalidUCharsString;

        unicodeString=s=buffer;
        s=printUnicode(cc.unicode, cc.unicodeLength, unicodeString);
        s=printBytes(cc.bytes, cc.bytesLength, bytesString=s);
        s=printBytes(result, resultLength, resultString=s);
        s=printOffsets(cc.offsets, cc.bytesLength, offsetsString=s);
        s=printOffsets(resultOffsets, resultLength, resultOffsetsString=s);
        s=printUnicode(cc.invalidUChars, cc.invalidLength, invalidCharsString=s);
        s=printUnicode(resultInvalidUChars, resultInvalidLength, resultInvalidUCharsString=s);

        if((s-buffer)>(int32_t)sizeof(buffer)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) fatal error: checkFromUnicode() test output buffer overflow writing %d chars\n",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, (int)(s-buffer));
            exit(1);
        }

        errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) failed: %s\n"
              "  unicode <%s>[%d]\n"
              " expected <%s>[%d]\n"
              "  result  <%s>[%d]\n"
              " offsets         <%s>\n"
              "  result offsets <%s>\n"
              " error code expected %s got %s\n"
              "  invalidChars expected <%s> got <%s>\n",
              cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, msg,
              unicodeString, cc.unicodeLength,
              bytesString, cc.bytesLength,
              resultString, resultLength,
              offsetsString,
              resultOffsetsString,
              u_errorName(cc.outErrorCode), u_errorName(resultErrorCode),
              invalidCharsString, resultInvalidUCharsString);

        return FALSE;
    }
}

#endif /* #if !UCONFIG_NO_LEGACY_CONVERSION */
