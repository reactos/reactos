/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  strtest.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov22
*   created by: Markus W. Scherer
*/

/*
 * Test character- and string- related settings in utypes.h,
 * macros in putil.h, and constructors in unistr.h .
 */

#ifndef __STRTEST_H__
#define __STRTEST_H__

#include "intltest.h"

class StringTest : public IntlTest {
public:
    StringTest() {}
    virtual ~StringTest();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

private:
    void TestEndian(void);
    void TestSizeofTypes(void);
    void TestCharsetFamily(void);
};

#endif
