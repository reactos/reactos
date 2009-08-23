/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/topoology.c
 * PURPOSE:         KS CBaseUnknown functions
 * PROGRAMMER:      Johannes Anderwald
 *                  KJK::Hyperion
 */


#include "priv.h"

#ifdef _X86_
#define KS_DECL_CXX(RET_) extern RET_ __fastcall
#define KS_THIS(CLASS_)   CLASS_ * This, void * dummy_
#else
#define KS_DECL_CXX(RET_) extern RET_ __cdecl
#define KS_THIS(CLASS_)   CLASS_ * This
#endif

typedef struct
{
    INonDelegatedUnknownVtbl *lpVtbl;
    IIndirectedUnknownVtbl *lpVtblIndirectedUnknown;

    LONG m_RefCount;

    BOOLEAN m_UsingClassId;
    CLSID m_ClassId;
    IUnknown* m_UnknownOuter;

}IBaseUnknownImpl;



NTSTATUS
NTAPI
INonDelegatedUnknown_fnQueryInterface(
    INonDelegatedUnknown * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtbl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->m_RefCount);
        return STATUS_SUCCESS;
    }
    return STATUS_NOINTERFACE;
}

ULONG
NTAPI
INonDelegatedUnknown_fnAddRef(
    INonDelegatedUnknown * iface)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtbl);

    return InterlockedIncrement(&This->m_RefCount);
}

ULONG
NTAPI
INonDelegatedUnknown_fnRelease(
    INonDelegatedUnknown * iface)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtbl);

    InterlockedDecrement(&This->m_RefCount);

    /* Return new reference count */
    return This->m_RefCount;
}

static INonDelegatedUnknownVtbl vt_INonDelegatedUnknownVtbl =
{
    INonDelegatedUnknown_fnQueryInterface,
    INonDelegatedUnknown_fnAddRef,
    INonDelegatedUnknown_fnRelease
};

NTSTATUS
NTAPI
IIndirectedUnknown_fnQueryInterface(
    IIndirectedUnknown * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->QueryInterface(This->m_UnknownOuter, refiid, Output);
}

ULONG
NTAPI
IIndirectedUnknown_fnAddRef(
    IIndirectedUnknown * iface)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->AddRef(This->m_UnknownOuter);
}

ULONG
NTAPI
IIndirectedUnknown_fnRelease(
    IIndirectedUnknown * iface)
{
    IBaseUnknownImpl * This = (IBaseUnknownImpl*)CONTAINING_RECORD(iface, IBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->Release(This->m_UnknownOuter);
}

static IIndirectedUnknownVtbl vt_IIndirectedUnknownVtbl =
{
    IIndirectedUnknown_fnQueryInterface,
    IIndirectedUnknown_fnAddRef,
    IIndirectedUnknown_fnRelease
};


// On x86, the function is named @__CBaseUnknown_ConstructorWithGUID@16
// On non-x86, the function is named __CBaseUnknown_ConstructorWithGUID
KS_DECL_CXX(IBaseUnknownImpl *) CBaseUnknown_ConstructorWithGUID(KS_THIS(IBaseUnknownImpl), const GUID *lpGUID, IUnknown * OuterUnknown)
{

    This->lpVtbl = &vt_INonDelegatedUnknownVtbl;
    This->lpVtblIndirectedUnknown = &vt_IIndirectedUnknownVtbl;

    /* class uses class id */
    This->m_UsingClassId = TRUE;

    /* copy guid */
    RtlMoveMemory(&This->m_ClassId, lpGUID, sizeof(GUID));

    /* set refcount to zero */
    This->m_RefCount = 0;

    if (OuterUnknown)
    {
        /* use outer unknown */
        This->m_UnknownOuter = OuterUnknown;
    }
    else
    {
        /* use unknown from INonDelegatedUnknown */
        This->m_UnknownOuter = (PUNKNOWN)&This->lpVtbl;
    }

    /* return result */
    return This;
}

// On x86, the function is named @__CBaseUnknown_Constructor@12
// On non-x86, the function is named ___CBaseUnknown_Constructor
KS_DECL_CXX(IBaseUnknownImpl *) CBaseUnknown_Constructor(KS_THIS(IBaseUnknownImpl), IUnknown * OuterUnknown)
{

    This->lpVtbl = &vt_INonDelegatedUnknownVtbl;
    This->lpVtblIndirectedUnknown = &vt_IIndirectedUnknownVtbl;

    /* class uses class id */
    This->m_UsingClassId = FALSE;

    /* set refcount to zero */
    This->m_RefCount = 0;

    if (OuterUnknown)
    {
        /* use outer unknown */
        This->m_UnknownOuter = OuterUnknown;
    }
    else
    {
        /* use unknown from INonDelegatedUnknown */
        This->m_UnknownOuter = (PUNKNOWN)&This->lpVtbl;
    }

    /* return result */
    return This;
}

// On x86, the function is named @__CBaseUnknown_Destructor@12
// On non-x86, the function is named __CBaseUnknown_Destructor
KS_DECL_CXX(IBaseUnknownImpl *) CBaseUnknown_Destructor(KS_THIS(IBaseUnknownImpl), IUnknown * OuterUnknown)
{
    /* restore vtbl's */
    This->lpVtbl = &vt_INonDelegatedUnknownVtbl;
    This->lpVtblIndirectedUnknown = &vt_IIndirectedUnknownVtbl;


    if (OuterUnknown)
    {
        /* use outer unknown */
        This->m_UnknownOuter = OuterUnknown;
    }
    else
    {
        /* use unknown from INonDelegatedUnknown */
        This->m_UnknownOuter = (PUNKNOWN)&This->lpVtbl;
    }

    /* return result */
    return This;
}

KS_DECL_CXX(IBaseUnknownImpl *) CBaseUnknown_DefaultDestructor(KS_THIS(IBaseUnknownImpl))
{
    /* restore vtbl's */
    This->lpVtbl = &vt_INonDelegatedUnknownVtbl;
    This->lpVtblIndirectedUnknown = &vt_IIndirectedUnknownVtbl;


    /* return result */
    return This;
}

