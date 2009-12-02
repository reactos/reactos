/*
*******************************************************************************
*
*   Copyright (C) 1999-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  toolutil.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov19
*   created by: Markus W. Scherer
*
*   This file contains utility functions for ICU tools like genccode.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "toolutil.h"

#ifdef U_WINDOWS
#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>
#   include <direct.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#endif
#include <errno.h>

U_CAPI const char * U_EXPORT2
getLongPathname(const char *pathname) {
#ifdef U_WINDOWS
    /* anticipate problems with "short" pathnames */
    static WIN32_FIND_DATA info;
    HANDLE file=FindFirstFileA(pathname, &info);
    if(file!=INVALID_HANDLE_VALUE) {
        if(info.cAlternateFileName[0]!=0) {
            /* this file has a short name, get and use the long one */
            const char *basename=findBasename(pathname);
            if(basename!=pathname) {
                /* prepend the long filename with the original path */
                uprv_memmove(info.cFileName+(basename-pathname), info.cFileName, uprv_strlen(info.cFileName)+1);
                uprv_memcpy(info.cFileName, pathname, basename-pathname);
            }
            pathname=info.cFileName;
        }
        FindClose(file);
    }
#endif
    return pathname;
}

U_CAPI const char * U_EXPORT2
findBasename(const char *filename) {
    const char *basename=uprv_strrchr(filename, U_FILE_SEP_CHAR);

#if U_FILE_ALT_SEP_CHAR!=U_FILE_SEP_CHAR
    if(basename==NULL) {
        /* Use lenient matching on Windows, which can accept either \ or /
           This is useful for environments like Win32+CygWin which have both.
        */
        basename=uprv_strrchr(filename, U_FILE_ALT_SEP_CHAR);
    }
#endif

    if(basename!=NULL) {
        return basename+1;
    } else {
        return filename;
    }
}

U_CAPI void U_EXPORT2
uprv_mkdir(const char *pathname, UErrorCode *status) {
    int retVal = 0;
#if defined(U_WINDOWS)
    retVal = _mkdir(pathname);
#else
    retVal = mkdir(pathname, S_IRWXU | (S_IROTH | S_IXOTH) | (S_IROTH | S_IXOTH));
#endif
    if (retVal && errno != EEXIST) {
        *status = U_FILE_ACCESS_ERROR;
    }
}

/* tool memory helper ------------------------------------------------------- */

struct UToolMemory {
    char name[64];
    int32_t capacity, maxCapacity, size, index;
    void *array;
    UAlignedMemory staticArray[1];
};

U_CAPI UToolMemory * U_EXPORT2
utm_open(const char *name, int32_t initialCapacity, int32_t maxCapacity, int32_t size) {
    UToolMemory *mem;

    if(maxCapacity<initialCapacity) {
        maxCapacity=initialCapacity;
    }

    mem=(UToolMemory *)uprv_malloc(sizeof(UToolMemory)+initialCapacity*size);
    if(mem==NULL) {
        fprintf(stderr, "error: %s - out of memory\n", name);
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    mem->array=mem->staticArray;

    uprv_strcpy(mem->name, name);
    mem->capacity=initialCapacity;
    mem->maxCapacity=maxCapacity;
    mem->size=size;
    mem->index=0;
    return mem;
}

U_CAPI void U_EXPORT2
utm_close(UToolMemory *mem) {
    if(mem!=NULL) {
        if(mem->array!=mem->staticArray) {
            uprv_free(mem->array);
        }
        uprv_free(mem);
    }
}


U_CAPI void * U_EXPORT2
utm_getStart(UToolMemory *mem) {
    return (char *)mem->array;
}

U_CAPI int32_t U_EXPORT2
utm_countItems(UToolMemory *mem) {
    return mem->index;
}


static UBool
utm_hasCapacity(UToolMemory *mem, int32_t capacity) {
    if(mem->capacity<capacity) {
        int32_t newCapacity;

        if(mem->maxCapacity<capacity) {
            fprintf(stderr, "error: %s - trying to use more than maxCapacity=%ld units\n",
                    mem->name, (long)mem->maxCapacity);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }

        /* try to allocate a larger array */
        if(capacity>=2*mem->capacity) {
            newCapacity=capacity;
        } else if(mem->capacity<=mem->maxCapacity/3) {
            newCapacity=2*mem->capacity;
        } else {
            newCapacity=mem->maxCapacity;
        }

        if(mem->array==mem->staticArray) {
            mem->array=uprv_malloc(newCapacity*mem->size);
            if(mem->array!=NULL) {
                uprv_memcpy(mem->array, mem->staticArray, mem->index*mem->size);
            }
        } else {
            mem->array=uprv_realloc(mem->array, newCapacity*mem->size);
        }

        if(mem->array==NULL) {
            fprintf(stderr, "error: %s - out of memory\n", mem->name);
            exit(U_MEMORY_ALLOCATION_ERROR);
        }
    }

    return TRUE;
}

U_CAPI void * U_EXPORT2
utm_alloc(UToolMemory *mem) {
    char *p=(char *)mem->array+mem->index*mem->size;
    int32_t newIndex=mem->index+1;
    if(utm_hasCapacity(mem, newIndex)) {
        mem->index=newIndex;
        uprv_memset(p, 0, mem->size);
    }
    return p;
}

U_CAPI void * U_EXPORT2
utm_allocN(UToolMemory *mem, int32_t n) {
    char *p=(char *)mem->array+mem->index*mem->size;
    int32_t newIndex=mem->index+n;
    if(utm_hasCapacity(mem, newIndex)) {
        mem->index=newIndex;
        uprv_memset(p, 0, n*mem->size);
    }
    return p;
}
