/*
 * Copyright 2005-2009 Jacek Caban for CodeWeavers
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

#ifndef _MSHTML_PRIVATE_H_
#define _MSHTML_PRIVATE_H_

#ifdef __REACTOS__
#define COBJMACROS
#include <windef.h>
#endif
#include "wingdi.h"
#include "docobj.h"
#include "docobjectservice.h"
#include "comcat.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "hlink.h"
#include "perhist.h"
#include "dispex.h"
#include "objsafe.h"
#include "htiframe.h"
#include "tlogstg.h"
#include "shdeprecated.h"

#include "wine/list.h"
#include "wine/rbtree.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "nsiface.h"

#include "mshtml_private_iface.h"
#include "../jscript/jsdisp.h"

#include <assert.h>

#define NS_ERROR_GENERATE_FAILURE(module,code) \
    ((nsresult) (((UINT32)(1u<<31)) | ((UINT32)(module+0x45)<<16) | ((UINT32)(code))))
#define NS_ERROR_GENERATE_SUCCESS(module,code) \
    ((nsresult) (((UINT32)(module+0x45)<<16) | ((UINT32)(code))))

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_ERROR_OUT_OF_MEMORY    ((nsresult)0x8007000EL)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_INVALID_POINTER  ((nsresult)0x80004003L)
#define NS_ERROR_NULL_POINTER     NS_ERROR_INVALID_POINTER
#define NS_ERROR_NOT_AVAILABLE    ((nsresult)0x80040111L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)
#define NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR ((nsresult)0x80530007)
#define NS_ERROR_DOM_SYNTAX_ERR   ((nsresult)0x8053000c)

#define NS_ERROR_MODULE_NETWORK    6

#define NS_BINDING_ABORTED         NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 2)
#define NS_ERROR_UNKNOWN_PROTOCOL  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 18)
#define NS_SUCCESS_DEFAULT_ACTION  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_NETWORK, 66)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

#define MSHTML_E_INVALID_PROPERTY 0x800a01b6
#define MSHTML_E_INVALID_ACTION   0x800a01bd
#define MSHTML_E_NODOC            0x800a025c
#define MSHTML_E_NOT_FUNC         0x800a138a

typedef struct HTMLWindow HTMLWindow;
typedef struct HTMLInnerWindow HTMLInnerWindow;
typedef struct HTMLOuterWindow HTMLOuterWindow;
typedef struct HTMLDocumentNode HTMLDocumentNode;
typedef struct HTMLDocumentObj HTMLDocumentObj;
typedef struct HTMLFrameBase HTMLFrameBase;
typedef struct GeckoBrowser GeckoBrowser;
typedef struct HTMLAttributeCollection HTMLAttributeCollection;
typedef struct DOMEvent DOMEvent;
typedef struct HTMLDOMNode HTMLDOMNode;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct BSCallback BSCallback;
typedef struct EventTarget EventTarget;
typedef struct ScriptHost ScriptHost;

#define TID_LIST \
    XIID(NULL) \
    XDIID(DispCEventObj) \
    XDIID(DispCPlugins) \
    XDIID(DispDOMChildrenCollection) \
    XDIID(DispDOMCustomEvent) \
    XDIID(DispDOMEvent) \
    XDIID(DispDOMKeyboardEvent) \
    XDIID(DispDOMMessageEvent) \
    XDIID(DispDOMMouseEvent) \
    XDIID(DispDOMProgressEvent) \
    XDIID(DispDOMStorageEvent) \
    XDIID(DispDOMUIEvent) \
    XDIID(DispDOMDocumentType) \
    XDIID(DispHTMLAnchorElement) \
    XDIID(DispHTMLAreaElement) \
    XDIID(DispHTMLAttributeCollection) \
    XDIID(DispHTMLBody) \
    XDIID(DispHTMLButtonElement) \
    XDIID(DispHTMLCommentElement) \
    XDIID(DispHTMLCurrentStyle) \
    XDIID(DispHTMLDocument) \
    XDIID(DispHTMLDOMAttribute) \
    XDIID(DispHTMLDOMImplementation) \
    XDIID(DispHTMLDOMRange) \
    XDIID(DispHTMLDOMTextNode) \
    XDIID(DispHTMLElementCollection) \
    XDIID(DispHTMLEmbed) \
    XDIID(DispHTMLFormElement) \
    XDIID(DispHTMLGenericElement) \
    XDIID(DispHTMLFrameElement) \
    XDIID(DispHTMLHeadElement) \
    XDIID(DispHTMLHtmlElement) \
    XDIID(DispHTMLHistory) \
    XDIID(DispHTMLIFrame) \
    XDIID(DispHTMLImg) \
    XDIID(DispHTMLInputElement) \
    XDIID(DispHTMLLabelElement) \
    XDIID(DispHTMLLinkElement) \
    XDIID(DispHTMLLocation) \
    XDIID(DispHTMLMetaElement) \
    XDIID(DispHTMLNamespaceCollection) \
    XDIID(DispHTMLNavigator) \
    XDIID(DispHTMLObjectElement) \
    XDIID(DispHTMLOptionElement) \
    XDIID(DispHTMLScreen) \
    XDIID(DispHTMLScriptElement) \
    XDIID(DispHTMLSelectElement) \
    XDIID(DispHTMLStyle) \
    XDIID(DispHTMLStyleElement) \
    XDIID(DispHTMLStyleSheet) \
    XDIID(DispHTMLStyleSheetRule) \
    XDIID(DispHTMLStyleSheetRulesCollection) \
    XDIID(DispHTMLStyleSheetsCollection) \
    XDIID(DispHTMLTable) \
    XDIID(DispHTMLTableCell) \
    XDIID(DispHTMLTableRow) \
    XDIID(DispHTMLTextAreaElement) \
    XDIID(DispHTMLTitleElement) \
    XDIID(DispHTMLUnknownElement) \
    XDIID(DispHTMLW3CComputedStyle) \
    XDIID(DispHTMLWindow2) \
    XDIID(DispHTMLXMLHttpRequest) \
    XDIID(DispSVGCircleElement) \
    XDIID(DispSVGSVGElement) \
    XDIID(DispSVGTSpanElement) \
    XDIID(HTMLDocumentEvents) \
    XDIID(HTMLDocumentEvents2) \
    XDIID(HTMLElementEvents2) \
    XIID(IDOMCustomEvent) \
    XIID(IDOMEvent) \
    XIID(IDOMKeyboardEvent) \
    XIID(IDOMMessageEvent) \
    XIID(IDOMMouseEvent) \
    XIID(IDOMProgressEvent) \
    XIID(IDOMStorageEvent) \
    XIID(IDOMUIEvent) \
    XIID(IDOMDocumentType) \
    XIID(IDocumentEvent) \
    XIID(IDocumentRange) \
    XIID(IDocumentSelector) \
    XIID(IElementSelector) \
    XIID(IElementTraversal) \
    XIID(IEventTarget) \
    XIID(IHTMLAnchorElement) \
    XIID(IHTMLAreaElement) \
    XIID(IHTMLAttributeCollection) \
    XIID(IHTMLAttributeCollection2) \
    XIID(IHTMLAttributeCollection3) \
    XIID(IHTMLBodyElement) \
    XIID(IHTMLBodyElement2) \
    XIID(IHTMLButtonElement) \
    XIID(IHTMLCSSStyleDeclaration) \
    XIID(IHTMLCSSStyleDeclaration2) \
    XIID(IHTMLCommentElement) \
    XIID(IHTMLCurrentStyle) \
    XIID(IHTMLCurrentStyle2) \
    XIID(IHTMLCurrentStyle3) \
    XIID(IHTMLCurrentStyle4) \
    XIID(IHTMLDocument2) \
    XIID(IHTMLDocument3) \
    XIID(IHTMLDocument4) \
    XIID(IHTMLDocument5) \
    XIID(IHTMLDocument6) \
    XIID(IHTMLDocument7) \
    XIID(IHTMLDOMAttribute) \
    XIID(IHTMLDOMAttribute2) \
    XIID(IHTMLDOMChildrenCollection) \
    XIID(IHTMLDOMImplementation) \
    XIID(IHTMLDOMImplementation2) \
    XIID(IHTMLDOMNode) \
    XIID(IHTMLDOMNode2) \
    XIID(IHTMLDOMNode3) \
    XIID(IHTMLDOMRange) \
    XIID(IHTMLDOMTextNode) \
    XIID(IHTMLDOMTextNode2) \
    XIID(IHTMLElement) \
    XIID(IHTMLElement2) \
    XIID(IHTMLElement3) \
    XIID(IHTMLElement4) \
    XIID(IHTMLElement6) \
    XIID(IHTMLElement7) \
    XIID(IHTMLElementCollection) \
    XIID(IHTMLEmbedElement) \
    XIID(IHTMLEventObj) \
    XIID(IHTMLEventObj5) \
    XIID(IHTMLFiltersCollection) \
    XIID(IHTMLFormElement) \
    XIID(IHTMLFrameBase) \
    XIID(IHTMLFrameBase2) \
    XIID(IHTMLFrameElement3) \
    XIID(IHTMLGenericElement) \
    XIID(IHTMLHeadElement) \
    XIID(IHTMLHtmlElement) \
    XIID(IHTMLIFrameElement) \
    XIID(IHTMLIFrameElement2) \
    XIID(IHTMLIFrameElement3) \
    XIID(IHTMLImageElementFactory) \
    XIID(IHTMLImgElement) \
    XIID(IHTMLInputElement) \
    XIID(IHTMLInputTextElement2) \
    XIID(IHTMLLabelElement) \
    XIID(IHTMLLinkElement) \
    XIID(IHTMLLocation) \
    XIID(IHTMLMetaElement) \
    XIID(IHTMLMimeTypesCollection) \
    XIID(IHTMLNamespaceCollection) \
    XIID(IHTMLObjectElement) \
    XIID(IHTMLObjectElement2) \
    XIID(IHTMLOptionElement) \
    XIID(IHTMLOptionElementFactory) \
    XIID(IHTMLPerformance) \
    XIID(IHTMLPerformanceNavigation) \
    XIID(IHTMLPerformanceTiming) \
    XIID(IHTMLPluginsCollection) \
    XIID(IHTMLRect) \
    XIID(IHTMLRect2) \
    XIID(IHTMLRectCollection) \
    XIID(IHTMLScreen) \
    XIID(IHTMLScriptElement) \
    XIID(IHTMLSelectElement) \
    XIID(IHTMLSelectionObject) \
    XIID(IHTMLSelectionObject2) \
    XIID(IHTMLStorage) \
    XIID(IHTMLStyle) \
    XIID(IHTMLStyle2) \
    XIID(IHTMLStyle3) \
    XIID(IHTMLStyle4) \
    XIID(IHTMLStyle5) \
    XIID(IHTMLStyle6) \
    XIID(IHTMLStyleElement) \
    XIID(IHTMLStyleElement2) \
    XIID(IHTMLStyleSheet) \
    XIID(IHTMLStyleSheet4) \
    XIID(IHTMLStyleSheetRule) \
    XIID(IHTMLStyleSheetRulesCollection) \
    XIID(IHTMLStyleSheetsCollection) \
    XIID(IHTMLTable) \
    XIID(IHTMLTable2) \
    XIID(IHTMLTable3) \
    XIID(IHTMLTableCell) \
    XIID(IHTMLTableRow) \
    XIID(IHTMLTextAreaElement) \
    XIID(IHTMLTextContainer) \
    XIID(IHTMLTitleElement) \
    XIID(IHTMLTxtRange) \
    XIID(IHTMLUniqueName) \
    XIID(IHTMLWindow2) \
    XIID(IHTMLWindow3) \
    XIID(IHTMLWindow4) \
    XIID(IHTMLWindow5) \
    XIID(IHTMLWindow6) \
    XIID(IHTMLWindow7) \
    XIID(IHTMLXMLHttpRequest) \
    XIID(IHTMLXMLHttpRequest2) \
    XIID(IHTMLXMLHttpRequestFactory) \
    XIID(IOmHistory) \
    XIID(IOmNavigator) \
    XIID(ISVGCircleElement) \
    XIID(ISVGElement) \
    XIID(ISVGSVGElement) \
    XIID(ISVGTSpanElement) \
    XIID(ISVGTextContentElement)

#define PRIVATE_TID_LIST \
    XIID(IWineDOMTokenList) \
    XIID(IWineHTMLCharacterData) \
    XIID(IWineHTMLDOMNodePrivate) \
    XIID(IWineHTMLElementPrivate) \
    XIID(IWineHTMLWindowPrivate) \
    XIID(IWineHTMLWindowCompatPrivate) \
    XIID(IWineCSSProperties) \
    XIID(IWinePageTransitionEvent) \
    XIID(IWineXMLHttpRequestPrivate) \
    XIID(IWineMSHTMLConsole) \
    XIID(IWineMSHTMLMediaQueryList) \
    XIID(IWineMSHTMLMutationObserver)

typedef enum {
#define XIID(iface) iface ## _tid,
#define XDIID(iface) iface ## _tid,
TID_LIST
    LAST_public_tid,
PRIVATE_TID_LIST
#undef XIID
#undef XDIID
    LAST_tid
} tid_t;

typedef enum {
    COMPAT_MODE_INVALID = -1,
    COMPAT_MODE_QUIRKS,
    COMPAT_MODE_IE5,
    COMPAT_MODE_IE7,
    COMPAT_MODE_IE8,
    COMPAT_MODE_IE9,
    COMPAT_MODE_IE10,
    COMPAT_MODE_IE11
} compat_mode_t;

#define COMPAT_MODE_CNT (COMPAT_MODE_IE11+1)
#define COMPAT_MODE_NONE COMPAT_MODE_QUIRKS

typedef struct {
    unsigned document_mode;
    unsigned ie_version;
} compat_mode_info_t;

extern const compat_mode_info_t compat_mode_info[COMPAT_MODE_CNT];

typedef struct dispex_data_t dispex_data_t;
typedef struct dispex_dynamic_data_t dispex_dynamic_data_t;

#define MSHTML_DISPID_CUSTOM_MIN 0x60000000
#define MSHTML_DISPID_CUSTOM_MAX 0x6fffffff
#define MSHTML_CUSTOM_DISPID_CNT (MSHTML_DISPID_CUSTOM_MAX-MSHTML_DISPID_CUSTOM_MIN)

typedef struct DispatchEx DispatchEx;
typedef struct nsCycleCollectionTraversalCallback nsCycleCollectionTraversalCallback;
typedef struct dispex_static_data_t dispex_static_data_t;

typedef struct {
    UINT_PTR x;
} nsCycleCollectingAutoRefCnt;

/*
   dispex is our base IDispatchEx implementation for all mshtml objects, and the vtbl allows
   customizing the behavior depending on the object. Objects have basically 3 types of props:

   - builtin props: These props are implicitly generated from the TypeInfo (disp_tid and iface_tids in dispex_static_data_t).
   - custom props:  These props are specific to an object, they are created using vtbl below (e.g. indexed props in HTMLRectCollection).
   - dynamic props: These props are generally allocated by external code (e.g. 'document.wine = 42' creates 'wine' dynamic prop on document)
*/
typedef struct {
    /* Used to provide object specific interfaces (do not AddRef, just return the iface) */
    void *(*query_interface)(DispatchEx*,REFIID);

    /* Used to implement Cycle Collection callbacks; note that the destructor is not optional!
       Unlike delete_cycle_collectable, unlink is called before the destructor (if available). */
    void (*destructor)(DispatchEx*);
    void (*traverse)(DispatchEx*,nsCycleCollectionTraversalCallback*);
    void (*unlink)(DispatchEx*);

    /* Called on the last release, when the refcount reaches 0 */
    void (*last_release)(DispatchEx*);

    /* Called to get outer interface when it may be different than DispatchEx */
    IWineJSDispatchHost *(*get_outer_iface)(DispatchEx*);

    /* Called when the object wants to handle DISPID_VALUE invocations */
    HRESULT (*value)(DispatchEx*,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);

    /* Used when the object has custom props, and this returns DISPIDs for them */
    HRESULT (*get_dispid)(DispatchEx*,const WCHAR*,DWORD,DISPID*);

    /* Similar to get_dispid, but called only when a dynamic property can't be found */
    HRESULT (*find_dispid)(DispatchEx*,const WCHAR*,DWORD,DISPID*);

    /* Similar to get_dispid, but called before any other lookup */
    HRESULT (*lookup_dispid)(DispatchEx*,const WCHAR*,DWORD,DISPID*);

    /* These are called when the object implements GetMemberName, InvokeEx, DeleteMemberByDispID and GetNextDispID for custom props */
    HRESULT (*invoke)(DispatchEx*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*delete)(DispatchEx*,DISPID);
    HRESULT (*next_dispid)(DispatchEx*,DISPID,DISPID*);
    HRESULT (*get_prop_desc)(DispatchEx*,DISPID,struct property_info*);

    /* Similar to invoke, but allows overriding all dispids */
    HRESULT (*disp_invoke)(DispatchEx*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);

    /* Used by objects that want to delay their compat mode initialization until actually needed */
    HTMLInnerWindow *(*get_script_global)(DispatchEx*,dispex_static_data_t**);

    /* Used by objects that want to populate some dynamic props on initialization */
    HRESULT (*populate_props)(DispatchEx*);

    /* Used by objects that want to return a custom name instead of the one in the dispex static data */
    const char *(*get_name)(DispatchEx*);
} dispex_static_data_vtbl_t;

