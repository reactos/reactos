/*
 *  M A P I X . H
 *  
 *  Definitions of objects/flags, etc used by Extended MAPI.
 *  
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef MAPIX_H
#define MAPIX_H

/* Include common MAPI header files if they haven't been already. */
#ifndef MAPIDEFS_H
#include <mapidefs.h>
#endif
#ifndef MAPICODE_H
#include <mapicode.h>
#endif
#ifndef MAPIGUID_H
#include <mapiguid.h>
#endif
#ifndef MAPITAGS_H
#include <mapitags.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif  

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

/* Forward interface declarations */

DECLARE_MAPI_INTERFACE_PTR(IProfAdmin,          LPPROFADMIN);
DECLARE_MAPI_INTERFACE_PTR(IMsgServiceAdmin,    LPSERVICEADMIN);
DECLARE_MAPI_INTERFACE_PTR(IMAPISession,        LPMAPISESSION);

/* ------------------------------------------------------ */
/* shared with simple mapi */

typedef ULONG       FLAGS;

/* MAPILogon() flags.       */

#define MAPI_LOGON_UI           0x00000001  /* Display logon UI                 */
#define MAPI_NEW_SESSION        0x00000002  /* Don't use shared session         */
#define MAPI_ALLOW_OTHERS       0x00000008  /* Make this a shared session       */
#define MAPI_EXPLICIT_PROFILE   0x00000010  /* Don't use default profile        */
#define MAPI_EXTENDED           0x00000020  /* Extended MAPI Logon              */
#define MAPI_FORCE_DOWNLOAD     0x00001000  /* Get new mail before return       */
#define MAPI_SERVICE_UI_ALWAYS  0x00002000  /* Do logon UI in all providers     */
#define MAPI_NO_MAIL            0x00008000  /* Do not activate transports       */
/* #define MAPI_NT_SERVICE          0x00010000  Allow logon from an NT service  */
#ifndef MAPI_PASSWORD_UI
#define MAPI_PASSWORD_UI        0x00020000  /* Display password UI only         */
#endif
#define MAPI_TIMEOUT_SHORT      0x00100000  /* Minimal wait for logon resources */

#define MAPI_SIMPLE_DEFAULT (MAPI_LOGON_UI | MAPI_FORCE_DOWNLOAD | MAPI_ALLOW_OTHERS)
#define MAPI_SIMPLE_EXPLICIT (MAPI_NEW_SESSION | MAPI_FORCE_DOWNLOAD | MAPI_EXPLICIT_PROFILE)

/* Structure passed to MAPIInitialize(), and its ulFlags values */

typedef struct
{
    ULONG           ulVersion;
    ULONG           ulFlags;
} MAPIINIT_0, FAR *LPMAPIINIT_0;

typedef MAPIINIT_0 MAPIINIT;
typedef MAPIINIT FAR *LPMAPIINIT;

#define MAPI_INIT_VERSION               0

#define MAPI_MULTITHREAD_NOTIFICATIONS  0x00000001
/* Reserved for MAPI                    0x40000000 */
/* #define MAPI_NT_SERVICE              0x00010000  Use from NT service */

/* MAPI base functions */

typedef HRESULT (STDAPICALLTYPE MAPIINITIALIZE)(
    LPVOID          lpMapiInit
);
typedef MAPIINITIALIZE FAR *LPMAPIINITIALIZE;

typedef void (STDAPICALLTYPE MAPIUNINITIALIZE)(void);
typedef MAPIUNINITIALIZE FAR *LPMAPIUNINITIALIZE;

MAPIINITIALIZE      MAPIInitialize;
MAPIUNINITIALIZE    MAPIUninitialize;


/*  Extended MAPI Logon function */


