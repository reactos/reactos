/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.16 2002/09/01 20:39:56 dwelch Exp $
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


HBITMAP hPseudoStockBitmap; /* 1x1 bitmap for memory DCs */

static PGDI_HANDLE_TABLE  HandleTable = 0;
static FAST_MUTEX  HandleTableMutex;
static FAST_MUTEX  RefCountHandling;

#define GDI_HANDLE_NUMBER  0x4000

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

static PGDI_HANDLE_ENTRY
GDIOBJ_iGetHandleEntryForIndex (WORD TableIndex)
{
  //DPRINT("GDIOBJ_iGetHandleEntryForIndex: TableIndex: %d,\n handle: %x, ptr: %x\n", TableIndex, HandleTable->Handles [TableIndex], &(HandleTable->Handles [TableIndex])  );
  //DPRINT("GIG: HandleTable: %x, Handles: %x, \n TableIndex: %x, pt: %x\n", HandleTable,  HandleTable->Handles, TableIndex, ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex));
  //DPRINT("GIG: Hndl: %x, mag: %x\n", ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex), ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex)->wMagic);
  return  ((PGDI_HANDLE_ENTRY)HandleTable->Handles+TableIndex);
}

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

/*-----------------7/12/2002 11:38AM----------------
 * Allocate memory for GDI object and return handle to it
 * Use GDIOBJ_Lock to obtain pointer to the new object.
 * --------------------------------------------------*/
HGDIOBJ GDIOBJ_AllocObj(WORD Size, WORD Magic)
{
  	PGDIOBJHDR  newObject;
  	PGDI_HANDLE_ENTRY  handleEntry;

	DPRINT("GDIOBJ_AllocObj: size: %d, magic: %x\n", Size, Magic);
  	newObject = ExAllocatePool (PagedPool, Size + sizeof (GDIOBJHDR));
  	if (newObject == NULL)
  	{
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

BOOL  GDIOBJ_FreeObj(HGDIOBJ hObj, WORD Magic)
{
  	PGDIOBJHDR  objectHeader;
  	PGDI_HANDLE_ENTRY  handleEntry;
	PGDIOBJ 	Obj;
	BOOL 	bRet = TRUE;

  	handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)hObj & 0xffff);
	DPRINT("GDIOBJ_FreeObj: hObj: %d, magic: %x, handleEntry: %x\n", hObj, Magic, handleEntry );
  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && handleEntry->wMagic != GO_MAGIC_DONTCARE )
	     || handleEntry->hProcessId != PsGetCurrentProcessId ())
  	  return  FALSE;

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);

  	// check that the reference count is zero. if not then set flag
  	// and delete object when releaseobj is called
  	ExAcquireFastMutex(&RefCountHandling);
  	if( ( objectHeader->dwCount & ~0x80000000 ) > 0 ){
  		objectHeader->dwCount |= 0x80000000;
		DPRINT("GDIOBJ_FreeObj: delayed object deletion");
  		ExReleaseFastMutex(&RefCountHandling);
		return TRUE;
  	}
  	ExReleaseFastMutex(&RefCountHandling);

	//allow object to delete internal data
	Obj = (PGDIOBJ)((PCHAR)handleEntry->pObject + sizeof(GDIOBJHDR));
	switch( handleEntry->wMagic ){
 		case GO_REGION_MAGIC:
			bRet = RGNDATA_InternalDelete( (PROSRGNDATA) Obj );
			break;
 		case GO_PEN_MAGIC:
 		case GO_PALETTE_MAGIC:
 		case GO_BITMAP_MAGIC:
			bRet = Bitmap_InternalDelete( (PBITMAPOBJ) Obj );
			break;
 		case GO_DC_MAGIC:
			bRet = DC_InternalDeleteDC( (PDC) Obj );
			break;
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
  	// (RJJ) set wMagic last to avoid race condition
  	handleEntry->wMagic = 0;


  	return  TRUE;
}

PGDIOBJ GDIOBJ_LockObj( HGDIOBJ hObj, WORD Magic )
{
  	PGDI_HANDLE_ENTRY handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) hObj & 0xffff);
  	PGDIOBJHDR  objectHeader;

	DPRINT("GDIOBJ_LockObj: hObj: %d, magic: %x, \n handleEntry: %x, mag %x\n", hObj, Magic, handleEntry, handleEntry->wMagic);
  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && handleEntry->wMagic != GO_MAGIC_DONTCARE )
	     || (handleEntry->hProcessId != (HANDLE)0xFFFFFFFF &&
		 handleEntry->hProcessId != PsGetCurrentProcessId ()))
  	  return  NULL;

	objectHeader = (PGDIOBJHDR) handleEntry->pObject;
	ASSERT(objectHeader);

	ExAcquireFastMutex(&RefCountHandling);
	objectHeader->dwCount++;
	ExReleaseFastMutex(&RefCountHandling);

	DPRINT("GDIOBJ_LockObj: PGDIOBJ %x\n",  ((PCHAR)objectHeader + sizeof(GDIOBJHDR)) );
	return (PGDIOBJ)((PCHAR)objectHeader + sizeof(GDIOBJHDR));
}

