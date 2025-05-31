/*
 * Copyright (C) 2023 Paul Gofman for CodeWeavers
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

#ifndef __NTIORING_X_H_
#define __NTIORING_X_H_

enum IORING_VERSION
{
    IORING_VERSION_INVALID = 0,
    IORING_VERSION_1       = 1,
    IORING_VERSION_2       = 2,
    IORING_VERSION_3       = 300,
};
typedef enum IORING_VERSION IORING_VERSION;

enum IORING_FEATURE_FLAGS
{
    IORING_FEATURE_FLAGS_NONE           = 0,
    IORING_FEATURE_UM_EMULATION         = 0x00000001,
    IORING_FEATURE_SET_COMPLETION_EVENT = 0x00000002,
};
typedef enum IORING_FEATURE_FLAGS IORING_FEATURE_FLAGS;

#endif
