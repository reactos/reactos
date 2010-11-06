/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


/**
 * IntlTestUtilities is the medium level test class for everything in the directory "utility".
 */

#include "unicode/utypes.h"
#include "itutil.h"
#include "strtest.h"
#include "loctest.h"
#include "citrtest.h"
#include "ustrtest.h"
#include "ucdtest.h"
#include "restest.h"
#include "restsnew.h"
#include "tsmthred.h"
#include "tsputil.h"
#include "uobjtest.h"
#include "utxttest.h"
#include "v32test.h"
#include "uvectest.h" 
#include "aliastst.h"
#include "usettest.h"
//#include "custrtest.h"
//#include "ccitrtst.h"
//#include "cloctest.h"
//#include "ctres.h"
//#include "ctucd.h"

#define CASE(id, test) case id:                               \
                          name = #test;                       \
                          if (exec) {                         \
                              logln(#test "---"); logln();    \
                              test t;                         \
                              callTest(t, par);               \
                          }                                   \
                          break

void IntlTestUtilities::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    if (exec) logln("TestSuite Utilities: ");
    switch (index) {
        CASE(0, MultithreadTest); 
        CASE(1, StringTest); 
        CASE(2, UnicodeStringTest); 
        CASE(3, LocaleTest); 
        CASE(4, CharIterTest); 
        CASE(5, UnicodeTest); 
        CASE(6, ResourceBundleTest); 
        CASE(7, NewResourceBundleTest); 
        CASE(8, PUtilTest); 
        CASE(9, UObjectTest); 
        CASE(10, UVector32Test); 
        CASE(11, UVectorTest); 
        CASE(12, UTextTest); 
        CASE(13, MultithreadTest); 
        CASE(14, UnicodeSetTest); 
        default: name = ""; break; //needed to end loop
    }
}

