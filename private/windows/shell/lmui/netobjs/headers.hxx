extern "C"
{
// Definitions for code to find if an NT machine is in a domain
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <windows.h>
#include <windowsx.h>
#include <wchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include <lm.h>
#include <shlobj.h>
#include <debug.h>
#include <messages.h>

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

////////////////////////////////////////////////////////////////////////////
//
// global variables
//

extern UINT         g_NonOLEDLLRefs;
extern HINSTANCE    g_hInstance;

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
    DECLARE_DEBUG(NetObjectsUI)

    #define appDebugOut(x) NetObjectsUIInlineDebugOut x
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

#define SIG_CNetObj                 0xabcdef00
#define SIG_CPage                   0xabcdef01

#endif // DBG == 1
