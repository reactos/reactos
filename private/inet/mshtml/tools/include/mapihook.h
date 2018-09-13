/*
 *  M A P I H O O K . H
 *
 *  Defines the SpoolerMsgHook provider interface.
 *
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef MAPIHOOK_H
#define MAPIHOOK_H

#ifndef MAPIDEFS_H
#include <mapidefs.h>
#include <mapicode.h>
#include <mapiguid.h>
#include <mapitags.h>
#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ISpoolerHook Interface ------------------------------------------------ */

/* MsgHooks */

#define HOOK_DELETE         ((ULONG) 0x00000001)
#define HOOK_CANCEL         ((ULONG) 0x00000002)

#define MAPI_ISPOOLERHOOK_METHODS(IPURE)                            \
    MAPIMETHOD(InboundMsgHook)                                      \
        (THIS_  LPMESSAGE                   lpMessage,              \
                LPMAPIFOLDER                lpFolder,               \
                LPMDB                       lpMDB,                  \
                ULONG FAR *                 lpulFlags,              \
                ULONG FAR *                 lpcbEntryID,            \
                LPBYTE FAR *                lppEntryID) IPURE;      \
    MAPIMETHOD(OutboundMsgHook)                                     \
        (THIS_  LPMESSAGE                   lpMessage,              \
                LPMAPIFOLDER                lpFolder,               \
                LPMDB                       lpMDB,                  \
                ULONG FAR *                 lpulFlags,              \
                ULONG FAR *                 lpcbEntryID,            \
                LPBYTE FAR *                lppEntryID) IPURE;      \
    
#undef       INTERFACE
#define      INTERFACE  ISpoolerHook
DECLARE_MAPI_INTERFACE_(ISpoolerHook, IUnknown)
{
    BEGIN_INTERFACE 
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_ISPOOLERHOOK_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(ISpoolerHook, LPSPOOLERHOOK);

/* Hook Provider Entry Point */

#define HOOK_INBOUND        ((ULONG) 0x00000200)
#define HOOK_OUTBOUND       ((ULONG) 0x00000400)

typedef HRESULT (STDMAPIINITCALLTYPE HPPROVIDERINIT)(
    LPMAPISESSION           lpSession,
    HINSTANCE               hInstance,
    LPALLOCATEBUFFER        lpAllocateBuffer,
    LPALLOCATEMORE          lpAllocateMore,
    LPFREEBUFFER            lpFreeBuffer,
    LPMAPIUID               lpSectionUID,
    ULONG                   ulFlags,
    LPSPOOLERHOOK FAR *     lppSpoolerHook
);

HPPROVIDERINIT HPProviderInit;

#ifdef __cplusplus
}      /* extern "C"  */
#endif /* __cplusplus */

#endif /* MAPIHOOK_H  */
