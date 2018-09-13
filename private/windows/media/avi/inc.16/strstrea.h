/***
*strstream.h - definitions/declarations for strstreambuf, strstream
*
*   Copyright (c) 1991-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the strstream and strstreambuf classes.
*   [AT&T C++]
*
****/

#ifndef _INC_STRSTREAM
#define _INC_STRSTREAM

#include <iostream.h>

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

#ifdef M_I86HM
#define _HFAR_ __far
#else 
#define _HFAR_
#endif 

class strstreambuf : public streambuf  {
public:
        strstreambuf();
        strstreambuf(int);
        strstreambuf(char _HFAR_ *, int, char _HFAR_ * = 0);
        strstreambuf(unsigned char _HFAR_ *, int, unsigned char _HFAR_ * = 0);
        strstreambuf(signed char _HFAR_ _HFAR_ *, int, signed char _HFAR_ * = 0);
        strstreambuf(void _HFAR_ * (*a)(long), void (*f) (void _HFAR_ *));
        ~strstreambuf();

    void    freeze(int =1);
    char _HFAR_ * str();

virtual int overflow(int);
virtual int underflow();
virtual streambuf* setbuf(char  _HFAR_ *, int);
virtual streampos seekoff(streamoff, ios::seek_dir, int);
virtual int sync();     // not in spec.

protected:
virtual int doallocate();
private:
    int x_dynamic;
    int     x_bufmin;
    int     _fAlloc;
    int x_static;
    void _HFAR_ * (* x_alloc)(long);
    void    (* x_free)(void _HFAR_ *);
};

class istrstream : public istream {
public:
        istrstream(char _HFAR_ *);
        istrstream(char _HFAR_ *, int);
        ~istrstream();

inline  strstreambuf* rdbuf() const { return (strstreambuf*) ios::rdbuf(); }
inline  char _HFAR_ *   str() { return rdbuf()->str(); }
};

class ostrstream : public ostream {
public:
        ostrstream();
        ostrstream(char _HFAR_ *, int, int = ios::out);
        ~ostrstream();

inline  int pcount() const { return rdbuf()->out_waiting(); }
inline  strstreambuf* rdbuf() const { return (strstreambuf*) ios::rdbuf(); }
inline  char _HFAR_ *   str() { return rdbuf()->str(); }
};

class strstream : public iostream { // strstreambase ???
public:
        strstream();
        strstream(char _HFAR_ *, int, int);
        ~strstream();

inline  int pcount() const { return rdbuf()->out_waiting(); } // not in spec.
inline  strstreambuf* rdbuf() const { return (strstreambuf*) ostream::rdbuf(); }
inline  char _HFAR_ *   str() { return rdbuf()->str(); }
};

// Restore default packing
#pragma pack()

#endif 
