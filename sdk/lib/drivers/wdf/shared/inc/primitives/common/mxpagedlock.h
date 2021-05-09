/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxPagedLock.h

Abstract:

    Mode agnostic definition of paged lock

    See MxPagedLockKm.h and MxPagedLockUm.h for mode
    specific implementations

Author:



Revision History:



--*/

#pragma once

//
// MxPagedLockNoDynam has no c'tor/d'tor
// so as to be usable in global structs in km
//
// MxPagedLock dervies from it and adds c'tor/d'tor
//
class MxPagedLockNoDynam
{

DECLARE_DBGFLAG_INITIALIZED;

protected:
    MdPagedLock m_Lock;
public:
    _Must_inspect_result_
    __inline
    NTSTATUS
    Initialize(
        );

    __drv_maxIRQL(APC_LEVEL)
    __drv_setsIRQL(APC_LEVEL)
    __drv_savesIRQLGlobal(FastMutexObject, this->m_Lock)
    _Acquires_lock_(this->m_Lock)
    __inline
    VOID
    Acquire(
        );


    __inline
    VOID
    AcquireUnsafe(
        );

    _Must_inspect_result_
    __drv_maxIRQL(APC_LEVEL)
    __drv_savesIRQLGlobal(FastMutexObject, this->m_Lock)
    __drv_valueIs(==1;==0)
    __drv_when(return==1, __drv_setsIRQL(APC_LEVEL))
    _When_(return==1, _Acquires_lock_(this->m_Lock))
    __inline
    BOOLEAN
    TryToAcquire(
        );

    __drv_requiresIRQL(APC_LEVEL)
    __drv_restoresIRQLGlobal(FastMutexObject,this->m_Lock)
    _Releases_lock_(this->m_Lock)
    __inline
    VOID
    Release(
        );


    __inline
    VOID
    ReleaseUnsafe(
        );

    __inline
    VOID
    Uninitialize(
        );
};

class MxPagedLock : public MxPagedLockNoDynam
{
public:
    __inline
    MxPagedLock();

    __inline
    ~MxPagedLock();
};

