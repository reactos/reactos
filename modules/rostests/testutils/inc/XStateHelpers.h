/*
 * PROJECT:     ReactOS tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for helper functions related to extended state
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <windef.h>
#include <winnt.h>

// These are not officially documented
#define XSTATE_PKRU                         9
#define XSTATE_HDC                          13
#define XSTATE_UINTR                        14
#define XSTATE_LBR                          15
#define XSTATE_MASK_PKRU                    (1LL << (XSTATE_PKRU))
#define XSTATE_MASK_HDC                     (1LL << (XSTATE_HDC))
#define XSTATE_MASK_UINTR                   (1LL << (XSTATE_UINTR))
#define XSTATE_MASK_LBR                     (1LL << (XSTATE_LBR))

#define XSTATE_MASK_SUPERVISOR \
    (XSTATE_MASK_IPT | \
     XSTATE_MASK_PASID | \
     XSTATE_MASK_CET_U | \
     XSTATE_MASK_CET_S | \
     XSTATE_MASK_HDC | \
     XSTATE_MASK_UINTR | \
     XSTATE_MASK_LBR)

EXTERN_C SIZE_T Get_UserShareData_XStateOffset(ULONG NtDdiVersion);

/* Accessors */
EXTERN_C PXSTATE_CONFIGURATION KUSER_SHARED_DATA_XState(PKUSER_SHARED_DATA _SharedUserData);
EXTERN_C ULONG64 XSTATE_CONFIGURATION_EnabledFeatures(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C ULONG64 XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C ULONG XSTATE_CONFIGURATION_Size(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C XSTATE_FEATURE* XSTATE_CONFIGURATION_Features(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C PULONG XSTATE_CONFIGURATION_AllFeatures(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C ULONG64 XSTATE_CONFIGURATION_AlignedFeatures(PXSTATE_CONFIGURATION XStateConfig);
EXTERN_C ULONG XSTATE_CONFIGURATION_ControlFlags(PXSTATE_CONFIGURATION XStateConfig);

/* Helper functions */
EXTERN_C ULONG XSTATE_CONFIGURATION_CalculateSizeByComponentMask(PXSTATE_CONFIGURATION XStateConfig, ULONG64 ComponentMask);
EXTERN_C ULONG XSTATE_CONFIGURATION_GetXSaveSize(PXSTATE_CONFIGURATION XStateConfig, ULONG64 ComponentMask);


#ifdef __cplusplus
template<ULONG NtDdiVersion>
struct TXSTATE_CONFIGURATION;

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN7>
{
    ULONGLONG EnabledFeatures;                                              //0x0
    ULONG Size;                                                             //0x8
    ULONG OptimizedSave:1;                                                  //0xc
    XSTATE_FEATURE Features[64];                                            //0x10
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN8>
{
    ULONGLONG EnabledFeatures;                                              //0x0
    ULONGLONG EnabledVolatileFeatures;                                      //0x8
    ULONG Size;                                                             //0x10
    union
    {
        ULONG ControlFlags;                                                 //0x14
        struct
        {
            ULONG OptimizedSave:1;                                          //0x14
            ULONG CompactionEnabled:1;                                      //0x14
            ULONG ExtendedFeatureDisable:1;                                 //0x14
        };
    };
    XSTATE_FEATURE Features[64];                                            //0x18
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN10> : TXSTATE_CONFIGURATION<NTDDI_WIN8>
{
    ULONGLONG EnabledSupervisorFeatures;                                    //0x218
    ULONGLONG AlignedFeatures;                                              //0x220
    ULONG AllFeatureSize;                                                   //0x228
    ULONG AllFeatures[64];                                                  //0x22c
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5> : TXSTATE_CONFIGURATION<NTDDI_WIN10>
{
    ULONGLONG EnabledUserVisibleSupervisorFeatures;                         //0x330
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN11> : TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5>
{
    ULONGLONG ExtendedFeatureDisableFeatures;                               //0x338
    ULONG AllNonLargeFeatureSize;                                           //0x340
    ULONG Spare;                                                            //0x344
};

template<ULONG NtDdiVersion>
TXSTATE_CONFIGURATION<NtDdiVersion>* GetOsXState(void)
{
    SIZE_T Offset = Get_UserShareData_XStateOffset(NtDdiVersion);
    PVOID Pointer = (PVOID)((ULONG_PTR)SharedUserData + Offset);
    return (TXSTATE_CONFIGURATION<NtDdiVersion>*)Pointer;
}

#endif // __cplusplus


EXTERN_C ULONG GetXStateNtDdiVersion(void);
