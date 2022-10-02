#pragma once

#ifdef SANITIZER_ENABLED
    VOID FASTCALL SanitizeReadPtr(LPCVOID ptr, UINT_PTR cb, BOOL bNullOK);
    VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK);
    VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK);
    VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK);

    VOID FASTCALL SanitizeHeapSystem(VOID);
    SIZE_T FASTCALL SanitizeHeapMemory(PVOID P, ULONG Tag);

    PVOID FASTCALL
    ExAllocatePoolWithTagSanitize(POOL_TYPE PoolType,
                                  SIZE_T NumberOfBytes,
                                  ULONG Tag);
    VOID FASTCALL
    ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree);

    #undef ExAllocatePoolWithTag
    #define ExAllocatePoolWithTag ExAllocatePoolWithTagSanitize

    #undef ExFreePoolWithTag
    #define ExFreePoolWithTag ExFreePoolWithTagSanitize
#else
    #define SanitizeReadPtr(ptr, cb, bNullOK)
    #define SanitizeWritePtr(ptr, cb, bNullOK)
    #define SanitizeStringPtrA(psz, bNullOK)
    #define SanitizeStringPtrW(psz, bNullOK)
    #define SanitizeHeapSystem()
    #define SanitizeHeapMemory(P, Tag) ((SIZE_T)0)
    #define ExAllocatePoolWithTagSanitize ExAllocatePoolWithTag
    #define ExFreePoolWithTagSanitize ExFreePoolWithTag
#endif