typedef HRESULT (STDMETHODCALLTYPE MAPILOGONEX)(
    ULONG ulUIParam,
    LPTSTR lpszProfileName,
    LPTSTR lpszPassword,
    ULONG ulFlags,   /*  ulFlags takes all that SimpleMAPI does + MAPI_UNICODE */
    LPMAPISESSION FAR * lppSession
);
typedef MAPILOGONEX FAR *LPMAPILOGONEX;

MAPILOGONEX MAPILogonEx;


typedef SCODE (STDMETHODCALLTYPE MAPIALLOCATEBUFFER)(
    ULONG           cbSize,
    LPVOID FAR *    lppBuffer
);

typedef SCODE (STDMETHODCALLTYPE MAPIALLOCATEMORE)(
    ULONG           cbSize,
    LPVOID          lpObject,
    LPVOID FAR *    lppBuffer
);

typedef ULONG (STDAPICALLTYPE MAPIFREEBUFFER)(
    LPVOID          lpBuffer
);

typedef MAPIALLOCATEBUFFER FAR  *LPMAPIALLOCATEBUFFER;
typedef MAPIALLOCATEMORE FAR    *LPMAPIALLOCATEMORE;
typedef MAPIFREEBUFFER FAR      *LPMAPIFREEBUFFER;

MAPIALLOCATEBUFFER MAPIAllocateBuffer;
MAPIALLOCATEMORE MAPIAllocateMore;
MAPIFREEBUFFER MAPIFreeBuffer;

typedef HRESULT (STDMETHODCALLTYPE MAPIADMINPROFILES)(
    ULONG ulFlags,
    LPPROFADMIN FAR *lppProfAdmin
);

typedef MAPIADMINPROFILES FAR *LPMAPIADMINPROFILES;

MAPIADMINPROFILES MAPIAdminProfiles;

/* IMAPISession Interface -------------------------------------------------- */

/* Flags for OpenEntry and others */

/*#define MAPI_MODIFY               ((ULONG) 0x00000001) */

/* Flags for Logoff */

#define MAPI_LOGOFF_SHARED      0x00000001  /* Close all shared sessions    */
#define MAPI_LOGOFF_UI          0x00000002  /* It's OK to present UI        */

/* Flags for SetDefaultStore. They are mutually exclusive. */

#define MAPI_DEFAULT_STORE          0x00000001  /* for incoming messages */
#define MAPI_SIMPLE_STORE_TEMPORARY 0x00000002  /* for simple MAPI and CMC */
#define MAPI_SIMPLE_STORE_PERMANENT 0x00000003  /* for simple MAPI and CMC */
#define MAPI_PRIMARY_STORE          0x00000004  /* Used by some clients */
#define MAPI_SECONDARY_STORE        0x00000005  /* Used by some clients */

/* Flags for ShowForm. */

#define MAPI_POST_MESSAGE       0x00000001  /* Selects post/send semantics */
#define MAPI_NEW_MESSAGE        0x00000002  /* Governs copying during submission */

/*  MessageOptions */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/*  QueryDefaultMessageOpt */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

