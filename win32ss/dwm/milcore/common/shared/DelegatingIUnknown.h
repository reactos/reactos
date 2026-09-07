// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once
#include <combaseapi.h>
#include <system_error>
#include <cassert>
#include <utility>

#undef INTERFACE
#define INTERFACE IDelegatingIUnknown

DECLARE_INTERFACE_(IDelegatingIUnknown, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown methods
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    END_INTERFACE
};

interface __declspec(uuid("4c812044-98aa-470c-9676-7cd5550bbd3f")) IDelegatingIUnknown;

/// <summary>
/// This class is a helper for multiple-inheritance 
/// from two different IUnknowns, where the controlling/primary
/// IUnknown should always be delegated the responsibility
/// of managing <see cref="IUnknown::AddRef"/>, <see cref="IUnknown::Release"/>
/// and <see cref="IUnknown::QueryInterface"/>. 
/// 
/// This is especially helpful when a QI on the primary/controlling IUnknown
/// returns a pointer a secondary inheritance chain, and we'd like
/// to ensure that the caller continues to call into the correct 
/// version of the IUnknown for ref-counting.
/// </summary>
class  CDelegatingUnknown : 
    public IDelegatingIUnknown
{
private:
    IUnknown* m_pUnkOther;

public:
    typedef CDelegatingUnknown base;

protected:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <remarks>
    /// Protected, so that it can be called only by classes
    /// derived from <see cref="CDelegatingUnknown"/>
    /// </remarks>
    inline CDelegatingUnknown(IUnknown* pUnkOther)
        : m_pUnkOther(pUnkOther)
    {
        if (m_pUnkOther == nullptr)
        {
            throw std::logic_error("pUnkOther==nullptr");
        }
    }

public:

    inline virtual ~CDelegatingUnknown() { }

    inline STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) override
    {
        assert(m_pUnkOther != nullptr);
        return m_pUnkOther->QueryInterface(riid, ppv);
    }

    inline STDMETHOD_(ULONG, AddRef)(THIS) override
    {
        assert(m_pUnkOther != nullptr);
        return m_pUnkOther->AddRef();
    }

    inline STDMETHOD_(ULONG, Release)(THIS) override
    {
        assert(m_pUnkOther != nullptr);
        return m_pUnkOther->Release();
    }
};

/// <summary>
/// Declare a delegating IUnknown interface using this macro
/// Usage is similar to DECLARE_INTERFACE macro. For example:
/// <code>
/// <![CDATA[
/// DECLARE_DELEGATING_INTERFACE(IDpiProvider, "AB9362AC-E5EF-43DB-9D4A-556283341DC8")
/// {
///     BEGIN_INTERFACE
/// 
///     // IDpiProvider methods
///     STDMETHOD_(const DPI_AWARENESS_CONTEXT, GetDpiAwarenessContext)(THIS) const PURE;
///     STDMETHOD(GetCurrentDpi)(THIS_ DpiScale* pDpiScale) const PURE;
///     STDMETHOD_(BOOL, IsPerMonitorDpiAware)(THIS) const PURE;
/// 
///     END_INTERFACE
/// };
/// ]]>
/// </code>
/// </summary>
#define  DECLARE_DELEGATING_INTERFACE(iface,guid)       \
interface __declspec(uuid(guid)) iface;                 \
DECLARE_INTERFACE_(iface, IDelegatingIUnknown)          \


/// <summary>
/// Define a delegating IUnknown interface
/// </summary>
/// <param name="iface">Name of the interface previously declared using DECLARE_DELEGATING_INTERFACE</param>
/// <param name="impl">Name of the implementing class</param>
/// <remarks>
/// The implementing class must invoke the base class constructor and pass the controlling IUnknown
/// instance, as shown in the following example.
/// 
/// <code>base</code> is a special typedef exposed by the macro that refers to the base type that
/// looks for a pointer to the controlling IUnknown object.
/// <code>>
/// <![CDATA[
/// DEFINE_DELEGATING_INTERFACE(IDpiProvider, DpiProvider)
/// {
///    private:
///         DpiScale& m_dpi;
/// 
///    public:
///         inline DpiProvider(IUnknown*pControllingUnknown, const DpiScale& dpi) :
///             base(pControllingUknown),       // <<<<<<<< Pass controlling IUnknown to 'base' 
///             m_dpi(dpi)
///         {
///         }
/// };
/// ]]>
/// </code>
/// </remarks>
#define DEFINE_DELEGATING_INTERFACE(iface,impl)         \
class impl : public iface, public CDelegatingUnknown    \