#define ALL_PROTOTYPES                     \
    X(Attr)                                \
    X(CSSRule)                             \
    X(CSSStyleDeclaration)                 \
    X(CSSStyleRule)                        \
    X(CSSStyleSheet)                       \
    X(CharacterData)                       \
    X(ClientRect)                          \
    X(ClientRectList)                      \
    X(Comment)                             \
    X(Console)                             \
    X(CustomEvent)                         \
    X(DOMImplementation)                   \
    X(DOMTokenList)                        \
    X(Document)                            \
    X(DocumentFragment)                    \
    X(DocumentType)                        \
    X(Element)                             \
    X(Event)                               \
    X(HTMLAnchorElement)                   \
    X(HTMLAreaElement)                     \
    X(HTMLBodyElement)                     \
    X(HTMLButtonElement)                   \
    X(HTMLCollection)                      \
    X(HTMLDocument)                        \
    X(HTMLElement)                         \
    X(HTMLEmbedElement)                    \
    X(HTMLFormElement)                     \
    X(HTMLFrameElement)                    \
    X(HTMLHeadElement)                     \
    X(HTMLHtmlElement)                     \
    X(HTMLIFrameElement)                   \
    X(HTMLImageElement)                    \
    X(HTMLInputElement)                    \
    X(HTMLLabelElement)                    \
    X(HTMLLinkElement)                     \
    X(HTMLMetaElement)                     \
    X(HTMLObjectElement)                   \
    X(HTMLOptionElement)                   \
    X(HTMLScriptElement)                   \
    X(HTMLSelectElement)                   \
    X(HTMLStyleElement)                    \
    X(HTMLTableCellElement)                \
    X(HTMLTableDataCellElement)            \
    X(HTMLTableElement)                    \
    X(HTMLTableRowElement)                 \
    X(HTMLTextAreaElement)                 \
    X(HTMLTitleElement)                    \
    X(HTMLUnknownElement)                  \
    X(History)                             \
    X(KeyboardEvent)                       \
    X(MSCSSProperties)                     \
    X(MSCSSRuleList)                       \
    X(MSCurrentStyleCSSProperties)         \
    X(MSEventObj)                          \
    X(MSNamespaceInfoCollection)           \
    X(MSSelection)                         \
    X(MSStyleCSSProperties)                \
    X(MediaQueryList)                      \
    X(MessageEvent)                        \
    X(MimeTypeArray)                       \
    X(MouseEvent)                          \
    X(MutationObserver)                    \
    X(NamedNodeMap)                        \
    X(Navigator)                           \
    X(Node)                                \
    X(NodeList)                            \
    X(PageTransitionEvent)                 \
    X(Performance)                         \
    X(PerformanceNavigation)               \
    X(PerformanceTiming)                   \
    X(PluginArray)                         \
    X(ProgressEvent)                       \
    X(Range)                               \
    X(SVGCircleElement)                    \
    X(SVGElement)                          \
    X(SVGSVGElement)                       \
    X(SVGTSpanElement)                     \
    X(SVGTextContentElement)               \
    X(SVGTextPositioningElement)           \
    X(Screen)                              \
    X(Storage)                             \
    X(StorageEvent)                        \
    X(StyleSheet)                          \
    X(StyleSheetList)                      \
    X(Text)                                \
    X(TextRange)                           \
    X(UIEvent)                             \
    X(Window)                              \
    X(XMLHttpRequest)

