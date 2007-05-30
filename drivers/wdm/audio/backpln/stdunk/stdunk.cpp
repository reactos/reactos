/*
    TODO:
        Need to call ASSERT on m_ref_count to ensure it is valid.
*/

#define PUT_GUIDS_HERE

//#include <portcls.h>
#include <punknown.h>
#include <stdunk.h>

#include <ntddk.h>


/*
    HACK ALERT
    This is a little bit of a hack, but ReactOS doesn't seem to have this
    defined. TODO: Make the aligned test truly aligned.
*/

#define IsEqualGUID(a, b) \
    RtlEqualMemory(&a, &b, sizeof(GUID))

#define IsEqualGUIDAligned(a, b) \
    IsEqualGUID(a, b)

/*
    Shut the linker up - can also pass -defsym ___cxa_pure_virtual=0
*/
extern "C" void __cxa_pure_virtual(void) {}

/*
    IUnknown
*/

CUnknown::CUnknown(PUNKNOWN outer_unknown)
{
    m_ref_count = 0;

    if ( outer_unknown )
    {
        m_outer_unknown = outer_unknown;
    }
    else
    {
        m_outer_unknown = PUNKNOWN(dynamic_cast<PNONDELEGATINGUNKNOWN>(this));
    }
}

CUnknown::~CUnknown()
{
}

/*
    INonDelegatingUnknown
*/

STDMETHODIMP_(ULONG)
CUnknown::NonDelegatingAddRef(void)
{
    InterlockedIncrement(&m_ref_count);
    return m_ref_count;
}

STDMETHODIMP_(ULONG)
CUnknown::NonDelegatingRelease(void)
{
    if ( InterlockedDecrement(&m_ref_count) == 0 )
    {
        delete this;
        return 0;
    }

    return m_ref_count;
}

STDMETHODIMP_(NTSTATUS)
CUnknown::NonDelegatingQueryInterface(
    IN  REFIID  iid,
    IN  PVOID*  ppvoid)
{
    //if ( RtlEqualMemory(&iid, &IID_IUnknown, sizeof(GUID)) )
    {
        *ppvoid = (PVOID)((PUNKNOWN) this);
    }
 //   else
    {
        *ppvoid = NULL;
    }

    if ( *ppvoid )
    {
        ((PUNKNOWN)(*ppvoid))->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}
