/*
 * Copyright 2005 Jacek Caban
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_PROMPTSERVICE_CONTRACTID "@mozilla.org/embedcomp/prompt-service;1"
#define NS_TOOLTIPTEXTPROVIDER_CONTRACTID "@mozilla.org/embedcomp/tooltiptextprovider;1"

#define NS_TOOLTIPTEXTPROVIDER_CLASSNAME "nsTooltipTextProvider"

static const nsIID NS_PROMPTSERVICE_CID =
    {0xa2112d6a,0x0e28,0x421f,{0xb4,0x6a,0x25,0xc0,0xb3,0x8,0xcb,0xd0}};
static const nsIID NS_TOOLTIPTEXTPROVIDER_CID =
    {0x0b666e3e,0x569a,0x462c,{0xa7,0xf0,0xb1,0x6b,0xb1,0x5d,0x42,0xff}};

static nsresult NSAPI nsPromptService_QueryInterface(nsIPromptService *iface,
        nsIIDRef riid, void **result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIPromptService, riid)) {
        TRACE("(IID_nsIPromptService %p)\n", result);
        *result = iface;
    }

    if(*result)
        return NS_OK;

    TRACE("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsPromptService_AddRef(nsIPromptService *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsPromptService_Release(nsIPromptService *iface)
{
    return 1;
}

static nsresult NSAPI nsPromptService_Alert(nsIPromptService *iface, mozIDOMWindowProxy *aParent,
        const PRUnichar *aDialogTitle, const PRUnichar *aText)
{
    HTMLOuterWindow *window;
    BSTR text;

    TRACE("(%p %s %s)\n", aParent, debugstr_w(aDialogTitle), debugstr_w(aText));

    window = mozwindow_to_window(aParent);
    if(!window) {
        WARN("Could not find HTMLWindow for mozIDOMWindowProxy %p\n", aParent);
        return NS_ERROR_UNEXPECTED;
    }

    text = SysAllocString(aText);
    IHTMLWindow2_alert(&window->base.IHTMLWindow2_iface, text);
    SysFreeString(text);

    return NS_OK;
}

static nsresult NSAPI nsPromptService_AlertCheck(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, const PRUnichar *aCheckMsg, cpp_bool *aCheckState)
{
    FIXME("(%p %s %s %s %p)\n", aParent, debugstr_w(aDialogTitle), debugstr_w(aText),
          debugstr_w(aCheckMsg), aCheckState);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_Confirm(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle, const PRUnichar *aText,
        cpp_bool *_retval)
{
    FIXME("(%p %s %s %p)\n", aParent, debugstr_w(aDialogTitle), debugstr_w(aText), _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_ConfirmCheck(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, const PRUnichar *aCheckMsg, cpp_bool *aCheckState,
        cpp_bool *_retval)
{
    FIXME("(%p %s %s %s %p %p)\n", aParent, debugstr_w(aDialogTitle), debugstr_w(aText),
        debugstr_w(aCheckMsg), aCheckState, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_ConfirmEx(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, UINT32 aButtonFlags, const PRUnichar *aButton0Title,
        const PRUnichar *aButton1Title, const PRUnichar *aButton2Title,
        const PRUnichar *aCheckMsg, cpp_bool *aCheckState, LONG *_retval)
{
    FIXME("(%p %s %s %08x %s %s %s %s %p %p) hack!\n", aParent, debugstr_w(aDialogTitle),
          debugstr_w(aText), aButtonFlags, debugstr_w(aButton0Title),
          debugstr_w(aButton1Title), debugstr_w(aButton2Title), debugstr_w(aCheckMsg),
          aCheckState, _retval);

    /*
     * FIXME:
     * This is really very very ugly hack!!!
     */

    if(aButton0Title && !memcmp(aButton0Title, L"Continue", sizeof(L"Continue")))
        *_retval = 0;
    else if(aButton1Title && !memcmp(aButton1Title, L"Continue", sizeof(L"Continue")))
        *_retval = 1;
    else if(aButton2Title && !memcmp(aButton2Title, L"Continue", sizeof(L"Continue")))
        *_retval = 2;
    /* else
     *     let's hope that _retval is set to the default value */

    return NS_OK;
}

