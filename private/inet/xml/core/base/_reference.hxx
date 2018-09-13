/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _REFERENCE_HXX
#define _REFERENCE_HXX

void assign(IUnknown ** ppref, void * pref);

void release(IUnknown ** ppref);

void assignRO(IUnknown ** ppref, IUnknown * pref, const bool addRef);

#ifdef RENTAL_MODEL
BOOL checkRentalAssign(void * pWhere, void * pWhat);
#endif

class DLLEXPORT Object;

template <class T>
void assign(T ** ppref, void * pref){assign((IUnknown **) ppref, pref);}

template <class T>
void release(T ** p){release((IUnknown **) p);}

#define REF_NOINIT ((float)0.0)
    
template <class T> class _reference
{    
private:    T * _p;

            // special constructor to avoid re-initializing to 0 when embedded
public:     _reference(float)
            {
                Assert(_p == NULL);
            }


public:        _reference() : _p(NULL) {}

public:        _reference(T * p) : _p(p) 
               { 
                   if (_p) 
                   {
#ifdef RENTAL_MODEL
                       Assert(checkRentalAssign(this, _p));
#endif
                       _p->AddRef(); 
                   }
               }    

public:     _reference(const _reference<T> & r)
            {
                _p = r._p; 
                if (_p) 
                {
#ifdef RENTAL_MODEL
                    Assert(checkRentalAssign(this, _p));
#endif
                    _p->AddRef();
                }
            }

public:        ~_reference() { release(&_p); }

public:        operator T * () const { return _p; }    

public:        T & operator * () { return *_p; }

public:        T * operator -> () { return _p; }    

public:        T * operator -> () const { return _p; }    

public:         T** operator & () { Assert(_p==NULL); return &_p; }

public:        _reference & operator = (T * p) { assign(&_p, p); return *this; }

public:        _reference & operator = (const _reference<T> & r) { assign(&_p, r._p); return *this; }


};

#ifndef _DEBUG
#define _tsreference _reference
#else
template <class T> class _tsreference // thread safe reference (Debug Asserts on multi-threaded access)
{    
private:    T * _p;
#ifdef _DEBUG
            DWORD _dwThreadId;
#endif
            // special constructor to avoid re-initializing to 0 when embedded
public:     _tsreference(float)
            {
                Assert(_p == NULL);
            }


public:        _tsreference() : _p(NULL) {}

public:        _tsreference(T * p) : _p(p) 
               { 
#ifdef _DEBUG
                   _dwThreadId = GetCurrentThreadId();
#endif
                   if (_p) 
                   {
#ifdef RENTAL_MODEL
                       Assert(checkRentalAssign(this, _p));
#endif
                       _p->AddRef(); 
                   }
               }    

public:     _tsreference(const _tsreference<T> & r)
            {
#ifdef _DEBUG
                _dwThreadId = GetCurrentThreadId();
#endif
                _p = r._p; 
                if (_p) 
                {
#ifdef RENTAL_MODEL
                    Assert(checkRentalAssign(this, _p));
#endif
                    _p->AddRef();
                }
            }

public:        ~_tsreference() { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    release(&_p); }

public:        operator T * () const { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    return _p; }    

public:        T & operator * () { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    return *_p; }

public:        T * operator -> () { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    return _p; }    

public:        T * operator -> () const { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    return _p; }    

public:        _tsreference & operator = (T * p) { 
                    Assert(_dwThreadId == 0 || null == _p || _dwThreadId == GetCurrentThreadId());
                    assign(&_p, p); 
                    _dwThreadId = GetCurrentThreadId();
                    return *this; }

public:        _tsreference & operator = (const _tsreference<T> & r) { return operator=((T*)r._p); }

               // We can't guarentee thread safety on this method because we don't know
               // which thread is really going to assign the _p member - it currently
               // assumes that this thread will assign to it.
private:        T** operator & () { Assert(FALSE && "Don't use this method!");                                     
                                    _dwThreadId = GetCurrentThreadId();
                                    return &_p; }

};
#endif

void weakAssign(Object ** ppref, void * pref);

void weakRelease(Object ** ppref);

template <class T> class _weakreference
{    
private:    T * _p;

            // special constructor to avoid re-initializing to 0 when embedded
public:     _weakreference(float)
            {
                Assert(_p == NULL);
            }

public:        _weakreference() : _p(NULL) {}

public:        _weakreference(T * p) : _p(p) 
               { 
                   if (_p)
                   {
#ifdef RENTAL_MODEL
                       Assert(checkRentalAssign(this, _p));
#endif
                       _p->weakAddRef(); 
                   }
               }    

public:        _weakreference(const _weakreference<T> & r)
            {
                _p = r._p; 
                if (_p)
                {
#ifdef RENTAL_MODEL
                    Assert(checkRentalAssign(this, _p));
#endif
                    _p->weakAddRef();
                }
            }

public:        ~_weakreference() { weakRelease((Object **)&_p); }

public:        operator T * () const { return _p; }

public:        T & operator * () { return *_p; }

public:        T * operator -> () { return _p; }    

public:        _weakreference & operator = (T * p) { weakAssign((Object **)&_p, p); return *this; }

public:        _weakreference & operator = (const _weakreference<T> & r) { weakAssign((Object **)&_p, r._p); return *this; }
};



