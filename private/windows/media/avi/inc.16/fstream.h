/***
*fstream.h - definitions/declarations for filebuf and fstream classes
*
*   Copyright (c) 1991-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the classes, values, macros, and functions
*   used by the filebuf and fstream classes.
*   [AT&T C++]
*
****/

#ifndef _INC_FSTREAM
#define _INC_FSTREAM

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

typedef int filedesc;

class filebuf : public streambuf {
public:
static  const int   openprot;   // default share/prot mode for open

// optional share values for 3rd argument (prot) of open or constructor
static  const int   sh_compat;  // compatibility share mode
static  const int   sh_none;    // exclusive mode no sharing
static  const int   sh_read;    // allow read sharing
static  const int   sh_write;   // allow write sharing
// use (sh_read | sh_write) to allow both read and write sharing

// options for setmode member function
static  const int   binary;
static  const int   text;

            filebuf();
            filebuf(filedesc);
            filebuf(filedesc, char _HFAR_ *, int);
            ~filebuf();

    filebuf*    attach(filedesc);
    filedesc    fd() const { return (x_fd==-1) ? EOF : x_fd; }
    int     is_open() const { return (x_fd!=-1); }
    filebuf*    open(const char _HFAR_ *, int, int = filebuf::openprot);
    filebuf*    close();
    int     setmode(int = filebuf::text);

virtual int     overflow(int=EOF);
virtual int     underflow();

virtual streambuf*  setbuf(char _HFAR_ *, int);
virtual streampos   seekoff(streamoff, ios::seek_dir, int);
// virtual  streampos   seekpos(streampos, int);
virtual int     sync();

private:
    filedesc    x_fd;
    int     x_fOpened;
};

class ifstream : public istream {
public:
    ifstream();
    ifstream(const char _HFAR_ *, int =ios::in, int = filebuf::openprot);
    ifstream(filedesc);
    ifstream(filedesc, char _HFAR_ *, int);
    ~ifstream();

    streambuf * setbuf(char _HFAR_ *, int);
    filebuf* rdbuf() const { return (filebuf*) ios::rdbuf(); }

    void attach(filedesc);
    filedesc fd() const { return rdbuf()->fd(); }

    int is_open() const { return rdbuf()->is_open(); }
    void open(const char _HFAR_ *, int =ios::in, int = filebuf::openprot);
    void close();
    int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};

class ofstream : public ostream {
public:
    ofstream();
    ofstream(const char _HFAR_ *, int =ios::out, int = filebuf::openprot);
    ofstream(filedesc);
    ofstream(filedesc, char _HFAR_ *, int);
    ~ofstream();

    streambuf * setbuf(char _HFAR_ *, int);
    filebuf* rdbuf() const { return (filebuf*) ios::rdbuf(); }

    void attach(filedesc);
    filedesc fd() const { return rdbuf()->fd(); }

    int is_open() const { return rdbuf()->is_open(); }
    void open(const char _HFAR_ *, int =ios::out, int = filebuf::openprot);
    void close();
    int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};

class fstream : public iostream {
public:
    fstream();
    fstream(const char _HFAR_ *, int, int = filebuf::openprot);
    fstream(filedesc);
    fstream(filedesc, char _HFAR_ *, int);
    ~fstream();

    streambuf * setbuf(char _HFAR_ *, int);
    filebuf* rdbuf() const { return (filebuf*) ostream::rdbuf(); }

    void attach(filedesc);
    filedesc fd() const { return rdbuf()->fd(); }

    int is_open() const { return rdbuf()->is_open(); }
    void open(const char _HFAR_ *, int, int = filebuf::openprot);
    void close();
    int setmode(int mode = filebuf::text) { return rdbuf()->setmode(mode); }
};

// manipulators to dynamically change file access mode (filebufs only)
inline  ios& binary(ios& _fstrm) \
   { ((filebuf*)_fstrm.rdbuf())->setmode(filebuf::binary); return _fstrm; }
inline  ios& text(ios& _fstrm) \
   { ((filebuf*)_fstrm.rdbuf())->setmode(filebuf::text); return _fstrm; }

// Restore default packing
#pragma pack()

#endif 
