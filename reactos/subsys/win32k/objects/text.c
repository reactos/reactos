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
/* $Id: text.c,v 1.32 2003/05/18 17:16:18 ea Exp $ */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../eng/handle.h"

#include <include/inteng.h>
#include <include/text.h>
#include <include/eng.h>

#define NDEBUG
#include <win32k/debug1.h>

FT_Library  library;

typedef struct _FONTTABLE {
  HFONT hFont;
  LPCWSTR FaceName;
} FONTTABLE, *PFONTTABLE;

FONTTABLE FontTable[256];
INT FontsLoaded = 0;

BOOL FASTCALL InitFontSupport(VOID)
{
  ULONG error;
  UINT File;
  static WCHAR *FontFiles[] =
  {
  L"\\SystemRoot\\media\\fonts\\helb____.ttf",
  L"\\SystemRoot\\media\\fonts\\timr____.ttf",
  L"\\SystemRoot\\media\\fonts\\Vera.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraBd.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraBI.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraIt.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraMoBd.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraMoBI.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraMoIt.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraMono.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraSe.ttf",
  L"\\SystemRoot\\media\\fonts\\VeraSeBd.ttf"
  };

  error = FT_Init_FreeType(&library);
  if(error)
  {
    return FALSE;
  }

  for (File = 0; File < sizeof(FontFiles) / sizeof(WCHAR *); File++)
    {
    DPRINT("Loading font %S\n", FontFiles[File]);

    W32kAddFontResource(FontFiles[File]);
    }

  DPRINT("All fonts loaded\n");

  return TRUE;
}

static NTSTATUS STDCALL
GetFontObjectsFromTextObj(PTEXTOBJ TextObj, HFONT *FontHandle, PFONTOBJ *FontObj, PFONTGDI *FontGDI)
{
  NTSTATUS Status = STATUS_SUCCESS;

  ASSERT(NULL != TextObj && NULL != TextObj->GDIFontHandle);
  if (NULL != TextObj && NULL != TextObj->GDIFontHandle)
  {
    if (NT_SUCCESS(Status) && NULL != FontHandle)
    {
      *FontHandle = TextObj->GDIFontHandle;
    }
    if (NT_SUCCESS(Status) && NULL != FontObj)
    {
      *FontObj = AccessUserObject((ULONG) TextObj->GDIFontHandle);
      if (NULL == *FontObj)
      {
	ASSERT(FALSE);
	Status = STATUS_INVALID_HANDLE;
      }
    }
    if (NT_SUCCESS(Status) && NULL != FontGDI)
    {
      *FontGDI = AccessInternalObject((ULONG) TextObj->GDIFontHandle);
      if (NULL == *FontGDI)
      {
	ASSERT(FALSE);
	Status = STATUS_INVALID_HANDLE;
      }
    }
  }
  else
  {
    Status = STATUS_INVALID_HANDLE;
  }

  return Status;
}

