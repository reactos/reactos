/*
*******************************************************************************
*                                                                             *
* Copyright (C) 1999-2006, International Business Machines Corporation        *
*               and others. All Rights Reserved.                              *
*                                                                             *
*******************************************************************************
*   file name:  uresdata.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec08
*   created by: Markus W. Scherer
* Modification History:
*
*   Date        Name        Description
*   06/20/2000  helena      OS/400 port changes; mostly typecast.
*   06/24/02    weiv        Added support for resource sharing
*/

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uarrsort.h"
#include "udataswp.h"
#include "ucol_swp.h"
#include "uresdata.h"
#include "uresimp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/*
 * Resource access helpers
 */

/* get a const char* pointer to the key with the keyOffset byte offset from pRoot */
#define RES_GET_KEY(pRoot, keyOffset) ((const char *)(pRoot)+(keyOffset))
#define URESDATA_ITEM_NOT_FOUND -1

/*
 * All the type-access functions assume that
 * the resource is of the expected type.
 */


/*
 * Array functions
 */
static Resource
_res_getArrayItem(Resource *pRoot, Resource res, int32_t indexR) {
    const int32_t *p=(const int32_t *)RES_GET_POINTER(pRoot, res);
    if(indexR<*p) {
        return ((const Resource *)(p))[1+indexR];
    } else {
        return RES_BOGUS;   /* indexR>itemCount */
    }
}

/*
 * Table functions
 *
 * Important: the key offsets are 16-bit byte offsets from pRoot,
 * and the itemCount is one more 16-bit, too.
 * Thus, there are (count+1) uint16_t values.
 * In order to 4-align the Resource item values, there is a padding
 * word if count is even, i.e., there is exactly (~count&1)
 * 16-bit padding words.
 *
 * For Table32, both the count and the key offsets are int32_t's
 * and need not alignment.
 */
static const char *
_res_getTableKey(const Resource *pRoot, const Resource res, int32_t indexS) {
    const uint16_t *p=(const uint16_t *)RES_GET_POINTER(pRoot, res);
    if((uint32_t)indexS<(uint32_t)*p) {
        return RES_GET_KEY(pRoot, p[indexS+1]);
    } else {
        return NULL;    /* indexS>itemCount */
    }
}

static const char *
_res_getTable32Key(const Resource *pRoot, const Resource res, int32_t indexS) {
    const int32_t *p=(const int32_t *)RES_GET_POINTER(pRoot, res);
    if((uint32_t)indexS<(uint32_t)*p) {
        return RES_GET_KEY(pRoot, p[indexS+1]);
    } else {
        return NULL;    /* indexS>itemCount */
    }
}


static Resource
_res_getTableItem(const Resource *pRoot, const Resource res, int32_t indexR) {
    const uint16_t *p=(const uint16_t *)RES_GET_POINTER(pRoot, res);
    int32_t count=*p;
    if((uint32_t)indexR<(uint32_t)count) {
        return ((const Resource *)(p+1+count+(~count&1)))[indexR];
    } else {
        return RES_BOGUS;   /* indexR>itemCount */
    }
}

static Resource
_res_getTable32Item(const Resource *pRoot, const Resource res, int32_t indexR) {
    const int32_t *p=(const int32_t *)RES_GET_POINTER(pRoot, res);
    int32_t count=*p;
    if((uint32_t)indexR<(uint32_t)count) {
        return ((const Resource *)(p+1+count))[indexR];
    } else {
        return RES_BOGUS;   /* indexR>itemCount */
    }
}


static Resource
_res_findTableItem(const Resource *pRoot, const Resource res, const char *key,
                   int32_t *index, const char **realKey) {
    const uint16_t *p=(const uint16_t *)RES_GET_POINTER(pRoot, res);
    uint32_t mid, start, limit;
    uint32_t lastMid;
    int result;

    limit=*p++; /* number of entries */

    if(limit != 0) {
        /* do a binary search for the key */
        start=0;
        lastMid = UINT32_MAX;
        for (;;) {
            mid = (uint32_t)((start + limit) / 2);
            if (lastMid == mid) {   /* Have we moved? */
                break;  /* We haven't moved, and it wasn't found. */
            }
            lastMid = mid;
            result = uprv_strcmp(key, RES_GET_KEY(pRoot, p[mid]));

            if (result < 0) {
                limit = mid;
            } else if (result > 0) {
                start = mid;
            } else {
                /* We found it! */
                *index=mid;
                *realKey=RES_GET_KEY(pRoot, p[mid]);
                limit=*(p-1);   /* itemCount */
                return ((const Resource *)(p+limit+(~limit&1)))[mid];
            }
        }
    }

    *index=URESDATA_ITEM_NOT_FOUND;
    return RES_BOGUS;   /* not found or table is empty. */
}

