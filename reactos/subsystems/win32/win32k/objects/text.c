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
/* $Id$ */

#include <w32k.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/tttables.h>

#define NDEBUG
#include <debug.h>

FT_Library  library;

typedef struct _FONT_ENTRY {
  LIST_ENTRY ListEntry;
  FONTGDI *Font;
  UNICODE_STRING FaceName;
  BYTE NotEnum;
} FONT_ENTRY, *PFONT_ENTRY;

/* The FreeType library is not thread safe, so we have
   to serialize access to it */
static FAST_MUTEX FreeTypeLock;

static LIST_ENTRY FontListHead;
static FAST_MUTEX FontListLock;
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
  { SYMBOL_CHARSET, 42 /* CP_SYMBOL */, FS(31)},
};

VOID FASTCALL
IntLoadSystemFonts(VOID);

INT FASTCALL
IntGdiAddFontResource(PUNICODE_STRING FileName, DWORD Characteristics);

BOOL FASTCALL
InitFontSupport(VOID)
{
   ULONG ulError;

   InitializeListHead(&FontListHead);
   ExInitializeFastMutex(&FontListLock);
   ExInitializeFastMutex(&FreeTypeLock);

   ulError = FT_Init_FreeType(&library);
   if (ulError) {
      DPRINT1("FT_Init_FreeType failed with error code 0x%x\n", ulError);
      return FALSE;
   }

   IntLoadSystemFonts();

   return TRUE;
}

/*
 * IntLoadSystemFonts
 *
 * Search the system font directory and adds each font found.
 */

VOID FASTCALL
IntLoadSystemFonts(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING Directory, SearchPattern, FileName, TempString;
   IO_STATUS_BLOCK Iosb;
   HANDLE hDirectory;
   BYTE *DirInfoBuffer;
   PFILE_DIRECTORY_INFORMATION DirInfo;
   BOOL bRestartScan = TRUE;
   NTSTATUS Status;

   RtlInitUnicodeString(&Directory, L"\\SystemRoot\\media\\fonts\\");
   /* FIXME: Add support for other font types */
   RtlInitUnicodeString(&SearchPattern, L"*.ttf");

   InitializeObjectAttributes(
      &ObjectAttributes,
      &Directory,
      OBJ_CASE_INSENSITIVE,
      NULL,
      NULL);

   Status = ZwOpenFile(
      &hDirectory,
      SYNCHRONIZE | FILE_LIST_DIRECTORY,
      &ObjectAttributes,
      &Iosb,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);

   if (NT_SUCCESS(Status))
   {
      DirInfoBuffer = ExAllocatePool(PagedPool, 0x4000);
      if (DirInfoBuffer == NULL)
      {
         ZwClose(hDirectory);
         return;
      }

      FileName.Buffer = ExAllocatePool(PagedPool, MAX_PATH * sizeof(WCHAR));
      if (FileName.Buffer == NULL)
      {
         ExFreePool(DirInfoBuffer);
         ZwClose(hDirectory);
         return;
      }
      FileName.Length = 0;
      FileName.MaximumLength = MAX_PATH * sizeof(WCHAR);

      while (1)
      {
         Status = ZwQueryDirectoryFile(
            hDirectory,
            NULL,
            NULL,
            NULL,
            &Iosb,
            DirInfoBuffer,
            0x4000,
            FileDirectoryInformation,
            FALSE,
            &SearchPattern,
            bRestartScan);

         if (!NT_SUCCESS(Status) || Status == STATUS_NO_MORE_FILES)
         {
            break;
         }

         DirInfo = (PFILE_DIRECTORY_INFORMATION)DirInfoBuffer;
         while (1)
         {
            TempString.Buffer = DirInfo->FileName;
            TempString.Length =
            TempString.MaximumLength = DirInfo->FileNameLength;
            RtlCopyUnicodeString(&FileName, &Directory);
            RtlAppendUnicodeStringToString(&FileName, &TempString);
            IntGdiAddFontResource(&FileName, 0);
            if (DirInfo->NextEntryOffset == 0)
               break;
            DirInfo = (PFILE_DIRECTORY_INFORMATION)((ULONG_PTR)DirInfo + DirInfo->NextEntryOffset);
         }

         bRestartScan = FALSE;
      }

      ExFreePool(FileName.Buffer);
      ExFreePool(DirInfoBuffer);
      ZwClose(hDirectory);
   }
}

/*
 * IntGdiAddFontResource
 *
 * Adds the font resource from the specified file to the system.
 */

