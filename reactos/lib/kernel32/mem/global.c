/* $Id: global.c,v 1.26 2004/07/03 17:40:22 navaraf Exp $
 *
 * Win32 Global/Local heap functions (GlobalXXX, LocalXXX).
 * These functions included in Win32 for compatibility with 16 bit Windows
 * Especially the moveable blocks and handles are oldish. 
 * But the ability to directly allocate memory with GPTR and LPTR is widely
 * used.
 *
 * Updated to support movable memory with algorithms taken from wine.
 */

#include <k32.h>
#include <time.h>

#define NDEBUG
#include "../include/debug.h"

#ifdef _GNUC_
#define STRUCT_PACK __attribute__((packed))
#else
#define STRUCT_PACK
#endif

#define MAGIC_GLOBAL_USED 0x5342BEEF
#define GLOBAL_LOCK_MAX   0xFF

/*Wine found that some applications complain if memory isn't 8 byte aligned.
* We make use of that experience here.
*/
#define HANDLE_SIZE          8  /*sizeof(HANDLE) *2 */


typedef struct __GLOBAL_LOCAL_HANDLE
{
    DWORD   Magic;
    LPVOID  Pointer; STRUCT_PACK
    BYTE    Flags;
    BYTE    LockCount;
} GLOBAL_HANDLE, LOCAL_HANDLE, *PGLOBAL_HANDLE, *PLOCAL_HANDLE;

#define HANDLE_TO_INTERN(h)  ((PGLOBAL_HANDLE)(((char *)(h))-4))
#define INTERN_TO_HANDLE(i)  ((HGLOBAL) &((i)->Pointer))
#define POINTER_TO_HANDLE(p) (*(PHANDLE)(p - HANDLE_SIZE))
#define ISHANDLE(h)          ((((ULONG)(h)) & 0x4)!=0)
#define ISPOINTER(h)         ((((ULONG)(h)) & 0x4)==0)


static void DbgPrintStruct(PGLOBAL_HANDLE h)
{
    DPRINT("Magic:     0x%X\n", h->Magic);
    DPRINT("Pointer:   0x%X\n", h->Pointer);
    DPRINT("Flags:     0x%X\n", h->Flags);
    DPRINT("LockCount: 0x%X\n", h->LockCount);
}



/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalAlloc(UINT uFlags,
            DWORD dwBytes)
{

    PGLOBAL_HANDLE phandle    = 0;
    PVOID          palloc     = 0;
    UINT           heap_flags = 0;

    if (uFlags & GMEM_ZEROINIT)
    {
        heap_flags = HEAP_ZERO_MEMORY;
    }

    DPRINT("GlobalAlloc( 0x%X, 0x%lX )\n", uFlags, dwBytes);
    
    //Changed hProcessHeap to GetProcessHeap()
    if ((uFlags & GMEM_MOVEABLE)==0) /* POINTER */
    {
        palloc = RtlAllocateHeap(GetProcessHeap(), heap_flags, dwBytes);
        if (! ISPOINTER(palloc))
        {
            DPRINT1("GlobalAlloced pointer which is not 8-byte aligned\n");
            RtlFreeHeap(GetProcessHeap(), 0, palloc);
            return NULL;
        }
        return (HGLOBAL) palloc;
    }
    else  /* HANDLE */
    {
        HeapLock(hProcessHeap);

        phandle = RtlAllocateHeap(GetProcessHeap(), 0,  sizeof(GLOBAL_HANDLE));
        if (phandle)
        {
            phandle->Magic     = MAGIC_GLOBAL_USED;
            phandle->Flags     = uFlags >> 8;
            phandle->LockCount = 0;
            phandle->Pointer   = 0;

            if (dwBytes)
            {
                palloc = RtlAllocateHeap(GetProcessHeap(), heap_flags, dwBytes + HANDLE_SIZE);
                if (palloc)
                {
                    *(PHANDLE)palloc = INTERN_TO_HANDLE(phandle);
                    phandle->Pointer = palloc + HANDLE_SIZE;
                }
                else /*failed to allocate the memory block*/
                {
                    RtlFreeHeap(GetProcessHeap(), 0, phandle);
                    phandle = 0;
                }
            }
            else
            {
                DPRINT("Allocated a 0 size movable block.\n");
                DbgPrintStruct(phandle);
                DPRINT("Address of the struct: 0x%X\n", phandle);
                DPRINT("Address of pointer:    0x%X\n", &(phandle->Pointer));
            }
        }
        HeapUnlock(hProcessHeap);

        if (phandle)
        {
            if (ISPOINTER(INTERN_TO_HANDLE(phandle)))
            {
                DPRINT1("GlobalAlloced handle which is 8-byte aligned but shouldn't be\n");
                RtlFreeHeap(GetProcessHeap(), 0, palloc);
                RtlFreeHeap(GetProcessHeap(), 0, phandle);
                return NULL;
            }
            return INTERN_TO_HANDLE(phandle);	
        }
        else
            return (HGLOBAL)0;
    }
}


