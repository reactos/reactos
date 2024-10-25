#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winver.h>
#include <shellapi.h>
#include <objbase.h>

#define RECYCLEBINMAXDRIVECOUNT 26

/* Structures used by the API Interface */

typedef UINT RECYCLEBINFILESIZETYPE;

typedef struct _RECYCLEBINFILEIDENTITY
{
    FILETIME DeletionTime;
    LPCWSTR RecycledFullPath; /* "C:\Recycled\Dc1.ext" etc. */
} RECYCLEBINFILEIDENTITY, *PRECYCLEBINFILEIDENTITY;

typedef struct _RECYCLEBINSTRING
{
    LPCWSTR String;
    LPWSTR Alloc;
} RECYCLEBINSTRING, *PRECYCLEBINSTRING;

typedef struct _DELETED_FILE_INFO
{
    FILETIME LastModification;
    FILETIME DeletionTime;
    RECYCLEBINFILESIZETYPE FileSize;
    DWORD Attributes;
    RECYCLEBINSTRING OriginalFullPath;
    RECYCLEBINSTRING RecycledFullPath;
} DELETED_FILE_INFO, *PDELETED_FILE_INFO;

/* Distinct handle type for deleted file/folder */
DECLARE_HANDLE(HDELFILE);
#define IRecycleBinFileFromHDELFILE(hDF) ( (IRecycleBinFile*)(hDF) )

/* API Interface */

static inline void
FreeRecycleBinString(PRECYCLEBINSTRING pRBS)
{
    SHFree(pRBS->Alloc);
    pRBS->String = pRBS->Alloc = NULL;
}

static inline void
InitializeRecycleBinStringRef(PRECYCLEBINSTRING pRBS, LPCWSTR String)
{
    pRBS->String = String;
    pRBS->Alloc = NULL;
}

EXTERN_C HRESULT
GetRecycleBinPathFromDriveNumber(UINT Drive, LPWSTR Path);

/* Function called for each deleted file in the recycle bin
 * Context: value given by the caller of the EnumerateRecycleBin function
 * hDeletedFile: a handle to the deleted file
 * Returning FALSE stops the enumeration.
 * Remarks: the handle must be closed with the CloseRecycleBinHandle function
 */
typedef BOOL (CALLBACK *PENUMERATE_RECYCLEBIN_CALLBACK)(IN PVOID Context, IN HDELFILE hDeletedFile);

/* Closes a file deleted handle.
 * hDeletedFile: the handle to close
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remark: The handle is obtained in the PENUMERATE_RECYCLEBIN_CALLBACK callback
 */
BOOL WINAPI
CloseRecycleBinHandle(
    IN HDELFILE hDeletedFile);

/* Moves a file to the recycle bin.
 * FileName: the name of the file to move the recycle bin
 * Returns TRUE if operation succeeded, FALSE otherwise.
 */
BOOL WINAPI
DeleteFileToRecycleBinA(
    IN LPCSTR FileName);
BOOL WINAPI
DeleteFileToRecycleBinW(
    IN LPCWSTR FileName);
#ifdef UNICODE
#define DeleteFileToRecycleBin DeleteFileToRecycleBinW
#else
#define DeleteFileToRecycleBin DeleteFileToRecycleBinA
#endif

/* Deletes a file in the recycle bin.
 * hDeletedFile: handle of the deleted file to delete
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remark: The handle is obtained in the PENUMERATE_RECYCLEBIN_CALLBACK callback
 */
BOOL WINAPI
DeleteFileInRecycleBin(
    IN HDELFILE hDeletedFile);

/* Removes all elements contained in a recycle bin
 * pszRoot: the name of the drive containing the recycle bin
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remarks: 'pszRoot' can be NULL to mean 'all recycle bins'.
 */
BOOL WINAPI
EmptyRecycleBinA(
    IN LPCSTR pszRoot OPTIONAL);
BOOL WINAPI
EmptyRecycleBinW(
    IN LPCWSTR pszRoot OPTIONAL);
#ifdef UNICODE
#define EmptyRecycleBin EmptyRecycleBinW
#else
#define EmptyRecycleBin EmptyRecycleBinA
#endif

/* Enumerate contents of a recycle bin.
 * pszRoot: the name of the drive containing the recycle bin
 * pFnCallback: callback function to be called for each deleted item found
 * Context: some value which will be given back in the callback function
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remarks: 'pszRoot' can be NULL to mean 'all recycle bins'.
 */
BOOL WINAPI
EnumerateRecycleBinA(
    IN LPCSTR pszRoot OPTIONAL,
    IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
    IN PVOID Context OPTIONAL);
BOOL WINAPI
EnumerateRecycleBinW(
    IN LPCWSTR pszRoot OPTIONAL,
    IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
    IN PVOID Context OPTIONAL);
#ifdef UNICODE
#define EnumerateRecycleBin EnumerateRecycleBinW
#else
#define EnumerateRecycleBin EnumerateRecycleBinA
#endif

EXTERN_C HDELFILE
GetRecycleBinFileHandle(
    IN LPCWSTR pszRoot OPTIONAL,
    IN const RECYCLEBINFILEIDENTITY *pFI);

/* Restores a deleted file
 * hDeletedFile: handle of the deleted file to restore
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remarks: if the function succeeds, the handle is not valid anymore.
 */
BOOL WINAPI
RestoreFileFromRecycleBin(
    IN HDELFILE hDeletedFile);

/* COM interface */

