/* $Id: global.c,v 1.12 2003/07/10 18:50:51 chorns Exp $
 *
 * Win32 Global/Local heap functions (GlobalXXX, LocalXXX).
 * These functions included in Win32 for compatibility with 16 bit Windows
 * Especially the moveable blocks and handles are oldish. 
 * But the ability to directly allocate memory with GPTR and LPTR is widely
 * used.
 */

/*
 * NOTE: Only fixed memory is implemented!!
 */

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

#if 0
#define MAGIC_GLOBAL_USED 0x5342BEEF
#define GLOBAL_LOCK_MAX   0xFF

typedef struct __GLOBAL_LOCAL_HANDLE
{
   ULONG	Magic;
   LPVOID	Pointer;
   BYTE		Flags;
   BYTE		LockCount;
} GLOBAL_HANDLE, LOCAL_HANDLE, *PGLOBAL_HANDLE, *PLOCAL_HANDLE;
#endif

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalAlloc(UINT uFlags,
	    DWORD dwBytes)
{
#if 0
   PGLOBAL_HANDLE	phandle;
   PVOID		palloc;
#endif

   DPRINT("GlobalAlloc( 0x%X, 0x%lX )\n", uFlags, dwBytes);

   if ((uFlags & GMEM_MOVEABLE)==0) /* POINTER */
     {
       if ((uFlags & GMEM_ZEROINIT)==0)
          return ((HGLOBAL)RtlAllocateHeap(hProcessHeap, 0, dwBytes));
	   else
          return ((HGLOBAL)RtlAllocateHeap(hProcessHeap, HEAP_ZERO_MEMORY, dwBytes));
     }
   else  /* HANDLE */
     {
#if 0
      HeapLock(__ProcessHeap);


      phandle=__HeapAllocFragment(__ProcessHeap, 0,  sizeof(GLOBAL_HANDLE));
      if(dwBytes)
      {
         palloc=HeapAlloc(__ProcessHeap, 0, size+sizeof(HANDLE));
         *(PHANDLE)palloc=(HANDLE) &(phandle->Pointer);
         phandle->Pointer=palloc+sizeof(HANDLE);
      }
      else
         phandle->Pointer=NULL;
      phandle->Magic=MAGIC_GLOBAL_USED;
      phandle->Flags=uFlags>>8;
      phandle->LockCount=0;
      HeapUnlock(__ProcessHeap);

      return (HGLOBAL) &(phandle->Pointer);
#endif
	return (HGLOBAL)NULL;
     }
}


/*
 * @implemented
 */
UINT STDCALL
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
   if (hMem != INVALID_HANDLE_VALUE)
     GlobalLock(hMem);
}


/*
 * @unimplemented
 */
UINT STDCALL
GlobalFlags(HGLOBAL hMem)
{
#if 0
   DWORD		retval;
   PGLOBAL_HANDLE	phandle;
#endif

   DPRINT("GlobalFlags( 0x%lX )\n", (ULONG)hMem);

#if 0
   if(((ULONG)hmem%8)==0)
   {
      retval=0;
   }
   else
   {
      HeapLock(__ProcessHeap);
      phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
      if(phandle->Magic==MAGIC_GLOBAL_USED)
      {               
         retval=phandle->LockCount + (phandle->Flags<<8);
         if(phandle->Pointer==0)
            retval|= LMEM_DISCARDED;
      }
      else
      {
         DPRINT("GlobalSize: invalid handle\n");
         retval=0;
      }
      HeapUnlock(__ProcessHeap);
   }
   return retval;
#endif

   return 0;
}


