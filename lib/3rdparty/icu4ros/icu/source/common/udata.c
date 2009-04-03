/*
******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  udata.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999oct25
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "unicode/udata.h"
#include "unicode/uversion.h"
#include "uhash.h"
#include "ucln_cmn.h"
#include "putilimp.h"

#include "udatamem.h"
#include "umapfile.h"
#include "ucmndata.h"

/***********************************************************************
*
*   Notes on the organization of the ICU data implementation
*
*      All of the public API is defined in udata.h
*
*      The implementation is split into several files...
*
*         - udata.c  (this file) contains higher level code that knows about
*                     the search paths for locating data, caching opened data, etc.
*
*         - umapfile.c  contains the low level platform-specific code for actually loading
*                     (memory mapping, file reading, whatever) data into memory.
*
*         - ucmndata.c  deals with the tables of contents of ICU data items within
*                     an ICU common format data file.  The implementation includes
*                     an abstract interface and support for multiple TOC formats.
*                     All knowledge of any specific TOC format is encapsulated here.
*
*         - udatamem.c has code for managing UDataMemory structs.  These are little
*                     descriptor objects for blocks of memory holding ICU data of
*                     various types.
*/

/* configuration ---------------------------------------------------------- */

/* If you are excruciatingly bored turn this on .. */
/* #define UDATA_DEBUG 1 */

#if defined(UDATA_DEBUG)
#   include <stdio.h>
#endif


/***********************************************************************
*
*    static (Global) data
*
************************************************************************/
static UDataMemory *gCommonICUData = NULL;    /* Pointer to the common ICU data.           */
                                              /*   May be updated once, if we started with */
                                              /*   a stub or subset library.               */

static UDataMemory *gStubICUData   = NULL;    /* If gCommonICUData does get updated, remember */
                                              /*   the original one so that it can be cleaned */
                                              /*   up when ICU is shut down.                  */

static UHashtable  *gCommonDataCache = NULL;  /* Global hash table of opened ICU data files.  */

static UDataFileAccess  gDataFileAccess = UDATA_DEFAULT_ACCESS;

static UBool U_CALLCONV
udata_cleanup(void)
{
    if (gCommonDataCache) {             /* Delete the cache of user data mappings.  */
        uhash_close(gCommonDataCache);  /*   Table owns the contents, and will delete them. */
        gCommonDataCache = NULL;        /*   Cleanup is not thread safe.                */
    }

    if (gCommonICUData != NULL) {
        udata_close(gCommonICUData);    /* Clean up common ICU Data             */
        gCommonICUData = NULL;
    }

    if (gStubICUData != NULL) {
        udata_close(gStubICUData);      /* Clean up the stub ICU Data             */
        gStubICUData = NULL;
    }


    return TRUE;                   /* Everything was cleaned up */
}




/*
 * setCommonICUData.   Set a UDataMemory to be the global ICU Data
 */
static void
setCommonICUData(UDataMemory *pData,     /*  The new common data.  Belongs to caller, we copy it. */
                 UDataMemory *oldData,   /*  Old ICUData ptr.  Overwrite of this value is ok,     */
                                         /*     of any others is not.                             */
                 UBool       warn,       /*  If true, set USING_DEFAULT warning if ICUData was    */
                                         /*    changed by another thread before we got to it.     */
                 UErrorCode *pErr)
{
    UDataMemory  *newCommonData = UDataMemory_createNewInstance(pErr);
    if (U_FAILURE(*pErr)) {
        return;
    }

    /*  For the assignment, other threads must cleanly see either the old            */
    /*    or the new, not some partially initialized new.  The old can not be        */
    /*    deleted - someone may still have a pointer to it lying around in           */
    /*    their locals.                                                              */
    UDatamemory_assign(newCommonData, pData);
    umtx_lock(NULL);
    if (gCommonICUData==oldData) {
        gStubICUData   = gCommonICUData;   /* remember the old Common Data, so it can be cleaned up. */
        gCommonICUData = newCommonData;
        ucln_common_registerCleanup(UCLN_COMMON_UDATA, udata_cleanup);
    }
    else {
        if  (warn==TRUE) {
            *pErr = U_USING_DEFAULT_WARNING;
        }
        uprv_free(newCommonData);
    }
    umtx_unlock(NULL);
}

static const char *
findBasename(const char *path) {
    const char *basename=uprv_strrchr(path, U_FILE_SEP_CHAR);
    if(basename==NULL) {
        return path;
    } else {
        return basename+1;
    }
}

#ifdef UDATA_DEBUG
static const char *
packageNameFromPath(const char *path)
{
    if((path == NULL) || (*path == 0)) {
        return U_ICUDATA_NAME;
    }

    path = findBasename(path);

    if((path == NULL) || (*path == 0)) {
        return U_ICUDATA_NAME;
    }

    return path;
}
#endif

/*----------------------------------------------------------------------*
 *                                                                      *
 *   Cache for common data                                              *
 *      Functions for looking up or adding entries to a cache of        *
 *      data that has been previously opened.  Avoids a potentially     *
 *      expensive operation of re-opening the data for subsequent       *
 *      uses.                                                           *
 *                                                                      *
 *      Data remains cached for the duration of the process.            *
 *                                                                      *
 *----------------------------------------------------------------------*/

typedef struct DataCacheElement {
    char          *name;
    UDataMemory   *item;
} DataCacheElement;



/*
 * Deleter function for DataCacheElements.
 *         udata cleanup function closes the hash table; hash table in turn calls back to
 *         here for each entry.
 */
static void U_CALLCONV DataCacheElement_deleter(void *pDCEl) {
    DataCacheElement *p = (DataCacheElement *)pDCEl;
    udata_close(p->item);              /* unmaps storage */
    uprv_free(p->name);                /* delete the hash key string. */
    uprv_free(pDCEl);                  /* delete 'this'          */
}

 /*   udata_getCacheHashTable()
 *     Get the hash table used to store the data cache entries.
 *     Lazy create it if it doesn't yet exist.
 */
