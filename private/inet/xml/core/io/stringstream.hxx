/*
 * @(#)StringStream.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#ifndef _CORE_IO_STRINGSTREAM
#define _CORE_IO_STRINGSTREAM

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif

#ifndef _CORE_LANG_STRINGBUFFER
#include "core/lang/stringbuffer.hxx"
#endif

#ifndef _CORE_IO_STREAM
#include "core/io/stream.hxx"
#endif


DEFINE_CLASS(StringStream);

class StringStream : public Base,
                     public Stream
{
    DECLARE_CLASS_MEMBERS_NOQI_I1(StringStream, Base, Stream);
    DECLARE_CLASS_INSTANCE(StringStream, Base);

protected:

    StringStream() { _fOnce = TRUE;};

public:

	static DLLEXPORT StringStream * newStringStream(String * text);
	static DLLEXPORT StringStream * newStringStream(StringBuffer * sb);

    virtual int get()
    {
        Assert(in != null);

        return index < size ? in->charAt(index++) : -1;
    }

    virtual HRESULT  put(TCHAR c)
	{
        Assert(out != null);

		out->append(c);
        return S_OK;
	}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);
    virtual HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead);
    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);

protected: 
    
    virtual void finalize()
    {
        in = null;
        out = null;
        super::finalize();
    }

private:

	int size;
	int index;
	RString in;
    RStringBuffer out;
    BOOL _fOnce;
};

#endif _CORE_IO_STRINGSTREAM
