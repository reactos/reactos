/*
*******************************************************************************
*
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File reslist.c
*
* Modification History:
*
*   Date        Name        Description
*   02/21/00    weiv        Creation.
*******************************************************************************
*/

#include <assert.h>
#include <stdio.h>
#include "reslist.h"
#include "unewdata.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "errmsg.h"

#define BIN_ALIGNMENT 16

static UBool gIncludeCopyright = FALSE;

uint32_t res_write(UNewDataMemory *mem, struct SResource *res,
                   uint32_t usedOffset, UErrorCode *status);

static const UDataInfo dataInfo= {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x52, 0x65, 0x73, 0x42},     /* dataFormat="resb" */
    {1, 2, 0, 0},                 /* formatVersion */
    {1, 4, 0, 0}                  /* dataVersion take a look at version inside parsed resb*/
};

static uint8_t calcPadding(uint32_t size) {
    /* returns space we need to pad */
    return (uint8_t) ((size % sizeof(uint32_t)) ? (sizeof(uint32_t) - (size % sizeof(uint32_t))) : 0);

}

void setIncludeCopyright(UBool val){
    gIncludeCopyright=val;
}

UBool getIncludeCopyright(void){
    return gIncludeCopyright;
}

/* Writing Functions */
static uint32_t string_write(UNewDataMemory *mem, struct SResource *res,
                             uint32_t usedOffset, UErrorCode *status) {
    udata_write32(mem, res->u.fString.fLength);
    udata_writeUString(mem, res->u.fString.fChars, res->u.fString.fLength + 1);
    udata_writePadding(mem, calcPadding(res->fSize));

    return usedOffset;
}

/* Writing Functions */
static uint32_t alias_write(UNewDataMemory *mem, struct SResource *res,
                             uint32_t usedOffset, UErrorCode *status) {
    udata_write32(mem, res->u.fString.fLength);
    udata_writeUString(mem, res->u.fString.fChars, res->u.fString.fLength + 1);
    udata_writePadding(mem, calcPadding(res->fSize));

    return usedOffset;
}