static UHashtable *udata_getHashTable() {
    UErrorCode   err = U_ZERO_ERROR;
    UBool        cacheIsInitialized;
    UHashtable  *tHT = NULL;

    UMTX_CHECK(NULL, (gCommonDataCache != NULL), cacheIsInitialized);

    if (cacheIsInitialized) {
        return gCommonDataCache;
    }

    tHT = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &err);
    uhash_setValueDeleter(tHT, DataCacheElement_deleter);

    umtx_lock(NULL);
    if (gCommonDataCache == NULL) {
        gCommonDataCache = tHT;
        tHT = NULL;
        ucln_common_registerCleanup(UCLN_COMMON_UDATA, udata_cleanup);
    }
    umtx_unlock(NULL);
    if (tHT != NULL) {
        uhash_close(tHT);
    }

    if (U_FAILURE(err)) {
        return NULL;      /* TODO:  handle this error better.  */
    }
    return gCommonDataCache;
}



static UDataMemory *udata_findCachedData(const char *path)
{
    UHashtable        *htable;
    UDataMemory       *retVal = NULL;
    DataCacheElement  *el;
    const char        *baseName;

    baseName = findBasename(path);   /* Cache remembers only the base name, not the full path. */
    htable = udata_getHashTable();
    umtx_lock(NULL);
    el = (DataCacheElement *)uhash_get(htable, baseName);
    umtx_unlock(NULL);
    if (el != NULL) {
        retVal = el->item;
    }
#ifdef UDATA_DEBUG
    fprintf(stderr, "Cache: [%s] -> %p\n", baseName, retVal);
#endif
    return retVal;
}