/*
    A ReadOnly Reference is like a Reference, but we have the option
    of telling it to AddRef or not.  In order to save ourselves from
    having another instance variable, we use the lowest bit of the 
    pointer to indicate whether it should be AddRef'd or not.

    Therefore, when we assign the pointer, we add the bit, and
    when we hand it out, we clear the bit.
*/

inline IUnknown * RawPointer(IUnknown * p) 
{ 
    return (IUnknown *) (LONG_PTR(p) & ~LONG_PTR(0x01));
}

inline IUnknown * MakePointer(IUnknown * p, const bool addRef) 
{ 
    return (IUnknown *)(LONG_PTR(p) | LONG_PTR(addRef));
}

inline bool IsAddRef(IUnknown * p) 
{
    return (LONG_PTR(p) & LONG_PTR(0x01));
}


template <class T> class _readonlyreference
{    
private:    T * _p;

private:    _readonlyreference() : _p(NULL) {};      // Don't want people doing this


public:     _readonlyreference(const bool addRef)
            {
                Assert(_p == NULL);

                // AddRef is 1 or 0.  Adding it to the 
                // pointer will set the lowest bit to the same.
                _p = (T *)addRef;
            };


            
public:     _readonlyreference(T * p, const bool addRef) 
            { 
                init(p, addRef);
            };    

               
public:     _readonlyreference(const _reference<T> & r, const bool addRef)
            {
                init(r._p, addRef);
            };

public:     _readonlyreference(const _readonlyreference<T> & r)
            {
                init(r.rawPointer(), r.isAddRef());
            };

public:     ~_readonlyreference() 
            { 
                if (isAddRef())
                {
                    _p = rawPointer();
                    release(&_p); 
                }
            }

public:   T * rawPointer() const
            { 
                return (T *) RawPointer(_p);
            }

public:        bool isAddRef() const {return IsAddRef(_p);}
public:         void setAddRef(const bool addRef) 
                {
                   Assert(_p == NULL || _p == ((T *)addRef));
                    _p = (T *)addRef;
                }

private:    void init(T * p, bool addRef)
            {
               Assert(_p == NULL || _p == ((T *)addRef));

               if (p) 
               {
#ifdef RENTAL_MODEL
                   Assert(checkRentalAssign(this, p));
#endif
                   if (addRef)
                   {
                      p->AddRef();
                   }
               }
               _p = makePointer(p, addRef);
            }                

public:         void assign(T *p, const bool addRef) 
                {
                    assignRO((IUnknown **) &_p, p, addRef);
                };

public:        operator T * () const 
               { 
                   return rawPointer();
               }    

public:        T & operator * () 
               { 
                   return *rawPointer(); 
               }

public:        T * operator -> () 
               { 
                   return rawPointer(); 
               }    

public:        T * operator -> () const 
               { 
                   return rawPointer(); 
               }    

private:         T** operator & () { Assert(_p==NULL); return &_p; }

public:        _readonlyreference & operator = (T * p) 
               {
                   bool addRef = isAddRef();

                   assign(p, addRef);

                   return *this; 
               }

               
public:        _readonlyreference & operator = (const _reference<T> & r) 
               { 
                   bool addRef = isAddRef();

                   assign(r._p, addRef);

                   return *this; 
               }


public:       _readonlyreference & operator = (const _readonlyreference<T> & r) 
               { 
                  assign(r.rawPointer(), r.isAddRef());

                  return *this;
               }
};




class _globalreference;

class _globalreference
{
public:     IUnknown * _p;

public:     _globalreference * _next;

public:     static _globalreference * Object;

public:     void assign(void * pref);
};

extern void ClearReferences();

template <class T> class _staticreference
{    
public:     T * _p;

public:     _globalreference * _next;

public:        operator T * () { return (T *)_p; }    

public:        operator T * () const { return (T *)_p; }    

public:        T & operator * () { return *(T *)_p; }

public:        T * operator -> () { return (T *)_p; }    

public:        T * operator -> () const { return (T *)_p; }    

public:     T ** operator & () { Assert(_p==NULL); return &(T *)_p; }

public:        _staticreference & operator = (T * p) { ((_globalreference *)this)->assign(p); return *this; }

//public:        _staticreference & operator = (const _staticreference<T> & r) { assign(&_p, r._p); return *this; }
};

#endif _REFERENCE_HXX



