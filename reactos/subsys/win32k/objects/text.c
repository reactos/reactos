/*
 * ReactOS W32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 * Parts based on Wine code
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 * Copyright 2002,2003 Shachar Shemesh
 * Copyright 2001 Huw D M Davies for CodeWeavers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: text.c,v 1.90 2004/04/23 21:35:59 weiden Exp $ */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <napi/win32.h>
#include <internal/safe.h>
#include <win32k/brush.h>
#include <win32k/dc.h>
#include <win32k/text.h>
#include <win32k/font.h>
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

static PWCHAR ElfScripts[32] = { /* these are in the order of the fsCsb[0] bits */
  L"Western", /*00*/
  L"Central_European",
  L"Cyrillic",
  L"Greek",
  L"Turkish",
  L"Hebrew",
  L"Arabic",
  L"Baltic",
  L"Vietnamese", /*08*/
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, /*15*/
  L"Thai",
  L"Japanese",
  L"CHINESE_GB2312",
  L"Hangul",
  L"CHINESE_BIG5",
  L"Hangul(Johab)",
  NULL, NULL, /*23*/
  NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  L"Symbol" /*31*/
};

/*
 *  For NtGdiTranslateCharsetInfo
 */
#define FS(x) {{0,0,0,0},{0x1<<(x),0}}
#define MAXTCIINDEX 32
static CHARSETINFO FontTci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, FS(0)},
  { EASTEUROPE_CHARSET, 1250, FS(1)},
  { RUSSIAN_CHARSET, 1251, FS(2)},
  { GREEK_CHARSET, 1253, FS(3)},
  { TURKISH_CHARSET, 1254, FS(4)},
  { HEBREW_CHARSET, 1255, FS(5)},
  { ARABIC_CHARSET, 1256, FS(6)},
  { BALTIC_CHARSET, 1257, FS(7)},
  { VIETNAMESE_CHARSET, 1258, FS(8)},
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* ANSI and OEM */
  { THAI_CHARSET,  874,  FS(16)},
  { SHIFTJIS_CHARSET, 932, FS(17)},
  { GB2312_CHARSET, 936, FS(18)},
  { HANGEUL_CHARSET, 949, FS(19)},
  { CHINESEBIG5_CHARSET, 950, FS(20)},
  { JOHAB_CHARSET, 1361, FS(21)},
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* reserved for system */
  { DEFAULT_CHARSET, 0, FS(0)},
  { SYMBOL_CHARSET, CP_SYMBOL, FS(31)},
};


BOOL FASTCALL
IntIsFontRenderingEnabled(VOID)
{
  BOOL Ret;
  HDC hDC;
  PDC dc;
  SURFOBJ *SurfObj;
  Ret = RenderingEnabled;
  hDC = IntGetScreenDC();
  if(hDC)
  {
    dc = DC_LockDc(hDC);
    SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);
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
  FONTOBJ *FontObj;
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
  FontObj = (FONTOBJ*) AccessUserObject( (ULONG) NewFont );
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

  IntLockFreeType;
  error = FT_New_Memory_Face(library, buffer, size, 0, &face);
  IntUnLockFreeType;
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
  FontGDI->TextMetric.tmAscent = (face->size->metrics.ascender + 32) >> 6; // units above baseline
  FontGDI->TextMetric.tmDescent = (32 - face->size->metrics.descender) >> 6; // units below baseline
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
    
    IntLockProcessPrivateFonts(Win32Process);
    InsertTailList(&Win32Process->PrivateFontListHead, &entry->ListEntry);
    FontsLoaded++;
    IntUnLockProcessPrivateFonts(Win32Process);
  }
  else
  {
    IntLockGlobalFonts;
    InsertTailList(&FontListHead, &entry->ListEntry);
    FontsLoaded++;
    IntUnLockGlobalFonts;
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
    BOOLEAN Result = FALSE;
	
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
		if (bRestartScan)
		  {
                pBuff = ExAllocatePool(NonPagedPool,0x4000);
		    if (pBuff == NULL)
		      {
		        break;
		      }
                RtlInitUnicodeString(&cchFilename,0);
                cchFilename.MaximumLength = 0x1000;
                cchFilename.Buffer = ExAllocatePoolWithTag(PagedPool,cchFilename.MaximumLength, TAG_STRING);
		    if (cchFilename.Buffer == NULL)
		      {
		        ExFreePool(pBuff);
		        break;
		      }
		  }
				    
				Status = NtQueryDirectoryFile( hDirectory,
				                               NULL,
				                               NULL,
				                               NULL,
				                               &Iosb,
				                               pBuff,
				                               0x4000,
				                               FileDirectoryInformation,
					       FALSE,
				                               &cchSearchPattern,
				                               bRestartScan );
				   
				if( !NT_SUCCESS(Status) || Status == STATUS_NO_MORE_FILES )
		  {
				ExFreePool(pBuff);
				ExFreePool(cchFilename.Buffer);
		    break;
		  }
				bRestartScan = FALSE;
                iFileData = (PFILE_DIRECTORY_INFORMATION)pBuff;
		while(1)
		  {
		    UNICODE_STRING tmpString;
		    tmpString.Buffer = iFileData->FileName;
		    tmpString.Length = tmpString.MaximumLength = iFileData->FileNameLength;
                    RtlCopyUnicodeString(&cchFilename, &cchDir);
                    RtlAppendUnicodeStringToString(&cchFilename, &tmpString);
		    cchFilename.Buffer[cchFilename.Length / sizeof(WCHAR)] = 0;
		    if (0 != IntGdiAddFontResource(&cchFilename, 0))
		      {
		        Result = TRUE;
		      }
		    if (iFileData->NextEntryOffset == 0)
		      {
		        break;
		      }
		    iFileData = (PVOID)iFileData + iFileData->NextEntryOffset;
		  }
			}
        }
    }
    ZwClose(hDirectory);
    return Result;
}

static NTSTATUS STDCALL
GetFontObjectsFromTextObj(PTEXTOBJ TextObj, HFONT *FontHandle, FONTOBJ **FontObj, PFONTGDI *FontGDI)
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

/*************************************************************************
 * TranslateCharsetInfo
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in Cs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *Src.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
static BOOLEAN STDCALL
IntTranslateCharsetInfo(PDWORD Src, /* [in]
                         if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
                         if flags == TCI_SRCCHARSET: a character set value
                         if flags == TCI_SRCCODEPAGE: a code page value */
                        LPCHARSETINFO Cs, /* [out] structure to receive charset information */
                        DWORD Flags /* [in] determines interpretation of lpSrc */)
{
  int Index = 0;

  switch (Flags)
    {
      case TCI_SRCFONTSIG:
	while (0 == (*Src >> Index & 0x0001) && Index < MAXTCIINDEX)
          {
            Index++;
          }
        break;
      case TCI_SRCCODEPAGE:
        while ((UINT) (Src) != FontTci[Index].ciACP && Index < MAXTCIINDEX)
          {
            Index++;
          }
        break;
      case TCI_SRCCHARSET:
        while ((UINT) (Src) != FontTci[Index].ciCharset && Index < MAXTCIINDEX)
          {
            Index++;
          }
        break;
      default:
        return FALSE;
    }

  if (MAXTCIINDEX <= Index || DEFAULT_CHARSET == FontTci[Index].ciCharset)
    {
      return FALSE;
    }

  memcpy(Cs, &FontTci[Index], sizeof(CHARSETINFO));

  return TRUE;
}