/*
 * @implemented
 */
SIZE_T STDCALL
GlobalCompact(DWORD dwMinFree)
{
   return RtlCompactHeap(hProcessHeap, 0);
}


/*
 * @implemented
 */
VOID STDCALL
GlobalFix(HGLOBAL hMem)
{
   if (INVALID_HANDLE_VALUE != hMem)
     GlobalLock(hMem);
}

/*
 * @implemented
 */
UINT STDCALL
GlobalFlags(HGLOBAL hMem)
{
    DWORD		    retval;
    PGLOBAL_HANDLE	phandle;

    DPRINT("GlobalFlags( 0x%lX )\n", (ULONG)hMem);

    if(!ISHANDLE(hMem))
    {
        DPRINT("GlobalFlags: Fixed memory.\n");
        retval = 0;
    }
    else
    {
        HeapLock(GetProcessHeap());
        
        phandle = HANDLE_TO_INTERN(hMem);

        /*DbgPrintStruct(phandle);*/

        if (MAGIC_GLOBAL_USED == phandle->Magic)
        {
            /*DbgPrint("GlobalFlags: Magic number ok\n");
            **DbgPrint("GlobalFlags: pointer is 0x%X\n", phandle->Pointer);
            */
            retval = phandle->LockCount + (phandle->Flags << 8);
            if (0 == phandle->Pointer)
            {
                retval = retval | GMEM_DISCARDED;
            }
        }
        else
        {
            DPRINT1("GlobalSize: invalid handle\n");
            retval = 0;
        }
        HeapUnlock(GetProcessHeap());
    }
    return retval;
}


/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalFree(HGLOBAL hMem)
{
    PGLOBAL_HANDLE phandle;
    
    DPRINT("GlobalFree( 0x%lX )\n", (ULONG)hMem);

    if (ISPOINTER(hMem)) /* POINTER */
    {
        RtlFreeHeap(GetProcessHeap(), 0, (PVOID)hMem);
        hMem = 0;
    }
    else /* HANDLE */
    {
        HeapLock(GetProcessHeap());
        
        phandle = HANDLE_TO_INTERN(hMem);

        if(MAGIC_GLOBAL_USED == phandle->Magic)
        {

            if(phandle->LockCount!=0)
            {
                DPRINT1("Warning! GlobalFree(0x%X) Freeing a handle to a locked object.\n", hMem);
                SetLastError(ERROR_INVALID_HANDLE);
            }
            
            if(phandle->Pointer)
                RtlFreeHeap(GetProcessHeap(), 0, phandle->Pointer - HANDLE_SIZE);
            
            RtlFreeHeap(GetProcessHeap(), 0, phandle);
        }
        HeapUnlock(GetProcessHeap());
        
        hMem = 0;
    }
    return hMem;
}


/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalHandle(LPCVOID pMem)
{
    HGLOBAL              handle = 0;
    PGLOBAL_HANDLE         test = 0;
    LPCVOID        pointer_test = 0;

    DPRINT("GlobalHandle( 0x%lX )\n", (ULONG)pMem);
    if (0 == pMem) /*Invalid argument */
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DPRINT1("Error: 0 handle.\n");
        return 0;
    }
  
    HeapLock(GetProcessHeap());
    /* Now test to see if this pointer is associated with a handle.
    * This is done by calling RtlValidateHeap() and seeing if it fails.
    */
    if (RtlValidateHeap(GetProcessHeap(), 0, (char *)pMem)) /*FIXED*/
    {
        handle = (HGLOBAL)pMem;
        return handle;
    }
    else /*MOVABLE*/
    {
        handle = POINTER_TO_HANDLE(pMem);        
    }
    
     
    /* Test to see if this memory is valid*/
    test  = HANDLE_TO_INTERN(handle);
    if (!IsBadReadPtr(test, sizeof(GLOBAL_HANDLE)))
    {
        if (MAGIC_GLOBAL_USED == test->Magic)
        {
            pointer_test = test->Pointer;
            if (!RtlValidateHeap(GetProcessHeap(), 0, ((char *)pointer_test) - HANDLE_SIZE) ||
                !RtlValidateHeap(GetProcessHeap(), 0, test))
            {
                SetLastError(ERROR_INVALID_HANDLE);
                handle = 0;
            }
        }
    }
    else
    {
        DPRINT1("GlobalHandle: Bad read pointer.\n");
        SetLastError(ERROR_INVALID_HANDLE);
        handle = 0;
    }

    HeapUnlock(GetProcessHeap());

    return handle;
}


