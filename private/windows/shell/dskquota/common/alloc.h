#ifndef _INC_DSKQUOTA_ALLOC_H
#define _INC_DSKQUOTA_ALLOC_H
//////////////////////////////////////////////////////////////////////////////
/*  File: alloc.h

    Description: Installs a "new handler" that throws CAllocException
        when a memory allocation request fails.  Required since our
        compiler doesn't throw bad_alloc on memory alloc failures.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

//
// Declarations for overloading global new and delete.
//
void * __cdecl operator new(size_t size);
void __cdecl operator delete(void *ptr);



#endif // _INC_DSKQUOTA_ALLOC_H
