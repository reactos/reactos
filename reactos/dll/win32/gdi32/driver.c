/*
 * Graphics driver management functions
 *
 * Copyright 1994 Bob Amstadt
 * Copyright 1996, 2001 Alexandre Julliard
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
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ddrawgdi.h"

#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

struct graphics_driver
{
    struct graphics_driver *next;
    struct graphics_driver *prev;
    HMODULE                 module;  /* module handle */
    unsigned int            count;   /* reference count */
    DC_FUNCTIONS            funcs;
};

static struct graphics_driver *first_driver;
static struct graphics_driver *display_driver;

static CRITICAL_SECTION driver_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &driver_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": driver_section") }
};
static CRITICAL_SECTION driver_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/**********************************************************************
 *	     create_driver
 *
 * Allocate and fill the driver structure for a given module.
 */
static struct graphics_driver *create_driver( HMODULE module )
{
    struct graphics_driver *driver;

    if (!(driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver)))) return NULL;
    driver->next   = NULL;
    driver->prev   = NULL;
    driver->module = module;
    driver->count  = 1;

    /* fill the function table */
    if (module)
    {
#define GET_FUNC(name) driver->funcs.p##name = (void*)GetProcAddress( module, #name )
        GET_FUNC(AbortDoc);
        GET_FUNC(AbortPath);
        GET_FUNC(AlphaBlend);
        GET_FUNC(AngleArc);
        GET_FUNC(Arc);
        GET_FUNC(ArcTo);
        GET_FUNC(BeginPath);
        GET_FUNC(BitBlt);
        GET_FUNC(ChoosePixelFormat);
        GET_FUNC(Chord);
        GET_FUNC(CloseFigure);
        GET_FUNC(CreateBitmap);
        GET_FUNC(CreateDC);
        GET_FUNC(CreateDIBSection);
        GET_FUNC(DeleteBitmap);
        GET_FUNC(DeleteDC);
        GET_FUNC(DescribePixelFormat);
        GET_FUNC(DeviceCapabilities);
        GET_FUNC(Ellipse);
        GET_FUNC(EndDoc);
        GET_FUNC(EndPage);
        GET_FUNC(EndPath);
        GET_FUNC(EnumDeviceFonts);
        GET_FUNC(ExcludeClipRect);
        GET_FUNC(ExtDeviceMode);
        GET_FUNC(ExtEscape);
        GET_FUNC(ExtFloodFill);
        GET_FUNC(ExtSelectClipRgn);
        GET_FUNC(ExtTextOut);
        GET_FUNC(FillPath);
        GET_FUNC(FillRgn);
        GET_FUNC(FlattenPath);
        GET_FUNC(FrameRgn);
        GET_FUNC(GdiComment);
        GET_FUNC(GetBitmapBits);
        GET_FUNC(GetCharWidth);
        GET_FUNC(GetDCOrgEx);
        GET_FUNC(GetDIBColorTable);
        GET_FUNC(GetDIBits);
        GET_FUNC(GetDeviceCaps);
        GET_FUNC(GetDeviceGammaRamp);
        GET_FUNC(GetICMProfile);
        GET_FUNC(GetNearestColor);
        GET_FUNC(GetPixel);
        GET_FUNC(GetPixelFormat);
        GET_FUNC(GetSystemPaletteEntries);
        GET_FUNC(GetTextExtentExPoint);
        GET_FUNC(GetTextMetrics);
        GET_FUNC(IntersectClipRect);
        GET_FUNC(InvertRgn);
        GET_FUNC(LineTo);
        GET_FUNC(MoveTo);
        GET_FUNC(ModifyWorldTransform);
        GET_FUNC(OffsetClipRgn);
        GET_FUNC(OffsetViewportOrg);
        GET_FUNC(OffsetWindowOrg);
        GET_FUNC(PaintRgn);
        GET_FUNC(PatBlt);
        GET_FUNC(Pie);
        GET_FUNC(PolyBezier);
        GET_FUNC(PolyBezierTo);
        GET_FUNC(PolyDraw);
        GET_FUNC(PolyPolygon);
        GET_FUNC(PolyPolyline);
        GET_FUNC(Polygon);
        GET_FUNC(Polyline);
        GET_FUNC(PolylineTo);
        GET_FUNC(RealizeDefaultPalette);
        GET_FUNC(RealizePalette);
        GET_FUNC(Rectangle);
        GET_FUNC(ResetDC);
        GET_FUNC(RestoreDC);
        GET_FUNC(RoundRect);
        GET_FUNC(SaveDC);
        GET_FUNC(ScaleViewportExt);
        GET_FUNC(ScaleWindowExt);
        GET_FUNC(SelectBitmap);
        GET_FUNC(SelectBrush);
        GET_FUNC(SelectClipPath);
        GET_FUNC(SelectFont);
        GET_FUNC(SelectPalette);
        GET_FUNC(SelectPen);
        GET_FUNC(SetArcDirection);
        GET_FUNC(SetBitmapBits);
        GET_FUNC(SetBkColor);
        GET_FUNC(SetBkMode);
        GET_FUNC(SetDCBrushColor);
        GET_FUNC(SetDCOrg);
        GET_FUNC(SetDCPenColor);
        GET_FUNC(SetDIBColorTable);
        GET_FUNC(SetDIBits);
        GET_FUNC(SetDIBitsToDevice);
        GET_FUNC(SetDeviceClipping);
        GET_FUNC(SetDeviceGammaRamp);
        GET_FUNC(SetMapMode);
        GET_FUNC(SetMapperFlags);
        GET_FUNC(SetPixel);
        GET_FUNC(SetPixelFormat);
        GET_FUNC(SetPolyFillMode);
        GET_FUNC(SetROP2);
        GET_FUNC(SetRelAbs);
        GET_FUNC(SetStretchBltMode);
        GET_FUNC(SetTextAlign);
        GET_FUNC(SetTextCharacterExtra);
        GET_FUNC(SetTextColor);
        GET_FUNC(SetTextJustification);
        GET_FUNC(SetViewportExt);
        GET_FUNC(SetViewportOrg);
        GET_FUNC(SetWindowExt);
        GET_FUNC(SetWindowOrg);
        GET_FUNC(SetWorldTransform);
        GET_FUNC(StartDoc);
        GET_FUNC(StartPage);
        GET_FUNC(StretchBlt);
        GET_FUNC(StretchDIBits);
        GET_FUNC(StrokeAndFillPath);
        GET_FUNC(StrokePath);
        GET_FUNC(SwapBuffers);
        GET_FUNC(UnrealizePalette);
        GET_FUNC(WidenPath);

        /* OpenGL32 */
        GET_FUNC(wglCreateContext);
        GET_FUNC(wglDeleteContext);
        GET_FUNC(wglGetProcAddress);
        GET_FUNC(wglGetPbufferDCARB);
        GET_FUNC(wglMakeContextCurrentARB);
        GET_FUNC(wglMakeCurrent);
        GET_FUNC(wglSetPixelFormatWINE);
        GET_FUNC(wglShareLists);
        GET_FUNC(wglUseFontBitmapsA);
        GET_FUNC(wglUseFontBitmapsW);
#undef GET_FUNC
    }
    else memset( &driver->funcs, 0, sizeof(driver->funcs) );

    /* add it to the list */
    driver->prev = NULL;
    if ((driver->next = first_driver)) driver->next->prev = driver;
    first_driver = driver;
    return driver;
}


