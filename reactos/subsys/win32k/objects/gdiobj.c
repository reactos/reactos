/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.80 2004/12/19 00:03:56 royce Exp $
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>

#ifdef __USE_W32API
/* F*(&#$ header mess!!!! */
HANDLE
STDCALL PsGetProcessId(
   	PEPROCESS	Process
	);
#endif /* __USE_W32API */




#define GDI_ENTRY_TO_INDEX(ht, e)                                              \
  (((ULONG_PTR)(e) - (ULONG_PTR)&((ht)->Entries[0])) / sizeof(GDI_TABLE_ENTRY))
#define GDI_HANDLE_GET_ENTRY(HandleTable, h)                                   \
  (&(HandleTable)->Entries[GDI_HANDLE_GET_INDEX((h))])

#define GDIBdyToHdr(body)                                                      \
  ((PGDIOBJHDR)(body) - 1)
#define GDIHdrToBdy(hdr)                                                       \
  (PGDIOBJ)((PGDIOBJHDR)(hdr) + 1)

/* apparently the first 10 entries are never used in windows as they are empty */
#define RESERVE_ENTRIES_COUNT 10

typedef struct _GDI_HANDLE_TABLE
{
  PPAGED_LOOKASIDE_LIST LookasideLists;

  SLIST_HEADER FreeEntriesHead;
  SLIST_ENTRY FreeEntries[((GDI_HANDLE_COUNT * sizeof(GDI_TABLE_ENTRY)) << 3) /
                          (sizeof(SLIST_ENTRY) << 3)];

  GDI_TABLE_ENTRY Entries[GDI_HANDLE_COUNT];
} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

typedef struct
{
  ULONG Type;
  ULONG Size;
  GDICLEANUPPROC CleanupProc;
} GDI_OBJ_INFO, *PGDI_OBJ_INFO;

/*
 * Dummy GDI Cleanup Callback
 */
BOOL INTERNAL_CALL
GDI_CleanupDummy(PVOID ObjectBody)
{
  return TRUE;
}

/* Testing shows that regions are the most used GDIObj type,
   so put that one first for performance */
const
GDI_OBJ_INFO ObjInfo[] =
{
   /* Type */                   /* Size */             /* CleanupProc */
  {GDI_OBJECT_TYPE_REGION,      sizeof(ROSRGNDATA),    RGNDATA_Cleanup},
  {GDI_OBJECT_TYPE_BITMAP,      sizeof(BITMAPOBJ),     BITMAP_Cleanup},
  {GDI_OBJECT_TYPE_DC,          sizeof(DC),            DC_Cleanup},
  {GDI_OBJECT_TYPE_PALETTE,     sizeof(PALGDI),        PALETTE_Cleanup},
  {GDI_OBJECT_TYPE_BRUSH,       sizeof(GDIBRUSHOBJ),   BRUSH_Cleanup},
  {GDI_OBJECT_TYPE_PEN,         sizeof(GDIBRUSHOBJ),   GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_FONT,        sizeof(TEXTOBJ),       GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_DCE,         sizeof(DCE),           DCE_Cleanup},
/*{GDI_OBJECT_TYPE_DIRECTDRAW,  sizeof(DD_DIRECTDRAW), DD_Cleanup},
  {GDI_OBJECT_TYPE_DD_SURFACE,  sizeof(DD_SURFACE),    DDSURF_Cleanup},*/
  {GDI_OBJECT_TYPE_EXTPEN,      0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_METADC,      0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_METAFILE,    0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_ENHMETAFILE, 0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_ENHMETADC,   0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_MEMDC,       0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_EMF,         0,                     GDI_CleanupDummy}
};

#define OBJTYPE_COUNT (sizeof(ObjInfo) / sizeof(ObjInfo[0]))

static PGDI_HANDLE_TABLE HandleTable = NULL;
static LARGE_INTEGER ShortDelay;

#define DelayExecution() \
  DPRINT("%s:%i: Delay\n", __FILE__, __LINE__); \
  KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay)

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
static PGDI_HANDLE_TABLE INTERNAL_CALL
GDIOBJ_iAllocHandleTable(VOID)
{
  PGDI_HANDLE_TABLE handleTable;
  UINT ObjType;
  UINT i;
  PGDI_TABLE_ENTRY Entry;

  handleTable = ExAllocatePoolWithTag(NonPagedPool, sizeof(GDI_HANDLE_TABLE), TAG_GDIHNDTBLE);
  ASSERT( handleTable );
  RtlZeroMemory(handleTable, sizeof(GDI_HANDLE_TABLE));

  /*
   * initialize the free entry cache
   */
  InitializeSListHead(&handleTable->FreeEntriesHead);
  Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
  for(i = GDI_HANDLE_COUNT - 1; i >= RESERVE_ENTRIES_COUNT; i--)
  {
    InterlockedPushEntrySList(&handleTable->FreeEntriesHead, &handleTable->FreeEntries[i]);
  }

  handleTable->LookasideLists = ExAllocatePoolWithTag(NonPagedPool,
                                                      OBJTYPE_COUNT * sizeof(PAGED_LOOKASIDE_LIST),
                                                      TAG_GDIHNDTBLE);
  if(handleTable->LookasideLists == NULL)
  {
    ExFreePool(handleTable);
    return NULL;
  }

  for(ObjType = 0; ObjType < OBJTYPE_COUNT; ObjType++)
  {
    ExInitializePagedLookasideList(handleTable->LookasideLists + ObjType, NULL, NULL, 0,
                                   ObjInfo[ObjType].Size + sizeof(GDIOBJHDR), TAG_GDIOBJ, 0);
  }

  ShortDelay.QuadPart = -5000LL; /* FIXME - 0.5 ms? */

  return handleTable;
}

