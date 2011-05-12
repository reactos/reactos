/*
 * Copyright 2011 Samuel Serapión
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
#include "ntlm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);


BOOL SetupIsActive(VOID);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);

        /* hack: rsaehn has still not registered its crypto providers */
        /* its not like we are going to logon to anything yet */
        if(!SetupIsActive())
        {
            //REACTOS BUG: even after 2nd stage crypto providers are not available!
            //NtlmInitializeRNG();
            //NtlmInitializeProtectedMemory();
        }
        NtlmCredentialInitialize();
        NtlmContextInitialize();
        break;
    case DLL_PROCESS_DETACH:
        NtlmContextTerminate();
        NtlmCredentialTerminate();
        NtlmTerminateRNG();
        NtlmTerminateProtectedMemory();
        break;
    default:
        break;
    }
    return TRUE;
}