int
STDCALL
W32kAddFontResource(LPCWSTR  Filename)
{
  HFONT NewFont;
  PFONTOBJ FontObj;
  PFONTGDI FontGDI;
  UNICODE_STRING uFileName;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileStdInfo;
  PVOID buffer;
  ULONG size;
  INT error;
  FT_Face face;
  ANSI_STRING StringA;
  UNICODE_STRING StringU;
  IO_STATUS_BLOCK Iosb;

  NewFont = (HFONT)CreateGDIHandle(sizeof( FONTGDI ), sizeof( FONTOBJ ));
  FontObj = (PFONTOBJ) AccessUserObject( (ULONG) NewFont );
  FontGDI = (PFONTGDI) AccessInternalObject( (ULONG) NewFont );

  RtlCreateUnicodeString(&uFileName, (LPWSTR)Filename);

  //  Open the Module
  InitializeObjectAttributes(&ObjectAttributes, &uFileName, 0, NULL, NULL);

  Status = NtOpenFile(&FileHandle, FILE_ALL_ACCESS, &ObjectAttributes, &Iosb, 0, 0);

  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Could not open module file: %S\n", Filename);
    return 0;
  }

  //  Get the size of the file
  Status = NtQueryInformationFile(FileHandle, &Iosb, &FileStdInfo, sizeof(FileStdInfo), FileStandardInformation);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Could not get file size\n");
    return 0;
  }

  //  Allocate nonpageable memory for driver
  size = FileStdInfo.EndOfFile.u.LowPart;
  buffer = ExAllocatePool(NonPagedPool, size);

  if (buffer == NULL)
  {
    DPRINT1("could not allocate memory for module");
    return 0;
  }

  //  Load driver into memory chunk
  Status = NtReadFile(FileHandle, 0, 0, 0, &Iosb, buffer, FileStdInfo.EndOfFile.u.LowPart, 0, 0);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("could not read module file into memory");
    ExFreePool(buffer);
    return 0;
  }

  NtClose(FileHandle);

  error = FT_New_Memory_Face(library, buffer, size, 0, &face);
  if (error == FT_Err_Unknown_File_Format)
  {
    DPRINT1("Unknown font file format\n");
    return 0;
  }
  else if (error)
  {
    DPRINT1("Error reading font file (error code: %u)\n", error); // 48
    return 0;
  }

  // FontGDI->Filename = Filename; perform strcpy
  FontGDI->face = face;

  // FIXME: Complete text metrics
  FontGDI->TextMetric.tmAscent = face->size->metrics.ascender; // units above baseline
  FontGDI->TextMetric.tmDescent = face->size->metrics.descender; // units below baseline
  FontGDI->TextMetric.tmHeight = FontGDI->TextMetric.tmAscent + FontGDI->TextMetric.tmDescent;

  DPRINT("Font loaded: %s (%s)\n", face->family_name, face->style_name);
  DPRINT("Num glyphs: %u\n", face->num_glyphs);

  // Add this font resource to the font table
  FontTable[FontsLoaded].hFont = NewFont;

  RtlInitAnsiString(&StringA, (LPSTR)face->family_name);
  RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
  FontTable[FontsLoaded].FaceName = ExAllocatePool(NonPagedPool, (StringU.Length + 1) * 2);
  wcscpy((LPWSTR)FontTable[FontsLoaded].FaceName, StringU.Buffer);
  RtlFreeUnicodeString(&StringU);

  FontsLoaded++;

  return 1;
}

NTSTATUS FASTCALL
TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont)
{
  PTEXTOBJ TextObj;
  NTSTATUS Status = STATUS_SUCCESS;

  *NewFont = TEXTOBJ_AllocText();
  if (NULL != *NewFont)
  {
    TextObj = TEXTOBJ_LockText(*NewFont);
    if (NULL != TextObj)
    {
      memcpy(&TextObj->logfont, lf, sizeof(LOGFONTW));
      if (lf->lfEscapement != lf->lfOrientation)
      {
	/* this should really depend on whether GM_ADVANCED is set */
	TextObj->logfont.lfOrientation = TextObj->logfont.lfEscapement;
      }
      TEXTOBJ_UnlockText(*NewFont);
    }
    else
    {
      ASSERT(FALSE);
      Status = STATUS_INVALID_HANDLE;
    }
  }
  else
  {
    Status = STATUS_NO_MEMORY;
  }

  return Status;
}

HFONT
STDCALL
W32kCreateFont(int  Height,
               int  Width,
               int  Escapement,
               int  Orientation,
               int  Weight,
               DWORD  Italic,
               DWORD  Underline,
               DWORD  StrikeOut,
               DWORD  CharSet,
               DWORD  OutputPrecision,
               DWORD  ClipPrecision,
               DWORD  Quality,
               DWORD  PitchAndFamily,
               LPCWSTR  Face)
{
  LOGFONTW logfont;
  HFONT NewFont;
  NTSTATUS Status = STATUS_SUCCESS;

  logfont.lfHeight = Height;
  logfont.lfWidth = Width;
  logfont.lfEscapement = Escapement;
  logfont.lfOrientation = Orientation;
  logfont.lfWeight = Weight;
  logfont.lfItalic = Italic;
  logfont.lfUnderline = Underline;
  logfont.lfStrikeOut = StrikeOut;
  logfont.lfCharSet = CharSet;
  logfont.lfOutPrecision = OutputPrecision;
  logfont.lfClipPrecision = ClipPrecision;
  logfont.lfQuality = Quality;
  logfont.lfPitchAndFamily = PitchAndFamily;

  if (NULL != Face)
  {
    Status = MmCopyFromCaller(logfont.lfFaceName, Face, sizeof(logfont.lfFaceName));
  }
  else
  {
    logfont.lfFaceName[0] = L'\0';
  }

  if (NT_SUCCESS(Status))
    {
    Status = TextIntCreateFontIndirect(&logfont, &NewFont);
    }

  return NT_SUCCESS(Status) ? NewFont : NULL;
}

