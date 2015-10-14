/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winnls.h"
#include "winerror.h"
#include "wine/winternl.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define HGDIOBJ_32(h16)   ((HGDIOBJ)(ULONG_PTR)(h16))

#define GDI_HEAP_SIZE 0xffe0

/***********************************************************************
 *          GDI stock objects
 */

static const LOGBRUSH WhiteBrush = { BS_SOLID, RGB(255,255,255), 0 };
static const LOGBRUSH BlackBrush = { BS_SOLID, RGB(0,0,0), 0 };
static const LOGBRUSH NullBrush  = { BS_NULL, 0, 0 };

static const LOGBRUSH LtGrayBrush = { BS_SOLID, RGB(192,192,192), 0 };
static const LOGBRUSH GrayBrush   = { BS_SOLID, RGB(128,128,128), 0 };
static const LOGBRUSH DkGrayBrush = { BS_SOLID, RGB(64,64,64), 0 };

static const LOGPEN WhitePen = { PS_SOLID, { 0, 0 }, RGB(255,255,255) };
static const LOGPEN BlackPen = { PS_SOLID, { 0, 0 }, RGB(0,0,0) };
static const LOGPEN NullPen  = { PS_NULL,  { 0, 0 }, 0 };

static const LOGBRUSH DCBrush = { BS_SOLID, RGB(255,255,255), 0 };
static const LOGPEN DCPen     = { PS_SOLID, { 0, 0 }, RGB(0,0,0) };

/* reserve one extra entry for the stock default bitmap */
/* this is what Windows does too */
#define NB_STOCK_OBJECTS (STOCK_LAST+2)

static HGDIOBJ stock_objects[NB_STOCK_OBJECTS];

static CRITICAL_SECTION gdi_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &gdi_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": gdi_section") }
};
static CRITICAL_SECTION gdi_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/****************************************************************************
 *
 *	language-independent stock fonts
 *
 */

static const LOGFONTW OEMFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, {'\0'} };

static const LOGFONTW AnsiFixedFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
  {'C','o','u','r','i','e','r','\0'} };

static const LOGFONTW AnsiVarFont =
{ 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
  0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
  {'M','S',' ','S','a','n','s',' ','S','e','r','i','f','\0'} };

/******************************************************************************
 *
 *      language-dependent stock fonts
 *
 *      'ANSI' charset and 'DEFAULT' charset is not same.
 *      The chars in CP_ACP should be drawn with 'DEFAULT' charset.
 *      'ANSI' charset seems to be identical with ISO-8859-1.
 *      'DEFAULT' charset is a language-dependent charset.
 *
 *      'System' font seems to be an alias for language-dependent font.
 */

/*
 * language-dependent stock fonts for all known charsets
 * please see TranslateCharsetInfo (dlls/gdi/font.c) and
 * CharsetBindingInfo (dlls/x11drv/xfont.c),
 * and modify entries for your language if needed.
 */
struct DefaultFontInfo
{
        UINT            charset;
        LOGFONTW        SystemFont;
        LOGFONTW        DeviceDefaultFont;
        LOGFONTW        SystemFixedFont;
        LOGFONTW        DefaultGuiFont; /* Note for this font the lfHeight member should be the point size */
};

static const struct DefaultFontInfo default_fonts[] =
{
    {   ANSI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   EASTEUROPE_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   RUSSIAN_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   GREEK_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GREEK_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   TURKISH_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, TURKISH_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   HEBREW_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HEBREW_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   ARABIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ARABIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   BALTIC_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, BALTIC_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   THAI_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, THAI_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   SHIFTJIS_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   GB2312_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   HANGEUL_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   CHINESEBIG5_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, CHINESEBIG5_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
    {   JOHAB_CHARSET,
        { /* System */
          16, 7, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'S','y','s','t','e','m','\0'}
        },
        { /* Device Default */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'\0'}
        },
        { /* System Fixed */
          16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
           {'\0'}
        },
        { /* DefaultGuiFont */
           8, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, JOHAB_CHARSET,
           0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS,
           {'M','S',' ','S','h','e','l','l',' ','D','l','g','\0'}
        },
    },
};


/*************************************************************************
 * __wine_make_gdi_object_system    (GDI32.@)
 *
 * USER has to tell GDI that its system brushes and pens are non-deletable.
 * For a description of the GDI object magics and their flags,
 * see "Undocumented Windows" (wrong about the OBJECT_NOSYSTEM flag, though).
 */
