/*
 * @(#)InputStream.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_IO_INPUTSTREAM
#define _CORE_IO_INPUTSTREAM

#ifndef _BASE_HXX
#include "core/base/base.hxx"
#endif


DEFINE_CLASS(InputStream);
DEFINE_CLASS(URL);

/**
 * An abstract input class
 */
class NOVTABLE InputStream: public Base
{

    DECLARE_CLASS_MEMBERS(InputStream, Base);

public:

    virtual int read() = 0; 

    virtual int read(abyte * b) 
    {
        return read(b, 0, b->length());
    }

    virtual int read(abyte * b, int off, int len) { return -1; };

    virtual void close() {}
};


#endif _CORE_IO_INPUTSTREAM