static Resource
_res_findTable32Item(const Resource *pRoot, const Resource res, const char *key,
                     int32_t *index, const char **realKey) {
    const int32_t *p=(const int32_t *)RES_GET_POINTER(pRoot, res);
    int32_t mid, start, limit;
    int32_t lastMid;
    int result;

    limit=*p++; /* number of entries */

    if(limit != 0) {
        /* do a binary search for the key */
        start=0;
        lastMid = INT32_MAX;
        for (;;) {
            mid = (uint32_t)((start + limit) / 2);
            if (lastMid == mid) {   /* Have we moved? */
                break;  /* We haven't moved, and it wasn't found. */
            }
            lastMid = mid;
            result = uprv_strcmp(key, RES_GET_KEY(pRoot, p[mid]));

            if (result < 0) {
                limit = mid;
            } else if (result > 0) {
                start = mid;
            } else {
                /* We found it! */
                *index=mid;
                *realKey=RES_GET_KEY(pRoot, p[mid]);
                return ((const Resource *)(p+(*(p-1))))[mid];
            }
        }
    }

    *index=URESDATA_ITEM_NOT_FOUND;
    return RES_BOGUS;   /* not found or table is empty. */
}

/* helper for res_load() ---------------------------------------------------- */

static UBool U_CALLCONV
isAcceptable(void *context,
             const char *type, const char *name,
             const UDataInfo *pInfo) {
    uprv_memcpy(context, pInfo->formatVersion, 4);
    return (UBool)(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->sizeofUChar==U_SIZEOF_UCHAR &&
        pInfo->dataFormat[0]==0x52 &&   /* dataFormat="ResB" */
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x42 &&
        pInfo->formatVersion[0]==1);
}

/* semi-public functions ---------------------------------------------------- */

U_CFUNC UBool
res_load(ResourceData *pResData,
         const char *path, const char *name, UErrorCode *errorCode) {
    UVersionInfo formatVersion;
    UResType rootType;

    /* load the ResourceBundle file */
    pResData->data=udata_openChoice(path, "res", name, isAcceptable, formatVersion, errorCode);
    if(U_FAILURE(*errorCode)) {
        return FALSE;
    }

    /* get its memory and root resource */
    pResData->pRoot=(Resource *)udata_getMemory(pResData->data);
    pResData->rootRes=*pResData->pRoot;
    pResData->noFallback=FALSE;

    /* currently, we accept only resources that have a Table as their roots */
    rootType=RES_GET_TYPE(pResData->rootRes);
    if(rootType!=URES_TABLE && rootType!=URES_TABLE32) {
        *errorCode=U_INVALID_FORMAT_ERROR;
        udata_close(pResData->data);
        pResData->data=NULL; 
        return FALSE;
    }

    if(formatVersion[0]>1 || (formatVersion[0]==1 && formatVersion[1]>=1)) {
        /* bundles with formatVersion 1.1 and later contain an indexes[] array */
        const int32_t *indexes=(const int32_t *)pResData->pRoot+1;
        if(indexes[URES_INDEX_LENGTH]>URES_INDEX_ATTRIBUTES) {
            pResData->noFallback=(UBool)(indexes[URES_INDEX_ATTRIBUTES]&URES_ATT_NO_FALLBACK);
        }
    }

    return TRUE;
}

U_CFUNC void
res_unload(ResourceData *pResData) {
    if(pResData->data!=NULL) {
        udata_close(pResData->data);
        pResData->data=NULL;
    }
}

U_CFUNC const UChar *
res_getString(const ResourceData *pResData, const Resource res, int32_t *pLength) {
    /*
     * The data structure is documented as supporting res==0 for empty strings.
     * Return a fixed pointer in such a case.
     * This was dropped in uresdata.c 1.17 as part of Jitterbug 1005 work
     * on code coverage for ICU 2.0.
     * Re-added for consistency with the design and with other code.
     */
    static const int32_t emptyString[2]={ 0, 0 };
    if(res!=RES_BOGUS && RES_GET_TYPE(res)==URES_STRING) {
        const int32_t *p= res==0 ? emptyString : (const int32_t *)RES_GET_POINTER(pResData->pRoot, res);
        if (pLength) {
            *pLength=*p;
        }
        return (const UChar *)++p;
    } else {
        if (pLength) {
            *pLength=0;
        }
        return NULL;
    }
}