static nsresult NSAPI nsPromptService_Prompt(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, PRUnichar **aValue, const PRUnichar *aCheckMsg,
        cpp_bool *aCheckState, cpp_bool *_retval)
{
    FIXME("(%p %s %s %p %s %p %p)\n", aParent, debugstr_w(aDialogTitle), debugstr_w(aText),
          aValue, debugstr_w(aCheckMsg), aCheckState, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_PromptUsernameAndPassword(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, PRUnichar **aUsername, PRUnichar **aPassword,
        const PRUnichar *aCheckMsg, cpp_bool *aCheckState, cpp_bool *_retval)
{
    FIXME("(%p %s %s %p %p %s %p %p)\n", aParent, debugstr_w(aDialogTitle),
        debugstr_w(aText), aUsername, aPassword, debugstr_w(aCheckMsg), aCheckState,
        _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_PromptPassword(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, PRUnichar **aPassword, const PRUnichar *aCheckMsg,
        cpp_bool *aCheckState, cpp_bool *_retval)
{
    FIXME("(%p %s %s %p %s %p %p)\n", aParent, debugstr_w(aDialogTitle),
          debugstr_w(aText), aPassword, debugstr_w(aCheckMsg), aCheckState, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsPromptService_Select(nsIPromptService *iface,
        mozIDOMWindowProxy *aParent, const PRUnichar *aDialogTitle,
        const PRUnichar *aText, UINT32 aCount, const PRUnichar **aSelectList,
        LONG *aOutSelection, cpp_bool *_retval)
{
    FIXME("(%p %s %s %d %p %p %p)\n", aParent, debugstr_w(aDialogTitle),
        debugstr_w(aText), aCount, aSelectList, aOutSelection, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIPromptServiceVtbl PromptServiceVtbl = {
    nsPromptService_QueryInterface,
    nsPromptService_AddRef,
    nsPromptService_Release,
    nsPromptService_Alert,
    nsPromptService_AlertCheck,
    nsPromptService_Confirm,
    nsPromptService_ConfirmCheck,
    nsPromptService_ConfirmEx,
    nsPromptService_Prompt,
    nsPromptService_PromptUsernameAndPassword,
    nsPromptService_PromptPassword,
    nsPromptService_Select
};

static nsIPromptService nsPromptService = { &PromptServiceVtbl };

static nsresult NSAPI nsTooltipTextProvider_QueryInterface(nsITooltipTextProvider *iface,
        nsIIDRef riid, void **result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsITooltipTextProvider, riid)) {
        TRACE("(IID_nsITooltipTextProvider %p)\n", result);
        *result = iface;
    }

    if(*result) {
        nsITooltipTextProvider_AddRef(iface);
        return NS_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsTooltipTextProvider_AddRef(nsITooltipTextProvider *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsTooltipTextProvider_Release(nsITooltipTextProvider *iface)
{
    return 1;
}

static nsresult NSAPI nsTooltipTextProvider_GetNodeText(nsITooltipTextProvider *iface,
        nsIDOMNode *aNode, PRUnichar **aText, cpp_bool *_retval)
{
    nsIDOMHTMLElement *nselem;
    nsIDOMNode *node = aNode, *parent;
    nsAString title_str;
    const PRUnichar *title = NULL;
    nsresult nsres;

    TRACE("(%p %p %p)\n", aNode, aText, _retval);

    *aText = NULL;

    nsAString_Init(&title_str, NULL);

    do {
        nsres = nsIDOMNode_QueryInterface(node, &IID_nsIDOMHTMLElement, (void**)&nselem);
        if(NS_SUCCEEDED(nsres)) {
            title = NULL;

            nsIDOMHTMLElement_GetTitle(nselem, &title_str);
            nsIDOMHTMLElement_Release(nselem);

            nsAString_GetData(&title_str, &title);
            if(title && *title) {
                if(node != aNode)
                    nsIDOMNode_Release(node);
                break;
            }
        }

        nsres = nsIDOMNode_GetParentNode(node, &parent);
        if(NS_FAILED(nsres))
            parent = NULL;

        if(node != aNode)
            nsIDOMNode_Release(node);
        node = parent;
    } while(node);

    if(title && *title) {
        int size = (lstrlenW(title)+1)*sizeof(PRUnichar);

        *aText = nsalloc(size);
        memcpy(*aText, title, size);
        TRACE("aText = %s\n", debugstr_w(*aText));

        *_retval = TRUE;
    }else {
        *_retval = FALSE;
    }

    nsAString_Finish(&title_str);

    return NS_OK;
}

static const nsITooltipTextProviderVtbl nsTooltipTextProviderVtbl = {
    nsTooltipTextProvider_QueryInterface,
    nsTooltipTextProvider_AddRef,
    nsTooltipTextProvider_Release,
    nsTooltipTextProvider_GetNodeText
};

static nsITooltipTextProvider nsTooltipTextProvider = { &nsTooltipTextProviderVtbl };

typedef struct {
    nsIFactory nsIFactory_iface;
    nsISupports *service;
} nsServiceFactory;

static inline nsServiceFactory *impl_from_nsIFactory(nsIFactory *iface)
{
    return CONTAINING_RECORD(iface, nsServiceFactory, nsIFactory_iface);
}

static nsresult NSAPI nsServiceFactory_QueryInterface(nsIFactory *iface, nsIIDRef riid,
        void **result)
{
    nsServiceFactory *This = impl_from_nsIFactory(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIFactory_iface;
    }else if(IsEqualGUID(&IID_nsIFactory, riid)) {
        TRACE("(%p)->(IID_nsIFactory %p)\n", This, result);
        *result = &This->nsIFactory_iface;
    }

    if(*result)
        return NS_OK;

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsServiceFactory_AddRef(nsIFactory *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsServiceFactory_Release(nsIFactory *iface)
{
    return 1;
}

static nsresult NSAPI nsServiceFactory_CreateInstance(nsIFactory *iface,
        nsISupports *aOuter, const nsIID *iid, void **result)
{
    nsServiceFactory *This = impl_from_nsIFactory(iface);

    TRACE("(%p)->(%p %s %p)\n", This, aOuter, debugstr_guid(iid), result);

    return nsISupports_QueryInterface(This->service, iid, result);
}

static nsresult NSAPI nsServiceFactory_LockFactory(nsIFactory *iface, cpp_bool lock)
{
    nsServiceFactory *This = impl_from_nsIFactory(iface);
    WARN("(%p)->(%x)\n", This, lock);
    return NS_OK;
}

static const nsIFactoryVtbl nsServiceFactoryVtbl = {
    nsServiceFactory_QueryInterface,
    nsServiceFactory_AddRef,
    nsServiceFactory_Release,
    nsServiceFactory_CreateInstance,
    nsServiceFactory_LockFactory
};

static nsServiceFactory nsPromptServiceFactory = {
    { &nsServiceFactoryVtbl },
    (nsISupports*)&nsPromptService
};

static nsServiceFactory nsTooltipTextFactory = {
    { &nsServiceFactoryVtbl },
    (nsISupports*)&nsTooltipTextProvider
};

void register_nsservice(nsIComponentRegistrar *registrar, nsIServiceManager *service_manager)
{
    nsresult nsres;

    nsres = nsIComponentRegistrar_RegisterFactory(registrar, &NS_PROMPTSERVICE_CID,
            "Prompt Service", NS_PROMPTSERVICE_CONTRACTID, &nsPromptServiceFactory.nsIFactory_iface);
    if(NS_FAILED(nsres))
        ERR("RegisterFactory failed: %08lx\n", nsres);

    nsres = nsIComponentRegistrar_RegisterFactory(registrar, &NS_TOOLTIPTEXTPROVIDER_CID,
            NS_TOOLTIPTEXTPROVIDER_CLASSNAME, NS_TOOLTIPTEXTPROVIDER_CONTRACTID,
            &nsTooltipTextFactory.nsIFactory_iface);
    if(NS_FAILED(nsres))
        ERR("RegisterFactory failed: %08lx\n", nsres);
}
