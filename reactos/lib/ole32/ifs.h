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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/***********************************************************************
 * IMalloc16 interface
 */

typedef struct IMalloc16 IMalloc16, *LPMALLOC16;

#undef INTERFACE
#define INTERFACE IMalloc16
#define IMalloc16_METHODS \
    IUnknown_METHODS \
    STDMETHOD_(LPVOID,Alloc)(THIS_ DWORD   cb) PURE; \
    STDMETHOD_(LPVOID,Realloc)(THIS_ LPVOID  pv, DWORD  cb) PURE; \
    STDMETHOD_(void,Free)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(DWORD,GetSize)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(INT16,DidAlloc)(THIS_ LPVOID  pv) PURE; \
    STDMETHOD_(LPVOID,HeapMinimize)(THIS) PURE;
ICOM_DEFINE(IMalloc16,IUnknown)
#undef INTERFACE

/**********************************************************************/

extern LPMALLOC16 IMalloc16_Constructor();

/**********************************************************************/

typedef struct ILockBytes16 *LPLOCKBYTES16, ILockBytes16;

#define INTERFACE ILockBytes
#define ILockBytes16_METHODS \
	IUnknown_METHODS \
	STDMETHOD(ReadAt)(THIS_ ULARGE_INTEGER ulOffset, void *pv, ULONG  cb, ULONG *pcbRead) PURE; \
	STDMETHOD(WriteAt)(THIS_ ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten) PURE; \
	STDMETHOD(Flush)(THIS) PURE; \
	STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER cb) PURE; \
	STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE; \
	STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER  cb, DWORD dwLockType) PURE; \
	STDMETHOD(Stat)(THIS_ STATSTG *pstatstg, DWORD grfStatFlag) PURE;
ICOM_DEFINE(ILockBytes16,IUnknown)
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

typedef struct IStream16 IStream16, *LPSTREAM16;

#define INTERFACE IStream16
#define IStream16_METHODS \
    ISequentialStream_METHODS \
    STDMETHOD(Seek)(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) PURE; \
    STDMETHOD(SetSize)(THIS_ ULARGE_INTEGER libNewSize) PURE; \
    STDMETHOD(CopyTo)(THIS_ IStream16* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) PURE; \
    STDMETHOD(Commit)(THIS_ DWORD grfCommitFlags) PURE; \
    STDMETHOD(Revert)(THIS) PURE; \
    STDMETHOD(LockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) PURE; \
    STDMETHOD(UnlockRegion)(THIS_ ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) PURE; \
    STDMETHOD(Stat)(THIS_ STATSTG* pstatstg, DWORD grfStatFlag) PURE; \
    STDMETHOD(Clone)(THIS_ IStream16** ppstm) PURE;
ICOM_DEFINE(IStream16,ISequentialStream)
#undef INTERFACE

/**********************************************************************/

typedef OLECHAR16 **SNB16;

typedef struct IStorage16 IStorage16, *LPSTORAGE16;

#define INTERFACE IStorage16
#define IStorage16_METHODS \
    IUnknown_METHODS \
    STDMETHOD_(HRESULT,CreateStream)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream16** ppstm) PURE; \
    STDMETHOD_(HRESULT,OpenStream)(THIS_ LPCOLESTR16 pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream16** ppstm) PURE; \
    STDMETHOD_(HRESULT,CreateStorage)(THIS_ LPCOLESTR16 pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage16** ppstg) PURE; \
    STDMETHOD_(HRESULT,OpenStorage)(THIS_ LPCOLESTR16 pwcsName, IStorage16* pstgPriority, DWORD grfMode, SNB16 snbExclude, DWORD reserved, IStorage16** ppstg) PURE; \
    STDMETHOD_(HRESULT,CopyTo)(THIS_ DWORD ciidExclude, const IID* rgiidExclude, SNB16 snbExclude, IStorage16* pstgDest) PURE; \
    STDMETHOD_(HRESULT,MoveElementTo)(THIS_ LPCOLESTR16 pwcsName, IStorage16* pstgDest, LPCOLESTR16 pwcsNewName, DWORD grfFlags) PURE; \
    STDMETHOD_(HRESULT,Commit)(THIS_ DWORD grfCommitFlags) PURE; \
    STDMETHOD_(HRESULT,Revert)(THIS) PURE; \
    STDMETHOD_(HRESULT,EnumElements)(THIS_ DWORD reserved1, void* reserved2, DWORD reserved3, IEnumSTATSTG** ppenum) PURE; \
    STDMETHOD_(HRESULT,DestroyElement)(THIS_ LPCOLESTR16 pwcsName) PURE; \
    STDMETHOD_(HRESULT,RenameElement)(THIS_ LPCOLESTR16 pwcsOldName, LPCOLESTR16 pwcsNewName) PURE; \
    STDMETHOD_(HRESULT,SetElementTimes)(THIS_ LPCOLESTR16 pwcsName, const FILETIME* pctime, const FILETIME* patime, const FILETIME* pmtime) PURE; \
    STDMETHOD_(HRESULT,SetClass)(THIS_ REFCLSID clsid) PURE; \
    STDMETHOD_(HRESULT,SetStateBits)(THIS_ DWORD grfStateBits, DWORD grfMask) PURE; \
    STDMETHOD_(HRESULT,Stat)(THIS_ STATSTG* pstatstg, DWORD grfStatFlag) PURE;
ICOM_DEFINE(IStorage16,IUnknown)
#undef INTERFACE

#endif /* __WINE_OLE_IFS_H */
