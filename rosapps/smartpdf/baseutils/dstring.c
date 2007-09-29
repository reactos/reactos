/****************************************************************************
 *
 * Dynamic strings
 ****************************************************************************/

/*
 * tcl.h --
 *
 *      This header file describes the externally-visible facilities
 *      of the Tcl interpreter.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tcl.h 1.283 96/10/02 17:17:39
 */


#include "dstring.h"
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "str_strsafe.h"

/*
 *----------------------------------------------------------------------
 *
 * DStringInit --
 *
 *      Initializes a dynamic string, discarding any previous contents
 *      of the string (DStringFree should have been called already
 *      if the dynamic string was previously in use).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The dynamic string is initialized to be empty.
 *
 *----------------------------------------------------------------------
 */

void
DStringInit(DString* pDs)
{
    pDs->pString = pDs->staticSpace;
    pDs->length = 0;
    pDs->spaceAvl = kDstringStaticSize;
    pDs->staticSpace[0] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DStringAppend --
 *
 *      Append more characters to the current value of a dynamic string.
 *
 * Results:
 *      The return value is a pointer to the dynamic string's new value.
 *
 * Side effects:
 *      Length bytes from string (or all of string if length is less
 *      than zero) are added to the current value of the string.  Memory
 *      gets reallocated if needed to accomodate the string's new size.
 *
 *----------------------------------------------------------------------
 */

char *
DStringAppend(DString *pDs,
                 const char* string,
                 int         length)
{
    int   newSize;
    char* newString;
    char* dst;
    const char* end;

    if (length < 0) {
        length = strlen(string);
    }
    newSize = length + pDs->length;

    /*
     * Allocate a larger buffer for the string if the current one isn't
     * large enough.  Allocate extra space in the new buffer so that there
     * will be room to grow before we have to allocate again.
     */

    if (newSize >= pDs->spaceAvl) {
        pDs->spaceAvl = newSize*2;
        newString = (char *) malloc((unsigned) pDs->spaceAvl);
        memcpy((void *)newString, (void *) pDs->pString,
                (size_t) pDs->length);
        if (pDs->pString != pDs->staticSpace) {
            free(pDs->pString);
        }
        pDs->pString = newString;
    }

    /*
     * Copy the new string into the buffer at the end of the old
     * one.
     */

    for (dst = pDs->pString + pDs->length, end = string+length;
            string < end; string++, dst++) {
        *dst = *string;
    }
    *dst = 0;
    pDs->length += length;
    return pDs->pString;
}

/*
 *----------------------------------------------------------------------
 *
 * DStringSetLength --
 *
 *      Change the length of a dynamic string.  This can cause the
 *      string to either grow or shrink, depending on the value of
 *      length.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The length of pDsis changed to length and a null byte is
 *      stored at that position in the string.  If length is larger
 *      than the space allocated for pDs, then a panic occurs.
 *
 *----------------------------------------------------------------------
 */

void
DStringSetLength(DString* pDs,
                    int         length)
{
    if (length < 0) {
        length = 0;
    }
    if (length >= pDs->spaceAvl) {
        char *newString;

        pDs->spaceAvl = length+1;
        newString = (char *) malloc((unsigned) pDs->spaceAvl);

        /*
         * SPECIAL NOTE: must use memcpy, not strcpy, to copy the string
         * to a larger buffer, since there may be embedded NULLs in the
         * string in some cases.
         */

        memcpy((void *) newString, (void*) pDs->pString,
                (size_t) pDs->length);
        if (pDs->pString != pDs->staticSpace) {
            free(pDs->pString);
        }
        pDs->pString = newString;
    }
    pDs->length = length;
    pDs->pString[length] = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DStringFree --
 *
 *      Frees up any memory allocated for the dynamic string and
 *      reinitializes the string to an empty state.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The previous contents of the dynamic string are lost, and
 *      the new value is an empty string.
 *
 *----------------------------------------------------------------------
 */

void
DStringFree(DString* pDs)
{
    if (pDs->pString != pDs->staticSpace) {
        free(pDs->pString);
    }
    pDs->pString = pDs->staticSpace;
    pDs->length = 0;
    pDs->spaceAvl = kDstringStaticSize;
    pDs->staticSpace[0] = 0;
}

/*
 * DStringSprintf --
 *
 * Append a formatted string to a dstring
 */

void
DStringSprintf(DString* pDs,
                   const char* pFormat,
                   ...)
{
#ifdef _WIN32
    HRESULT     hr;
    va_list     args;
    char        message[256];
    char  *     buf;
    size_t      bufCchSize;
    char *      result = NULL;

    buf = &(message[0]);
    bufCchSize = sizeof(message);

    va_start(args, pFormat);
    for (;;)
    {
        /* TODO: this only works on windows with recent C library */
        hr = StringCchVPrintfA(buf, bufCchSize, pFormat, args);
        if (S_OK == hr)
            break;
        if (STRSAFE_E_INSUFFICIENT_BUFFER != hr)
        {
            /* any error other than buffer not big enough:
               a) should not happen
               b) means we give up */
            goto Error;
        }
        /* we have to make the buffer bigger. The algorithm used to calculate
           the new size is arbitrary (aka. educated guess) */
        if (buf != &(message[0]))
            free(buf);
        if (bufCchSize < 4*1024)
            bufCchSize += bufCchSize;
        else
            bufCchSize += 1024;
        buf = (char *)malloc(bufCchSize*sizeof(char));
        if (NULL == buf)
            goto Error;
    }
    va_end(args);

    DStringAppend(pDs, buf, -1);
Error:
    if (buf != &(message[0]))
        free((void*)buf);
    return;
#else
  va_list args;
  char*   pBuffer;
  int     len;

  va_start(args, pFormat);
  len = vasprintf(&pBuffer, pFormat, args);
  DStringAppend(pDs, pBuffer, len);
  free(pBuffer);
  va_end(args);
#endif
}

/****************************************************************************
 * DStringAppendLowerCase --
 *
 * Append text to a dstring lowercased
 */

char*
DStringAppendLowerCase(DString*   pDs,
                         const char*    string,
                         int            length)
{
    int   newSize;
    char* newString;
    char* dst;
    const char* end;

    if (length < 0) {
        length = strlen(string);
    }
    newSize = length + pDs->length;

    /*
     * Allocate a larger buffer for the string if the current one isn't
     * large enough.  Allocate extra space in the new buffer so that there
     * will be room to grow before we have to allocate again.
     */

    if (newSize >= pDs->spaceAvl) {
        pDs->spaceAvl = newSize*2;
        newString = (char *) malloc((unsigned) pDs->spaceAvl);
        memcpy((void *)newString, (void *) pDs->pString,
                (size_t) pDs->length);
        if (pDs->pString != pDs->staticSpace) {
            free(pDs->pString);
        }
        pDs->pString = newString;
    }

    /*
     * Copy the new string into the buffer at the end of the old
     * one.
     */

    for (dst = pDs->pString + pDs->length, end = string+length;
            string < end; string++, dst++) {
        *dst = tolower(*string);
    }
    *dst = 0;
    pDs->length += length;
    return pDs->pString;
}
