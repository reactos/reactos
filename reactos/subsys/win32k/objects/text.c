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
/* $Id: text.c,v 1.76 2004/02/21 21:15:22 navaraf Exp $ */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <napi/win32.h>
#include <internal/safe.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/kapi.h>
#include <include/error.h>
#include <include/desktop.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/tttables.h>

#include "../eng/handle.h"

#include <include/inteng.h>
#include <include/text.h>
#include <include/eng.h>
#include <include/palette.h>
#include <include/tags.h>

#define NDEBUG
#include <win32k/debug1.h>

FT_Library  library;

typedef struct _FONT_ENTRY {
  LIST_ENTRY ListEntry;
  HFONT hFont;
  UNICODE_STRING FaceName;
  BYTE NotEnum;
} FONT_ENTRY, *PFONT_ENTRY;

/* The FreeType library is not thread safe, so we have
   to serialize access to it */
static FAST_MUTEX FreeTypeLock;

static LIST_ENTRY FontListHead;
static FAST_MUTEX FontListLock;
static INT FontsLoaded = 0; /* number of all fonts loaded (including private fonts */
static BOOL RenderingEnabled = TRUE;

BOOL FASTCALL
IntIsFontRenderingEnabled(VOID)
{
  BOOL Ret;
  HDC hDC;
  PDC dc;
  PSURFOBJ SurfObj;
  Ret = RenderingEnabled;
  hDC = IntGetScreenDC();
  if(hDC)
  {
    dc = DC_LockDc(hDC);
    SurfObj = (PSURFOBJ)AccessUserObject((ULONG) dc->Surface);
    if(SurfObj)
      Ret = (SurfObj->iBitmapFormat >= BMF_8BPP);
    DC_UnlockDc(hDC);
  }
  return Ret;
}

VOID FASTCALL
IntEnableFontRendering(BOOL Enable)
{
  RenderingEnabled = Enable;
}

FT_Render_Mode FASTCALL
IntGetFontRenderMode(LOGFONTW *logfont)
{  
  switch(logfont->lfQuality)
  {
    //case ANTIALIASED_QUALITY:
    case DEFAULT_QUALITY:
      return FT_RENDER_MODE_NORMAL;
    case DRAFT_QUALITY:
      return FT_RENDER_MODE_LIGHT;
    //case NONANTIALIASED_QUALITY:
    case PROOF_QUALITY:
      return FT_RENDER_MODE_MONO;
    //case CLEARTYPE_QUALITY:
    //  return FT_RENDER_MODE_LCD;
  }
  return FT_RENDER_MODE_MONO;
}

int FASTCALL
IntGdiAddFontResource(PUNICODE_STRING Filename, DWORD fl)
{
  HFONT NewFont;
  PFONTOBJ FontObj;
  PFONTGDI FontGDI;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION FileStdInfo;
  PVOID buffer;
  ULONG size;
  INT error;
  FT_Face face;
  ANSI_STRING StringA;
  IO_STATUS_BLOCK Iosb;
  PFONT_ENTRY entry;

  NewFont = (HFONT)CreateGDIHandle(sizeof( FONTGDI ), sizeof( FONTOBJ ));
  FontObj = (PFONTOBJ) AccessUserObject( (ULONG) NewFont );
  FontGDI = (PFONTGDI) AccessInternalObject( (ULONG) NewFont );

  //  Open the Module
  InitializeObjectAttributes(&ObjectAttributes, Filename, 0, NULL, NULL);

  Status = ZwOpenFile(&FileHandle, 
                      GENERIC_READ|SYNCHRONIZE, 
                      &ObjectAttributes, 
                      &Iosb, 
                      0, //ShareAccess
                      FILE_SYNCHRONOUS_IO_NONALERT);

  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Could not open module file: %wZ\n", Filename);
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
  buffer = ExAllocatePoolWithTag(NonPagedPool, size, TAG_GDITEXT);

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

  ExAcquireFastMutex(&FreeTypeLock);
  error = FT_New_Memory_Face(library, buffer, size, 0, &face);
  ExReleaseFastMutex(&FreeTypeLock);
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
  
  entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(FONT_ENTRY), TAG_FONT);
  if(!entry)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
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
  entry->hFont = NewFont;
  entry->NotEnum = (fl & FR_NOT_ENUM);
  RtlInitAnsiString(&StringA, (LPSTR)face->family_name);
  RtlAnsiStringToUnicodeString(&entry->FaceName, &StringA, TRUE);
  
  if(fl & FR_PRIVATE)
  {
    PW32PROCESS Win32Process = PsGetWin32Process();
    
    ExAcquireFastMutex(&Win32Process->PrivateFontListLock);
    InsertTailList(&Win32Process->PrivateFontListHead, &entry->ListEntry);
    FontsLoaded++;
    ExReleaseFastMutex(&Win32Process->PrivateFontListLock);
  }
  else
  {
    ExAcquireFastMutex(&FontListLock);
    InsertTailList(&FontListHead, &entry->ListEntry);
    FontsLoaded++;
    ExReleaseFastMutex(&FontListLock);
  }

  return 1;
}