U_CFUNC const UChar *
res_getAlias(const ResourceData *pResData, const Resource res, int32_t *pLength) {
    if(res!=RES_BOGUS && RES_GET_TYPE(res)==URES_ALIAS) {
        const int32_t *p=(const int32_t *)RES_GET_POINTER(pResData->pRoot, res);
        if (pLength) {
            *pLength=*p;
        }
        return (const UChar *)++p;
    } else {
        if (pLength) {
            *pLength=0;
        }
        return NULL;
    }
}

U_CFUNC const uint8_t *
res_getBinary(const ResourceData *pResData, const Resource res, int32_t *pLength) {
    if(res!=RES_BOGUS) {
        const int32_t *p=(const int32_t *)RES_GET_POINTER(pResData->pRoot, res);
        *pLength=*p++;
        if (*pLength == 0) {
            p = NULL;
        }
        return (const uint8_t *)p;
    } else {
        *pLength=0;
        return NULL;
    }
}


U_CFUNC const int32_t *
res_getIntVector(const ResourceData *pResData, const Resource res, int32_t *pLength) {
    if(res!=RES_BOGUS && RES_GET_TYPE(res)==URES_INT_VECTOR) {
        const int32_t *p=(const int32_t *)RES_GET_POINTER(pResData->pRoot, res);
        *pLength=*p++;
        if (*pLength == 0) {
            p = NULL;
        }
        return (const int32_t *)p;
    } else {
        *pLength=0;
        return NULL;
    }
}

U_CFUNC int32_t
res_countArrayItems(const ResourceData *pResData, const Resource res) {
    if(res!=RES_BOGUS) {
        switch(RES_GET_TYPE(res)) {
        case URES_STRING:
        case URES_BINARY:
        case URES_ALIAS:
        case URES_INT:
        case URES_INT_VECTOR:
            return 1;
        case URES_ARRAY:
        case URES_TABLE32: {
            const int32_t *p=(const int32_t *)RES_GET_POINTER(pResData->pRoot, res);
            return *p;
        }
        case URES_TABLE: {
            const uint16_t *p=(const uint16_t *)RES_GET_POINTER(pResData->pRoot, res);
            return *p;
        }
        default:
            break;
        }
    } 
    return 0;
}

U_CFUNC Resource
res_getResource(const ResourceData *pResData, const char *key) {
    int32_t index;
    const char *realKey;
    if(RES_GET_TYPE(pResData->rootRes)==URES_TABLE) {
        return _res_findTableItem(pResData->pRoot, pResData->rootRes, key, &index, &realKey);
    } else {
        return _res_findTable32Item(pResData->pRoot, pResData->rootRes, key, &index, &realKey);
    }
}

U_CFUNC Resource 
res_getArrayItem(const ResourceData *pResData, Resource array, const int32_t indexR) {
    return _res_getArrayItem(pResData->pRoot, array, indexR);
}

