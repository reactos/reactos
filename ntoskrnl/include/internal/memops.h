#pragma once

/* Kernel-safe memory operations for early boot and AMD64 */

#ifdef _WIN64

/* Kernel-safe zero memory function that avoids compiler builtins */
static __inline VOID
KiZeroMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length)
{
    volatile PULONG64 Ptr64;
    volatile PUCHAR Ptr8;
    SIZE_T i;
    SIZE_T Count64;
    SIZE_T Remainder;
    
    /* For large allocations, use 64-bit writes for better performance */
    if (Length >= 8)
    {
        /* Align to 8 bytes if needed */
        Ptr8 = (volatile PUCHAR)Destination;
        while (((ULONG_PTR)Ptr8 & 7) && Length > 0)
        {
            *Ptr8++ = 0;
            Length--;
        }
        
        /* Now do 64-bit writes */
        Ptr64 = (volatile PULONG64)Ptr8;
        Count64 = Length / 8;
        Remainder = Length & 7;
        
        /* Zero 8 bytes at a time */
        for (i = 0; i < Count64; i++)
        {
            Ptr64[i] = 0;
        }
        
        /* Zero any remaining bytes */
        Ptr8 = (volatile PUCHAR)&Ptr64[Count64];
        for (i = 0; i < Remainder; i++)
        {
            Ptr8[i] = 0;
        }
    }
    else
    {
        /* Small allocation, just use byte writes */
        Ptr8 = (volatile PUCHAR)Destination;
        for (i = 0; i < Length; i++)
        {
            Ptr8[i] = 0;
        }
    }
}

/* Override RtlZeroMemory for kernel use on AMD64 */
#ifdef RtlZeroMemory
#undef RtlZeroMemory
#endif
#define RtlZeroMemory(Destination, Length) KiZeroMemory((Destination), (Length))

/* Kernel-safe copy memory function */
static __inline VOID
KiCopyMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID *Source,
    _In_ SIZE_T Length)
{
    volatile PULONG64 Dest64;
    PULONG64 Src64;
    volatile PUCHAR Dest8;
    PUCHAR Src8;
    SIZE_T i;
    SIZE_T Count64;
    SIZE_T Remainder;
    
    /* For large copies, use 64-bit copies for better performance */
    if (Length >= 8 && 
        (((ULONG_PTR)Destination & 7) == ((ULONG_PTR)Source & 7)))
    {
        /* Align to 8 bytes if needed */
        Dest8 = (volatile PUCHAR)Destination;
        Src8 = (PUCHAR)Source;
        while (((ULONG_PTR)Dest8 & 7) && Length > 0)
        {
            *Dest8++ = *Src8++;
            Length--;
        }
        
        /* Now do 64-bit copies */
        Dest64 = (volatile PULONG64)Dest8;
        Src64 = (PULONG64)Src8;
        Count64 = Length / 8;
        Remainder = Length & 7;
        
        /* Copy 8 bytes at a time */
        for (i = 0; i < Count64; i++)
        {
            Dest64[i] = Src64[i];
        }
        
        /* Copy any remaining bytes */
        Dest8 = (volatile PUCHAR)&Dest64[Count64];
        Src8 = (PUCHAR)&Src64[Count64];
        for (i = 0; i < Remainder; i++)
        {
            Dest8[i] = Src8[i];
        }
    }
    else
    {
        /* Unaligned or small copy, use byte copies */
        Dest8 = (volatile PUCHAR)Destination;
        Src8 = (PUCHAR)Source;
        for (i = 0; i < Length; i++)
        {
            Dest8[i] = Src8[i];
        }
    }
}

/* Override RtlCopyMemory for kernel use on AMD64 */
#ifdef RtlCopyMemory
#undef RtlCopyMemory
#endif
#define RtlCopyMemory(Destination, Source, Length) KiCopyMemory((Destination), (Source), (Length))

/* Kernel-safe fill memory function */
static __inline VOID
KiFillMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ UCHAR Fill)
{
    volatile PUCHAR Ptr = (volatile PUCHAR)Destination;
    SIZE_T i;
    
    /* Use volatile to prevent compiler optimization */
    for (i = 0; i < Length; i++)
    {
        Ptr[i] = Fill;
    }
}

/* Override RtlFillMemory for kernel use on AMD64 */
#ifdef RtlFillMemory
#undef RtlFillMemory
#endif
#define RtlFillMemory(Destination, Length, Fill) KiFillMemory((Destination), (Length), (Fill))

#endif /* _WIN64 */

/* EOF */