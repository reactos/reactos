/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include <windows.h>

#include "unicode/utypes.h"
#include "loengine.h"
#include "rsurface.h"
#include "gsupport.h"

#include "gdiglue.h"

#include "LETypes.h"
#include "LEFontInstance.h"
#include "GDIGUISupport.h"
#include "GDIFontMap.h"
#include "ScriptCompositeFontInstance.h"


U_CDECL_BEGIN

gs_guiSupport *gs_gdiGuiSupportOpen()
{
    return (gs_guiSupport *) new GDIGUISupport();
}

void gs_gdiGuiSupportClose(gs_guiSupport *guiSupport)
{
    GDIGUISupport *gs = (GDIGUISupport *) guiSupport;

    delete gs;
}

rs_surface *rs_gdiRenderingSurfaceOpen(HDC hdc)
{
    return (rs_surface *) new GDISurface(hdc);
}

void rs_gdiRenderingSurfaceSetHDC(rs_surface *surface, HDC hdc)
{
    GDISurface *rs = (GDISurface *) surface;

    rs->setHDC(hdc);
}

void rs_gdiRenderingSurfaceClose(rs_surface *surface)
{
    GDISurface *rs = (GDISurface *) surface;

    delete rs;
}

fm_fontMap *fm_gdiFontMapOpen(rs_surface *surface, const char *fileName, le_int16 pointSize, gs_guiSupport *guiSupport, LEErrorCode *status)
{
    return (fm_fontMap *) new GDIFontMap((GDISurface *) surface, fileName, pointSize, (GDIGUISupport *) guiSupport, *status);
}

void fm_fontMapClose(fm_fontMap *fontMap)
{
    GDIFontMap *fm = (GDIFontMap *) fontMap;

    delete fm;
}

le_font *le_scriptCompositeFontOpen(fm_fontMap *fontMap)
{
    return (le_font *) new ScriptCompositeFontInstance((FontMap *) fontMap);
}

void le_fontClose(le_font *font)
{
    LEFontInstance *fi = (LEFontInstance *) font;

    delete fi;
}

U_CDECL_END