U_CFUNC Resource
res_findResource(const ResourceData *pResData, Resource r, char** path, const char** key) {
  /* we pass in a path. CollationElements/Sequence or zoneStrings/3/2 etc. 
   * iterates over a path and stops when a scalar resource is found. This  
   * CAN be an alias. Path gets set to the part that has not yet been processed. 
   */

  char *pathP = *path, *nextSepP = *path;
  char *closeIndex = NULL;
  Resource t1 = r;
  Resource t2;
  int32_t indexR = 0;
  UResType type = RES_GET_TYPE(t1);

  /* if you come in with an empty path, you'll be getting back the same resource */
  if(!uprv_strlen(pathP)) {
      return r;
  }

  /* one needs to have an aggregate resource in order to search in it */
  if(!(type == URES_TABLE || type == URES_TABLE32 || type == URES_ARRAY)) {
      return RES_BOGUS;
  }
  
  while(nextSepP && *pathP && t1 != RES_BOGUS &&
        (type == URES_TABLE || type == URES_TABLE32 || type == URES_ARRAY)
  ) {
    /* Iteration stops if: the path has been consumed, we found a non-existing
     * resource (t1 == RES_BOGUS) or we found a scalar resource (including alias)
     */
    nextSepP = uprv_strchr(pathP, RES_PATH_SEPARATOR);
    /* if there are more separators, terminate string 
     * and set path to the remaining part of the string
     */
    if(nextSepP != NULL) {
      *nextSepP = 0; /* overwrite the separator with a NUL to terminate the key */
      *path = nextSepP+1;
    } else {
      *path = uprv_strchr(pathP, 0);
    }

    /* if the resource is a table */
    /* try the key based access */
    if(type == URES_TABLE) {
      t2 = _res_findTableItem(pResData->pRoot, t1, pathP, &indexR, key);
      if(t2 == RES_BOGUS) { 
        /* if we fail to get the resource by key, maybe we got an index */
        indexR = uprv_strtol(pathP, &closeIndex, 10);
        if(closeIndex != pathP) {
          /* if we indeed have an index, try to get the item by index */
          t2 = res_getTableItemByIndex(pResData, t1, indexR, key);
        }
      }
    } else if(type == URES_TABLE32) {
      t2 = _res_findTable32Item(pResData->pRoot, t1, pathP, &indexR, key);
      if(t2 == RES_BOGUS) { 
        /* if we fail to get the resource by key, maybe we got an index */
        indexR = uprv_strtol(pathP, &closeIndex, 10);
        if(closeIndex != pathP) {
          /* if we indeed have an index, try to get the item by index */
          t2 = res_getTableItemByIndex(pResData, t1, indexR, key);
        }
      }
    } else if(type == URES_ARRAY) {
      indexR = uprv_strtol(pathP, &closeIndex, 10);
      if(closeIndex != pathP) {
        t2 = _res_getArrayItem(pResData->pRoot, t1, indexR);
      } else {
        t2 = RES_BOGUS; /* have an array, but don't have a valid index */
      }
      *key = NULL;
    } else { /* can't do much here, except setting t2 to bogus */
      t2 = RES_BOGUS;
    }
    t1 = t2;
    type = RES_GET_TYPE(t1);
    /* position pathP to next resource key/index */
    pathP = *path;
  }

  return t1;
}

U_CFUNC Resource 
res_getTableItemByKey(const ResourceData *pResData, Resource table,
                      int32_t *indexR, const char **key ){
    if(key != NULL && *key != NULL) {
        if(RES_GET_TYPE(table)==URES_TABLE) {
            return _res_findTableItem(pResData->pRoot, table, *key, indexR, key);
        } else {
            return _res_findTable32Item(pResData->pRoot, table, *key, indexR, key);
        }
    } else {
        return RES_BOGUS;
    }
}

U_CFUNC Resource 
res_getTableItemByIndex(const ResourceData *pResData, Resource table,
                        int32_t indexR, const char **key) {
    if(indexR>-1) {
        if(RES_GET_TYPE(table)==URES_TABLE) {
            if(key != NULL) {
                *key = _res_getTableKey(pResData->pRoot, table, indexR);
            }
            return _res_getTableItem(pResData->pRoot, table, indexR);
        } else {
            if(key != NULL) {
                *key = _res_getTable32Key(pResData->pRoot, table, indexR);
            }
            return _res_getTable32Item(pResData->pRoot, table, indexR);
        }
    } else {
        return RES_BOGUS;
    }
}

/* resource bundle swapping ------------------------------------------------- */

/*
 * Need to always enumerate the entire item tree,
 * track the lowest address of any item to use as the limit for char keys[],
 * track the highest address of any item to return the size of the data.
 *
 * We should have thought of storing those in the data...
 * It is possible to extend the data structure by putting additional values
 * in places that are inaccessible by ordinary enumeration of the item tree.
 * For example, additional integers could be stored at the beginning or
 * end of the key strings; this could be indicated by a minor version number,
 * and the data swapping would have to know about these values.
 *
 * The data structure does not forbid keys to be shared, so we must swap
 * all keys once instead of each key when it is referenced.
 *
 * These swapping functions assume that a resource bundle always has a length
 * that is a multiple of 4 bytes.
 * Currently, this is trivially true because genrb writes bundle tree leaves
 * physically first, before their branches, so that the root table with its
 * array of resource items (uint32_t values) is always last.
 */

/* definitions for table sorting ------------------------ */

