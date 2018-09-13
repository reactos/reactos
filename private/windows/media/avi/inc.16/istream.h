/***
*istream.h - definitions/declarations for the istream class
*
*   Copyright (c) 1990-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the istream class.
*   [AT&T C++]
*
****/

#ifndef _INC_ISTREAM
#define _INC_ISTREAM

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

class istream : virtual public ios {

public:
    istream(streambuf*);
    virtual ~istream();

    int  ipfx(int =0);
    void isfx() { }

    inline istream& operator>>(istream& (*_f)(istream&));
    inline istream& operator>>(ios& (*_f)(ios&));
    istream& operator>>(char _HFAR_ *);
    inline istream& operator>>(unsigned char _HFAR_ *);
    inline istream& operator>>(signed char _HFAR_ *);
    istream& operator>>(char _HFAR_ &);
    inline istream& operator>>(unsigned char _HFAR_ &);
    inline istream& operator>>(signed char _HFAR_ &);
    istream& operator>>(short _HFAR_ &);
    istream& operator>>(unsigned short _HFAR_ &);
    istream& operator>>(int _HFAR_ &);
    istream& operator>>(unsigned int _HFAR_ &);
    istream& operator>>(long _HFAR_ &);
    istream& operator>>(unsigned long _HFAR_ &);
    istream& operator>>(float _HFAR_ &);
    istream& operator>>(double _HFAR_ &);
    istream& operator>>(long double _HFAR_ &);
    istream& operator>>(streambuf*);

    int get();
    istream& get(char _HFAR_ *,int,char ='\n');
    inline istream& get(unsigned char _HFAR_ *,int,char ='\n');
    inline istream& get(signed char _HFAR_ *,int,char ='\n');
    istream& get(char _HFAR_ &);
    inline istream& get(unsigned char _HFAR_ &);
    inline istream& get(signed char _HFAR_ &);
    istream& get(streambuf&,char ='\n');
    inline istream& getline(char _HFAR_ *,int,char ='\n');
    inline istream& getline(unsigned char _HFAR_ *,int,char ='\n');
    inline istream& getline(signed char _HFAR_ *,int,char ='\n');

    inline istream& ignore(int =1,int =EOF);
    istream& read(char _HFAR_ *,int);
    inline istream& read(unsigned char _HFAR_ *,int);
    inline istream& read(signed char _HFAR_ *,int);

    int gcount() const { return x_gcount; }
    int peek();
    istream& putback(char);
    int sync();

    istream& seekg(streampos);
    istream& seekg(streamoff,ios::seek_dir);
    streampos tellg();

    void eatwhite();    // consider: protect and friend with manipulator ws
protected:
    istream();
    istream(const istream&);    // treat as private
    istream& operator=(streambuf* _isb); // treat as private
    istream& operator=(const istream& _is) { return operator=(_is.rdbuf()); }
    int do_ipfx(int);

private:
    istream(ios&);
    int getint(char _HFAR_ *);
    int getdouble(char _HFAR_ *, int);
    int _fGline;
    int x_gcount;
};

    inline istream& istream::operator>>(istream& (*_f)(istream&)) { (*_f)(*this); return *this; }
    inline istream& istream::operator>>(ios& (*_f)(ios&)) { (*_f)(*this); return *this; }

    inline istream& istream::operator>>(unsigned char _HFAR_ * _s) { return operator>>((char _HFAR_ *)_s); }
    inline istream& istream::operator>>(signed char _HFAR_ * _s) { return operator>>((char _HFAR_ *)_s); }

    inline istream& istream::operator>>(unsigned char _HFAR_ & _c) { return operator>>((char _HFAR_ &) _c); }
    inline istream& istream::operator>>(signed char _HFAR_ & _c) { return operator>>((char _HFAR_ &) _c); }

    inline istream& istream::get(unsigned char _HFAR_ * b, int lim ,char delim) { return get((char _HFAR_ *)b, lim, delim); }
    inline istream& istream::get(signed char _HFAR_ * b, int lim, char delim) { return get((char _HFAR_ *)b, lim, delim); }

    inline istream& istream::get(unsigned char _HFAR_ & _c) { return get((char _HFAR_ &)_c); }
    inline istream& istream::get(signed char _HFAR_ & _c) { return get((char _HFAR_ &)_c); }

    inline istream& istream::getline(char _HFAR_ * _b,int _lim,char _delim) { _fGline++; return get(_b, _lim, _delim); }
    inline istream& istream::getline(unsigned char _HFAR_ * _b,int _lim,char _delim) { _fGline++; return get((char _HFAR_ *)_b, _lim, _delim); }
    inline istream& istream::getline(signed char _HFAR_ * _b,int _lim,char _delim) { _fGline++; return get((char _HFAR_ *)_b, _lim, _delim); }

    inline istream& istream::ignore(int _n,int delim) { _fGline++; return get((char _HFAR_ *)0, _n+1, (char)delim); }

    inline istream& istream::read(unsigned char _HFAR_ * _ptr, int _n) { return read((char _HFAR_ *) _ptr, _n); }
    inline istream& istream::read(signed char _HFAR_ * _ptr, int _n) { return read((char _HFAR_ *) _ptr, _n); }

class istream_withassign : public istream {
    public:
        istream_withassign();
        istream_withassign(streambuf*);
        ~istream_withassign();
    istream& operator=(const istream& _is) { return istream::operator=(_is); }
    istream& operator=(streambuf* _isb) { return istream::operator=(_isb); }
};

#ifndef _WINDLL
extern istream_withassign cin;
#endif 

inline istream& ws(istream& _ins) { _ins.eatwhite(); return _ins; }

ios&        dec(ios&);
ios&        hex(ios&);
ios&        oct(ios&);

// Restore default packing
#pragma pack()

#endif 
