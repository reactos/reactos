/*
 * Unit tests for fiber functions
 *
 * Copyright (c) 2010 André Hentschel
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <winternl.h>
#include "wine/test.h"
#include <winuser.h>

static LPVOID (WINAPI *pCreateFiber)(SIZE_T,LPFIBER_START_ROUTINE,LPVOID);
static LPVOID (WINAPI *pConvertThreadToFiber)(LPVOID);
static BOOL (WINAPI *pConvertFiberToThread)(void);
static void (WINAPI *pSwitchToFiber)(LPVOID);
static void (WINAPI *pDeleteFiber)(LPVOID);
static LPVOID (WINAPI *pConvertThreadToFiberEx)(LPVOID,DWORD);
static LPVOID (WINAPI *pCreateFiberEx)(SIZE_T,SIZE_T,DWORD,LPFIBER_START_ROUTINE,LPVOID);
static BOOL (WINAPI *pIsThreadAFiber)(void);
static DWORD (WINAPI *pFlsAlloc)(PFLS_CALLBACK_FUNCTION);
static BOOL (WINAPI *pFlsFree)(DWORD);
static PVOID (WINAPI *pFlsGetValue)(DWORD);
static BOOL (WINAPI *pFlsSetValue)(DWORD,PVOID);
static void (WINAPI *pRtlAcquirePebLock)(void);
static void (WINAPI *pRtlReleasePebLock)(void);
static NTSTATUS (WINAPI *pRtlFlsAlloc)(PFLS_CALLBACK_FUNCTION,DWORD*);
static NTSTATUS (WINAPI *pRtlFlsFree)(ULONG);
static NTSTATUS (WINAPI *pRtlFlsSetValue)(ULONG,void *);
static NTSTATUS (WINAPI *pRtlFlsGetValue)(ULONG,void **);
static void (WINAPI *pRtlProcessFlsData)(void *fls_data, ULONG flags);
static void *fibers[3];
static BYTE testparam = 185;
static DWORD fls_index_to_set = FLS_OUT_OF_INDEXES;
static void* fls_value_to_set;

static int fiberCount = 0;
static int cbCount = 0;

static VOID init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f);
    X(CreateFiber);
    X(ConvertThreadToFiber);
    X(ConvertFiberToThread);
    X(SwitchToFiber);
    X(DeleteFiber);
    X(ConvertThreadToFiberEx);
    X(CreateFiberEx);
    X(IsThreadAFiber);
    X(FlsAlloc);
    X(FlsFree);
    X(FlsGetValue);
    X(FlsSetValue);
#undef X

#define X(f) p##f = (void*)GetProcAddress(hntdll, #f);
    X(RtlFlsAlloc);
    X(RtlFlsFree);
    X(RtlFlsSetValue);
    X(RtlFlsGetValue);
    X(RtlProcessFlsData);
    X(RtlAcquirePebLock);
    X(RtlReleasePebLock);
#undef X

}

static VOID WINAPI FiberLocalStorageProc(PVOID lpFlsData)
{
    ok(lpFlsData == fls_value_to_set,
       "FlsData expected not to be changed, value is %p, expected %p\n",
       lpFlsData, fls_value_to_set);
    cbCount++;
}

static VOID WINAPI FiberMainProc(LPVOID lpFiberParameter)
{
    BYTE *tparam = (BYTE *)lpFiberParameter;
    TEB *teb = NtCurrentTeb();

    ok(!teb->FlsSlots, "Got unexpected FlsSlots %p.\n", teb->FlsSlots);

    fiberCount++;
    ok(*tparam == 185, "Parameterdata expected not to be changed\n");
    if (fls_index_to_set != FLS_OUT_OF_INDEXES)
    {
        void* ret;
        BOOL bret;

        SetLastError( 0xdeadbeef );
        ret = pFlsGetValue(fls_index_to_set);
        ok(ret == NULL, "FlsGetValue returned %p, expected NULL\n", ret);
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %lu.\n", GetLastError());

        /* Set the FLS value */
        bret = pFlsSetValue(fls_index_to_set, fls_value_to_set);
        ok(bret, "FlsSetValue failed with error %lu\n", GetLastError());

        ok(!!teb->FlsSlots, "Got unexpected FlsSlots %p.\n", teb->FlsSlots);

        /* Verify that FlsGetValue retrieves the value set by FlsSetValue */
        SetLastError( 0xdeadbeef );
        ret = pFlsGetValue(fls_index_to_set);
        ok(ret == fls_value_to_set, "FlsGetValue returned %p, expected %p\n", ret, fls_value_to_set);
        ok(GetLastError() == ERROR_SUCCESS, "FlsGetValue error %lu\n", GetLastError());
    }
    pSwitchToFiber(fibers[0]);
}

