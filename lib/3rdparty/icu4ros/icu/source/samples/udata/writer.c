/*
*******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  writer.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000sep5
*   created by: Vladimir Weinstein
*/

/******************************************************************************
 * A program to write simple binary data readable by udata - example for
 * ICU workshop
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "unicode/utypes.h"
#include "unicode/udata.h"

/* this is private - available only through toolutil */
#include "unewdata.h"

#define DATA_NAME "mypkg_example"
#define DATA_TYPE "dat"

/* UDataInfo cf. udata.h */
static const UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    0x4D, 0x79, 0x44, 0x74,     /* dataFormat="MyDt" */
    1, 0, 0, 0,                 /* formatVersion */
    1, 0, 0, 0                  /* dataVersion */
};


/* Excersise: add writing out other data types */
/* see icu/source/tools/toolutil/unewdata.h    */
/* for other possibilities                     */

extern int
main(int argc, const char *argv[]) {
    UNewDataMemory *pData;
    UErrorCode errorCode=U_ZERO_ERROR;
    char stringValue[]={'E', 'X', 'A', 'M', 'P', 'L', 'E', '\0'};
    uint16_t intValue=2000;
    
    long dataLength;
    uint32_t size;
#ifdef WIN32
    char *currdir = _getcwd(NULL, 0);
#else
    char *currdir = getcwd(NULL, 0);
#endif

    pData=udata_create(currdir, DATA_TYPE, DATA_NAME, &dataInfo,
                       U_COPYRIGHT_STRING, &errorCode);

    if(currdir != NULL) {
        free(currdir);
    }


    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "Error: unable to create data memory, error %d\n", errorCode);
        exit(errorCode);
    }

    /* write the data to the file */
    /* a 16 bit value  and a String*/
    printf("Writing uint16_t value of %d\n", intValue);
    udata_write16(pData, intValue);
    printf("Writing string value of %s\n", stringValue);
    udata_writeString(pData, stringValue, sizeof(stringValue));

    /* finish up */
    dataLength=udata_finish(pData, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "Error: error %d writing the output file\n", errorCode);
        exit(errorCode);
    }
    size=sizeof(stringValue) + sizeof(intValue);


    if(dataLength!=(long)size) {
        fprintf(stderr, "Error: data length %ld != calculated size %lu\n", dataLength, size);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }
    return 0;
}