static uint32_t array_write(UNewDataMemory *mem, struct SResource *res,
                            uint32_t usedOffset, UErrorCode *status) {
    uint32_t *resources = NULL;
    uint32_t  i         = 0;

    struct SResource *current = NULL;

    if (U_FAILURE(*status)) {
        return 0;
    }

    if (res->u.fArray.fCount > 0) {
        resources = (uint32_t *) uprv_malloc(sizeof(uint32_t) * res->u.fArray.fCount);

        if (resources == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        current = res->u.fArray.fFirst;
        i = 0;

        while (current != NULL) {
            if (current->fType == URES_INT) {
                resources[i] = (current->fType << 28) | (current->u.fIntValue.fValue & 0xFFFFFFF);
            } else if (current->fType == URES_BINARY) {
                uint32_t uo = usedOffset;

                usedOffset    = res_write(mem, current, usedOffset, status);
                resources[i]  = (current->fType << 28) | (usedOffset >> 2);
                usedOffset   += current->fSize + calcPadding(current->fSize) - (usedOffset - uo);
            } else {
                usedOffset    = res_write(mem, current, usedOffset, status);
                resources[i]  = (current->fType << 28) | (usedOffset >> 2);
                usedOffset   += current->fSize + calcPadding(current->fSize);
            }

            i++;
            current = current->fNext;
        }

        /* usedOffset += res->fSize + pad; */

        udata_write32(mem, res->u.fArray.fCount);
        udata_writeBlock(mem, resources, sizeof(uint32_t) * res->u.fArray.fCount);
        uprv_free(resources);
    } else {
        /* array is empty */
        udata_write32(mem, 0);
    }

    return usedOffset;
}

static uint32_t intvector_write(UNewDataMemory *mem, struct SResource *res,
                                uint32_t usedOffset, UErrorCode *status) {
  uint32_t i = 0;
    udata_write32(mem, res->u.fIntVector.fCount);
    for(i = 0; i<res->u.fIntVector.fCount; i++) {
      udata_write32(mem, res->u.fIntVector.fArray[i]);
    }

    return usedOffset;
}

static uint32_t bin_write(UNewDataMemory *mem, struct SResource *res,
                          uint32_t usedOffset, UErrorCode *status) {
    uint32_t pad       = 0;
    uint32_t extrapad  = calcPadding(res->fSize);
    uint32_t dataStart = usedOffset + sizeof(res->u.fBinaryValue.fLength);

    if (dataStart % BIN_ALIGNMENT) {
        pad = (BIN_ALIGNMENT - dataStart % BIN_ALIGNMENT);
        udata_writePadding(mem, pad);
        usedOffset += pad;
    }

    udata_write32(mem, res->u.fBinaryValue.fLength);
    if (res->u.fBinaryValue.fLength > 0) {
        udata_writeBlock(mem, res->u.fBinaryValue.fData, res->u.fBinaryValue.fLength);
    }
    udata_writePadding(mem, (BIN_ALIGNMENT - pad + extrapad));

    return usedOffset;
}

static uint32_t int_write(UNewDataMemory *mem, struct SResource *res,
                          uint32_t usedOffset, UErrorCode *status) {
    return usedOffset;
}

static uint32_t table_write(UNewDataMemory *mem, struct SResource *res,
                            uint32_t usedOffset, UErrorCode *status) {
    uint8_t   pad       = 0;
    uint32_t  i         = 0;
    uint16_t *keys16    = NULL;
    int32_t  *keys32    = NULL;
    uint32_t *resources = NULL;

    struct SResource *current = NULL;

    if (U_FAILURE(*status)) {
        return 0;
    }

    pad = calcPadding(res->fSize);

    if (res->u.fTable.fCount > 0) {
        if(res->fType == URES_TABLE) {
            keys16 = (uint16_t *) uprv_malloc(sizeof(uint16_t) * res->u.fTable.fCount);
            if (keys16 == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        } else {
            keys32 = (int32_t *) uprv_malloc(sizeof(int32_t) * res->u.fTable.fCount);
            if (keys32 == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        }

        resources = (uint32_t *) uprv_malloc(sizeof(uint32_t) * res->u.fTable.fCount);

        if (resources == NULL) {
            uprv_free(keys16);
            uprv_free(keys32);
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        current = res->u.fTable.fFirst;
        i       = 0;

        while (current != NULL) {
            assert(i < res->u.fTable.fCount);

            /* where the key is */
            if(res->fType == URES_TABLE) {
                keys16[i] = (uint16_t) current->fKey;
            } else {
                keys32[i] = current->fKey;
            }

            if (current->fType == URES_INT) {
                resources[i] = (current->fType << 28) | (current->u.fIntValue.fValue & 0xFFFFFFF);
            } else if (current->fType == URES_BINARY) {
                uint32_t uo = usedOffset;

                usedOffset    = res_write(mem, current, usedOffset, status);
                resources[i]  = (current->fType << 28) | (usedOffset >> 2);
                usedOffset   += current->fSize + calcPadding(current->fSize) - (usedOffset - uo);
            } else {
                usedOffset    = res_write(mem, current, usedOffset, status);
                resources[i]  = (current->fType << 28) | (usedOffset >> 2);
                usedOffset   += current->fSize + calcPadding(current->fSize);
            }

            i++;
            current = current->fNext;
        }

        if(res->fType == URES_TABLE) {
            udata_write16(mem, (uint16_t)res->u.fTable.fCount);

            udata_writeBlock(mem, keys16, sizeof(uint16_t) * res->u.fTable.fCount);
            udata_writePadding(mem, pad);
        } else {
            udata_write32(mem, res->u.fTable.fCount);

            udata_writeBlock(mem, keys32, sizeof(int32_t) * res->u.fTable.fCount);
        }

        udata_writeBlock(mem, resources, sizeof(uint32_t) * res->u.fTable.fCount);

        uprv_free(keys16);
        uprv_free(keys32);
        uprv_free(resources);
    } else {
        /* table is empty */
        if(res->fType == URES_TABLE) {
            udata_write16(mem, 0);
            udata_writePadding(mem, pad);
        } else {
            udata_write32(mem, 0);
        }
    }

    return usedOffset;
}

uint32_t res_write(UNewDataMemory *mem, struct SResource *res,
                   uint32_t usedOffset, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return 0;
    }

    if (res != NULL) {
        switch (res->fType) {
        case URES_STRING:
            return string_write    (mem, res, usedOffset, status);
        case URES_ALIAS:
            return alias_write    (mem, res, usedOffset, status);
        case URES_INT_VECTOR:
            return intvector_write (mem, res, usedOffset, status);
        case URES_BINARY:
            return bin_write       (mem, res, usedOffset, status);
        case URES_INT:
            return int_write       (mem, res, usedOffset, status);
        case URES_ARRAY:
            return array_write     (mem, res, usedOffset, status);
        case URES_TABLE:
        case URES_TABLE32:
            return table_write     (mem, res, usedOffset, status);

        default:
            break;
        }
    }

    *status = U_INTERNAL_PROGRAM_ERROR;
    return 0;
}

void bundle_write(struct SRBRoot *bundle, const char *outputDir, const char *outputPkg, char *writtenFilename, int writtenFilenameLen, UErrorCode *status) {
    UNewDataMemory *mem        = NULL;
    uint8_t         pad        = 0;
    uint32_t        root       = 0;
    uint32_t        usedOffset = 0;
    uint32_t        top, size;
    char            dataName[1024];
    int32_t         indexes[URES_INDEX_TOP];

    if (writtenFilename && writtenFilenameLen) {
        *writtenFilename = 0;
    }

    if (U_FAILURE(*status)) {
        return;
    }

    if (writtenFilename) {
       int32_t off = 0, len = 0;
       if (outputDir) {
           len = (int32_t)uprv_strlen(outputDir);
           if (len > writtenFilenameLen) {
               len = writtenFilenameLen;
           }
           uprv_strncpy(writtenFilename, outputDir, len);
       }
       if (writtenFilenameLen -= len) {
           off += len;
           writtenFilename[off] = U_FILE_SEP_CHAR;
           if (--writtenFilenameLen) {
               ++off;
               if(outputPkg != NULL)
               {
                   uprv_strcpy(writtenFilename+off, outputPkg);
                   off += (int32_t)uprv_strlen(outputPkg);
                   writtenFilename[off] = '_';
                   ++off;
               }

               len = (int32_t)uprv_strlen(bundle->fLocale);
               if (len > writtenFilenameLen) {
                   len = writtenFilenameLen;
               }
               uprv_strncpy(writtenFilename + off, bundle->fLocale, len);
               if (writtenFilenameLen -= len) {
                   off += len;
                   len = 5;
                   if (len > writtenFilenameLen) {
                       len = writtenFilenameLen;
                   }
                   uprv_strncpy(writtenFilename +  off, ".res", len);
               }
           }
       }
    }

    if(outputPkg)
    {
        uprv_strcpy(dataName, outputPkg);
        uprv_strcat(dataName, "_");
        uprv_strcat(dataName, bundle->fLocale);
    }
    else
    {
        uprv_strcpy(dataName, bundle->fLocale);
    }

    mem = udata_create(outputDir, "res", dataName, &dataInfo, (gIncludeCopyright==TRUE)? U_COPYRIGHT_STRING:NULL, status);
    if(U_FAILURE(*status)){
        return;
    }
    pad = calcPadding(bundle->fKeyPoint);

    usedOffset = bundle->fKeyPoint + pad ; /* top of the strings */

    /* we're gonna put the main table at the end */
    top = usedOffset + bundle->fRoot->u.fTable.fChildrenSize;
    root = (top) >> 2 | (bundle->fRoot->fType << 28);

    /* write the root item */
    udata_write32(mem, root);

    /* add to top the size of the root item */
    top += bundle->fRoot->fSize;
    top += calcPadding(top);

    /*
     * formatVersion 1.1 (ICU 2.8):
     * write int32_t indexes[] after root and before the strings
     * to make it easier to parse resource bundles in icuswap or from Java etc.
     */
    uprv_memset(indexes, 0, sizeof(indexes));
    indexes[URES_INDEX_LENGTH]=             URES_INDEX_TOP;
    indexes[URES_INDEX_STRINGS_TOP]=        (int32_t)(usedOffset>>2);
    indexes[URES_INDEX_RESOURCES_TOP]=      (int32_t)(top>>2);
    indexes[URES_INDEX_BUNDLE_TOP]=         indexes[URES_INDEX_RESOURCES_TOP];
    indexes[URES_INDEX_MAX_TABLE_LENGTH]=   bundle->fMaxTableLength;

    /*
     * formatVersion 1.2 (ICU 3.6):
     * write indexes[URES_INDEX_ATTRIBUTES] with URES_ATT_NO_FALLBACK set or not set
     * the memset() above initialized all indexes[] to 0
     */
    if(bundle->noFallback) {
        indexes[URES_INDEX_ATTRIBUTES]=URES_ATT_NO_FALLBACK;
    }

    /* write the indexes[] */
    udata_writeBlock(mem, indexes, sizeof(indexes));

    /* write the table key strings */
    udata_writeBlock(mem, bundle->fKeys+URES_STRINGS_BOTTOM,
                          bundle->fKeyPoint-URES_STRINGS_BOTTOM);

    /* write the padding bytes after the table key strings */
    udata_writePadding(mem, pad);

    /* write all of the bundle contents: the root item and its children */
    usedOffset = res_write(mem, bundle->fRoot, usedOffset, status);

    size = udata_finish(mem, status);
    if(top != size) {
        fprintf(stderr, "genrb error: wrote %u bytes but counted %u\n",
                (int)size, (int)top);
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
}

/* Opening Functions */
struct SResource* res_open(const struct UString* comment, UErrorCode* status){
    struct SResource *res;

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res = (struct SResource *) uprv_malloc(sizeof(struct SResource));

    if (res == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(res, 0, sizeof(struct SResource));

    res->fComment = NULL;
    if(comment != NULL){
        res->fComment = (struct UString *) uprv_malloc(sizeof(struct UString));
        if(res->fComment == NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(res);
            return NULL;
        }
        ustr_init(res->fComment);
        ustr_cpy(res->fComment, comment, status);
    }
    return res;

}
struct SResource* table_open(struct SRBRoot *bundle, char *tag,  const struct UString* comment, UErrorCode *status) {

    struct SResource *res = res_open(comment, status);

    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;

    /*
     * always open a table not a table32 in case it remains empty -
     * try to use table32 only when necessary
     */
    res->fType = URES_TABLE;
    res->fSize = sizeof(uint16_t);

    res->u.fTable.fCount        = 0;
    res->u.fTable.fChildrenSize = 0;
    res->u.fTable.fFirst        = NULL;
    res->u.fTable.fRoot         = bundle;

    return res;
}

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {

    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_ARRAY;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;
    res->fSize = sizeof(int32_t);

    res->u.fArray.fCount        = 0;
    res->u.fArray.fChildrenSize = 0;
    res->u.fArray.fFirst        = NULL;
    res->u.fArray.fLast         = NULL;

    return res;
}

struct SResource *string_open(struct SRBRoot *bundle, char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_STRING;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;

    res->u.fString.fLength = len;
    res->u.fString.fChars  = (UChar *) uprv_malloc(sizeof(UChar) * (len + 1));

    if (res->u.fString.fChars == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }

    uprv_memcpy(res->u.fString.fChars, value, sizeof(UChar) * (len + 1));
    res->fSize = sizeof(int32_t) + sizeof(UChar) * (len+1);

    return res;
}

/* TODO: make alias_open and string_open use the same code */
struct SResource *alias_open(struct SRBRoot *bundle, char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_ALIAS;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;

    res->u.fString.fLength = len;
    res->u.fString.fChars  = (UChar *) uprv_malloc(sizeof(UChar) * (len + 1));

    if (res->u.fString.fChars == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }

    uprv_memcpy(res->u.fString.fChars, value, sizeof(UChar) * (len + 1));
    res->fSize = sizeof(int32_t) + sizeof(UChar) * (len + 1);

    return res;
}


struct SResource* intvector_open(struct SRBRoot *bundle, char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_INT_VECTOR;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;
    res->fSize = sizeof(int32_t);

    res->u.fIntVector.fCount = 0;
    res->u.fIntVector.fArray = (uint32_t *) uprv_malloc(sizeof(uint32_t) * RESLIST_MAX_INT_VECTOR);

    if (res->u.fIntVector.fArray == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }

    return res;
}

struct SResource *int_open(struct SRBRoot *bundle, char *tag, int32_t value, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_INT;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fSize              = 0;
    res->fNext              = NULL;
    res->u.fIntValue.fValue = value;

    return res;
}

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(comment, status);

    if (U_FAILURE(*status)) {
        return NULL;
    }

    res->fType = URES_BINARY;
    res->fKey  = bundle_addtag(bundle, tag, status);

    if (U_FAILURE(*status)) {
        uprv_free(res->fComment);
        uprv_free(res);
        return NULL;
    }

    res->fNext = NULL;

    res->u.fBinaryValue.fLength = length;
    res->u.fBinaryValue.fFileName = NULL;
    if(fileName!=NULL && uprv_strcmp(fileName, "") !=0){
        res->u.fBinaryValue.fFileName = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(fileName)+1));
        uprv_strcpy(res->u.fBinaryValue.fFileName,fileName);
    }
    if (length > 0) {
        res->u.fBinaryValue.fData   = (uint8_t *) uprv_malloc(sizeof(uint8_t) * length);

        if (res->u.fBinaryValue.fData == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(res);
            return NULL;
        }

        uprv_memcpy(res->u.fBinaryValue.fData, data, length);
    }
    else {
        res->u.fBinaryValue.fData = NULL;
    }

    res->fSize = sizeof(int32_t) + sizeof(uint8_t) * length + BIN_ALIGNMENT;

    return res;
}

struct SRBRoot *bundle_open(const struct UString* comment, UErrorCode *status) {
    struct SRBRoot *bundle = NULL;

    if (U_FAILURE(*status)) {
        return NULL;
    }

    bundle = (struct SRBRoot *) uprv_malloc(sizeof(struct SRBRoot));

    if (bundle == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    uprv_memset(bundle, 0, sizeof(struct SRBRoot));

    bundle->fLocale   = NULL;

    bundle->fKeys     = (char *) uprv_malloc(sizeof(char) * KEY_SPACE_SIZE);
    bundle->fKeysCapacity = KEY_SPACE_SIZE;

    if(comment != NULL){

    }

    if (bundle->fKeys == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(bundle);
        return NULL;
    }

    /* formatVersion 1.1: start fKeyPoint after the root item and indexes[] */
    bundle->fKeyPoint = URES_STRINGS_BOTTOM;
    uprv_memset(bundle->fKeys, 0, URES_STRINGS_BOTTOM);

    bundle->fCount = 0;
    bundle->fRoot  = table_open(bundle, NULL, comment, status);

    if (bundle->fRoot == NULL || U_FAILURE(*status)) {
        if (U_SUCCESS(*status)) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        }

        uprv_free(bundle->fKeys);
        uprv_free(bundle);

        return NULL;
    }

    return bundle;
}

/* Closing Functions */
void table_close(struct SResource *table, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;

    current = table->u.fTable.fFirst;

    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev, status);
    }

    table->u.fTable.fFirst = NULL;
}