static void test_ConvertThreadToFiber(void)
{
    void *ret;

    if (pConvertThreadToFiber)
    {
        fibers[0] = pConvertThreadToFiber(&testparam);
        ok(fibers[0] != NULL, "ConvertThreadToFiber failed with error %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = pConvertThreadToFiber(&testparam);
        ok(!ret, "Got non NULL ret.\n");
        ok(GetLastError() == ERROR_ALREADY_FIBER, "Got unexpected error %lu.\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiber not present\n" );
    }
}

static void test_ConvertThreadToFiberEx(void)
{
    void *ret;

    if (pConvertThreadToFiberEx)
    {
        fibers[0] = pConvertThreadToFiberEx(&testparam, 0);
        ok(fibers[0] != NULL, "ConvertThreadToFiberEx failed with error %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        ret = pConvertThreadToFiberEx(&testparam, 0);
        ok(!ret, "Got non NULL ret.\n");
        ok(GetLastError() == ERROR_ALREADY_FIBER, "Got unexpected error %lu.\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertThreadToFiberEx not present\n" );
    }
}

static void test_ConvertFiberToThread(void)
{
    if (pConvertFiberToThread)
    {
        BOOL ret = pConvertFiberToThread();
        ok(ret, "ConvertFiberToThread failed with error %lu\n", GetLastError());
    }
    else
    {
        win_skip( "ConvertFiberToThread not present\n" );
    }
}

static void test_FiberHandling(void)
{
    fiberCount = 0;
    fibers[0] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[0] != NULL, "CreateFiber failed with error %lu\n", GetLastError());
    pDeleteFiber(fibers[0]);

    test_ConvertThreadToFiber();
    test_ConvertFiberToThread();
    if (pConvertThreadToFiberEx)
        test_ConvertThreadToFiberEx();
    else
        test_ConvertThreadToFiber();

    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != NULL, "CreateFiber failed with error %lu\n", GetLastError());

    pSwitchToFiber(fibers[1]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    pDeleteFiber(fibers[1]);

    if (pCreateFiberEx)
    {
        fibers[1] = pCreateFiberEx(0,0,0,FiberMainProc,&testparam);
        ok(fibers[1] != NULL, "CreateFiberEx failed with error %lu\n", GetLastError());

        pSwitchToFiber(fibers[1]);
        ok(fiberCount == 2, "Wrong fiber count: %d\n", fiberCount);
        pDeleteFiber(fibers[1]);
    }
    else win_skip( "CreateFiberEx not present\n" );

    if (pIsThreadAFiber) ok(pIsThreadAFiber(), "IsThreadAFiber reported FALSE\n");
    test_ConvertFiberToThread();
    if (pIsThreadAFiber) ok(!pIsThreadAFiber(), "IsThreadAFiber reported TRUE\n");
}

#define FLS_TEST_INDEX_COUNT 4096

static unsigned int check_linked_list(const LIST_ENTRY *le, const LIST_ENTRY *search_entry, unsigned int *index_found)
{
    unsigned int count = 0;
    LIST_ENTRY *entry;

    *index_found = ~0;

    for (entry = le->Flink; entry != le; entry = entry->Flink)
    {
        if (entry == search_entry)
        {
            ok(*index_found == ~0, "Duplicate list entry.\n");
            *index_found = count;
        }
        ++count;
    }
    return count;
}

static unsigned int test_fls_callback_call_count;

static void WINAPI test_fls_callback(void *data)
{
    ++test_fls_callback_call_count;
}

static unsigned int test_fls_chunk_size(unsigned int chunk_index)
{
    return 0x10 << chunk_index;
}

static unsigned int test_fls_chunk_index_from_index(unsigned int index, unsigned int *index_in_chunk)
{
    unsigned int chunk_index = 0;

    while (index >= test_fls_chunk_size(chunk_index))
        index -= test_fls_chunk_size(chunk_index++);

    *index_in_chunk = index;
    return chunk_index;
}

static HANDLE test_fiberlocalstorage_peb_locked_event;
static HANDLE test_fiberlocalstorage_done_event;


static DWORD WINAPI test_FiberLocalStorage_thread(void *arg)
{
    pRtlAcquirePebLock();
    SetEvent(test_fiberlocalstorage_peb_locked_event);
    WaitForSingleObject(test_fiberlocalstorage_done_event, INFINITE);
    pRtlReleasePebLock();
    return 0;
}

static void test_FiberLocalStorage(void)
{
    static DWORD fls_indices[FLS_TEST_INDEX_COUNT];
    unsigned int i, j, count, entry_count, index;
    LIST_ENTRY *fls_list_head, saved_entry;
    TEB_FLS_DATA *fls_data, *new_fls_data;
    GLOBAL_FLS_DATA *g_fls_data;
    DWORD fls, fls_2, result;
    TEB *teb = NtCurrentTeb();
    PEB *peb = teb->Peb;
    NTSTATUS status;
    HANDLE hthread;
    ULONG index2;
    SIZE_T size;
    void* val;
    BOOL ret;

    if (!pFlsAlloc || !pFlsSetValue || !pFlsGetValue || !pFlsFree)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    if (pRtlFlsAlloc)
    {
        if (pRtlFlsGetValue)
        {
            status = pRtlFlsGetValue(0, NULL);
            ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);
        }
        else
        {
            win_skip("RtlFlsGetValue is not available.\n");
        }

        for (i = 0; i < FLS_TEST_INDEX_COUNT; ++i)
        {
            fls_indices[i] = 0xdeadbeef;
            status = pRtlFlsAlloc(test_fls_callback, &fls_indices[i]);
            ok(!status || status == STATUS_NO_MEMORY, "Got unexpected status %#lx.\n", status);
            if (status)
            {
                ok(fls_indices[i] == 0xdeadbeef, "Got unexpected index %#lx.\n", fls_indices[i]);
                break;
            }
            if (pRtlFlsSetValue)
            {
                status = pRtlFlsSetValue(fls_indices[i], (void *)(ULONG_PTR)(i + 1));
                ok(!status, "Got unexpected status %#lx.\n", status);
            }
        }
        count = i;

        fls_data = teb->FlsSlots;

        /* FLS limits are increased since Win10 18312. */
        ok(count && (count <= 127 || (count > 4000 && count < 4096)), "Got unexpected count %u.\n", count);

        if (!peb->FlsCallback)
        {
            ok(pRtlFlsSetValue && pRtlFlsGetValue, "Missing RtlFlsGetValue / RtlFlsSetValue.\n");
            ok(!peb->FlsBitmap, "Got unexpected FlsBitmap %p.\n", peb->FlsBitmap);
            ok(!peb->FlsListHead.Flink && !peb->FlsListHead.Blink, "Got nonzero FlsListHead.\n");
            ok(!peb->FlsHighIndex, "Got unexpected FlsHighIndex %lu.\n", peb->FlsHighIndex);

            fls_list_head = fls_data->fls_list_entry.Flink;

            entry_count = check_linked_list(fls_list_head, &fls_data->fls_list_entry, &index);
            ok(entry_count == 1, "Got unexpected count %u.\n", entry_count);
            ok(!index, "Got unexpected index %u.\n", index);

            g_fls_data = CONTAINING_RECORD(fls_list_head, GLOBAL_FLS_DATA, fls_list_head);

            ok(g_fls_data->fls_high_index == 0xfef, "Got unexpected fls_high_index %#lx.\n", g_fls_data->fls_high_index);

            for (i = 0; i < 8; ++i)
            {
                ok(!!g_fls_data->fls_callback_chunks[i], "Got zero fls_callback_chunks[%u].\n", i);
                ok(g_fls_data->fls_callback_chunks[i]->count == test_fls_chunk_size(i),
                        "Got unexpected g_fls_data->fls_callback_chunks[%u]->count %lu.\n",
                        i, g_fls_data->fls_callback_chunks[i]->count);

                size = HeapSize(GetProcessHeap(), 0, g_fls_data->fls_callback_chunks[i]);
                ok(size == sizeof(ULONG_PTR) + sizeof(FLS_CALLBACK) * test_fls_chunk_size(i),
                        "Got unexpected size %p.\n", (void *)size);

                ok(!!fls_data->fls_data_chunks[i], "Got zero fls_data->fls_data_chunks[%u].\n", i);
                ok(!fls_data->fls_data_chunks[i][0], "Got unexpected fls_data->fls_data_chunks[%u][0] %p.\n",
                        i, fls_data->fls_data_chunks[i][0]);
                size = HeapSize(GetProcessHeap(), 0, fls_data->fls_data_chunks[i]);
                ok(size == sizeof(void *) * (test_fls_chunk_size(i) + 1), "Got unexpected size %p.\n", (void *)size);

                if (!i)
                {
                    ok(g_fls_data->fls_callback_chunks[0]->callbacks[0].callback == (void *)~(ULONG_PTR)0,
                            "Got unexpected callback %p.\n",
                            g_fls_data->fls_callback_chunks[0]->callbacks[0].callback);
                }

                for (j = i ? 0 : fls_indices[0]; j < test_fls_chunk_size(i); ++j)
                {
                    ok(!g_fls_data->fls_callback_chunks[i]->callbacks[j].unknown,
                            "Got unexpected unknown %p, i %u, j %u.\n",
                            g_fls_data->fls_callback_chunks[i]->callbacks[j].unknown, i, j);
                    ok(g_fls_data->fls_callback_chunks[i]->callbacks[j].callback == test_fls_callback,
                            "Got unexpected callback %p, i %u, j %u.\n",
                            g_fls_data->fls_callback_chunks[i]->callbacks[j].callback, i, j);
                }
            }
            for (i = 0; i < count; ++i)
            {
                j = test_fls_chunk_index_from_index(fls_indices[i], &index);
                ok(fls_data->fls_data_chunks[j][index + 1] == (void *)(ULONG_PTR)(i + 1),
                        "Got unexpected FLS value %p, i %u, j %u, index %u.\n",
                        fls_data->fls_data_chunks[j][index + 1], i, j, index);
            }
            j = test_fls_chunk_index_from_index(fls_indices[0x10], &index);
            g_fls_data->fls_callback_chunks[j]->callbacks[index].callback = NULL;
            status = pRtlFlsFree(fls_indices[0x10]);
            ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);

            g_fls_data->fls_callback_chunks[j]->callbacks[index].callback = test_fls_callback;
            test_fls_callback_call_count = 0;
            status = pRtlFlsFree(fls_indices[0x10]);
            ok(!status, "Got unexpected status %#lx.\n", status);
            ok(test_fls_callback_call_count == 1, "Got unexpected callback call count %u.\n",
                    test_fls_callback_call_count);

            ok(!fls_data->fls_data_chunks[j][0], "Got unexpected fls_data->fls_data_chunks[%u][0] %p.\n",
                    j, fls_data->fls_data_chunks[j][0]);
            ok(!g_fls_data->fls_callback_chunks[j]->callbacks[index].callback,
                    "Got unexpected callback %p.\n",
                    g_fls_data->fls_callback_chunks[j]->callbacks[index].callback);

            fls_data->fls_data_chunks[j][index + 1] = (void *)(ULONG_PTR)0x28;
            status = pRtlFlsAlloc(test_fls_callback, &index2);
            ok(!status, "Got unexpected status %#lx.\n", status);
            ok(index2 == fls_indices[0x10], "Got unexpected index %lu.\n", index2);
            ok(fls_data->fls_data_chunks[j][index + 1] == (void *)(ULONG_PTR)0x28, "Got unexpected data %p.\n",
                    fls_data->fls_data_chunks[j][index + 1]);

            status = pRtlFlsSetValue(index2, (void *)(ULONG_PTR)0x11);
            ok(!status, "Got unexpected status %#lx.\n", status);

            teb->FlsSlots = NULL;

            val = (void *)0xdeadbeef;
            status = pRtlFlsGetValue(fls_indices[1], &val);
            new_fls_data = teb->FlsSlots;
            ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);
            ok(val == (void *)0xdeadbeef, "Got unexpected val %p.\n", val);
            ok(!new_fls_data, "Got unexpected teb->FlsSlots %p.\n", new_fls_data);

            status = pRtlFlsSetValue(fls_indices[1], (void *)(ULONG_PTR)0x28);
            new_fls_data = teb->FlsSlots;
            ok(!status, "Got unexpected status %#lx.\n", status);
            ok(!!new_fls_data, "Got unexpected teb->FlsSlots %p.\n", new_fls_data);

            entry_count = check_linked_list(fls_list_head, &fls_data->fls_list_entry, &index);
            ok(entry_count == 2, "Got unexpected count %u.\n", entry_count);
            ok(!index, "Got unexpected index %u.\n", index);
            check_linked_list(fls_list_head, &new_fls_data->fls_list_entry, &index);
            ok(index == 1, "Got unexpected index %u.\n", index);

            val = (void *)0xdeadbeef;
            status = pRtlFlsGetValue(fls_indices[2], &val);
            ok(!status, "Got unexpected status %#lx.\n", status);
            ok(!val, "Got unexpected val %p.\n", val);


            /* With bit 0 of flags set RtlProcessFlsData is removing FLS data from the linked list
             * and calls FLS callbacks. With bit 1 set the memory is freed. The remaining bits do not seem
             * to have any obvious effect. */
            for (i = 2; i < 32; ++i)
            {
                pRtlProcessFlsData(new_fls_data, 1 << i);
                size = HeapSize(GetProcessHeap(), 0, new_fls_data);
                ok(size == sizeof(*new_fls_data), "Got unexpected size %p.\n", (void *)size);
            }

            if (0)
            {
                pRtlProcessFlsData(new_fls_data, 2);
                entry_count = check_linked_list(fls_list_head, &fls_data->fls_list_entry, &index);
                ok(entry_count == 2, "Got unexpected count %u.\n", entry_count);

                /* Crashes on Windows. */
                HeapSize(GetProcessHeap(), 0, new_fls_data);
            }

            test_fiberlocalstorage_peb_locked_event = CreateEventA(NULL, FALSE, FALSE, NULL);
            test_fiberlocalstorage_done_event = CreateEventA(NULL, FALSE, FALSE, NULL);
            hthread = CreateThread(NULL, 0, test_FiberLocalStorage_thread, NULL, 0, NULL);
            ok(!!hthread, "CreateThread failed.\n");
            result = WaitForSingleObject(test_fiberlocalstorage_peb_locked_event, INFINITE);
            ok(result == WAIT_OBJECT_0, "Got unexpected result %lu.\n", result);
            teb->FlsSlots = NULL;

            test_fls_callback_call_count = 0;
            saved_entry = new_fls_data->fls_list_entry;
            pRtlProcessFlsData(new_fls_data, 1);
            ok(!teb->FlsSlots, "Got unexpected teb->FlsSlots %p.\n", teb->FlsSlots);

            teb->FlsSlots = fls_data;
            ok(test_fls_callback_call_count == 1, "Got unexpected callback call count %u.\n",
                    test_fls_callback_call_count);

            SetEvent(test_fiberlocalstorage_done_event);
            WaitForSingleObject(hthread, INFINITE);
            CloseHandle(hthread);
            CloseHandle(test_fiberlocalstorage_peb_locked_event);
            CloseHandle(test_fiberlocalstorage_done_event);

            ok(new_fls_data->fls_list_entry.Flink == saved_entry.Flink, "Got unexpected Flink %p.\n",
                    saved_entry.Flink);
            ok(new_fls_data->fls_list_entry.Blink == saved_entry.Blink, "Got unexpected Flink %p.\n",
                    saved_entry.Blink);
            size = HeapSize(GetProcessHeap(), 0, new_fls_data);
            ok(size == sizeof(*new_fls_data), "Got unexpected size %p.\n", (void *)size);
            test_fls_callback_call_count = 0;
            i = test_fls_chunk_index_from_index(fls_indices[1], &index);
            new_fls_data->fls_data_chunks[i][index + 1] = (void *)(ULONG_PTR)0x28;
            pRtlProcessFlsData(new_fls_data, 2);
            ok(!test_fls_callback_call_count, "Got unexpected callback call count %u.\n",
                    test_fls_callback_call_count);

            if (0)
            {
                /* crashes on Windows. */
                HeapSize(GetProcessHeap(), 0, new_fls_data);
            }

            entry_count = check_linked_list(fls_list_head, &fls_data->fls_list_entry, &index);
            ok(entry_count == 1, "Got unexpected count %u.\n", entry_count);
            ok(!index, "Got unexpected index %u.\n", index);
        }
        else
        {
            win_skip("Old FLS data storage layout, skipping test.\n");
            g_fls_data = NULL;
        }

        if (0)
        {
            /* crashes on Windows. */
            pRtlFlsGetValue(fls_indices[0], NULL);
        }

        for (i = 0; i < count; ++i)
        {
            if (pRtlFlsGetValue)
            {
                status = pRtlFlsGetValue(fls_indices[i], &val);
                ok(!status, "Got unexpected status %#lx.\n", status);
                ok(val == (void *)(ULONG_PTR)(i + 1), "Got unexpected val %p, i %u.\n", val, i);
            }

            status = pRtlFlsFree(fls_indices[i]);
            ok(!status, "Got unexpected status %#lx, i %u.\n", status, i);
        }

        if (!peb->FlsCallback)
        {
            ok(g_fls_data->fls_high_index == 0xfef, "Got unexpected fls_high_index %#lx.\n",
                    g_fls_data->fls_high_index);

            for (i = 0; i < 8; ++i)
            {
                ok(!!g_fls_data->fls_callback_chunks[i], "Got zero fls_callback_chunks[%u].\n", i);
                ok(!!fls_data->fls_data_chunks[i], "Got zero fls_data->fls_data_chunks[%u].\n", i);
            }
        }
    }
    else
    {
        win_skip("RtlFlsAlloc is not available.\n");
    }

    /* Test an unallocated index
     * FlsFree should fail
     * FlsGetValue and FlsSetValue should succeed
     */
    SetLastError( 0xdeadbeef );
    ret = pFlsFree( 127 );
    ok( !ret, "freeing fls index 127 (unallocated) succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "freeing fls index 127 (unallocated) wrong error %lu\n", GetLastError() );

    val = pFlsGetValue( 127 );
    ok( val == NULL,
        "getting fls index 127 (unallocated) failed with error %lu\n", GetLastError() );

    if (pRtlFlsGetValue)
    {
        val = (void *)0xdeadbeef;
        status = pRtlFlsGetValue(127, &val);
        ok( !status, "Got unexpected status %#lx.\n", status );
        ok( !val, "Got unexpected val %p.\n", val );
    }

    ret = pFlsSetValue( 127, (void*) 0x217 );
    ok( ret, "setting fls index 127 (unallocated) failed with error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 127 );
    ok( val == (void*) 0x217, "fls index 127 (unallocated) wrong value %p\n", val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index 127 (unallocated) failed with error %lu\n", GetLastError() );

    if (pRtlFlsGetValue)
    {
        val = (void *)0xdeadbeef;
        status = pRtlFlsGetValue(127, &val);
        ok( !status, "Got unexpected status %#lx.\n", status );
        ok( val == (void*)0x217, "Got unexpected val %p.\n", val );
    }

    /* FlsFree, FlsGetValue, and FlsSetValue out of bounds should return
     * ERROR_INVALID_PARAMETER
     */
    SetLastError( 0xdeadbeef );
    ret = pFlsFree( 128 );
    ok( !ret, "freeing fls index 128 (out of bounds) succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "freeing fls index 128 (out of bounds) wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pFlsSetValue( 128, (void*) 0x217 );
    ok( ret || GetLastError() == ERROR_INVALID_PARAMETER,
        "setting fls index 128 (out of bounds) wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 128 );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || val == (void *)0x217,
        "getting fls index 128 (out of bounds) wrong error %lu\n", GetLastError() );

    /* Test index 0 */
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 0 );
    ok( !val, "fls index 0 set to %p\n", val );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %lu\n", GetLastError() );
    if (pRtlFlsGetValue)
    {
        val = (void *)0xdeadbeef;
        status = pRtlFlsGetValue(0, &val);
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );
        ok( val == (void*)0xdeadbeef, "Got unexpected val %p.\n", val );
    }

    SetLastError( 0xdeadbeef );
    ret = pFlsSetValue( 0, (void *)0xdeadbeef );
    ok( !ret, "setting fls index 0 succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %lu\n", GetLastError() );
    if (pRtlFlsSetValue)
    {
        status = pRtlFlsSetValue( 0, (void *)0xdeadbeef );
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );
    }
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( 0 );
    ok( !val, "fls index 0 wrong value %p\n", val );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "setting fls index wrong error %lu\n", GetLastError() );

    /* Test creating an FLS index */
    fls = pFlsAlloc( NULL );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed\n" );
    ok( fls != 0, "fls index 0 allocated\n" );
    val = pFlsGetValue( fls );
    ok( !val, "fls index %lu wrong value %p\n", fls, val );
    SetLastError( 0xdeadbeef );
    ret = pFlsSetValue( fls, (void *)0xdeadbeef );
    ok( ret, "setting fls index %lu failed\n", fls );
    ok( GetLastError() == 0xdeadbeef, "setting fls index wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbeef, "fls index %lu wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %lu failed with error %lu\n", fls, GetLastError() );
    pFlsFree( fls );

    /* Undefined behavior: verify the value is NULL after it the slot is freed */
    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls );
    ok( val == NULL, "fls index %lu wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %lu failed with error %lu\n", fls, GetLastError() );

    /* Undefined behavior: verify the value is settable after the slot is freed */
    ret = pFlsSetValue( fls, (void *)0xdeadbabe );
    ok( ret, "setting fls index %lu failed\n", fls );
    val = pFlsGetValue( fls );
    ok( val == (void *)0xdeadbabe, "fls index %lu wrong value %p\n", fls, val );

    /* Try to create the same FLS index again, and verify that is initialized to NULL */
    fls_2 = pFlsAlloc( NULL );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %lu\n", GetLastError() );
    /* If this fails it is not an API error, but the test will be inconclusive */
    ok( fls_2 == fls, "different FLS index allocated, was %lu, now %lu\n", fls, fls_2 );

    SetLastError( 0xdeadbeef );
    val = pFlsGetValue( fls_2 );
    ok( val == NULL || val == (void *)0xdeadbabe, "fls index %lu wrong value %p\n", fls, val );
    ok( GetLastError() == ERROR_SUCCESS,
        "getting fls index %lu failed with error %lu\n", fls_2, GetLastError() );
    pFlsFree( fls_2 );
}

