#pragma once


typedef struct _FONT_ENTRY
{
    LIST_ENTRY ListEntry;
    FONTGDI *Font;
    UNICODE_STRING FaceName;
    UNICODE_STRING StyleName;
    BYTE NotEnum;
} FONT_ENTRY, *PFONT_ENTRY;

typedef struct _FONT_ENTRY_MEM
{
    LIST_ENTRY ListEntry;
    FONT_ENTRY *Entry;
} FONT_ENTRY_MEM, *PFONT_ENTRY_MEM;

typedef struct _FONT_ENTRY_COLL_MEM
{
    LIST_ENTRY ListEntry;
    HANDLE Handle;
    FONT_ENTRY_MEM *Entry;
} FONT_ENTRY_COLL_MEM, *PFONT_ENTRY_COLL_MEM;

typedef struct _EMULATION_BOLD_ITALIC
{
    BOOLEAN Bold;
    BOOLEAN Italic;
} EMULATION_BOLD_ITALIC, *PEMULATION_BOLD_ITALIC;

#include <pshpack1.h> /* We don't like padding for this structure */
typedef struct _FONT_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    FT_BitmapGlyph BitmapGlyph;
    DWORD dwHash;

    /* The following members are hashed */
    INT GlyphIndex;
    FT_Face Face;
    LONG lfHeight;
    _ANONYMOUS_UNION union {
        EMULATION_BOLD_ITALIC Emu;
        WORD EmuBoldItalic;
    } DUMMYUNIONNAME;
    WORD RenderMode;
    FT_Matrix matTransform;
} FONT_CACHE_ENTRY, *PFONT_CACHE_ENTRY;
#include <poppack.h>

C_ASSERT(offsetof(FONT_CACHE_ENTRY, GlyphIndex) % sizeof(DWORD) == 0);
C_ASSERT(sizeof(FONT_CACHE_ENTRY) % sizeof(DWORD) == 0);

/*
 * FONTSUBST_... --- constants for font substitutes
 */
#define FONTSUBST_FROM          0
#define FONTSUBST_TO            1
#define FONTSUBST_FROM_AND_TO   2

/*
 * FONTSUBST_ENTRY --- font substitute entry
 */
typedef struct FONTSUBST_ENTRY
{
    LIST_ENTRY      ListEntry;
    UNICODE_STRING  FontNames[FONTSUBST_FROM_AND_TO];
    BYTE            CharSets[FONTSUBST_FROM_AND_TO];
} FONTSUBST_ENTRY, *PFONTSUBST_ENTRY;


typedef struct GDI_LOAD_FONT
{
    PUNICODE_STRING     pFileName;
    PSHARED_MEM         Memory;
    DWORD               Characteristics;
    UNICODE_STRING      RegValueName;
    BOOL                IsTrueType;
    BYTE                CharSet;
    PFONT_ENTRY_MEM     PrivateEntry;
} GDI_LOAD_FONT, *PGDI_LOAD_FONT;