HFONT
STDCALL
W32kCreateFontIndirect(CONST LPLOGFONTW lf)
{
  LOGFONTW SafeLogfont;
  HFONT NewFont;
  NTSTATUS Status = STATUS_SUCCESS;

  if (NULL != lf)
  {
    Status = MmCopyFromCaller(&SafeLogfont, lf, sizeof(LOGFONTW));
    if (NT_SUCCESS(Status))
    {
      Status = TextIntCreateFontIndirect(&SafeLogfont, &NewFont);
    }
  }
  else
  {
    Status = STATUS_INVALID_PARAMETER;
  }

  return NT_SUCCESS(Status) ? NewFont : NULL;
}

BOOL
STDCALL
W32kCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROCW  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONTW  Logfont,
                            FONTENUMPROCW  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROCW  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kExtTextOut(HDC  hDC,
                     int  X,
                     int  Y,
                     UINT  Options,
                     CONST LPRECT  rc,
                     LPCWSTR  String,
                     UINT  Count,
                     CONST LPINT  Dx)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTS  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGetGlyphOutline(HDC  hDC,
                           UINT  Char,
                           UINT  Format,
                           LPGLYPHMETRICS  gm,
                           DWORD  Bufsize,
                           LPVOID  Buffer,
                           CONST LPMAT2 mat2)
{
  UNIMPLEMENTED;


}

