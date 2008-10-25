/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
 
#ifndef __CalendarLimitTest__
#define __CalendarLimitTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "caltztst.h"

/**
 * This test verifies the behavior of Calendar around the very earliest limits
 * which it can handle.  It also verifies the behavior for large values of millis.
 *
 * Bug ID 4033662.
 */
class CalendarLimitTest: public CalendarTimeZoneTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public: // package
    //test routine used by TestCalendarLimit
    virtual void test(UDate millis, Calendar *cal, DateFormat *fmt);

    // bug 986c: deprecate nextDouble/previousDouble
    //static double nextDouble(double a);
    //static double previousDouble(double a);
    static UBool withinErr(double a, double b, double err);

public:
    // test behaviour and error reporting at boundaries of defined range
    virtual void TestCalendarLimit(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // __CalendarLimitTest__