typedef enum {
    PROT_NONE,
#define X(name) PROT_##name,
    ALL_PROTOTYPES
#undef X
    PROT_LAST,
} prototype_id_t;

struct dispex_static_data_t {
    const char *name;
    const dispex_static_data_vtbl_t *vtbl;
    const tid_t disp_tid;
    const tid_t* const iface_tids;
    void (*init_info)(dispex_data_t*,compat_mode_t);
    HRESULT (*init_constructor)(HTMLInnerWindow*,DispatchEx**);
    dispex_data_t *info_cache[COMPAT_MODE_CNT];
    dispex_data_t *prototype_info[COMPAT_MODE_CNT - COMPAT_MODE_IE9];
    dispex_data_t *delayed_init_info;
    prototype_id_t id;
    prototype_id_t prototype_id;
    prototype_id_t constructor_id;
    UINT32 js_flags;
    compat_mode_t min_compat_mode;
    compat_mode_t max_compat_mode;
    char prototype_name[64];
};

#define X(name) extern dispex_static_data_t name ## _dispex;
ALL_PROTOTYPES
#undef X

extern dispex_static_data_t *object_descriptors[PROT_LAST];

typedef HRESULT (*dispex_hook_invoke_t)(DispatchEx*,WORD,DISPPARAMS*,VARIANT*,
                                        EXCEPINFO*,IServiceProvider*);

typedef struct {
    DISPID dispid;
    dispex_hook_invoke_t invoke;
    const WCHAR *name;
} dispex_hook_t;

struct DispatchEx {
    IWineJSDispatchHost IWineJSDispatchHost_iface;

    nsCycleCollectingAutoRefCnt ccref;

    IWineJSDispatch *jsdisp;
    dispex_data_t *info;
    dispex_dynamic_data_t *dynamic_data;
};

#define DISPEX_IDISPATCH_NOUNK_IMPL(prefix, iface_name, dispex)                                \
    static HRESULT WINAPI prefix##_GetTypeInfoCount(iface_name *iface, UINT *count)            \
    {                                                                                          \
        return IWineJSDispatchHost_GetTypeInfoCount(&(dispex).IWineJSDispatchHost_iface, count);       \
    }                                                                                          \
    static HRESULT WINAPI prefix##_GetTypeInfo(iface_name *iface,                              \
            UINT index, LCID lcid, ITypeInfo **ret)                                            \
    {                                                                                          \
        return IWineJSDispatchHost_GetTypeInfo(&(dispex).IWineJSDispatchHost_iface, index, lcid, ret); \
    }                                                                                          \
    static HRESULT WINAPI prefix##_GetIDsOfNames(iface_name *iface, REFIID riid,               \
            LPOLESTR *names, UINT count, LCID lcid, DISPID *dispid)                            \
    {                                                                                          \
        return IWineJSDispatchHost_GetIDsOfNames(&(dispex).IWineJSDispatchHost_iface,                  \
                riid, names, count, lcid, dispid);                                             \
    }                                                                                          \
    static HRESULT WINAPI prefix##_Invoke(iface_name *iface, DISPID dispid, REFIID riid,       \
            LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, UINT *err) \
    {                                                                                          \
        return IWineJSDispatchHost_Invoke(&(dispex).IWineJSDispatchHost_iface, dispid,                 \
                riid, lcid, flags, params, res, ei, err);                                      \
    }

#define DISPEX_IDISPATCH_IMPL(prefix, iface_name, dispex)                                      \
    static HRESULT WINAPI prefix##_QueryInterface(iface_name *iface, REFIID riid, void **ppv)  \
    {                                                                                          \
        return IWineJSDispatchHost_QueryInterface(&(dispex).IWineJSDispatchHost_iface, riid, ppv);     \
    }                                                                                          \
    static ULONG WINAPI prefix##_AddRef(iface_name *iface)                                     \
    {                                                                                          \
        return IWineJSDispatchHost_AddRef(&(dispex).IWineJSDispatchHost_iface);                        \
    }                                                                                          \
    static ULONG WINAPI prefix##_Release(iface_name *iface)                                    \
    {                                                                                          \
        return IWineJSDispatchHost_Release(&(dispex).IWineJSDispatchHost_iface);                       \
    }                                                                                          \
    DISPEX_IDISPATCH_NOUNK_IMPL(prefix, iface_name, dispex)

typedef struct {
    void *vtbl;
    int ref_flags;
    void *callbacks;
} ExternalCycleCollectionParticipant;

typedef struct {
    nsresult (NSAPI *traverse)(void*,void*,nsCycleCollectionTraversalCallback*);
    nsresult (NSAPI *unlink)(void*);
    void (NSAPI *delete_cycle_collectable)(void*);
} CCObjCallback;

