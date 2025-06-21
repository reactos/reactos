/*
 * Copyright 2016-2018 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_TYPES_H
#define __VKD3D_TYPES_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * \file vkd3d_types.h
 *
 * This file contains definitions for basic types used by vkd3d libraries.
 */

#define VKD3D_FORCE_32_BIT_ENUM(name) name##_FORCE_32BIT = 0x7fffffff

/**
 * Result codes returned by some vkd3d functions. Error codes always have
 * negative values; non-error codes never do.
 */
enum vkd3d_result
{
    /** Success. */
    VKD3D_OK = 0,
    /** Success as a result of there being nothing to do. \since 1.12 */
    VKD3D_FALSE = 1,
    /** An unspecified failure occurred. */
    VKD3D_ERROR = -1,
    /** There are not enough resources available to complete the operation. */
    VKD3D_ERROR_OUT_OF_MEMORY = -2,
    /** One or more parameters passed to a vkd3d function were invalid. */
    VKD3D_ERROR_INVALID_ARGUMENT = -3,
    /** A shader passed to a vkd3d function was invalid. */
    VKD3D_ERROR_INVALID_SHADER = -4,
    /** The operation is not implemented in this version of vkd3d. */
    VKD3D_ERROR_NOT_IMPLEMENTED = -5,
    /** The object or entry already exists. \since 1.12 */
    VKD3D_ERROR_KEY_ALREADY_EXISTS = -6,
    /** The requested object was not found. \since 1.12 */
    VKD3D_ERROR_NOT_FOUND = -7,
    /** The output buffer is larger than the requested object \since 1.12. */
    VKD3D_ERROR_MORE_DATA = -8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_RESULT),
};

typedef void (*PFN_vkd3d_log)(const char *format, va_list args);

#ifdef _WIN32
# define VKD3D_IMPORT
# define VKD3D_EXPORT
#elif defined(__GNUC__)
# define VKD3D_IMPORT
# define VKD3D_EXPORT __attribute__((visibility("default")))
#else
# define VKD3D_IMPORT
# define VKD3D_EXPORT
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __VKD3D_TYPES_H */
