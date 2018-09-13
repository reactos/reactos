#ifndef _INC_DSKQUOTA_AUTOPTR_H
#define _INC_DSKQUOTA_AUTOPTR_H
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

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4284)

template<class _TYPE> class a_ptr
{
public:

    typedef _TYPE element_type;

    a_ptr(_TYPE *_P = 0)
            : _Owns(_P != 0), _Ptr(_P)
    {}

    typedef _TYPE _U;

    a_ptr(const a_ptr<_U>& _Y) : _Owns(_Y._Owns), _Ptr((_TYPE *)_Y.disown())
    {}

    virtual void nukeit() = 0
    {
    }

    a_ptr<_TYPE>& operator=(const a_ptr<_U>& _Y)
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

    a_ptr<_TYPE>& replace(const a_ptr<_U>& _Y)
    {
        return *this = _Y;
    }

    virtual ~a_ptr()
    {
    }

    operator _TYPE*()
    {
        return get();
    }

    operator const _TYPE*() const
    {
        return get();
    }

    _TYPE& operator*() const
    {
        return (*get());
    }

    _TYPE *operator->() const
    {
        return (get());
    }

    _TYPE *get() const
    {
        return (_Ptr);
    }

    _TYPE *disown() const
    {
        ((a_ptr<_TYPE> *)this)->_Owns = FALSE;
        return (_Ptr);
    }

    _TYPE ** getaddr()
    {
        *this = (_TYPE *) NULL;
        _Owns = TRUE;
        return (&_Ptr);
    }

protected:

    BOOL _Owns;
    _TYPE *_Ptr;
};

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4284)
#endif

// autoptr
//

template<class _TYPE>
class autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit()
    {
        delete _Ptr;
    }

public:

    ~autoptr()
    {
        if (_Owns)
            this->nukeit();
    }

    autoptr(_TYPE *_P = 0)
        : a_ptr<_TYPE>(_P)
    {
    }

};


template<class _TYPE>
class array_autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit()
    {
        if (_Ptr)
            delete[] _Ptr;
    }

public:

    ~array_autoptr()
    {
        if (_Owns)
            this->nukeit();
    }

    array_autoptr(_TYPE *_P = 0)
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
    virtual void nukeit()
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

    ~sh_autoptr()
    {
        if (_Owns)
            this->nukeit();
    }

    sh_autoptr(_TYPE *_P = 0)
        : a_ptr<_TYPE>(_P)
    {
    }

};

// com_autoptr (nothing to do with ole automation... its an automatic ole ptr)
//
// Smart pointer that calls disown() on the referent when the pointer itself
// goes out of scope

template<class _TYPE>
class com_autoptr : public a_ptr<_TYPE>
{
    virtual void nukeit()
    {
        if (_Ptr)
            _Ptr->Release();
    }

public:

    ~com_autoptr()
    {
        if (_Owns)
            this->nukeit();
    }

    com_autoptr(_TYPE *_P = 0)
        : a_ptr<_TYPE>(_P)
    {
    }

};


#endif // _INC_DSKQUOTA_AUTOPTR_H

