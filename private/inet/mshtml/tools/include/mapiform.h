/*
 *  M A P I F O R M . H
 *
 *  Declarations of interfaces for clients and providers of MAPI
 *  forms and form registries.
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef MAPIFORM_H
#define MAPIFORM_H

/* Include common MAPI header files if they haven't been already. */

#ifndef MAPIDEFS_H
#include <mapidefs.h>
#include <mapicode.h>
#include <mapiguid.h>
#include <mapitags.h>
#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

#ifndef _MAC
typedef const RECT FAR *LPCRECT;
#endif

/* HFRMREG is an enumeration which represents a registry container.
 * Microsoft reserves the values from 0 to 0x3FFF for its own use.
 */

typedef ULONG HFRMREG;

#define HFRMREG_DEFAULT  0
#define HFRMREG_LOCAL    1
#define HFRMREG_PERSONAL 2
#define HFRMREG_FOLDER   3

DECLARE_MAPI_INTERFACE_PTR(IPersistMessage, LPPERSISTMESSAGE);
DECLARE_MAPI_INTERFACE_PTR(IMAPIMessageSite, LPMAPIMESSAGESITE);
DECLARE_MAPI_INTERFACE_PTR(IMAPISession, LPMAPISESSION);
DECLARE_MAPI_INTERFACE_PTR(IMAPIViewContext, LPMAPIVIEWCONTEXT);
DECLARE_MAPI_INTERFACE_PTR(IMAPIViewAdviseSink, LPMAPIVIEWADVISESINK);
DECLARE_MAPI_INTERFACE_PTR(IMAPIFormAdviseSink, LPMAPIFORMADVISESINK);
DECLARE_MAPI_INTERFACE_PTR(IMAPIFormInfo, LPMAPIFORMINFO);
DECLARE_MAPI_INTERFACE_PTR(IMAPIFormMgr, LPMAPIFORMMGR);
DECLARE_MAPI_INTERFACE_PTR(IMAPIFormContainer, LPMAPIFORMCONTAINER);
DECLARE_MAPI_INTERFACE_PTR(IMAPIForm, LPMAPIFORM);
DECLARE_MAPI_INTERFACE_PTR(IMAPIFormFactory, LPMAPIFORMFACTORY);

typedef const char FAR *FAR * LPPCSTR;
typedef LPMAPIFORMINFO FAR *LPPMAPIFORMINFO;

STDAPI MAPIOpenFormMgr(LPMAPISESSION pSession, LPMAPIFORMMGR FAR * ppmgr);
STDAPI MAPIOpenLocalFormContainer(LPMAPIFORMCONTAINER FAR * ppfcnt);


/*-- GetLastError ----------------------------------------------------------*/
/* This defines the GetLastError method held in common by most mapiform
 * interfaces.  It is defined separately so that an implementor may include
 * more than one mapiform interface in a class.
 */

#define MAPI_GETLASTERROR_METHOD(IPURE)                                 \
    MAPIMETHOD(GetLastError) (THIS_                                     \
        /*in*/  HRESULT hResult,                                        \
    /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIERROR FAR * lppMAPIError) IPURE;                  \


/*-- IPersistMessage -------------------------------------------------------*/
/* This interface is implemented by forms and is used to save,
 * initialize and load forms to and from messages.
 */

#define MAPI_IPERSISTMESSAGE_METHODS(IPURE)                             \
    MAPIMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;             \
    MAPIMETHOD(IsDirty)(THIS) IPURE;                                    \
    MAPIMETHOD(InitNew)(THIS_                                           \
        /*in*/ LPMAPIMESSAGESITE pMessageSite,                          \
        /*in*/ LPMESSAGE pMessage) IPURE;                               \
    MAPIMETHOD(Load)(THIS_                                              \
        /*in*/ LPMAPIMESSAGESITE pMessageSite,                          \
        /*in*/ LPMESSAGE pMessage,                                      \
        /*in*/ ULONG ulMessageStatus,                                   \
        /*in*/ ULONG ulMessageFlags) IPURE;                             \
    MAPIMETHOD(Save)(THIS_                                              \
        /*in*/ LPMESSAGE pMessage,                                      \
        /*in*/ ULONG fSameAsLoad) IPURE;                                \
    MAPIMETHOD(SaveCompleted)(THIS_                                     \
        /*in*/ LPMESSAGE pMessage) IPURE;                               \
    MAPIMETHOD(HandsOffMessage)(THIS) IPURE;                            \

