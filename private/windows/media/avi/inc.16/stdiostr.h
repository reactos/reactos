/***
*stdiostr.h - definitions/declarations for stdiobuf, stdiostream
*
*   Copyright (c) 1991-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the stdiostream and stdiobuf classes.
*   [AT&T C++]
*
****/

#include <iostream.h>
#include <stdio.h>

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

#ifndef _INC_STDIOSTREAM
#define _INC_STDIOSTREAM
class stdiobuf : public streambuf  {
public:
    stdiobuf(FILE* f);
FILE *  stdiofile() { return _str; }

virtual int pbackfail(int c);
virtual int overflow(int c = EOF);
virtual int underflow();
virtual streampos seekoff( streamoff, ios::seek_dir, int =ios::in|ios::out);
virtual int sync();
    ~stdiobuf();
    int setrwbuf(int _rsize, int _wsize); // CONSIDER: move to ios::
// protected:
// virtual int doallocate();
private:
    FILE * _str;
};

// obsolescent
class stdiostream : public iostream {   // note: spec.'d as : public IOS...
public:
    stdiostream(FILE *);
    ~stdiostream();
    stdiobuf* rdbuf() const { return (stdiobuf*) ostream::rdbuf(); }

private:
};

// Restore default packing
#pragma pack()

#endif 
