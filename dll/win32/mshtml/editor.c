/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NSCMD_ALIGN        "cmd_align"
#define NSCMD_BEGINLINE    "cmd_beginLine"
#define NSCMD_BOLD         "cmd_bold"
#define NSCMD_CHARNEXT     "cmd_charNext"
#define NSCMD_CHARPREVIOUS "cmd_charPrevious"
#define NSCMD_COPY         "cmd_copy"
#define NSCMD_CUT          "cmd_cut"
#define NSCMD_DELETECHARFORWARD   "cmd_deleteCharForward"
#define NSCMD_DELETEWORDFORWARD   "cmd_deleteWordForward"
#define NSCMD_ENDLINE      "cmd_endLine"
#define NSCMD_FONTCOLOR    "cmd_fontColor"
#define NSCMD_FONTFACE     "cmd_fontFace"
#define NSCMD_INDENT       "cmd_indent"
#define NSCMD_INSERTHR     "cmd_insertHR"
#define NSCMD_INSERTLINKNOUI    "cmd_insertLinkNoUI"
#define NSCMD_ITALIC       "cmd_italic"
#define NSCMD_LINENEXT     "cmd_lineNext"
#define NSCMD_LINEPREVIOUS "cmd_linePrevious"
#define NSCMD_MOVEBOTTOM   "cmd_moveBottom"
#define NSCMD_MOVEPAGEDOWN "cmd_movePageDown"
#define NSCMD_MOVEPAGEUP   "cmd_movePageUp"
#define NSCMD_MOVETOP      "cmd_moveTop"
#define NSCMD_OL           "cmd_ol"
#define NSCMD_OUTDENT      "cmd_outdent"
#define NSCMD_PASTE        "cmd_paste"
#define NSCMD_SELECTBEGINLINE     "cmd_selectBeginLine"
#define NSCMD_SELECTBOTTOM        "cmd_selectBottom"
#define NSCMD_SELECTCHARNEXT      "cmd_selectCharNext"
#define NSCMD_SELECTCHARPREVIOUS  "cmd_selectCharPrevious"
#define NSCMD_SELECTENDLINE       "cmd_selectEndLine"
#define NSCMD_SELECTLINENEXT      "cmd_selectLineNext"
#define NSCMD_SELECTLINEPREVIOUS  "cmd_selectLinePrevious"
#define NSCMD_SELECTPAGEDOWN      "cmd_selectPageDown"
#define NSCMD_SELECTPAGEUP        "cmd_selectPageUp"
#define NSCMD_SELECTTOP           "cmd_selectTop"
#define NSCMD_SELECTWORDNEXT      "cmd_selectWordNext"
#define NSCMD_SELECTWORDPREVIOUS  "cmd_selectWordPrevious"
#define NSCMD_UL           "cmd_ul"
#define NSCMD_UNDERLINE    "cmd_underline"
#define NSCMD_WORDNEXT     "cmd_wordNext"
#define NSCMD_WORDPREVIOUS "cmd_wordPrevious"

#define NSSTATE_ATTRIBUTE "state_attribute"
#define NSSTATE_ALL       "state_all"

#define NSALIGN_CENTER "center"
#define NSALIGN_LEFT   "left"
#define NSALIGN_RIGHT  "right"

#define DOM_VK_LEFT     VK_LEFT
#define DOM_VK_UP       VK_UP
#define DOM_VK_RIGHT    VK_RIGHT
#define DOM_VK_DOWN     VK_DOWN
#define DOM_VK_DELETE   VK_DELETE
#define DOM_VK_HOME     VK_HOME
#define DOM_VK_END      VK_END

void set_dirty(GeckoBrowser *browser, VARIANT_BOOL dirty)
{
    nsresult nsres;

    if(browser->usermode != EDITMODE || !browser->editor)
        return;

    if(dirty) {
        nsres = nsIEditor_IncrementModificationCount(browser->editor, 1);
        if(NS_FAILED(nsres))
            ERR("IncrementModificationCount failed: %08lx\n", nsres);
    }else {
        nsres = nsIEditor_ResetModificationCount(browser->editor);
        if(NS_FAILED(nsres))
            ERR("ResetModificationCount failed: %08lx\n", nsres);
    }
}

static void do_ns_editor_command(GeckoBrowser *This, const char *cmd)
{
    nsresult nsres;

    if(!This->editor_controller)
        return;

    nsres = nsIController_DoCommand(This->editor_controller, cmd);
    if(NS_FAILED(nsres))
        ERR("DoCommand(%s) failed: %08lx\n", debugstr_a(cmd), nsres);
}

