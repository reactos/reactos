/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     Safe memory operations for early boot environment
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

#include <freeldr.h>

/*
 * Custom memory operations for AMD64 that avoid SSE/AVX instructions
 * in early boot environment.
 */

#ifdef _M_AMD64

/* Simple byte-by-byte operations without aggressive optimizations */
VOID
NTAPI
FrLdrZeroMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length)
{
    volatile UCHAR* Dest = (volatile UCHAR*)Destination;
    SIZE_T i;
    
    /* Safety check */
    if (Destination == NULL || Length == 0)
        return;
    
    /* Simple byte-by-byte zeroing to avoid alignment issues */
    for (i = 0; i < Length; i++)
    {
        Dest[i] = 0;
    }
}

VOID
NTAPI
FrLdrCopyMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length)
{
    UCHAR* Dest = (UCHAR*)Destination;
    const UCHAR* Src = (const UCHAR*)Source;
    SIZE_T i;
    
    for (i = 0; i < Length; i++)
    {
        Dest[i] = Src[i];
    }
}

VOID
NTAPI
FrLdrFillMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ UCHAR Fill)
{
    UCHAR* Dest = (UCHAR*)Destination;
    SIZE_T i;
    
    for (i = 0; i < Length; i++)
    {
        Dest[i] = Fill;
    }
}

VOID
NTAPI
FrLdrMoveMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length)
{
    UCHAR* Dest = (UCHAR*)Destination;
    const UCHAR* Src = (const UCHAR*)Source;
    SIZE_T i;
    
    /* Handle overlapping memory regions */
    if (Dest > Src && Dest < Src + Length)
    {
        /* Copy backwards */
        for (i = Length; i > 0; i--)
        {
            Dest[i - 1] = Src[i - 1];
        }
    }
    else
    {
        /* Copy forwards */
        for (i = 0; i < Length; i++)
        {
            Dest[i] = Src[i];
        }
    }
}

#else /* !_M_AMD64 */

/* For non-AMD64 builds, use standard implementations */
VOID
NTAPI
FrLdrZeroMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length)
{
    RtlZeroMemory(Destination, Length);
}

VOID
NTAPI
FrLdrCopyMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length)
{
    RtlCopyMemory(Destination, Source, Length);
}

VOID
NTAPI
FrLdrFillMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ UCHAR Fill)
{
    RtlFillMemory(Destination, Length, Fill);
}

VOID
NTAPI
FrLdrMoveMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) const VOID* Source,
    _In_ SIZE_T Length)
{
    RtlMoveMemory(Destination, Source, Length);
}

#endif /* _M_AMD64 */

/* Custom memset implementation for FreeLoader */
#ifdef _M_AMD64
void* memset(void* dest, int ch, size_t count)
{
    volatile unsigned char* p = (volatile unsigned char*)dest;
    unsigned char c = (unsigned char)ch;
    size_t i;
    
    for (i = 0; i < count; i++)
    {
        p[i] = c;
    }
    
    return dest;
}
#endif