INT FASTCALL
IntGdiAddFontResource(PUNICODE_STRING FileName, DWORD Characteristics)
{
   FONTGDI *FontGDI;
   NTSTATUS Status;
   HANDLE FileHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PVOID Buffer = NULL;
   IO_STATUS_BLOCK Iosb;
   INT Error;
   FT_Face Face;
   ANSI_STRING AnsiFaceName;
   PFONT_ENTRY Entry;
   PSECTION_OBJECT SectionObject;
   ULONG ViewSize = 0;

   /* Open the font file */

   InitializeObjectAttributes(&ObjectAttributes, FileName, 0, NULL, NULL);
   Status = ZwOpenFile(
      &FileHandle,
      FILE_GENERIC_READ | SYNCHRONIZE,
      &ObjectAttributes,
      &Iosb,
      FILE_SHARE_READ,
      FILE_SYNCHRONOUS_IO_NONALERT);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Could not font file: %wZ\n", FileName);
      return 0;
   }

   Status = MmCreateSection((PVOID)&SectionObject, SECTION_ALL_ACCESS,
                            NULL, NULL, PAGE_READONLY,
                            0, FileHandle, NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Could not map file: %wZ\n", FileName);
      ZwClose(FileHandle);
      return 0;
   }

   ZwClose(FileHandle);

   Status = MmMapViewInSystemSpace(SectionObject, &Buffer, &ViewSize);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Could not map file: %wZ\n", FileName);
      return Status;
   }

   IntLockFreeType;
   Error = FT_New_Memory_Face(
      library,
      Buffer,
      ViewSize,
      0,
      &Face);
   IntUnLockFreeType;

   if (Error)
   {
      if (Error == FT_Err_Unknown_File_Format)
         DPRINT("Unknown font file format\n");
      else
         DPRINT("Error reading font file (error code: %u)\n", Error);
      ObDereferenceObject(SectionObject);
      return 0;
   }

   Entry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_ENTRY), TAG_FONT);
   if (!Entry)
   {
      FT_Done_Face(Face);
      ObDereferenceObject(SectionObject);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
   }

   FontGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(FONTGDI), TAG_FONTOBJ);
   if(FontGDI == NULL)
   {
      FT_Done_Face(Face);
      ObDereferenceObject(SectionObject);
      ExFreePool(Entry);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
   }

   /* FontGDI->Filename = FileName; perform strcpy */
   FontGDI->face = Face;

   /* FIXME: Complete text metrics */
   FontGDI->TextMetric.tmAscent = (Face->size->metrics.ascender + 32) >> 6; /* units above baseline */
   FontGDI->TextMetric.tmDescent = (32 - Face->size->metrics.descender) >> 6; /* units below baseline */
   FontGDI->TextMetric.tmHeight = (Face->size->metrics.ascender -
                                   Face->size->metrics.descender) >> 6;

   DPRINT("Font loaded: %s (%s)\n", Face->family_name, Face->style_name);
   DPRINT("Num glyphs: %u\n", Face->num_glyphs);

   /* Add this font resource to the font table */

   Entry->Font = FontGDI;
   Entry->NotEnum = (Characteristics & FR_NOT_ENUM);
   RtlInitAnsiString(&AnsiFaceName, (LPSTR)Face->family_name);
   RtlAnsiStringToUnicodeString(&Entry->FaceName, &AnsiFaceName, TRUE);

   if (Characteristics & FR_PRIVATE)
   {
      PW32PROCESS Win32Process = PsGetCurrentProcessWin32Process();
      IntLockProcessPrivateFonts(Win32Process);
      InsertTailList(&Win32Process->PrivateFontListHead, &Entry->ListEntry);
      IntUnLockProcessPrivateFonts(Win32Process);
   }
   else
   {
      IntLockGlobalFonts;
      InsertTailList(&FontListHead, &Entry->ListEntry);
      IntUnLockGlobalFonts;
   }

   return 1;
}

BOOL FASTCALL
IntIsFontRenderingEnabled(VOID)
{
   BOOL Ret = RenderingEnabled;
   HDC hDC;

   hDC = IntGetScreenDC();
   if (hDC)
      Ret = (NtGdiGetDeviceCaps(hDC, BITSPIXEL) > 8) && RenderingEnabled;

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
    case NONANTIALIASED_QUALITY:
      return FT_RENDER_MODE_MONO;
    case DRAFT_QUALITY:
      return FT_RENDER_MODE_LIGHT;
/*    case CLEARTYPE_QUALITY:
        return FT_RENDER_MODE_LCD; */
  }
  return FT_RENDER_MODE_NORMAL;
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

  /* Reserve for prepending '\??\' */
  SafeFileName.Length += 4 * sizeof(WCHAR);
  SafeFileName.MaximumLength += 4 * sizeof(WCHAR);

  src = SafeFileName.Buffer;
  SafeFileName.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, SafeFileName.MaximumLength, TAG_STRING);
  if(!SafeFileName.Buffer)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  /* Prepend '\??\' */
  RtlCopyMemory(SafeFileName.Buffer, L"\\??\\", 4 * sizeof(WCHAR));

  Status = MmCopyFromCaller(SafeFileName.Buffer + 4, src, SafeFileName.MaximumLength - (4 * sizeof(WCHAR)));
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
      TEXTOBJ_UnlockText(TextObj);
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
	DPRINT1("NtGdiCreateScalableFontResource - is unimplemented, have a nice day and keep going");
  return FALSE;
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

