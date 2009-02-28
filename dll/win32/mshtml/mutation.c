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

#include "mshtml_private.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

enum {
    MUTATION_COMMENT,
    MUTATION_SCRIPT
};

void set_mutation_observer(NSContainer *nscontainer, nsIDOMHTMLDocument *nshtmldoc)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(nshtmldoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineAddObserver(nsdoc, NSDOCOBS(nscontainer));
    nsIDOMNSDocument_Release(nsdoc);
}

void remove_mutation_observer(NSContainer *nscontainer, nsIDOMHTMLDocument *nshtmldoc)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(nshtmldoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineRemoveObserver(nsdoc, NSDOCOBS(nscontainer));
    nsIDOMNSDocument_Release(nsdoc);
}

#define IE_MAJOR_VERSION 7
#define IE_MINOR_VERSION 0

static BOOL handle_insert_comment(HTMLDocument *doc, const PRUnichar *comment)
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
    nsAString_Init(&nsstr, buf);
    heap_free(buf);

    /* FIXME: Find better way to insert HTML to document. */
    nsres = nsIDOMHTMLDocument_Write(doc->nsdoc, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("Write failed: %08x\n", nsres);
        return FALSE;
    }

    return TRUE;
}

static void add_script_runner(NSContainer *This)
{
    nsIDOMNSDocument *nsdoc;
    nsresult nsres;

    nsres = nsIDOMHTMLDocument_QueryInterface(This->doc->nsdoc, &IID_nsIDOMNSDocument, (void**)&nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSDocument: %08x\n", nsres);
        return;
    }

    nsIDOMNSDocument_WineAddScriptRunner(nsdoc, NSRUNNABLE(This));
    nsIDOMNSDocument_Release(nsdoc);
}

#define NSRUNNABLE_THIS(iface) DEFINE_THIS(NSContainer, Runnable, iface)

static nsresult NSAPI nsRunnable_QueryInterface(nsIRunnable *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSRUNNABLE_THIS(iface);

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
    NSContainer *This = NSRUNNABLE_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsRunnable_Release(nsIRunnable *iface)
{
    NSContainer *This = NSRUNNABLE_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static void pop_mutation_queue(NSContainer *nscontainer)
{
    mutation_queue_t *tmp = nscontainer->mutation_queue;

    if(!tmp)
        return;

    nscontainer->mutation_queue = tmp->next;
    if(!tmp->next)
        nscontainer->mutation_queue_tail = NULL;

    nsISupports_Release(tmp->nsiface);
    heap_free(tmp);
}

static nsresult NSAPI nsRunnable_Run(nsIRunnable *iface)
{
    NSContainer *This = NSRUNNABLE_THIS(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    while(This->mutation_queue) {
        switch(This->mutation_queue->type) {
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
                remove_comment = handle_insert_comment(This->doc, comment);
            }

            nsAString_Finish(&comment_str);

            if(remove_comment) {
                nsIDOMNode *nsparent, *tmp;
                nsAString magic_str;

                static const PRUnichar remove_comment_magicW[] =
                    {'#','!','w','i','n','e', 'r','e','m','o','v','e','!','#',0};

                nsAString_Init(&magic_str, remove_comment_magicW);
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

        case MUTATION_SCRIPT: {
            nsIDOMHTMLScriptElement *nsscript;

            nsres = nsISupports_QueryInterface(This->mutation_queue->nsiface, &IID_nsIDOMHTMLScriptElement,
                                               (void**)&nsscript);
            if(NS_FAILED(nsres)) {
                ERR("Could not get nsIDOMHTMLScriptElement: %08x\n", nsres);
                break;
            }

            doc_insert_script(This->doc, nsscript);
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

#define NSDOCOBS_THIS(iface) DEFINE_THIS(NSContainer, DocumentObserver, iface)

static nsresult NSAPI nsDocumentObserver_QueryInterface(nsIDocumentObserver *iface,
        nsIIDRef riid, nsQIResult result)
{
    NSContainer *This = NSDOCOBS_THIS(iface);

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

    nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
    return NS_OK;
}

static nsrefcnt NSAPI nsDocumentObserver_AddRef(nsIDocumentObserver *iface)
{
    NSContainer *This = NSDOCOBS_THIS(iface);
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsDocumentObserver_Release(nsIDocumentObserver *iface)
{
    NSContainer *This = NSDOCOBS_THIS(iface);
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static void NSAPI nsDocumentObserver_CharacterDataWillChange(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
{
}

static void NSAPI nsDocumentObserver_CharacterDataChanged(nsIDocumentObserver *iface,
        nsIDocument *aDocument, nsIContent *aContent, void /*CharacterDataChangeInfo*/ *aInfo)
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

static void push_mutation_queue(NSContainer *nscontainer, DWORD type, nsISupports *nsiface)
{
    mutation_queue_t *elem;

    elem = heap_alloc(sizeof(mutation_queue_t));
    if(!elem)
        return;

    elem->next = NULL;
    elem->type = type;
    elem->nsiface = nsiface;
    nsISupports_AddRef(nsiface);

    if(nscontainer->mutation_queue_tail)
        nscontainer->mutation_queue_tail = nscontainer->mutation_queue_tail->next = elem;
    else
        nscontainer->mutation_queue = nscontainer->mutation_queue_tail = elem;
}

static void NSAPI nsDocumentObserver_BindToDocument(nsIDocumentObserver *iface, nsIDocument *aDocument,
        nsIContent *aContent)
{
    NSContainer *This = NSDOCOBS_THIS(iface);
    nsIDOMComment *nscomment;
    nsIDOMElement *nselem;
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMElement, (void**)&nselem);
    if(NS_SUCCEEDED(nsres)) {
        check_event_attr(This->doc, nselem);
        nsIDOMElement_Release(nselem);
    }

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMComment, (void**)&nscomment);
    if(NS_SUCCEEDED(nsres)) {
        TRACE("comment node\n");

        push_mutation_queue(This, MUTATION_COMMENT, (nsISupports*)nscomment);
        nsIDOMComment_Release(nscomment);
        add_script_runner(This);
    }
}

static void NSAPI nsDocumentObserver_DoneAddingChildren(nsIDocumentObserver *iface, nsIContent *aContent,
        PRBool aHaveNotified)
{
    NSContainer *This = NSDOCOBS_THIS(iface);
    nsIDOMHTMLScriptElement *nsscript;
    nsresult nsres;

    TRACE("(%p)->(%p %x)\n", This, aContent, aHaveNotified);

    nsres = nsISupports_QueryInterface(aContent, &IID_nsIDOMHTMLScriptElement, (void**)&nsscript);
    if(NS_SUCCEEDED(nsres)) {
        push_mutation_queue(This, MUTATION_SCRIPT, (nsISupports*)nsscript);
        nsIDOMHTMLScriptElement_Release(nsscript);
        add_script_runner(This);
    }
}

#undef NSMUTATIONOBS_THIS

static const nsIDocumentObserverVtbl nsDocumentObserverVtbl = {
    nsDocumentObserver_QueryInterface,
    nsDocumentObserver_AddRef,
    nsDocumentObserver_Release,
    nsDocumentObserver_CharacterDataWillChange,
    nsDocumentObserver_CharacterDataChanged,
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

void init_mutation(NSContainer *nscontainer)
{
    nscontainer->lpDocumentObserverVtbl  = &nsDocumentObserverVtbl;
    nscontainer->lpRunnableVtbl          = &nsRunnableVtbl;
}
