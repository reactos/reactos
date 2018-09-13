#ifndef __DSKQUOTA_DEBUG_H
#define __DSKQUOTA_DEBUG_H
///////////////////////////////////////////////////////////////////////////////
/*  File: debug.h

    Description: Contains debug output and assertion macros/functions.
        This file was originally copied from the shell's shelldll project
        and modified for the WIN32-only build enviroment of the dskquota 
        project.  

        This code is compiled ONLY when the DEBUG macro is defined.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/06/96    Copied from shelldll project.                        BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#undef Assert
#undef AssertE
#undef AssertMsg
#undef DebugMsg
#undef FullDebugMsg

// Debug mask APIs

#define DM_NONE      0x00000000      // Shhhhh. Be vewy, vewy quiet.
#define DM_TRACE     0x00000001      // Trace messages
#define DM_WARNING   0x00000002      // Warning
#define DM_ERROR     0x00000004      // Error
#define DM_ASSERT    0x00000008      // Assertions
#define DM_ALLOC     0x00000010      // trace/show allocations
#define DM_ALLOCSUM  0x00000020      // Dump alloc summary on DLL unload.
#define DM_REG       0x00000040      // Registry calls
#define DM_CONSTRUCT 0x00000080      // Object constructors and destructors.
#define DM_OLE       0x00000100      // QueryInterface, AddRef, Release.
#define DM_WINDOWMSG 0x00000200      // Windows messages.
#define DM_NOPREFIX  0x20000000      // Don't prepend thread ID or dll name.
#define DM_NONEWLINE 0x40000000      // Don't append /r/n to end of message.
#define DM_NOW       0x80000000      // Just because (temporary).
#define DM_ALL       0xFFFFFFFF      // Hope you like watching text scroll by!

//
// NOTE: Default debug mask is 0 (show nothing)
//
// Inside debugger, you can modify dwDebugMask variable.
//
// Set debug mask; returning previous.
//
extern "C" UINT WINAPI SetDebugMask(UINT mask);

//
// Get debug mask.
//
UINT WINAPI GetDebugMask();

#define ASSERTSEG  // Undefined for WIN32 in original shelldll code.

//
// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)      /* ;Internal */ \
    static const TCHAR ASSERTSEG sz[] = msg;

#ifndef NOSHELLDEBUG    // Others have own versions of these.
#ifdef DEBUG

// Assert(f)  -- Generate "assertion failed in line x of file.c"
//               message if f is NOT true.
//
// AssertMsg(f, msg, args...)  -- Generate wsprintf-formatted msg w/params
//                          if f is NOT true.
//
// DebugMsg(mask, msg, args...) -
//         Generate wsprintf-formatted msg using
//         specified debug mask.  System debug mask
//         governs whether message is output.
//
void WINAPI AssertFailed(LPCTSTR szFile, int line);
#define Assert(f)                                 \
    {                                             \
        DEBUGTEXT(szFile, TEXT(__FILE__));              \
        if (!(f))                                 \
            AssertFailed(szFile, __LINE__);       \
    }
#define AssertE(f) Assert(f)

void __cdecl _AssertMsg(BOOL f, LPCTSTR pszMsg, ...);
#define AssertMsg   _AssertMsg

void __cdecl _DebugMsg(UINT mask, LPCTSTR psz, ...);
#define DebugMsg    _DebugMsg

#ifdef FULL_DEBUG
#define FullDebugMsg    _DebugMsg
#else 
#define FullDebugMsg    1 ? (void)0 : (void)
#endif

#else // !DEBUG

#define Assert(f)
#define AssertE(f)      (f)
#define AssertMsg       1 ? (void)0 : (void)
#define DebugMsg        1 ? (void)0 : (void)
#define FullDebugMsg    1 ? (void)0 : (void)

#endif // DEBUG
#endif // NOSHELLDEBUG
#endif // __DSKQUOTA_DEBUG_H