void CDECL __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set)
{
    GDIOBJHDR *ptr = GDI_GetObjPtr( handle, 0 );
    ptr->system = !!set;
    GDI_ReleaseObj( handle );
}

/******************************************************************************
 *      get_default_fonts
 */
static const struct DefaultFontInfo* get_default_fonts(UINT charset)
{
        unsigned int n;

        for(n=0;n<(sizeof(default_fonts)/sizeof(default_fonts[0]));n++)
        {
                if ( default_fonts[n].charset == charset )
                        return &default_fonts[n];
        }

        FIXME( "unhandled charset 0x%08x - use ANSI_CHARSET for default stock objects\n", charset );
        return &default_fonts[0];
}


/******************************************************************************
 *      get_default_charset    (internal)
 *
 * get the language-dependent charset that can handle CP_ACP correctly.
 */
static UINT get_default_charset( void )
{
    CHARSETINFO     csi;
    UINT    uACP;

    uACP = GetACP();
    csi.ciCharset = ANSI_CHARSET;
    if ( !TranslateCharsetInfo( ULongToPtr(uACP), &csi, TCI_SRCCODEPAGE ) )
    {
        FIXME( "unhandled codepage %u - use ANSI_CHARSET for default stock objects\n", uACP );
        return ANSI_CHARSET;
    }

    return csi.ciCharset;
}

static const WCHAR dpi_key_name[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
static const WCHAR dpi_value_name[] = {'L','o','g','P','i','x','e','l','s','\0'};

/******************************************************************************
 *      get_dpi   (internal)
 *
 * get the dpi from the registry
 */
static DWORD get_dpi( void )
{
    DWORD dpi = 96;
#if 0
    HKEY hkey;

    if (RegOpenKeyW(HKEY_CURRENT_CONFIG, dpi_key_name, &hkey) == ERROR_SUCCESS)
    {
        DWORD type, size, new_dpi;

        size = sizeof(new_dpi);
        if(RegQueryValueExW(hkey, dpi_value_name, NULL, &type, (void *)&new_dpi, &size) == ERROR_SUCCESS)
        {
            if(type == REG_DWORD && new_dpi != 0)
                dpi = new_dpi;
        }
        RegCloseKey(hkey);
    }
#endif
    return dpi;
}


/***********************************************************************
 *           GDI_inc_ref_count
 *
 * Increment the reference count of a GDI object.
 */
HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, 0 )))
    {
        header->selcount++;
        GDI_ReleaseObj( handle );
    }
    else handle = 0;

    return handle;
}


/***********************************************************************
 *           GDI_dec_ref_count
 *
 * Decrement the reference count of a GDI object.
 */
BOOL GDI_dec_ref_count( HGDIOBJ handle )
{
    GDIOBJHDR *header;

    if ((header = GDI_GetObjPtr( handle, 0 )))
    {
        assert( header->selcount );
        if (!--header->selcount && header->deleted)
        {
            /* handle delayed DeleteObject*/
            header->deleted = 0;
            GDI_ReleaseObj( handle );
            TRACE( "executing delayed DeleteObject for %p\n", handle );
            DeleteObject( handle );
        }
        else GDI_ReleaseObj( handle );
    }
    return header != NULL;
}


