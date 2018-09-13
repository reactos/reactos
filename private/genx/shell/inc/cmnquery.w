#ifndef __cmnquery_h
#define __cmnquery_h
#ifndef __cmnquryp_h        ;internal
#define __cmnquryp_h        ;internal

DEFINE_GUID(IID_IQueryFormW, 0x8cfcee30, 0x39bd, 0x11d0, 0xb8, 0xd1, 0x0, 0xa0, 0x24, 0xab, 0x2d, 0xbb);
DEFINE_GUID(IID_IQueryFormA, 0x66c95d82, 0x104e, 0x11d1, 0xb1, 0x31, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);
DEFINE_GUID(IID_IPersistQueryW, 0x1a3114b8, 0xa62e, 0x11d0, 0xa6, 0xc5, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);
DEFINE_GUID(IID_IPersistQueryA, 0x66c95d82, 0x104e, 0x11d1, 0xb1, 0x31, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);

DEFINE_GUID(CLSID_CommonQuery,  0x83bc5ec0, 0x6f2a, 0x11d0, 0xa1, 0xc4, 0x0, 0xaa, 0x00, 0xc1, 0x6e, 0x65);
DEFINE_GUID(IID_ICommonQueryW, 0xab50dec0, 0x6f1d, 0x11d0, 0xa1, 0xc4, 0x0, 0xaa, 0x00, 0xc1, 0x6e, 0x65);
DEFINE_GUID(IID_ICommonQueryA, 0x3399fb0b, 0x18eb, 0x11d1, 0xb1, 0x3e, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);

;begin_internal

DEFINE_GUID(IID_IQueryFrame, 0x7e8c7c20, 0x7c9d, 0x11d0, 0x91, 0x3f, 0x0, 0xaa, 0x00, 0xc1, 0x6e, 0x65);
DEFINE_GUID(IID_IQueryHandler,  0xa60cc73f, 0xe0fc, 0x11d0, 0x97, 0x50, 0x0, 0xa0, 0xc9, 0x06, 0xaf, 0x45);

;end_internal

#ifndef GUID_DEFS_ONLY      ;both

//-----------------------------------------------------------------------------
// IQueryForm
//-----------------------------------------------------------------------------

//
// A query form object is registered under the query handlers CLSID,
// a list is stored in the registry:
//
//  HKCR\CLSID\{CLSID query handler}\Forms
//
// For each form object there are server values which can be defined:
//
//  Flags           = flags for the form object:
//                      QUERYFORM_CHANGESFORMLIST
//                      QUERYFORM_CHANGESOPTFORMLIST
//
//  CLSID           = string containing the CLSID of the InProc server to invoke
//                    to get the IQueryFormObject.
//
//  Forms           = a sub key containing the CLSIDs for the forms registered
//                    by IQueryForm::AddForms (or modified by ::AddPages), if
//                    the flags are 0, then we scan this list looking for a match
//                    for the default form specified.
//

#define QUERYFORM_CHANGESFORMLIST       0x000000001
#define QUERYFORM_CHANGESOPTFORMLIST    0x000000002

//
// Query Forms
// ===========
//  Query forms are registered and have query pages added to them, a form without
//  pages is not displayed.  Each form has a unique CLSID to allow it to be
//  selected by invoking the query dialog.
//

#define CQFF_NOGLOBALPAGES  0x0000001       // = 1 => doesn't have global pages added
#define CQFF_ISOPTIONAL     0x0000002       // = 1 => form is hidden, unless optional forms requested
;begin_internal
#define CQFF_ISNEVERLISTED  0x0000004       // = 1 => form not listed in the form selector
;end_internal

struct _cqform_W;
typedef struct _cqform_W CQFORM_W, * LPCQFORM_W;
typedef HRESULT (CALLBACK *LPCQADDFORMSPROC_W)(LPARAM lParam, LPCQFORM_W pForm);

struct _cqform_A;
typedef struct _cqform_A CQFORM_A, * LPCQFORM_A;
typedef HRESULT (CALLBACK *LPCQADDFORMSPROC_A)(LPARAM lParam, LPCQFORM_A pForm);

struct _cqform_W
{
    DWORD   cbStruct;
    DWORD   dwFlags;
    CLSID   clsid;
    HICON   hIcon;
    LPCWSTR pszTitle;
};

struct _cqform_A
{
    DWORD   cbStruct;
    DWORD   dwFlags;
    CLSID   clsid;
    HICON   hIcon;
    LPCSTR  pszTitle;
};

