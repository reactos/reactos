/*
 * X11DRV initialization code
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2000 Alexandre Julliard
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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#ifdef HAVE_XKB
#include <X11/XKBlib.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
#include <X11/extensions/Xrender.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "x11drv.h"
#include "xvidmode.h"
//#include "xrandr.h"
#include "xcomposite.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);
WINE_DECLARE_DEBUG_CHANNEL(synchronous);

static CRITICAL_SECTION X11DRV_CritSection;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &X11DRV_CritSection,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": X11DRV_CritSection") }
};
static CRITICAL_SECTION X11DRV_CritSection = { &critsect_debug, -1, 0, 0, 0, 0 };

Screen *screen;
Visual *visual;
unsigned int screen_width;
unsigned int screen_height;
unsigned int screen_bpp;
unsigned int screen_depth;
RECT virtual_screen_rect;
Window root_window;
int dxgrab = 0;
int usexvidmode = 1;
int usexrandr = 1;
int usexcomposite = 1;
int use_xkb = 1;
int use_take_focus = 1;
int use_primary_selection = 0;
int managed_mode = 1;
int decorated_mode = 1;
int private_color_map = 0;
int primary_monitor = 0;
int client_side_with_core = 1;
int client_side_with_render = 1;
int client_side_antialias_with_core = 1;
int client_side_antialias_with_render = 1;
int copy_default_colors = 128;
int alloc_system_colors = 256;
DWORD thread_data_tls_index = TLS_OUT_OF_INDEXES;
int xrender_error_base = 0;
HMODULE x11drv_module = 0;

static x11drv_error_callback err_callback;   /* current callback for error */
static Display *err_callback_display;        /* display callback is set for */
static void *err_callback_arg;               /* error callback argument */
static int err_callback_result;              /* error callback result */
static unsigned long err_serial;             /* serial number of first request */
static int (*old_error_handler)( Display *, XErrorEvent * );
static int use_xim = 1;
static char input_style[20];

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

Atom X11DRV_Atoms[NB_XATOMS - FIRST_XATOM];

static const char * const atom_names[NB_XATOMS - FIRST_XATOM] =
{
    "CLIPBOARD",
    "COMPOUND_TEXT",
    "INCR",
    "MULTIPLE",
    "SELECTION_DATA",
    "TARGETS",
    "TEXT",
    "UTF8_STRING",
    "RAW_ASCENT",
    "RAW_DESCENT",
    "RAW_CAP_HEIGHT",
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_STATE",
    "WM_TAKE_FOCUS",
    "DndProtocol",
    "DndSelection",
    "_ICC_PROFILE",
    "_MOTIF_WM_HINTS",
    "_NET_STARTUP_INFO_BEGIN",
    "_NET_STARTUP_INFO",
    "_NET_SUPPORTED",
    "_NET_SYSTEM_TRAY_OPCODE",
    "_NET_SYSTEM_TRAY_S0",
    "_NET_WM_MOVERESIZE",
    "_NET_WM_NAME",
    "_NET_WM_PID",
    "_NET_WM_PING",
    "_NET_WM_STATE",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_WINDOW_OPACITY",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WORKAREA",
    "_XEMBED_INFO",
    "XdndAware",
    "XdndEnter",
    "XdndPosition",
    "XdndStatus",
    "XdndLeave",
    "XdndFinished",
    "XdndDrop",
    "XdndActionCopy",
    "XdndActionMove",
    "XdndActionLink",
    "XdndActionAsk",
    "XdndActionPrivate",
    "XdndSelection",
    "XdndTarget",
    "XdndTypeList",
    "WCF_DIB",
    "image/gif",
    "image/jpeg",
    "image/png",
    "text/html",
    "text/plain",
    "text/rtf",
    "text/richtext",
    "text/uri-list"
};

/***********************************************************************
 *		ignore_error
 *
 * Check if the X error is one we can ignore.
 */