/***********************************************************************
 *           DllMain
 *
 * GDI initialization.
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    LOGFONTW default_gui_font;
    const struct DefaultFontInfo* deffonts;
    int i;

    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( inst );
    WineEngInit();

    /* create stock objects */
    stock_objects[WHITE_BRUSH]  = CreateBrushIndirect( &WhiteBrush );
    stock_objects[LTGRAY_BRUSH] = CreateBrushIndirect( &LtGrayBrush );
    stock_objects[GRAY_BRUSH]   = CreateBrushIndirect( &GrayBrush );
    stock_objects[DKGRAY_BRUSH] = CreateBrushIndirect( &DkGrayBrush );
    stock_objects[BLACK_BRUSH]  = CreateBrushIndirect( &BlackBrush );
    stock_objects[NULL_BRUSH]   = CreateBrushIndirect( &NullBrush );

    stock_objects[WHITE_PEN]    = CreatePenIndirect( &WhitePen );
    stock_objects[BLACK_PEN]    = CreatePenIndirect( &BlackPen );
    stock_objects[NULL_PEN]     = CreatePenIndirect( &NullPen );

    stock_objects[DEFAULT_PALETTE] = PALETTE_Init();
    stock_objects[DEFAULT_BITMAP]  = CreateBitmap( 1, 1, 1, 1, NULL );

    /* language-independent stock fonts */
    stock_objects[OEM_FIXED_FONT]      = CreateFontIndirectW( &OEMFixedFont );
    stock_objects[ANSI_FIXED_FONT]     = CreateFontIndirectW( &AnsiFixedFont );
    stock_objects[ANSI_VAR_FONT]       = CreateFontIndirectW( &AnsiVarFont );

    /* language-dependent stock fonts */
    deffonts = get_default_fonts(get_default_charset());
    stock_objects[SYSTEM_FONT]         = CreateFontIndirectW( &deffonts->SystemFont );
    stock_objects[DEVICE_DEFAULT_FONT] = CreateFontIndirectW( &deffonts->DeviceDefaultFont );
    stock_objects[SYSTEM_FIXED_FONT]   = CreateFontIndirectW( &deffonts->SystemFixedFont );

    /* For the default gui font, we use the lfHeight member in deffonts as a place-holder
       for the point size so we must convert this into a true height */
    default_gui_font = deffonts->DefaultGuiFont;
    default_gui_font.lfHeight = -MulDiv(default_gui_font.lfHeight, get_dpi(), 72);
    stock_objects[DEFAULT_GUI_FONT]    = CreateFontIndirectW( &default_gui_font );

    stock_objects[DC_BRUSH]     = CreateBrushIndirect( &DCBrush );
    stock_objects[DC_PEN]       = CreatePenIndirect( &DCPen );

    /* clear the NOSYSTEM bit on all stock objects*/
    for (i = 0; i < NB_STOCK_OBJECTS; i++)
    {
        if (!stock_objects[i])
        {
            if (i == 9) continue;  /* there's no stock object 9 */
            ERR( "could not create stock object %d\n", i );
            return FALSE;
        }
        __wine_make_gdi_object_system( stock_objects[i], TRUE );
    }

    return TRUE;
}

#define FIRST_LARGE_HANDLE 16
#define MAX_LARGE_HANDLES ((GDI_HEAP_SIZE >> 2) - FIRST_LARGE_HANDLE)
static GDIOBJHDR *large_handles[MAX_LARGE_HANDLES];
static int next_large_handle;
static LONG debug_count;

static const char *gdi_obj_type( unsigned type )
{
    switch ( type )
    {
        case OBJ_PEN: return "OBJ_PEN";
        case OBJ_BRUSH: return "OBJ_BRUSH";
        case OBJ_DC: return "OBJ_DC";
        case OBJ_METADC: return "OBJ_METADC";
        case OBJ_PAL: return "OBJ_PAL";
        case OBJ_FONT: return "OBJ_FONT";
        case OBJ_BITMAP: return "OBJ_BITMAP";
        case OBJ_REGION: return "OBJ_REGION";
        case OBJ_METAFILE: return "OBJ_METAFILE";
        case OBJ_MEMDC: return "OBJ_MEMDC";
        case OBJ_EXTPEN: return "OBJ_EXTPEN";
        case OBJ_ENHMETADC: return "OBJ_ENHMETADC";
        case OBJ_ENHMETAFILE: return "OBJ_ENHMETAFILE";
        case OBJ_COLORSPACE: return "OBJ_COLORSPACE";
        default: return "UNKNOWN";
    }
}

static void dump_gdi_objects( void )
{
    int i;

    TRACE( "%u objects:\n", MAX_LARGE_HANDLES );

    EnterCriticalSection( &gdi_section );
    for (i = 0; i < MAX_LARGE_HANDLES; i++)
    {
        if (!large_handles[i])
        {
            TRACE( "index %d handle %p FREE\n", i, (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2) );
            continue;
        }
        TRACE( "handle %p obj %p type %s selcount %u deleted %u\n",
               (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2),
               large_handles[i], gdi_obj_type( large_handles[i]->type ),
               large_handles[i]->selcount, large_handles[i]->deleted );
    }
    LeaveCriticalSection( &gdi_section );
}