#ifdef UNICODE
#define CQFORM           CQFORM_W
#define LPCQFORM         LPCQFORM_W
#define LPCQADDFORMSPROC LPCQADDFORMSPROC_W
#else
#define CQFORM           CQFORM_A
#define LPCQFORM         LPCQFORM_A
#define LPCQADDFORMSPROC LPCQADDFORMSPROC_A
#endif

//
// Query Form Pages
// ================
//  When a query form has been registered the caller can then add pages to it,
//  any form can have pages appended.
//

;begin_internal
#define CQPF_ISGLOBAL               0x00000001  // = 1 => this page is global, and added to all forms
;end_internal

struct _cqpage_W;
typedef struct _cqpage_W CQPAGE_W, * LPCQPAGE_W;
typedef HRESULT (CALLBACK *LPCQADDPAGESPROC_W)(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE_W pPage);
typedef HRESULT (CALLBACK *LPCQPAGEPROC_W)(LPCQPAGE_W pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct _cqpage_A;
typedef struct _cqpage_A CQPAGE_A, * LPCQPAGE_A;
typedef HRESULT (CALLBACK *LPCQADDPAGESPROC_A)(LPARAM lParam, REFCLSID clsidForm, LPCQPAGE_A pPage);
typedef HRESULT (CALLBACK *LPCQPAGEPROC_A)(LPCQPAGE_A pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct _cqpage_W
{
    DWORD          cbStruct;
    DWORD          dwFlags;
    LPCQPAGEPROC_W pPageProc;
    HINSTANCE      hInstance;
    INT            idPageName;
    INT            idPageTemplate;
    DLGPROC        pDlgProc;
    LPARAM         lParam;
};

struct _cqpage_A
{
    DWORD          cbStruct;
    DWORD          dwFlags;
    LPCQPAGEPROC_A pPageProc;
    HINSTANCE      hInstance;
    INT            idPageName;
    INT            idPageTemplate;
    DLGPROC        pDlgProc;
    LPARAM         lParam;
};

#ifdef UNICODE
#define CQPAGE           CQPAGE_W
#define LPCQPAGE         LPCQPAGE_W
#define LPCQADDPAGESPROC LPCQADDPAGESPROC_W
#define LPCQPAGEPROC     LPCQPAGEPROC_W
#else
#define CQPAGE           CQPAGE_A
#define LPCQPAGE         LPCQPAGE_A
#define LPCQADDPAGESPROC LPCQADDPAGESPROC_A
#define LPCQPAGEPROC     LPCQPAGEPROC_A
#endif

//
// IQueryForm interfaces
//

#undef  INTERFACE
#define INTERFACE IQueryFormW

DECLARE_INTERFACE_(IQueryFormW, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IQueryForm methods
    STDMETHOD(Initialize)(THIS_ HKEY hkForm) PURE;
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC_W pAddFormsProc, LPARAM lParam) PURE;
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC_W pAddPagesProc, LPARAM lParam) PURE;
};

#undef  INTERFACE
#define INTERFACE IQueryFormA

DECLARE_INTERFACE_(IQueryFormA, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IQueryForm methods
    STDMETHOD(Initialize)(THIS_ HKEY hkForm) PURE;
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC_A pAddFormsProc, LPARAM lParam) PURE;
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC_A pAddPagesProc, LPARAM lParam) PURE;
};

#ifdef UNICODE
#define IQueryForm     IQueryFormW
#define IID_IQueryForm IID_IQueryFormW
#else
#define IQueryForm     IQueryFormA
#define IID_IQueryForm IID_IQueryFormA
#endif

//
// Messages for pages
//

#define CQPM_INITIALIZE             0x00000001
#define CQPM_RELEASE                0x00000002
#define CQPM_ENABLE                 0x00000003 // wParam = TRUE/FALSE (enable, disable), lParam = 0
#define CQPM_GETPARAMETERS          0x00000005 // wParam = 0, lParam = -> receives the LocalAlloc
#define CQPM_CLEARFORM              0x00000006 // wParam, lParam = 0
#define CQPM_PERSIST                0x00000007 // wParam = fRead, lParam -> IPersistQuery
#define CQPM_HELP                   0x00000008 // wParam = 0, lParam -> LPHELPINFO
#define CQPM_SETDEFAULTPARAMETERS   0x00000009 // wParam = 0, lParam -> OPENQUERYWINDOW

#define CQPM_HANDLERSPECIFIC        0x10000000

