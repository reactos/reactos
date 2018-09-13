/***
*ostream.h - definitions/declarations for the ostream class
*
*   Copyright (c) 1991-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the ostream class.
*   [AT&T C++]
*
****/

#ifndef _INC_OSTREAM
#define _INC_OSTREAM

#include <ios.h>

// Force word packing to avoid possible -Zp override
#pragma pack(2)

#pragma warning(disable:4505)       // disable unwanted /W4 warning
// #pragma warning(default:4505)    // use this to reenable, if necessary

#ifdef M_I86HM
#define _HFAR_ __far
#else 
#define _HFAR_
#endif 

typedef long streamoff, streampos;

class ostream : virtual public ios {

public:
    ostream(streambuf*);
    virtual ~ostream();

    ostream& flush();
    int  opfx();
    void osfx();

inline  ostream& operator<<(ostream& (*f)(ostream&));
inline  ostream& operator<<(ios& (*f)(ios&));
    ostream& operator<<(const char _HFAR_ *);
inline  ostream& operator<<(const unsigned char _HFAR_ *);
inline  ostream& operator<<(const signed char _HFAR_ *);
inline  ostream& operator<<(char);
    ostream& operator<<(unsigned char);
inline  ostream& operator<<(signed char);
    ostream& operator<<(short);
    ostream& operator<<(unsigned short);
    ostream& operator<<(int);
    ostream& operator<<(unsigned int);
    ostream& operator<<(long);
    ostream& operator<<(unsigned long);
inline  ostream& operator<<(float);
    ostream& operator<<(double);
    ostream& operator<<(long double);
    ostream& operator<<(const void _HFAR_ *);
    ostream& operator<<(streambuf*);
inline  ostream& put(char);
    ostream& put(unsigned char);
inline  ostream& put(signed char);
    ostream& write(const char _HFAR_ *,int);
inline  ostream& write(const unsigned char _HFAR_ *,int);
inline  ostream& write(const signed char _HFAR_ *,int);
    ostream& seekp(streampos);
    ostream& seekp(streamoff,ios::seek_dir);
    streampos tellp();

protected:
    ostream();
    ostream(const ostream&);    // treat as private
    ostream& operator=(streambuf*); // treat as private
    ostream& operator=(const ostream& _os) {return operator=(_os.rdbuf()); }
    int do_opfx(int);       // not used
    void do_osfx();         // not used

private:
    ostream(ios&);
    ostream& writepad(const char _HFAR_ *, const char _HFAR_ *);
    int x_floatused;
};

inline ostream& ostream::operator<<(ostream& (*f)(ostream&)) { (*f)(*this); return *this; }
inline ostream& ostream::operator<<(ios& (*f)(ios& )) { (*f)(*this); return *this; }

inline  ostream& ostream::operator<<(char c) { return operator<<((unsigned char) c); }
inline  ostream& ostream::operator<<(signed char c) { return operator<<((unsigned char) c); }

inline  ostream& ostream::operator<<(const unsigned char _HFAR_ * s) { return operator<<((const char _HFAR_ *) s); }
inline  ostream& ostream::operator<<(const signed char _HFAR_ * s) { return operator<<((const char _HFAR_ *) s); }

inline  ostream& ostream::operator<<(float f) { x_floatused = 1; return operator<<((double) f); }

inline  ostream& ostream::put(char c) { return put((unsigned char) c); }
inline  ostream& ostream::put(signed char c) { return put((unsigned char) c); }

inline  ostream& ostream::write(const unsigned char _HFAR_ * s, int n) { return write((char _HFAR_ *) s, n); }
inline  ostream& ostream::write(const signed char _HFAR_ * s, int n) { return write((char _HFAR_ *) s, n); }


class ostream_withassign : public ostream {
    public:
        ostream_withassign();
        ostream_withassign(streambuf* _is);
        ~ostream_withassign();
    ostream& operator=(const ostream& _os) { return ostream::operator=(_os.rdbuf()); }
    ostream& operator=(streambuf* _sb) { return ostream::operator=(_sb); }
};

#ifndef _WINDLL
extern ostream_withassign cout;
extern ostream_withassign cerr;
extern ostream_withassign clog;
#endif 

inline ostream& flush(ostream& _outs) { return _outs.flush(); }
inline ostream& endl(ostream& _outs) { return _outs << '\n' << flush; }
inline ostream& ends(ostream& _outs) { return _outs << char('\0'); }

ios&        dec(ios&);
ios&        hex(ios&);
ios&        oct(ios&);

// Restore default packing
#pragma pack()

#endif 
