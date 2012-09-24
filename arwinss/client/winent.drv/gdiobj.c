/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/gdidrv.c
 * PURPOSE:         GDI driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosgdidrv);

/* GLOBALS ****************************************************************/
static struct list handle_mapping_list = LIST_INIT( handle_mapping_list );
static CRITICAL_SECTION handle_mapping_cs;
static BOOL StockObjectsInitialized = FALSE;

typedef struct _HMAPPING
{
    HGDIOBJ hUser;
    HGDIOBJ hKernel;
    struct list entry;
} HMAPPING, *PHMAPPING;

PGDI_TABLE_ENTRY GdiHandleTable = NULL;
PGDI_SHARED_HANDLE_TABLE GdiSharedHandleTable = NULL;
HANDLE CurrentProcessId = NULL;
HMAPPING GdiStockMapping[STOCK_LAST + 2];

/* FUNCTIONS **************************************************************/

VOID InitHandleMapping()
{
    InitializeCriticalSection(&handle_mapping_cs);
}

VOID InitStockObjectsMapping()
{
    HGDIOBJ hKernel, hStockUser;

    hKernel = NtGdiGetStockObject(DEFAULT_BITMAP);
    hStockUser = GetStockObject( STOCK_LAST+1 );

    TRACE("Adding hUser %x hKernel %x\n", hStockUser, hKernel);

    /* Make sure that both kernel mode and user mode objects are initialized */
    if(hKernel && hStockUser)
    {
        GdiStockMapping[STOCK_LAST+1].hKernel = hKernel;
        GdiStockMapping[STOCK_LAST+1].hUser = hStockUser;
        StockObjectsInitialized = TRUE;
    }
}

VOID AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser)
{
    PHMAPPING mapping = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HMAPPING));
    if(!mapping)
        return;

    mapping->hKernel = hKernel;
    mapping->hUser = hUser;

    EnterCriticalSection(&handle_mapping_cs);
    list_add_tail(&handle_mapping_list, &mapping->entry);
    LeaveCriticalSection(&handle_mapping_cs);
}

static PHMAPPING FindHandleMapping(HGDIOBJ hUser)
{
    PHMAPPING item;
    DWORD i;

    /* Check the static stock objects mapping first */
    for (i=0; i<sizeof(GdiStockMapping)/sizeof(HMAPPING); i++)
        if (GdiStockMapping[i].hUser == hUser) return &GdiStockMapping[i];

    LIST_FOR_EACH_ENTRY( item, &handle_mapping_list, HMAPPING, entry )
    {
        if (item->hUser == hUser)
        {
            return item;
        }
    }

    return NULL;
}

HGDIOBJ MapUserHandle(HGDIOBJ hUser)
{
    PHMAPPING mapping;

    if (!hUser) return NULL;

    /* Map stock objects if not mapped yet */
    if(!StockObjectsInitialized)
        InitStockObjectsMapping();

    EnterCriticalSection(&handle_mapping_cs);

    mapping = FindHandleMapping(hUser);

    if (!mapping)
        ERR("Couldn't find a mapping for handle %x\n", hUser);

    LeaveCriticalSection(&handle_mapping_cs);

    return mapping ? mapping->hKernel : NULL;
}

VOID RemoveHandleMapping(HGDIOBJ hUser)
{
    PHMAPPING mapping;

    if (!hUser) return;

    TRACE("Remove handle mapping %x\n", hUser);

    EnterCriticalSection(&handle_mapping_cs);

    mapping = FindHandleMapping(hUser);
    if(mapping == NULL)
    {
        LeaveCriticalSection(&handle_mapping_cs);
        return;
    }

    list_remove(&mapping->entry);
    LeaveCriticalSection(&handle_mapping_cs);
}

VOID CleanupHandleMapping()
{
    PHMAPPING mapping;

    while(!list_empty(&handle_mapping_list))
    {
        mapping = LIST_ENTRY(list_head(&handle_mapping_list), HMAPPING, entry);
        RemoveHandleMapping(mapping->hUser);
    }
}

BOOL GdiInitHandleTable()
{
    if(GdiHandleTable)
        return TRUE;

    GdiHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
    GdiSharedHandleTable = NtCurrentTeb()->ProcessEnvironmentBlock->GdiSharedHandleTable;
    CurrentProcessId = NtCurrentTeb()->ClientId.UniqueProcess;

    if(!GdiHandleTable)
        return FALSE;

    return TRUE;
}

BOOL GdiGetHandleUserData(HGDIOBJ hGdiObj, DWORD ObjectType, PVOID *UserData)
{
    PGDI_TABLE_ENTRY Entry ;

    if(!GdiInitHandleTable())
    {
        ERR("GdiInitHandleTable failed!\n");
        return FALSE;
    }

    Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);

    if((Entry->Type & GDI_ENTRY_BASETYPE_MASK) == ObjectType &&
            ( (Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK ) ==
            GDI_HANDLE_GET_TYPE(hGdiObj))
    {
        HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
        if(pid == NULL || pid == CurrentProcessId)
        {
            //
            // Need to test if we have Read & Write access to the VM address space.
            //
            BOOL Result = TRUE;
            if(Entry->UserData)
            {
                volatile CHAR *Current = (volatile CHAR*)Entry->UserData;
                _SEH2_TRY
                {
                    *Current = *Current;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Result = FALSE;
                }
                _SEH2_END
            }
            else
            {
                Result = FALSE; // Can not be zero.
                ERR("Object doesn't have user data handle 0x%x!\n", hGdiObj);
            }
            if (Result) *UserData = Entry->UserData;
            return Result;
        }
        else
        {
            ERR("Wrong user data pid handle 0x%x!\n", hGdiObj);
        }
    }
    else
    {
        ERR("Wrong object type handle 0x%x!\n", hGdiObj);
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

PDC_ATTR
GdiGetDcAttr(HDC hdc)
{
    PDC_ATTR pdcattr;

    if (!GdiGetHandleUserData((HGDIOBJ)hdc, GDI_OBJECT_TYPE_DC, (PVOID*)&pdcattr)) return NULL;
    return pdcattr;
}