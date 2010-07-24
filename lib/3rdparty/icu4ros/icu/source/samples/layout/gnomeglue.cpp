/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include <gnome.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "unicode/utypes.h"
#include "loengine.h"
#include "rsurface.h"
#include "gsupport.h"

#include "gnomeglue.h"

#include "LETypes.h"
#include "LEFontInstance.h"
#include "GnomeGUISupport.h"
#include "GnomeFontMap.h"
#include "GnomeFontInstance.h"
#include "ScriptCompositeFontInstance.h"


U_CDECL_BEGIN

gs_guiSupport *gs_gnomeGuiSupportOpen()
{
    return (gs_guiSupport *) new GnomeGUISupport();
}

void gs_gnomeGuiSupportClose(gs_guiSupport *guiSupport)
{
    GnomeGUISupport *gs = (GnomeGUISupport *) guiSupport;

    delete gs;
}

rs_surface *rs_gnomeRenderingSurfaceOpen(GtkWidget *theWidget)
{
    return (rs_surface *) new GnomeSurface(theWidget);
}

void rs_gnomeRenderingSurfaceClose(rs_surface *surface)
{
    GnomeSurface *rs = (GnomeSurface *) surface;

    delete rs;
}

fm_fontMap *fm_gnomeFontMapOpen(FT_Library engine, const char *fileName, le_int16 pointSize, gs_guiSupport *guiSupport, LEErrorCode *status)
{
    return (fm_fontMap *) new GnomeFontMap(engine, fileName, pointSize, (GnomeGUISupport *) guiSupport, *status);
}

void fm_fontMapClose(fm_fontMap *fontMap)
{
    GnomeFontMap *fm = (GnomeFontMap *) fontMap;

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