static inline PPAGED_LOOKASIDE_LIST
FindLookasideList(DWORD ObjectType)
{
  int Index;

  for (Index = 0; Index < OBJTYPE_COUNT; Index++)
  {
    if (ObjInfo[Index].Type == ObjectType)
    {
      return HandleTable->LookasideLists + Index;
    }
  }

  DPRINT1("Can't find lookaside list for object type 0x%08x\n", ObjectType);

  return NULL;
}

static inline BOOL
RunCleanupCallback(PGDIOBJ pObj, DWORD ObjectType)
{
  int Index;

  for (Index = 0; Index < OBJTYPE_COUNT; Index++)
  {
    if (ObjInfo[Index].Type == ObjectType)
    {
      return ((GDICLEANUPPROC)ObjInfo[Index].CleanupProc)(pObj);
    }
  }

  DPRINT1("Can't find cleanup callback for object type 0x%08x\n", ObjectType);
  return TRUE;
}

static inline ULONG
GetObjectSize(DWORD ObjectType)
{
  int Index;

  for (Index = 0; Index < OBJTYPE_COUNT; Index++)
  {
    if (ObjInfo[Index].Type == ObjectType)
    {
      return ObjInfo[Index].Size;
    }
  }

  DPRINT1("Can't find size for object type 0x%08x\n", ObjectType);
  return 0;
}

#ifdef DBG

static int leak_reported = 0;
#define GDI_STACK_LEVELS 4
static ULONG GDIHandleAllocator[GDI_STACK_LEVELS][GDI_HANDLE_COUNT];
struct DbgOpenGDIHandle
{
	ULONG loc;
	int count;
};
#define H 1024
static struct DbgOpenGDIHandle h[H];

void IntDumpHandleTable ( int which )
{
	int i, n = 0, j;

	/*  step through GDI handle table and find out who our culprit is... */
	for ( i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++ )
	{
		for ( j = 0; j < n; j++ )
		{
			if ( GDIHandleAllocator[which][i] == h[j].loc )
				break;
		}
		if ( j < H )
		{
			if ( j == n )
			{
				h[j].loc = GDIHandleAllocator[which][i];
				h[j].count = 1;
				n = n + 1;
			}
			else
				h[j].count++;
		}
	}
	/* bubble sort time! weeeeee!! */
	for ( i = 0; i < n-1; i++ )
	{
		if ( h[i].count < h[i+1].count )
		{
			struct DbgOpenGDIHandle t;
			t.loc = h[i+1].loc;
			t.count = h[i+1].count;
			h[i+1].loc = h[i].loc;
			h[i+1].count = h[i].count;
			j = i;
			while ( j > 0 && h[j-1].count < t.count )
				j--;
			h[j] = t;
		}
	}
	/* print the first 30 offenders... */
	DbgPrint ( "Worst GDI Handle leak offenders - stack trace level %i (out of %i unique locations):\n", which, n );
	for ( i = 0; i < 30 && i < n; i++ )
	{
		DbgPrint ( "\t" );
		if ( !KeRosPrintAddress ( (PVOID)h[i].loc ) )
			DbgPrint ( "<%X>", h[i].loc );
		DbgPrint ( " (%i allocations)\n", h[i].count );
	}
}
#endif /* DBG */

