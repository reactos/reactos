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

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define WM_PROCESSTASK 0x8008

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

void remove_doc_tasks(const HTMLDocument *doc)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    task_t *iter, *tmp;

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

static void set_parsecomplete(HTMLDocument *doc)
{
    IOleCommandTarget *olecmd = NULL;

    TRACE("(%p)\n", doc);

    if(doc->usermode == EDITMODE)
        init_editor(doc);

    call_property_onchanged(&doc->cp_propnotif, 1005);

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
    }

    return thread_data;
}
