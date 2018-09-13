//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       alloc.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_ALLOC_H
#define _INC_CSCVIEW_ALLOC_H
//////////////////////////////////////////////////////////////////////////////
/*  File: alloc.h

    Description: Installs a "new handler" that throws CMemoryException
        when a memory allocation request fails.  Required since our
        compiler doesn't throw bad_alloc on memory alloc failures.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
//
// Currently, USE_NEW_HANDLER is undefined.  Defining a new handler affects
// memory allocation behavior for each thread in a process.  Since the
// viewer code could be (most likely is) running on another thread,
// I didn't want to cause the throwing of exceptions to code that isn't
// prepared to handle those exceptions.  Therefore, I've opted for overloading
// global new and delete which is bound at compile time to the viewer modules
// only (which are exception-aware).  [brianau - 12/8/97]
//
#ifdef USE_NEW_HANDLER
#ifndef _INC_NEW
#   include <new.h>
#endif


class NewHandler
{
    public:
        NewHandler(void)
            { _set_new_handler(HandlerFunc); }

    private:
        static int __cdecl HandlerFunc(size_t size)
            { throw CException(ERROR_OUTOFMEMORY); return 0;}
};

#endif // USE_NEW_HANDLER


//
// Declarations for overloading global new and delete.
//
void * __cdecl operator new(size_t size);
void __cdecl operator delete(void *ptr) throw();



#endif // _CSCVIEW_ALLOC_H