BOOL FASTCALL InitFontSupport(VOID)
{
	ULONG ulError;
	UNICODE_STRING cchDir, cchFilename, cchSearchPattern ;
	OBJECT_ATTRIBUTES obAttr;
	IO_STATUS_BLOCK Iosb;
	HANDLE hDirectory;
	NTSTATUS Status;
	PFILE_DIRECTORY_INFORMATION iFileData;
	PVOID   pBuff;
	BOOLEAN bRestartScan = TRUE;
	
	InitializeListHead(&FontListHead);
    ExInitializeFastMutex(&FontListLock);
    ExInitializeFastMutex(&FreeTypeLock);
	
    ulError = FT_Init_FreeType(&library);
    
    if(!ulError)
    {
        RtlInitUnicodeString(&cchDir, L"\\SystemRoot\\Media\\Fonts\\");

		RtlInitUnicodeString(&cchSearchPattern,L"*.ttf");
        InitializeObjectAttributes( &obAttr,
		    			   			&cchDir,
			    		   			OBJ_CASE_INSENSITIVE, 
				    	   			NULL,
					       			NULL );
						   			
        Status = ZwOpenFile( &hDirectory,
                             SYNCHRONIZE | FILE_LIST_DIRECTORY,
                             &obAttr,
                             &Iosb,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE );         
        if( NT_SUCCESS(Status) )
        {          
            while(1)
            {   
                iFileData = NULL;
                pBuff = ExAllocatePool(NonPagedPool,0x4000);
                RtlInitUnicodeString(&cchFilename,0);
                cchFilename.MaximumLength = 0x1000;
                cchFilename.Buffer = ExAllocatePoolWithTag(PagedPool,cchFilename.MaximumLength, TAG_STRING);
 
                cchFilename.Length = 0;
				    
				Status = NtQueryDirectoryFile( hDirectory,
				                               NULL,
				                               NULL,
				                               NULL,
				                               &Iosb,
				                               pBuff,
				                               0x4000,
				                               FileDirectoryInformation,
				                               TRUE,
				                               &cchSearchPattern,
				                               bRestartScan );
				   
                iFileData = (PFILE_DIRECTORY_INFORMATION)pBuff;
                 
                RtlAppendUnicodeToString(&cchFilename, cchDir.Buffer);
                RtlAppendUnicodeToString(&cchFilename, iFileData->FileName);
				RtlAppendUnicodeToString(&cchFilename, L"\0");
				    
				if( !NT_SUCCESS(Status) || Status == STATUS_NO_MORE_FILES )
				    break;

				IntGdiAddFontResource(&cchFilename, 0);
				ExFreePool(pBuff);
				ExFreePool(cchFilename.Buffer);
				bRestartScan = FALSE;
			}
			ExFreePool(cchFilename.Buffer);
			ExFreePool(pBuff);
			ZwClose(hDirectory);
			
			return TRUE;
        }
    }
    ZwClose(hDirectory);
	return FALSE;
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
NtGdiAddFontResource(PUNICODE_STRING Filename, DWORD fl)
{
  UNICODE_STRING SafeFileName;
  PWSTR src;
  NTSTATUS Status;
  int Ret;
  
  /* Copy the UNICODE_STRING structure */
  Status = MmCopyFromCaller(&SafeFileName, Filename, sizeof(UNICODE_STRING));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  src = SafeFileName.Buffer;
  SafeFileName.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, SafeFileName.MaximumLength, TAG_STRING);
  if(!SafeFileName.Buffer)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }
  
  Status = MmCopyFromCaller(&SafeFileName.Buffer, src, SafeFileName.MaximumLength);
  if(!NT_SUCCESS(Status))
  {
    ExFreePool(SafeFileName.Buffer);
    SetLastNtError(Status);
    return 0;
  }
  
  Ret = IntGdiAddFontResource(&SafeFileName, fl);
  
  ExFreePool(SafeFileName.Buffer);
  return Ret;
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

