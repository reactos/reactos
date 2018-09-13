/***
*ios.h - definitions/declarations for the ios class.
*
*   Copyright (c) 1990-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the ios class.
*   [AT&T C++]
*
****/

#ifndef _INC_IOS
#define _INC_IOS


#ifdef M_I86HM
#define _HFAR_ __far
#else 
#define _HFAR_
#endif 

#ifndef NULL
#define NULL    0
#endif 

#ifndef EOF
#define EOF (-1)
#endif 

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

class streambuf;
class ostream;

class ios {

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
             binary    = 0x80 };    // CONSIDER: not in latest spec.

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

    static const long basefield;    // dec | oct | hex
    static const long adjustfield;  // left | right | internal
    static const long floatfield;   // scientific | fixed

    ios(streambuf*);            // differs from ANSI
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

    inline long _HFAR_ & iword(int) const;
    inline void _HFAR_ * _HFAR_ & pword(int) const;

    static long bitalloc();
    static int xalloc();
    static void sync_with_stdio();

protected:
    ios();
    ios(const ios&);            // treat as private
    ios& operator=(const ios&);
    void init(streambuf*);

    enum { skipping, tied };
    streambuf*  bp;

    int     state;
    int     ispecial;           // not used
    int     ospecial;           // not used
    int     isfx_special;       // not used
    int     osfx_special;       // not used
    int     x_delbuf;           // if set, rdbuf() deleted by ~ios

    ostream* x_tie;
    long    x_flags;
    int     x_precision;
    int     x_width;
    char    x_fill;

    static void (*stdioflush)();    // not used
public:
    int delbuf() const { return x_delbuf; }
    void    delbuf(int _i) { x_delbuf = _i; }

private:
    static long x_maxbit;
    static long _HFAR_ * x_statebuf;  // used by xalloc()
    static int x_curindex;
// consider: make interal static to ios::sync_with_stdio()
    static int sunk_with_stdio;     // make sure sync_with done only once
};

inline ios& dec(ios& _strm) { _strm.setf(ios::dec,ios::basefield); return _strm; }
inline ios& hex(ios& _strm) { _strm.setf(ios::hex,ios::basefield); return _strm; }
inline ios& oct(ios& _strm) { _strm.setf(ios::oct,ios::basefield); return _strm; }

inline long ios::flags() const { return x_flags; }
inline long ios::flags(long _l){ long _lO; _lO = x_flags; x_flags = _l; return _lO; }

inline long ios::setf(long _l,long _m){ long _lO; _lO = x_flags; x_flags = (_l&_m) | (x_flags&(~_m)); return _lO; }
inline long ios::setf(long _l){ long _lO; _lO = x_flags; x_flags |= _l; return _lO; }
inline long ios::unsetf(long _l){ long _lO; _lO = x_flags; x_flags &= (~_l); return _lO; }

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
inline void ios::clear(int _i){ state = _i; }
inline int  ios::eof() const { return state & eofbit; }
inline int  ios::fail() const { return state & (badbit | failbit); }
inline int  ios::good() const { return state == 0; }

inline streambuf* ios::rdbuf() const { return bp; }

inline long _HFAR_ & ios::iword(int _i) const { return x_statebuf[_i] ; }
inline void _HFAR_ * _HFAR_ & ios::pword(int _i) const { return (void _HFAR_ * _HFAR_ &)x_statebuf[_i]; }

// Restore default packing
#pragma pack()

#endif 