DEFINE_GUID(IID_nsXPCOMCycleCollectionParticipant, 0x9674489b,0x1f6f,0x4550,0xa7,0x30, 0xcc,0xae,0xdd,0x10,0x4c,0xf9);

extern nsrefcnt (__cdecl *ccref_incr)(nsCycleCollectingAutoRefCnt*,nsISupports*);
extern nsrefcnt (__cdecl *ccref_decr)(nsCycleCollectingAutoRefCnt*,nsISupports*,ExternalCycleCollectionParticipant*);
extern void (__cdecl *ccref_init)(nsCycleCollectingAutoRefCnt*,nsrefcnt);
extern void (__cdecl *ccp_init)(ExternalCycleCollectionParticipant*,const CCObjCallback*);
extern void (__cdecl *describe_cc_node)(nsCycleCollectingAutoRefCnt*,const char*,nsCycleCollectionTraversalCallback*);
extern void (__cdecl *note_cc_edge)(nsISupports*,const char*,nsCycleCollectionTraversalCallback*);

void init_dispatch(DispatchEx*,dispex_static_data_t*,HTMLInnerWindow*,compat_mode_t);
void init_dispatch_with_owner(DispatchEx*,dispex_static_data_t*,DispatchEx*);
HTMLInnerWindow *get_script_global(DispatchEx*);
void dispex_props_unlink(DispatchEx*);
HRESULT change_type(VARIANT*,VARIANT*,VARTYPE,IServiceProvider*);
HRESULT dispex_get_dprop_ref(DispatchEx*,const WCHAR*,BOOL,VARIANT**);
HRESULT get_dispids(tid_t,DWORD*,DISPID**);
HRESULT remove_attribute(DispatchEx*,DISPID,VARIANT_BOOL*);
HRESULT dispex_get_dynid(DispatchEx*,const WCHAR*,BOOL,DISPID*);
void release_typelib(void);
HRESULT get_class_typeinfo(const CLSID*,ITypeInfo**);
const void *dispex_get_vtbl(DispatchEx*);
void dispex_info_add_interface(dispex_data_t*,tid_t,const dispex_hook_t*);
void dispex_info_add_dispids(dispex_data_t*,tid_t,const DISPID*);
compat_mode_t dispex_compat_mode(DispatchEx*);
HRESULT dispex_to_string(DispatchEx*,BSTR*);
HRESULT dispex_call_builtin(DispatchEx *dispex, DISPID id, DISPPARAMS *dp,
                            VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller);
HRESULT dispex_prop_get(DispatchEx *dispex, DISPID id, LCID lcid, VARIANT *r, EXCEPINFO *ei,
                        IServiceProvider *caller);
HRESULT dispex_prop_put(DispatchEx *dispex, DISPID id, LCID lcid, VARIANT *v, EXCEPINFO *ei,
                        IServiceProvider *caller);
HRESULT dispex_get_chain_builtin_id(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *pid);
HRESULT dispex_get_id(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *pid);
HRESULT dispex_next_id(DispatchEx *dispex, DISPID id, BOOL enum_all_own_props, DISPID *ret);
HRESULT dispex_prop_name(DispatchEx *dispex, DISPID id, BSTR *ret);
HRESULT dispex_define_property(DispatchEx *dispex, const WCHAR *name, DWORD flags, VARIANT *v, DISPID *id);
HRESULT dispex_index_prop_desc(DispatchEx*,DISPID,struct property_info*);
IWineJSDispatchHost *dispex_outer_iface(DispatchEx *dispex);
HRESULT get_constructor(HTMLInnerWindow *script_global, prototype_id_t id, DispatchEx **ret);
HRESULT get_prototype(HTMLInnerWindow *script_global, prototype_id_t id, DispatchEx **ret);

typedef enum {
    DISPEXPROP_CUSTOM,
    DISPEXPROP_DYNAMIC,
    DISPEXPROP_BUILTIN
} dispex_prop_type_t;

dispex_prop_type_t get_dispid_type(DISPID);

typedef enum {
    GLOBAL_SCRIPTVAR,
    GLOBAL_ELEMENTVAR,
    GLOBAL_DISPEXVAR,
    GLOBAL_FRAMEVAR
} global_prop_type_t;

typedef struct {
    global_prop_type_t type;
    BSTR name;
    ScriptHost *script_host;
    DISPID id;
} global_prop_t;

struct EventTarget {
    DispatchEx dispex;
    IEventTarget IEventTarget_iface;
    struct wine_rb_tree handler_map;
};

typedef struct {
    DispatchEx dispex;
    IHTMLOptionElementFactory IHTMLOptionElementFactory_iface;

    HTMLInnerWindow *window;
} HTMLOptionElementFactory;

typedef struct {
    DispatchEx dispex;
    IHTMLImageElementFactory IHTMLImageElementFactory_iface;

    HTMLInnerWindow *window;
} HTMLImageElementFactory;

struct HTMLLocation {
    DispatchEx dispex;
    IHTMLLocation IHTMLLocation_iface;

    HTMLOuterWindow *window;
};

typedef struct {
    DispatchEx dispex;
    IOmHistory IOmHistory_iface;

    HTMLInnerWindow *window;
} OmHistory;

typedef struct nsChannelBSC nsChannelBSC;

struct HTMLWindow {
    IHTMLWindow2       IHTMLWindow2_iface;
    IHTMLWindow3       IHTMLWindow3_iface;
    IHTMLWindow4       IHTMLWindow4_iface;
    IHTMLWindow5       IHTMLWindow5_iface;
    IHTMLWindow6       IHTMLWindow6_iface;
    IHTMLWindow7       IHTMLWindow7_iface;
    IHTMLPrivateWindow IHTMLPrivateWindow_iface;
    IServiceProvider   IServiceProvider_iface;
    ITravelLogClient   ITravelLogClient_iface;
    IObjectIdentity    IObjectIdentity_iface;
    IProvideMultipleClassInfo IProvideMultipleClassInfo_iface;
    IWineHTMLWindowPrivate IWineHTMLWindowPrivate_iface;
    IWineHTMLWindowCompatPrivate IWineHTMLWindowCompatPrivate_iface;

    HTMLInnerWindow *inner_window;
    HTMLOuterWindow *outer_window;
};

struct HTMLOuterWindow {
    HTMLWindow base;
    IEventTarget IEventTarget_iface;
    IWineJSDispatchHost IWineJSDispatchHost_iface;

    nsCycleCollectingAutoRefCnt ccref;
    LONG task_magic;

    nsIDOMWindow *nswindow;
    mozIDOMWindowProxy *window_proxy;
    HTMLOuterWindow *parent;
    HTMLFrameBase *frame_element;

    GeckoBrowser *browser;
    struct list browser_entry;

    READYSTATE readystate;
    unsigned readystate_locked;
    BOOL readystate_pending;

    HTMLInnerWindow *pending_window;
    HTMLLocation *location;
    IMoniker *mon;
    IUri *uri;
    IUri *uri_nofrag;
    BSTR url;
    DWORD load_flags;

    struct list sibling_entry;
    struct wine_rb_entry entry;
};

struct HTMLInnerWindow {
    HTMLWindow base;
    EventTarget event_target;

    HTMLDocumentNode *doc;
    nsIDOMWindow *dom_window;

    struct list children;
    struct list script_hosts;
    struct list documents;
    IWineJScript *jscript;

    IHTMLEventObj *event;

    HTMLImageElementFactory *image_factory;
    HTMLOptionElementFactory *option_factory;
    IHTMLScreen *screen;
    OmHistory *history;
    IOmNavigator *navigator;
    IHTMLStorage *session_storage;
    IHTMLStorage *local_storage;
    IWineMSHTMLConsole *console;

    BOOL performance_initialized;
    VARIANT performance;

    unsigned blocking_depth;
    unsigned parser_callback_cnt;
    struct list script_queue;

    global_prop_t *global_props;
    DWORD global_prop_cnt;
    DWORD global_prop_size;

    LONG task_magic;

    IMoniker *mon;
    nsChannelBSC *bscallback;
    struct list bindings;

    ULONG navigation_type;
    ULONG redirect_count;

    DispatchEx *prototypes[PROT_LAST];
    DispatchEx *constructors[PROT_LAST];

    ULONGLONG navigation_start_time;
    ULONGLONG unload_event_start_time;
    ULONGLONG unload_event_end_time;
    ULONGLONG redirect_time;
    ULONGLONG dns_lookup_time;
    ULONGLONG connect_time;
    ULONGLONG request_time;
    ULONGLONG response_start_time;
    ULONGLONG response_end_time;
    ULONGLONG dom_interactive_time;
    ULONGLONG dom_complete_time;
    ULONGLONG dom_content_loaded_event_start_time;
    ULONGLONG dom_content_loaded_event_end_time;
    ULONGLONG load_event_start_time;
    ULONGLONG load_event_end_time;
    ULONGLONG first_paint_time;
};

