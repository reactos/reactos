/*
 *  M A P I S P I . H
 *
 *  Defines the calls and structures exchanged between MAPI or the spooler
 *  and the MAPI service providers
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef MAPISPI_H
#define MAPISPI_H
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

#if defined (WIN16) || defined (DOS) || defined (DOS16)
#include <storage.h>
#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*  The MAPI SPI has a version number.  MAPIX.DLL knows and supports
 *  one or more versions of the SPI.  Each provider supports one or
 *  more versions of the SPI.  Checks are performed in both MAPIX.DLL
 *  and in the provider to ensure that they agree to use exactly one
 *  version of the MAPI SPI.
 *
 *  The SPI version number is composed of a major (8-bit) version,
 *  minor (8-bit) version, and micro (16-bit) version.  The first
 *  retail ship of MAPI 1.0 is expected to be version 1.0.0.
 *  The major version number changes rarely.
 *  The minor version number changes opon each retail ship of
 *  MAPI if the SPI has been modified.
 *  The micro version number changes internally at Microsoft
 *  during development of MAPI.
 *
 *  The version of the SPI documented by this set of header files
 *  is ALWAYS known as "CURRENT_SPI_VERSION".  If you write a
 *  service provider, and get a new set of header files, and update
 *  your code to the new interface, you'll be at the "current" version.
 */
#define CURRENT_SPI_VERSION 0x00010010L

/*  Here are some well-known SPI version numbers:
 *  (These will eventually be useful for provider-writers who
 *  might choose to make provider DLLs that support more than
 *  one version of the MAPI SPI.
 */
#define PDK1_SPI_VERSION    0x00010000L /* 0.1.0  MAPI PDK1 Spring 1993 */

#define PDK2_SPI_VERSION    0x00010008L /* 0.1.8  MAPI PDK2 Spring 1994 */

#define PDK3_SPI_VERSION    0x00010010L /* 0.1.16 MAPI PDK3 Fall 1994   */

/*
 * Forward declaration of interface pointers specific to the service
 * provider interface.
 */
DECLARE_MAPI_INTERFACE_PTR(IMAPISupport, LPMAPISUP);

/* IMAPISupport Interface -------------------------------------------------- */

/* Notification key structure for the MAPI notification engine */

typedef struct
{
    ULONG       cb;             /* How big the key is */
    BYTE        ab[MAPI_DIM];   /* Key contents */
} NOTIFKEY, FAR * LPNOTIFKEY;

#define CbNewNOTIFKEY(_cb)      (offsetof(NOTIFKEY,ab) + (_cb))
#define CbNOTIFKEY(_lpkey)      (offsetof(NOTIFKEY,ab) + (_lpkey)->cb)
#define SizedNOTIFKEY(_cb, _name) \
    struct _NOTIFKEY_ ## _name \
{ \
    ULONG       cb; \
    BYTE        ab[_cb]; \
} _name


/* For Subscribe() */

#define NOTIFY_SYNC             ((ULONG) 0x40000000)

/* For Notify() */

#define NOTIFY_CANCELED         ((ULONG) 0x80000000)


/* From the Notification Callback function (well, this is really a ulResult) */

#define CALLBACK_DISCONTINUE    ((ULONG) 0x80000000)

/* For Transport's SpoolerNotify() */

#define NOTIFY_NEWMAIL          ((ULONG) 0x00000001)
#define NOTIFY_READYTOSEND      ((ULONG) 0x00000002)
#define NOTIFY_SENTDEFERRED     ((ULONG) 0x00000004)
#define NOTIFY_CRITSEC          ((ULONG) 0x00001000)
#define NOTIFY_NONCRIT          ((ULONG) 0x00002000)
#define NOTIFY_CONFIG_CHANGE    ((ULONG) 0x00004000)
#define NOTIFY_CRITICAL_ERROR   ((ULONG) 0x10000000)

/* For Message Store's SpoolerNotify() */

#define NOTIFY_NEWMAIL_RECEIVED ((ULONG) 0x20000000)

/* For ModifyStatusRow() */

#define STATUSROW_UPDATE        ((ULONG) 0x10000000)

/* For IStorageFromStream() */

#define STGSTRM_RESET           ((ULONG) 0x00000000)
#define STGSTRM_CURRENT         ((ULONG) 0x10000000)
#define STGSTRM_MODIFY          ((ULONG) 0x00000002)
#define STGSTRM_CREATE          ((ULONG) 0x00001000)

/* For GetOneOffTable() */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/* For CreateOneOff() */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */
/****** MAPI_SEND_NO_RICH_INFO  ((ULONG) 0x00010000) */

