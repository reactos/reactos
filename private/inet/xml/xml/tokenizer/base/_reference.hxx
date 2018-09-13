
#ifndef __reference_HXX
#define __reference_HXX

/*
 * @(#)_reference.hxx 1.0 2/27/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
    
void _assign(IUnknown ** ppref, IUnknown * pref);
void _release(IUnknown ** ppref);

template <class T>
void assign(T ** ppref, T * pref){ _assign((IUnknown **) ppref, pref);}

//----------------------------------------------------------------------------
template <class T> class _reference
{    
private:
    T * _p;
public:
    
    _reference() : _p(NULL) {}

    _reference(T * p) : _p(p) { if (_p) _p->AddRef(); }    

    _reference(const _reference<T> & r) { _p = r._p; if (_p) _p->AddRef(); }

    ~_reference() { _release((IUnknown **)&_p); }

    operator T * () { return _p; }    

    operator T * () const { return _p; }    

    T & operator * () { return *_p; }

    T * operator -> () { return _p; }    

    T * operator -> () const { return _p; }    

    T** operator & () { return &_p; }

    _reference & operator = (T * p) { assign(&_p, p); return *this; }

    _reference & operator = (const _reference<T> & r) { return operator=(r._p); }
};

#endif __reference_HXX
