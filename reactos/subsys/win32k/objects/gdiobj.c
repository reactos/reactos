/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.20 2003/01/18 20:50:43 ei Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/gdiobj.h>
#include <win32k/brush.h>
#include <win32k/pen.h>
#include <win32k/text.h>
#include <win32k/dc.h>
#include <win32k/bitmaps.h>
#include <win32k/region.h>
#define NDEBUG
#include <win32k/debug1.h>

//  GDI stock objects

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

static LOGFONT OEMFixedFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

/* Filler to make the location counter dword aligned again.  This is necessary
   since (a) LOGFONT is packed, (b) gcc places initialised variables in the code
   segment, and (c) Solaris assembler is stupid.  */
static UINT align_OEMFixedFont = 1;

static LOGFONT AnsiFixedFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

static UINT align_AnsiFixedFont = 1;

static LOGFONT AnsiVarFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" };

static UINT align_AnsiVarFont = 1;

static LOGFONT SystemFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"System" };

static UINT align_SystemFont = 1;

static LOGFONT DeviceDefaultFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"" };

static UINT align_DeviceDefaultFont = 1;

static LOGFONT SystemFixedFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"" };

static UINT align_SystemFixedFont = 1;

/* FIXME: Is this correct? */
static LOGFONT DefaultGuiFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"MS Sans Serif" };

static UINT align_DefaultGuiFont = 1;

static HGDIOBJ *StockObjects[NB_STOCK_OBJECTS]; // we dont assign these statically as WINE does because we might redesign
                                                // the way handles work, so it's more dynamic now


HBITMAP hPseudoStockBitmap; /*! 1x1 bitmap for memory DCs */

static PGDI_HANDLE_TABLE  HandleTable = 0;
static FAST_MUTEX  HandleTableMutex;
static FAST_MUTEX  RefCountHandling;

/*! Size of the GDI handle table
 * per http://www.wd-mag.com/articles/1999/9902/9902b/9902b.htm?topic=articles
 * gdi handle table can hold 0x4000 handles
*/
#define GDI_HANDLE_NUMBER  0x4000

/*!
 * Allocate GDI object table.
 * \param	Size - number of entries in the object table.
*/
static PGDI_HANDLE_TABLE
GDIOBJ_iAllocHandleTable (WORD Size)
{
  PGDI_HANDLE_TABLE  handleTable;

  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  handleTable = ExAllocatePool(PagedPool,
                               sizeof (GDI_HANDLE_TABLE) +
                                 sizeof (GDI_HANDLE_ENTRY) * Size);
  ASSERT( handleTable );
  memset (handleTable,
          0,
          sizeof (GDI_HANDLE_TABLE) + sizeof (GDI_HANDLE_ENTRY) * Size);
  handleTable->wTableSize = Size;
  ExReleaseFastMutexUnsafe (&HandleTableMutex);

  return  handleTable;
}

/*!
 * Returns the entry into the handle table by index.
*/
static PGDI_HANDLE_ENTRY
GDIOBJ_iGetHandleEntryForIndex (WORD TableIndex)
{
  //DPRINT("GDIOBJ_iGetHandleEntryForIndex: TableIndex: %d,\n handle: %x, ptr: %x\n", TableIndex, HandleTable->Handles [TableIndex], &(HandleTable->Handles [TableIndex])  );
  //DPRINT("GIG: HandleTable: %x, Handles: %x, \n TableIndex: %x, pt: %x\n", HandleTable,  HandleTable->Handles, TableIndex, ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex));
  //DPRINT("GIG: Hndl: %x, mag: %x\n", ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex), ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex)->wMagic);
  return  ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex);
}

/*!
 * Finds next free entry in the GDI handle table.
 * \return	index into the table is successful, zero otherwise.
*/
static WORD
GDIOBJ_iGetNextOpenHandleIndex (void)
{
  WORD  tableIndex;

  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  for (tableIndex = 1; tableIndex < HandleTable->wTableSize; tableIndex++)
  {
    if (HandleTable->Handles [tableIndex].wMagic == 0)
    {
      HandleTable->Handles [tableIndex].wMagic = GO_MAGIC_DONTCARE;
      break;
    }
  }
  ExReleaseFastMutexUnsafe (&HandleTableMutex);

  return  (tableIndex < HandleTable->wTableSize) ? tableIndex : 0;
}