#undef INTERFACE
#define INTERFACE IPersistMessage
DECLARE_MAPI_INTERFACE_(IPersistMessage, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IPERSISTMESSAGE_METHODS(PURE)
};


/*-- IMAPIMessageSite ------------------------------------------------------*/

#define MAPI_IMAPIMESSAGESITE_METHODS(IPURE)                            \
    MAPIMETHOD(GetSession) (THIS_                                       \
        /*out*/ LPMAPISESSION FAR * ppSession) IPURE;                   \
    MAPIMETHOD(GetStore) (THIS_                                         \
        /*out*/ LPMDB FAR * ppStore) IPURE;                             \
    MAPIMETHOD(GetFolder) (THIS_                                        \
        /*out*/ LPMAPIFOLDER FAR * ppFolder) IPURE;                     \
    MAPIMETHOD(GetMessage) (THIS_                                       \
        /*out*/ LPMESSAGE FAR * ppmsg) IPURE;                           \
    MAPIMETHOD(GetFormManager) (THIS_                                   \
        /*out*/ LPMAPIFORMMGR FAR * ppFormMgr) IPURE;                   \
    MAPIMETHOD(NewMessage) (THIS_                                       \
        /*in*/  ULONG fComposeInFolder,                                 \
        /*in*/  LPMAPIFOLDER pFolderFocus,                              \
        /*in*/  LPPERSISTMESSAGE pPersistMessage,                       \
        /*out*/ LPMESSAGE FAR * ppMessage,                              \
        /*out*/ LPMAPIMESSAGESITE FAR * ppMessageSite,                  \
        /*out*/ LPMAPIVIEWCONTEXT FAR * ppViewContext) IPURE;           \
    MAPIMETHOD(CopyMessage) (THIS_                                      \
        /*in*/  LPMAPIFOLDER pFolderDestination) IPURE;                 \
    MAPIMETHOD(MoveMessage) (THIS_                                      \
        /*in*/  LPMAPIFOLDER pFolderDestination,                        \
        /*in*/  LPMAPIVIEWCONTEXT pViewContext,                         \
        /*in*/  LPCRECT prcPosRect) IPURE;                              \
    MAPIMETHOD(DeleteMessage) (THIS_                                    \
        /*in*/  LPMAPIVIEWCONTEXT pViewContext,                         \
        /*in*/  LPCRECT prcPosRect) IPURE;                              \
    MAPIMETHOD(SaveMessage) (THIS) IPURE;                               \
    MAPIMETHOD(SubmitMessage) (THIS_                                    \
        /*in*/ ULONG ulFlags) IPURE;                                    \
    MAPIMETHOD(GetSiteStatus) (THIS_                                    \
        /*out*/ LPULONG lpulStatus) IPURE;                              \

#undef INTERFACE
#define INTERFACE IMAPIMessageSite
DECLARE_MAPI_INTERFACE_(IMAPIMessageSite, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIMESSAGESITE_METHODS(PURE)
};


/*-- IMAPIForm -------------------------------------------------------------*/
/* This interface is implemented by forms for the benefit of viewers.
 * One method (ShutdownForm) is provided such that simple forms implementing
 * only IMAPIForm and IPersistMessage have reasonable embedding behavior.
 */

