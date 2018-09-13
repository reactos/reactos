/***
*ios.h - definitions/declarations for the ios class.
*
*	Copyright (c) 1990-1996, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file defines the classes, values, macros, and functions
*	used by the ios class.
*	[AT&T C++]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus

#ifndef _INC_IOS
#define _INC_IOS

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef	_MSC_VER
// Currently, all MS C compilers for Win32 platforms default to 8 byte
// alignment.
#pragma pack(push,8)

#include <useoldio.h>

#endif	// _MSC_VER

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


#ifdef _MT

typedef struct __CRT_LIST_ENTRY {
   struct __CRT_LIST_ENTRY *Flink;
   struct __CRT_LIST_ENTRY *Blink;
} _CRT_LIST_ENTRY;

typedef struct _CRT_CRITICAL_SECTION_DEBUG {
    unsigned short Type;
    unsigned short CreatorBackTraceIndex;
    struct _CRT_CRITICAL_SECTION *CriticalSection;
    _CRT_LIST_ENTRY ProcessLocksList;
    unsigned long EntryCount;
    unsigned long ContentionCount;
    unsigned long Depth;
    void * OwnerBackTrace[ 5 ];
} _CRT_CRITICAL_SECTION_DEBUG, *_PCRT_CRITICAL_SECTION_DEBUG;

typedef struct _CRT_CRITICAL_SECTION {
    _PCRT_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    long LockCount;
    long RecursionCount;
    void * OwningThread;        // from the thread's ClientId->UniqueThread
    void * LockSemaphore;
    unsigned long Reserved;
} _CRT_CRITICAL_SECTION, *_PCRT_CRITICAL_SECTION;

extern "C" {
_CRTIMP void __cdecl _mtlock(_PCRT_CRITICAL_SECTION);
_CRTIMP void __cdecl _mtunlock(_PCRT_CRITICAL_SECTION);
}

#endif /* _MT */

#ifndef NULL
#define NULL	0
#endif

#ifndef EOF
#define EOF	(-1)
#endif

#ifdef	_MSC_VER
// C4514: "unreferenced inline function has been removed"
#pragma warning(disable:4514) // disable C4514 warning
// #pragma warning(default:4514)	// use this to reenable, if desired
#endif	// _MSC_VER

class _CRTIMP streambuf;
class _CRTIMP ostream;

class _CRTIMP ios {

public:
    enum io_state {  goodbit = 0x00,
		     eofbit  = 0x01,
		     failbit = 0x02,
		     badbit  = 0x04 };

    enum open_mode { in        = 0x01,
		     out       = 0x02,
		     ate       = 0x04,
		     app       = 0x08,
		     trunc     = 0x10,
		     nocreate  = 0x20,
		     noreplace = 0x40,
		     binary    = 0x80 };

    enum seek_dir { beg=0, cur=1, end=2 };

    enum {  skipws     = 0x0001,
	    left       = 0x0002,
	    right      = 0x0004,
	    internal   = 0x0008,
	    dec        = 0x0010,
	    oct        = 0x0020,
	    hex        = 0x0040,
	    showbase   = 0x0080,
	    showpoint  = 0x0100,
	    uppercase  = 0x0200,
	    showpos    = 0x0400,
	    scientific = 0x0800,
	    fixed      = 0x1000,
	    unitbuf    = 0x2000,
	    stdio      = 0x4000
				 };

    static const long basefield;	// dec | oct | hex
    static const long adjustfield;	// left | right | internal
    static const long floatfield;	// scientific | fixed

    ios(streambuf*);			// differs from ANSI
    virtual ~ios();

    inline long flags() const;
    inline long flags(long _l);

    inline long setf(long _f,long _m);
    inline long setf(long _l);
    inline long unsetf(long _l);

    inline int width() const;
    inline int width(int _i);

    inline ostream* tie(ostream* _os);
    inline ostream* tie() const;

    inline char fill() const;
    inline char fill(char _c);

    inline int precision(int _i);
    inline int precision() const;

    inline int rdstate() const;
    inline void clear(int _i = 0);

//  inline operator void*() const;
    operator void *() const { if(state&(badbit|failbit) ) return 0; return (void *)this; }
    inline int operator!() const;

    inline int  good() const;
    inline int  eof() const;
    inline int  fail() const;
    inline int  bad() const;

    inline streambuf* rdbuf() const;

    inline long & iword(int) const;
    inline void * & pword(int) const;

    static long bitalloc();
    static int xalloc();
    static void sync_with_stdio();

#ifdef	_MT
    inline void __cdecl setlock();
    inline void __cdecl clrlock();
    void __cdecl lock() { if (LockFlg<0) _mtlock(lockptr()); };
    void __cdecl unlock() { if (LockFlg<0) _mtunlock(lockptr()); }
    inline void __cdecl lockbuf();
    inline void __cdecl unlockbuf();
#else
    void __cdecl lock() { }
    void __cdecl unlock() { }
    void __cdecl lockbuf() { }
    void __cdecl unlockbuf() { }
#endif

protected:
    ios();
    ios(const ios&);			// treat as private
    ios& operator=(const ios&);
    void init(streambuf*);