typedef enum {
    UNKNOWN_USERMODE,
    BROWSEMODE,
    EDITMODE        
} USERMODE;

typedef struct _cp_static_data_t {
    tid_t tid;
    void (*on_advise)(IUnknown*,struct _cp_static_data_t*);
    BOOL pass_event_arg;
    DWORD id_cnt;
    DISPID *ids;
} cp_static_data_t;

typedef struct {
    const IID *riid;
    cp_static_data_t *desc;
} cpc_entry_t;

typedef struct ConnectionPointContainer {
    IConnectionPointContainer IConnectionPointContainer_iface;

    ConnectionPoint *cps;
    const cpc_entry_t *cp_entries;
    IUnknown *outer;
    struct ConnectionPointContainer *forward_container;
} ConnectionPointContainer;

struct  ConnectionPoint {
    IConnectionPoint IConnectionPoint_iface;

    ConnectionPointContainer *container;

    union {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;

    const IID *iid;
    cp_static_data_t *data;
};

struct HTMLDocumentObj {
    IUnknown IUnknown_inner;
    IDispatchEx IDispatchEx_iface;
    ICustomDoc ICustomDoc_iface;
    IHTMLDocument2 IHTMLDocument2_iface;
    IHTMLDocument3 IHTMLDocument3_iface;
    IHTMLDocument4 IHTMLDocument4_iface;
    IHTMLDocument5 IHTMLDocument5_iface;
    IHTMLDocument6 IHTMLDocument6_iface;
    IHTMLDocument7 IHTMLDocument7_iface;
    IDocumentSelector IDocumentSelector_iface;
    IDocumentEvent IDocumentEvent_iface;
    ISupportErrorInfo ISupportErrorInfo_iface;
    IProvideMultipleClassInfo IProvideMultipleClassInfo_iface;
    IMarkupServices IMarkupServices_iface;
    IMarkupContainer IMarkupContainer_iface;
    IDisplayServices IDisplayServices_iface;
    IDocumentRange IDocumentRange_iface;
    IOleDocumentView IOleDocumentView_iface;
    IViewObjectEx IViewObjectEx_iface;
    IPersistMoniker IPersistMoniker_iface;
    IPersistFile IPersistFile_iface;
    IMonikerProp IMonikerProp_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IPersistHistory IPersistHistory_iface;
    IHlinkTarget IHlinkTarget_iface;
    IOleCommandTarget IOleCommandTarget_iface;
    IOleObject IOleObject_iface;
    IOleDocument IOleDocument_iface;
    IOleControl IOleControl_iface;
    IOleInPlaceActiveObject IOleInPlaceActiveObject_iface;
    IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless_iface;
    IObjectWithSite IObjectWithSite_iface;
    IOleContainer IOleContainer_iface;
    IObjectSafety IObjectSafety_iface;
    IServiceProvider IServiceProvider_iface;
    ITargetContainer ITargetContainer_iface;
    IEventTarget IEventTarget_iface;

    IWindowForBindingUI IWindowForBindingUI_iface;

    LONG ref;

    IUnknown *outer_unk;
    HTMLOuterWindow *window;
    GeckoBrowser *nscontainer;
    HTMLDocumentNode *doc_node;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleCommandTarget *client_cmdtrg;
    BOOL custom_hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *ip_window;
    IAdviseSink *view_sink;
    IDocObjectService *doc_object_service;
    IUnknown *webbrowser;
    ITravelLog *travel_log;
    IUnknown *browser_service;
    IOleAdviseHolder *advise_holder;

    ConnectionPointContainer cp_container;
    DOCHOSTUIINFO hostinfo;

    IOleUndoManager *undomgr;
    IHTMLEditServices *editsvcs;

    HWND hwnd;
    HWND tooltips_hwnd;

    BOOL is_mhtml;
    BOOL request_uiactivate;
    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL hostui_setup;
    BOOL container_locked;
    BOOL focus;
    BOOL has_popup;
    INT download_state;

    LPWSTR mime;

    DWORD update;
    LONG task_magic;
    SIZEL extent;
};

typedef struct nsWeakReference nsWeakReference;


typedef enum {
    SCRIPTMODE_GECKO,
    SCRIPTMODE_ACTIVESCRIPT
} SCRIPTMODE;

struct GeckoBrowser {
    nsIWebBrowserChrome      nsIWebBrowserChrome_iface;
    nsIContextMenuListener   nsIContextMenuListener_iface;
    nsIURIContentListener    nsIURIContentListener_iface;
    nsIEmbeddingSiteWindow   nsIEmbeddingSiteWindow_iface;
    nsITooltipListener       nsITooltipListener_iface;
    nsIInterfaceRequestor    nsIInterfaceRequestor_iface;
    nsISupportsWeakReference nsISupportsWeakReference_iface;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserFocus *focus;

    HTMLOuterWindow *content_window;

    nsIEditor *editor;
    nsIController *editor_controller;

    LONG ref;

    nsWeakReference *weak_reference;

    HTMLDocumentObj *doc;

    nsIURIContentListener *content_listener;

    HWND hwnd;
    SCRIPTMODE script_mode;
    USERMODE usermode;

    struct list document_nodes;
    struct list outer_windows;
};

typedef struct {
    const CLSID *clsid;
    const cpc_entry_t *cpc_entries;
    HRESULT (*clone)(HTMLDOMNode*,nsIDOMNode*,HTMLDOMNode**);
    HRESULT (*get_attr_col)(HTMLDOMNode*,HTMLAttributeCollection**);
    EventTarget *(*get_event_prop_target)(HTMLDOMNode*,int);
    HRESULT (*put_disabled)(HTMLDOMNode*,VARIANT_BOOL);
    HRESULT (*get_disabled)(HTMLDOMNode*,VARIANT_BOOL*);
    HRESULT (*get_document)(HTMLDOMNode*,IDispatch**);
    HRESULT (*get_readystate)(HTMLDOMNode*,BSTR*);
    HRESULT (*bind_to_tree)(HTMLDOMNode*);
    BOOL (*is_text_edit)(HTMLDOMNode*);
    BOOL (*is_settable)(HTMLDOMNode*,DISPID);
} NodeImplVtbl;

struct HTMLDOMNode {
    EventTarget   event_target;
    IHTMLDOMNode  IHTMLDOMNode_iface;
    IHTMLDOMNode2 IHTMLDOMNode2_iface;
    IHTMLDOMNode3 IHTMLDOMNode3_iface;
    IWineHTMLDOMNodePrivate IWineHTMLDOMNodePrivate_iface;
    const NodeImplVtbl *vtbl;

    nsIDOMNode *nsnode;
    HTMLDocumentNode *doc;
};

HTMLDOMNode *unsafe_impl_from_IHTMLDOMNode(IHTMLDOMNode*);

static inline void node_addref(HTMLDOMNode *node)
{
    IHTMLDOMNode_AddRef(&node->IHTMLDOMNode_iface);
}

static inline void node_release(HTMLDOMNode *node)
{
    IHTMLDOMNode_Release(&node->IHTMLDOMNode_iface);
}

typedef struct {
    HTMLDOMNode node;
    ConnectionPointContainer cp_container;

    IHTMLElement  IHTMLElement_iface;
    IHTMLElement2 IHTMLElement2_iface;
    IHTMLElement3 IHTMLElement3_iface;
    IHTMLElement4 IHTMLElement4_iface;
    IHTMLElement6 IHTMLElement6_iface;
    IHTMLElement7 IHTMLElement7_iface;
    IHTMLUniqueName IHTMLUniqueName_iface;
    IElementSelector IElementSelector_iface;
    IElementTraversal IElementTraversal_iface;
    IProvideMultipleClassInfo IProvideMultipleClassInfo_iface;
    IWineHTMLElementPrivate IWineHTMLElementPrivate_iface;

    nsIDOMElement *dom_element;       /* NULL for legacy comments represented as HTML elements */
    nsIDOMHTMLElement *html_element;  /* NULL for non-HTML elements (like SVG elements) */
    HTMLStyle *style;
    HTMLStyle *runtime_style;
    HTMLAttributeCollection *attrs;
    WCHAR *filter;
    unsigned unique_id;
} HTMLElement;

extern cp_static_data_t HTMLElementEvents2_data;
#define HTMLELEMENT_CPC {&DIID_HTMLElementEvents2, &HTMLElementEvents2_data}
extern const cpc_entry_t HTMLElement_cpc[];

struct HTMLFrameBase {
    HTMLElement element;

