/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/topoology.c
 * PURPOSE:         KS CBaseUnknown functions
 * PROGRAMMER:      Johannes Anderwald
 *                  KJK::Hyperion
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

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

}CBaseUnknownImpl;



NTSTATUS
NTAPI
INonDelegatedUnknown_fnQueryInterface(
    INonDelegatedUnknown * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtbl);

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
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtbl);

    return InterlockedIncrement(&This->m_RefCount);
}

ULONG
NTAPI
INonDelegatedUnknown_fnRelease(
    INonDelegatedUnknown * iface)
{
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtbl);

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
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->QueryInterface(This->m_UnknownOuter, refiid, Output);
}

ULONG
NTAPI
IIndirectedUnknown_fnAddRef(
    IIndirectedUnknown * iface)
{
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->AddRef(This->m_UnknownOuter);
}

ULONG
NTAPI
IIndirectedUnknown_fnRelease(
    IIndirectedUnknown * iface)
{
    CBaseUnknownImpl * This = (CBaseUnknownImpl*)CONTAINING_RECORD(iface, CBaseUnknownImpl, lpVtblIndirectedUnknown);

    return This->m_UnknownOuter->lpVtbl->Release(This->m_UnknownOuter);
}

static IIndirectedUnknownVtbl vt_IIndirectedUnknownVtbl =
{
    IIndirectedUnknown_fnQueryInterface,
    IIndirectedUnknown_fnAddRef,
    IIndirectedUnknown_fnRelease
};


KS_DECL_CXX(CBaseUnknownImpl *) CBaseUnknown_ConstructorWithGUID(KS_THIS(CBaseUnknownImpl), const GUID *lpGUID, IUnknown * OuterUnknown)
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

KS_DECL_CXX(CBaseUnknownImpl *) CBaseUnknown_Constructor(KS_THIS(CBaseUnknownImpl), IUnknown * OuterUnknown)
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

KS_DECL_CXX(VOID) CBaseUnknown_Destructor(KS_THIS(CBaseUnknownImpl), IUnknown * OuterUnknown)
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
}

KS_DECL_CXX(VOID) CBaseUnknown_DefaultDestructor(KS_THIS(CBaseUnknownImpl))
{
    /* restore vtbl's */
    This->lpVtbl = &vt_INonDelegatedUnknownVtbl;
    This->lpVtblIndirectedUnknown = &vt_IIndirectedUnknownVtbl;
}