static void test_FiberLocalStorageCallback(PFLS_CALLBACK_FUNCTION cbfunc)
{
    DWORD fls;
    BOOL ret;
    void* val, *val2;

    if (!pFlsAlloc || !pFlsSetValue || !pFlsGetValue || !pFlsFree)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    /* Test that the callback is executed */
    cbCount = 0;
    fls = pFlsAlloc( cbfunc );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %lu\n", GetLastError() );

    val = (void*) 0x1587;
    fls_value_to_set = val;
    ret = pFlsSetValue( fls, val );
    ok(ret, "FlsSetValue failed with error %lu\n", GetLastError() );

    val2 = pFlsGetValue( fls );
    ok(val == val2, "FlsGetValue returned %p, expected %p\n", val2, val);

    ret = pFlsFree( fls );
    ok(ret, "FlsFree failed with error %lu\n", GetLastError() );
    ok( cbCount == 1, "Wrong callback count: %d\n", cbCount );

    /* Test that callback is not executed if value is NULL */
    cbCount = 0;
    fls = pFlsAlloc( cbfunc );
    ok( fls != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %lu\n", GetLastError() );

    ret = pFlsSetValue( fls, NULL );
    ok( ret, "FlsSetValue failed with error %lu\n", GetLastError() );

    pFlsFree( fls );
    ok( ret, "FlsFree failed with error %lu\n", GetLastError() );
    ok( cbCount == 0, "Wrong callback count: %d\n", cbCount );
}

static void test_FiberLocalStorageWithFibers(PFLS_CALLBACK_FUNCTION cbfunc)
{
    void* val1 = (void*) 0x314;
    void* val2 = (void*) 0x152;
    BOOL ret;

    if (!pFlsAlloc || !pFlsFree || !pFlsSetValue || !pFlsGetValue)
    {
        win_skip( "Fiber Local Storage not supported\n" );
        return;
    }

    fls_index_to_set = pFlsAlloc(cbfunc);
    ok(fls_index_to_set != FLS_OUT_OF_INDEXES, "FlsAlloc failed with error %lu\n", GetLastError());

    test_ConvertThreadToFiber();

    fiberCount = 0;
    cbCount = 0;
    fibers[1] = pCreateFiber(0,FiberMainProc,&testparam);
    fibers[2] = pCreateFiber(0,FiberMainProc,&testparam);
    ok(fibers[1] != NULL, "CreateFiber failed with error %lu\n", GetLastError());
    ok(fibers[2] != NULL, "CreateFiber failed with error %lu\n", GetLastError());
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pSwitchToFiber(fibers[1]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val2;
    pSwitchToFiber(fibers[2]);
    ok(fiberCount == 1, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    fls_value_to_set = val2;
    ret = pFlsSetValue(fls_index_to_set, fls_value_to_set);
    ok(ret, "FlsSetValue failed\n");
    ok(val2 == pFlsGetValue(fls_index_to_set), "FlsGetValue failed\n");

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pDeleteFiber(fibers[1]);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 1, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val2;
    pFlsFree(fls_index_to_set);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 2, "Wrong callback count: %d\n", cbCount);

    fiberCount = 0;
    cbCount = 0;
    fls_value_to_set = val1;
    pDeleteFiber(fibers[2]);
    ok(fiberCount == 0, "Wrong fiber count: %d\n", fiberCount);
    ok(cbCount == 0, "Wrong callback count: %d\n", cbCount);

    test_ConvertFiberToThread();
}

#define check_current_actctx_is(e,t) check_current_actctx_is_(__LINE__, e, t)
static void check_current_actctx_is_(int line, HANDLE expected_actctx, BOOL todo)
{
    HANDLE cur_actctx;
    BOOL ret;

    cur_actctx = (void*)0xdeadbeef;
    ret = GetCurrentActCtx(&cur_actctx);
    ok_(__FILE__, line)(ret, "thread GetCurrentActCtx failed, %lu\n", GetLastError());

    todo_wine_if(todo)
    ok_(__FILE__, line)(cur_actctx == expected_actctx, "got %p, expected %p\n", cur_actctx, expected_actctx);

    ReleaseActCtx(cur_actctx);
}

static DWORD WINAPI subthread_actctx_func(void *actctx)
{
    HANDLE fiber;
    BOOL ret;

    check_current_actctx_is(actctx, FALSE);

    fiber = pConvertThreadToFiber(NULL);
    ok(fiber != NULL, "ConvertThreadToFiber returned error %lu\n", GetLastError());
    check_current_actctx_is(actctx, FALSE);
    fibers[2] = fiber;

    SwitchToFiber(fibers[0]);
    check_current_actctx_is(actctx, FALSE);

    ok(fibers[2] == fiber, "fibers[2]: expected %p, got %p\n", fiber, fibers[2]);
    fibers[2] = NULL;
    ret = pConvertFiberToThread();
    ok(ret, "ConvertFiberToThread returned error %lu\n", GetLastError());
    check_current_actctx_is(actctx, FALSE);

    return 0;
}

static void WINAPI fiber_actctx_func(void *actctx)
{
    ULONG_PTR cookie;
    DWORD tid, wait;
    HANDLE thread;
    BOOL ret;

    check_current_actctx_is(NULL, FALSE);

    ret = ActivateActCtx(actctx, &cookie);
    ok(ret, "ActivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(actctx, FALSE);

    SwitchToFiber(fibers[0]);
    check_current_actctx_is(actctx, FALSE);

    thread = CreateThread(NULL, 0, subthread_actctx_func, actctx, 0, &tid);
    ok(thread != NULL, "CreateThread returned error %lu\n", GetLastError());

    wait = WaitForSingleObject(thread, INFINITE);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %lu (last error: %lu)\n",
       wait, GetLastError());
    CloseHandle(thread);

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(NULL, FALSE);

    SwitchToFiber(fibers[0]);
    ok(0, "unreachable\n");
}

/* Test that activation context is switched on SwitchToFiber() call */
static void subtest_fiber_actctx_switch(HANDLE current_actctx, HANDLE child_actctx)
{
    fibers[1] = pCreateFiber(0, fiber_actctx_func, child_actctx);
    ok(fibers[1] != NULL, "CreateFiber returned error %lu\n", GetLastError());
    check_current_actctx_is(current_actctx, FALSE);

    SwitchToFiber(fibers[1]);
    check_current_actctx_is(current_actctx, FALSE);

    SwitchToFiber(fibers[1]);
    check_current_actctx_is(current_actctx, FALSE);

    SwitchToFiber(fibers[2]);
    check_current_actctx_is(current_actctx, FALSE);
    ok(fibers[2] == NULL, "expected fiber to be deleted (got %p)\n", fibers[2]);

    DeleteFiber(fibers[1]);
    fibers[1] = NULL;
}

static void WINAPI exit_thread_fiber_func(void *unused)
{
    BOOL ret;

    ret = pConvertFiberToThread();
    ok(ret, "ConvertFiberToThread returned error %lu\n", GetLastError());

    ExitThread(16);
}

static DWORD WINAPI thread_actctx_func_early_exit(void *actctx)
{
    HANDLE exit_thread_fiber;

    check_current_actctx_is(actctx, FALSE);

    fibers[1] = pConvertThreadToFiber(NULL);
    ok(fibers[1] != NULL, "ConvertThreadToFiber returned error %lu\n", GetLastError());
    check_current_actctx_is(actctx, FALSE);

    exit_thread_fiber = pCreateFiber(0, exit_thread_fiber_func, NULL);
    ok(exit_thread_fiber != NULL, "CreateFiber returned error %lu\n", GetLastError());

    /* exit thread, but keep current fiber */
    SwitchToFiber(exit_thread_fiber);
    check_current_actctx_is(actctx, FALSE);

    SwitchToFiber(fibers[0]);
    ok(0, "unreachable\n");
    return 17;
}

/* Test that fiber activation context stack is preserved regardless of creator thread's lifetime */
static void subtest_fiber_actctx_preservation(HANDLE current_actctx, HANDLE child_actctx)
{
    ULONG_PTR cookie;
    DWORD tid, wait;
    HANDLE thread;
    BOOL ret;

    ret = ActivateActCtx(child_actctx, &cookie);
    ok(ret, "ActivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(child_actctx, FALSE);

    thread = CreateThread(NULL, 0, thread_actctx_func_early_exit, child_actctx, 0, &tid);
    ok(thread != NULL, "CreateThread returned error %lu\n", GetLastError());

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(current_actctx, FALSE);

    wait = WaitForSingleObject(thread, INFINITE);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %lu (last error: %lu)\n",
       wait, GetLastError());
    CloseHandle(thread);

    /* The exited thread has been converted to a fiber */
    SwitchToFiber(fibers[1]);
    check_current_actctx_is(current_actctx, FALSE);

    DeleteFiber(fibers[1]);
    fibers[1] = NULL;
}

static HANDLE create_actctx_from_module_manifest(void)
{
    ACTCTXW actctx;

    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;
    actctx.lpResourceName = MAKEINTRESOURCEW(124);
    actctx.hModule = GetModuleHandleW(NULL);

    return CreateActCtxW(&actctx);
}

static void test_fiber_actctx(void)
{
    ULONG_PTR cookies[3];
    HANDLE actctxs[3];
    size_t i, j;
    BOOL ret;

    for (i = 0; i < ARRAY_SIZE(actctxs); i++)
    {
        actctxs[i] = create_actctx_from_module_manifest();
        ok(actctxs[i] != INVALID_HANDLE_VALUE, "failed to create context, error %lu\n", GetLastError());
        for (j = 0; j < i; j++)
        {
            ok(actctxs[i] != actctxs[j],
               "actctxs[%Iu] (%p) and actctxs[%Iu] (%p) should not alias\n",
               i, actctxs[i], j, actctxs[j]);
        }
    }

    ret = ActivateActCtx(actctxs[0], &cookies[0]);
    ok(ret, "ActivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(actctxs[0], FALSE);

    test_ConvertThreadToFiber();
    check_current_actctx_is(actctxs[0], FALSE);

    ret = ActivateActCtx(actctxs[1], &cookies[1]);
    ok(ret, "ActivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(actctxs[1], FALSE);

    subtest_fiber_actctx_switch(actctxs[1], actctxs[2]);
    subtest_fiber_actctx_preservation(actctxs[1], actctxs[2]);

    ret = DeactivateActCtx(0, cookies[1]);
    ok(ret, "DeactivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(actctxs[0], FALSE);

    test_ConvertFiberToThread();
    check_current_actctx_is(actctxs[0], FALSE);

    ret = DeactivateActCtx(0, cookies[0]);
    ok(ret, "DeactivateActCtx returned error %lu\n", GetLastError());
    check_current_actctx_is(NULL, FALSE);

    for (i = ARRAY_SIZE(actctxs); i > 0; )
        ReleaseActCtx(actctxs[--i]);
}


static void WINAPI fls_exit_deadlock_callback(void *arg)
{
    if (arg == (void *)1)
        Sleep(INFINITE);
    if (arg == (void *)2)
        /* Unfortunately this test won't affect the exit code if it fails, but
         * at least it will print a failure message. */
        ok(RtlDllShutdownInProgress(), "expected DLL shutdown\n");
}

static DWORD CALLBACK fls_exit_deadlock_thread(void *arg)
{
    FlsSetValue((DWORD_PTR)arg, (void *)1);
    return 0;
}

static void fls_exit_deadlock_child(void)
{
    DWORD index = FlsAlloc(fls_exit_deadlock_callback);
    FlsSetValue(index, (void *)2);
    CreateThread(NULL, 0, fls_exit_deadlock_thread, (void *)(DWORD_PTR)index, 0, NULL);
    Sleep(100);
    ExitProcess(0);
}

static void test_fls_exit_deadlock(void)
{
    char **argv, cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    BOOL ret;

    /* Regression test for the following deadlock:
     *
     * Thread A             Thread B
     * -----------------------------
     * ExitThread
     * acquire FLS lock
     * call FLS callback
     *                      ExitProcess
     *                      terminate thread A
     *                      acquire FLS lock
     *
     * Thread B deadlocks on acquiring the FLS lock (in order to itself call its
     * FLS callbacks.)
     */

    winetest_get_mainargs(&argv);
    sprintf(cmdline, "%s %s fls_exit_deadlock", argv[0], argv[1]);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "failed to create child, error %lu\n", GetLastError());
    ret = WaitForSingleObject(pi.hProcess, 1000);
    ok(!ret, "wait failed\n");
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

START_TEST(fiber)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);

    if (argc == 3 && !strcmp(argv[2], "fls_exit_deadlock"))
    {
        fls_exit_deadlock_child();
        return;
    }

    init_funcs();

    if (!pCreateFiber)
    {
        win_skip( "Fibers not supported by win95\n" );
        return;
    }

    test_FiberHandling();
    test_FiberLocalStorage();
    test_FiberLocalStorageCallback(FiberLocalStorageProc);
    test_FiberLocalStorageWithFibers(FiberLocalStorageProc);
    test_fiber_actctx();
    test_fls_exit_deadlock();
}
