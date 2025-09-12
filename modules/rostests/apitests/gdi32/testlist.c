#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_AddFontMemResourceEx(void);
extern void func_AddFontResource(void);
extern void func_AddFontResourceEx(void);
extern void func_BeginPath(void);
extern void func_CombineRgn(void);
extern void func_CombineTransform(void);
extern void func_CreateBitmap(void);
extern void func_CreateBitmapIndirect(void);
extern void func_CreateCompatibleDC(void);
extern void func_CreateDIBitmap(void);
extern void func_CreateDIBPatternBrush(void);
extern void func_CreateFont(void);
extern void func_CreateFontIndirect(void);
extern void func_CreateIconIndirect(void);
extern void func_CreatePen(void);
extern void func_CreateRectRgn(void);
extern void func_DPtoLP(void);
extern void func_EngAcquireSemaphore(void);
extern void func_EngCreateSemaphore(void);
extern void func_EngReleaseSemaphore(void);
extern void func_EnumFontFamilies(void);
extern void func_ExcludeClipRect(void);
extern void func_ExtCreatePen(void);
extern void func_ExtCreateRegion(void);
extern void func_FrameRgn(void);
extern void func_GdiConvertBitmap(void);
extern void func_GdiConvertBrush(void);
extern void func_GdiConvertDC(void);
extern void func_GdiConvertFont(void);
extern void func_GdiConvertPalette(void);
extern void func_GdiConvertRegion(void);
extern void func_GdiDeleteLocalDC(void);
extern void func_GdiGetCharDimensions(void);
extern void func_GdiGetLocalBrush(void);
extern void func_GdiGetLocalDC(void);
extern void func_GdiReleaseLocalDC(void);
extern void func_GdiSetAttrs(void);
extern void func_GetClipBox(void);
extern void func_GetClipRgn(void);
extern void func_GetCurrentObject(void);
extern void func_GetDIBColorTable(void);
extern void func_GetDIBits(void);
extern void func_GetFontResourceInfoW(void);
extern void func_GetGlyphIndices(void);
extern void func_GetGlyphOutline(void);
extern void func_GetObject(void);
extern void func_GetPixel(void);
extern void func_GetRandomRgn(void);
extern void func_GetStockObject(void);
extern void func_GetTextExtentExPoint(void);
extern void func_GetTextFace(void);
extern void func_GetTextMetrics(void);
extern void func_LPtoDP(void);
extern void func_MaskBlt(void);
extern void func_NtGdiAddFontResource(void);
extern void func_OffsetClipRgn(void);
extern void func_OffsetRgn(void);
extern void func_PaintRgn(void);
extern void func_PatBlt(void);
extern void func_Rectangle(void);
extern void func_RealizePalette(void);
extern void func_SelectObject(void);
extern void func_SetBoundsRect(void);
extern void func_SetBrushOrgEx(void);
extern void func_SetDCPenColor(void);
extern void func_SetDIBits(void);
extern void func_SetDIBitsToDevice(void);
extern void func_SetLayout(void);
extern void func_SetMapMode(void);
extern void func_SetPixel(void);
extern void func_SetSysColors(void);
extern void func_SetWindowExtEx(void);
extern void func_SetWorldTransform(void);
extern void func_StretchBlt(void);
extern void func_TextTransform(void);