#define MAPI_IMAPIFORM_METHODS(IPURE)                                   \
    MAPIMETHOD(SetViewContext) (THIS_                                   \
        /*in*/  LPMAPIVIEWCONTEXT pViewContext) IPURE;                  \
    MAPIMETHOD(GetViewContext) (THIS_                                   \
        /*out*/ LPMAPIVIEWCONTEXT FAR * ppViewContext) IPURE;           \
    MAPIMETHOD(ShutdownForm)(THIS_                                             \
        /*in*/  ULONG ulSaveOptions) IPURE;                             \
    MAPIMETHOD(DoVerb) (THIS_                                           \
        /*in*/  LONG iVerb,                                             \
        /*in*/  LPMAPIVIEWCONTEXT lpViewContext, /* can be null */      \
        /*in*/  ULONG hwndParent,                                       \
        /*in*/  LPCRECT lprcPosRect) IPURE;                             \
    MAPIMETHOD(Advise)(THIS_                                            \
        /*in*/  LPMAPIVIEWADVISESINK pAdvise,                           \
        /*out*/ ULONG FAR * pdwStatus) IPURE;                           \
    MAPIMETHOD(Unadvise) (THIS_                                         \
        /*in*/  ULONG ulConnection) IPURE;                              \

#undef INTERFACE
#define INTERFACE IMAPIForm
DECLARE_MAPI_INTERFACE_(IMAPIForm, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIFORM_METHODS(PURE)
};

typedef enum tagSAVEOPTS
{
    SAVEOPTS_SAVEIFDIRTY = 0,
    SAVEOPTS_NOSAVE = 1,
    SAVEOPTS_PROMPTSAVE = 2
} SAVEOPTS;


/*-- IMAPIViewContext ------------------------------------------------------*/
/* Implemented by viewers to support next/previous in forms.
 */

/* Structure passed in GetPrintSetup  */

typedef struct {
    ULONG ulFlags;  /* MAPI_UNICODE */
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    ULONG ulFirstPageNumber;
    ULONG fPrintAttachments;
} FORMPRINTSETUP, FAR * LPFORMPRINTSETUP;

/* Values for pulFormat in GetSaveStream */

#define SAVE_FORMAT_TEXT                1
#define SAVE_FORMAT_RICHTEXT            2

/* Values from 0 to 0x3fff are reserved for future definition by Microsoft */

#define MAPI_IMAPIVIEWCONTEXT_METHODS(IPURE)                            \
    MAPIMETHOD(SetAdviseSink)(THIS_                                     \
        /*in*/  LPMAPIFORMADVISESINK pmvns) IPURE;                      \
    MAPIMETHOD(ActivateNext)(THIS_                                      \
        /*in*/  ULONG ulDir,                                            \
        /*in*/  LPCRECT prcPosRect) IPURE;                              \
    MAPIMETHOD(GetPrintSetup)(THIS_                                     \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPFORMPRINTSETUP FAR * lppFormPrintSetup) IPURE;        \
    MAPIMETHOD(GetSaveStream)(THIS_                                     \
        /*out*/ ULONG FAR * pulFlags,                                   \
        /*out*/ ULONG FAR * pulFormat,                                  \
        /*out*/ LPSTREAM FAR * ppstm) IPURE;                            \
    MAPIMETHOD(GetViewStatus) (THIS_                                    \
        /*out*/ LPULONG lpulStatus) IPURE;                              \

#undef INTERFACE
#define INTERFACE IMAPIViewContext
DECLARE_MAPI_INTERFACE_(IMAPIViewContext, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIVIEWCONTEXT_METHODS(PURE)
};

#define VCSTATUS_NEXT                           0x00000001
#define VCSTATUS_PREV                           0x00000002
#define VCSTATUS_MODAL                          0x00000004
#define VCSTATUS_INTERACTIVE                    0x00000008
#define VCSTATUS_READONLY                       0x00000010
#define VCSTATUS_DELETE                         0x00010000
#define VCSTATUS_COPY                           0x00020000
#define VCSTATUS_MOVE                           0x00040000
#define VCSTATUS_SUBMIT                         0x00080000
#define VCSTATUS_DELETE_IS_MOVE                 0x00100000
#define VCSTATUS_SAVE                           0x00200000
#define VCSTATUS_NEW_MESSAGE                    0x00400000

