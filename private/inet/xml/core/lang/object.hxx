/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_LANG_OBJECT
#define _CORE_LANG_OBJECT

#include <unknwn.h>

class DLLEXPORT Class;
typedef _staticreference<Class> SRClass;

class String;

extern DLLEXPORT TAG tagRefCount;
extern DLLEXPORT TAG tagPointerCache;

class NOVTABLE DLLEXPORT Object : public IUnknown
{
friend class Base;

    protected:    Object();

    protected:    virtual ~Object();

            /**
             * Return the this pointer of the object.
             * (useful to get to the main object from an interface)
             */
    public:    virtual Base * getBase() = 0;

    public:    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject) { return E_NOINTERFACE; }

    public:    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;

    public:    virtual ULONG STDMETHODCALLTYPE Release() = 0;

    public:    virtual void weakAddRef() = 0;

    public:    virtual void weakRelease() = 0;

    /**
     */
    public: virtual Class * getClass() const { return null; }

    /**
     */
    public: virtual int hashCode();

    /**
     */
    public: virtual bool equals(Object * obj);

    /**
     */
    public: virtual Object * clone(); //throws CloneNotSupportedException;

    /**
     */
    public: virtual String * toString();

    /**
     */
    protected: virtual void finalize(); //throws Throwable { }
};

#include "core/base/base.hxx"

DEFINE_CLASS(Object);
        

#endif _CORE_LANG_OBJECT
