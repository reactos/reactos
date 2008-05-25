/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _PARSEPOSITIONIONTEST_
#define _PARSEPOSITIONIONTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"


/** 
 * Performs test for ParsePosition
 **/
class ParsePositionTest: public IntlTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    void TestParsePosition(void);
    void TestFieldPosition(void);
    void TestFieldPosition_example(void);
    void Test4109023(void);

protected:
    UBool failure(UErrorCode status, const char* msg);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _PARSEPOSITIONIONTEST_
//eof
