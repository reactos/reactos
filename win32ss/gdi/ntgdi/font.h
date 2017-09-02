#pragma once


typedef struct _FONT_ENTRY
{
    LIST_ENTRY ListEntry;
    FONTGDI *Font;
    UNICODE_STRING FaceName;
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
    UINT Handle;
    FONT_ENTRY_MEM *Entry;
} FONT_ENTRY_COLL_MEM, *PFONT_ENTRY_COLL_MEM;

typedef struct _FONT_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    int GlyphIndex;
    FT_Face Face;
    FT_BitmapGlyph BitmapGlyph;
    int Height;
    MATRIX mxWorldToDevice;
} FONT_CACHE_ENTRY, *PFONT_CACHE_ENTRY;


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
    PFONT_ENTRY_MEM     PrivateEntry;
} GDI_LOAD_FONT, *PGDI_LOAD_FONT;

