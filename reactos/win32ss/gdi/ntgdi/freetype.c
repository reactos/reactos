/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/freetype.c
 * PURPOSE:         FreeType font engine interface
 * PROGRAMMER:      Copyright 2001 Huw D M Davies for CodeWeavers.
 *                  Copyright 2006 Dmitry Timoshkov for CodeWeavers.
 */

/** Includes ******************************************************************/

#include <win32k.h>

#include FT_GLYPH_H
#include FT_TYPE1_TABLES_H
#include FT_TRUETYPE_TABLES_H
#include FT_TRIGONOMETRY_H
#include FT_BITMAP_H
#include FT_OUTLINE_H
#include FT_WINFONTS_H

#include <gdi/eng/floatobj.h>

#define NDEBUG
#include <debug.h>

#ifndef FT_MAKE_TAG
#define FT_MAKE_TAG( ch0, ch1, ch2, ch3 ) \
       ( ((DWORD)(BYTE)(ch0) << 24) | ((DWORD)(BYTE)(ch1) << 16) | \
       ((DWORD)(BYTE)(ch2) << 8) | (DWORD)(BYTE)(ch3) )
#endif

extern const MATRIX gmxWorldToDeviceDefault;
extern const MATRIX gmxWorldToPageDefault;

// HACK!! Fix XFORMOBJ then use 1:16 / 16:1
#define gmxWorldToDeviceDefault gmxWorldToPageDefault

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
static PFAST_MUTEX FreeTypeLock;

static LIST_ENTRY FontListHead;
static PFAST_MUTEX FontListLock;
static BOOL RenderingEnabled = TRUE;

#define IntLockGlobalFonts \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(FontListLock)

#define IntUnLockGlobalFonts \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(FontListLock)

#define IntLockFreeType \
  ExEnterCriticalRegionAndAcquireFastMutexUnsafe(FreeTypeLock)

#define IntUnLockFreeType \
  ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(FreeTypeLock)

#define MAX_FONT_CACHE 256

typedef struct _FONT_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    int GlyphIndex;
    FT_Face Face;
    FT_BitmapGlyph BitmapGlyph;
    int Height;
    MATRIX mxWorldToDevice;
} FONT_CACHE_ENTRY, *PFONT_CACHE_ENTRY;
static LIST_ENTRY FontCacheListHead;
static UINT FontCacheNumEntries;

static PWCHAR ElfScripts[32] =   /* These are in the order of the fsCsb[0] bits */
{
    L"Western", /* 00 */
    L"Central_European",
    L"Cyrillic",
    L"Greek",
    L"Turkish",
    L"Hebrew",
    L"Arabic",
    L"Baltic",
    L"Vietnamese", /* 08 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 15 */
    L"Thai",
    L"Japanese",
    L"CHINESE_GB2312",
    L"Hangul",
    L"CHINESE_BIG5",
    L"Hangul(Johab)",
    NULL, NULL, /* 23 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    L"Symbol" /* 31 */
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
    /* Reserved for alternate ANSI and OEM */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* Reserved for system */
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
    /* Fast Mutexes must be allocated from non paged pool */
    FontListLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), TAG_INTERNAL_SYNC);
    if (FontListLock == NULL)
    {
        return FALSE;
    }

    ExInitializeFastMutex(FontListLock);
    FreeTypeLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), TAG_INTERNAL_SYNC);
    if (FreeTypeLock == NULL)
    {
        return FALSE;
    }
    ExInitializeFastMutex(FreeTypeLock);

    ulError = FT_Init_FreeType(&library);
    if (ulError)
    {
        DPRINT1("FT_Init_FreeType failed with error code 0x%x\n", ulError);
        return FALSE;
    }

    IntLoadSystemFonts();

    return TRUE;
}

