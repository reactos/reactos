#pragma once

#ifdef SANITIZER_ENABLED
    VOID FASTCALL SanitizeReadPtr(LPCVOID lp, UINT_PTR ucb, BOOL bCanBeNull);
    VOID FASTCALL SanitizeWritePtr(LPVOID lp, UINT_PTR ucb, BOOL bCanBeNull);
    VOID FASTCALL SanitizeStringPtrA(LPSTR lpsz, BOOL bCanBeNull);
    VOID FASTCALL SanitizeStringPtrW(LPWSTR lpsz, BOOL bCanBeNull);

    VOID FASTCALL SanitizeHeapSystem(VOID);
    VOID FASTCALL SanitizeHeapMemory(PVOID P, POOL_TYPE PoolType, ULONG Tag);

    PVOID FASTCALL
    ExAllocatePoolWithTagSanitize(POOL_TYPE PoolType,
                                  SIZE_T NumberOfBytes,
                                  ULONG Tag);
    VOID FASTCALL
    ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree);

    VOID FASTCALL SanitizeDoubleFreeSuspicious(PVOID P, SIZE_T NumberOfBytes);
#else
    #define SanitizeReadPtr(lp, ucb, bCanBeNull)
    #define SanitizeWritePtr(lp, ucb, bCanBeNull)
    #define SanitizeStringPtrA(lpsz, bCanBeNull)
    #define SanitizeStringPtrW(lpsz, bCanBeNull)
    #define SanitizeHeapSystem()
    #define SanitizeHeapMemory(P, PoolType, Tag)
    #define ExAllocatePoolWithTagSanitize ExAllocatePoolWithTag
    #define ExFreePoolWithTagSanitize ExFreePoolWithTag
#endif