/*!
 * Allocate memory for GDI object and return handle to it.
 *
 * \param ObjectType - type of object \ref GDI object types
 *
 * \return Handle of the allocated object.
 *
 * \note Use GDIOBJ_Lock() to obtain pointer to the new object.
 * \todo return the object pointer and lock it by default.
*/
HGDIOBJ INTERNAL_CALL
#ifdef GDI_DEBUG
GDIOBJ_AllocObjDbg(const char* file, int line, ULONG ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_AllocObj(ULONG ObjectType)
#endif /* GDI_DEBUG */
{
  PW32PROCESS W32Process;
  PGDIOBJHDR  newObject;
  PPAGED_LOOKASIDE_LIST LookasideList;
  LONG CurrentProcessId, LockedProcessId;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  W32Process = PsGetWin32Process();
  /* HACK HACK HACK: simplest-possible quota implementation - don't allow a process
     to take too many GDI objects, itself. */
  if ( W32Process && W32Process->GDIObjects >= 0x2710 )
    return NULL;

  ASSERT(ObjectType != GDI_OBJECT_TYPE_DONTCARE);

  LookasideList = FindLookasideList(ObjectType);
  if(LookasideList != NULL)
  {
    newObject = ExAllocateFromPagedLookasideList(LookasideList);
    if(newObject != NULL)
    {
      PSLIST_ENTRY FreeEntry;
      PGDI_TABLE_ENTRY Entry;
      PGDIOBJ ObjectBody;
      LONG TypeInfo;

      /* shift the process id to the left so we can use the first bit to lock
         the object.
         FIXME - don't shift once ROS' PIDs match with nt! */
      CurrentProcessId = (LONG)PsGetCurrentProcessId() << 1;
      LockedProcessId = CurrentProcessId | 0x1;

      newObject->LockingThread = NULL;
      newObject->Locks = 0;

#ifdef GDI_DEBUG
      newObject->createdfile = file;
      newObject->createdline = line;
      newObject->lockfile = NULL;
      newObject->lockline = 0;
#endif

      ObjectBody = GDIHdrToBdy(newObject);

      RtlZeroMemory(ObjectBody, GetObjectSize(ObjectType));

      TypeInfo = (ObjectType & 0xFFFF0000) | (ObjectType >> 16);

      FreeEntry = InterlockedPopEntrySList(&HandleTable->FreeEntriesHead);
      if(FreeEntry != NULL)
      {
        LONG PrevProcId;
        UINT Index;
        HGDIOBJ Handle;

        /* calculate the entry from the address of the entry in the free slot array */
        Index = ((ULONG_PTR)FreeEntry - (ULONG_PTR)&HandleTable->FreeEntries[0]) /
                sizeof(HandleTable->FreeEntries[0]);
        Entry = &HandleTable->Entries[Index];
        Handle = (HGDIOBJ)((Index & 0xFFFF) | (ObjectType & 0xFFFF0000));

LockHandle:
        PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, 0);
        if(PrevProcId == 0)
        {
          ASSERT(Entry->KernelData == NULL);

          Entry->KernelData = ObjectBody;

          /* we found a free entry, no need to exchange this field atomically
             since we're holding the lock */
          Entry->Type = TypeInfo;

          /* unlock the entry */
          InterlockedExchange(&Entry->ProcessId, CurrentProcessId);

#ifdef DBG
          {
            PULONG Frame;
			int which;
#if defined __GNUC__
            __asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
#elif defined(_MSC_VER)
            __asm mov [Frame], ebp
#endif
			Frame = (PULONG)Frame[0]; /* step out of AllocObj() */
			for ( which = 0; which < GDI_STACK_LEVELS && Frame[1] != 0 && Frame[1] != 0xDEADBEEF; which++ )
			{
	            GDIHandleAllocator[which][Index] = Frame[1]; /* step out of AllocObj() */
				Frame = ((PULONG)Frame[0]);
			}
			for ( ; which < GDI_STACK_LEVELS; which++ )
				GDIHandleAllocator[which][Index] = 0xDEADBEEF;
          }
#endif /* DBG */

          if(W32Process != NULL)
          {
            InterlockedIncrement(&W32Process->GDIObjects);
          }

          DPRINT("GDIOBJ_AllocObj: 0x%x ob: 0x%x\n", Handle, ObjectBody);
          return Handle;
        }
        else
        {
#ifdef GDI_DEBUG
          if(++Attempts > 20)
          {
            DPRINT1("[%d]Waiting on 0x%x\n", Attempts, Handle);
          }
#endif
          /* damn, someone is trying to lock the object even though it doesn't
             eve nexist anymore, wait a little and try again!
             FIXME - we shouldn't loop forever! Give up after some time! */
          DelayExecution();
          /* try again */
          goto LockHandle;
        }
      }

      ExFreeToPagedLookasideList(LookasideList, newObject);
      DPRINT1("Failed to insert gdi object into the handle table, no handles left!\n");
#ifdef DBG
	  if ( !leak_reported )
	  {
		  DPRINT1("reporting gdi handle abusers:\n");
		  int which;
		  for ( which = 0; which < GDI_STACK_LEVELS; which++ )
		      IntDumpHandleTable(which);
		  leak_reported = 1;
	  }
	  else
	  {
		  DPRINT1("gdi handle abusers already reported!\n");
	  }
#endif /* DBG */
    }
    else
    {
      DPRINT1("Not enough memory to allocate gdi object!\n");
    }
  }
  else
  {
    DPRINT1("Failed to find lookaside list for object type 0x%x\n", ObjectType);
  }
  return NULL;
}

/*!
 * Free memory allocated for the GDI object. For each object type this function calls the
 * appropriate cleanup routine.
 *
 * \param hObj       - handle of the object to be deleted.
 *
 * \return Returns TRUE if succesful.
 * \return Returns FALSE if the cleanup routine returned FALSE or the object doesn't belong
 * to the calling process.
*/
BOOL INTERNAL_CALL
#ifdef GDI_DEBUG
GDIOBJ_FreeObjDbg(const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_FreeObj(HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
  PGDI_TABLE_ENTRY Entry;
  PPAGED_LOOKASIDE_LIST LookasideList;
  LONG ProcessId, LockedProcessId, PrevProcId, ExpectedType;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_FreeObj: hObj: 0x%08x\n", hObj);

  if(GDI_HANDLE_IS_STOCKOBJ(hObj))
  {
    DPRINT1("GDIOBJ_FreeObj() failed, can't delete stock object handle: 0x%x !!!\n", hObj);
#ifdef GDI_DEBUG
    DPRINT1("-> called from %s:%i\n", file, line);
#endif
    return FALSE;
  }

  /* shift the process id to the left so we can use the first bit to lock the object.
     FIXME - don't shift once ROS' PIDs match with nt! */
  ProcessId = (LONG)PsGetCurrentProcessId() << 1;
  LockedProcessId = ProcessId | 0x1;

  ExpectedType = ((ObjectType != GDI_OBJECT_TYPE_DONTCARE) ? ObjectType : 0);

  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);

LockHandle:
  /* lock the object, we must not delete global objects, so don't exchange the locking
     process ID to zero when attempting to lock a global object... */
  PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    if(Entry->Type != 0 && Entry->KernelData != NULL && (ExpectedType == 0 || ((Entry->Type << 16) == ExpectedType)))
    {
      PGDIOBJHDR GdiHdr;

      GdiHdr = GDIBdyToHdr(Entry->KernelData);

      if(GdiHdr->LockingThread == NULL)
      {
        BOOL Ret;
        PW32PROCESS W32Process = PsGetWin32Process();
        ULONG Type = Entry->Type << 16;

        /* Clear the type field so when unlocking the handle it gets finally deleted */
        Entry->Type = 0;
        Entry->KernelData = NULL;

        /* unlock the handle slot */
        InterlockedExchange(&Entry->ProcessId, 0);

        /* push this entry to the free list */
        InterlockedPushEntrySList(&HandleTable->FreeEntriesHead,
                                  &HandleTable->FreeEntries[GDI_ENTRY_TO_INDEX(HandleTable, Entry)]);

        if(W32Process != NULL)
        {
          InterlockedDecrement(&W32Process->GDIObjects);
        }

        /* call the cleanup routine. */
        Ret = RunCleanupCallback(GDIHdrToBdy(GdiHdr), Type);

        /* Now it's time to free the memory */
        LookasideList = FindLookasideList(Type);
        if(LookasideList != NULL)
        {
          ExFreeToPagedLookasideList(LookasideList, GdiHdr);
        }

        return Ret;
      }
      else
      {
        /* the object is currently locked. just clear the type field so when the
           object gets unlocked it will be finally deleted from the table. */
        Entry->Type = 0;

        /* unlock the handle slot */
        InterlockedExchange(&Entry->ProcessId, 0);

        /* report a successful deletion as the object is actually removed from the table */
        return TRUE;
      }
    }
    else
    {
      if(Entry->Type != 0)
      {
        DPRINT1("Attempted to delete object 0x%x, type mismatch (0x%x : 0x%x)\n", hObj, ObjectType, ExpectedType);
      }
      else
      {
        DPRINT1("Attempted to delete object 0x%x which was already deleted!\n", hObj);
      }
      InterlockedExchange(&Entry->ProcessId, PrevProcId);
    }
  }
  else if(PrevProcId == LockedProcessId)
  {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting on 0x%x\n", Attempts, hObj);
    }
