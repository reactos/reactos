/*
 * Copyright 2003 Martin Fuchs
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
 // Explorer and Desktop clone
 //
 // shellhook.h
 //
 // Martin Fuchs, 17.08.2003
 //


#ifdef _SHELLHOOK_IMPL
#define	DECL_SHELLHOOK __declspec(dllexport)
#else
#define	DECL_SHELLHOOK __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

DECL_SHELLHOOK void InstallShellHook(HWND callback_hwnd, UINT callback_msg);
DECL_SHELLHOOK void DeinstallShellHook();

#ifdef __cplusplus
};
#endif
