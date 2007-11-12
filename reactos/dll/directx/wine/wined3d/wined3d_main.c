/*
 * Direct3D wine internal interface main
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004      Jason Edmeades
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

#include "initguid.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wine_d3d);

int num_lock = 0;
void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;


/* When updating default value here, make sure to update winecfg as well,
 * where appropriate. */
wined3d_settings_t wined3d_settings = 
{
    VS_HW,          /* Hardware by default */
    PS_HW,          /* Hardware by default */
    VBO_HW,         /* Hardware by default */
    FALSE,          /* Use of GLSL disabled by default */
    ORM_BACKBUFFER, /* Use the backbuffer to do offscreen rendering */
    RTL_AUTO,       /* Automatically determine best locking method */
    64*1024*1024    /* 64MB texture memory by default */
};

WineD3DGlobalStatistics *wineD3DGlobalStatistics = NULL;

long globalChangeGlRam(long glram){
    /* FIXME: replace this function with object tracking */
    int result;

    wineD3DGlobalStatistics->glsurfaceram     += glram;
    TRACE("Adjusted gl ram by %ld to %d\n", glram, wineD3DGlobalStatistics->glsurfaceram);
    result = wineD3DGlobalStatistics->glsurfaceram;
    return result;

}

IWineD3D* WINAPI WineDirect3DCreate(UINT SDKVersion, UINT dxVersion, IUnknown *parent) {
    IWineD3DImpl* object;

    if (!InitAdapters()) {
        WARN("Failed to initialize direct3d adapters, Direct3D will not be available\n");
        if(dxVersion > 7) {
            ERR("Direct3D%d is not available without opengl\n", dxVersion);
            return NULL;
        }
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DImpl));
    object->lpVtbl = &IWineD3D_Vtbl;
    object->dxVersion = dxVersion;
    object->ref = 1;
    object->parent = parent;

    /*Create a structure for storing global data in*/
    if(wineD3DGlobalStatistics == NULL){
        TRACE("Creating global statistics store\n");
        wineD3DGlobalStatistics = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wineD3DGlobalStatistics));

    }

    TRACE("Created WineD3D object @ %p for d3d%d support\n", object, dxVersion);

    return (IWineD3D *)object;
}

