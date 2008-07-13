/*
*******************************************************************************
*
*   Copyright (C) 2002-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*/

#include <stdio.h>
#include <unicode/brkiter.h>
#include <stdlib.h>

U_CFUNC int c_main(void);


void printUnicodeString(const UnicodeString &s) {
    char charBuf[1000];
    s.extract(0, s.length(), charBuf, sizeof(charBuf)-1, 0);   
    charBuf[sizeof(charBuf)-1] = 0;          
    printf("%s", charBuf);
}


void printTextRange( BreakIterator& iterator, 
                    int32_t start, int32_t end )
{
    CharacterIterator *strIter = iterator.getText().clone();
    UnicodeString  s;
    strIter->getText(s);

    printf(" %ld %ld\t", (long)start, (long)end);
    printUnicodeString(UnicodeString(s, 0, start));
    printf("|");
    printUnicodeString(UnicodeString(s, start, end-start));
    printf("|");
    printUnicodeString(UnicodeString(s, end));
    puts("");
    delete strIter;
}


/* Print each element in order: */
void printEachForward( BreakIterator& boundary)
{
    int32_t start = boundary.first();
    for (int32_t end = boundary.next();
         end != BreakIterator::DONE;
         start = end, end = boundary.next())
    {
        printTextRange( boundary, start, end );
    }
}

/* Print each element in reverse order: */
void printEachBackward( BreakIterator& boundary)
{
    int32_t end = boundary.last();
    for (int32_t start = boundary.previous();
         start != BreakIterator::DONE;
         end = start, start = boundary.previous())
    {
        printTextRange( boundary, start, end );
    }
}

/* Print the first element */
void printFirst(BreakIterator& boundary)
{
    int32_t start = boundary.first();
    int32_t end = boundary.next();
    printTextRange( boundary, start, end );
}

/* Print the last element */
void printLast(BreakIterator& boundary)
{
    int32_t end = boundary.last();
    int32_t start = boundary.previous();
    printTextRange( boundary, start, end );
}

/* Print the element at a specified position */
void printAt(BreakIterator &boundary, int32_t pos )
{
    int32_t end = boundary.following(pos);
    int32_t start = boundary.previous();
    printTextRange( boundary, start, end );
}

/* Creating and using text boundaries */
int main( void )
{
    puts("ICU Break Iterator Sample Program\n");
    puts("C++ Break Iteration\n");
    BreakIterator* boundary;
    UnicodeString stringToExamine("Aaa bbb ccc. Ddd eee fff.");
    printf("Examining: ");
    printUnicodeString(stringToExamine);
    puts("");

    //print each sentence in forward and reverse order
    UErrorCode status = U_ZERO_ERROR;
    boundary = BreakIterator::createSentenceInstance(
        Locale::getUS(), status );
    if (U_FAILURE(status)) {
        printf("failed to create sentence break iterator.  status = %s", 
            u_errorName(status));
        exit(1);
    }

    boundary->setText(stringToExamine);
    puts("\n Sentence Boundaries... ");
    puts("----- forward: -----------");
    printEachForward(*boundary);
    puts("----- backward: ----------");
    printEachBackward(*boundary);
    delete boundary;

    //print each word in order
    printf("\n Word Boundaries... \n");
    boundary = BreakIterator::createWordInstance(
        Locale::getUS(), status);
    boundary->setText(stringToExamine);
    puts("----- forward: -----------");
    printEachForward(*boundary);
    //print first element
    puts("----- first: -------------");
    printFirst(*boundary);
    //print last element
    puts("----- last: --------------");
    printLast(*boundary);
    //print word at charpos 10
    puts("----- at pos 10: ---------");
    printAt(*boundary, 10 );

    delete boundary;

    puts("\nEnd C++ Break Iteration");

    // Call the C version
    return c_main();
}
