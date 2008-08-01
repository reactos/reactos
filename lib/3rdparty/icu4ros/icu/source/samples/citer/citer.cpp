/*
*******************************************************************************
*
*     Copyright (C) 2002-2007, International Business Machines
*     Corporation and others.    All Rights Reserved.
*
*******************************************************************************
*/

#include "unicode/uchriter.h"
#include "unicode/schriter.h"
#include "unicode/ustring.h"
#include <stdio.h>
#include <unicode/brkiter.h>
#include <unicode/ustdio.h>
#include <stdlib.h>

static UFILE *out;

void printUnicodeString(const UnicodeString &s)
{
    u_fprintf(out, "%S", s);
}

void printUChar(UChar32 ch)
{
    if(ch < 127) {
        u_fprintf(out, "%C", (UChar) ch);
    } else if (ch == CharacterIterator::DONE) {
        u_fprintf(out, "[CharacterIterator::DONE = 0xFFFF]");
    } else {
        u_fprintf(out, "[%X]", ch);
    }
}

class Test
{
public:
    void TestUChariter();
    void TestStringiter();
};

void Test::TestUChariter() {
    const char testChars[] = "Now is the time for all good men to come "
        "to the aid of their country.";

    UnicodeString testString(testChars,"");
    const UChar *testText = testString.getTerminatedBuffer();

    UCharCharacterIterator iter(testText, u_strlen(testText));
    UCharCharacterIterator* test2 = (UCharCharacterIterator*)iter.clone();

    u_fprintf(out, "testText = %s", testChars);

    if (iter != *test2 ) {
        u_fprintf(out, "clone() or equals() failed: Two clones tested unequal\n");
    }

    UnicodeString result1, result2;
    // getting and comparing the text within the iterators
    iter.getText(result1);
    test2->getText(result2);
    if (result1 != result2) {
        u_fprintf(out, "iter.getText() != clone.getText()\n");
    } 

    u_fprintf(out, "\n");

    // Demonstrates seeking forward using the iterator.
    u_fprintf(out, "Forward  = ");

    UChar c = iter.first();
    printUChar(c);    // The first char
    int32_t i = 0;

    if (iter.startIndex() != 0 || iter.endIndex() != u_strlen(testText)) {
        u_fprintf(out, "startIndex() or endIndex() failed\n");
    }


    // Testing forward iteration...
    do {
        if (c == CharacterIterator::DONE && i != u_strlen(testText)) {
            u_fprintf(out, "Iterator reached end prematurely");
        }
        else if (c != testText[i]) {
            u_fprintf(out, "Character mismatch at position %d\n" + i);
        }
        if (iter.current() != c) {
            u_fprintf(out, "current() isn't working right");
        }
        if (iter.getIndex() != i) {
            u_fprintf(out, "getIndex() isn't working right\n");
        }
        if (c != CharacterIterator::DONE) {
            c = iter.next();
            i++;
        }

        u_fprintf(out, "|");
        printUChar(c);

    } while (c != CharacterIterator::DONE);        

    delete test2;
    u_fprintf(out, "\n");
}


void Test::TestStringiter() {
    const char testChars[] = "Now is the time for all good men to come "
        "to the aid of their country.";

    UnicodeString testString(testChars,"");
    const UChar *testText    = testString.getTerminatedBuffer();

    StringCharacterIterator iter(testText, u_strlen(testText));
    StringCharacterIterator* test2 = (StringCharacterIterator*)iter.clone();

    if (iter != *test2 ) {
        u_fprintf(out, "clone() or equals() failed: Two clones tested unequal\n");
    }

    UnicodeString result1, result2;
    // getting and comparing the text within the iterators
    iter.getText(result1);
    test2->getText(result2);
    if (result1 != result2) {
        u_fprintf(out, "getText() failed\n");
    }

    u_fprintf(out, "Backwards: ");

    UChar c = iter.last();
    int32_t i = iter.endIndex();

    printUChar(c);
    i--; // already printed out the last char 

    if (iter.startIndex() != 0 || iter.endIndex() != u_strlen(testText)) {
        u_fprintf(out, "startIndex() or endIndex() failed\n");
    }

    // Testing backward iteration over a range...
    do {
        if (c == CharacterIterator::DONE) {
            u_fprintf(out, "Iterator reached end prematurely\n");
        }
        else if (c != testText[i]) {
            u_fprintf(out, "Character mismatch at position %d\n", i);
        }
        if (iter.current() != c) {
            u_fprintf(out, "current() isn't working right\n");
        }
        if (iter.getIndex() != i) {
            u_fprintf(out, "getIndex() isn't working right [%d should be %d]\n", iter.getIndex(), i);
        }
        if (c != CharacterIterator::DONE) {
            c = iter.previous();
            i--;
        }

        u_fprintf(out, "|");
        printUChar(c);
    } while (c != CharacterIterator::DONE);

    u_fprintf(out, "\n");
    delete test2;
}

/* Creating and using text boundaries */
int main( void )
{
    UErrorCode status = U_ZERO_ERROR;

    out = u_finit(stdout, NULL, NULL);

    u_fprintf(out, "ICU Iteration Sample Program (C++)\n\n");

    Test t;

    u_fprintf(out, "\n");
    u_fprintf(out, "Test::TestUCharIter()\n");

    t.TestUChariter();

    u_fprintf(out, "-----\n");
    u_fprintf(out, "Test::TestStringchariter()\n");

    t.TestStringiter();

    u_fprintf(out, "-----\n");

    return 0;
}
