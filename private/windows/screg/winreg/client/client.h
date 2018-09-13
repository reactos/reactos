/*++


Copyright (c) 1992 Microsoft Corporation

Module Name:

    client.h

Abstract:

    This module is the header file for the client side of the Win32 DLL

Author:

    Ramon J. San Andres (ramonsa) 13-May-1992

--*/

#if DBG
    extern BOOLEAN  BreakPointOnEntry;
#endif

//
// Macros to manage local versus remote handles (HKEYs), as
// well as class registration keys from HKEY_CLASSES_ROOT
//

#define REMOTE_HANDLE_TAG    ( 0x00000001 )

#define REG_CLASSES_MASK     ( 0x00000003 )
#define REG_CLASSES_SPECIAL_TAG ( 0x00000002 )

//
//  BOOL
//  IsLocalHandle(
//      IN HKEY Handle
//      );
//

#define IsLocalHandle( Handle )                                         \
    ( ! ((( DWORD_PTR )( Handle )) & REMOTE_HANDLE_TAG ))

//
//  BOOL
//  IsSpeciaClassesHandle(
//      IN HKEY Handle
//      );
//

#define IsSpecialClassesHandle( Handle )                                 \
    ( ((( DWORD_PTR )( Handle )) & REG_CLASSES_SPECIAL_TAG ))


//
//  VOID
//  TagRemoteHandle(
//      IN PHKEY Handle
//      );
//

#define TagRemoteHandle( Handle )                                       \
    ASSERT( IsLocalHandle( *Handle ));                                  \
    ( *Handle = (( HKEY )((( DWORD_PTR )( *Handle )) | REMOTE_HANDLE_TAG )))

//
//  HKEY
//  DereferenceRemoteHandle(
//      IN HKEY Handle
//      );
//

#define DereferenceRemoteHandle( Handle )                               \
    (( HKEY )((( DWORD_PTR )( Handle )) & ~REMOTE_HANDLE_TAG ))

//
//  HKEY
//  TagSpecialClassesHandle (
//      IN HKEY Handle
//      );
//

#define TagSpecialClassesHandle( Handle )                                       \
    ASSERT( IsLocalHandle( *Handle ));                                  \
    ( *Handle = (( HKEY )((( ULONG_PTR )( *Handle )) | REG_CLASSES_SPECIAL_TAG )))


//
// disable predefined cache not enabled on remote !
//
#define CLOSE_LOCAL_HANDLE(TempHandle)                              \
    if( TempHandle != NULL ) {                                      \
        /* disable cache is not enabled on remote registry */       \
        ASSERT( IsLocalHandle(TempHandle) );                        \
        LocalBaseRegCloseKey(&TempHandle);                          \
    }

//
// ConvertKey is a temporary work around to convert predefined registry
// keys that are not sign extended to their sign extended form for 64-bit
// systems.
//

#if defined(_WIN64)
__inline
VOID
ConvertKey(
    HKEY *Handle
    )

{

    if ((((ULONG_PTR)*Handle & 0xffffffff80000000UI64) == 0x80000000UI64)) {
        *Handle = (HKEY)((ULONG_PTR)*Handle | 0xffffffff80000000UI64);
    }
}
#else
#define ConvertKey(Handle)
#endif


#if defined(LEAK_TRACK)


typedef struct _RegLeakTraceInfo {
    DWORD   dwMaxStackDepth;
    LPTSTR  szSymPath;
    BOOL    bEnableLeakTrack;

} RegLeakTraceInfo;


extern RegLeakTraceInfo g_RegLeakTraceInfo;

#endif // LEAK_TRACK



