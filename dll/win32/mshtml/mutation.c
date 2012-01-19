/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "shlguid.h"

#include "mshtml_private.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

enum {
    MUTATION_BINDTOTREE,
    MUTATION_COMMENT,
    MUTATION_ENDLOAD,
    MUTATION_SCRIPT
};

#define IE_MAJOR_VERSION 7
#define IE_MINOR_VERSION 0

static BOOL handle_insert_comment(HTMLDocumentNode *doc, const PRUnichar *comment)
{
    DWORD len;
    int majorv = 0, minorv = 0;
    const PRUnichar *ptr, *end;
    nsAString nsstr;
    PRUnichar *buf;
    nsresult nsres;

    enum {
        CMP_EQ,
        CMP_LT,
        CMP_LTE,
        CMP_GT,
        CMP_GTE
    } cmpt = CMP_EQ;

    static const PRUnichar endifW[] = {'<','!','[','e','n','d','i','f',']'};

    if(comment[0] != '[' || comment[1] != 'i' || comment[2] != 'f')
        return FALSE;

    ptr = comment+3;
    while(isspaceW(*ptr))
        ptr++;

    if(ptr[0] == 'l' && ptr[1] == 't') {
        ptr += 2;
        if(*ptr == 'e') {
            cmpt = CMP_LTE;
            ptr++;
        }else {
            cmpt = CMP_LT;
        }
    }else if(ptr[0] == 'g' && ptr[1] == 't') {
        ptr += 2;
        if(*ptr == 'e') {
            cmpt = CMP_GTE;
            ptr++;
        }else {
            cmpt = CMP_GT;
        }
    }

    if(!isspaceW(*ptr++))
        return FALSE;
    while(isspaceW(*ptr))
        ptr++;

    if(ptr[0] != 'I' || ptr[1] != 'E')
        return FALSE;

    ptr +=2;
    if(!isspaceW(*ptr++))
        return FALSE;
    while(isspaceW(*ptr))
        ptr++;

    if(!isdigitW(*ptr))
        return FALSE;
    while(isdigitW(*ptr))
        majorv = majorv*10 + (*ptr++ - '0');

    if(*ptr == '.') {
        ptr++;
        if(!isdigitW(*ptr))
            return FALSE;
        while(isdigitW(*ptr))
            minorv = minorv*10 + (*ptr++ - '0');
    }

    while(isspaceW(*ptr))
        ptr++;
    if(ptr[0] != ']' || ptr[1] != '>')
        return FALSE;
    ptr += 2;

    len = strlenW(ptr);
    if(len < sizeof(endifW)/sizeof(WCHAR))
        return FALSE;

    end = ptr + len-sizeof(endifW)/sizeof(WCHAR);
    if(memcmp(end, endifW, sizeof(endifW)))
        return FALSE;

    switch(cmpt) {
    case CMP_EQ:
        if(majorv == IE_MAJOR_VERSION && minorv == IE_MINOR_VERSION)
            break;
        return FALSE;
    case CMP_LT:
        if(majorv > IE_MAJOR_VERSION)
            break;
        if(majorv == IE_MAJOR_VERSION && minorv > IE_MINOR_VERSION)
            break;
        return FALSE;
    case CMP_LTE:
        if(majorv > IE_MAJOR_VERSION)
            break;
        if(majorv == IE_MAJOR_VERSION && minorv >= IE_MINOR_VERSION)
            break;
        return FALSE;
    case CMP_GT:
        if(majorv < IE_MAJOR_VERSION)
            break;
        if(majorv == IE_MAJOR_VERSION && minorv < IE_MINOR_VERSION)
            break;
        return FALSE;
    case CMP_GTE:
        if(majorv < IE_MAJOR_VERSION)
            break;
        if(majorv == IE_MAJOR_VERSION && minorv <= IE_MINOR_VERSION)
            break;
        return FALSE;
    }

    buf = heap_alloc((end-ptr+1)*sizeof(WCHAR));
    if(!buf)
        return FALSE;

    memcpy(buf, ptr, (end-ptr)*sizeof(WCHAR));
    buf[end-ptr] = 0;
    nsAString_InitDepend(&nsstr, buf);

    /* FIXME: Find better way to insert HTML to document. */
    nsres = nsIDOMHTMLDocument_Write(doc->nsdoc, &nsstr);
    nsAString_Finish(&nsstr);
    heap_free(buf);
    if(NS_FAILED(nsres)) {
        ERR("Write failed: %08x\n", nsres);
        return FALSE;
    }

    return TRUE;
}