#endif
    /* the object is currently locked, wait some time and try again.
       FIXME - we shouldn't loop forever! Give up after some time! */
    DelayExecution();
    /* try again */
    goto LockHandle;
  }
  else
  {
    if((PrevProcId >> 1) == 0)
    {
      DPRINT1("Attempted to free global gdi handle 0x%x, caller needs to get ownership first!!!", hObj);
    }
    else
    {
      DPRINT1("Attempted to free foreign handle: 0x%x Owner: 0x%x from Caller: 0x%x\n", hObj, PrevProcId >> 1, ProcessId >> 1);
    }
#ifdef GDI_DEBUG
    DPRINT1("-> called from %s:%i\n", file, line);
#endif
  }

  return FALSE;
}

/*!
 * Lock multiple objects. Use this function when you need to lock multiple objects and some of them may be
 * duplicates. You should use this function to avoid trying to lock the same object twice!
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and ObjectType fields.
 * \param	nObj	number of objects to lock
 * \return	for each entry in pList this function sets pObj field to point to the object.
 *
 * \note this function uses an O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL INTERNAL_CALL
GDIOBJ_LockMultipleObj(PGDIMULTILOCK pList, INT nObj)
{
  INT i, j;
  ASSERT( pList );
  /* FIXME - check for "invalid" handles */
  /* go through the list checking for duplicate objects */
  for (i = 0; i < nObj; i++)
    {
      pList[i].pObj = NULL;
      for (j = 0; j < i; j++)
	{
	  if (pList[i].hObj == pList[j].hObj)
	    {
	      /* already locked, so just copy the pointer to the object */
	      pList[i].pObj = pList[j].pObj;
	      break;
	    }
	}

      if (NULL == pList[i].pObj)
	{
	  /* object hasn't been locked, so lock it. */
	  if (NULL != pList[i].hObj)
	    {
	      pList[i].pObj = GDIOBJ_LockObj(pList[i].hObj, pList[i].ObjectType);
	    }
	}
    }

  return TRUE;
}

/*!
 * Unlock multiple objects. Use this function when you need to unlock multiple objects and some of them may be
 * duplicates.
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and ObjectType fields.
 * \param	nObj	number of objects to lock
 *
 * \note this function uses O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL INTERNAL_CALL
GDIOBJ_UnlockMultipleObj(PGDIMULTILOCK pList, INT nObj)
{
  INT i, j;
  ASSERT(pList);

  /* go through the list checking for duplicate objects */
  for (i = 0; i < nObj; i++)
    {
      if (NULL != pList[i].pObj)
	{
	  for (j = i + 1; j < nObj; j++)
	    {
	      if ((pList[i].pObj == pList[j].pObj))
		{
		  /* set the pointer to zero for all duplicates */
		  pList[j].pObj = NULL;
		}
	    }
	  GDIOBJ_UnlockObj(pList[i].hObj);
	  pList[i].pObj = NULL;
	}
    }

  return TRUE;
}

/*!
 * Initialization of the GDI object engine.
*/
VOID INTERNAL_CALL
InitGdiObjectHandleTable (VOID)
{
  DPRINT("InitGdiObjectHandleTable\n");

  HandleTable = GDIOBJ_iAllocHandleTable();
  DPRINT("HandleTable: %x\n", HandleTable);
}

/*!
 * Delete GDI object
 * \param	hObject object handle
 * \return	if the function fails the returned value is FALSE.
*/
BOOL STDCALL
NtGdiDeleteObject(HGDIOBJ hObject)
{
  DPRINT("NtGdiDeleteObject handle 0x%08x\n", hObject);

  return NULL != hObject
         ? GDIOBJ_FreeObj(hObject, GDI_OBJECT_TYPE_DONTCARE) : FALSE;
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL INTERNAL_CALL
GDI_CleanupForProcess (struct _EPROCESS *Process)
{
  PGDI_TABLE_ENTRY Entry, End;
  PEPROCESS CurrentProcess;
  PW32PROCESS W32Process;
  LONG ProcId;
  ULONG Index = RESERVE_ENTRIES_COUNT;

  DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Pid);
  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
    {
      KeAttachProcess(Process);
    }
  W32Process = Process->Win32Process;
  ASSERT(W32Process);

  if(W32Process->GDIObjects > 0)
  {
    /* FIXME - Instead of building the handle here and delete it using GDIOBJ_FreeObj
               we should delete it directly here! */
    ProcId = ((LONG)Process->UniqueProcessId << 1);

    End = &HandleTable->Entries[GDI_HANDLE_COUNT];
    for(Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
        Entry < End;
        Entry++, Index++)
    {
      /* ignore the lock bit */
      if((Entry->ProcessId & ~0x1) == ProcId && Entry->Type != 0)
      {
        HGDIOBJ ObjectHandle;

        /* Create the object handle for the entry, the upper 16 bit of the
           Type field includes the type of the object including the stock
           object flag - but since stock objects don't have a process id we can
           simply ignore this fact here. */
        ObjectHandle = (HGDIOBJ)(Index | (Entry->Type & 0xFFFF0000));

        if(GDIOBJ_FreeObj(ObjectHandle, GDI_OBJECT_TYPE_DONTCARE) &&
           W32Process->GDIObjects == 0)
        {
          /* there are no more gdi handles for this process, bail */
          break;
        }
      }
    }
  }

  if (CurrentProcess != Process)
    {
      KeDetachProcess();
    }

  DPRINT("Completed cleanup for process %d\n", Pid);

  return TRUE;
}