static nsresult get_ns_command_state(GeckoBrowser *This, const char *cmd, nsICommandParams *nsparam)
{
    nsICommandManager *cmdmgr;
    nsresult nsres;

    nsres = get_nsinterface((nsISupports*)This->webbrowser, &IID_nsICommandManager, (void**)&cmdmgr);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsICommandManager: %08lx\n", nsres);
        return nsres;
    }

    nsres = nsICommandManager_GetCommandState(cmdmgr, cmd, This->doc->window->window_proxy, nsparam);
    if(NS_FAILED(nsres))
        ERR("GetCommandState(%s) failed: %08lx\n", debugstr_a(cmd), nsres);

    nsICommandManager_Release(cmdmgr);
    return nsres;
}

static DWORD query_ns_edit_status(HTMLDocumentNode *doc, const char *nscmd)
{
    nsICommandParams *nsparam;
    cpp_bool b = FALSE;

    if(doc->browser->usermode != EDITMODE || !doc->window->base.outer_window || doc->window->base.outer_window->readystate < READYSTATE_INTERACTIVE)
        return OLECMDF_SUPPORTED;

    if(nscmd) {
        nsparam = create_nscommand_params();
        get_ns_command_state(doc->browser, nscmd, nsparam);

        nsICommandParams_GetBooleanValue(nsparam, NSSTATE_ALL, &b);

        nsICommandParams_Release(nsparam);
    }

    return OLECMDF_SUPPORTED | OLECMDF_ENABLED | (b ? OLECMDF_LATCHED : 0);
}

static void set_ns_align(HTMLDocumentNode *doc, const char *align_str)
{
    nsICommandParams *nsparam;

    nsparam = create_nscommand_params();
    nsICommandParams_SetCStringValue(nsparam, NSSTATE_ATTRIBUTE, align_str);

    do_ns_command(doc, NSCMD_ALIGN, nsparam);

    nsICommandParams_Release(nsparam);
}

static DWORD query_align_status(HTMLDocumentNode *doc, const WCHAR *align)
{
    DWORD ret = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
    nsAString justify_str;
    cpp_bool b;
    nsresult nsres;

    if(doc->browser->usermode != EDITMODE || !doc->window->base.outer_window || doc->window->base.outer_window->readystate < READYSTATE_INTERACTIVE)
        return OLECMDF_SUPPORTED;

    if(!doc->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&justify_str, align);
    nsres = nsIDOMHTMLDocument_QueryCommandState(doc->html_document, &justify_str, &b);
    nsAString_Finish(&justify_str);
    if(NS_SUCCEEDED(nsres) && b)
        ret |= OLECMDF_LATCHED;

    return ret;
}


static nsISelection *get_ns_selection(HTMLDocumentNode *doc)
{
    nsISelection *nsselection = NULL;
    nsresult nsres;

    if(!doc->window)
        return NULL;

    nsres = nsIDOMWindow_GetSelection(doc->window->dom_window, &nsselection);
    if(NS_FAILED(nsres))
        ERR("GetSelection failed %08lx\n", nsres);

    return nsselection;

}

static void remove_child_attr(nsIDOMElement *elem, LPCWSTR tag, nsAString *attr_str)
{
    cpp_bool has_children;
    UINT32 child_cnt, i;
    nsIDOMNode *child_node;
    nsIDOMNodeList *node_list;
    UINT16 node_type;

    nsIDOMElement_HasChildNodes(elem, &has_children);
    if(!has_children)
        return;

    nsIDOMElement_GetChildNodes(elem, &node_list);
    nsIDOMNodeList_GetLength(node_list, &child_cnt);

    for(i=0; i<child_cnt; i++) {
        nsIDOMNodeList_Item(node_list, i, &child_node);

        nsIDOMNode_GetNodeType(child_node, &node_type);
        if(node_type == ELEMENT_NODE) {
            nsIDOMElement *child_elem;
            nsAString tag_str;
            const PRUnichar *ctag;

            nsIDOMNode_QueryInterface(child_node, &IID_nsIDOMElement, (void**)&child_elem);

            nsAString_Init(&tag_str, NULL);
            nsIDOMElement_GetTagName(child_elem, &tag_str);
            nsAString_GetData(&tag_str, &ctag);

            if(!wcsicmp(ctag, tag))
                /* FIXME: remove node if there are no more attributes */
                nsIDOMElement_RemoveAttribute(child_elem, attr_str);

            nsAString_Finish(&tag_str);

            remove_child_attr(child_elem, tag, attr_str);

            nsIDOMElement_Release(child_elem);
        }

        nsIDOMNode_Release(child_node);
    }

    nsIDOMNodeList_Release(node_list);
}