/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalFree(HGLOBAL hMem)
{
#if 0
   PGLOBAL_HANDLE phandle;
#endif

   DPRINT("GlobalFree( 0x%lX )\n", (ULONG)hMem);

   if (((ULONG)hMem % 4) == 0) /* POINTER */
     {
	RtlFreeHeap(hProcessHeap, 0, (PVOID)hMem);
     }
   else /* HANDLE */
     {
#if 0
      HeapLock(__ProcessHeap);
      phandle=(PGLOBAL_HANDLE)(((LPVOID) hMem)-4);
      if(phandle->Magic==MAGIC_GLOBAL_USED)
      {
         HeapLock(__ProcessHeap);
         if(phandle->LockCount!=0)
            SetLastError(ERROR_INVALID_HANDLE);
         if(phandle->Pointer)
            HeapHeapFree(GetProcessHeap(),0,__ProcessHeap, 0, phandle->Pointer-sizeof(HANDLE));
         __HeapFreeFragment(__ProcessHeap, 0, phandle);
      }
      HeapUnlock(__ProcessHeap);
#endif
	hMem = NULL;
     }
   return hMem;
}


/*
 * @implemented
 */
HGLOBAL STDCALL
GlobalHandle(LPCVOID pMem)
{
   DPRINT("GlobalHandle( 0x%lX )\n", (ULONG)pMem);

#if 0
   if(((ULONG)pmem%8)==0) /* FIXED */
      return (HGLOBAL) pmem;
   else  /* MOVEABLE */
      return (HGLOBAL) *(LPVOID *)(pmem-sizeof(HANDLE));
#endif

   return (HGLOBAL)pMem;
}


/*
 * @unimplemented
 */
LPVOID STDCALL
GlobalLock(HGLOBAL hMem)
{
#if 0
   PGLOBAL_HANDLE phandle;
   LPVOID         palloc;
#endif

   DPRINT("GlobalLock( 0x%lX )\n", (ULONG)hMem);

#if 0
   if(((ULONG)hmem%8)==0)
      return (LPVOID) hmem;

   HeapLock(__ProcessHeap);
   phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
   if(phandle->Magic==MAGIC_GLOBAL_USED)
   {
      if(phandle->LockCount<GLOBAL_LOCK_MAX)
         phandle->LockCount++;
      palloc=phandle->Pointer;
   }
   else
   {
      DPRINT("GlobalLock: invalid handle\n");
      palloc=(LPVOID) hmem;
   }
   HeapUnlock(__ProcessHeap);
   return palloc;
#else
   return (LPVOID)hMem;
#endif
}


/*
 * @unimplemented
 */
VOID STDCALL
GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
	NTSTATUS		Status;
	SYSTEM_PERFORMANCE_INFO	Spi;
	QUOTA_LIMITS		Ql;
	VM_COUNTERS		Vmc;
	PIMAGE_NT_HEADERS	ImageNtHeader;

	RtlZeroMemory (lpBuffer, sizeof (MEMORYSTATUS));
	lpBuffer->dwLength = sizeof (MEMORYSTATUS);
	Status = NtQuerySystemInformation (
			SystemPerformanceInformation,
			& Spi,
			sizeof Spi,
			NULL
			);
	/* FIXME: perform computations and fill lpBuffer fields */
	Status = NtQueryInformationProcess (
			GetCurrentProcess(),
			ProcessQuotaLimits,
			& Ql,
			sizeof Ql,
			NULL
			);
	/* FIXME: perform computations and fill lpBuffer fields */
	Status = NtQueryInformationProcess (
			GetCurrentProcess(),
			ProcessVmCounters,
			& Vmc,
			sizeof Vmc,
			NULL
			);
	/* FIXME: perform computations and fill lpBuffer fields */
	ImageNtHeader = RtlImageNtHeader ((PVOID)NtCurrentPeb()->ImageBaseAddress);
	/* FIXME: perform computations and fill lpBuffer fields */
}


