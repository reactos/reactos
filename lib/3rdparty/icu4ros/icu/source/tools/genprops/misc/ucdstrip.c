/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucdstrip.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003feb20
*   created by: Markus W. Scherer
*
*   Simple tool for Unicode Character Database files with semicolon-delimited fields.
*   Removes comments behind data lines but not in others.
*
*   To compile, just call a C compiler/linker with this source file.
*   On Windows: cl ucdstrip.c
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

extern int
main(int argc, const char *argv[]) {
    static char line[2000];
    char *end;

    while(gets(line)!=NULL) {
        if(strtol(line, &end, 16)>=0 && end!=line) {
            /* code point or range followed by semicolon and data, remove comment */
            *endOfData(line)=0;
        }
        puts(line);
    }

    return 0;
}