/*!
 * Return pointer to the object by handle.
 *
 * \param hObj 		Object handle
 * \return		Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Get rid of the ObjectType parameter!
*/
PGDIOBJ INTERNAL_CALL
#ifdef GDI_DEBUG
GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_LockObj (HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
  PGDI_TABLE_ENTRY Entry;
  PETHREAD Thread;
  LONG ProcessId, LockedProcessId, PrevProcId, ExpectedType;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_LockObj: hObj: 0x%08x\n", hObj);

  Thread = PsGetCurrentThread();

  /* shift the process id to the left so we can use the first bit to lock the object.
     FIXME - don't shift once ROS' PIDs match with nt! */
  ProcessId = (LONG)PsGetCurrentProcessId() << 1;
  LockedProcessId = ProcessId | 0x1;

  ExpectedType = ((ObjectType != GDI_OBJECT_TYPE_DONTCARE) ? ObjectType : 0);

  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);

LockHandle:
  /* lock the object, we must not delete stock objects, so don't check!!! */
  PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    LONG EntryType = Entry->Type << 16;

    /* we're locking an object that belongs to our process or it's a global
       object if ProcessId == 0 here. ProcessId can only be 0 here if it previously
       failed to lock the object and it turned out to be a global object. */
    if(EntryType != 0 && Entry->KernelData != NULL && (ExpectedType == 0 || (EntryType == ExpectedType)))
    {
      PETHREAD PrevThread;
      PGDIOBJHDR GdiHdr;

      GdiHdr = GDIBdyToHdr(Entry->KernelData);

      /* save the pointer to the calling thread so we know it was this thread
         that locked the object. There's no need to do this atomically as we're
         holding the lock of the handle slot, but this way it's easier ;) */
      PrevThread = InterlockedCompareExchangePointer(&GdiHdr->LockingThread, Thread, NULL);

      if(PrevThread == NULL || PrevThread == Thread)
      {
        if(++GdiHdr->Locks == 1)
        {
#ifdef GDI_DEBUG
          GdiHdr->lockfile = file;
          GdiHdr->lockline = line;
#endif
        }

        InterlockedExchange(&Entry->ProcessId, PrevProcId);

        /* we're done, return the object body */
        return GDIHdrToBdy(GdiHdr);
      }
      else
      {
        InterlockedExchange(&Entry->ProcessId, PrevProcId);

#ifdef GDI_DEBUG
        if(++Attempts > 20)
        {
          DPRINT1("[%d]Waiting at %s:%i as 0x%x on 0x%x\n", Attempts, file, line, Thread, PrevThread);
        }
#endif

        DelayExecution();
        goto LockHandle;
      }
    }
    else
    {
      InterlockedExchange(&Entry->ProcessId, PrevProcId);

      if(EntryType == 0)
      {
        DPRINT1("Attempted to lock object 0x%x that is deleted!\n", hObj);
        KeRosDumpStackFrames ( NULL, 20 );
      }
      else
      {
        DPRINT1("Attempted to lock object 0x%x, type mismatch (0x%x : 0x%x)\n", hObj, EntryType, ExpectedType);
        KeRosDumpStackFrames ( NULL, 20 );
      }
#ifdef GDI_DEBUG
      DPRINT1("-> called from %s:%i\n", file, line);
#endif
    }
  }
  else if(PrevProcId == LockedProcessId)
  {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting from %s:%i on 0x%x\n", Attempts, file, line, hObj);
    }
#endif
    /* the handle is currently locked, wait some time and try again.
       FIXME - we shouldn't loop forever! Give up after some time! */
    DelayExecution();
    /* try again */
    goto LockHandle;
  }
  else if((PrevProcId & ~0x1) == 0)
  {
    /* we're trying to lock a global object, change the ProcessId to 0 and try again */
    ProcessId = 0x0;
    LockedProcessId = ProcessId |0x1;

    goto LockHandle;
  }
  else
  {
    DPRINT1("Attempted to lock foreign handle: 0x%x, Owner: 0x%x locked: 0x%x Caller: 0x%x, stockobj: 0x%x\n", hObj, PrevProcId >> 1, PrevProcId & 0x1, PsGetCurrentProcessId(), GDI_HANDLE_IS_STOCKOBJ(hObj));
    KeRosDumpStackFrames ( NULL, 20 );
#ifdef GDI_DEBUG
    DPRINT1("-> called from %s:%i\n", file, line);
#endif
  }

  return NULL;
}


/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param hObj 		Object handle
 *
 * \note This function performs delayed cleanup. If the object is locked when GDI_FreeObj() is called
 * then \em this function frees the object when reference count is zero.
*/
BOOL INTERNAL_CALL
#ifdef GDI_DEBUG
GDIOBJ_UnlockObjDbg (const char* file, int line, HGDIOBJ hObj)
#else /* !GDI_DEBUG */
GDIOBJ_UnlockObj (HGDIOBJ hObj)
#endif /* GDI_DEBUG */
{
  PGDI_TABLE_ENTRY Entry;
  PETHREAD Thread;
  LONG ProcessId, LockedProcessId, PrevProcId;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_UnlockObj: hObj: 0x%08x\n", hObj);
  Thread = PsGetCurrentThread();

  /* shift the process id to the left so we can use the first bit to lock the object.
     FIXME - don't shift once ROS' PIDs match with nt! */
  ProcessId = (LONG)PsGetCurrentProcessId() << 1;
  LockedProcessId = ProcessId | 0x1;

  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);

