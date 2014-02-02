/*
 * Copyright (C) 2006 Dmitry Timoshkov
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

#ifndef __WINE_MPRAPI_H
#define __WINE_MPRAPI_H

#include <lmcons.h>
#include <ras.h>
#include <in6addr.h>
#include <wincrypt.h>

#define MAX_DEVICE_NAME                 128
#define MAX_DEVICETYPE_NAME              16
#define MAX_INTERFACE_NAME_LEN          256
#define MAX_MEDIA_NAME                   16
#define MAX_PHONE_NUMBER_LEN            128
#define MAX_PORT_NAME                    16
#define MAX_TRANSPORT_NAME_LEN           40


#ifdef __cplusplus
extern "C" {
#endif

BOOL APIENTRY MprAdminIsServiceRunning(LPWSTR);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_MPRAPI_H */
