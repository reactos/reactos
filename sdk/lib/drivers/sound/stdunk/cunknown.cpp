/*
    ReactOS Kernel-Mode COM
    IUnknown implementations

    LICENSE
        Please see COPYING in the top-level directory for license information.

    AUTHORS
        Andrew Greenwood
*/

#include <stdunk.h>

inline
PVOID
KCOM_New(
    size_t size,
    POOL_TYPE pool_type,
    ULONG tag)
{
    PVOID result;

    result = ExAllocatePoolWithTag(pool_type, size, tag);

    if (result)
        RtlZeroMemory(result, size);

    return result;
}

PVOID
__cdecl
operator new(
    size_t size,
    POOL_TYPE pool_type)
{
    return KCOM_New(size, pool_type, 'wNcP');
}

PVOID
__cdecl
operator new(
    size_t size,
    POOL_TYPE pool_type,
    ULONG tag)
{
    return KCOM_New(size, pool_type, tag);
}

void
__cdecl
operator delete(
    PVOID ptr)
{
    ExFreePool(ptr);
}

void
__cdecl
operator delete(
    PVOID ptr, UINT_PTR)
{
    ExFreePool(ptr);
}

CUnknown::CUnknown(PUNKNOWN outer_unknown)
{
    m_ref_count = 0;

    if ( outer_unknown )
        m_outer_unknown = outer_unknown;
    else
        m_outer_unknown = PUNKNOWN(dynamic_cast<PNONDELEGATINGUNKNOWN>(this));
}

CUnknown::~CUnknown()
{
}

STDMETHODIMP_(ULONG)
CUnknown::NonDelegatingAddRef()
{
    InterlockedIncrement(&m_ref_count);
    return m_ref_count;
}

STDMETHODIMP_(ULONG)
CUnknown::NonDelegatingRelease()
{
    if ( InterlockedDecrement(&m_ref_count) == 0 )
    {
        m_ref_count ++;
        delete this;
        return 0;
    }

    return m_ref_count;
}

STDMETHODIMP_(NTSTATUS)
CUnknown::NonDelegatingQueryInterface(
    IN  REFIID iid,
    PVOID* ppVoid)
{
    /* FIXME */
    #if 0
    if ( IsEqualGUID(iid, IID_IUnknown) )   /* TODO: Aligned? */
        *ppVoid = PVOID(PUNKNOWN(this));
    else
        *ppVoid = NULL;
    #endif

    if ( *ppVoid )
    {
        PUNKNOWN(*ppVoid)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

#if __GNUC__
extern "C" void __cxa_pure_virtual() { ASSERT(FALSE); }
#endif
