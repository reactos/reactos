/*
 * Direct3D X 9 main file
 *
 * Copyright (C) 2007 David Adam
 * Copyright (C) 2008 Tony Wasserka
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
 *
 */

#include "d3dx9_36_private.h"

/***********************************************************************
 * DllMain.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    }
    return TRUE;
}

/***********************************************************************
 * D3DXCheckVersion
 * Checks whether we are compiling against the correct d3d and d3dx library.
 */
BOOL WINAPI D3DXCheckVersion(UINT d3dsdkvers, UINT d3dxsdkvers)
{
    if(d3dsdkvers==D3D_SDK_VERSION && d3dxsdkvers==36)
        return TRUE;
    else
        return FALSE;
}

/***********************************************************************
 * D3DXDebugMute
 * Returns always FALSE for us.
 */
BOOL WINAPI D3DXDebugMute(BOOL mute)
{
    return FALSE;
}

/***********************************************************************
 * D3DXGetDriverLevel.
 * Returns always 900 (DX 9) for us
 */
UINT WINAPI D3DXGetDriverLevel(struct IDirect3DDevice9 *device)
{
    return 900;
}