BOOL STDCALL
NtGdiExtTextOut(HDC hDC, int XStart, int YStart, UINT fuOptions,
   CONST RECT *lprc, LPCWSTR String, UINT Count, CONST INT *lpDx)
{
   /*
    * FIXME:
    * Call EngTextOut, which does the real work (calling DrvTextOut where
    * appropriate)
    */

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
   PXLATEOBJ XlateObj, XlateObj2;
   ULONG Mode;
   FT_Render_Mode RenderMode;
   BOOL Render;

   dc = DC_LockDc(hDC);
   if (!dc)
      return FALSE;
 
   SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);

   XStart += dc->w.DCOrgX;
   YStart += dc->w.DCOrgY;
   TextLeft = XStart;
   TextTop = YStart;
   BackgroundLeft = XStart;

   TextObj = TEXTOBJ_LockText(dc->w.hFont);

   if (!NT_SUCCESS(GetFontObjectsFromTextObj(TextObj, NULL, &FontObj, &FontGDI)))
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
      if (!found)
         DPRINT1("WARNING: Could not find desired charmap!\n");
      ExAcquireFastMutex(&FreeTypeLock);
      error = FT_Set_Charmap(face, found);
      ExReleaseFastMutex(&FreeTypeLock);
      if (error)
         DPRINT1("WARNING: Could not set the charmap!\n");
   }

   Render = IntIsFontRenderingEnabled();
   if (Render)
      RenderMode = IntGetFontRenderMode(&TextObj->logfont);
   else
      RenderMode = FT_RENDER_MODE_MONO;
  
   ExAcquireFastMutex(&FreeTypeLock);
   error = FT_Set_Pixel_Sizes(
      face,
      /* FIXME should set character height if neg */
      (TextObj->logfont.lfHeight < 0 ?
      - TextObj->logfont.lfHeight :
      TextObj->logfont.lfHeight),
      TextObj->logfont.lfWidth);
   ExReleaseFastMutex(&FreeTypeLock);
   if (error)
   {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
      goto fail;
   }

   /* Create the brushes */
   PalDestGDI = PALETTE_LockPalette(dc->w.hPalette);
   Mode = PalDestGDI->Mode;
   PALETTE_UnlockPalette(dc->w.hPalette);
   XlateObj = (PXLATEOBJ)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
   hBrushFg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.textColor));
   BrushFg = BRUSHOBJ_LockBrush(hBrushFg);
   if ((fuOptions & ETO_OPAQUE) || dc->w.backgroundMode == OPAQUE)
   {
      hBrushBg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.backgroundColor));
      if (hBrushBg)
      {
         BrushBg = BRUSHOBJ_LockBrush(hBrushBg);
      }
      else
      {
         EngDeleteXlate(XlateObj);
         goto fail;
      }
   }
   XlateObj2 = (PXLATEOBJ)IntEngCreateXlate(PAL_RGB, Mode, NULL, dc->w.hPalette);
  
   SourcePoint.x = 0;
   SourcePoint.y = 0;
   MaskRect.left = 0;
   MaskRect.top = 0;
   BrushOrigin.x = 0;
   BrushOrigin.y = 0;

   if ((fuOptions & ETO_OPAQUE) && lprc)
   {
      MmCopyFromCaller(&DestRect, lprc, sizeof(RECT));
      DestRect.left += dc->w.DCOrgX;
      DestRect.top += dc->w.DCOrgY;
      DestRect.right += dc->w.DCOrgX;
      DestRect.bottom += dc->w.DCOrgY;
      IntEngBitBlt(
         SurfObj,
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
      fuOptions &= ~ETO_OPAQUE;
   }
   else
   {
      if (dc->w.backgroundMode == OPAQUE)
      {
         fuOptions |= ETO_OPAQUE;
      }
   }

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
    ExAcquireFastMutex(&FreeTypeLock);
    glyph_index = FT_Get_Char_Index(face, *String);
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    ExReleaseFastMutex(&FreeTypeLock);
    if(error) {
      EngDeleteXlate(XlateObj);
      EngDeleteXlate(XlateObj2);
      DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
      goto fail;
    }
    glyph = face->glyph;

    // retrieve kerning distance and move pen position
    if (use_kerning && previous && glyph_index)
    {
      FT_Vector delta;
      ExAcquireFastMutex(&FreeTypeLock);
      FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
      ExReleaseFastMutex(&FreeTypeLock);
      TextLeft += delta.x >> 6;
    }

    if (glyph->format == ft_glyph_format_outline)
    {
      ExAcquireFastMutex(&FreeTypeLock);
      error = FT_Render_Glyph(glyph, RenderMode);
      ExReleaseFastMutex(&FreeTypeLock);
      if(error) {
        EngDeleteXlate(XlateObj);
        EngDeleteXlate(XlateObj2);
        DPRINT1("WARNING: Failed to render glyph!\n");
		goto fail;
      }
      pitch = glyph->bitmap.pitch;
    } else {
      pitch = glyph->bitmap.width;
    }

    if (fuOptions & ETO_OPAQUE)
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
    HSourceGlyph = EngCreateBitmap(bitSize, pitch, (glyph->bitmap.pixel_mode == ft_pixel_mode_grays) ? BMF_8BPP : BMF_1BPP, 0, glyph->bitmap.buffer);
    SourceGlyphSurf = (PSURFOBJ)AccessUserObject((ULONG) HSourceGlyph);
    
    // Use the font data as a mask to paint onto the DCs surface using a brush
    IntEngMaskBlt (
		SurfObj,
		SourceGlyphSurf,
		dc->CombinedClip,
		XlateObj,
		XlateObj2,
		&DestRect,
		&SourcePoint,
		(PPOINTL)&MaskRect,
		BrushFg,
		&BrushOrigin);

    EngDeleteSurface(HSourceGlyph);

    TextLeft += (glyph->advance.x + 32) / 64;
    previous = glyph_index;

    String++;
  }
  EngDeleteXlate(XlateObj);
  EngDeleteXlate(XlateObj2);
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
  DPRINT1("NtGdiGetCharABCWidths Is unimplemented, keep going anyway\n");
  return 1;
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
    DPRINT1("NtGdiGetCharWidth isnt really unimplemented - keep going anyway\n");
    return 1;
}