#define VCDIR_NEXT                              VCSTATUS_NEXT
#define VCDIR_PREV                              VCSTATUS_PREV
#define VCDIR_DELETE                            VCSTATUS_DELETE
#define VCDIR_MOVE                              VCSTATUS_MOVE


/*-- IMAPIFormAdviseSink ---------------------------------------------------*/
/* Part of form server, held by view; receives notifications from the view.
 *
 * This part of the form server, but is not an interface on the form
 * object.  This means that clients should not expect to QueryInterface
 * from an IMAPIForm* or IOleObject* to this interface, or vice versa.
 */

#define MAPI_IMAPIFORMADVISESINK_METHODS(IPURE)                         \
    STDMETHOD(OnChange)(THIS_ ULONG ulDir) IPURE;                       \
    STDMETHOD(OnActivateNext)(THIS_                                     \
        /*in*/  LPCSTR lpszMessageClass,                                \
        /*in*/  ULONG ulMessageStatus,                                  \
        /*in*/  ULONG ulMessageFlags,                                   \
        /*out*/ LPPERSISTMESSAGE FAR * ppPersistMessage) IPURE;         \

#undef INTERFACE
#define INTERFACE IMAPIFormAdviseSink
DECLARE_MAPI_INTERFACE_(IMAPIFormAdviseSink, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPIFORMADVISESINK_METHODS(PURE)
};


/*-- IMAPIViewAdviseSink ---------------------------------------------------*/
/* Part of view context, held by form; receives notifications from the form.
 */

#define MAPI_IMAPIVIEWADVISESINK_METHODS(IPURE)                         \
    MAPIMETHOD(OnShutdown)(THIS) IPURE;                                    \
    MAPIMETHOD(OnNewMessage)(THIS) IPURE;                               \
    MAPIMETHOD(OnPrint)(THIS_                                           \
        /*in*/ ULONG dwPageNumber,                                      \
        /*in*/ HRESULT hrStatus) IPURE;                                 \
    MAPIMETHOD(OnSubmitted) (THIS) IPURE;                               \
    MAPIMETHOD(OnSaved) (THIS) IPURE;                                   \

#undef INTERFACE
#define INTERFACE IMAPIViewAdviseSink
DECLARE_MAPI_INTERFACE_(IMAPIViewAdviseSink, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPIVIEWADVISESINK_METHODS(PURE)
};


/*-- IMAPIFormInfo ---------------------------------------------------------*/
/* Is implemented by registries.  Describes the form.
 */

/* Single enum value */

typedef struct
{                               /* fpev */
    LPTSTR pszDisplayName;      /* carries the display string */
    ULONG nVal;                 /* the value for the above enumeration */
} SMAPIFormPropEnumVal, FAR * LPMAPIFORMPROPENUMVAL;

/* MAPI Form property descriptor */

/*
 * Values for the tag in the SMAPIFormProp structure
 *
 * Microsoft reserves the range from 0 to 0x3FFF for future use in its other
 * forms registry implementations.
 */

typedef ULONG FORMPROPSPECIALTYPE;

#define FPST_VANILLA                    0
#define FPST_ENUM_PROP                  1

typedef struct
{
    ULONG ulFlags;              /* Contains MAPI_UNICODE if strings are UNICODE */
    ULONG nPropType;            /* type of the property, hiword is 0 */
    MAPINAMEID nmid;            /* id of the property */
    LPTSTR pszDisplayName;
    FORMPROPSPECIALTYPE nSpecialType;   /* tag for the following union */
    union
    {
        struct
        {
            MAPINAMEID nmidIdx;
            ULONG cfpevAvailable;   /* # of enums */
            LPMAPIFORMPROPENUMVAL pfpevAvailable;
        } s1;                   /* Property String/Number association Enumeration */
    } u;
} SMAPIFormProp, FAR * LPMAPIFORMPROP;

/* Array of form properties */

