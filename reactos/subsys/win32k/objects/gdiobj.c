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
 * $Id: gdiobj.c,v 1.71.4.2 2004/09/13 21:28:17 weiden Exp $
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* FIXME - This is a HACK!!!!!! If you don't get the warnings anymore, this hack
           should be removed immediately!! */
//#define IGNORE_PID_WHILE_LOCKING

#ifdef __USE_W32API
/* F*(&#$ header mess!!!! */
HANDLE
STDCALL PsGetProcessId(
   	PEPROCESS	Process
	);
#endif /* __USE_W32API */


#define GDI_ENTRY_TO_INDEX(ht, e) \
  (((PCHAR)(e) - (PCHAR)&((ht)->Entries[0])) / sizeof(GDI_TABLE_ENTRY))
#define GDI_HANDLE_GET_ENTRY(HandleTable, h) \
  (&(HandleTable)->Entries[GDI_HANDLE_GET_INDEX((h))])
#define GDI_VALID_OBJECT(h, obj, t, f) \
  (((h) == (obj)->hGdiHandle) && \
   ((GDI_HANDLE_GET_TYPE((obj)->hGdiHandle) == (t)) || ((t) == GDI_OBJECT_TYPE_DONTCARE)))

#define GDIBdyToHdr(body) \
  (PGDIOBJHDR)((PCHAR)(body) - sizeof(GDI_TABLE_ENTRY))
#define GDIHdrToBdy(hdr) \
  (PGDIOBJ)((PCHAR)(hdr) + sizeof(GDI_TABLE_ENTRY))

/* apparently the first 10 entries are never used in windows as they are empty */
#define RESERVE_ENTRIES_COUNT 10

typedef struct _GDI_HANDLE_TABLE
{
  LONG HandlesCount;
  LONG nEntries;
  PPAGED_LOOKASIDE_LIST LookasideLists;

  PGDI_TABLE_ENTRY EntriesEnd;

  GDI_TABLE_ENTRY Entries[1];
} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

typedef struct
{
  ULONG Type;
  ULONG Size;
  PVOID CleanupProc; /* GDICLEANUPPROC */
} GDI_OBJ_INFO, *PGDI_OBJ_INFO;

/*
 * Dummy GDI Cleanup Callback
 */
BOOL FASTCALL
GDI_CleanupDummy(PGDIOBJ pObj)
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