/*!
 * Allocate memory for GDI object and return handle to it.
 *
 * \param Size - size of the GDI object. This shouldn't to include the size of GDIOBJHDR.
 * The actual amount of allocated memory is sizeof(GDIOBJHDR)+Size
 * \param Magic - object magic (see GDI Magic)
 *
 * \return Handle of the allocated object.
 *
 * \note Use GDIOBJ_Lock() to obtain pointer to the new object.
*/
HGDIOBJ GDIOBJ_AllocObj(WORD Size, WORD Magic)
{
  	PGDIOBJHDR  newObject;
  	PGDI_HANDLE_ENTRY  handleEntry;

	DPRINT("GDIOBJ_AllocObj: size: %d, magic: %x\n", Size, Magic);
  	newObject = ExAllocatePool (PagedPool, Size + sizeof (GDIOBJHDR));
  	if (newObject == NULL)
  	{
	  DPRINT("GDIOBJ_AllocObj: failed\n");
  	  return  NULL;
  	}
  	RtlZeroMemory (newObject, Size + sizeof (GDIOBJHDR));

  	newObject->wTableIndex = GDIOBJ_iGetNextOpenHandleIndex ();
	newObject->dwCount = 0;
  	handleEntry = GDIOBJ_iGetHandleEntryForIndex (newObject->wTableIndex);
  	handleEntry->wMagic = Magic;
  	handleEntry->hProcessId = PsGetCurrentProcessId ();
  	handleEntry->pObject = newObject;
	DPRINT("GDIOBJ_AllocObj: object handle %d\n", newObject->wTableIndex );
  	return  (HGDIOBJ) newObject->wTableIndex;
}

/*!
 * Free memory allocated for the GDI object. For each object type this function calls the
 * appropriate cleanup routine.
 *
 * \param hObj - handle of the object to be deleted.
 * \param Magic - object magic or GO_MAGIC_DONTCARE.
 * \param Flag - if set to GDIOBJFLAG_IGNOREPID then the routine doesn't check if the process that
 * tries to delete the object is the same one that created it.
 *
 * \return Returns TRUE if succesful.
 *
 * \note You should only use GDIOBJFLAG_IGNOREPID if you are cleaning up after the process that terminated.
 * \note This function deferres object deletion if it is still in use.
*/
BOOL  GDIOBJ_FreeObj(HGDIOBJ hObj, WORD Magic, DWORD Flag)
{
  	PGDIOBJHDR  objectHeader;
  	PGDI_HANDLE_ENTRY  handleEntry;
	PGDIOBJ 	Obj;
	BOOL 	bRet = TRUE;

  	handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)hObj & 0xffff);
	DPRINT("GDIOBJ_FreeObj: hObj: %d, magic: %x, handleEntry: %x\n", (WORD)hObj & 0xffff, Magic, handleEntry );

  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && Magic != GO_MAGIC_DONTCARE )
	     || ((handleEntry->hProcessId != PsGetCurrentProcessId()) && !(Flag & GDIOBJFLAG_IGNOREPID))){

	  DPRINT("Can't Delete hObj: %d, magic: %x, pid:%d\n currpid:%d, flag:%d, hmm:%d\n",(WORD)hObj & 0xffff, handleEntry->wMagic, handleEntry->hProcessId, PsGetCurrentProcessId(), (Flag&GDIOBJFLAG_IGNOREPID), ((handleEntry->hProcessId != PsGetCurrentProcessId()) && !(Flag&GDIOBJFLAG_IGNOREPID)) );
  	  return  FALSE;
	}

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);
	DPRINT("FreeObj: locks: %x\n", objectHeader->dwCount );
	if( !(Flag & GDIOBJFLAG_IGNORELOCK) ){
  		// check that the reference count is zero. if not then set flag
  		// and delete object when releaseobj is called
  		ExAcquireFastMutex(&RefCountHandling);
  		if( ( objectHeader->dwCount & ~0x80000000 ) > 0 ){
			DPRINT("GDIOBJ_FreeObj: delayed object deletion: count %d\n", objectHeader->dwCount);
  			objectHeader->dwCount |= 0x80000000;
  			ExReleaseFastMutex(&RefCountHandling);
			return TRUE;
  		}
  		ExReleaseFastMutex(&RefCountHandling);
	}

	//allow object to delete internal data
	Obj = (PGDIOBJ)((PCHAR)handleEntry->pObject + sizeof(GDIOBJHDR));
	switch( handleEntry->wMagic ){
 		case GO_REGION_MAGIC:
			bRet = RGNDATA_InternalDelete( (PROSRGNDATA) Obj );
			break;
 		case GO_BITMAP_MAGIC:
			bRet = Bitmap_InternalDelete( (PBITMAPOBJ) Obj );
			break;
 		case GO_DC_MAGIC:
			bRet = DC_InternalDeleteDC( (PDC) Obj );
			break;
 		case GO_PEN_MAGIC:
 		case GO_PALETTE_MAGIC:
 		case GO_DISABLED_DC_MAGIC:
 		case GO_META_DC_MAGIC:
 		case GO_METAFILE_MAGIC:
 		case GO_METAFILE_DC_MAGIC:
 		case GO_ENHMETAFILE_MAGIC:
 		case GO_ENHMETAFILE_DC_MAGIC:

 		case GO_BRUSH_MAGIC:
 		case GO_FONT_MAGIC:
			break;
	}
  	handleEntry->hProcessId = 0;
	ExFreePool (handleEntry->pObject);
	handleEntry->pObject = 0;
  	handleEntry->wMagic = 0;

  	return  TRUE;
}

