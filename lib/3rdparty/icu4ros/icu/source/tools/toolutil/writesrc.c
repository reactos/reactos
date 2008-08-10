/*
*******************************************************************************
*
*   Copyright (C) 2005-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  writesrc.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005apr23
*   created by: Markus W. Scherer
*
*   Helper functions for writing source code for data.
*/

#include <stdio.h>
#include <time.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "utrie.h"
#include "cstring.h"
#include "writesrc.h"

U_CAPI FILE * U_EXPORT2
usrc_create(const char *path, const char *filename) {
    char buffer[1024];
    const char *p;
    char *q;
    FILE *f;
    char c;

    if(path==NULL) {
        p=filename;
    } else {
        /* concatenate path and filename, with U_FILE_SEP_CHAR in between if necessary */
        uprv_strcpy(buffer, path);
        q=buffer+uprv_strlen(buffer);
        if(q>buffer && (c=*(q-1))!=U_FILE_SEP_CHAR && c!=U_FILE_ALT_SEP_CHAR) {
            *q++=U_FILE_SEP_CHAR;
        }
        uprv_strcpy(q, filename);
        p=buffer;
    }

    f=fopen(p, "w");
    if(f!=NULL) {
        char year[8];
        const struct tm *lt;
        time_t t;

        time(&t);
        lt=localtime(&t);
        strftime(year, sizeof(year), "%Y", lt);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", lt);
        fprintf(
            f,
            "/*\n"
            " * Copyright (C) 1999-%s, International Business Machines\n"
            " * Corporation and others.  All Rights Reserved.\n"
            " *\n"
            " * file name: %s\n"
            " *\n"
            " * machine-generated on: %s\n"
            " */\n\n",
            year,
            filename,
            buffer);
    } else {
        fprintf(
            stderr,
            "usrc_create(%s, %s): unable to create file\n",
            path!=NULL ? path : "", filename);
    }
    return f;
}

U_CAPI void U_EXPORT2
usrc_writeArray(FILE *f,
                const char *prefix,
                const void *p, int32_t width, int32_t length,
                const char *postfix) {
    const uint8_t *p8;
    const uint16_t *p16;
    const uint32_t *p32;
    uint32_t value;
    int32_t i, col;

    p8=NULL;
    p16=NULL;
    p32=NULL;
    switch(width) {
    case 8:
        p8=(const uint8_t *)p;
        break;
    case 16:
        p16=(const uint16_t *)p;
        break;
    case 32:
        p32=(const uint32_t *)p;
        break;
    default:
        fprintf(stderr, "usrc_writeArray(width=%ld) unrecognized width\n", (long)width);
        return;
    }
    if(prefix!=NULL) {
        fprintf(f, prefix, (long)length);
    }
    for(i=col=0; i<length; ++i, ++col) {
        if(i>0) {
            if(col<16) {
                fputc(',', f);
            } else {
                fputs(",\n", f);
                col=0;
            }
        }
        switch(width) {
        case 8:
            value=p8[i];
            break;
        case 16:
            value=p16[i];
            break;
        case 32:
            value=p32[i];
            break;
        default:
            value=0; /* unreachable */
            break;
        }
        fprintf(f, value<=9 ? "%lu" : "0x%lx", (unsigned long)value);
    }
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUTrieArrays(FILE *f,
                      const char *indexPrefix, const char *dataPrefix,
                      const UTrie *pTrie,
                      const char *postfix) {
    if(pTrie->data32==NULL) {
        /* 16-bit trie */
        usrc_writeArray(f, indexPrefix, pTrie->index, 16, pTrie->indexLength+pTrie->dataLength, postfix);
    } else {
        /* 32-bit trie */
        usrc_writeArray(f, indexPrefix, pTrie->index, 16, pTrie->indexLength, postfix);
        usrc_writeArray(f, dataPrefix, pTrie->data32, 32, pTrie->dataLength, postfix);
    }
}

U_CAPI void U_EXPORT2
usrc_writeUTrieStruct(FILE *f,
                      const char *prefix,
                      const UTrie *pTrie,
                      const char *indexName, const char *dataName,
                      const char *getFoldingOffsetName,
                      const char *postfix) {
    if(prefix!=NULL) {
        fputs(prefix, f);
    }
    if(dataName==NULL) {
        dataName="NULL";
    }
    if(getFoldingOffsetName==NULL) {
        getFoldingOffsetName="utrie_defaultGetFoldingOffset";
    }
    fprintf(
        f,
        "    %s,\n"
        "    %s,\n"
        "    %s,\n"
        "    %ld,\n"
        "    %ld,\n"
        "    %lu,\n"
        "    %s\n",
        indexName,
        dataName,
        getFoldingOffsetName,
        (long)pTrie->indexLength, (long)pTrie->dataLength,
        (unsigned long)pTrie->initialValue,
        pTrie->isLatin1Linear ? "TRUE" : "FALSE");
    if(postfix!=NULL) {
        fputs(postfix, f);
    }
}