const struct test winetest_testlist[] =
{
    { "AddFontMemResourceEx", func_AddFontMemResourceEx },
    { "AddFontResource", func_AddFontResource },
    { "AddFontResourceEx", func_AddFontResourceEx },
    { "BeginPath", func_BeginPath },
    { "CombineRgn", func_CombineRgn },
    { "CombineTransform", func_CombineTransform },
    { "CreateBitmap", func_CreateBitmap },
    { "CreateBitmapIndirect", func_CreateBitmapIndirect },
    { "CreateCompatibleDC", func_CreateCompatibleDC },
    { "CreateDIBitmap", func_CreateDIBitmap },
    { "CreateDIBPatternBrush", func_CreateDIBPatternBrush },
    { "CreateFont", func_CreateFont },
    { "CreateFontIndirect", func_CreateFontIndirect },
    { "CreateIconIndirect", func_CreateIconIndirect },
    { "CreatePen", func_CreatePen },
    { "CreateRectRgn", func_CreateRectRgn },
    { "DPtoLP", func_DPtoLP },
    { "EngAcquireSemaphore", func_EngAcquireSemaphore },
    { "EngCreateSemaphore", func_EngCreateSemaphore },
    { "EngReleaseSemaphore", func_EngReleaseSemaphore },
    { "EnumFontFamilies", func_EnumFontFamilies },
    { "ExcludeClipRect", func_ExcludeClipRect },
    { "ExtCreatePen", func_ExtCreatePen },
    { "ExtCreateRegion", func_ExtCreateRegion },
    { "FrameRgn", func_FrameRgn },
    { "GdiConvertBitmap", func_GdiConvertBitmap },
    { "GdiConvertBrush", func_GdiConvertBrush },
    { "GdiConvertDC", func_GdiConvertDC },
    { "GdiConvertFont", func_GdiConvertFont },
    { "GdiConvertPalette", func_GdiConvertPalette },
    { "GdiConvertRegion", func_GdiConvertRegion },
    { "GdiDeleteLocalDC", func_GdiDeleteLocalDC },
    { "GdiGetCharDimensions", func_GdiGetCharDimensions },
    { "GdiGetLocalBrush", func_GdiGetLocalBrush },
    { "GdiGetLocalDC", func_GdiGetLocalDC },
    { "GdiReleaseLocalDC", func_GdiReleaseLocalDC },
    { "GdiSetAttrs", func_GdiSetAttrs },
    { "GetClipBox", func_GetClipBox },
    { "GetClipRgn", func_GetClipRgn },
    { "GetCurrentObject", func_GetCurrentObject },
    { "GetDIBColorTable", func_GetDIBColorTable },
    { "GetDIBits", func_GetDIBits },
    { "GetFontResourceInfoW", func_GetFontResourceInfoW },
    { "GetGlyphIndices", func_GetGlyphIndices },
    { "GetGlyphOutline", func_GetGlyphOutline },
    { "GetPixel", func_GetPixel },
    { "GetObject", func_GetObject },
    { "GetRandomRgn", func_GetRandomRgn },
    { "GetStockObject", func_GetStockObject },
    { "GetTextExtentExPoint", func_GetTextExtentExPoint },
    { "GetTextMetrics", func_GetTextMetrics },
    { "GetTextFace", func_GetTextFace },
    { "LPtoDP", func_LPtoDP },
    { "MaskBlt", func_MaskBlt },
    { "NtGdiAddFontResource", func_NtGdiAddFontResource},
    { "OffsetClipRgn", func_OffsetClipRgn },
    { "OffsetRgn", func_OffsetRgn },
    { "PaintRgn", func_PaintRgn },
    { "PatBlt", func_PatBlt },
    { "Rectangle", func_Rectangle },
    { "RealizePalette", func_RealizePalette },
    { "SelectObject", func_SelectObject },
    { "SetBoundsRect", func_SetBoundsRect },
    { "SetBrushOrgEx", func_SetBrushOrgEx },
    { "SetDCPenColor", func_SetDCPenColor },
    { "SetDIBits", func_SetDIBits },
    { "SetDIBitsToDevice", func_SetDIBitsToDevice },
    { "SetLayout", func_SetLayout },
    { "SetMapMode", func_SetMapMode },
    { "SetPixel", func_SetPixel },
    { "SetSysColors", func_SetSysColors },
    { "SetWindowExtEx", func_SetWindowExtEx },
    { "SetWorldTransform", func_SetWorldTransform },
    { "StretchBlt", func_StretchBlt },
    { "TextTransform", func_TextTransform },

    { 0, 0 }
};