static void FASTCALL
FillTM(TEXTMETRICW *TM, FT_Face Face, TT_OS2 *pOS2, TT_HoriHeader *pHori)
{
  FT_Fixed XScale, YScale;
  int Ascent, Descent;

  XScale = Face->size->metrics.x_scale;
  YScale = Face->size->metrics.y_scale;

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

#if 0 /* This (Wine) code doesn't seem to work correctly for us */
  TM->tmAscent = (FT_MulFix(Ascent, YScale) + 32) >> 6;
  TM->tmDescent = (FT_MulFix(Descent, YScale) + 32) >> 6;
#else
  TM->tmAscent = (Face->size->metrics.ascender + 32) >> 6; /* units above baseline */
  TM->tmDescent = (32 - Face->size->metrics.descender) >> 6; /* units below baseline */
#endif
  TM->tmInternalLeading = (FT_MulFix(Ascent + Descent
                                     - Face->units_per_EM, YScale) + 32) >> 6;

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
  TM->tmMaxCharWidth = (FT_MulFix(Face->bbox.xMax - Face->bbox.xMin,
                                  XScale) + 32) >> 6;
  TM->tmWeight = pOS2->usWeightClass;
  TM->tmOverhang = 0;
  TM->tmDigitizedAspectX = 300;
  TM->tmDigitizedAspectY = 300;
  TM->tmFirstChar = pOS2->usFirstCharIndex;
  TM->tmLastChar = pOS2->usLastCharIndex;
  TM->tmDefaultChar = pOS2->usDefaultChar;
  TM->tmBreakChar = L'\0' != pOS2->usBreakChar ? pOS2->usBreakChar : ' ';
  TM->tmItalic = (Face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0;
  TM->tmUnderlined = 0; /* entry in OS2 table */
  TM->tmStruckOut = 0; /* entry in OS2 table */

  /* Yes TPMF_FIXED_PITCH is correct; braindead api */
  if (! FT_IS_FIXED_WIDTH(Face))
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

  if (FT_IS_SCALABLE(Face))
    {
      TM->tmPitchAndFamily |= TMPF_VECTOR;
    }
  if (FT_IS_SFNT(Face))
    {
      TM->tmPitchAndFamily |= TMPF_TRUETYPE;
    }

  TM->tmCharSet = DEFAULT_CHARSET;
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

  Otm->otmSize = Needed;

  FillTM(&Otm->otmTextMetrics, FontGDI->face, pOS2, pHori);

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

  IntUnLockFreeType;

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
  FONTGDI *FontGDI;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

      FontGDI = CurrentEntry->Font;
      ASSERT(FontGDI);

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
  Win32Process = PsGetCurrentProcessWin32Process();
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

  RtlZeroMemory(Info, sizeof(FONTFAMILYINFO));
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
  FONTGDI *FontGDI;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

      FontGDI = CurrentEntry->Font;
      ASSERT(FontGDI);

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
  Win32Process = PsGetCurrentProcessWin32Process();
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
  return 0;
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
   BITMAPOBJ *BitmapObj = NULL;
   int error, glyph_index, n, i;
   FT_Face face;
   FT_GlyphSlot glyph;
   LONGLONG TextLeft, RealXStart;
   ULONG TextTop, previous, BackgroundLeft;
   FT_Bool use_kerning;
   RECTL DestRect, MaskRect, SpecifiedDestRect;
   POINTL SourcePoint, BrushOrigin;
   HBRUSH hBrushFg = NULL;
   PGDIBRUSHOBJ BrushFg = NULL;
   GDIBRUSHINST BrushFgInst;
   HBRUSH hBrushBg = NULL;
   PGDIBRUSHOBJ BrushBg = NULL;
   GDIBRUSHINST BrushBgInst;
   HBITMAP HSourceGlyph;
   SURFOBJ *SourceGlyphSurf;
   SIZEL bitSize;
   FT_CharMap found = 0, charmap;
   INT yoff;
   FONTOBJ *FontObj;
   PFONTGDI FontGDI;
   PTEXTOBJ TextObj = NULL;
   PPALGDI PalDestGDI;
   XLATEOBJ *XlateObj=NULL, *XlateObj2=NULL;
   ULONG Mode;
   FT_Render_Mode RenderMode;
   BOOLEAN Render;
   NTSTATUS Status;
   INT *Dx = NULL;
   POINT Start;
   BOOL DoBreak = FALSE;

   // TODO: Write test-cases to exactly match real Windows in different
   // bad parameters (e.g. does Windows check the DC or the RECT first?).
   dc = DC_LockDc(hDC);
   if (!dc)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   if (dc->IsIC)
   {
      DC_UnlockDc(dc);
      /* Yes, Windows really returns TRUE in this case */
      return TRUE;
   }

   if (lprc && (fuOptions & (ETO_OPAQUE | ETO_CLIPPED)))
   {
      // At least one of the two flags were specified. Copy lprc. Once.
      Status = MmCopyFromCaller(&SpecifiedDestRect, lprc, sizeof(RECT));
      if (!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      IntLPtoDP(dc, (POINT *) &SpecifiedDestRect, 2);
   }

   if (NULL != UnsafeDx && Count > 0)
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

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if ( !BitmapObj )
   {
      goto fail;
   }
   SurfObj = &BitmapObj->SurfObj;
   ASSERT(SurfObj);

   Start.x = XStart; Start.y = YStart;
   IntLPtoDP(dc, &Start, 1);

   RealXStart = (Start.x + dc->w.DCOrgX) << 6;
   YStart = Start.y + dc->w.DCOrgY;

   /* Create the brushes */
   PalDestGDI = PALETTE_LockPalette(dc->w.hPalette);
   if ( !PalDestGDI )
      Mode = PAL_RGB;
   else
   {
      Mode = PalDestGDI->Mode;
      PALETTE_UnlockPalette(PalDestGDI);
   }
   XlateObj = (XLATEOBJ*)IntEngCreateXlate(Mode, PAL_RGB, dc->w.hPalette, NULL);
   if ( !XlateObj )
   {
      goto fail;
   }
   hBrushFg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.textColor), 0);
   if ( !hBrushFg )
   {
      goto fail;
   }
   BrushFg = BRUSHOBJ_LockBrush(hBrushFg);
   if ( !BrushFg )
   {
      goto fail;
   }
   IntGdiInitBrushInstance(&BrushFgInst, BrushFg, NULL);
   if ((fuOptions & ETO_OPAQUE) || dc->w.backgroundMode == OPAQUE)
   {
      hBrushBg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, dc->w.backgroundColor), 0);
      if ( !hBrushBg )
      {
         goto fail;
      }
      BrushBg = BRUSHOBJ_LockBrush(hBrushBg);
      if ( !BrushBg )
      {
         goto fail;
      }
      IntGdiInitBrushInstance(&BrushBgInst, BrushBg, NULL);
   }
   XlateObj2 = (XLATEOBJ*)IntEngCreateXlate(PAL_RGB, Mode, NULL, dc->w.hPalette);
   if ( !XlateObj2 )
   {
      goto fail;
   }

   SourcePoint.x = 0;
   SourcePoint.y = 0;
   MaskRect.left = 0;
   MaskRect.top = 0;
   BrushOrigin.x = 0;
   BrushOrigin.y = 0;

   if ((fuOptions & ETO_OPAQUE) && lprc)
   {
      DestRect.left   = SpecifiedDestRect.left   + dc->w.DCOrgX;
      DestRect.top    = SpecifiedDestRect.top    + dc->w.DCOrgY;
      DestRect.right  = SpecifiedDestRect.right  + dc->w.DCOrgX;
      DestRect.bottom = SpecifiedDestRect.bottom + dc->w.DCOrgY;
      IntLPtoDP(dc, (LPPOINT)&DestRect, 2);
      IntEngBitBlt(
         &BitmapObj->SurfObj,
         NULL,
         NULL,
         dc->CombinedClip,
         NULL,
         &DestRect,
         &SourcePoint,
         &SourcePoint,
         &BrushBgInst.BrushObject,
         &BrushOrigin,
         ROP3_TO_ROP4(PATCOPY));
      fuOptions &= ~ETO_OPAQUE;
   }
   else
   {
      if (dc->w.backgroundMode == OPAQUE)
      {
         fuOptions |= ETO_OPAQUE;
      }
   }

   TextObj = TEXTOBJ_LockText(dc->w.hFont);
   if(TextObj == NULL)
   {
      goto fail;
   }

   FontObj = TextObj->Font;
   ASSERT(FontObj);
   FontGDI = ObjToGDI(FontObj, FONT);
   ASSERT(FontGDI);

   IntLockFreeType;
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
      {
         DPRINT1("WARNING: Could not find desired charmap!\n");
      }
      error = FT_Set_Charmap(face, found);
         if (error)
      {
         DPRINT1("WARNING: Could not set the charmap!\n");
      }
   }

   Render = IntIsFontRenderingEnabled();
   if (Render)
      RenderMode = IntGetFontRenderMode(&TextObj->logfont);
   else
      RenderMode = FT_RENDER_MODE_MONO;

   error = FT_Set_Pixel_Sizes(
      face,
      TextObj->logfont.lfWidth,
      /* FIXME should set character height if neg */
      (TextObj->logfont.lfHeight < 0 ?
      - TextObj->logfont.lfHeight :
      TextObj->logfont.lfHeight == 0 ? 11 : TextObj->logfont.lfHeight));
   if (error)
   {
      DPRINT1("Error in setting pixel sizes: %u\n", error);
      IntUnLockFreeType;
      goto fail;
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
      ULONGLONG TextWidth = 0;
      LPCWSTR TempText = String;
      int Start;

      /*
       * Calculate width of the text.
       */

      if (NULL != Dx)
      {
         Start = Count < 2 ? 0 : Count - 2;
         TextWidth = Count < 2 ? 0 : (Dx[Count - 2] << 6);
      }
      else
      {
         Start = 0;
      }
      TempText = String + Start;

      for (i = Start; i < Count; i++)
      {
         glyph_index = FT_Get_Char_Index(face, *TempText);
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
            TextWidth += delta.x;
         }

         TextWidth += glyph->advance.x;

         previous = glyph_index;
         TempText++;
      }

      previous = 0;

      if (dc->w.textAlign & TA_RIGHT)
      {
         RealXStart -= TextWidth;
      }
      else
      {
         RealXStart -= TextWidth / 2;
      }
   }

   TextLeft = RealXStart;
   TextTop = YStart;
   BackgroundLeft = (RealXStart + 32) >> 6;

   /*
    * The main rendering loop.
    */

   for (i = 0; i < Count; i++)
   {
      glyph_index = FT_Get_Char_Index(face, *String);
      error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

      if (error)
      {
         DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
	 IntUnLockFreeType;
         goto fail;
      }

      glyph = face->glyph;

      /* retrieve kerning distance and move pen position */
      if (use_kerning && previous && glyph_index && NULL == Dx)
      {
         FT_Vector delta;
         FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
         TextLeft += delta.x;
      }

      if (glyph->format == ft_glyph_format_outline)
      {
         error = FT_Render_Glyph(glyph, RenderMode);
         if (error)
         {
            DPRINT1("WARNING: Failed to render glyph!\n");
            goto fail;
         }
      }

      if (fuOptions & ETO_OPAQUE)
      {
         DestRect.left = BackgroundLeft;
         DestRect.right = (TextLeft + glyph->advance.x + 32) >> 6;
         DestRect.top = TextTop + yoff - ((face->size->metrics.ascender + 32) >> 6);
         DestRect.bottom = TextTop + yoff + ((32 - face->size->metrics.descender) >> 6);
         IntEngBitBlt(
            &BitmapObj->SurfObj,
            NULL,
            NULL,
            dc->CombinedClip,
            NULL,
            &DestRect,
            &SourcePoint,
            &SourcePoint,
            &BrushBgInst.BrushObject,
            &BrushOrigin,
            ROP3_TO_ROP4(PATCOPY));
         BackgroundLeft = DestRect.right;
      }

      DestRect.left = ((TextLeft + 32) >> 6) + glyph->bitmap_left;
      DestRect.right = DestRect.left + glyph->bitmap.width;
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
       *
       * FIXME: DIB bitmaps should have an lDelta which is a multiple of 4.
       * Here we pass in the pitch from the FreeType bitmap, which is not
       * guaranteed to be a multiple of 4. If it's not, we should expand
       * the FreeType bitmap to a temporary bitmap.
       */

      HSourceGlyph = EngCreateBitmap(bitSize, glyph->bitmap.pitch,
                                     (glyph->bitmap.pixel_mode == ft_pixel_mode_grays) ?
                                     BMF_8BPP : BMF_1BPP, BMF_TOPDOWN,
                                     glyph->bitmap.buffer);
      if ( !HSourceGlyph )
      {
        DPRINT1("WARNING: EngLockSurface() failed!\n");
	IntUnLockFreeType;
        goto fail;
      }
      SourceGlyphSurf = EngLockSurface((HSURF)HSourceGlyph);
      if ( !SourceGlyphSurf )
      {
        EngDeleteSurface((HSURF)HSourceGlyph);
        DPRINT1("WARNING: EngLockSurface() failed!\n");
	IntUnLockFreeType;
        goto fail;
      }

      /*
       * Use the font data as a mask to paint onto the DCs surface using a
       * brush.
       */

      if (lprc &&
          (fuOptions & ETO_CLIPPED) &&
          DestRect.right >= SpecifiedDestRect.right + dc->w.DCOrgX)
      {
         // We do the check '>=' instead of '>' to possibly save an iteration
         // through this loop, since it's breaking after the drawing is done,
         // and x is always incremented.
         DestRect.right = SpecifiedDestRect.right + dc->w.DCOrgX;
         DoBreak = TRUE;
      }

      IntEngMaskBlt(
         SurfObj,
         SourceGlyphSurf,
         dc->CombinedClip,
         XlateObj,
         XlateObj2,
         &DestRect,
         &SourcePoint,
         (PPOINTL)&MaskRect,
         &BrushFgInst.BrushObject,
         &BrushOrigin);

      EngUnlockSurface(SourceGlyphSurf);
      EngDeleteSurface((HSURF)HSourceGlyph);

      if (DoBreak)
      {
         break;
      }

      if (NULL == Dx)
      {
         TextLeft += glyph->advance.x;
      }
      else
      {
         TextLeft += Dx[i] << 6;
      }
      previous = glyph_index;

      String++;
   }

   IntUnLockFreeType;

   EngDeleteXlate(XlateObj);
   EngDeleteXlate(XlateObj2);
   BITMAPOBJ_UnlockBitmap(BitmapObj);
   if(TextObj != NULL)
     TEXTOBJ_UnlockText(TextObj);
   if (hBrushBg != NULL)
   {
      BRUSHOBJ_UnlockBrush(BrushBg);
      NtGdiDeleteObject(hBrushBg);
   }
   BRUSHOBJ_UnlockBrush(BrushFg);
   NtGdiDeleteObject(hBrushFg);
   if (NULL != Dx)
   {
      ExFreePool(Dx);
   }
   DC_UnlockDc( dc );

   return TRUE;

