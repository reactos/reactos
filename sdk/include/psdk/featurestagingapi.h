/*
 * Copyright (C) 2021 Alex Henrie
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

#ifndef _FEATURESTAGINGAPI_H_
#define _FEATURESTAGINGAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(FEATURE_STATE_CHANGE_SUBSCRIPTION);

typedef void WINAPI FEATURE_STATE_CHANGE_CALLBACK(void*);

typedef enum FEATURE_CHANGE_TIME
{
    FEATURE_CHANGE_TIME_READ,
    FEATURE_CHANGE_TIME_MODULE_RELOAD,
    FEATURE_CHANGE_TIME_SESSION,
    FEATURE_CHANGE_TIME_REBOOT
} FEATURE_CHANGE_TIME;

typedef enum FEATURE_ENABLED_STATE
{
    FEATURE_ENABLED_STATE_DEFAULT,
    FEATURE_ENABLED_STATE_DISABLED,
    FEATURE_ENABLED_STATE_ENABLED
} FEATURE_ENABLED_STATE;

#ifdef __cplusplus
}
#endif

#endif
