/*
 ********************************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 ********************************************************************************
 */

#include "unicode/utypes.h"
#include "unicode/ucsdet.h"

#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE 8192

int main(int argc, char *argv[])
{
    static char buffer[BUFFER_SIZE];
    int32_t arg;

    if( argc <= 1 ) {
        printf("Usage: %s [filename]...\n", argv[0]);
        return -1;
    }

    for(arg = 1; arg < argc; arg += 1) {
        FILE *file;
        char *filename = argv[arg];
        int32_t inputLength, match, matchCount = 0;
        UCharsetDetector* csd;
        const UCharsetMatch **csm;
        UErrorCode status = U_ZERO_ERROR;

        if (arg > 1) {
            printf("\n");
        }

        file = fopen(filename, "rb");

        if (file == NULL) {
            printf("Cannot open file \"%s\"\n\n", filename);
            continue;
        }

        printf("%s:\n", filename);

        inputLength = (int32_t) fread(buffer, 1, BUFFER_SIZE, file);

        fclose(file);

        csd = ucsdet_open(&status);
        ucsdet_setText(csd, buffer, inputLength, &status);

        csm = ucsdet_detectAll(csd, &matchCount, &status);

        for(match = 0; match < matchCount; match += 1) {
            const char *name = ucsdet_getName(csm[match], &status);
            const char *lang = ucsdet_getLanguage(csm[match], &status);
            int32_t confidence = ucsdet_getConfidence(csm[match], &status);

            if (lang == NULL || strlen(lang) == 0) {
                lang = "**";
            }

            printf("%s (%s) %d\n", name, lang, confidence);
        }

        ucsdet_close(csd);
    }
    
    return 0;
}

