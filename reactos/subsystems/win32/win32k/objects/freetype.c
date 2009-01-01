/*
 * FreeType font engine interface
 *
 * Copyright 2001 Huw D M Davies for CodeWeavers.
 * Copyright 2006 Dmitry Timoshkov for CodeWeavers.
 *
 * This file contains the WineEng* functions.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
/*
 *
 * Addaped for the use in ReactOS.
 *
 */
/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/freetype.c
 * PURPOSE:         Freetype library support
 * PROGRAMMER:
 */

/** Includes ******************************************************************/

#include <w32k.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TYPE1_TABLES_H
#include <freetype/tttables.h>
#include <freetype/fttrigon.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/ftwinfnt.h>

#define NDEBUG
#include <debug.h>

#ifndef FT_MAKE_TAG
#define FT_MAKE_TAG( ch0, ch1, ch2, ch3 ) \
       ( ((DWORD)(BYTE)(ch0) << 24) | ((DWORD)(BYTE)(ch1) << 16) | \
       ((DWORD)(BYTE)(ch2) << 8) | (DWORD)(BYTE)(ch3) )
#endif

FT_Library  library;

typedef struct _FONT_ENTRY
{
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

#define MAX_FONT_CACHE 256
UINT Hits;
UINT Misses;

typedef struct _FONT_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    int GlyphIndex;
    FT_Face Face;
    FT_Glyph Glyph;
    int Height;
} FONT_CACHE_ENTRY, *PFONT_CACHE_ENTRY;
static LIST_ENTRY FontCacheListHead;
static UINT FontCacheNumEntries;