void array_close(struct SResource *array, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    
    if(array==NULL){
        return;
    }
    current = array->u.fArray.fFirst;
    
    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev, status);
    }
    array->u.fArray.fFirst = NULL;
}

void string_close(struct SResource *string, UErrorCode *status) {
    if (string->u.fString.fChars != NULL) {
        uprv_free(string->u.fString.fChars);
        string->u.fString.fChars =NULL;
    }
}

void alias_close(struct SResource *alias, UErrorCode *status) {
    if (alias->u.fString.fChars != NULL) {
        uprv_free(alias->u.fString.fChars);
        alias->u.fString.fChars =NULL;
    }
}

void intvector_close(struct SResource *intvector, UErrorCode *status) {
    if (intvector->u.fIntVector.fArray != NULL) {
        uprv_free(intvector->u.fIntVector.fArray);
        intvector->u.fIntVector.fArray =NULL;
    }
}

void int_close(struct SResource *intres, UErrorCode *status) {
    /* Intentionally left blank */
}

void bin_close(struct SResource *binres, UErrorCode *status) {
    if (binres->u.fBinaryValue.fData != NULL) {
        uprv_free(binres->u.fBinaryValue.fData);
        binres->u.fBinaryValue.fData = NULL;
    }
}

void res_close(struct SResource *res, UErrorCode *status) {
    if (res != NULL) {
        switch(res->fType) {
        case URES_STRING:
            string_close(res, status);
            break;
        case URES_ALIAS:
            alias_close(res, status);
            break;
        case URES_INT_VECTOR:
            intvector_close(res, status);
            break;
        case URES_BINARY:
            bin_close(res, status);
            break;
        case URES_INT:
            int_close(res, status);
            break;
        case URES_ARRAY:
            array_close(res, status);
            break;
        case URES_TABLE:
        case URES_TABLE32:
            table_close(res, status);
            break;
        default:
            /* Shouldn't happen */
            break;
        }

        uprv_free(res);
    }
}

