#include "recyclebin.h"
#include "sddl.h"

/* Defines */
#define RECYCLEBIN_MAGIC  0x6e694252
#define DELETEDFILE_MAGIC 0x6e694253

#define RECYCLE_BIN_DIRECTORY_WITH_ACL    L"RECYCLER"
#define RECYCLE_BIN_DIRECTORY_WITHOUT_ACL L"RECYCLED"
#define RECYCLE_BIN_FILE_NAME             L"INFO2"

#define ROUND_UP(N, S) ((( (N) + (S)  - 1) / (S) ) * (S) )

/* List manipulation */
#define InitializeListHead(le)  (void)((le)->Flink = (le)->Blink = (le))
#define InsertTailList(le,e)    do { PLIST_ENTRY b = (le)->Blink; (e)->Flink = (le); (e)->Blink = b; b->Flink = (e); (le)->Blink = (e); } while (0)
#define RemoveEntryList(Entry)  { PLIST_ENTRY _EX_Blink, _EX_Flink; _EX_Flink = (Entry)->Flink; _EX_Blink = (Entry)->Blink; _EX_Blink->Flink = _EX_Flink; _EX_Flink->Blink = _EX_Blink; }

/* Typedefs */
struct _RECYCLE_BIN;
typedef struct _RECYCLE_BIN *PRECYCLE_BIN;
struct _REFCOUNT_DATA;
typedef struct _REFCOUNT_DATA *PREFCOUNT_DATA;

typedef BOOL (*PINT_ENUMERATE_RECYCLEBIN_CALLBACK)(IN PVOID Context OPTIONAL, IN HANDLE hDeletedFile);
typedef BOOL (*PDESTROY_DATA)    (IN PREFCOUNT_DATA pData);

typedef BOOL (*PCLOSE_HANDLE)    (IN HANDLE hHandle);
typedef BOOL (*PDELETE_FILE)     (IN PRECYCLE_BIN bin, IN LPCWSTR FullPath, IN LPCWSTR FileName);
typedef BOOL (*PEMPTY_RECYCLEBIN)(IN PRECYCLE_BIN* bin);
typedef BOOL (*PENUMERATE_FILES) (IN PRECYCLE_BIN bin, IN PINT_ENUMERATE_RECYCLEBIN_CALLBACK pFnCallback, IN PVOID Context OPTIONAL);
typedef BOOL (*PGET_DETAILS)     (IN PRECYCLE_BIN bin, IN HANDLE hDeletedFile, IN DWORD BufferSize, IN OUT PDELETED_FILE_DETAILS_W FileDetails OPTIONAL, OUT LPDWORD RequiredSize OPTIONAL);
typedef BOOL (*PRESTORE_FILE)    (IN PRECYCLE_BIN bin, IN HANDLE hDeletedFile);

typedef struct _RECYCLEBIN_CALLBACKS
{
	PCLOSE_HANDLE     CloseHandle;
	PDELETE_FILE      DeleteFile;
	PEMPTY_RECYCLEBIN EmptyRecycleBin;
	PENUMERATE_FILES  EnumerateFiles;
	PGET_DETAILS      GetDetails;
	PRESTORE_FILE     RestoreFile;
} RECYCLEBIN_CALLBACKS, *PRECYCLEBIN_CALLBACKS;

typedef struct _REFCOUNT_DATA
{
	DWORD ReferenceCount;
	PDESTROY_DATA Close;
} REFCOUNT_DATA;

typedef struct _RECYCLE_BIN
{
	DWORD magic; /* RECYCLEBIN_MAGIC */
	LIST_ENTRY ListEntry;
	REFCOUNT_DATA refCount;
	HANDLE hInfo;
	RECYCLEBIN_CALLBACKS Callbacks;
	LPWSTR InfoFile;
	WCHAR Folder[ANY_SIZE]; /* [drive]:\[RECYCLE_BIN_DIRECTORY]\{SID} */
} RECYCLE_BIN;

typedef struct _DELETED_FILE_HANDLE
{
	DWORD magic; /* DELETEDFILE_MAGIC */
	REFCOUNT_DATA refCount;
	PRECYCLE_BIN bin;
	HANDLE hDeletedFile; /* specific to recycle bin format */
} DELETED_FILE_HANDLE, *PDELETED_FILE_HANDLE;

/* Structures on disk */

#include <pshpack1.h>

typedef struct _INFO2_HEADER
{
	DWORD dwVersion;
	DWORD dwNumberOfEntries;
	DWORD dwHighestRecordUniqueId;
	DWORD dwRecordSize;
	DWORD dwTotalLogicalSize;
} INFO2_HEADER, *PINFO2_HEADER;

#include <poppack.h>

/* Prototypes */

/* openclose.c */

BOOL
IntCheckDeletedFileHandle(
	IN HANDLE hDeletedFile);

PRECYCLE_BIN
IntReferenceRecycleBin(
	IN WCHAR driveLetter);

/* recyclebin_v5.c */

VOID
InitializeCallbacks5(
	IN OUT PRECYCLEBIN_CALLBACKS Callbacks);

/* refcount.c */

BOOL
InitializeHandle(
	IN PREFCOUNT_DATA pData,
	IN PDESTROY_DATA pFnClose OPTIONAL);

BOOL
ReferenceHandle(
	IN PREFCOUNT_DATA pData);

BOOL
DereferenceHandle(
	IN PREFCOUNT_DATA pData);