#define INTERFACE IRecycleBinFile
DECLARE_INTERFACE_(IRecycleBinFile, IUnknown)
{
    BEGIN_INTERFACE

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IRecycleBinFile methods */
    STDMETHOD(IsEqualIdentity)(THIS_ const RECYCLEBINFILEIDENTITY *pFI) PURE;
    STDMETHOD(GetInfo)(THIS_ PDELETED_FILE_INFO pInfo) PURE;
    STDMETHOD(GetLastModificationTime)(THIS_ FILETIME *pLastModificationTime) PURE;
    STDMETHOD(GetDeletionTime)(THIS_ FILETIME *pDeletionTime) PURE;
    STDMETHOD(GetFileSize)(THIS_ ULARGE_INTEGER *pFileSize) PURE;
    STDMETHOD(GetPhysicalFileSize)(THIS_ ULARGE_INTEGER *pPhysicalFileSize) PURE;
    STDMETHOD(GetAttributes)(THIS_ DWORD *pAttributes) PURE;
    STDMETHOD(GetFileName)(THIS_ SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize) PURE;
    STDMETHOD(Delete)(THIS) PURE;
    STDMETHOD(Restore)(THIS) PURE;

    END_INTERFACE
};
#undef INTERFACE

#define INTERFACE IRecycleBinEnumList
DECLARE_INTERFACE_(IRecycleBinEnumList, IUnknown)
{
    BEGIN_INTERFACE

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IRecycleBinEnumList methods */
    STDMETHOD(Next)(THIS_ DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ DWORD celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;

    END_INTERFACE
};
#undef INTERFACE

#define INTERFACE IRecycleBin
DECLARE_INTERFACE_(IRecycleBin, IUnknown)
{
    BEGIN_INTERFACE

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IRecycleBin methods */
    STDMETHOD(DeleteFile)(THIS_ LPCWSTR szFileName) PURE;
    STDMETHOD(EmptyRecycleBin)(THIS) PURE;
    STDMETHOD(EnumObjects)(THIS_ IRecycleBinEnumList **ppEnumList) PURE;
    STDMETHOD(GetDirectory)(THIS_ LPWSTR szPath) PURE;

    END_INTERFACE
};
#undef INTERFACE

EXTERN_C const IID IID_IRecycleBinFile;
EXTERN_C const IID IID_IRecycleBinEnumList;
EXTERN_C const IID IID_IRecycleBin;

#if (!defined(__cplusplus) || defined(CINTERFACE)) && defined(COBJMACROS)
#define IRecycleBinFile_QueryInterface(This, riid, ppvObject) \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IRecycleBinFile_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IRecycleBinFile_Release(This) \
    (This)->lpVtbl->Release(This)
#define IRecycleBinFile_IsEqualIdentity(This, pFI) \
    (This)->lpVtbl->IsEqualIdentity(This, pFI)
#define IRecycleBinFile_GetInfo(This, pInfo) \
    (This)->lpVtbl->GetInfo(This, pInfo)
#define IRecycleBinFile_GetLastModificationTime(This, pLastModificationTime) \
    (This)->lpVtbl->GetLastModificationTime(This, pLastModificationTime)
#define IRecycleBinFile_GetDeletionTime(This, pDeletionTime) \
    (This)->lpVtbl->GetDeletionTime(This, pDeletionTime)
#define IRecycleBinFile_GetFileSize(This, pFileSize) \
    (This)->lpVtbl->GetFileSize(This, pFileSize)
#define IRecycleBinFile_GetPhysicalFileSize(This, pPhysicalFileSize) \
    (This)->lpVtbl->GetPhysicalFileSize(This, pPhysicalFileSize)
#define IRecycleBinFile_GetAttributes(This, pAttributes) \
    (This)->lpVtbl->GetAttributes(This, pAttributes)
#define IRecycleBinFile_GetFileName(This, BufferSize, Buffer, RequiredSize) \
    (This)->lpVtbl->GetFileName(This, BufferSize, Buffer, RequiredSize)
#define IRecycleBinFile_Delete(This) \
    (This)->lpVtbl->Delete(This)
#define IRecycleBinFile_Restore(This) \
    (This)->lpVtbl->Restore(This)

#define IRecycleBinEnumList_QueryInterface(This, riid, ppvObject) \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IRecycleBinEnumList_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IRecycleBinEnumList_Release(This) \
    (This)->lpVtbl->Release(This)
#define IRecycleBinEnumList_Next(This, celt, rgelt, pceltFetched) \
    (This)->lpVtbl->Next(This, celt, rgelt, pceltFetched)
#define IRecycleBinEnumList_Skip(This, celt) \
    (This)->lpVtbl->Skip(This, celt)
#define IRecycleBinEnumList_Reset(This) \
    (This)->lpVtbl->Reset(This)

#define IRecycleBin_QueryInterface(This, riid, ppvObject) \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IRecycleBin_AddRef(This) \
    (This)->lpVtbl->AddRef(This)
#define IRecycleBin_Release(This) \
    (This)->lpVtbl->Release(This)
#define IRecycleBin_DeleteFile(This, szFileName) \
    (This)->lpVtbl->DeleteFile(This, szFileName)
#define IRecycleBin_EmptyRecycleBin(This) \
    (This)->lpVtbl->EmptyRecycleBin(This)
#define IRecycleBin_EnumObjects(This, ppEnumList) \
    (This)->lpVtbl->EnumObjects(This, ppEnumList)
#define IRecycleBin_GetDirectory(This, szPath) \
    (This)->lpVtbl->GetDirectory(This, szPath)
#endif

EXTERN_C HRESULT
GetDefaultRecycleBin(
    IN LPCWSTR pszVolume OPTIONAL,
    OUT IRecycleBin **pprb);

/* Recycle Bin shell folder internal API */
void CRecycleBin_NotifyRecycled(LPCWSTR OrigPath, const WIN32_FIND_DATAW *pFind,
                                const RECYCLEBINFILEIDENTITY *pFI);


#ifdef __cplusplus
}
#endif