static void add_script_runner(HTMLDocumentNode *This)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(This->nsdoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineAddScriptRunner(nsdoc, NSRUNNABLE(This));
    nsIDOMNSDocument_Release(nsdoc);
}

#define NSRUNNABLE_THIS(iface) DEFINE_THIS(HTMLDocumentNode, IRunnable, iface)

static nsresult NSAPI nsRunnable_QueryInterface(nsIRunnable *iface,
        nsIIDRef riid, nsQIResult result)
{
    HTMLDocumentNode *This = NSRUNNABLE_THIS(iface);

    if(IsEqualGUID(riid, &IID_nsISupports)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSRUNNABLE(This);
    }else if(IsEqualGUID(riid, &IID_nsIRunnable)) {
        TRACE("(%p)->(IID_nsIRunnable %p)\n", This, result);
        *result = NSRUNNABLE(This);
    }else {
        *result = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsRunnable_AddRef(nsIRunnable *iface)
{
    HTMLDocumentNode *This = NSRUNNABLE_THIS(iface);
    return htmldoc_addref(&This->basedoc);
}

static nsrefcnt NSAPI nsRunnable_Release(nsIRunnable *iface)
{
    HTMLDocumentNode *This = NSRUNNABLE_THIS(iface);
    return htmldoc_release(&This->basedoc);
}

static void push_mutation_queue(HTMLDocumentNode *doc, DWORD type, nsISupports *nsiface)
{
    mutation_queue_t *elem;

    elem = heap_alloc(sizeof(mutation_queue_t));
    if(!elem)
        return;

    elem->next = NULL;
    elem->type = type;
    elem->nsiface = nsiface;
    if(nsiface)
        nsISupports_AddRef(nsiface);

    if(doc->mutation_queue_tail) {
        doc->mutation_queue_tail = doc->mutation_queue_tail->next = elem;
    }else {
        doc->mutation_queue = doc->mutation_queue_tail = elem;
        add_script_runner(doc);
    }
}

static void pop_mutation_queue(HTMLDocumentNode *doc)
{
    mutation_queue_t *tmp = doc->mutation_queue;

    if(!tmp)
        return;

    doc->mutation_queue = tmp->next;
    if(!tmp->next)
        doc->mutation_queue_tail = NULL;

    if(tmp->nsiface)
        nsISupports_Release(tmp->nsiface);
    heap_free(tmp);
}

static void bind_to_tree(HTMLDocumentNode *doc, nsISupports *nsiface)
{
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;

    nsres = nsISupports_QueryInterface(nsiface, &IID_nsIDOMNode, (void**)&nsnode);
    if(NS_FAILED(nsres))
        return;

    node = get_node(doc, nsnode, TRUE);
    nsIDOMNode_Release(nsnode);
    if(!node) {
        ERR("Could not get node\n");
        return;
    }

    if(node->vtbl->bind_to_tree)
        node->vtbl->bind_to_tree(node);
}

/* Calls undocumented 69 cmd of CGID_Explorer */
static void call_explorer_69(HTMLDocumentObj *doc)
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

static void parse_complete_proc(task_t *task)
{
    HTMLDocumentObj *doc = ((docobj_task_t*)task)->doc;

    TRACE("(%p)\n", doc);

    if(doc->usermode == EDITMODE)
        init_editor(&doc->basedoc);

    call_explorer_69(doc);
    if(doc->view_sink)
        IAdviseSink_OnViewChange(doc->view_sink, DVASPECT_CONTENT, -1);
    call_property_onchanged(&doc->basedoc.cp_propnotif, 1005);
    call_explorer_69(doc);

    /* FIXME: IE7 calls EnableModelless(TRUE), EnableModelless(FALSE) and sets interactive state here */

    set_ready_state(doc->basedoc.window, READYSTATE_INTERACTIVE);
}

static void handle_end_load(HTMLDocumentNode *This)
{
    docobj_task_t *task;

    TRACE("\n");

    if(!This->basedoc.doc_obj)
        return;

    if(This != This->basedoc.doc_obj->basedoc.doc_node) {
        set_ready_state(This->basedoc.window, READYSTATE_INTERACTIVE);
        return;
    }

    task = heap_alloc(sizeof(docobj_task_t));
    if(!task)
        return;

    task->doc = This->basedoc.doc_obj;

    /*
     * This should be done in the worker thread that parses HTML,
     * but we don't have such thread (Gecko parses HTML for us).
     */
    push_task(&task->header, &parse_complete_proc, This->basedoc.doc_obj->basedoc.task_magic);
}

static nsresult NSAPI nsRunnable_Run(nsIRunnable *iface)
{
    HTMLDocumentNode *This = NSRUNNABLE_THIS(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    while(This->mutation_queue) {
        switch(This->mutation_queue->type) {
        case MUTATION_BINDTOTREE:
            bind_to_tree(This, This->mutation_queue->nsiface);
            break;

        case MUTATION_COMMENT: {
            nsIDOMComment *nscomment;
            nsAString comment_str;
            BOOL remove_comment = FALSE;

            nsres = nsISupports_QueryInterface(This->mutation_queue->nsiface, &IID_nsIDOMComment, (void**)&nscomment);
            if(NS_FAILED(nsres)) {
                ERR("Could not get nsIDOMComment iface:%08x\n", nsres);
                return NS_OK;
            }

            nsAString_Init(&comment_str, NULL);
            nsres = nsIDOMComment_GetData(nscomment, &comment_str);
            if(NS_SUCCEEDED(nsres)) {
                const PRUnichar *comment;

                nsAString_GetData(&comment_str, &comment);
                remove_comment = handle_insert_comment(This, comment);
            }

            nsAString_Finish(&comment_str);

            if(remove_comment) {
                nsIDOMNode *nsparent, *tmp;
                nsAString magic_str;

                static const PRUnichar remove_comment_magicW[] =
                    {'#','!','w','i','n','e', 'r','e','m','o','v','e','!','#',0};

                nsAString_InitDepend(&magic_str, remove_comment_magicW);
                nsres = nsIDOMComment_SetData(nscomment, &magic_str);
                nsAString_Finish(&magic_str);
                if(NS_FAILED(nsres))
                    ERR("SetData failed: %08x\n", nsres);

                nsIDOMComment_GetParentNode(nscomment, &nsparent);
                if(nsparent) {
                    nsIDOMNode_RemoveChild(nsparent, (nsIDOMNode*)nscomment, &tmp);
                    nsIDOMNode_Release(nsparent);
                    nsIDOMNode_Release(tmp);
                }
            }

            nsIDOMComment_Release(nscomment);
            break;
        }

        case MUTATION_ENDLOAD:
            handle_end_load(This);
            break;

        case MUTATION_SCRIPT: {
            nsIDOMHTMLScriptElement *nsscript;

            nsres = nsISupports_QueryInterface(This->mutation_queue->nsiface, &IID_nsIDOMHTMLScriptElement,
                                               (void**)&nsscript);
            if(NS_FAILED(nsres)) {
                ERR("Could not get nsIDOMHTMLScriptElement: %08x\n", nsres);
                break;
            }

            doc_insert_script(This->basedoc.window, nsscript);
            nsIDOMHTMLScriptElement_Release(nsscript);
            break;
        }

        default:
            ERR("invalid type %d\n", This->mutation_queue->type);
        }

        pop_mutation_queue(This);
    }

    return S_OK;
}

#undef NSRUNNABLE_THIS

static const nsIRunnableVtbl nsRunnableVtbl = {
    nsRunnable_QueryInterface,
    nsRunnable_AddRef,
    nsRunnable_Release,
    nsRunnable_Run
};

#define NSDOCOBS_THIS(iface) DEFINE_THIS(HTMLDocumentNode, IDocumentObserver, iface)

static nsresult NSAPI nsDocumentObserver_QueryInterface(nsIDocumentObserver *iface,
        nsIIDRef riid, nsQIResult result)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = NSDOCOBS(This);
    }else if(IsEqualGUID(&IID_nsIMutationObserver, riid)) {
        TRACE("(%p)->(IID_nsIMutationObserver %p)\n", This, result);
        *result = NSDOCOBS(This);
    }else if(IsEqualGUID(&IID_nsIDocumentObserver, riid)) {
        TRACE("(%p)->(IID_nsIDocumentObserver %p)\n", This, result);
        *result = NSDOCOBS(This);
    }else {
        *result = NULL;
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        return NS_NOINTERFACE;
    }

    htmldoc_addref(&This->basedoc);
    return NS_OK;
}

static nsrefcnt NSAPI nsDocumentObserver_AddRef(nsIDocumentObserver *iface)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);
    return htmldoc_addref(&This->basedoc);
}

