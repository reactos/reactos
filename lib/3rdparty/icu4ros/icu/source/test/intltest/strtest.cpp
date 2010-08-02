/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  strtest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov22
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "intltest.h"
#include "strtest.h"
#include "unicode/ustring.h"

#if defined(U_WINDOWS) && defined(_MSC_VER)
#include <vector>
using namespace std;
#endif

StringTest::~StringTest() {}

void StringTest::TestEndian(void) {
    union {
        uint8_t byte;
        uint16_t word;
    } u;
    u.word=0x0100;
    if(U_IS_BIG_ENDIAN!=u.byte) {
        errln("TestEndian: U_IS_BIG_ENDIAN needs to be fixed in platform.h");
    }
}

void StringTest::TestSizeofTypes(void) {
    if(U_SIZEOF_WCHAR_T!=sizeof(wchar_t)) {
        errln("TestSizeofWCharT: U_SIZEOF_WCHAR_T!=sizeof(wchar_t) - U_SIZEOF_WCHAR_T needs to be fixed in platform.h");
    }
#ifdef U_INT64_T_UNAVAILABLE
    errln("int64_t and uint64_t are undefined.");
#else
    if(8!=sizeof(int64_t)) {
        errln("TestSizeofTypes: 8!=sizeof(int64_t) - int64_t needs to be fixed in platform.h");
    }
    if(8!=sizeof(uint64_t)) {
        errln("TestSizeofTypes: 8!=sizeof(uint64_t) - uint64_t needs to be fixed in platform.h");
    }
#endif
    if(8!=sizeof(double)) {
        errln("8!=sizeof(double) - putil.c code may not work");
    }
    if(4!=sizeof(int32_t)) {
        errln("4!=sizeof(int32_t)");
    }
    if(4!=sizeof(uint32_t)) {
        errln("4!=sizeof(uint32_t)");
    }
    if(2!=sizeof(int16_t)) {
        errln("2!=sizeof(int16_t)");
    }
    if(2!=sizeof(uint16_t)) {
        errln("2!=sizeof(uint16_t)");
    }
    if(2!=sizeof(UChar)) {
        errln("2!=sizeof(UChar)");
    }
    if(1!=sizeof(int8_t)) {
        errln("1!=sizeof(int8_t)");
    }
    if(1!=sizeof(uint8_t)) {
        errln("1!=sizeof(uint8_t)");
    }
    if(1!=sizeof(UBool)) {
        errln("1!=sizeof(UBool)");
    }
}

void StringTest::TestCharsetFamily(void) {
    unsigned char c='A';
    if( U_CHARSET_FAMILY==U_ASCII_FAMILY && c!=0x41 ||
        U_CHARSET_FAMILY==U_EBCDIC_FAMILY && c!=0xc1
    ) {
        errln("TestCharsetFamily: U_CHARSET_FAMILY needs to be fixed in platform.h");
    }
}

U_STRING_DECL(ustringVar, "aZ0 -", 5);

void StringTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite Character and String Test: ");
    }
    switch(index) {
    case 0:
        name="TestEndian";
        if(exec) {
            TestEndian();
        }
        break;
    case 1:
        name="TestSizeofTypes";
        if(exec) {
            TestSizeofTypes();
        }
        break;
    case 2:
        name="TestCharsetFamily";
        if(exec) {
            TestCharsetFamily();
        }
        break;
    case 3:
        name="Test_U_STRING";
        if(exec) {
            U_STRING_INIT(ustringVar, "aZ0 -", 5);
            if( sizeof(ustringVar)/sizeof(*ustringVar)!=6 ||
                ustringVar[0]!=0x61 ||
                ustringVar[1]!=0x5a ||
                ustringVar[2]!=0x30 ||
                ustringVar[3]!=0x20 ||
                ustringVar[4]!=0x2d ||
                ustringVar[5]!=0
            ) {
                errln("Test_U_STRING: U_STRING_DECL with U_STRING_INIT does not work right! "
                      "See putil.h and utypes.h with platform.h.");
            }
        }
        break;
    case 4:
        name="Test_UNICODE_STRING";
        if(exec) {
            UnicodeString ustringVar=UNICODE_STRING("aZ0 -", 5);
            if( ustringVar.length()!=5 ||
                ustringVar[0]!=0x61 ||
                ustringVar[1]!=0x5a ||
                ustringVar[2]!=0x30 ||
                ustringVar[3]!=0x20 ||
                ustringVar[4]!=0x2d
            ) {
                errln("Test_UNICODE_STRING: UNICODE_STRING does not work right! "
                      "See unistr.h and utypes.h with platform.h.");
            }
        }
        break;
    case 5:
        name="Test_UNICODE_STRING_SIMPLE";
        if(exec) {
            UnicodeString ustringVar=UNICODE_STRING_SIMPLE("aZ0 -");
            if( ustringVar.length()!=5 ||
                ustringVar[0]!=0x61 ||
                ustringVar[1]!=0x5a ||
                ustringVar[2]!=0x30 ||
                ustringVar[3]!=0x20 ||
                ustringVar[4]!=0x2d
            ) {
                errln("Test_UNICODE_STRING_SIMPLE: UNICODE_STRING_SIMPLE does not work right! "
                      "See unistr.h and utypes.h with platform.h.");
            }
        }
        break;
    case 6:
        name="Test_UTF8_COUNT_TRAIL_BYTES";
        if(exec) {
            if(UTF8_COUNT_TRAIL_BYTES(0x7F) != 0
                || UTF8_COUNT_TRAIL_BYTES(0xC0) != 1
                || UTF8_COUNT_TRAIL_BYTES(0xE0) != 2
                || UTF8_COUNT_TRAIL_BYTES(0xF0) != 3)
            {
                errln("Test_UTF8_COUNT_TRAIL_BYTES: UTF8_COUNT_TRAIL_BYTES does not work right! "
                      "See utf8.h.");
            }
        }
        break;
    case 7:
        name="TestSTLCompatibility";
        if(exec) {
#if defined(U_WINDOWS) && defined(_MSC_VER)
            /* Just make sure that it compiles with STL's placement new usage. */
            vector<UnicodeString> myvect;
            myvect.push_back(UnicodeString("blah"));
#endif
        }
        break;
    default:
        name="";
        break;
    }
}