    IHTMLFrameBase  IHTMLFrameBase_iface;
    IHTMLFrameBase2 IHTMLFrameBase2_iface;

    HTMLOuterWindow *content_window;

    nsIDOMHTMLFrameElement *nsframe;
    nsIDOMHTMLIFrameElement *nsiframe;
};

typedef struct nsDocumentEventListener nsDocumentEventListener;

struct HTMLDocumentNode {
    HTMLDOMNode node;

    IHTMLDocument2               IHTMLDocument2_iface;
    IHTMLDocument3               IHTMLDocument3_iface;
    IHTMLDocument4               IHTMLDocument4_iface;
    IHTMLDocument5               IHTMLDocument5_iface;
    IHTMLDocument6               IHTMLDocument6_iface;
    IHTMLDocument7               IHTMLDocument7_iface;
    IDocumentSelector            IDocumentSelector_iface;
    IDocumentEvent               IDocumentEvent_iface;
    ISupportErrorInfo            ISupportErrorInfo_iface;
    IProvideMultipleClassInfo    IProvideMultipleClassInfo_iface;
    IMarkupServices              IMarkupServices_iface;
    IMarkupContainer             IMarkupContainer_iface;
    IDisplayServices             IDisplayServices_iface;
    IDocumentRange               IDocumentRange_iface;
    IPersistMoniker              IPersistMoniker_iface;
    IPersistFile                 IPersistFile_iface;
    IMonikerProp                 IMonikerProp_iface;
    IPersistStreamInit           IPersistStreamInit_iface;
    IPersistHistory              IPersistHistory_iface;
    IHlinkTarget                 IHlinkTarget_iface;
    IOleCommandTarget            IOleCommandTarget_iface;
    IOleObject                   IOleObject_iface;
    IOleDocument                 IOleDocument_iface;
    IOleControl                  IOleControl_iface;
    IOleInPlaceActiveObject      IOleInPlaceActiveObject_iface;
    IOleInPlaceObjectWindowless  IOleInPlaceObjectWindowless_iface;
    IObjectWithSite              IObjectWithSite_iface;
    IOleContainer                IOleContainer_iface;
    IObjectSafety                IObjectSafety_iface;
    IServiceProvider             IServiceProvider_iface;
    IInternetHostSecurityManager IInternetHostSecurityManager_iface;

    nsIDocumentObserver          nsIDocumentObserver_iface;
    ConnectionPointContainer cp_container;
    HTMLInnerWindow *window;
    HTMLDocumentObj *doc_obj;

    GeckoBrowser *browser;
    struct list browser_entry;

    HTMLInnerWindow *script_global;
    struct list script_global_entry;

    compat_mode_t document_mode;
    BOOL document_mode_locked;

    nsIDOMDocument *dom_document;
    nsIDOMHTMLDocument *html_document;
    unsigned int emulate_mode : 1;
    unsigned int content_ready : 1;
    unsigned int unload_sent : 1;

    IHTMLDOMImplementation *dom_implementation;
    IHTMLNamespaceCollection *namespaces;

    ICatInformation *catmgr;
    nsDocumentEventListener *nsevent_listener;
    BOOL *event_vector;

    WCHAR **elem_vars;
    unsigned elem_vars_size;
    unsigned elem_vars_cnt;

    BOOL skip_mutation_notif;

    UINT charset;

    unsigned unique_id;

    struct list selection_list;
    struct list range_list;
    struct list plugin_hosts;
};

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);
HRESULT MHTMLDocument_Create(IUnknown*,REFIID,void**);
HRESULT HTMLLoadOptions_Create(IUnknown*,REFIID,void**);
HRESULT create_document_node(nsIDOMDocument*,GeckoBrowser*,HTMLInnerWindow*,HTMLInnerWindow*,
                             compat_mode_t,HTMLDocumentNode**);
HRESULT create_doctype_node(HTMLDocumentNode*,nsIDOMNode*,HTMLDOMNode**);

HRESULT create_outer_window(GeckoBrowser*,mozIDOMWindowProxy*,HTMLOuterWindow*,HTMLOuterWindow**);
HRESULT update_window_doc(HTMLInnerWindow*);
HTMLOuterWindow *mozwindow_to_window(const mozIDOMWindowProxy*);
void get_top_window(HTMLOuterWindow*,HTMLOuterWindow**);
HRESULT HTMLOptionElementFactory_Create(HTMLInnerWindow*,HTMLOptionElementFactory**);
HRESULT HTMLImageElementFactory_Create(HTMLInnerWindow*,HTMLImageElementFactory**);
HRESULT HTMLXMLHttpRequestFactory_Create(HTMLInnerWindow*,DispatchEx**);
HRESULT create_location(HTMLOuterWindow*,HTMLLocation**);
HRESULT create_navigator(HTMLInnerWindow*,IOmNavigator**);
HRESULT create_html_screen(HTMLInnerWindow*,IHTMLScreen**);
HRESULT create_performance(HTMLInnerWindow*,IHTMLPerformance**);
HRESULT create_history(HTMLInnerWindow*,OmHistory**);
HRESULT create_namespace_collection(HTMLDocumentNode*,IHTMLNamespaceCollection**);
HRESULT create_dom_implementation(HTMLDocumentNode*,IHTMLDOMImplementation**);
void detach_dom_implementation(IHTMLDOMImplementation*);
HRESULT create_html_storage(HTMLInnerWindow*,BOOL,IHTMLStorage**);

void HTMLDocument_View_Init(HTMLDocumentObj*);
void HTMLDocumentObj_Persist_Init(HTMLDocumentObj*);
void HTMLDocumentObj_Service_Init(HTMLDocumentObj*);
void HTMLDocumentObj_OleCmd_Init(HTMLDocumentObj*);
void TargetContainer_Init(HTMLDocumentObj*);

HRESULT HTMLWindow_get_prop_desc(DispatchEx *dispex, DISPID id, struct property_info *desc);
HRESULT HTMLWindow_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
                          VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller);

void HTMLDocumentNode_Persist_Init(HTMLDocumentNode*);
void HTMLDocumentNode_Service_Init(HTMLDocumentNode*);
void HTMLDocumentNode_OleCmd_Init(HTMLDocumentNode*);
void HTMLDocumentNode_OleObj_Init(HTMLDocumentNode*);
void HTMLDocumentNode_SecMgr_Init(HTMLDocumentNode*);

HRESULT HTMLCurrentStyle_Create(HTMLElement*,IHTMLCurrentStyle**);

void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*,const cpc_entry_t*);
void ConnectionPointContainer_Destroy(ConnectionPointContainer*);

HRESULT create_gecko_browser(HTMLDocumentObj*,GeckoBrowser**);
void detach_gecko_browser(GeckoBrowser*);

DWORD get_compat_mode_version(compat_mode_t compat_mode);
compat_mode_t lock_document_mode(HTMLDocumentNode*);

void init_mutation(nsIComponentManager*);
void init_document_mutation(HTMLDocumentNode*);
void release_document_mutation(HTMLDocumentNode*);
JSContext *get_context_from_document(nsIDOMDocument*);

void HTMLDocument_LockContainer(HTMLDocumentObj*,BOOL);
void show_context_menu(HTMLDocumentObj*,DWORD,POINT*,IDispatch*);
void notif_focus(HTMLDocumentObj*);

void show_tooltip(HTMLDocumentObj*,DWORD,DWORD,LPCWSTR);
void hide_tooltip(HTMLDocumentObj*);
HRESULT get_client_disp_property(IOleClientSite*,DISPID,VARIANT*);

UINT get_document_charset(HTMLDocumentNode*);

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**);

BOOL load_gecko(void);
void close_gecko(void);
void register_nsservice(nsIComponentRegistrar*,nsIServiceManager*);
void init_nsio(nsIComponentManager*);
void release_nsio(void);
BOOL is_gecko_path(const char*);
void set_viewer_zoom(GeckoBrowser*,float);
float get_viewer_zoom(GeckoBrowser*);

void init_dispex_cc(void);
void init_window_cc(void);

HRESULT nsuri_to_url(LPCWSTR,BOOL,BSTR*);

HRESULT call_disp_func(IDispatch*,DISPPARAMS*,VARIANT*);
void call_property_onchanged(ConnectionPointContainer*,DISPID);
HRESULT call_set_active_object(IOleInPlaceUIWindow*,IOleInPlaceActiveObject*);

void nsfree(void*);
void *nsalloc(size_t) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(nsfree) __WINE_MALLOC;

