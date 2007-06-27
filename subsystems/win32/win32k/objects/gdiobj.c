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
 * $Id$
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FIXME include right header for KeRosDumpStackFrames */
VOID
NTAPI
KeRosDumpStackFrames(
    PULONG Frame,
    ULONG FrameCount
);

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

typedef struct
{
  ULONG Type;
  ULONG Size;
  GDICLEANUPPROC CleanupProc;
} GDI_OBJ_INFO, *PGDI_OBJ_INFO;

/*
 * Dummy GDI Cleanup Callback
 */
static BOOL INTERNAL_CALL
GDI_CleanupDummy(PVOID ObjectBody)
{
  return TRUE;
}

/* Testing shows that regions are the most used GDIObj type,
   so put that one first for performance */
static const
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
  {GDI_OBJECT_TYPE_DIRECTDRAW,  sizeof(DD_DIRECTDRAW), DD_Cleanup},
  {GDI_OBJECT_TYPE_DD_SURFACE,  sizeof(DD_SURFACE),    DDSURF_Cleanup},
  {GDI_OBJECT_TYPE_EXTPEN,      sizeof(GDIBRUSHOBJ),   EXTPEN_Cleanup},
  {GDI_OBJECT_TYPE_METADC,      0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_METAFILE,    0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_ENHMETAFILE, 0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_ENHMETADC,   0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_MEMDC,       0,                     GDI_CleanupDummy},
  {GDI_OBJECT_TYPE_EMF,         0,                     GDI_CleanupDummy}
};

#define OBJTYPE_COUNT (sizeof(ObjInfo) / sizeof(ObjInfo[0]))

static LARGE_INTEGER ShortDelay;

#define DelayExecution() \
  DPRINT("%s:%i: Delay\n", __FILE__, __LINE__); \
  KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay)

#ifdef GDI_DEBUG
BOOLEAN STDCALL KiRosPrintAddress(PVOID Address);
VOID STDCALL KeRosDumpStackFrames(PULONG Frame, ULONG FrameCount);
ULONG STDCALL KeRosGetStackFrames(PULONG Frames, ULONG FrameCount);
#endif

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
PGDI_HANDLE_TABLE INTERNAL_CALL
GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject)
{
  PGDI_HANDLE_TABLE HandleTable = NULL;
  LARGE_INTEGER htSize;
  UINT ObjType;
  UINT i;
  ULONG ViewSize = 0;
  PGDI_TABLE_ENTRY Entry;
  NTSTATUS Status;

  ASSERT(SectionObject != NULL);

  htSize.QuadPart = sizeof(GDI_HANDLE_TABLE);

  Status = MmCreateSection((PVOID*)SectionObject,
                           SECTION_ALL_ACCESS,
                           NULL,
                           &htSize,
                           PAGE_READWRITE,
                           SEC_COMMIT,
                           NULL,
                           NULL);
  if (!NT_SUCCESS(Status))
      return NULL;

  /* FIXME - use MmMapViewInSessionSpace once available! */
  Status = MmMapViewInSystemSpace(*SectionObject,
                                  (PVOID*)&HandleTable,
                                  &ViewSize);
  if (!NT_SUCCESS(Status))
  {
      ObDereferenceObject(*SectionObject);
      *SectionObject = NULL;
      return NULL;
  }

  RtlZeroMemory(HandleTable, sizeof(GDI_HANDLE_TABLE));

  /*
   * initialize the free entry cache
   */
  InitializeSListHead(&HandleTable->FreeEntriesHead);
  Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
  for(i = GDI_HANDLE_COUNT - 1; i >= RESERVE_ENTRIES_COUNT; i--)
  {
    InterlockedPushEntrySList(&HandleTable->FreeEntriesHead, &HandleTable->FreeEntries[i]);
  }

  HandleTable->LookasideLists = ExAllocatePoolWithTag(NonPagedPool,
                                                      OBJTYPE_COUNT * sizeof(PAGED_LOOKASIDE_LIST),
                                                      TAG_GDIHNDTBLE);
  if(HandleTable->LookasideLists == NULL)
  {
    MmUnmapViewInSystemSpace(HandleTable);
    ObDereferenceObject(*SectionObject);
    *SectionObject = NULL;
    return NULL;
  }

  for(ObjType = 0; ObjType < OBJTYPE_COUNT; ObjType++)
  {
    ExInitializePagedLookasideList(HandleTable->LookasideLists + ObjType, NULL, NULL, 0,
                                   ObjInfo[ObjType].Size + sizeof(GDIOBJHDR), TAG_GDIOBJ, 0);
  }

  ShortDelay.QuadPart = -5000LL; /* FIXME - 0.5 ms? */

  return HandleTable;
}