static nsrefcnt NSAPI nsDocumentObserver_Release(nsIDocumentObserver *iface)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);
    return htmldoc_release(&This->basedoc);
}

static void NSAPI nsDocumentObserver_CharacterDataWillChange(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
{
}

static void NSAPI nsDocumentObserver_CharacterDataChanged(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
{
}

static void NSAPI nsDocumentObserver_AttributeWillChange(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent, PRInt32 aNameSpaceID, nsIAtom *aAttribute, PRInt32 aModType)
{
}

static void NSAPI nsDocumentObserver_AttributeChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent, PRInt32 aNameSpaceID, nsIAtom *aAttribute, PRInt32 aModType, PRUint32 aStateMask)
{
}

static void NSAPI nsDocumentObserver_ContentAppended(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, PRInt32 aNewIndexInContainer)
{
}

static void NSAPI nsDocumentObserver_ContentInserted(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, nsIContent *aChild, PRInt32 aIndexInContainer)
{
}

static void NSAPI nsDocumentObserver_ContentRemoved(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContainer, nsIContent *aChild, PRInt32 aIndexInContainer)
{
}

static void NSAPI nsDocumentObserver_NodeWillBeDestroyed(nsIDocumentObserver *iface, const nsINode *aNode)
{
}

static void NSAPI nsDocumentObserver_ParentChainChanged(nsIDocumentObserver *iface, nsIContent *aContent)
{
}

static void NSAPI nsDocumentObserver_BeginUpdate(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsUpdateType aUpdateType)
{
}

static void NSAPI nsDocumentObserver_EndUpdate(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsUpdateType aUpdateType)
{
}

static void NSAPI nsDocumentObserver_BeginLoad(nsIDocumentObserver *iface, nsIDocument *aDocument)
{
}

static void NSAPI nsDocumentObserver_EndLoad(nsIDocumentObserver *iface, nsIDocument *aDocument)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);

    TRACE("\n");

    This->content_ready = TRUE;
    push_mutation_queue(This, MUTATION_ENDLOAD, NULL);
}