    enum { skipping, tied };
    streambuf*	bp;

    int     state;
    int     ispecial;			// not used
    int     ospecial;			// not used
    int     isfx_special;		// not used
    int     osfx_special;		// not used
    int     x_delbuf;			// if set, rdbuf() deleted by ~ios

    ostream* x_tie;
    long    x_flags;
    int     x_precision;
    char    x_fill;
    int     x_width;

    static void (*stdioflush)();	// not used

#ifdef	_MT
    static void lockc() { _mtlock(& x_lockc); }
    static void unlockc() { _mtunlock( & x_lockc); }
    _PCRT_CRITICAL_SECTION lockptr() { return & x_lock; }
#else
    static void lockc() { }
    static void unlockc() { }
#endif

public:
    int	delbuf() const { return x_delbuf; }
    void    delbuf(int _i) { x_delbuf = _i; }

private:
    static long x_maxbit;
    static int x_curindex;
    static int sunk_with_stdio;		// make sure sync_with done only once
#ifdef	_MT
#define MAXINDEX 7
    static long x_statebuf[MAXINDEX+1];  // used by xalloc()
    static int fLockcInit;		// used to see if x_lockc initialized
    static _CRT_CRITICAL_SECTION x_lockc; // used to lock static (class) data members
    int LockFlg;			// enable locking flag
    _CRT_CRITICAL_SECTION x_lock;	// used for multi-thread lock on object
#else
    static long * x_statebuf;  // used by xalloc()
#endif
};

#include <streamb.h>

inline _CRTIMP ios& __cdecl dec(ios& _strm) { _strm.setf(ios::dec,ios::basefield); return _strm; }
inline _CRTIMP ios& __cdecl hex(ios& _strm) { _strm.setf(ios::hex,ios::basefield); return _strm; }
inline _CRTIMP ios& __cdecl oct(ios& _strm) { _strm.setf(ios::oct,ios::basefield); return _strm; }

inline long ios::flags() const { return x_flags; }
inline long ios::flags(long _l){ long _lO; _lO = x_flags; x_flags = _l; return _lO; }

inline long ios::setf(long _l,long _m){ long _lO; lock(); _lO = x_flags; x_flags = (_l&_m) | (x_flags&(~_m)); unlock(); return _lO; }
inline long ios::setf(long _l){ long _lO; lock(); _lO = x_flags; x_flags |= _l; unlock(); return _lO; }
inline long ios::unsetf(long _l){ long _lO; lock(); _lO = x_flags; x_flags &= (~_l); unlock(); return _lO; }

inline int ios::width() const { return x_width; }
inline int ios::width(int _i){ int _iO; _iO = (int)x_width; x_width = _i; return _iO; }

inline ostream* ios::tie(ostream* _os){ ostream* _osO; _osO = x_tie; x_tie = _os; return _osO; }
inline ostream* ios::tie() const { return x_tie; }
inline char ios::fill() const { return x_fill; }
inline char ios::fill(char _c){ char _cO; _cO = x_fill; x_fill = _c; return _cO; }
inline int ios::precision(int _i){ int _iO; _iO = (int)x_precision; x_precision = _i; return _iO; }
inline int ios::precision() const { return x_precision; }

inline int ios::rdstate() const { return state; }

// inline ios::operator void *() const { if(state&(badbit|failbit) ) return 0; return (void *)this; }
inline int ios::operator!() const { return state&(badbit|failbit); }

inline int  ios::bad() const { return state & badbit; }
inline void ios::clear(int _i){ lock(); state = _i; unlock(); }
inline int  ios::eof() const { return state & eofbit; }
inline int  ios::fail() const { return state & (badbit | failbit); }
inline int  ios::good() const { return state == 0; }

inline streambuf* ios::rdbuf() const { return bp; }

inline long & ios::iword(int _i) const { return x_statebuf[_i] ; }
inline void * & ios::pword(int _i) const { return (void * &)x_statebuf[_i]; }

#ifdef	_MT
    inline void ios::setlock() { LockFlg--; if (bp) bp->setlock(); }
    inline void ios::clrlock() { if (LockFlg <= 0) LockFlg++; if (bp) bp->clrlock(); }
    inline void ios::lockbuf() { bp->lock(); }
    inline void ios::unlockbuf() { bp->unlock(); }
#endif

#ifdef	_MSC_VER
// Restore default packing
#pragma pack(pop)
#endif	// _MSC_VER

#endif	// _INC_IOS

#endif /* __cplusplus */