void bundle_close(struct SRBRoot *bundle, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;

    if (bundle->fRoot != NULL) {
        current = bundle->fRoot->u.fTable.fFirst;

        while (current != NULL) {
            prev    = current;
            current = current->fNext;

            res_close(prev, status);
        }

        uprv_free(bundle->fRoot);
    }

    if (bundle->fLocale != NULL) {
        uprv_free(bundle->fLocale);
    }

    if (bundle->fKeys != NULL) {
        uprv_free(bundle->fKeys);
    }

    uprv_free(bundle);
}

/* Adding Functions */
void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    struct SResTable *list;

    if (U_FAILURE(*status)) {
        return;
    }

    /* remember this linenumber to report to the user if there is a duplicate key */
    res->line = linenumber;

    /* here we need to traverse the list */
    list = &(table->u.fTable);

    if(table->fType == URES_TABLE && res->fKey > 0xffff) {
        /* this table straddles the 64k strings boundary, update to a table32 */
        table->fType = URES_TABLE32;

        /*
         * increase the size because count and each string offset
         * increase from uint16_t to int32_t
         */
        table->fSize += (1 + list->fCount) * 2;
    }

    ++(list->fCount);
    if(list->fCount > (uint32_t)list->fRoot->fMaxTableLength) {
        list->fRoot->fMaxTableLength = list->fCount;
    }

    /*
     * URES_TABLE:   6 bytes = 1 uint16_t key string offset + 1 uint32_t Resource
     * URES_TABLE32: 8 bytes = 1 int32_t key string offset + 1 uint32_t Resource
     */
    table->fSize += table->fType == URES_TABLE ? 6 : 8;

    table->u.fTable.fChildrenSize += res->fSize + calcPadding(res->fSize);

    if (res->fType == URES_TABLE || res->fType == URES_TABLE32) {
        table->u.fTable.fChildrenSize += res->u.fTable.fChildrenSize;
    } else if (res->fType == URES_ARRAY) {
        table->u.fTable.fChildrenSize += res->u.fArray.fChildrenSize;
    }

    /* is list still empty? */
    if (list->fFirst == NULL) {
        list->fFirst = res;
        res->fNext   = NULL;
        return;
    }

    current = list->fFirst;

    while (current != NULL) {
        if (uprv_strcmp(((list->fRoot->fKeys) + (current->fKey)), ((list->fRoot->fKeys) + (res->fKey))) < 0) {
            prev    = current;
            current = current->fNext;
        } else if (uprv_strcmp(((list->fRoot->fKeys) + (current->fKey)), ((list->fRoot->fKeys) + (res->fKey))) > 0) {
            /* we're either in front of list, or in middle */
            if (prev == NULL) {
                /* front of the list */
                list->fFirst = res;
            } else {
                /* middle of the list */
                prev->fNext = res;
            }

            res->fNext = current;
            return;
        } else {
            /* Key already exists! ERROR! */
            error(linenumber, "duplicate key '%s' in table, first appeared at line %d", list->fRoot->fKeys + current->fKey, current->line);
            *status = U_UNSUPPORTED_ERROR;
            return;
        }
    }

    /* end of list */
    prev->fNext = res;
    res->fNext  = NULL;
}