LockHandle:
  /* lock the handle, we must not delete stock objects, so don't check!!! */
  PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    /* we're unlocking an object that belongs to our process or it's a global
       object if ProcessId == 0 here. ProcessId can only be 0 here if it previously
       failed to lock the object and it turned out to be a global object. */
    if(Entry->KernelData != NULL)
    {
      PETHREAD PrevThread;
      PGDIOBJHDR GdiHdr;

      GdiHdr = GDIBdyToHdr(Entry->KernelData);

      PrevThread = GdiHdr->LockingThread;
      if(PrevThread == Thread)
      {
        BOOL Ret;

        if(--GdiHdr->Locks == 0)
        {
          GdiHdr->LockingThread = NULL;

#ifdef GDI_DEBUG
          GdiHdr->lockfile = NULL;
          GdiHdr->lockline = 0;
#endif
        }

        if(Entry->Type == 0 && GdiHdr->Locks == 0)
        {
          PPAGED_LOOKASIDE_LIST LookasideList;
          PW32PROCESS W32Process = PsGetWin32Process();
          DWORD Type = GDI_HANDLE_GET_TYPE(hObj);

          ASSERT(ProcessId != 0); /* must not delete a global handle!!!! */

          /* we should delete the handle */
          Entry->KernelData = NULL;
          InterlockedExchange(&Entry->ProcessId, 0);

          InterlockedPushEntrySList(&HandleTable->FreeEntriesHead,
                                    &HandleTable->FreeEntries[GDI_ENTRY_TO_INDEX(HandleTable, Entry)]);

          if(W32Process != NULL)
          {
            InterlockedDecrement(&W32Process->GDIObjects);
          }

          /* call the cleanup routine. */
          Ret = RunCleanupCallback(GDIHdrToBdy(GdiHdr), Type);

          /* Now it's time to free the memory */
          LookasideList = FindLookasideList(Type);
          if(LookasideList != NULL)
          {
            ExFreeToPagedLookasideList(LookasideList, GdiHdr);
          }
        }
        else
        {
          /* remove the handle slot lock */
          InterlockedExchange(&Entry->ProcessId, PrevProcId);
          Ret = TRUE;
        }

        /* we're done*/
        return Ret;
      }
#ifdef GDI_DEBUG
      else if(PrevThread != NULL)
      {
        DPRINT1("Attempted to unlock object 0x%x, previously locked by other thread (0x%x) from %s:%i (called from %s:%i)\n",
                hObj, PrevThread, GdiHdr->lockfile, GdiHdr->lockline, file, line);
        InterlockedExchange(&Entry->ProcessId, PrevProcId);
      }
#endif
      else
      {
#ifdef GDI_DEBUG
        if(++Attempts > 20)
        {
          DPRINT1("[%d]Waiting at %s:%i as 0x%x on 0x%x\n", Attempts, file, line, Thread, PrevThread);
        }
#endif
        /* FIXME - we should give up after some time unless we want to wait forever! */
        InterlockedExchange(&Entry->ProcessId, PrevProcId);

        DelayExecution();
        goto LockHandle;
      }
    }
    else
    {
      InterlockedExchange(&Entry->ProcessId, PrevProcId);
      DPRINT1("Attempted to unlock object 0x%x that is deleted!\n", hObj);
    }
  }
  else if(PrevProcId == LockedProcessId)
  {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting from %s:%i on 0x%x\n", Attempts, file, line, hObj);
    }
#endif
    /* the handle is currently locked, wait some time and try again.
       FIXME - we shouldn't loop forever! Give up after some time! */
    DelayExecution();
    /* try again */
    goto LockHandle;
  }
  else if((PrevProcId & ~0x1) == 0)
  {
    /* we're trying to unlock a global object, change the ProcessId to 0 and try again */
    ProcessId = 0x0;
    LockedProcessId = ProcessId |0x1;

    goto LockHandle;
  }
  else
  {
    DPRINT1("Attempted to unlock foreign handle: 0x%x, Owner: 0x%x locked: 0x%x Caller: 0x%x, stockobj: 0x%x\n", hObj, PrevProcId >> 1, PrevProcId & 0x1, PsGetCurrentProcessId(), GDI_HANDLE_IS_STOCKOBJ(hObj));
  }

  return FALSE;
}

BOOL INTERNAL_CALL
GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle)
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId;
  BOOL Ret;

  DPRINT("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);

  if(!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
  {
    ProcessId = (LONG)PsGetCurrentProcessId() << 1;

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, ObjectHandle);
    Ret = Entry->KernelData != NULL &&
          Entry->Type != 0 &&
          (Entry->ProcessId & ~0x1) == ProcessId;

    return Ret;
  }

  return FALSE;
}

BOOL INTERNAL_CALL
GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj)
{
/*
 * FIXME !!!!! THIS FUNCTION NEEDS TO BE FIXED - IT IS NOT SAFE WHEN OTHER THREADS
 *             MIGHT ATTEMPT TO LOCK THE OBJECT DURING THIS CALL!!!
 */
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId, LockedProcessId, PrevProcId;
  PETHREAD Thread;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  ASSERT(hObj);

  DPRINT("GDIOBJ_ConvertToStockObj: hObj: 0x%08x\n", *hObj);

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(*hObj))
  {
    /* shift the process id to the left so we can use the first bit to lock the object.
       FIXME - don't shift once ROS' PIDs match with nt! */
    ProcessId = (LONG)PsGetCurrentProcessId() << 1;
    LockedProcessId = ProcessId | 0x1;

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, *hObj);

