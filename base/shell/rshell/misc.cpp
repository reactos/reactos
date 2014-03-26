/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

DWORD WINAPI WinList_Init(void)
{
    /* do something here (perhaps we may want to add our own implementation fo win8) */
    return 0;
}

void *operator new (size_t, void *buf)
{
    return buf;
}

class CRShellModule : public CComModule
{
public:
};

CRShellModule                               gModule;
CAtlWinModule                               gWinModule;

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID fImpLoad)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        /* HACK - the global constructors don't run, so I placement new them here */
        new (&gModule) CRShellModule;
        new (&gWinModule) CAtlWinModule;
        new (&_AtlBaseModule) CAtlBaseModule;
        new (&_AtlComModule) CAtlComModule;

        gModule.Init(NULL, hInstance, NULL);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        gModule.Term();
    }
    return TRUE;
}
