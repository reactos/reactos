/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.11 2001/11/02 06:10:11 rex Exp $
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/gdiobj.h>
#include <win32k/brush.h>
#include <win32k/pen.h>
#include <win32k/text.h>

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

static PGDI_HANDLE_TABLE
GDIOBJ_iAllocHandleTable (WORD Size)
{
  PGDI_HANDLE_TABLE  handleTable;

//  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  handleTable = ExAllocatePool(PagedPool, 
                               sizeof (GDI_HANDLE_TABLE) + 
                                 sizeof (GDI_HANDLE_ENTRY) * Size);
  memset (handleTable, 
          0, 
          sizeof (GDI_HANDLE_TABLE) + sizeof (GDI_HANDLE_ENTRY) * Size);
  handleTable->wTableSize = Size;
//  ExReleaseFastMutexUnsafe (&HandleTableMutex);

  return  handleTable;
}

static PGDI_HANDLE_ENTRY
GDIOBJ_iGetHandleEntryForIndex (WORD TableIndex)
{
  return  &HandleTable->Handles [TableIndex];
}

static WORD
GDIOBJ_iGetNextOpenHandleIndex (void)
{
  WORD  tableIndex;

//  ExAcquireFastMutexUnsafe (&HandleTableMutex);
  for (tableIndex = 1; tableIndex < HandleTable->wTableSize; tableIndex++)
  {
    if (HandleTable->Handles [tableIndex].wMagic == 0)
    {
      HandleTable->Handles [tableIndex].wMagic = GO_MAGIC_DONTCARE;
      break;
    }
  }
//  ExReleaseFastMutexUnsafe (&HandleTableMutex);
  
  return  (tableIndex < HandleTable->wTableSize) ? tableIndex : 0;
}

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic)
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
  handleEntry->hProcessId = 0; // PsGetCurrentProcessId ();
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
      handleEntry->hProcessId != 0 /* PsGetCurrentProcess () */)
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
      handleEntry->hProcessId != 0 /* PsGetCurrentProcess () */)
    return  NULL;

  return  (PGDIOBJ) (((PCHAR)handleEntry->pObject) + sizeof (GDIOBJHDR));
}

WORD  GDIOBJ_GetHandleMagic (HGDIOBJ ObjectHandle)
{
  PGDI_HANDLE_ENTRY  handleEntry;

  if (ObjectHandle == NULL)
    return  0;

  handleEntry = GDIOBJ_iGetHandleEntryForIndex ((WORD)ObjectHandle & 0xffff);
  if (handleEntry == 0 || 
      handleEntry->hProcessId != 0 /* PsGetCurrentProcess () */)
    return  0;

  return  handleEntry->wMagic;
}

VOID
InitGdiObjectHandleTable (void)
{
  DbgPrint ("InitGdiObjectHandleTable\n");
//  ExInitializeFastMutex (&HandleTableMutex);
  HandleTable = GDIOBJ_iAllocHandleTable (0x1000);
}

VOID CreateStockObjects(void)
{
  // Create GDI Stock Objects from the logical structures we've defined

  StockObjects[WHITE_BRUSH] =  W32kCreateBrushIndirect(&WhiteBrush);
  StockObjects[LTGRAY_BRUSH] = W32kCreateBrushIndirect(&LtGrayBrush);
  StockObjects[GRAY_BRUSH] =   W32kCreateBrushIndirect(&GrayBrush);
  StockObjects[DKGRAY_BRUSH] = W32kCreateBrushIndirect(&DkGrayBrush);
  StockObjects[BLACK_BRUSH] =  W32kCreateBrushIndirect(&BlackBrush);
  StockObjects[NULL_BRUSH] =   W32kCreateBrushIndirect(&NullBrush);

  StockObjects[WHITE_PEN] = W32kCreatePenIndirect(&WhitePen);
  StockObjects[BLACK_PEN] = W32kCreatePenIndirect(&BlackPen);
  StockObjects[NULL_PEN] =  W32kCreatePenIndirect(&NullPen);

  StockObjects[OEM_FIXED_FONT] =      W32kCreateFontIndirect(&OEMFixedFont);
  StockObjects[ANSI_FIXED_FONT] =     W32kCreateFontIndirect(&AnsiFixedFont);
  StockObjects[SYSTEM_FONT] =         W32kCreateFontIndirect(&SystemFont);
  StockObjects[DEVICE_DEFAULT_FONT] = W32kCreateFontIndirect(&DeviceDefaultFont);
  StockObjects[SYSTEM_FIXED_FONT] =   W32kCreateFontIndirect(&SystemFixedFont);
  StockObjects[DEFAULT_GUI_FONT] =    W32kCreateFontIndirect(&DefaultGuiFont);

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