DWORD
STDCALL
W32kGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRICW  otm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentExPoint(HDC  hDC,
                               LPCWSTR String,
                               int  Count,
                               int  MaxExtent,
                               LPINT  Fit,
                               LPINT  Dx,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextExtentPoint(HDC  hDC,
                             LPCWSTR  String,
                             int  Count,
                             LPSIZE  Size)
{
  PDC dc = (PDC)AccessUserObject((ULONG) hDC);
  PFONTGDI FontGDI;
  FT_Face face;
  FT_GlyphSlot glyph;
  INT error, pitch, glyph_index, i;
  ULONG TotalWidth = 0, MaxHeight = 0, CurrentChar = 0, SpaceBetweenChars = 5;

  FontGDI = (PFONTGDI)AccessInternalObject((ULONG) dc->w.hFont);

  for(i=0; i<Count; i++)
  {
    glyph_index = FT_Get_Char_Index(face, *String);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if(error) DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
    glyph = face->glyph;

    if (glyph->format == ft_glyph_format_outline)
    {
      error = FT_Render_Glyph(glyph, ft_render_mode_mono);
      if(error) DPRINT1("WARNING: Failed to render glyph!\n");
      pitch = glyph->bitmap.pitch;
    } else {
      pitch = glyph->bitmap.width;
    }

    TotalWidth += pitch-1;
    if((glyph->bitmap.rows-1) > MaxHeight) MaxHeight = glyph->bitmap.rows-1;

    CurrentChar++;

    if(CurrentChar < Size->cx) TotalWidth += SpaceBetweenChars;
    String++;
  }

  Size->cx = TotalWidth;
  Size->cy = MaxHeight;
}

BOOL
STDCALL
W32kGetTextExtentPoint32(HDC  hDC,
                               LPCWSTR  String,
                               int  Count,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int
STDCALL
W32kGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetTextMetrics(HDC hDC,
                   LPTEXTMETRICW tm)
{
  PDC dc;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  NTSTATUS Status = STATUS_SUCCESS;
  TEXTMETRICW SafeTm;
  FT_Face Face;
  ULONG Error;

  dc = DC_HandleToPtr(hDC);
  if (NULL == dc || NULL == tm)
  {
    Status = STATUS_INVALID_PARAMETER;
  }
  else
  {
    TextObj = TEXTOBJ_LockText(dc->w.hFont);
    if (NULL != TextObj)
    {
      Status = GetFontObjectsFromTextObj(TextObj, NULL, NULL, &FontGDI);
      if (NT_SUCCESS(Status))
      {
	Face = FontGDI->face;
	Error = FT_Set_Pixel_Sizes(Face, TextObj->logfont.lfHeight,
	                           TextObj->logfont.lfWidth);
	if (0 != Error)
	  {
	  DPRINT1("Error in setting pixel sizes: %u\n", Error);
	  Status = STATUS_UNSUCCESSFUL;
	  }
        else
	  {
	  memcpy(&SafeTm, &FontGDI->TextMetric, sizeof(TEXTMETRICW));
  	  SafeTm.tmAscent = (Face->size->metrics.ascender + 32) / 64; // units above baseline
	  SafeTm.tmDescent = (Face->size->metrics.descender + 32) / 64; // units below baseline
	  SafeTm.tmHeight = (Face->size->metrics.ascender +
	                     Face->size->metrics.descender + 32) / 64;
	  Status = MmCopyToCaller(tm, &SafeTm, sizeof(TEXTMETRICW));
	  }
      }
      TEXTOBJ_UnlockText(dc->w.hFont);
    }
    else
    {
      ASSERT(FALSE);
      Status = STATUS_INVALID_HANDLE;
    }
    DC_ReleasePtr(hDC);
  }

  return NT_SUCCESS(Status);
}

BOOL
STDCALL
W32kPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXT  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kRemoveFontResource(LPCWSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;

  dc = DC_HandleToPtr(hDC);
  if (!dc)
    {
      return  0;
    }
  prevAlign = dc->w.textAlign;
  dc->w.textAlign = Mode;
  DC_ReleasePtr( hDC );
  return  prevAlign;
}

COLORREF
STDCALL
W32kSetTextColor(HDC hDC,
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_HandleToPtr(hDC);

  if (!dc)
  {
    return 0x80000000;
  }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  DC_ReleasePtr( hDC );
  return  oldColor;
}

BOOL
STDCALL
W32kSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count)
{
  // Fixme: Call EngTextOut, which does the real work (calling DrvTextOut where appropriate)

  DC *dc = DC_HandleToPtr(hDC);
  SURFOBJ *SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);
  int error, glyph_index, n, i;
  FT_Face face;
  FT_GlyphSlot glyph;
  ULONG TextLeft, TextTop, pitch, previous;
  FT_Bool use_kerning;
  RECTL DestRect, MaskRect;
  POINTL SourcePoint, BrushOrigin;
  HBRUSH hBrush = NULL;
  PBRUSHOBJ Brush = NULL;
  HBITMAP HSourceGlyph;
  PSURFOBJ SourceGlyphSurf;
  SIZEL bitSize;
  FT_CharMap found = 0, charmap;
  INT yoff;
  PFONTOBJ FontObj;
  PFONTGDI FontGDI;
  PTEXTOBJ TextObj;
  PPALGDI PalDestGDI;
  PXLATEOBJ XlateObj;

  if( !dc )
	return FALSE;

  XStart += dc->w.DCOrgX;
  YStart += dc->w.DCOrgY;
  TextLeft = XStart;
  TextTop = YStart;

  TextObj = TEXTOBJ_LockText(dc->w.hFont);

  if (! NT_SUCCESS(GetFontObjectsFromTextObj(TextObj, NULL, &FontObj, &FontGDI)))
  {
    goto fail;
  }
  face = FontGDI->face;

  if (face->charmap == NULL)
  {
    DPRINT("WARNING: No charmap selected!\n");
    DPRINT("This font face has %d charmaps\n", face->num_charmaps);

    for (n = 0; n < face->num_charmaps; n++)
    {
      charmap = face->charmaps[n];
      DPRINT("found charmap encoding: %u\n", charmap->encoding);
      if (charmap->encoding != 0)
      {
        found = charmap;
        break;
      }
    }
    if (!found) DPRINT1("WARNING: Could not find desired charmap!\n");
    error = FT_Set_Charmap(face, found);
    if (error) DPRINT1("WARNING: Could not set the charmap!\n");
  }

  error = FT_Set_Pixel_Sizes(face, TextObj->logfont.lfHeight, TextObj->logfont.lfWidth);
  if(error) {
    DPRINT1("Error in setting pixel sizes: %u\n", error);
	goto fail;
  }

  // Create the brush
  PalDestGDI = (PPALGDI)AccessInternalObject((ULONG) dc->w.hPalette);
  XlateObj = (PXLATEOBJ)IntEngCreateXlate(PalDestGDI->Mode, PAL_RGB, dc->w.hPalette, NULL);
  hBrush = W32kCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.textColor));
  Brush = BRUSHOBJ_LockBrush(hBrush);
  EngDeleteXlate(XlateObj);

  SourcePoint.x = 0;
  SourcePoint.y = 0;
  MaskRect.left = 0;
  MaskRect.top = 0;
  BrushOrigin.x = 0;
  BrushOrigin.y = 0;

  // Determine the yoff from the dc's w.textAlign
  if (dc->w.textAlign & TA_BASELINE) {
    yoff = 0;
  }
  else
  if (dc->w.textAlign & TA_BOTTOM) {
    yoff = -face->size->metrics.descender / 64;
  }
  else { // TA_TOP
    yoff = face->size->metrics.ascender / 64;
  }

  use_kerning = FT_HAS_KERNING(face);
  previous = 0;

  for(i=0; i<Count; i++)
  {
    glyph_index = FT_Get_Char_Index(face, *String);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if(error) {
      DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
      goto fail;
    }
    glyph = face->glyph;

    // retrieve kerning distance and move pen position
    if (use_kerning && previous && glyph_index)
    {
      FT_Vector delta;
      FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
      TextLeft += delta.x >> 6;
    }

    if (glyph->format == ft_glyph_format_outline)
    {
      error = FT_Render_Glyph(glyph, ft_render_mode_mono);
      if(error) {
        DPRINT1("WARNING: Failed to render glyph!\n");
		goto fail;
      }
      pitch = glyph->bitmap.pitch;
    } else {
      pitch = glyph->bitmap.width;
    }

    DestRect.left = TextLeft;
    DestRect.top = TextTop + yoff - glyph->bitmap_top;
    DestRect.right = TextLeft + glyph->bitmap.width;
    DestRect.bottom = DestRect.top + glyph->bitmap.rows;
    bitSize.cx = pitch;
    bitSize.cy = glyph->bitmap.rows;
    MaskRect.right = glyph->bitmap.width;
    MaskRect.bottom = glyph->bitmap.rows;

    // We should create the bitmap out of the loop at the biggest possible glyph size
    // Then use memset with 0 to clear it and sourcerect to limit the work of the transbitblt

    HSourceGlyph = EngCreateBitmap(bitSize, pitch, BMF_1BPP, 0, glyph->bitmap.buffer);
    SourceGlyphSurf = (PSURFOBJ)AccessUserObject((ULONG) HSourceGlyph);

    // Use the font data as a mask to paint onto the DCs surface using a brush
    IntEngBitBlt(SurfObj, NULL, SourceGlyphSurf, NULL, NULL, &DestRect, &SourcePoint, &MaskRect, Brush, &BrushOrigin, 0xAACC);

    EngDeleteSurface(HSourceGlyph);

    TextLeft += glyph->advance.x >> 6;
    previous = glyph_index;

    String++;
  }
  TEXTOBJ_UnlockText( dc->w.hFont );
  BRUSHOBJ_UnlockBrush(hBrush);
  W32kDeleteObject( hBrush );
  DC_ReleasePtr( hDC );
  return TRUE;