/* For ReadReceipt() */
#define MAPI_NON_READ           ((ULONG) 0x00000001)

/* For DoConfigPropSheet() */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

/* Preprocessor calls: */

/* PreprocessMessage, first ordinal in RegisterPreprocessor(). */

typedef HRESULT (STDMETHODCALLTYPE PREPROCESSMESSAGE)(
                    LPVOID lpvSession,
                    LPMESSAGE lpMessage,
                    LPADRBOOK lpAdrBook,
                    LPMAPIFOLDER lpFolder,
                    LPALLOCATEBUFFER AllocateBuffer,
                    LPALLOCATEMORE AllocateMore,
                    LPFREEBUFFER FreeBuffer,
                    ULONG FAR *lpcOutbound,
                    LPMESSAGE FAR * FAR *lpppMessage,
                    LPADRLIST FAR *lppRecipList);

/* RemovePreprocessInfo, second ordinal in RegisterPreprocessor(). */

typedef HRESULT (STDMETHODCALLTYPE REMOVEPREPROCESSINFO)(LPMESSAGE lpMessage);

/* Function pointer for GetReleaseInfo */

#define MAPI_IMAPISUPPORT_METHODS1(IPURE)                               \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(GetMemAllocRoutines)                                     \
        (THIS_  LPALLOCATEBUFFER FAR *      lpAllocateBuffer,           \
                LPALLOCATEMORE FAR *        lpAllocateMore,             \
                LPFREEBUFFER FAR *          lpFreeBuffer) IPURE;        \
    MAPIMETHOD(Subscribe)                                               \
        (THIS_  LPNOTIFKEY                  lpKey,                      \
                ULONG                       ulEventMask,                \
                ULONG                       ulFlags,                    \
                LPMAPIADVISESINK            lpAdviseSink,               \
                ULONG FAR *                 lpulConnection) IPURE;      \
    MAPIMETHOD(Unsubscribe)                                             \
        (THIS_  ULONG                       ulConnection) IPURE;        \
    MAPIMETHOD(Notify)                                                  \
        (THIS_  LPNOTIFKEY                  lpKey,                      \
                ULONG                       cNotification,              \
                LPNOTIFICATION              lpNotifications,            \
                ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(ModifyStatusRow)                                         \
        (THIS_  ULONG                       cValues,                    \
                LPSPropValue                lpColumnVals,               \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(OpenProfileSection)                                      \
        (THIS_  LPMAPIUID                   lpUid,                      \
                ULONG                       ulFlags,                    \
                LPPROFSECT FAR *            lppProfileObj) IPURE;       \
    MAPIMETHOD(RegisterPreprocessor)                                    \
        (THIS_  LPMAPIUID                   lpMuid,                     \
                LPTSTR                      lpszAdrType,                \
                LPTSTR                      lpszDLLName,                \
                LPSTR   /* String8! */      lpszPreprocess,             \
                LPSTR   /* String8! */      lpszRemovePreprocessInfo,   \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(NewUID)                                                  \
        (THIS_  LPMAPIUID                   lpMuid) IPURE;              \
    MAPIMETHOD(MakeInvalid)                                             \
        (THIS_  ULONG                       ulFlags,                    \
                LPVOID                      lpObject,                   \
                ULONG                       ulRefCount,                 \
                ULONG                       cMethods) IPURE;            \

#define MAPI_IMAPISUPPORT_METHODS2(IPURE)                               \
    MAPIMETHOD(SpoolerYield)                                            \
        (THIS_  ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(SpoolerNotify)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPVOID                      lpvData) IPURE;             \
    MAPIMETHOD(CreateOneOff)                                            \
        (THIS_  LPTSTR                      lpszName,                   \
                LPTSTR                      lpszAdrType,                \
                LPTSTR                      lpszAddress,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpcbEntryID,                \
                LPENTRYID FAR *             lppEntryID) IPURE;          \
    MAPIMETHOD(SetProviderUID)                                          \
        (THIS_  LPMAPIUID                   lpProviderID,               \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(CompareEntryIDs)                                         \
        (THIS_  ULONG                       cbEntry1,                   \
                LPENTRYID                   lpEntry1,                   \
                ULONG                       cbEntry2,                   \
                LPENTRYID                   lpEntry2,                   \
                ULONG                       ulCompareFlags,             \
                ULONG FAR *                 lpulResult) IPURE;          \
    MAPIMETHOD(OpenTemplateID)                                          \
        (THIS_  ULONG                       cbTemplateID,               \
                LPENTRYID                   lpTemplateID,               \
                ULONG                       ulTemplateFlags,            \
                LPMAPIPROP                  lpMAPIPropData,             \
                LPCIID                      lpInterface,                \
                LPMAPIPROP FAR *            lppMAPIPropNew,             \
                LPMAPIPROP                  lpMAPIPropSibling) IPURE;   \
    MAPIMETHOD(OpenEntry)                                               \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulOpenFlags,                \
                ULONG FAR *                 lpulObjType,                \
                LPUNKNOWN FAR *             lppUnk) IPURE;              \
    MAPIMETHOD(GetOneOffTable)                                          \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
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
    MAPIMETHOD(NewEntry)                                                \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                ULONG                       cbEIDContainer,             \
                LPENTRYID                   lpEIDContainer,             \
                ULONG                       cbEIDNewEntryTpl,           \
                LPENTRYID                   lpEIDNewEntryTpl,           \
                ULONG FAR *                 lpcbEIDNewEntry,            \
                LPENTRYID FAR *             lppEIDNewEntry) IPURE;      \
    MAPIMETHOD(DoConfigPropsheet)                                       \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPTSTR                      lpszTitle,                  \
                LPMAPITABLE                 lpDisplayTable,             \
                LPMAPIPROP                  lpCOnfigData,               \
                ULONG                       ulTopPage) IPURE;           \
    MAPIMETHOD(CopyMessages)                                            \
        (THIS_  LPCIID                      lpSrcInterface,             \
                LPVOID                      lpSrcFolder,                \
                LPENTRYLIST                 lpMsgList,                  \
                LPCIID                      lpDestInterface,            \
                LPVOID                      lpDestFolder,               \
                ULONG                       ulUIParam,                  \
                LPMAPIPROGRESS              lpProgress,                 \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(CopyFolder)                                              \
        (THIS_  LPCIID                      lpSrcInterface,             \
                LPVOID                      lpSrcFolder,                \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpDestInterface,            \
                LPVOID                      lpDestFolder,               \
                LPTSTR                      lszNewFolderName,           \
                ULONG                       ulUIParam,                  \
                LPMAPIPROGRESS              lpProgress,                 \
                ULONG                       ulFlags) IPURE;             \

#define MAPI_IMAPISUPPORT_METHODS3(IPURE)                               \
    MAPIMETHOD(DoCopyTo)                                                \
        (THIS_  LPCIID                      lpSrcInterface,             \
                LPVOID                      lpSrcObj,                   \
                ULONG                       ciidExclude,                \
                LPCIID                      rgiidExclude,               \
                LPSPropTagArray             lpExcludeProps,             \
                ULONG                       ulUIParam,                  \
                LPMAPIPROGRESS              lpProgress,                 \
                LPCIID                      lpDestInterface,            \
                LPVOID                      lpDestObj,                  \
                ULONG                       ulFlags,                    \
                LPSPropProblemArray FAR *   lppProblems) IPURE;         \
    MAPIMETHOD(DoCopyProps)                                             \
        (THIS_  LPCIID                      lpSrcInterface,             \
                LPVOID                      lpSrcObj,                   \
                LPSPropTagArray             lpIncludeProps,             \
                ULONG                       ulUIParam,                  \
                LPMAPIPROGRESS              lpProgress,                 \
                LPCIID                      lpDestInterface,            \
                LPVOID                      lpDestObj,                  \
                ULONG                       ulFlags,                    \
                LPSPropProblemArray FAR *   lppProblems) IPURE;         \
    MAPIMETHOD(DoProgressDialog)                                        \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags,                    \
                LPMAPIPROGRESS FAR *        lppProgress) IPURE;         \
    MAPIMETHOD(ReadReceipt)                                             \
        (THIS_  ULONG                       ulFlags,                    \
                LPMESSAGE                   lpReadMessage,              \
                LPMESSAGE FAR *             lppEmptyMessage) IPURE;     \
    MAPIMETHOD(PrepareSubmit)                                           \
        (THIS_  LPMESSAGE                   lpMessage,                  \
                ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(ExpandRecips)                                            \
        (THIS_  LPMESSAGE                   lpMessage,                  \
                ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(UpdatePAB)                                               \
        (THIS_  ULONG                       ulFlags,                    \
                LPMESSAGE                   lpMessage) IPURE;           \
    MAPIMETHOD(DoSentMail)                                              \
        (THIS_  ULONG                       ulFlags,                    \
                LPMESSAGE                   lpMessage) IPURE;           \
    MAPIMETHOD(OpenAddressBook)                                         \
        (THIS_  LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPADRBOOK FAR *             lppAdrBook) IPURE;          \
    MAPIMETHOD(Preprocess)                                              \
        (THIS_  ULONG                       ulFlags,                    \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID) IPURE;           \
    MAPIMETHOD(CompleteMsg)                                             \
        (THIS_  ULONG                       ulFlags,                    \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID) IPURE;           \
    MAPIMETHOD(StoreLogoffTransports)                                   \
        (THIS_  ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(StatusRecips)                                            \
        (THIS_  LPMESSAGE                   lpMessage,                  \
                LPADRLIST                   lpRecipList) IPURE;         \
    MAPIMETHOD(WrapStoreEntryID)                                        \
        (THIS_  ULONG                       cbOrigEntry,                \
                LPENTRYID                   lpOrigEntry,                \
                ULONG FAR *                 lpcbWrappedEntry,           \
                LPENTRYID FAR *             lppWrappedEntry) IPURE;     \
    MAPIMETHOD(ModifyProfile)                                           \
        (THIS_  ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(IStorageFromStream)                                      \
        (THIS_  LPUNKNOWN                   lpUnkIn,                    \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                LPSTORAGE FAR *             lppStorageOut) IPURE;       \
    MAPIMETHOD(GetSvcConfigSupportObj)                                  \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPISUP FAR *             lppSvcSupport) IPURE;       \

#undef       INTERFACE
#define      INTERFACE  IMAPISupport
DECLARE_MAPI_INTERFACE_(IMAPISupport, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPISUPPORT_METHODS1(PURE)
    MAPI_IMAPISUPPORT_METHODS2(PURE)
    MAPI_IMAPISUPPORT_METHODS3(PURE)
};


/********************************************************************/
/*                                                                  */
/*                          ADDRESS BOOK SPI                        */
/*                                                                  */
/********************************************************************/

/* Address Book Provider ------------------------------------------------- */

/* OpenTemplateID() */
#define FILL_ENTRY              ((ULONG) 0x00000001)

/* For Logon() */

/*#define AB_NO_DIALOG          ((ULONG) 0x00000001) in mapidefs.h */
/*#define MAPI_UNICODE          ((ULONG) 0x80000000) in mapidefs.h */



DECLARE_MAPI_INTERFACE_PTR(IABProvider, LPABPROVIDER);
DECLARE_MAPI_INTERFACE_PTR(IABLogon,    LPABLOGON);

#define MAPI_IABPROVIDER_METHODS(IPURE)                                 \
    MAPIMETHOD(Shutdown)                                                \
        (THIS_  ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(Logon)                                                   \
        (THIS_  LPMAPISUP                   lpMAPISup,                  \
                ULONG                       ulUIParam,                  \
                LPTSTR                      lpszProfileName,            \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulpcbSecurity,            \
                LPBYTE FAR *                lppbSecurity,               \
                LPMAPIERROR FAR *           lppMAPIError,               \
                LPABLOGON FAR *             lppABLogon) IPURE;          \

#undef       INTERFACE
#define      INTERFACE  IABProvider
DECLARE_MAPI_INTERFACE_(IABProvider, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IABPROVIDER_METHODS(PURE)
};

/* For GetOneOffTable() */
/****** MAPI_UNICODE            ((ULONG) 0x80000000) */

#define MAPI_IABLOGON_METHODS(IPURE)                                    \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(Logoff)                                                  \
        (THIS_  ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(OpenEntry)                                               \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPUNKNOWN FAR *             lppUnk) IPURE;              \
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
    MAPIMETHOD(OpenStatusEntry)                                         \
        (THIS_  LPCIID                       lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPMAPISTATUS FAR *          lppEntry) IPURE;            \
    MAPIMETHOD(OpenTemplateID)                                          \
        (THIS_  ULONG                       cbTemplateID,               \
                LPENTRYID                   lpTemplateID,               \
                ULONG                       ulTemplateFlags,            \
                LPMAPIPROP                  lpMAPIPropData,             \
                LPCIID                       lpInterface,                \
                LPMAPIPROP FAR *            lppMAPIPropNew,             \
                LPMAPIPROP                  lpMAPIPropSibling) IPURE;   \
    MAPIMETHOD(GetOneOffTable)                                          \
        (THIS_  ULONG                       ulFlags,                    \
                LPMAPITABLE FAR *           lppTable) IPURE;            \
    MAPIMETHOD(PrepareRecips)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPSPropTagArray             lpPropTagArray,             \
                LPADRLIST                   lpRecipList) IPURE;         \

#undef       INTERFACE
#define      INTERFACE  IABLogon
DECLARE_MAPI_INTERFACE_(IABLogon, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IABLOGON_METHODS(PURE)
};

typedef HRESULT (STDMAPIINITCALLTYPE ABPROVIDERINIT)(
    HINSTANCE           hInstance,
    LPMALLOC            lpMalloc,
    LPALLOCATEBUFFER    lpAllocateBuffer,
    LPALLOCATEMORE      lpAllocateMore,
    LPFREEBUFFER        lpFreeBuffer,
    ULONG               ulFlags,
    ULONG               ulMAPIVer,
    ULONG FAR *         lpulProviderVer,
    LPABPROVIDER FAR *  lppABProvider
);

ABPROVIDERINIT ABProviderInit;



/********************************************************************/
/*                                                                  */
/*                          TRANSPORT SPI                           */
/*                                                                  */
/********************************************************************/

/* For DeinitTransport */

#define DEINIT_NORMAL               ((ULONG) 0x00000001)
#define DEINIT_HURRY                ((ULONG) 0x80000000)

/* For TransportLogon */

/* Flags that the Spooler may pass to the transport: */

#define LOGON_NO_DIALOG             ((ULONG) 0x00000001)
#define LOGON_NO_CONNECT            ((ULONG) 0x00000004)
#define LOGON_NO_INBOUND            ((ULONG) 0x00000008)
#define LOGON_NO_OUTBOUND           ((ULONG) 0x00000010)
/*#define MAPI_UNICODE              ((ULONG) 0x80000000) in mapidefs.h */

/* Flags that the transport may pass to the Spooler: */

#define LOGON_SP_IDLE               ((ULONG) 0x00010000)
#define LOGON_SP_POLL               ((ULONG) 0x00020000)
#define LOGON_SP_RESOLVE            ((ULONG) 0x00040000)


DECLARE_MAPI_INTERFACE_PTR(IXPProvider, LPXPPROVIDER);
DECLARE_MAPI_INTERFACE_PTR(IXPLogon, LPXPLOGON);

#define MAPI_IXPPROVIDER_METHODS(IPURE)                                 \
    MAPIMETHOD(Shutdown)                                                \
        (THIS_  ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(TransportLogon)                                          \
        (THIS_  LPMAPISUP                   lpMAPISup,                  \
                ULONG                       ulUIParam,                  \
                LPTSTR                      lpszProfileName,            \
                ULONG FAR *                 lpulFlags,                  \
                LPMAPIERROR FAR *           lppMAPIError,               \
                LPXPLOGON FAR *             lppXPLogon) IPURE;          \

#undef       INTERFACE
#define      INTERFACE  IXPProvider
DECLARE_MAPI_INTERFACE_(IXPProvider, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IXPPROVIDER_METHODS(PURE)
};

/* OptionData returned from call to RegisterOptions */

#define OPTION_TYPE_RECIPIENT       ((ULONG) 0x00000001)
#define OPTION_TYPE_MESSAGE         ((ULONG) 0x00000002)

typedef struct _OPTIONDATA
{
    ULONG           ulFlags;        /* MAPI_RECIPIENT, MAPI_MESSAGE */
    LPGUID          lpRecipGUID;    /* Same as returned by AddressTypes() */
    LPTSTR          lpszAdrType;    /* Same as returned by AddressTypes() */
    LPTSTR          lpszDLLName;    /* Options DLL */
    ULONG           ulOrdinal;      /* Ordinal in that DLL */
    ULONG           cbOptionsData;  /* Count of bytes in lpbOptionsData */
    LPBYTE          lpbOptionsData; /* Providers per [recip|message] option data */
    ULONG           cOptionsProps;  /* Count of Options default prop values */
    LPSPropValue    lpOptionsProps; /* Default Options property values */
} OPTIONDATA, FAR *LPOPTIONDATA;

typedef SCODE (STDMAPIINITCALLTYPE OPTIONCALLBACK)(
            HINSTANCE           hInst,
            LPMALLOC            lpMalloc,
            ULONG               ulFlags,
            ULONG               cbOptionData,
            LPBYTE              lpbOptionData,
            LPMAPISUP           lpMAPISup,
            LPMAPIPROP          lpDataSource,
            LPMAPIPROP FAR *    lppWrappedSource,
            LPMAPIERROR FAR *   lppMAPIError);

/* For XP_AddressTypes */

/*#define MAPI_UNICODE              ((ULONG) 0x80000000) in mapidefs.h */

/* For XP_RegisterRecipOptions */

/*#define MAPI_UNICODE              ((ULONG) 0x80000000) in mapidefs.h */

/* For XP_RegisterMessageOptions */

/*#define MAPI_UNICODE              ((ULONG) 0x80000000) in mapidefs.h */

/* For TransportNotify */

#define NOTIFY_ABORT_DEFERRED       ((ULONG) 0x40000000)
#define NOTIFY_CANCEL_MESSAGE       ((ULONG) 0x80000000)
#define NOTIFY_BEGIN_INBOUND        ((ULONG) 0x00000001)
#define NOTIFY_END_INBOUND          ((ULONG) 0x00010000)
#define NOTIFY_BEGIN_OUTBOUND       ((ULONG) 0x00000002)
#define NOTIFY_END_OUTBOUND         ((ULONG) 0x00020000)
#define NOTIFY_BEGIN_INBOUND_FLUSH  ((ULONG) 0x00000004)
#define NOTIFY_END_INBOUND_FLUSH    ((ULONG) 0x00040000)
#define NOTIFY_BEGIN_OUTBOUND_FLUSH ((ULONG) 0x00000008)
#define NOTIFY_END_OUTBOUND_FLUSH   ((ULONG) 0x00080000)

/* For TransportLogoff */

#define LOGOFF_NORMAL               ((ULONG) 0x00000001)
#define LOGOFF_HURRY                ((ULONG) 0x80000000)

/* For SubmitMessage */

#define BEGIN_DEFERRED              ((ULONG) 0x00000001)

/* For EndMessage */

/* Flags that the Spooler may pass to the Transport: */

/* Flags that the transport may pass to the Spooler: */

#define END_RESEND_NOW              ((ULONG) 0x00010000)
#define END_RESEND_LATER            ((ULONG) 0x00020000)
#define END_DONT_RESEND             ((ULONG) 0x00040000)

#define MAPI_IXPLOGON_METHODS(IPURE)                                    \
    MAPIMETHOD(AddressTypes)                                            \
        (THIS_  ULONG FAR *                 lpulFlags,                  \
                ULONG FAR *                 lpcAdrType,                 \
                LPTSTR FAR * FAR *          lpppAdrTypeArray,           \
                ULONG FAR *                 lpcMAPIUID,                 \
                LPMAPIUID FAR * FAR *       lpppUIDArray) IPURE;        \
    MAPIMETHOD(RegisterOptions)                                         \
        (THIS_  ULONG FAR *                 lpulFlags,                  \
                ULONG FAR *                 lpcOptions,                 \
                LPOPTIONDATA FAR *          lppOptions) IPURE;          \
    MAPIMETHOD(TransportNotify)                                         \
        (THIS_  ULONG FAR *                 lpulFlags,                  \
                LPVOID FAR *                lppvData) IPURE;            \
    MAPIMETHOD(Idle)                                                    \
        (THIS_  ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(TransportLogoff)                                         \
        (THIS_  ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(SubmitMessage)                                           \
        (THIS_  ULONG                       ulFlags,                    \
                LPMESSAGE                   lpMessage,                  \
                ULONG FAR *                 lpulMsgRef,                 \
                ULONG FAR *                 lpulReturnParm) IPURE;      \
    MAPIMETHOD(EndMessage)                                              \
        (THIS_  ULONG                       ulMsgRef,                   \
                ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(Poll)                                                    \
        (THIS_  ULONG FAR *                 lpulIncoming) IPURE;        \
    MAPIMETHOD(StartMessage)                                            \
        (THIS_  ULONG                       ulFlags,                    \
                LPMESSAGE                   lpMessage,                  \
                ULONG FAR *                 lpulMsgRef) IPURE;          \
    MAPIMETHOD(OpenStatusEntry)                                         \
        (THIS_  LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPMAPISTATUS FAR *          lppEntry) IPURE;            \
    MAPIMETHOD(ValidateState)                                           \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       ulFlags) IPURE;             \
    MAPIMETHOD(FlushQueues)                                             \
        (THIS_  ULONG                       ulUIParam,                  \
                ULONG                       cbTargetTransport,          \
                LPENTRYID                   lpTargetTransport,          \
                ULONG                       ulFlags) IPURE;             \

#undef       INTERFACE
#define      INTERFACE  IXPLogon
DECLARE_MAPI_INTERFACE_(IXPLogon, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IXPLOGON_METHODS(PURE)
};


/* Transport Provider Entry Point */

typedef HRESULT (STDMAPIINITCALLTYPE XPPROVIDERINIT)(
    HINSTANCE           hInstance,
    LPMALLOC            lpMalloc,
    LPALLOCATEBUFFER    lpAllocateBuffer,
    LPALLOCATEMORE      lpAllocateMore,
    LPFREEBUFFER        lpFreeBuffer,
    ULONG               ulFlags,
    ULONG               ulMAPIVer,
    ULONG FAR *         lpulProviderVer,
    LPXPPROVIDER FAR *  lppXPProvider);

XPPROVIDERINIT XPProviderInit;

/********************************************************************/
/*                                                                  */
/*                          MESSAGE STORE SPI                       */
/*                                                                  */
/********************************************************************/

/* Flags and enums */

/* For Logon() */

/*#define MAPI_UNICODE          ((ULONG) 0x80000000) in mapidefs.h */
/*#define MDB_NO_DIALOG         ((ULONG) 0x00000001) in mapidefs.h */
/*#define MDB_WRITE             ((ULONG) 0x00000004) in mapidefs.h */
/*#define MAPI_DEFERRED_ERRORS  ((ULONG) 0x00000008) in mapidefs.h */
/*#define MDB_TEMPORARY         ((ULONG) 0x00000020) in mapidefs.h */
/*#define MDB_NO_MAIL           ((ULONG) 0x00000080) in mapidefs.h */

/* For SpoolerLogon() */

/*#define MAPI_UNICODE          ((ULONG) 0x80000000) in mapidefs.h */
/*#define MDB_NO_DIALOG         ((ULONG) 0x00000001) in mapidefs.h */
/*#define MDB_WRITE             ((ULONG) 0x00000004) in mapidefs.h */
/*#define MAPI_DEFERRED_ERRORS  ((ULONG) 0x00000008) in mapidefs.h */

/* GetCredentials, SetCredentials */

#define LOGON_SP_TRANSPORT      ((ULONG) 0x00000001)
#define LOGON_SP_PROMPT         ((ULONG) 0x00000002)
#define LOGON_SP_NEWPW          ((ULONG) 0x00000004)
#define LOGON_CHANGED           ((ULONG) 0x00000008)

/* DoMCDialog */

#define DIALOG_FOLDER           ((ULONG) 0x00000001)
#define DIALOG_MESSAGE          ((ULONG) 0x00000002)
#define DIALOG_PROP             ((ULONG) 0x00000004)
#define DIALOG_ATTACH           ((ULONG) 0x00000008)

#define DIALOG_MOVE             ((ULONG) 0x00000010)
#define DIALOG_COPY             ((ULONG) 0x00000020)
#define DIALOG_DELETE           ((ULONG) 0x00000040)

#define DIALOG_ALLOW_CANCEL     ((ULONG) 0x00000080)
#define DIALOG_CONFIRM_CANCEL   ((ULONG) 0x00000100)

/* ExpandRecips */

#define NEEDS_PREPROCESSING     ((ULONG) 0x00000001)
#define NEEDS_SPOOLER           ((ULONG) 0x00000002)

/* PrepareSubmit */

#define CHECK_SENDER            ((ULONG) 0x00000001)
#define NON_STANDARD            ((ULONG) 0x00010000)


DECLARE_MAPI_INTERFACE_PTR(IMSLogon, LPMSLOGON);
DECLARE_MAPI_INTERFACE_PTR(IMSProvider, LPMSPROVIDER);

/* Message Store Provider Interface (IMSPROVIDER) */

#define MAPI_IMSPROVIDER_METHODS(IPURE)                                 \
    MAPIMETHOD(Shutdown)                                                \
        (THIS_  ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(Logon)                                                   \
        (THIS_  LPMAPISUP                   lpMAPISup,                  \
                ULONG                       ulUIParam,                  \
                LPTSTR                      lpszProfileName,            \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                ULONG                       ulFlags,                    \
                LPCIID                      lpInterface,                \
                ULONG FAR *                 lpcbSpoolSecurity,          \
                LPBYTE FAR *                lppbSpoolSecurity,          \
                LPMAPIERROR FAR *           lppMAPIError,               \
                LPMSLOGON FAR *             lppMSLogon,                 \
                LPMDB FAR *                 lppMDB) IPURE;              \
    MAPIMETHOD(SpoolerLogon)                                            \
        (THIS_  LPMAPISUP                   lpMAPISup,                  \
                ULONG                       ulUIParam,                  \
                LPTSTR                      lpszProfileName,            \
                ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                ULONG                       ulFlags,                    \
                LPCIID                      lpInterface,                \
                ULONG                       cbSpoolSecurity,            \
                LPBYTE                      lpbSpoolSecurity,           \
                LPMAPIERROR FAR *           lppMAPIError,               \
                LPMSLOGON FAR *             lppMSLogon,                 \
                LPMDB FAR *                 lppMDB) IPURE;              \
    MAPIMETHOD(CompareStoreIDs)                                         \
        (THIS_  ULONG                       cbEntryID1,                 \
                LPENTRYID                   lpEntryID1,                 \
                ULONG                       cbEntryID2,                 \
                LPENTRYID                   lpEntryID2,                 \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulResult) IPURE;          \

#undef       INTERFACE
#define      INTERFACE  IMSProvider
DECLARE_MAPI_INTERFACE_(IMSProvider, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMSPROVIDER_METHODS(PURE)
};

/* The MSLOGON object is returned by the Logon() method of the
 * MSPROVIDER interface.  This object is for use by MAPIX.DLL.
 */
#define MAPI_IMSLOGON_METHODS(IPURE)                                    \
    MAPIMETHOD(GetLastError)                                            \
        (THIS_  HRESULT                     hResult,                    \
                ULONG                       ulFlags,                    \
                LPMAPIERROR FAR *           lppMAPIError) IPURE;        \
    MAPIMETHOD(Logoff)                                                  \
        (THIS_  ULONG FAR *                 lpulFlags) IPURE;           \
    MAPIMETHOD(OpenEntry)                                               \
        (THIS_  ULONG                       cbEntryID,                  \
                LPENTRYID                   lpEntryID,                  \
                LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPUNKNOWN FAR *             lppUnk) IPURE;              \
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
    MAPIMETHOD(OpenStatusEntry)                                         \
        (THIS_  LPCIID                      lpInterface,                \
                ULONG                       ulFlags,                    \
                ULONG FAR *                 lpulObjType,                \
                LPVOID FAR *                lppEntry) IPURE;            \

#undef       INTERFACE
#define      INTERFACE  IMSLogon
DECLARE_MAPI_INTERFACE_(IMSLogon, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMSLOGON_METHODS(PURE)
};

/* Message Store Provider Entry Point */

typedef HRESULT (STDMAPIINITCALLTYPE MSPROVIDERINIT)(
    HINSTANCE               hInstance,
    LPMALLOC                lpMalloc,           /* AddRef() if you keep it */
    LPALLOCATEBUFFER        lpAllocateBuffer,   /* -> AllocateBuffer */
    LPALLOCATEMORE          lpAllocateMore,     /* -> AllocateMore   */
    LPFREEBUFFER            lpFreeBuffer,       /* -> FreeBuffer     */
    ULONG                   ulFlags,
    ULONG                   ulMAPIVer,
    ULONG FAR *             lpulProviderVer,
    LPMSPROVIDER FAR *      lppMSProvider
);

MSPROVIDERINIT MSProviderInit;


/********************************************************************/
/*                                                                  */
/*                    MESSAGE SERVICE CONFIGURATION                 */
/*                                                                  */
/********************************************************************/

/* Flags for service configuration entry point */

/* #define MAPI_UNICODE              0x80000000 */
/* #define SERVICE_UI_ALWAYS         0x00000002 */
/* #define SERVICE_UI_ALLOWED        0x00000010 */
#define MSG_SERVICE_UI_READ_ONLY     0x00000008 /* display parameters only */
#define SERVICE_LOGON_FAILED         0x00000020 /* reconfigure provider */

/* Contexts for service configuration entry point */

#define MSG_SERVICE_INSTALL         0x00000001
#define MSG_SERVICE_CREATE          0x00000002
#define MSG_SERVICE_CONFIGURE       0x00000003
#define MSG_SERVICE_DELETE          0x00000004
#define MSG_SERVICE_UNINSTALL       0x00000005
#define MSG_SERVICE_PROVIDER_CREATE 0x00000006
#define MSG_SERVICE_PROVIDER_DELETE 0x00000007

/* Prototype for service configuration entry point */

typedef HRESULT (STDAPICALLTYPE MSGSERVICEENTRY)(
    HINSTANCE       hInstance,
    LPMALLOC        lpMalloc,
    LPMAPISUP       lpMAPISup,
    ULONG           ulUIParam,
    ULONG           ulFlags,
    ULONG           ulContext,
    ULONG           cValues,
    LPSPropValue    lpProps,
    LPPROVIDERADMIN lpProviderAdmin,
    LPMAPIERROR FAR *lppMapiError
);
typedef MSGSERVICEENTRY FAR *LPMSGSERVICEENTRY;


#ifdef __cplusplus
}
#endif

#endif /* MAPISPI_H */
