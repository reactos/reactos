/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 * $Id: gdiobj.c,v 1.70 2004/07/04 01:23:32 navaraf Exp $
 *
 */
#include <w32k.h>

/* count all gdi objects */
#define GDI_COUNT_OBJECTS 1

/*! Size of the GDI handle table
 * http://www.windevnet.com/documents/s=7290/wdj9902b/9902b.htm
 * gdi handle table can hold 0x4000 handles
*/
#define GDI_HANDLE_COUNT 0x4000

#define GDI_GLOBAL_PROCESS ((HANDLE) 0xffffffff)

#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000

#define GDI_HANDLE_CREATE(i, t)    ((HANDLE)(((i) & GDI_HANDLE_INDEX_MASK) | ((t) & GDI_HANDLE_TYPE_MASK)))
#define GDI_HANDLE_GET_INDEX(h)    (((DWORD)(h)) & GDI_HANDLE_INDEX_MASK)
#define GDI_HANDLE_GET_TYPE(h)     (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK)
#define GDI_HANDLE_IS_TYPE(h, t)   ((t) == (((DWORD)(h)) & GDI_HANDLE_TYPE_MASK))
#define GDI_HANDLE_IS_STOCKOBJ(h)  (0 != (((DWORD)(h)) & GDI_HANDLE_STOCK_MASK))
#define GDI_HANDLE_SET_STOCKOBJ(h) ((h) = (HANDLE)(((DWORD)(h)) | GDI_HANDLE_STOCK_MASK))

#define GDI_TYPE_TO_MAGIC(t) ((WORD) ((t) >> 16))
#define GDI_MAGIC_TO_TYPE(m) ((DWORD)(m) << 16)

/* FIXME Ownership of GDI objects by processes not properly implemented yet */
#if 0
#define GDI_VALID_OBJECT(h, obj, t, f) \
  (NULL != (obj) \
   && (GDI_MAGIC_TO_TYPE((obj)->Magic) == (t) || GDI_OBJECT_TYPE_DONTCARE == (t)) \
   && (GDI_HANDLE_GET_TYPE((h)) == GDI_MAGIC_TO_TYPE((obj)->Magic)) \
   && (((obj)->hProcessId == PsGetCurrentProcessId()) \
       || (GDI_GLOBAL_PROCESS == (obj)->hProcessId) \
       || ((f) & GDIOBJFLAG_IGNOREPID)))
#else
#define GDI_VALID_OBJECT(h, obj, t, f) \
  (NULL != (obj) \
   && (GDI_MAGIC_TO_TYPE((obj)->Magic) == (t) || GDI_OBJECT_TYPE_DONTCARE == (t)) \
   && (GDI_HANDLE_GET_TYPE((h)) == GDI_MAGIC_TO_TYPE((obj)->Magic)))
#endif

typedef struct _GDI_HANDLE_TABLE
{
  WORD wTableSize;
  WORD AllocationHint;
  #if GDI_COUNT_OBJECTS
  ULONG HandlesCount;
  #endif
  PPAGED_LOOKASIDE_LIST LookasideLists;
  PGDIOBJHDR Handles[1];
} GDI_HANDLE_TABLE, *PGDI_HANDLE_TABLE;

typedef struct
{
  ULONG Type;
  ULONG Size;
} GDI_OBJ_SIZE;

const
GDI_OBJ_SIZE ObjSizes[] =
{
  /* Testing shows that regions are the most used GDIObj type,
     so put that one first for performance */
  {GDI_OBJECT_TYPE_REGION,      sizeof(ROSRGNDATA)},
  {GDI_OBJECT_TYPE_BITMAP,      sizeof(BITMAPOBJ)},
  {GDI_OBJECT_TYPE_DC,          sizeof(DC)},
  {GDI_OBJECT_TYPE_PALETTE,     sizeof(PALGDI)},
  {GDI_OBJECT_TYPE_BRUSH,       sizeof(GDIBRUSHOBJ)},
  {GDI_OBJECT_TYPE_PEN,         sizeof(GDIBRUSHOBJ)},
  {GDI_OBJECT_TYPE_FONT,        sizeof(TEXTOBJ)},
  {GDI_OBJECT_TYPE_DCE,         sizeof(DCE)},
/*
  {GDI_OBJECT_TYPE_DIRECTDRAW,  sizeof(DD_DIRECTDRAW)},
  {GDI_OBJECT_TYPE_DD_SURFACE,  sizeof(DD_SURFACE)},
*/
  {GDI_OBJECT_TYPE_EXTPEN,      0},
  {GDI_OBJECT_TYPE_METADC,      0},
  {GDI_OBJECT_TYPE_METAFILE,    0},
  {GDI_OBJECT_TYPE_ENHMETAFILE, 0},
  {GDI_OBJECT_TYPE_ENHMETADC,   0},
  {GDI_OBJECT_TYPE_MEMDC,       0},
  {GDI_OBJECT_TYPE_EMF,         0}
};

