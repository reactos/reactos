/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef REGEXTST_H
#define REGEXTST_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "intltest.h"


class RegexTest: public IntlTest {
public:
  
    RegexTest();
    virtual ~RegexTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    // The following are test functions that are visible from the intltest test framework.
    virtual void API_Match();
    virtual void API_Pattern();
    virtual void API_Replace();
    virtual void Basic();
    virtual void Extended();
    virtual void Errors();
    virtual void PerlTests();

    // The following functions are internal to the regexp tests.
    virtual UBool doRegexLMTest(const char *pat, const char *text, UBool looking, UBool match, int32_t line);
    virtual void regex_find(const UnicodeString &pat, const UnicodeString &flags,
        const UnicodeString &input, int32_t line);
    virtual void regex_err(const char *pat, int32_t errline, int32_t errcol,
                            UErrorCode expectedStatus, int32_t line);
    virtual UChar *ReadAndConvertFile(const char *fileName, int32_t &len, UErrorCode &status);
    virtual const char *getPath(char buffer[2048], const char *filename);

};

#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS
#endif
