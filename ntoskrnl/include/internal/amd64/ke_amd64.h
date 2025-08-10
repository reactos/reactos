/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/amd64/ke_amd64.h
 * PURPOSE:         AMD64 Kernel Support Macros
 * PROGRAMMERS:     ReactOS Team
 */

#pragma once

/* 
 * AMD64 Cross-Module Function Call Support
 * 
 * On AMD64, the kernel uses the small memory model (-mcmodel=small) which limits
 * RIP-relative addressing to ±2GB. When the kernel is relocated or when calling
 * functions in different modules that may be far apart in memory, direct calls
 * can fail or cause crashes.
 * 
 * These macros provide a workaround by using indirect calls through function
 * pointers, which use absolute addressing instead of RIP-relative addressing.
 */

/* Macro to safely call a cross-module function on AMD64 */
#define AMD64_SAFE_CALL(FuncName, FuncType, ...)                               \
    do {                                                                        \
        volatile FuncType pfn = (FuncType)&FuncName;                          \
        if (pfn && (ULONG_PTR)pfn > 0xFFFFF80000000000ULL) {                  \
            pfn(__VA_ARGS__);                                                  \
        }                                                                       \
    } while (0)

/* Macro to safely call a cross-module function with return value on AMD64 */
#define AMD64_SAFE_CALL_RET(RetVar, FuncName, FuncType, ...)                  \
    do {                                                                        \
        volatile FuncType pfn = (FuncType)&FuncName;                          \
        if (pfn && (ULONG_PTR)pfn > 0xFFFFF80000000000ULL) {                  \
            RetVar = pfn(__VA_ARGS__);                                        \
        }                                                                       \
    } while (0)

/* Special macro for functions with no arguments */
#define AMD64_SAFE_CALL_NOARGS(FuncName, FuncType)                            \
    do {                                                                        \
        volatile FuncType pfn = (FuncType)&FuncName;                          \
        if (pfn && (ULONG_PTR)pfn > 0xFFFFF80000000000ULL) {                  \
            pfn();                                                             \
        }                                                                       \
    } while (0)

/* Special macro for functions with no arguments but with return value */
#define AMD64_SAFE_CALL_RET_NOARGS(RetVar, FuncName, FuncType)               \
    do {                                                                        \
        volatile FuncType pfn = (FuncType)&FuncName;                          \
        if (pfn && (ULONG_PTR)pfn > 0xFFFFF80000000000ULL) {                  \
            RetVar = pfn();                                                    \
        }                                                                       \
    } while (0)

/* Function type definitions for common kernel functions */
typedef VOID (NTAPI *PFN_PO_INIT_PRCB)(IN PKPRCB Prcb);
typedef VOID (NTAPI *PFN_KI_SAVE_PROC_STATE)(IN PKPROCESSOR_STATE ProcessorState);
typedef VOID (NTAPI *PFN_KI_GET_CACHE_INFO)(VOID);
typedef BOOLEAN (NTAPI *PFN_HAL_INIT_SYSTEM)(IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock);
typedef BOOLEAN (NTAPI *PFN_MM_INIT_SYSTEM)(IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock);
typedef BOOLEAN (NTAPI *PFN_OB_INIT_SYSTEM)(VOID);
typedef BOOLEAN (NTAPI *PFN_SE_INIT_SYSTEM)(VOID);
typedef BOOLEAN (NTAPI *PFN_PS_INIT_SYSTEM)(IN PLOADER_PARAMETER_BLOCK LoaderBlock);
typedef BOOLEAN (NTAPI *PFN_PP_INIT_SYSTEM)(VOID);

/* Helper macro to check if a function pointer is valid */
#define IS_VALID_KERNEL_PTR(ptr) \
    ((ptr) != NULL && (ULONG_PTR)(ptr) > 0xFFFFF80000000000ULL)

/* Debug output helpers for AMD64 kernel */
#define AMD64_DEBUG_PRINT(msg)                                                 \
    do {                                                                        \
        const char *p = (msg);                                                 \
        while (*p) {                                                           \
            while ((__inbyte(0x3F8 + 5) & 0x20) == 0);                       \
            __outbyte(0x3F8, *p++);                                           \
        }                                                                       \
    } while (0)
