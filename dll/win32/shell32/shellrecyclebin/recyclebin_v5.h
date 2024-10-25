/* Recycle bin management
 * This file is under the GPLv2 licence
 * Copyright (C) 2006 Hervé Poussineau <hpoussin@reactos.org>
 */

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

#define INTERFACE IRecycleBin5
DECLARE_INTERFACE_(IRecycleBin5, IRecycleBin)
{
    BEGIN_INTERFACE

    /* IUnknown interface */
    STDMETHOD(QueryInterface)(THIS_ IN REFIID riid, OUT void **ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IRecycleBin interface */
    STDMETHOD(DeleteFile)(THIS_ IN LPCWSTR szFileName) PURE;
    STDMETHOD(EmptyRecycleBin)(THIS);
    STDMETHOD(EnumObjects)(THIS_ OUT IRecycleBinEnumList **ppEnumList) PURE;
    STDMETHOD(GetDirectory)(THIS_ LPWSTR szPath) PURE;

    /* IRecycleBin5 interface */
    STDMETHOD(Delete)(
        THIS_
        IN LPCWSTR pDeletedFileName,
        IN DELETED_FILE_RECORD *pDeletedFile) PURE;
    STDMETHOD(Restore)(
        THIS_
        IN LPCWSTR pDeletedFileName,
        IN DELETED_FILE_RECORD *pDeletedFile) PURE;
    STDMETHOD(OnClosing)(
        THIS_
        IN IRecycleBinEnumList *prbel) PURE;

    END_INTERFACE
};
#undef INTERFACE

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

EXTERN_C
HRESULT
RecycleBin5Enum_Constructor(
    _In_ IRecycleBin5 *prb,
    _In_ HANDLE hInfo,
    _In_ HANDLE hInfoMapped,
    _In_ LPCWSTR szPrefix,
    _Out_ IUnknown **ppUnknown);

#ifdef __cplusplus
}
#endif