/*
 * @implemented
 */
LPVOID STDCALL
GlobalLock(HGLOBAL hMem)
{
    PGLOBAL_HANDLE phandle;
    LPVOID         palloc;
    
    DPRINT("GlobalLock( 0x%lX )\n", (ULONG)hMem);

    if (ISPOINTER(hMem))
        return (LPVOID) hMem;

    HeapLock(GetProcessHeap());

    phandle = HANDLE_TO_INTERN(hMem);
    
    if(MAGIC_GLOBAL_USED == phandle->Magic)
    {
        if(GLOBAL_LOCK_MAX > phandle->LockCount)
        {
            phandle->LockCount++;
        }
        palloc = phandle->Pointer;
    }
    else
    {
        DPRINT("GlobalLock: invalid handle\n");
        palloc = (LPVOID) hMem;
    }

    HeapUnlock(GetProcessHeap());
    
    return palloc;
}

/*
 * @implemented
 */
BOOL
STDCALL
GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer)
{
  SYSTEM_BASIC_INFORMATION	SysBasicInfo;
  SYSTEM_PERFORMANCE_INFORMATION	SysPerfInfo;
  ULONG UserMemory;
  NTSTATUS Status;

    DPRINT("GlobalMemoryStatusEx\n");

    if (lpBuffer->dwLength != sizeof(MEMORYSTATUSEX))
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
      }

   Status = ZwQuerySystemInformation(SystemBasicInformation,
                                     &SysBasicInfo,
                                     sizeof(SysBasicInfo),
                                     NULL);
   if (!NT_SUCCESS(Status))
     {
       SetLastErrorByStatus(Status);
       return FALSE;
     }

   Status = ZwQuerySystemInformation(SystemPerformanceInformation,
                                     &SysPerfInfo,
                                     sizeof(SysPerfInfo),
                                     NULL);
   if (!NT_SUCCESS(Status))
     {
       SetLastErrorByStatus(Status);
       return FALSE;
     }            

   Status = ZwQuerySystemInformation(SystemFullMemoryInformation,
                                     &UserMemory,
                                     sizeof(ULONG),
                                     NULL);
   if (!NT_SUCCESS(Status))
     {
       SetLastErrorByStatus(Status);
       return FALSE;
     }            

/*
 * Load percentage 0 thru 100. 0 is good and 100 is bad.
 *
 *	Um = allocated memory / physical memory
 *	Um =      177 MB      /     256 MB        = 69.1%
 *
 *	Mult allocated memory by 100 to move decimal point up.
 */
   lpBuffer->dwMemoryLoad = (SysBasicInfo.NumberOfPhysicalPages -
  			     SysPerfInfo.AvailablePages) * 100 /
    			     SysBasicInfo.NumberOfPhysicalPages;

	DPRINT1("Memory Load: %d\n",lpBuffer->dwMemoryLoad );
    
   lpBuffer->ullTotalPhys = SysBasicInfo.NumberOfPhysicalPages *
   					SysBasicInfo.PhysicalPageSize;
   lpBuffer->ullAvailPhys = SysPerfInfo.AvailablePages *
    					SysBasicInfo.PhysicalPageSize;

	DPRINT("%d\n",SysPerfInfo.AvailablePages );
	DPRINT("%d\n",lpBuffer->ullAvailPhys );

   lpBuffer->ullTotalPageFile = SysPerfInfo.TotalCommitLimit *
    					SysBasicInfo.PhysicalPageSize;

	DPRINT("%d\n",lpBuffer->ullTotalPageFile );

   lpBuffer->ullAvailPageFile = ((SysPerfInfo.TotalCommitLimit -
    					SysPerfInfo.TotalCommittedPages) *
    					SysBasicInfo.PhysicalPageSize);

