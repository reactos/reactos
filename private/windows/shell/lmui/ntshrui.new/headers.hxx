#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <wchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include <lm.h>
#include <msshrui.h>

extern "C"
{
#include <icanon.h>
#include <winnetp.h>
#include <mpr.h>
}

#include <messages.h>
#include <shlobj.h>

#include <debug.h>

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
// global variables
//

extern UINT         g_NonOLEDLLRefs;
extern HINSTANCE    g_hInstance;
extern WCHAR        g_szShare[];
extern BOOL         g_fSharingEnabled;
extern UINT         g_uiMaxUsers;   // max number of users based on product type

extern WCHAR        g_szAdminShare[]; // ADMIN$
extern WCHAR        g_szIpcShare[];   // IPC$

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
            appDebugOut((DEB_IERROR, \
                "**** ERROR RETURN <%s @line %d> -> 0x%08lx\n", \
                __FILE__, __LINE__, hr)); \
        }

    #define CHECK_NEW(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_IERROR, \
                "**** NULL POINTER (OUT OF MEMORY!) <%s @line %d>\n", \
                __FILE__, __LINE__)); \
        }

    #define CHECK_NULL(p) \
        if ( NULL == (p) ) \
        { \
            appDebugOut((DEB_IERROR, \
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
#define SIG_CShare                        0xabcdef01
#define SIG_CShareInfo                    0xabcdef02
#define SIG_CDlgNewShare                  0xabcdef03
#define SIG_CShareCF                      0xabcdef04
#define SIG_CBuffer                       0xabcdef05
#define SIG_CStrHashBucketElem            0xabcdef06
#define SIG_CStrHashBucket                0xabcdef07
#define SIG_CStrHashTable                 0xabcdef08
#define SIG_CShareCopyHook                0xabcdef09

#endif // DBG == 1