fail:
   if ( XlateObj2 != NULL )
      EngDeleteXlate(XlateObj2);
   if ( XlateObj != NULL )
      EngDeleteXlate(XlateObj);
   if(TextObj != NULL)
     TEXTOBJ_UnlockText(TextObj);
   BITMAPOBJ_UnlockBitmap(BitmapObj);
   if (hBrushBg != NULL)
   {
      BRUSHOBJ_UnlockBrush(BrushBg);
      NtGdiDeleteObject(hBrushBg);
   }
   if (hBrushFg != NULL)
   {
      BRUSHOBJ_UnlockBrush(BrushFg);
      NtGdiDeleteObject(hBrushFg);
   }
   if (NULL != Dx)
   {
      ExFreePool(Dx);
   }
   DC_UnlockDc(dc);

   return FALSE;
}

BOOL
STDCALL
NtGdiGetAspectRatioFilterEx(HDC  hDC,
                                 LPSIZE  AspectRatio)
{
  UNIMPLEMENTED;
  return FALSE;
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
  return FALSE;
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
  return 0;
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
   HFONT hFont = 0;

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
   hFont = dc->w.hFont;
   TextObj = TEXTOBJ_LockText(hFont);
   DC_UnlockDc(dc);

   if (TextObj == NULL)
   {
      ExFreePool(SafeBuffer);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   FontGDI = ObjToGDI(TextObj->Font, FONT);

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
                      TextObj->logfont.lfWidth,
                      /* FIXME should set character height if neg */
                      (TextObj->logfont.lfHeight < 0 ?
                       - TextObj->logfont.lfHeight :
                       TextObj->logfont.lfHeight == 0 ? 11 : TextObj->logfont.lfHeight));

   for (i = FirstChar; i <= LastChar; i++)
   {
      glyph_index = FT_Get_Char_Index(face, i);
      FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
      SafeBuffer[i - FirstChar] = (face->glyph->advance.x + 32) >> 6;
   }
   IntUnLockFreeType;
   TEXTOBJ_UnlockText(TextObj);
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
  return FALSE;
}

