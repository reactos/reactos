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

#ifdef __REACTOS__
#pragma once
#endif // __REACTOS__

typedef struct {
    HTMLElement element;

    IHTMLScriptElement IHTMLScriptElement_iface;

    nsIDOMHTMLScriptElement *nsscript;
    BOOL parsed;
    BOOL parse_on_bind;
    BOOL pending_readystatechange_event;
    READYSTATE readystate;
} HTMLScriptElement;

typedef struct {
    struct list entry;
    HTMLScriptElement *script;
} script_queue_entry_t;

HRESULT script_elem_from_nsscript(HTMLDocumentNode*,nsIDOMHTMLScriptElement*,HTMLScriptElement**) DECLSPEC_HIDDEN;
void bind_event_scripts(HTMLDocumentNode*) DECLSPEC_HIDDEN;

void release_script_hosts(HTMLInnerWindow*) DECLSPEC_HIDDEN;
void connect_scripts(HTMLInnerWindow*) DECLSPEC_HIDDEN;
void doc_insert_script(HTMLInnerWindow*,HTMLScriptElement*) DECLSPEC_HIDDEN;
IDispatch *script_parse_event(HTMLInnerWindow*,LPCWSTR) DECLSPEC_HIDDEN;
HRESULT exec_script(HTMLInnerWindow*,const WCHAR*,const WCHAR*,VARIANT*) DECLSPEC_HIDDEN;
void set_script_mode(HTMLOuterWindow*,SCRIPTMODE) DECLSPEC_HIDDEN;
BOOL find_global_prop(HTMLInnerWindow*,BSTR,DWORD,ScriptHost**,DISPID*) DECLSPEC_HIDDEN;
IDispatch *get_script_disp(ScriptHost*) DECLSPEC_HIDDEN;