#define OBJTYPE_COUNT (sizeof(ObjSizes) / sizeof(ObjSizes[0]))

/*  GDI stock objects */

static LOGBRUSH WhiteBrush =
{ BS_SOLID, RGB(255,255,255), 0 };

static LOGBRUSH LtGrayBrush =
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
{ BS_SOLID, RGB(192,192,192), 0 };

static LOGBRUSH GrayBrush =
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
{ BS_SOLID, RGB(128,128,128), 0 };

static LOGBRUSH DkGrayBrush =
/* This is BS_HATCHED, for 1 bitperpixel. This makes the spray work in pbrush */
/* NB_HATCH_STYLES is an index into HatchBrushes */
{ BS_HATCHED, RGB(0,0,0), NB_HATCH_STYLES };

static LOGBRUSH BlackBrush =
{ BS_SOLID, RGB(0,0,0), 0 };

static LOGBRUSH NullBrush =
{ BS_NULL, 0, 0 };

static LOGPEN WhitePen =
{ PS_SOLID, { 0, 0 }, RGB(255,255,255) };

static LOGPEN BlackPen =
{ PS_SOLID, { 0, 0 }, RGB(0,0,0) };

static LOGPEN NullPen =
{ PS_NULL, { 0, 0 }, 0 };

static LOGFONTW OEMFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Bitstream Vera Sans Mono" };

static LOGFONTW AnsiFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Bitstream Vera Sans Mono" };

/*static LOGFONTW AnsiVarFont =
 *{ 10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
 *  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" }; */

static LOGFONTW SystemFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Bitstream Vera Sans" };

static LOGFONTW DeviceDefaultFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Bitstream Vera Sans" };

static LOGFONTW SystemFixedFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Bitstream Vera Sans Mono" };