BOOL STDCALL
NtGdiTranslateCharsetInfo(PDWORD Src,
                          LPCHARSETINFO UnsafeCs,
                          DWORD Flags)
{
  CHARSETINFO Cs;
  BOOLEAN Ret;
  NTSTATUS Status;

  Ret = IntTranslateCharsetInfo(Src, &Cs, Flags);
  if (Ret)
    {
      Status = MmCopyToCaller(UnsafeCs, &Cs, sizeof(CHARSETINFO));
      if (! NT_SUCCESS(Status))
        {
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return FALSE;
        }
    }

  return (BOOL) Ret;
}


/*************************************************************
 * IntGetOutlineTextMetrics
 *
 */
static unsigned FASTCALL
IntGetOutlineTextMetrics(PFONTGDI FontGDI, UINT Size,
                         OUTLINETEXTMETRICW *Otm)
{
  unsigned Needed;
  TT_OS2 *pOS2;
  TT_HoriHeader *pHori;
  TT_Postscript *pPost;
  FT_Fixed XScale, YScale;
  ANSI_STRING FamilyNameA, StyleNameA;
  UNICODE_STRING FamilyNameW, StyleNameW, Regular;
  char *Cp;
  int Ascent, Descent;
  TEXTMETRICW *TM;

  Needed = sizeof(OUTLINETEXTMETRICW);

  RtlInitAnsiString(&FamilyNameA, FontGDI->face->family_name);
  RtlAnsiStringToUnicodeString(&FamilyNameW, &FamilyNameA, TRUE);

  RtlInitAnsiString(&StyleNameA, FontGDI->face->style_name);
  RtlAnsiStringToUnicodeString(&StyleNameW, &StyleNameA, TRUE);

  /* These names should be read from the TT name table */

  /* length of otmpFamilyName */
  Needed += FamilyNameW.Length + sizeof(WCHAR);

  RtlInitUnicodeString(&Regular, L"regular");
  /* length of otmpFaceName */
  if (0 == RtlCompareUnicodeString(&StyleNameW, &Regular, TRUE))
    {
      Needed += FamilyNameW.Length + sizeof(WCHAR); /* just the family name */
    }
  else
    {
      Needed += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1); /* family + " " + style */
    }

  /* length of otmpStyleName */
  Needed += StyleNameW.Length + sizeof(WCHAR);

  /* length of otmpFullName */
  Needed += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1);

  if (Size < Needed)
    {
      RtlFreeUnicodeString(&FamilyNameW);
      RtlFreeUnicodeString(&StyleNameW);
      return Needed;
    }

  XScale = FontGDI->face->size->metrics.x_scale;
  YScale = FontGDI->face->size->metrics.y_scale;

  IntLockFreeType;
  pOS2 = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_os2);
  if (NULL == pOS2)
    {
      IntUnLockFreeType;
      DPRINT1("Can't find OS/2 table - not TT font?\n");
      RtlFreeUnicodeString(&StyleNameW);
      RtlFreeUnicodeString(&FamilyNameW);
      return 0;
    }

  pHori = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_hhea);
  if (NULL == pHori)
    {
      IntUnLockFreeType;
      DPRINT1("Can't find HHEA table - not TT font?\n");
      RtlFreeUnicodeString(&StyleNameW);
      RtlFreeUnicodeString(&FamilyNameW);
      return 0;
    }

  pPost = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_post); /* we can live with this failing */
  IntUnLockFreeType;

  Otm->otmSize = Needed;

  if (0 == pOS2->usWinAscent + pOS2->usWinDescent)
    {
      Ascent = pHori->Ascender;
      Descent = -pHori->Descender;
    }
  else
    {
      Ascent = pOS2->usWinAscent;
      Descent = pOS2->usWinDescent;
    }

  TM = &Otm->otmTextMetrics;
  TM->tmAscent = (FT_MulFix(Ascent, YScale) + 32) >> 6;
  TM->tmDescent = (FT_MulFix(Descent, YScale) + 32) >> 6;
  TM->tmInternalLeading = (FT_MulFix(Ascent + Descent
                                     - FontGDI->face->units_per_EM, YScale) + 32) >> 6;

  TM->tmHeight = TM->tmAscent + TM->tmDescent;

  /* MSDN says:
   *  el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
   */
  TM->tmExternalLeading = max(0, (FT_MulFix(pHori->Line_Gap
                                            - ((Ascent + Descent)
                                               - (pHori->Ascender - pHori->Descender)),
                                            YScale) + 32) >> 6);

  TM->tmAveCharWidth = (FT_MulFix(pOS2->xAvgCharWidth, XScale) + 32) >> 6;
  if (0 == TM->tmAveCharWidth)
    {
      TM->tmAveCharWidth = 1; 
    }
  TM->tmMaxCharWidth = (FT_MulFix(FontGDI->face->bbox.xMax - FontGDI->face->bbox.xMin,
                                 XScale) + 32) >> 6;
  TM->tmWeight = pOS2->usWeightClass;
  TM->tmOverhang = 0;
  TM->tmDigitizedAspectX = 300;
  TM->tmDigitizedAspectY = 300;
  TM->tmFirstChar = pOS2->usFirstCharIndex;
  TM->tmLastChar = pOS2->usLastCharIndex;
  TM->tmDefaultChar = pOS2->usDefaultChar;
  TM->tmBreakChar = L'\0' != pOS2->usBreakChar ? pOS2->usBreakChar : ' ';
  TM->tmItalic = (FontGDI->face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0;
  TM->tmUnderlined = 0; /* entry in OS2 table */
  TM->tmStruckOut = 0; /* entry in OS2 table */

  /* Yes TPMF_FIXED_PITCH is correct; braindead api */
  if (! FT_IS_FIXED_WIDTH(FontGDI->face))
    {
      TM->tmPitchAndFamily = TMPF_FIXED_PITCH;
    }
  else
    {
      TM->tmPitchAndFamily = 0;
    }

  switch (pOS2->panose[PAN_FAMILYTYPE_INDEX])
    {
      case PAN_FAMILY_SCRIPT:
        TM->tmPitchAndFamily |= FF_SCRIPT;
        break;
      case PAN_FAMILY_DECORATIVE:
      case PAN_FAMILY_PICTORIAL:
        TM->tmPitchAndFamily |= FF_DECORATIVE;
        break;
      case PAN_FAMILY_TEXT_DISPLAY:
        if (0 == TM->tmPitchAndFamily) /* fixed */
          {
            TM->tmPitchAndFamily = FF_MODERN;
          }
        else
          {
            switch (pOS2->panose[PAN_SERIFSTYLE_INDEX])
              {
                case PAN_SERIF_NORMAL_SANS:
                case PAN_SERIF_OBTUSE_SANS:
                case PAN_SERIF_PERP_SANS:
                  TM->tmPitchAndFamily |= FF_SWISS;
                  break;
                default:
                  TM->tmPitchAndFamily |= FF_ROMAN;
                  break;
              }
          }
        break;
      default:
        TM->tmPitchAndFamily |= FF_DONTCARE;
    }

  if (FT_IS_SCALABLE(FontGDI->face))
    {
      TM->tmPitchAndFamily |= TMPF_VECTOR;
    }
  if (FT_IS_SFNT(FontGDI->face))
    {
      TM->tmPitchAndFamily |= TMPF_TRUETYPE;
    }

#ifndef TODO
  TM->tmCharSet = DEFAULT_CHARSET;
#endif

  Otm->otmFiller = 0;
  memcpy(&Otm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
  Otm->otmfsSelection = pOS2->fsSelection;
  Otm->otmfsType = pOS2->fsType;
  Otm->otmsCharSlopeRise = pHori->caret_Slope_Rise;
  Otm->otmsCharSlopeRun = pHori->caret_Slope_Run;
  Otm->otmItalicAngle = 0; /* POST table */
  Otm->otmEMSquare = FontGDI->face->units_per_EM;
  Otm->otmAscent = (FT_MulFix(pOS2->sTypoAscender, YScale) + 32) >> 6;
  Otm->otmDescent = (FT_MulFix(pOS2->sTypoDescender, YScale) + 32) >> 6;
  Otm->otmLineGap = (FT_MulFix(pOS2->sTypoLineGap, YScale) + 32) >> 6;
  Otm->otmsCapEmHeight = (FT_MulFix(pOS2->sCapHeight, YScale) + 32) >> 6;
  Otm->otmsXHeight = (FT_MulFix(pOS2->sxHeight, YScale) + 32) >> 6;
  Otm->otmrcFontBox.left = (FT_MulFix(FontGDI->face->bbox.xMin, XScale) + 32) >> 6;
  Otm->otmrcFontBox.right = (FT_MulFix(FontGDI->face->bbox.xMax, XScale) + 32) >> 6;
  Otm->otmrcFontBox.top = (FT_MulFix(FontGDI->face->bbox.yMax, YScale) + 32) >> 6;
  Otm->otmrcFontBox.bottom = (FT_MulFix(FontGDI->face->bbox.yMin, YScale) + 32) >> 6;
  Otm->otmMacAscent = 0; /* where do these come from ? */
  Otm->otmMacDescent = 0;
  Otm->otmMacLineGap = 0;
  Otm->otmusMinimumPPEM = 0; /* TT Header */
  Otm->otmptSubscriptSize.x = (FT_MulFix(pOS2->ySubscriptXSize, XScale) + 32) >> 6;
  Otm->otmptSubscriptSize.y = (FT_MulFix(pOS2->ySubscriptYSize, YScale) + 32) >> 6;
  Otm->otmptSubscriptOffset.x = (FT_MulFix(pOS2->ySubscriptXOffset, XScale) + 32) >> 6;
  Otm->otmptSubscriptOffset.y = (FT_MulFix(pOS2->ySubscriptYOffset, YScale) + 32) >> 6;
  Otm->otmptSuperscriptSize.x = (FT_MulFix(pOS2->ySuperscriptXSize, XScale) + 32) >> 6;
  Otm->otmptSuperscriptSize.y = (FT_MulFix(pOS2->ySuperscriptYSize, YScale) + 32) >> 6;
  Otm->otmptSuperscriptOffset.x = (FT_MulFix(pOS2->ySuperscriptXOffset, XScale) + 32) >> 6;
  Otm->otmptSuperscriptOffset.y = (FT_MulFix(pOS2->ySuperscriptYOffset, YScale) + 32) >> 6;
  Otm->otmsStrikeoutSize = (FT_MulFix(pOS2->yStrikeoutSize, YScale) + 32) >> 6;
  Otm->otmsStrikeoutPosition = (FT_MulFix(pOS2->yStrikeoutPosition, YScale) + 32) >> 6;
  if (NULL == pPost)
    {
      Otm->otmsUnderscoreSize = 0;
      Otm->otmsUnderscorePosition = 0;
    }
  else
    {
      Otm->otmsUnderscoreSize = (FT_MulFix(pPost->underlineThickness, YScale) + 32) >> 6;
      Otm->otmsUnderscorePosition = (FT_MulFix(pPost->underlinePosition, YScale) + 32) >> 6;
    }

  /* otmp* members should clearly have type ptrdiff_t, but M$ knows best */
  Cp = (char*) Otm + sizeof(OUTLINETEXTMETRICW);
  Otm->otmpFamilyName = (LPSTR)(Cp - (char*) Otm);
  wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
  Cp += FamilyNameW.Length + sizeof(WCHAR);
  Otm->otmpStyleName = (LPSTR)(Cp - (char*) Otm);
  wcscpy((WCHAR*) Cp, StyleNameW.Buffer);
  Cp += StyleNameW.Length + sizeof(WCHAR);
  Otm->otmpFaceName = (LPSTR)(Cp - (char*) Otm);
  wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
  if (0 != RtlCompareUnicodeString(&StyleNameW, &Regular, TRUE))
    {
      wcscat((WCHAR*) Cp, L" ");
      wcscat((WCHAR*) Cp, StyleNameW.Buffer);
      Cp += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1);
    }
  else
    {
      Cp += FamilyNameW.Length + sizeof(WCHAR);
    }
  Otm->otmpFullName = (LPSTR)(Cp - (char*) Otm);
  wcscpy((WCHAR*) Cp, FamilyNameW.Buffer);
  wcscat((WCHAR*) Cp, L" ");
  wcscat((WCHAR*) Cp, StyleNameW.Buffer);

  RtlFreeUnicodeString(&StyleNameW);
  RtlFreeUnicodeString(&FamilyNameW);

  return Needed;
}

