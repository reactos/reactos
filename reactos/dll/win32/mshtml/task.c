/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

#define WM_PROCESSTASK 0x8008
#define TIMER_ID 0x3000

typedef struct {
    HTMLInnerWindow *window;
    DWORD id;
    DWORD time;
    DWORD interval;
    IDispatch *disp;

    struct list entry;
} task_timer_t;

static void default_task_destr(task_t *task)
{
    heap_free(task);
}

HRESULT push_task(task_t *task, task_proc_t proc, task_proc_t destr, LONG magic)
{
    thread_data_t *thread_data;

    thread_data = get_thread_data(TRUE);
    if(!thread_data) {
        if(destr)
            destr(task);
        else
            heap_free(task);
        return E_OUTOFMEMORY;
    }

    task->target_magic = magic;
    task->proc = proc;
    task->destr = destr ? destr : default_task_destr;
    task->next = NULL;

    if(thread_data->task_queue_tail)
        thread_data->task_queue_tail->next = task;
    else
        thread_data->task_queue_head = task;

    thread_data->task_queue_tail = task;

    PostMessageW(thread_data->thread_hwnd, WM_PROCESSTASK, 0, 0);
    return S_OK;
}

static task_t *pop_task(void)
{
    thread_data_t *thread_data;
    task_t *task;

    thread_data = get_thread_data(FALSE);
    if(!thread_data)
        return NULL;

    task = thread_data->task_queue_head;
    if(!task)
        return NULL;

    thread_data->task_queue_head = task->next;
    if(!thread_data->task_queue_head)
        thread_data->task_queue_tail = NULL;

    return task;
}

static void release_task_timer(HWND thread_hwnd, task_timer_t *timer)
{
    list_remove(&timer->entry);

    IDispatch_Release(timer->disp);

    heap_free(timer);
}

void remove_target_tasks(LONG target)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    struct list *liter, *ltmp;
    task_timer_t *timer;
    task_t *iter, *tmp;

    if(!thread_data)
        return;

    LIST_FOR_EACH_SAFE(liter, ltmp, &thread_data->timer_list) {
        timer = LIST_ENTRY(liter, task_timer_t, entry);
        if(timer->window->task_magic == target)
            release_task_timer(thread_data->thread_hwnd, timer);
    }

    if(!list_empty(&thread_data->timer_list)) {
        DWORD tc = GetTickCount();

        timer = LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry);
        SetTimer(thread_data->thread_hwnd, TIMER_ID, max( (int)(timer->time - tc), 0 ), NULL);
    }

    while(thread_data->task_queue_head && thread_data->task_queue_head->target_magic == target) {
        iter = pop_task();
        iter->destr(iter);
    }

    for(iter = thread_data->task_queue_head; iter; iter = iter->next) {
        while(iter->next && iter->next->target_magic == target) {
            tmp = iter->next;
            iter->next = tmp->next;
            tmp->destr(tmp);
        }

        if(!iter->next)
            thread_data->task_queue_tail = iter;
    }
}

LONG get_task_target_magic(void)
{
    static LONG magic = 0x10000000;
    return InterlockedIncrement(&magic);
}

static BOOL queue_timer(thread_data_t *thread_data, task_timer_t *timer)
{
    task_timer_t *iter;

    list_remove(&timer->entry);

    if(list_empty(&thread_data->timer_list)
       || LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry)->time > timer->time) {

        list_add_head(&thread_data->timer_list, &timer->entry);
        return TRUE;
    }

    LIST_FOR_EACH_ENTRY(iter, &thread_data->timer_list, task_timer_t, entry) {
        if(iter->time > timer->time) {
            list_add_tail(&iter->entry, &timer->entry);
            return FALSE;
        }
    }

    list_add_tail(&thread_data->timer_list, &timer->entry);
    return FALSE;
}

HRESULT set_task_timer(HTMLInnerWindow *window, DWORD msec, BOOL interval, IDispatch *disp, LONG *id)
{
    thread_data_t *thread_data;
    task_timer_t *timer;
    DWORD tc = GetTickCount();

    static DWORD id_cnt = 0x20000000;

    thread_data = get_thread_data(TRUE);
    if(!thread_data)
        return E_OUTOFMEMORY;

    timer = heap_alloc(sizeof(task_timer_t));
    if(!timer)
        return E_OUTOFMEMORY;

    timer->id = id_cnt++;
    timer->window = window;
    timer->time = tc + msec;
    timer->interval = interval ? msec : 0;
    list_init(&timer->entry);

    IDispatch_AddRef(disp);
    timer->disp = disp;

    if(queue_timer(thread_data, timer))
        SetTimer(thread_data->thread_hwnd, TIMER_ID, msec, NULL);

    *id = timer->id;
    return S_OK;
}