/* VM available to the calling processes, User Mem? */
   lpBuffer->ullTotalVirtual = SysBasicInfo.HighestUserAddress - 
    					SysBasicInfo.LowestUserAddress;

   lpBuffer->ullAvailVirtual = (lpBuffer->ullTotalVirtual -
    					(UserMemory *
    					 SysBasicInfo.PhysicalPageSize));

	DPRINT("%d\n",lpBuffer->ullAvailVirtual );
	DPRINT("%d\n",UserMemory);
	DPRINT("%d\n",SysBasicInfo.PhysicalPageSize);

/* lol! Memory from beyond! */
   lpBuffer->ullAvailExtendedVirtual = 0;
   return TRUE;
}

/*
 * @implemented
 */
VOID STDCALL
GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
    MEMORYSTATUSEX lpBufferEx;
#if 0    
    if (lpBuffer->dwLength != sizeof(MEMORYSTATUS))
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;      
      }
#endif
    lpBufferEx.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&lpBufferEx))
      {

	 lpBuffer->dwLength 	   = sizeof(MEMORYSTATUS);

         lpBuffer->dwMemoryLoad    = lpBufferEx.dwMemoryLoad;
         lpBuffer->dwTotalPhys     = lpBufferEx.ullTotalPhys;
         lpBuffer->dwAvailPhys     = lpBufferEx.ullAvailPhys;
         lpBuffer->dwTotalPageFile = lpBufferEx.ullTotalPageFile;
         lpBuffer->dwAvailPageFile = lpBufferEx.ullAvailPageFile;
         lpBuffer->dwTotalVirtual  = lpBufferEx.ullTotalVirtual;
         lpBuffer->dwAvailVirtual  = lpBufferEx.ullAvailVirtual;
      }
}


HGLOBAL STDCALL
GlobalReAlloc(HGLOBAL hMem,
	      DWORD dwBytes,
	      UINT uFlags)
{

    LPVOID         palloc = 0;
    HGLOBAL        hnew = 0;
    PGLOBAL_HANDLE phandle = 0;
    ULONG          heap_flags = 0;

    DPRINT("GlobalReAlloc( 0x%lX, 0x%lX, 0x%X )\n", (ULONG)hMem, dwBytes, uFlags);

    hnew = 0;
    
    if (uFlags & GMEM_ZEROINIT)
    {
        heap_flags = HEAP_ZERO_MEMORY;
    }

    HeapLock(GetProcessHeap());
    
    if(uFlags & GMEM_MODIFY) /* modify flags */
    {
        if( ISPOINTER(hMem) && (uFlags & GMEM_MOVEABLE))
        {
            /* make a fixed block moveable
            * actually only NT is able to do this. And it's soo simple
            */
            if (0 == hMem)
            {
                SetLastError( ERROR_NOACCESS );
                hnew = 0;
            }
            else
            {
                dwBytes   = RtlSizeHeap(GetProcessHeap(), 0, (LPVOID) hMem);
                hnew      = GlobalAlloc( uFlags, dwBytes);
                palloc    = GlobalLock(hnew);
                memcpy(palloc, (LPVOID) hMem, dwBytes);
                GlobalUnlock(hnew);
                RtlFreeHeap(GetProcessHeap(),0,hMem);
            }
        }
        else if(ISPOINTER(hMem) && (uFlags & GMEM_DISCARDABLE))
        {
            /* change the flags to make our block "discardable" */
            phandle = HANDLE_TO_INTERN(hMem);
            phandle->Flags = phandle->Flags | (GMEM_DISCARDABLE >> 8);
            hnew = hMem;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            hnew = 0;
        }
    }
    else
    {
        if(ISPOINTER(hMem))
        {
            /* reallocate fixed memory */
            hnew = (HANDLE)RtlReAllocateHeap(GetProcessHeap(), heap_flags, (LPVOID) hMem, dwBytes);
        }
        else
        {
            /* reallocate a moveable block */
            phandle= HANDLE_TO_INTERN(hMem);
#if 0
            if(phandle->LockCount != 0)
            {
                SetLastError(ERROR_INVALID_HANDLE);
            }
            else
#endif
            if (0 != dwBytes)
            {
                hnew = hMem;
                if(phandle->Pointer)
                {
                    palloc = RtlReAllocateHeap(GetProcessHeap(), heap_flags,
                                         phandle->Pointer - HANDLE_SIZE,
                                         dwBytes + HANDLE_SIZE);
                    if (0 == palloc)
                    {
                        hnew = 0;
                    }
                    else
                    {
                        *(PHANDLE)palloc = hMem;
                        phandle->Pointer = palloc + HANDLE_SIZE;
                    }
                }
                else
                {
                    palloc = RtlAllocateHeap(GetProcessHeap(), heap_flags, dwBytes + HANDLE_SIZE);
                    if (0 == palloc)
                    {
                        hnew = 0;
                    }
                    else
                    {
                        *(PHANDLE)palloc = hMem;
                        phandle->Pointer = palloc + HANDLE_SIZE;
                    }
                }
            }
            else
            {
                hnew = hMem;
                if(phandle->Pointer)
                {
                    RtlFreeHeap(GetProcessHeap(), 0, phandle->Pointer - HANDLE_SIZE);
                    phandle->Pointer = 0;
                }
            }
        }
    }
    HeapUnlock(GetProcessHeap());
   
    return hnew;
}