static PFONTGDI FASTCALL
FindFaceNameInList(PUNICODE_STRING FaceName, PLIST_ENTRY Head)
{
  PLIST_ENTRY Entry;
  PFONT_ENTRY CurrentEntry;
  ANSI_STRING EntryFaceNameA;
  UNICODE_STRING EntryFaceNameW;
  PFONTGDI FontGDI;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

      if (NULL == (FontGDI = AccessInternalObject((ULONG) CurrentEntry->hFont)))
        {
          Entry = Entry->Flink;
          continue;
        }
      RtlInitAnsiString(&EntryFaceNameA, FontGDI->face->family_name);
      RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
      if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
          EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
          EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }

      if (0 == RtlCompareUnicodeString(FaceName, &EntryFaceNameW, TRUE))
        {
          RtlFreeUnicodeString(&EntryFaceNameW);
          return FontGDI;
        }

      RtlFreeUnicodeString(&EntryFaceNameW);
      Entry = Entry->Flink;
    }

  return NULL;
}

static PFONTGDI FASTCALL
FindFaceNameInLists(PUNICODE_STRING FaceName)
{
  PW32PROCESS Win32Process;
  PFONTGDI Font;

  /* Search the process local list */
  Win32Process = PsGetWin32Process();
  IntLockProcessPrivateFonts(Win32Process);
  Font = FindFaceNameInList(FaceName, &Win32Process->PrivateFontListHead);
  IntUnLockProcessPrivateFonts(Win32Process);
  if (NULL != Font)
    {
      return Font;
    }

  /* Search the global list */
  IntLockGlobalFonts;
  Font = FindFaceNameInList(FaceName, &FontListHead);
  IntUnLockGlobalFonts;

  return Font;
}

