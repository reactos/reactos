#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <ole2.h>
#include <wchar.h>
#include <lm.h>
#include <macfile.h>
#include <fpnwcomm.h>
#include <fpnwapi.h>

#include <shellapi.h>   // public
#include <shlobj.h>     // public
#include <shsemip.h>    // shell semi-private
#include <shlobjp.h>    // shell private
#include <shellp.h>     // shell private

extern "C"
{
#include <icanon.h>
}

#include <messages.h>

#include <debug.h>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Debugging stuff
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
// Fix the warning levels
//

#pragma warning(3:4092)     // sizeof returns 'unsigned long'
#pragma warning(3:4121)     // structure is sensitive to alignment
#pragma warning(3:4125)     // decimal digit in octal sequence
#pragma warning(3:4130)     // logical operation on address of string constant
#pragma warning(3:4132)     // const object should be initialized
#pragma warning(4:4200)     // nonstandard zero-sized array extension
#pragma warning(4:4206)     // Source File is empty
#pragma warning(3:4208)     // delete[exp] - exp evaluated but ignored
#pragma warning(3:4212)     // function declaration used ellipsis
#pragma warning(3:4220)     // varargs matched remaining parameters
#pragma warning(4:4509)     // SEH used in function w/ _trycontext
#pragma warning(error:4700) // Local used w/o being initialized
#pragma warning(3:4706)     // assignment w/i conditional expression
#pragma warning(3:4709)     // command operator w/o index expression

//////////////////////////////////////////////////////////////////////////////

#if DBG == 1
    DECLARE_DEBUG(Sharing)

    #define appDebugOut(x) SharingInlineDebugOut x
    #define appAssert(x)   Win4Assert(x)

    #define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** ERROR RETURN <%s @line %d> -> 0x%08lx\n", \
                __FILE__, __LINE__, hr)); \
        }

    #define CHECK_NEW(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** NULL POINTER (OUT OF MEMORY!) <%s @line %d>\n", \
                __FILE__, __LINE__)); \
        }

    #define CHECK_NULL(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_ERROR, \
                "**** NULL POINTER <%s @line %d>: %s\n", \
                __FILE__, __LINE__, #p)); \
        }

    #define CHECK_THIS  appAssert(NULL != this && "'this' pointer is NULL")

    #define DECLARE_SIG     ULONG __sig
    #define INIT_SIG(class) __sig = SIG_##class
    #define CHECK_SIG(class)  \
              appAssert((NULL != this) && "'this' pointer is NULL"); \
              appAssert((SIG_##class == __sig) && "Signature doesn't match")

#else // DBG == 1

    #define appDebugOut(x)
    #define appAssert(x)
    #define CHECK_HRESULT(hr)
    #define CHECK_NEW(p)
    #define CHECK_NULL(p)
    #define CHECK_THIS

    #define DECLARE_SIG
    #define INIT_SIG(class)
    #define CHECK_SIG(class)

#endif // DBG == 1

#if DBG == 1

#define SIG_CSharingPropertyPage          0xabcdef00
#define SIG_CSfmPropertyPage              0xabcdef01
#define SIG_CShareInfo                    0xabcdef02
#define SIG_CDlgNewShare                  0xabcdef03
#define SIG_CShareHashBucketElem          0xabcdef04
#define SIG_CShareHashBucket              0xabcdef05
#define SIG_CShareHashTable               0xabcdef06
#define SIG_CShare                        0xabcdef07
#define SIG_CBuffer                       0xabcdef08
#define SIG_CFpnwPropertyPage             0xabcdef09

#endif // DBG == 1

////////////////////////////////////////////////////////////////////////////
//
// macros
//

#ifndef offsetof
#define offsetof(type,field) ((size_t)&(((type*)0)->field))
#endif

#define IMPL(class,member,pointer) \
    (&((class*)0)->member == pointer, \
    ((class*)(((long)pointer) - offsetof(class,member))))

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

////////////////////////////////////////////////////////////////////////////
//
// Hard-coded constants: user limit on shares
//
// Note: the maximum number of users on the workstation is hard-coded in the
// server as 10. The max number on the server is essentially a dword, but we
// are using the common up/down control, which only supports a word value.
//
// Note that DEFAULT_MAX_USERS must be <= both the server and workstation
// maximums!

#define MAX_USERS_ON_WORKSTATION 10
#define MAX_USERS_ON_SERVER      UD_MAXVAL

#define DEFAULT_MAX_USERS        10

////////////////////////////////////////////////////////////////////////////
//
// Dynamically loaded API typedefs
//

//
// SFM APIs (from macfile.h)
//

typedef
DWORD
(*PFNAfpAdminConnect)(
        IN  LPWSTR              lpwsServerName,
        OUT PAFP_SERVER_HANDLE  phAfpServer
);

typedef
VOID
(*PFNAfpAdminDisconnect)(
        IN  AFP_SERVER_HANDLE   hAfpServer
);

typedef
VOID
(*PFNAfpAdminBufferFree)(
        IN PVOID                pBuffer
);

typedef
DWORD
(*PFNAfpAdminVolumeEnum)(
        IN  AFP_SERVER_HANDLE   hAfpServer,
        OUT LPBYTE *            lpbBuffer,
        IN  DWORD               dwPrefMaxLen,
        OUT LPDWORD             lpdwEntriesRead,
        OUT LPDWORD             lpdwTotalEntries,
        IN  LPDWORD             lpdwResumeHandle
);

typedef
DWORD
(*PFNAfpAdminVolumeSetInfo)(
        IN  AFP_SERVER_HANDLE   hAfpServer,
        IN  LPBYTE              pBuffer,
        IN  DWORD               dwParmNum
);

typedef
DWORD
(*PFNAfpAdminVolumeGetInfo)(
        IN  AFP_SERVER_HANDLE   hAfpServer,
        IN  LPWSTR              lpwsVolumeName,
        OUT LPBYTE *            lpbBuffer
);

typedef
DWORD
(*PFNAfpAdminVolumeDelete)(
        IN AFP_SERVER_HANDLE    hAfpServer,
        IN LPWSTR               lpwsVolumeName
);

//
// FPNW APIs (from fpnwapi.h)
//

typedef
DWORD
(*PFNFpnwVolumeAdd)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);

typedef
DWORD
(*PFNFpnwVolumeDel)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);

