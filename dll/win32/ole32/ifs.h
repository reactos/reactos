/*
 * Copyright 1997 Marcus Meissner
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

#ifndef __WINE_OLE_IFS_H
#define __WINE_OLE_IFS_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

typedef CHAR OLECHAR16;
typedef LPSTR LPOLESTR16;
typedef LPCSTR LPCOLESTR16;

#define STDMETHOD16CALLTYPE __cdecl
#define STDMETHOD16(m) HRESULT (STDMETHOD16CALLTYPE *m)
#define STDMETHOD16_(t,m) t (STDMETHOD16CALLTYPE *m)


/***********************************************************************
 * IMalloc16 interface
 */

typedef struct IMalloc16 *LPMALLOC16;

#define INTERFACE IMalloc16
DECLARE_INTERFACE_(IMalloc16,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD16_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD16_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD16_(ULONG,Release)(THIS) PURE;
    /*** IMalloc16 methods ***/
    STDMETHOD16_(LPVOID,Alloc)(THIS_ DWORD   cb) PURE;
    STDMETHOD16_(LPVOID,Realloc)(THIS_ LPVOID  pv, DWORD  cb) PURE;
    STDMETHOD16_(void,Free)(THIS_ LPVOID  pv) PURE;
    STDMETHOD16_(DWORD,GetSize)(THIS_ LPVOID  pv) PURE;
    STDMETHOD16_(INT16,DidAlloc)(THIS_ LPVOID  pv) PURE;
    STDMETHOD16_(LPVOID,HeapMinimize)(THIS) PURE;
};
#undef INTERFACE

/**********************************************************************/

typedef struct ILockBytes16 *LPLOCKBYTES16;

#define INTERFACE ILockBytes16
DECLARE_INTERFACE_(ILockBytes16,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD16_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD16_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD16_(ULONG,Release)(THIS) PURE;
    /*** ILockBytes16 methods ***/
    STDMETHOD16(ReadAt)(THIS_ ULARGE_INTEGER ulOffset, void *pv, ULONG  cb, ULONG *pcbRead) PURE;
    STDMETHOD16(WriteAt)(THIS_ ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten) PURE;
    STDMETHOD16(Flush)(THIS) PURE;
    STDMETHOD16(SetSize)(THIS_ ULARGE_INTEGER cb) PURE;
    STDMETHOD16(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE;
    STDMETHOD16(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE;
    STDMETHOD16(Stat)(THIS_ STATSTG *pstatstg, DWORD grfStatFlag) PURE;
};
#undef INTERFACE

/**********************************************************************/

typedef struct tagSTATSTG16
{
    LPOLESTR16 pwcsName;
    DWORD type;
    ULARGE_INTEGER cbSize;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD grfMode;
    DWORD grfLocksSupported;
    CLSID clsid;
    DWORD grfStateBits;
    DWORD reserved;
} STATSTG16;

typedef struct IStream16 *LPSTREAM16;

#define INTERFACE IStream16
DECLARE_INTERFACE_(IStream16,ISequentialStream)
{
    /*** IUnknown methods ***/
    STDMETHOD16_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD16_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD16_(ULONG,Release)(THIS) PURE;
    /*** ISequentialStream methods ***/
    STDMETHOD16_(HRESULT,Read)(THIS_ void* pv, ULONG cb, ULONG* pcbRead) PURE;
    STDMETHOD16_(HRESULT,Write)(THIS_ const void* pv, ULONG cb, ULONG* pcbWritten) PURE;
    /*** IStream16 methods ***/
    STDMETHOD16(Seek)(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) PURE;
    STDMETHOD16(SetSize)(THIS_ ULARGE_INTEGER libNewSize) PURE;
    STDMETHOD16(CopyTo)(THIS_ IStream16* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) PURE;
    STDMETHOD16(Commit)(THIS_ DWORD grfCommitFlags) PURE;
    STDMETHOD16(Revert)(THIS) PURE;
    STDMETHOD16(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) PURE;
    STDMETHOD16(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) PURE;
    STDMETHOD16(Stat)(THIS_ STATSTG* pstatstg, DWORD grfStatFlag) PURE;
    STDMETHOD16(Clone)(THIS_ IStream16** ppstm) PURE;
};
#undef INTERFACE

/**********************************************************************/

typedef OLECHAR16 **SNB16;

typedef struct IStorage16 *LPSTORAGE16;

#define INTERFACE IStorage16
DECLARE_INTERFACE_(IStorage16,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD16_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD16_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD16_(ULONG,Release)(THIS) PURE;
    /*** IStorage16 methods ***/
    STDMETHOD16_(HRESULT,CreateStream)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream16** ppstm) PURE;
    STDMETHOD16_(HRESULT,OpenStream)(THIS_ LPCOLESTR16 pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream16** ppstm) PURE;
    STDMETHOD16_(HRESULT,CreateStorage)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage16** ppstg) PURE;
    STDMETHOD16_(HRESULT,OpenStorage)(THIS_ LPCOLESTR16 pwcsName, IStorage16* pstgPriority, DWORD grfMode, SNB16 snbExclude, DWORD reserved, IStorage16** ppstg) PURE;
    STDMETHOD16_(HRESULT,CopyTo)(THIS_ DWORD ciidExclude, const IID* rgiidExclude, SNB16 snbExclude, IStorage16* pstgDest) PURE;
    STDMETHOD16_(HRESULT,MoveElementTo)(THIS_ LPCOLESTR16 pwcsName, IStorage16* pstgDest, LPCOLESTR16 pwcsNewName, DWORD grfFlags) PURE;
    STDMETHOD16_(HRESULT,Commit)(THIS_ DWORD grfCommitFlags) PURE;
    STDMETHOD16_(HRESULT,Revert)(THIS) PURE;
    STDMETHOD16_(HRESULT,EnumElements)(THIS_ DWORD reserved1, void* reserved2, DWORD reserved3, IEnumSTATSTG** ppenum) PURE;
    STDMETHOD16_(HRESULT,DestroyElement)(THIS_ LPCOLESTR16 pwcsName) PURE;
    STDMETHOD16_(HRESULT,RenameElement)(THIS_ LPCOLESTR16 pwcsOldName, LPCOLESTR16 pwcsNewName) PURE;
    STDMETHOD16_(HRESULT,SetElementTimes)(THIS_ LPCOLESTR16 pwcsName, const FILETIME* pctime, const FILETIME* patime, const FILETIME* pmtime) PURE;
    STDMETHOD16_(HRESULT,SetClass)(THIS_ REFCLSID clsid) PURE;
    STDMETHOD16_(HRESULT,SetStateBits)(THIS_ DWORD grfStateBits, DWORD grfMask) PURE;
    STDMETHOD16_(HRESULT,Stat)(THIS_ STATSTG* pstatstg, DWORD grfStatFlag) PURE;
};
#undef INTERFACE

#endif /* __WINE_OLE_IFS_H */
