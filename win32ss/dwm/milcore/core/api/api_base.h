// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//      Declaration of base MIL API object, CMILObject, and template class,
//      LocalMILObject, which can wrap a CMILObject for use in a limited scope
//      of existance such as on the stack.
//

#ifndef __API_BASE_H__
#define __API_BASE_H__

/*=========================================================================*\
    CMILObject - MIL base object interface
\*=========================================================================*/

class CMILObject :
    public CMILCOMBase
{
public:
    DECLARE_COM_BASE

    CMILObject(
        __in_ecount_opt(1) CMILFactory *pFactory
        );
    virtual ~CMILObject();

    /* QI Support method */
    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppvObject
        );

protected:
    CMILFactory * const m_pFactory;
};

#define DECLARE_MIL_OBJECT\
    DECLARE_COM_BASE\
    \
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject); \

/*=========================================================================*/

//+----------------------------------------------------------------------------
//
//  Template:  LocalMILObject
//
//  Synopsis:  Template class for creating a CMILObject in a limited scope of
//             existance.  Reference counting is allowed, but it is illegal to
//             a) try to delete a LocalMILObject using Release
//             b) hold onto a LocalMILObject longer than its scope would allow
//
//  Notes:     All objects must be on the stack (as a local) or a member.  Once
//             an instance of this class goes out of scope the object is
//             destroyed.
//
//             Objects may be allocated through a CBufferDispenser if the class
//             already has support for that.
//

template <class MILObject>
class LocalMILObject : public MILObject
{
public:

    //
    // Constructor
    //
    //   Local objects don't have a factory association; so, any MILObject
    //   which allows local usage must provide a default contructor capable of
    //   handling this.
    //

    LocalMILObject() : MILObject() {
    #if DBG
        // start the refcount at 1
        m_uDbgRefCount = 1;
    #endif
    }
#if DBG
    ~LocalMILObject() {
        // Assert that no other object maintains a reference to this object
        // when it is being deconstructed
        AssertMsgW(m_uDbgRefCount == 1, L"LocalMILObject has been leaked!");
    }
#endif

private:

    //
    // Illegal allocation operators
    //
    //   These are declared, but not defined such that any use that gets around
    //   the private protection will generate a link time error.
    //

    __allocator __out_bcount(cb) void * operator new(size_t cb);
    __allocator __out_bcount(cb) void * operator new[](size_t cb);
    __out_bcount(cb) void * operator new(size_t cb, __out_bcount(cb) void * pv);
    // delete is not overriden to allow CBufferDispenser usage

    //
    // AddRef and Release are overriden to avoid the cost of InterlockedIncrement
    //
    ULONG STDMETHODCALLTYPE AddRef(void) override
    {
    #if DBG 
        ++m_uDbgRefCount; 
    #endif
    
        // Don't do anything in retail (avoids cost of InterlockedIncrement)
        return 1;
     }

    ULONG STDMETHODCALLTYPE Release(void) override
    {
    #if DBG
        // Assert that nothing is trying to delete this object through Release
        AssertMsgW(m_uDbgRefCount > 1, L"Attempt to delete a LocalMILObject through Release()");
        --m_uDbgRefCount;
    #endif
    
        // Don't do anything in retail (avoids cost of InterlockedIncrement)
        return 1;
    }

#if DBG
    //
    // The following methods are debug only to avoid potentially creating
    // an extra v-table in a retail build.
    //

    // Illegal IUnknown interfaces

    HRESULT STDMETHODCALLTYPE QueryInterface(
        __in_ecount(1) REFIID riid, 
        __deref_out void **ppvObject
        )
    {
        Assert(FALSE);
        *ppvObject = NULL;
        RRETURN(E_FAIL);
    }

    // Illegal CMILCOMBase interfaces

    HRESULT STDMETHODCALLTYPE HrFindInterface(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppvObject
        )
    {
        Assert(FALSE);
        *ppvObject = NULL;
        RRETURN(E_FAIL);
    }
#endif

#if DBG
    // Additional reference count added here so as not to make debug and free
    // versions differ in behavior. (Changing the base class reference count
    // when a subclass read this member would be bad.)
    UINT m_uDbgRefCount;
#endif
};

#endif /* !__API_BASE_H__ */