typedef
DWORD
(*PFNFpnwVolumeEnum)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);

typedef
DWORD
(*PFNFpnwVolumeGetInfo)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
);

typedef
DWORD
(*PFNFpnwVolumeSetInfo)(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);

typedef
DWORD
(*PFNFpnwApiBufferFree)(
    IN  LPVOID pBuffer
);

////////////////////////////////////////////////////////////////////////////
//
// global variables
//

extern HINSTANCE    g_hInstance;
extern UINT         g_NonOLEDLLRefs;
extern UINT         g_uiMaxUsers;   // max number of users based on product type
extern WCHAR        g_szAdminShare[]; // ADMIN$
extern WCHAR        g_szIpcShare[];   // IPC$
extern UINT         g_cfHIDA;

extern HINSTANCE                g_hSFMApis;
extern HINSTANCE                g_hFPNWApis;
extern PFNAfpAdminConnect       g_pfnAfpAdminConnect;
extern PFNAfpAdminDisconnect    g_pfnAfpAdminDisconnect;
extern PFNAfpAdminBufferFree    g_pfnAfpAdminBufferFree;
extern PFNAfpAdminVolumeEnum    g_pfnAfpAdminVolumeEnum;
extern PFNAfpAdminVolumeSetInfo g_pfnAfpAdminVolumeSetInfo;
extern PFNAfpAdminVolumeGetInfo g_pfnAfpAdminVolumeGetInfo;
extern PFNAfpAdminVolumeDelete  g_pfnAfpAdminVolumeDelete;
extern PFNFpnwVolumeAdd         g_pfnFpnwVolumeAdd;
extern PFNFpnwVolumeDel         g_pfnFpnwVolumeDel;
extern PFNFpnwVolumeEnum        g_pfnFpnwVolumeEnum;
extern PFNFpnwVolumeGetInfo     g_pfnFpnwVolumeGetInfo;
extern PFNFpnwVolumeSetInfo     g_pfnFpnwVolumeSetInfo;
extern PFNFpnwApiBufferFree     g_pfnFpnwApiBufferFree;

//////////////////////////////////////////////////////////////////////////////

enum COLUMNS1
{
    ICOL1_NAME = 0,
    ICOL1_COMMENT,
    ICOL1_MAX
};

enum COLUMNS2
{
    ICOL2_NAME = 0,
    ICOL2_COMMENT,
    ICOL2_PATH,
    ICOL2_MAXUSES,
    ICOL2_SERVICE,
    ICOL2_MAX
};

//////////////////////////////////////////////////////////////////////////////