LockHandle:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
    if(PrevProcId == ProcessId)
    {
      LONG NewType, PrevType, OldType;

      /* we're locking an object that belongs to our process. First calculate
         the new object type including the stock object flag and then try to
         exchange it.*/
      NewType = GDI_HANDLE_GET_TYPE(*hObj);
      NewType |= NewType >> 16;
      /* This is the type that the object should have right now, save it */
      OldType = NewType;
      /* As the object should be a stock object, set it's flag, but only in the upper 16 bits */
      NewType |= GDI_HANDLE_STOCK_MASK;

      /* Try to exchange the type field - but only if the old (previous type) matches! */
      PrevType = InterlockedCompareExchange(&Entry->Type, NewType, OldType);
      if(PrevType == OldType && Entry->KernelData != NULL)
      {
        PETHREAD PrevThread;
        PGDIOBJHDR GdiHdr;

        /* We successfully set the stock object flag.
           KernelData should never be NULL here!!! */
        ASSERT(Entry->KernelData);

        GdiHdr = GDIBdyToHdr(Entry->KernelData);

        PrevThread = GdiHdr->LockingThread;
        if(PrevThread == NULL || PrevThread == Thread)
        {
          /* dereference the process' object counter */
          if(PrevProcId != GDI_GLOBAL_PROCESS)
          {
            PEPROCESS OldProcess;
            PW32PROCESS W32Process;
            NTSTATUS Status;

            /* FIXME */
            Status = PsLookupProcessByProcessId((PVOID)(PrevProcId >> 1), &OldProcess);
            if(NT_SUCCESS(Status))
            {
              W32Process = OldProcess->Win32Process;
              if(W32Process != NULL)
              {
                InterlockedDecrement(&W32Process->GDIObjects);
              }
              ObDereferenceObject(OldProcess);
            }
          }

          /* remove the process id lock and make it global */
          InterlockedExchange(&Entry->ProcessId, GDI_GLOBAL_PROCESS);

          *hObj = (HGDIOBJ)((ULONG)(*hObj) | GDI_HANDLE_STOCK_MASK);

          /* we're done, successfully converted the object */
          return TRUE;
        }
        else
        {
#ifdef GDI_DEBUG
          if(++Attempts > 20)
          {
            if(GdiHdr->lockfile != NULL)
            {
              DPRINT1("[%d]Locked %s:%i by 0x%x (we're 0x%x)\n", Attempts, GdiHdr->lockfile, GdiHdr->lockline, PrevThread, Thread);
            }
          }
#endif
          /* WTF?! The object is already locked by a different thread!
             Release the lock, wait a bit and try again!
             FIXME - we should give up after some time unless we want to wait forever! */
          InterlockedExchange(&Entry->ProcessId, PrevProcId);

          DelayExecution();
          goto LockHandle;
        }
      }
      else
      {
        DPRINT1("Attempted to convert object 0x%x that is deleted! Should never get here!!!\n", hObj);
      }
    }
    else if(PrevProcId == LockedProcessId)
    {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting on 0x%x\n", Attempts, hObj);
    }
#endif
      /* the object is currently locked, wait some time and try again.
         FIXME - we shouldn't loop forever! Give up after some time! */
      DelayExecution();
      /* try again */
      goto LockHandle;
    }
    else
    {
      DPRINT1("Attempted to convert invalid handle: 0x%x\n", hObj);
    }
  }

  return FALSE;
}

void INTERNAL_CALL
GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId, LockedProcessId, PrevProcId;
  PETHREAD Thread;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_SetOwnership: hObj: 0x%x, NewProcess: 0x%x\n", ObjectHandle, (NewOwner ? PsGetProcessId(NewOwner) : 0));

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
  {
    /* shift the process id to the left so we can use the first bit to lock the object.
       FIXME - don't shift once ROS' PIDs match with nt! */
    ProcessId = (LONG)PsGetCurrentProcessId() << 1;
    LockedProcessId = ProcessId | 0x1;

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, ObjectHandle);

LockHandle:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, ProcessId, LockedProcessId);
    if(PrevProcId == ProcessId)
    {
      PETHREAD PrevThread;

      if(Entry->Type != 0 && Entry->KernelData != NULL)
      {
        PGDIOBJHDR GdiHdr = GDIBdyToHdr(Entry->KernelData);

        PrevThread = GdiHdr->LockingThread;
        if(PrevThread == NULL || PrevThread == Thread)
        {
          PEPROCESS OldProcess;
          PW32PROCESS W32Process;
          NTSTATUS Status;

          /* dereference the process' object counter */
          /* FIXME */
          Status = PsLookupProcessByProcessId((PVOID)(PrevProcId >> 1), &OldProcess);
          if(NT_SUCCESS(Status))
          {
            W32Process = OldProcess->Win32Process;
            if(W32Process != NULL)
            {
              InterlockedDecrement(&W32Process->GDIObjects);
            }
            ObDereferenceObject(OldProcess);
          }

          if(NewOwner != NULL)
          {
            /* FIXME */
            ProcessId = (LONG)PsGetProcessId(NewOwner) << 1;

            /* Increase the new process' object counter */
            W32Process = NewOwner->Win32Process;
            if(W32Process != NULL)
            {
              InterlockedIncrement(&W32Process->GDIObjects);
            }
          }
          else
            ProcessId = 0;

          /* remove the process id lock and change it to the new process id */
          InterlockedExchange(&Entry->ProcessId, ProcessId);

          /* we're done! */
          return;
        }
        else
        {
#ifdef GDI_DEBUG
          if(++Attempts > 20)
          {
            if(GdiHdr->lockfile != NULL)
            {
              DPRINT1("[%d]Locked from %s:%i by 0x%x (we're 0x%x)\n", Attempts, GdiHdr->lockfile, GdiHdr->lockline, PrevThread, Thread);
            }
          }
#endif
          /* WTF?! The object is already locked by a different thread!
             Release the lock, wait a bit and try again! DO reset the pid lock
             so we make sure we don't access invalid memory in case the object is
             being deleted in the meantime (because we don't have aquired a reference
             at this point).
             FIXME - we should give up after some time unless we want to wait forever! */
          InterlockedExchange(&Entry->ProcessId, PrevProcId);

          DelayExecution();
          goto LockHandle;
        }
      }
      else
      {
        DPRINT1("Attempted to change ownership of an object 0x%x currently being destroyed!!!\n", ObjectHandle);
      }
    }
    else if(PrevProcId == LockedProcessId)
    {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting on 0x%x\n", Attempts, ObjectHandle);
    }
