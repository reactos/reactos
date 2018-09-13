//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  memory.cxx
//
//  This file contains the new and delete routines for memory management in
//  the LSP dll.  Rather than using the memory management provided by the
//  C++ system we'll use the system allocator.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version.
//
//---------------------------------------------------------------------------

#include <windows.h>

static HANDLE g_hLspHeap = 0;

//---------------------------------------------------------------------------
//  InitializeLspAllocator()
//
//---------------------------------------------------------------------------
DWORD InitializeLspAllocator(void)
    {
    DWORD dwStatus = ERROR_SUCCESS;

    if (!g_hLspHeap)
        {
        g_hLspHeap = GetProcessHeap();
        if (!g_hLspHeap)
           {
           dwStatus = GetLastError();
           }
        }

    return dwStatus;
    }


//---------------------------------------------------------------------------
//  AllocWrapper()
//
//---------------------------------------------------------------------------
inline void *AllocWrapper( IN size_t size )
{
    if (!g_hLspHeap)
       {
       if (ERROR_SUCCESS != InitializeLspAllocator())
          {
          return 0;
          }
       }

    void *pobj = HeapAlloc( g_hLspHeap, 0, size );

    return pobj;
}

//---------------------------------------------------------------------------
//  FreeWrapper()
//
//---------------------------------------------------------------------------
inline void FreeWrapper( IN void *pobj )
    {
    if (g_hLspHeap)
       {
       HeapFree( g_hLspHeap, 0, pobj );
       }
    }

//---------------------------------------------------------------------------
//  operator new
//
//---------------------------------------------------------------------------
void *
__cdecl
operator new ( IN size_t size )
    {
    return AllocWrapper(size);
    }

//---------------------------------------------------------------------------
//  operator delete
//
//---------------------------------------------------------------------------
void
__cdecl
operator delete ( IN void * obj )
    {
    FreeWrapper(obj);
    }

