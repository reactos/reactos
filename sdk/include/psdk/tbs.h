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

#ifndef _TBS_H_
#define _TBS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef UINT32 TBS_RESULT;

typedef struct tdTBS_CONTEXT_PARAMS
{
    UINT32 version;
} TBS_CONTEXT_PARAMS, *PTBS_CONTEXT_PARAMS;

typedef void *TBS_HCONTEXT, **PTBS_HCONTEXT;

#ifdef __cplusplus
}
#endif

#endif
