/*
    "Unknown" implementation, in C
    by Andrew Greenwood

    Not quite sure how this is used, but the C++ variant is intended for
    implementing a NonDelegatingUnknown object
*/

#include <stdunk.h>

STDMETHODCALLTYPE
NTSTATUS
Unknown_QueryInterface(
    IUnknown* this,
    IN  REFIID refiid,
    OUT PVOID* output)
{
    /* TODO */
    return STATUS_SUCCESS;
}

STDMETHODCALLTYPE
ULONG
Unknown_AddRef(
    IUnknown* unknown_this)
{
    struct CUnknown* this = CONTAINING_RECORD(unknown_this, struct CUnknown, IUnknown);

    InterlockedIncrement(&this->m_ref_count);
    return this->m_ref_count;
}

STDMETHODCALLTYPE
ULONG
Unknown_Release(
    IUnknown* unknown_this)
{
    struct CUnknown* this = CONTAINING_RECORD(unknown_this, struct CUnknown, IUnknown);

    InterlockedDecrement(&this->m_ref_count);

    if ( this->m_ref_count == 0 )
    {
        ExFreePool(this);
        return 0;
    }

    return this->m_ref_count;
}


/*
    The vtable for Unknown
*/

const IUnknownVtbl UnknownVtbl =
{
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

