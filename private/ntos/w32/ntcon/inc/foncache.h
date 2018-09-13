/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    foncache.h

Abstract:

        This file is definition of fullscreen dll

Author:

    Kazuhiko  Matsubara  21-June-1994

Revision History:

Notes:

--*/



typedef struct _FONT_IMAGE {
    LIST_ENTRY ImageList;                            // link list of other font size.
    COORD FontSize;
    PBYTE ImageBits;                                 // WORD aligned.
} FONT_IMAGE, *PFONT_IMAGE;

typedef struct _FONT_LOW_OFFSET {
    PFONT_IMAGE FontOffsetLow[256];                  // array is low order of Unicode <i.e LOBYTE(Unicode)>
} FONT_LOW_OFFSET, *PFONT_LOW_OFFSET;

typedef struct _FONT_HIGHLOW_OFFSET {
    PFONT_LOW_OFFSET  FontOffsetHighLow[16];         // array is high (3-0bit) order of Unicode <i.e LO4BIT(HIBYTE(Unicode))>
} FONT_HIGHLOW_OFFSET, *PFONT_HIGHLOW_OFFSET;

typedef struct _FONT_HIGHHIGH_OFFSET {
    PFONT_HIGHLOW_OFFSET FontOffsetHighHigh[16];     // array is high (7-4bit) order of Unicode <i.e HI4BIT(HIBYTE(Unicode))>
} FONT_HIGHHIGH_OFFSET, *PFONT_HIGHHIGH_OFFSET;



typedef struct _FONT_CACHE_INFORMATION {
    ULONG  FullScreenFontIndex;
    COORD  FullScreenFontSize;
    PBYTE  BaseImageBits;
    FONT_HIGHHIGH_OFFSET FontTable;
} FONT_CACHE_INFORMATION, *PFONT_CACHE_INFORMATION;


#define FONT_MATCHED   1
#define FONT_STRETCHED 2

#define ADD_IMAGE     1
#define REPLACE_IMAGE 2

#define BITMAP_BITS_BYTE_ALIGN   8                   // BYTE align is 8 bit
#define BITMAP_BITS_WORD_ALIGN  16                   // WORD align is 16 bit
#define BITMAP_ARRAY_BYTE  3                         // BYTE array is 8 bit  (shift count = 3)


typedef struct _FONT_CACHE_AREA {
    PFONT_IMAGE FontImage;
    DWORD       Area;
} FONT_CACHE_AREA, *PFONT_CACHE_AREA;


#define BITMAP_PLANES      1
#define BITMAP_BITS_PIXEL  1


#define BYTE_ALIGN  sizeof(BYTE)
#define WORD_ALIGN  sizeof(WORD)

//
// Font cache manager
//
ULONG
CreateFontCache(
    OUT PFONT_CACHE_INFORMATION *FontCache
    );

ULONG
DestroyFontCache(
    IN PFONT_CACHE_INFORMATION FontCache
    );

ULONG
GetFontImage(
    IN PFONT_CACHE_INFORMATION FontCache,
    IN WCHAR wChar,
    IN COORD FontSize,
    IN DWORD dwAlign,
    OUT VOID *ImageBits
    );

ULONG
GetStretchedFontImage(
    IN PFONT_CACHE_INFORMATION FontCache,
    IN WCHAR wChar,
    IN COORD FontSize,
    IN DWORD dwAlign,
    OUT VOID *ImageBits
    );

ULONG
GetFontImagePointer(
    IN PFONT_CACHE_INFORMATION FontCache,
    IN WCHAR wChar,
    IN COORD FontSize,
    OUT PFONT_IMAGE *FontImage
    );

ULONG
SetFontImage(
    IN PFONT_CACHE_INFORMATION FontCache,
    IN WCHAR wChar,
    IN COORD FontSize,
    IN DWORD dwAlign,
    IN CONST VOID *ImageBits
    );

DWORD
CalcBitmapBufferSize(
    IN COORD FontSize,
    IN DWORD dwAlign
    );

NTSTATUS
GetExpandFontImage(
    PFONT_CACHE_INFORMATION FontCache,
    WCHAR wChar,
    COORD InputFontSize,
    COORD OutputFontSize,
    PWORD OutputFontImage
    );