BOOL nsACString_Init(nsACString *str, const char *data);
void nsACString_InitDepend(nsACString*,const char*);
void nsACString_SetData(nsACString*,const char*);
UINT32 nsACString_GetData(const nsACString*,const char**);
void nsACString_Finish(nsACString*);

BOOL nsAString_Init(nsAString*,const PRUnichar*);
void nsAString_InitDepend(nsAString*,const PRUnichar*);
void nsAString_SetData(nsAString*,const PRUnichar*);
UINT32 nsAString_GetData(const nsAString*,const PRUnichar**);
void nsAString_Finish(nsAString*);

#define NSSTR_IMPLICIT_PX    0x01
#define NSSTR_COLOR          0x02

HRESULT map_nsresult(nsresult);
HRESULT return_nsstr(nsresult,nsAString*,BSTR*);
HRESULT return_nsstr_variant(nsresult,nsAString*,unsigned,VARIANT*);
HRESULT variant_to_nsstr(VARIANT*,BOOL,nsAString*);
HRESULT return_nsform(nsresult,nsIDOMHTMLFormElement*,IHTMLFormElement**);

nsICommandParams *create_nscommand_params(void);
HRESULT nsnode_to_nsstring(nsIDOMNode*,nsAString*);
void setup_editor_controller(GeckoBrowser*);
nsresult get_nsinterface(nsISupports*,REFIID,void**);
nsIWritableVariant *create_nsvariant(void);
nsIXMLHttpRequest *create_nsxhr(nsIDOMWindow *nswindow);
nsresult create_nsfile(const PRUnichar*,nsIFile**);
char *get_nscategory_entry(const char*,const char*);

HRESULT create_pending_window(HTMLOuterWindow*,nsChannelBSC*);
HRESULT start_binding(HTMLInnerWindow*,BSCallback*,IBindCtx*);
HRESULT async_start_doc_binding(HTMLOuterWindow*,HTMLInnerWindow*,DWORD);
void abort_window_bindings(HTMLInnerWindow*);
void set_download_state(HTMLDocumentObj*,int);
void call_docview_84(HTMLDocumentObj*);
HRESULT reload_page(HTMLOuterWindow*);

void set_ready_state(HTMLOuterWindow*,READYSTATE);
HRESULT get_readystate_string(READYSTATE,BSTR*);

HRESULT HTMLSelectionObject_Create(HTMLDocumentNode*,nsISelection*,IHTMLSelectionObject**);
HRESULT HTMLTxtRange_Create(HTMLDocumentNode*,nsIDOMRange*,IHTMLTxtRange**);
HRESULT create_style_sheet(nsIDOMStyleSheet*,DispatchEx*,IHTMLStyleSheet**);
HRESULT create_style_sheet_collection(nsIDOMStyleSheetList*,HTMLDocumentNode*,
                                      IHTMLStyleSheetsCollection**);
HRESULT create_dom_range(nsIDOMRange*,HTMLDocumentNode*,IHTMLDOMRange**);
HRESULT create_markup_pointer(IMarkupPointer**);

void detach_document_node(HTMLDocumentNode*);
void detach_selection(HTMLDocumentNode*);
void detach_ranges(HTMLDocumentNode*);
HRESULT get_node_text(HTMLDOMNode*,BSTR*);
HRESULT replace_node_by_html(nsIDOMDocument*,nsIDOMNode*,const WCHAR*);

HRESULT create_nselem(HTMLDocumentNode*,const WCHAR*,nsIDOMElement**);
HRESULT create_element(HTMLDocumentNode*,const WCHAR*,HTMLElement**);

HRESULT HTMLDOMTextNode_Create(HTMLDocumentNode*,nsIDOMNode*,HTMLDOMNode**);

BOOL variant_to_nscolor(const VARIANT *v, nsAString *nsstr);
HRESULT nscolor_to_str(LPCWSTR color, BSTR *ret);

static inline BOOL is_main_content_window(HTMLOuterWindow *window)
{
    return window->browser && window == window->browser->content_window;
}

struct HTMLAttributeCollection {
    DispatchEx dispex;
    IHTMLAttributeCollection IHTMLAttributeCollection_iface;
    IHTMLAttributeCollection2 IHTMLAttributeCollection2_iface;
    IHTMLAttributeCollection3 IHTMLAttributeCollection3_iface;

    HTMLElement *elem;
    struct list attrs;
};

typedef struct {
    DispatchEx dispex;
    IHTMLDOMAttribute IHTMLDOMAttribute_iface;
    IHTMLDOMAttribute2 IHTMLDOMAttribute2_iface;

    /* value is valid only for detached attributes (when elem == NULL). */
    VARIANT value;
    /* name must be valid for detached attributes */
    WCHAR *name;

    HTMLElement *elem;
    DISPID dispid;
    struct list entry;
} HTMLDOMAttribute;

HTMLDOMAttribute *unsafe_impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute*);

HRESULT HTMLDOMAttribute_Create(const WCHAR*,HTMLElement*,DISPID,HTMLDocumentNode*,HTMLDOMAttribute**);

HRESULT HTMLElement_Create(HTMLDocumentNode*,nsIDOMNode*,BOOL,HTMLElement**);
HRESULT HTMLCommentElement_Create(HTMLDocumentNode*,nsIDOMNode*,HTMLElement**);
HRESULT HTMLAnchorElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLAreaElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLBodyElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLButtonElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLEmbedElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLFormElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLFrameElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLHeadElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLHtmlElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLIFrame_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLStyleElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLImgElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLInputElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLLabelElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLLinkElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLMetaElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLObjectElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLOptionElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLScriptElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLSelectElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLTable_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLTableCell_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLTableRow_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLTextAreaElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLTitleElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);
HRESULT HTMLGenericElement_Create(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**);

HRESULT create_svg_element(HTMLDocumentNode*,nsIDOMSVGElement*,const WCHAR*,HTMLElement**);

void HTMLDOMNode_Init(HTMLDocumentNode*,HTMLDOMNode*,nsIDOMNode*,dispex_static_data_t*);
void HTMLElement_Init(HTMLElement*,HTMLDocumentNode*,nsIDOMElement*,dispex_static_data_t*);

void init_event_target(EventTarget*,dispex_static_data_t*,HTMLInnerWindow*);
void *EventTarget_query_interface(EventTarget*,REFIID);
void EventTarget_init_dispex_info(dispex_data_t*,compat_mode_t);

void *HTMLDOMNode_query_interface(DispatchEx*,REFIID);
void HTMLDOMNode_destructor(DispatchEx*);
void HTMLDOMNode_traverse(DispatchEx*,nsCycleCollectionTraversalCallback*);
void HTMLDOMNode_unlink(DispatchEx*);
void HTMLDOMNode_init_dispex_info(dispex_data_t*,compat_mode_t);

void *HTMLElement_query_interface(DispatchEx*,REFIID);
void HTMLElement_destructor(DispatchEx*);
void HTMLElement_traverse(DispatchEx*,nsCycleCollectionTraversalCallback*);
void HTMLElement_unlink(DispatchEx*);
HRESULT HTMLElement_populate_props(DispatchEx*);
HRESULT HTMLElement_clone(HTMLDOMNode*,nsIDOMNode*,HTMLDOMNode**);
HRESULT HTMLElement_get_attr_col(HTMLDOMNode*,HTMLAttributeCollection**);
void HTMLElement_init_dispex_info(dispex_data_t*,compat_mode_t);

HRESULT get_node(nsIDOMNode*,BOOL,HTMLDOMNode**);
HRESULT get_element(nsIDOMElement*,HTMLElement**);
HRESULT get_document_node(nsIDOMDocument*,HTMLDocumentNode**);

HTMLElement *unsafe_impl_from_IHTMLElement(IHTMLElement*);

HRESULT search_window_props(HTMLInnerWindow*,const WCHAR*,DWORD,DISPID*);
HRESULT get_frame_by_name(HTMLOuterWindow*,const WCHAR*,BOOL,HTMLOuterWindow**);
HRESULT get_doc_elem_by_id(HTMLDocumentNode*,const WCHAR*,HTMLElement**);
HTMLOuterWindow *get_target_window(HTMLOuterWindow*,nsAString*,BOOL*);
HRESULT handle_link_click_event(HTMLElement*,nsAString*,nsAString*,nsIDOMEvent*,BOOL*);

HRESULT WINAPI wrapper_QueryInterface(IUnknown *iface, REFIID riid, void **ppv);
ULONG WINAPI wrapper_AddRef(IUnknown *iface);
ULONG WINAPI wrapper_Release(IUnknown *iface);
extern const void *iface_wrapper_vtbl[];