HGLOBAL STDCALL
GlobalReAlloc(HGLOBAL hMem,
	      DWORD dwBytes,
	      UINT uFlags)
{
#if 0
   LPVOID		palloc;
   HGLOBAL		hnew;
   PGLOBAL_HANDLE	phandle;
#endif

   DPRINT("GlobalReAlloc( 0x%lX, 0x%lX, 0x%X )\n", (ULONG)hMem, dwBytes, uFlags);

#if 0
   hnew=NULL;
   HeapLock(__ProcessHeap);
   if(flags & GMEM_MODIFY) /* modify flags */
   {
      if( (((ULONG)hmem%8)==0) && (flags & GMEM_MOVEABLE))
      {
         /* make a fixed block moveable
          * actually only NT is able to do this. And it's soo simple
          */
         size=HeapSize(__ProcessHeap, 0, (LPVOID) hmem);
         hnew=GlobalAlloc( flags, size);
         palloc=GlobalLock(hnew);
         memcpy(palloc, (LPVOID) hmem, size);
         GlobalUnlock(hnew);
         GlobalHeapFree(GetProcessHeap(),0,hmem);
      }
      else if((((ULONG)hmem%8) != 0)&&(flags & GMEM_DISCARDABLE))
      {
         /* change the flags to make our block "discardable" */
         phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
         phandle->Flags = phandle->Flags | (GMEM_DISCARDABLE >> 8);
         hnew=hmem;
      }
      else
      {
         SetLastError(ERROR_INVALID_PARAMETER);
         hnew=NULL;
      }
   }
   else
   {
      if(((ULONG)hmem%8)!=0)
      {
         /* reallocate fixed memory */
         hnew=(HANDLE)HeapReAlloc(__ProcessHeap, 0, (LPVOID) hmem, size);
      }
      else
      {
         /* reallocate a moveable block */
         phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
         if(phandle->LockCount!=0)
            SetLastError(ERROR_INVALID_HANDLE);
         else if(size!=0)
         {
            hnew=hmem;
            if(phandle->Pointer)
            {
               palloc=HeapReAlloc(__ProcessHeap, 0,
                                  phandle->Pointer-sizeof(HANDLE),
                                  size+sizeof(HANDLE) );
               phandle->Pointer=palloc+sizeof(HANDLE);
            }
            else
            {
               palloc=HeapAlloc(__ProcessHeap, 0, size+sizeof(HANDLE));
               *(PHANDLE)palloc=hmem;
               phandle->Pointer=palloc+sizeof(HANDLE);
            }
         }
         else
         {
            if(phandle->Pointer)
            {
               HeapHeapFree(GetProcessHeap(),0,__ProcessHeap, 0, phandle->Pointer-sizeof(HANDLE));
               phandle->Pointer=NULL;
            }
         }
      }
   }
   HeapUnlock(__ProcessHeap);
   return hnew;
#else
   return ((HGLOBAL)RtlReAllocateHeap(hProcessHeap, uFlags, (LPVOID)hMem, dwBytes));
#endif
}


DWORD STDCALL
GlobalSize(HGLOBAL hMem)
{
   DWORD		retval;
#if 0
   PGLOBAL_HANDLE	phandle;
#endif

   DPRINT("GlobalSize( 0x%lX )\n", (ULONG)hMem);

   if(((ULONG)hMem % 4) == 0)
   {
      retval = RtlSizeHeap(hProcessHeap, 0, hMem);
   }
   else
   {
#if 0
      HeapLock(__ProcessHeap);
      phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
      if(phandle->Magic==MAGIC_GLOBAL_USED)
      {
         retval=HeapSize(__ProcessHeap, 0, (phandle->Pointer)-sizeof(HANDLE))-4;
      }
      else
      {
         DPRINT("GlobalSize: invalid handle\n");
         retval=0;
      }
      HeapUnlock(__ProcessHeap);
#endif
      retval = 0;
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
 * @unimplemented
 */
BOOL STDCALL
GlobalUnlock(HGLOBAL hMem)
{
#if 0
   PGLOBAL_HANDLE	phandle;
   BOOL			locked;
#endif

   DPRINT("GlobalUnlock( 0x%lX )\n", (ULONG)hMem);

#if 0
   if(((ULONG)hmem%8)==0)
      return FALSE;

   HeapLock(__ProcessHeap);
   phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
   if(phandle->Magic==MAGIC_GLOBAL_USED)
   {
      if((phandle->LockCount<GLOBAL_LOCK_MAX)&&(phandle->LockCount>0))
         phandle->LockCount--;

      locked=(phandle->LockCount==0) ? TRUE : FALSE;
   }
   else
   {
      DPRINT("GlobalUnlock: invalid handle\n");
      locked=FALSE;
   }
   HeapUnlock(__ProcessHeap);
   return locked;
#endif

   return TRUE;
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

/* EOF */