static void NSAPI nsDocumentObserver_ContentStatesChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent1, nsIContent *aContent2, PRInt32 aStateMask)
{
}

static void NSAPI nsDocumentObserver_StyleSheetAdded(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIStyleSheet *aStyleSheet, PRBool aDocumentSheet)
{
}

static void NSAPI nsDocumentObserver_StyleSheetRemoved(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIStyleSheet *aStyleSheet, PRBool aDocumentSheet)
{
}

static void NSAPI nsDocumentObserver_StyleSheetApplicableStateChanged(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIStyleSheet *aStyleSheet, PRBool aApplicable)
{
}

static void NSAPI nsDocumentObserver_StyleRuleChanged(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIStyleSheet *aStyleSheet, nsIStyleRule *aOldStyleRule, nsIStyleSheet *aNewStyleRule)
{
}

static void NSAPI nsDocumentObserver_StyleRuleAdded(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIStyleSheet *aStyleSheet, nsIStyleRule *aStyleRule)
{
}

static void NSAPI nsDocumentObserver_StyleRuleRemoved(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIStyleSheet *aStyleSheet, nsIStyleRule *aStyleRule)
{
}

static void NSAPI nsDocumentObserver_BindToDocument(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);
    nsIDOMHTMLIFrameElement *nsiframe;
    nsIDOMHTMLFrameElement *nsframe;
    nsIDOMComment *nscomment;
    nsIDOMElement *nselem;
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMElement, (void**)&nselem);
    if(NS_SUCCEEDED(nsres)) {
        check_event_attr(This, nselem);
        nsIDOMElement_Release(nselem);
    }

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMComment, (void**)&nscomment);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("comment node\n");

        push_mutation_queue(This, MUTATION_COMMENT, (nsISupports*)nscomment);
        nsIDOMComment_Release(nscomment);
    }

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMHTMLIFrameElement, (void**)&nsiframe);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("iframe node\n");

        push_mutation_queue(This, MUTATION_BINDTOTREE, (nsISupports*)nsiframe);
        nsIDOMHTMLIFrameElement_Release(nsiframe);
    }

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMHTMLFrameElement, (void**)&nsframe);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("frame node\n");

        push_mutation_queue(This, MUTATION_BINDTOTREE, (nsISupports*)nsframe);
        nsIDOMHTMLFrameElement_Release(nsframe);
    }
}

