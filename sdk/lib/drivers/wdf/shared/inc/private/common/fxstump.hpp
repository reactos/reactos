/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxStump.hpp

Abstract:

Author:

Revision History:

--*/

#ifndef _FXSTUMP_HPP_
#define _FXSTUMP_HPP_

struct FxStump {

protected:
    FxStump(
        VOID
        )
    {
    }

public:
    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        return FxPoolAllocate(FxDriverGlobals, NonPagedPool, Size);
    }

    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in POOL_TYPE PoolType
        )
    {
        return FxPoolAllocate(FxDriverGlobals, PoolType, Size);
    }

    VOID
    operator delete(
        __in PVOID pointer
        )
    {
        if (pointer) {
            FxPoolFree(pointer);
        }
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    PVOID
    operator new[](
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        return FxPoolAllocate(FxDriverGlobals, NonPagedPool, Size);
    }

    VOID
    operator delete[](
        __in PVOID pointer
        )
    {
        if (pointer) {
            FxPoolFree(pointer);
        }
    }

#endif

};

struct FxGlobalsStump : public FxStump {

private:
    PFX_DRIVER_GLOBALS m_Globals;

public:
    FxGlobalsStump(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        m_Globals = FxDriverGlobals;
    }

    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        )
    {
        return m_Globals;
    }
};

#endif // _FXSTUMP_HPP_