/* FIXME: Is this correct? */
static LOGFONTW DefaultGuiFont =
{ 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Bitstream Vera Sans" };

#define NB_STOCK_OBJECTS (DEFAULT_GUI_FONT + 1)

static HGDIOBJ StockObjects[NB_STOCK_OBJECTS];
static PGDI_HANDLE_TABLE  HandleTable = 0;
static FAST_MUTEX  HandleTableMutex;
static FAST_MUTEX  RefCountHandling;
static LARGE_INTEGER  ShortDelay;

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
 * Notes:: Must be called at IRQL < DISPATCH_LEVEL.
*/
static PGDI_HANDLE_TABLE FASTCALL
GDIOBJ_iAllocHandleTable (WORD Size)
{
  PGDI_HANDLE_TABLE  handleTable;
  ULONG MemSize;
  UINT ObjType;
  
  MemSize = sizeof(GDI_HANDLE_TABLE) + sizeof(PGDIOBJ) * Size;

  /* prevent APC delivery for the *FastMutexUnsafe calls */
  const KIRQL PrevIrql = KfRaiseIrql(APC_LEVEL);
  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  handleTable = ExAllocatePoolWithTag(PagedPool, MemSize, TAG_GDIHNDTBLE);
  ASSERT( handleTable );
  memset (handleTable, 0, MemSize);
#if GDI_COUNT_OBJECTS
  handleTable->HandlesCount = 0;
#endif
  handleTable->wTableSize = Size;
  handleTable->AllocationHint = 1;
  handleTable->LookasideLists = ExAllocatePoolWithTag(PagedPool,
                                                      OBJTYPE_COUNT * sizeof(PAGED_LOOKASIDE_LIST),
                                                      TAG_GDIHNDTBLE);
  if (NULL == handleTable->LookasideLists)
    {
      ExFreePool(handleTable);
      ExReleaseFastMutexUnsafe (&HandleTableMutex);
      KfLowerIrql(PrevIrql);
      return NULL;
    }
  for (ObjType = 0; ObjType < OBJTYPE_COUNT; ObjType++)
    {
      ExInitializePagedLookasideList(handleTable->LookasideLists + ObjType, NULL, NULL, 0,
                                     ObjSizes[ObjType].Size + sizeof(GDIOBJHDR), TAG_GDIOBJ, 0);
    }
  ExReleaseFastMutexUnsafe (&HandleTableMutex);
  KfLowerIrql(PrevIrql);

  return handleTable;
}

/*!
 * Returns the entry into the handle table by index.
*/
static PGDIOBJHDR FASTCALL
GDIOBJ_iGetObjectForIndex(WORD TableIndex)
{
  if (0 == TableIndex || HandleTable->wTableSize < TableIndex)
    {
      DPRINT1("Invalid TableIndex %u\n", (unsigned) TableIndex);
      return NULL;
    }

  return HandleTable->Handles[TableIndex];
}

/*!
 * Finds next free entry in the GDI handle table.
 * \return	index into the table is successful, zero otherwise.
*/
static WORD FASTCALL
GDIOBJ_iGetNextOpenHandleIndex (void)
{
   WORD tableIndex;

   for (tableIndex = HandleTable->AllocationHint;
        tableIndex < HandleTable->wTableSize;
        tableIndex++)
   {
      if (HandleTable->Handles[tableIndex] == NULL)
      {
         HandleTable->AllocationHint = tableIndex + 1;
         return tableIndex;
      }
   }

   for (tableIndex = 1;
        tableIndex < HandleTable->AllocationHint;
        tableIndex++)
   {
      if (HandleTable->Handles[tableIndex] == NULL)
      {
         HandleTable->AllocationHint = tableIndex + 1;
         return tableIndex;
      }
   }

   return 0;
}

static PPAGED_LOOKASIDE_LIST FASTCALL
FindLookasideList(DWORD ObjectType)
{
  int Index;

  for (Index = 0; Index < OBJTYPE_COUNT; Index++)
    {
      if (ObjSizes[Index].Type == ObjectType)
        {
          return HandleTable->LookasideLists + Index;
        }
    }

  DPRINT1("Can't find lookaside list for object type 0x%08x\n", ObjectType);

  return NULL;
}

/*!
 * Allocate memory for GDI object and return handle to it.
 *
 * \param Size - size of the GDI object. This shouldn't to include the size of GDIOBJHDR.
 * The actual amount of allocated memory is sizeof(GDIOBJHDR)+Size
 * \param ObjectType - type of object \ref GDI object types
 * \param CleanupProcPtr - Routine to be called on destruction of object
 *
 * \return Handle of the allocated object.
 *
 * \note Use GDIOBJ_Lock() to obtain pointer to the new object.
*/
HGDIOBJ FASTCALL
GDIOBJ_AllocObj(WORD Size, DWORD ObjectType, GDICLEANUPPROC CleanupProc)
{
  PW32PROCESS W32Process;
  PGDIOBJHDR  newObject;
  WORD Index;
  PPAGED_LOOKASIDE_LIST LookasideList;

  ExAcquireFastMutex(&HandleTableMutex);
  Index = GDIOBJ_iGetNextOpenHandleIndex ();
  if (0 == Index)
    {
      ExReleaseFastMutex(&HandleTableMutex);
      DPRINT1("Out of GDI handles\n");
      return NULL;
    }

  LookasideList = FindLookasideList(ObjectType);
  if (NULL == LookasideList)
    {
      ExReleaseFastMutex(&HandleTableMutex);
      return NULL;
    }
  newObject = ExAllocateFromPagedLookasideList(LookasideList);
  if (NULL == newObject)
    {
      ExReleaseFastMutex(&HandleTableMutex);
      DPRINT1("Unable to allocate GDI object from lookaside list\n");
      return NULL;
    }
  RtlZeroMemory (newObject, Size + sizeof(GDIOBJHDR));

  newObject->wTableIndex = Index;

  newObject->dwCount = 0;
  newObject->hProcessId = PsGetCurrentProcessId ();
  newObject->CleanupProc = CleanupProc;
  newObject->Magic = GDI_TYPE_TO_MAGIC(ObjectType);
  newObject->lockfile = NULL;
  newObject->lockline = 0;
#if 0
  ExInitializeFastMutex(&newObject->Lock);
#else
  newObject->LockTid = 0;
  newObject->LockCount = 0;
#endif
  HandleTable->Handles[Index] = newObject;
#if GDI_COUNT_OBJECTS
  HandleTable->HandlesCount++;
#endif
  ExReleaseFastMutex(&HandleTableMutex);
  
  W32Process = PsGetCurrentProcess()->Win32Process;
  if(W32Process)
  {
    W32Process->GDIObjects++;
  }

  return GDI_HANDLE_CREATE(Index, ObjectType);
}

/*!
 * Free memory allocated for the GDI object. For each object type this function calls the
 * appropriate cleanup routine.
 *
 * \param hObj       - handle of the object to be deleted.
 * \param ObjectType - one of the \ref GDI object types
 * or GDI_OBJECT_TYPE_DONTCARE.
 * \param Flag       - if set to GDIOBJFLAG_IGNOREPID then the routine doesn't check if the process that
 * tries to delete the object is the same one that created it.
 *
 * \return Returns TRUE if succesful.
 *
 * \note You should only use GDIOBJFLAG_IGNOREPID if you are cleaning up after the process that terminated.
 * \note This function deferres object deletion if it is still in use.
*/
BOOL STDCALL
GDIOBJ_FreeObj(HGDIOBJ hObj, DWORD ObjectType, DWORD Flag)
{
  PW32PROCESS W32Process;
  PGDIOBJHDR objectHeader;
  PGDIOBJ Obj;
  PPAGED_LOOKASIDE_LIST LookasideList;
  BOOL 	bRet = TRUE;

  objectHeader = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));
  DPRINT("GDIOBJ_FreeObj: hObj: 0x%08x, object: %x\n", hObj, objectHeader);

  if (! GDI_VALID_OBJECT(hObj, objectHeader, ObjectType, Flag)
      || GDI_GLOBAL_PROCESS == objectHeader->hProcessId)

    {
      DPRINT1("Can't delete hObj:0x%08x, type:0x%08x, flag:%d\n", hObj, ObjectType, Flag);
      return FALSE;
    }

  DPRINT("FreeObj: locks: %x\n", objectHeader->dwCount );
  if (!(Flag & GDIOBJFLAG_IGNORELOCK))
    {
      /* check that the reference count is zero. if not then set flag
       * and delete object when releaseobj is called */
      ExAcquireFastMutex(&RefCountHandling);
      if ((objectHeader->dwCount & ~0x80000000) > 0 )
	{
	  DPRINT("GDIOBJ_FreeObj: delayed object deletion: count %d\n", objectHeader->dwCount);
	  objectHeader->dwCount |= 0x80000000;
	  ExReleaseFastMutex(&RefCountHandling);
	  return TRUE;
	}
      ExReleaseFastMutex(&RefCountHandling);
    }

  /* allow object to delete internal data */
  if (NULL != objectHeader->CleanupProc)
    {
      Obj = (PGDIOBJ)((PCHAR)objectHeader + sizeof(GDIOBJHDR));
      bRet = (*(objectHeader->CleanupProc))(Obj);
    }
  LookasideList = FindLookasideList(GDI_MAGIC_TO_TYPE(objectHeader->Magic));
  if (NULL != LookasideList)
    {
      ExFreeToPagedLookasideList(LookasideList, objectHeader);
    }
  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  HandleTable->Handles[GDI_HANDLE_GET_INDEX(hObj)] = NULL;
