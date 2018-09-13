/*
 * @(#)StringStream.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "core/io/stringstream.hxx"

#ifndef _CORE_UTIL_CHARTYPE_HXX
#include "core/util/chartype.hxx"
#endif // _CORE_UTIL_CHARTYPE_HXX


DEFINE_CLASS_MEMBERS_NEWINSTANCE(StringStream, _T("StringStream"), Base);

HRESULT STDMETHODCALLTYPE StringStream::QueryInterface(REFIID iid, void ** ppvObject)
{
    if (iid == IID_IStream)
    {
        *ppvObject = (IStream*)this;
        AddRef();
        return S_OK;
    }
    else
        return super::QueryInterface(iid, ppvObject);
}

StringStream * StringStream::newStringStream(String * text)
{
    Assert(text != null);
    
    StringStream * s = new StringStream();
    s->in = text;
    s->size = text->length();
    s->index = 0;
    return s;
}


StringStream * StringStream::newStringStream(StringBuffer * sb)
{
    Assert(sb != null);

    StringStream * s = new StringStream();
    s->out = sb;
    return s;
}

HRESULT STDMETHODCALLTYPE 
StringStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    if (index == size)
    {
        // The convention is to return S_FALSE when at the end.
        *pcbRead = 0;
        return S_FALSE;
    }
    if (cb <= sizeof(WCHAR) * 2) // Make sure we can write byte marker and 1 other character
    {
        *pcbRead = 0;
        return 0;
    }        

    ULONG i = 0;
    WCHAR* buf = (WCHAR*)pv;
    cb /= sizeof(WCHAR); 
    
    if (_fOnce)
    {
        // first return unicode byte order mark
        memcpy(buf, s_ByteOrderMark, sizeof(WCHAR));
        i++;
        cb--;
        _fOnce = FALSE;
    }

    while (i < cb)
    {
        int ch = get();
        if (ch == -1)
            break;
        buf[i++] = (WCHAR)ch;
    }
    *pcbRead = i*sizeof(WCHAR);
    return S_OK; 
};

HRESULT STDMETHODCALLTYPE 
StringStream::Write(void const* pv, ULONG cb, ULONG * pcbWritten)
{
    Assert((cb / sizeof(WCHAR))*sizeof(WCHAR) == cb);

    TCHAR* buf = (TCHAR*)pv;
    ULONG len = cb / sizeof(WCHAR); // UNICODE chars
    out->append(buf, 0, len);
    if (pcbWritten)         // this is an optional arg.
        *pcbWritten = cb;
    return S_OK;
}