static inline BOOL ignore_error( Display *display, XErrorEvent *event )
{
    if (event->request_code == X_SetInputFocus && event->error_code == BadMatch) return TRUE;

    /* ignore a number of errors on gdi display caused by creating/destroying windows */
    if (display == gdi_display)
    {
        if (event->error_code == BadDrawable || event->error_code == BadGC) return TRUE;
#ifdef HAVE_X11_EXTENSIONS_XRENDER_H
        if (xrender_error_base)  /* check for XRender errors */
        {
            if (event->error_code == xrender_error_base + BadPicture) return TRUE;
        }
#endif
    }
    return FALSE;
}


/***********************************************************************
 *		X11DRV_expect_error
 *
 * Setup a callback function that will be called on an X error.  The
 * callback must return non-zero if the error is the one it expected.
 * This function acquires the x11 lock; X11DRV_check_error must be
 * called in all cases to release it.
 */
void X11DRV_expect_error( Display *display, x11drv_error_callback callback, void *arg )
{
    wine_tsx11_lock();
    err_callback         = callback;
    err_callback_display = display;
    err_callback_arg     = arg;
    err_callback_result  = 0;
    err_serial           = NextRequest(display);
}


/***********************************************************************
 *		X11DRV_check_error
 *
 * Check if an expected X11 error occurred; return non-zero if yes.
 * Also release the x11 lock obtained in X11DRV_expect_error.
 * The caller is responsible for calling XSync first if necessary.
 */
int X11DRV_check_error(void)
{
    int ret;
    err_callback = NULL;
    ret = err_callback_result;
    wine_tsx11_unlock();
    return ret;
}


/***********************************************************************
 *		error_handler
 */
static int error_handler( Display *display, XErrorEvent *error_evt )
{
    if (err_callback && display == err_callback_display &&
        (long)(error_evt->serial - err_serial) >= 0)
    {
        if ((err_callback_result = err_callback( display, error_evt, err_callback_arg )))
        {
            TRACE( "got expected error %d req %d\n",
                   error_evt->error_code, error_evt->request_code );
            return 0;
        }
    }
    if (ignore_error( display, error_evt ))
    {
        TRACE( "got ignored error %d req %d\n",
               error_evt->error_code, error_evt->request_code );
        return 0;
    }
    if (TRACE_ON(synchronous))
    {
        ERR( "X protocol error: serial=%ld, request_code=%d - breaking into debugger\n",
             error_evt->serial, error_evt->request_code );
        DebugBreak();  /* force an entry in the debugger */
    }
    old_error_handler( display, error_evt );
    return 0;
}

/***********************************************************************
 *		wine_tsx11_lock   (X11DRV.@)
 */
