// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    Module Name: MIL

\*=========================================================================*/

#pragma once

/*=========================================================================*\
    CMILCOMBase - Base COM object for MIL

      See Management.(hpp|cpp) in Engine\common for the counterpart class
       which has its lifetime managed - CMILManagedCOMBase

\*=========================================================================*/

class CMILCOMBase : public IUnknown
{
private:
    
    LONG m_cRef;
    
public:

    CMILCOMBase();
    virtual ~CMILCOMBase();
    
    // internal IUnknown.
    ULONG InternalAddRef(void);
    ULONG InternalRelease(void);
    HRESULT InternalQueryInterface(__in_ecount(1) REFIID riid, __deref_opt_out void **ppvObject);

    // QI Support method
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject) { return E_NOINTERFACE; };
};


/*=========================================================================*\

    DECLARE_COM_BASE:

    Include this macro the public methods list for all MILCOM stubs.

\*=========================================================================*/

#define DECLARE_COM_BASE\
    STDMETHOD_(ULONG, AddRef)(void) {return InternalAddRef();}\
    STDMETHOD_(ULONG,Release)(void) {return InternalRelease();}\
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)\
    { return InternalQueryInterface(riid, ppvObject); } \

/*=========================================================================*\

    class CObjectUniqueness

\*=========================================================================*/

class CObjectUniqueness
{
public:
    CObjectUniqueness()
    {
        m_nUniqueCount = 1;
    }

    UINT GetUniqueCount() const
    {
        return m_nUniqueCount;
    }

    void UpdateUniqueCount()
    {
        m_nUniqueCount++;
        if (m_nUniqueCount == Invalid)
        {
            // We've wrapped around.  Wrap around to 1 instead of 0 to ensure
            // we always have an invalid uniqueness number

            m_nUniqueCount++;
        }
    }

    // Invalid number that represents something that always differs from our uniqueness 
    // value

    enum {Invalid = 0};

private:
    UINT m_nUniqueCount;
};


#ifdef DBG
//+-----------------------------------------------------------------------
//
//  Function:   DbgHasMultipleReferences
//
//  Synopsis:   Dbg-only method that returns whether or not there are
//              multiple references to the object.
//
//              This shouldn't be used in production code because it 
//              AddRef & Releases in an unperformant, non-synchronized manner
//              to avoid adding a GetCount entry to the vtable of CMILObject.
//              It can be used in debug builds to Assert the state of 
//              a ref counted object when the caller synchronizes access.
//
//  Returns:   - TRUE if there is more than 1 reference to this object
//             - FALSE otherwise
//
//------------------------------------------------------------------------
template <typename RefCountedObject>
BOOL 
DbgHasMultipleReferences(
    __in_ecount(1) RefCountedObject *pRefCountedObject // Object to obtain ref count for
    )
{
    Assert(pRefCountedObject);
    
    // AddRef & Release to get reference count
    UINT cRef = pRefCountedObject->AddRef();
    pRefCountedObject->Release();

    // Are there more references that the reference passed in and the one we just added?
    return (cRef > 2);      
}
#endif DBG

/**************************************************************************
*
* Class Description:
*
*   CUnknownWrapperNoRef
*
*   This template overrides the IUnknown::AddRef and IUnknown::Release methods 
*   to ignore reference counting.  This is useful for classes that implement IUnknown but
*   have been stack allocated, and should never be deleted.  Any QI requests will return
*   the appropriate interface but *not* actually bump up a reference count, so the callers
*   should be careful to manage the COM object life time when using this wrapper.
*
*
**************************************************************************/

template <class COMObject>
class CUnknownNoRefWrapper : public COMObject
{
public:
    CUnknownNoRefWrapper() : COMObject() {}

    // No ref count is maintained.
    
    ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) { return 0; }
};





