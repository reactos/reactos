/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include "activscp.h"

struct HTMLScriptElement {
    HTMLElement element;

    IHTMLScriptElement IHTMLScriptElement_iface;

    nsIDOMHTMLScriptElement *nsscript;
    BOOL parsed;
    BOOL parse_on_bind;
    BOOL pending_readystatechange_event;
    READYSTATE readystate;
    WCHAR *src_text; /* sctipt text downloaded from src */
    BSCallback *binding; /* weak reference to current binding */
};

typedef struct {
    struct list entry;
    HTMLScriptElement *script;
} script_queue_entry_t;

HRESULT script_elem_from_nsscript(nsIDOMHTMLScriptElement*,HTMLScriptElement**);
void bind_event_scripts(HTMLDocumentNode*);
HRESULT load_script(HTMLScriptElement*,const WCHAR*,BOOL);

void release_script_hosts(HTMLInnerWindow*);
void connect_scripts(HTMLInnerWindow*);
void doc_insert_script(HTMLInnerWindow*,HTMLScriptElement*,BOOL);
IDispatch *script_parse_event(HTMLInnerWindow*,LPCWSTR);
HRESULT exec_script(HTMLInnerWindow*,const WCHAR*,const WCHAR*,VARIANT*);
void update_browser_script_mode(GeckoBrowser*,IUri*);
BOOL find_global_prop(HTMLInnerWindow*,const WCHAR*,DWORD,ScriptHost**,DISPID*);
HRESULT global_prop_still_exists(HTMLInnerWindow*,global_prop_t*);
IDispatch *get_script_disp(ScriptHost*);
IWineJSDispatch *get_script_jsdisp(ScriptHost*);
IActiveScriptSite *get_first_script_site(HTMLInnerWindow*);
void initialize_script_global(HTMLInnerWindow*);
