/*
 * Copyright (C) 2008 Google (Roy Shea)
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

#ifndef __MSTASK_PRIVATE_H__
#define __MSTASK_PRIVATE_H__

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mstask.h"

extern LONG dll_ref DECLSPEC_HIDDEN;

typedef struct ClassFactoryImpl ClassFactoryImpl;
extern ClassFactoryImpl MSTASK_ClassFactory DECLSPEC_HIDDEN;

extern HRESULT TaskTriggerConstructor(LPVOID *ppObj) DECLSPEC_HIDDEN;
extern HRESULT TaskSchedulerConstructor(LPVOID *ppObj) DECLSPEC_HIDDEN;
extern HRESULT TaskConstructor(LPCWSTR pwszTaskName, LPVOID *ppObj) DECLSPEC_HIDDEN;

#endif /* __MSTASK_PRIVATE_H__ */