/***********************************************************************
 *           alloc_gdi_handle
 *
 * Allocate a GDI handle for an object, which must have been allocated on the process heap.
 */
HGDIOBJ alloc_gdi_handle( GDIOBJHDR *obj, WORD type, const struct gdi_obj_funcs *funcs )
{
    int i;

    /* initialize the object header */
    obj->type     = type;
    obj->system   = 0;
    obj->deleted  = 0;
    obj->selcount = 0;
    obj->funcs    = funcs;
    obj->hdcs     = NULL;

    EnterCriticalSection( &gdi_section );
    for (i = next_large_handle + 1; i < MAX_LARGE_HANDLES; i++)
        if (!large_handles[i]) goto found;
    for (i = 0; i <= next_large_handle; i++)
        if (!large_handles[i]) goto found;
    LeaveCriticalSection( &gdi_section );

    ERR( "out of GDI object handles, expect a crash\n" );
    if (TRACE_ON(gdi)) dump_gdi_objects();
    return 0;

 found:
    large_handles[i] = obj;
    next_large_handle = i;
    LeaveCriticalSection( &gdi_section );
    TRACE( "allocated %s %p %u/%u\n",
           gdi_obj_type(type), (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2),
           InterlockedIncrement( &debug_count ), MAX_LARGE_HANDLES );
    return (HGDIOBJ)(ULONG_PTR)((i + FIRST_LARGE_HANDLE) << 2);
}


/***********************************************************************
 *           free_gdi_handle
 *
 * Free a GDI handle and return a pointer to the object.
 */