void CDECL wine_tsx11_lock(void)
{
    EnterCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *		wine_tsx11_unlock   (X11DRV.@)
 */
void CDECL wine_tsx11_unlock(void)
{
    LeaveCriticalSection( &X11DRV_CritSection );
}


/***********************************************************************
 *		depth_to_bpp
 *
 * Convert X11-reported depth to the BPP value that Windows apps expect to see.
 */
unsigned int depth_to_bpp( unsigned int depth )
{
    switch (depth)
    {
    case 1:
    case 8:
        return depth;
    case 15:
    case 16:
        return 16;
    case 24:
        /* This is not necessarily right. X11 always has 24 bits per pixel, but it can run
         * with 24 bit framebuffers and 32 bit framebuffers. It doesn't make any difference
         * for windowing, but gl applications can get visuals with alpha channels. So we
         * should check the framebuffer and/or opengl formats available to find out what the
         * framebuffer actually does
         */
    case 32:
        return 32;
    default:
        FIXME( "Unexpected X11 depth %d bpp, what to report to app?\n", depth );
        return depth;
    }
}


/***********************************************************************
 *		get_config_key
 *
 * Get a config key from either the app-specific or the default config
 */
static inline DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                                    char *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    if (defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}


/***********************************************************************
 *		setup_options
 *
 * Setup the x11drv options.
 */
static void setup_options(void)
{
    char buffer[MAX_PATH+16];
    HKEY hkey, appkey = 0;
    DWORD len;

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\X11 Driver", &hkey )) hkey = 0;

    /* open the app-specific key */

    len = (GetModuleFileNameA( 0, buffer, MAX_PATH ));
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        char *p, *appname = buffer;
        if ((p = strrchr( appname, '/' ))) appname = p + 1;
        if ((p = strrchr( appname, '\\' ))) appname = p + 1;
        strcat( appname, "\\X11 Driver" );
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\X11 Driver */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    if (!get_config_key( hkey, appkey, "Managed", buffer, sizeof(buffer) ))
        managed_mode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "Decorated", buffer, sizeof(buffer) ))
        decorated_mode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "DXGrab", buffer, sizeof(buffer) ))
        dxgrab = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXVidMode", buffer, sizeof(buffer) ))
        usexvidmode = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXRandR", buffer, sizeof(buffer) ))
        usexrandr = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseTakeFocus", buffer, sizeof(buffer) ))
        use_take_focus = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UsePrimarySelection", buffer, sizeof(buffer) ))
        use_primary_selection = IS_OPTION_TRUE( buffer[0] );

    screen_depth = 0;
    if (!get_config_key( hkey, appkey, "ScreenDepth", buffer, sizeof(buffer) ))
        screen_depth = atoi(buffer);

    if (!get_config_key( hkey, appkey, "ClientSideWithCore", buffer, sizeof(buffer) ))
        client_side_with_core = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideWithRender", buffer, sizeof(buffer) ))
        client_side_with_render = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideAntiAliasWithCore", buffer, sizeof(buffer) ))
        client_side_antialias_with_core = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "ClientSideAntiAliasWithRender", buffer, sizeof(buffer) ))
        client_side_antialias_with_render = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "UseXIM", buffer, sizeof(buffer) ))
        use_xim = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "PrivateColorMap", buffer, sizeof(buffer) ))
        private_color_map = IS_OPTION_TRUE( buffer[0] );

    if (!get_config_key( hkey, appkey, "PrimaryMonitor", buffer, sizeof(buffer) ))
        primary_monitor = atoi( buffer );

    if (!get_config_key( hkey, appkey, "CopyDefaultColors", buffer, sizeof(buffer) ))
        copy_default_colors = atoi(buffer);

    if (!get_config_key( hkey, appkey, "AllocSystemColors", buffer, sizeof(buffer) ))
        alloc_system_colors = atoi(buffer);

    get_config_key( hkey, appkey, "InputStyle", input_style, sizeof(input_style) );

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );
}

#ifdef SONAME_LIBXCOMPOSITE

#define MAKE_FUNCPTR(f) typeof(f) * p##f;
MAKE_FUNCPTR(XCompositeQueryExtension)
MAKE_FUNCPTR(XCompositeQueryVersion)
MAKE_FUNCPTR(XCompositeVersion)
MAKE_FUNCPTR(XCompositeRedirectWindow)
MAKE_FUNCPTR(XCompositeRedirectSubwindows)
MAKE_FUNCPTR(XCompositeUnredirectWindow)
MAKE_FUNCPTR(XCompositeUnredirectSubwindows)
MAKE_FUNCPTR(XCompositeCreateRegionFromBorderClip)
MAKE_FUNCPTR(XCompositeNameWindowPixmap)
#undef MAKE_FUNCPTR

static int xcomp_event_base;
static int xcomp_error_base;

