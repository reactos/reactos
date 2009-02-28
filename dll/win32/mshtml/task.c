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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmcid.h"
#include "shlguid.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define WM_PROCESSTASK 0x8008
#define TIMER_ID 0x3000

typedef struct {
    HTMLDocument *doc;
    DWORD id;
    DWORD time;
    DWORD interval;
    IDispatch *disp;

    struct list entry;
} task_timer_t;

void push_task(task_t *task)
{
    thread_data_t *thread_data = get_thread_data(TRUE);

    if(thread_data->task_queue_tail)
        thread_data->task_queue_tail->next = task;
    else
        thread_data->task_queue_head = task;

    thread_data->task_queue_tail = task;

    PostMessageW(thread_data->thread_hwnd, WM_PROCESSTASK, 0, 0);
}

static task_t *pop_task(void)
{
    thread_data_t *thread_data = get_thread_data(TRUE);
    task_t *task = thread_data->task_queue_head;

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

void remove_doc_tasks(const HTMLDocument *doc)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    struct list *liter, *ltmp;
    task_timer_t *timer;
    task_t *iter, *tmp;

    LIST_FOR_EACH_SAFE(liter, ltmp, &thread_data->timer_list) {
        timer = LIST_ENTRY(liter, task_timer_t, entry);
        if(timer->doc == doc)
            release_task_timer(thread_data->thread_hwnd, timer);
    }

    if(!list_empty(&thread_data->timer_list)) {
        timer = LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry);
        SetTimer(thread_data->thread_hwnd, TIMER_ID, timer->time - GetTickCount(), NULL);
    }

    if(!thread_data)
        return;

    while(thread_data->task_queue_head
          && thread_data->task_queue_head->doc == doc)
        pop_task();

    for(iter = thread_data->task_queue_head; iter; iter = iter->next) {
        while(iter->next && iter->next->doc == doc) {
            tmp = iter->next;
            iter->next = tmp->next;
            heap_free(tmp);
        }

        if(!iter->next)
            thread_data->task_queue_tail = iter;
    }
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

DWORD set_task_timer(HTMLDocument *doc, DWORD msec, BOOL interval, IDispatch *disp)
{
    thread_data_t *thread_data = get_thread_data(TRUE);
    task_timer_t *timer;
    DWORD tc = GetTickCount();

    static DWORD id_cnt = 0x20000000;

    timer = heap_alloc(sizeof(task_timer_t));
    timer->id = id_cnt++;
    timer->doc = doc;
    timer->time = tc + msec;
    timer->interval = interval ? msec : 0;
    list_init(&timer->entry);

    IDispatch_AddRef(disp);
    timer->disp = disp;

    if(queue_timer(thread_data, timer))
        SetTimer(thread_data->thread_hwnd, TIMER_ID, msec, NULL);

    return timer->id;
}

HRESULT clear_task_timer(HTMLDocument *doc, BOOL interval, DWORD id)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    task_timer_t *iter;

    if(!thread_data)
        return S_OK;

    LIST_FOR_EACH_ENTRY(iter, &thread_data->timer_list, task_timer_t, entry) {
        if(iter->id == id && iter->doc == doc && (iter->interval == 0) == !interval) {
            release_task_timer(thread_data->thread_hwnd, iter);
            return S_OK;
        }
    }

    WARN("timet not found\n");
    return S_OK;
}

static void set_downloading(HTMLDocument *doc)
{
    IOleCommandTarget *olecmd;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    if(doc->frame)
        IOleInPlaceFrame_SetStatusText(doc->frame, NULL /* FIXME */);

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(SUCCEEDED(hres)) {
        VARIANT var;

        V_VT(&var) = VT_I4;
        V_I4(&var) = 1;

        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETDOWNLOADSTATE, OLECMDEXECOPT_DONTPROMPTUSER,
                               &var, NULL);
        IOleCommandTarget_Release(olecmd);
    }

    if(doc->hostui) {
        IDropTarget *drop_target = NULL;

        hres = IDocHostUIHandler_GetDropTarget(doc->hostui, NULL /* FIXME */, &drop_target);
        if(drop_target) {
            FIXME("Use IDropTarget\n");
            IDropTarget_Release(drop_target);
        }
    }
}

/* Calls undocumented 69 cmd of CGID_Explorer */
static void call_explorer_69(HTMLDocument *doc)
{
    IOleCommandTarget *olecmd;
    VARIANT var;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(FAILED(hres))
        return;

    VariantInit(&var);
    hres = IOleCommandTarget_Exec(olecmd, &CGID_Explorer, 69, 0, NULL, &var);
    IOleCommandTarget_Release(olecmd);
    if(SUCCEEDED(hres) && V_VT(&var) != VT_NULL)
        FIXME("handle result\n");
}