DWORD STDCALL
GlobalSize(HGLOBAL hMem)
{
    SIZE_T         retval  = 0;
    PGLOBAL_HANDLE phandle = 0;
    
    DPRINT("GlobalSize( 0x%lX )\n", (ULONG)hMem);
    
    if(ISPOINTER(hMem)) /*FIXED*/
    {
        retval = RtlSizeHeap(GetProcessHeap(), 0, hMem);
    }
    else /*MOVEABLE*/
    {
        HeapLock(GetProcessHeap());
        
        phandle = HANDLE_TO_INTERN(hMem);
        
        if (MAGIC_GLOBAL_USED == phandle->Magic)
        {
            if (0 != phandle->Pointer)/*NOT DISCARDED*/
            {
                retval = RtlSizeHeap(GetProcessHeap(), 0, phandle->Pointer - HANDLE_SIZE);
                
                if (retval == (SIZE_T)-1) /*RtlSizeHeap failed*/
                {
                    /*
                    **TODO: RtlSizeHeap does not set last error.
                    **      We should choose an error value to set as
                    **      the last error. Which One?
                    */
                    DPRINT("GlobalSize:  RtlSizeHeap failed.\n");
                    retval = 0;
                }
                else /*Everything is ok*/
                {
                    retval = retval - HANDLE_SIZE;
                }
            }
        }
        else
        {
            DPRINT("GlobalSize: invalid handle\n");
        }
        HeapUnlock(GetProcessHeap());
    }
    return retval;
}


/*
 * @implemented
 */
VOID STDCALL
GlobalUnfix(HGLOBAL hMem)
{
   if (hMem != INVALID_HANDLE_VALUE)
     GlobalUnlock(hMem);
}


/*
 * @implemented
 */
BOOL STDCALL
GlobalUnlock(HGLOBAL hMem)
{

   PGLOBAL_HANDLE	phandle;
   BOOL			    locked = FALSE;

   DPRINT("GlobalUnlock( 0x%lX )\n", (ULONG)hMem);

   if(ISPOINTER(hMem))
   {
       SetLastError(ERROR_NOT_LOCKED);
      return FALSE;
   }

   HeapLock(GetProcessHeap());

   phandle = HANDLE_TO_INTERN(hMem);
   if(MAGIC_GLOBAL_USED == phandle->Magic)
   {
      if (0 >= phandle->LockCount)
      {
          locked = FALSE;
          SetLastError(ERROR_NOT_LOCKED);
      }
      else if (GLOBAL_LOCK_MAX > phandle->LockCount)
      {
         phandle->LockCount--;
         locked = (0 == phandle->LockCount) ? TRUE : FALSE;
         SetLastError(NO_ERROR);
      }
   }
   else
   {
      DPRINT("GlobalUnlock: invalid handle\n");
      locked = FALSE;
   }
   HeapUnlock(GetProcessHeap());
   return locked;
}


/*
 * @implemented
 */
BOOL STDCALL
GlobalUnWire(HGLOBAL hMem)
{
   return GlobalUnlock(hMem);
}


/*
 * @implemented
 */
LPVOID STDCALL
GlobalWire(HGLOBAL hMem)
{
   return GlobalLock(hMem);
}

/*
 * @implemented
 */
//HGLOBAL STDCALL
//GlobalDiscard(HGLOBAL hMem)
//{
//    return GlobalReAlloc(hMem, 0, GMEM_MOVEABLE);
//}
/* EOF */