static void X11DRV_XComposite_Init(void)
{
    void *xcomposite_handle = wine_dlopen(SONAME_LIBXCOMPOSITE, RTLD_NOW, NULL, 0);
    if (!xcomposite_handle)
    {
        TRACE("Unable to open %s, XComposite disabled\n", SONAME_LIBXCOMPOSITE);
        usexcomposite = 0;
        return;
    }

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(xcomposite_handle, #f, NULL, 0)) == NULL) \
        goto sym_not_found;
    LOAD_FUNCPTR(XCompositeQueryExtension)
    LOAD_FUNCPTR(XCompositeQueryVersion)
    LOAD_FUNCPTR(XCompositeVersion)
    LOAD_FUNCPTR(XCompositeRedirectWindow)
    LOAD_FUNCPTR(XCompositeRedirectSubwindows)
    LOAD_FUNCPTR(XCompositeUnredirectWindow)
    LOAD_FUNCPTR(XCompositeUnredirectSubwindows)
    LOAD_FUNCPTR(XCompositeCreateRegionFromBorderClip)
    LOAD_FUNCPTR(XCompositeNameWindowPixmap)
#undef LOAD_FUNCPTR

    if(!pXCompositeQueryExtension(gdi_display, &xcomp_event_base,
                                  &xcomp_error_base)) {
        TRACE("XComposite extension could not be queried; disabled\n");
        wine_dlclose(xcomposite_handle, NULL, 0);
        xcomposite_handle = NULL;
        usexcomposite = 0;
        return;
    }
    TRACE("XComposite is up and running error_base = %d\n", xcomp_error_base);
    return;

sym_not_found:
    TRACE("Unable to load function pointers from %s, XComposite disabled\n", SONAME_LIBXCOMPOSITE);
    wine_dlclose(xcomposite_handle, NULL, 0);
    xcomposite_handle = NULL;
    usexcomposite = 0;
}
#endif /* defined(SONAME_LIBXCOMPOSITE) */


/***********************************************************************
 *           X11DRV process initialisation routine
 */
static BOOL process_attach(void)
{
    Display *display;

    setup_options();

    if ((thread_data_tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) return FALSE;

    /* Open display */

    if (!(display = XOpenDisplay( "192.168.1.159:0.0" ))) return FALSE;

    //fcntl( ConnectionNumber(display), F_SETFD, 1 ); /* set close on exec flag */
    screen = DefaultScreenOfDisplay( display );
    visual = DefaultVisual( display, DefaultScreen(display) );
    root_window = DefaultRootWindow( display );
    gdi_display = display;
    old_error_handler = XSetErrorHandler( error_handler );

    /* Initialize screen depth */

    if (screen_depth)  /* depth specified */
    {
        int depth_count, i;
        int *depth_list = XListDepths(display, DefaultScreen(display), &depth_count);
        for (i = 0; i < depth_count; i++)
            if (depth_list[i] == screen_depth) break;
        XFree( depth_list );
        if (i >= depth_count)
        {
            WARN( "invalid depth %d, using default\n", screen_depth );
            screen_depth = 0;
        }
    }
    if (!screen_depth) screen_depth = DefaultDepthOfScreen( screen );
    screen_bpp = depth_to_bpp( screen_depth );

    XInternAtoms( display, (char **)atom_names, NB_XATOMS - FIRST_XATOM, False, X11DRV_Atoms );

    if (TRACE_ON(synchronous)) XSynchronize( display, True );

    xinerama_init( WidthOfScreen(screen), HeightOfScreen(screen) );
    X11DRV_Settings_Init();

#ifdef SONAME_LIBXXF86VM
    /* initialize XVidMode */
    X11DRV_XF86VM_Init();
#endif
#ifdef SONAME_LIBXRANDR
    /* initialize XRandR */
    X11DRV_XRandR_Init();
#endif
#ifdef SONAME_LIBXCOMPOSITE
    X11DRV_XComposite_Init();
#endif

#ifdef HAVE_XKB
    if (use_xkb) use_xkb = XkbUseExtension( gdi_display, NULL, NULL );
#endif
    X11DRV_InitKeyboard( gdi_display );
    X11DRV_InitClipboard();
    if (use_xim) use_xim = X11DRV_InitXIM( input_style );

    return TRUE;
}


/***********************************************************************
 *           X11DRV thread termination routine
 */
static void thread_detach(void)
{
    struct x11drv_thread_data *data = TlsGetValue( thread_data_tls_index );

    if (data)
    {
        X11DRV_ResetSelectionOwner();
        wine_tsx11_lock();
        if (data->xim) XCloseIM( data->xim );
        if (data->font_set) XFreeFontSet( data->display, data->font_set );
        XCloseDisplay( data->display );
        wine_tsx11_unlock();
        HeapFree( GetProcessHeap(), 0, data );
    }
}


/***********************************************************************
 *           X11DRV process termination routine
 */
static void process_detach(void)
{
#ifdef SONAME_LIBXXF86VM
    /* cleanup XVidMode */
    X11DRV_XF86VM_Cleanup();
#endif
    if(using_client_side_fonts)
        X11DRV_XRender_Finalize();

    /* cleanup GDI */
    X11DRV_GDI_Finalize();
    X11DRV_OpenGL_Cleanup();

    IME_UnregisterClasses();
    DeleteCriticalSection( &X11DRV_CritSection );
    TlsFree( thread_data_tls_index );
}


/* store the display fd into the message queue */
static void set_queue_display_fd( Display *display )
{
#if 0
    HANDLE handle;
    int ret;

    if (wine_server_fd_to_handle( ConnectionNumber(display), GENERIC_READ | SYNCHRONIZE, 0, &handle ))
    {
        MESSAGE( "x11drv: Can't allocate handle for display fd\n" );
        ExitProcess(1);
    }
    SERVER_START_REQ( set_queue_fd )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret)
    {
        MESSAGE( "x11drv: Can't store handle for display fd\n" );
        ExitProcess(1);
    }
    CloseHandle( handle );
#endif
}


/***********************************************************************
 *           X11DRV thread initialisation routine
 */
struct x11drv_thread_data *x11drv_init_thread_data(void)
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    if (data) return data;

    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )))
    {
        ERR( "could not create data\n" );
        ExitProcess(1);
    }
    wine_tsx11_lock();
    if (!(data->display = XOpenDisplay("192.168.1.159:0.0")))
    {
        wine_tsx11_unlock();
        MESSAGE( "x11drv: Can't open display: %s\n", XDisplayName(NULL) );
        MESSAGE( "Please ensure that your X server is running and that $DISPLAY is set correctly.\n" );
        ExitProcess(1);
    }

    //fcntl( ConnectionNumber(data->display), F_SETFD, 1 ); /* set close on exec flag */