#if GDI_COUNT_OBJECTS
  HandleTable->HandlesCount--;
#endif
  ExReleaseFastMutexUnsafe (&HandleTableMutex);
  
  W32Process = PsGetCurrentProcess()->Win32Process;
  if(W32Process)
  {
    W32Process->GDIObjects--;
  }

  return bRet;
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
	  GDIOBJ_UnlockObj(pList[i].hObj, pList[i].ObjectType);
	  pList[i].pObj = NULL;
	}
    }

  return TRUE;
}

/*!
 * Get the type of the object.
 * \param 	ObjectHandle - handle of the object.
 * \return 	One of the \ref GDI object types
*/
DWORD FASTCALL
GDIOBJ_GetObjectType(HGDIOBJ ObjectHandle)
{
  PGDIOBJHDR ObjHdr;

  ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));
  if (NULL == ObjHdr
      || ! GDI_VALID_OBJECT(ObjectHandle, ObjHdr, GDI_MAGIC_TO_TYPE(ObjHdr->Magic), 0))
    {
      DPRINT1("Invalid ObjectHandle 0x%08x\n", ObjectHandle);
      return 0;
    }
  DPRINT("GDIOBJ_GetObjectType for handle 0x%08x returns 0x%08x\n", ObjectHandle,
         GDI_MAGIC_TO_TYPE(ObjHdr->Magic));

  return GDI_MAGIC_TO_TYPE(ObjHdr->Magic);
}

