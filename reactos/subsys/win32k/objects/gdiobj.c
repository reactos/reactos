/*
 * GDIOBJ.C - GDI object manipulation routines
 *
 * $Id: gdiobj.c,v 1.7 2001/03/31 15:35:08 jfilby Exp $
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
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" };

/* Filler to make the location counter dword aligned again.  This is necessary
   since (a) LOGFONT is packed, (b) gcc places initialised variables in the code
   segment, and (c) Solaris assembler is stupid.  */
static UINT align_OEMFixedFont = 1;

static LOGFONT AnsiFixedFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" };

static UINT align_AnsiFixedFont = 1;

static LOGFONT AnsiVarFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "MS Sans Serif" };

static UINT align_AnsiVarFont = 1;

static LOGFONT SystemFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "System" };

static UINT align_SystemFont = 1;

static LOGFONT DeviceDefaultFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" };

static UINT align_DeviceDefaultFont = 1;

static LOGFONT SystemFixedFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" };

static UINT align_SystemFixedFont = 1;

/* FIXME: Is this correct? */
static LOGFONT DefaultGuiFont =
{ 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "MS Sans Serif" };

static UINT align_DefaultGuiFont = 1;

static HGDIOBJ *StockObjects[NB_STOCK_OBJECTS]; // we dont assign these statically as WINE does because we might redesign
                                                // the way handles work, so it's more dynamic now

HBITMAP hPseudoStockBitmap; /* 1x1 bitmap for memory DCs */

PGDIOBJ  GDIOBJ_AllocObject(WORD Size, WORD Magic)
{
  PGDIOBJHDR  NewObj;

  NewObj = ExAllocatePool(PagedPool, Size + sizeof (GDIOBJHDR)); // FIXME: Allocate with tag of MAGIC?

  if (NewObj == NULL)
    {
      return  NULL;
    }

  RtlZeroMemory(NewObj, Size + sizeof (GDIOBJHDR));

  NewObj->wMagic = Magic;
#if 0
  KeInitializeSpinlock(&NewObj->Lock);
#endif

  return  (PGDIOBJ)(((PCHAR) NewObj) + sizeof (GDIOBJHDR));
}

BOOL  GDIOBJ_FreeObject (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  ObjHdr;

  ObjHdr = (PGDIOBJHDR)(((PCHAR)Obj) - sizeof (GDIOBJHDR));
  if (ObjHdr->wMagic != Magic)
  {
    return FALSE;
  }
  ExFreePool (ObjHdr);
  return TRUE;
}

HGDIOBJ  GDIOBJ_PtrToHandle (PGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  if (Obj == NULL) return NULL;
  objHeader = (PGDIOBJHDR) (((PCHAR)Obj) - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
  {
    return  0;
  }
  
  return  (HGDIOBJ) objHeader;
}

PGDIOBJ  GDIOBJ_HandleToPtr (HGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;

  if (Obj == NULL) return NULL;

  objHeader = (PGDIOBJHDR) Obj;

  /*  FIXME: Lock object for duration  */

  if ((objHeader->wMagic != Magic) && (Magic != GO_MAGIC_DONTCARE))
  {
    return  0;
  }

  return  (PGDIOBJ) (((PCHAR)Obj) + sizeof (GDIOBJHDR));
}

BOOL  GDIOBJ_LockObject (HGDIOBJ Obj)
{
  /* FIXME: write this  */
  // return  TRUE;
}

BOOL  GDIOBJ_UnlockObject (HGDIOBJ Obj)
{
  /* FIXME: write this  */
  return  TRUE;
}

HGDIOBJ  GDIOBJ_GetNextObject (HGDIOBJ Obj, WORD Magic)
{
  PGDIOBJHDR  objHeader;
  
  objHeader = (PGDIOBJHDR) ((PCHAR) Obj - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
  {
    return 0;
  }

  return objHeader->hNext;
}

HGDIOBJ  GDIOBJ_SetNextObject (HGDIOBJ Obj, WORD Magic, HGDIOBJ NextObj)
{
  PGDIOBJHDR  objHeader;
  HGDIOBJ  oldNext;
  
  /* FIXME: should we lock/unlock the object here? */
  objHeader = (PGDIOBJHDR) ((PCHAR) Obj - sizeof (GDIOBJHDR));
  if (objHeader->wMagic != Magic)
  {
    return  0;
  }
  oldNext = objHeader->hNext;
  objHeader->hNext = NextObj;
  
  return  oldNext;
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

  StockObjects[DEFAULT_PALETTE] = PALETTE_Init();
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
