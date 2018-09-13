/*
 * @(#)StringInputStream.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _CORE_IO_STRINGINPUTSTREAM
#define _CORE_IO_STRINGINPUTSTREAM


#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif

#ifndef _CORE_IO_INPUTSTREAM
#include "core/io/inputstream.hxx"
#endif


DEFINE_CLASS(XMLInputStream);

class StringInputStream : public InputStream
{
    DECLARE_CLASS_MEMBERS(StringInputStream, InputStream);

protected: StringInputStream() : buf(REF_NOINIT) {}

public: 
    static StringInputStream * newStringInputStream(String * text);

    virtual int read(); // throws IOException;

    virtual int read(abyte * b, int off, int len);

    protected: virtual void finalize()
    {
        buf = null;
        super::finalize();
    }

    int size;
    int index;
    RString buf;
};



#endif _CORE_IO_STRINGINPUTSTREAM