static void FASTCALL
FontFamilyFillInfo(PFONTFAMILYINFO Info, PCWSTR FaceName, PFONTGDI FontGDI)
{
  ANSI_STRING StyleA;
  UNICODE_STRING StyleW;
  TT_OS2 *pOS2;
  FONTSIGNATURE fs;
  DWORD fs_fsCsb0;
  CHARSETINFO CharSetInfo;
  unsigned i, Size;
  OUTLINETEXTMETRICW *Otm;
  LOGFONTW *Lf;
  TEXTMETRICW *TM;
  NEWTEXTMETRICW *Ntm;
  
  ZeroMemory(Info, sizeof(FONTFAMILYINFO));
  Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
  Otm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
  if (NULL == Otm)
    {
      return;
    }
  IntGetOutlineTextMetrics(FontGDI, Size, Otm);

  Lf = &Info->EnumLogFontEx.elfLogFont;
  TM = &Otm->otmTextMetrics;

  Lf->lfHeight = TM->tmHeight;
  Lf->lfWidth = TM->tmAveCharWidth;
  Lf->lfWeight = TM->tmWeight;
  Lf->lfItalic = TM->tmItalic;
  Lf->lfPitchAndFamily = (TM->tmPitchAndFamily & 0xf1) + 1;
  Lf->lfCharSet = TM->tmCharSet;
  Lf->lfOutPrecision = OUT_STROKE_PRECIS;
  Lf->lfClipPrecision = CLIP_STROKE_PRECIS;
  Lf->lfQuality = DRAFT_QUALITY;

  Ntm = &Info->NewTextMetricEx.ntmTm;
  Ntm->tmHeight = TM->tmHeight;
  Ntm->tmAscent = TM->tmAscent;
  Ntm->tmDescent = TM->tmDescent;
  Ntm->tmInternalLeading = TM->tmInternalLeading;
  Ntm->tmExternalLeading = TM->tmExternalLeading;
  Ntm->tmAveCharWidth = TM->tmAveCharWidth;
  Ntm->tmMaxCharWidth = TM->tmMaxCharWidth;
  Ntm->tmWeight = TM->tmWeight;
  Ntm->tmOverhang = TM->tmOverhang;
  Ntm->tmDigitizedAspectX = TM->tmDigitizedAspectX;
  Ntm->tmDigitizedAspectY = TM->tmDigitizedAspectY;
  Ntm->tmFirstChar = TM->tmFirstChar;
  Ntm->tmLastChar = TM->tmLastChar;
  Ntm->tmDefaultChar = TM->tmDefaultChar;
  Ntm->tmBreakChar = TM->tmBreakChar;
  Ntm->tmItalic = TM->tmItalic;
  Ntm->tmUnderlined = TM->tmUnderlined;
  Ntm->tmStruckOut = TM->tmStruckOut;
  Ntm->tmPitchAndFamily = TM->tmPitchAndFamily;
  Ntm->tmCharSet = TM->tmCharSet;
  Ntm->ntmFlags = TM->tmItalic ? NTM_ITALIC : 0;
  if (550 < TM->tmWeight)
    {
      Ntm->ntmFlags |= NTM_BOLD;
    }
  if (0 == Ntm->ntmFlags)
    {
      Ntm->ntmFlags = NTM_REGULAR;
    }

  Ntm->ntmSizeEM = Otm->otmEMSquare;
  Ntm->ntmCellHeight = 0;
  Ntm->ntmAvgWidth = 0;

  Info->FontType = (0 != (TM->tmPitchAndFamily & TMPF_TRUETYPE)
                    ? TRUETYPE_FONTTYPE : 0);
  if (0 == (TM->tmPitchAndFamily & TMPF_VECTOR))
    {
      Info->FontType |= RASTER_FONTTYPE;
    }

  ExFreePool(Otm);

  wcsncpy(Info->EnumLogFontEx.elfLogFont.lfFaceName, FaceName, LF_FACESIZE);
  wcsncpy(Info->EnumLogFontEx.elfFullName, FaceName, LF_FULLFACESIZE);
  RtlInitAnsiString(&StyleA, FontGDI->face->style_name);
  RtlAnsiStringToUnicodeString(&StyleW, &StyleA, TRUE);
  wcsncpy(Info->EnumLogFontEx.elfStyle, StyleW.Buffer, LF_FACESIZE);
  RtlFreeUnicodeString(&StyleW);

  Info->EnumLogFontEx.elfLogFont.lfCharSet = DEFAULT_CHARSET;
  Info->EnumLogFontEx.elfScript[0] = L'\0';
  IntLockFreeType;
  pOS2 = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_os2);
  IntUnLockFreeType;
  if (NULL != pOS2)
    {
      Info->NewTextMetricEx.ntmFontSig.fsCsb[0] = pOS2->ulCodePageRange1;
      Info->NewTextMetricEx.ntmFontSig.fsCsb[1] = pOS2->ulCodePageRange2;
      Info->NewTextMetricEx.ntmFontSig.fsUsb[0] = pOS2->ulUnicodeRange1;
      Info->NewTextMetricEx.ntmFontSig.fsUsb[1] = pOS2->ulUnicodeRange2;
      Info->NewTextMetricEx.ntmFontSig.fsUsb[2] = pOS2->ulUnicodeRange3;
      Info->NewTextMetricEx.ntmFontSig.fsUsb[3] = pOS2->ulUnicodeRange4;

      fs_fsCsb0 = pOS2->ulCodePageRange1;
      if (0 == pOS2->version)
        {
          FT_UInt Dummy;
	
          if (FT_Get_First_Char(FontGDI->face, &Dummy) < 0x100)
            {
              fs_fsCsb0 |= 1;
            }
          else
            {
              fs_fsCsb0 |= 1L << 31;
            }
        }
      if (0 == fs_fsCsb0)
        { /* let's see if we can find any interesting cmaps */
          for (i = 0; i < FontGDI->face->num_charmaps; i++)
            {
              switch (FontGDI->face->charmaps[i]->encoding)
                {
                  case ft_encoding_unicode:
                  case ft_encoding_apple_roman:
                    fs_fsCsb0 |= 1;
                    break;
                  case ft_encoding_symbol:
                    fs_fsCsb0 |= 1L << 31;
                    break;
                  default:
                    break;
                }
            }
        }

      for(i = 0; i < 32; i++)
        {
          if (0 != (fs_fsCsb0 & (1L << i)))
            {
              fs.fsCsb[0] = 1L << i;
              fs.fsCsb[1] = 0;
              if (! IntTranslateCharsetInfo(fs.fsCsb, &CharSetInfo, TCI_SRCFONTSIG))
                {
                  CharSetInfo.ciCharset = DEFAULT_CHARSET;
                }
              if (31 == i)
                {
                  CharSetInfo.ciCharset = SYMBOL_CHARSET;
                }
              if (DEFAULT_CHARSET != CharSetInfo.ciCharset)
                {
                  Info->EnumLogFontEx.elfLogFont.lfCharSet = CharSetInfo.ciCharset;
                  if (NULL != ElfScripts[i])
                    {
                      wcscpy(Info->EnumLogFontEx.elfScript, ElfScripts[i]);
                    }
                  else
                    {
                      DPRINT1("Unknown elfscript for bit %d\n", i);
                    }
                }
            }
        }
    }
}

