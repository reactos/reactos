//====== Assertion/Debug output APIs =================================

#undef Assert
#undef AssertE
#undef AssertMsg
#undef DebugMsg
#undef FullDebugMsg

// Debug mask APIs

// NOTE: You can #define your own DM_* values using bits in the HI BYTE
#define DM_TRACE    0x0001	// Trace messages
#define DM_WARNING  0x0002	// Warning
#define DM_ERROR    0x0004	// Error
#define DM_ASSERT   0x0008	// Assertions

// NOTE: Default debug mask is 0x00ff (show everything)
//
// Inside debugger, you can modify wDebugMask variable.
//
// Set debug mask; returning previous.
//
UINT WINAPI SetDebugMask(UINT mask);

// Get debug mask.
//
UINT WINAPI GetDebugMask();

#ifndef WIN32
#if (defined(BUILDDLL) && !defined(ASSERTSEG))
#define ASSERTSEG _based(_segname("_ERRTEXT"))
#endif

#ifndef ASSERTSEG
#define ASSERTSEG _based(_segname("_CODE"))
#endif
#else
#define ASSERTSEG
#endif

// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
#define DEBUGTEXT(sz, msg)	/* ;Internal */ \
    static const char ASSERTSEG sz[] = msg;

#ifndef NOSHELLDEBUG	// Others have own versions of these.
#ifdef DEBUG

// Assert(f)  -- Generate "assertion failed in line x of file.c"
//		 message if f is NOT true.
//
// AssertMsg(f, msg, args...)  -- Generate wsprintf-formatted msg w/params
//			    if f is NOT true.
//
// DebugMsg(mask, msg, args...) -
//	   Generate wsprintf-formatted msg using
//	   specified debug mask.  System debug mask
//	   governs whether message is output.
//
void WINAPI AssertFailed(LPCSTR szFile, int line);
#define Assert(f)                                 \
    {                                             \
        DEBUGTEXT(szFile, __FILE__);              \
        if (!(f))                                 \
            AssertFailed(szFile, __LINE__);       \
    }
#define AssertE(f) Assert(f)

void __cdecl _AssertMsg(BOOL f, LPCSTR pszMsg, ...);
#define AssertMsg   _AssertMsg

void __cdecl _DebugMsg(UINT mask, LPCSTR psz, ...);
#define DebugMsg    _DebugMsg

#ifdef FULL_DEBUG
#define FullDebugMsg    _DebugMsg
#else
#define FullDebugMsg    1 ? (void)0 : (void)
#endif

#else

#define Assert(f)
#define AssertE(f)	(f)
#define AssertMsg       1 ? (void)0 : (void)
#define DebugMsg        1 ? (void)0 : (void)
#define FullDebugMsg    1 ? (void)0 : (void)

#endif
#endif // NOSHELLDEBUG