HRESULT clear_task_timer(HTMLInnerWindow *window, BOOL interval, DWORD id)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    task_timer_t *iter;

    if(!thread_data)
        return S_OK;

    LIST_FOR_EACH_ENTRY(iter, &thread_data->timer_list, task_timer_t, entry) {
        if(iter->id == id && iter->window == window && !iter->interval == !interval) {
            release_task_timer(thread_data->thread_hwnd, iter);
            return S_OK;
        }
    }

    WARN("timet not found\n");
    return S_OK;
}

static void call_timer_disp(IDispatch *disp)
{
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    EXCEPINFO ei;
    VARIANT res;
    HRESULT hres;

    V_VT(&res) = VT_EMPTY;
    memset(&ei, 0, sizeof(ei));

    TRACE(">>>\n");
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dp, &res, &ei, NULL);
    if(hres == S_OK)
        TRACE("<<<\n");
    else
        WARN("<<< %08x\n", hres);

    VariantClear(&res);
}

static LRESULT process_timer(void)
{
    thread_data_t *thread_data;
    IDispatch *disp;
    DWORD tc;
    task_timer_t *timer=NULL, *last_timer;

    TRACE("\n");

    thread_data = get_thread_data(FALSE);
    assert(thread_data != NULL);

    if(list_empty(&thread_data->timer_list)) {
        KillTimer(thread_data->thread_hwnd, TIMER_ID);
        return 0;
    }

    last_timer = LIST_ENTRY(list_tail(&thread_data->timer_list), task_timer_t, entry);
    do {
        tc = GetTickCount();
        if(timer == last_timer) {
            timer = LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry);
            SetTimer(thread_data->thread_hwnd, TIMER_ID, timer->time>tc ? timer->time-tc : 0, NULL);
            return 0;
        }

        timer = LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry);

        if(timer->time > tc) {
            SetTimer(thread_data->thread_hwnd, TIMER_ID, timer->time-tc, NULL);
            return 0;
        }

        disp = timer->disp;
        IDispatch_AddRef(disp);

        if(timer->interval) {
            timer->time += timer->interval;
            queue_timer(thread_data, timer);
        }else {
            release_task_timer(thread_data->thread_hwnd, timer);
        }

        call_timer_disp(disp);

        IDispatch_Release(disp);
    }while(!list_empty(&thread_data->timer_list));

    KillTimer(thread_data->thread_hwnd, TIMER_ID);
    return 0;
}

static LRESULT WINAPI hidden_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_PROCESSTASK:
        while(1) {
            task_t *task = pop_task();
            if(!task)
                break;

            task->proc(task);
            task->destr(task);
        }

        return 0;
    case WM_TIMER:
        return process_timer();
    }

    if(msg > WM_USER)
        FIXME("(%p %d %lx %lx)\n", hwnd, msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND create_thread_hwnd(void)
{
    static ATOM hidden_wnd_class = 0;
    static const WCHAR wszInternetExplorer_Hidden[] = {'I','n','t','e','r','n','e','t',
            ' ','E','x','p','l','o','r','e','r','_','H','i','d','d','e','n',0};

    if(!hidden_wnd_class) {
        WNDCLASSEXW wndclass = {
            sizeof(WNDCLASSEXW), 0,
            hidden_proc,
            0, 0, hInst, NULL, NULL, NULL, NULL,
            wszInternetExplorer_Hidden,
            NULL
        };

        hidden_wnd_class = RegisterClassExW(&wndclass);
    }

    return CreateWindowExW(0, wszInternetExplorer_Hidden, NULL, WS_POPUP,
                           0, 0, 0, 0, NULL, NULL, hInst, NULL);
}

HWND get_thread_hwnd(void)
{
    thread_data_t *thread_data;

    thread_data = get_thread_data(TRUE);
    if(!thread_data)
        return NULL;

    if(!thread_data->thread_hwnd)
        thread_data->thread_hwnd = create_thread_hwnd();

    return thread_data->thread_hwnd;
}

thread_data_t *get_thread_data(BOOL create)
{
    thread_data_t *thread_data;

    if(mshtml_tls == TLS_OUT_OF_INDEXES) {
        DWORD tls;

        if(!create)
            return NULL;

        tls = TlsAlloc();
        if(tls == TLS_OUT_OF_INDEXES)
            return NULL;

        tls = InterlockedCompareExchange((LONG*)&mshtml_tls, tls, TLS_OUT_OF_INDEXES);
        if(tls != mshtml_tls)
            TlsFree(tls);
    }

    thread_data = TlsGetValue(mshtml_tls);
    if(!thread_data && create) {
        thread_data = heap_alloc_zero(sizeof(thread_data_t));
        if(!thread_data)
            return NULL;

        TlsSetValue(mshtml_tls, thread_data);
        list_init(&thread_data->timer_list);
    }

    return thread_data;
}