#define MAPI_IMAPISESSION_METHODS(IPURE)                                \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(GetMsgStoresTable)                                       \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
    MAPIMETHOD(OpenMsgStore)                                            \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPMDB FAR *                 lppMDB) IPURE;              \
    MAPIMETHOD(OpenAddressBook)                                         \
        (THIS_  ULONG                       ulUIParam,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPADRBOOK FAR *             lppAdrBook) IPURE;          \
    MAPIMETHOD(OpenProfileSection)                                      \
        (THIS_  LPMAPIUID                   lpUID,                      \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPPROFSECT FAR *            lppProfSect) IPURE;         \
    MAPIMETHOD(GetStatusTable)                                          \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
    MAPIMETHOD(OpenEntry)                                               \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPUNKNOWN FAR *             lppUnk) IPURE;  \
    MAPIMETHOD(CompareEntryIDs)                                         \
        (THIS_  ULONG                       cbEntryID1,                 \
                LPENTRYID                   lpEntryID1,                 \
                ULONG                       cbEntryID2,                 \
                LPENTRYID                   lpEntryID2,                 \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulResult) IPURE;          \
    MAPIMETHOD(Advise)                                                  \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                ULONG                       ulEventMask,                \
                LPMAPIADVISESINK            lpAdviseSink,               \
                ULONG FAR *                 lpulConnection) IPURE;      \
    MAPIMETHOD(Unadvise)                                                \
        (THIS_  ULONG                       ulConnection) IPURE;        \
    MAPIMETHOD(MessageOptions)                                          \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPTSTR                      lpszAdrType,                \
                LPMESSAGE                   lpMessage) IPURE;           \
    MAPIMETHOD(QueryDefaultMessageOpt)                                  \
        (THIS_  LPTSTR                      lpszAdrType,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpcValues,                  \
                LPSPropValue FAR *          lppOptions) IPURE;          \
    MAPIMETHOD(EnumAdrTypes)                                            \
        (THIS_  ULONG                       ulFlags,                    \
                ULONG FAR *                 lpcAdrTypes,                \
                LPTSTR FAR * FAR *          lpppszAdrTypes) IPURE;      \
    MAPIMETHOD(QueryIdentity)                                           \
        (THIS_  ULONG FAR *                 lpcbEntryID,                \
                LPENTRYID FAR *             lppEntryID) IPURE;          \
    MAPIMETHOD(Logoff)                                                  \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                ULONG                       ulReserved) IPURE;          \
    MAPIMETHOD(SetDefaultStore)                                         \
        (THIS_  ULONG                       ulFlags,                    \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID) IPURE;           \
    MAPIMETHOD(AdminServices)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPSERVICEADMIN FAR *        lppServiceAdmin) IPURE;     \
    MAPIMETHOD(ShowForm)                                                \
        (THIS_  ULONG                       ulUIParam,                  \
                LPMDB                       lpMsgStore,                 \
                LPMAPIFOLDER                lpParentFolder,             \
                LPCIID                      lpInterface,                \
                ULONG                       ulMessageToken,             \
                LPMESSAGE                   lpMessageSent,              \
                ULONG                       ulFlags,                    \
                ULONG                       ulMessageStatus,            \
                ULONG                       ulMessageFlags,             \
                ULONG                       ulAccess,                   \
                LPSTR                       lpszMessageClass) IPURE;    \
    MAPIMETHOD(PrepareForm)                                             \
        (THIS_  LPCIID                      lpInterface,                \
                LPMESSAGE                   lpMessage,                  \
                ULONG FAR *                 lpulMessageToken) IPURE;    \


#undef       INTERFACE
#define      INTERFACE  IMAPISession
DECLARE_MAPI_INTERFACE_(IMAPISession, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPISESSION_METHODS(PURE)
};

/*DECLARE_MAPI_INTERFACE_PTR(IMAPISession, LPMAPISESSION);*/

/* IAddrBook Interface ----------------------------------------------------- */

/*  CreateOneOff */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */
/****** MAPI_SEND_NO_RICH_INFO      ((ULONG) 0x00010000) */

/*  RecipOptions */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/*  QueryDefaultRecipOpt */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/*  GetSearchPath */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */


#define MAPI_IADDRBOOK_METHODS(IPURE)                                   \
    MAPIMETHOD(OpenEntry)                                               \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPUNKNOWN FAR *             lppUnk) IPURE;  \
    MAPIMETHOD(CompareEntryIDs)                                         \
        (THIS_  ULONG                       cbEntryID1,                 \
                LPENTRYID                   lpEntryID1,                 \
                ULONG                       cbEntryID2,                 \
                LPENTRYID                   lpEntryID2,                 \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulResult) IPURE;          \
    MAPIMETHOD(Advise)                                                  \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                ULONG                       ulEventMask,                \
                LPMAPIADVISESINK            lpAdviseSink,               \
                ULONG FAR *                 lpulConnection) IPURE;      \
    MAPIMETHOD(Unadvise)                                                \
        (THIS_  ULONG                       ulConnection) IPURE;        \
    MAPIMETHOD(CreateOneOff)                                            \
        (THIS_  LPTSTR                      lpszName,                   \
                LPTSTR                      lpszAdrType,                \
                LPTSTR                      lpszAddress,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpcbEntryID,                \
                LPENTRYID FAR *             lppEntryID) IPURE;          \
    MAPIMETHOD(NewEntry)                                                \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                ULONG                       cbEIDContainer,             \
                LPENTRYID                   lpEIDContainer,             \
                ULONG                       cbEIDNewEntryTpl,           \
                LPENTRYID                   lpEIDNewEntryTpl,           \
                ULONG FAR *                 lpcbEIDNewEntry,            \
                LPENTRYID FAR *             lppEIDNewEntry) IPURE;      \
    MAPIMETHOD(ResolveName)                                             \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPTSTR                      lpszNewEntryTitle,          \
                LPADRLIST                   lpAdrList) IPURE;           \
    MAPIMETHOD(Address)                                                 \
        (THIS_  ULONG FAR *                 lpulUIParam,                \
                LPADRPARM                   lpAdrParms,                 \
                LPADRLIST FAR *             lppAdrList) IPURE;          \
    MAPIMETHOD(Details)                                                 \
        (THIS_  ULONG FAR *                 lpulUIParam,                \
                LPFNDISMISS                 lpfnDismiss,                \
                LPVOID                      lpvDismissContext,          \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPFNBUTTON                  lpfButtonCallback,          \
                LPVOID                      lpvButtonContext,           \
                LPTSTR                      lpszButtonText,             \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(RecipOptions)                                            \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPADRENTRY                  lpRecip) IPURE;             \
    MAPIMETHOD(QueryDefaultRecipOpt)                                    \
        (THIS_  LPTSTR                      lpszAdrType,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpcValues,                  \
                LPSPropValue FAR *          lppOptions) IPURE;          \
    MAPIMETHOD(GetPAB)                                                  \
        (THIS_  ULONG FAR *                 lpcbEntryID,                \
                LPENTRYID FAR *             lppEntryID) IPURE;          \
    MAPIMETHOD(SetPAB)                                                  \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID) IPURE;           \
    MAPIMETHOD(GetDefaultDir)                                           \
        (THIS_  ULONG FAR *                 lpcbEntryID,                \
                LPENTRYID FAR *             lppEntryID) IPURE;          \
    MAPIMETHOD(SetDefaultDir)                                           \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID) IPURE;           \
    MAPIMETHOD(GetSearchPath)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPSRowSet FAR *             lppSearchPath) IPURE;       \
    MAPIMETHOD(SetSearchPath)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPSRowSet                   lpSearchPath) IPURE;        \
    MAPIMETHOD(PrepareRecips)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPSPropTagArray             lpPropTagArray,             \
                LPADRLIST                   lpRecipList) IPURE;         \

#undef       INTERFACE
#define      INTERFACE  IAddrBook
DECLARE_MAPI_INTERFACE_(IAddrBook, IMAPIProp)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPIPROP_METHODS(PURE)
    MAPI_IADDRBOOK_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IAddrBook, LPADRBOOK);

/* IProfAdmin Interface ---------------------------------------------------- */

/* Flags for CreateProfile */
#define MAPI_DEFAULT_SERVICES           0x00000001

/* GetProfileTable */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

