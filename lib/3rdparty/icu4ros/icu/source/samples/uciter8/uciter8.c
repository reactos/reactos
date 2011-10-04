/*
*******************************************************************************
*
*   Copyright (C) 2003-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uciter8.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003jan10
*   created by: Markus W. Scherer
*
*   This file contains sample code that illustrates reading
*   8-bit Unicode text leniently, accepting a mix of UTF-8 and CESU-8
*   and also accepting single surrogates.
*/

#include <stdio.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/uiter.h"
#include "uit_len8.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

#define log_err printf

/* UCharIterator test ------------------------------------------------------- */

/*
 * The following code is a copy of the UCharIterator test code in
 * source/test/cintltst/custrtst.c,
 * testing the lenient-8 iterator instead of the UTF-8 one.
 */

/*
 * Compare results from two iterators, should be same.
 * Assume that the text is not empty and that
 * iteration start==0 and iteration limit==length.
 */
static void
compareIterators(UCharIterator *iter1, const char *n1,
                 UCharIterator *iter2, const char *n2) {
    int32_t i, pos1, pos2, middle, length;
    UChar32 c1, c2;

    /* compare lengths */
    length=iter1->getIndex(iter1, UITER_LENGTH);
    pos2=iter2->getIndex(iter2, UITER_LENGTH);
    if(length!=pos2) {
        log_err("%s->getIndex(length)=%d != %d=%s->getIndex(length)\n", n1, length, pos2, n2);
        return;
    }

    /* set into the middle */
    middle=length/2;

    pos1=iter1->move(iter1, middle, UITER_ZERO);
    if(pos1!=middle) {
        log_err("%s->move(from 0 to middle %d)=%d does not move to the middle\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, middle, UITER_ZERO);
    if(pos2!=middle) {
        log_err("%s->move(from 0 to middle %d)=%d does not move to the middle\n", n2, middle, pos2);
        return;
    }

    /* test current() */
    c1=iter1->current(iter1);
    c2=iter2->current(iter2);
    if(c1!=c2) {
        log_err("%s->current()=U+%04x != U+%04x=%s->current() at middle=%d\n", n1, c1, c2, n2, middle);
        return;
    }

    /* move forward 3 UChars */
    for(i=0; i<3; ++i) {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* move backward 5 UChars */
    for(i=0; i<5; ++i) {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* iterate forward from the beginning */
    pos1=iter1->move(iter1, 0, UITER_START);
    if(pos1<0) {
        log_err("%s->move(start) failed\n", n1);
        return;
    }
    if(!iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n1);
        return;
    }

    pos2=iter2->move(iter2, 0, UITER_START);
    if(pos2<0) {
        log_err("%s->move(start) failed\n", n2);
        return;
    }
    if(!iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n1);
        return;
    }
    if(iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n2);
        return;
    }

    /* back to the middle */
    pos1=iter1->move(iter1, middle, UITER_ZERO);
    if(pos1!=middle) {
        log_err("%s->move(from end to middle %d)=%d does not move to the middle\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, middle, UITER_ZERO);
    if(pos2!=middle) {
        log_err("%s->move(from end to middle %d)=%d does not move to the middle\n", n2, middle, pos2);
        return;
    }

    /* move to index 1 */
    pos1=iter1->move(iter1, 1, UITER_ZERO);
    if(pos1!=1) {
        log_err("%s->move(from middle %d to 1)=%d does not move to 1\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, 1, UITER_ZERO);
    if(pos2!=1) {
        log_err("%s->move(from middle %d to 1)=%d does not move to 1\n", n2, middle, pos2);
        return;
    }

    /* iterate backward from the end */
    pos1=iter1->move(iter1, 0, UITER_LIMIT);
    if(pos1<0) {
        log_err("%s->move(limit) failed\n", n1);
        return;
    }
    if(!iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n1);
        return;
    }

    pos2=iter2->move(iter2, 0, UITER_LIMIT);
    if(pos2<0) {
        log_err("%s->move(limit) failed\n", n2);
        return;
    }
    if(!iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n1);
        return;
    }
    if(iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n2);
        return;
    }
}

/*
 * Test the iterator's getState() and setState() functions.
 * iter1 and iter2 must be set up for the same iterator type and the same string
 * but may be physically different structs (different addresses).
 *
 * Assume that the text is not empty and that
 * iteration start==0 and iteration limit==length.
 * It must be 2<=middle<=length-2.
 */
static void
testIteratorState(UCharIterator *iter1, UCharIterator *iter2, const char *n, int32_t middle) {
    UChar32 u[4];

    UErrorCode errorCode;
    UChar32 c;
    uint32_t state;
    int32_t i, j;

    /* get four UChars from the middle of the string */
    iter1->move(iter1, middle-2, UITER_ZERO);
    for(i=0; i<4; ++i) {
        c=iter1->next(iter1);
        if(c<0) {
            /* the test violates the assumptions, see comment above */
            log_err("test error: %s[%d]=%d\n", n, middle-2+i, c);
            return;
        }
        u[i]=c;
    }

    /* move to the middle and get the state */
    iter1->move(iter1, -2, UITER_CURRENT);
    state=uiter_getState(iter1);

    /* set the state into the second iterator and compare the results */
    errorCode=U_ZERO_ERROR;
    uiter_setState(iter2, state, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("%s->setState(0x%x) failed: %s\n", n, state, u_errorName(errorCode));
        return;
    }

    c=iter2->current(iter2);
    if(c!=u[2]) {
        log_err("%s->current(at %d)=U+%04x!=U+%04x\n", n, middle, c, u[2]);
    }

    c=iter2->previous(iter2);
    if(c!=u[1]) {
        log_err("%s->previous(at %d)=U+%04x!=U+%04x\n", n, middle-1, c, u[1]);
    }

    iter2->move(iter2, 2, UITER_CURRENT);
    c=iter2->next(iter2);
    if(c!=u[3]) {
        log_err("%s->next(at %d)=U+%04x!=U+%04x\n", n, middle+1, c, u[3]);
    }

    iter2->move(iter2, -3, UITER_CURRENT);
    c=iter2->previous(iter2);
    if(c!=u[0]) {
        log_err("%s->previous(at %d)=U+%04x!=U+%04x\n", n, middle-2, c, u[0]);
    }

    /* move the second iterator back to the middle */
    iter2->move(iter2, 1, UITER_CURRENT);
    iter2->next(iter2);

    /* check that both are in the middle */
    i=iter1->getIndex(iter1, UITER_CURRENT);
    j=iter2->getIndex(iter2, UITER_CURRENT);
    if(i!=middle) {
        log_err("%s->getIndex(current)=%d!=%d as expected\n", n, i, middle);
    }
    if(i!=j) {
        log_err("%s->getIndex(current)=%d!=%d after setState()\n", n, j, i);
    }

    /* compare lengths */
    i=iter1->getIndex(iter1, UITER_LENGTH);
    j=iter2->getIndex(iter2, UITER_LENGTH);
    if(i!=j) {
        log_err("%s->getIndex(length)=%d!=%d before/after setState()\n", n, i, j);
    }
}

static void
TestLenient8Iterator() {
    static const UChar text[]={
        0x61, 0x62, 0x63,
        /* dffd 107fd             d801    dffd - in UTF-16, U+107fd=<d801 dffd> */
        0xdffd, 0xd801, 0xdffd, 0xd801, 0xdffd, 
        0x78, 0x79, 0x7a, 0
    };
    static const uint8_t bytes[]={
        0x61, 0x62, 0x63,
        /* dffd            107fd                    d801               dffd - mixture */
        0xed, 0xbf, 0xbd,  0xf0, 0x90, 0x9f, 0xbd,  0xed, 0xa0, 0x81,  0xed, 0xbf, 0xbd,
        0x78, 0x79, 0x7a, 0
    };

    UCharIterator iter1, iter2;
    UChar32 c1, c2;
    int32_t length;

    puts("test a UCharIterator for lenient 8-bit Unicode (accept single surrogates)");

    /* compare the same string between UTF-16 and lenient-8 UCharIterators */
    uiter_setString(&iter1, text, -1);
    uiter_setLenient8(&iter2, (const char *)bytes, sizeof(bytes)-1);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "Lenient8Iterator");

    /* try again with length=-1 */
    uiter_setLenient8(&iter2, (const char *)bytes, -1);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "Lenient8Iterator_1");

    /* test get/set state */
    length=LENGTHOF(text)-1;
    uiter_setLenient8(&iter1, (const char*)bytes, -1);
    testIteratorState(&iter1, &iter2, "Lenient8IteratorState", length/2);
    testIteratorState(&iter1, &iter2, "Lenient8IteratorStatePlus1", length/2+1);

    /* ---------------------------------------------------------------------- */

    puts("no output so far means that the lenient-8 iterator works fine");

    puts("iterate forward:\nUTF-16\tlenient-8");
    uiter_setString(&iter1, text, -1);
    iter1.move(&iter1, 0, UITER_START);
    iter2.move(&iter2, 0, UITER_START);
    for(;;) {
        c1=iter1.next(&iter1);
        c2=iter2.next(&iter2);
        if(c1<0 && c2<0) {
            break;
        }
        if(c1<0) {
            printf("\t%04x\n", c2);
        } else if(c2<0) {
            printf("%04x\n", c1);
        } else {
            printf("%04x\t%04x\n", c1, c2);
        }
    }
}

extern int
main(int argc, const char *argv[]) {
    TestLenient8Iterator();
    return 0;
}