DWORD
STDCALL
NtGdiGetFontLanguageInfo(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

ULONG
APIENTRY
NtGdiGetGlyphOutline(
    IN HDC hdc,
    IN WCHAR wch,
    IN UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID pvBuf,
    IN LPMAT2 pmat2,
    IN BOOL bIgnoreRotation)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
STDCALL
NtGdiGetKerningPairs(HDC  hDC,
                           DWORD  NumPairs,
                           LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetOutlineTextMetrics(HDC  hDC,
                                UINT  Data,
                                LPOUTLINETEXTMETRICW  otm)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    OUT LPRASTERIZER_STATUS praststat,
    IN ULONG cjBytes)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 Based on "Undocumented W2k Secrets", Table B-2, page 473.
 This function does not exist. See note in gdi32/objects/text.c
 GetTextCharset. This should be moved to include/win32k/ntgdibad.h.
*/
UINT
STDCALL
NtGdiGetTextCharset(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

INT
APIENTRY
NtGdiGetTextCharsetInfo(
    IN HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    IN DWORD dwFlags)
{
  PDC Dc;
  UINT Ret = DEFAULT_CHARSET, i = 0, fs_fsCsb0 = 0;
  HFONT hFont;
  PTEXTOBJ TextObj;
  PFONTGDI FontGdi;
  FONTSIGNATURE fs;
  TT_OS2 *pOS2;
  FT_Face Face;
  NTSTATUS Status;
  
  Dc = DC_LockDc(hdc);
  if (!Dc)
    {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return Ret;
    }
  hFont = Dc->w.hFont;
  TextObj = TEXTOBJ_LockText(hFont);
  DC_UnlockDc( Dc );
  if ( TextObj == NULL)
    {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return Ret;
    }
  FontGdi = ObjToGDI(TextObj->Font, FONT);
  Face = FontGdi->face;
  TEXTOBJ_UnlockText(TextObj);
  IntLockFreeType;
  pOS2 = FT_Get_Sfnt_Table(Face, ft_sfnt_os2);
  IntUnLockFreeType;
  memset(&fs, 0, sizeof(FONTSIGNATURE));
  if (NULL != pOS2)
    {
      fs.fsCsb[0] = pOS2->ulCodePageRange1;
      fs.fsCsb[1] = pOS2->ulCodePageRange2;
      fs.fsUsb[0] = pOS2->ulUnicodeRange1;
      fs.fsUsb[1] = pOS2->ulUnicodeRange2;
      fs.fsUsb[2] = pOS2->ulUnicodeRange3;
      fs.fsUsb[3] = pOS2->ulUnicodeRange4;
      fs_fsCsb0   = pOS2->ulCodePageRange1;
      if (pOS2->version == 0) 
        {
          FT_UInt dummy;

          if(FT_Get_First_Char( Face, &dummy ) < 0x100)
                fs_fsCsb0 |= 1;
          else
                fs_fsCsb0 |= 1L << 31;
        }
    }
  DPRINT("Csb 1=%x  0=%x\n", fs.fsCsb[1],fs.fsCsb[0]);
  if (lpSig)
    {
      Status = MmCopyToCaller(lpSig, &fs,  sizeof(FONTSIGNATURE));
      if (! NT_SUCCESS(Status))
        {
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          return Ret;
        }
    }
  if (0 == fs_fsCsb0)
    { /* let's see if we can find any interesting cmaps */
       for (i = 0; i < Face->num_charmaps; i++)
          {
              switch (Face->charmaps[i]->encoding)
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
  while (0 == (fs_fsCsb0 >> i & 0x0001) && i < MAXTCIINDEX)
       {
          i++;
       }
  Ret = FontTci[i].ciCharset;
  DPRINT("CharSet %d\n",Ret);
  return Ret;
}

static BOOL
FASTCALL
TextIntGetTextExtentPoint(PDC dc,
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
  ULONGLONG TotalWidth = 0;
  FT_CharMap charmap, found = NULL;
  BOOL use_kerning;

  FontGDI = ObjToGDI(TextObj->Font, FONT);

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
                             TextObj->logfont.lfWidth,
                             /* FIXME should set character height if neg */
                             (TextObj->logfont.lfHeight < 0 ?
                              - TextObj->logfont.lfHeight :
                              TextObj->logfont.lfHeight == 0 ? 11 : TextObj->logfont.lfHeight));
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
	  TotalWidth += delta.x;
	}

      TotalWidth += glyph->advance.x;

      if (((TotalWidth + 32) >> 6) <= MaxExtent && NULL != Fit)
	{
	  *Fit = i + 1;
	}
      if (NULL != Dx)
	{
	  Dx[i] = (TotalWidth + 32) >> 6;
	}

      previous = glyph_index;
      String++;
    }

  Size->cx = (TotalWidth + 32) >> 6;
  Size->cy = (TextObj->logfont.lfHeight < 0 ? - TextObj->logfont.lfHeight : TextObj->logfont.lfHeight);
  Size->cy = EngMulDiv(Size->cy, IntGdiGetDeviceCaps(dc, LOGPIXELSY), 72);

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
  if ( TextObj )
  {
    Result = TextIntGetTextExtentPoint(dc, TextObj, String, Count, MaxExtent,
                                     NULL == UnsafeFit ? NULL : &Fit, Dx, &Size);
  }
  else
    Result = FALSE;
  TEXTOBJ_UnlockText(TextObj);
  DC_UnlockDc(dc);

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
NtGdiGetTextExtent(HDC hdc,
                   LPWSTR lpwsz,
                   INT cwc,
                   LPSIZE psize,
                   UINT flOpts)
{
  return NtGdiGetTextExtentExPoint(hdc, lpwsz, cwc, 0, NULL, NULL, psize);
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
  if ( TextObj != NULL )
  {
    Result = TextIntGetTextExtentPoint (
      dc, TextObj, String, Count, 0, NULL, NULL, &Size);
    TEXTOBJ_UnlockText(TextObj);
  }
  else
    Result = FALSE;
  DC_UnlockDc(dc);

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

INT STDCALL
NtGdiGetTextFace(HDC hDC, INT Count, LPWSTR FaceName)
{
   PDC Dc;
   HFONT hFont;
   PTEXTOBJ TextObj;
   NTSTATUS Status;

   Dc = DC_LockDc(hDC);
   if (Dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   hFont = Dc->w.hFont;
   DC_UnlockDc(Dc);

   TextObj = TEXTOBJ_LockText(hFont);
   ASSERT(TextObj != NULL);
   Count = min(Count, wcslen(TextObj->logfont.lfFaceName));
   Status = MmCopyToCaller(FaceName, TextObj->logfont.lfFaceName, Count * sizeof(WCHAR));
   TEXTOBJ_UnlockText(TextObj);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }

   return Count;
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
  TT_HoriHeader *pHori;
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
      FontGDI = ObjToGDI(TextObj->Font, FONT);

      Face = FontGDI->face;
      IntLockFreeType;
      Error = FT_Set_Pixel_Sizes(Face,
	                         TextObj->logfont.lfWidth,
	                         /* FIXME should set character height if neg */
                                 (TextObj->logfont.lfHeight < 0 ?
                                  - TextObj->logfont.lfHeight :
                                  TextObj->logfont.lfHeight == 0 ? 11 : TextObj->logfont.lfHeight));
      IntUnLockFreeType;
      if (0 != Error)
	{
          DPRINT1("Error in setting pixel sizes: %u\n", Error);
          Status = STATUS_UNSUCCESSFUL;
	}
      else
	{
          memcpy(&SafeTm, &FontGDI->TextMetric, sizeof(TEXTMETRICW));

          Status = STATUS_SUCCESS;
          IntLockFreeType;
          pOS2 = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_os2);
          if (NULL == pOS2)
            {
              DPRINT1("Can't find OS/2 table - not TT font?\n");
              Status = STATUS_INTERNAL_ERROR;
            }

          pHori = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_hhea);
          if (NULL == pHori)
            {
              DPRINT1("Can't find HHEA table - not TT font?\n");
              Status = STATUS_INTERNAL_ERROR;
            }

          IntUnLockFreeType;

          if (NT_SUCCESS(Status))
            {
              FillTM(&SafeTm, FontGDI->face, pOS2, pHori);
              Status = MmCopyToCaller(tm, &SafeTm, sizeof(TEXTMETRICW));
            }
	}
      TEXTOBJ_UnlockText(TextObj);
    }
  else
    {
      Status = STATUS_INVALID_HANDLE;
    }
  DC_UnlockDc(dc);

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
  return FALSE;
}

