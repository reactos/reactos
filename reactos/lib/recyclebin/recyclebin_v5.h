/* Recycle bin management
 * This file is under the GPLv2 licence
 * Copyright (C) 2006 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "recyclebin_private.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack1.h>

/* MS Windows 2000/XP/2003 */
typedef struct _DELETED_FILE_RECORD
{
	CHAR FileNameA[MAX_PATH];
	DWORD dwRecordUniqueId;
	DWORD dwDriveNumber;
	FILETIME DeletionTime;
	DWORD dwPhysicalFileSize;
	WCHAR FileNameW[MAX_PATH];
} DELETED_FILE_RECORD, *PDELETED_FILE_RECORD;

#include <poppack.h>

/* COM interface */

typedef interface IRecycleBin5 IRecycleBin5;
EXTERN_C const IID IID_IRecycleBin5;

typedef struct IRecycleBin5Vtbl
{
	/* IRecycleBin interface */
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(
		IN IRecycleBin5 *This,
		IN REFIID riid,
		OUT void **ppvObject);

	ULONG (STDMETHODCALLTYPE *AddRef)(
		IN IRecycleBin5 *This);

	ULONG (STDMETHODCALLTYPE *Release)(
		IN IRecycleBin5 *This);

	HRESULT (STDMETHODCALLTYPE *DeleteFile)(
		IN IRecycleBin5 *This,
		IN LPCWSTR szFileName);

	HRESULT (STDMETHODCALLTYPE *EmptyRecycleBin)(
		IN IRecycleBin5 *This);

	HRESULT (STDMETHODCALLTYPE *EnumObjects)(
		IN IRecycleBin5 *This,
		OUT IRecycleBinEnumList **ppEnumList);

	/* IRecycleBin5 interface */
	HRESULT (STDMETHODCALLTYPE *Delete)(
		IN IRecycleBin5 *This,
		IN LPCWSTR pDeletedFileName,
		IN DELETED_FILE_RECORD *pDeletedFile);

	HRESULT (STDMETHODCALLTYPE *Restore)(
		IN IRecycleBin5 *This,
		IN LPCWSTR pDeletedFileName,
		IN DELETED_FILE_RECORD *pDeletedFile);

	HRESULT (STDMETHODCALLTYPE *OnClosing)(
		IN IRecycleBin5 *This,
		IN IRecycleBinEnumList *prbel);
} IRecycleBin5Vtbl;

interface IRecycleBin5
{
	CONST_VTBL struct IRecycleBin5Vtbl *lpVtbl;
};

#ifdef COBJMACROS
#define IRecycleBin5_QueryInterface(This, riid, ppvObject) \
	(This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IRecycleBin5_AddRef(This) \
	(This)->lpVtbl->AddRef(This)
#define IRecycleBin5_Release(This) \
	(This)->lpVtbl->Release(This)
#define IRecycleBin5_DeleteFile(This, szFileName) \
	(This)->lpVtbl->DeleteFile(This, szFileName)
#define IRecycleBin5_EmptyRecycleBin(This) \
	(This)->lpVtbl->EmptyRecycleBin(This)
#define IRecycleBin5_EnumObjects(This, ppEnumList) \
	(This)->lpVtbl->EnumObjects(This, ppEnumList)
#define IRecycleBin5_Delete(This, pDeletedFileName, pDeletedFile) \
	(This)->lpVtbl->Delete(This, pDeletedFileName, pDeletedFile)
#define IRecycleBin5_Restore(This, pDeletedFileName, pDeletedFile) \
	(This)->lpVtbl->Restore(This, pDeletedFileName, pDeletedFile)
#define IRecycleBin5_OnClosing(This, prb5el) \
	(This)->lpVtbl->OnClosing(This, prb5el)
#endif

HRESULT
RecycleBin5_Enumerator_Constructor(
	IN IRecycleBin5 *prb,
	IN HANDLE hInfo,
	IN HANDLE hInfoMapped,
	IN LPCWSTR szPrefix,
	OUT IUnknown **ppUnknown);

#ifdef __cplusplus
}
#endif