/*
 * row of a temporary array
 *
 * gets platform-endian key string indexes and sorting indexes;
 * after sorting this array by keys, the actual key/value arrays are permutated
 * according to the sorting indexes
 */
typedef struct Row {
    int32_t keyIndex, sortIndex;
} Row;

static int32_t
ures_compareRows(const void *context, const void *left, const void *right) {
    const char *keyChars=(const char *)context;
    return (int32_t)uprv_strcmp(keyChars+((const Row *)left)->keyIndex,
                                keyChars+((const Row *)right)->keyIndex);
}

typedef struct TempTable {
    const char *keyChars;
    Row *rows;
    int32_t *resort;
} TempTable;

enum {
    STACK_ROW_CAPACITY=200
};

/* binary data with known formats is swapped too */
typedef enum UResSpecialType {
    URES_NO_SPECIAL_TYPE,
    URES_COLLATION_BINARY,
    URES_SPECIAL_TYPE_COUNT
} UResSpecialType;

/* resource table key for collation binaries: "%%CollationBin" */
static const UChar gCollationBinKey[]={
    0x25, 0x25,
    0x43, 0x6f, 0x6c, 0x6c, 0x61, 0x74, 0x69, 0x6f, 0x6e,
    0x42, 0x69, 0x6e,
    0
};

/*
 * preflight one resource item and set bottom and top values;
 * length, bottom, and top count Resource item offsets (4 bytes each), not bytes
 */
