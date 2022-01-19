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

#define ANY_SIZE 1

/* Structures used by the API Interface */

typedef struct _DELETED_FILE_DETAILS_A
{
    FILETIME      LastModification;
    FILETIME      DeletionTime;
    ULARGE_INTEGER FileSize;
    ULARGE_INTEGER PhysicalFileSize;
    DWORD         Attributes;
    CHAR          FileName[ANY_SIZE];
} DELETED_FILE_DETAILS_A, *PDELETED_FILE_DETAILS_A;
typedef struct _DELETED_FILE_DETAILS_W
{
    FILETIME      LastModification;
    FILETIME      DeletionTime;
    ULARGE_INTEGER FileSize;
    ULARGE_INTEGER PhysicalFileSize;
    DWORD         Attributes;
    WCHAR         FileName[ANY_SIZE];
} DELETED_FILE_DETAILS_W, *PDELETED_FILE_DETAILS_W;
#ifdef UNICODE
#define DELETED_FILE_DETAILS  DELETED_FILE_DETAILS_W
#define PDELETED_FILE_DETAILS PDELETED_FILE_DETAILS_W
#else
#define DELETED_FILE_DETAILS  DELETED_FILE_DETAILS_A
#define PDELETED_FILE_DETAILS PDELETED_FILE_DETAILS_A
#endif

/* API Interface */

/* Function called for each deleted file in the recycle bin
 * Context: value given by the caller of the EnumerateRecycleBin function
 * hDeletedFile: a handle to the deleted file
 * Returning FALSE stops the enumeration.
 * Remarks: the handle must be closed with the CloseRecycleBinHandle function
 */
typedef BOOL (WINAPI *PENUMERATE_RECYCLEBIN_CALLBACK)(IN PVOID Context, IN HANDLE hDeletedFile);

/* Closes a file deleted handle.
 * hDeletedFile: the handle to close
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remark: The handle is obtained in the PENUMERATE_RECYCLEBIN_CALLBACK callback
 */
BOOL WINAPI
CloseRecycleBinHandle(
    IN HANDLE hDeletedFile);

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

/* Moves a file to the recycle bin.
 * hDeletedFile: handle of the deleted file to delete
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remark: The handle is obtained in the PENUMERATE_RECYCLEBIN_CALLBACK callback
 */
BOOL WINAPI
DeleteFileHandleToRecycleBin(
    IN HANDLE hDeletedFile);

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

BOOL WINAPI
GetDeletedFileTypeNameW(
    IN HANDLE hDeletedFile,
    OUT LPWSTR pTypeName,
    IN DWORD BufferSize,
    OUT LPDWORD RequiredSize OPTIONAL);

/* Gets details about a deleted file
 * hDeletedFile: handle of the deleted file to get details about
 * BufferSize: size of the 'FileDetails' buffer, in bytes
 * FileDetails: if the function succeeded, contains details about the deleted file
 * RequiredSize: contains the minimal buffer size required to get file information details
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remark: The handle is obtained in the PENUMERATE_RECYCLEBIN_CALLBACK callback
 */
BOOL WINAPI
GetDeletedFileDetailsA(
    IN HANDLE hDeletedFile,
    IN DWORD BufferSize,
    IN OUT PDELETED_FILE_DETAILS_A FileDetails OPTIONAL,
    OUT LPDWORD RequiredSize OPTIONAL);
BOOL WINAPI
GetDeletedFileDetailsW(
    IN HANDLE hDeletedFile,
    IN DWORD BufferSize,
    IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL,
    OUT LPDWORD RequiredSize OPTIONAL);
#ifdef UNICODE
#define GetDeletedFileDetails GetDeletedFileDetailsW
#else
#define GetDeletedFileDetails GetDeletedFileDetailsA
#endif

/* Get details about a whole recycle bin
 * pszVolume:
 * pulTotalItems:
 * pulTotalSize
 */
BOOL WINAPI
GetRecycleBinDetails(
    IN LPCWSTR pszVolume OPTIONAL,
    OUT ULARGE_INTEGER *pulTotalItems,
    OUT ULARGE_INTEGER *pulTotalSize);

/* Restores a deleted file
 * hDeletedFile: handle of the deleted file to restore
 * Returns TRUE if operation succeeded, FALSE otherwise.
 * Remarks: if the function succeeds, the handle is not valid anymore.
 */
BOOL WINAPI
RestoreFile(
    IN HANDLE hDeletedFile);

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
    STDMETHOD(GetLastModificationTime)(THIS_ FILETIME *pLastModificationTime) PURE;
    STDMETHOD(GetDeletionTime)(THIS_ FILETIME *pDeletionTime) PURE;
    STDMETHOD(GetFileSize)(THIS_ ULARGE_INTEGER *pFileSize) PURE;
    STDMETHOD(GetPhysicalFileSize)(THIS_ ULARGE_INTEGER *pPhysicalFileSize) PURE;
    STDMETHOD(GetAttributes)(THIS_ DWORD *pAttributes) PURE;
    STDMETHOD(GetFileName)(THIS_ SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize) PURE;
    STDMETHOD(GetTypeName)(THIS_ SIZE_T BufferSize, LPWSTR Buffer, SIZE_T *RequiredSize) PURE;
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
    STDMETHOD(Next)(THIS_ DWORD celt, IRecycleBinFile **rgelt, DWORD *pceltFetched);
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
    STDMETHOD(DeleteFile)(THIS_ LPCWSTR szFileName);
    STDMETHOD(EmptyRecycleBin)(THIS);
    STDMETHOD(EnumObjects)(THIS_ IRecycleBinEnumList **ppEnumList);

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
#define IRecycleBinFile_GetTypeName(This, BufferSize, Buffer, RequiredSize) \
    (This)->lpVtbl->GetTypeName(This, BufferSize, Buffer, RequiredSize)
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
#endif

HRESULT WINAPI
GetDefaultRecycleBin(
    IN LPCWSTR pszVolume OPTIONAL,
    OUT IRecycleBin **pprb);

#ifdef __cplusplus
}
#endif