#define DelayExecution() DbgPrint("%s:%i: Delay\n", __FILE__, __LINE__); KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay)

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
static PGDI_HANDLE_TABLE FASTCALL
GDIOBJ_iAllocHandleTable(ULONG Entries)
{
  PGDI_HANDLE_TABLE handleTable;
  UINT ObjType;
  ULONG MemSize = sizeof(GDI_HANDLE_TABLE) + (sizeof(GDI_TABLE_ENTRY) * (Entries - 1));

  handleTable = ExAllocatePoolWithTag(PagedPool, MemSize, TAG_GDIHNDTBLE);
  ASSERT( handleTable );
  RtlZeroMemory(handleTable, MemSize);

  handleTable->HandlesCount = 0;
  handleTable->nEntries = Entries;

  handleTable->EntriesEnd = &handleTable->Entries[Entries];

  handleTable->LookasideLists = ExAllocatePoolWithTag(PagedPool,
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

  ShortDelay.QuadPart = -100;

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
HGDIOBJ FASTCALL
GDIOBJ_AllocObj(ULONG ObjectType)
{
  PW32PROCESS W32Process;
  PGDIOBJHDR  newObject;
  PPAGED_LOOKASIDE_LIST LookasideList;
  LONG CurrentProcessId;
  
  ASSERT(ObjectType != GDI_OBJECT_TYPE_DONTCARE);

  LookasideList = FindLookasideList(ObjectType);
  if(LookasideList != NULL)
  {
    newObject = ExAllocateFromPagedLookasideList(LookasideList);
    if(newObject != NULL)
    {
      PGDI_TABLE_ENTRY Entry;
      PGDIOBJ ObjectBody;
      LONG TypeInfo;

      /* shift the process id to the left so we can use the first bit to lock
         the object.
         FIXME - don't shift once ROS' PIDs match with nt! */
      CurrentProcessId = (LONG)PsGetCurrentProcessId() << 1;
      W32Process = PsGetWin32Process();

      newObject->RefCount = 1;
      newObject->LockingThread = NULL;
      newObject->Type = ObjectType;
      newObject->LookasideList = LookasideList;

#ifdef GDI_DEBUG
      newObject->lockfile = NULL;
      newObject->lockline = 0;
#endif

      ObjectBody = GDIHdrToBdy(newObject);
      
      RtlZeroMemory(ObjectBody, GetObjectSize(ObjectType));
      
      TypeInfo = (ObjectType & 0xFFFF0000) | (ObjectType >> 16);

      /* Search for a free handle entry */
      for(Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
          Entry < HandleTable->EntriesEnd;
          Entry++)
      {
        if(InterlockedCompareExchangePointer(&Entry->KernelData, ObjectBody, NULL) == NULL)
        {
          UINT Index;

          /* we found a free entry */
          InterlockedExchange(&Entry->ProcessId, CurrentProcessId);
          InterlockedExchange(&Entry->Type, TypeInfo);
          
          if(W32Process != NULL)
          {
            InterlockedIncrement(&W32Process->GDIObjects);
          }

          Index = GDI_ENTRY_TO_INDEX(HandleTable, Entry);
          DPRINT("GDIOBJ_AllocObj: 0x%x kd:0x%x ob: 0x%x\n", ((Index & 0xFFFF) | (ObjectType & 0xFFFF0000)), Entry->KernelData, ObjectBody);
          return (HGDIOBJ)((Index & 0xFFFF) | (ObjectType & 0xFFFF0000));
        }
      }

      ExFreeToPagedLookasideList(LookasideList, newObject);
      DPRINT1("Failed to insert gdi object into the handle table, no handles left!\n");
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
BOOL FASTCALL
GDIOBJ_FreeObj(HGDIOBJ hObj, DWORD ObjectType)
{
  /* FIXME - get rid of the ObjectType and Flag parameters, they're obsolete! */
  PGDI_TABLE_ENTRY Entry;
  PPAGED_LOOKASIDE_LIST LookasideList;
  LONG ProcessId, LockedProcessId, PrevProcId, ExpectedType;

  DPRINT("GDIOBJ_FreeObj: hObj: 0x%08x\n", hObj);

  /* shift the process id to the left so we can use the first bit to lock the object.
     FIXME - don't shift once ROS' PIDs match with nt! */
  ProcessId = (LONG)PsGetCurrentProcessId() << 1;
  LockedProcessId = ProcessId | 0x1;
  
  ExpectedType = ((ObjectType != GDI_OBJECT_TYPE_DONTCARE) ? 0 : (ObjectType & 0xFFFF0000) | (ObjectType >> 16));
  
  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);
  
LockHandle:
  /* lock the object, we must not delete stock objects, so don't check!!! */
  PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    LONG PrevType;
    
ExchangeType:
    /* clear the type field as we're about to delete the object */
    PrevType = InterlockedCompareExchange(&Entry->Type, 0, ExpectedType);
    if(PrevType != 0 && PrevType == ExpectedType)
    {
      PGDIOBJHDR GdiHdr = GDIBdyToHdr(Entry->KernelData);
      
      /* we're save now, any attempt to lock or delete the object will fail as
         the type field now is 0, which indicates the object is being deleted.
         We're safe to access the object now and decrement the reference counter */
      if(InterlockedDecrement(&GdiHdr->RefCount) == 0)
      {
        BOOL Ret;
        
        /* We removed the keep-alive reference, it's time to clear the PID and object
           pointer in the table. */
        InterlockedExchange(&Entry->ProcessId, 0);
        InterlockedExchangePointer(&Entry->KernelData, NULL);

        /* As we removed the keep-alive reference no thread is allowed to hold the lock */
        ASSERT(GdiHdr->LockingThread == NULL);
        
        /* call the cleanup routine. */
        Ret = RunCleanupCallback(GDIHdrToBdy(GdiHdr), GdiHdr->Type);
        
        /* Now it's time to free the memory */
        LookasideList = FindLookasideList(PrevType << 16);
        if(LookasideList != NULL)
        {
          ExFreeToPagedLookasideList(LookasideList, GdiHdr);
        }
        
        return Ret;
      }
      else
      {
        /* the LockingThread field should not be NULL here, the object must be locked right now! */
        ASSERT(GdiHdr->LockingThread != NULL);

        /* there's still some references to the object, some kids locked the object
           more than once!!! We however remove the object from the table so they're
           not accessible anymore. The object gets deleted as soon as the reference
           counter is 0 in the unlocking routine. */
        InterlockedExchange(&Entry->ProcessId, 0);
        InterlockedExchangePointer(&Entry->KernelData, NULL);
        
        /* report a successful deletion as the object is actually removed from the table */
        return TRUE;
      }
    }
    else if(PrevType != 0)
    {
      /* try again, the caller passed GDI_OBJECT_TYPE_DONTCARE as type */
      ExpectedType = PrevType;
      goto ExchangeType;
    }
    else
    {
      if(ObjectType == 0)
      {
        DPRINT1("Another thread already deleted the object 0x%x!\n", hObj);
      }
      else
      {
        DPRINT1("Attempted to delete object 0x%x, type mismatch (0x%x : 0x%x)\n", hObj, ObjectType, ExpectedType);
      }
    }
  }
  else if(PrevProcId == LockedProcessId)
  {
    /* the object is currently locked, wait some time and try again.
       FIXME - we shouldn't loop forever! Give up after some time! */
    DelayExecution();
    /* try again */
    goto LockHandle;
  }
#ifdef IGNORE_PID_WHILE_LOCKING
  /* FIXME - HACK HACK HACK HACK!!!!!!
             Remove when no longer needed! */
  else if((PrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
  {
    DPRINT("HACK!!!! Deleting Object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", hObj, PrevProcId >> 1, PsGetCurrentProcessId());
#ifdef GDI_DEBUG
    DPRINT("\tcalled from %s:%i\n", file, line);
#endif
    ProcessId = PrevProcId & 0xFFFFFFFE;
    LockedProcessId = ProcessId | 0x1;
    goto LockHandle;
  }
#endif /* IGNORE_PID_WHILE_LOCKING */
  else
  {
    DPRINT1("Attempted to free invalid handle: 0x%x\n", hObj);
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
BOOL FASTCALL
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
BOOL FASTCALL
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
	  GDIOBJ_UnlockObj(pList[i].pObj);
	  pList[i].pObj = NULL;
	}
    }

  return TRUE;
}

/*!
 * Initialization of the GDI object engine.
*/
VOID FASTCALL
InitGdiObjectHandleTable (VOID)
{
  DPRINT("InitGdiObjectHandleTable\n");

  HandleTable = GDIOBJ_iAllocHandleTable (GDI_HANDLE_COUNT);
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
BOOL FASTCALL
CleanupForProcess (struct _EPROCESS *Process, INT Pid)
{
  PGDI_TABLE_ENTRY Entry;
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
    ProcId = ((LONG)PsGetCurrentProcessId() << 1);

    for(Entry = &HandleTable->Entries[RESERVE_ENTRIES_COUNT];
        Entry < HandleTable->EntriesEnd;
        Entry++, Index++)
    {
      /* ignore the lock bit */
      if((Entry->ProcessId & 0xFFFFFFFE) == ProcId && Entry->Type != 0)
      {
        HANDLE ObjectHandle;

        /* Create the object handle for the entry, the upper 16 bit of the
           Type field includes the type of the object including the stock
           object flag - but since stock objects don't have a process id we can
           simply ignore this fact here. */
        ObjectHandle = (HANDLE)(Index | (Entry->Type & 0xFFFF0000));

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

#define GDIOBJ_TRACKLOCKS

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
PGDIOBJ FASTCALL
#ifdef GDI_DEBUG
GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
#else /* !GDI_DEBUG */
GDIOBJ_LockObj (HGDIOBJ hObj, DWORD ObjectType)
#endif /* GDI_DEBUG */
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId, LockedProcessId, PrevProcId, ExpectedType;

  DPRINT("GDIOBJ_LockObj: hObj: 0x%08x\n", hObj);

  /* shift the process id to the left so we can use the first bit to lock the object.
     FIXME - don't shift once ROS' PIDs match with nt! */
  ProcessId = (LONG)PsGetCurrentProcessId() << 1;
  LockedProcessId = ProcessId | 0x1;
  
  ExpectedType = ((ObjectType != GDI_OBJECT_TYPE_DONTCARE) ? 0 : (ObjectType & 0xFFFF0000) | (ObjectType >> 16));
  
  Entry = GDI_HANDLE_GET_ENTRY(HandleTable, hObj);

LockHandle:
  /* lock the object, we must not delete stock objects, so don't check!!! */
  PrevProcId = InterlockedCompareExchange(&Entry->ProcessId, LockedProcessId, ProcessId);
  if(PrevProcId == ProcessId)
  {
    LONG EntryType = Entry->Type;
    
CheckType:
    /* we're locking an object that belongs to our process or it's a global
       object if ProcessId == 0 here. ProcessId can only be 0 here if it previously
       failed to lock the object and it turned out to be a global object. */
    if(EntryType != 0 && EntryType == ExpectedType)
    {
      PW32THREAD W32PrevThread, W32Thread = PsGetWin32Thread();
      PGDIOBJHDR GdiHdr;
      
      GdiHdr = GDIBdyToHdr(Entry->KernelData);
      
      /* save the pointer to the calling thread so we know it was this thread
         that locked the object */
      W32PrevThread = InterlockedCompareExchangePointer(&GdiHdr->LockingThread, W32Thread, NULL);
      if(W32PrevThread == NULL || W32PrevThread == W32Thread)
      {
        /* it's time to increment the lock counter! We only allow recursive locks
           from the same thread! */
        InterlockedIncrement(&GdiHdr->RefCount);
        
#ifdef GDI_DEBUG
        if(GdiHdr->lockfile != NULL)
        {
          GdiHdr->lockfile = file;
          GdiHdr->lockline = line;
        }
#endif

        /* remove the process id lock so we can recursively lock the object */
        InterlockedExchange(&Entry->ProcessId, PrevProcId);

        /* we're done, return the object body */
        return GDIHdrToBdy(GdiHdr);
      }
      else
      {

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
    else if(EntryType != 0)
    {
      /* Try again, the caller passed GDI_OBJECT_TYPE_DONTCARE */
      ExpectedType = EntryType;
      goto CheckType;
    }
    else
    {
      if(EntryType == 0)
      {
        DPRINT1("Attempted to lock object 0x%x that is deleted! Should never get here!!!\n", hObj);
      }
      else
      {
        DPRINT1("Attempted to lock object 0x%x, type mismatch (0x%x : 0x%x)\n", hObj, ObjectType, ExpectedType);
      }
    }
  }
  else if((PrevProcId & 0xFFFFFFFE) == 0)
  {
    /* we're trying to lock a global object, change the ProcessId to 0 and try again */
    ProcessId = 0x0;
    LockedProcessId = 0x1;

    goto LockHandle;
  }
  else if(PrevProcId == LockedProcessId)
  {
    /* the object is currently locked, wait some time and try again.
       FIXME - we shouldn't loop forever! Give up after some time! */
    DelayExecution();
    /* try again */
    goto LockHandle;
  }
#ifdef IGNORE_PID_WHILE_LOCKING
  /* FIXME - HACK HACK HACK HACK!!!!!!
             Remove when no longer needed! */
  else if((PrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
  {
    DPRINT("HACK!!!! Locking Object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", hObj, PrevProcId >> 1, PsGetCurrentProcessId());
#ifdef GDI_DEBUG
    DPRINT("\tcalled from %s:%i\n", file, line);
#endif
    ProcessId = PrevProcId & 0xFFFFFFFE;
    LockedProcessId = ProcessId | 0x1;
    goto LockHandle;
  }
#endif /* IGNORE_PID_WHILE_LOCKING */
  else
  {
    DPRINT1("Attempted to lock invalid handle: 0x%x, PrevProcId=0x%x locked: 0x%x pid: 0x%x, stockobj: 0x%x\n", hObj, PrevProcId >> 1, PrevProcId & 0x1, PsGetCurrentProcessId(), GDI_HANDLE_IS_STOCKOBJ(hObj));
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
BOOL FASTCALL
#ifdef GDI_DEBUG
GDIOBJ_UnlockObjDbg (const char* file, int line, PGDIOBJ Object)
#else /* !GDI_DEBUG */
GDIOBJ_UnlockObj (PGDIOBJ Object)
#endif /* GDI_DEBUG */
{
  LONG NewRefs;
  PGDIOBJHDR GdiHdr = GDIBdyToHdr(Object);
  
  NewRefs = InterlockedDecrement(&GdiHdr->RefCount);
  if(NewRefs == 1)
  {
    /* only the keep-alive reference is still there, so we're not locking the
       object anymore! */
    InterlockedExchangePointer(&GdiHdr->LockingThread, NULL);
  }
  else if(NewRefs == 0)
  {
    BOOL Ret;

    /* We're not locking the object anymore! */
    InterlockedExchangePointer(&GdiHdr->LockingThread, NULL);

    /* Even though we removed the keep-alive reference we don't need to remove
       it from the table anymore, GDIOBJ_FreeObj() did that for us already.
       
       Call the cleanup routine. */
    Ret = RunCleanupCallback(GDIHdrToBdy(GdiHdr), GdiHdr->Type);

    /* Now it's time to free the memory */
    ExFreeToPagedLookasideList(GdiHdr->LookasideList, GdiHdr);

    return Ret;
  }
  
  return TRUE;
}

BOOL FASTCALL
GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle)
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId;
  BOOL Ret;

  DPRINT1("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);

  if(!GDI_HANDLE_IS_STOCKOBJ(ObjectHandle))
  {
    ProcessId = (LONG)PsGetCurrentProcessId() << 1;

    Entry = GDI_HANDLE_GET_ENTRY(HandleTable, ObjectHandle);
    Ret = Entry->KernelData != NULL &&
          Entry->Type != 0 &&
          (Entry->ProcessId & 0xFFFFFFFE) == ProcessId;

    return Ret;
  }

  return FALSE;
}

BOOL FASTCALL
GDIOBJ_ConvertToStockObj(HGDIOBJ *hObj)
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId, LockedProcessId, PrevProcId;

  DPRINT("GDIOBJ_ConvertToStockObj: hObj: 0x%08x\n", *hObj);

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
      if(PrevType == OldType)
      {
        PW32THREAD W32PrevThread, W32Thread = PsGetWin32Thread();
        PGDIOBJHDR GdiHdr;

        /* We successfully set the stock object flag.
           KernelData should never be NULL here!!! */
        ASSERT(Entry->KernelData);

        GdiHdr = GDIBdyToHdr(Entry->KernelData);

        /* save the pointer to the calling thread so we know it was this thread
           that locked the object */
        W32PrevThread = InterlockedCompareExchangePointer(&GdiHdr->LockingThread, W32Thread, NULL);
        if(W32PrevThread == NULL || W32PrevThread == W32Thread)
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
          
          /* Remove the thread lock */
          InterlockedExchangePointer(&GdiHdr->LockingThread, W32PrevThread);

          *hObj = (HGDIOBJ)((ULONG)(*hObj) | GDI_HANDLE_STOCK_MASK);

          /* we're done, successfully converted the object */
          return TRUE;
        }
        else
        {

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

void FASTCALL
GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
  PGDI_TABLE_ENTRY Entry;
  LONG ProcessId, LockedProcessId, PrevProcId;

  DPRINT1("GDIOBJ_SetOwnership: hObj: 0x%x, NewProcess: 0x%x\n", ObjectHandle, (NewOwner ? PsGetProcessId(NewOwner) : 0));

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
      PW32THREAD W32PrevThread, W32Thread = PsGetWin32Thread();
      PGDIOBJHDR GdiHdr;

      if(Entry->Type != 0)
      {
        /* KernelData should never be NULL here!!! */
        ASSERT(Entry->KernelData);

        GdiHdr = GDIBdyToHdr(Entry->KernelData);

        /* save the pointer to the calling thread so we know it was this thread
           that locked the object */
        W32PrevThread = InterlockedCompareExchangePointer(&GdiHdr->LockingThread, NULL, W32Thread);
        if(W32PrevThread == NULL || W32PrevThread == W32Thread)
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

          /* Remove the thread lock */
          InterlockedExchangePointer(&GdiHdr->LockingThread, W32PrevThread);

          /* we're done! */
          return;
        }
        else
        {
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
      /* the object is currently locked, wait some time and try again.
         FIXME - we shouldn't loop forever! Give up after some time! */
      DelayExecution();
      /* try again */
      goto LockHandle;
    }
    else if((PrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
    {
      /* FIXME - should we really allow changing the ownership of objects we don't own? */
      DPRINT1("WARNING! Changing ownership of object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", ObjectHandle, PrevProcId >> 1, PsGetCurrentProcessId());
      ProcessId = PrevProcId & 0xFFFFFFFE;
      LockedProcessId = ProcessId | 0x1;
      goto LockHandle;
    }
    else
    {
      DPRINT1("Attempted to change owner of invalid handle: 0x%x\n", ObjectHandle);
    }
  }
}

void FASTCALL
GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
  PGDI_TABLE_ENTRY FromEntry;
  LONG FromProcessId, FromLockedProcessId, FromPrevProcId;

  DPRINT("GDIOBJ_CopyOwnership: from: 0x%x, to: 0x%x\n", CopyFrom, CopyTo);

  if(!GDI_HANDLE_IS_STOCKOBJ(CopyFrom) && !GDI_HANDLE_IS_STOCKOBJ(CopyTo))
  {
    FromEntry = GDI_HANDLE_GET_ENTRY(HandleTable, CopyFrom);
    
    FromProcessId = FromEntry->ProcessId & 0xFFFFFFFE;
    FromLockedProcessId = FromProcessId | 0x1;

LockHandleFrom:
    /* lock the object, we must not convert stock objects, so don't check!!! */
    FromPrevProcId = InterlockedCompareExchange(&FromEntry->ProcessId, FromProcessId, FromLockedProcessId);
    if(FromPrevProcId == FromProcessId)
    {
      PW32THREAD W32PrevThread, W32Thread = PsGetWin32Thread();
      PGDIOBJHDR GdiHdr;

      if(FromEntry->Type != 0)
      {
        /* KernelData should never be NULL here!!! */
        ASSERT(FromEntry->KernelData);

        GdiHdr = GDIBdyToHdr(FromEntry->KernelData);

        /* save the pointer to the calling thread so we know it was this thread
           that locked the object */
        W32PrevThread = InterlockedCompareExchangePointer(&GdiHdr->LockingThread, NULL, W32Thread);
        if(W32PrevThread == NULL || W32PrevThread == W32Thread)
        {
          /* now let's change the ownership of the target object */
          
          if((FromPrevProcId & 0xFFFFFFFE) != 0)
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
          
          InterlockedExchangePointer(&GdiHdr->LockingThread, NULL);
          
          /* we're done! */
          return;
        }
        else
        {
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
      /* the object is currently locked, wait some time and try again.
         FIXME - we shouldn't loop forever! Give up after some time! */
      DelayExecution();
      /* try again */
      goto LockHandleFrom;
    }
    else if((FromPrevProcId >> 1) != (LONG)PsGetCurrentProcessId())
    {
      /* FIXME - should we really allow copying ownership from objects that we don't even own? */
      DPRINT1("WARNING! Changing copying ownership of object 0x%x (pid: 0x%x) from pid 0x%x!!!\n", CopyFrom, FromPrevProcId >> 1, PsGetCurrentProcessId());
      FromProcessId = FromPrevProcId & 0xFFFFFFFE;
      FromLockedProcessId = FromProcessId | 0x1;
      goto LockHandleFrom;
    }
    else
    {
      DPRINT1("Attempted to copy ownership from invalid handle: 0x%x\n", CopyFrom);
    }
  }
}

PVOID FASTCALL
GDI_MapHandleTable(HANDLE hProcess)
{
  DPRINT("%s:%i: %s(): FIXME - Map handle table into the process memory space!\n",
         __FILE__, __LINE__, __FUNCTION__);
  /* FIXME - Map the entire gdi handle table read-only to userland into the
             scope of hProcess and return the pointer */
  return NULL;
}

/* EOF */