typedef struct
{
    ULONG cProps;
    ULONG ulPad;                /* Pad to 8-byte alignment for insurance */
    SMAPIFormProp aFormProp[MAPI_DIM];
} SMAPIFormPropArray, FAR * LPMAPIFORMPROPARRAY;

#define CbMAPIFormPropArray(_c) \
         (offsetof(SMAPIFormPropArray, aFormProp) + \
         (_c)*sizeof(SMAPIFormProp))

/* Structure defining the layout of an mapi verb description */

typedef struct
{
    LONG lVerb;
    LPTSTR szVerbname;
    DWORD fuFlags;
    DWORD grfAttribs;
    ULONG ulFlags;              /* Either 0 or MAPI_UNICODE */
} SMAPIVerb, FAR * LPMAPIVERB;

/* Structure used for returning arrays of mapi verbs */

typedef struct
{
    ULONG cMAPIVerb;            /* Number of verbs in the structure */
    SMAPIVerb aMAPIVerb[MAPI_DIM];
} SMAPIVerbArray, FAR * LPMAPIVERBARRAY;

#define CbMAPIVerbArray(_c) \
         (offsetof(SMAPIVerbArray, aMAPIVerb) + \
         (_c)*sizeof(SMAPIVerb))

#define MAPI_IMAPIFORMINFO_METHODS(IPURE)                               \
    MAPIMETHOD(CalcFormPropSet)(THIS_                                   \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIFORMPROPARRAY FAR * ppFormPropArray) IPURE;       \
    MAPIMETHOD(CalcVerbSet)(THIS_                                       \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIVERBARRAY FAR * ppMAPIVerbArray) IPURE;           \
    MAPIMETHOD(MakeIconFromBinary)(THIS_                                \
        /*in*/ ULONG nPropID,                                           \
        /*out*/ HICON FAR* phicon) IPURE;                               \
    MAPIMETHOD(SaveForm)(THIS_                                          \
        /*in*/ LPCTSTR szFileName) IPURE;                               \
    MAPIMETHOD(OpenFormContainer)(THIS_                                 \
        /*out*/ LPMAPIFORMCONTAINER FAR * ppformcontainer) IPURE;       \

#undef INTERFACE
#define INTERFACE IMAPIFormInfo
DECLARE_MAPI_INTERFACE_(IMAPIFormInfo, IMAPIProp)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPIPROP_METHODS(PURE)    /* note: subsumes getlasterror */
    MAPI_IMAPIFORMINFO_METHODS(PURE)
};


/* Enumeration of permissible values for PR_FORM_MESSAGE_BEHAVIOR */

#define MAPI_MESSAGE_BEHAVIOR_IPM 0
#define MAPI_MESSAGE_BEHAVIOR_FOLDER 1


/*-- IMAPIFormMgr ----------------------------------------------------------*/
/* The client-visible interface for form resolution and dispatch.
 */

/* Structure containing an array of message class strings */

typedef struct
{
    ULONG cValues;
    LPCSTR aMessageClass[MAPI_DIM];
} SMessageClassArray, FAR * LPSMESSAGECLASSARRAY;

#define CbMessageClassArray(_c) \
        (offsetof(SMessageClassArray, aMessageClass) + (_c)*sizeof(LPCSTR))

/* Structure containing an array of IMAPIFormInfo interfaces */

typedef struct
{
    ULONG cForms;
    LPMAPIFORMINFO aFormInfo[MAPI_DIM];
} SMAPIFormInfoArray, FAR * LPSMAPIFORMINFOARRAY;

#define CbMAPIFormInfoArray(_c) \
         (offsetof(SMAPIFormInfoArray, aFormInfo) + \
         (_c)*sizeof(LPMAPIFORMINFO))

/* Flags for IMAPIFormMgr::SelectFormContainer */

#define MAPIFORM_SELECT_ALL_REGISTRIES           0
#define MAPIFORM_SELECT_FOLDER_REGISTRY_ONLY     1
#define MAPIFORM_SELECT_NON_FOLDER_REGISTRY_ONLY 2