/**********************************************************************
 *	     load_display_driver
 *
 * Special case for loading the display driver: get the name from the config file
 */
static struct graphics_driver *load_display_driver(void)
{
    char buffer[MAX_PATH], libname[32], *name, *next;
    HMODULE module = 0;
    HKEY hkey;

    if (display_driver)  /* already loaded */
    {
        display_driver->count++;
        return display_driver;
    }

    strcpy( buffer, "x11" );  /* default value */
    /* @@ Wine registry key: HKCU\Software\Wine\Drivers */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Drivers", &hkey ))
    {
        DWORD type, count = sizeof(buffer);
        RegQueryValueExA( hkey, "Graphics", 0, &type, (LPBYTE) buffer, &count );
        RegCloseKey( hkey );
    }

    name = buffer;
    while (name)
    {
        next = strchr( name, ',' );
        if (next) *next++ = 0;

        snprintf( libname, sizeof(libname), "wine%s.drv", name );
        if ((module = LoadLibraryA( libname )) != 0) break;
        name = next;
    }

    if (!(display_driver = create_driver( module )))
    {
        MESSAGE( "Could not create graphics driver '%s'\n", buffer );
        FreeLibrary( module );
        ExitProcess(1);
    }

    display_driver->count++;  /* we don't want to free it */
    return display_driver;
}


/**********************************************************************
 *	     DRIVER_load_driver
 */