#define MAPI_IPROFADMIN_METHODS(IPURE)                                  \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(GetProfileTable)                                         \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
    MAPIMETHOD(CreateProfile)                                           \
        (THIS_  LPTSTR                      lpszProfileName,            \
                LPTSTR                      lpszPassword,               \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(DeleteProfile)                                           \
        (THIS_  LPTSTR                      lpszProfileName,            \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(ChangeProfilePassword)                                   \
        (THIS_  LPTSTR                      lpszProfileName,            \
                LPTSTR                      lpszOldPassword,            \
                LPTSTR                      lpszNewPassword,            \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(CopyProfile)                                             \
        (THIS_  LPTSTR                      lpszOldProfileName,         \
                LPTSTR                      lpszOldPassword,            \
                LPTSTR                      lpszNewProfileName,         \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(RenameProfile)                                           \
        (THIS_  LPTSTR                      lpszOldProfileName,         \
                LPTSTR                      lpszOldPassword,            \
                LPTSTR                      lpszNewProfileName,         \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(SetDefaultProfile)                                       \
        (THIS_  LPTSTR                      lpszProfileName,            \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(AdminServices)                                           \
        (THIS_  LPTSTR                      lpszProfileName,            \
                LPTSTR                      lpszPassword,               \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPSERVICEADMIN FAR *        lppServiceAdmin) IPURE;     \


#undef       INTERFACE
#define      INTERFACE  IProfAdmin
DECLARE_MAPI_INTERFACE_(IProfAdmin, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IPROFADMIN_METHODS(PURE)
};

/* IMsgServiceAdmin Interface ---------------------------------------------- */

/* Values for PR_RESOURCE_FLAGS in message service table */

#define SERVICE_DEFAULT_STORE       0x00000001
#define SERVICE_SINGLE_COPY         0x00000002
#define SERVICE_CREATE_WITH_STORE   0x00000004
#define SERVICE_PRIMARY_IDENTITY    0x00000008
#define SERVICE_NO_PRIMARY_IDENTITY 0x00000020

/*  GetMsgServiceTable */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/*  GetProviderTable */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

#define MAPI_IMSGSERVICEADMIN_METHODS(IPURE)                            \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(GetMsgServiceTable)                                      \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
    MAPIMETHOD(CreateMsgService)                                        \
        (THIS_  LPTSTR                      lpszService,                \
                LPTSTR                      lpszDisplayName,            \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(DeleteMsgService)                                        \
        (THIS_  LPMAPIUID                   lpUID) IPURE;               \
    MAPIMETHOD(CopyMsgService)                                          \
        (THIS_  LPMAPIUID                   lpUID,                      \
                LPTSTR                      lpszDisplayName,            \
                LPCIID                      lpInterfaceToCopy,          \
                LPCIID                      lpInterfaceDst,             \
                LPVOID                      lpObjectDst,                \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(RenameMsgService)                                        \
        (THIS_  LPMAPIUID                   lpUID,                      \
                ULONG                       ulFlags,                    \
                LPTSTR                      lpszDisplayName) IPURE;     \
    MAPIMETHOD(ConfigureMsgService)                                     \
        (THIS_  LPMAPIUID                   lpUID,                      \
                ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                ULONG                       cValues,                    \
                LPSPropValue                lpProps) IPURE;             \
    MAPIMETHOD(OpenProfileSection)                                      \
        (THIS_  LPMAPIUID                   lpUID,                      \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPPROFSECT FAR *            lppProfSect) IPURE;         \
    MAPIMETHOD(MsgServiceTransportOrder)                                \
        (THIS_  ULONG                       cUID,                       \
                LPMAPIUID                   lpUIDList,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(AdminProviders)                                          \
        (THIS_  LPMAPIUID                   lpUID,                      \
                ULONG                       ulFlags,                    \
                LPPROVIDERADMIN FAR *       lppProviderAdmin) IPURE;    \
    MAPIMETHOD(SetPrimaryIdentity)                                      \
        (THIS_  LPMAPIUID                   lpUID,                      \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(GetProviderTable)                                        \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \


#undef       INTERFACE
#define      INTERFACE  IMsgServiceAdmin
DECLARE_MAPI_INTERFACE_(IMsgServiceAdmin, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMSGSERVICEADMIN_METHODS(PURE)
};

#ifdef  __cplusplus
}       /*  extern "C" */
#endif  

#endif /* MAPIX_H */
