/*
 * Copyright 2019 Zebediah Figura
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

#ifndef __WINE_WINE_HTTP_H
#define __WINE_WINE_HTTP_H

#include <windef.h>
#include <http.h>
#include <winioctl.h>

#define IOCTL_HTTP_ADD_URL          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, 0)
#define IOCTL_HTTP_REMOVE_URL       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, 0)
#define IOCTL_HTTP_RECEIVE_REQUEST  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, 0)
#define IOCTL_HTTP_SEND_RESPONSE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, 0)

struct http_add_url_params
{
    HTTP_URL_CONTEXT context;
    char url[1];
};

struct http_receive_request_params
{
    ULONGLONG addr; /* user-mode buffer address */
    HTTP_REQUEST_ID id;
    ULONG flags;
    ULONG bits;
};

struct http_response
{
    HTTP_REQUEST_ID id;
    int len;
    char buffer[1];
};

#endif