void *free_gdi_handle( HGDIOBJ handle )
{
    GDIOBJHDR *object = NULL;
    int i;

    i = ((ULONG_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        EnterCriticalSection( &gdi_section );
        object = large_handles[i];
        large_handles[i] = NULL;
        LeaveCriticalSection( &gdi_section );
    }
    if (object)
    {
        TRACE( "freed %s %p %u/%u\n", gdi_obj_type( object->type ), handle,
               InterlockedDecrement( &debug_count ) + 1, MAX_LARGE_HANDLES );
        object->type  = 0;  /* mark it as invalid */
        object->funcs = NULL;
    }
    return object;
}


/***********************************************************************
 *           GDI_GetObjPtr
 *
 * Return a pointer to the GDI object associated to the handle.
 * Return NULL if the object has the wrong magic number.
 * The object must be released with GDI_ReleaseObj.
 */
void *GDI_GetObjPtr( HGDIOBJ handle, WORD type )
{
    GDIOBJHDR *ptr = NULL;
    int i;

    EnterCriticalSection( &gdi_section );

    i = ((UINT_PTR)handle >> 2) - FIRST_LARGE_HANDLE;
    if (i >= 0 && i < MAX_LARGE_HANDLES)
    {
        ptr = large_handles[i];
        if (ptr && type && ptr->type != type) ptr = NULL;
    }

    if (!ptr)
    {
        LeaveCriticalSection( &gdi_section );
        WARN( "Invalid handle %p\n", handle );
    }

    return ptr;
}


/***********************************************************************
 *           GDI_ReleaseObj
 *
 */
void GDI_ReleaseObj( HGDIOBJ handle )
{
    LeaveCriticalSection( &gdi_section );
}


/***********************************************************************
 *           GDI_CheckNotLock
 */
void GDI_CheckNotLock(void)
{
    if (gdi_section.OwningThread == ULongToHandle(GetCurrentThreadId()) && gdi_section.RecursionCount)
    {
        ERR( "BUG: holding GDI lock\n" );
        DebugBreak();
    }
}


/***********************************************************************
 *           DeleteObject    (GDI32.@)
 *
 * Delete a Gdi object.
 *
 * PARAMS
 *  obj [I] Gdi object to delete
 *
 * RETURNS
 *  Success: TRUE. If obj was not returned from GetStockObject(), any resources
 *           it consumed are released.
 *  Failure: FALSE, if obj is not a valid Gdi object, or is currently selected
 *           into a DC.
 */
BOOL WINAPI DeleteObject( HGDIOBJ obj )
{
      /* Check if object is valid */

    struct hdc_list *hdcs_head;
    const struct gdi_obj_funcs *funcs;
    GDIOBJHDR * header;

    if (HIWORD(obj)) return FALSE;

    if (!(header = GDI_GetObjPtr( obj, 0 ))) return FALSE;

    if (header->system)
    {
	TRACE("Preserving system object %p\n", obj);
        GDI_ReleaseObj( obj );
	return TRUE;
    }

    while ((hdcs_head = header->hdcs) != NULL)
    {
        DC *dc = get_dc_ptr(hdcs_head->hdc);

        header->hdcs = hdcs_head->next;
        TRACE("hdc %p has interest in %p\n", hdcs_head->hdc, obj);

        if(dc)
        {
            if(dc->funcs->pDeleteObject)
            {
                GDI_ReleaseObj( obj );  /* release the GDI lock */
                dc->funcs->pDeleteObject( dc->physDev, obj );
                header = GDI_GetObjPtr( obj, 0 );  /* and grab it again */
            }
            release_dc_ptr( dc );
        }
        HeapFree(GetProcessHeap(), 0, hdcs_head);
        if (!header) return FALSE;
    }

    if (header->selcount)
    {
        TRACE("delayed for %p because object in use, count %u\n", obj, header->selcount );
        header->deleted = 1;  /* mark for delete */
        GDI_ReleaseObj( obj );
        return TRUE;
    }

    TRACE("%p\n", obj );

      /* Delete object */

    funcs = header->funcs;
    GDI_ReleaseObj( obj );
    if (funcs && funcs->pDeleteObject)
        return funcs->pDeleteObject( obj );
    else
        return FALSE;
}

/***********************************************************************
 *           GDI_hdc_using_object
 *
 * Call this if the dc requires DeleteObject notification
 */
BOOL GDI_hdc_using_object(HGDIOBJ obj, HDC hdc)
{
    GDIOBJHDR * header;
    struct hdc_list **pphdc;

    TRACE("obj %p hdc %p\n", obj, hdc);

    if (!(header = GDI_GetObjPtr( obj, 0 ))) return FALSE;

    if (header->system)
    {
        GDI_ReleaseObj(obj);
        return FALSE;
    }

    for(pphdc = &header->hdcs; *pphdc; pphdc = &(*pphdc)->next)
        if((*pphdc)->hdc == hdc)
            break;

    if(!*pphdc) {
        *pphdc = HeapAlloc(GetProcessHeap(), 0, sizeof(**pphdc));
        (*pphdc)->hdc = hdc;
        (*pphdc)->next = NULL;
    }

    GDI_ReleaseObj(obj);
    return TRUE;
}

/***********************************************************************
 *           GDI_hdc_not_using_object
 *
 */
BOOL GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc)
{
    GDIOBJHDR * header;
    struct hdc_list *phdc, **prev;

    TRACE("obj %p hdc %p\n", obj, hdc);

    if (!(header = GDI_GetObjPtr( obj, 0 ))) return FALSE;

    if (header->system)
    {
        GDI_ReleaseObj(obj);
        return FALSE;
    }

    phdc = header->hdcs;
    prev = &header->hdcs;

    while(phdc) {
        if(phdc->hdc == hdc) {
            *prev = phdc->next;
            HeapFree(GetProcessHeap(), 0, phdc);
            phdc = *prev;
        } else {
            prev = &phdc->next;
            phdc = phdc->next;
        }
    }

    GDI_ReleaseObj(obj);
    return TRUE;
}

/***********************************************************************
 *           GetStockObject    (GDI32.@)
 */
HGDIOBJ WINAPI GetStockObject( INT obj )
{
    HGDIOBJ ret;
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    ret = stock_objects[obj];
    TRACE("returning %p\n", ret );
    return ret;
}


/***********************************************************************
 *           GetObjectA    (GDI32.@)
 */
INT WINAPI GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    const struct gdi_obj_funcs *funcs;
    GDIOBJHDR * ptr;
    INT result = 0;

    TRACE("%p %d %p\n", handle, count, buffer );

    if (!(ptr = GDI_GetObjPtr( handle, 0 ))) return 0;
    funcs = ptr->funcs;
    GDI_ReleaseObj( handle );

    if (funcs && funcs->pGetObjectA)
    {
        if (buffer && ((ULONG_PTR)buffer >> 16) == 0) /* catch apps getting argument order wrong */
            SetLastError( ERROR_NOACCESS );
        else
            result = funcs->pGetObjectA( handle, count, buffer );
    }
    else
        SetLastError( ERROR_INVALID_HANDLE );

    return result;
}