static inline DWORD get_config_key(HKEY defkey, HKEY appkey, const char* name, char* buffer, DWORD size)
{
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

static void wined3d_do_nothing(void)
{
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("WineD3D DLLMain Reason=%d\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       HMODULE mod;
       char buffer[MAX_PATH+10];
       DWORD size = sizeof(buffer);
       HKEY hkey = 0;
       HKEY appkey = 0;
       DWORD len;
       WNDCLASSA wc;

       wined3d_settings.emulated_textureram = 64*1024*1024;

       /* We need our own window class for a fake window which we use to retrieve GL capabilities */
       /* We might need CS_OWNDC in the future if we notice strange things on Windows.
        * Various articles/posts about OpenGL problems on Windows recommend this. */
       wc.style                = CS_HREDRAW | CS_VREDRAW;
       wc.lpfnWndProc          = DefWindowProcA;
       wc.cbClsExtra           = 0;
       wc.cbWndExtra           = 0;
       wc.hInstance            = hInstDLL;
       wc.hIcon                = LoadIconA(NULL, (LPCSTR)IDI_WINLOGO);
       wc.hCursor              = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
       wc.hbrBackground        = NULL;
       wc.lpszMenuName         = NULL;
       wc.lpszClassName        = "WineD3D_OpenGL";

       if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
       {
           ERR("Failed to register window class 'WineD3D_OpenGL'!\n");
           return FALSE;
       }

       DisableThreadLibraryCalls(hInstDLL);

       mod = GetModuleHandleA( "winex11.drv" );
       if (mod)
       {
           wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
           wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
       }
       else /* We are most likely on Windows */
       {
           wine_tsx11_lock_ptr   = wined3d_do_nothing;
           wine_tsx11_unlock_ptr = wined3d_do_nothing;
       }
       /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
       if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &hkey ) ) hkey = 0;

       len = GetModuleFileNameA( 0, buffer, MAX_PATH );
       if (len && len < MAX_PATH)
       {
            HKEY tmpkey;
            /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
            if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
            {
                char *p, *appname = buffer;
                if ((p = strrchr( appname, '/' ))) appname = p + 1;
                if ((p = strrchr( appname, '\\' ))) appname = p + 1;
                strcat( appname, "\\Direct3D" );
                TRACE("appname = [%s]\n", appname);
                if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
                RegCloseKey( tmpkey );
            }
       }

       if ( 0 != hkey || 0 != appkey )
       {
            if ( !get_config_key( hkey, appkey, "VertexShaderMode", buffer, size) )
            {
                if (!strcmp(buffer,"none"))
                {
                    TRACE("Disable vertex shaders\n");
                    wined3d_settings.vs_mode = VS_NONE;
                }
            }
            if ( !get_config_key( hkey, appkey, "PixelShaderMode", buffer, size) )
            {
                if (!strcmp(buffer,"enabled"))
                {
                    TRACE("Allow pixel shaders\n");
                    wined3d_settings.ps_mode = PS_HW;
                }
                if (!strcmp(buffer,"disabled"))
                {
                    TRACE("Disable pixel shaders\n");
                    wined3d_settings.ps_mode = PS_NONE;
                }
            }
            if ( !get_config_key( hkey, appkey, "VertexBufferMode", buffer, size) )
            {
                if (!strcmp(buffer,"none"))
                {
                    TRACE("Disable Vertex Buffer Hardware support\n");
                    wined3d_settings.vbo_mode = VBO_NONE;
                }
                else if (!strcmp(buffer,"hardware"))
                {
                    TRACE("Allow Vertex Buffer Hardware support\n");
                    wined3d_settings.vbo_mode = VBO_HW;
                }
            }
            if ( !get_config_key( hkey, appkey, "UseGLSL", buffer, size) )
            {
                if (!strcmp(buffer,"enabled"))
                {
                    TRACE("Use of GL Shading Language enabled for systems that support it\n");
                    wined3d_settings.glslRequested = TRUE;
                }
                else
                {
                    TRACE("Use of GL Shading Language disabled\n");
                }
            }
            if ( !get_config_key( hkey, appkey, "OffscreenRenderingMode", buffer, size) )
            {
                if (!strcmp(buffer,"backbuffer"))
                {
                    TRACE("Using the backbuffer for offscreen rendering\n");
                    wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                }
                else if (!strcmp(buffer,"pbuffer"))
                {
                    TRACE("Using PBuffers for offscreen rendering\n");
                    wined3d_settings.offscreen_rendering_mode = ORM_PBUFFER;
                }
                else if (!strcmp(buffer,"fbo"))
                {
                    TRACE("Using FBOs for offscreen rendering\n");
                    wined3d_settings.offscreen_rendering_mode = ORM_FBO;
                }
            }
            if ( !get_config_key( hkey, appkey, "RenderTargetLockMode", buffer, size) )
            {
                if (!strcmp(buffer,"disabled"))
                {
                    TRACE("Disabling render target locking\n");
                    wined3d_settings.rendertargetlock_mode = RTL_DISABLE;
                }
                else if (!strcmp(buffer,"readdraw"))
                {
                    TRACE("Using glReadPixels for render target reading and glDrawPixels for writing\n");
                    wined3d_settings.rendertargetlock_mode = RTL_READDRAW;
                }
                else if (!strcmp(buffer,"readtex"))
                {
                    TRACE("Using glReadPixels for render target reading and textures for writing\n");
                    wined3d_settings.rendertargetlock_mode = RTL_READTEX;
                }
                else if (!strcmp(buffer,"texdraw"))
                {
                    TRACE("Using textures for render target reading and glDrawPixels for writing\n");
                    wined3d_settings.rendertargetlock_mode = RTL_TEXDRAW;
                }
                else if (!strcmp(buffer,"textex"))
                {
                    TRACE("Reading render targets via textures and writing via textures\n");
                    wined3d_settings.rendertargetlock_mode = RTL_TEXTEX;
                }
            }
            if ( !get_config_key( hkey, appkey, "VideoMemorySize", buffer, size) )
            {
                int TmpVideoMemorySize = atoi(buffer);
                if(TmpVideoMemorySize > 0)
                {
                    wined3d_settings.emulated_textureram = TmpVideoMemorySize *1024*1024;
                    TRACE("Use %iMB = %d byte for emulated_textureram\n",
                            TmpVideoMemorySize,
                            wined3d_settings.emulated_textureram);
                }
                else
                    ERR("VideoMemorySize is %i but must be >0\n", TmpVideoMemorySize);
            }
       }
       if (wined3d_settings.vs_mode == VS_HW)
           TRACE("Allow HW vertex shaders\n");
       if (wined3d_settings.ps_mode == PS_NONE)
           TRACE("Disable pixel shaders\n");
       if (wined3d_settings.vbo_mode == VBO_NONE)
           TRACE("Disable Vertex Buffer Hardware support\n");
       if (wined3d_settings.glslRequested)
           TRACE("If supported by your system, GL Shading Language will be used\n");

       if (appkey) RegCloseKey( appkey );
       if (hkey) RegCloseKey( hkey );
    }
    return TRUE;
}
