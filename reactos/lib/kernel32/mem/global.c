/* $Id: global.c,v 1.5 2001/01/20 12:18:54 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>


#define MAGIC_GLOBAL_USED 0x5342BEEF
#define GLOBAL_LOCK_MAX   0xFF

typedef struct __GLOBAL_LOCAL_HANDLE
{
   ULONG	Magic;
   LPVOID	Pointer;
   BYTE		Flags;
   BYTE		LockCount;
} GLOBAL_HANDLE, LOCAL_HANDLE, *PGLOBAL_HANDLE, *PLOCAL_HANDLE;

/*********************************************************************
*                    GlobalAlloc  --  KERNEL32                       *
*********************************************************************/
HGLOBAL WINAPI GlobalAlloc(UINT flags, DWORD size)
{
   PGLOBAL_HANDLE	phandle;
   PVOID		palloc;

   DPRINT("GlobalAlloc( 0x%X, 0x%lX )\n", flags, size );

   if((flags & GMEM_MOVEABLE)==0) /* POINTER */
   {
      palloc=RtlAllocateHeap(hProcessHeap, 0, size);
      return (HGLOBAL) palloc;
   }
   else  /* HANDLE */
   {
#if 0
      HeapLock(__ProcessHeap);


      phandle=__HeapAllocFragment(__ProcessHeap, 0,  sizeof(GLOBAL_HANDLE));
      if(size)
      {
         palloc=HeapAlloc(__ProcessHeap, 0, size+sizeof(HANDLE));
         *(PHANDLE)palloc=(HANDLE) &(phandle->Pointer);
         phandle->Pointer=palloc+sizeof(HANDLE);
      }
      else
         phandle->Pointer=NULL;
      phandle->Magic=MAGIC_GLOBAL_USED;
      phandle->Flags=flags>>8;
      phandle->LockCount=0;
      HeapUnlock(__ProcessHeap);

      return (HGLOBAL) &(phandle->Pointer);
#endif
      return (HGLOBAL)NULL;
   }
}

/*********************************************************************
*                    GlobalLock  --  KERNEL32                        *
*********************************************************************/
#if 0
LPVOID WINAPI GlobalLock(HGLOBAL hmem)
{
   PGLOBAL_HANDLE phandle;
   LPVOID         palloc;

   DPRINT("GlobalLock( 0x%lX )\n", (ULONG) hmem );

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
}
#endif

/*********************************************************************
*                    GlobalUnlock  --  KERNEL32                      *
*********************************************************************/
#if 0
BOOL WINAPI GlobalUnlock(HGLOBAL hmem)
{
   PGLOBAL_HANDLE	phandle;
   BOOL			locked;

   DPRINT("GlobalUnlock( 0x%lX )\n", (ULONG) hmem );

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
}
#endif

/*********************************************************************
*                    GlobalHandle  --  KERNEL32                      *
*********************************************************************/
#if 0
HGLOBAL WINAPI GlobalHandle(LPCVOID pmem)
{
   DPRINT("GlobalHandle( 0x%lX )\n", (ULONG) pmem );

   if(((ULONG)pmem%8)==0) /* FIXED */
      return (HGLOBAL) pmem;
   else  /* MOVEABLE */
      return (HGLOBAL) *(LPVOID *)(pmem-sizeof(HANDLE));
}
#endif

/*********************************************************************
*                    GlobalReAlloc  --  KERNEL32                     *
*********************************************************************/
#if 0
HGLOBAL WINAPI GlobalReAlloc(HGLOBAL hmem, DWORD size, UINT flags)
{
   LPVOID		palloc;
   HGLOBAL		hnew;
   PGLOBAL_HANDLE	phandle;

   DPRINT("GlobalReAlloc( 0x%lX, 0x%lX, 0x%X )\n", (ULONG) hmem, size, flags );

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
}
#endif

/*********************************************************************
*                    GlobalFree  --  KERNEL32                        *
*********************************************************************/
HGLOBAL WINAPI GlobalFree(HGLOBAL hmem)
{
#if 0
   PGLOBAL_HANDLE phandle;
#endif

   DPRINT("GlobalFree( 0x%lX )\n", (ULONG) hmem );

   if (((ULONG)hmem % 4) == 0) /* POINTER */
     {
	RtlFreeHeap(hProcessHeap, 0, (LPVOID)hmem);
     }
   else /* HANDLE */
     {
#if 0
      HeapLock(__ProcessHeap);
      phandle=(PGLOBAL_HANDLE)(((LPVOID) hmem)-4);
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
	hmem = NULL;
     }
   return hmem;
}

/*********************************************************************
*                    GlobalSize  --  KERNEL32                        *
*********************************************************************/
DWORD WINAPI GlobalSize(HGLOBAL hmem)
{
   DWORD		retval;
   PGLOBAL_HANDLE	phandle;

   DPRINT("GlobalSize( 0x%lX )\n", (ULONG) hmem );

   if(((ULONG)hmem % 4) == 0)
   {
      retval = RtlSizeHeap(hProcessHeap, 0, hmem);
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

/*********************************************************************
*                    GlobalWire  --  KERNEL32                        *
*********************************************************************/
#if 0
LPVOID WINAPI GlobalWire(HGLOBAL hmem)
{
   return GlobalLock( hmem );
}
#endif

/*********************************************************************
*                    GlobalUnWire  --  KERNEL32                      *
*********************************************************************/
#if 0
BOOL WINAPI GlobalUnWire(HGLOBAL hmem)
{
   return GlobalUnlock( hmem);
}
#endif

/*********************************************************************
*                    GlobalFix  --  KERNEL32                         *
*********************************************************************/
#if 0
VOID WINAPI GlobalFix(HGLOBAL hmem)
{
   GlobalLock( hmem );
}
#endif

/*********************************************************************
*                    GlobalUnfix  --  KERNEL32                       *
*********************************************************************/
#if 0
VOID WINAPI GlobalUnfix(HGLOBAL hmem)
{
   GlobalUnlock( hmem);
}
#endif

/*********************************************************************
*                    GlobalFlags  --  KERNEL32                       *
*********************************************************************/
#if 0
UINT WINAPI GlobalFlags(HGLOBAL hmem)
{
   return LocalFlags( (HLOCAL) hmem);
}
#endif

/* EOF */