const DC_FUNCTIONS *DRIVER_load_driver( LPCWSTR name )
{
    HMODULE module;
    struct graphics_driver *driver;
    static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };
    static const WCHAR display1W[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};

    EnterCriticalSection( &driver_section );

    /* display driver is a special case */
    if (!strcmpiW( name, displayW ) || 
        !strcmpiW( name, display1W ))
    {
        driver = load_display_driver();
        LeaveCriticalSection( &driver_section );
        return &driver->funcs;
    }

    if ((module = GetModuleHandleW( name )))
    {
        for (driver = first_driver; driver; driver = driver->next)
        {
            if (driver->module == module)
            {
                driver->count++;
                LeaveCriticalSection( &driver_section );
                return &driver->funcs;
            }
        }
    }

    if (!(module = LoadLibraryW( name )))
    {
        LeaveCriticalSection( &driver_section );
        return NULL;
    }

    if (!(driver = create_driver( module )))
    {
        FreeLibrary( module );
        LeaveCriticalSection( &driver_section );
        return NULL;
    }

    TRACE( "loaded driver %p for %s\n", driver, debugstr_w(name) );
    LeaveCriticalSection( &driver_section );
    return &driver->funcs;
}


/**********************************************************************
 *	     DRIVER_get_driver
 *
 * Get a new copy of an existing driver.
 */
const DC_FUNCTIONS *DRIVER_get_driver( const DC_FUNCTIONS *funcs )
{
    struct graphics_driver *driver;

    EnterCriticalSection( &driver_section );
    for (driver = first_driver; driver; driver = driver->next)
        if (&driver->funcs == funcs) break;
    if (!driver) ERR( "driver not found, trouble ahead\n" );
    driver->count++;
    LeaveCriticalSection( &driver_section );
    return funcs;
}


/**********************************************************************
 *	     DRIVER_release_driver
 *
 * Release a driver by decrementing ref count and freeing it if needed.
 */
void DRIVER_release_driver( const DC_FUNCTIONS *funcs )
{
    struct graphics_driver *driver;

    EnterCriticalSection( &driver_section );

    for (driver = first_driver; driver; driver = driver->next)
        if (&driver->funcs == funcs) break;

    if (!driver) goto done;
    if (--driver->count) goto done;

    /* removed last reference, free it */
    if (driver->next) driver->next->prev = driver->prev;
    if (driver->prev) driver->prev->next = driver->next;
    else first_driver = driver->next;
    if (driver == display_driver) display_driver = NULL;

    FreeLibrary( driver->module );
    HeapFree( GetProcessHeap(), 0, driver );
 done:
    LeaveCriticalSection( &driver_section );
}


/*****************************************************************************
 *      DRIVER_GetDriverName
 *
 */
BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size )
{
    static const WCHAR displayW[] = { 'd','i','s','p','l','a','y',0 };
    static const WCHAR devicesW[] = { 'd','e','v','i','c','e','s',0 };
    static const WCHAR display1W[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};
    static const WCHAR empty_strW[] = { 0 };
    WCHAR *p;

    /* display is a special case */
    if (!strcmpiW( device, displayW ) ||
        !strcmpiW( device, display1W ))
    {
        lstrcpynW( driver, displayW, size );
        return TRUE;
    }

    size = GetProfileStringW(devicesW, device, empty_strW, driver, size);
    if(!size) {
        WARN("Unable to find %s in [devices] section of win.ini\n", debugstr_w(device));
        return FALSE;
    }
    p = strchrW(driver, ',');
    if(!p)
    {
        WARN("%s entry in [devices] section of win.ini is malformed.\n", debugstr_w(device));
        return FALSE;
    }
    *p = 0;
    TRACE("Found %s for %s\n", debugstr_w(driver), debugstr_w(device));
    return TRUE;
}


/***********************************************************************
 *           GdiConvertToDevmodeW    (GDI32.@)
 */
DEVMODEW * WINAPI GdiConvertToDevmodeW(const DEVMODEA *dmA)
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc(GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra);
    if (!dmW) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmDeviceName, -1,
                                   dmW->dmDeviceName, CCHDEVICENAME);
    /* copy slightly more, to avoid long computations */
    memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME);

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        if (dmA->dmFields & DM_FORMNAME)
            MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmFormName, -1,
                                       dmW->dmFormName, CCHFORMNAME);
        else
            dmW->dmFormName[0] = 0;

        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels));
    }

    if (dmA->dmDriverExtra)
        memcpy((char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra);

    dmW->dmSize = dmW_size;

    return dmW;
}


/*****************************************************************************
 *      @ [GDI32.100]
 *
 * This should thunk to 16-bit and simply call the proc with the given args.
 */