/* Flags for IMAPIFormMgr::CalcFormPropSet */

#define FORMPROPSET_UNION                 0
#define FORMPROPSET_INTERSECTION          1

/* Flags for IMAPIFormMgr::ResolveMessageClass and
   IMAPIFormMgr::ResolveMultipleMessageClasses */

#define MAPIFORM_EXACTMATCH             0x0020

#define MAPI_IMAPIFORMMGR_METHODS(IPURE)                                \
    MAPIMETHOD(LoadForm)(THIS_                                          \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPCSTR lpszMessageClass,                                \
        /*in*/  ULONG ulMessageStatus,                                  \
        /*in*/  ULONG ulMessageFlags,                                   \
        /*in*/  LPMAPIFOLDER pFolderFocus,                              \
        /*in*/  LPMAPIMESSAGESITE pMessageSite,                         \
        /*in*/  LPMESSAGE pmsg,                                         \
        /*in*/  LPMAPIVIEWCONTEXT pViewContext,                         \
        /*in*/  REFIID riid,                                            \
        /*out*/ LPVOID FAR *ppvObj) IPURE;                              \
    MAPIMETHOD(ResolveMessageClass)(THIS_                               \
        /*in*/  LPCSTR szMsgClass,                                      \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPMAPIFOLDER pFolderFocus, /* can be null */            \
        /*out*/ LPMAPIFORMINFO FAR* ppResult) IPURE;                    \
    MAPIMETHOD(ResolveMultipleMessageClasses)(THIS_                     \
        /*in*/  LPSMESSAGECLASSARRAY pMsgClasses,                       \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPMAPIFOLDER pFolderFocus, /* can be null */            \
        /*out*/ LPSMAPIFORMINFOARRAY FAR * pfrminfoarray) IPURE;        \
    MAPIMETHOD(CalcFormPropSet)(THIS_                                   \
        /*in*/  LPSMAPIFORMINFOARRAY pfrminfoarray,                     \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIFORMPROPARRAY FAR* ppResults) IPURE;              \
    MAPIMETHOD(CreateForm)(THIS_                                        \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPMAPIFORMINFO pfrminfoToActivate,                      \
        /*in*/  REFIID refiidToAsk,                                     \
        /*out*/ LPVOID FAR* ppvObj) IPURE;                              \
    MAPIMETHOD(SelectForm)(THIS_                                        \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPCTSTR pszTitle,                                       \
        /*in*/  LPMAPIFOLDER pfld,                                      \
        /*out*/ LPMAPIFORMINFO FAR * ppfrminfoReturned) IPURE;          \
    MAPIMETHOD(SelectMultipleForms)(THIS_                               \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPCTSTR pszTitle,                                       \
        /*in*/  LPMAPIFOLDER pfld,                                      \
        /*in*/  LPSMAPIFORMINFOARRAY pfrminfoarray,                     \
        /*out*/ LPSMAPIFORMINFOARRAY FAR * ppfrminfoarray) IPURE;       \
    MAPIMETHOD(SelectFormContainer)(THIS_                               \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIFORMCONTAINER FAR * lppfcnt) IPURE;               \
    MAPIMETHOD(OpenFormContainer)(THIS_                                 \
        /*in*/  HFRMREG hfrmreg,                                        \
        /*in*/  LPUNKNOWN lpunk,                                        \
        /*out*/ LPMAPIFORMCONTAINER FAR * lppfcnt) IPURE;               \
    MAPIMETHOD(PrepareForm)(THIS_                                       \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPMAPIFORMINFO pfrminfo) IPURE;                         \
    MAPIMETHOD(IsInConflict)(THIS_                                      \
        /*in*/  ULONG ulMessageFlags,                                   \
        /*in*/  ULONG ulMessageStatus,                                  \
        /*in*/  LPCSTR szMessageClass,                                  \
        /*in*/  LPMAPIFOLDER pFolderFocus) IPURE;                       \