/*!
 * Return pointer to the object by handle.
 *
 * \param hObj 		Object handle
 * \param Magic		one of the magic numbers defined in \ref GDI Magic
 * \return			Pointer to the object.
 *
 * \note Process can only get pointer to the objects it created or global objects.
 *
 * \todo Don't allow to lock the objects twice! Synchronization!
*/
PGDIOBJ GDIOBJ_LockObj( HGDIOBJ hObj, WORD Magic )
{
  	PGDI_HANDLE_ENTRY handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) hObj & 0xffff);
  	PGDIOBJHDR  objectHeader;

	DPRINT("GDIOBJ_LockObj: hObj: %d, magic: %x, \n handleEntry: %x, mag %x\n", hObj, Magic, handleEntry, handleEntry->wMagic);
  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && Magic != GO_MAGIC_DONTCARE )
	     || (handleEntry->hProcessId != (HANDLE)0xFFFFFFFF &&
		 handleEntry->hProcessId != PsGetCurrentProcessId ())){
	  DPRINT("GDIBOJ_LockObj failed for %d, magic: %d, reqMagic\n",(WORD) hObj & 0xffff, handleEntry->wMagic, Magic);
  	  return  NULL;
	}

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);
	if( objectHeader->dwCount > 0 ){
		DbgPrint("Caution! GDIOBJ_LockObj trying to lock object second time\n" );
		DbgPrint("\t called from: %x\n", __builtin_return_address(0));
	}

	ExAcquireFastMutex(&RefCountHandling);
	objectHeader->dwCount++;
	ExReleaseFastMutex(&RefCountHandling);
	return (PGDIOBJ)((PCHAR)objectHeader + sizeof(GDIOBJHDR));
}

/*!
 * Lock multiple objects. Use this function when you need to lock multiple objects and some of them may be
 * duplicates. You should use this function to avoid trying to lock the same object twice!
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and Magic fields.
 * \param	nObj	number of objects to lock
 * \return	for each entry in pList this function sets pObj field to point to the object.
 *
 * \note this function uses an O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL GDIOBJ_LockMultipleObj( PGDIMULTILOCK pList, INT nObj )
{
	INT i, j;
	ASSERT( pList );
	//go through the list checking for duplicate objects
	for( i = 0; i < nObj; i++ ){
		(pList+i)->pObj = NULL;
		for( j = 0; j < i; j++ ){
			if( ((pList+i)->hObj == (pList+j)->hObj)
			  && ((pList+i)->Magic == (pList+j)->Magic) ){
				//already locked, so just copy the pointer to the object
				(pList+i)->pObj = (pList+j)->pObj;
				break;
			}
		}
		if( (pList+i)->pObj == NULL ){
			//object hasn't been locked, so lock it.
			(pList+i)->pObj = GDIOBJ_LockObj( (pList+i)->hObj, (pList+i)->Magic );
		}
	}
	return TRUE;
}

/*!
 * Release GDI object. Every object locked by GDIOBJ_LockObj() must be unlocked. You should unlock the object
 * as soon as you don't need to have access to it's data.

 * \param hObj 		Object handle
 * \param Magic		one of the magic numbers defined in \ref GDI Magic
 *
 * \note This function performs delayed cleanup. If the object is locked when GDI_FreeObj() is called
 * then \em this function frees the object when reference count is zero.
 *
 * \todo Change synchronization algorithm.
*/
BOOL GDIOBJ_UnlockObj( HGDIOBJ hObj, WORD Magic )
{
  	PGDI_HANDLE_ENTRY handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) hObj & 0xffff);
  	PGDIOBJHDR  objectHeader;

	DPRINT("GDIOBJ_UnlockObj: hObj: %d, magic: %x, \n handleEntry: %x\n", hObj, Magic, handleEntry);
  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && Magic != GO_MAGIC_DONTCARE )
	      || (handleEntry->hProcessId != (HANDLE)0xFFFFFFFF &&
		 handleEntry->hProcessId != PsGetCurrentProcessId ())){
	  DPRINT( "GDIOBJ_UnLockObj: failed\n");
	  return  FALSE;
	}

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);

  	ExAcquireFastMutex(&RefCountHandling);
	if( ( objectHeader->dwCount & ~0x80000000 ) == 0 ){
		ExReleaseFastMutex(&RefCountHandling);
		DPRINT( "GDIOBJ_UnLockObj: unlock object that is not locked\n" );
		return FALSE;
	}

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);
	objectHeader->dwCount--;

	if( objectHeader->dwCount  == 0x80000000 ){
		//delayed object release
		objectHeader->dwCount = 0;
		ExReleaseFastMutex(&RefCountHandling);
		DPRINT("GDIOBJ_UnlockObj: delayed delete\n");
		return GDIOBJ_FreeObj( hObj, Magic, GDIOBJFLAG_DEFAULT );
	}
	ExReleaseFastMutex(&RefCountHandling);
	return TRUE;
}