INT WINAPI GDI_CallDevInstall16( FARPROC16 lpfnDevInstallProc, HWND hWnd,
                                 LPSTR lpModelName, LPSTR OldPort, LPSTR NewPort )
{
    FIXME("(%p, %p, %s, %s, %s)\n", lpfnDevInstallProc, hWnd, lpModelName, OldPort, NewPort );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.101]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * ExtDeviceModePropSheet proc.
 *
 * Note: The driver calls a callback routine for each property sheet page; these
 * pages are supposed to be filled into the structure pointed to by lpPropSheet.
 * The layout of this structure is:
 *
 * struct
 * {
 *   DWORD  nPages;
 *   DWORD  unknown;
 *   HPROPSHEETPAGE  pages[10];
 * };
 */
INT WINAPI GDI_CallExtDeviceModePropSheet16( HWND hWnd, LPCSTR lpszDevice,
                                             LPCSTR lpszPort, LPVOID lpPropSheet )
{
    FIXME("(%p, %s, %s, %p)\n", hWnd, lpszDevice, lpszPort, lpPropSheet );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.102]
 *
 * This should load the correct driver for lpszDevice and call this driver's
 * ExtDeviceMode proc.
 *
 * FIXME: convert ExtDeviceMode to unicode in the driver interface
 */
INT WINAPI GDI_CallExtDeviceMode16( HWND hwnd,
                                    LPDEVMODEA lpdmOutput, LPSTR lpszDevice,
                                    LPSTR lpszPort, LPDEVMODEA lpdmInput,
                                    LPSTR lpszProfile, DWORD fwMode )
{
    WCHAR deviceW[300];
    WCHAR bufW[300];
    char buf[300];
    HDC hdc;
    DC *dc;
    INT ret = -1;

    TRACE("(%p, %p, %s, %s, %p, %s, %d)\n",
          hwnd, lpdmOutput, lpszDevice, lpszPort, lpdmInput, lpszProfile, fwMode );

    if (!lpszDevice) return -1;
    if (!MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, deviceW, 300)) return -1;

    if(!DRIVER_GetDriverName( deviceW, bufW, 300 )) return -1;

    if (!WideCharToMultiByte(CP_ACP, 0, bufW, -1, buf, 300, NULL, NULL)) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pExtDeviceMode)
	    ret = dc->funcs->pExtDeviceMode( buf, hwnd, lpdmOutput, lpszDevice, lpszPort,
                                             lpdmInput, lpszProfile, fwMode );
	release_dc_ptr( dc );
    }
    DeleteDC( hdc );
    return ret;
}

/****************************************************************************
 *      @ [GDI32.103]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * AdvancedSetupDialog proc.
 */
INT WINAPI GDI_CallAdvancedSetupDialog16( HWND hwnd, LPSTR lpszDevice,
                                          LPDEVMODEA devin, LPDEVMODEA devout )
{
    TRACE("(%p, %s, %p, %p)\n", hwnd, lpszDevice, devin, devout );
    return -1;
}

/*****************************************************************************
 *      @ [GDI32.104]
 *
 * This should load the correct driver for lpszDevice and calls this driver's
 * DeviceCapabilities proc.
 *
 * FIXME: convert DeviceCapabilities to unicode in the driver interface
 */
DWORD WINAPI GDI_CallDeviceCapabilities16( LPCSTR lpszDevice, LPCSTR lpszPort,
                                           WORD fwCapability, LPSTR lpszOutput,
                                           LPDEVMODEA lpdm )
{
    WCHAR deviceW[300];
    WCHAR bufW[300];
    char buf[300];
    HDC hdc;
    DC *dc;
    INT ret = -1;

    TRACE("(%s, %s, %d, %p, %p)\n", lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );

    if (!lpszDevice) return -1;
    if (!MultiByteToWideChar(CP_ACP, 0, lpszDevice, -1, deviceW, 300)) return -1;

    if(!DRIVER_GetDriverName( deviceW, bufW, 300 )) return -1;

    if (!WideCharToMultiByte(CP_ACP, 0, bufW, -1, buf, 300, NULL, NULL)) return -1;

    if (!(hdc = CreateICA( buf, lpszDevice, lpszPort, NULL ))) return -1;

    if ((dc = get_dc_ptr( hdc )))
    {
        if (dc->funcs->pDeviceCapabilities)
            ret = dc->funcs->pDeviceCapabilities( buf, lpszDevice, lpszPort,
                                                  fwCapability, lpszOutput, lpdm );
        release_dc_ptr( dc );
    }
    DeleteDC( hdc );
    return ret;
}


/************************************************************************
 *             Escape  [GDI32.@]
 */
