/*
    ReactOS Kernel-Mode COM
    IUnknown implementations

    LICENSE
        Please see COPYING in the top-level directory for license information.

    AUTHORS
        Andrew Greenwood
*/

#include <stdunk.h>

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
