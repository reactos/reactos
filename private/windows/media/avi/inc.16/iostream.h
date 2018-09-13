/***
*iostream.h - definitions/declarations for iostream classes
*
*   Copyright (c) 1990-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the iostream classes.
*   [AT&T C++]
*
****/

#ifndef _INC_IOSTREAM
#define _INC_IOSTREAM

typedef long streamoff, streampos;

#include <ios.h>        // Define ios.

#include <streamb.h>        // Define streambuf.

#include <istream.h>        // Define istream.

#include <ostream.h>        // Define ostream.

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

class iostream : public istream, public ostream {
public:
    iostream(streambuf*);
    virtual ~iostream();
protected:
// consider: make private??
    iostream();
    iostream(const iostream&);
inline iostream& operator=(streambuf*);
inline iostream& operator=(iostream&);
private:
    iostream(ios&);
    iostream(istream&);
    iostream(ostream&);
};

inline iostream& iostream::operator=(streambuf* _sb) { istream::operator=(_sb); ostream::operator=(_sb); return *this; }

inline iostream& iostream::operator=(iostream& _strm) { return operator=(_strm.rdbuf()); }

class Iostream_init {
public:
    Iostream_init();
    Iostream_init(ios &, int =0);   // treat as private
    ~Iostream_init();
};

// used internally
// static Iostream_init __iostreaminit; // initializes cin/cout/cerr/clog

// Restore default packing
#pragma pack()

#endif 