static int FASTCALL
FindFaceNameInInfo(PUNICODE_STRING FaceName, PFONTFAMILYINFO Info, DWORD InfoEntries)
{
  DWORD i;
  UNICODE_STRING InfoFaceName;

  for (i = 0; i < InfoEntries; i++)
    {
      RtlInitUnicodeString(&InfoFaceName, Info[i].EnumLogFontEx.elfLogFont.lfFaceName);
      if (0 == RtlCompareUnicodeString(&InfoFaceName, FaceName, TRUE))
        {
          return i;
        }
    }

  return -1;
}

static BOOLEAN FASTCALL
FontFamilyInclude(LPLOGFONTW LogFont, PUNICODE_STRING FaceName,
                  PFONTFAMILYINFO Info, DWORD InfoEntries)
{
  UNICODE_STRING LogFontFaceName;

  RtlInitUnicodeString(&LogFontFaceName, LogFont->lfFaceName);
  if (0 != LogFontFaceName.Length
      && 0 != RtlCompareUnicodeString(&LogFontFaceName, FaceName, TRUE))
    {
      return FALSE;
    }

  return FindFaceNameInInfo(FaceName, Info, InfoEntries) < 0;
}

static BOOLEAN FASTCALL
GetFontFamilyInfoForList(LPLOGFONTW LogFont,
                         PFONTFAMILYINFO Info,
                         DWORD *Count,
                         DWORD Size,
                         PLIST_ENTRY Head)
{
  PLIST_ENTRY Entry;
  PFONT_ENTRY CurrentEntry;
  ANSI_STRING EntryFaceNameA;
  UNICODE_STRING EntryFaceNameW;
  PFONTGDI FontGDI;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

      if (NULL == (FontGDI = AccessInternalObject((ULONG) CurrentEntry->hFont)))
        {
          Entry = Entry->Flink;
          continue;
        }
      RtlInitAnsiString(&EntryFaceNameA, FontGDI->face->family_name);
      RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
      if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
          EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
          EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }

      if (FontFamilyInclude(LogFont, &EntryFaceNameW, Info, min(*Count, Size)))
        {
          if (*Count < Size)
            {
              FontFamilyFillInfo(Info + *Count, EntryFaceNameW.Buffer, FontGDI);
            }
          (*Count)++;
        }
      RtlFreeUnicodeString(&EntryFaceNameW);
      Entry = Entry->Flink;
    }

  return TRUE;
}

typedef struct FontFamilyInfoCallbackContext
{
  LPLOGFONTW LogFont;
  PFONTFAMILYINFO Info;
  DWORD Count;
  DWORD Size;
} FONT_FAMILY_INFO_CALLBACK_CONTEXT, *PFONT_FAMILY_INFO_CALLBACK_CONTEXT;

static NTSTATUS STDCALL
FontFamilyInfoQueryRegistryCallback(IN PWSTR ValueName, IN ULONG ValueType,
                                    IN PVOID ValueData, IN ULONG ValueLength,
                                    IN PVOID Context, IN PVOID EntryContext)
{
  PFONT_FAMILY_INFO_CALLBACK_CONTEXT InfoContext;
  UNICODE_STRING RegistryName, RegistryValue;
  int Existing;
  PFONTGDI FontGDI;

  if (REG_SZ != ValueType)
    {
      return STATUS_SUCCESS;
    }
  InfoContext = (PFONT_FAMILY_INFO_CALLBACK_CONTEXT) Context;
  RtlInitUnicodeString(&RegistryName, ValueName);

  /* Do we need to include this font family? */
  if (FontFamilyInclude(InfoContext->LogFont, &RegistryName, InfoContext->Info,
                        min(InfoContext->Count, InfoContext->Size)))
    {
      RtlInitUnicodeString(&RegistryValue, (PCWSTR) ValueData);
      Existing = FindFaceNameInInfo(&RegistryValue, InfoContext->Info,
                                    min(InfoContext->Count, InfoContext->Size));
      if (0 <= Existing)
        {
          /* We already have the information about the "real" font. Just copy it */
          if (InfoContext->Count < InfoContext->Size)
            {
              InfoContext->Info[InfoContext->Count] = InfoContext->Info[Existing];
              wcsncpy(InfoContext->Info[InfoContext->Count].EnumLogFontEx.elfLogFont.lfFaceName,
                      RegistryName.Buffer, LF_FACESIZE);
            }
          InfoContext->Count++;
          return STATUS_SUCCESS;
        }

      /* Try to find information about the "real" font */
      FontGDI = FindFaceNameInLists(&RegistryValue);
      if (NULL == FontGDI)
        {
          /* "Real" font not found, discard this registry entry */
          return STATUS_SUCCESS;
        }

      /* Return info about the "real" font but with the name of the alias */
      if (InfoContext->Count < InfoContext->Size)
        {
          FontFamilyFillInfo(InfoContext->Info + InfoContext->Count,
                             RegistryName.Buffer, FontGDI);
        }
      InfoContext->Count++;
      return STATUS_SUCCESS;
    }

  return STATUS_SUCCESS;
}

static BOOLEAN FASTCALL
GetFontFamilyInfoForSubstitutes(LPLOGFONTW LogFont,
                                PFONTFAMILYINFO Info,
                                DWORD *Count,
                                DWORD Size)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  FONT_FAMILY_INFO_CALLBACK_CONTEXT Context;
  NTSTATUS Status;

  /* Enumerate font families found in HKLM\Software\Microsoft\Windows NT\CurrentVersion\SysFontSubstitutes
     The real work is done in the registry callback function */
  Context.LogFont = LogFont;
  Context.Info = Info;
  Context.Count = *Count;
  Context.Size = Size;

  QueryTable[0].QueryRoutine = FontFamilyInfoQueryRegistryCallback;
  QueryTable[0].Flags = 0;
  QueryTable[0].Name = NULL;
  QueryTable[0].EntryContext = NULL;
  QueryTable[0].DefaultType = REG_NONE;
  QueryTable[0].DefaultData = NULL;
  QueryTable[0].DefaultLength = 0;

  QueryTable[1].QueryRoutine = NULL;
  QueryTable[1].Name = NULL;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                  L"SysFontSubstitutes",
                                  QueryTable,
                                  &Context,
                                  NULL);
  if (NT_SUCCESS(Status))
    {
      *Count = Context.Count;
    }

  return NT_SUCCESS(Status) || STATUS_OBJECT_NAME_NOT_FOUND == Status;
}