/*!
 * Initialization of the GDI object engine.
*/
VOID FASTCALL
InitGdiObjectHandleTable (VOID)
{
  DPRINT("InitGdiObjectHandleTable\n");
  ExInitializeFastMutex (&HandleTableMutex);
  ExInitializeFastMutex (&RefCountHandling);

  ShortDelay.QuadPart = -100;

  HandleTable = GDIOBJ_iAllocHandleTable (GDI_HANDLE_COUNT);
  DPRINT("HandleTable: %x\n", HandleTable );

  InitEngHandleTable();
}

/*!
 * Creates a bunch of stock objects: brushes, pens, fonts.
*/
VOID FASTCALL
CreateStockObjects(void)
{
  unsigned Object;

  DPRINT("Beginning creation of stock objects\n");

  /* Create GDI Stock Objects from the logical structures we've defined */

  StockObjects[WHITE_BRUSH] =  IntGdiCreateBrushIndirect(&WhiteBrush);
  StockObjects[LTGRAY_BRUSH] = IntGdiCreateBrushIndirect(&LtGrayBrush);
  StockObjects[GRAY_BRUSH] =   IntGdiCreateBrushIndirect(&GrayBrush);
  StockObjects[DKGRAY_BRUSH] = IntGdiCreateBrushIndirect(&DkGrayBrush);
  StockObjects[BLACK_BRUSH] =  IntGdiCreateBrushIndirect(&BlackBrush);
  StockObjects[NULL_BRUSH] =   IntGdiCreateBrushIndirect(&NullBrush);

  StockObjects[WHITE_PEN] = IntGdiCreatePenIndirect(&WhitePen);
  StockObjects[BLACK_PEN] = IntGdiCreatePenIndirect(&BlackPen);
  StockObjects[NULL_PEN] =  IntGdiCreatePenIndirect(&NullPen);

  (void) TextIntCreateFontIndirect(&OEMFixedFont, (HFONT*)&StockObjects[OEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&AnsiFixedFont, (HFONT*)&StockObjects[ANSI_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFont, (HFONT*)&StockObjects[SYSTEM_FONT]);
  (void) TextIntCreateFontIndirect(&DeviceDefaultFont, (HFONT*)&StockObjects[DEVICE_DEFAULT_FONT]);
  (void) TextIntCreateFontIndirect(&SystemFixedFont, (HFONT*)&StockObjects[SYSTEM_FIXED_FONT]);
  (void) TextIntCreateFontIndirect(&DefaultGuiFont, (HFONT*)&StockObjects[DEFAULT_GUI_FONT]);

  StockObjects[DEFAULT_PALETTE] = (HGDIOBJ*)PALETTE_Init();

  for (Object = 0; Object < NB_STOCK_OBJECTS; Object++)
    {
      if (NULL != StockObjects[Object])
	{
	  GDIOBJ_SetOwnership(StockObjects[Object], NULL);
/*	  GDI_HANDLE_SET_STOCKOBJ(StockObjects[Object]);*/
	}
    }

  DPRINT("Completed creation of stock objects\n");
}

/*!
 * Return stock object.
 * \param	Object - stock object id.
 * \return	Handle to the object.
*/
HGDIOBJ STDCALL
NtGdiGetStockObject(INT Object)
{
  DPRINT("NtGdiGetStockObject index %d\n", Object);

  return ((Object < 0) || (NB_STOCK_OBJECTS <= Object)) ? NULL : StockObjects[Object];
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
         ? GDIOBJ_FreeObj(hObject, GDI_OBJECT_TYPE_DONTCARE, GDIOBJFLAG_DEFAULT) : FALSE;
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that will be destroyed.
*/
BOOL FASTCALL
CleanupForProcess (struct _EPROCESS *Process, INT Pid)
{
  DWORD i;
  PGDIOBJHDR objectHeader;
  PEPROCESS CurrentProcess;

  DPRINT("Starting CleanupForProcess prochandle %x Pid %d\n", Process, Pid);
  CurrentProcess = PsGetCurrentProcess();
  if (CurrentProcess != Process)
    {
      KeAttachProcess(Process);
    }

  for(i = 1; i < HandleTable->wTableSize; i++)
    {
      objectHeader = GDIOBJ_iGetObjectForIndex(i);
      if (NULL != objectHeader &&
          (INT) objectHeader->hProcessId == Pid)
	{
	  DPRINT("CleanupForProcess: %d, process: %d, locks: %d, magic: 0x%x", i, objectHeader->hProcessId, objectHeader->dwCount, objectHeader->Magic);
	  GDIOBJ_FreeObj(GDI_HANDLE_CREATE(i, GDI_MAGIC_TO_TYPE(objectHeader->Magic)),
	                 GDI_MAGIC_TO_TYPE(objectHeader->Magic),
	                 GDIOBJFLAG_IGNOREPID | GDIOBJFLAG_IGNORELOCK);
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

#ifdef GDIOBJ_LockObj
#undef GDIOBJ_LockObj
PGDIOBJ FASTCALL
GDIOBJ_LockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  DPRINT("(%s:%i) GDIOBJ_LockObjDbg(0x%08x,0x%08x)\n", file, line, hObj, ObjectType);
  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      int reason = 0;
      if (NULL == ObjHdr)
	{
	  reason = 1;
	}
      else if (GDI_MAGIC_TO_TYPE(ObjHdr->Magic) != ObjectType && ObjectType != GDI_OBJECT_TYPE_DONTCARE)
	{
	  reason = 2;
	}
      else if (ObjHdr->hProcessId != GDI_GLOBAL_PROCESS
	   && ObjHdr->hProcessId != PsGetCurrentProcessId())
	{
	  reason = 3;
	}
      else if (GDI_HANDLE_GET_TYPE(hObj) != ObjectType && ObjectType != GDI_OBJECT_TYPE_DONTCARE)
	{
	  reason = 4;
	}
      DPRINT1("GDIOBJ_LockObj failed for 0x%08x, reqtype 0x%08x reason %d\n",
              hObj, ObjectType, reason );
      DPRINT1("\tcalled from: %s:%i\n", file, line );
      return NULL;
    }

#if 0
#ifdef NDEBUG
  ExAcquireFastMutex(&ObjHdr->Lock);
#else /* NDEBUG */
  if (! ExTryToAcquireFastMutex(&ObjHdr->Lock))
    {
      DPRINT1("Caution! GDIOBJ_LockObj trying to lock object 0x%x second time\n", hObj);
      DPRINT1("  called from: %s:%i (thread %x)\n", file, line, KeGetCurrentThread());
      if (NULL != ObjHdr->lockfile)
        {
          DPRINT1("  previously locked from: %s:%i (thread %x)\n", ObjHdr->lockfile, ObjHdr->lockline, ObjHdr->Lock.Owner);
        }
      ExAcquireFastMutex(&ObjHdr->Lock);
      DPRINT1("  Disregard previous message about object 0x%x, it's ok\n", hObj);
    }
#endif /* NDEBUG */
#else
  if (ObjHdr->LockTid == (DWORD)PsGetCurrentThreadId())
    {
      InterlockedIncrement(&ObjHdr->LockCount);
    }
  else
    {
      for (;;)
        {
          if (InterlockedCompareExchange(&ObjHdr->LockTid, (DWORD)PsGetCurrentThreadId(), 0))
            {
              InterlockedIncrement(&ObjHdr->LockCount);
              break;
            }
          /* FIXME: KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay); */
        }
    }
#endif

  ExAcquireFastMutex(&RefCountHandling);
  ObjHdr->dwCount++;
  ExReleaseFastMutex(&RefCountHandling);

  if (NULL == ObjHdr->lockfile)
    {
      ObjHdr->lockfile = file;
      ObjHdr->lockline = line;
    }

  return (PGDIOBJ)((PCHAR)ObjHdr + sizeof(GDIOBJHDR));
}
#endif//GDIOBJ_LockObj

#ifdef GDIOBJ_UnlockObj
#undef GDIOBJ_UnlockObj
BOOL FASTCALL
GDIOBJ_UnlockObjDbg (const char* file, int line, HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      DPRINT1("GDIBOJ_UnlockObj failed for 0x%08x, reqtype 0x%08x\n",
		  hObj, ObjectType);
      DPRINT1("\tcalled from: %s:%i\n", file, line);
      return FALSE;
    }
  DPRINT("(%s:%i) GDIOBJ_UnlockObj(0x%08x,0x%08x)\n", file, line, hObj, ObjectType);
  ObjHdr->lockfile = NULL;
  ObjHdr->lockline = 0;

  return GDIOBJ_UnlockObj(hObj, ObjectType);
}
#endif//GDIOBJ_LockObj

/*!
 * Return pointer to the object by handle.
 *
 * \param hObj 		Object handle
 * \param ObjectType	one of the object types defined in \ref GDI object types
 * \return		Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Don't allow to lock the objects twice! Synchronization!
*/
PGDIOBJ FASTCALL
GDIOBJ_LockObj(HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  DPRINT("GDIOBJ_LockObj: hObj: 0x%08x, type: 0x%08x, objhdr: %x\n", hObj, ObjectType, ObjHdr);
  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
      DPRINT1("GDIBOJ_LockObj failed for 0x%08x, type 0x%08x\n",
		  hObj, ObjectType);
      return NULL;
    }

#if 0
  ExAcquireFastMutex(&ObjHdr->Lock);
#else
  if (ObjHdr->LockTid == (DWORD)PsGetCurrentThreadId())
    {
      InterlockedIncrement(&ObjHdr->LockCount);
    }
  else
    {
      for (;;)
        {
          if (InterlockedCompareExchange(&ObjHdr->LockTid, (DWORD)PsGetCurrentThreadId(), 0))
            {
              InterlockedIncrement(&ObjHdr->LockCount);
              break;
            }
          /* FIXME: KeDelayExecutionThread(KernelMode, FALSE, &ShortDelay); */
        }
    }
#endif

  ExAcquireFastMutex(&RefCountHandling);
  ObjHdr->dwCount++;
  ExReleaseFastMutex(&RefCountHandling);
  return (PGDIOBJ)((PCHAR)ObjHdr + sizeof(GDIOBJHDR));
}

/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param hObj 		Object handle
 * \param ObjectType	one of the object types defined in \ref GDI object types
 *
 * \note This function performs delayed cleanup. If the object is locked when GDI_FreeObj() is called
 * then \em this function frees the object when reference count is zero.
 *
 * \todo Change synchronization algorithm.
*/
#undef GDIOBJ_UnlockObj
BOOL FASTCALL
GDIOBJ_UnlockObj(HGDIOBJ hObj, DWORD ObjectType)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(hObj));

  DPRINT("GDIOBJ_UnlockObj: hObj: 0x%08x, type: 0x%08x, objhdr: %x\n", hObj, ObjectType, ObjHdr);
  if (! GDI_VALID_OBJECT(hObj, ObjHdr, ObjectType, GDIOBJFLAG_DEFAULT))
    {
    DPRINT1( "GDIOBJ_UnLockObj: failed\n");
    return FALSE;
  }

#if 0
  ExReleaseFastMutex(&ObjHdr->Lock);
#else
  if (InterlockedDecrement(&ObjHdr->LockCount) == 0)
    {
      InterlockedExchange(&ObjHdr->LockTid, 0);
    }
#endif

  ExAcquireFastMutex(&RefCountHandling);
  if (0 == (ObjHdr->dwCount & ~0x80000000))
    {
      ExReleaseFastMutex(&RefCountHandling);
      DPRINT1( "GDIOBJ_UnLockObj: unlock object (0x%x) that is not locked\n", hObj );
      return FALSE;
    }

  ObjHdr->dwCount--;

  if (ObjHdr->dwCount == 0x80000000)
    {
      //delayed object release
      ObjHdr->dwCount = 0;
      ExReleaseFastMutex(&RefCountHandling);
      DPRINT("GDIOBJ_UnlockObj: delayed delete\n");
      return GDIOBJ_FreeObj(hObj, ObjectType, GDIOBJFLAG_DEFAULT);
    }
  ExReleaseFastMutex(&RefCountHandling);

  return TRUE;
}

