/*
 * Copyright 2004 Martin Fuchs
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


 //
 // NotifyHook DLL for ROS Explorer
 //
 // notifyhook.h
 //
 // Martin Fuchs, 17.03.2004
 //


#ifdef _NOTIFYHOOK_IMPL
#define	DECL_NOTIFYHOOK __declspec(dllexport)
#else
#define	DECL_NOTIFYHOOK __declspec(dllimport)
#ifdef _MSC_VER
#pragma comment(lib, "notifyhook")
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

DECL_NOTIFYHOOK UINT InstallNotifyHook();
DECL_NOTIFYHOOK void DeinstallNotifyHook();

DECL_NOTIFYHOOK void GetWindowModulePath(HWND hwnd);
DECL_NOTIFYHOOK int GetWindowModulePathCopyData(LPARAM lparam, HWND* phwnd, LPSTR buffer, int size);

#ifdef __cplusplus
};
#endif