#undef         INTERFACE
#define         INTERFACE    IMAPIFormMgr
DECLARE_MAPI_INTERFACE_(IMAPIFormMgr, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIFORMMGR_METHODS(PURE)
};

/* Platform numbers (used in .CFG files for forms) */

#define MAPIFORM_CPU_X86                1
#define MAPIFORM_CPU_MIP                2
#define MAPIFORM_CPU_AXP                3
#define MAPIFORM_CPU_PPC                4
#define MAPIFORM_CPU_M68                5

#define MAPIFORM_OS_WIN_31              1
#define MAPIFORM_OS_WINNT_35            2
#define MAPIFORM_OS_WIN_95              3
#define MAPIFORM_OS_MAC_7x              4
#define MAPIFORM_OS_WINNT_40            5

#define MAPIFORM_PLATFORM(CPU, OS) ((ULONG) ((((ULONG) CPU) << 16) | OS))


/*-- IMAPIFormContainer -------------------------------------------------*/

/*  Flags for IMAPIFormMgr::CalcFormPropSet */

/* #define FORMPROPSET_UNION            0   */
/* #define FORMPROPSET_INTERSECTION     1   */

/*  Flags for IMAPIFormMgr::InstallForm     */

#define MAPIFORM_INSTALL_DIALOG                 MAPI_DIALOG
#define MAPIFORM_INSTALL_OVERWRITEONCONFLICT    0x0010

/*  Flags for IMAPIFormContainer::ResolveMessageClass and
    IMAPIFormContainer::ResolveMultipleMessageClasses */
/* #define MAPIFORM_EXACTIMATCH    0x0020   */

#define MAPI_IMAPIFORMCONTAINER_METHODS(IPURE)                       \
    MAPIMETHOD(InstallForm)(THIS_                                   \
        /*in*/  ULONG ulUIParam,                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  LPCTSTR szCfgPathName) IPURE;                           \
    MAPIMETHOD(RemoveForm)(THIS_                                        \
        /*in*/  LPCSTR szMessageClass) IPURE;                           \
    MAPIMETHOD(ResolveMessageClass) (THIS_                              \
        /*in*/  LPCSTR szMessageClass,                                  \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIFORMINFO FAR * pforminfo) IPURE;                  \
    MAPIMETHOD(ResolveMultipleMessageClasses) (THIS_                    \
        /*in*/  LPSMESSAGECLASSARRAY pMsgClassArray,                    \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPSMAPIFORMINFOARRAY FAR * ppfrminfoarray) IPURE;       \
    MAPIMETHOD(CalcFormPropSet)(THIS_                                   \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPMAPIFORMPROPARRAY FAR * ppResults) IPURE;             \
    MAPIMETHOD(GetDisplay)(THIS_                                        \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPTSTR FAR * pszDisplayName) IPURE;                     \

#undef INTERFACE
#define INTERFACE IMAPIFormContainer
DECLARE_MAPI_INTERFACE_(IMAPIFormContainer, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIFORMCONTAINER_METHODS(PURE)
};

/*-- IMAPIFormFactory ------------------------------------------------------*/

#define MAPI_IMAPIFORMFACTORY_METHODS(IPURE)                            \
    MAPIMETHOD(CreateClassFactory) (THIS_                               \
        /*in*/  REFCLSID clsidForm,                                     \
        /*in*/  ULONG ulFlags,                                          \
        /*out*/ LPCLASSFACTORY FAR * lppClassFactory) IPURE;            \
    MAPIMETHOD(LockServer) (THIS_                                       \
        /*in*/  ULONG ulFlags,                                          \
        /*in*/  ULONG fLockServer) IPURE;                               \

#undef INTERFACE
#define INTERFACE IMAPIFormFactory
DECLARE_MAPI_INTERFACE_(IMAPIFormFactory, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_GETLASTERROR_METHOD(PURE)
    MAPI_IMAPIFORMFACTORY_METHODS(PURE)
};

#endif                          /* MAPIFORM_H */

