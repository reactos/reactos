/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "bstr.hxx"

bstr::bstr(const WCHAR * w) : _bstr(NULL)
{
    set(w);
}

bstr::bstr(const bstr & b) : _bstr(NULL)
{
    set(b._bstr);
}

bstr::bstr(const String * s) : _bstr(NULL)
{
    set(s);
}

bstr & bstr::operator = (const String * s)
{
    return set(s);
}    


bstr & bstr::set(const WCHAR * w)
{
    if (_bstr)
    {
        SysFreeString(_bstr);
        _bstr = NULL;
    }
    if (w != NULL)
    {
        _bstr = SysAllocString(w);
        if (!_bstr)
            Exception::throwEOutOfMemory(); // Exception::OutOfMemoryException);
    }
    return *this; 
}


bstr & bstr::set(const String * s)
{
    if (_bstr)
    {
        SysFreeString(_bstr);
        _bstr = NULL;
    }
    if (s != NULL)
    {
        _bstr = s->getBSTR();
    }
    return *this; 
}

