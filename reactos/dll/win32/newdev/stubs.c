/*
 * New device installer (newdev.dll)
 *
 * Copyright 2005 Hervé Poussineau (hpoussin@reactos.org)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "newdev_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(newdev);

/*
* @unimplemented
*/
BOOL WINAPI
InstallNewDevice(
	IN HWND hwndParent,
	IN LPGUID ClassGuid OPTIONAL,
	OUT PDWORD Reboot)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}

/*
* @unimplemented
*/
BOOL WINAPI
InstallSelectedDriverW(
	IN HWND hwndParent,
	IN HDEVINFO DeviceInfoSet,
	IN LPCWSTR Reserved,
	IN BOOL Backup,
	OUT PDWORD pReboot)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_GEN_FAILURE);
	return FALSE;
}
