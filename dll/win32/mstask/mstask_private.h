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

extern LONG dll_ref;

typedef struct
{
    const IClassFactoryVtbl *lpVtbl;
    LONG ref;
} ClassFactoryImpl;
extern ClassFactoryImpl MSTASK_ClassFactory;

typedef struct
{
    const ITaskTriggerVtbl *lpVtbl;
    LONG ref;
    TASK_TRIGGER triggerCond;
} TaskTriggerImpl;
extern HRESULT TaskTriggerConstructor(LPVOID *ppObj);

typedef struct
{
    const ITaskSchedulerVtbl *lpVtbl;
    LONG ref;
} TaskSchedulerImpl;
extern HRESULT TaskSchedulerConstructor(LPVOID *ppObj);

typedef struct
{
    const ITaskVtbl *lpVtbl;
    const IPersistFileVtbl *persistVtbl;
    LONG ref;
    LPWSTR taskName;
    LPWSTR applicationName;
    LPWSTR parameters;
    LPWSTR comment;
    DWORD maxRunTime;
    LPWSTR accountName;
} TaskImpl;
extern HRESULT TaskConstructor(LPCWSTR pwszTaskName, LPVOID *ppObj);

#endif /* __MSTASK_PRIVATE_H__ */