#endif
      /* the object is currently locked, wait some time and try again.
         FIXME - we shouldn't loop forever! Give up after some time! */
      DelayExecution();
      /* try again */
      goto LockHandle;
    }
    else if((PrevProcId >> 1) == 0)
    {
      /* allow changing ownership of global objects */
      ProcessId = 0;
      LockedProcessId = ProcessId | 0x1;
      goto LockHandle;
    }
    else if((PrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
    {
      DPRINT1("Attempted to change ownership of object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", ObjectHandle, PrevProcId >> 1, PsGetCurrentProcessId());
    }
    else
    {
      DPRINT1("Attempted to change owner of invalid handle: 0x%x\n", ObjectHandle);
    }
  }
}

void INTERNAL_CALL
GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
  PGDI_TABLE_ENTRY FromEntry;
  PETHREAD Thread;
  LONG FromProcessId, FromLockedProcessId, FromPrevProcId;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_CopyOwnership: from: 0x%x, to: 0x%x\n", CopyFrom, CopyTo);

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(CopyFrom) && !GDI_HANDLE_IS_STOCKOBJ(CopyTo))
  {
    FromEntry = GDI_HANDLE_GET_ENTRY(HandleTable, CopyFrom);

    FromProcessId = FromEntry->ProcessId & ~0x1;
    FromLockedProcessId = FromProcessId | 0x1;

LockHandleFrom:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    FromPrevProcId = InterlockedCompareExchange(&FromEntry->ProcessId, FromProcessId, FromLockedProcessId);
    if(FromPrevProcId == FromProcessId)
    {
      PETHREAD PrevThread;
      PGDIOBJHDR GdiHdr;

      if(FromEntry->Type != 0 && FromEntry->KernelData != NULL)
      {
        GdiHdr = GDIBdyToHdr(FromEntry->KernelData);

        /* save the pointer to the calling thread so we know it was this thread
           that locked the object */
        PrevThread = GdiHdr->LockingThread;
        if(PrevThread == NULL || PrevThread == Thread)
        {
          /* now let's change the ownership of the target object */

          if((FromPrevProcId & ~0x1) != 0)
          {
            PEPROCESS ProcessTo;
            /* FIXME */
            if(NT_SUCCESS(PsLookupProcessByProcessId((PVOID)(FromPrevProcId >> 1), &ProcessTo)))
            {
              GDIOBJ_SetOwnership(CopyTo, ProcessTo);
              ObDereferenceObject(ProcessTo);
            }
          }
          else
          {
            /* mark the object as global */
            GDIOBJ_SetOwnership(CopyTo, NULL);
          }

          InterlockedExchange(&FromEntry->ProcessId, FromPrevProcId);
        }
        else
        {
#ifdef GDI_DEBUG
          if(++Attempts > 20)
          {
            if(GdiHdr->lockfile != NULL)
            {
              DPRINT1("[%d]Locked from %s:%i by 0x%x (we're 0x%x)\n", Attempts, GdiHdr->lockfile, GdiHdr->lockline, PrevThread, Thread);
            }
          }
#endif
          /* WTF?! The object is already locked by a different thread!
             Release the lock, wait a bit and try again! DO reset the pid lock
             so we make sure we don't access invalid memory in case the object is
             being deleted in the meantime (because we don't have aquired a reference
             at this point).
             FIXME - we should give up after some time unless we want to wait forever! */
          InterlockedExchange(&FromEntry->ProcessId, FromPrevProcId);

          DelayExecution();
          goto LockHandleFrom;
        }
      }
      else
      {
        DPRINT1("Attempted to copy ownership from an object 0x%x currently being destroyed!!!\n", CopyFrom);
      }
    }
    else if(FromPrevProcId == FromLockedProcessId)
    {
#ifdef GDI_DEBUG
    if(++Attempts > 20)
    {
      DPRINT1("[%d]Waiting on 0x%x\n", Attempts, CopyFrom);
    }
#endif
      /* the object is currently locked, wait some time and try again.
         FIXME - we shouldn't loop forever! Give up after some time! */
      DelayExecution();
      /* try again */
      goto LockHandleFrom;
    }
    else if((FromPrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
    {
      /* FIXME - should we really allow copying ownership from objects that we don't even own? */
      DPRINT1("WARNING! Changing copying ownership of object 0x%x (pid: 0x%x) to pid 0x%x!!!\n", CopyFrom, FromPrevProcId >> 1, PsGetCurrentProcessId());
      FromProcessId = FromPrevProcId & ~0x1;
      FromLockedProcessId = FromProcessId | 0x1;
      goto LockHandleFrom;
    }
    else
    {
      DPRINT1("Attempted to copy ownership from invalid handle: 0x%x\n", CopyFrom);
    }
  }
}

PVOID INTERNAL_CALL
GDI_MapHandleTable(HANDLE hProcess)
{
  DPRINT("%s:%i: %s(): FIXME - Map handle table into the process memory space!\n",
         __FILE__, __LINE__, __FUNCTION__);
  /* FIXME - Map the entire gdi handle table read-only to userland into the
             scope of hProcess and return the pointer */
  return NULL;
}

/* EOF */
