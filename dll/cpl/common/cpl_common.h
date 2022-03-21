/**
 * @file cpl_common.h
 * @brief Auxiliary functions for the realization
 *        of a one-instance cpl applet
 *
 * Copyright 2022 Raymond Czerny
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

#ifndef _CPL_COMMON_H_
#define _CPL_COMMON_H_

#include <windows.h>

HWND CPL_GetHWndByCaption(const WCHAR* Caption);
HWND CPL_GetHWndByResource(HINSTANCE hInstance, UINT uID);

#endif
