/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2004, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  reader.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2000sep5
 *   created by: Vladimir Weinstein
 */

/*******************************************************************************
 * Derived from Madhu Katragadda gentest
 *******************************************************************************/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"

#define DATA_NAME "example"
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

static UBool
isAcceptable(void *context, 
             const char *type, const char *name,
             const UDataInfo *pInfo){

    if( pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==0x4D &&   /* dataFormat="MyDt" */
        pInfo->dataFormat[1]==0x79 &&
        pInfo->dataFormat[2]==0x44 &&
        pInfo->dataFormat[3]==0x74 &&
        pInfo->formatVersion[0]==1 &&
        pInfo->dataVersion[0]==1   ) {
        return TRUE;
    } else {
        return FALSE;
    }


}

extern int
main(int argc, const char *argv[]) {
    UDataMemory *result = NULL;
    UErrorCode status=U_ZERO_ERROR;

    uint16_t intValue = 0;

    char *string = NULL;
    uint16_t *intPointer = NULL;

    const void *dataMemory = NULL;
    char curPathBuffer[1024];
 
#ifdef WIN32
    char *currdir = _getcwd(NULL, 0);
#else
    char *currdir = getcwd(NULL, 0);
#endif

    /* need to put  "current/dir/pkgname" as path */
    strcpy(curPathBuffer, currdir);
    strcat(curPathBuffer, U_FILE_SEP_STRING);
    strcat(curPathBuffer, "mypkg"); /* package name */

    result=udata_openChoice(curPathBuffer, DATA_TYPE, DATA_NAME, isAcceptable, NULL, &status);

    if(currdir != NULL) {
        free(currdir);
    }

    if(U_FAILURE(status)){
        printf("Failed to open data file example.dat in %s with error number %d\n", curPathBuffer, status);
        return -1;
    }

    dataMemory = udata_getMemory(result);

    intPointer = (uint16_t *)dataMemory;

    printf("Read value %d from data file\n", *intPointer);

    string = (char *) (intPointer+1);

    printf("Read string %s from data file\n", string);

    if(U_SUCCESS(status)){
        udata_close(result);
    }

    return 0;
}