int STDCALL
NtGdiGetFontFamilyInfo(HDC Dc,
                       LPLOGFONTW UnsafeLogFont,
                       PFONTFAMILYINFO UnsafeInfo,
                       DWORD Size)
{
  NTSTATUS Status;
  LOGFONTW LogFont;
  PFONTFAMILYINFO Info;
  DWORD Count;
  PW32PROCESS Win32Process;

  /* Make a safe copy */
  Status = MmCopyFromCaller(&LogFont, UnsafeLogFont, sizeof(LOGFONTW));
  if (! NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return -1;
    }

  /* Allocate space for a safe copy */
  Info = ExAllocatePoolWithTag(PagedPool, Size * sizeof(FONTFAMILYINFO), TAG_GDITEXT);
  if (NULL == Info)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return -1;
    }

  /* Enumerate font families in the global list */
  IntLockGlobalFonts;
  Count = 0;
  if (! GetFontFamilyInfoForList(&LogFont, Info, &Count, Size, &FontListHead) )
    {
      IntUnLockGlobalFonts;
      ExFreePool(Info);
      return -1;
    }
  IntUnLockGlobalFonts;

  /* Enumerate font families in the process local list */
  Win32Process = PsGetWin32Process();
  IntLockProcessPrivateFonts(Win32Process);
  if (! GetFontFamilyInfoForList(&LogFont, Info, &Count, Size, 
                                 &Win32Process->PrivateFontListHead))
    {
      IntUnLockProcessPrivateFonts(Win32Process);
      ExFreePool(Info);
      return -1;
    }
  IntUnLockProcessPrivateFonts(Win32Process);

  /* Enumerate font families in the registry */
  if (! GetFontFamilyInfoForSubstitutes(&LogFont, Info, &Count, Size))
    {
      ExFreePool(Info);
      return -1;
    }

  /* Return data to caller */
  if (0 != Count)
    {
      Status = MmCopyToCaller(UnsafeInfo, Info,
                              (Count < Size ? Count : Size) * sizeof(FONTFAMILYINFO));
      if (! NT_SUCCESS(Status))
        {
          ExFreePool(Info);
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return -1;
        }
    }

  ExFreePool(Info);

  return Count;
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
NtGdiExtTextOut(
   HDC hDC,
   INT XStart,
   INT YStart,
   UINT fuOptions,
   CONST RECT *lprc,
   LPCWSTR String,
   UINT Count,
   CONST INT *UnsafeDx)
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
   PGDIBRUSHOBJ BrushFg = NULL;
   HBRUSH hBrushBg = NULL;
   PGDIBRUSHOBJ BrushBg = NULL;
   HBITMAP HSourceGlyph;
   SURFOBJ *SourceGlyphSurf;
   SIZEL bitSize;
   FT_CharMap found = 0, charmap;
   INT yoff;
   FONTOBJ *FontObj;
   PFONTGDI FontGDI;
   PTEXTOBJ TextObj;
   PPALGDI PalDestGDI;
   XLATEOBJ *XlateObj, *XlateObj2;
   ULONG Mode;
   FT_Render_Mode RenderMode;
   BOOLEAN Render;
   NTSTATUS Status;
   INT *Dx = NULL;;

   dc = DC_LockDc(hDC);
   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (NULL != UnsafeDx)
   {
      Dx = ExAllocatePoolWithTag(PagedPool, Count * sizeof(INT), TAG_GDITEXT);
      if (NULL == Dx)
      {
         goto fail;
      }
      Status = MmCopyFromCaller(Dx, UnsafeDx, Count * sizeof(INT));
      if (! NT_SUCCESS(Status))
      {
         goto fail;
      }
   }
 
   SurfObj = (SURFOBJ*)AccessUserObject((ULONG) dc->Surface);

   XStart += dc->w.DCOrgX;
   YStart += dc->w.DCOrgY;

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
      IntLockFreeType;
      error = FT_Set_Charmap(face, found);
      IntUnLockFreeType;
      if (error)
         DPRINT1("WARNING: Could not set the charmap!\n");
   }

   Render = IntIsFontRenderingEnabled();
   if (Render)
      RenderMode = IntGetFontRenderMode(&TextObj->logfont);
   else
      RenderMode = FT_RENDER_MODE_MONO;
  
   IntLockFreeType;
   error = FT_Set_Pixel_Sizes(
      face,
      /* FIXME should set character height if neg */
      (TextObj->logfont.lfHeight < 0 ?
      - TextObj->logfont.lfHeight :
      TextObj->logfont.lfHeight),
      TextObj->logfont.lfWidth);
   IntUnLockFreeType;
   if (error)
   {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
      goto fail;
   }

   /* Create the brushes */
   PalDestGDI = PALETTE_LockPalette(dc->w.hPalette);
   if ( !PalDestGDI )
	   Mode = PAL_RGB;
   else
   {
	   Mode = PalDestGDI->Mode;
	   PALETTE_UnlockPalette(dc->w.hPalette);
   }
   XlateObj = (XLATEOBJ*)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
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
   XlateObj2 = (XLATEOBJ*)IntEngCreateXlate(PAL_RGB, Mode, NULL, dc->w.hPalette);
  
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
         &BrushBg->BrushObject,
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

   /*
    * Process the vertical alignment and determine the yoff.
    */

   if (dc->w.textAlign & TA_BASELINE)
      yoff = 0;
   else if (dc->w.textAlign & TA_BOTTOM)
      yoff = -face->size->metrics.descender >> 6;
   else /* TA_TOP */
      yoff = face->size->metrics.ascender >> 6;

   use_kerning = FT_HAS_KERNING(face);
   previous = 0;

   /*
    * Process the horizontal alignment and modify XStart accordingly.
    */

   if (dc->w.textAlign & (TA_RIGHT | TA_CENTER))
   {
      UINT TextWidth = 0;
      LPCWSTR TempText = String;
      int Start;

      /*
       * Calculate width of the text.
       */

      if (NULL != Dx)
      {
         Start = Count < 2 ? 0 : Count - 2;
         TextWidth = Count < 2 ? 0 : Dx[Count - 2];
      }
      else
      {
         Start = 0;
      }
      TempText = String + Start;

      for (i = Start; i < Count; i++)
      {
         IntLockFreeType;
         glyph_index = FT_Get_Char_Index(face, *TempText);
         error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
         IntUnLockFreeType;
      
         if (error)
         {
            DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
         }

         glyph = face->glyph;

         /* retrieve kerning distance */
         if (use_kerning && previous && glyph_index)
         {
            FT_Vector delta;
            IntLockFreeType;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            IntUnLockFreeType;
            TextWidth += delta.x >> 6;
         }

         TextWidth += glyph->advance.x >> 6;

         previous = glyph_index;
         TempText++;
      }

      previous = 0;

      if (dc->w.textAlign & TA_RIGHT)
      {
         XStart -= TextWidth;
      }
      else
      {
         XStart -= TextWidth / 2;
      }
   }

   TextLeft = XStart;
   TextTop = YStart;
   BackgroundLeft = XStart;

   /*
    * The main rendering loop.
    */

   for (i = 0; i < Count; i++)
   {
      IntLockFreeType;
      glyph_index = FT_Get_Char_Index(face, *String);
      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      IntUnLockFreeType;

      if (error)
      {
         EngDeleteXlate(XlateObj);
         EngDeleteXlate(XlateObj2);
         DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
         goto fail;
      }

      glyph = face->glyph;

      /* retrieve kerning distance and move pen position */
      if (use_kerning && previous && glyph_index && NULL == Dx)
      {
         FT_Vector delta;
         IntLockFreeType;
         FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
         IntUnLockFreeType;
         TextLeft += delta.x >> 6;
      }

      if (glyph->format == ft_glyph_format_outline)
      {
         IntLockFreeType;
         error = FT_Render_Glyph(glyph, RenderMode);
         IntUnLockFreeType;
         if (error)
         {
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
         DestRect.right = TextLeft + ((glyph->advance.x + 32) >> 6);
         DestRect.top = TextTop + yoff - ((face->size->metrics.ascender + 32) >> 6);
         DestRect.bottom = TextTop + yoff + ((32 - face->size->metrics.descender) >> 6);
         IntEngBitBlt(
            SurfObj,
            NULL,
            NULL,
            dc->CombinedClip,
            NULL,
            &DestRect,
            &SourcePoint,
            &SourcePoint,
            &BrushBg->BrushObject,
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
    
      /*
       * We should create the bitmap out of the loop at the biggest possible
       * glyph size. Then use memset with 0 to clear it and sourcerect to
       * limit the work of the transbitblt.
       */

      HSourceGlyph = EngCreateBitmap(bitSize, pitch, (glyph->bitmap.pixel_mode == ft_pixel_mode_grays) ? BMF_8BPP : BMF_1BPP, 0, glyph->bitmap.buffer);
      SourceGlyphSurf = (SURFOBJ*)AccessUserObject((ULONG) HSourceGlyph);
    
      /*
       * Use the font data as a mask to paint onto the DCs surface using a
       * brush.
       */

      IntEngMaskBlt(
         SurfObj,
         SourceGlyphSurf,
         dc->CombinedClip,
         XlateObj,
         XlateObj2,
         &DestRect,
         &SourcePoint,
         (PPOINTL)&MaskRect,
         &BrushFg->BrushObject,
         &BrushOrigin);

      EngDeleteSurface((HSURF)HSourceGlyph);

      if (NULL == Dx)
      {
        TextLeft += (glyph->advance.x + 32) >> 6;
      }
      else
      {
        TextLeft += Dx[i];
      }
      previous = glyph_index;

      String++;
   }

   EngDeleteXlate(XlateObj);
   EngDeleteXlate(XlateObj2);
   TEXTOBJ_UnlockText(dc->w.hFont);
   if (hBrushBg != NULL)
   {
      BRUSHOBJ_UnlockBrush(hBrushBg);
      NtGdiDeleteObject(hBrushBg);
   }
   BRUSHOBJ_UnlockBrush(hBrushFg);
   NtGdiDeleteObject(hBrushFg);
   if (NULL != Dx)
   {
      ExFreePool(Dx);
   }
   DC_UnlockDc(hDC);
 
   return TRUE;

fail:
   TEXTOBJ_UnlockText(dc->w.hFont);
   if (hBrushBg != NULL)
   {
      BRUSHOBJ_UnlockBrush(hBrushBg);
      NtGdiDeleteObject(hBrushBg);
   }
   if (hBrushFg != NULL)
   {
      BRUSHOBJ_UnlockBrush(hBrushFg);
      NtGdiDeleteObject(hBrushFg);
   }
   if (NULL != Dx)
   {
      ExFreePool(Dx);
   }
   DC_UnlockDc(hDC);

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

   BufferSize = (LastChar - FirstChar + 1) * sizeof(INT);
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

      IntLockFreeType;
      FT_Set_Charmap(face, found);
      IntUnLockFreeType;
   }

   IntLockFreeType;
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
      SafeBuffer[i - FirstChar] = face->glyph->advance.x >> 6;
   }
   IntUnLockFreeType;
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

      IntLockFreeType;
      error = FT_Set_Charmap(face, found);
      IntUnLockFreeType;
      if (error)
	{
	  DPRINT1("WARNING: Could not set the charmap!\n");
	}
    }

  IntLockFreeType;
  error = FT_Set_Pixel_Sizes(face,
                             /* FIXME should set character height if neg */
                             (TextObj->logfont.lfHeight < 0 ?
                              - TextObj->logfont.lfHeight :
                              TextObj->logfont.lfHeight),
                             TextObj->logfont.lfWidth);
  IntUnLockFreeType;
  if (error)
    {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
    }

  use_kerning = FT_HAS_KERNING(face);
  previous = 0;

  for (i = 0; i < Count; i++)
    {
      IntLockFreeType;
      glyph_index = FT_Get_Char_Index(face, *String);
      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      IntUnLockFreeType;
      if (error)
	{
	  DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
	}
      glyph = face->glyph;

      /* retrieve kerning distance */
      if (use_kerning && previous && glyph_index)
	{
	  FT_Vector delta;
          IntLockFreeType;
	  FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
          IntUnLockFreeType;
	  TotalWidth += delta.x >> 6;
	}

      TotalWidth += glyph->advance.x >> 6;

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

  if (NULL == tm)
  {
    SetLastWin32Error(STATUS_INVALID_PARAMETER);
    return FALSE;
  }
  
  if(!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  TextObj = TEXTOBJ_LockText(dc->w.hFont);
  if (NULL != TextObj)
  {
    Status = GetFontObjectsFromTextObj(TextObj, NULL, NULL, &FontGDI);
    if (NT_SUCCESS(Status))
    {
      Face = FontGDI->face;
      IntLockFreeType;
      Error = FT_Set_Pixel_Sizes(Face,
	                         /* FIXME should set character height if neg */
	                         (TextObj->logfont.lfHeight < 0 ?
	                          - TextObj->logfont.lfHeight :
	                          TextObj->logfont.lfHeight),
	                         TextObj->logfont.lfWidth);
      IntUnLockFreeType;
      if (0 != Error)
	{
	DPRINT1("Error in setting pixel sizes: %u\n", Error);
	Status = STATUS_UNSUCCESSFUL;
	}
      else
	{
	memcpy(&SafeTm, &FontGDI->TextMetric, sizeof(TEXTMETRICW));
        IntLockFreeType;
        pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
        IntUnLockFreeType;
        if (NULL == pOS2)
          {
            DPRINT1("Can't find OS/2 table - not TT font?\n");
            Status = STATUS_UNSUCCESSFUL;
          }
        else
          {
            SafeTm.tmAveCharWidth = (pOS2->xAvgCharWidth + 32) >> 6;
          }
  	SafeTm.tmAscent = (Face->size->metrics.ascender + 32) >> 6; // units above baseline
	SafeTm.tmDescent = (32 - Face->size->metrics.descender) >> 6; // units below baseline
	SafeTm.tmHeight = SafeTm.tmAscent + SafeTm.tmDescent;
        SafeTm.tmMaxCharWidth = (Face->size->metrics.max_advance + 32) >> 6;
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
  
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return TRUE;
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
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return GDI_ERROR;
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
  HBRUSH hBrush;

  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }

  oldColor = dc->w.textColor;
  dc->w.textColor = color;
  hBrush = dc->w.hBrush;
  DC_UnlockDc( hDC );
  NtGdiSelectObject(hDC, hBrush);
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

static UINT FASTCALL
GetFontScore(LOGFONTW *LogFont, PUNICODE_STRING FaceName, PFONTGDI FontGDI)
{
  ANSI_STRING EntryFaceNameA;
  UNICODE_STRING EntryFaceNameW;
  unsigned Size;
  OUTLINETEXTMETRICW *Otm;
  LONG WeightDiff;
  NTSTATUS Status;
  UINT Score = 1;
  
  RtlInitAnsiString(&EntryFaceNameA, FontGDI->face->family_name);
  Status = RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
  if (NT_SUCCESS(Status))
    {
      if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
          EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
          EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }
      if (0 == RtlCompareUnicodeString(FaceName, &EntryFaceNameW, TRUE))
        {
          Score += 49;
        }
      RtlFreeUnicodeString(&EntryFaceNameW);
    }

  Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
  Otm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
  if (NULL == Otm)
    {
      return Score;
    }
  IntGetOutlineTextMetrics(FontGDI, Size, Otm);

  if ((0 != LogFont->lfItalic && 0 != Otm->otmTextMetrics.tmItalic) ||
      (0 == LogFont->lfItalic && 0 == Otm->otmTextMetrics.tmItalic))
    {
      Score += 25;
    }
  if (LogFont->lfWeight < Otm->otmTextMetrics.tmWeight)
    {
      WeightDiff = Otm->otmTextMetrics.tmWeight - LogFont->lfWeight;
    }
  else
    {
      WeightDiff = LogFont->lfWeight - Otm->otmTextMetrics.tmWeight;
    }
  Score += (1000 - WeightDiff) / (1000 / 25);

  ExFreePool(Otm);

  return Score;
}

static VOID FASTCALL
FindBestFontFromList(HFONT *Font, UINT *MatchScore, LOGFONTW *LogFont,
                     PUNICODE_STRING FaceName, PLIST_ENTRY Head)
{
  PLIST_ENTRY Entry;
  PFONT_ENTRY CurrentEntry;
  PFONTGDI FontGDI;
  UINT Score;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);
      if (NULL == (FontGDI = AccessInternalObject((ULONG) CurrentEntry->hFont)))
        {
          Entry = Entry->Flink;
          continue;
        }
      Score = GetFontScore(LogFont, FaceName, FontGDI);
      if (*MatchScore < Score)
        {
          *Font = CurrentEntry->hFont;
          *MatchScore = Score;
        }
      Entry = Entry->Flink;
    }
}

static BOOLEAN FASTCALL
SubstituteFontFamilyKey(PUNICODE_STRING FaceName,
                        LPCWSTR Key)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;
  UNICODE_STRING Value;

  RtlInitUnicodeString(&Value, NULL);

  QueryTable[0].QueryRoutine = NULL;
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND |
                        RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].Name = FaceName->Buffer;
  QueryTable[0].EntryContext = &Value;
  QueryTable[0].DefaultType = REG_NONE;
  QueryTable[0].DefaultData = NULL;
  QueryTable[0].DefaultLength = 0;

  QueryTable[1].QueryRoutine = NULL;
  QueryTable[1].Name = NULL;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                  Key,
                                  QueryTable,
                                  NULL,
                                  NULL);
  if (NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(FaceName);
      *FaceName = Value;
    }

  return NT_SUCCESS(Status);
}

