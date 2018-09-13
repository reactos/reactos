/***
*streamb.h - definitions/declarations for the streambuf class
*
*   Copyright (c) 1990-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the streambuf class.
*   [AT&T C++]
*
****/

#ifndef _INC_STREAMB
#define _INC_STREAMB


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

typedef long streampos, streamoff;

class streambuf {
public:

    virtual ~streambuf();

    inline int in_avail() const;
    inline int out_waiting() const;
    int sgetc();
    int snextc();
    int sbumpc();
    void stossc();

    inline int sputbackc(char);

    inline int sputc(int);
    inline int sputn(const char _HFAR_ *,int);
    inline int sgetn(char _HFAR_ *,int);

    virtual int sync();

//  enum seek_dir { beg=0, cur=1, end=2 };  // CONSIDER: needed ???

    virtual streambuf* setbuf(char _HFAR_ *, int);
    virtual streampos seekoff(streamoff,ios::seek_dir,int =ios::in|ios::out);
    virtual streampos seekpos(streampos,int =ios::in|ios::out);

    virtual int xsputn(const char _HFAR_ *,int);
    virtual int xsgetn(char _HFAR_ *,int);

    virtual int overflow(int =EOF) = 0; // pure virtual function
    virtual int underflow() = 0;    // pure virtual function

    virtual int pbackfail(int);

    void dbp();

protected:
    streambuf();
    streambuf(char _HFAR_ *,int);

    inline char _HFAR_ * base() const;
    inline char _HFAR_ * ebuf() const;
    inline char _HFAR_ * pbase() const;
    inline char _HFAR_ * pptr() const;
    inline char _HFAR_ * epptr() const;
    inline char _HFAR_ * eback() const;
    inline char _HFAR_ * gptr() const;
    inline char _HFAR_ * egptr() const;
    inline int blen() const;
    inline void setp(char _HFAR_ *,char _HFAR_ *);
    inline void setg(char _HFAR_ *,char _HFAR_ *,char _HFAR_ *);
    inline void pbump(int);
    inline void gbump(int);

    void setb(char _HFAR_ *,char _HFAR_ *,int =0);
    inline int unbuffered() const;
    inline void unbuffered(int);
    int allocate();
    virtual int doallocate();

private:
    int _fAlloc;
    int _fUnbuf;
    int x_lastc;
    char _HFAR_ * _base;
    char _HFAR_ * _ebuf;
    char _HFAR_ * _pbase;
    char _HFAR_ * _pptr;
    char _HFAR_ * _epptr;
    char _HFAR_ * _eback;
    char _HFAR_ * _gptr;
    char _HFAR_ * _egptr;
};

inline int streambuf::in_avail() const { return (gptr()<_egptr) ? (_egptr-gptr()) : 0; }
inline int streambuf::out_waiting() const { return (_pptr>=_pbase) ? (_pptr-_pbase) : 0; }

inline int streambuf::sputbackc(char _c){ return (_eback<gptr()) ? *(--_gptr)=_c : pbackfail(_c); }

inline int streambuf::sputc(int _i){ return (_pptr<_epptr) ? (unsigned char)(*(_pptr++)=(char)_i) : overflow(_i); }

inline int streambuf::sputn(const char _HFAR_ * _str,int _n) { return xsputn(_str, _n); }
inline int streambuf::sgetn(char _HFAR_ * _str,int _n) { return xsgetn(_str, _n); }

inline char _HFAR_ * streambuf::base() const { return _base; }
inline char _HFAR_ * streambuf::ebuf() const { return _ebuf; }
inline int streambuf::blen() const  {return ((_ebuf > _base) ? (_ebuf-_base) : 0); }
inline char _HFAR_ * streambuf::pbase() const { return _pbase; }
inline char _HFAR_ * streambuf::pptr() const { return _pptr; }
inline char _HFAR_ * streambuf::epptr() const { return _epptr; }
inline char _HFAR_ * streambuf::eback() const { return _eback; }
inline char _HFAR_ * streambuf::gptr() const { return _gptr; }
inline char _HFAR_ * streambuf::egptr() const { return _egptr; }
inline void streambuf::gbump(int n) { if (_egptr) _gptr += n; }
inline void streambuf::pbump(int n) { if (_epptr) _pptr += n; }
inline void streambuf::setg(char _HFAR_ * eb, char _HFAR_ * g, char _HFAR_ * eg) {_eback=eb; _gptr=g; _egptr=eg; x_lastc=EOF; }
inline void streambuf::setp(char _HFAR_ * p, char _HFAR_ * ep) {_pptr=_pbase=p; _epptr=ep; }
inline int streambuf::unbuffered() const { return _fUnbuf; }
inline void streambuf::unbuffered(int fUnbuf) { _fUnbuf = fUnbuf; }

// Restore default packing
#pragma pack()

#endif 