fail:
  TEXTOBJ_UnlockText( dc->w.hFont );
  if( hBrush ){
    BRUSHOBJ_UnlockBrush(hBrush);
    W32kDeleteObject( hBrush );
  }
  DC_ReleasePtr( hDC );
  return FALSE;
}

UINT
STDCALL
W32kTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,
                               DWORD  Flags)
{
  UNIMPLEMENTED;
}

NTSTATUS FASTCALL
TextIntRealizeFont(HFONT FontHandle)
{
  UINT i;
  NTSTATUS Status = STATUS_SUCCESS;
  PTEXTOBJ TextObj;

  TextObj = TEXTOBJ_LockText(FontHandle);
  ASSERT(TextObj);
  if (NULL != TextObj)
    {
    for(i = 0; NULL == TextObj->GDIFontHandle && i < FontsLoaded; i++)
    {
      if (0 == wcscmp(FontTable[i].FaceName, TextObj->logfont.lfFaceName))
      {
	TextObj->GDIFontHandle = FontTable[i].hFont;
      }
    }

    if (NULL == TextObj->GDIFontHandle)
    {
      if (0 != FontsLoaded)
      {
	DPRINT("Requested font %S not found, using first available font\n",
	       TextObj->logfont.lfFaceName)
	TextObj->GDIFontHandle = FontTable[0].hFont;
      }
      else
      {
	DPRINT1("Requested font %S not found, no fonts loaded at all\n",
	        TextObj->logfont.lfFaceName)
	Status = STATUS_NOT_FOUND;
      }
    }

    ASSERT(! NT_SUCCESS(Status) || NULL != TextObj->GDIFontHandle);

    TEXTOBJ_UnlockText(FontHandle);
  }
  else
  {
    Status = STATUS_INVALID_HANDLE;
  }

  return Status;
}

/* EOF */
