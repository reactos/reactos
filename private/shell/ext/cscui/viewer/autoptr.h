//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       autoptr.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_AUTOPTR_H
#define _INC_CSCVIEW_AUTOPTR_H
///////////////////////////////////////////////////////////////////////////////
/*  File: autoptr.h

    Description: Template auto pointer classes to support normal C++ pointers
        as well as shell and COM object pointers.

        This code was created by DavePl for the Entertainment Center project.
        It worked very well so I've "borrowed" it (thanks Dave).  I think his
        original implementation borrowed from the STL implementation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////


// a_ptr
//
// Safe pointer class that knows to delete the referrent object when
// the pointer goes out of scope or is replaced, etc.

template<class _TYPE> class a_ptr 
{
public:

    typedef _TYPE element_type;
    
    a_ptr(_TYPE *_P = 0) throw()
            : _Owns(_P != 0), _Ptr(_P) 
    {}
    
    typedef _TYPE _U;
    
    a_ptr(const a_ptr<_U>& _Y) throw()
        : _Owns(_Y._Owns), _Ptr((_TYPE *)_Y.disown()) 
    {}

    virtual void nukeit() throw() = 0
    {
    }

    a_ptr<_TYPE>& operator=(const a_ptr<_U>& _Y) throw()
    {
        if ((void *)this != (void *)&_Y)
        {
            if (_Owns)
                nukeit();
            _Owns = _Y._Owns;
            _Ptr = (_TYPE *)_Y.disown();

//            ASSERT( !_Owns || _Ptr );
        }
        return (*this); 
    }

    a_ptr<_TYPE>& replace(const a_ptr<_U>& _Y) throw()
    {
        return *this = _Y;
    }

    virtual ~a_ptr() throw()
    {
    }
    
    operator _TYPE*() throw()
    { 
        return get(); 
    }

    operator const _TYPE*() const throw()
    { 
        return get(); 
    }
            
    _TYPE& operator*() const throw()
    {
        return (*get()); 
    }

    _TYPE *get() const throw()
    {
        return (_Ptr); 
    }

    _TYPE *disown() const throw()
    {
        ((a_ptr<_TYPE> *)this)->_Owns = FALSE;
        return (_Ptr); 
    }

    _TYPE ** getaddr() throw()
    { 
        *this = (_TYPE *) NULL;
        _Owns = TRUE;
        return (&_Ptr); 
    }

protected:

    BOOL _Owns;
    _TYPE *_Ptr;
};

// autoptr
//

template<class _TYPE>
class autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit() throw()
    {
        delete _Ptr;
    }

public:

    ~autoptr() throw()
    {
        if (_Owns)
            this->nukeit();
    }

    autoptr(_TYPE *_P = 0) throw()
        : a_ptr<_TYPE>(_P)
    {
    }

    _TYPE *operator->() const throw()
    {
        return (get()); 
    }
};


template<class _TYPE>
class array_autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit() throw()
    {
        if (_Ptr)
            delete[] _Ptr;
    }

public:

    ~array_autoptr() throw()
    {
        if (_Owns)
            this->nukeit();
    }

    array_autoptr(_TYPE *_P = 0) throw()
        : a_ptr<_TYPE>(_P)
    {
    }

};



// sh_autoptr
//
// Smart pointer that manually runs the referent's destructor and then
// calls the shell's task allocator to free the object's memory footprint

template<class _TYPE>
class sh_autoptr : virtual public a_ptr<_TYPE>
{
    virtual void nukeit() throw()
    {
        if (_Ptr)
        {
            IMalloc *pMalloc;
            _Ptr->~_TYPE();
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                pMalloc->Free(_Ptr);
                pMalloc->Release();
            }
        }
    }

public:

    ~sh_autoptr() throw()
    {
        if (_Owns)
            this->nukeit();
    }

    sh_autoptr(_TYPE *_P = 0) throw()
        : a_ptr<_TYPE>(_P)
    {
    }

    _TYPE *operator->() const throw()
    {
        return (get()); 
    }
};

// com_autoptr (nothing to do with ole automation... its an automatic ole ptr)
//
// Smart pointer that calls disown() on the referent when the pointer itself
// goes out of scope

template<class _TYPE>
class com_autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit() throw()
    {
        if (_Ptr)
            _Ptr->Release();
    }

public:

    ~com_autoptr() throw()
    {
        if (_Owns)
            this->nukeit();
    }

    com_autoptr(_TYPE *_P = 0) throw()
        : a_ptr<_TYPE>(_P)
    {
    }

    _TYPE *operator->() const throw()
    {
        return (get()); 
    }
};


#endif // _INC_CSCVIEW_AUTOPTR_H
 