static __inline PPAGED_LOOKASIDE_LIST
FindLookasideList(PGDI_HANDLE_TABLE HandleTable,
                  DWORD ObjectType)
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

static __inline BOOL
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

static __inline ULONG
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

#ifdef GDI_DEBUG

static int leak_reported = 0;
#define GDI_STACK_LEVELS 12
static ULONG GDIHandleAllocator[GDI_HANDLE_COUNT][GDI_STACK_LEVELS];
struct DbgOpenGDIHandle
{
	ULONG idx;
	int count;
};
#define H 1024
static struct DbgOpenGDIHandle h[H];

void IntDumpHandleTable(PGDI_HANDLE_TABLE HandleTable)
{
	int i, n = 0, j, k, J;

	if ( leak_reported )
	{
		DPRINT1("gdi handle abusers already reported!\n");
		return;
	}

	leak_reported = 1;
	DPRINT1("reporting gdi handle abusers:\n");

	/* step through GDI handle table and find out who our culprit is... */
	for ( i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++ )
	{
		for ( j = 0; j < n; j++ )
		{
next:
			J = h[j].idx;
			for ( k = 0; k < GDI_STACK_LEVELS; k++ )
			{
				if ( GDIHandleAllocator[i][k]
				  != GDIHandleAllocator[J][k] )
				{
					if ( ++j == n )
						goto done;
					else
						goto next;
				}
			}
			goto done;
		}
done:
		if ( j < H )
		{
			if ( j == n )
			{
				h[j].idx = i;
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
			t = h[i+1];
			h[i+1] = h[i];
			j = i;
			while ( j > 0 && h[j-1].count < t.count )
				j--;
			h[j] = t;
		}
	}
	/* print the worst offenders... */
	DbgPrint ( "Worst GDI Handle leak offenders (out of %i unique locations):\n", n );
	for ( i = 0; i < n && h[i].count > 1; i++ )
	{
		int j;
		DbgPrint ( " %i allocs: ", h[i].count );
		for ( j = 0; j < GDI_STACK_LEVELS; j++ )
		{
			ULONG Addr = GDIHandleAllocator[h[i].idx][j];
			if ( !KiRosPrintAddress ( (PVOID)Addr ) )
				DbgPrint ( "<%X>", Addr );
		}
		DbgPrint ( "\n" );
	}
	if ( i < n && h[i].count == 1 )
		DbgPrint ( "(list terminated - the remaining entries have 1 allocation only)\n" );
}
#endif /* GDI_DEBUG */

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
GDIOBJ_AllocObjDbg(PGDI_HANDLE_TABLE HandleTable, const char* file, int line, ULONG ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_AllocObj(PGDI_HANDLE_TABLE HandleTable, ULONG ObjectType)
#endif /* GDI_DEBUG */
{
  PW32PROCESS W32Process;
  PGDIOBJHDR  newObject;
  PPAGED_LOOKASIDE_LIST LookasideList;
  HANDLE CurrentProcessId, LockedProcessId;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  W32Process = PsGetCurrentProcessWin32Process();
  /* HACK HACK HACK: simplest-possible quota implementation - don't allow a process
     to take too many GDI objects, itself. */
  if ( W32Process && W32Process->GDIObjects >= 0x2710 )
    return NULL;

  ASSERT(ObjectType != GDI_OBJECT_TYPE_DONTCARE);

  LookasideList = FindLookasideList(HandleTable, ObjectType);
  if(LookasideList != NULL)
  {
    newObject = ExAllocateFromPagedLookasideList(LookasideList);
    if(newObject != NULL)
    {
      PSLIST_ENTRY FreeEntry;
      PGDI_TABLE_ENTRY Entry;
      PGDIOBJ ObjectBody;
      LONG TypeInfo;

      CurrentProcessId = PsGetCurrentProcessId();
      LockedProcessId = (HANDLE)((ULONG_PTR)CurrentProcessId | 0x1);

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

      TypeInfo = (ObjectType & GDI_HANDLE_TYPE_MASK) | (ObjectType >> 16);

      FreeEntry = InterlockedPopEntrySList(&HandleTable->FreeEntriesHead);
      if(FreeEntry != NULL)
      {
        HANDLE PrevProcId;
        UINT Index;

        /* calculate the entry from the address of the entry in the free slot array */
        Index = ((ULONG_PTR)FreeEntry - (ULONG_PTR)&HandleTable->FreeEntries[0]) /
                sizeof(HandleTable->FreeEntries[0]);
        Entry = &HandleTable->Entries[Index];

LockHandle:
        PrevProcId = InterlockedCompareExchangePointer(&Entry->ProcessId, LockedProcessId, 0);
        if(PrevProcId == NULL)
        {
          HGDIOBJ Handle;

          ASSERT(Entry->KernelData == NULL);

          Entry->KernelData = ObjectBody;

          /* copy the reuse-counter */
          TypeInfo |= Entry->Type & GDI_HANDLE_REUSE_MASK;

          /* we found a free entry, no need to exchange this field atomically
             since we're holding the lock */
          Entry->Type = TypeInfo;

          /* unlock the entry */
          (void)InterlockedExchangePointer(&Entry->ProcessId, CurrentProcessId);

#ifdef GDI_DEBUG
          memset ( GDIHandleAllocator[Index], 0xcd, GDI_STACK_LEVELS * sizeof(ULONG) );
          KeRosGetStackFrames ( GDIHandleAllocator[Index], GDI_STACK_LEVELS );
#endif /* GDI_DEBUG */

          if(W32Process != NULL)
          {
            InterlockedIncrement(&W32Process->GDIObjects);
          }
          Handle = (HGDIOBJ)((Index & 0xFFFF) | (TypeInfo & (GDI_HANDLE_TYPE_MASK | GDI_HANDLE_REUSE_MASK)));

          DPRINT("GDIOBJ_AllocObj: 0x%x ob: 0x%x\n", Handle, ObjectBody);
          return Handle;
        }
        else
        {
#ifdef GDI_DEBUG
          if(++Attempts > 20)
          {
            DPRINT1("[%d]Waiting on handle in index 0x%x\n", Attempts, Index);
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
#ifdef GDI_DEBUG
      IntDumpHandleTable(HandleTable);
#endif /* GDI_DEBUG */
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
GDIOBJ_FreeObjDbg(PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_FreeObj(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
  PGDI_TABLE_ENTRY Entry;
  PPAGED_LOOKASIDE_LIST LookasideList;
  HANDLE ProcessId, LockedProcessId, PrevProcId;
  LONG ExpectedType;
  BOOL Silent;
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

  ProcessId = PsGetCurrentProcessId();
  LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

  Silent = (ObjectType & GDI_OBJECT_TYPE_SILENT);
  ObjectType &= ~GDI_OBJECT_TYPE_SILENT;

  ExpectedType = ((ObjectType != GDI_OBJECT_TYPE_DONTCARE) ? ObjectType : 0);

  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);

LockHandle:
  /* lock the object, we must not delete global objects, so don't exchange the locking
     process ID to zero when attempting to lock a global object... */
  PrevProcId = InterlockedCompareExchangePointer(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    if(Entry->Type != 0 && Entry->KernelData != NULL &&
       (ExpectedType == 0 || ((Entry->Type << 16) == ExpectedType)) &&
       (Entry->Type & (GDI_HANDLE_TYPE_MASK | GDI_HANDLE_REUSE_MASK)) ==
       ((ULONG_PTR)hObj & (GDI_HANDLE_TYPE_MASK | GDI_HANDLE_REUSE_MASK)))
    {
      PGDIOBJHDR GdiHdr;

      GdiHdr = GDIBdyToHdr(Entry->KernelData);

      if(GdiHdr->Locks == 0)
      {
        BOOL Ret;
        PW32PROCESS W32Process = PsGetCurrentProcessWin32Process();
        ULONG Type = Entry->Type << 16;

        /* Clear the type field so when unlocking the handle it gets finally deleted and increment reuse counter */
        Entry->Type = ((Entry->Type >> GDI_HANDLE_REUSECNT_SHIFT) + 1) << GDI_HANDLE_REUSECNT_SHIFT;
        Entry->KernelData = NULL;

        /* unlock the handle slot */
        (void)InterlockedExchangePointer(&Entry->ProcessId, NULL);

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
        LookasideList = FindLookasideList(HandleTable, Type);
        if(LookasideList != NULL)
        {
          ExFreeToPagedLookasideList(LookasideList, GdiHdr);
        }

        return Ret;
      }
      else
      {
        /*
         * The object is currently locked, so freeing is forbidden!
         */
        DPRINT1("GdiHdr->Locks: %d\n", GdiHdr->Locks);
#ifdef GDI_DEBUG
        DPRINT1("Locked from: %s:%d\n", GdiHdr->lockfile, GdiHdr->lockline);
#endif
        ASSERT(FALSE);
      }
    }
    else
    {
      if((Entry->Type & ~GDI_HANDLE_REUSE_MASK) != 0)
      {
        DPRINT1("Attempted to delete object 0x%x, type mismatch (0x%x : 0x%x)\n", hObj, ObjectType, ExpectedType);
        KeRosDumpStackFrames(NULL, 20);
      }
      else
      {
        DPRINT1("Attempted to delete object 0x%x which was already deleted!\n", hObj);
         KeRosDumpStackFrames(NULL, 20);
      }
      (void)InterlockedExchangePointer(&Entry->ProcessId, PrevProcId);
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
    if(!Silent)
    {
      if(((ULONG_PTR)PrevProcId & ~0x1) == 0)
      {
        DPRINT1("Attempted to free global gdi handle 0x%x, caller needs to get ownership first!!!\n", hObj);
        KeRosDumpStackFrames(NULL, 20);
      }
      else
      {
        DPRINT1("Attempted to free foreign handle: 0x%x Owner: 0x%x from Caller: 0x%x\n", hObj, (ULONG_PTR)PrevProcId & ~0x1, (ULONG_PTR)ProcessId & ~0x1);
        KeRosDumpStackFrames(NULL, 20);
      }
#ifdef GDI_DEBUG
      DPRINT1("-> called from %s:%i\n", file, line);
#endif
    }
  }

  return FALSE;
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
         ? GDIOBJ_FreeObj(GdiHandleTable, hObject, GDI_OBJECT_TYPE_DONTCARE) : FALSE;
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL INTERNAL_CALL
GDI_CleanupForProcess (PGDI_HANDLE_TABLE HandleTable, struct _EPROCESS *Process)
{
  PGDI_TABLE_ENTRY Entry, End;
  PEPROCESS CurrentProcess;
  PW32PROCESS W32Process;
  HANDLE ProcId;
  ULONG Index = RESERVE_ENTRIES_COUNT;

  DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Process->UniqueProcessId);
  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
    {
      KeAttachProcess(&Process->Pcb);
    }
  W32Process = (PW32PROCESS)Process->Win32Process;
  ASSERT(W32Process);

  if(W32Process->GDIObjects > 0)
  {
    /* FIXME - Instead of building the handle here and delete it using GDIOBJ_FreeObj
               we should delete it directly here! */
    ProcId = Process->UniqueProcessId;

    End = &HandleTable->Entries[GDI_HANDLE_COUNT];
    for(Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
        Entry != End;
        Entry++, Index++)
    {
      /* ignore the lock bit */
      if((HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) == ProcId && (Entry->Type & ~GDI_HANDLE_REUSE_MASK) != 0)
      {
        HGDIOBJ ObjectHandle;

        /* Create the object handle for the entry, the upper 16 bit of the
           Type field includes the type of the object including the stock
           object flag - but since stock objects don't have a process id we can
           simply ignore this fact here. */
        ObjectHandle = (HGDIOBJ)(Index | (Entry->Type & 0xFFFF0000));

        if(GDIOBJ_FreeObj(HandleTable, ObjectHandle, GDI_OBJECT_TYPE_DONTCARE) &&
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

  DPRINT("Completed cleanup for process %d\n", Process->UniqueProcessId);

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
GDIOBJ_LockObjDbg (PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_LockObj (PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
   USHORT HandleIndex;
   PGDI_TABLE_ENTRY HandleEntry;
   HANDLE ProcessId, HandleProcessId, LockedProcessId, PrevProcId;
   PGDIOBJ Object = NULL;

   HandleIndex = GDI_HANDLE_GET_INDEX(hObj);

   /* Check that the handle index is valid. */
   if (HandleIndex >= GDI_HANDLE_COUNT)
      return NULL;

   HandleEntry = &HandleTable->Entries[HandleIndex];

   ProcessId = (HANDLE)((ULONG_PTR)PsGetCurrentProcessId() & ~1);
   HandleProcessId = (HANDLE)((ULONG_PTR)HandleEntry->ProcessId & ~1);
    
   /* Check for invalid owner. */
   if (ProcessId != HandleProcessId && HandleProcessId != NULL)
   {
      return NULL;
   }
   
   /*
    * Prevent the thread from being terminated during the locking process.
    * It would result in undesired effects and inconsistency of the global
    * handle table.
    */

   KeEnterCriticalRegion();

   /*
    * Loop until we either successfully lock the handle entry & object or
    * fail some of the check.
    */
   
   for (;;)
   {
      /* Lock the handle table entry. */
      LockedProcessId = (HANDLE)((ULONG_PTR)HandleProcessId | 0x1);
      PrevProcId = InterlockedCompareExchangePointer(&HandleEntry->ProcessId, 
                                                     LockedProcessId,
                                                     HandleProcessId);

      if (PrevProcId == HandleProcessId)
      {
         LONG HandleType = HandleEntry->Type << 16;

         /*
          * We're locking an object that belongs to our process or it's a
          * global object if HandleProcessId is 0 here.
          */

         /* FIXME: Check the upper 16-bits of handle number! */
         if (HandleType != 0 && HandleEntry->KernelData != NULL &&
             (ObjectType == GDI_OBJECT_TYPE_DONTCARE ||
              HandleType == ObjectType))
         {
            PGDIOBJHDR GdiHdr = GDIBdyToHdr(HandleEntry->KernelData);
            PETHREAD Thread = PsGetCurrentThread();

            if (GdiHdr->Locks == 0)
            {
               GdiHdr->LockingThread = Thread;
               GdiHdr->Locks = 1;
#ifdef GDI_DEBUG
               GdiHdr->lockfile = file;
               GdiHdr->lockline = line;
#endif
               Object = HandleEntry->KernelData;
            }
            else
            {
               InterlockedIncrement((PLONG)&GdiHdr->Locks);
               if (GdiHdr->LockingThread != Thread)
               {
                  InterlockedDecrement((PLONG)&GdiHdr->Locks);

                  /* Unlock the handle table entry. */
                  (void)InterlockedExchangePointer(&HandleEntry->ProcessId, PrevProcId);

                  DelayExecution();
                  continue;
               }
               Object = HandleEntry->KernelData;
            }
         }
         else
         {
            /*
             * Debugging code. Report attempts to lock deleted handles and
             * locking type mismatches.
             */

            if ((HandleType & ~GDI_HANDLE_REUSE_MASK) == 0)
            {
               DPRINT1("Attempted to lock object 0x%x that is deleted!\n", hObj);
               KeRosDumpStackFrames(NULL, 20);
            }
            else
            {
               DPRINT1("Attempted to lock object 0x%x, type mismatch (0x%x : 0x%x)\n",
                  hObj, HandleType & ~GDI_HANDLE_REUSE_MASK, ObjectType & ~GDI_HANDLE_REUSE_MASK);

               KeRosDumpStackFrames(NULL, 20);
            }
#ifdef GDI_DEBUG
            DPRINT1("-> called from %s:%i\n", file, line);
#endif
         }

         /* Unlock the handle table entry. */
         (void)InterlockedExchangePointer(&HandleEntry->ProcessId, PrevProcId);

         break;
      }
      else
      {
         /*
          * The handle is currently locked, wait some time and try again.
          */

         DelayExecution();
         continue;
      }
   }

   KeLeaveCriticalRegion();

   return Object;
}


/*!
 * Return pointer to the object by handle (and allow sharing of the handle
 * across threads).
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
GDIOBJ_ShareLockObjDbg (PGDI_HANDLE_TABLE HandleTable, const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_ShareLockObj (PGDI_HANDLE_TABLE HandleTable, HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
   USHORT HandleIndex;
   PGDI_TABLE_ENTRY HandleEntry;
   HANDLE ProcessId, HandleProcessId, LockedProcessId, PrevProcId;
   PGDIOBJ Object = NULL;

   HandleIndex = GDI_HANDLE_GET_INDEX(hObj);

   /* Check that the handle index is valid. */
   if (HandleIndex >= GDI_HANDLE_COUNT)
      return NULL;

   HandleEntry = &HandleTable->Entries[HandleIndex];

   ProcessId = (HANDLE)((ULONG_PTR)PsGetCurrentProcessId() & ~1);
   HandleProcessId = (HANDLE)((ULONG_PTR)HandleEntry->ProcessId & ~1);
    
   /* Check for invalid owner. */
   if (ProcessId != HandleProcessId && HandleProcessId != NULL)
   {
      return NULL;
   }
   
   /*
    * Prevent the thread from being terminated during the locking process.
    * It would result in undesired effects and inconsistency of the global
    * handle table.
    */

   KeEnterCriticalRegion();

   /*
    * Loop until we either successfully lock the handle entry & object or
    * fail some of the check.
    */
   
   for (;;)
   {
      /* Lock the handle table entry. */
      LockedProcessId = (HANDLE)((ULONG_PTR)HandleProcessId | 0x1);
      PrevProcId = InterlockedCompareExchangePointer(&HandleEntry->ProcessId, 
                                                     LockedProcessId,
                                                     HandleProcessId);

      if (PrevProcId == HandleProcessId)
      {
         LONG HandleType = HandleEntry->Type << 16;

         /*
          * We're locking an object that belongs to our process or it's a
          * global object if HandleProcessId is 0 here.
          */

         /* FIXME: Check the upper 16-bits of handle number! */
         if (HandleType != 0 && HandleEntry->KernelData != NULL &&
             (ObjectType == GDI_OBJECT_TYPE_DONTCARE ||
              HandleType == ObjectType))
         {
            PGDIOBJHDR GdiHdr = GDIBdyToHdr(HandleEntry->KernelData);

#ifdef GDI_DEBUG
            if (InterlockedIncrement((PLONG)&GdiHdr->Locks) == 1)
            {
               GdiHdr->lockfile = file;
               GdiHdr->lockline = line;
            }
#else
            InterlockedIncrement((PLONG)&GdiHdr->Locks);
#endif
            Object = HandleEntry->KernelData;
         }
         else
         {
            /*
             * Debugging code. Report attempts to lock deleted handles and
             * locking type mismatches.
             */

            if ((HandleType & ~GDI_HANDLE_REUSE_MASK) == 0)
            {
               DPRINT1("Attempted to lock object 0x%x that is deleted!\n", hObj);
               KeRosDumpStackFrames(NULL, 20);
            }
            else
            {
               DPRINT1("Attempted to lock object 0x%x, type mismatch (0x%x : 0x%x)\n",
                  hObj, HandleType & ~GDI_HANDLE_REUSE_MASK, ObjectType & ~GDI_HANDLE_REUSE_MASK);

               KeRosDumpStackFrames(NULL, 20);
            }
#ifdef GDI_DEBUG
            DPRINT1("-> called from %s:%i\n", file, line);
#endif
         }

         /* Unlock the handle table entry. */
         (void)InterlockedExchangePointer(&HandleEntry->ProcessId, PrevProcId);

         break;
      }
      else
      {
         /*
          * The handle is currently locked, wait some time and try again.
          */

         DelayExecution();
         continue;
      }
   }

   KeLeaveCriticalRegion();

   return Object;
}


/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param Object 	Object pointer (as returned by GDIOBJ_LockObj).
 */
VOID INTERNAL_CALL
GDIOBJ_UnlockObjByPtr(PGDI_HANDLE_TABLE HandleTable, PGDIOBJ Object)
{
   PGDIOBJHDR GdiHdr = GDIBdyToHdr(Object);
#ifdef GDI_DEBUG
   if (InterlockedDecrement((PLONG)&GdiHdr->Locks) == 0)
   {
      GdiHdr->lockfile = NULL;
      GdiHdr->lockline = 0;
   }
#else
   InterlockedDecrement((PLONG)&GdiHdr->Locks);
#endif
}

BOOL INTERNAL_CALL
GDIOBJ_OwnedByCurrentProcess(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ ObjectHandle)
{
  PGDI_TABLE_ENTRY Entry;
  HANDLE ProcessId;
  BOOL Ret;

  DPRINT("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);

  if(!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
  {
    ProcessId = PsGetCurrentProcessId();

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, ObjectHandle);
    Ret = Entry->KernelData != NULL &&
          (Entry->Type & ~GDI_HANDLE_REUSE_MASK) != 0 &&
          (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) == ProcessId;

    return Ret;
  }

  return FALSE;
}

BOOL INTERNAL_CALL
GDIOBJ_ConvertToStockObj(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ *hObj)
{
/*
 * FIXME !!!!! THIS FUNCTION NEEDS TO BE FIXED - IT IS NOT SAFE WHEN OTHER THREADS
 *             MIGHT ATTEMPT TO LOCK THE OBJECT DURING THIS CALL!!!
 */
  PGDI_TABLE_ENTRY Entry;
  HANDLE ProcessId, LockedProcessId, PrevProcId;
  PETHREAD Thread;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  ASSERT(hObj);

  DPRINT("GDIOBJ_ConvertToStockObj: hObj: 0x%08x\n", *hObj);

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(*hObj))
  {
    ProcessId = PsGetCurrentProcessId();
    LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, *hObj);

LockHandle:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    PrevProcId = InterlockedCompareExchangePointer(&Entry->ProcessId, LockedProcessId, ProcessId);
    if(PrevProcId == ProcessId)
    {
      LONG NewType, PrevType, OldType;

      /* we're locking an object that belongs to our process. First calculate
         the new object type including the stock object flag and then try to
         exchange it.*/
      NewType = GDI_HANDLE_GET_TYPE(*hObj);
      NewType |= NewType >> 16;
      NewType |= (ULONG_PTR)(*hObj) & GDI_HANDLE_REUSE_MASK;

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
        if(GdiHdr->Locks == 0 || PrevThread == Thread)
        {
          /* dereference the process' object counter */
          if(PrevProcId != GDI_GLOBAL_PROCESS)
          {
            PEPROCESS OldProcess;
            PW32PROCESS W32Process;
            NTSTATUS Status;

            /* FIXME */
            Status = PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)PrevProcId & ~0x1), &OldProcess);
            if(NT_SUCCESS(Status))
            {
              W32Process = (PW32PROCESS)OldProcess->Win32Process;
              if(W32Process != NULL)
              {
                InterlockedDecrement(&W32Process->GDIObjects);
              }
              ObDereferenceObject(OldProcess);
            }
          }

          /* remove the process id lock and make it global */
          (void)InterlockedExchangePointer(&Entry->ProcessId, GDI_GLOBAL_PROCESS);

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
          (void)InterlockedExchangePointer(&Entry->ProcessId, PrevProcId);

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
GDIOBJ_SetOwnership(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
  PGDI_TABLE_ENTRY Entry;
  HANDLE ProcessId, LockedProcessId, PrevProcId;
  PETHREAD Thread;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_SetOwnership: hObj: 0x%x, NewProcess: 0x%x\n", ObjectHandle, (NewOwner ? PsGetProcessId(NewOwner) : 0));

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
  {
    ProcessId = PsGetCurrentProcessId();
    LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, ObjectHandle);

LockHandle:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    PrevProcId = InterlockedCompareExchangePointer(&Entry->ProcessId, ProcessId, LockedProcessId);
    if(PrevProcId == ProcessId)
    {
      PETHREAD PrevThread;

      if((Entry->Type & ~GDI_HANDLE_REUSE_MASK) != 0 && Entry->KernelData != NULL)
      {
        PGDIOBJHDR GdiHdr = GDIBdyToHdr(Entry->KernelData);

        PrevThread = GdiHdr->LockingThread;
        if(GdiHdr->Locks == 0 || PrevThread == Thread)
        {
          PEPROCESS OldProcess;
          PW32PROCESS W32Process;
          NTSTATUS Status;

          /* dereference the process' object counter */
          /* FIXME */
          if((ULONG_PTR)PrevProcId & ~0x1)
          {
            Status = PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)PrevProcId & ~0x1), &OldProcess);
            if(NT_SUCCESS(Status))
            {
              W32Process = (PW32PROCESS)OldProcess->Win32Process;
              if(W32Process != NULL)
              {
                InterlockedDecrement(&W32Process->GDIObjects);
              }
              ObDereferenceObject(OldProcess);
            }
          }

          if(NewOwner != NULL)
          {
            ProcessId = PsGetProcessId(NewOwner);

            /* Increase the new process' object counter */
            W32Process = (PW32PROCESS)NewOwner->Win32Process;
            if(W32Process != NULL)
            {
              InterlockedIncrement(&W32Process->GDIObjects);
            }
          }
          else
            ProcessId = 0;

          /* remove the process id lock and change it to the new process id */
          (void)InterlockedExchangePointer(&Entry->ProcessId, ProcessId);

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
          (void)InterlockedExchangePointer(&Entry->ProcessId, PrevProcId);

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
    else if(((ULONG_PTR)PrevProcId & ~0x1) == 0)
    {
      /* allow changing ownership of global objects */
      ProcessId = NULL;
      LockedProcessId = (HANDLE)((ULONG_PTR)ProcessId | 0x1);
      goto LockHandle;
    }
    else if((HANDLE)((ULONG_PTR)PrevProcId & ~0x1) != PsGetCurrentProcessId())
    {
      DPRINT1("Attempted to change ownership of object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", ObjectHandle, (ULONG_PTR)PrevProcId & ~0x1, PsGetCurrentProcessId());
    }
    else
    {
      DPRINT1("Attempted to change owner of invalid handle: 0x%x\n", ObjectHandle);
    }
  }
}

void INTERNAL_CALL
GDIOBJ_CopyOwnership(PGDI_HANDLE_TABLE HandleTable, HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
  PGDI_TABLE_ENTRY FromEntry;
  PETHREAD Thread;
  HANDLE FromProcessId, FromLockedProcessId, FromPrevProcId;
#ifdef GDI_DEBUG
  ULONG Attempts = 0;
#endif

  DPRINT("GDIOBJ_CopyOwnership: from: 0x%x, to: 0x%x\n", CopyFrom, CopyTo);

  Thread = PsGetCurrentThread();

  if(!GDI_HANDLE_IS_STOCKOBJ(CopyFrom) && !GDI_HANDLE_IS_STOCKOBJ(CopyTo))
  {
    FromEntry = GDI_HANDLE_GET_ENTRY(HandleTable, CopyFrom);

    FromProcessId = (HANDLE)((ULONG_PTR)FromEntry->ProcessId & ~0x1);
    FromLockedProcessId = (HANDLE)((ULONG_PTR)FromProcessId | 0x1);

LockHandleFrom:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    FromPrevProcId = InterlockedCompareExchangePointer(&FromEntry->ProcessId, FromProcessId, FromLockedProcessId);
    if(FromPrevProcId == FromProcessId)
    {
      PETHREAD PrevThread;
      PGDIOBJHDR GdiHdr;

      if((FromEntry->Type & ~GDI_HANDLE_REUSE_MASK) != 0 && FromEntry->KernelData != NULL)
      {
        GdiHdr = GDIBdyToHdr(FromEntry->KernelData);

        /* save the pointer to the calling thread so we know it was this thread
           that locked the object */
        PrevThread = GdiHdr->LockingThread;
        if(GdiHdr->Locks == 0 || PrevThread == Thread)
        {
          /* now let's change the ownership of the target object */

          if(((ULONG_PTR)FromPrevProcId & ~0x1) != 0)
          {
            PEPROCESS ProcessTo;
            /* FIXME */
            if(NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1), &ProcessTo)))
            {
              GDIOBJ_SetOwnership(HandleTable, CopyTo, ProcessTo);
              ObDereferenceObject(ProcessTo);
            }
          }
          else
          {
            /* mark the object as global */
            GDIOBJ_SetOwnership(HandleTable, CopyTo, NULL);
          }

          (void)InterlockedExchangePointer(&FromEntry->ProcessId, FromPrevProcId);
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
          (void)InterlockedExchangePointer(&FromEntry->ProcessId, FromPrevProcId);

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
    else if((HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1) != PsGetCurrentProcessId())
    {
      /* FIXME - should we really allow copying ownership from objects that we don't even own? */
      DPRINT1("WARNING! Changing copying ownership of object 0x%x (pid: 0x%x) to pid 0x%x!!!\n", CopyFrom, (ULONG_PTR)FromPrevProcId & ~0x1, PsGetCurrentProcessId());
      FromProcessId = (HANDLE)((ULONG_PTR)FromPrevProcId & ~0x1);
      FromLockedProcessId = (HANDLE)((ULONG_PTR)FromProcessId | 0x1);
      goto LockHandleFrom;
    }
    else
    {
      DPRINT1("Attempted to copy ownership from invalid handle: 0x%x\n", CopyFrom);
    }
  }
}

PVOID INTERNAL_CALL
GDI_MapHandleTable(PSECTION_OBJECT SectionObject, PEPROCESS Process)
{
    PVOID MappedView = NULL;
    NTSTATUS Status;
    LARGE_INTEGER Offset;
    ULONG ViewSize = sizeof(GDI_HANDLE_TABLE);

    Offset.QuadPart = 0;

    ASSERT(SectionObject != NULL);
    ASSERT(Process != NULL);

    Status = MmMapViewOfSection(SectionObject,
                                Process,
                                &MappedView,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_READONLY);

    if (!NT_SUCCESS(Status))
        return NULL;

    return MappedView;
}

/* EOF */
