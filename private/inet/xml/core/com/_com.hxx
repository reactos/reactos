/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _COM_HXX
#define _COM_HXX

#ifndef _STATICUNKNOWN_HXX
#include "staticunknown.hxx"
#endif

#ifdef UNIX
#ifndef _CORE_LANG_OBJECT
#include "../lang/object.hxx"
#endif
#endif // UNIX


EXTERN_C const IID IID_Object;

class NOVTABLE __comexport : public __unknown
{
protected:  RObject _pWrapped;

public:     __comexport(const IID * riid, Object * p);

protected:  virtual ~__comexport();

public:     HRESULT QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject);

public:     ULONG Release();

public:     static Object * getExported(IUnknown * p, REFIID refIID);

};

template <class T, class I, const IID * I_IID> class NOVTABLE _comexport : public I, public __comexport
{
public:     _comexport<T, I, I_IID>(T * pWrapped) : __comexport(I_IID, reinterpret_cast<Object *>(pWrapped)) {}

public:     virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
            {
                return __comexport::QueryInterface((I *)this, iid, ppvObject);
            }

public:     virtual ULONG STDMETHODCALLTYPE AddRef()
            {
                return __comexport::AddRef();
            }

public:     virtual ULONG STDMETHODCALLTYPE Release()
            {
                return __comexport::Release();
            }

public:     T * getWrapped()
            {
                return reinterpret_cast<T *>((Object *)_pWrapped);
            }

public:     static T * getExported(IUnknown * p, REFIID refIID)
            {
                return reinterpret_cast<T *>(__comexport::getExported(p, refIID));
            }
};

template <class I, class T> class NOVTABLE _comimport : public Base, public T
{
    DECLARE_CLASS_MEMBERS_I1(_comimport, Base, T);

protected:    _reference<I>    _wrapped;

public:        _comimport(I * p) : _wrapped(p) {}


protected:  virtual void finalize()
            {
                _wrapped = null;
            }
};

#endif _COM_HXX