static void
ures_preflightResource(const UDataSwapper *ds,
                       const Resource *inBundle, int32_t length,
                       Resource res,
                       int32_t *pBottom, int32_t *pTop, int32_t *pMaxTableLength,
                       UErrorCode *pErrorCode) {
    const Resource *p;
    int32_t offset;

    if(res==0 || RES_GET_TYPE(res)==URES_INT) {
        /* empty string or integer, nothing to do */
        return;
    }

    /* all other types use an offset to point to their data */
    offset=(int32_t)RES_GET_OFFSET(res);
    if(0<=length && length<=offset) {
        udata_printError(ds, "ures_preflightResource(res=%08x) resource offset exceeds bundle length %d\n",
                         res, length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    } else if(offset<*pBottom) {
        *pBottom=offset;
    }
    p=inBundle+offset;

    switch(RES_GET_TYPE(res)) {
    case URES_ALIAS:
        /* physically same value layout as string, fall through */
    case URES_STRING:
        /* top=offset+1+(string length +1)/2 rounded up */
        offset+=1+((udata_readInt32(ds, (int32_t)*p)+1)+1)/2;
        break;
    case URES_BINARY:
        /* top=offset+1+(binary length)/4 rounded up */
        offset+=1+(udata_readInt32(ds, (int32_t)*p)+3)/4;
        break;
    case URES_TABLE:
    case URES_TABLE32:
        {
            Resource item;
            int32_t i, count;

            if(RES_GET_TYPE(res)==URES_TABLE) {
                /* get table item count */
                const uint16_t *pKey16=(const uint16_t *)p;
                count=ds->readUInt16(*pKey16++);

                /* top=((1+ table item count)/2 rounded up)+(table item count) */
                offset+=((1+count)+1)/2;
            } else {
                /* get table item count */
                const int32_t *pKey32=(const int32_t *)p;
                count=udata_readInt32(ds, *pKey32++);

                /* top=(1+ table item count)+(table item count) */
                offset+=1+count;
            }

            if(count>*pMaxTableLength) {
                *pMaxTableLength=count;
            }

            p=inBundle+offset; /* pointer to table resources */
            offset+=count;

            /* recurse */
            if(offset<=length) {
                for(i=0; i<count; ++i) {
                    item=ds->readUInt32(*p++);
                    ures_preflightResource(ds, inBundle, length, item,
                                           pBottom, pTop, pMaxTableLength,
                                           pErrorCode);
                    if(U_FAILURE(*pErrorCode)) {
                        udata_printError(ds, "ures_preflightResource(table res=%08x)[%d].recurse(%08x) failed\n",
                                         res, i, item);
                        break;
                    }
                }
            }
        }
        break;
    case URES_ARRAY:
        {
            Resource item;
            int32_t i, count;

            /* top=offset+1+(array length) */
            count=udata_readInt32(ds, (int32_t)*p++);
            offset+=1+count;

            /* recurse */
            if(offset<=length) {
                for(i=0; i<count; ++i) {
                    item=ds->readUInt32(*p++);
                    ures_preflightResource(ds, inBundle, length, item,
                                           pBottom, pTop, pMaxTableLength,
                                           pErrorCode);
                    if(U_FAILURE(*pErrorCode)) {
                        udata_printError(ds, "ures_preflightResource(array res=%08x)[%d].recurse(%08x) failed\n",
                                         res, i, item);
                        break;
                    }
                }
            }
        }
        break;
    case URES_INT_VECTOR:
        /* top=offset+1+(vector length) */
        offset+=1+udata_readInt32(ds, (int32_t)*p);
        break;
    default:
        /* also catches RES_BOGUS */
        udata_printError(ds, "ures_preflightResource(res=%08x) unknown resource type\n", res);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        break;
    }

    if(U_FAILURE(*pErrorCode)) {
        /* nothing to do */
    } else if(0<=length && length<offset) {
        udata_printError(ds, "ures_preflightResource(res=%08x) resource limit exceeds bundle length %d\n",
                         res, length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    } else if(offset>*pTop) {
        *pTop=offset;
    }
}

/*
 * swap one resource item
 * since preflighting succeeded, we need not check offsets against length any more
 */
static void
ures_swapResource(const UDataSwapper *ds,
                  const Resource *inBundle, Resource *outBundle,
                  Resource res, /* caller swaps res itself */
                  UResSpecialType specialType,
                  TempTable *pTempTable,
                  UErrorCode *pErrorCode) {
    const Resource *p;
    Resource *q;
    int32_t offset, count;

    if(res==0 || RES_GET_TYPE(res)==URES_INT) {
        /* empty string or integer, nothing to do */
        return;
    }

    /* all other types use an offset to point to their data */
    offset=(int32_t)RES_GET_OFFSET(res);
    p=inBundle+offset;
    q=outBundle+offset;

    switch(RES_GET_TYPE(res)) {
    case URES_ALIAS:
        /* physically same value layout as string, fall through */
    case URES_STRING:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length */
        ds->swapArray32(ds, p, 4, q, pErrorCode);
        /* swap each UChar (the terminating NUL would not change) */
        ds->swapArray16(ds, p+1, 2*count, q+1, pErrorCode);
        break;
    case URES_BINARY:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length */
        ds->swapArray32(ds, p, 4, q, pErrorCode);
        /* no need to swap or copy bytes - ures_swap() copied them all */

        /* swap known formats */
        if(specialType==URES_COLLATION_BINARY) {
#if !UCONFIG_NO_COLLATION
            ucol_swapBinary(ds, p+1, count, q+1, pErrorCode);
#endif
        }
        break;
    case URES_TABLE:
    case URES_TABLE32:
        {
            const uint16_t *pKey16;
            uint16_t *qKey16;

            const int32_t *pKey32;
            int32_t *qKey32;

            Resource item;
            int32_t i, oldIndex;

            if(RES_GET_TYPE(res)==URES_TABLE) {
                /* get table item count */
                pKey16=(const uint16_t *)p;
                qKey16=(uint16_t *)q;
                count=ds->readUInt16(*pKey16);

                pKey32=qKey32=NULL;

                /* swap count */
                ds->swapArray16(ds, pKey16++, 2, qKey16++, pErrorCode);

                offset+=((1+count)+1)/2;
            } else {
                /* get table item count */
                pKey32=(const int32_t *)p;
                qKey32=(int32_t *)q;
                count=udata_readInt32(ds, *pKey32);

                pKey16=qKey16=NULL;

                /* swap count */
                ds->swapArray32(ds, pKey32++, 4, qKey32++, pErrorCode);

                offset+=1+count;
            }

            if(count==0) {
                break;
            }

            p=inBundle+offset; /* pointer to table resources */
            q=outBundle+offset;

            /* recurse */
            for(i=0; i<count; ++i) {
                /*
                 * detect a collation binary that is to be swapped via
                 * ds->compareInvChars(ds, outData+readUInt16(pKey[i]), "%%CollationBin")
                 * etc.
                 *
                 * use some UDataSwapFn pointer from somewhere for collation swapping
                 * because the common library cannot directly call into the i18n library
                 */
                if(0==ds->compareInvChars(ds,
                            ((const char *)outBundle)+
                                (pKey16!=NULL ?
                                    ds->readUInt16(pKey16[i]) :
                                    udata_readInt32(ds, pKey32[i])),
                            -1,
                            gCollationBinKey, LENGTHOF(gCollationBinKey)-1)
                ) {
                    specialType=URES_COLLATION_BINARY;
                } else {
                    specialType=URES_NO_SPECIAL_TYPE;
                }

                item=ds->readUInt32(p[i]);
                ures_swapResource(ds, inBundle, outBundle, item, specialType, pTempTable, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "ures_swapResource(table res=%08x)[%d].recurse(%08x) failed\n",
                                     res, i, item);
                    return;
                }
            }

            if(ds->inCharset==ds->outCharset) {
                /* no need to sort, just swap the offset/value arrays */
                if(pKey16!=NULL) {
                    ds->swapArray16(ds, pKey16, count*2, qKey16, pErrorCode);
                    ds->swapArray32(ds, p, count*4, q, pErrorCode);
                } else {
                    /* swap key offsets and items as one array */
                    ds->swapArray32(ds, pKey32, count*2*4, qKey32, pErrorCode);
                }
                break;
            }

            /*
             * We need to sort tables by outCharset key strings because they
             * sort differently for different charset families.
             * ures_swap() already set pTempTable->keyChars appropriately.
             * First we set up a temporary table with the key indexes and
             * sorting indexes and sort that.
             * Then we permutate and copy/swap the actual values.
             */
            if(pKey16!=NULL) {
                for(i=0; i<count; ++i) {
                    pTempTable->rows[i].keyIndex=ds->readUInt16(pKey16[i]);
                    pTempTable->rows[i].sortIndex=i;
                }
            } else {
                for(i=0; i<count; ++i) {
                    pTempTable->rows[i].keyIndex=udata_readInt32(ds, pKey32[i]);
                    pTempTable->rows[i].sortIndex=i;
                }
            }
            uprv_sortArray(pTempTable->rows, count, sizeof(Row),
                           ures_compareRows, pTempTable->keyChars,
                           FALSE, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "ures_swapResource(table res=%08x).uprv_sortArray(%d items) failed\n",
                                 res, count);
                return;
            }

            /*
             * copy/swap/permutate items
             *
             * If we swap in-place, then the permutation must use another
             * temporary array (pTempTable->resort)
             * before the results are copied to the outBundle.
             */
            /* keys */
            if(pKey16!=NULL) {
                uint16_t *rKey16;

                if(pKey16!=qKey16) {
                    rKey16=qKey16;
                } else {
                    rKey16=(uint16_t *)pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray16(ds, pKey16+oldIndex, 2, rKey16+i, pErrorCode);
                }
                if(qKey16!=rKey16) {
                    uprv_memcpy(qKey16, rKey16, 2*count);
                }
            } else {
                int32_t *rKey32;

                if(pKey32!=qKey32) {
                    rKey32=qKey32;
                } else {
                    rKey32=pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray32(ds, pKey32+oldIndex, 4, rKey32+i, pErrorCode);
                }
                if(qKey32!=rKey32) {
                    uprv_memcpy(qKey32, rKey32, 4*count);
                }
            }

            /* resources */
            {
                Resource *r;


                if(p!=q) {
                    r=q;
                } else {
                    r=(Resource *)pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray32(ds, p+oldIndex, 4, r+i, pErrorCode);
                }
                if(q!=r) {
                    uprv_memcpy(q, r, 4*count);
                }
            }
        }
        break;
    case URES_ARRAY:
        {
            Resource item;
            int32_t i;

            count=udata_readInt32(ds, (int32_t)*p);
            /* swap length */
            ds->swapArray32(ds, p++, 4, q++, pErrorCode);

            /* recurse */
            for(i=0; i<count; ++i) {
                item=ds->readUInt32(p[i]);
                ures_swapResource(ds, inBundle, outBundle, item, URES_NO_SPECIAL_TYPE, pTempTable, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "ures_swapResource(array res=%08x)[%d].recurse(%08x) failed\n",
                                     res, i, item);
                    return;
                }
            }

            /* swap items */
            ds->swapArray32(ds, p, 4*count, q, pErrorCode);
        }
        break;
    case URES_INT_VECTOR:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length and each integer */
        ds->swapArray32(ds, p, 4*(1+count), q, pErrorCode);
        break;
    default:
        /* also catches RES_BOGUS */
        *pErrorCode=U_UNSUPPORTED_ERROR;
        break;
    }
}