static PWCHAR ElfScripts[32] =   /* these are in the order of the fsCsb[0] bits */
{
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
 *  For TranslateCharsetInfo
 */
#define CP_SYMBOL   42
#define MAXTCIINDEX 32
static const CHARSETINFO FontTci[MAXTCIINDEX] =
{
    /* ANSI */
    { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
    { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
    { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
    { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
    { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
    { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
    { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
    { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
    { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
    /* reserved by ANSI */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* ANSI and OEM */
    { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
    { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
    { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
    { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
    { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
    { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
    /* reserved for alternate ANSI and OEM */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* reserved for system */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

BOOL FASTCALL
InitFontSupport(VOID)
{
    ULONG ulError;

    InitializeListHead(&FontListHead);
    InitializeListHead(&FontCacheListHead);
    FontCacheNumEntries = 0;
    ExInitializeFastMutex(&FontListLock);
    ExInitializeFastMutex(&FreeTypeLock);

    ulError = FT_Init_FreeType(&library);
    if (ulError)
    {
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
    BOOLEAN bRestartScan = TRUE;
    NTSTATUS Status;

    RtlInitUnicodeString(&Directory, L"\\SystemRoot\\Fonts\\");
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
    HANDLE FileHandle, KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID Buffer = NULL;
    IO_STATUS_BLOCK Iosb;
    INT Error;
    FT_Face Face;
    ANSI_STRING AnsiFaceName;
    PFONT_ENTRY Entry;
    PSECTION_OBJECT SectionObject;
    ULONG ViewSize = 0;
    LARGE_INTEGER SectionSize;
    FT_Fixed XScale, YScale;
    UNICODE_STRING FontRegPath = RTL_CONSTANT_STRING(L"\\REGISTRY\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");

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
        DPRINT("Could not load font file: %wZ\n", FileName);
        return 0;
    }

    SectionSize.QuadPart = 0LL;
    Status = MmCreateSection((PVOID)&SectionObject, SECTION_ALL_ACCESS,
                             NULL, &SectionSize, PAGE_READONLY,
                             SEC_COMMIT, FileHandle, NULL);
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
    if (FontGDI == NULL)
    {
        FT_Done_Face(Face);
        ObDereferenceObject(SectionObject);
        ExFreePoolWithTag(Entry, TAG_FONT);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    FontGDI->Filename = ExAllocatePoolWithTag(PagedPool, FileName->Length + sizeof(WCHAR), TAG_PFF);
    if (FontGDI->Filename == NULL)
    {
        EngFreeMem(FontGDI);
        FT_Done_Face(Face);
        ObDereferenceObject(SectionObject);
        ExFreePoolWithTag(Entry, TAG_FONT);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    RtlCopyMemory(FontGDI->Filename, FileName->Buffer, FileName->Length);
    FontGDI->Filename[FileName->Length / sizeof(WCHAR)] = L'\0';
    FontGDI->face = Face;

    /* FIXME: Complete text metrics */
    XScale = Face->size->metrics.x_scale;
    YScale = Face->size->metrics.y_scale;
#if 0 /* This (Wine) code doesn't seem to work correctly for us */
    FontGDI->TextMetric.tmAscent =  (FT_MulFix(Face->ascender, YScale) + 32) >> 6;
    FontGDI->TextMetric.tmDescent = (FT_MulFix(Face->descender, YScale) + 32) >> 6;
    FontGDI->TextMetric.tmHeight =  (FT_MulFix(Face->ascender, YScale) -
                                     FT_MulFix(Face->descender, YScale)) >> 6;
#else
    FontGDI->TextMetric.tmAscent  = (Face->size->metrics.ascender + 32) >> 6; /* units above baseline */
    FontGDI->TextMetric.tmDescent = (32 - Face->size->metrics.descender) >> 6; /* units below baseline */
    FontGDI->TextMetric.tmHeight = (Face->size->metrics.ascender - Face->size->metrics.descender) >> 6;
#endif



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
        InitializeObjectAttributes(&ObjectAttributes, &FontRegPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = ZwOpenKey(&KeyHandle, KEY_WRITE, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            LPWSTR pName = wcsrchr(FileName->Buffer, L'\\');
            if (pName)
            {
                pName++;
                ZwSetValueKey(KeyHandle, &Entry->FaceName, 0, REG_SZ, pName, (wcslen(pName) + 1) * sizeof(WCHAR));
            }
            ZwClose(KeyHandle);
        }
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
    switch (logfont->lfQuality)
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


NTSTATUS FASTCALL
TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont)
{
    PTEXTOBJ TextObj;

    TextObj = TEXTOBJ_AllocTextWithHandle();
    if (!TextObj)
    {
        return STATUS_NO_MEMORY;
    }

    *NewFont = TextObj->BaseObject.hHmgr;
    RtlCopyMemory(&TextObj->logfont.elfEnumLogfontEx.elfLogFont, lf, sizeof(LOGFONTW));
    if (lf->lfEscapement != lf->lfOrientation)
    {
        /* this should really depend on whether GM_ADVANCED is set */
        TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation =
            TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfEscapement;
    }
    TEXTOBJ_UnlockText(TextObj);

    return STATUS_SUCCESS;
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
static BOOLEAN APIENTRY
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
        while ( *Src != FontTci[Index].ciACP && Index < MAXTCIINDEX)
        {
            Index++;
        }
        break;
    case TCI_SRCCHARSET:
        while ( *Src != FontTci[Index].ciCharset && Index < MAXTCIINDEX)
        {
            Index++;
        }
        break;
    default:
        return FALSE;
    }

    if (Index >= MAXTCIINDEX || DEFAULT_CHARSET == FontTci[Index].ciCharset)
    {
        return FALSE;
    }

    RtlCopyMemory(Cs, &FontTci[Index], sizeof(CHARSETINFO));

    return TRUE;
}


static void FASTCALL
FillTM(TEXTMETRICW *TM, PFONTGDI FontGDI, TT_OS2 *pOS2, TT_HoriHeader *pHori, FT_WinFNT_HeaderRec *pWin)
{
    FT_Fixed XScale, YScale;
    int Ascent, Descent;
    FT_Face Face = FontGDI->face;

    XScale = Face->size->metrics.x_scale;
    YScale = Face->size->metrics.y_scale;

    if (pWin)
    {
        TM->tmHeight           = pWin->pixel_height;
        TM->tmAscent           = pWin->ascent;
        TM->tmDescent          = TM->tmHeight - TM->tmAscent;
        TM->tmInternalLeading  = pWin->internal_leading;
        TM->tmExternalLeading  = pWin->external_leading;
        TM->tmAveCharWidth     = pWin->avg_width;
        TM->tmMaxCharWidth     = pWin->max_width;
        TM->tmWeight           = pWin->weight;
        TM->tmOverhang         = 0;
        TM->tmDigitizedAspectX = pWin->horizontal_resolution;
        TM->tmDigitizedAspectY = pWin->vertical_resolution;
        TM->tmFirstChar        = pWin->first_char;
        TM->tmLastChar         = pWin->last_char;
        TM->tmDefaultChar      = pWin->default_char + pWin->first_char;
        TM->tmBreakChar        = pWin->break_char + pWin->first_char;
        TM->tmItalic           = pWin->italic;
        TM->tmUnderlined       = FontGDI->Underline;
        TM->tmStruckOut        = FontGDI->StrikeOut;
        TM->tmPitchAndFamily   = pWin->pitch_and_family;
        TM->tmCharSet          = pWin->charset;
        return;
    }

    if (0  == pOS2->usWinAscent + pOS2->usWinDescent)
    {
        Ascent = pHori->Ascender;
        Descent = -pHori->Descender;
    }
    else
    {
        Ascent = pOS2->usWinAscent;
        Descent = pOS2->usWinDescent;
    }

#if 0 /* This (Wine) code doesn't seem to work correctly for us, cmd issue */
    TM->tmAscent = (FT_MulFix(Ascent, YScale) + 32) >> 6;
    TM->tmDescent = (FT_MulFix(Descent, YScale) + 32) >> 6;
#else /* This (ros) code was previously affected by a FreeType bug, but it works now */
    TM->tmAscent = (Face->size->metrics.ascender + 32) >> 6; /* units above baseline */
    TM->tmDescent = (32 - Face->size->metrics.descender) >> 6; /* units below baseline */
#endif

    TM->tmInternalLeading = (FT_MulFix(Ascent + Descent - Face->units_per_EM, YScale) + 32) >> 6;
    TM->tmHeight = TM->tmAscent + TM->tmDescent; // we need add 1 height more after scale it right

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

    /* Correct forumla to get the maxcharwidth from unicode and ansi font */
    TM->tmMaxCharWidth = (FT_MulFix(Face->max_advance_width, XScale) + 32) >> 6;

    TM->tmWeight = pOS2->usWeightClass;
    TM->tmOverhang = 0;
    TM->tmDigitizedAspectX = 300;
    TM->tmDigitizedAspectY = 300;
    TM->tmFirstChar = pOS2->usFirstCharIndex;
    TM->tmLastChar = pOS2->usLastCharIndex;
    TM->tmDefaultChar = pOS2->usDefaultChar;
    TM->tmBreakChar = L'\0' != pOS2->usBreakChar ? pOS2->usBreakChar : ' ';
    TM->tmItalic = (Face->style_flags & FT_STYLE_FLAG_ITALIC) ? 255 : 0;
    TM->tmUnderlined = FontGDI->Underline;
    TM->tmStruckOut  = FontGDI->StrikeOut;

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
INT FASTCALL
IntGetOutlineTextMetrics(PFONTGDI FontGDI,
                         UINT Size,
                         OUTLINETEXTMETRICW *Otm)
{
    unsigned Needed;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    TT_Postscript *pPost;
    FT_Fixed XScale, YScale;
    ANSI_STRING FamilyNameA, StyleNameA;
    UNICODE_STRING FamilyNameW, StyleNameW, Regular;
    FT_WinFNT_HeaderRec Win;
    FT_Error Error;
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

    Error = FT_Get_WinFNT_Header(FontGDI->face , &Win);

    Otm->otmSize = Needed;

//  FillTM(&Otm->otmTextMetrics, FontGDI, pOS2, pHori, !Error ? &Win : 0);
    if (!(FontGDI->flRealizedType & FDM_TYPE_TEXT_METRIC))
    {
        FillTM(&FontGDI->TextMetric, FontGDI, pOS2, pHori, !Error ? &Win : 0);
        FontGDI->flRealizedType |= FDM_TYPE_TEXT_METRIC;
    }

    RtlCopyMemory(&Otm->otmTextMetrics, &FontGDI->TextMetric, sizeof(TEXTMETRICW));

    Otm->otmFiller = 0;
    RtlCopyMemory(&Otm->otmPanoseNumber, pOS2->panose, PANOSE_COUNT);
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
    CHARSETINFO CharSetInfo;
    unsigned i, Size;
    OUTLINETEXTMETRICW *Otm;
    LOGFONTW *Lf;
    TEXTMETRICW *TM;
    NEWTEXTMETRICW *Ntm;
    DWORD fs0;

    RtlZeroMemory(Info, sizeof(FONTFAMILYINFO));
    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    Otm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
    if (!Otm)
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
    Lf->lfOutPrecision = OUT_OUTLINE_PRECIS;
    Lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    Lf->lfQuality = PROOF_QUALITY;

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

    if (550 < TM->tmWeight) Ntm->ntmFlags |= NTM_BOLD;

    if (0 == Ntm->ntmFlags) Ntm->ntmFlags = NTM_REGULAR;

    Ntm->ntmSizeEM = Otm->otmEMSquare;
    Ntm->ntmCellHeight = 0;
    Ntm->ntmAvgWidth = 0;

    Info->FontType = (0 != (TM->tmPitchAndFamily & TMPF_TRUETYPE)
                      ? TRUETYPE_FONTTYPE : 0);

    if (0 == (TM->tmPitchAndFamily & TMPF_VECTOR))
        Info->FontType |= RASTER_FONTTYPE;

    ExFreePoolWithTag(Otm, TAG_GDITEXT);

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
        fs.fsCsb[0] = pOS2->ulCodePageRange1;
        fs.fsCsb[1] = pOS2->ulCodePageRange2;
        fs.fsUsb[0] = pOS2->ulUnicodeRange1;
        fs.fsUsb[1] = pOS2->ulUnicodeRange2;
        fs.fsUsb[2] = pOS2->ulUnicodeRange3;
        fs.fsUsb[3] = pOS2->ulUnicodeRange4;

        if (0 == pOS2->version)
        {
            FT_UInt Dummy;

            if (FT_Get_First_Char(FontGDI->face, &Dummy) < 0x100)
                fs.fsCsb[0] |= FS_LATIN1;
            else
                fs.fsCsb[0] |= FS_SYMBOL;
        }
        if (fs.fsCsb[0] == 0)
        { /* let's see if we can find any interesting cmaps */
            for (i = 0; i < FontGDI->face->num_charmaps; i++)
            {
                switch (FontGDI->face->charmaps[i]->encoding)
                {
                case FT_ENCODING_UNICODE:
                case FT_ENCODING_APPLE_ROMAN:
                    fs.fsCsb[0] |= FS_LATIN1;
                    break;
                case FT_ENCODING_MS_SYMBOL:
                    fs.fsCsb[0] |= FS_SYMBOL;
                    break;
                default:
                    break;
                }
            }
        }
        for (i = 0; i < MAXTCIINDEX; i++)
        {
            fs0 = 1L << i;
            if (fs.fsCsb[0] & fs0)
            {
                if (!IntTranslateCharsetInfo(&fs0, &CharSetInfo, TCI_SRCFONTSIG))
                {
                    CharSetInfo.ciCharset = DEFAULT_CHARSET;
                }
                if (DEFAULT_CHARSET != CharSetInfo.ciCharset)
                {
                    Info->EnumLogFontEx.elfLogFont.lfCharSet = CharSetInfo.ciCharset;
                    if (NULL != ElfScripts[i])
                        wcscpy(Info->EnumLogFontEx.elfScript, ElfScripts[i]);
                    else
                    {
                        DPRINT1("Unknown elfscript for bit %d\n", i);
                    }
                }
            }
        }
        Info->NewTextMetricEx.ntmFontSig = fs;
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

static NTSTATUS APIENTRY
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
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {{0}};
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

BOOL
FASTCALL
ftGdiGetRasterizerCaps(LPRASTERIZER_STATUS lprs)
{
    if ( lprs )
    {
        lprs->nSize = sizeof(RASTERIZER_STATUS);
        lprs->wFlags = TT_AVAILABLE | TT_ENABLED;
        lprs->nLanguageID = gusLanguageID;
        return TRUE;
    }
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
}


FT_Glyph APIENTRY
ftGdiGlyphCacheGet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height)
{
    PLIST_ENTRY CurrentEntry;
    PFONT_CACHE_ENTRY FontEntry;

//   DbgPrint("CacheGet\n");

    CurrentEntry = FontCacheListHead.Flink;
    while (CurrentEntry != &FontCacheListHead)
    {
        FontEntry = (PFONT_CACHE_ENTRY)CurrentEntry;
        if (FontEntry->Face == Face &&
                FontEntry->GlyphIndex == GlyphIndex &&
                FontEntry->Height == Height)
            break;
        CurrentEntry = CurrentEntry->Flink;
    }

    if (CurrentEntry == &FontCacheListHead)
    {
//      DbgPrint("Miss! %x\n", FontEntry->Glyph);
        /*
              Misses++;
              if (Misses>100) {
                 DbgPrint ("Hits: %d Misses: %d\n", Hits, Misses);
                 Hits = Misses = 0;
              }
        */
        return NULL;
    }

    RemoveEntryList(CurrentEntry);
    InsertHeadList(&FontCacheListHead, CurrentEntry);

//   DbgPrint("Hit! %x\n", FontEntry->Glyph);
    /*
       Hits++;

          if (Hits>100) {
             DbgPrint ("Hits: %d Misses: %d\n", Hits, Misses);
             Hits = Misses = 0;
          }
    */
    return FontEntry->Glyph;
}

FT_Glyph APIENTRY
ftGdiGlyphCacheSet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height,
    FT_GlyphSlot GlyphSlot,
    FT_Render_Mode RenderMode)
{
    FT_Glyph GlyphCopy;
    INT error;
    PFONT_CACHE_ENTRY NewEntry;

//   DbgPrint("CacheSet.\n");

    error = FT_Get_Glyph(GlyphSlot, &GlyphCopy);
    if (error)
    {
        DbgPrint("Failure caching glyph.\n");
        return NULL;
    };
    error = FT_Glyph_To_Bitmap(&GlyphCopy, RenderMode, 0, 1);
    if (error)
    {
        DbgPrint("Failure rendering glyph.\n");
        return NULL;
    };

    NewEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_CACHE_ENTRY), TAG_FONT);
    if (!NewEntry)
    {
        DbgPrint("Alloc failure caching glyph.\n");
        FT_Done_Glyph(GlyphCopy);
        return NULL;
    }

    NewEntry->GlyphIndex = GlyphIndex;
    NewEntry->Face = Face;
    NewEntry->Glyph = GlyphCopy;
    NewEntry->Height = Height;

    InsertHeadList(&FontCacheListHead, &NewEntry->ListEntry);
    if (FontCacheNumEntries++ > MAX_FONT_CACHE)
    {
        NewEntry = (PFONT_CACHE_ENTRY)FontCacheListHead.Blink;
        FT_Done_Glyph(NewEntry->Glyph);
        RemoveTailList(&FontCacheListHead);
        ExFreePool(NewEntry);
        FontCacheNumEntries--;
    }

//   DbgPrint("Returning the glyphcopy: %x\n", GlyphCopy);

    return GlyphCopy;
}


static
void
FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
    return;
}

/*
   This function builds an FT_Fixed from a float. It puts the integer part
   in the highest 16 bits and the decimal part in the lowest 16 bits of the FT_Fixed.
   It fails if the integer part of the float number is greater than SHORT_MAX.
*/
static __inline FT_Fixed FT_FixedFromFloat(float f)
{
    short value = f;
    unsigned short fract = (f - value) * 0xFFFF;
    return (FT_Fixed)((long)value << 16 | (unsigned long)fract);
}

/*
   This function builds an FT_Fixed from a FIXED. It simply put f.value
   in the highest 16 bits and f.fract in the lowest 16 bits of the FT_Fixed.
*/
static __inline FT_Fixed FT_FixedFromFIXED(FIXED f)
{
    return (FT_Fixed)((long)f.value << 16 | (unsigned long)f.fract);
}

/*
 * Based on WineEngGetGlyphOutline
 *
 */
ULONG
FASTCALL
ftGdiGetGlyphOutline(
    PDC dc,
    WCHAR wch,
    UINT iFormat,
    LPGLYPHMETRICS pgm,
    ULONG cjBuf,
    PVOID pvBuf,
    LPMAT2 pmat2,
    BOOL bIgnoreRotation)
{
    static const FT_Matrix identityMat = {(1 << 16), 0, 0, (1 << 16)};
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = 0;
    GLYPHMETRICS gm;
    ULONG Size;
    FT_Face ft_face;
    FT_UInt glyph_index;
    DWORD width, height, pitch, needed = 0;
    FT_Bitmap ft_bitmap;
    FT_Error error;
    INT left, right, top = 0, bottom = 0;
    FT_Angle angle = 0;
    FT_Int load_flags = FT_LOAD_DEFAULT | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
    FLOAT eM11, widthRatio = 1.0;
    FT_Matrix transMat = identityMat;
    BOOL needsTransform = FALSE;
    INT orientation;
    LONG aveWidth;
    INT adv, lsb, bbx; /* These three hold to widths of the unrotated chars */
    OUTLINETEXTMETRICW *potm;
    int n = 0;
    FT_CharMap found = 0, charmap;
    XFORM xForm;

    DPRINT("%d, %08x, %p, %08lx, %p, %p\n", wch, iFormat, pgm,
           cjBuf, pvBuf, pmat2);

    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    MatrixS2XForm(&xForm, &dc->DcLevel.mxWorldToDevice);
    eM11 = xForm.eM11;

    hFont = Dc_Attr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    FontGDI = ObjToGDI(TextObj->Font, FONT);
    ft_face = FontGDI->face;

    aveWidth = FT_IS_SCALABLE(ft_face) ? TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth: 0;
    orientation = FT_IS_SCALABLE(ft_face) ? TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation: 0;

    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    potm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
    if (!potm)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        TEXTOBJ_UnlockText(TextObj);
        return GDI_ERROR;
    }
    IntGetOutlineTextMetrics(FontGDI, Size, potm);

    IntLockFreeType;

    /* During testing, I never saw this used. In here just incase.*/
    if (ft_face->charmap == NULL)
    {
        DPRINT("WARNING: No charmap selected!\n");
        DPRINT("This font face has %d charmaps\n", ft_face->num_charmaps);

        for (n = 0; n < ft_face->num_charmaps; n++)
        {
            charmap = ft_face->charmaps[n];
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
        error = FT_Set_Charmap(ft_face, found);
        if (error)
        {
            DPRINT1("WARNING: Could not set the charmap!\n");
        }
    }

//  FT_Set_Pixel_Sizes(ft_face,
//                     TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
    /* FIXME should set character height if neg */
//                     (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ? - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
//                      TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));

    TEXTOBJ_UnlockText(TextObj);

    if (iFormat & GGO_GLYPH_INDEX)
    {
        glyph_index = wch;
        iFormat &= ~GGO_GLYPH_INDEX;
    }
    else  glyph_index = FT_Get_Char_Index(ft_face, wch);

    if (orientation || (iFormat != GGO_METRICS && iFormat != GGO_BITMAP) || aveWidth || pmat2)
        load_flags |= FT_LOAD_NO_BITMAP;

    if (iFormat & GGO_UNHINTED)
    {
        load_flags |= FT_LOAD_NO_HINTING;
        iFormat &= ~GGO_UNHINTED;
    }

    error = FT_Load_Glyph(ft_face, glyph_index, load_flags);
    if (error)
    {
        DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
        IntUnLockFreeType;
        if (potm) ExFreePoolWithTag(potm, TAG_GDITEXT);
        return GDI_ERROR;
    }
    IntUnLockFreeType;

    if (aveWidth && potm)
    {
        widthRatio = (FLOAT)aveWidth * eM11 /
                     (FLOAT) potm->otmTextMetrics.tmAveCharWidth;
    }

    left = (INT)(ft_face->glyph->metrics.horiBearingX * widthRatio) & -64;
    right = (INT)((ft_face->glyph->metrics.horiBearingX +
                   ft_face->glyph->metrics.width) * widthRatio + 63) & -64;

    adv = (INT)((ft_face->glyph->metrics.horiAdvance * widthRatio) + 63) >> 6;
    lsb = left >> 6;
    bbx = (right - left) >> 6;

    DPRINT("Advance = %d, lsb = %d, bbx = %d\n",adv, lsb, bbx);

    IntLockFreeType;

    /* Scaling transform */
    if (aveWidth)
    {
        FT_Matrix scaleMat;
        DPRINT("Scaling Trans!\n");
        scaleMat.xx = FT_FixedFromFloat(widthRatio);
        scaleMat.xy = 0;
        scaleMat.yx = 0;
        scaleMat.yy = (1 << 16);
        FT_Matrix_Multiply(&scaleMat, &transMat);
        needsTransform = TRUE;
    }

    /* Slant transform */
    if (potm->otmTextMetrics.tmItalic)
    {
        FT_Matrix slantMat;
        DPRINT("Slant Trans!\n");
        slantMat.xx = (1 << 16);
        slantMat.xy = ((1 << 16) >> 2);
        slantMat.yx = 0;
        slantMat.yy = (1 << 16);
        FT_Matrix_Multiply(&slantMat, &transMat);
        needsTransform = TRUE;
    }

    /* Rotation transform */
    if (orientation)
    {
        FT_Matrix rotationMat;
        FT_Vector vecAngle;
        DPRINT("Rotation Trans!\n");
        angle = FT_FixedFromFloat((float)orientation / 10.0);
        FT_Vector_Unit(&vecAngle, angle);
        rotationMat.xx = vecAngle.x;
        rotationMat.xy = -vecAngle.y;
        rotationMat.yx = -rotationMat.xy;
        rotationMat.yy = rotationMat.xx;
        FT_Matrix_Multiply(&rotationMat, &transMat);
        needsTransform = TRUE;
    }

    /* Extra transformation specified by caller */
    if (pmat2)
    {
        FT_Matrix extraMat;
        DPRINT("MAT2 Matrix Trans!\n");
        extraMat.xx = FT_FixedFromFIXED(pmat2->eM11);
        extraMat.xy = FT_FixedFromFIXED(pmat2->eM21);
        extraMat.yx = FT_FixedFromFIXED(pmat2->eM12);
        extraMat.yy = FT_FixedFromFIXED(pmat2->eM22);
        FT_Matrix_Multiply(&extraMat, &transMat);
        needsTransform = TRUE;
    }

    if (potm) ExFreePoolWithTag(potm, TAG_GDITEXT); /* It looks like we are finished with potm ATM.*/

    if (!needsTransform)
    {
        DPRINT("No Need to be Transformed!\n");
        top = (ft_face->glyph->metrics.horiBearingY + 63) & -64;
        bottom = (ft_face->glyph->metrics.horiBearingY -
                  ft_face->glyph->metrics.height) & -64;
        gm.gmCellIncX = adv;
        gm.gmCellIncY = 0;
    }
    else
    {
        INT xc, yc;
        FT_Vector vec;
        for (xc = 0; xc < 2; xc++)
        {
            for (yc = 0; yc < 2; yc++)
            {
                vec.x = (ft_face->glyph->metrics.horiBearingX +
                         xc * ft_face->glyph->metrics.width);
                vec.y = ft_face->glyph->metrics.horiBearingY -
                        yc * ft_face->glyph->metrics.height;
                DPRINT("Vec %ld,%ld\n", vec.x, vec.y);
                FT_Vector_Transform(&vec, &transMat);
                if (xc == 0 && yc == 0)
                {
                    left = right = vec.x;
                    top = bottom = vec.y;
                }
                else
                {
                    if (vec.x < left) left = vec.x;
                    else if (vec.x > right) right = vec.x;
                    if (vec.y < bottom) bottom = vec.y;
                    else if (vec.y > top) top = vec.y;
                }
            }
        }
        left = left & -64;
        right = (right + 63) & -64;
        bottom = bottom & -64;
        top = (top + 63) & -64;

        DPRINT("transformed box: (%d,%d - %d,%d)\n", left, top, right, bottom);
        vec.x = ft_face->glyph->metrics.horiAdvance;
        vec.y = 0;
        FT_Vector_Transform(&vec, &transMat);
        gm.gmCellIncX = (vec.x+63) >> 6;
        gm.gmCellIncY = -((vec.y+63) >> 6);
    }
    gm.gmBlackBoxX = (right - left) >> 6;
    gm.gmBlackBoxY = (top - bottom) >> 6;
    gm.gmptGlyphOrigin.x = left >> 6;
    gm.gmptGlyphOrigin.y = top >> 6;

    DPRINT("CX %d CY %d BBX %d BBY %d GOX %d GOY %d\n",
           gm.gmCellIncX, gm.gmCellIncY,
           gm.gmBlackBoxX, gm.gmBlackBoxY,
           gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    IntUnLockFreeType;

    if (pgm) RtlCopyMemory(pgm, &gm, sizeof(GLYPHMETRICS));

    if (iFormat == GGO_METRICS)
    {
        DPRINT("GGO_METRICS Exit!\n");
        return 1; /* FIXME */
    }

    if (ft_face->glyph->format != ft_glyph_format_outline && iFormat != GGO_BITMAP)
    {
        DPRINT1("loaded a bitmap\n");
        return GDI_ERROR;
    }

    switch (iFormat)
    {
    case GGO_BITMAP:
        width = gm.gmBlackBoxX;
        height = gm.gmBlackBoxY;
        pitch = ((width + 31) >> 5) << 2;
        needed = pitch * height;

        if (!pvBuf || !cjBuf) break;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT w = (ft_face->glyph->bitmap.width + 7) >> 3;
            INT h = ft_face->glyph->bitmap.rows;
            while (h--)
            {
                RtlCopyMemory(dst, src, w);
                src += ft_face->glyph->bitmap.pitch;
                dst += pitch;
            }
            break;
        }

        case ft_glyph_format_outline:
            ft_bitmap.width = width;
            ft_bitmap.rows = height;
            ft_bitmap.pitch = pitch;
            ft_bitmap.pixel_mode = ft_pixel_mode_mono;
            ft_bitmap.buffer = pvBuf;

            IntLockFreeType;
            if (needsTransform)
            {
                FT_Outline_Transform(&ft_face->glyph->outline, &transMat);
            }
            FT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );
            /* Note: FreeType will only set 'black' bits for us. */
            RtlZeroMemory(pvBuf, needed);
            FT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
            IntUnLockFreeType;
            break;

        default:
            DPRINT1("loaded glyph format %x\n", ft_face->glyph->format);
            return GDI_ERROR;
        }
        break;

    case GGO_GRAY2_BITMAP:
    case GGO_GRAY4_BITMAP:
    case GGO_GRAY8_BITMAP:
    {
        unsigned int mult, row, col;
        BYTE *start, *ptr;

        width = gm.gmBlackBoxX;
        height = gm.gmBlackBoxY;
        pitch = (width + 3) / 4 * 4;
        needed = pitch * height;

        if (!pvBuf || !cjBuf) break;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT h = ft_face->glyph->bitmap.rows;
            INT x;
            while (h--)
            {
                for (x = 0; x < pitch; x++)
                {
                    if (x < ft_face->glyph->bitmap.width)
                        dst[x] = (src[x / 8] & (1 << ( (7 - (x % 8))))) ? 0xff : 0;
                    else
                        dst[x] = 0;
                }
                src += ft_face->glyph->bitmap.pitch;
                dst += pitch;
            }
            return needed;
        }
        case ft_glyph_format_outline:
        {
            ft_bitmap.width = width;
            ft_bitmap.rows = height;
            ft_bitmap.pitch = pitch;
            ft_bitmap.pixel_mode = ft_pixel_mode_grays;
            ft_bitmap.buffer = pvBuf;

            IntLockFreeType;
            if (needsTransform)
            {
                FT_Outline_Transform(&ft_face->glyph->outline, &transMat);
            }
            FT_Outline_Translate(&ft_face->glyph->outline, -left, -bottom );
            RtlZeroMemory(ft_bitmap.buffer, cjBuf);
            FT_Outline_Get_Bitmap(library, &ft_face->glyph->outline, &ft_bitmap);
            IntUnLockFreeType;

            if (iFormat == GGO_GRAY2_BITMAP)
                mult = 4;
            else if (iFormat == GGO_GRAY4_BITMAP)
                mult = 16;
            else if (iFormat == GGO_GRAY8_BITMAP)
                mult = 64;
            else
            {
                return GDI_ERROR;
            }
        }
        default:
            DPRINT1("loaded glyph format %x\n", ft_face->glyph->format);
            return GDI_ERROR;
        }
        start = pvBuf;
        for (row = 0; row < height; row++)
        {
            ptr = start;
            for (col = 0; col < width; col++, ptr++)
            {
                *ptr = (((int)*ptr) * mult + 128) / 256;
            }
            start += pitch;
        }
        break;
    }

    case GGO_NATIVE:
    {
        int contour, point = 0, first_pt;
        FT_Outline *outline = &ft_face->glyph->outline;
        TTPOLYGONHEADER *pph;
        TTPOLYCURVE *ppc;
        DWORD pph_start, cpfx, type;

        if (cjBuf == 0) pvBuf = NULL; /* This is okay, need cjBuf to allocate. */

        IntLockFreeType;
        if (needsTransform && pvBuf) FT_Outline_Transform(outline, &transMat);

        for (contour = 0; contour < outline->n_contours; contour++)
        {
            pph_start = needed;
            pph = (TTPOLYGONHEADER *)((char *)pvBuf + needed);
            first_pt = point;
            if (pvBuf)
            {
                pph->dwType = TT_POLYGON_TYPE;
                FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
            }
            needed += sizeof(*pph);
            point++;
            while (point <= outline->contours[contour])
            {
                ppc = (TTPOLYCURVE *)((char *)pvBuf + needed);
                type = (outline->tags[point] & FT_Curve_Tag_On) ?
                       TT_PRIM_LINE : TT_PRIM_QSPLINE;
                cpfx = 0;
                do
                {
                    if (pvBuf)
                        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                    cpfx++;
                    point++;
                }
                while (point <= outline->contours[contour] &&
                        (outline->tags[point] & FT_Curve_Tag_On) ==
                        (outline->tags[point-1] & FT_Curve_Tag_On));

                /* At the end of a contour Windows adds the start point, but
                   only for Beziers */
                if (point > outline->contours[contour] &&
                        !(outline->tags[point-1] & FT_Curve_Tag_On))
                {
                    if (pvBuf)
                        FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
                    cpfx++;
                }
                else if (point <= outline->contours[contour] &&
                         outline->tags[point] & FT_Curve_Tag_On)
                {
                    /* add closing pt for bezier */
                    if (pvBuf)
                        FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                    cpfx++;
                    point++;
                }
                if (pvBuf)
                {
                    ppc->wType = type;
                    ppc->cpfx = cpfx;
                }
                needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
            }
            if (pvBuf) pph->cb = needed - pph_start;
        }
        IntUnLockFreeType;
        break;
    }
    case GGO_BEZIER:
    {
        /* Convert the quadratic Beziers to cubic Beziers.
           The parametric eqn for a cubic Bezier is, from PLRM:
           r(t) = at^3 + bt^2 + ct + r0
           with the control points:
           r1 = r0 + c/3
           r2 = r1 + (c + b)/3
           r3 = r0 + c + b + a

           A quadratic Beizer has the form:
           p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

           So equating powers of t leads to:
           r1 = 2/3 p1 + 1/3 p0
           r2 = 2/3 p1 + 1/3 p2
           and of course r0 = p0, r3 = p2
         */

        int contour, point = 0, first_pt;
        FT_Outline *outline = &ft_face->glyph->outline;
        TTPOLYGONHEADER *pph;
        TTPOLYCURVE *ppc;
        DWORD pph_start, cpfx, type;
        FT_Vector cubic_control[4];
        if (cjBuf == 0) pvBuf = NULL;

        if (needsTransform && pvBuf)
        {
            IntLockFreeType;
            FT_Outline_Transform(outline, &transMat);
            IntUnLockFreeType;
        }

        for (contour = 0; contour < outline->n_contours; contour++)
        {
            pph_start = needed;
            pph = (TTPOLYGONHEADER *)((char *)pvBuf + needed);
            first_pt = point;
            if (pvBuf)
            {
                pph->dwType = TT_POLYGON_TYPE;
                FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
            }
            needed += sizeof(*pph);
            point++;
            while (point <= outline->contours[contour])
            {
                ppc = (TTPOLYCURVE *)((char *)pvBuf + needed);
                type = (outline->tags[point] & FT_Curve_Tag_On) ?
                       TT_PRIM_LINE : TT_PRIM_CSPLINE;
                cpfx = 0;
                do
                {
                    if (type == TT_PRIM_LINE)
                    {
                        if (pvBuf)
                            FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                        cpfx++;
                        point++;
                    }
                    else
                    {
                        /* Unlike QSPLINEs, CSPLINEs always have their endpoint
                           so cpfx = 3n */

                        /* FIXME: Possible optimization in endpoint calculation
                           if there are two consecutive curves */
                        cubic_control[0] = outline->points[point-1];
                        if (!(outline->tags[point-1] & FT_Curve_Tag_On))
                        {
                            cubic_control[0].x += outline->points[point].x + 1;
                            cubic_control[0].y += outline->points[point].y + 1;
                            cubic_control[0].x >>= 1;
                            cubic_control[0].y >>= 1;
                        }
                        if (point+1 > outline->contours[contour])
                            cubic_control[3] = outline->points[first_pt];
                        else
                        {
                            cubic_control[3] = outline->points[point+1];
                            if (!(outline->tags[point+1] & FT_Curve_Tag_On))
                            {
                                cubic_control[3].x += outline->points[point].x + 1;
                                cubic_control[3].y += outline->points[point].y + 1;
                                cubic_control[3].x >>= 1;
                                cubic_control[3].y >>= 1;
                            }
                        }
                        /* r1 = 1/3 p0 + 2/3 p1
                           r2 = 1/3 p2 + 2/3 p1 */
                        cubic_control[1].x = (2 * outline->points[point].x + 1) / 3;
                        cubic_control[1].y = (2 * outline->points[point].y + 1) / 3;
                        cubic_control[2] = cubic_control[1];
                        cubic_control[1].x += (cubic_control[0].x + 1) / 3;
                        cubic_control[1].y += (cubic_control[0].y + 1) / 3;
                        cubic_control[2].x += (cubic_control[3].x + 1) / 3;
                        cubic_control[2].y += (cubic_control[3].y + 1) / 3;
                        if (pvBuf)
                        {
                            FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
                            FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
                            FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
                        }
                        cpfx += 3;
                        point++;
                    }
                }
                while (point <= outline->contours[contour] &&
                        (outline->tags[point] & FT_Curve_Tag_On) ==
                        (outline->tags[point-1] & FT_Curve_Tag_On));
                /* At the end of a contour Windows adds the start point,
                   but only for Beziers and we've already done that.
                */
                if (point <= outline->contours[contour] &&
                        outline->tags[point] & FT_Curve_Tag_On)
                {
                    /* This is the closing pt of a bezier, but we've already
                      added it, so just inc point and carry on */
                    point++;
                }
                if (pvBuf)
                {
                    ppc->wType = type;
                    ppc->cpfx = cpfx;
                }
                needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
            }
            if (pvBuf) pph->cb = needed - pph_start;
        }
        break;
    }

    default:
        DPRINT1("Unsupported format %d\n", iFormat);
        return GDI_ERROR;
    }

    DPRINT("ftGdiGetGlyphOutline END and needed %d\n", needed);
    return needed;
}

BOOL
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
    FT_Glyph realglyph;
    INT error, n, glyph_index, i, previous;
    ULONGLONG TotalWidth = 0;
    FT_CharMap charmap, found = NULL;
    BOOL use_kerning;
    FT_Render_Mode RenderMode;
    BOOLEAN Render;

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->face;
    if (NULL != Fit)
    {
        *Fit = 0;
    }

    IntLockFreeType;
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

    Render = IntIsFontRenderingEnabled();
    if (Render)
        RenderMode = IntGetFontRenderMode(&TextObj->logfont.elfEnumLogfontEx.elfLogFont);
    else
        RenderMode = FT_RENDER_MODE_MONO;

    error = FT_Set_Pixel_Sizes(face,
                               TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                               /* FIXME should set character height if neg */
                               (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ?
                                - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
                                TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));
    if (error)
    {
        DPRINT1("Error in setting pixel sizes: %u\n", error);
    }

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    for (i = 0; i < Count; i++)
    {
        glyph_index = FT_Get_Char_Index(face, *String);
        if (!(realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                             TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)))
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
                break;
            }

            glyph = face->glyph;
            realglyph = ftGdiGlyphCacheSet(face, glyph_index,
                                           TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight, glyph, RenderMode);
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %u]\n", glyph_index);
                break;
            }
        }

        /* retrieve kerning distance */
        if (use_kerning && previous && glyph_index)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TotalWidth += delta.x;
        }

        TotalWidth += realglyph->advance.x >> 10;

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
    IntUnLockFreeType;

    Size->cx = (TotalWidth + 32) >> 6;
    Size->cy = (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ? - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight);
    Size->cy = EngMulDiv(Size->cy, IntGdiGetDeviceCaps(dc, LOGPIXELSY), 72);

    return TRUE;
}


INT
FASTCALL
ftGdiGetTextCharsetInfo(
    PDC Dc,
    LPFONTSIGNATURE lpSig,
    DWORD dwFlags)
{
    PDC_ATTR Dc_Attr;
    UINT Ret = DEFAULT_CHARSET, i;
    HFONT hFont;
    PTEXTOBJ TextObj;
    PFONTGDI FontGdi;
    FONTSIGNATURE fs;
    TT_OS2 *pOS2;
    FT_Face Face;
    CHARSETINFO csi;
    DWORD cp, fs0;
    USHORT usACP, usOEM;

    Dc_Attr = Dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &Dc->Dc_Attr;
    hFont = Dc_Attr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
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
        if (pOS2->version == 0)
        {
            FT_UInt dummy;

            if (FT_Get_First_Char( Face, &dummy ) < 0x100)
                fs.fsCsb[0] |= FS_LATIN1;
            else
                fs.fsCsb[0] |= FS_SYMBOL;
        }
    }
    DPRINT("Csb 1=%x  0=%x\n", fs.fsCsb[1],fs.fsCsb[0]);
    if (fs.fsCsb[0] == 0)
    { /* let's see if we can find any interesting cmaps */
        for (i = 0; i < Face->num_charmaps; i++)
        {
            switch (Face->charmaps[i]->encoding)
            {
            case FT_ENCODING_UNICODE:
            case FT_ENCODING_APPLE_ROMAN:
                fs.fsCsb[0] |= FS_LATIN1;
                break;
            case FT_ENCODING_MS_SYMBOL:
                fs.fsCsb[0] |= FS_SYMBOL;
                break;
            default:
                break;
            }
        }
    }
    if (lpSig)
    {
        RtlCopyMemory(lpSig, &fs, sizeof(FONTSIGNATURE));
    }

    RtlGetDefaultCodePage(&usACP, &usOEM);
    cp = usACP;

    if (IntTranslateCharsetInfo(&cp, &csi, TCI_SRCCODEPAGE))
        if (csi.fs.fsCsb[0] & fs.fsCsb[0])
        {
            DPRINT("Hit 1\n");
            Ret = csi.ciCharset;
            goto Exit;
        }

    for (i = 0; i < MAXTCIINDEX; i++)
    {
        fs0 = 1L << i;
        if (fs.fsCsb[0] & fs0)
        {
            if (IntTranslateCharsetInfo(&fs0, &csi, TCI_SRCFONTSIG))
            {
                //*cp = csi.ciACP;
                DPRINT("Hit 2\n");
                Ret = csi.ciCharset;
                goto Exit;
            }
            else
                DPRINT1("TCI failing on %x\n", fs0);
        }
    }
Exit:
    DPRINT("CharSet %d CodePage %d\n",csi.ciCharset, csi.ciACP);
    return (MAKELONG(csi.ciACP, csi.ciCharset));
}


DWORD
FASTCALL
ftGetFontUnicodeRanges(PFONTGDI Font, PGLYPHSET glyphset)
{
    DWORD size = 0;
    DWORD num_ranges = 0;
    FT_Face face = Font->face;

    if (face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt glyph_code = 0;
        FT_ULong char_code, char_code_prev;

        char_code_prev = char_code = FT_Get_First_Char(face, &glyph_code);

        DPRINT("face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %04lx\n",
               face->num_glyphs, glyph_code, char_code);

        if (!glyph_code) return 0;

        if (glyphset)
        {
            glyphset->ranges[0].wcLow = (USHORT)char_code;
            glyphset->ranges[0].cGlyphs = 0;
            glyphset->cGlyphsSupported = 0;
        }

        num_ranges = 1;
        while (glyph_code)
        {
            if (char_code < char_code_prev)
            {
                DPRINT1("expected increasing char code from FT_Get_Next_Char\n");
                return 0;
            }
            if (char_code - char_code_prev > 1)
            {
                num_ranges++;
                if (glyphset)
                {
                    glyphset->ranges[num_ranges - 1].wcLow = (USHORT)char_code;
                    glyphset->ranges[num_ranges - 1].cGlyphs = 1;
                    glyphset->cGlyphsSupported++;
                }
            }
            else if (glyphset)
            {
                glyphset->ranges[num_ranges - 1].cGlyphs++;
                glyphset->cGlyphsSupported++;
            }
            char_code_prev = char_code;
            char_code = FT_Get_Next_Char(face, char_code, &glyph_code);
        }
    }
    else
        DPRINT1("encoding %u not supported\n", face->charmap->encoding);

    size = sizeof(GLYPHSET) + sizeof(WCRANGE) * (num_ranges - 1);
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
    }
    return size;
}


BOOL
FASTCALL
ftGdiGetTextMetricsW(
    HDC hDC,
    PTMW_INTERNAL ptmwi)
{
    PDC dc;
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face Face;
    TT_OS2 *pOS2;
    TT_HoriHeader *pHori;
    FT_WinFNT_HeaderRec Win;
    ULONG Error;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!ptmwi)
    {
        SetLastWin32Error(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
    TextObj = RealizeFontInit(Dc_Attr->hlfntNew);
    if (NULL != TextObj)
    {
        FontGDI = ObjToGDI(TextObj->Font, FONT);

        Face = FontGDI->face;
        IntLockFreeType;
        Error = FT_Set_Pixel_Sizes(Face,
                                   TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                                   /* FIXME should set character height if neg */
                                   (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ?
                                    - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
                                    TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));
        IntUnLockFreeType;
        if (0 != Error)
        {
            DPRINT1("Error in setting pixel sizes: %u\n", Error);
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
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

            Error = FT_Get_WinFNT_Header(FontGDI->face , &Win);

            IntUnLockFreeType;

            if (NT_SUCCESS(Status))
            {
                if (!(FontGDI->flRealizedType & FDM_TYPE_TEXT_METRIC))
                {
                    FillTM(&FontGDI->TextMetric, FontGDI, pOS2, pHori, !Error ? &Win : 0);
                    FontGDI->flRealizedType |= FDM_TYPE_TEXT_METRIC;
                }

                RtlCopyMemory(&ptmwi->TextMetric, &FontGDI->TextMetric, sizeof(TEXTMETRICW));
                /* FIXME: Fill Diff member */
                RtlZeroMemory(&ptmwi->Diff, sizeof(ptmwi->Diff));
            }
        }
        TEXTOBJ_UnlockText(TextObj);
    }
    else
    {
        Status = STATUS_INVALID_HANDLE;
    }
    DC_UnlockDc(dc);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }
    return TRUE;
}


DWORD
FASTCALL
ftGdiGetFontData(
    PFONTGDI FontGdi,
    DWORD Table,
    DWORD Offset,
    PVOID Buffer,
    DWORD Size)
{
    DWORD Result = GDI_ERROR;

    IntLockFreeType;

    if (FT_IS_SFNT(FontGdi->face))
    {
        if (Table)
            Table = Table >> 24 | Table << 24 | (Table >> 8 & 0xFF00) |
                    (Table << 8 & 0xFF0000);

        if (!Buffer) Size = 0;

        if (Buffer && Size)
        {
            FT_Error Error;
            FT_ULong Needed = 0;

            Error = FT_Load_Sfnt_Table(FontGdi->face, Table, Offset, NULL, &Needed);

            if ( !Error && Needed < Size) Size = Needed;
        }
        if (!FT_Load_Sfnt_Table(FontGdi->face, Table, Offset, Buffer, &Size))
            Result = Size;
    }

    IntUnLockFreeType;

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
    if (LogFont->lfWeight != FW_DONTCARE)
    {
        if (LogFont->lfWeight < Otm->otmTextMetrics.tmWeight)
        {
            WeightDiff = Otm->otmTextMetrics.tmWeight - LogFont->lfWeight;
        }
        else
        {
            WeightDiff = LogFont->lfWeight - Otm->otmTextMetrics.tmWeight;
        }
        Score += (1000 - WeightDiff) / (1000 / 25);
    }
    else
    {
        Score += 25;
    }

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
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {{0}};
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

static
VOID
FASTCALL
IntFontType(PFONTGDI Font)
{
    PS_FontInfoRec psfInfo;
    FT_ULong tmp_size = 0;

    if (FT_HAS_MULTIPLE_MASTERS(Font->face))
        Font->FontObj.flFontType |= FO_MULTIPLEMASTER;
    if (FT_HAS_VERTICAL( Font->face ))
        Font->FontObj.flFontType |= FO_VERT_FACE;
    if (FT_IS_SCALABLE( Font->face ))
        Font->FontObj.flFontType |= FO_TYPE_RASTER;
    if (FT_IS_SFNT(Font->face))
    {
        Font->FontObj.flFontType |= FO_TYPE_TRUETYPE;
        if (FT_Get_Sfnt_Table(Font->face, ft_sfnt_post))
            Font->FontObj.flFontType |= FO_POSTSCRIPT;
    }
    if (!FT_Get_PS_Font_Info(Font->face, &psfInfo ))
    {
        Font->FontObj.flFontType |= FO_POSTSCRIPT;
    }
    /* check for the presence of the 'CFF ' table to check if the font is Type1 */
    if (!FT_Load_Sfnt_Table(Font->face, FT_MAKE_TAG('C','F','F',' '), 0, NULL, &tmp_size))
    {
        Font->FontObj.flFontType |= (FO_CFF|FO_POSTSCRIPT);
    }
}

NTSTATUS
FASTCALL
TextIntRealizeFont(HFONT FontHandle, PTEXTOBJ pTextObj)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEXTOBJ TextObj;
    UNICODE_STRING FaceName;
    PW32PROCESS Win32Process;
    UINT MatchScore;

    if (!pTextObj)
    {
        TextObj = TEXTOBJ_LockText(FontHandle);
        if (NULL == TextObj)
        {
            return STATUS_INVALID_HANDLE;
        }

        if (TextObj->fl & TEXTOBJECT_INIT)
        {
            TEXTOBJ_UnlockText(TextObj);
            return STATUS_SUCCESS;
        }
    }
    else
        TextObj = pTextObj;

    if (! RtlCreateUnicodeString(&FaceName, TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfFaceName))
    {
        if (!pTextObj) TEXTOBJ_UnlockText(TextObj);
        return STATUS_NO_MEMORY;
    }
    SubstituteFontFamily(&FaceName, 0);
    MatchScore = 0;
    TextObj->Font = NULL;

    /* First search private fonts */
    Win32Process = PsGetCurrentProcessWin32Process();
    IntLockProcessPrivateFonts(Win32Process);
    FindBestFontFromList(&TextObj->Font, &MatchScore,
                         &TextObj->logfont.elfEnumLogfontEx.elfLogFont, &FaceName,
                         &Win32Process->PrivateFontListHead);
    IntUnLockProcessPrivateFonts(Win32Process);

    /* Search system fonts */
    IntLockGlobalFonts;
    FindBestFontFromList(&TextObj->Font, &MatchScore,
                         &TextObj->logfont.elfEnumLogfontEx.elfLogFont, &FaceName,
                         &FontListHead);
    IntUnLockGlobalFonts;
    if (NULL == TextObj->Font)
    {
        DPRINT1("Requested font %S not found, no fonts loaded at all\n",
                TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfFaceName);
        Status = STATUS_NOT_FOUND;
    }
    else
    {
        PFONTGDI FontGdi = ObjToGDI(TextObj->Font, FONT);
        // Need hdev, when freetype is loaded need to create DEVOBJ for
        // Consumer and Producer.
        TextObj->Font->iUniq = 1; // Now it can be cached.
        IntFontType(FontGdi);
        FontGdi->flType = TextObj->Font->flFontType;
        FontGdi->flRealizedType = 0;
        FontGdi->Underline = TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfUnderline ? 0xff : 0;
        FontGdi->StrikeOut = TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfStrikeOut ? 0xff : 0;
        TextObj->fl |= TEXTOBJECT_INIT;
        Status = STATUS_SUCCESS;
    }

    RtlFreeUnicodeString(&FaceName);
    if (!pTextObj) TEXTOBJ_UnlockText(TextObj);

    ASSERT((NT_SUCCESS(Status) ^ (NULL == TextObj->Font)) != 0);

    return Status;
}


static
BOOL
FASTCALL
IntGetFullFileName(
    POBJECT_NAME_INFORMATION NameInfo,
    ULONG Size,
    PUNICODE_STRING FileName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Desired;

    InitializeObjectAttributes(&ObjectAttributes,
                               FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenFile(
                 &hFile,
                 0, //FILE_READ_ATTRIBUTES,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenFile() failed (Status = 0x%lx)\n", Status);
        return FALSE;
    }

    Status = ZwQueryObject(hFile, ObjectNameInformation, NameInfo, Size, &Desired);
    ZwClose(hFile);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwQueryObject() failed (Status = %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
FASTCALL
IntGdiGetFontResourceInfo(
    PUNICODE_STRING FileName,
    PVOID pBuffer,
    DWORD *pdwBytes,
    DWORD dwType)
{
    UNICODE_STRING EntryFileName;
    POBJECT_NAME_INFORMATION NameInfo1, NameInfo2;
    PLIST_ENTRY ListEntry;
    PFONT_ENTRY FontEntry;
    FONTFAMILYINFO Info;
    ULONG Size;
    BOOL bFound = FALSE;

    /* Create buffer for full path name */
    Size = sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR);
    NameInfo1 = ExAllocatePoolWithTag(PagedPool, Size, TAG_FINF);
    if (!NameInfo1)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Get the full path name */
    if (!IntGetFullFileName(NameInfo1, Size, FileName))
    {
        ExFreePool(NameInfo1);
        return FALSE;
    }

    /* Create a buffer for the entries' names */
    NameInfo2 = ExAllocatePoolWithTag(PagedPool, Size, TAG_FINF);
    if (!NameInfo2)
    {
        ExFreePool(NameInfo1);
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Try to find the pathname in the global font list */
    IntLockGlobalFonts;
    for (ListEntry = FontListHead.Flink;
            ListEntry != &FontListHead;
            ListEntry = ListEntry->Flink)
    {
        FontEntry = CONTAINING_RECORD(ListEntry, FONT_ENTRY, ListEntry);
        if (FontEntry->Font->Filename != NULL)
        {
            RtlInitUnicodeString(&EntryFileName , FontEntry->Font->Filename);
            if (IntGetFullFileName(NameInfo2, Size, &EntryFileName))
            {
                if (RtlEqualUnicodeString(&NameInfo1->Name, &NameInfo2->Name, FALSE))
                {
                    /* found */
                    FontFamilyFillInfo(&Info, FontEntry->FaceName.Buffer, FontEntry->Font);
                    bFound = TRUE;
                    break;
                }
            }
        }
    }
    IntUnLockGlobalFonts;

    /* Free the buffers */
    ExFreePool(NameInfo1);
    ExFreePool(NameInfo2);

    if (!bFound && dwType != 5)
    {
        /* Font could not be found in system table
           dwType == 5 will still handle this */
        return FALSE;
    }

    switch (dwType)
    {
    case 0: /* FIXME: returns 1 or 2, don't know what this is atm */
        *(DWORD*)pBuffer = 1;
        *pdwBytes = sizeof(DWORD);
        break;

    case 1: /* Copy the full font name */
        Size = wcslen(Info.EnumLogFontEx.elfFullName) + 1;
        Size = min(Size , LF_FULLFACESIZE) * sizeof(WCHAR);
        RtlCopyMemory(pBuffer, Info.EnumLogFontEx.elfFullName, Size);
        // FIXME: Do we have to zeroterminate?
        *pdwBytes = Size;
        break;

    case 2: /* Copy a LOGFONTW structure */
        Info.EnumLogFontEx.elfLogFont.lfWidth = 0;
        RtlCopyMemory(pBuffer, &Info.EnumLogFontEx.elfLogFont, sizeof(LOGFONTW));
        *pdwBytes = sizeof(LOGFONTW);
        break;

    case 3: /* FIXME: What exactly is copied here? */
        *(DWORD*)pBuffer = 1;
        *pdwBytes = sizeof(DWORD*);
        break;

    case 5: /* Looks like a BOOL that is copied, TRUE, if the font was not found */
        *(BOOL*)pBuffer = !bFound;
        *pdwBytes = sizeof(BOOL);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


BOOL
FASTCALL
ftGdiRealizationInfo(PFONTGDI Font, PREALIZATION_INFO Info)
{
    if (FT_HAS_FIXED_SIZES(Font->face))
        Info->iTechnology = RI_TECH_BITMAP;
    else
    {
        if (FT_IS_SCALABLE(Font->face))
            Info->iTechnology = RI_TECH_SCALABLE;
        else
            Info->iTechnology = RI_TECH_FIXED;
    }
    Info->iUniq = Font->FontObj.iUniq;
    Info->dwUnknown = -1;
    return TRUE;
}


DWORD
FASTCALL
ftGdiGetKerningPairs( PFONTGDI Font,
                      DWORD cPairs,
                      LPKERNINGPAIR pKerningPair)
{
    DWORD Count = 0;
    INT i = 0;
    FT_Face face = Font->face;

    if (FT_HAS_KERNING(face) && face->charmap->encoding == FT_ENCODING_UNICODE)
    {
        FT_UInt previous_index = 0, glyph_index = 0;
        FT_ULong char_code, char_previous;
        FT_Vector delta;

        char_previous = char_code = FT_Get_First_Char(face, &glyph_index);

        IntUnLockFreeType;

        while (glyph_index)
        {
            if (previous_index && glyph_index)
            {
                FT_Get_Kerning(face, previous_index, glyph_index, FT_KERNING_DEFAULT, &delta);

                if (pKerningPair && cPairs)
                {
                    pKerningPair[i].wFirst      = char_previous;
                    pKerningPair[i].wSecond     = char_code;
                    pKerningPair[i].iKernAmount = delta.x;
                    i++;
                    if (i == cPairs) break;
                }
                Count++;
            }
            previous_index = glyph_index;
            char_previous = char_code;
            char_code = FT_Get_Next_Char(face, char_code, &glyph_index);
        }
        IntUnLockFreeType;
    }
    return Count;
}


//////////////////
//
// Functions needing sorting.
//
///////////////
int APIENTRY
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

BOOL
APIENTRY
GreExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL LPRECT lprc,
    IN LPWSTR String,
    IN INT Count,
    IN OPTIONAL LPINT Dx,
    IN DWORD dwCodePage)
{
    /*
     * FIXME:
     * Call EngTextOut, which does the real work (calling DrvTextOut where
     * appropriate)
     */

    DC *dc;
    PDC_ATTR Dc_Attr;
    SURFOBJ *SurfObj;
    BITMAPOBJ *BitmapObj = NULL;
    int error, glyph_index, n, i;
    FT_Face face;
    FT_GlyphSlot glyph;
    FT_Glyph realglyph;
    FT_BitmapGlyph realglyph2;
    LONGLONG TextLeft, RealXStart;
    ULONG TextTop, previous, BackgroundLeft;
    FT_Bool use_kerning;
    RECTL DestRect, MaskRect;
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
    POINT Start;
    BOOL DoBreak = FALSE;
    HPALETTE hDestPalette;
    USHORT DxShift;

    // TODO: Write test-cases to exactly match real Windows in different
    // bad parameters (e.g. does Windows check the DC or the RECT first?).
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (dc->DC_Type == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && String == NULL))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        goto fail;
    }

    DxShift = fuOptions & ETO_PDY ? 1 : 0;

    if (PATH_IsPathOpen(dc->DcLevel))
    {
        if (!PATH_ExtTextOut( dc,
                              XStart,
                              YStart,
                              fuOptions,
                              (const RECT *)lprc,
                              String,
                              Count,
                              (const INT *)Dx)) goto fail;
        goto good;
    }

    if (lprc && (fuOptions & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        IntLPtoDP(dc, (POINT *)lprc, 2);
    }

    BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
    if ( !BitmapObj )
    {
        goto fail;
    }
    SurfObj = &BitmapObj->SurfObj;
    ASSERT(SurfObj);

    Start.x = XStart;
    Start.y = YStart;
    IntLPtoDP(dc, &Start, 1);

    RealXStart = (Start.x + dc->ptlDCOrig.x) << 6;
    YStart = Start.y + dc->ptlDCOrig.y;

    /* Create the brushes */
    hDestPalette = BitmapObj->hDIBPalette;
    if (!hDestPalette) hDestPalette = pPrimarySurface->DevInfo.hpalDefault;
    PalDestGDI = PALETTE_LockPalette(hDestPalette);
    if ( !PalDestGDI )
        Mode = PAL_RGB;
    else
    {
        Mode = PalDestGDI->Mode;
        PALETTE_UnlockPalette(PalDestGDI);
    }
    XlateObj = (XLATEOBJ*)IntEngCreateXlate(Mode, PAL_RGB, hDestPalette, NULL);
    if ( !XlateObj )
    {
        goto fail;
    }
    hBrushFg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, Dc_Attr->crForegroundClr), 0);
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
    if ((fuOptions & ETO_OPAQUE) || Dc_Attr->jBkMode == OPAQUE)
    {
        hBrushBg = NtGdiCreateSolidBrush(XLATEOBJ_iXlate(XlateObj, Dc_Attr->crBackgroundClr), 0);
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
    XlateObj2 = (XLATEOBJ*)IntEngCreateXlate(PAL_RGB, Mode, NULL, hDestPalette);
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
        DestRect.left   = lprc->left   + dc->ptlDCOrig.x;
        DestRect.top    = lprc->top    + dc->ptlDCOrig.y;
        DestRect.right  = lprc->right  + dc->ptlDCOrig.x;
        DestRect.bottom = lprc->bottom + dc->ptlDCOrig.y;
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
        if (Dc_Attr->jBkMode == OPAQUE)
        {
            fuOptions |= ETO_OPAQUE;
        }
    }

    TextObj = RealizeFontInit(Dc_Attr->hlfntNew);
    if (TextObj == NULL)
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
        RenderMode = IntGetFontRenderMode(&TextObj->logfont.elfEnumLogfontEx.elfLogFont);
    else
        RenderMode = FT_RENDER_MODE_MONO;

    error = FT_Set_Pixel_Sizes(
                face,
                TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                /* FIXME should set character height if neg */
                (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ?
                 - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
                 TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));
    if (error)
    {
        DPRINT1("Error in setting pixel sizes: %u\n", error);
        IntUnLockFreeType;
        goto fail;
    }

    /*
     * Process the vertical alignment and determine the yoff.
     */

    if (Dc_Attr->lTextAlign & TA_BASELINE)
        yoff = 0;
    else if (Dc_Attr->lTextAlign & TA_BOTTOM)
        yoff = -face->size->metrics.descender >> 6;
    else /* TA_TOP */
        yoff = face->size->metrics.ascender >> 6;

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    /*
     * Process the horizontal alignment and modify XStart accordingly.
     */

    if (Dc_Attr->lTextAlign & (TA_RIGHT | TA_CENTER))
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
            TextWidth = Count < 2 ? 0 : (Dx[(Count-2)<<DxShift] << 6);
        }
        else
        {
            Start = 0;
        }
        TempText = String + Start;

        for (i = Start; i < Count; i++)
        {
            if (fuOptions & ETO_GLYPH_INDEX)
                glyph_index = *TempText;
            else
                glyph_index = FT_Get_Char_Index(face, *TempText);

            if (!(realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                                 TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)))
            {
                error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                if (error)
                {
                    DPRINT1("WARNING: Failed to load and render glyph! [index: %u]\n", glyph_index);
                }

                glyph = face->glyph;
                realglyph = ftGdiGlyphCacheSet(face, glyph_index,
                                               TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight, glyph, RenderMode);
                if (!realglyph)
                {
                    DPRINT1("Failed to render glyph! [index: %u]\n", glyph_index);
                    IntUnLockFreeType;
                    goto fail;
                }

            }
            /* retrieve kerning distance */
            if (use_kerning && previous && glyph_index)
            {
                FT_Vector delta;
                FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
                TextWidth += delta.x;
            }

            TextWidth += realglyph->advance.x >> 10;

            previous = glyph_index;
            TempText++;
        }

        previous = 0;

        if (Dc_Attr->lTextAlign & TA_RIGHT)
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
        if (fuOptions & ETO_GLYPH_INDEX)
            glyph_index = *String;
        else
            glyph_index = FT_Get_Char_Index(face, *String);

        if (!(realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                             TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)))
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("Failed to load and render glyph! [index: %u]\n", glyph_index);
                IntUnLockFreeType;
                goto fail;
            }
            glyph = face->glyph;
            realglyph = ftGdiGlyphCacheSet(face,
                                           glyph_index,
                                           TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                           glyph,
                                           RenderMode);
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %u]\n", glyph_index);
                IntUnLockFreeType;
                goto fail;
            }
        }
