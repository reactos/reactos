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

#include <pshpack1.h> /* We don't like padding for these structures for hashing */

typedef struct _FONT_ASPECT
{
    _ANONYMOUS_UNION union {
        WORD EmuBoldItalic;
        struct {
            BYTE Bold;
            BYTE Italic;
        } Emu;
    } DUMMYUNIONNAME;
    WORD RenderMode;
} FONT_ASPECT, *PFONT_ASPECT;

typedef struct _FONT_CACHE_HASHED
{
    INT GlyphIndex;
    FT_Face Face;
    LONG lfHeight;
    LONG lfWidth;
    _ANONYMOUS_UNION union {
        DWORD AspectValue;
        FONT_ASPECT Aspect;
    } DUMMYUNIONNAME;
    FT_Matrix matTransform;
} FONT_CACHE_HASHED, *PFONT_CACHE_HASHED;

#include <poppack.h>

typedef struct _FONT_CACHE_ENTRY
{
    LIST_ENTRY ListEntry;
    FT_BitmapGlyph BitmapGlyph;
    DWORD dwHash;
    FONT_CACHE_HASHED Hashed;
} FONT_CACHE_ENTRY, *PFONT_CACHE_ENTRY;

C_ASSERT(FIELD_OFFSET(FONT_CACHE_ENTRY, Hashed) % sizeof(DWORD) == 0); /* for hashing */
C_ASSERT(sizeof(FONT_CACHE_HASHED) % sizeof(DWORD) == 0); /* for hashing */

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