/***********************************************************************
 *           GetObjectW    (GDI32.@)
 */
INT WINAPI GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    const struct gdi_obj_funcs *funcs;
    GDIOBJHDR * ptr;
    INT result = 0;
    TRACE("%p %d %p\n", handle, count, buffer );

    if (!(ptr = GDI_GetObjPtr( handle, 0 ))) return 0;
    funcs = ptr->funcs;
    GDI_ReleaseObj( handle );

    if (funcs && funcs->pGetObjectW)
    {
        if (buffer && ((ULONG_PTR)buffer >> 16) == 0) /* catch apps getting argument order wrong */
            SetLastError( ERROR_NOACCESS );
        else
            result = funcs->pGetObjectW( handle, count, buffer );
    }
    else
        SetLastError( ERROR_INVALID_HANDLE );

    return result;
}

/***********************************************************************
 *           GetObjectType    (GDI32.@)
 */
DWORD WINAPI GetObjectType( HGDIOBJ handle )
{
    GDIOBJHDR * ptr;
    DWORD result;

    if (!(ptr = GDI_GetObjPtr( handle, 0 )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }
    result = ptr->type;
    GDI_ReleaseObj( handle );
    TRACE("%p -> %u\n", handle, result );
    return result;
}

/***********************************************************************
 *           GetCurrentObject    	(GDI32.@)
 *
 * Get the currently selected object of a given type in a device context.
 *
 * PARAMS
 *  hdc  [I] Device context to get the current object from
 *  type [I] Type of current object to get (OBJ_* defines from "wingdi.h")
 *
 * RETURNS
 *  Success: The current object of the given type selected in hdc.
 *  Failure: A NULL handle.
 *
 * NOTES
 * - only the following object types are supported:
 *| OBJ_PEN
 *| OBJ_BRUSH
 *| OBJ_PAL
 *| OBJ_FONT
 *| OBJ_BITMAP
 */
HGDIOBJ WINAPI GetCurrentObject(HDC hdc,UINT type)
{
    HGDIOBJ ret = 0;
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    switch (type) {
	case OBJ_EXTPEN: /* fall through */
	case OBJ_PEN:	 ret = dc->hPen; break;
	case OBJ_BRUSH:	 ret = dc->hBrush; break;
	case OBJ_PAL:	 ret = dc->hPalette; break;
	case OBJ_FONT:	 ret = dc->hFont; break;
	case OBJ_BITMAP: ret = dc->hBitmap; break;

	/* tests show that OBJ_REGION is explicitly ignored */
	case OBJ_REGION: break;
        default:
            /* the SDK only mentions those above */
            FIXME("(%p,%d): unknown type.\n",hdc,type);
	    break;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SelectObject    (GDI32.@)
 *
 * Select a Gdi object into a device context.
 *
 * PARAMS
 *  hdc  [I] Device context to associate the object with
 *  hObj [I] Gdi object to associate with hdc
 *
 * RETURNS
 *  Success: A non-NULL handle representing the previously selected object of
 *           the same type as hObj.
 *  Failure: A NULL object. If hdc is invalid, GetLastError() returns ERROR_INVALID_HANDLE.
 *           if hObj is not a valid object handle, no last error is set. In either
 *           case, hdc is unaffected by the call.
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ hObj )
{
    HGDIOBJ ret = 0;
    GDIOBJHDR *header;

    TRACE( "(%p,%p)\n", hdc, hObj );

    header = GDI_GetObjPtr( hObj, 0 );
    if (header)
    {
        const struct gdi_obj_funcs *funcs = header->funcs;
        GDI_ReleaseObj( hObj );
        if (funcs && funcs->pSelectObject) ret = funcs->pSelectObject( hObj, hdc );
    }
    return ret;
}


/***********************************************************************
 *           UnrealizeObject    (GDI32.@)
 */
BOOL WINAPI UnrealizeObject( HGDIOBJ obj )
{
    BOOL result = FALSE;
    GDIOBJHDR * header = GDI_GetObjPtr( obj, 0 );

    if (header)
    {
        const struct gdi_obj_funcs *funcs = header->funcs;

        GDI_ReleaseObj( obj );
        if (funcs && funcs->pUnrealizeObject)
            result = header->funcs->pUnrealizeObject( obj );
        else
            result = TRUE;
    }
    return result;
}


/* Solid colors to enumerate */
static const COLORREF solid_colors[] =
{ RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xff),
RGB(0xff,0x00,0x00), RGB(0x00,0xff,0x00),
RGB(0x00,0x00,0xff), RGB(0xff,0xff,0x00),
RGB(0xff,0x00,0xff), RGB(0x00,0xff,0xff),
RGB(0x80,0x00,0x00), RGB(0x00,0x80,0x00),
RGB(0x80,0x80,0x00), RGB(0x00,0x00,0x80),
RGB(0x80,0x00,0x80), RGB(0x00,0x80,0x80),
RGB(0x80,0x80,0x80), RGB(0xc0,0xc0,0xc0)
};


/***********************************************************************
 *           EnumObjects    (GDI32.@)
 */
INT WINAPI EnumObjects( HDC hdc, INT nObjType,
                            GOBJENUMPROC lpEnumFunc, LPARAM lParam )
{
    UINT i;
    INT retval = 0;
    LOGPEN pen;
    LOGBRUSH brush;

    TRACE("%p %d %p %08lx\n", hdc, nObjType, lpEnumFunc, lParam );
    switch(nObjType)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            pen.lopnStyle   = PS_SOLID;
            pen.lopnWidth.x = 1;
            pen.lopnWidth.y = 0;
            pen.lopnColor   = solid_colors[i];
            retval = lpEnumFunc( &pen, lParam );
            TRACE("solid pen %08x, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            brush.lbStyle = BS_SOLID;
            brush.lbColor = solid_colors[i];
            brush.lbHatch = 0;
            retval = lpEnumFunc( &brush, lParam );
            TRACE("solid brush %08x, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }

        /* Now enumerate hatched brushes */
        if (retval) for (i = HS_HORIZONTAL; i <= HS_DIAGCROSS; i++)
        {
            brush.lbStyle = BS_HATCHED;
            brush.lbColor = RGB(0,0,0);
            brush.lbHatch = i;
            retval = lpEnumFunc( &brush, lParam );
            TRACE("hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        break;

    default:
        /* FIXME: implement Win32 types */
        WARN("(%d): Invalid type\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           SetObjectOwner    (GDI32.@)
 */
void WINAPI SetObjectOwner( HGDIOBJ handle, HANDLE owner )
{
    /* Nothing to do */
}

/***********************************************************************
 *           GdiInitializeLanguagePack    (GDI32.@)
 */
DWORD WINAPI GdiInitializeLanguagePack( DWORD arg )
{
    FIXME("stub\n");
    return 0;
}

/***********************************************************************
 *           GdiFlush    (GDI32.@)
 */
BOOL WINAPI GdiFlush(void)
{
    return TRUE;  /* FIXME */
}


/***********************************************************************
 *           GdiGetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiGetBatchLimit(void)
{
    return 1;  /* FIXME */
}


/***********************************************************************
 *           GdiSetBatchLimit    (GDI32.@)
 */
DWORD WINAPI GdiSetBatchLimit( DWORD limit )
{
    return 1; /* FIXME */
}


/*******************************************************************
 *      GetColorAdjustment [GDI32.@]
 *
 *
 */
BOOL WINAPI GetColorAdjustment(HDC hdc, LPCOLORADJUSTMENT lpca)
{
        FIXME("stub\n");
        return 0;
}

/*******************************************************************
 *      GdiComment [GDI32.@]
 *
 *
 */
BOOL WINAPI GdiComment(HDC hdc, UINT cbSize, const BYTE *lpData)
{
    DC *dc = get_dc_ptr(hdc);
    BOOL ret = FALSE;
    if(dc)
    {
        if (dc->funcs->pGdiComment)
            ret = dc->funcs->pGdiComment( dc->physDev, cbSize, lpData );
        release_dc_ptr( dc );
    }
    return ret;
}

/*******************************************************************
 *      SetColorAdjustment [GDI32.@]
 *
 *
 */
BOOL WINAPI SetColorAdjustment(HDC hdc, const COLORADJUSTMENT* lpca)
{
        FIXME("stub\n");
        return 0;
}
