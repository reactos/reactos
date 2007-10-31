#ifndef __RECYCLEBIN_H
#define __RECYCLEBIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#define ANY_SIZE 1

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

typedef BOOL (WINAPI *PENUMERATE_RECYCLEBIN_CALLBACK)(IN PVOID Context, IN HANDLE hDeletedFile);

BOOL WINAPI
CloseRecycleBinHandle(
	IN HANDLE hDeletedFile);

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

BOOL WINAPI
EmptyRecycleBinA(
	IN LPCSTR pszRoot);
BOOL WINAPI
EmptyRecycleBinW(
	IN LPCWSTR pszRoot);
#ifdef UNICODE
#define EmptyRecycleBin EmptyRecycleBinW
#else
#define EmptyRecycleBin EmptyRecycleBinA
#endif

BOOL WINAPI
EnumerateRecycleBinA(
	IN CHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL);
BOOL WINAPI
EnumerateRecycleBinW(
	IN WCHAR driveLetter,
	IN PENUMERATE_RECYCLEBIN_CALLBACK pFnCallback,
	IN PVOID Context OPTIONAL);
#ifdef UNICODE
#define EnumerateRecycleBin EnumerateRecycleBinW
#else
#define EnumerateRecycleBin EnumerateRecycleBinA
#endif

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

BOOL WINAPI
RestoreFile(
	IN HANDLE hDeletedFile);

/* COM interface */

typedef interface IRecycleBinFile IRecycleBinFile;
EXTERN_C const IID IID_IRecycleBinFile;

typedef struct IRecycleBinFileVtbl
{
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(
		IN IRecycleBinFile *This,
		IN REFIID riid,
		OUT void **ppvObject);

	ULONG (STDMETHODCALLTYPE *AddRef)(
		IN IRecycleBinFile *This);

	ULONG (STDMETHODCALLTYPE *Release)(
		IN IRecycleBinFile *This);

	HRESULT (STDMETHODCALLTYPE *GetLastModificationTime)(
		IN IRecycleBinFile *This,
		OUT FILETIME *pLastModificationTime);

	HRESULT (STDMETHODCALLTYPE *GetDeletionTime)(
		IN IRecycleBinFile *This,
		OUT FILETIME *pDeletionTime);

	HRESULT (STDMETHODCALLTYPE *GetFileSize)(
		IN IRecycleBinFile *This,
		OUT ULARGE_INTEGER *pFileSize);

	HRESULT (STDMETHODCALLTYPE *GetPhysicalFileSize)(
		IN IRecycleBinFile *This,
		OUT ULARGE_INTEGER *pPhysicalFileSize);

	HRESULT (STDMETHODCALLTYPE *GetAttributes)(
		IN IRecycleBinFile *This,
		OUT DWORD *pAttributes);

	HRESULT (STDMETHODCALLTYPE *GetFileName)(
		IN IRecycleBinFile *This,
		IN SIZE_T BufferSize,
		IN OUT LPWSTR Buffer,
		OUT SIZE_T *RequiredSize);

	HRESULT (STDMETHODCALLTYPE *Delete)(
		IN IRecycleBinFile *This);

	HRESULT (STDMETHODCALLTYPE *Restore)(
		IN IRecycleBinFile *This);
} IRecycleBinFileVtbl;

interface IRecycleBinFile
{
	CONST_VTBL struct IRecycleBinFileVtbl *lpVtbl;
};

#ifdef COBJMACROS
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
#define IRecycleBinFile_Delete(This) \
	(This)->lpVtbl->Delete(This)
#define IRecycleBinFile_Restore(This) \
	(This)->lpVtbl->Restore(This)
#endif

typedef interface IRecycleBinEnumList IRecycleBinEnumList;
EXTERN_C const IID IID_IRecycleBinEnumList;

typedef struct IRecycleBinEnumListVtbl
{
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(
		IN IRecycleBinEnumList *This,
		IN REFIID riid,
		OUT void **ppvObject);

	ULONG (STDMETHODCALLTYPE *AddRef)(
		IN IRecycleBinEnumList *This);

	ULONG (STDMETHODCALLTYPE *Release)(
		IN IRecycleBinEnumList *This);

	HRESULT (STDMETHODCALLTYPE *Next)(
		IN IRecycleBinEnumList *This,
		IN DWORD celt,
		IN OUT IRecycleBinFile **rgelt,
		OUT DWORD *pceltFetched);

	HRESULT (STDMETHODCALLTYPE *Skip)(
		IN IRecycleBinEnumList *This,
		IN DWORD celt);

	HRESULT (STDMETHODCALLTYPE *Reset)(
		IN IRecycleBinEnumList *This);
} IRecycleBinEnumListVtbl;

interface IRecycleBinEnumList
{
	CONST_VTBL struct IRecycleBinEnumListVtbl *lpVtbl;
};

#ifdef COBJMACROS
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
#endif

typedef interface IRecycleBin IRecycleBin;
EXTERN_C const IID IID_IRecycleBin;

typedef struct IRecycleBinVtbl
{
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(
		IN IRecycleBin *This,
		IN REFIID riid,
		OUT void **ppvObject);

	ULONG (STDMETHODCALLTYPE *AddRef)(
		IN IRecycleBin *This);

	ULONG (STDMETHODCALLTYPE *Release)(
		IN IRecycleBin *This);

	HRESULT (STDMETHODCALLTYPE *DeleteFile)(
		IN IRecycleBin *This,
		IN LPCWSTR szFileName);

	HRESULT (STDMETHODCALLTYPE *EmptyRecycleBin)(
		IN IRecycleBin *This);

	HRESULT (STDMETHODCALLTYPE *EnumObjects)(
		IN IRecycleBin *This,
		OUT IRecycleBinEnumList **ppEnumList);
} IRecycleBinVtbl;

interface IRecycleBin
{
	CONST_VTBL struct IRecycleBinVtbl *lpVtbl;
};

#ifdef COBJMACROS
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

#endif /* __RECYCLEBIN_H */
