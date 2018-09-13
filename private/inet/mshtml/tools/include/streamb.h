/***
*streamb.h - definitions/declarations for the streambuf class
*
*	Copyright (c) 1990-1996, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file defines the classes, values, macros, and functions
*	used by the streambuf class.
*	[AT&T C++]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus

#ifndef _INC_STREAMB
#define _INC_STREAMB

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


#include <ios.h>	// need ios::seek_dir definition

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

typedef long streampos, streamoff;

class _CRTIMP ios;

class _CRTIMP streambuf {
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
    inline int sputn(const char *,int);
    inline int sgetn(char *,int);

    virtual int sync();

    virtual streambuf* setbuf(char *, int);
    virtual streampos seekoff(streamoff,ios::seek_dir,int =ios::in|ios::out);
    virtual streampos seekpos(streampos,int =ios::in|ios::out);

    virtual int xsputn(const char *,int);
    virtual int xsgetn(char *,int);

    virtual int overflow(int =EOF) = 0;	// pure virtual function
    virtual int underflow() = 0;	// pure virtual function

    virtual int pbackfail(int);

    void dbp();

#ifdef	_MT
    void setlock() { LockFlg--; }	// <0 indicates lock required;
    void clrlock() { if (LockFlg <= 0) LockFlg++; }
    void lock() { if (LockFlg<0) _mtlock(lockptr()); };
    void unlock() { if (LockFlg<0) _mtunlock(lockptr()); }
#else
    void lock() { }
    void unlock() { }
#endif

protected:
    streambuf();
    streambuf(char *,int);

    inline char * base() const;
    inline char * ebuf() const;
    inline char * pbase() const;
    inline char * pptr() const;
    inline char * epptr() const;
    inline char * eback() const;
    inline char * gptr() const;
    inline char * egptr() const;
    inline int blen() const;
    inline void setp(char *,char *);
    inline void setg(char *,char *,char *);
    inline void pbump(int);
    inline void gbump(int);

    void setb(char *,char *,int =0);
    inline int unbuffered() const;
    inline void unbuffered(int);
    int allocate();
    virtual int doallocate();
#ifdef	_MT
    _PCRT_CRITICAL_SECTION lockptr() { return & x_lock; }
#endif

private:
    int _fAlloc;
    int _fUnbuf;
    int x_lastc;
    char * _base;
    char * _ebuf;
    char * _pbase;
    char * _pptr;
    char * _epptr;
    char * _eback;
    char * _gptr;
    char * _egptr;
#ifdef	_MT
    int LockFlg;		// <0 indicates locking required
   _CRT_CRITICAL_SECTION x_lock;	// lock needed only for multi-thread operation
#endif
};

inline int streambuf::in_avail() const { return (gptr()<_egptr) ? (_egptr-gptr()) : 0; }
inline int streambuf::out_waiting() const { return (_pptr>=_pbase) ? (_pptr-_pbase) : 0; }

inline int streambuf::sputbackc(char _c){ return (_eback<gptr()) ? *(--_gptr)=_c : pbackfail(_c); }

inline int streambuf::sputc(int _i){ return (_pptr<_epptr) ? (unsigned char)(*(_pptr++)=(char)_i) : overflow(_i); }

inline int streambuf::sputn(const char * _str,int _n) { return xsputn(_str, _n); }
inline int streambuf::sgetn(char * _str,int _n) { return xsgetn(_str, _n); }

inline char * streambuf::base() const { return _base; }
inline char * streambuf::ebuf() const { return _ebuf; }
inline int streambuf::blen() const  {return ((_ebuf > _base) ? (_ebuf-_base) : 0); }
inline char * streambuf::pbase() const { return _pbase; }
inline char * streambuf::pptr() const { return _pptr; }
inline char * streambuf::epptr() const { return _epptr; }
inline char * streambuf::eback() const { return _eback; }
inline char * streambuf::gptr() const { return _gptr; }
inline char * streambuf::egptr() const { return _egptr; }
inline void streambuf::gbump(int _n) { if (_egptr) _gptr += _n; }
inline void streambuf::pbump(int _n) { if (_epptr) _pptr += _n; }
inline void streambuf::setg(char * _eb, char * _g, char * _eg) {_eback=_eb; _gptr=_g; _egptr=_eg; x_lastc=EOF; }
inline void streambuf::setp(char * _p, char * _ep) {_pptr=_pbase=_p; _epptr=_ep; }
inline int streambuf::unbuffered() const { return _fUnbuf; }
inline void streambuf::unbuffered(int _f) { _fUnbuf = _f; }

#ifdef	_MSC_VER
// Restore previous packing
#pragma pack(pop)
#endif	// _MSC_VER

#endif	// _INC_STREAMB

#endif /* __cplusplus */
