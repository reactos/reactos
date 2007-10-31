#include "recyclebin.h"
#include "sddl.h"
#include <wine/debug.h>

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

/* recyclebin_generic.c */

HRESULT RecycleBinGeneric_Constructor(OUT IUnknown **ppUnknown);

/* recyclebin_generic_enumerator.c */

HRESULT RecycleBinGeneric_Enumerator_Constructor(OUT IRecycleBinEnumList **pprbel);

/* recyclebin_v5.c */

HRESULT RecycleBin5_Constructor(IN LPCWSTR VolumePath, OUT IUnknown **ppUnknown);
