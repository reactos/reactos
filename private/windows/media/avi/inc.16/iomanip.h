/***
*iomanip.h - definitions/declarations for iostream's parameterized manipulators
*
*   Copyright (c) 1991-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the iostream classes' paramterized manipulators.
*   [AT&T C++]
*
****/

#ifndef _INC_IOMANIP
#define _INC_IOMANIP
#include <iostream.h>

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

// CONSIDER: use macro to define these
// #define __MKMANIP(X) \#define X##(T) __##X##_ \#\# T
// __MKMANIP(SMANIP);
// __MKMANIP(SAPP);
// __MKMANIP(IMANIP);
// __MKMANIP(IAPP);
// __MKMANIP(OMANIP);
// __MKMANIP(OAPP);
// __MKMANIP(IOMANIP);
// __MKMANIP(IOAPP);

#define SMANIP(T) __SMANIP_##T
#define SAPP(T) __SAPP_##T
#define IMANIP(T) __IMANIP_##T
#define IAPP(T) __IAPP_##T
#define OMANIP(T) __OMANIP_##T
#define OAPP(T) __OAPP_##T
#define IOMANIP(T) __IOMANIP_##T
#define IOAPP(T) __IOAPP_##T

#define IOMANIPdeclare(T)  \
class SMANIP(T) { \
public: \
    SMANIP(T)(ios& (*f)(ios&,T), T t) { _fp = f; _tp = t; } \
    friend istream& operator>>(istream& s, SMANIP(T) & sm) { (*(sm._fp))(s,sm._tp); return s; } \
    friend ostream& operator<<(ostream& s, SMANIP(T) & sm) { (*(sm._fp))(s,sm._tp); return s; } \
private:    \
    ios& (* _fp)(ios&,T); \
    T _tp; \
};  \
class SAPP(T) { \
public: \
    SAPP(T)( ios& (*f)(ios&,T)) { _fp = f; }    \
    SMANIP(T) operator()(T t) { return SMANIP(T)(_fp,t); }  \
private:    \
    ios& (* _fp)(ios&,T); \
};  \
class IMANIP(T) { \
public: \
    IMANIP(T)(istream& (*f)(istream&,T), T t) { _fp = f; _tp = t; } \
    friend istream& operator>>(istream& s, IMANIP(T) & sm) { (*sm._fp)(s,sm._tp); return s; } \
private:    \
    istream& (* _fp)(istream&,T); \
    T _tp;  \
};  \
class IAPP(T) { \
public: \
    IAPP(T)( istream& (*f)(istream&,T)) { _fp = f; }    \
    IMANIP(T) operator()(T t) { return IMANIP(T)(_fp,t); }  \
private:    \
    istream& (* _fp)(istream&,T); \
};  \
class OMANIP(T) { \
public: \
    OMANIP(T)(ostream& (*f)(ostream&,T), T t) { _fp = f; _tp = t; } \
    friend ostream& operator<<(ostream& s, OMANIP(T) & sm) { (*sm._fp)(s,sm._tp); return s; } \
private:    \
    ostream& (* _fp)(ostream&,T); \
    T _tp; \
};  \
class OAPP(T) { \
public: \
    OAPP(T)(ostream& (*f)(ostream&,T)) { _fp = f; } \
    OMANIP(T) operator()(T t) { return OMANIP(T)(_fp,t); } \
private:    \
    ostream& (* _fp)(ostream&,T); \
};  \
\
class IOMANIP(T) { \
public: \
    IOMANIP(T)(iostream& (*f)(iostream&,T), T t) { _fp = f; _tp = t; } \
    friend istream& operator>>(iostream& s, IOMANIP(T) & sm) { (*sm._fp)(s,sm._tp); return s; } \
    friend ostream& operator<<(iostream& s, IOMANIP(T) & sm) { (*sm._fp)(s,sm._tp); return s; } \
private:    \
    iostream& (* _fp)(iostream&,T); \
    T _tp; \
};  \
class IOAPP(T) {    \
public: \
    IOAPP(T)( iostream& (*f)(iostream&,T)) { _fp = f; } \
    IOMANIP(T) operator()(T t) { return IOMANIP(T)(_fp,t); }    \
private:    \
    iostream& (* _fp)(iostream&,T); \
}; \


IOMANIPdeclare(int)

IOMANIPdeclare(long)

inline ios& __resetiosflags(ios& s, long _flg) { s.setf(0,_flg); return s; }
inline ios& __setfill(ios& s, int _fc) { s.fill((char)_fc); return s; }
inline ios& __setiosflags(ios& s, long _flg) { s.setf(_flg); return s; }
inline ios& __setprecision(ios& s, int _pre) { s.precision(_pre); return s; }
inline ios& __setw(ios& s, int _wid) { s.width(_wid); return s; }

inline SMANIP(long) resetiosflags(long _l) { return SMANIP(long)(__resetiosflags, _l); }
inline SMANIP(int)  setfill(int _m) {return SMANIP(int)(__setfill, _m); }
inline SMANIP(long) setiosflags(long _l) {return SMANIP(long)(__setiosflags, _l); }
inline SMANIP(int)  setprecision(int _p) {return SMANIP(int)(__setprecision, _p); }
inline SMANIP(int)  setw(int _w) { return SMANIP(int)(__setw, _w); }

// Restore default packing
#pragma pack()

#endif 