static void get_font_size(HTMLDocumentNode *doc, WCHAR *ret)
{
    nsISelection *nsselection = get_ns_selection(doc);
    nsIDOMElement *elem = NULL;
    nsIDOMNode *node = NULL, *tmp_node;
    nsAString tag_str;
    LPCWSTR tag;
    UINT16 node_type;
    nsresult nsres;

    *ret = 0;

    if(!nsselection)
        return;

    nsISelection_GetFocusNode(nsselection, &node);
    nsISelection_Release(nsselection);

    while(node) {
        nsres = nsIDOMNode_GetNodeType(node, &node_type);
        if(NS_FAILED(nsres) || node_type == DOCUMENT_NODE)
            break;

        if(node_type == ELEMENT_NODE) {
            nsIDOMNode_QueryInterface(node, &IID_nsIDOMElement, (void**)&elem);

            nsAString_Init(&tag_str, NULL);
            nsIDOMElement_GetTagName(elem, &tag_str);
            nsAString_GetData(&tag_str, &tag);

            if(!wcsicmp(tag, L"font")) {
                nsAString val_str;
                const PRUnichar *val;

                TRACE("found font tag %p\n", elem);

                get_elem_attr_value(elem, L"size", &val_str, &val);
                if(*val) {
                    TRACE("found size %s\n", debugstr_w(val));
                    lstrcpyW(ret, val);
                }

                nsAString_Finish(&val_str);
            }

            nsAString_Finish(&tag_str);
            nsIDOMElement_Release(elem);
        }

        if(*ret)
            break;

        tmp_node = node;
        nsIDOMNode_GetParentNode(node, &node);
        nsIDOMNode_Release(tmp_node);
    }

    if(node)
        nsIDOMNode_Release(node);
}

static void set_font_size(HTMLDocumentNode *doc, LPCWSTR size)
{
    nsISelection *nsselection;
    cpp_bool collapsed;
    nsIDOMElement *elem;
    nsIDOMRange *range;
    LONG range_cnt = 0;
    nsAString size_str;
    nsAString val_str;

    nsselection = get_ns_selection(doc);
    if(!nsselection)
        return;

    nsISelection_GetRangeCount(nsselection, &range_cnt);
    if(range_cnt != 1) {
        FIXME("range_cnt %ld not supprted\n", range_cnt);
        if(!range_cnt) {
            nsISelection_Release(nsselection);
            return;
        }
    }

    create_nselem(doc, L"font", &elem);

    nsAString_InitDepend(&size_str, L"size");
    nsAString_InitDepend(&val_str, size);

    nsIDOMElement_SetAttribute(elem, &size_str, &val_str);
    nsAString_Finish(&val_str);

    nsISelection_GetRangeAt(nsselection, 0, &range);
    nsISelection_GetIsCollapsed(nsselection, &collapsed);
    nsISelection_RemoveAllRanges(nsselection);

    nsIDOMRange_SurroundContents(range, (nsIDOMNode*)elem);

    if(collapsed) {
        nsISelection_Collapse(nsselection, (nsIDOMNode*)elem, 0);
    }else {
        /* Remove all size attributes from the range */
        remove_child_attr(elem, L"font", &size_str);
        nsISelection_SelectAllChildren(nsselection, (nsIDOMNode*)elem);
    }

    nsISelection_Release(nsselection);
    nsIDOMRange_Release(range);
    nsIDOMElement_Release(elem);

    nsAString_Finish(&size_str);

    set_dirty(doc->browser, VARIANT_TRUE);
}

static void handle_arrow_key(HTMLDocumentNode *doc, nsIDOMEvent *event, nsIDOMKeyEvent *key_event, const char * const cmds[4])
{
    int i=0;
    cpp_bool b;

    nsIDOMKeyEvent_GetCtrlKey(key_event, &b);
    if(b)
        i |= 1;

    nsIDOMKeyEvent_GetShiftKey(key_event, &b);
    if(b)
        i |= 2;

    if(cmds[i])
        do_ns_editor_command(doc->browser, cmds[i]);

    nsIDOMEvent_PreventDefault(event);
}