static void NSAPI nsDocumentObserver_DoneAddingChildren(nsIDocumentObserver *iface, nsIContent *aContent,
        PRBool aHaveNotified)
{
    HTMLDocumentNode *This = NSDOCOBS_THIS(iface);
    nsIDOMHTMLScriptElement *nsscript;
    nsresult nsres;

    TRACE("(%p)->(%p %x)\n", This, aContent, aHaveNotified);

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("script node\n");

        push_mutation_queue(This, MUTATION_SCRIPT, (nsISupports*)nsscript);
        nsIDOMHTMLScriptElement_Release(nsscript);
    }
}

#undef NSMUTATIONOBS_THIS

static const nsIDocumentObserverVtbl nsDocumentObserverVtbl = {
    nsDocumentObserver_QueryInterface,
    nsDocumentObserver_AddRef,
    nsDocumentObserver_Release,
    nsDocumentObserver_CharacterDataWillChange,
    nsDocumentObserver_CharacterDataChanged,
    nsDocumentObserver_AttributeWillChange,
    nsDocumentObserver_AttributeChanged,
    nsDocumentObserver_ContentAppended,
    nsDocumentObserver_ContentInserted,
    nsDocumentObserver_ContentRemoved,
    nsDocumentObserver_NodeWillBeDestroyed,
    nsDocumentObserver_ParentChainChanged,
    nsDocumentObserver_BeginUpdate,
    nsDocumentObserver_EndUpdate,
    nsDocumentObserver_BeginLoad,
    nsDocumentObserver_EndLoad,
    nsDocumentObserver_ContentStatesChanged,
    nsDocumentObserver_StyleSheetAdded,
    nsDocumentObserver_StyleSheetRemoved,
    nsDocumentObserver_StyleSheetApplicableStateChanged,
    nsDocumentObserver_StyleRuleChanged,
    nsDocumentObserver_StyleRuleAdded,
    nsDocumentObserver_StyleRuleRemoved,
    nsDocumentObserver_BindToDocument,
    nsDocumentObserver_DoneAddingChildren
};

void init_mutation(HTMLDocumentNode *doc)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    doc->lpIDocumentObserverVtbl  = &nsDocumentObserverVtbl;
    doc->lpIRunnableVtbl          = &nsRunnableVtbl;

    nsres = nsIDOMHTMLDocument_QueryInterface(doc->nsdoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineAddObserver(nsdoc, NSDOCOBS(doc));
    nsIDOMNSDocument_Release(nsdoc);
}

void release_mutation(HTMLDocumentNode *doc)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(doc->nsdoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineRemoveObserver(nsdoc, NSDOCOBS(doc));
    nsIDOMNSDocument_Release(nsdoc);
}