U_CAPI int32_t U_EXPORT2
ures_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    const Resource *inBundle;
    Resource rootRes;
    int32_t headerSize, maxTableLength;

    Row rows[STACK_ROW_CAPACITY];
    int32_t resort[STACK_ROW_CAPACITY];
    TempTable tempTable;

    /* the following integers count Resource item offsets (4 bytes each), not bytes */
    int32_t bundleLength, stringsBottom, bottom, top;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x52 &&   /* dataFormat="ResB" */
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x42 &&
        pInfo->formatVersion[0]==1
    )) {
        udata_printError(ds, "ures_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not a resource bundle\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    /* a resource bundle must contain at least one resource item */
    if(length<0) {
        bundleLength=-1;
    } else {
        bundleLength=(length-headerSize)/4;

        /* formatVersion 1.1 must have a root item and at least 5 indexes */
        if( bundleLength<
                (pInfo->formatVersion[1]==0 ? 1 : 1+5)
        ) {
            udata_printError(ds, "ures_swap(): too few bytes (%d after header) for a resource bundle\n",
                             length-headerSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    inBundle=(const Resource *)((const char *)inData+headerSize);
    rootRes=ds->readUInt32(*inBundle);

    if(pInfo->formatVersion[1]==0) {
        /* preflight to get the bottom, top and maxTableLength values */
        stringsBottom=1; /* just past root */
        bottom=0x7fffffff;
        top=maxTableLength=0;
        ures_preflightResource(ds, inBundle, bundleLength, rootRes,
                               &bottom, &top, &maxTableLength,
                               pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ures_preflightResource(root res=%08x) failed\n",
                             rootRes);
            return 0;
        }
    } else {
        /* formatVersion 1.1 adds the indexes[] array */
        const int32_t *inIndexes;

        inIndexes=(const int32_t *)(inBundle+1);

        stringsBottom=1+udata_readInt32(ds, inIndexes[URES_INDEX_LENGTH]);
        bottom=udata_readInt32(ds, inIndexes[URES_INDEX_STRINGS_TOP]);
        top=udata_readInt32(ds, inIndexes[URES_INDEX_BUNDLE_TOP]);
        maxTableLength=udata_readInt32(ds, inIndexes[URES_INDEX_MAX_TABLE_LENGTH]);

        if(0<=bundleLength && bundleLength<top) {
            udata_printError(ds, "ures_swap(): resource top %d exceeds bundle length %d\n",
                             top, bundleLength);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    if(length>=0) {
        Resource *outBundle=(Resource *)((char *)outData+headerSize);

        /* copy the bundle for binary and inaccessible data */
        if(inData!=outData) {
            uprv_memcpy(outBundle, inBundle, 4*top);
        }

        /* swap the key strings, but not the padding bytes (0xaa) after the last string and its NUL */
        udata_swapInvStringBlock(ds, inBundle+stringsBottom, 4*(bottom-stringsBottom),
                                    outBundle+stringsBottom, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ures_swap().udata_swapInvStringBlock(keys[%d]) failed\n", 4*(bottom-1));
            return 0;
        }

        /* allocate the temporary table for sorting resource tables */
        tempTable.keyChars=(const char *)outBundle; /* sort by outCharset */
        if(maxTableLength<=STACK_ROW_CAPACITY) {
            tempTable.rows=rows;
            tempTable.resort=resort;
        } else {
            tempTable.rows=(Row *)uprv_malloc(maxTableLength*sizeof(Row)+maxTableLength*4);
            if(tempTable.rows==NULL) {
                udata_printError(ds, "ures_swap(): unable to allocate memory for sorting tables (max length: %d)\n",
                                 maxTableLength);
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
            tempTable.resort=(int32_t *)(tempTable.rows+maxTableLength);
        }

        /* swap the resources */
        ures_swapResource(ds, inBundle, outBundle, rootRes, URES_NO_SPECIAL_TYPE, &tempTable, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ures_swapResource(root res=%08x) failed\n",
                             rootRes);
        }

        if(tempTable.rows!=rows) {
            uprv_free(tempTable.rows);
        }

        /* swap the root resource and indexes */
        ds->swapArray32(ds, inBundle, stringsBottom*4, outBundle, pErrorCode);
    }

    return headerSize+4*top;
}