//-----------------------------------------------------------------------------
// IPersistQuery
//-----------------------------------------------------------------------------

// IPersistQuery interface

#undef  INTERFACE
#define INTERFACE IPersistQueryW

DECLARE_INTERFACE_(IPersistQueryW, IPersist)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IPersist
    STDMETHOD(GetClassID)(THIS_ CLSID* pClassID) PURE;

    // IPersistQuery
    STDMETHOD(WriteString)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPCWSTR pValue) PURE;
    STDMETHOD(ReadString)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPWSTR pBuffer, INT cchBuffer) PURE;
    STDMETHOD(WriteInt)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, INT value) PURE;
    STDMETHOD(ReadInt)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPINT pValue) PURE;
    STDMETHOD(WriteStruct)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct) PURE;
    STDMETHOD(ReadStruct)(THIS_ LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct) PURE;
    STDMETHOD(Clear)(THIS) PURE;
};

#undef  INTERFACE
#define INTERFACE IPersistQueryA

DECLARE_INTERFACE_(IPersistQueryA, IPersist)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IPersist
    STDMETHOD(GetClassID)(THIS_ CLSID* pClassID) PURE;

    // IPersistQuery
    STDMETHOD(WriteString)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPCSTR pValue) PURE;
    STDMETHOD(ReadString)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPSTR pBuffer, INT cchBuffer) PURE;
    STDMETHOD(WriteInt)(THIS_ LPCSTR pSection, LPCSTR pValueName, INT value) PURE;
    STDMETHOD(ReadInt)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPINT pValue) PURE;
    STDMETHOD(WriteStruct)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPVOID pStruct, DWORD cbStruct) PURE;
    STDMETHOD(ReadStruct)(THIS_ LPCSTR pSection, LPCSTR pValueName, LPVOID pStruct, DWORD cbStruct) PURE;
    STDMETHOD(Clear)(THIS) PURE;
};

#ifdef UNICODE
#define IPersistQuery     IPersistQueryW
#define IID_IPersistQuery IID_IPersistQueryW
#else
#define IPersistQuery     IPersistQueryA
#define IID_IPersistQuery IID_IPersistQueryA
#endif


//-----------------------------------------------------------------------------
// ICommonQuery
//-----------------------------------------------------------------------------

#define OQWF_OKCANCEL               0x00000001 // = 1 => Provide OK/Cancel buttons
#define OQWF_DEFAULTFORM            0x00000002 // = 1 => clsidDefaultQueryForm is valid
#define OQWF_SINGLESELECT           0x00000004 // = 1 => view to have single selection (depends on viewer)
#define OQWF_LOADQUERY              0x00000008 // = 1 => use the IPersistQuery to load the given query
#define OQWF_REMOVESCOPES           0x00000010 // = 1 => remove scope picker from dialog
#define OQWF_REMOVEFORMS            0x00000020 // = 1 => remove form picker from dialog
#define OQWF_ISSUEONOPEN            0x00000040 // = 1 => issue query on opening the dialog
#define OQWF_SHOWOPTIONAL           0x00000080 // = 1 => list optional forms by default
;begin_internal
#define OQWF_HIDESEARCHPANE         0x00000100 // = 1 => hide the search pane by on opening
;end_internal
#define OQWF_SAVEQUERYONOK          0x00000200 // = 1 => use the IPersistQuery to write the query on close
#define OQWF_HIDEMENUS              0x00000400 // = 1 => no menu bar displayed
#define OQWF_HIDESEARCHUI           0x00000800 // = 1 => dialog is filter, therefore start, stop, new search etc

#define OQWF_PARAMISPROPERTYBAG     0x80000000 // = 1 => the form parameters ptr is an IPropertyBag (ppbFormParameters)

typedef struct
{
    DWORD           cbStruct;                   // structure size
    DWORD           dwFlags;                    // flags (OQFW_*)
    CLSID           clsidHandler;               // clsid of handler we are using
    LPVOID          pHandlerParameters;         // handler specific structure for initialization
    CLSID           clsidDefaultForm;           // default form to be selected (if OQF_DEFAULTFORM == 1 )
    IPersistQueryW* pPersistQuery;              // IPersistQuery used for loading queries
    union
    {
        void*         pFormParameters;
        IPropertyBag* ppbFormParameters;
    };
} OPENQUERYWINDOW_W, * LPOPENQUERYWINDOW_W;

