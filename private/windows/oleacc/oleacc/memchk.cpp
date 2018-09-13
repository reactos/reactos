// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  MEMCHK.CPP
//
//  Copyright 1996- Microsoft Corporation.
//
//  Simple new/delete counting error checking library
//
// --------------------------------------------------------------------------
#include "oleacc_p.h"
#include "default.h"
#include "w95trace.h"
#include "memchk.h"


#ifdef _DEBUG

struct MemInfo
{
	LONG    m_NumAlloc;
	LONG    m_NumFree;
};

// Two MemInfo structures - one for allocations through new/delete,
// one for allocations through SharedAlloc/SharedFree
MemInfo g_MemInfo;
MemInfo g_SharedMemInfo;

#endif // _DEBUG



#ifndef _DEBUG

// Non-_DEBUG new/delete call-through to LocalAlloc/Free...

// --------------------------------------------------------------------------
//
//  new()
//
//  We implement this ourself to avoid pulling in the C++ runtime.
//
// --------------------------------------------------------------------------

void *  __cdecl operator new(size_t nSize)
{
    // Zero init just to save some headaches
    return (void *)LocalAlloc(LPTR, nSize);
}


// --------------------------------------------------------------------------
//
//  delete()
//
//  We implement this ourself to avoid pulling in the C++ runtime.
//
// --------------------------------------------------------------------------
void  __cdecl operator delete(void *pv)
{
    LocalFree((HLOCAL)pv);
}


// --------------------------------------------------------------------------
//
//  SharedAlloc()
//
//  This allocates out of the shared heap on Win '95. On NT, we need to
//  use VirtualAllocEx to allocate memory in the other process. The caller
//  of SharedAlloc will need to then use ReadProcessMemory to read the data
//  from the VirtualAlloc'ed memory. What I am going to do is create 2 new
//  functions - SharedRead and SharedWrite, that will read and write shared
//  memory. On Win95, they will just use CopyMemory, but on NT they will use
//  ReadProcessMemory and WriteProcessMemory.
//
//  Parameters:
//      UINT    cbSize      Size of the memory block required
//      HWND    hwnd        Window handle in the process to allocate
//                          the shared memory in.
//      HANDLE* pProcHandle Pointer to a handle that has the process
//                          handle filled in on return. This must be saved
//                          for use in calls to SharedRead, SharedWrite,
//                          and SharedFree.
//
//  Returns:
//      Pointer to the allocated memory, or NULL if it fails. Access to the
//      memory must be done using SharedRead and SharedWrite. On success,
//      pProcHandle is filled in as well.
//
// --------------------------------------------------------------------------
LPVOID SharedAlloc(UINT cbSize,HWND hwnd,HANDLE *pProcessHandle)
{
#ifdef _X86_
    if (fWindows95)
        return(HeapAlloc(hheapShared, HEAP_ZERO_MEMORY, cbSize));
    else
#endif // _X86_
    {
    DWORD   dwProcessId;
    HANDLE  hProcess;

        if (GetWindowThreadProcessId (hwnd,&dwProcessId))
        {
            hProcess = OpenProcess (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
									FALSE,dwProcessId);
            if (pProcessHandle)
                *pProcessHandle = hProcess;
            return (MyVirtualAllocEx(hProcess,NULL,cbSize,MEM_COMMIT,PAGE_READWRITE));
        }
        return NULL;
    }
}


// --------------------------------------------------------------------------
//
//  SharedFree()
//
//  This frees shared memory.
//
// --------------------------------------------------------------------------
VOID SharedFree(LPVOID lpv,HANDLE hProcess)
{
#ifdef _X86_
    if (fWindows95)
        HeapFree(hheapShared, 0, lpv);
    else
#endif // _X86_
    {
        MyVirtualFreeEx(hProcess,lpv,0,MEM_RELEASE);
        CloseHandle (hProcess);
    }
}