INT WINAPI Escape( HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data )
{
    INT ret;
    POINT *pt;

    switch (escape)
    {
    case ABORTDOC:
        return AbortDoc( hdc );

    case ENDDOC:
        return EndDoc( hdc );

    case GETPHYSPAGESIZE:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALWIDTH );
        pt->y = GetDeviceCaps( hdc, PHYSICALHEIGHT );
        return 1;

    case GETPRINTINGOFFSET:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, PHYSICALOFFSETX );
        pt->y = GetDeviceCaps( hdc, PHYSICALOFFSETY );
        return 1;

    case GETSCALINGFACTOR:
        pt = out_data;
        pt->x = GetDeviceCaps( hdc, SCALINGFACTORX );
        pt->y = GetDeviceCaps( hdc, SCALINGFACTORY );
        return 1;

    case NEWFRAME:
        return EndPage( hdc );

    case SETABORTPROC:
        return SetAbortProc( hdc, (ABORTPROC)in_data );

    case STARTDOC:
        {
            DOCINFOA doc;
            char *name = NULL;

            /* in_data may not be 0 terminated so we must copy it */
            if (in_data)
            {
                name = HeapAlloc( GetProcessHeap(), 0, in_count+1 );
                memcpy( name, in_data, in_count );
                name[in_count] = 0;
            }
            /* out_data is actually a pointer to the DocInfo structure and used as
             * a second input parameter */
            if (out_data) doc = *(DOCINFOA *)out_data;
            else
            {
                doc.cbSize = sizeof(doc);
                doc.lpszOutput = NULL;
                doc.lpszDatatype = NULL;
                doc.fwType = 0;
            }
            doc.lpszDocName = name;
            ret = StartDocA( hdc, &doc );
            HeapFree( GetProcessHeap(), 0, name );
            if (ret > 0) ret = StartPage( hdc );
            return ret;
        }

    case QUERYESCSUPPORT:
        {
            const INT *ptr = (const INT *)in_data;
            if (in_count < sizeof(INT)) return 0;
            switch(*ptr)
            {
            case ABORTDOC:
            case ENDDOC:
            case GETPHYSPAGESIZE:
            case GETPRINTINGOFFSET:
            case GETSCALINGFACTOR:
            case NEWFRAME:
            case QUERYESCSUPPORT:
            case SETABORTPROC:
            case STARTDOC:
                return TRUE;
            }
            break;
        }
    }

    /* if not handled internally, pass it to the driver */
    return ExtEscape( hdc, escape, in_count, in_data, 0, out_data );
}


/******************************************************************************
 *		ExtEscape	[GDI32.@]
 *
 * Access capabilities of a particular device that are not available through GDI.
 *
 * PARAMS
 *    hdc         [I] Handle to device context
 *    nEscape     [I] Escape function
 *    cbInput     [I] Number of bytes in input structure
 *    lpszInData  [I] Pointer to input structure
 *    cbOutput    [I] Number of bytes in output structure
 *    lpszOutData [O] Pointer to output structure
 *
 * RETURNS
 *    Success: >0
 *    Not implemented: 0
 *    Failure: <0
 */
INT WINAPI ExtEscape( HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData,
                      INT cbOutput, LPSTR lpszOutData )
{
    INT ret = 0;
    DC * dc = get_dc_ptr( hdc );
    if (dc)
    {
        if (dc->funcs->pExtEscape)
            ret = dc->funcs->pExtEscape( dc->physDev, nEscape, cbInput, lpszInData, cbOutput, lpszOutData );
        release_dc_ptr( dc );
    }
    return ret;
}


/*******************************************************************
 *      DrawEscape [GDI32.@]
 *
 *
 */
INT WINAPI DrawEscape(HDC hdc, INT nEscape, INT cbInput, LPCSTR lpszInData)
{
    FIXME("DrawEscape, stub\n");
    return 0;
}

/*******************************************************************
 *      NamedEscape [GDI32.@]
 */
INT WINAPI NamedEscape( HDC hdc, LPCWSTR pDriver, INT nEscape, INT cbInput, LPCSTR lpszInData,
                        INT cbOutput, LPSTR lpszOutData )
{
    FIXME("(%p, %s, %d, %d, %p, %d, %p)\n",
          hdc, wine_dbgstr_w(pDriver), nEscape, cbInput, lpszInData, cbOutput,
          lpszOutData);
    return 0;
}

/*******************************************************************
 *      DdQueryDisplaySettingsUniqueness [GDI32.@]
 *      GdiEntry13                       [GDI32.@]
 */
ULONG WINAPI DdQueryDisplaySettingsUniqueness(VOID)
{
    FIXME("stub\n");
    return 0;
}