BOOL FASTCALL
GDIOBJ_OwnedByCurrentProcess(HGDIOBJ ObjectHandle)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));

  DPRINT("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);
  ASSERT(GDI_VALID_OBJECT(ObjectHandle, ObjHdr, GDI_OBJECT_TYPE_DONTCARE, GDIOBJFLAG_IGNOREPID));

  return ObjHdr->hProcessId == PsGetCurrentProcessId();
}

void FASTCALL
GDIOBJ_SetOwnership(HGDIOBJ ObjectHandle, PEPROCESS NewOwner)
{
  PGDIOBJHDR ObjHdr = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(ObjectHandle));
  PEPROCESS OldProcess;
  PW32PROCESS W32Process;
  NTSTATUS Status;

  DPRINT("GDIOBJ_OwnedByCurrentProcess: ObjectHandle: 0x%08x\n", ObjectHandle);
  ASSERT(GDI_VALID_OBJECT(ObjectHandle, ObjHdr, GDI_OBJECT_TYPE_DONTCARE, GDIOBJFLAG_IGNOREPID));

  if ((NULL == NewOwner && GDI_GLOBAL_PROCESS != ObjHdr->hProcessId)
      || (NULL != NewOwner && ObjHdr->hProcessId != (HANDLE) NewOwner->UniqueProcessId))
    {
      Status = PsLookupProcessByProcessId((PVOID)ObjHdr->hProcessId, &OldProcess);
      if (NT_SUCCESS(Status))
        {
          W32Process = OldProcess->Win32Process;
          if (W32Process)
            {
              W32Process->GDIObjects--;
            }
          ObDereferenceObject(OldProcess);
        }
    }

  if (NULL == NewOwner)
    {
      ObjHdr->hProcessId = GDI_GLOBAL_PROCESS;
    }
  else if (ObjHdr->hProcessId != (HANDLE) NewOwner->UniqueProcessId)
    {
      ObjHdr->hProcessId = (HANDLE) NewOwner->UniqueProcessId;
      W32Process = NewOwner->Win32Process;
      if (W32Process)
        {
          W32Process->GDIObjects++;
        }
    }
}

