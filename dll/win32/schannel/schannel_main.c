/*
 * SSL/TLS Security Library
 *
 * Copyright 2007 Rob Shearman, for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#ifdef __REACTOS__
#include <sspi.h>
#include <schannel.h>
#include <wine/list.h>
#include "schannel_priv.h"
#endif

WINE_DEFAULT_DEBUG_CHANNEL(schannel);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

#ifndef __REACTOS__
	if (fdwReason == DLL_WINE_PREATTACH) return FALSE;	/* prefer native version */
#endif

	if (fdwReason == DLL_PROCESS_ATTACH)
#ifdef __REACTOS__
	{
#endif
		DisableThreadLibraryCalls(hinstDLL);
#ifdef __REACTOS__
		SECUR32_initSchannelSP();
	}
#endif
	return TRUE;
}

BOOL WINAPI SslEmptyCacheA(LPSTR target, DWORD flags)
{
    FIXME("%s %x\n", debugstr_a(target), flags);
    return TRUE;
}

BOOL WINAPI SslEmptyCacheW(LPWSTR target, DWORD flags)
{
    FIXME("%s %x\n", debugstr_w(target), flags);
    return TRUE;
}

#ifdef __REACTOS__

PSecurityFunctionTableW
WINAPI
schan_InitSecurityInterfaceW(VOID)
{
    TRACE("InitSecurityInterfaceW() called\n");
    return &schanTableW;
}

PSecurityFunctionTableA
WINAPI
schan_InitSecurityInterfaceA(VOID)
{
    TRACE("InitSecurityInterfaceA() called\n");
    return &schanTableA;
}

#endif /* __REACTOS__ */
