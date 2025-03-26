#ifndef _RECYCLEBIN_PRIVATE_H_
#define _RECYCLEBIN_PRIVATE_H_

#include <stdio.h>

#define COBJMACROS

#include <shlobj.h>

#include "recyclebin.h"
#include "recyclebin_v5.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(recyclebin);

#ifdef __cplusplus
static inline HRESULT HResultFromWin32(DWORD hr)
{
     // HRESULT_FROM_WIN32 will evaluate its parameter twice, this function will not.
    return HRESULT_FROM_WIN32(hr);
}
#endif

/* Defines */

#define RECYCLE_BIN_DIRECTORY_WITH_ACL    L"RECYCLER"
#define RECYCLE_BIN_DIRECTORY_WITHOUT_ACL L"RECYCLED"
#define RECYCLE_BIN_FILE_NAME             L"INFO2"
#define RECYCLE_BIN_FILE_NAME_V1          L"INFO"

#define ROUND_UP(N, S) ((( (N) + (S)  - 1) / (S) ) * (S) )

/* Structures on disk */

#include <pshpack1.h>

typedef struct _INFO2_HEADER
{
    DWORD dwVersion;
    DWORD dwNumberOfEntries; /* unused */
    DWORD dwHighestRecordUniqueId; /* unused */
    DWORD dwRecordSize;
    DWORD dwTotalLogicalSize;
} INFO2_HEADER, *PINFO2_HEADER;

#include <poppack.h>

/* Prototypes */

/* recyclebin_generic.c */

EXTERN_C
HRESULT RecycleBinGeneric_Constructor(OUT IUnknown **ppUnknown);

EXTERN_C
BOOL RecycleBinGeneric_IsEqualFileIdentity(const RECYCLEBINFILEIDENTITY *p1, const RECYCLEBINFILEIDENTITY *p2);

/* recyclebin_generic_enumerator.c */

EXTERN_C
HRESULT RecycleBinGenericEnum_Constructor(OUT IRecycleBinEnumList **pprbel);

/* recyclebin_v5.c */

EXTERN_C
HRESULT RecycleBin5_Constructor(_In_ LPCWSTR VolumePath, _Out_ IUnknown **ppUnknown);

#endif /* _RECYCLEBIN_PRIVATE_H_ */