void handle_edit_event(HTMLDocumentNode *doc, nsIDOMEvent *event)
{
    nsIDOMKeyEvent *key_event;
    UINT32 code;

    nsIDOMEvent_QueryInterface(event, &IID_nsIDOMKeyEvent, (void**)&key_event);

    nsIDOMKeyEvent_GetKeyCode(key_event, &code);

    switch(code) {
    case DOM_VK_LEFT: {
        static const char * const cmds[] = {
            NSCMD_CHARPREVIOUS,
            NSCMD_WORDPREVIOUS,
            NSCMD_SELECTCHARPREVIOUS,
            NSCMD_SELECTWORDPREVIOUS
        };

        TRACE("left\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_RIGHT: {
        static const char * const cmds[] = {
            NSCMD_CHARNEXT,
            NSCMD_WORDNEXT,
            NSCMD_SELECTCHARNEXT,
            NSCMD_SELECTWORDNEXT
        };

        TRACE("right\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_UP: {
        static const char * const cmds[] = {
            NSCMD_LINEPREVIOUS,
            NSCMD_MOVEPAGEUP,
            NSCMD_SELECTLINEPREVIOUS,
            NSCMD_SELECTPAGEUP
        };

        TRACE("up\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_DOWN: {
        static const char * const cmds[] = {
            NSCMD_LINENEXT,
            NSCMD_MOVEPAGEDOWN,
            NSCMD_SELECTLINENEXT,
            NSCMD_SELECTPAGEDOWN
        };

        TRACE("down\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_DELETE: {
        static const char * const cmds[] = {
            NSCMD_DELETECHARFORWARD,
            NSCMD_DELETEWORDFORWARD,
            NULL, NULL
        };

        TRACE("delete\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_HOME: {
        static const char * const cmds[] = {
            NSCMD_BEGINLINE,
            NSCMD_MOVETOP,
            NSCMD_SELECTBEGINLINE,
            NSCMD_SELECTTOP
        };

        TRACE("home\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    case DOM_VK_END: {
        static const char * const cmds[] = {
            NSCMD_ENDLINE,
            NSCMD_MOVEBOTTOM,
            NSCMD_SELECTENDLINE,
            NSCMD_SELECTBOTTOM
        };

        TRACE("end\n");
        handle_arrow_key(doc, event, key_event, cmds);
        break;
    }
    }

    nsIDOMKeyEvent_Release(key_event);
}

static void set_ns_fontname(HTMLDocumentNode *doc, const char *fontname)
{
    nsICommandParams *nsparam = create_nscommand_params();

    nsICommandParams_SetCStringValue(nsparam, NSSTATE_ATTRIBUTE, fontname);
    do_ns_command(doc, NSCMD_FONTFACE, nsparam);
    nsICommandParams_Release(nsparam);
}

static HRESULT exec_delete(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", doc, in, out);

    do_ns_editor_command(doc->browser, NSCMD_DELETECHARFORWARD);

    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_fontname(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", doc, in, out);

    if(in) {
        char *stra;

        if(V_VT(in) != VT_BSTR) {
            FIXME("Unsupported fontname %s\n", debugstr_variant(out));
            return E_INVALIDARG;
        }

        TRACE("%s\n", debugstr_w(V_BSTR(in)));

        stra = strdupWtoA(V_BSTR(in));
        set_ns_fontname(doc, stra);
        free(stra);

        update_doc(doc->browser->doc, UPDATE_UI);
    }

    if(out) {
        nsICommandParams *nsparam;
        LPWSTR strw;
        char *stra;
        DWORD len;
        nsresult nsres;

        V_VT(out) = VT_BSTR;
        V_BSTR(out) = NULL;

        nsparam = create_nscommand_params();

        nsres = get_ns_command_state(doc->browser, NSCMD_FONTFACE, nsparam);
        if(NS_FAILED(nsres))
            return S_OK;

        nsICommandParams_GetCStringValue(nsparam, NSSTATE_ATTRIBUTE, &stra);
        nsICommandParams_Release(nsparam);

        len = MultiByteToWideChar(CP_ACP, 0, stra, -1, NULL, 0);
        strw = malloc(len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, stra, -1, strw, len);
        nsfree(stra);

        V_BSTR(out) = SysAllocString(strw);
        free(strw);
    }

    return S_OK;
}

static HRESULT exec_forecolor(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", doc, in, out);

    if(in) {
        if(V_VT(in) == VT_I4) {
            nsICommandParams *nsparam = create_nscommand_params();
            char color_str[10];

            sprintf(color_str, "#%02lx%02lx%02lx",
                    V_I4(in)&0xff, (V_I4(in)>>8)&0xff, (V_I4(in)>>16)&0xff);

            nsICommandParams_SetCStringValue(nsparam, NSSTATE_ATTRIBUTE, color_str);
            do_ns_command(doc, NSCMD_FONTCOLOR, nsparam);

            nsICommandParams_Release(nsparam);
        }else {
            FIXME("unsupported forecolor %s\n", debugstr_variant(in));
        }

        update_doc(doc->browser->doc, UPDATE_UI);
    }

    if(out) {
        FIXME("unsupported out\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT exec_fontsize(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%p %p)\n", doc, in, out);

    if(out) {
        WCHAR val[10] = {0};

        get_font_size(doc, val);
        V_VT(out) = VT_I4;
        V_I4(out) = wcstol(val, NULL, 10);
    }

    if(in) {
        switch(V_VT(in)) {
        case VT_I4: {
            WCHAR size[10];
            wsprintfW(size, L"%d", V_I4(in));
            set_font_size(doc, size);
            break;
        }
        case VT_BSTR:
            set_font_size(doc, V_BSTR(in));
            break;
        default:
            FIXME("unsupported fontsize %s\n", debugstr_variant(in));
        }

        update_doc(doc->browser->doc, UPDATE_UI);
    }

    return S_OK;
}

static HRESULT exec_font(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{

    FIXME("(%p)->(%p %p)\n", doc, in, out);
    return E_NOTIMPL;
}

static HRESULT exec_bold(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_BOLD, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_italic(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_ITALIC, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT query_justify(HTMLDocumentNode *doc, OLECMD *cmd)
{
    switch(cmd->cmdID) {
    case IDM_JUSTIFYCENTER:
        TRACE("(%p) IDM_JUSTIFYCENTER\n", doc);
        cmd->cmdf = query_align_status(doc, L"justifycenter");
        break;
    case IDM_JUSTIFYLEFT:
        TRACE("(%p) IDM_JUSTIFYLEFT\n", doc);
        /* FIXME: We should set OLECMDF_LATCHED only if it's set explicitly. */
        if(doc->browser->usermode != EDITMODE || !doc->window->base.outer_window || doc->window->base.outer_window->readystate < READYSTATE_INTERACTIVE)
            cmd->cmdf = OLECMDF_SUPPORTED;
        else
            cmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
        break;
    case IDM_JUSTIFYRIGHT:
        TRACE("(%p) IDM_JUSTIFYRIGHT\n", doc);
        cmd->cmdf = query_align_status(doc, L"justifyright");
        break;
    }

    return S_OK;
}

static HRESULT exec_justifycenter(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    set_ns_align(doc, NSALIGN_CENTER);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_justifyleft(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    set_ns_align(doc, NSALIGN_LEFT);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_justifyright(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    set_ns_align(doc, NSALIGN_RIGHT);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_underline(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_UNDERLINE, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_horizontalline(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_INSERTHR, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_orderlist(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_OL, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_unorderlist(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_UL, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_indent(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_INDENT, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_outdent(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)\n", doc);

    if(in || out)
        FIXME("unsupported args\n");

    do_ns_command(doc, NSCMD_OUTDENT, NULL);
    update_doc(doc->browser->doc, UPDATE_UI);
    return S_OK;
}

static HRESULT exec_composesettings(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    WCHAR *ptr;

    if(out || !in || V_VT(in) != VT_BSTR) {
        WARN("invalid arg %s\n", debugstr_variant(in));
        return E_INVALIDARG;
    }

    TRACE("(%p)->(%lx %s)\n", doc, cmdexecopt, debugstr_w(V_BSTR(in)));

    update_doc(doc->browser->doc, UPDATE_UI);

    ptr = V_BSTR(in);
    if(*ptr == '1')
        exec_bold(doc, cmdexecopt, NULL, NULL);
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    if(*++ptr == '1')
        exec_italic(doc, cmdexecopt, NULL, NULL);
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    if(*++ptr == '1')
        exec_underline(doc, cmdexecopt, NULL, NULL);
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    if(is_digit(*++ptr)) {
        VARIANT v;

        V_VT(&v) = VT_I4;
        V_I4(&v) = *ptr-'0';

        exec_fontsize(doc, cmdexecopt, &v, NULL);
    }
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    if(*++ptr != ',')
        FIXME("set font color\n");
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    if(*++ptr != ',')
        FIXME("set background color\n");
    ptr = wcschr(ptr, ',');
    if(!ptr)
        return S_OK;

    ptr++;
    if(*ptr) {
        VARIANT v;

        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(ptr);

        exec_fontname(doc, cmdexecopt, &v, NULL);

        SysFreeString(V_BSTR(&v));
    }

    return S_OK;
}

HRESULT editor_exec_copy(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    update_doc(doc->browser->doc, UPDATE_UI);
    do_ns_editor_command(doc->browser, NSCMD_COPY);
    return S_OK;
}

HRESULT editor_exec_cut(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    update_doc(doc->browser->doc, UPDATE_UI);
    do_ns_editor_command(doc->browser, NSCMD_CUT);
    return S_OK;
}

HRESULT editor_exec_paste(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    update_doc(doc->browser->doc, UPDATE_UI);
    do_ns_editor_command(doc->browser, NSCMD_PASTE);
    return S_OK;
}

static HRESULT exec_setdirty(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    TRACE("(%p)->(%08lx %p %p)\n", doc, cmdexecopt, in, out);

    if(!in)
        return S_OK;

    if(V_VT(in) == VT_BOOL)
        set_dirty(doc->browser, V_BOOL(in));
    else
        FIXME("unsupported arg %s\n", debugstr_variant(in));

    return S_OK;
}

static HRESULT query_edit_status(HTMLDocumentNode *doc, OLECMD *cmd)
{
    switch(cmd->cmdID) {
    case IDM_DELETE:
        TRACE("CGID_MSHTML: IDM_DELETE\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_FONTNAME:
        TRACE("CGID_MSHTML: IDM_FONTNAME\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_FONTSIZE:
        TRACE("CGID_MSHTML: IDM_FONTSIZE\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_BOLD:
        TRACE("CGID_MSHTML: IDM_BOLD\n");
        cmd->cmdf = query_ns_edit_status(doc, NSCMD_BOLD);
        break;
    case IDM_FORECOLOR:
        TRACE("CGID_MSHTML: IDM_FORECOLOR\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_ITALIC:
        TRACE("CGID_MSHTML: IDM_ITALIC\n");
        cmd->cmdf = query_ns_edit_status(doc, NSCMD_ITALIC);
        break;
    case IDM_UNDERLINE:
        TRACE("CGID_MSHTML: IDM_UNDERLINE\n");
        cmd->cmdf = query_ns_edit_status(doc, NSCMD_UNDERLINE);
        break;
    case IDM_HORIZONTALLINE:
        TRACE("CGID_MSHTML: IDM_HORIZONTALLINE\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_ORDERLIST:
        TRACE("CGID_MSHTML: IDM_ORDERLIST\n");
        cmd->cmdf = query_ns_edit_status(doc, NSCMD_OL);
        break;
    case IDM_UNORDERLIST:
        TRACE("CGID_MSHTML: IDM_HORIZONTALLINE\n");
        cmd->cmdf = query_ns_edit_status(doc, NSCMD_UL);
        break;
    case IDM_INDENT:
        TRACE("CGID_MSHTML: IDM_INDENT\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_OUTDENT:
        TRACE("CGID_MSHTML: IDM_OUTDENT\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    case IDM_HYPERLINK:
        TRACE("CGID_MSHTML: IDM_HYPERLINK\n");
        cmd->cmdf = query_ns_edit_status(doc, NULL);
        break;
    }

    return S_OK;
}

static INT_PTR CALLBACK hyperlink_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            INT def_idx;
            HWND hwndCB = GetDlgItem(hwnd, IDC_TYPE);
            HWND hwndURL = GetDlgItem(hwnd, IDC_URL);
            INT len;

            SetWindowLongPtrW(hwnd, DWLP_USER, lparam);

            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"(other)");
            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"file:");
            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"ftp:");
            def_idx = SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"http:");
            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"https:");
            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"mailto:");
            SendMessageW(hwndCB, CB_INSERTSTRING, -1, (LPARAM)L"news:");
            SendMessageW(hwndCB, CB_SETCURSEL, def_idx, 0);

            /* force the updating of the URL edit box */
            SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_TYPE, CBN_SELCHANGE), (LPARAM)hwndCB);

            SetFocus(hwndURL);
            len = SendMessageW(hwndURL, WM_GETTEXTLENGTH, 0, 0);
            SendMessageW(hwndURL, EM_SETSEL, 0, len);

            return FALSE;
        }
        case WM_COMMAND:
            switch (wparam)
            {
                case MAKEWPARAM(IDCANCEL, BN_CLICKED):
                    EndDialog(hwnd, wparam);
                    return TRUE;
                case MAKEWPARAM(IDOK, BN_CLICKED):
                {
                    BSTR *url = (BSTR *)GetWindowLongPtrW(hwnd, DWLP_USER);
                    HWND hwndURL = GetDlgItem(hwnd, IDC_URL);
                    INT len = GetWindowTextLengthW(hwndURL);
                    *url = SysAllocStringLen(NULL, len + 1);
                    GetWindowTextW(hwndURL, *url, len + 1);
                    EndDialog(hwnd, wparam);
                    return TRUE;
                }
                case MAKEWPARAM(IDC_TYPE, CBN_SELCHANGE):
                {
                    HWND hwndURL = GetDlgItem(hwnd, IDC_URL);
                    INT item;
                    INT len;
                    LPWSTR type;
                    LPWSTR url;
                    LPWSTR p;
                    static const WCHAR wszSlashSlash[] = {'/','/'};

                    /* get string of currently selected hyperlink type */
                    item = SendMessageW((HWND)lparam, CB_GETCURSEL, 0, 0);
                    len = SendMessageW((HWND)lparam, CB_GETLBTEXTLEN, item, 0);
                    type = malloc((len + 1) * sizeof(WCHAR));
                    SendMessageW((HWND)lparam, CB_GETLBTEXT, item, (LPARAM)type);

                    if (!wcscmp(type, L"(other)"))
                        *type = '\0';

                    /* get current URL */
                    len = GetWindowTextLengthW(hwndURL);
                    url = malloc((len + lstrlenW(type) + 3) * sizeof(WCHAR));
                    GetWindowTextW(hwndURL, url, len + 1);

                    /* strip off old protocol */
                    p = wcschr(url, ':');
                    if (p && p[1] == '/' && p[2] == '/')
                        p += 3;
                    if (!p) p = url;
                    memmove(url + (*type != '\0' ? lstrlenW(type) + 2 : 0), p, (len + 1 - (p - url)) * sizeof(WCHAR));

                    /* add new protocol */
                    if (*type != '\0')
                    {
                        memcpy(url, type, (lstrlenW(type) + 1) * sizeof(WCHAR));
                        if (wcscmp(type, L"mailto:") && wcscmp(type, L"news:"))
                            memcpy(url + lstrlenW(type), wszSlashSlash, sizeof(wszSlashSlash));
                    }

                    SetWindowTextW(hwndURL, url);

                    free(url);
                    free(type);
                    return TRUE;
                }
            }
            return FALSE;
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        default:
            return FALSE;
    }
}

static HRESULT exec_hyperlink(HTMLDocumentNode *doc, DWORD cmdexecopt, VARIANT *in, VARIANT *out)
{
    nsAString href_str, ns_url;
    nsIHTMLEditor *html_editor;
    nsIDOMElement *anchor_elem;
    cpp_bool insert_link_at_caret;
    nsISelection *nsselection;
    BSTR url = NULL;
    INT ret;
    HRESULT hres = E_FAIL;

    TRACE("%p, 0x%lx, %p, %p\n", doc, cmdexecopt, in, out);

    if (cmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER)
    {
        if (!in || V_VT(in) != VT_BSTR)
        {
            WARN("invalid arg\n");
            return E_INVALIDARG;
        }
        url = V_BSTR(in);
    }
    else
    {
        ret = DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_HYPERLINK), NULL /* FIXME */, hyperlink_dlgproc, (LPARAM)&url);
        if (ret != IDOK)
            return OLECMDERR_E_CANCELED;
    }

    if(!doc->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsselection = get_ns_selection(doc);
    if (!nsselection)
        return E_FAIL;

    /* create an element for the link */
    create_nselem(doc, L"a", &anchor_elem);

    nsAString_InitDepend(&href_str, L"href");
    nsAString_InitDepend(&ns_url, url);
    nsIDOMElement_SetAttribute(anchor_elem, &href_str, &ns_url);
    nsAString_Finish(&href_str);

    nsISelection_GetIsCollapsed(nsselection, &insert_link_at_caret);

    /* create an element with text of URL */
    if (insert_link_at_caret) {
        nsIDOMNode *unused_node;
        nsIDOMText *text_node;

        nsIDOMDocument_CreateTextNode(doc->dom_document, &ns_url, &text_node);

        /* wrap the <a> tags around the text element */
        nsIDOMElement_AppendChild(anchor_elem, (nsIDOMNode*)text_node, &unused_node);
        nsIDOMText_Release(text_node);
        nsIDOMNode_Release(unused_node);
    }

    nsAString_Finish(&ns_url);

    nsIEditor_QueryInterface(doc->browser->editor, &IID_nsIHTMLEditor, (void **)&html_editor);
    if (html_editor) {
        nsresult nsres;

        if (insert_link_at_caret) {
            /* add them to the document at the caret position */
            nsres = nsIHTMLEditor_InsertElementAtSelection(html_editor, anchor_elem, FALSE);
            nsISelection_SelectAllChildren(nsselection, (nsIDOMNode*)anchor_elem);
        }else /* add them around the selection using the magic provided to us by nsIHTMLEditor */
            nsres = nsIHTMLEditor_InsertLinkAroundSelection(html_editor, anchor_elem);

        nsIHTMLEditor_Release(html_editor);
        hres = NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
    }

    nsISelection_Release(nsselection);
    nsIDOMElement_Release(anchor_elem);

    if (cmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER)
        SysFreeString(url);

    TRACE("-- 0x%08lx\n", hres);
    return hres;
}

const cmdtable_t editmode_cmds[] = {
    {IDM_DELETE,          query_edit_status,    exec_delete},
    {IDM_FONTNAME,        query_edit_status,    exec_fontname},
    {IDM_FONTSIZE,        query_edit_status,    exec_fontsize},
    {IDM_FORECOLOR,       query_edit_status,    exec_forecolor},
    {IDM_BOLD,            query_edit_status,    exec_bold},
    {IDM_ITALIC,          query_edit_status,    exec_italic},
    {IDM_JUSTIFYCENTER,   query_justify,        exec_justifycenter},
    {IDM_JUSTIFYRIGHT,    query_justify,        exec_justifyright},
    {IDM_JUSTIFYLEFT,     query_justify,        exec_justifyleft},
    {IDM_FONT,            NULL,                 exec_font},
    {IDM_UNDERLINE,       query_edit_status,    exec_underline},
    {IDM_HORIZONTALLINE,  query_edit_status,    exec_horizontalline},
    {IDM_ORDERLIST,       query_edit_status,    exec_orderlist},
    {IDM_UNORDERLIST,     query_edit_status,    exec_unorderlist},
    {IDM_INDENT,          query_edit_status,    exec_indent},
    {IDM_OUTDENT,         query_edit_status,    exec_outdent},
    {IDM_COMPOSESETTINGS, NULL,                 exec_composesettings},
    {IDM_HYPERLINK,       query_edit_status,    exec_hyperlink},
    {IDM_SETDIRTY,        NULL,                 exec_setdirty},
    {0,NULL,NULL}
};

void init_editor(HTMLDocumentNode *doc)
{
    update_doc(doc->browser->doc, UPDATE_UI);

    set_ns_fontname(doc, "Times New Roman");
}

HRESULT browser_is_dirty(GeckoBrowser *browser)
{
    cpp_bool modified;

    if(browser->usermode != EDITMODE || !browser->editor)
        return S_FALSE;

    nsIEditor_GetDocumentModified(browser->editor, &modified);
    return modified ? S_OK : S_FALSE;
}

HRESULT setup_edit_mode(HTMLDocumentObj *doc)
{
    IMoniker *mon;
    HRESULT hres;

    if(doc->nscontainer->usermode == EDITMODE)
        return S_OK;

    doc->nscontainer->usermode = EDITMODE;

    if(doc->window->mon) {
        CLSID clsid = IID_NULL;
        hres = IMoniker_GetClassID(doc->window->mon, &clsid);
        if(SUCCEEDED(hres)) {
            /* We should use IMoniker::Save here */
            FIXME("Use CLSID %s\n", debugstr_guid(&clsid));
        }
    }

    if(doc->frame)
        IOleInPlaceFrame_SetStatusText(doc->frame, NULL);

    doc->window->readystate = READYSTATE_UNINITIALIZED;

    if(doc->client) {
        IOleCommandTarget *cmdtrg;

        hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
        if(SUCCEEDED(hres)) {
            VARIANT var;

            V_VT(&var) = VT_I4;
            V_I4(&var) = 0;
            IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 37, 0, &var, NULL);

            IOleCommandTarget_Release(cmdtrg);
        }
    }

    if(doc->hostui) {
        DOCHOSTUIINFO hostinfo;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(doc->hostui, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%lu %08lx %08lx %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
    }

    update_doc(doc, UPDATE_UI);

    if(doc->window->mon) {
        /* FIXME: We should find nicer way to do this */
        remove_target_tasks(doc->task_magic);

        mon = doc->window->mon;
        IMoniker_AddRef(mon);
    }else {
        hres = CreateURLMoniker(NULL, L"about:blank", &mon);
        if(FAILED(hres)) {
            FIXME("CreateURLMoniker failed: %08lx\n", hres);
            return hres;
        }
    }

    hres = IPersistMoniker_Load(&doc->IPersistMoniker_iface, TRUE, mon, NULL, 0);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return hres;

    if(doc->ui_active) {
        if(doc->ip_window)
            call_set_active_object(doc->ip_window, NULL);
        if(doc->hostui)
            IDocHostUIHandler_HideUI(doc->hostui);
    }

    if(doc->ui_active) {
        RECT rcBorderWidths;

        if(doc->hostui)
            IDocHostUIHandler_ShowUI(doc->hostui, DOCHOSTUITYPE_AUTHOR,
                    &doc->IOleInPlaceActiveObject_iface, &doc->IOleCommandTarget_iface,
                    doc->frame, doc->ip_window);

        if(doc->ip_window)
            call_set_active_object(doc->ip_window, &doc->IOleInPlaceActiveObject_iface);

        SetRectEmpty(&rcBorderWidths);
        if(doc->frame)
            IOleInPlaceFrame_SetBorderSpace(doc->frame, &rcBorderWidths);
    }

    return S_OK;
}