#ifdef HAVE_XKB
    if (use_xkb && XkbUseExtension( data->display, NULL, NULL ))
        XkbSetDetectableAutoRepeat( data->display, True, NULL );
#endif

    if (TRACE_ON(synchronous)) XSynchronize( data->display, True );
    wine_tsx11_unlock();

    set_queue_display_fd( data->display );
    TlsSetValue( thread_data_tls_index, data );

    if (use_xim) X11DRV_SetupXIM();
    X11DRV_SetCursor( NULL );

    return data;
}


/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        x11drv_module = hinst;
        ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    }
    return ret;
}

/***********************************************************************
 *              GetScreenSaveActive (X11DRV.@)
 *
 * Returns the active status of the screen saver
 */
BOOL CDECL X11DRV_GetScreenSaveActive(void)
{
    int timeout, temp;
    wine_tsx11_lock();
    XGetScreenSaver(gdi_display, &timeout, &temp, &temp, &temp);
    wine_tsx11_unlock();
    return timeout != 0;
}

/***********************************************************************
 *              SetScreenSaveActive (X11DRV.@)
 *
 * Activate/Deactivate the screen saver
 */
void CDECL X11DRV_SetScreenSaveActive(BOOL bActivate)
{
    int timeout, interval, prefer_blanking, allow_exposures;
    static int last_timeout = 15 * 60;

    wine_tsx11_lock();
    XGetScreenSaver(gdi_display, &timeout, &interval, &prefer_blanking,
                    &allow_exposures);
    if (timeout) last_timeout = timeout;

    timeout = bActivate ? last_timeout : 0;
    XSetScreenSaver(gdi_display, timeout, interval, prefer_blanking,
                    allow_exposures);
    wine_tsx11_unlock();
}