// 'Empty' functions to keep compiler/linker happy in case client
// calls these in non _DEBUG code...
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void InitMemChk()
{
	// Do nothing
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
void UninitMemChk()
{
	// Do nothing
}



#else // _DEBUG #############################################################

// --------------------------------------------------------------------------
// DEBUG new - increments new count, calls through to LocalAlloc...
// --------------------------------------------------------------------------
void *  __cdecl operator new(unsigned int nSize)
{
    // Zero init just to save some headaches
    void * pv = (void *)LocalAlloc(LPTR, nSize);

	if( ! pv )
	{
		return NULL;
	}

	// Update statistics...
	InterlockedIncrement( & g_MemInfo.m_NumAlloc );

	// return pointer to alloc'd space...
	return pv;
}


// --------------------------------------------------------------------------
// DEBUG delete - increments delete count, calls through to LocalFree...
// --------------------------------------------------------------------------
void  __cdecl operator delete(void *pv)
{
	// C++ allows 'delete NULL'...
	if( pv == NULL )
		return;

    // Update statistics...
	InterlockedIncrement( & g_MemInfo.m_NumFree );

    LocalFree((HLOCAL)pv);
}

// --------------------------------------------------------------------------
//
//  DEBUG SharedAlloc()
//
//  Does alloc, updates count.
// --------------------------------------------------------------------------
LPVOID SharedAlloc(UINT cbSize,HWND hwnd,HANDLE *pProcessHandle)
{
#ifdef _X86_
    if (fWindows95)
	{
		// Update statistics...
		InterlockedIncrement( & g_SharedMemInfo.m_NumAlloc );

        return(HeapAlloc(hheapShared, HEAP_ZERO_MEMORY, cbSize));
	}
    else
#endif // _X86_
    {
    DWORD   dwProcessId;
    HANDLE  hProcess;

        if (GetWindowThreadProcessId (hwnd,&dwProcessId))
        {
			// Update statistics...
			InterlockedIncrement( & g_SharedMemInfo.m_NumAlloc );

            hProcess = OpenProcess (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
									FALSE,dwProcessId);
            if (pProcessHandle)
                *pProcessHandle = hProcess;
            return (MyVirtualAllocEx(hProcess,NULL,cbSize,MEM_COMMIT,PAGE_READWRITE));
        }
        return NULL;
    }
}



// --------------------------------------------------------------------------
//
//  DEBUG SharedFree()
//
//  frees shared memory, updates free count.
//
// --------------------------------------------------------------------------
VOID SharedFree(LPVOID lpv,HANDLE hProcess)
{
	// Update statistics...
	InterlockedIncrement( & g_SharedMemInfo.m_NumFree );

#ifdef _X86_
    if (fWindows95)
        HeapFree(hheapShared, 0, lpv);
    else
#endif // _X86_
    {
        MyVirtualFreeEx(hProcess,lpv,0,MEM_RELEASE);
        CloseHandle (hProcess);
    }
}


// --------------------------------------------------------------------------
// InitMemChk - sets alloc/free counts to zero.
// --------------------------------------------------------------------------
void InitMemChk()
{
	g_MemInfo.m_NumAlloc = 0;
	g_MemInfo.m_NumFree = 0;

	g_SharedMemInfo.m_NumAlloc = 0;
	g_SharedMemInfo.m_NumFree = 0;
}

// --------------------------------------------------------------------------
// UninitMemChk - outputs stats including number of unfree'd objects...
//
// Note that Shared memory is often allocated from one process and free'd
// from another, so when a process detatches the numbers may not match up.
// At some point in time it might be more useful to keep this as a global
// across all instances of the DLL.
// --------------------------------------------------------------------------
void UninitMemChk()
{
    DBPRINTF( TEXT("Total objects: %d, unfreed: %d\n"),
    	g_MemInfo.m_NumAlloc,
    	g_MemInfo.m_NumAlloc - g_MemInfo.m_NumFree );

    DBPRINTF( TEXT("Total Shared objects: %d, unfreed: %d\n"),
    	g_SharedMemInfo.m_NumAlloc,
    	g_SharedMemInfo.m_NumAlloc - g_SharedMemInfo.m_NumFree );
}

#endif // _DEBUG