void array_add(struct SResource *array, struct SResource *res, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    if (array->u.fArray.fFirst == NULL) {
        array->u.fArray.fFirst = res;
        array->u.fArray.fLast  = res;
    } else {
        array->u.fArray.fLast->fNext = res;
        array->u.fArray.fLast        = res;
    }

    (array->u.fArray.fCount)++;

    array->fSize += sizeof(uint32_t);
    array->u.fArray.fChildrenSize += res->fSize + calcPadding(res->fSize);

    if (res->fType == URES_TABLE || res->fType == URES_TABLE32) {
        array->u.fArray.fChildrenSize += res->u.fTable.fChildrenSize;
    } else if (res->fType == URES_ARRAY) {
        array->u.fArray.fChildrenSize += res->u.fArray.fChildrenSize;
    }
}

void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    *(intvector->u.fIntVector.fArray + intvector->u.fIntVector.fCount) = value;
    intvector->u.fIntVector.fCount++;

    intvector->fSize += sizeof(uint32_t);
}

/* Misc Functions */

void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status) {

    if(U_FAILURE(*status)) {
        return;
    }

    if (bundle->fLocale!=NULL) {
        uprv_free(bundle->fLocale);
    }

    bundle->fLocale= (char*) uprv_malloc(sizeof(char) * (u_strlen(locale)+1));

    if(bundle->fLocale == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    /*u_strcpy(bundle->fLocale, locale);*/
    u_UCharsToChars(locale, bundle->fLocale, u_strlen(locale)+1);

}


int32_t
bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status) {
    int32_t keypos, length;

    if (U_FAILURE(*status)) {
        return -1;
    }

    if (tag == NULL) {
        /* do not set an error: the root table has a NULL tag */
        return -1;
    }

    keypos = bundle->fKeyPoint;

    bundle->fKeyPoint += length = (int32_t) (uprv_strlen(tag) + 1);

    if (bundle->fKeyPoint >= bundle->fKeysCapacity) {
        /* overflow - resize the keys buffer */
        bundle->fKeysCapacity += KEY_SPACE_SIZE;
        bundle->fKeys = uprv_realloc(bundle->fKeys, bundle->fKeysCapacity);
        if(bundle->fKeys == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
    }

    uprv_memcpy(bundle->fKeys + keypos, tag, length);

    return keypos;
}