void FASTCALL
GDIOBJ_CopyOwnership(HGDIOBJ CopyFrom, HGDIOBJ CopyTo)
{
  PGDIOBJHDR ObjHdrFrom = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(CopyFrom));
  PGDIOBJHDR ObjHdrTo = GDIOBJ_iGetObjectForIndex(GDI_HANDLE_GET_INDEX(CopyTo));
  NTSTATUS Status;
  PEPROCESS ProcessFrom;
  PEPROCESS CurrentProcess;

  ASSERT(NULL != ObjHdrFrom && NULL != ObjHdrTo);
  if (NULL != ObjHdrFrom && NULL != ObjHdrTo
      && ObjHdrTo->hProcessId != ObjHdrFrom->hProcessId)
    {
      if (ObjHdrFrom->hProcessId == GDI_GLOBAL_PROCESS)
        {
          GDIOBJ_SetOwnership(CopyTo, NULL);
        }
      else
        {
          /* Warning: ugly hack ahead
           *
           * During process cleanup, we can't call PsLookupProcessByProcessId
           * for the current process, 'cause that function will try to
           * reference the process, and since the process is closing down
           * that will result in a bugcheck.
           * So, instead, we call PsGetCurrentProcess, which doesn't reference
           * the process. If the current process is indeed the one we're
           * looking for, we use it, otherwise we can (safely) call
           * PsLookupProcessByProcessId
           */
          CurrentProcess = PsGetCurrentProcess();
          if (ObjHdrFrom->hProcessId == (HANDLE) CurrentProcess->UniqueProcessId)
            {
              GDIOBJ_SetOwnership(CopyTo, CurrentProcess);
            }
          else
            {
              Status = PsLookupProcessByProcessId((PVOID) ObjHdrFrom->hProcessId, &ProcessFrom);
              if (NT_SUCCESS(Status))
                {
                  GDIOBJ_SetOwnership(CopyTo, ProcessFrom);
                  ObDereferenceObject(ProcessFrom);
                }
            }
        }
    }
}

/* EOF */
