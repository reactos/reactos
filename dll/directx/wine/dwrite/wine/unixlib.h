/*
 * PROJECT:     ReactOS dwrite DLL
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactOS emulation layer for dwrite unixlib calls
 * COPYRIGHT:   Copyright 2026 Mikhail Tyukin <mishakeys20@gmail.com>
 */

#pragma once

#include "dwrite_private.h"

#define __wine_init_unix_call() 0
#define dlclose(x) 0
#define dlsym() 0

// Define freetype functions
#define pFT_Activate_Size FT_Activate_Size
#define pFT_Done_Face FT_Done_Face
#define pFT_Done_FreeType FT_Done_FreeType
#define pFT_Done_Glyph FT_Done_Glyph
#define pFT_Done_Size FT_Done_Size
#define pFT_Get_First_Char FT_Get_First_Char
#define pFT_Get_Glyph FT_Get_Glyph
#define pFT_Get_Kerning FT_Get_Kerning
#define pFT_Get_Sfnt_Table FT_Get_Sfnt_Table
#define pFT_Glyph_Copy FT_Glyph_Copy
#define pFT_Glyph_Get_CBox FT_Glyph_Get_CBox
#define pFT_Glyph_Transform FT_Glyph_Transform
#define pFT_Init_FreeType FT_Init_FreeType
#define pFT_Library_Version FT_Library_Version
#define pFT_Load_Glyph FT_Load_Glyph
#define pFT_Matrix_Multiply FT_Matrix_Multiply
#define pFT_MulDiv FT_MulDiv
#define pFT_New_Memory_Face FT_New_Memory_Face
#define pFT_New_Size FT_New_Size
#define pFT_Outline_Copy FT_Outline_Copy
#define pFT_Outline_Decompose FT_Outline_Decompose
#define pFT_Outline_Done FT_Outline_Done
#define pFT_Outline_Embolden FT_Outline_Embolden
#define pFT_Outline_Get_Bitmap FT_Outline_Get_Bitmap
#define pFT_Outline_New FT_Outline_New
#define pFT_Outline_Transform FT_Outline_Transform
#define pFT_Outline_Translate FT_Outline_Translate
#define pFT_Set_Pixel_Sizes FT_Set_Pixel_Sizes

typedef NTSTATUS (*unixlib_entry_t)( void *args );

// Defined in freetype.c
extern const unixlib_entry_t __wine_unix_call_funcs[];
extern const unixlib_entry_t __wine_unix_call_wow64_funcs[];

static inline int __reactos_call_unix_process_attach(PVOID args)
{
    return __wine_unix_call_funcs[0](args);
}

static inline int __reactos_call_unix_process_detach(PVOID args)
{
    return __wine_unix_call_funcs[1](args);
}

static inline int __reactos_call_unix_create_font_object(PVOID args)
{
    return __wine_unix_call_funcs[2](args);
}

static inline int __reactos_call_unix_release_font_object(PVOID args)
{
    return __wine_unix_call_funcs[3](args);
}

static inline int __reactos_call_unix_get_glyph_outline(PVOID args)
{
    return __wine_unix_call_funcs[4](args);
}

static inline int __reactos_call_unix_get_glyph_count(PVOID args)
{
    return __wine_unix_call_funcs[5](args);
}

static inline int __reactos_call_unix_get_glyph_advance(PVOID args)
{
    return __wine_unix_call_funcs[6](args);
}

static inline int __reactos_call_unix_get_glyph_bbox(PVOID args)
{
    return __wine_unix_call_funcs[7](args);
}

static inline int __reactos_call_unix_get_glyph_bitmap(PVOID args)
{
    return __wine_unix_call_funcs[8](args);
}

static inline int __reactos_call_unix_get_design_glyph_metrics(PVOID args)
{
    return __wine_unix_call_funcs[9](args);
}

// Forward wine unix call to real freetype library
#undef WINE_UNIX_CALL
#define WINE_UNIX_CALL(code,args) __reactos_call_ ## code(args)
