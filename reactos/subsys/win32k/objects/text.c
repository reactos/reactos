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
/* $Id: text.c,v 1.56 2003/12/12 20:51:42 gvg Exp $ */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>
#include <include/error.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/tttables.h>

#include "../eng/handle.h"

#include <include/inteng.h>
#include <include/text.h>
#include <include/eng.h>
#include <include/palette.h>

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
  L"\\SystemRoot\\media\\fonts\\Vera.ttf",
  L"\\SystemRoot\\media\\fonts\\helb____.ttf",
  L"\\SystemRoot\\media\\fonts\\timr____.ttf",
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

    NtGdiAddFontResource(FontFiles[File]);
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
NtGdiAddFontResource(LPCWSTR  Filename)
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

  Status = ZwOpenFile(&FileHandle, 
                      GENERIC_READ|SYNCHRONIZE, 
                      &ObjectAttributes, 
                      &Iosb, 
                      0, //ShareAccess
                      FILE_SYNCHRONOUS_IO_NONALERT);

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
  Status = ZwReadFile(FileHandle, 
                      NULL, 
                      NULL, 
                      NULL, 
                      &Iosb, 
                      buffer, 
                      FileStdInfo.EndOfFile.u.LowPart, 
                      NULL, 
                      NULL);
                      
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("could not read module file into memory");
    ExFreePool(buffer);
    return 0;
  }

  ZwClose(FileHandle);

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
  FontGDI->TextMetric.tmAscent = (face->size->metrics.ascender + 32) / 64; // units above baseline
  FontGDI->TextMetric.tmDescent = (- face->size->metrics.descender + 32) / 64; // units below baseline
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
/* FIXME */
/*      ASSERT(FALSE);*/
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
NtGdiCreateFont(int  Height,
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
    int Size = sizeof(logfont.lfFaceName) / sizeof(WCHAR);
    wcsncpy((wchar_t *)logfont.lfFaceName, Face, Size - 1);
    /* Be 101% sure to have '\0' at end of string */
    logfont.lfFaceName[Size - 1] = '\0';
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
NtGdiCreateFontIndirect(CONST LPLOGFONTW lf)
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
NtGdiCreateScalableFontResource(DWORD  Hidden,
                                     LPCWSTR  FontRes,
                                     LPCWSTR  FontFile,
                                     LPCWSTR  CurrentPath)
{
  UNIMPLEMENTED;
}

int
STDCALL
NtGdiEnumFontFamilies(HDC  hDC,
                          LPCWSTR  Family,
                          FONTENUMPROCW  EnumFontFamProc,
                          LPARAM  lParam)
{
  UNIMPLEMENTED;
}

int
STDCALL
NtGdiEnumFontFamiliesEx(HDC  hDC,
                            LPLOGFONTW  Logfont,
                            FONTENUMEXPROCW  EnumFontFamExProc,
                            LPARAM  lParam,
                            DWORD  Flags)
{
  UNIMPLEMENTED;
}

int
STDCALL
NtGdiEnumFonts(HDC  hDC,
                   LPCWSTR FaceName,
                   FONTENUMPROCW  FontFunc,
                   LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiExtTextOut(HDC  hdc,
                     int  X,
                     int  Y,
                     UINT  fuOptions,
                     CONST RECT  *lprc,
                     LPCWSTR  lpString,
                     UINT  cbCount,
                     CONST INT  *lpDx)
{
  /* FIXME: Implement */
  return NtGdiTextOut(hdc, X, Y, lpString, cbCount);
}

BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetCharABCWidths(HDC  hDC,
                           UINT  FirstChar,
                           UINT  LastChar,
                           LPABC  abc)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetCharABCWidthsFloat(HDC  hDC,
                                UINT  FirstChar,
                                UINT  LastChar,
                                LPABCFLOAT  abcF)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiGetCharacterPlacement(HDC  hDC,
                                 LPCWSTR  String,
                                 int  Count,
                                 int  MaxExtent,
                                 LPGCP_RESULTSW  Results,
                                 DWORD  Flags)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetCharWidth(HDC  hDC,
                       UINT  FirstChar,
                       UINT  LastChar,
                       LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetCharWidthFloat(HDC  hDC,
                            UINT  FirstChar,
                            UINT  LastChar,
                            PFLOAT  Buffer)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiGetGlyphOutline(HDC  hDC,
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
NtGdiGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRICW  otm)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetRasterizerCaps(LPRASTERIZER_STATUS  rs,
                            UINT  Size)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetTextCharsetInfo(HDC  hDC,
                             LPFONTSIGNATURE  Sig,
                             DWORD  Flags)
{
  UNIMPLEMENTED;
}

static BOOL
FASTCALL
TextIntGetTextExtentPoint(PTEXTOBJ TextObj,
                          LPCWSTR String,
                          int Count,
                          int MaxExtent,
                          LPINT Fit,
                          LPINT Dx,
                          LPSIZE Size)
{
  PFONTGDI FontGDI;
  FT_Face face;
  FT_GlyphSlot glyph;
  INT error, n, glyph_index, i, previous;
  LONG TotalWidth = 0, MaxHeight = 0;
  FT_CharMap charmap, found = NULL;
  BOOL use_kerning;

  GetFontObjectsFromTextObj(TextObj, NULL, NULL, &FontGDI);
  face = FontGDI->face;
  if (NULL != Fit)
    {
      *Fit = 0;
    }

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

      if (! found)
	{
	  DPRINT1("WARNING: Could not find desired charmap!\n");
	}

      error = FT_Set_Charmap(face, found);
      if (error)
	{
	  DPRINT1("WARNING: Could not set the charmap!\n");
	}
    }

  error = FT_Set_Pixel_Sizes(face,
                             /* FIXME should set character height if neg */
                             (TextObj->logfont.lfHeight < 0 ?
                              - TextObj->logfont.lfHeight :
                              TextObj->logfont.lfHeight),
                             TextObj->logfont.lfWidth);
  if (error)
    {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
    }

  use_kerning = FT_HAS_KERNING(face);
  previous = 0;

  for (i = 0; i < Count; i++)
    {
      glyph_index = FT_Get_Char_Index(face, *String);
      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      if (error)
	{
	  DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
	}
      glyph = face->glyph;

      /* retrieve kerning distance */
      if (use_kerning && previous && glyph_index)
	{
	  FT_Vector delta;
	  FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
	  TotalWidth += delta.x >> 6;
	}

      TotalWidth += glyph->advance.x >> 6;
      if (glyph->format == ft_glyph_format_outline)
	{
	  error = FT_Render_Glyph(glyph, ft_render_mode_mono);
	  if (error)
	    {
	      DPRINT1("WARNING: Failed to render glyph!\n");
	    }

	  if (0 != glyph->bitmap.rows && MaxHeight < (glyph->bitmap.rows - 1))
	    {
	      MaxHeight = glyph->bitmap.rows - 1;
	    }
    	}

      if (TotalWidth <= MaxExtent && NULL != Fit)
	{
	  *Fit = i + 1;
	}
      if (NULL != Dx)
	{
	  Dx[i] = TotalWidth;
	}

      previous = glyph_index;
      String++;
    }

  Size->cx = TotalWidth;
  Size->cy = MaxHeight;

  return TRUE;
}

BOOL
STDCALL
NtGdiGetTextExtentExPoint(HDC hDC,
                         LPCWSTR UnsafeString,
                         int Count,
                         int MaxExtent,
                         LPINT UnsafeFit,
                         LPINT UnsafeDx,
                         LPSIZE UnsafeSize)
{
  PDC dc;
  LPWSTR String;
  SIZE Size;
  NTSTATUS Status;
  BOOLEAN Result;
  INT Fit;
  LPINT Dx;
  PTEXTOBJ TextObj;

  if (Count < 0)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  if (0 == Count)
    {
      Size.cx = 0;
      Size.cy = 0;
      Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  return FALSE;
	}
      return TRUE;
    }

  String = ExAllocatePool(PagedPool, Count * sizeof(WCHAR));
  if (NULL == String)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (NULL != UnsafeDx)
    {
      Dx = ExAllocatePool(PagedPool, Count * sizeof(INT));
      if (NULL == Dx)
	{
	  ExFreePool(String);
	  SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
	  return FALSE;
	}
    }
  else
    {
      Dx = NULL;
    }

  Status = MmCopyFromCaller(String, UnsafeString, Count * sizeof(WCHAR));
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Dx)
	{
	  ExFreePool(Dx);
	}
      ExFreePool(String);
      SetLastNtError(Status);
      return FALSE;
    }

  dc = DC_LockDc(hDC);
  if (NULL == dc)
    {
      if (NULL != Dx)
	{
	  ExFreePool(Dx);
	}
      ExFreePool(String);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  TextObj = TEXTOBJ_LockText(dc->w.hFont);
  DC_UnlockDc(hDC);
  Result = TextIntGetTextExtentPoint(TextObj, String, Count, MaxExtent,
                                     NULL == UnsafeFit ? NULL : &Fit, Dx, &Size);
  TEXTOBJ_UnlockText(dc->w.hFont);

  ExFreePool(String);
  if (! Result)
    {
      if (NULL != Dx)
	{
	  ExFreePool(Dx);
	}
      return FALSE;
    }

  if (NULL != UnsafeFit)
    {
      Status = MmCopyToCaller(UnsafeFit, &Fit, sizeof(INT));
      if (! NT_SUCCESS(Status))
	{
	  if (NULL != Dx)
	    {
	      ExFreePool(Dx);
	    }
	  SetLastNtError(Status);
	  return FALSE;
	}
    }

  if (NULL != UnsafeDx)
    {
      Status = MmCopyToCaller(UnsafeDx, Dx, Count * sizeof(INT));
      if (! NT_SUCCESS(Status))
	{
	  if (NULL != Dx)
	    {
	      ExFreePool(Dx);
	    }
	  SetLastNtError(Status);
	  return FALSE;
	}
    }
  if (NULL != Dx)
    {
      ExFreePool(Dx);
    }

  Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

BOOL
STDCALL
NtGdiGetTextExtentPoint(HDC hDC,
                       LPCWSTR String,
                       int Count,
                       LPSIZE Size)
{
  return NtGdiGetTextExtentExPoint(hDC, String, Count, 0, NULL, NULL, Size);
}

BOOL
STDCALL
NtGdiGetTextExtentPoint32(HDC hDC,
                         LPCWSTR UnsafeString,
                         int Count,
                         LPSIZE UnsafeSize)
{
  PDC dc;
  LPWSTR String;
  SIZE Size;
  NTSTATUS Status;
  BOOLEAN Result;
  PTEXTOBJ TextObj;

  if (Count < 0)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  if (0 == Count)
    {
      Size.cx = 0;
      Size.cy = 0;
      Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  return FALSE;
	}
      return TRUE;
    }

  String = ExAllocatePool(PagedPool, Count * sizeof(WCHAR));
  if (NULL == String)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  Status = MmCopyFromCaller(String, UnsafeString, Count * sizeof(WCHAR));
  if (! NT_SUCCESS(Status))
    {
      ExFreePool(String);
      SetLastNtError(Status);
      return FALSE;
    }

  dc = DC_LockDc(hDC);
  if (NULL == dc)
    {
      ExFreePool(String);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  TextObj = TEXTOBJ_LockText(dc->w.hFont);
  DC_UnlockDc(hDC);
  Result = TextIntGetTextExtentPoint (
	  TextObj, String, Count, 0, NULL, NULL, &Size);
  dc = DC_LockDc(hDC);
  ASSERT(dc); // it succeeded earlier, it should now, too
  TEXTOBJ_UnlockText(dc->w.hFont);
  DC_UnlockDc(hDC);

  ExFreePool(String);
  if (! Result)
    {
      return FALSE;
    }

  Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

int
STDCALL
NtGdiGetTextFace(HDC  hDC,
                     int  Count,
                     LPWSTR  FaceName)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGetTextMetrics(HDC hDC,
                   LPTEXTMETRICW tm)
{
  PDC dc;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  NTSTATUS Status = STATUS_SUCCESS;
  TEXTMETRICW SafeTm;
  FT_Face Face;
  TT_OS2 *pOS2;
  ULONG Error;

  dc = DC_LockDc(hDC);
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
	Error = FT_Set_Pixel_Sizes(Face,
	                           /* FIXME should set character height if neg */
	                           (TextObj->logfont.lfHeight < 0 ?
	                            - TextObj->logfont.lfHeight :
	                            TextObj->logfont.lfHeight),
	                           TextObj->logfont.lfWidth);
	if (0 != Error)
	  {
	  DPRINT1("Error in setting pixel sizes: %u\n", Error);
	  Status = STATUS_UNSUCCESSFUL;
	  }
        else
	  {
	  memcpy(&SafeTm, &FontGDI->TextMetric, sizeof(TEXTMETRICW));
          pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
          if (NULL == pOS2)
            {
              DPRINT1("Can't find OS/2 table - not TT font?\n");
              Status = STATUS_UNSUCCESSFUL;
            }
          else
            {
              SafeTm.tmAveCharWidth = (pOS2->xAvgCharWidth + 32) / 64;
            }
  	  SafeTm.tmAscent = (Face->size->metrics.ascender + 32) / 64; // units above baseline
	  SafeTm.tmDescent = (- Face->size->metrics.descender + 32) / 64; // units below baseline
	  SafeTm.tmHeight = SafeTm.tmAscent + SafeTm.tmDescent;
          SafeTm.tmMaxCharWidth = (Face->size->metrics.max_advance + 32) / 64;
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
    DC_UnlockDc(hDC);
  }

  return NT_SUCCESS(Status);
}

BOOL
STDCALL
NtGdiPolyTextOut(HDC  hDC,
                      CONST LPPOLYTEXTW  txt,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiRemoveFontResource(LPCWSTR  FileName)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      return  0;
    }
  prevAlign = dc->w.textAlign;
  dc->w.textAlign = Mode;
  DC_UnlockDc( hDC );
  return  prevAlign;
}

COLORREF
STDCALL
NtGdiSetTextColor(HDC hDC,
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_LockDc(hDC);

  if (!dc)
  {
    return 0x80000000;
  }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  DC_UnlockDc( hDC );
  return  oldColor;
}

BOOL
STDCALL
NtGdiSetTextJustification(HDC  hDC,
                               int  BreakExtra,
                               int  BreakCount)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiTextOut(HDC  hDC,
                  int  XStart,
                  int  YStart,
                  LPCWSTR  String,
                  int  Count)
{
  // Fixme: Call EngTextOut, which does the real work (calling DrvTextOut where appropriate)

  DC *dc;
  SURFOBJ *SurfObj;
  int error, glyph_index, n, i;
  FT_Face face;
  FT_GlyphSlot glyph;
  ULONG TextLeft, TextTop, pitch, previous, BackgroundLeft;
  FT_Bool use_kerning;
  RECTL DestRect, MaskRect;
  POINTL SourcePoint, BrushOrigin;
  HBRUSH hBrushFg = NULL;
  PBRUSHOBJ BrushFg = NULL;
  HBRUSH hBrushBg = NULL;
  PBRUSHOBJ BrushBg = NULL;
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
  ULONG Mode;

  dc = DC_LockDc(hDC);
  if( !dc )
	return FALSE;
  SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);

  XStart += dc->w.DCOrgX;
  YStart += dc->w.DCOrgY;
  TextLeft = XStart;
  TextTop = YStart;
  BackgroundLeft = XStart;

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

  error = FT_Set_Pixel_Sizes(face,
                             /* FIXME should set character height if neg */
                             (TextObj->logfont.lfHeight < 0 ?
                              - TextObj->logfont.lfHeight :
                              TextObj->logfont.lfHeight),
                             TextObj->logfont.lfWidth);
  if(error) {
    DPRINT1("Error in setting pixel sizes: %u\n", error);
	goto fail;
  }

  // Create the brushes
  PalDestGDI = PALETTE_LockPalette(dc->w.hPalette);
  Mode = PalDestGDI->Mode;
  PALETTE_UnlockPalette(dc->w.hPalette);
  XlateObj = (PXLATEOBJ)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
  hBrushFg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.textColor));
  BrushFg = BRUSHOBJ_LockBrush(hBrushFg);
  if (OPAQUE == dc->w.backgroundMode)
    {
      hBrushBg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.backgroundColor));
      if(hBrushBg)
      {
        BrushBg = BRUSHOBJ_LockBrush(hBrushBg);
      }
      else
      {
        EngDeleteXlate(XlateObj);
        goto fail;
      }
    }
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

    if (OPAQUE == dc->w.backgroundMode)
      {
	DestRect.left = BackgroundLeft;
	DestRect.right = TextLeft + (glyph->advance.x + 32) / 64;
	DestRect.top = TextTop + yoff - (face->size->metrics.ascender + 32) / 64;
	DestRect.bottom = TextTop + yoff + (- face->size->metrics.descender + 32) / 64;
	IntEngBitBlt(SurfObj,
	             NULL,
		     NULL,
	             dc->CombinedClip,
	             NULL,
	             &DestRect,
	             &SourcePoint,
	             &SourcePoint,
	             BrushBg,
	             &BrushOrigin,
		     PATCOPY);
	BackgroundLeft = DestRect.right;
      }

    DestRect.left = TextLeft;
    DestRect.right = TextLeft + glyph->bitmap.width;
    DestRect.top = TextTop + yoff - glyph->bitmap_top;
    DestRect.bottom = DestRect.top + glyph->bitmap.rows;
	
    bitSize.cx = glyph->bitmap.width;
    bitSize.cy = glyph->bitmap.rows;
    MaskRect.right = glyph->bitmap.width;
    MaskRect.bottom = glyph->bitmap.rows;

    // We should create the bitmap out of the loop at the biggest possible glyph size
    // Then use memset with 0 to clear it and sourcerect to limit the work of the transbitblt

    HSourceGlyph = EngCreateBitmap(bitSize, pitch, BMF_1BPP, 0, glyph->bitmap.buffer);
    SourceGlyphSurf = (PSURFOBJ)AccessUserObject((ULONG) HSourceGlyph);

    // Use the font data as a mask to paint onto the DCs surface using a brush
    IntEngBitBlt (
		SurfObj,
		NULL,
		SourceGlyphSurf,
		dc->CombinedClip,
		NULL,
		&DestRect,
		&SourcePoint,
		(PPOINTL)&MaskRect,
		BrushFg,
		&BrushOrigin,
		0xAACC );

    EngDeleteSurface(HSourceGlyph);

    TextLeft += (glyph->advance.x + 32) / 64;
    previous = glyph_index;

    String++;
  }
  TEXTOBJ_UnlockText(dc->w.hFont);
  if (NULL != hBrushBg)
    {
      BRUSHOBJ_UnlockBrush(hBrushBg);
      NtGdiDeleteObject(hBrushBg);
    }
  BRUSHOBJ_UnlockBrush(hBrushFg);
  NtGdiDeleteObject(hBrushFg);
  DC_UnlockDc(hDC);
  return TRUE;

fail:
  TEXTOBJ_UnlockText( dc->w.hFont );
  if (NULL != hBrushBg)
    {
      BRUSHOBJ_UnlockBrush(hBrushBg);
      NtGdiDeleteObject(hBrushBg);
    }
  if (NULL != hBrushFg)
    {
      BRUSHOBJ_UnlockBrush(hBrushFg);
      NtGdiDeleteObject(hBrushFg);
    }
  DC_UnlockDc( hDC );
  return FALSE;
}

UINT
STDCALL
NtGdiTranslateCharsetInfo(PDWORD  Src,
                               LPCHARSETINFO  CSI,
                               DWORD  Flags)
{
  UNIMPLEMENTED;
}

NTSTATUS FASTCALL
TextIntRealizeFont(HFONT FontHandle)
{
  LONG i;
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
