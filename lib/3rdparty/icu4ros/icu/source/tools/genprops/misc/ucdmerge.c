/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucdmerge.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003feb20
*   created by: Markus W. Scherer
*
*   Simple tool for Unicode Character Database files with semicolon-delimited fields.
*   Merges adjacent, identical per-code point data lines into one line with range syntax.
*
*   To compile, just call a C compiler/linker with this source file.
*   On Windows: cl ucdmerge.c
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *
skipWhitespace(const char *s) {
    while(*s==' ' || *s=='\t') {
        ++s;
    }
    return s;
}

/* return the first character position after the end of the data */
static char *
endOfData(const char *l) {
    char *end;
    char c;

    end=strchr(l, '#');
    if(end!=NULL) {
        /* ignore whitespace before the comment */
        while(l!=end && ((c=*(end-1))==' ' || c=='\t')) {
            --end;
        }
    } else {
        end=strchr(l, 0);
    }
    return end;
}

static int
sameData(const char *l1, const char *l2) {
    char *end1, *end2;
    int length;

    /* find the first semicolon in each line - there must be one */
    l1=strchr(l1, ';')+1;
    l2=strchr(l2, ';')+1;

    /* find the end of data: end of string or start of comment */
    end1=endOfData(l1);
    end2=endOfData(l2);

    /* compare the line data portions */
    length=end1-l1;
    return length==(end2-l2) && 0==memcmp(l1, l2, length);
}

extern int
main(int argc, const char *argv[]) {
    static char line[2000], firstLine[2000], lastLine[2000];
    char *end;
    long first, last, c;
    int finished;

    first=last=-1;
    finished=0;

    for(;;) {
        if(gets(line)!=NULL) {
            /* parse the initial code point, if any */
            c=strtol(line, &end, 16);
            if(end!=line && *skipWhitespace(end)==';') {
                /* single code point followed by semicolon and data, keep c */
            } else {
                c=-1;
            }
        } else {
            line[0]=0;
            c=-1;
            finished=1;
        }

        if(last>=0 && (c!=(last+1) || !sameData(firstLine, line))) {
            /* output the current range */
            if(first==last) {
                /* there was no range, just output the one line we found */
                puts(firstLine);
            } else {
                /* there was a real range, merge their lines */
                end=strchr(lastLine, '#');
                if(end==NULL) {
                    /* no comment in second line */
                    printf("%04lX..%04lX%s\n",
                            first, last,            /* code point range */
                            strchr(firstLine, ';'));/* first line starting from the first ; */
                } else if(strchr(firstLine, '#')==NULL) {
                    /* no comment in first line */
                    printf("%04lX..%04lX%s%s\n",
                            first, last,            /* code point range */
                            strchr(firstLine, ';'), /* first line starting from the first ; */
                            end);                   /* comment from second line */
                } else {
                    /* merge comments from both lines */
                    printf("%04lX..%04lX%s..%s\n",
                            first, last,            /* code point range */
                            strchr(firstLine, ';'), /* first line starting from the first ; */
                            skipWhitespace(end+1)); /* comment from second line, after # and spaces */
                }
            }
            first=last=-1;
        }

        if(c<0) {
            if(finished) {
                break;
            }

            /* no data on this line, output as is */
            puts(line);
        } else {
            /* data on this line, store for possible range compaction */
            if(last<0) {
                /* set as the first line in a possible range */
                first=last=c;
                strcpy(firstLine, line);
                lastLine[0]=0;
            } else /* must be c==(last+1) && sameData() because of previous conditions */ {
                /* continue with the current range */
                last=c;
                strcpy(lastLine, line);
            }
        }
    }

    return 0;
}