typedef struct
{
    DWORD           cbStruct;                   // structure size
    DWORD           dwFlags;                    // flags (OQFW_*)
    CLSID           clsidHandler;               // clsid of handler we are using
    LPVOID          pHandlerParameters;         // handler specific structure for initialization
    CLSID           clsidDefaultForm;           // default form to be selected (if OQF_DEFAULTFORM == 1 )
    IPersistQueryA* pPersistQuery;              // IPersistQuery used for loading queries
    union
    {
        void*         pFormParameters;
        IPropertyBag* ppbFormParameters;
    };
} OPENQUERYWINDOW_A, * LPOPENQUERYWINDOW_A;

#ifdef UNICODE
#define OPENQUERYWINDOW OPENQUERYWINDOW_W
#define LPOPENQUERYWINDOW LPOPENQUERYWINDOW_W
#else
#define OPENQUERYWINDOW OPENQUERYWINDOW_A
#define LPOPENQUERYWINDOW LPOPENQUERYWINDOW_A
#endif


// ICommonQuery

#undef  INTERFACE
#define INTERFACE ICommonQueryW

DECLARE_INTERFACE_(ICommonQueryW, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // ICommonQuery methods
    STDMETHOD(OpenQueryWindow)(THIS_ HWND hwndParent, LPOPENQUERYWINDOW_W pQueryWnd, IDataObject** ppDataObject) PURE;
};

#undef  INTERFACE
#define INTERFACE ICommonQueryA

DECLARE_INTERFACE_(ICommonQueryA, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // ICommonQuery methods
    STDMETHOD(OpenQueryWindow)(THIS_ HWND hwndParent, LPOPENQUERYWINDOW_A pQueryWnd, IDataObject** ppDataObject) PURE;
};

#ifdef UNICODE
#define ICommonQuery     ICommonQueryW
#define IID_ICommonQuery IID_ICommonQueryW
#else
#define ICommonQuery     ICommonQueryA
#define IID_ICommonQuery IID_ICommonQueryA
#endif


;begin_internal

//-----------------------------------------------------------------------------
// Query handler interfaces structures etc
//-----------------------------------------------------------------------------

//
// Query Scopes
// ============
//  A query scope is an opaque structure passed between the query handler
//  and the query frame.  When the handler is first invoked it is asked
//  to declare its scope objects, which inturn the frame holds.  When the
//  query is issued the scope is passed back to the handler.
//
//  When a scope is registered the cbSize field of the structure passed
//  is used to define how large the scope is, that entire blob is then
//  copied into a heap allocation.  Therefore allowing the handler
//  to create scope blocks on the stack, knowing that the frame will
//  take a copy when it calls the AddProc.
//

struct _cqscope;
typedef struct _cqscope CQSCOPE;
typedef CQSCOPE*        LPCQSCOPE;

typedef HRESULT (CALLBACK *LPCQSCOPEPROC)(LPCQSCOPE pScope, UINT uMsg, LPVOID pVoid);

struct _cqscope
{
    DWORD         cbStruct;
    DWORD         dwFlags;
    LPCQSCOPEPROC pScopeProc;
    LPARAM        lParam;
};

#define CQSM_INITIALIZE         0x0000000
#define CQSM_RELEASE            0x0000001
#define CQSM_GETDISPLAYINFO_A   0x0000002   // pVoid -> CQSCOPEDISPLAYINFO_A
#define CQSM_GETDISPLAYINFO_W   0x0000003   // pVoid -> CQSCOPEDISPLAYINFO_W
#define CQSM_SCOPEEQUAL         0x0000004   // pVoid -> CQSCOPE

// CQSM_GETDISPLAYINFO structure and thunk data

typedef struct
{
    DWORD cbStruct;
    DWORD dwFlags;
    LPSTR pDisplayName;
    INT   cchDisplayName;
    LPSTR pIconLocation;
    INT   cchIconLocation;
    INT   iIconResID;
    INT   iIndent;
} CQSCOPEDISPLAYINFO_A, * LPCQSCOPEDISPLAYINFO_A;

typedef struct
{
    DWORD  cbStruct;
    DWORD  dwFlags;
    LPWSTR pDisplayName;
    INT    cchDisplayName;
    LPWSTR pIconLocation;
    INT    cchIconLocation;
    INT    iIconResID;
    INT    iIndent;
} CQSCOPEDISPLAYINFO_W, * LPCQSCOPEDISPLAYINFO_W;