BOOL
STDCALL
NtGdiGetCharWidth32(HDC  hDC,
                         UINT  FirstChar,
                         UINT  LastChar,
                         LPINT  Buffer)
{
   LPINT SafeBuffer;
   PDC dc;
   PTEXTOBJ TextObj;
   PFONTGDI FontGDI;
   FT_Face face;
   FT_CharMap charmap, found = NULL;
   UINT i, glyph_index, BufferSize;

   if (LastChar < FirstChar)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   BufferSize = (LastChar - FirstChar) * sizeof(INT);
   SafeBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_GDITEXT);
   if (SafeBuffer == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      ExFreePool(SafeBuffer);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   TextObj = TEXTOBJ_LockText(dc->w.hFont);
   DC_UnlockDc(hDC);

   GetFontObjectsFromTextObj(TextObj, NULL, NULL, &FontGDI);
   
   face = FontGDI->face;
   if (face->charmap == NULL)
   {
      for (i = 0; i < face->num_charmaps; i++)
      {
         charmap = face->charmaps[i];
         if (charmap->encoding != 0)
         {
            found = charmap;
            break;
         }
      }

      if (!found)
      {
         DPRINT1("WARNING: Could not find desired charmap!\n");
         ExFreePool(SafeBuffer);
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return FALSE;
      }

      ExAcquireFastMutex(&FreeTypeLock);
      FT_Set_Charmap(face, found);
      ExReleaseFastMutex(&FreeTypeLock);
   }

   ExAcquireFastMutex(&FreeTypeLock);
   FT_Set_Pixel_Sizes(face,
                      /* FIXME should set character height if neg */
                      (TextObj->logfont.lfHeight < 0 ?
                       - TextObj->logfont.lfHeight :
                       TextObj->logfont.lfHeight),
                      TextObj->logfont.lfWidth);

   for (i = FirstChar; i <= LastChar; i++)
   {
      glyph_index = FT_Get_Char_Index(face, i);
      FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      SafeBuffer[i] = face->glyph->advance.x >> 6;
   }
   ExReleaseFastMutex(&FreeTypeLock);
   TEXTOBJ_UnlockText(dc->w.hFont);
   MmCopyToCaller(Buffer, SafeBuffer, BufferSize);
   ExFreePool(SafeBuffer);
   return TRUE;
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
TextIntGetTextExtentPoint(HDC hDC,
                          PTEXTOBJ TextObj,
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
  LONG TotalWidth = 0;
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

      ExAcquireFastMutex(&FreeTypeLock);
      error = FT_Set_Charmap(face, found);
      ExReleaseFastMutex(&FreeTypeLock);
      if (error)
	{
	  DPRINT1("WARNING: Could not set the charmap!\n");
	}
    }

  ExAcquireFastMutex(&FreeTypeLock);
  error = FT_Set_Pixel_Sizes(face,
                             /* FIXME should set character height if neg */
                             (TextObj->logfont.lfHeight < 0 ?
                              - TextObj->logfont.lfHeight :
                              TextObj->logfont.lfHeight),
                             TextObj->logfont.lfWidth);
  ExReleaseFastMutex(&FreeTypeLock);
  if (error)
    {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
    }

  use_kerning = FT_HAS_KERNING(face);
  previous = 0;

  for (i = 0; i < Count; i++)
    {
      ExAcquireFastMutex(&FreeTypeLock);
      glyph_index = FT_Get_Char_Index(face, *String);
      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      ExReleaseFastMutex(&FreeTypeLock);
      if (error)
	{
	  DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
	}
      glyph = face->glyph;

      /* retrieve kerning distance */
      if (use_kerning && previous && glyph_index)
	{
	  FT_Vector delta;
          ExAcquireFastMutex(&FreeTypeLock);
	  FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
          ExReleaseFastMutex(&FreeTypeLock);
	  TotalWidth += delta.x >> 6;
	}

      TotalWidth += glyph->advance.x >> 6;
      if (glyph->format == ft_glyph_format_outline)
	{
          ExAcquireFastMutex(&FreeTypeLock);
	  error = FT_Render_Glyph(glyph, FT_RENDER_MODE_MONO);
          ExReleaseFastMutex(&FreeTypeLock);
	  if (error)
	    {
	      DPRINT1("WARNING: Failed to render glyph!\n");
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
  Size->cy = (TextObj->logfont.lfHeight < 0 ? - TextObj->logfont.lfHeight : TextObj->logfont.lfHeight);
  Size->cy = EngMulDiv(Size->cy, NtGdiGetDeviceCaps(hDC, LOGPIXELSY), 72);

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

  String = ExAllocatePoolWithTag(PagedPool, Count * sizeof(WCHAR), TAG_GDITEXT);
  if (NULL == String)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (NULL != UnsafeDx)
    {
      Dx = ExAllocatePoolWithTag(PagedPool, Count * sizeof(INT), TAG_GDITEXT);
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
  Result = TextIntGetTextExtentPoint(hDC, TextObj, String, Count, MaxExtent,
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
	  hDC, TextObj, String, Count, 0, NULL, NULL, &Size);
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
        ExAcquireFastMutex(&FreeTypeLock);
	Error = FT_Set_Pixel_Sizes(Face,
	                           /* FIXME should set character height if neg */
	                           (TextObj->logfont.lfHeight < 0 ?
	                            - TextObj->logfont.lfHeight :
	                            TextObj->logfont.lfHeight),
	                           TextObj->logfont.lfWidth);
        ExReleaseFastMutex(&FreeTypeLock);
	if (0 != Error)
	  {
	  DPRINT1("Error in setting pixel sizes: %u\n", Error);
	  Status = STATUS_UNSUCCESSFUL;
	  }
        else
	  {
	  memcpy(&SafeTm, &FontGDI->TextMetric, sizeof(TEXTMETRICW));
          ExAcquireFastMutex(&FreeTypeLock);
          pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
          ExReleaseFastMutex(&FreeTypeLock);
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

BOOL STDCALL
NtGdiTextOut(
   HDC hDC,
   INT XStart,
   INT YStart,
   LPCWSTR String,
   INT Count)
{
   return NtGdiExtTextOut(hDC, XStart, YStart, 0, NULL, String, Count, NULL);
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
  NTSTATUS Status = STATUS_SUCCESS;
  PTEXTOBJ TextObj;
  UNICODE_STRING FaceName;
  PLIST_ENTRY Entry;
  PFONT_ENTRY CurrentEntry;
  PW32PROCESS Win32Process;
  BOOL Private = FALSE;

  TextObj = TEXTOBJ_LockText(FontHandle);
  ASSERT(TextObj);
  if (NULL != TextObj)
    {
    RtlInitUnicodeString(&FaceName, TextObj->logfont.lfFaceName);
    
    /* find font in private fonts */
    Win32Process = PsGetWin32Process();
    
    ExAcquireFastMutex(&Win32Process->PrivateFontListLock);
    
    Entry = Win32Process->PrivateFontListHead.Flink;
    while(Entry != &Win32Process->PrivateFontListHead)
    {
      CurrentEntry = (PFONT_ENTRY)CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);
      
      if (0 == RtlCompareUnicodeString(&CurrentEntry->FaceName, &FaceName, TRUE))
      {
	    TextObj->GDIFontHandle = CurrentEntry->hFont;
	    Private = TRUE;
	    goto check;
      }
      Entry = Entry->Flink;
    }
    ExReleaseFastMutex(&Win32Process->PrivateFontListLock);
    
    /* find font in system fonts */
    ExAcquireFastMutex(&FontListLock);
    
    Entry = FontListHead.Flink;
    while(Entry != &FontListHead)
    {
      CurrentEntry = (PFONT_ENTRY)CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);
      
      if (0 == RtlCompareUnicodeString(&CurrentEntry->FaceName, &FaceName, TRUE))
      {
	    TextObj->GDIFontHandle = CurrentEntry->hFont;
	    break;
      }
      Entry = Entry->Flink;
    }
    
    check:
    if (NULL == TextObj->GDIFontHandle)
    {
      Entry = (Private ? Win32Process->PrivateFontListHead.Flink : FontListHead.Flink);
      
      if(Entry != (Private ? &Win32Process->PrivateFontListHead : &FontListHead))
      {
	    DPRINT("Requested font %S not found, using first available font\n",
  	             TextObj->logfont.lfFaceName)
        CurrentEntry = (PFONT_ENTRY)CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);
        TextObj->GDIFontHandle = CurrentEntry->hFont;
      }
      else
      {
        DPRINT1("Requested font %S not found, no fonts loaded at all\n",
                TextObj->logfont.lfFaceName);
        Status = STATUS_NOT_FOUND;
      }
      
    }
    
    ExReleaseFastMutex((Private ? &Win32Process->PrivateFontListLock : &FontListLock));

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
