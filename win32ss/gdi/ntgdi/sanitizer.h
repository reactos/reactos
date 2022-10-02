#pragma once

#ifdef SANITIZER_ENABLED
    VOID FASTCALL SanitizeReadPtr(LPCVOID ptr, UINT_PTR cb, BOOL bCanBeNull);
    VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bCanBeNull);
    VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bCanBeNull);
    VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bCanBeNull);

    VOID FASTCALL SanitizeHeapSystem(VOID);
    SIZE_T FASTCALL SanitizeHeapMemory(PVOID P, ULONG Tag);

    PVOID FASTCALL
    ExAllocatePoolWithTagSanitize(POOL_TYPE PoolType,
                                  SIZE_T NumberOfBytes,
                                  ULONG Tag);
    VOID FASTCALL
    ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree);
#else
    #define SanitizeReadPtr(ptr, cb, bCanBeNull)
    #define SanitizeWritePtr(ptr, cb, bCanBeNull)
    #define SanitizeStringPtrA(psz, bCanBeNull)
    #define SanitizeStringPtrW(psz, bCanBeNull)
    #define SanitizeHeapSystem()
    #define SanitizeHeapMemory(P, Tag) ((SIZE_T)0)
    #define ExAllocatePoolWithTagSanitize ExAllocatePoolWithTag
    #define ExFreePoolWithTagSanitize ExFreePoolWithTag
#endif