/*!
 * Unlock multiple objects. Use this function when you need to unlock multiple objects and some of them may be
 * duplicates.
 *
 * \param	pList 	pointer to the list that contains handles to the objects. You should set hObj and Magic fields.
 * \param	nObj	number of objects to lock
 *
 * \note this function uses O(n^2) algoritm because we shouldn't need to call it with more than 3 or 4 objects.
*/
BOOL GDIOBJ_UnlockMultipleObj( PGDIMULTILOCK pList, INT nObj )
{
	INT i, j;
	ASSERT( pList );
	//go through the list checking for duplicate objects
	for( i = 0; i < nObj; i++ ){
		if( (pList+i)->pObj != NULL ){
			for( j = i+1; j < nObj; j++ ){
				if( ((pList+i)->pObj == (pList+j)->pObj) ){
					//set the pointer to zero for all duplicates
					(pList+j)->pObj = NULL;
				}
			}
			GDIOBJ_UnlockObj( (pList+i)->hObj, (pList+i)->Magic );
			(pList+i)->pObj = NULL;
		}
	}
	return TRUE;
}

/*!
 * Marks the object as global. (Creator process ID is set to 0xFFFFFFFF). Global objects may be
 * accessed by any process.
 * \param 	ObjectHandle - handle of the object to make global.
 *
 * \note	Only stock objects should be marked global.
*/
VOID GDIOBJ_MarkObjectGlobal(HGDIOBJ ObjectHandle)
{
  PGDI_HANDLE_ENTRY  handleEntry;

  if (ObjectHandle == NULL)
    return;

  handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)ObjectHandle & 0xffff);
  if (handleEntry == 0)
    return;

  handleEntry->hProcessId = (HANDLE)0xFFFFFFFF;
}

/*!
 * Get the type (magic value) of the object.
 * \param 	ObjectHandle - handle of the object.
 * \return 	GDI Magic value.
*/
WORD  GDIOBJ_GetHandleMagic (HGDIOBJ ObjectHandle)
{
  PGDI_HANDLE_ENTRY  handleEntry;

  if (ObjectHandle == NULL)
    return  0;

  handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)ObjectHandle & 0xffff);
  if (handleEntry == 0 ||
      (handleEntry->hProcessId != (HANDLE)0xFFFFFFFF &&
       handleEntry->hProcessId != PsGetCurrentProcessId ()))
    return  0;

  return  handleEntry->wMagic;
}

/*!
 * Initialization of the GDI object engine.
*/
VOID
InitGdiObjectHandleTable (void)
{
  DPRINT ("InitGdiObjectHandleTable\n");
  ExInitializeFastMutex (&HandleTableMutex);
  ExInitializeFastMutex (&RefCountHandling);

  HandleTable = GDIOBJ_iAllocHandleTable (GDI_HANDLE_NUMBER);
  DPRINT("HandleTable: %x\n", HandleTable );

  InitEngHandleTable();
}

