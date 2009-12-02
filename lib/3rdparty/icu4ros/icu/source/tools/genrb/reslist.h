/*
*******************************************************************************
*
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File reslist.h
*
* Modification History:
*
*   Date        Name        Description
*   02/21/00    weiv        Creation.
*******************************************************************************
*/

#ifndef RESLIST_H
#define RESLIST_H

#define KEY_SPACE_SIZE 65536
#define RESLIST_MAX_INT_VECTOR 2048

#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "uresdata.h"
#include "cmemory.h"
#include "cstring.h"
#include "unewdata.h"
#include "ustr.h"

U_CDECL_BEGIN

/* Resource bundle root table */
struct SRBRoot {
  char *fLocale;
  int32_t fKeyPoint;
  char *fKeys;
  int32_t fKeysCapacity;
  int32_t fCount;
  struct SResource *fRoot;
  int32_t fMaxTableLength;
  UBool noFallback; /* see URES_ATT_NO_FALLBACK */
};

struct SRBRoot *bundle_open(const struct UString* comment, UErrorCode *status);
void bundle_write(struct SRBRoot *bundle, const char *outputDir, const char *outputPkg, char *writtenFilename, int writtenFilenameLen, UErrorCode *status);

/* write a java resource file */
void bundle_write_java(struct SRBRoot *bundle, const char *outputDir, const char* outputEnc, char *writtenFilename, 
                       int writtenFilenameLen, const char* packageName, const char* bundleName, UErrorCode *status);

/* write a xml resource file */
/* commented by Jing*/
/* void bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, 
                  char *writtenFilename, int writtenFilenameLen,UErrorCode *status); */

/* added by Jing*/
void bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, const char* rbname,
                  char *writtenFilename, int writtenFilenameLen, const char* language, const char* package, UErrorCode *status);

void bundle_close(struct SRBRoot *bundle, UErrorCode *status);
void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status);
int32_t bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status);

/* Various resource types */
struct SResource* res_open(const struct UString* comment, UErrorCode* status);

struct SResTable {
    uint32_t fCount;
    uint32_t fChildrenSize;
    struct SResource *fFirst;
    struct SRBRoot *fRoot;
};

struct SResource* table_open(struct SRBRoot *bundle, char *tag, const struct UString* comment, UErrorCode *status);
void table_close(struct SResource *table, UErrorCode *status);
void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status);

struct SResArray {
    uint32_t fCount;
    uint32_t fChildrenSize;
    struct SResource *fFirst;
    struct SResource *fLast;
};

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status);
void array_close(struct SResource *array, UErrorCode *status);
void array_add(struct SResource *array, struct SResource *res, UErrorCode *status);

struct SResString {
    uint32_t fLength;
    UChar *fChars;
};

struct SResource *string_open(struct SRBRoot *bundle, char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);
void string_close(struct SResource *string, UErrorCode *status);

struct SResource *alias_open(struct SRBRoot *bundle, char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);
void alias_close(struct SResource *string, UErrorCode *status);

struct SResIntVector {
    uint32_t fCount;
    uint32_t *fArray;
};

struct SResource* intvector_open(struct SRBRoot *bundle, char *tag,  const struct UString* comment, UErrorCode *status);
void intvector_close(struct SResource *intvector, UErrorCode *status);
void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status);

struct SResInt {
    uint32_t fValue;
};

struct SResource *int_open(struct SRBRoot *bundle, char *tag, int32_t value, const struct UString* comment, UErrorCode *status);
void int_close(struct SResource *intres, UErrorCode *status);

struct SResBinary {
    uint32_t fLength;
    uint8_t *fData;
    char* fFileName; /* file name for binary or import binary tags if any */
};

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status);
void bin_close(struct SResource *binres, UErrorCode *status);

/* Resource place holder */

struct SResource {
    UResType fType;
    int32_t  fKey;
    uint32_t fSize; /* Size in bytes outside the header part */
    int      line;  /* used internally to report duplicate keys in tables */
    struct SResource *fNext; /*This is for internal chaining while building*/
    struct UString *fComment;
    union {
        struct SResTable fTable;
        struct SResArray fArray;
        struct SResString fString;
        struct SResIntVector fIntVector;
        struct SResInt fIntValue;
        struct SResBinary fBinaryValue;
    } u;
};

void res_close(struct SResource *res, UErrorCode *status);
void setIncludeCopyright(UBool val);
UBool getIncludeCopyright(void);

U_CDECL_END
#endif /* #ifndef RESLIST_H */