//      DbgPrint("realglyph: %x\n", realglyph);
//      DbgPrint("TextLeft: %d\n", TextLeft);

        /* retrieve kerning distance and move pen position */
        if (use_kerning && previous && glyph_index && NULL == Dx)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TextLeft += delta.x;
        }
//      DPRINT1("TextLeft: %d\n", TextLeft);
//      DPRINT1("TextTop: %d\n", TextTop);

        if (realglyph->format == ft_glyph_format_outline)
        {
            DbgPrint("Should already be done\n");
//         error = FT_Render_Glyph(glyph, RenderMode);
            error = FT_Glyph_To_Bitmap(&realglyph, RenderMode, 0, 0);
            if (error)
            {
                DPRINT1("WARNING: Failed to render glyph!\n");
                goto fail;
            }
        }
        realglyph2 = (FT_BitmapGlyph)realglyph;

//      DPRINT1("Pitch: %d\n", pitch);
//      DPRINT1("Advance: %d\n", realglyph->advance.x);

        if (fuOptions & ETO_OPAQUE)
        {
            DestRect.left = BackgroundLeft;
            DestRect.right = (TextLeft + (realglyph->advance.x >> 10) + 32) >> 6;
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

        DestRect.left = ((TextLeft + 32) >> 6) + realglyph2->left;
        DestRect.right = DestRect.left + realglyph2->bitmap.width;
        DestRect.top = TextTop + yoff - realglyph2->top;
        DestRect.bottom = DestRect.top + realglyph2->bitmap.rows;

        bitSize.cx = realglyph2->bitmap.width;
        bitSize.cy = realglyph2->bitmap.rows;
        MaskRect.right = realglyph2->bitmap.width;
        MaskRect.bottom = realglyph2->bitmap.rows;

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

        HSourceGlyph = EngCreateBitmap(bitSize, realglyph2->bitmap.pitch,
                                       (realglyph2->bitmap.pixel_mode == ft_pixel_mode_grays) ?
                                       BMF_8BPP : BMF_1BPP, BMF_TOPDOWN,
                                       realglyph2->bitmap.buffer);
        if ( !HSourceGlyph )
        {
            DPRINT1("WARNING: EngLockSurface() failed!\n");
            // FT_Done_Glyph(realglyph);
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
                DestRect.right >= lprc->right + dc->ptlDCOrig.x)
        {
            // We do the check '>=' instead of '>' to possibly save an iteration
            // through this loop, since it's breaking after the drawing is done,
            // and x is always incremented.
            DestRect.right = lprc->right + dc->ptlDCOrig.x;
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
            TextLeft += realglyph->advance.x >> 10;
//         DbgPrint("new TextLeft: %d\n", TextLeft);
        }
        else
        {
            TextLeft += Dx[i<<DxShift] << 6;
//         DbgPrint("new TextLeft2: %d\n", TextLeft);
        }

        if (DxShift)
        {
            TextTop -= Dx[2 * i + 1] << 6;
        }

        previous = glyph_index;

        String++;
    }

    IntUnLockFreeType;

    EngDeleteXlate(XlateObj);
    EngDeleteXlate(XlateObj2);
    BITMAPOBJ_UnlockBitmap(BitmapObj);
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);
    if (hBrushBg != NULL)
    {
        BRUSHOBJ_UnlockBrush(BrushBg);
        NtGdiDeleteObject(hBrushBg);
    }
    BRUSHOBJ_UnlockBrush(BrushFg);
    NtGdiDeleteObject(hBrushFg);