static UDataMemory *udata_cacheDataItem(const char *path, UDataMemory *item, UErrorCode *pErr) {
    DataCacheElement *newElement;
    const char       *baseName;
    int32_t           nameLen;
    UHashtable       *htable;
    UDataMemory      *oldValue = NULL;
    UErrorCode        subErr = U_ZERO_ERROR;

    if (U_FAILURE(*pErr)) {
        return NULL;
    }

    /* Create a new DataCacheElement - the thingy we store in the hash table -
     * and copy the supplied path and UDataMemoryItems into it.
     */
    newElement = uprv_malloc(sizeof(DataCacheElement));
    if (newElement == NULL) {
        *pErr = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    newElement->item = UDataMemory_createNewInstance(pErr);
    if (U_FAILURE(*pErr)) {
        uprv_free(newElement);
        return NULL;
    }
    UDatamemory_assign(newElement->item, item);

    baseName = findBasename(path);
    nameLen = (int32_t)uprv_strlen(baseName);
    newElement->name = uprv_malloc(nameLen+1);
    if (newElement->name == NULL) {
        *pErr = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(newElement->item);
        uprv_free(newElement);
        return NULL;
    }
    uprv_strcpy(newElement->name, baseName);

    /* Stick the new DataCacheElement into the hash table.
    */
    htable = udata_getHashTable();
    umtx_lock(NULL);
    oldValue = uhash_get(htable, path);
    if (oldValue != NULL) {
        subErr = U_USING_DEFAULT_WARNING;
    }
    else {
        uhash_put(
            htable,
            newElement->name,               /* Key   */
            newElement,                     /* Value */
            &subErr);
    }
    umtx_unlock(NULL);

#ifdef UDATA_DEBUG
    fprintf(stderr, "Cache: [%s] <<< %p : %s. vFunc=%p\n", newElement->name, 
    newElement->item, u_errorName(subErr), newElement->item->vFuncs);
#endif

    if (subErr == U_USING_DEFAULT_WARNING || U_FAILURE(subErr)) {
        *pErr = subErr; /* copy sub err unto fillin ONLY if something happens. */
        uprv_free(newElement->name);
        uprv_free(newElement->item);
        uprv_free(newElement);
        return oldValue;
    }

    return newElement->item;
}



/*-------------------------------------------------------------------------------
 *
 *   TinyString   -  a small set of really simple string functions, for 
 *                   the purpose of consolidating buffer overflow code in one place
 *
 *                   Use wherever you would otherwise declare a fixed sized  char[xx] buffer.
 *                   Do non-growing ops by accessing fields of struct directly
 *                   Grow using the append function to automatically extend buffer
 *                   as needed.
 *
 *-------------------------------------------------------------------------------*/
typedef struct TinyString {
    char      *s;
    int32_t    length;
    char       fStaticBuf[100];
    int32_t    fCapacity;
} TinyString;

static void TinyString_init(TinyString *This) {
    This->s = This->fStaticBuf;
    *This->s = 0;
    This->length = 0;
    This->fCapacity = sizeof(This->fStaticBuf)-1;
}

static void TinyString_append(TinyString *This, const char *what) {
    int32_t  newLen;
    newLen = This->length + (int32_t)uprv_strlen(what); 
    if (newLen >= This->fCapacity) { 
        int32_t newCapacity = newLen * 2; 
        char *newBuf = (char *)uprv_malloc(newCapacity+1); 
        if (newBuf != NULL) { 
            uprv_strcpy(newBuf, This->s); 
            if (This->s != This->fStaticBuf) { 
                uprv_free(This->s);
            } 
            This->s = newBuf; 
            This->fCapacity = newCapacity; 
        } 
    }
    if (newLen < This->fCapacity) { 
        uprv_strcat(This->s+This->length, what);
        This->length = newLen;
    } 
}

static void TinyString_appendn(TinyString *This, const char *what, int32_t n) {
    int32_t  newLen;
    newLen = This->length + n;
    if (newLen >= This->fCapacity) { 
        int32_t newCapacity = newLen * 2; 
        char *newBuf = (char *)uprv_malloc(newCapacity+1); 
        if (newBuf != NULL) { 
            uprv_strcpy(newBuf, This->s); 
            if (This->s != This->fStaticBuf) { 
                uprv_free(This->s);
            } 
            This->s = newBuf; 
            This->fCapacity = newCapacity; 
        } 
    }
    if (newLen < This->fCapacity) { 
        uprv_strncat(This->s+This->length, what, n);
        This->length = newLen;
    } 
}
    
static void TinyString_dt(TinyString *This) {
    if (This->s != This->fStaticBuf) { 
        uprv_free(This->s); 
    }
    TinyString_init(This);
}




/*----------------------------------------------------------------------*==============
 *                                                                      *
 *  Path management.  Could be shared with other tools/etc if need be   *
 * later on.                                                            *
 *                                                                      *
 *----------------------------------------------------------------------*/

#define U_DATA_PATHITER_BUFSIZ  128        /* Size of local buffer for paths         */
                                           /*   Overflow causes malloc of larger buf */

typedef struct 
{
    const char *path;                              /* working path (u_icudata_Dir) */
    const char *nextPath;                          /* path following this one */
    const char *basename;                          /* item's basename (icudt22e_mt.res)*/
    const char *suffix;                            /* item suffix (can be null) */

    uint32_t    basenameLen;                       /* length of basename */

    char       *itemPath;                          /* path passed in with item name */
    char        itemPathBuf[U_DATA_PATHITER_BUFSIZ];

    char       *pathBuffer;                        /* output path for this it'ion */
    char        pathBufferA[U_DATA_PATHITER_BUFSIZ];

    char       *packageStub;                       /* example:  "/icudt28b". Will ignore that leaf in set paths. */
    char        packageStubBuf[U_DATA_PATHITER_BUFSIZ];
    uint32_t    packageStubLen;

    UBool       checkLastFour;                     /* if TRUE then allow paths such as '/foo/myapp.dat'
                                                    * to match, checks last 4 chars of suffix with
                                                    * last 4 of path, then previous chars. */
    
}  UDataPathIterator;

/** 
 * Initialize (or re-initialize) a user-supplied UDataPathIterator
 * Note: UDataPathIterator does not allocate storage, so it doesn't need to be closed.
 * 
 * @param iter  The iterator to be initialized. Its current state does not matter. 
 * @param path  The full pathname to be iterated over.  If NULL, defaults to U_ICUDATA_NAME 
 * @param pkg   Package which is being searched for, ex "icudt28l".  Will ignore leave directories such as /icudt28l 
 * @param item  Item to be searched for.  Can include full path, such as /a/b/foo.dat 
 * @param suffix  Optional item suffix, if not-null (ex. ".dat") then 'path' can contain 'item' explicitly.
 *               Ex:   'stuff.dat' would be found in '/a/foo:/tmp/stuff.dat:/bar/baz' as item #2.   
 *                     '/blarg/stuff.dat' would also be found.
 */
static void udata_pathiter_init(UDataPathIterator *iter, const char *path, const char *pkg,
                                const char *item, const char *suffix, UBool doCheckLastFour)
{
#ifdef UDATA_DEBUG
        fprintf(stderr, "SUFFIX1=%s PATH=%s\n", suffix, path);
#endif
    /** Path **/
    if(path == NULL) {
        iter->path = u_getDataDirectory();
    } else {
        iter->path = path;
    }

    /** Package **/
    if(pkg == NULL) {
      iter->packageStubLen = 0;
      iter->packageStub=iter->packageStubBuf;
      iter->packageStub[0] = 0;
    } else {
      if(uprv_strlen(pkg) + 2 > U_DATA_PATHITER_BUFSIZ) {
          iter->packageStub = uprv_malloc(uprv_strlen(pkg)+2);
      } else {
          iter->packageStub = iter->packageStubBuf;
      }
      iter->packageStub[0] = U_FILE_SEP_CHAR;
      uprv_strcpy(iter->packageStub+1, pkg);
      iter->packageStubLen = (int32_t)uprv_strlen(iter->packageStub);

#ifdef UDATA_DEBUG
        fprintf(stderr, "STUB=%s [%d]\n", iter->packageStub, iter->packageStubLen);
#endif
    }

    /** Item **/
    iter->basename = findBasename(item);
    iter->basenameLen = (int32_t)uprv_strlen(iter->basename);

    /** Item path **/
    iter->itemPath   = iter->itemPathBuf;
    if(iter->basename == item) {
        iter->itemPath[0] = 0;
        iter->nextPath = iter->path;
    } else { 
        int32_t  itemPathLen = (int32_t)(iter->basename-item);
        if (itemPathLen >= U_DATA_PATHITER_BUFSIZ) {
            char *t = (char *)uprv_malloc(itemPathLen+1);
            if (t != NULL) {
                iter->itemPath = t;
            } else {
                /* Malloc failed.  Ignore the itemPath. */
                itemPathLen = 0;
            }
        }
        uprv_strncpy(iter->itemPath, item, itemPathLen);
        iter->itemPath[itemPathLen]=0;
        iter->nextPath = iter->itemPath;
    }
#ifdef UDATA_DEBUG
    fprintf(stderr, "SUFFIX=%s [%p]\n", suffix, suffix);
#endif
    
    /** Suffix  **/
    if(suffix != NULL) {
        iter->suffix = suffix;
    } else {
        iter->suffix = "";
    }
    
    iter->checkLastFour = doCheckLastFour;
    
    /* pathBuffer will hold the output path strings returned by the this iterator
     *   Get an upper bound of possible string size, and make sure that the buffer
     *   is big enough (sum of length of each piece, 2 extra delimiters, + trailing NULL) */
    {
        int32_t  maxPathLen = (int32_t)(uprv_strlen(iter->path) + uprv_strlen(item) + uprv_strlen(iter->suffix) + iter->packageStubLen + 3);
        iter->pathBuffer = iter->pathBufferA;
        if (maxPathLen >= U_DATA_PATHITER_BUFSIZ) {
            iter->pathBuffer = (char *)uprv_malloc(maxPathLen);
            if (iter->pathBuffer == NULL) {
                iter->pathBuffer = iter->pathBufferA;
                iter->path = "";
            }
        }
    }

#ifdef UDATA_DEBUG
    fprintf(stderr, "%p: init %s -> [path=%s], [base=%s], [suff=%s], [itempath=%s], [nextpath=%s], [checklast4=%s]\n",
            iter,
            item,
            iter->path,
            iter->basename,
            iter->suffix,
            iter->itemPath,
            iter->nextPath,
            iter->checkLastFour?"TRUE":"false");
#endif

}

/**
 * Get the next path on the list.
 *
 * @param iter The Iter to be used 
 * @param len  If set, pointer to the length of the returned path, for convenience. 
 * @return Pointer to the next path segment, or NULL if there are no more.
 */
static const char *udata_pathiter_next(UDataPathIterator *iter)
{
    const char *path = NULL;
    uint32_t     pathLen = 0;
    const char *pathBasename;

    do
    {
        if( iter->nextPath == NULL ) {
            break;
        }
        
        path = iter->nextPath;
        
        if(iter->nextPath == iter->itemPath) { /* we were processing item's path. */
            iter->nextPath = iter->path; /* start with regular path next tm. */
            pathLen = (int32_t)uprv_strlen(path);
        } else {
            /* fix up next for next time */
            iter->nextPath = uprv_strchr(path, U_PATH_SEP_CHAR);
            if(iter->nextPath == NULL) {
                /* segment: entire path */
                pathLen = (int32_t)uprv_strlen(path); 
            } else {
                /* segment: until next segment */
                pathLen = (int32_t)(iter->nextPath - path);
                if(*iter->nextPath) { /* skip divider */
                    iter->nextPath ++;
                }
            }
        }

        if(pathLen == 0) {
            continue;
        }

#ifdef UDATA_DEBUG
        fprintf(stderr, "rest of path (IDD) = %s\n", path);
        fprintf(stderr, "                     ");
        { 
            uint32_t qqq;
            for(qqq=0;qqq<pathLen;qqq++)
            {
                fprintf(stderr, " ");
            }

            fprintf(stderr, "^\n");
        }
#endif
        uprv_strncpy(iter->pathBuffer, path, pathLen);
        iter->pathBuffer[pathLen] = 0;

        /* check for .dat files */
        pathBasename = findBasename(iter->pathBuffer);

        if(iter->checkLastFour == TRUE && 
           (pathLen>=4) &&
           uprv_strncmp(iter->pathBuffer +(pathLen-4),iter->suffix,4)==0 && /* suffix matches */
           uprv_strncmp(findBasename(iter->pathBuffer),iter->basename,iter->basenameLen)==0  && /* base matches */
           uprv_strlen(pathBasename)==(iter->basenameLen+4)) { /* base+suffix = full len */
            
#ifdef UDATA_DEBUG
            fprintf(stderr, "Have %s file on the path: %s\n", iter->suffix, iter->pathBuffer);
#endif
            /* do nothing */
        }
        else 
        {       /* regular dir path */
            if(iter->pathBuffer[pathLen-1] != U_FILE_SEP_CHAR) {
                if((pathLen>=4) &&
                   uprv_strncmp(iter->pathBuffer+(pathLen-4), ".dat", 4) == 0)
                {
#ifdef UDATA_DEBUG
                    fprintf(stderr, "skipping non-directory .dat file %s\n", iter->pathBuffer);
#endif
                    continue;
                }

                /* Check if it is a directory with the same name as our package */
                if(iter->packageStubLen &&  
                   (pathLen > iter->packageStubLen) &&
                   !uprv_strcmp(iter->pathBuffer + pathLen - iter->packageStubLen, iter->packageStub)) {
#ifdef UDATA_DEBUG
                  fprintf(stderr, "Found stub %s ( will add package %s of len %d)\n", iter->packageStub, iter->basename, iter->basenameLen);
#endif
                  pathLen -= iter->packageStubLen;
                }
                     
                iter->pathBuffer[pathLen++] = U_FILE_SEP_CHAR;
            } 
            
            uprv_strncpy(iter->pathBuffer + pathLen,  /* + basename */
                         iter->packageStub+1,
                         iter->packageStubLen-1);

            pathLen += iter->packageStubLen-1;

            if(*iter->suffix)  /* tack on suffix */
            {
                uprv_strcpy(iter->pathBuffer + pathLen,
                            iter->suffix);
                pathLen += (int32_t)uprv_strlen(iter->suffix);
            }
            
        }

#ifdef UDATA_DEBUG
        fprintf(stderr, " -->  %s\n", iter->pathBuffer);
#endif

        return iter->pathBuffer;

    } while(iter->path);

    /* fell way off the end */
    return NULL;
}


/*
 *   Path Iterator Destructor.  Clean up any allocated storage
 */
static void udata_pathiter_dt(UDataPathIterator *iter) {
     if (iter->itemPath != iter->itemPathBuf) {
         uprv_free(iter->itemPath);
         iter->itemPath = NULL;
     }
     if (iter->pathBuffer != iter->pathBufferA) {
         uprv_free(iter->pathBuffer);
         iter->pathBuffer = NULL;
     }
     if (iter->packageStub != iter->packageStubBuf) {
         uprv_free(iter->packageStub);
         iter->packageStub = NULL;
     }
}

/* ==================================================================================*/


/*----------------------------------------------------------------------*
 *                                                                      *
 *  Add a static reference to the common data  library                  *
 *   Unless overridden by an explicit udata_setCommonData, this will be *
 *      our common data.                                                *
 *                                                                      *
 *----------------------------------------------------------------------*/
extern  const DataHeader U_DATA_API U_ICUDATA_ENTRY_POINT;


/*----------------------------------------------------------------------*
 *                                                                      *
 *   openCommonData   Attempt to open a common format (.dat) file       *
 *                    Map it into memory (if it's not there already)    *
 *                    and return a UDataMemory object for it.           *
 *                                                                      *
 *                    If the requested data is already open and cached  *
 *                       just return the cached UDataMem object.        *
 *                                                                      *
 *----------------------------------------------------------------------*/
static UDataMemory *
openCommonData(const char *path,          /*  Path from OpenChoice?          */
               UBool isICUData,           /*  ICU Data true if path == NULL  */
               UErrorCode *pErrorCode)
{
    UDataMemory tData;
    UDataPathIterator iter;
    const char *pathBuffer;
    const char *inBasename;

    if (U_FAILURE(*pErrorCode)) {
        return NULL;
    }

    UDataMemory_init(&tData);

    /* ??????? TODO revisit this */ 
    if (isICUData) {
        /* "mini-cache" for common ICU data */
        if(gCommonICUData != NULL) {
            return gCommonICUData;
        }

        tData.pHeader = &U_ICUDATA_ENTRY_POINT;
        udata_checkCommonData(&tData, pErrorCode);
        setCommonICUData(&tData, NULL, FALSE, pErrorCode);
        return gCommonICUData;
    }


    /* request is NOT for ICU Data.  */

    /* Find the base name portion of the supplied path.   */
    /*   inBasename will be left pointing somewhere within the original path string.      */
    inBasename = findBasename(path);
#ifdef UDATA_DEBUG
    fprintf(stderr, "inBasename = %s\n", inBasename);
#endif

    if(*inBasename==0) {
        /* no basename.     This will happen if the original path was a directory name,   */
        /*    like  "a/b/c/".   (Fallback to separate files will still work.)             */
#ifdef UDATA_DEBUG
        fprintf(stderr, "ocd: no basename in %s, bailing.\n", path);
#endif
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

   /* Is the requested common data file already open and cached?                     */
   /*   Note that the cache is keyed by the base name only.  The rest of the path,   */
   /*     if any, is not considered.                                                 */
   {
        UDataMemory  *dataToReturn = udata_findCachedData(inBasename);
        if (dataToReturn != NULL) {
            return dataToReturn;
        }
    }

    /* Requested item is not in the cache.
     * Hunt it down, trying all the path locations
     */

    udata_pathiter_init(&iter, u_getDataDirectory(), inBasename, path, ".dat", TRUE);

    while((UDataMemory_isLoaded(&tData)==FALSE) && 
          (pathBuffer = udata_pathiter_next(&iter)) != NULL)
    {
#ifdef UDATA_DEBUG
        fprintf(stderr, "ocd: trying path %s - ", pathBuffer);
#endif
        uprv_mapFile(&tData, pathBuffer);
#ifdef UDATA_DEBUG
        fprintf(stderr, "%s\n", UDataMemory_isLoaded(&tData)?"LOADED":"not loaded");
#endif
    }
    udata_pathiter_dt(&iter);    /* Note:  this call may invalidate "pathBuffer" */

#if defined(OS390_STUBDATA) && defined(OS390BATCH)
    if (!UDataMemory_isLoaded(&tData)) {
        char ourPathBuffer[1024];
        /* One more chance, for extendCommonData() */
        uprv_strncpy(ourPathBuffer, path, 1019);
        ourPathBuffer[1019]=0;
        uprv_strcat(ourPathBuffer, ".dat");
        uprv_mapFile(&tData, ourPathBuffer);
    }
#endif

    if (!UDataMemory_isLoaded(&tData)) {
        /* no common data */
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return NULL;
    }

    /* we have mapped a file, check its header */
    udata_checkCommonData(&tData, pErrorCode);


    /* Cache the UDataMemory struct for this .dat file,
     *   so we won't need to hunt it down and map it again next time
     *   something is needed from it.                */
    return udata_cacheDataItem(inBasename, &tData, pErrorCode);
}


#ifdef OS390
#   define MAX_STUB_ENTRIES 8
#else
#   define MAX_STUB_ENTRIES 0
#endif


/*----------------------------------------------------------------------*
 *                                                                      *
 *   extendICUData   If the full set of ICU data was not loaded at      *
 *                   program startup, load it now.  This function will  *
 *                   be called when the lookup of an ICU data item in   *
 *                   the common ICU data fails.                         *
 *                                                                      *
 *                   The parameter is the UDataMemory in which the      *
 *                   search for a requested item failed.                *
 *                                                                      *
 *                   return true if new data is loaded, false otherwise.*
 *                                                                      *
 *----------------------------------------------------------------------*/
static UBool extendICUData(UDataMemory *failedData, UErrorCode *pErr)
{
    /*  If the data library that we are running with turns out to be the
     *   stub library (or, on the 390, the subset library), we will try to
     *   load a .dat file instead.  The stub library has no entries in its
     *   TOC, which is how we identify it here.
     */
    UDataMemory   *pData;
    UDataMemory   copyPData;

    if (failedData->vFuncs->NumEntries(failedData) > MAX_STUB_ENTRIES) {
        /*  Not the stub.  We can't extend.  */
        return FALSE;
    }

    /* See if we can explicitly open a .dat file for the ICUData. */
    pData = openCommonData(
               U_ICUDATA_NAME,            /*  "icudt20l" , for example.          */
               FALSE,                     /*  Pretend we're not opening ICUData  */
               pErr);

    /* How about if there is no pData, eh... */

   UDataMemory_init(&copyPData);
   if(pData != NULL) {
      UDatamemory_assign(&copyPData, pData);
      copyPData.map = 0;              /* The mapping for this data is owned by the hash table */
      copyPData.mapAddr = 0;          /*   which will unmap it when ICU is shut down.         */
                                      /* CommonICUData is also unmapped when ICU is shut down.*/
                                      /* To avoid unmapping the data twice, zero out the map  */
                                      /*   fields in the UDataMemory that we're assigning     */
                                      /*   to CommonICUData.                                  */

      setCommonICUData(&copyPData,    /*  The new common data.                                */
                   failedData,        /*  Old ICUData ptr.  Overwrite of this value is ok,    */
                   FALSE,             /*  No warnings if write didn't happen                  */
                   pErr);             /*  setCommonICUData honors errors; NOP if error set    */
    }
    

    return gCommonICUData != failedData;   /* Return true if ICUData pointer was updated.   */
                                    /*   (Could potentialy have been done by another thread racing */
                                    /*   us through here, but that's fine, we still return true    */
                                    /*   so that current thread will also examine extended data.   */
}




/*----------------------------------------------------------------------*
 *                                                                      *
 *   udata_setCommonData                                                *
 *                                                                      *
 *----------------------------------------------------------------------*/
U_CAPI void U_EXPORT2
udata_setCommonData(const void *data, UErrorCode *pErrorCode) {
    UDataMemory dataMemory;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    if(data==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    /* do we already have common ICU data set? */
    if(gCommonICUData != NULL) {
        *pErrorCode=U_USING_DEFAULT_WARNING;
        return;
    }

    /* set the data pointer and test for validity */
    UDataMemory_init(&dataMemory);
    UDataMemory_setData(&dataMemory, data);
    udata_checkCommonData(&dataMemory, pErrorCode);
    if (U_FAILURE(*pErrorCode)) {return;}

    /* we have good data */
    /* Set it up as the ICU Common Data.  */
    setCommonICUData(&dataMemory, NULL, TRUE, pErrorCode);
}




/*---------------------------------------------------------------------------
 *
 *  udata_setAppData
 *
 *---------------------------------------------------------------------------- */
U_CAPI void U_EXPORT2
udata_setAppData(const char *path, const void *data, UErrorCode *err)
{
    UDataMemory     udm;

    if(err==NULL || U_FAILURE(*err)) {
        return;
    }
    if(data==NULL) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    UDataMemory_init(&udm);
    udm.pHeader = data;
    udata_checkCommonData(&udm, err);
    udata_cacheDataItem(path, &udm, err);
}

/*----------------------------------------------------------------------------*
 *                                                                            *
 *  checkDataItem     Given a freshly located/loaded data item, either        *
 *                    an entry in a common file or a separately loaded file,  *
 *                    sanity check its header, and see if the data is         *
 *                    acceptable to the app.                                  *
 *                    If the data is good, create and return a UDataMemory    *
 *                    object that can be returned to the application.         *
 *                    Return NULL on any sort of failure.                     *
 *                                                                            *
 *----------------------------------------------------------------------------*/
static UDataMemory *
checkDataItem
(
 const DataHeader         *pHeader,         /* The data item to be checked.                */
 UDataMemoryIsAcceptable  *isAcceptable,    /* App's call-back function                    */
 void                     *context,         /*   pass-thru param for above.                */
 const char               *type,            /*   pass-thru param for above.                */
 const char               *name,            /*   pass-thru param for above.                */
 UErrorCode               *nonFatalErr,     /* Error code if this data was not acceptable  */
                                            /*   but openChoice should continue with       */
                                            /*   trying to get data from fallback path.    */
 UErrorCode               *fatalErr         /* Bad error, caller should return immediately */
 )
{
    UDataMemory  *rDataMem = NULL;          /* the new UDataMemory, to be returned.        */

    if (U_FAILURE(*fatalErr)) {
        return NULL;
    }

    if(pHeader->dataHeader.magic1==0xda &&
        pHeader->dataHeader.magic2==0x27 &&
        (isAcceptable==NULL || isAcceptable(context, type, name, &pHeader->info))
    ) {
        rDataMem=UDataMemory_createNewInstance(fatalErr);
        if (U_FAILURE(*fatalErr)) {
            return NULL;
        }
        rDataMem->pHeader = pHeader;
    } else {
        /* the data is not acceptable, look further */
        /* If we eventually find something good, this errorcode will be */
        /*    cleared out.                                              */
        *nonFatalErr=U_INVALID_FORMAT_ERROR;
    }
    return rDataMem;
}

/**
 * @return 0 if not loaded, 1 if loaded or err 
 */
static UDataMemory *doLoadFromIndividualFiles(const char *pkgName, 
        const char *dataPath, const char *tocEntryPathSuffix,
            /* following arguments are the same as doOpenChoice itself */
            const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *subErrorCode,
             UErrorCode *pErrorCode)
{
    UDataMemory *retVal = NULL;
    const char         *pathBuffer;
    UDataMemory         dataMemory;
    UDataMemory *pEntryData;

    UDataPathIterator   iter;
    /* look in ind. files: package\nam.typ  ========================= */
    /* init path iterator for individual files */
    udata_pathiter_init(&iter, dataPath, pkgName, path, tocEntryPathSuffix, FALSE);

    while((pathBuffer = udata_pathiter_next(&iter)))
    {
#ifdef UDATA_DEBUG
        fprintf(stderr, "UDATA: trying individual file %s\n", pathBuffer);
#endif
        if(uprv_mapFile(&dataMemory, pathBuffer))
        {
            pEntryData = checkDataItem(dataMemory.pHeader, isAcceptable, context, type, name, subErrorCode, pErrorCode);
            if (pEntryData != NULL) {
                /* Data is good.
                *  Hand off ownership of the backing memory to the user's UDataMemory.
                *  and return it.   */
                pEntryData->mapAddr = dataMemory.mapAddr;
                pEntryData->map     = dataMemory.map;

#ifdef UDATA_DEBUG
                fprintf(stderr, "** Mapped file: %s\n", pathBuffer);
#endif
                retVal = pEntryData;
                goto commonReturn;
            }

            /* the data is not acceptable, or some error occured.  Either way, unmap the memory */
            udata_close(&dataMemory);

            /* If we had a nasty error, bail out completely.  */
            if (U_FAILURE(*pErrorCode)) {
                retVal = NULL;
                goto commonReturn;
            }

            /* Otherwise remember that we found data but didn't like it for some reason  */
            *subErrorCode=U_INVALID_FORMAT_ERROR;
        }
#ifdef UDATA_DEBUG
        fprintf(stderr, "%s\n", UDataMemory_isLoaded(&dataMemory)?"LOADED":"not loaded");
#endif
    }
commonReturn:
    udata_pathiter_dt(&iter);
    return retVal;
}

/**
 * @return 0 if not loaded, 1 if loaded or err 
 */
static UDataMemory *doLoadFromCommonData(UBool isICUData, const char *pkgName, 
        const char *dataPath, const char *tocEntryPathSuffix, const char *tocEntryName,
            /* following arguments are the same as doOpenChoice itself */
            const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *subErrorCode,
             UErrorCode *pErrorCode)
{
    UDataMemory *retVal = NULL;
    UDataMemory        *pEntryData;
    const DataHeader   *pHeader;
    UDataMemory        *pCommonData;
    /* try to get common data.  The loop is for platforms such as the 390 that do
     *  not initially load the full set of ICU data.  If the lookup of an ICU data item
     *  fails, the full (but slower to load) set is loaded, the and the loop repeats,
     *  trying the lookup again.  Once the full set of ICU data is loaded, the loop wont
     *  repeat because the full set will be checked the first time through.
     *
     *  The loop also handles the fallback to a .dat file if the application linked
     *   to the stub data library rather than a real library.
     */
    for (;;) {
        pCommonData=openCommonData(path, isICUData, subErrorCode); /** search for pkg **/

        if(U_SUCCESS(*subErrorCode)) {
            int32_t length;

            /* look up the data piece in the common data */
            pHeader=pCommonData->vFuncs->Lookup(pCommonData, tocEntryName, &length, subErrorCode);
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: pHeader=%p - %s\n", tocEntryName, pHeader, u_errorName(*subErrorCode));
#endif

            if(pHeader!=NULL) {
                pEntryData = checkDataItem(pHeader, isAcceptable, context, type, name, subErrorCode, pErrorCode);
#ifdef UDATA_DEBUG
                fprintf(stderr, "pEntryData=%p\n", pEntryData);
#endif
                if (U_FAILURE(*pErrorCode)) {
                    retVal = NULL;
                    goto commonReturn;
                }
                if (pEntryData != NULL) {
                    pEntryData->length = length;
                    retVal =  pEntryData;
                    goto commonReturn;
                }
            }
        }
        /* Data wasn't found.  If we were looking for an ICUData item and there is
         * more data available, load it and try again,
         * otherwise break out of this loop. */
        if (!(isICUData && pCommonData && extendICUData(pCommonData, subErrorCode))) {
            break;
        }
    }

commonReturn:
    return retVal;
}

/*
 *  A note on the ownership of Mapped Memory
 *
 *  For common format files, ownership resides with the UDataMemory object
 *    that lives in the cache of opened common data.  These UDataMemorys are private
 *    to the udata implementation, and are never seen directly by users.
 *
 *    The UDataMemory objects returned to users will have the address of some desired
 *    data within the mapped region, but they wont have the mapping info itself, and thus
 *    won't cause anything to be removed from memory when they are closed.
 *
 *  For individual data files, the UDataMemory returned to the user holds the
 *  information necessary to unmap the data on close.  If the user independently
 *  opens the same data file twice, two completely independent mappings will be made.
 *  (There is no cache of opened data items from individual files, only a cache of
 *   opened Common Data files, that is, files containing a collection of data items.)
 *
 *  For common data passed in from the user via udata_setAppData() or
 *  udata_setCommonData(), ownership remains with the user.
 *
 *  UDataMemory objects themselves, as opposed to the memory they describe,
 *  can be anywhere - heap, stack/local or global.
 *  They have a flag to indicate when they're heap allocated and thus
 *  must be deleted when closed.
 */


/*----------------------------------------------------------------------------*
 *                                                                            *
 * main data loading functions                                                *
 *                                                                            *
 *----------------------------------------------------------------------------*/
static UDataMemory *
doOpenChoice(const char *path, const char *type, const char *name,
             UDataMemoryIsAcceptable *isAcceptable, void *context,
             UErrorCode *pErrorCode)
{
    UDataMemory         *retVal = NULL;
    
    TinyString          tocEntryName; /* entry name in tree format. ex:  'icudt28b/coll/ar.res' */
    TinyString          tocEntryPath; /* entry name in path format. ex:  'icudt28b\\coll\\ar.res' */

    TinyString          pkgName;
    TinyString          treeName;
#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)  /*  '/' vs '\' */
    TinyString          altSepPath;
#endif

    const char         *dataPath;

    int32_t             tocEntrySuffixIndex;
    const char         *tocEntryPathSuffix;
    UErrorCode          subErrorCode=U_ZERO_ERROR;
    const char         *treeChar;

    UBool               isICUData = FALSE;


    /* Is this path ICU data? */
    if(path == NULL ||
       !strcmp(path, U_ICUDATA_ALIAS) ||  /* "ICUDATA" */
       !uprv_strncmp(path, U_ICUDATA_NAME U_TREE_SEPARATOR_STRING, /* "icudt26e-" */
                     uprv_strlen(U_ICUDATA_NAME U_TREE_SEPARATOR_STRING)) ||  
       !uprv_strncmp(path, U_ICUDATA_ALIAS U_TREE_SEPARATOR_STRING, /* "ICUDATA-" */
                     uprv_strlen(U_ICUDATA_ALIAS U_TREE_SEPARATOR_STRING))) {
      isICUData = TRUE;
    }

#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)  /* Windows:  try "foo\bar" and "foo/bar" */
    /* remap from alternate path char to the main one */
    TinyString_init(&altSepPath);
    if(path) {
        char *p;
        if((p=uprv_strchr(path,U_FILE_ALT_SEP_CHAR))) {
            TinyString_append(&altSepPath, path);
            while((p=uprv_strchr(altSepPath.s,U_FILE_ALT_SEP_CHAR))) {
                *p = U_FILE_SEP_CHAR;
            }
#if defined (UDATA_DEBUG)
            fprintf(stderr, "Changed path from [%s] to [%s]\n", path, altSepPath.s);
#endif
            path = altSepPath.s;
        }
    }
#endif

    TinyString_init(&tocEntryName);
    TinyString_init(&tocEntryPath);

    TinyString_init(&pkgName);
    TinyString_init(&treeName);

    /* ======= Set up strings */
    if(path==NULL) {
        TinyString_append(&pkgName, U_ICUDATA_NAME); 
    } else {
        const char *pkg;
        const char *first;
        pkg = uprv_strrchr(path, U_FILE_SEP_CHAR);
        first = uprv_strchr(path, U_FILE_SEP_CHAR);
        if(uprv_pathIsAbsolute(path) || (pkg != first)) { /* more than one slash in the path- not a tree name */
            /* see if this is an /absolute/path/to/package  path */
            if(pkg) {
                TinyString_append(&pkgName, pkg+1);
            } else {
                TinyString_append(&pkgName, path);
            }
        } else {
            treeChar = uprv_strchr(path, U_TREE_SEPARATOR);
            if(treeChar) { 
                TinyString_append(&treeName, treeChar+1); /* following '-' */
                if(isICUData) {
                    TinyString_append(&pkgName, U_ICUDATA_NAME);
                } else {
                    TinyString_appendn(&pkgName, path, (int32_t)(treeChar-path));
                    if (first == NULL) {
                        /*
                        This user data has no path, but there is a tree name.
                        Look up the correct path from the data cache later.
                        */
                        path = pkgName.s;
                    }
                }
            } else {
                if(isICUData) {
                    TinyString_append(&pkgName, U_ICUDATA_NAME);
                } else {
                    TinyString_append(&pkgName, path);
                }
            }
        }
    }

#ifdef UDATA_DEBUG
    fprintf(stderr, " P=%s T=%s\n", pkgName.s, treeName.s);
#endif

    /* setting up the entry name and file name 
     * Make up a full name by appending the type to the supplied
     *  name, assuming that a type was supplied.
     */

    /* prepend the package */
    TinyString_append(&tocEntryName, pkgName.s);
    TinyString_append(&tocEntryPath, pkgName.s);
    tocEntrySuffixIndex = tocEntryName.length;

    if(treeName.s[0]) {
        TinyString_append(&tocEntryName, U_TREE_ENTRY_SEP_STRING);
        TinyString_append(&tocEntryName, treeName.s);

        TinyString_append(&tocEntryPath, U_FILE_SEP_STRING);
        TinyString_append(&tocEntryPath, treeName.s);
    }

    TinyString_append(&tocEntryName, U_TREE_ENTRY_SEP_STRING);
    TinyString_append(&tocEntryPath, U_FILE_SEP_STRING);
    TinyString_append(&tocEntryName, name);
    TinyString_append(&tocEntryPath, name);
    if(type!=NULL && *type!=0) {
        TinyString_append(&tocEntryName, ".");
        TinyString_append(&tocEntryName, type);
        TinyString_append(&tocEntryPath, ".");
        TinyString_append(&tocEntryPath, type);
    }
    tocEntryPathSuffix = tocEntryPath.s+tocEntrySuffixIndex; /* suffix starts here */

#ifdef UDATA_DEBUG
    fprintf(stderr, " tocEntryName = %s\n", tocEntryName.s);
    fprintf(stderr, " tocEntryPath = %s\n", tocEntryName.s);
#endif    

    if(path == NULL) {
        path = COMMON_DATA_NAME; /* "icudt26e" */
    }

    /************************ Begin loop looking for ind. files ***************/
#ifdef UDATA_DEBUG
    fprintf(stderr, "IND: inBasename = %s, pkg=%s\n", inBasename, packageNameFromPath(path));
#endif

    /* End of dealing with a null basename */
    dataPath = u_getDataDirectory();

    /****    COMMON PACKAGE  - only if packages are first. */
    if(gDataFileAccess == UDATA_PACKAGES_FIRST) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying packages (UDATA_PACKAGES_FIRST)\n");
#endif
        /* #2 */
        retVal = doLoadFromCommonData(isICUData, 
                            pkgName.s, dataPath, tocEntryPathSuffix, tocEntryName.s,
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            goto commonReturn;
        }
    }
    
    /****    INDIVIDUAL FILES  */
    if((gDataFileAccess==UDATA_PACKAGES_FIRST) ||
       (gDataFileAccess==UDATA_FILES_FIRST)) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying individual files\n");
#endif
        /* Check to make sure that there is a dataPath to iterate over */
        if ((dataPath && *dataPath) || !isICUData) {
            retVal = doLoadFromIndividualFiles(pkgName.s, dataPath, tocEntryPathSuffix,
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
            if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
                goto commonReturn;
            }
        }
    }

    /****    COMMON PACKAGE  */
    if((gDataFileAccess==UDATA_ONLY_PACKAGES) || 
       (gDataFileAccess==UDATA_FILES_FIRST)) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying packages (UDATA_ONLY_PACKAGES || UDATA_FILES_FIRST)\n");
#endif
        retVal = doLoadFromCommonData(isICUData, 
                            pkgName.s, dataPath, tocEntryPathSuffix, tocEntryName.s,
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            goto commonReturn;
        }
    }
    
    /* Load from DLL.  If we haven't attempted package load, we also haven't had any chance to
        try a DLL (static or setCommonData/etc)  load.
         If we ever have a "UDATA_ONLY_FILES", add it to the or list here.  */  
    if(gDataFileAccess==UDATA_NO_FILES) {
#ifdef UDATA_DEBUG
        fprintf(stderr, "Trying common data (UDATA_NO_FILES)\n");
#endif
        retVal = doLoadFromCommonData(isICUData, 
                            pkgName.s, "", tocEntryPathSuffix, tocEntryName.s,
                            path, type, name, isAcceptable, context, &subErrorCode, pErrorCode);
        if((retVal != NULL) || U_FAILURE(*pErrorCode)) {
            goto commonReturn;
        }
    }

    /* data not found */
    if(U_SUCCESS(*pErrorCode)) {
        if(U_SUCCESS(subErrorCode)) {
            /* file not found */
            *pErrorCode=U_FILE_ACCESS_ERROR;
        } else {
            /* entry point not found or rejected */
            *pErrorCode=subErrorCode;
        }
    }

commonReturn:
    TinyString_dt(&tocEntryName);
    TinyString_dt(&tocEntryPath);
    TinyString_dt(&pkgName);
    TinyString_dt(&treeName);
#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
    TinyString_dt(&altSepPath);
#endif
    return retVal;
}