BOOL
STDCALL
NtGdiRemoveFontResource(LPCWSTR  FileName)
{
  DPRINT1("NtGdiRemoveFontResource is UNIMPLEMENTED\n");
  return FALSE;
}

DWORD
STDCALL
NtGdiSetMapperFlags(HDC  hDC,
                          DWORD  Flag)
{
  UNIMPLEMENTED;
  return 0;
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
  DC_UnlockDc( dc );
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
  DC_UnlockDc( dc );
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
  return FALSE;
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

DWORD STDCALL
NtGdiGetFontData(
   HDC hDC,
   DWORD Table,
   DWORD Offset,
   LPVOID Buffer,
   DWORD Size)
{
   PDC Dc;
   HFONT hFont;
   PTEXTOBJ TextObj;
   PFONTGDI FontGdi;
   DWORD Result = GDI_ERROR;

   Dc = DC_LockDc(hDC);
   if (Dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return GDI_ERROR;
   }
   hFont = Dc->w.hFont;
   TextObj = TEXTOBJ_LockText(hFont);
   DC_UnlockDc(Dc);

   if (TextObj == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return GDI_ERROR;
   }

   FontGdi = ObjToGDI(TextObj->Font, FONT);

   IntLockFreeType;

   if (FT_IS_SFNT(FontGdi->face))
   {
       if (Table)
          Table = Table >> 24 | Table << 24 | (Table >> 8 & 0xFF00) |
                  (Table << 8 & 0xFF0000);

       if (Buffer == NULL)
          Size = 0;

       if (!FT_Load_Sfnt_Table(FontGdi->face, Table, Offset, Buffer, &Size))
          Result = Size;
   }

   IntUnLockFreeType;

   TEXTOBJ_UnlockText(TextObj);

   return Result;
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

static __inline VOID
FindBestFontFromList(FONTOBJ **FontObj, UINT *MatchScore, LOGFONTW *LogFont,
                     PUNICODE_STRING FaceName, PLIST_ENTRY Head)
{
  PLIST_ENTRY Entry;
  PFONT_ENTRY CurrentEntry;
  FONTGDI *FontGDI;
  UINT Score;

  Entry = Head->Flink;
  while (Entry != Head)
    {
      CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

      FontGDI = CurrentEntry->Font;
      ASSERT(FontGDI);

      Score = GetFontScore(LogFont, FaceName, FontGDI);
      if (*MatchScore == 0 || *MatchScore < Score)
        {
          *FontObj = GDIToObj(FontGDI, FONT);
          *MatchScore = Score;
        }
      Entry = Entry->Flink;
    }
}

static __inline BOOLEAN
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

static __inline void
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

  if (TextObj->Initialized)
    {
      TEXTOBJ_UnlockText(TextObj);
      return STATUS_SUCCESS;
    }

  if (! RtlCreateUnicodeString(&FaceName, TextObj->logfont.lfFaceName))
    {
      TEXTOBJ_UnlockText(TextObj);
      return STATUS_NO_MEMORY;
    }
  SubstituteFontFamily(&FaceName, 0);
  MatchScore = 0;
  TextObj->Font = NULL;

  /* First search private fonts */
  Win32Process = PsGetCurrentProcessWin32Process();
  IntLockProcessPrivateFonts(Win32Process);
  FindBestFontFromList(&TextObj->Font, &MatchScore,
                       &TextObj->logfont, &FaceName,
                       &Win32Process->PrivateFontListHead);
  IntUnLockProcessPrivateFonts(Win32Process);

  /* Search system fonts */
  IntLockGlobalFonts;
  FindBestFontFromList(&TextObj->Font, &MatchScore,
                       &TextObj->logfont, &FaceName,
                       &FontListHead);
  IntUnLockGlobalFonts;

  if (NULL == TextObj->Font)
    {
      DPRINT1("Requested font %S not found, no fonts loaded at all\n",
              TextObj->logfont.lfFaceName);
      Status = STATUS_NOT_FOUND;
    }
  else
    {
      TextObj->Initialized = TRUE;
      Status = STATUS_SUCCESS;
    }

  RtlFreeUnicodeString(&FaceName);
  TEXTOBJ_UnlockText(TextObj);

  ASSERT((NT_SUCCESS(Status) ^ (NULL == TextObj->Font)) != 0);

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