#ifdef UNICODE
#define CQSM_GETDISPLAYINFO  CQSM_GETDISPLAYINFO_W
#define CQSCOPEDISPLAYINFO CQSCOPEDISPLAYINFO_W
#define LPCQSCOPEDISPLAYINFO LPCQSCOPEDISPLAYINFO_W
#else
#define CQSM_GETDISPLAYINFO  CQSM_GETDISPLAYINFO_A
#define CQSCOPEDISPLAYINFO   CQSCOPEDISPLAYINFO_A
#define LPCQSCOPEDISPLAYINFO LPCQSCOPEDISPLAYINFO_A
#endif

//
// Command ID's reserved for the frame to use when talking to
// the handler.  The handler must use only the IDs in the
// range defined by CQID_MINHANDLERMENUID and CQID_MAXHANDLERMENUID
//

#define CQID_MINHANDLERMENUID   0x0100
#define CQID_MAXHANDLERMENUID   0x4000                              // all handler IDs must be below this threshold

#define CQID_FILE_CLOSE         (CQID_MAXHANDLERMENUID + 0x0100)
#define CQID_VIEW_SEARCHPANE    (CQID_MAXHANDLERMENUID + 0x0101)

#define CQID_LOOKFORLABEL       (CQID_MAXHANDLERMENUID + 0x0200)
#define CQID_LOOKFOR            (CQID_MAXHANDLERMENUID + 0x0201)

#define CQID_LOOKINLABEL        (CQID_MAXHANDLERMENUID + 0x0202)
#define CQID_LOOKIN             (CQID_MAXHANDLERMENUID + 0x0203)
#define CQID_BROWSE             (CQID_MAXHANDLERMENUID + 0x0204)

#define CQID_FINDNOW            (CQID_MAXHANDLERMENUID + 0x0205)
#define CQID_STOP               (CQID_MAXHANDLERMENUID + 0x0206)
#define CQID_CLEARALL           (CQID_MAXHANDLERMENUID + 0x0207)

//
// When calling IQueryHandler::ActivateView the following reason codes
// are passed to indicate the type of activation being performed
//

#define CQRVA_ACTIVATE         0x00 // wParam = 0, lParam = 0
#define CQRVA_DEACTIVATE       0x01 // wParam = 0, lParam = 0
#define CQRVA_INITMENUBAR      0x02 // wParam/lParam => WM_INITMENU
#define CQRVA_INITMENUBARPOPUP 0x03 // wParam/lParam => WM_INITMENUPOPUP
#define CQRVA_FORMCHANGED      0x04 // wParam = title length, lParam -> title string
#define CQRVA_STARTQUERY       0x05 // wParam = fStarted, lParam = 0
#define CQRVA_HELP             0x06 // wParma = 0, lParam = LPHELPINFO
#define CQRVA_CONTEXTMENU      0x07 // wParam/lParam from the WM_CONTEXTMENU call on the frame

//
// The frame creates the view and then queries the handler for display
// information (title, icon, animation etc).  These are all loaded as
// resources from the hInstance specified, if 0 is specified for any
// of the resource ID's then defaults are used.
//

typedef struct
{
    DWORD       dwFlags;                    // display attributes
    HINSTANCE   hInstance;                  // resource hInstance
    INT         idLargeIcon;                // resource ID's for icons
    INT         idSmallIcon;
    INT         idTitle;                    // resource ID for title string
    INT         idAnimation;                // resource ID for animation
} CQVIEWINFO, * LPCQVIEWINFO;

//
// IQueryHandler::GetViewObject is passed a scope indiciator to allow it
// to trim the result set.  All handlers must support CQRVS_SELECTION. Also,
// CQRVS_HANDLERMASK defines the flags available for the handler to
// use internally.
//

#define CQRVS_ALL           0x00000001
#define CQRVS_SELECTION     0x00000002
#define CQRVS_MASK          0x00ffffff
#define CQRVS_HANDLERMASK   0xff000000

//
// When invoking the query all the parameters, the scope, the form
// etc are bundled into this structure and then passed to the
// IQueryHandler::IssueQuery method, it inturn populates the view
// previously created with IQueryHandler::CreateResultView.
//

typedef struct
{
    DWORD       cbStruct;
    DWORD       dwFlags;
    LPCQSCOPE   pQueryScope;                // handler specific scope
    LPVOID      pQueryParameters;           // handle specific argument block
    CLSID       clsidForm;                  // form ID
} CQPARAMS, * LPCQPARAMS;