VOID
FtSetCoordinateTransform(
    FT_Face face,
    PMATRIX pmx)
{
    FT_Matrix ftmatrix;
    FLOATOBJ efTemp;

    /* Create a freetype matrix, by converting to 16.16 fixpoint format */
    efTemp = pmx->efM11;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.xx = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM12;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.xy = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM21;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.yx = FLOATOBJ_GetLong(&efTemp);

    efTemp = pmx->efM22;
    FLOATOBJ_MulLong(&efTemp, 0x00010000);
    ftmatrix.yy = FLOATOBJ_GetLong(&efTemp);

    /* Set the transformation matrix */
    FT_Set_Transform(face, &ftmatrix, 0);
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
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
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
        DirInfoBuffer = ExAllocatePoolWithTag(PagedPool, 0x4000, TAG_FONT);
        if (DirInfoBuffer == NULL)
        {
            ZwClose(hDirectory);
            return;
        }

        FileName.Buffer = ExAllocatePoolWithTag(PagedPool, MAX_PATH * sizeof(WCHAR), TAG_FONT);
        if (FileName.Buffer == NULL)
        {
            ExFreePoolWithTag(DirInfoBuffer, TAG_FONT);
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

        ExFreePoolWithTag(FileName.Buffer, TAG_FONT);
        ExFreePoolWithTag(DirInfoBuffer, TAG_FONT);
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
    PVOID SectionObject;
    ULONG ViewSize = 0;
    LARGE_INTEGER SectionSize;
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
    Status = MmCreateSection(&SectionObject, SECTION_ALL_ACCESS,
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
        ObDereferenceObject(SectionObject);
        return 0;
    }

    IntLockFreeType;
    Error = FT_New_Memory_Face(
                library,
                Buffer,
                ViewSize,
                0,
                &Face);
    IntUnLockFreeType;
    ObDereferenceObject(SectionObject);

    if (Error)
    {
        if (Error == FT_Err_Unknown_File_Format)
            DPRINT("Unknown font file format\n");
        else
            DPRINT("Error reading font file (error code: %d)\n", Error);
        return 0;
    }

    Entry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_ENTRY), TAG_FONT);
    if (!Entry)
    {
        FT_Done_Face(Face);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    FontGDI = EngAllocMem(FL_ZERO_MEMORY, sizeof(FONTGDI), GDITAG_RFONT);
    if (FontGDI == NULL)
    {
        FT_Done_Face(Face);
        ExFreePoolWithTag(Entry, TAG_FONT);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    FontGDI->Filename = ExAllocatePoolWithTag(PagedPool, FileName->Length + sizeof(WCHAR), GDITAG_PFF);
    if (FontGDI->Filename == NULL)
    {
        EngFreeMem(FontGDI);
        FT_Done_Face(Face);
        ExFreePoolWithTag(Entry, TAG_FONT);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    RtlCopyMemory(FontGDI->Filename, FileName->Buffer, FileName->Length);
    FontGDI->Filename[FileName->Length / sizeof(WCHAR)] = L'\0';
    FontGDI->face = Face;

    DPRINT("Font loaded: %s (%s)\n", Face->family_name, Face->style_name);
    DPRINT("Num glyphs: %d\n", Face->num_glyphs);

    /* Add this font resource to the font table */

    Entry->Font = FontGDI;
    Entry->NotEnum = (Characteristics & FR_NOT_ENUM);
    RtlInitAnsiString(&AnsiFaceName, (LPSTR)Face->family_name);
    Status = RtlAnsiStringToUnicodeString(&Entry->FaceName, &AnsiFaceName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(FontGDI->Filename, GDITAG_PFF);
        EngFreeMem(FontGDI);
        FT_Done_Face(Face);
        ExFreePoolWithTag(Entry, TAG_FONT);
        return 0;
    }

    if (Characteristics & FR_PRIVATE)
    {
        PPROCESSINFO Win32Process = PsGetCurrentProcessWin32Process();
        IntLockProcessPrivateFonts(Win32Process);
        InsertTailList(&Win32Process->PrivateFontListHead, &Entry->ListEntry);
        IntUnLockProcessPrivateFonts(Win32Process);
    }
    else
    {
        IntLockGlobalFonts;
        InsertTailList(&FontListHead, &Entry->ListEntry);
        InitializeObjectAttributes(&ObjectAttributes, &FontRegPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
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
    PLFONT plfont;

    plfont = LFONT_AllocFontWithHandle();
    if (!plfont)
    {
        return STATUS_NO_MEMORY;
    }

    ExInitializePushLock(&plfont->lock);
    *NewFont = plfont->BaseObject.hHmgr;
    RtlCopyMemory(&plfont->logfont.elfEnumLogfontEx.elfLogFont, lf, sizeof(LOGFONTW));
    if (lf->lfEscapement != lf->lfOrientation)
    {
        /* This should really depend on whether GM_ADVANCED is set */
        plfont->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation =
            plfont->logfont.elfEnumLogfontEx.elfLogFont.lfEscapement;
    }
    LFONT_UnlockFont(plfont);

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
        while (Index < MAXTCIINDEX && 0 == (*Src >> Index & 0x0001))
        {
            Index++;
        }
        break;
    case TCI_SRCCODEPAGE:
        while (Index < MAXTCIINDEX && *Src != FontTci[Index].ciACP)
        {
            Index++;
        }
        break;
    case TCI_SRCCHARSET:
        while (Index < MAXTCIINDEX && *Src != FontTci[Index].ciCharset)
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


static BOOL face_has_symbol_charmap(FT_Face ft_face)
{
    int i;

    for(i = 0; i < ft_face->num_charmaps; i++)
    {
        if(ft_face->charmaps[i]->encoding == FT_ENCODING_MS_SYMBOL)
            return TRUE;
    }
    return FALSE;
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

    if (pOS2->usWinAscent + pOS2->usWinDescent == 0)
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
    TM->tmAscent = (Face->size->metrics.ascender + 32) >> 6; /* Units above baseline */
    TM->tmDescent = (32 - Face->size->metrics.descender) >> 6; /* Units below baseline */
#endif
    TM->tmInternalLeading = (FT_MulFix(Ascent + Descent - Face->units_per_EM, YScale) + 32) >> 6;

    TM->tmHeight = TM->tmAscent + TM->tmDescent;

    /* MSDN says:
     *  el = MAX(0, LineGap - ((WinAscent + WinDescent) - (Ascender - Descender)))
     */
    TM->tmExternalLeading = max(0, (FT_MulFix(pHori->Line_Gap
                                    - ((Ascent + Descent)
                                       - (pHori->Ascender - pHori->Descender)),
                                    YScale) + 32) >> 6);

    TM->tmAveCharWidth = (FT_MulFix(pOS2->xAvgCharWidth, XScale) + 32) >> 6;
    if (TM->tmAveCharWidth == 0)
    {
        TM->tmAveCharWidth = 1;
    }

    /* Correct forumla to get the maxcharwidth from unicode and ansi font */
    TM->tmMaxCharWidth = (FT_MulFix(Face->max_advance_width, XScale) + 32) >> 6;

    TM->tmWeight = pOS2->usWeightClass;
    TM->tmOverhang = 0;
    TM->tmDigitizedAspectX = 96;
    TM->tmDigitizedAspectY = 96;
    if (face_has_symbol_charmap(Face) || (pOS2->usFirstCharIndex >= 0xf000 && pOS2->usFirstCharIndex < 0xf100))
    {
        USHORT cpOEM, cpAnsi;

        EngGetCurrentCodePage(&cpOEM, &cpAnsi);
        TM->tmFirstChar = 0;
        switch(cpAnsi)
        {
        case 1257: /* Baltic */
            TM->tmLastChar = 0xf8fd;
            break;
        default:
            TM->tmLastChar = 0xf0ff;
        }
        TM->tmBreakChar = 0x20;
        TM->tmDefaultChar = 0x1f;
    }
    else
    {
        TM->tmFirstChar = pOS2->usFirstCharIndex; /* Should be the first char in the cmap */
        TM->tmLastChar = pOS2->usLastCharIndex;   /* Should be min(cmap_last, os2_last) */

        if(pOS2->usFirstCharIndex <= 1)
            TM->tmBreakChar = pOS2->usFirstCharIndex + 2;
        else if (pOS2->usFirstCharIndex > 0xff)
            TM->tmBreakChar = 0x20;
        else
            TM->tmBreakChar = pOS2->usFirstCharIndex;
        TM->tmDefaultChar = TM->tmBreakChar - 1;
    }
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
        TM->tmPitchAndFamily |= FF_DECORATIVE;
        break;

    case PAN_ANY:
    case PAN_NO_FIT:
    case PAN_FAMILY_TEXT_DISPLAY:
    case PAN_FAMILY_PICTORIAL: /* Symbol fonts get treated as if they were text */
                               /* Which is clearly not what the panose spec says. */
        if (TM->tmPitchAndFamily == 0) /* Fixed */
        {
            TM->tmPitchAndFamily = FF_MODERN;
        }
        else
        {
            switch (pOS2->panose[PAN_SERIFSTYLE_INDEX])
            {
            case PAN_ANY:
            case PAN_NO_FIT:
            default:
                TM->tmPitchAndFamily |= FF_DONTCARE;
                break;

            case PAN_SERIF_COVE:
            case PAN_SERIF_OBTUSE_COVE:
            case PAN_SERIF_SQUARE_COVE:
            case PAN_SERIF_OBTUSE_SQUARE_COVE:
            case PAN_SERIF_SQUARE:
            case PAN_SERIF_THIN:
            case PAN_SERIF_BONE:
            case PAN_SERIF_EXAGGERATED:
            case PAN_SERIF_TRIANGLE:
                TM->tmPitchAndFamily |= FF_ROMAN;
                break;

            case PAN_SERIF_NORMAL_SANS:
            case PAN_SERIF_OBTUSE_SANS:
            case PAN_SERIF_PERP_SANS:
            case PAN_SERIF_FLARED:
            case PAN_SERIF_ROUNDED:
                TM->tmPitchAndFamily |= FF_SWISS;
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
    NTSTATUS status;

    Needed = sizeof(OUTLINETEXTMETRICW);

    RtlInitAnsiString(&FamilyNameA, FontGDI->face->family_name);
    status = RtlAnsiStringToUnicodeString(&FamilyNameW, &FamilyNameA, TRUE);
    if (!NT_SUCCESS(status))
    {
        return 0;
    }

    RtlInitAnsiString(&StyleNameA, FontGDI->face->style_name);
    status = RtlAnsiStringToUnicodeString(&StyleNameW, &StyleNameA, TRUE);
    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&FamilyNameW);
        return 0;
    }

    /* These names should be read from the TT name table */

    /* Length of otmpFamilyName */
    Needed += FamilyNameW.Length + sizeof(WCHAR);

    RtlInitUnicodeString(&Regular, L"regular");
    /* Length of otmpFaceName */
    if (0 == RtlCompareUnicodeString(&StyleNameW, &Regular, TRUE))
    {
        Needed += FamilyNameW.Length + sizeof(WCHAR); /* Just the family name */
    }
    else
    {
        Needed += FamilyNameW.Length + StyleNameW.Length + (sizeof(WCHAR) << 1); /* family + " " + style */
    }

    /* Length of otmpStyleName */
    Needed += StyleNameW.Length + sizeof(WCHAR);

    /* Length of otmpFullName */
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

    pPost = FT_Get_Sfnt_Table(FontGDI->face, ft_sfnt_post); /* We can live with this failing */

    Error = FT_Get_WinFNT_Header(FontGDI->face , &Win);

    Otm->otmSize = Needed;

    FillTM(&Otm->otmTextMetrics, FontGDI, pOS2, pHori, !Error ? &Win : 0);

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
    Otm->otmMacAscent = Otm->otmTextMetrics.tmAscent;
    Otm->otmMacDescent = -Otm->otmTextMetrics.tmDescent;
    Otm->otmMacLineGap = Otm->otmLineGap;
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
    if (!pPost)
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
    NTSTATUS status;

    Entry = Head->Flink;
    while (Entry != Head)
    {
        CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

        FontGDI = CurrentEntry->Font;
        ASSERT(FontGDI);

        RtlInitAnsiString(&EntryFaceNameA, FontGDI->face->family_name);
        status = RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
        if (!NT_SUCCESS(status))
        {
            break;
        }

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
    PPROCESSINFO Win32Process;
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
    NTSTATUS status;

    RtlZeroMemory(Info, sizeof(FONTFAMILYINFO));
    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    Otm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
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

    ExFreePoolWithTag(Otm, GDITAG_TEXT);

    RtlStringCbCopyW(Info->EnumLogFontEx.elfLogFont.lfFaceName,
                     sizeof(Info->EnumLogFontEx.elfLogFont.lfFaceName),
                     FaceName);
    RtlStringCbCopyW(Info->EnumLogFontEx.elfFullName,
                     sizeof(Info->EnumLogFontEx.elfFullName),
                     FaceName);
    RtlInitAnsiString(&StyleA, FontGDI->face->style_name);
    StyleW.Buffer = Info->EnumLogFontEx.elfStyle;
    StyleW.MaximumLength = sizeof(Info->EnumLogFontEx.elfStyle);
    status = RtlAnsiStringToUnicodeString(&StyleW, &StyleA, FALSE);
    if (!NT_SUCCESS(status))
    {
        return;
    }

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
        { /* Let's see if we can find any interesting cmaps */
            for (i = 0; i < (UINT)FontGDI->face->num_charmaps; i++)
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
                        DPRINT1("Unknown elfscript for bit %u\n", i);
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
    NTSTATUS status;

    Entry = Head->Flink;
    while (Entry != Head)
    {
        CurrentEntry = (PFONT_ENTRY) CONTAINING_RECORD(Entry, FONT_ENTRY, ListEntry);

        FontGDI = CurrentEntry->Font;
        ASSERT(FontGDI);

        RtlInitAnsiString(&EntryFaceNameA, FontGDI->face->family_name);
        status = RtlAnsiStringToUnicodeString(&EntryFaceNameW, &EntryFaceNameA, TRUE);
        if (!NT_SUCCESS(status))
        {
            return FALSE;
        }

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

_Function_class_(RTL_QUERY_REGISTRY_ROUTINE)
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
                RtlStringCbCopyNW(InfoContext->Info[InfoContext->Count].EnumLogFontEx.elfLogFont.lfFaceName,
                                  sizeof(InfoContext->Info[InfoContext->Count].EnumLogFontEx.elfLogFont.lfFaceName),
                                  RegistryName.Buffer,
                                  RegistryName.Length);
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

    /* Enumerate font families found in HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes
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
                                    L"FontSubstitutes",
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
    EngSetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

static
BOOL
SameScaleMatrix(
    PMATRIX pmx1,
    PMATRIX pmx2)
{
    return (FLOATOBJ_Equal(&pmx1->efM11, &pmx2->efM11) &&
            FLOATOBJ_Equal(&pmx1->efM12, &pmx2->efM12) &&
            FLOATOBJ_Equal(&pmx1->efM21, &pmx2->efM21) &&
            FLOATOBJ_Equal(&pmx1->efM22, &pmx2->efM22));
}

FT_BitmapGlyph APIENTRY
ftGdiGlyphCacheGet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height,
    PMATRIX pmx)
{
    PLIST_ENTRY CurrentEntry;
    PFONT_CACHE_ENTRY FontEntry;

    CurrentEntry = FontCacheListHead.Flink;
    while (CurrentEntry != &FontCacheListHead)
    {
        FontEntry = (PFONT_CACHE_ENTRY)CurrentEntry;
        if ((FontEntry->Face == Face) &&
            (FontEntry->GlyphIndex == GlyphIndex) &&
            (FontEntry->Height == Height) &&
            (SameScaleMatrix(&FontEntry->mxWorldToDevice, pmx)))
            break;
        CurrentEntry = CurrentEntry->Flink;
    }

    if (CurrentEntry == &FontCacheListHead)
    {
        return NULL;
    }

    RemoveEntryList(CurrentEntry);
    InsertHeadList(&FontCacheListHead, CurrentEntry);
    return FontEntry->BitmapGlyph;
}

FT_BitmapGlyph APIENTRY
ftGdiGlyphCacheSet(
    FT_Face Face,
    INT GlyphIndex,
    INT Height,
    PMATRIX pmx,
    FT_GlyphSlot GlyphSlot,
    FT_Render_Mode RenderMode)
{
    FT_Glyph GlyphCopy;
    INT error;
    PFONT_CACHE_ENTRY NewEntry;
    FT_Bitmap AlignedBitmap;
    FT_BitmapGlyph BitmapGlyph;

    error = FT_Get_Glyph(GlyphSlot, &GlyphCopy);
    if (error)
    {
        DPRINT1("Failure caching glyph.\n");
        return NULL;
    };

    error = FT_Glyph_To_Bitmap(&GlyphCopy, RenderMode, 0, 1);
    if (error)
    {
        FT_Done_Glyph(GlyphCopy);
        DPRINT1("Failure rendering glyph.\n");
        return NULL;
    };

    NewEntry = ExAllocatePoolWithTag(PagedPool, sizeof(FONT_CACHE_ENTRY), TAG_FONT);
    if (!NewEntry)
    {
        DPRINT1("Alloc failure caching glyph.\n");
        FT_Done_Glyph(GlyphCopy);
        return NULL;
    }

    BitmapGlyph = (FT_BitmapGlyph)GlyphCopy;
    FT_Bitmap_New(&AlignedBitmap);
    if(FT_Bitmap_Convert(GlyphSlot->library, &BitmapGlyph->bitmap, &AlignedBitmap, 4))
    {
        DPRINT1("Conversion failed\n");
        ExFreePoolWithTag(NewEntry, TAG_FONT);
        FT_Done_Glyph((FT_Glyph)BitmapGlyph);
        return NULL;
    }

    FT_Bitmap_Done(GlyphSlot->library, &BitmapGlyph->bitmap);
    BitmapGlyph->bitmap = AlignedBitmap;

    NewEntry->GlyphIndex = GlyphIndex;
    NewEntry->Face = Face;
    NewEntry->BitmapGlyph = BitmapGlyph;
    NewEntry->Height = Height;
    NewEntry->mxWorldToDevice = *pmx;

    InsertHeadList(&FontCacheListHead, &NewEntry->ListEntry);
    if (FontCacheNumEntries++ > MAX_FONT_CACHE)
    {
        NewEntry = (PFONT_CACHE_ENTRY)FontCacheListHead.Blink;
        FT_Done_Glyph((FT_Glyph)NewEntry->BitmapGlyph);
        RemoveTailList(&FontCacheListHead);
        ExFreePoolWithTag(NewEntry, TAG_FONT);
        FontCacheNumEntries--;
    }

    return BitmapGlyph;
}


static void FTVectorToPOINTFX(FT_Vector *vec, POINTFX *pt)
{
    pt->x.value = vec->x >> 6;
    pt->x.fract = (vec->x & 0x3f) << 10;
    pt->x.fract |= ((pt->x.fract >> 6) | (pt->x.fract >> 12));
    pt->y.value = vec->y >> 6;
    pt->y.fract = (vec->y & 0x3f) << 10;
    pt->y.fract |= ((pt->y.fract >> 6) | (pt->y.fract >> 12));
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

static unsigned int get_native_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    int needed = 0, point = 0, contour, first_pt;
    unsigned int pph_start, cpfx;
    DWORD type;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        /* Ignore contours containing one point */
        if (point == outline->contours[contour])
        {
            point++;
            continue;
        }

        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_QSPLINE;
            cpfx = 0;
            do
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            } while (point <= outline->contours[contour] &&
                    (outline->tags[point] & FT_Curve_Tag_On) ==
                    (outline->tags[point-1] & FT_Curve_Tag_On));
            /* At the end of a contour Windows adds the start point, but
               only for Beziers */
            if (point > outline->contours[contour] &&
               !(outline->tags[point-1] & FT_Curve_Tag_On))
            {
                if (buf)
                    FTVectorToPOINTFX(&outline->points[first_pt], &ppc->apfx[cpfx]);
                cpfx++;
            }
            else if (point <= outline->contours[contour] &&
                      outline->tags[point] & FT_Curve_Tag_On)
            {
                /* add closing pt for bezier */
                if (buf)
                    FTVectorToPOINTFX(&outline->points[point], &ppc->apfx[cpfx]);
                cpfx++;
                point++;
            }
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
}

static unsigned int get_bezier_glyph_outline(FT_Outline *outline, unsigned int buflen, char *buf)
{
    /* Convert the quadratic Beziers to cubic Beziers.
       The parametric eqn for a cubic Bezier is, from PLRM:
       r(t) = at^3 + bt^2 + ct + r0
       with the control points:
       r1 = r0 + c/3
       r2 = r1 + (c + b)/3
       r3 = r0 + c + b + a

       A quadratic Bezier has the form:
       p(t) = (1-t)^2 p0 + 2(1-t)t p1 + t^2 p2

       So equating powers of t leads to:
       r1 = 2/3 p1 + 1/3 p0
       r2 = 2/3 p1 + 1/3 p2
       and of course r0 = p0, r3 = p2
    */
    int contour, point = 0, first_pt;
    TTPOLYGONHEADER *pph;
    TTPOLYCURVE *ppc;
    DWORD pph_start, cpfx, type;
    FT_Vector cubic_control[4];
    unsigned int needed = 0;

    for (contour = 0; contour < outline->n_contours; contour++)
    {
        pph_start = needed;
        pph = (TTPOLYGONHEADER *)(buf + needed);
        first_pt = point;
        if (buf)
        {
            pph->dwType = TT_POLYGON_TYPE;
            FTVectorToPOINTFX(&outline->points[point], &pph->pfxStart);
        }
        needed += sizeof(*pph);
        point++;
        while (point <= outline->contours[contour])
        {
            ppc = (TTPOLYCURVE *)(buf + needed);
            type = outline->tags[point] & FT_Curve_Tag_On ?
                TT_PRIM_LINE : TT_PRIM_CSPLINE;
            cpfx = 0;
            do
            {
                if (type == TT_PRIM_LINE)
                {
                    if (buf)
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
                    if (buf)
                    {
                        FTVectorToPOINTFX(&cubic_control[1], &ppc->apfx[cpfx]);
                        FTVectorToPOINTFX(&cubic_control[2], &ppc->apfx[cpfx+1]);
                        FTVectorToPOINTFX(&cubic_control[3], &ppc->apfx[cpfx+2]);
                    }
                    cpfx += 3;
                    point++;
                }
            } while (point <= outline->contours[contour] &&
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
            if (buf)
            {
                ppc->wType = type;
                ppc->cpfx = cpfx;
            }
            needed += sizeof(*ppc) + (cpfx - 1) * sizeof(POINTFX);
        }
        if (buf)
            pph->cb = needed - pph_start;
    }
    return needed;
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
    PDC_ATTR pdcattr;
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

    DPRINT("%u, %08x, %p, %08lx, %p, %p\n", wch, iFormat, pgm,
           cjBuf, pvBuf, pmat2);

    pdcattr = dc->pdcattr;

    MatrixS2XForm(&xForm, &dc->pdcattr->mxWorldToDevice);
    eM11 = xForm.eM11;

    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    FontGDI = ObjToGDI(TextObj->Font, FONT);
    ft_face = FontGDI->face;

    aveWidth = FT_IS_SCALABLE(ft_face) ? TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth: 0;
    orientation = FT_IS_SCALABLE(ft_face) ? TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation: 0;

    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
    if (!potm)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        TEXTOBJ_UnlockText(TextObj);
        return GDI_ERROR;
    }
    IntGetOutlineTextMetrics(FontGDI, Size, potm);

    IntLockFreeType;

    /* During testing, I never saw this used. It is here just in case. */
    if (ft_face->charmap == NULL)
    {
        DPRINT("WARNING: No charmap selected!\n");
        DPRINT("This font face has %d charmaps\n", ft_face->num_charmaps);

        for (n = 0; n < ft_face->num_charmaps; n++)
        {
            charmap = ft_face->charmaps[n];
            DPRINT("Found charmap encoding: %i\n", charmap->encoding);
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

    FT_Set_Pixel_Sizes(ft_face,
                       abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth),
    /* FIXME: Should set character height if neg */
                       (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
                        dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
    FtSetCoordinateTransform(ft_face, DC_pmxWorldToDevice(dc));

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
        if (potm) ExFreePoolWithTag(potm, GDITAG_TEXT);
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
    /*if (aveWidth)*/
    {

        FT_Matrix ftmatrix;
        FLOATOBJ efTemp;

        PMATRIX pmx = DC_pmxWorldToDevice(dc);

        /* Create a freetype matrix, by converting to 16.16 fixpoint format */
        efTemp = pmx->efM11;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM12;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.xy = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM21;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yx = FLOATOBJ_GetLong(&efTemp);

        efTemp = pmx->efM22;
        FLOATOBJ_MulLong(&efTemp, 0x00010000);
        ftmatrix.yy = FLOATOBJ_GetLong(&efTemp);

        FT_Matrix_Multiply(&ftmatrix, &transMat);
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

    if (potm) ExFreePoolWithTag(potm, GDITAG_TEXT); /* It looks like we are finished with potm ATM. */

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

        DPRINT("Transformed box: (%d,%d - %d,%d)\n", left, top, right, bottom);
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

    DPRINT("CX %d CY %d BBX %u BBY %u GOX %d GOY %d\n",
           gm.gmCellIncX, gm.gmCellIncY,
           gm.gmBlackBoxX, gm.gmBlackBoxY,
           gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    IntUnLockFreeType;


    if (iFormat == GGO_METRICS)
    {
        DPRINT("GGO_METRICS Exit!\n");
        *pgm = gm;
        return 1; /* FIXME */
    }

    if (ft_face->glyph->format != ft_glyph_format_outline && iFormat != GGO_BITMAP)
    {
        DPRINT1("Loaded a bitmap\n");
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
        if (!needed) return GDI_ERROR;  /* empty glyph */
        if (needed > cjBuf)
            return GDI_ERROR;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT w = min( pitch, (ft_face->glyph->bitmap.width + 7) >> 3 );
            INT h = min( height, ft_face->glyph->bitmap.rows );
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
            DPRINT1("Loaded glyph format %x\n", ft_face->glyph->format);
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
        if (!needed) return GDI_ERROR;  /* empty glyph */
        if (needed > cjBuf)
            return GDI_ERROR;

        switch (ft_face->glyph->format)
        {
        case ft_glyph_format_bitmap:
        {
            BYTE *src = ft_face->glyph->bitmap.buffer, *dst = pvBuf;
            INT h = min( height, ft_face->glyph->bitmap.rows );
            INT x;
            while (h--)
            {
                for (x = 0; (UINT)x < pitch; x++)
                {
                    if (x < ft_face->glyph->bitmap.width)
                        dst[x] = (src[x / 8] & (1 << ( (7 - (x % 8))))) ? 0xff : 0;
                    else
                        dst[x] = 0;
                }
                src += ft_face->glyph->bitmap.pitch;
                dst += pitch;
            }
            break;
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
        default:
            DPRINT1("Loaded glyph format %x\n", ft_face->glyph->format);
            return GDI_ERROR;
        }
    }

    case GGO_NATIVE:
    {
        FT_Outline *outline = &ft_face->glyph->outline;

        if (cjBuf == 0) pvBuf = NULL; /* This is okay, need cjBuf to allocate. */

        IntLockFreeType;
        if (needsTransform && pvBuf) FT_Outline_Transform(outline, &transMat);

        needed = get_native_glyph_outline(outline, cjBuf, NULL);

        if (!pvBuf || !cjBuf)
        {
            IntUnLockFreeType;
            break;
        }
        if (needed > cjBuf)
        {
            IntUnLockFreeType;
            return GDI_ERROR;
        }
        get_native_glyph_outline(outline, cjBuf, pvBuf);
        IntUnLockFreeType;
        break;
    }
    case GGO_BEZIER:
    {
        FT_Outline *outline = &ft_face->glyph->outline;
        if (cjBuf == 0) pvBuf = NULL;

        if (needsTransform && pvBuf)
        {
            IntLockFreeType;
            FT_Outline_Transform(outline, &transMat);
            IntUnLockFreeType;
        }
        needed = get_bezier_glyph_outline(outline, cjBuf, NULL);

        if (!pvBuf || !cjBuf)
            break;
        if (needed > cjBuf)
            return GDI_ERROR;

        get_bezier_glyph_outline(outline, cjBuf, pvBuf);
        break;
    }

    default:
        DPRINT1("Unsupported format %u\n", iFormat);
        return GDI_ERROR;
    }

    DPRINT("ftGdiGetGlyphOutline END and needed %lu\n", needed);
    *pgm = gm;
    return needed;
}

BOOL
FASTCALL
TextIntGetTextExtentPoint(PDC dc,
                          PTEXTOBJ TextObj,
                          LPCWSTR String,
                          INT Count,
                          ULONG MaxExtent,
                          LPINT Fit,
                          LPINT Dx,
                          LPSIZE Size,
                          FLONG fl)
{
    PFONTGDI FontGDI;
    FT_Face face;
    FT_GlyphSlot glyph;
    FT_BitmapGlyph realglyph;
    INT error, n, glyph_index, i, previous;
    ULONGLONG TotalWidth = 0;
    FT_CharMap charmap, found = NULL;
    BOOL use_kerning;
    FT_Render_Mode RenderMode;
    BOOLEAN Render;
    PMATRIX pmxWorldToDevice;

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
            DPRINT("Found charmap encoding: %i\n", charmap->encoding);
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
                               /* FIXME: Should set character height if neg */
                               (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
							   dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
    if (error)
    {
        DPRINT1("Error in setting pixel sizes: %d\n", error);
    }

    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    for (i = 0; i < Count; i++)
    {
        if (fl & GTEF_INDICES)
            glyph_index = *String;
        else
            glyph_index = FT_Get_Char_Index(face, *String);

        if (!(realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                             TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                             pmxWorldToDevice)))
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("WARNING: Failed to load and render glyph! [index: %d]\n", glyph_index);
                break;
            }

            glyph = face->glyph;
            realglyph = ftGdiGlyphCacheSet(face,
                                           glyph_index,
                                           TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                           pmxWorldToDevice,
                                           glyph,
                                           RenderMode);
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                break;
            }
        }

        /* Retrieve kerning distance */
        if (use_kerning && previous && glyph_index)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TotalWidth += delta.x;
        }

        TotalWidth += realglyph->root.advance.x >> 10;

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
    Size->cy = (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
	            dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight));
    Size->cy = EngMulDiv(Size->cy, dc->ppdev->gdiinfo.ulLogPixelsY, 72);

    return TRUE;
}


INT
FASTCALL
ftGdiGetTextCharsetInfo(
    PDC Dc,
    LPFONTSIGNATURE lpSig,
    DWORD dwFlags)
{
    PDC_ATTR pdcattr;
    UINT Ret = DEFAULT_CHARSET;
    INT i;
    HFONT hFont;
    PTEXTOBJ TextObj;
    PFONTGDI FontGdi;
    FONTSIGNATURE fs;
    TT_OS2 *pOS2;
    FT_Face Face;
    CHARSETINFO csi;
    DWORD cp, fs0;
    USHORT usACP, usOEM;

    pdcattr = Dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
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
    { /* Let's see if we can find any interesting cmaps */
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
                // *cp = csi.ciACP;
                DPRINT("Hit 2\n");
                Ret = csi.ciCharset;
                goto Exit;
            }
            else
                DPRINT1("TCI failing on %x\n", fs0);
        }
    }
Exit:
    DPRINT("CharSet %u CodePage %u\n", csi.ciCharset, csi.ciACP);
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

        DPRINT("Face encoding FT_ENCODING_UNICODE, number of glyphs %ld, first glyph %u, first char %04lx\n",
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
                DPRINT1("Expected increasing char code from FT_Get_Next_Char\n");
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
        DPRINT1("Encoding %i not supported\n", face->charmap->encoding);

    size = sizeof(GLYPHSET) + sizeof(WCRANGE) * (num_ranges - 1);
    if (glyphset)
    {
        glyphset->cbThis = size;
        glyphset->cRanges = num_ranges;
        glyphset->flAccel = 0;
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
    PDC_ATTR pdcattr;
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
        EngSetLastError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(dc = DC_LockDc(hDC)))
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if (NULL != TextObj)
    {
        FontGDI = ObjToGDI(TextObj->Font, FONT);

        Face = FontGDI->face;
        IntLockFreeType;
        Error = FT_Set_Pixel_Sizes(Face,
                                   TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                                   /* FIXME: Should set character height if neg */
                                   (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
                                   dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
        FtSetCoordinateTransform(Face, DC_pmxWorldToDevice(dc));
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
                FillTM(&ptmwi->TextMetric, FontGDI, pOS2, pHori, !Error ? &Win : 0);

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
        static const UNICODE_STRING MarlettFaceNameW = RTL_CONSTANT_STRING(L"Marlett");
        static const UNICODE_STRING SymbolFaceNameW = RTL_CONSTANT_STRING(L"Symbol");
        static const UNICODE_STRING VGAFaceNameW = RTL_CONSTANT_STRING(L"VGA");

        if ((LF_FACESIZE - 1) * sizeof(WCHAR) < EntryFaceNameW.Length)
        {
            EntryFaceNameW.Length = (LF_FACESIZE - 1) * sizeof(WCHAR);
            EntryFaceNameW.Buffer[LF_FACESIZE - 1] = L'\0';
        }

        if (!RtlCompareUnicodeString(FaceName, &EntryFaceNameW, TRUE))
        {
            Score += 49;
        }

        /* FIXME: this is a work around to counter weird fonts on weird places.
           A proper fix would be to score fonts on more attributes than
           the ones in this function */
        if (!RtlCompareUnicodeString(&MarlettFaceNameW, &EntryFaceNameW, TRUE) &&
            RtlCompareUnicodeString(&MarlettFaceNameW, FaceName, TRUE))
        {
            Score = 0;
        }

        if (!RtlCompareUnicodeString(&SymbolFaceNameW, &EntryFaceNameW, TRUE) &&
            RtlCompareUnicodeString(&SymbolFaceNameW, FaceName, TRUE))
        {
            Score = 0;
        }

        if (!RtlCompareUnicodeString(&VGAFaceNameW, &EntryFaceNameW, TRUE) &&
            RtlCompareUnicodeString(&VGAFaceNameW, FaceName, TRUE))
        {
            Score = 0;
        }

        RtlFreeUnicodeString(&EntryFaceNameW);
    }

    Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
    Otm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
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

    ExFreePoolWithTag(Otm, GDITAG_TEXT);

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
ASSERT(FontObj && MatchScore && LogFont && FaceName && Head);
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

    if (SubstituteFontFamilyKey(FaceName, L"FontSubstitutes"))
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
    /* Check for the presence of the 'CFF ' table to check if the font is Type1 */
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
    PPROCESSINFO Win32Process;
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
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenFile(
                 &hFile,
                 0, // FILE_READ_ATTRIBUTES,
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
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Get the full path name */
    if (!IntGetFullFileName(NameInfo1, Size, FileName))
    {
        ExFreePoolWithTag(NameInfo1, TAG_FINF);
        return FALSE;
    }

    /* Create a buffer for the entries' names */
    NameInfo2 = ExAllocatePoolWithTag(PagedPool, Size, TAG_FINF);
    if (!NameInfo2)
    {
        ExFreePoolWithTag(NameInfo1, TAG_FINF);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
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
                    /* Found */
                    FontFamilyFillInfo(&Info, FontEntry->FaceName.Buffer, FontEntry->Font);
                    bFound = TRUE;
                    break;
                }
            }
        }
    }
    IntUnLockGlobalFonts;

    /* Free the buffers */
    ExFreePoolWithTag(NameInfo1, TAG_FINF);
    ExFreePool(NameInfo2);

    if (!bFound && dwType != 5)
    {
        /* Font could not be found in system table
           dwType == 5 will still handle this */
        return FALSE;
    }

    switch (dwType)
    {
    case 0: /* FIXME: Returns 1 or 2, don't know what this is atm */
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

        IntLockFreeType;

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


///////////////////////////////////////////////////////////////////////////
//
// Functions needing sorting.
//
///////////////////////////////////////////////////////////////////////////
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
    PPROCESSINFO Win32Process;

    /* Make a safe copy */
    Status = MmCopyFromCaller(&LogFont, UnsafeLogFont, sizeof(LOGFONTW));
    if (! NT_SUCCESS(Status))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    /* Allocate space for a safe copy */
    Info = ExAllocatePoolWithTag(PagedPool, Size * sizeof(FONTFAMILYINFO), GDITAG_TEXT);
    if (NULL == Info)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return -1;
    }

    /* Enumerate font families in the global list */
    IntLockGlobalFonts;
    Count = 0;
    if (! GetFontFamilyInfoForList(&LogFont, Info, &Count, Size, &FontListHead) )
    {
        IntUnLockGlobalFonts;
        ExFreePoolWithTag(Info, GDITAG_TEXT);
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
        ExFreePoolWithTag(Info, GDITAG_TEXT);
        return -1;
    }
    IntUnLockProcessPrivateFonts(Win32Process);

    /* Enumerate font families in the registry */
    if (! GetFontFamilyInfoForSubstitutes(&LogFont, Info, &Count, Size))
    {
        ExFreePoolWithTag(Info, GDITAG_TEXT);
        return -1;
    }

    /* Return data to caller */
    if (0 != Count)
    {
        Status = MmCopyToCaller(UnsafeInfo, Info,
                                (Count < Size ? Count : Size) * sizeof(FONTFAMILYINFO));
        if (! NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Info, GDITAG_TEXT);
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }
    }

    ExFreePoolWithTag(Info, GDITAG_TEXT);

    return Count;
}

FORCEINLINE
LONG
ScaleLong(LONG lValue, PFLOATOBJ pef)
{
    FLOATOBJ efTemp;

    /* Check if we have scaling different from 1 */
    if (!FLOATOBJ_Equal(pef, (PFLOATOBJ)&gef1))
    {
        /* Need to multiply */
        FLOATOBJ_SetLong(&efTemp, lValue);
        FLOATOBJ_Mul(&efTemp, pef);
        lValue = FLOATOBJ_GetLong(&efTemp);
    }

    return lValue;
}

BOOL
APIENTRY
GreExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL PRECTL lprc,
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
    PDC_ATTR pdcattr;
    SURFOBJ *SurfObj;
    SURFACE *psurf = NULL;
    int error, glyph_index, n, i;
    FT_Face face;
    FT_GlyphSlot glyph;
    FT_BitmapGlyph realglyph;
    LONGLONG TextLeft, RealXStart;
    ULONG TextTop, previous, BackgroundLeft;
    FT_Bool use_kerning;
    RECTL DestRect, MaskRect;
    POINTL SourcePoint, BrushOrigin;
    HBITMAP HSourceGlyph;
    SURFOBJ *SourceGlyphSurf;
    SIZEL bitSize;
    FT_CharMap found = 0, charmap;
    INT yoff;
    FONTOBJ *FontObj;
    PFONTGDI FontGDI;
    PTEXTOBJ TextObj = NULL;
    EXLATEOBJ exloRGB2Dst, exloDst2RGB;
    FT_Render_Mode RenderMode;
    BOOLEAN Render;
    POINT Start;
    BOOL DoBreak = FALSE;
    USHORT DxShift;
    PMATRIX pmxWorldToDevice;
    LONG fixAscender, fixDescender;
    FLOATOBJ Scale;

    // TODO: Write test-cases to exactly match real Windows in different
    // bad parameters (e.g. does Windows check the DC or the RECT first?).
    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(dc);
        /* Yes, Windows really returns TRUE in this case */
        return TRUE;
    }

    pdcattr = dc->pdcattr;

    if ((fuOptions & ETO_OPAQUE) || pdcattr->jBkMode == OPAQUE)
    {
        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);
    }

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && String == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto fail;
    }

    DxShift = fuOptions & ETO_PDY ? 1 : 0;

    if (PATH_IsPathOpen(dc->dclevel))
    {
        if (!PATH_ExtTextOut( dc,
                              XStart,
                              YStart,
                              fuOptions,
                              (const RECTL *)lprc,
                              String,
                              Count,
                              (const INT *)Dx)) goto fail;
        goto good;
    }

    if (lprc && (fuOptions & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        IntLPtoDP(dc, (POINT *)lprc, 2);
    }

    Start.x = XStart;
    Start.y = YStart;
    IntLPtoDP(dc, &Start, 1);

    RealXStart = ((LONGLONG)Start.x + dc->ptlDCOrig.x) << 6;
    YStart = Start.y + dc->ptlDCOrig.y;

    SourcePoint.x = 0;
    SourcePoint.y = 0;
    MaskRect.left = 0;
    MaskRect.top = 0;
    BrushOrigin.x = 0;
    BrushOrigin.y = 0;

    if (!dc->dclevel.pSurface)
    {
        goto fail;
    }

    if ((fuOptions & ETO_OPAQUE) && lprc)
    {
        DestRect.left   = lprc->left;
        DestRect.top    = lprc->top;
        DestRect.right  = lprc->right;
        DestRect.bottom = lprc->bottom;

        DestRect.left   += dc->ptlDCOrig.x;
        DestRect.top    += dc->ptlDCOrig.y;
        DestRect.right  += dc->ptlDCOrig.x;
        DestRect.bottom += dc->ptlDCOrig.y;

        DC_vPrepareDCsForBlit(dc, &DestRect, NULL, NULL);

        if (pdcattr->ulDirty_ & DIRTY_BACKGROUND)
            DC_vUpdateBackgroundBrush(dc);

        psurf = dc->dclevel.pSurface;
        IntEngBitBlt(
            &psurf->SurfObj,
            NULL,
            NULL,
            &dc->co.ClipObj,
            NULL,
            &DestRect,
            &SourcePoint,
            &SourcePoint,
            &dc->eboBackground.BrushObject,
            &BrushOrigin,
            ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
        fuOptions &= ~ETO_OPAQUE;
        DC_vFinishBlit(dc, NULL);
    }
    else
    {
        if (pdcattr->jBkMode == OPAQUE)
        {
            fuOptions |= ETO_OPAQUE;
        }
    }

    TextObj = RealizeFontInit(pdcattr->hlfntNew);
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
            DPRINT("Found charmap encoding: %i\n", charmap->encoding);
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
                /* FIXME: Should set character height if neg */
                (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
                dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
    if (error)
    {
        DPRINT1("Error in setting pixel sizes: %d\n", error);
        IntUnLockFreeType;
        goto fail;
    }

    if (dc->dcattr.iGraphicsMode == GM_ADVANCED)
    {
        pmxWorldToDevice = DC_pmxWorldToDevice(dc);
        FtSetCoordinateTransform(face, pmxWorldToDevice);

        fixAscender = ScaleLong(face->size->metrics.ascender, &pmxWorldToDevice->efM22);
        fixDescender = ScaleLong(face->size->metrics.descender, &pmxWorldToDevice->efM22);
    }
    else
    {
        pmxWorldToDevice = (PMATRIX)&gmxWorldToDeviceDefault;
        FtSetCoordinateTransform(face, pmxWorldToDevice);

        fixAscender = face->size->metrics.ascender;
        fixDescender = face->size->metrics.descender;
    }

    /*
     * Process the vertical alignment and determine the yoff.
     */

    if (pdcattr->lTextAlign & TA_BASELINE)
        yoff = 0;
    else if (pdcattr->lTextAlign & TA_BOTTOM)
        yoff = -fixDescender >> 6;
    else /* TA_TOP */
        yoff = fixAscender >> 6;

    use_kerning = FT_HAS_KERNING(face);
    previous = 0;

    /*
     * Process the horizontal alignment and modify XStart accordingly.
     */

    if (pdcattr->lTextAlign & (TA_RIGHT | TA_CENTER))
    {
        ULONGLONG TextWidth = 0;
        LPCWSTR TempText = String;
        int iStart;

        /*
         * Calculate width of the text.
         */

        if (NULL != Dx)
        {
            iStart = Count < 2 ? 0 : Count - 2;
            TextWidth = Count < 2 ? 0 : (Dx[(Count-2)<<DxShift] << 6);
        }
        else
        {
            iStart = 0;
        }
        TempText = String + iStart;

        for (i = iStart; i < Count; i++)
        {
            if (fuOptions & ETO_GLYPH_INDEX)
                glyph_index = *TempText;
            else
                glyph_index = FT_Get_Char_Index(face, *TempText);

            if (!(realglyph = ftGdiGlyphCacheGet(face, glyph_index,
                                                 TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                                 pmxWorldToDevice)))
            {
                error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                if (error)
                {
                    DPRINT1("WARNING: Failed to load and render glyph! [index: %d]\n", glyph_index);
                }

                glyph = face->glyph;
                realglyph = ftGdiGlyphCacheSet(face,
                                               glyph_index,
                                               TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                               pmxWorldToDevice,
                                               glyph,
                                               RenderMode);
                if (!realglyph)
                {
                    DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                    IntUnLockFreeType;
                    goto fail;
                }

            }
            /* Retrieve kerning distance */
            if (use_kerning && previous && glyph_index)
            {
                FT_Vector delta;
                FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
                TextWidth += delta.x;
            }

            TextWidth += realglyph->root.advance.x >> 10;

            previous = glyph_index;
            TempText++;
        }

        previous = 0;

        if ((pdcattr->lTextAlign & TA_CENTER) == TA_CENTER)
        {
            RealXStart -= TextWidth / 2;
        }
        else
        {
            RealXStart -= TextWidth;
        }
    }

    TextLeft = RealXStart;
    TextTop = YStart;
    BackgroundLeft = (RealXStart + 32) >> 6;

    /* Lock blit with a dummy rect */
    DC_vPrepareDCsForBlit(dc, NULL, NULL, NULL);

    psurf = dc->dclevel.pSurface;
    SurfObj = &psurf->SurfObj ;

    EXLATEOBJ_vInitialize(&exloRGB2Dst, &gpalRGB, psurf->ppal, 0, 0, 0);
    EXLATEOBJ_vInitialize(&exloDst2RGB, psurf->ppal, &gpalRGB, 0, 0, 0);

    if ((fuOptions & ETO_OPAQUE) && (dc->pdcattr->ulDirty_ & DIRTY_BACKGROUND))
        DC_vUpdateBackgroundBrush(dc) ;

    if(dc->pdcattr->ulDirty_ & DIRTY_TEXT)
        DC_vUpdateTextBrush(dc) ;

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
                                             TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                             pmxWorldToDevice)))
        {
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
            if (error)
            {
                DPRINT1("Failed to load and render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }
            glyph = face->glyph;
            realglyph = ftGdiGlyphCacheSet(face,
                                           glyph_index,
                                           TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight,
                                           pmxWorldToDevice,
                                           glyph,
                                           RenderMode);
            if (!realglyph)
            {
                DPRINT1("Failed to render glyph! [index: %d]\n", glyph_index);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }
        }

        /* retrieve kerning distance and move pen position */
        if (use_kerning && previous && glyph_index && NULL == Dx)
        {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 0, &delta);
            TextLeft += delta.x;
        }
        DPRINT("TextLeft: %I64d\n", TextLeft);
        DPRINT("TextTop: %lu\n", TextTop);
        DPRINT("Advance: %d\n", realglyph->root.advance.x);

        if (fuOptions & ETO_OPAQUE)
        {
            DestRect.left = BackgroundLeft;
            DestRect.right = (TextLeft + (realglyph->root.advance.x >> 10) + 32) >> 6;
            DestRect.top = TextTop + yoff - ((fixAscender + 32) >> 6);
            DestRect.bottom = TextTop + yoff + ((32 - fixDescender) >> 6);
            MouseSafetyOnDrawStart(dc->ppdev, DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);
            IntEngBitBlt(
                &psurf->SurfObj,
                NULL,
                NULL,
                &dc->co.ClipObj,
                NULL,
                &DestRect,
                &SourcePoint,
                &SourcePoint,
                &dc->eboBackground.BrushObject,
                &BrushOrigin,
                ROP4_FROM_INDEX(R3_OPINDEX_PATCOPY));
            MouseSafetyOnDrawEnd(dc->ppdev);
            BackgroundLeft = DestRect.right;
        }

        DestRect.left = ((TextLeft + 32) >> 6) + realglyph->left;
        DestRect.right = DestRect.left + realglyph->bitmap.width;
        DestRect.top = TextTop + yoff - realglyph->top;
        DestRect.bottom = DestRect.top + realglyph->bitmap.rows;

        bitSize.cx = realglyph->bitmap.width;
        bitSize.cy = realglyph->bitmap.rows;
        MaskRect.right = realglyph->bitmap.width;
        MaskRect.bottom = realglyph->bitmap.rows;

        /* Check if the bitmap has any pixels */
        if ((bitSize.cx != 0) && (bitSize.cy != 0))
        {
            /*
             * We should create the bitmap out of the loop at the biggest possible
             * glyph size. Then use memset with 0 to clear it and sourcerect to
             * limit the work of the transbitblt.
             */

            HSourceGlyph = EngCreateBitmap(bitSize, realglyph->bitmap.pitch,
                                           BMF_8BPP, BMF_TOPDOWN,
                                           realglyph->bitmap.buffer);
            if ( !HSourceGlyph )
            {
                DPRINT1("WARNING: EngCreateBitmap() failed!\n");
                // FT_Done_Glyph(realglyph);
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }
            SourceGlyphSurf = EngLockSurface((HSURF)HSourceGlyph);
            if ( !SourceGlyphSurf )
            {
                EngDeleteSurface((HSURF)HSourceGlyph);
                DPRINT1("WARNING: EngLockSurface() failed!\n");
                IntUnLockFreeType;
                DC_vFinishBlit(dc, NULL);
                goto fail2;
            }

            /*
             * Use the font data as a mask to paint onto the DCs surface using a
             * brush.
             */

            if (lprc && (fuOptions & ETO_CLIPPED) &&
                    DestRect.right >= lprc->right + dc->ptlDCOrig.x)
            {
                // We do the check '>=' instead of '>' to possibly save an iteration
                // through this loop, since it's breaking after the drawing is done,
                // and x is always incremented.
                DestRect.right = lprc->right + dc->ptlDCOrig.x;
                DoBreak = TRUE;
            }
            if (lprc && (fuOptions & ETO_CLIPPED) &&
                    DestRect.bottom >= lprc->bottom + dc->ptlDCOrig.y)
            {
                DestRect.bottom = lprc->bottom + dc->ptlDCOrig.y;
            }
            MouseSafetyOnDrawStart(dc->ppdev, DestRect.left, DestRect.top, DestRect.right, DestRect.bottom);
            if (!IntEngMaskBlt(
                SurfObj,
                SourceGlyphSurf,
                &dc->co.ClipObj,
                &exloRGB2Dst.xlo,
                &exloDst2RGB.xlo,
                &DestRect,
                (PPOINTL)&MaskRect,
                &dc->eboText.BrushObject,
                &BrushOrigin))
            {
                DPRINT1("Failed to MaskBlt a glyph!\n");
            }

            MouseSafetyOnDrawEnd(dc->ppdev) ;

            EngUnlockSurface(SourceGlyphSurf);
            EngDeleteSurface((HSURF)HSourceGlyph);
        }

        if (DoBreak)
        {
            break;
        }

        if (NULL == Dx)
        {
            TextLeft += realglyph->root.advance.x >> 10;
             DPRINT("New TextLeft: %I64d\n", TextLeft);
        }
        else
        {
            // FIXME this should probably be a matrix transform with TextTop as well.
            Scale = pdcattr->mxWorldToDevice.efM11;
            if (FLOATOBJ_Equal0(&Scale))
                FLOATOBJ_Set1(&Scale);

            FLOATOBJ_MulLong(&Scale, Dx[i<<DxShift] << 6); // do the shift before multiplying to preserve precision
            TextLeft += FLOATOBJ_GetLong(&Scale);
            DPRINT("New TextLeft2: %I64d\n", TextLeft);
        }

        if (DxShift)
        {
            TextTop -= Dx[2 * i + 1] << 6;
        }

        previous = glyph_index;

        String++;
    }
    IntUnLockFreeType;

    DC_vFinishBlit(dc, NULL) ;
    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);
good:
    DC_UnlockDc( dc );

    return TRUE;

fail2:
    EXLATEOBJ_vCleanup(&exloRGB2Dst);
    EXLATEOBJ_vCleanup(&exloDst2RGB);
fail:
    if (TextObj != NULL)
        TEXTOBJ_UnlockText(TextObj);

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
    RECTL SafeRect;
    BYTE LocalBuffer[STACK_TEXT_BUFFER_SIZE];
    PVOID Buffer = LocalBuffer;
    LPWSTR SafeString = NULL;
    LPINT SafeDx = NULL;
    ULONG BufSize, StringSize, DxSize = 0;

    /* Check if String is valid */
    if ((Count > 0xFFFF) || (Count > 0 && UnsafeString == NULL))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
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
            Buffer = ExAllocatePoolWithTag(PagedPool, BufSize, GDITAG_TEXT);
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
        ExFreePoolWithTag(Buffer, GDITAG_TEXT);
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
    IN OPTIONAL PWCHAR UnSafepwch,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    LPABC SafeBuff;
    LPABCFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PMATRIX pmxWorldToDevice;
    PWCHAR Safepwch = NULL;

    if (!Buffer)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (UnSafepwch)
    {
        UINT pwchSize = Count * sizeof(WCHAR);
        Safepwch = ExAllocatePoolWithTag(PagedPool, pwchSize, GDITAG_TEXT);

        if(!Safepwch)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        _SEH2_TRY
        {
            ProbeForRead(UnSafepwch, pwchSize, 1);
            RtlCopyMemory(Safepwch, UnSafepwch, pwchSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    if (!NT_SUCCESS(Status))
    {
        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(ABC); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, GDITAG_TEXT);
    if (!fl) SafeBuffF = (LPABCFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);

    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

        if(Safepwch)
            ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->face;
    if (face->charmap == NULL)
    {
        for (i = 0; i < (UINT)face->num_charmaps; i++)
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
            ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

            if(Safepwch)
                ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    IntLockFreeType;
    FT_Set_Pixel_Sizes(face,
                       TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                       /* FIXME: Should set character height if neg */
                       (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
                       dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        int adv, lsb, bbx, left, right;

        if (Safepwch)
        {
            if (fl & GCABCW_INDICES)
                glyph_index = Safepwch[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, Safepwch[i - FirstChar]);
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
//      DPRINT1("Advance Wine %d and Advance Ros %d\n",test, adv ); /* It's the same! */

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

    ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);

    if(Safepwch)
        ExFreePoolWithTag(Safepwch , GDITAG_TEXT);

    if (! NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

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
    IN OPTIONAL PWCHAR UnSafepwc,
    IN FLONG fl,
    OUT PVOID Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPINT SafeBuff;
    PFLOAT SafeBuffF = NULL;
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    FT_Face face;
    FT_CharMap charmap, found = NULL;
    UINT i, glyph_index, BufferSize;
    HFONT hFont = 0;
    PMATRIX pmxWorldToDevice;
    PWCHAR Safepwc = NULL;

    if (UnSafepwc)
    {
        UINT pwcSize = Count * sizeof(WCHAR);
        Safepwc = ExAllocatePoolWithTag(PagedPool, pwcSize, GDITAG_TEXT);

        if(!Safepwc)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        _SEH2_TRY
        {
            ProbeForRead(UnSafepwc, pwcSize, 1);
            RtlCopyMemory(Safepwc, UnSafepwc, pwcSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(Status);
        return FALSE;
    }

    BufferSize = Count * sizeof(INT); // Same size!
    SafeBuff = ExAllocatePoolWithTag(PagedPool, BufferSize, GDITAG_TEXT);
    if (!fl) SafeBuffF = (PFLOAT) SafeBuff;
    if (SafeBuff == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    /* Get the DC's world-to-device transformation matrix */
    pmxWorldToDevice = DC_pmxWorldToDevice(dc);
    DC_UnlockDc(dc);

    if (TextObj == NULL)
    {
        if(Safepwc)
            ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

        ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);

    face = FontGDI->face;
    if (face->charmap == NULL)
    {
        for (i = 0; i < (UINT)face->num_charmaps; i++)
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

            if(Safepwc)
                ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

            ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
            EngSetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        IntLockFreeType;
        FT_Set_Charmap(face, found);
        IntUnLockFreeType;
    }

    IntLockFreeType;
    FT_Set_Pixel_Sizes(face,
                       TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfWidth,
                       /* FIXME: Should set character height if neg */
                       (TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight == 0 ?
                       dc->ppdev->devinfo.lfDefaultFont.lfHeight : abs(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfHeight)));
    FtSetCoordinateTransform(face, pmxWorldToDevice);

    for (i = FirstChar; i < FirstChar+Count; i++)
    {
        if (Safepwc)
        {
            if (fl & GCW_INDICES)
                glyph_index = Safepwc[i - FirstChar];
            else
                glyph_index = FT_Get_Char_Index(face, Safepwc[i - FirstChar]);
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

    if(Safepwc)
        ExFreePoolWithTag(Safepwc, GDITAG_TEXT);

    ExFreePoolWithTag(SafeBuff, GDITAG_TEXT);
    return TRUE;
}

#if 0
DWORD
FASTCALL
GreGetGlyphIndicesW(
    _In_ HDC hdc,
    _In_reads_(cwc) LPWSTR pwc,
    _In_ INT cwc,
    _Out_writes_opt_(cwc) LPWORD pgi,
    _In_ DWORD iMode,
    _In_ DWORD dwUnknown)
{
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = 0;
    OUTLINETEXTMETRICW *potm;
    INT i;
    FT_Face face;
    WCHAR DefChar = 0xffff;
    PWSTR Buffer = NULL;
    ULONG Size;

    if ((!pwc) && (!pgi)) return cwc;

    dc = DC_LockDc(hdc);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);
    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);
    TEXTOBJ_UnlockText(TextObj);

    Buffer = ExAllocatePoolWithTag(PagedPool, cwc*sizeof(WORD), GDITAG_TEXT);
    if (!Buffer)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return GDI_ERROR;
    }

    if (iMode & GGI_MARK_NONEXISTING_GLYPHS) DefChar = 0x001f;  /* Indicate non existence */
    else
    {
        Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
        potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
        if (!potm)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            cwc = GDI_ERROR;
            goto ErrorRet;
        }
        IntGetOutlineTextMetrics(FontGDI, Size, potm);
        DefChar = potm->otmTextMetrics.tmDefaultChar; // May need this.
        ExFreePoolWithTag(potm, GDITAG_TEXT);
    }

    IntLockFreeType;
    face = FontGDI->face;

    for (i = 0; i < cwc; i++)
    {
        Buffer[i] = FT_Get_Char_Index(face, pwc[i]);
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

    if (pgi != NULL)
    {
        RtlCopyMemory(pgi, Buffer, cwc * sizeof(WORD));
    }

ErrorRet:
    if (Buffer) ExFreePoolWithTag(Buffer, GDITAG_TEXT);
    return cwc;
}
#endif // 0

/*
* @implemented
*/
__kernel_entry
W32KAPI
DWORD
APIENTRY
NtGdiGetGlyphIndicesW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) LPCWSTR pwc,
    _In_ INT cwc,
    _Out_writes_opt_(cwc) LPWORD pgi,
    _In_ DWORD iMode)
{
    PDC dc;
    PDC_ATTR pdcattr;
    PTEXTOBJ TextObj;
    PFONTGDI FontGDI;
    HFONT hFont = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    OUTLINETEXTMETRICW *potm;
    INT i;
    FT_Face face;
    WCHAR DefChar = 0xffff;
    PWSTR Buffer = NULL;
    ULONG Size, pwcSize;
    PWSTR Safepwc = NULL;
    LPCWSTR UnSafepwc = pwc;
    LPWORD UnSafepgi = pgi;

    if ((!UnSafepwc) && (!UnSafepgi)) return cwc;

    if ((UnSafepwc == NULL) || (UnSafepgi == NULL))
    {
        DPRINT1("UnSafepwc == %p, UnSafepgi = %p\n", UnSafepwc, UnSafepgi);
        return -1;
    }

    dc = DC_LockDc(hdc);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    TextObj = RealizeFontInit(hFont);
    DC_UnlockDc(dc);
    if (!TextObj)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }

    FontGDI = ObjToGDI(TextObj->Font, FONT);
    TEXTOBJ_UnlockText(TextObj);

    Buffer = ExAllocatePoolWithTag(PagedPool, cwc*sizeof(WORD), GDITAG_TEXT);
    if (!Buffer)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return GDI_ERROR;
    }

    if (iMode & GGI_MARK_NONEXISTING_GLYPHS) DefChar = 0x001f;  /* Indicate non existence */
    else
    {
        Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
        potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
        if (!potm)
        {
            Status = STATUS_NO_MEMORY;
            goto ErrorRet;
        }
        IntGetOutlineTextMetrics(FontGDI, Size, potm);
        DefChar = potm->otmTextMetrics.tmDefaultChar; // May need this.
        ExFreePoolWithTag(potm, GDITAG_TEXT);
    }

    pwcSize = cwc * sizeof(WCHAR);
    Safepwc = ExAllocatePoolWithTag(PagedPool, pwcSize, GDITAG_TEXT);

    if (!Safepwc)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorRet;
    }

    _SEH2_TRY
    {
        ProbeForRead(UnSafepwc, pwcSize, 1);
        RtlCopyMemory(Safepwc, UnSafepwc, pwcSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status)) goto ErrorRet;

    IntLockFreeType;
    face = FontGDI->face;

    if (DefChar == 0xffff && FT_IS_SFNT(face))
    {
        TT_OS2 *pOS2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        DefChar = (pOS2->usDefaultChar ? FT_Get_Char_Index(face, pOS2->usDefaultChar) : 0);
    }

    for (i = 0; i < cwc; i++)
    {
        Buffer[i] = FT_Get_Char_Index(face, Safepwc[i]);
        if (Buffer[i] == 0)
        {
            Buffer[i] = DefChar;
        }
    }

    IntUnLockFreeType;

    _SEH2_TRY
    {
        ProbeForWrite(UnSafepgi, cwc * sizeof(WORD), 1);
        RtlCopyMemory(UnSafepgi, Buffer, cwc * sizeof(WORD));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

ErrorRet:
    ExFreePoolWithTag(Buffer, GDITAG_TEXT);
    if (Safepwc != NULL)
    {
        ExFreePoolWithTag(Safepwc, GDITAG_TEXT);
    }
    if (NT_SUCCESS(Status)) return cwc;
    SetLastNtError(Status);
    return GDI_ERROR;
}

/* EOF */
