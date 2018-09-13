/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _BSTR_HXX
#define _BSTR_HXX

class bstr
{    
private:    BSTR _bstr;

private:    bstr & set(const WCHAR * w);

private:    bstr & set(const String * s);

public:     bstr() : _bstr(NULL) {}

public:     bstr(const WCHAR * w);

public:     bstr(const String * s);

public:     bstr(const bstr & b);

public:     ~bstr() { SysFreeString(_bstr); }

public:     operator BSTR () const { return _bstr; }    

public:     BSTR * operator & () { Assert(!_bstr); return &_bstr; }

public:     bstr & operator = (const WCHAR * w) { return set(w); }

public:     bstr & operator = (const bstr & b) { return set(b._bstr); }

public:     bstr & operator = (const String * s); 
};

#endif _BSTR_HXX
