/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "uparse.h"
#include "ucdtest.h"

#define LENGTHOF(array) (sizeof(array)/sizeof(array[0]))

UnicodeTest::UnicodeTest()
{
}

UnicodeTest::~UnicodeTest()
{
}

void UnicodeTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite UnicodeTest: ");
    switch (index) {
        case 0: name = "TestAdditionalProperties"; if(exec) TestAdditionalProperties(); break;  
        default: name = ""; break; //needed to end loop
    }
}

//====================================================
// private data used by the tests
//====================================================

// test DerivedCoreProperties.txt -------------------------------------------

// copied from genprops.c
static int32_t
getTokenIndex(const char *const tokens[], int32_t countTokens, const char *s) {
    const char *t, *z;
    int32_t i, j;

    s=u_skipWhitespace(s);
    for(i=0; i<countTokens; ++i) {
        t=tokens[i];
        if(t!=NULL) {
            for(j=0;; ++j) {
                if(t[j]!=0) {
                    if(s[j]!=t[j]) {
                        break;
                    }
                } else {
                    z=u_skipWhitespace(s+j);
                    if(*z==';' || *z==0) {
                        return i;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    return -1;
}

static const char *const
derivedCorePropsNames[]={
    "Math",
    "Alphabetic",
    "Lowercase",
    "Uppercase",
    "ID_Start",
    "ID_Continue",
    "XID_Start",
    "XID_Continue",
    "Default_Ignorable_Code_Point",
    "Grapheme_Extend",
    "Grapheme_Link", /* Unicode 5 moves this property here from PropList.txt */
    "Grapheme_Base"
};

static const UProperty
derivedCorePropsIndex[]={
    UCHAR_MATH,
    UCHAR_ALPHABETIC,
    UCHAR_LOWERCASE,
    UCHAR_UPPERCASE,
    UCHAR_ID_START,
    UCHAR_ID_CONTINUE,
    UCHAR_XID_START,
    UCHAR_XID_CONTINUE,
    UCHAR_DEFAULT_IGNORABLE_CODE_POINT,
    UCHAR_GRAPHEME_EXTEND,
    UCHAR_GRAPHEME_LINK,
    UCHAR_GRAPHEME_BASE
};

U_CFUNC void U_CALLCONV
derivedCorePropsLineFn(void *context,
                        char *fields[][2], int32_t /* fieldCount */,
                        UErrorCode *pErrorCode)
{
    UnicodeTest *me=(UnicodeTest *)context;
    uint32_t start, end;
    int32_t i;

    u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        me->errln("UnicodeTest: syntax error in DerivedCoreProperties.txt field 0 at %s\n", fields[0][0]);
        return;
    }

    /* parse derived binary property name, ignore unknown names */
    i=getTokenIndex(derivedCorePropsNames, LENGTHOF(derivedCorePropsNames), fields[1][0]);
    if(i<0) {
        me->errln("UnicodeTest warning: unknown property name '%s' in \n", fields[1][0]);
        return;
    }

    me->derivedCoreProps[i].add(start, end);
}

void UnicodeTest::TestAdditionalProperties() {
    // test DerivedCoreProperties.txt
    if(LENGTHOF(derivedCoreProps)<LENGTHOF(derivedCorePropsNames)) {
        errln("error: UnicodeTest::derivedCoreProps[] too short, need at least %d UnicodeSets\n",
              LENGTHOF(derivedCorePropsNames));
        return;
    }
    if(LENGTHOF(derivedCorePropsIndex)!=LENGTHOF(derivedCorePropsNames)) {
        errln("error in ucdtest.cpp: LENGTHOF(derivedCorePropsIndex)!=LENGTHOF(derivedCorePropsNames)\n");
        return;
    }

    char newPath[256];
    char backupPath[256];
    char *fields[2][2];
    UErrorCode errorCode=U_ZERO_ERROR;

    /* Look inside ICU_DATA first */
    strcpy(newPath, pathToDataDirectory());
    strcat(newPath, "unidata" U_FILE_SEP_STRING "DerivedCoreProperties.txt");

    // As a fallback, try to guess where the source data was located
    // at the time ICU was built, and look there.
#   ifdef U_TOPSRCDIR
        strcpy(backupPath, U_TOPSRCDIR  U_FILE_SEP_STRING "data");
#   else
        strcpy(backupPath, loadTestData(errorCode));
        strcat(backupPath, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data");
#   endif
    strcat(backupPath, U_FILE_SEP_STRING);
    strcat(backupPath, "unidata" U_FILE_SEP_STRING "DerivedCoreProperties.txt");

    u_parseDelimitedFile(newPath, ';', fields, 2, derivedCorePropsLineFn, this, &errorCode);

    if(errorCode==U_FILE_ACCESS_ERROR) {
        errorCode=U_ZERO_ERROR;
        u_parseDelimitedFile(backupPath, ';', fields, 2, derivedCorePropsLineFn, this, &errorCode);
    }
    if(U_FAILURE(errorCode)) {
        errln("error parsing DerivedCoreProperties.txt: %s\n", u_errorName(errorCode));
        return;
    }

    // now we have all derived core properties in the UnicodeSets
    // run them all through the API
    int32_t rangeCount, range;
    uint32_t i;
    UChar32 start, end;
    int32_t noErrors = 0;

    // test all TRUE properties
    for(i=0; i<LENGTHOF(derivedCorePropsNames); ++i) {
        rangeCount=derivedCoreProps[i].getRangeCount();
        for(range=0; range<rangeCount; ++range) {
            start=derivedCoreProps[i].getRangeStart(range);
            end=derivedCoreProps[i].getRangeEnd(range);
            for(; start<=end; ++start) {
                if(!u_hasBinaryProperty(start, derivedCorePropsIndex[i])) {
                    errln("UnicodeTest error: u_hasBinaryProperty(U+%04lx, %s)==FALSE is wrong\n", start, derivedCorePropsNames[i]);
                    if(noErrors++ > 100) {
                      errln("Too many errors, moving to the next test");
                      break;
                    }
                }
            }
        }
    }

    noErrors = 0;
    // invert all properties
    for(i=0; i<LENGTHOF(derivedCorePropsNames); ++i) {
        derivedCoreProps[i].complement();
    }

    // test all FALSE properties
    for(i=0; i<LENGTHOF(derivedCorePropsNames); ++i) {
        rangeCount=derivedCoreProps[i].getRangeCount();
        for(range=0; range<rangeCount; ++range) {
            start=derivedCoreProps[i].getRangeStart(range);
            end=derivedCoreProps[i].getRangeEnd(range);
            for(; start<=end; ++start) {
                if(u_hasBinaryProperty(start, derivedCorePropsIndex[i])) {
                    errln("UnicodeTest error: u_hasBinaryProperty(U+%04lx, %s)==TRUE is wrong\n", start, derivedCorePropsNames[i]);
                    if(noErrors++ > 100) {
                      errln("Too many errors, moving to the next test");
                      break;
                    }
                }
            }
        }
    }
}