static void FASTCALL
SubstituteFontFamily(PUNICODE_STRING FaceName, UINT Level)
{
  if (10 < Level) /* Enough is enough */
    {
      return;
    }

  if (SubstituteFontFamilyKey(FaceName, L"SysFontSubstitutes") ||
      SubstituteFontFamilyKey(FaceName, L"FontSubstitutes"))
    {
      SubstituteFontFamily(FaceName, Level + 1);
    }
}

NTSTATUS FASTCALL
TextIntRealizeFont(HFONT FontHandle)
{
  NTSTATUS Status = STATUS_SUCCESS;
  PTEXTOBJ TextObj;
  UNICODE_STRING FaceName;
  PW32PROCESS Win32Process;
  UINT MatchScore;

  TextObj = TEXTOBJ_LockText(FontHandle);
  if (NULL == TextObj)
    {
      return STATUS_INVALID_HANDLE;
    }

  if (! RtlCreateUnicodeString(&FaceName, TextObj->logfont.lfFaceName))
    {
      TEXTOBJ_UnlockText(FontHandle);
      return STATUS_NO_MEMORY;
    }
  SubstituteFontFamily(&FaceName, 0);
  MatchScore = 0;
  TextObj->GDIFontHandle = NULL;
    
  /* First search private fonts */
  Win32Process = PsGetWin32Process();
  IntLockProcessPrivateFonts(Win32Process);
  FindBestFontFromList(&TextObj->GDIFontHandle, &MatchScore,
                       &TextObj->logfont, &FaceName,
                       &Win32Process->PrivateFontListHead);
  IntUnLockProcessPrivateFonts(Win32Process);
    
  /* Search system fonts */
  IntLockGlobalFonts;
  FindBestFontFromList(&TextObj->GDIFontHandle, &MatchScore,
                       &TextObj->logfont, &FaceName,
                       &FontListHead);
  IntUnLockGlobalFonts;

  if (NULL == TextObj->GDIFontHandle)
    {
      DPRINT1("Requested font %S not found, no fonts loaded at all\n",
              TextObj->logfont.lfFaceName);
      Status = STATUS_NOT_FOUND;
    }

  RtlFreeUnicodeString(&FaceName);
  TEXTOBJ_UnlockText(FontHandle);
  ASSERT(! NT_SUCCESS(Status) || NULL != TextObj->GDIFontHandle);

  return Status;
}

INT FASTCALL
FontGetObject(PTEXTOBJ Font, INT Count, PVOID Buffer)
{
  if (Count < sizeof(LOGFONTW))
    {
      SetLastWin32Error(ERROR_BUFFER_OVERFLOW);
      return 0;
    }

  RtlCopyMemory(Buffer, &Font->logfont, sizeof(LOGFONTW));

  return sizeof(LOGFONTW);
}

/* EOF */
