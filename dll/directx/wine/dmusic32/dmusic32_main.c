/* DirectMusic32 Main
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic32);

HRESULT WINAPI DMUSIC32_CreateCDirectMusicEmulatePort (LPVOID ptr1, LPVOID ptr2, LPVOID ptr3)
{	
	FIXME("stub (undocumented function); if you see this, you're probably using native dmusic.dll. Use native dmusic32.dll as well!\n");
	return S_OK;
}

HRESULT WINAPI DMUSIC32_EnumLegacyDevices (LPVOID ptr1, LPVOID ptr2)
{	
	FIXME("stub (undocumented function); if you see this, you're probably using native dmusic.dll. Use native dmusic32.dll as well!\n");
	return S_OK;
}