//
// Query Frame Window Messages
// ===========================
//
//  CQFWM_ADDSCOPE
//  --------------
//      wParam = LPCQSCOPE, lParam = HIWORD(index), LOWORD(fSelect)
//
//  Add a scope to the scope list of the dialog, allows async scope collection
//  to be performed.  When the handlers AddScopes method is called then
//  handler can return S_OK, spin off a thread and post CQFWM_ADDSCOPE
//  messages to the frame, which will inturn allow the scopes to be
//  added to the control.  When the frame receives this message it copies
//  the scope as it does on IQueryFrame::AddScope, if the call fails it
//  returns FALSE.
//
#define CQFWM_ADDSCOPE (WM_USER+256)

//
//  CQFWM_GETFRAME
//  --------------
//      wParam = 0, lParam = (IQueryFrame**)
//
//  Allows an object to query for the frame window's IQueryFrame
//  interface, this is used by the property well to talk to the
//  other forms within the system.
//
#define CQFWM_GETFRAME (WM_USER+257)

//
//  CQFWM_ALLSCOPESADDED
//  --------------------
//      wParam = 0, lParam = 0
//
//  If a handler is adding scopes async, then it should issue this message
//  when all the scopes have been added.  That way if the caller specifies
//  OQWF_ISSUEONOPEN we can start the query once all the scopes have been
//  added.
//
#define CQFWM_ALLSCOPESADDED (WM_USER+258)

//
//  CQFWM_STARTQUERY
//  ----------------
//      wParam = 0, lParam = 0
//
//  This call can be made by the frame or the form, it allows it to
//  start the query running in those cases where a form really needs
//  this functionality.
//
//  NB: this should be kept private!
//
#define CQFWM_STARTQUERY (WM_USER+259)


//
// IQueryFrame
//

#undef  INTERFACE
#define INTERFACE   IQueryFrame

DECLARE_INTERFACE_(IQueryFrame, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IQueryFrame methods ***
    STDMETHOD(AddScope)(THIS_ LPCQSCOPE pScope, INT i, BOOL fSelect) PURE;
    STDMETHOD(GetWindow)(THIS_ HWND* phWnd) PURE;
    STDMETHOD(InsertMenus)(THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidth) PURE;
    STDMETHOD(RemoveMenus)(THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetMenu)(THIS_ HMENU hmenuShared, HOLEMENU holereservedMenu) PURE;
    STDMETHOD(SetStatusText)(THIS_ LPCTSTR pszStatusText) PURE;
    STDMETHOD(StartQuery)(THIS_ BOOL fStarting) PURE;
    STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(CallForm)(THIS_ LPCLSID pForm, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(GetScope)(THIS_ LPCQSCOPE* ppScope) PURE;
    STDMETHOD(GetHandler)(THIS_ REFIID riid, void **ppv) PURE;
};

//
// IQueryHandler interface
//

#undef  INTERFACE
#define INTERFACE IQueryHandler

DECLARE_INTERFACE_(IQueryHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IQueryHandler methods ***
    STDMETHOD(Initialize)(THIS_ IQueryFrame* pQueryFrame, DWORD dwOQWFlags, LPVOID pParameters) PURE;
    STDMETHOD(GetViewInfo)(THIS_ LPCQVIEWINFO pViewInfo) PURE;
    STDMETHOD(AddScopes)(THIS) PURE;
    STDMETHOD(BrowseForScope)(THIS_ HWND hwndParent, LPCQSCOPE pCurrentScope, LPCQSCOPE* ppScope) PURE;
    STDMETHOD(CreateResultView)(THIS_ HWND hwndParent, HWND* phWndView) PURE;
    STDMETHOD(ActivateView)(THIS_ UINT uState, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(InvokeCommand)(THIS_ HWND hwndParent, UINT idCmd) PURE;
    STDMETHOD(GetCommandString)(THIS_ UINT idCmd, DWORD dwFlags, LPTSTR pBuffer, INT cchBuffer) PURE;
    STDMETHOD(IssueQuery)(THIS_ LPCQPARAMS pQueryParams) PURE;
    STDMETHOD(StopQuery)(THIS) PURE;
    STDMETHOD(GetViewObject)(THIS_ UINT uScope, REFIID riid, LPVOID* ppvOut) PURE;
    STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery) PURE;
    STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery, LPCQSCOPE pScope) PURE;
};
;end_internal

;begin_both
#endif  // GUID_DEFS_ONLY
#endif
;end_both