/*!
 * Creates a bunch of stock objects: brushes, pens, fonts.
*/
VOID CreateStockObjects(void)
{
  // Create GDI Stock Objects from the logical structures we've defined

  StockObjects[WHITE_BRUSH] =  W32kCreateBrushIndirect(&WhiteBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[WHITE_BRUSH]);
  StockObjects[LTGRAY_BRUSH] = W32kCreateBrushIndirect(&LtGrayBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[LTGRAY_BRUSH]);
  StockObjects[GRAY_BRUSH] =   W32kCreateBrushIndirect(&GrayBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[GRAY_BRUSH]);
  StockObjects[DKGRAY_BRUSH] = W32kCreateBrushIndirect(&DkGrayBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[DKGRAY_BRUSH]);
  StockObjects[BLACK_BRUSH] =  W32kCreateBrushIndirect(&BlackBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[BLACK_BRUSH]);
  StockObjects[NULL_BRUSH] =   W32kCreateBrushIndirect(&NullBrush);
  GDIOBJ_MarkObjectGlobal(StockObjects[NULL_BRUSH]);

  StockObjects[WHITE_PEN] = W32kCreatePenIndirect(&WhitePen);
  GDIOBJ_MarkObjectGlobal(StockObjects[WHITE_PEN]);
  StockObjects[BLACK_PEN] = W32kCreatePenIndirect(&BlackPen);
  GDIOBJ_MarkObjectGlobal(StockObjects[BLACK_PEN]);
  StockObjects[NULL_PEN] =  W32kCreatePenIndirect(&NullPen);
  GDIOBJ_MarkObjectGlobal(StockObjects[NULL_PEN]);

  StockObjects[OEM_FIXED_FONT] =      W32kCreateFontIndirect(&OEMFixedFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[OEM_FIXED_FONT]);
  StockObjects[ANSI_FIXED_FONT] =     W32kCreateFontIndirect(&AnsiFixedFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[ANSI_FIXED_FONT]);
  StockObjects[SYSTEM_FONT] =         W32kCreateFontIndirect(&SystemFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[SYSTEM_FONT]);
  StockObjects[DEVICE_DEFAULT_FONT] =
    W32kCreateFontIndirect(&DeviceDefaultFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[DEVICE_DEFAULT_FONT]);
  StockObjects[SYSTEM_FIXED_FONT] =   W32kCreateFontIndirect(&SystemFixedFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[SYSTEM_FIXED_FONT]);
  StockObjects[DEFAULT_GUI_FONT] =    W32kCreateFontIndirect(&DefaultGuiFont);
  GDIOBJ_MarkObjectGlobal(StockObjects[DEFAULT_GUI_FONT]);

  StockObjects[DEFAULT_PALETTE] = (HGDIOBJ*)PALETTE_Init();
}

/*!
 * Return stock object.
 * \param	Object - stock object id.
 * \return	Handle to the object.
*/
HGDIOBJ STDCALL  W32kGetStockObject(INT  Object)
{
  // check when adding new objects
  if( (Object < 0) || (Object >= NB_STOCK_OBJECTS)  )
	return NULL;
  return StockObjects[Object];
}

/*!
 * Delete GDI object
 * \param	hObject object handle
 * \return	if the function fails the returned value is NULL.
*/
BOOL STDCALL  W32kDeleteObject(HGDIOBJ hObject)
{
  return GDIOBJ_FreeObj( hObject, GO_MAGIC_DONTCARE, GDIOBJFLAG_DEFAULT );
}

/*!
 * Internal function. Called when the process is destroyed to free the remaining GDI handles.
 * \param	Process - PID of the process that was destroyed.
*/
BOOL STDCALL W32kCleanupForProcess( INT Process )
{
	DWORD i;
  	PGDI_HANDLE_ENTRY handleEntry;
  	PGDIOBJHDR  objectHeader;

	for( i=1; i < GDI_HANDLE_NUMBER; i++ ){
		handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) i & 0xffff);
		if( handleEntry && handleEntry->wMagic != 0 && handleEntry->hProcessId == Process){
			objectHeader = (PGDIOBJHDR) handleEntry->pObject;
			DPRINT("\nW32kCleanup: %d, magic: %x \n process: %d, locks: %d", i, handleEntry->wMagic, handleEntry->hProcessId, objectHeader->dwCount);
			GDIOBJ_FreeObj( (WORD) i & 0xffff, GO_MAGIC_DONTCARE, GDIOBJFLAG_IGNOREPID|GDIOBJFLAG_IGNORELOCK );
		}
	}
	return TRUE;
}

/*!
 *	Internal function. Dumps all the objects for the given process.
 * \param	If process == 0 dump all the objects.
 *
*/
VOID STDCALL W32kDumpGdiObjects( INT Process )
{
	DWORD i;
  	PGDI_HANDLE_ENTRY handleEntry;
  	PGDIOBJHDR  objectHeader;

	for( i=1; i < GDI_HANDLE_NUMBER; i++ ){
		handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) i & 0xffff);
		if( handleEntry && handleEntry->wMagic != 0 ){
			objectHeader = (PGDIOBJHDR) handleEntry->pObject;
			DPRINT("\nHandle: %d, magic: %x \n process: %d, locks: %d", i, handleEntry->wMagic, handleEntry->hProcessId, objectHeader->dwCount);
		}
	}

}