static void set_parsecomplete(HTMLDocument *doc)
{
    IOleCommandTarget *olecmd = NULL;

    TRACE("(%p)\n", doc);

    if(doc->usermode == EDITMODE)
        init_editor(doc);

    call_explorer_69(doc);
    call_property_onchanged(&doc->cp_propnotif, 1005);
    call_explorer_69(doc);

    /* FIXME: IE7 calls EnableModelless(TRUE), EnableModelless(FALSE) and sets interactive state here */

    doc->readystate = READYSTATE_INTERACTIVE;
    call_property_onchanged(&doc->cp_propnotif, DISPID_READYSTATE);

    if(doc->client)
        IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);

    if(olecmd) {
        VARIANT state, progress;

        V_VT(&progress) = VT_I4;
        V_I4(&progress) = 0;
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSPOS, OLECMDEXECOPT_DONTPROMPTUSER,
                               &progress, NULL);

        V_VT(&state) = VT_I4;
        V_I4(&state) = 0;
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETDOWNLOADSTATE, OLECMDEXECOPT_DONTPROMPTUSER,
                               &state, NULL);

        IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 103, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, &CGID_MSHTML, IDM_PARSECOMPLETE, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_HTTPEQUIV_DONE, 0, NULL, NULL);

        IOleCommandTarget_Release(olecmd);
    }

    doc->readystate = READYSTATE_COMPLETE;
    call_property_onchanged(&doc->cp_propnotif, DISPID_READYSTATE);

    if(doc->frame) {
        static const WCHAR wszDone[] = {'D','o','n','e',0};
        IOleInPlaceFrame_SetStatusText(doc->frame, wszDone);
    }

    update_title(doc);
}

static void set_progress(HTMLDocument *doc)
{
    IOleCommandTarget *olecmd = NULL;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    if(doc->client)
        IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);

    if(olecmd) {
        VARIANT progress_max, progress;

        V_VT(&progress_max) = VT_I4;
        V_I4(&progress_max) = 0; /* FIXME */
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSMAX, OLECMDEXECOPT_DONTPROMPTUSER,
                               &progress_max, NULL);

        V_VT(&progress) = VT_I4;
        V_I4(&progress) = 0; /* FIXME */
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSPOS, OLECMDEXECOPT_DONTPROMPTUSER,
                               &progress, NULL);
    }

    if(doc->usermode == EDITMODE && doc->hostui) {
        DOCHOSTUIINFO hostinfo;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(doc->hostui, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%u %08x %08x %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
    }
}

static void task_start_binding(HTMLDocument *doc, BSCallback *bscallback)
{
    if(doc)
        start_binding(doc, bscallback, NULL);
    IUnknown_Release((IUnknown*)bscallback);
}

static void process_task(task_t *task)
{
    switch(task->task_id) {
    case TASK_SETDOWNLOADSTATE:
        set_downloading(task->doc);
        break;
    case TASK_PARSECOMPLETE:
        set_parsecomplete(task->doc);
        break;
    case TASK_SETPROGRESS:
        set_progress(task->doc);
        break;
    case TASK_START_BINDING:
        task_start_binding(task->doc, (BSCallback*)task->bscallback);
        break;
    default:
        ERR("Wrong task_id %d\n", task->task_id);
    }
}

static void call_timer_disp(IDispatch *disp)
{
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    EXCEPINFO ei;
    VARIANT res;
    HRESULT hres;

    V_VT(&res) = VT_EMPTY;
    memset(&ei, 0, sizeof(ei));
    hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_METHOD, &dp, &res, &ei, NULL);
    TRACE("ret %08x %s\n", hres, debugstr_variant(&res));
    VariantClear(&res);
}

static LRESULT process_timer(void)
{
    thread_data_t *thread_data = get_thread_data(TRUE);
    HTMLDocument *doc;
    IDispatch *disp;
    DWORD tc;
    task_timer_t *timer;

    TRACE("\n");

    while(!list_empty(&thread_data->timer_list)) {
        timer = LIST_ENTRY(list_head(&thread_data->timer_list), task_timer_t, entry);

        tc = GetTickCount();
        if(timer->time > tc) {
            SetTimer(thread_data->thread_hwnd, TIMER_ID, timer->time-tc, NULL);
            return 0;
        }

        doc = timer->doc;
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
    }

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

            process_task(task);
            heap_free(task);
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
    thread_data_t *thread_data = get_thread_data(TRUE);

    if(!thread_data->thread_hwnd)
        thread_data->thread_hwnd = create_thread_hwnd();

    return thread_data->thread_hwnd;
}

thread_data_t *get_thread_data(BOOL create)
{
    thread_data_t *thread_data;

    if(!mshtml_tls) {
        if(create)
            mshtml_tls = TlsAlloc();
        else
            return NULL;
    }

    thread_data = TlsGetValue(mshtml_tls);
    if(!thread_data && create) {
        thread_data = heap_alloc_zero(sizeof(thread_data_t));
        TlsSetValue(mshtml_tls, thread_data);
        list_init(&thread_data->timer_list);
    }

    return thread_data;
}