/* API ---------------------------------------------------------------------- */

U_CAPI UDataMemory * U_EXPORT2
udata_open(const char *path, const char *type, const char *name,
           UErrorCode *pErrorCode) {
#ifdef UDATA_DEBUG
  fprintf(stderr, "udata_open(): Opening: %s : %s . %s\n", (path?path:"NULL"), name, type);
    fflush(stderr);
#endif

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        return doOpenChoice(path, type, name, NULL, NULL, pErrorCode);
    }
}



U_CAPI UDataMemory * U_EXPORT2
udata_openChoice(const char *path, const char *type, const char *name,
                 UDataMemoryIsAcceptable *isAcceptable, void *context,
                 UErrorCode *pErrorCode) {
#ifdef UDATA_DEBUG
  fprintf(stderr, "udata_openChoice(): Opening: %s : %s . %s\n", (path?path:"NULL"), name, type);
#endif

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(name==NULL || *name==0 || isAcceptable==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    } else {
        return doOpenChoice(path, type, name, isAcceptable, context, pErrorCode);
    }
}



U_CAPI void U_EXPORT2
udata_getInfo(UDataMemory *pData, UDataInfo *pInfo) {
    if(pInfo!=NULL) {
        if(pData!=NULL && pData->pHeader!=NULL) {
            const UDataInfo *info=&pData->pHeader->info;
            uint16_t dataInfoSize=udata_getInfoSize(info);
            if(pInfo->size>dataInfoSize) {
                pInfo->size=dataInfoSize;
            }
            uprv_memcpy((uint16_t *)pInfo+1, (const uint16_t *)info+1, pInfo->size-2);
            if(info->isBigEndian!=U_IS_BIG_ENDIAN) {
                /* opposite endianness */
                uint16_t x=info->reservedWord;
                pInfo->reservedWord=(uint16_t)((x<<8)|(x>>8));
            }
        } else {
            pInfo->size=0;
        }
    }
}


U_CAPI void U_EXPORT2 udata_setFileAccess(UDataFileAccess access, UErrorCode *status)
{
    gDataFileAccess = access;
}

