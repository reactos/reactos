/*
 * Copyright 2012 Alistair Leslie-Hughes
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

#ifndef _SCRRUN_PRIVATE_H_
#define _SCRRUN_PRIVATE_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <objbase.h>
#include <oleauto.h>
#include <dispex.h>
#include <scrrun.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

extern HRESULT WINAPI FileSystem_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
extern HRESULT WINAPI Dictionary_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

typedef enum tid_t
{
    NULL_tid,
    IDictionary_tid,
    IDrive_tid,
    IDriveCollection_tid,
    IFile_tid,
    IFileCollection_tid,
    IFileSystem3_tid,
    IFolder_tid,
    IFolderCollection_tid,
    ITextStream_tid,
    LAST_tid
} tid_t;

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo) DECLSPEC_HIDDEN;

struct provideclassinfo {
    IProvideClassInfo IProvideClassInfo_iface;
    IUnknown *outer;
    const GUID *guid;
};

extern void init_classinfo(const GUID *guid, IUnknown *outer, struct provideclassinfo *classinfo) DECLSPEC_HIDDEN;

static inline void* __WINE_ALLOC_SIZE(1) heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#endif /* _SCRRUN_PRIVATE_H_ */