BOOL GDIOBJ_UnlockObj( HGDIOBJ hObj, WORD Magic )
{
  	PGDI_HANDLE_ENTRY handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD) hObj & 0xffff);
  	PGDIOBJHDR  objectHeader;

	DPRINT("GDIOBJ_UnlockObj: hObj: %d, magic: %x, \n handleEntry: %x\n", hObj, Magic, handleEntry);
  	if (handleEntry == 0 || (handleEntry->wMagic != Magic && handleEntry->wMagic != GO_MAGIC_DONTCARE )
	      || (handleEntry->hProcessId != (HANDLE)0xFFFFFFFF &&
		 handleEntry->hProcessId != PsGetCurrentProcessId ()))
  	  return  FALSE;

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
		return GDIOBJ_FreeObj( hObj, Magic );
	}
	ExReleaseFastMutex(&RefCountHandling);
	return TRUE;
}

/*
PGDIOBJ GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJHDR  newObject;
  PGDI_HANDLE_ENTRY  handleEntry;

  newObject = ExAllocatePool (PagedPool, Size + sizeof (GDIOBJHDR));
  if (newObject == NULL)
  {
    return  NULL;
  }
  RtlZeroMemory (newObject, Size + sizeof (GDIOBJHDR));

  newObject->wTableIndex = GDIOBJ_iGetNextOpenHandleIndex ();
  handleEntry = GDIOBJ_iGetHandleEntryForIndex (newObject->wTableIndex);
  handleEntry->wMagic = Magic;
  handleEntry->hProcessId = PsGetCurrentProcessId ();
  handleEntry->pObject = newObject;

  return  (PGDIOBJ)(((PCHAR) newObject) + sizeof (GDIOBJHDR));
}

BOOL  GDIOBJ_FreeObject (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objectHeader;
  PGDI_HANDLE_ENTRY  handleEntry;

  objectHeader = (PGDIOBJHDR)(((PCHAR)Obj) - sizeof (GDIOBJHDR));
  handleEntry = GDIOBJ_iGetHandleEntryForIndex (objectHeader->wTableIndex);
  if (handleEntry == 0 || handleEntry->wMagic != Magic)
    return  FALSE;
  handleEntry->hProcessId = 0;
  handleEntry->pObject = 0;
  // (RJJ) set wMagic last to avoid race condition
  handleEntry->wMagic = 0;
  ExFreePool (objectHeader);

  return  TRUE;
}

HGDIOBJ  GDIOBJ_PtrToHandle (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objectHeader;
  PGDI_HANDLE_ENTRY  handleEntry;

  if (Obj == NULL)
    return  NULL;
  objectHeader = (PGDIOBJHDR) (((PCHAR)Obj) - sizeof (GDIOBJHDR));
  handleEntry = GDIOBJ_iGetHandleEntryForIndex (objectHeader->wTableIndex);
  if (handleEntry == 0 ||
      handleEntry->wMagic != Magic ||
      handleEntry->hProcessId != PsGetCurrentProcessId () )
    return  NULL;

  return  (HGDIOBJ) objectHeader->wTableIndex;
}

PGDIOBJ  GDIOBJ_HandleToPtr (HGDIOBJ ObjectHandle, WORD Magic)
{
  PGDI_HANDLE_ENTRY  handleEntry;

  if (ObjectHandle == NULL)
    return NULL;

  handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)ObjectHandle & 0xffff);
  if (handleEntry == 0 ||
      (Magic != GO_MAGIC_DONTCARE && handleEntry->wMagic != Magic) ||
      handleEntry->hProcessId != PsGetCurrentProcessId () )
    return  NULL;

  return  (PGDIOBJ) (((PCHAR)handleEntry->pObject) + sizeof (GDIOBJHDR));
}
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

VOID
InitGdiObjectHandleTable (void)
{
  DPRINT ("InitGdiObjectHandleTable\n");
  ExInitializeFastMutex (&HandleTableMutex);
  ExInitializeFastMutex (&RefCountHandling);
  //per http://www.wd-mag.com/articles/1999/9902/9902b/9902b.htm?topic=articles
  //gdi handle table can hold 0x4000 handles
  HandleTable = GDIOBJ_iAllocHandleTable (GDI_HANDLE_NUMBER);
  DPRINT("HandleTable: %x\n", HandleTable );

  InitEngHandleTable();
}

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

HGDIOBJ STDCALL  W32kGetStockObject(INT  Object)
{
  HGDIOBJ ret;

/*  if ((Object < 0) || (Object >= NB_STOCK_OBJECTS)) return 0;
  if (!StockObjects[Object]) return 0;
  ret = FIRST_STOCK_HANDLE + Object;

  return ret; */

  return StockObjects[Object]; // FIXME........
}

BOOL STDCALL  W32kDeleteObject(HGDIOBJ hObject)
{
  return GDIOBJ_FreeObj( hObject, GO_MAGIC_DONTCARE );
}

// dump all the objects for process. if process == 0 dump all the objects
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