IHTMLElementCollection *create_all_collection(HTMLDOMNode*,BOOL);
IHTMLElementCollection *create_collection_from_nodelist(nsIDOMNodeList*,DispatchEx*);
IHTMLElementCollection *create_collection_from_htmlcol(nsIDOMHTMLCollection*,DispatchEx*);
HRESULT create_child_collection(nsIDOMNodeList*,DispatchEx*,IHTMLDOMChildrenCollection**);

HRESULT attr_value_to_string(VARIANT*);
HRESULT get_elem_attr_value_by_dispid(HTMLElement*,DISPID,VARIANT*);
HRESULT get_elem_source_index(HTMLElement*,LONG*);

nsresult get_elem_attr_value(nsIDOMElement*,const WCHAR*,nsAString*,const PRUnichar**);
HRESULT elem_string_attr_getter(HTMLElement*,const WCHAR*,BOOL,BSTR*);
HRESULT elem_string_attr_setter(HTMLElement*,const WCHAR*,const WCHAR*);

HRESULT elem_unique_id(unsigned id, BSTR *p);

/* commands */
typedef struct {
    DWORD id;
    HRESULT (*query)(HTMLDocumentNode*,OLECMD*);
    HRESULT (*exec)(HTMLDocumentNode*,DWORD,VARIANT*,VARIANT*);
} cmdtable_t;

extern const cmdtable_t editmode_cmds[];

void do_ns_command(HTMLDocumentNode*,const char*,nsICommandParams*);

/* timer */
#define UPDATE_UI       0x0001
#define UPDATE_TITLE    0x0002

void update_doc(HTMLDocumentObj*,DWORD);
void update_title(HTMLDocumentObj*);
void set_document_navigation(HTMLDocumentObj*,BOOL);

HRESULT do_query_service(IUnknown*,REFGUID,REFIID,void**);

/* editor */
HRESULT setup_edit_mode(HTMLDocumentObj*);
void init_editor(HTMLDocumentNode*);
void handle_edit_event(HTMLDocumentNode*,nsIDOMEvent*);
HRESULT editor_exec_copy(HTMLDocumentNode*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_cut(HTMLDocumentNode*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_paste(HTMLDocumentNode*,DWORD,VARIANT*,VARIANT*);
HRESULT browser_is_dirty(GeckoBrowser*);
void set_dirty(GeckoBrowser*,VARIANT_BOOL);

extern DWORD mshtml_tls;

typedef struct task_t task_t;
typedef void (*task_proc_t)(task_t*);

struct task_t {
    LONG target_magic;
    task_proc_t proc;
    task_proc_t destr;
    struct list entry;
};

typedef struct event_task_t event_task_t;
typedef void (*event_task_proc_t)(event_task_t*);

struct event_task_t {
    LONG target_magic;
    BOOL thread_blocked;
    event_task_proc_t proc;
    event_task_proc_t destr;
    struct list entry;
    HTMLInnerWindow *window;
};

typedef struct {
    task_t header;
    HTMLDocumentObj *doc;
} docobj_task_t;

typedef struct {
    HWND thread_hwnd;
    struct list task_list;
    struct list event_task_list;
    struct list timer_list;
    struct list *pending_xhr_events_tail;
    struct wine_rb_tree session_storage_map;
    void *blocking_xhr;
    unsigned tasks_locked;
    BOOL timer_blocked;
} thread_data_t;

thread_data_t *get_thread_data(BOOL);
HWND get_thread_hwnd(void);
void unblock_tasks_and_timers(thread_data_t*);
int session_storage_map_cmp(const void*,const struct wine_rb_entry*);
void destroy_session_storage(thread_data_t*);

LONG get_task_target_magic(void);
HRESULT push_task(task_t*,task_proc_t,task_proc_t,LONG);
HRESULT push_event_task(event_task_t*,HTMLInnerWindow*,event_task_proc_t,event_task_proc_t,LONG);
void remove_target_tasks(LONG);
ULONGLONG get_time_stamp(void);

enum timer_type {
    TIMER_TIMEOUT,
    TIMER_INTERVAL,
    TIMER_ANIMATION_FRAME,
};

HRESULT set_task_timer(HTMLInnerWindow*,LONG,enum timer_type,IDispatch*,LONG*);
HRESULT clear_task_timer(HTMLInnerWindow*,DWORD);
HRESULT clear_animation_timer(HTMLInnerWindow*,DWORD);

const WCHAR *parse_compat_version(const WCHAR*,compat_mode_t*);

const char *debugstr_mshtml_guid(const GUID*);

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

DEFINE_GUID(CLSID_CMarkup,0x3050f4fb,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);

DEFINE_GUID(CLSID_JScript, 0xf414c260,0x6ac0,0x11cf, 0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58);
DEFINE_GUID(CLSID_VBScript, 0xb54f3741,0x5b07,0x11cf, 0xa4,0xb0,0x00,0xaa,0x00,0x4a,0x55,0xe8);

DEFINE_GUID(IID_UndocumentedScriptIface,0x719c3050,0xf9d3,0x11cf,0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa0);
DEFINE_GUID(IID_IDispatchJS,0x719c3050,0xf9d3,0x11cf,0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa6);

/* memory allocation functions */

static inline WCHAR *strndupW(LPCWSTR str, unsigned len)
{
    LPWSTR ret = NULL;

    if(str) {
        ret = malloc((len + 1) * sizeof(WCHAR));
        if(ret)
        {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
    }

    return ret;
}

static inline WCHAR *strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = malloc(len * sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *strdupWtoA(const WCHAR *str)
{
    char *ret = NULL;

    if(str) {
        DWORD size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        ret = malloc(size);
        if(ret)
            WideCharToMultiByte(CP_ACP, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline WCHAR *strdupUtoW(const char *str)
{
    WCHAR *ret = NULL;

    if(str) {
        size_t len;

        len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        ret = malloc(len * sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_UTF8, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *strdupWtoU(const WCHAR *str)
{
    char *ret = NULL;

    if(str) {
        size_t size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        ret = malloc(size);
        if(ret)
            WideCharToMultiByte(CP_UTF8, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline char *strndupWtoU(const WCHAR *str, unsigned len)
{
    char *ret = NULL;
    DWORD size;

    if(str) {
        size = len ? WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL) : 0;
        ret = malloc(size + 1);
        if(ret) {
            if(len) WideCharToMultiByte(CP_UTF8, 0, str, len, ret, size, NULL, NULL);
            ret[size] = '\0';
        }
    }

    return ret;
}

static inline VARIANT_BOOL variant_bool(BOOL b)
{
    return b ? VARIANT_TRUE : VARIANT_FALSE;
}

static inline BOOL is_digit(WCHAR c)
{
    return '0' <= c && c <= '9';
}

static inline BOOL is_power_of_2(unsigned x)
{
    return !(x & (x - 1));
}

static inline void unlink_ref(void *p)
{
    IUnknown **ref = p;
    if(*ref) {
        IUnknown *unk = *ref;
        *ref = NULL;
        IUnknown_Release(unk);
    }
}

static inline void unlink_variant(VARIANT *v)
{
    if(V_VT(v) == VT_DISPATCH || V_VT(v) == VT_UNKNOWN)
        unlink_ref(&V_UNKNOWN(v));
}

static inline void traverse_variant(VARIANT *v, const char *name, nsCycleCollectionTraversalCallback *cb)
{
    if(V_VT(v) == VT_DISPATCH || V_VT(v) == VT_UNKNOWN)
        note_cc_edge((nsISupports*)V_UNKNOWN(v), name, cb);
}

static inline void block_task_processing(void)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    thread_data->tasks_locked++;
}

static inline void unblock_task_processing(void)
{
    thread_data_t *thread_data = get_thread_data(FALSE);
    if(!--thread_data->tasks_locked)
        unblock_tasks_and_timers(thread_data);
}

#ifdef __i386__
extern void *call_thiscall_func;
#endif

compat_mode_t get_max_compat_mode(IUri*);
UINT cp_from_charset_string(BSTR);
BSTR charset_string_from_cp(UINT);
HRESULT get_mime_type_display_name(const WCHAR*,BSTR*);
HINSTANCE get_shdoclc(void);
void set_statustext(HTMLDocumentObj*,INT,LPCWSTR);
IInternetSecurityManager *get_security_manager(void);

extern HINSTANCE hInst;
void create_console(HTMLInnerWindow *window, IWineMSHTMLConsole **ret);
HRESULT create_media_query_list(HTMLInnerWindow *window, BSTR media_query, IDispatch **ret);

#endif /* _MSHTML_PRIVATE_H_ */
