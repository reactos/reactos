/*
 * Copyright (C) 2006-2007 Stefan Dösinger(For CodeWeavers)
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

#define COBJMACROS
#include <d3d9.h>
#include <dxerr9.h>
#include "wine/test.h"

static IDirect3D9 *(WINAPI *pDirect3DCreate9)(UINT);

struct queryInfo
{
    D3DQUERYTYPE type;      /* Query to test */
    BOOL foundSupported;    /* If at least one windows driver has been found supporting this query */
    BOOL foundUnsupported;  /* If at least one windows driver has been found which does not support this query */
};

/* When running running this test on windows reveals any differences regarding known supported / unsupported queries,
 * change this table.
 *
 * When marking a query known supported or known unsupported please write one card which supports / does not support
 * the query.
 */
static struct queryInfo queries[] =
{
    {D3DQUERYTYPE_VCACHE,               TRUE /* geforce 6600 */,    TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_RESOURCEMANAGER,      FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_VERTEXSTATS,          FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_EVENT,                TRUE /* geforce 2 mx */,    TRUE /* ati mach64 */   },
    {D3DQUERYTYPE_OCCLUSION,            TRUE /* radeon M9 */,       TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_TIMESTAMP,            TRUE /* geforce 6600 */,    TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_TIMESTAMPDISJOINT,    TRUE /* geforce 6600 */,    TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_TIMESTAMPFREQ,        TRUE /* geforce 6600 */,    TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_PIPELINETIMINGS,      FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_INTERFACETIMINGS,     FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_VERTEXTIMINGS,        FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_PIXELTIMINGS,         FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_BANDWIDTHTIMINGS,     FALSE,                      TRUE /* geforce 2 mx */ },
    {D3DQUERYTYPE_CACHEUTILIZATION,     FALSE,                      TRUE /* geforce 2 mx */ },
};

static const char *queryName(D3DQUERYTYPE type)
{
    switch(type)
    {
        case D3DQUERYTYPE_VCACHE:               return "D3DQUERYTYPE_VCACHE";
        case D3DQUERYTYPE_RESOURCEMANAGER:      return "D3DQUERYTYPE_RESOURCEMANAGER";
        case D3DQUERYTYPE_VERTEXSTATS:          return "D3DQUERYTYPE_VERTEXSTATS";
        case D3DQUERYTYPE_EVENT:                return "D3DQUERYTYPE_EVENT";
        case D3DQUERYTYPE_OCCLUSION:            return "D3DQUERYTYPE_OCCLUSION";
        case D3DQUERYTYPE_TIMESTAMP:            return "D3DQUERYTYPE_TIMESTAMP";
        case D3DQUERYTYPE_TIMESTAMPDISJOINT:    return "D3DQUERYTYPE_TIMESTAMPDISJOINT";
        case D3DQUERYTYPE_TIMESTAMPFREQ:        return "D3DQUERYTYPE_TIMESTAMPFREQ";
        case D3DQUERYTYPE_PIPELINETIMINGS:      return "D3DQUERYTYPE_PIPELINETIMINGS";
        case D3DQUERYTYPE_INTERFACETIMINGS:     return "D3DQUERYTYPE_INTERFACETIMINGS";
        case D3DQUERYTYPE_VERTEXTIMINGS:        return "D3DQUERYTYPE_VERTEXTIMINGS";
        case D3DQUERYTYPE_PIXELTIMINGS:         return "D3DQUERYTYPE_PIXELTIMINGS";
        case D3DQUERYTYPE_BANDWIDTHTIMINGS:     return "D3DQUERYTYPE_BANDWIDTHTIMINGS";
        case D3DQUERYTYPE_CACHEUTILIZATION:     return "D3DQUERYTYPE_CACHEUTILIZATION";
        default: return "Unexpected query type";
    }
}

static void test_query_support(IDirect3D9 *pD3d, HWND hwnd)
{

    HRESULT               hr;

    IDirect3DDevice9      *pDevice = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DDISPLAYMODE        d3ddm;
    int                   i;
    IDirect3DQuery9       *pQuery = NULL;
    BOOL supported;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr), "Failed to create IDirect3D9Device (%s)\n", DXGetErrorString9(hr));
    if (FAILED(hr)) goto cleanup;

    for(i = 0; i < sizeof(queries) / sizeof(queries[0]); i++)
    {
        hr = IDirect3DDevice9_CreateQuery(pDevice, queries[i].type, NULL);
        ok(hr == D3D_OK || D3DERR_NOTAVAILABLE,
           "IDirect3DDevice9_CreateQuery returned unexpected return value %s for query %s\n", DXGetErrorString9(hr), queryName(queries[i].type));

        supported = (hr == D3D_OK ? TRUE : FALSE);
        trace("query %s is %s\n", queryName(queries[i].type), supported ? "supported" : "not supported");

        ok(!(supported == TRUE && queries[i].foundSupported == FALSE),
            "Query %s is supported on this system, but was not found supported before\n",
            queryName(queries[i].type));
        ok(!(supported == FALSE && queries[i].foundUnsupported == FALSE),
            "Query %s is not supported on this system, but was found to be supported on all other systems tested before\n",
            queryName(queries[i].type));

        hr = IDirect3DDevice9_CreateQuery(pDevice, queries[i].type, &pQuery);
        ok(hr == D3D_OK || D3DERR_NOTAVAILABLE,
           "IDirect3DDevice9_CreateQuery returned unexpected return value %s for query %s\n", DXGetErrorString9(hr), queryName(queries[i].type));
        ok(!(supported && !pQuery), "Query %s was claimed to be supported, but can't be created\n", queryName(queries[i].type));
        ok(!(!supported && pQuery), "Query %s was claimed not to be supported, but can be created\n", queryName(queries[i].type));
        if(pQuery)
        {
            IDirect3DQuery9_Release(pQuery);
            pQuery = NULL;
        }
    }

    cleanup:
    if(pDevice) IDirect3DDevice9_Release(pDevice);
}

START_TEST(query)
{
    HMODULE d3d9_handle = LoadLibraryA( "d3d9.dll" );
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    pDirect3DCreate9 = (void *)GetProcAddress( d3d9_handle, "Direct3DCreate9" );
    ok(pDirect3DCreate9 != NULL, "Failed to get address of Direct3DCreate9\n");
    if (pDirect3DCreate9)
    {
        IDirect3D9            *pD3d = NULL;
        HWND                  hwnd = NULL;

        pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
        if(!pD3d)
        {
            skip("Failed to create Direct3D9 object, not running tests\n");
            return;
        }
        hwnd = CreateWindow( "static", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
        if(!hwnd)
        {
            skip("Failed to create window\n");
            IDirect3D9_Release(pD3d);
            return;
        }

        test_query_support(pD3d, hwnd);

        DestroyWindow(hwnd);
        IDirect3D9_Release(pD3d);
    }
}
