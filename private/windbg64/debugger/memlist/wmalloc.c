/*** wmalloc.c - Memory management entry points not implemented by WINDOWS 386
*
*   Copyright <C> 1989, Microsoft Corporation
*
*   Purpose: To supply the C runtime memory management entry points,
*      which are defective in the WINDOWS 386 runtime library.
*
*   Revision History:
*     24-Apr-1989 ArthurC Created
*   [1]     09-Jul-1989 ArthurC Assume near pointer is local handle
*   [2]     21-Jul-1989 ArthurC Fixed definition of _cvw3_hmalloc
*   [3]     22-Aug-1989 ArthurC Fixed order of if evaluation to prevent GP
*   [4]     02-Oct-1989 ArthurC Removed unused local variable
*
*************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include "cvwin32.h"    // for critical sections in win32

#ifdef HOST32
    #ifndef _HUGE_
        #define _HUGE_
    #endif
#else // !HOST32
    #define _HUGE_  _huge
#endif // HOST32

#define _ffree      free
#define _frealloc   realloc
#define _fmsize     _msize

/*
 * This structure is used to keep track of global allocates
 * so they can be freed at cleanup time
 */

typedef struct ALLOC_HEADER
{
    struct ALLOC_HEADER FAR *   lpahNext;   /* pointer to next allocation */
    struct ALLOC_HEADER FAR *   lpahPrev;   /* pointer to previous allocation */
        /*
        ** WARNING WARNING WARNING
        **
        ** The total size of this structure MUST be an exact multiple
        ** of eight bytes.  This is because many CPUs will be very
        ** inefficient if the user's data doesn't start at 8-byte
        ** alignment.  (Even on the x86 version of NT, GetThreadContext
        ** will fail if the passed buffer isn't DWORD-aligned!)
        **
        ** So if you add or remove elements, be sure to
        ** pad the structure appropriately.
        */
} ALLOC_HEADER;
typedef ALLOC_HEADER FAR *LPAH;

ALLOC_HEADER static ahFirst, ahLast;

#ifdef __cplusplus
extern "C" {    // rest of file
#endif

/*
 * This initializes the doubly linked chain of globally allocated blocks
 */

static BOOL fCritSecInit = FALSE;

void  cvw3_minit(void)
{
    /*
    ** As mentioned above in the definition of ALLOC_HEADER,
    ** it is essential that sizeof(ALLOC_HEADER) be a multiple
    ** of 8.
    */
    assert((sizeof(ALLOC_HEADER) % 8) == 0);

    if (!fCritSecInit)
    {
        CVInitCritSection(icsWmalloc);
        fCritSecInit = TRUE;
    }

    ahFirst.lpahNext = &ahLast;
    ahFirst.lpahPrev = &ahFirst;
    ahLast.lpahPrev = &ahFirst;
    ahLast.lpahNext = NULL;
}

/*
 * cvw3_halloc does a malloc and chains the result so it can be cleaned up.
 */
void _HUGE_ * CDECL cvw3_halloc(long n, size_t size)
{
    LPAH            lpah;
    void _HUGE_ *   retval = NULL;

    CVEnterCritSection(icsWmalloc);

    /*
     * Allocate extra space for the handle and chain pointers.
     * Use calloc and not malloc since MHAlloc is supposed to 
     * return zero-filled memory.
     */
    lpah = (LPAH) calloc(size * n + sizeof(ALLOC_HEADER), 1);
    if(lpah)
    {
        /*
         * Add the new block to the chain.
         */
        *lpah = ahFirst;
        ahFirst.lpahNext->lpahPrev = lpah;
        ahFirst.lpahNext = lpah;

        /*
         * Return a pointer beyond the header.
         */
        retval = (lpah + 1);
    }

    CVLeaveCritSection(icsWmalloc);

    return retval;
}

/*
 * hfree unchains the block and frees it.
 */
void cvw3_hfree(void _HUGE_ *buffer)
{
    LPAH    lpah;

    CVEnterCritSection(icsWmalloc);

    lpah = (LPAH)buffer - 1;
    lpah->lpahNext->lpahPrev = lpah->lpahPrev;
    lpah->lpahPrev->lpahNext = lpah->lpahNext;
    free(lpah);

    CVLeaveCritSection(icsWmalloc);
}

void cvw3_ffree(void FAR *buffer)
{
    /* no need to call CVEnterCritSection, because cvw3_hfree() will do so */

    cvw3_hfree(buffer);
}

#ifdef WINQCXX

void PASCAL ResetWMallocStatics(void)
{
    /*
    ** In the current implementation there's nothing we need to do here,
    ** but I figured it's probably best that I not remove this function
    ** just yet
    */
}

#endif // !WINQCXX


void FAR *cvw3_fmalloc(size_t size)
{
    /* no need to call CVEnterCritSection, because cvw3_hfree() will do so */

    return (void FAR *) cvw3_halloc(size, 1);
}

void cvw3_mcleanup(void)
{
    LPAH lpah;
    LPAH lpah2;

    lpah = ahFirst.lpahNext;
    while (lpah2 = lpah->lpahNext)
    {
        free(lpah);
        lpah = lpah2;
    }

    if (fCritSecInit)
    {
        CVDeleteCritSection(icsWmalloc);
        fCritSecInit = FALSE;
    }

    // don't know why we call init from cleanup, because
    // all calls to cleanup are followed by calls to init.
    cvw3_minit();
}

size_t cvw3_fmsize(void FAR *buffer)
{
    LPAH lpah;

    lpah = (LPAH)buffer - 1;

    return (_fmsize(lpah) - sizeof(ALLOC_HEADER));
}

void FAR *cvw3_frealloc(void FAR *buffer, size_t size)
{
    LPAH        lpahOld;
    LPAH        lpahNew;
    void FAR *  retval = NULL;

    CVEnterCritSection(icsWmalloc);

    lpahOld = (LPAH) buffer - 1;
    lpahNew = (LPAH) _frealloc(lpahOld, size+sizeof(ALLOC_HEADER));

    /*
    ** If realloc succeeded, adjust pointers.
    */
    if (lpahNew)
    {
        /*
        ** Adjust adjacent pointers.
        */
        lpahNew->lpahPrev->lpahNext = lpahNew;
        lpahNew->lpahNext->lpahPrev = lpahNew;

        /*
        ** Adjust return value to just past header.
        */
        retval = (lpahNew + 1);
    }

    CVLeaveCritSection(icsWmalloc);

    return retval;
}


void PASCAL LDSFinit( void ) {
    cvw3_minit( );
    }

void FAR * PASCAL LDSFmalloc( size_t size ) {
    return cvw3_fmalloc ( size );
}

void FAR * PASCAL LDSFrealloc ( void FAR * buffer, size_t size ) {
    return cvw3_frealloc ( buffer, size );
}

void PASCAL LDSFfree ( void FAR * buffer ) {
    cvw3_ffree ( buffer );
}

#ifdef __cplusplus
} // extern "C"
#endif