good:
    DC_UnlockDc( dc );

    return TRUE;

fail:
    if ( XlateObj2 != NULL )
        EngDeleteXlate(XlateObj2);
    if ( XlateObj != NULL )
        EngDeleteXlate(XlateObj);
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);
    if (BitmapObj != NULL)
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
    DC_UnlockDc(dc);

    return FALSE;
}

#define STACK_TEXT_BUFFER_SIZE 100
BOOL
APIENTRY
NtGdiExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL LPRECT UnsafeRect,
    IN LPWSTR UnsafeString,
    IN INT Count,
    IN OPTIONAL LPINT UnsafeDx,
    IN DWORD dwCodePage)
{
    BOOL Result = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    RECT SafeRect;
    BYTE LocalBuffer[STACK_TEXT_BUFFER_SIZE];
    PVOID Buffer = LocalBuffer;
    LPWSTR SafeString = NULL;
    LPINT SafeDx = NULL;
    ULONG BufSize, StringSize, DxSize = 0;

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && UnsafeString == NULL))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (Count > 0)
    {
        /* Calculate buffer size for string and Dx values */
        BufSize = StringSize = Count * sizeof(WCHAR);
        if (UnsafeDx)
        {
            /* If ETO_PDY is specified, we have pairs of INTs */
            DxSize = (Count * sizeof(INT)) * (fuOptions & ETO_PDY ? 2 : 1);
            BufSize += DxSize;
        }

        /* Check if our local buffer is large enough */
        if (BufSize > STACK_TEXT_BUFFER_SIZE)
        {
            /* It's not, allocate a temp buffer */
            Buffer = ExAllocatePoolWithTag(PagedPool, BufSize, TAG_GDITEXT);
            if (!Buffer)
            {
                return FALSE;
            }
        }

        /* Probe and copy user mode data to the buffer */
        _SEH2_TRY
        {
            /* Put the Dx before the String to assure alignment of 4 */
            SafeString = (LPWSTR)(((ULONG_PTR)Buffer) + DxSize);

            /* Probe and copy the string */
            ProbeForRead(UnsafeString, StringSize, 1);
            memcpy((PVOID)SafeString, UnsafeString, StringSize);

            /* If we have Dx values... */
            if (UnsafeDx)
            {
                /* ... probe and copy them */
                SafeDx = Buffer;
                ProbeForRead(UnsafeDx, DxSize, 1);
                memcpy(SafeDx, UnsafeDx, DxSize);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }
    }

    /* If we have a rect, copy it */
    if (UnsafeRect)
    {
        _SEH2_TRY
        {
            ProbeForRead(UnsafeRect, sizeof(RECT), 1);
            SafeRect = *UnsafeRect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END
        if (!NT_SUCCESS(Status))
        {
            goto cleanup;
        }
    }

    /* Finally call the internal routine */
    Result = GreExtTextOutW(hDC,
                            XStart,
                            YStart,
                            fuOptions,
                            &SafeRect,
                            SafeString,
                            Count,
                            SafeDx,
                            dwCodePage);

cleanup:
    /* If we allocated a buffer, free it */
    if (Buffer != LocalBuffer)
    {
        ExFreePoolWithTag(Buffer, TAG_GDITEXT);
    }

    return Result;
}


/*
* @implemented
*/
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    IN HDC hDC,
    IN UINT FirstChar,
    IN ULONG Count,
    IN OPTIONAL PWCHAR pwch,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    LPABC SafeBuff;
    LPABCFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (pwch)
    {
        _SEH2_TRY
        {
            ProbeForRead(pwch,
            sizeof(PWSTR),
            1);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    if (!NT_SUCCESS(Status))
    {
        SetLastWin32Error(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(ABC); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_GDITEXT);
    if (!fl) SafeBuffF = (LPABCFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        ExFreePool(SafeBuff);
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
    hFont = Dc_Attr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        ExFreePool(SafeBuff);
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
            ExFreePool(SafeBuff);
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    IntLockFreeType;
    FT_Set_Pixel_Sizes(face,
                       TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                       /* FIXME should set character height if neg */
                       (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ? - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
                        TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        int adv, lsb, bbx, left, right;

        if (pwch)
        {
            if (fl & GCABCW_INDICES)
                glyph_index = pwch[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, pwch[i - FirstChar]);
        }
        else
        {
            if (fl & GCABCW_INDICES)
                glyph_index = i;
            else
                glyph_index = FT_Get_Char_Index(face, i);
        }
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        left = (INT)face->glyph->metrics.horiBearingX  & -64;
        right = (INT)((face->glyph->metrics.horiBearingX + face->glyph->metrics.width) + 63) & -64;
        adv  = (face->glyph->advance.x + 32) >> 6;

//      int test = (INT)(face->glyph->metrics.horiAdvance + 63) >> 6;
//      DPRINT1("Advance Wine %d and Advance Ros %d\n",test, adv ); /* It's the same!*/

        lsb = left >> 6;
        bbx = (right - left) >> 6;
        /*
              DPRINT1("lsb %d and bbx %d\n", lsb, bbx );
         */
        if (!fl)
        {
            SafeBuffF[i - FirstChar].abcfA = (FLOAT) lsb;
            SafeBuffF[i - FirstChar].abcfB = (FLOAT) bbx;
            SafeBuffF[i - FirstChar].abcfC = (FLOAT) (adv - lsb - bbx);
        }
        else
        {
            SafeBuff[i - FirstChar].abcA = lsb;
            SafeBuff[i - FirstChar].abcB = bbx;
            SafeBuff[i - FirstChar].abcC = adv - lsb - bbx;
        }
    }
    IntUnLockFreeType;
    TEXTOBJ_UnlockText(TextObj);
    Status = MmCopyToCaller(Buffer, SafeBuff, BufferSize);
    if (! NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        ExFreePool(SafeBuff);
        return FALSE;
    }
    ExFreePool(SafeBuff);
    DPRINT("NtGdiGetCharABCWidths Worked!\n");
    return TRUE;
}

/*
* @implemented
*/
BOOL
APIENTRY
NtGdiGetCharWidthW(
    IN HDC hDC,
    IN UINT FirstChar,
    IN UINT Count,
    IN OPTIONAL PWCHAR pwc,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPINT SafeBuff;
    PFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;

    if (pwc)
    {
        _SEH2_TRY
        {
            ProbeForRead(pwc,
            sizeof(PWSTR),
            1);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    if (!NT_SUCCESS(Status))
    {
        SetLastWin32Error(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(INT); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_GDITEXT);
    if (!fl) SafeBuffF = (PFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        ExFreePool(SafeBuff);
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
    hFont = Dc_Attr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        ExFreePool(SafeBuff);
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
            ExFreePool(SafeBuff);
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    IntLockFreeType;
    FT_Set_Pixel_Sizes(face,
                       TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                       /* FIXME should set character height if neg */
                       (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight < 0 ?
                        - TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight :
                        TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ? 11 : TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        if (pwc)
        {
            if (fl & GCW_INDICES)
                glyph_index = pwc[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, pwc[i - FirstChar]);
        }
        else
        {
            if (fl & GCW_INDICES)
                glyph_index = i;
            else
                glyph_index = FT_Get_Char_Index(face, i);
        }
        FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (!fl)
            SafeBuffF[i - FirstChar] = (FLOAT) ((face->glyph->advance.x + 32) >> 6);
        else
            SafeBuff[i - FirstChar] = (face->glyph->advance.x + 32) >> 6;
    }
    IntUnLockFreeType;
    TEXTOBJ_UnlockText(TextObj);
    MmCopyToCaller(Buffer, SafeBuff, BufferSize);
    ExFreePool(SafeBuff);
    return TRUE;
}


/*
* @implemented
*/
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR UnSafepwc,
    IN INT cwc,
    OUT OPTIONAL LPWORD UnSafepgi,
    IN DWORD iMode)
{
    PDC dc;
    PDC_ATTR Dc_Attr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    OUTLINETEXTMETRICW *potm;
    INT i;
    FT_Face face;
    WCHAR DefChar = 0xffff;
    PWSTR Buffer = NULL;
    ULONG Size;

    if ((!UnSafepwc) && (!UnSafepgi)) return cwc;

    dc = DC_LockDc(hdc);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    Dc_Attr = dc->pDc_Attr;
    if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
    hFont = Dc_Attr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);
    if (!TextObj)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);
    TEXTOBJ_UnlockText(TextObj);

    Buffer = ExAllocatePoolWithTag(PagedPool, cwc*sizeof(WORD), TAG_GDITEXT);
    if (!Buffer)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return GDI_ERROR;
    }

    if (iMode & GGI_MARK_NONEXISTING_GLYPHS) DefChar = 0x001f;  /* Indicate non existence */
    else
    {
        Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
        potm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
        if (!potm)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorRet;
        }
        IntGetOutlineTextMetrics(FontGDI, Size, potm);
        DefChar = potm->otmTextMetrics.tmDefaultChar; // May need this.
        ExFreePool(potm);
    }

    _SEH2_TRY
    {
        ProbeForRead(UnSafepwc,
        sizeof(PWSTR),
        1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status)) goto ErrorRet;

    IntLockFreeType;
    face = FontGDI->face;

    for (i = 0; i < cwc; i++)
    {
        Buffer[i] = FT_Get_Char_Index(face, UnSafepwc[i]);
        if (Buffer[i] == 0)
        {
            if (DefChar == 0xffff && FT_IS_SFNT(face))
            {
                TT_OS2 *pOS2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
                DefChar = (pOS2->usDefaultChar ? FT_Get_Char_Index(face, pOS2->usDefaultChar) : 0);
            }
            Buffer[i] = DefChar;
        }
    }

    IntUnLockFreeType;

    _SEH2_TRY
    {
        ProbeForWrite(UnSafepgi,
        sizeof(WORD),
        1);
        RtlCopyMemory(UnSafepgi,
                      Buffer,
                      cwc*sizeof(WORD));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

ErrorRet:
    ExFreePoolWithTag(Buffer, TAG_GDITEXT);
    if (NT_SUCCESS(Status)) return cwc;
    SetLastWin32Error(Status);
    return GDI_ERROR;
}


/* EOF */
