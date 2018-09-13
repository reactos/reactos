/****                                                                   ****
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: October 31, 1990 by David W. Gray                             *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include <windows.h>
#include <malloc.h>
#include "d.h"
//#include "types.h"
//#include "defs.h"
//#include "lb.h"
//#include "osassert.h"
//#include "mh.h"


/****                                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

void FAR *  PASCAL MHAlloc ( size_t size ) {
#if DEBUG > 1
     assert ( _fheapchk ( ) == _HEAPOK );
#endif
#ifdef HOST32
    return malloc ( size );
#else
    return _fmalloc ( size );
#endif
}


/****                                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

void FAR *  PASCAL MHRealloc ( void FAR *memblock, size_t size ) {
#if DEBUG > 1
    assert ( _fheapchk ( ) == _HEAPOK );
#endif
#ifdef HOST32
    return realloc ( memblock, size );
#else
    return _frealloc ( memblock, size );
#endif
}


/****                                                                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

void  PASCAL MHFree ( void FAR *memblock ) {
#if DEBUG > 1
    assert ( _fheapchk ( ) == _HEAPOK );
#endif
#ifdef  HOST32
    free( memblock );
#else
    _ffree ( memblock );
#endif
}
