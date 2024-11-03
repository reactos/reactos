////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

extern "C"
ULONG
MyRtlCompareMemory(
    PVOID s1,
    PVOID s2,
    ULONG len
    )
{
    ULONG i;

    for(i=0; i<len; i++) {
        if( ((char*)s1)[i] != ((char*)s2)[i] )
            break;
    }
    return i;
}

#define STRING_BUFFER_ALIGNMENT  (32)
#define STRING_BUFFER_ALIGN(sz)  (((sz)+STRING_BUFFER_ALIGNMENT)&(~((ULONG)(STRING_BUFFER_ALIGNMENT-1))))

#ifndef NT_NATIVE_MODE

extern "C"
ULONG
RtlCompareUnicodeString(
    PUNICODE_STRING s1,
    PUNICODE_STRING s2,
    BOOLEAN UpCase
    )
{
    ULONG i;

    if(s1->Length != s2->Length) return (-1);
    i = memcmp(s1->Buffer, s2->Buffer, (s1->Length) ? (s1->Length) : (s2->Length));
    return i;
}

extern "C"
NTSTATUS
RtlUpcaseUnicodeString(
    PUNICODE_STRING dst,
    PUNICODE_STRING src,
    BOOLEAN Alloc
    )
{
//    if(s1->Length != s2->Length) return (-1);
    memcpy(dst->Buffer, src->Buffer, src->Length);
    dst->Buffer[src->Length/sizeof(WCHAR)] = 0;
    dst->Length = src->Length;
    _wcsupr(dst->Buffer);
    return STATUS_SUCCESS;
}

extern "C"
NTSTATUS
RtlAppendUnicodeToString(
    IN PUNICODE_STRING Str1,
    IN PWSTR Str2
    )
{
    PWCHAR tmp;
    USHORT i;

#ifdef _X86_

    __asm push  ebx
    __asm push  esi
    __asm xor   ebx,ebx
    __asm mov   esi,Str2
Scan_1:
    __asm cmp   [word ptr esi+ebx],0
    __asm je    EO_Scan
    __asm add   ebx,2
    __asm jmp   Scan_1
EO_Scan:
    __asm mov   i,bx
    __asm pop   esi
    __asm pop   ebx

#else   // NO X86 optimization, use generic C/C++

    i=0;
    while(Str2[i]) {
       i++;
    }
    i *= sizeof(WCHAR);

#endif // _X86_

    tmp = Str1->Buffer;
    ASSERT(Str1->MaximumLength);
    if((Str1->Length+i+sizeof(WCHAR)) > Str1->MaximumLength) {
        PWCHAR tmp2 = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, STRING_BUFFER_ALIGN(i + Str1->Length + sizeof(WCHAR))*2, 'ilTS');
        if(!tmp2)
            return STATUS_INSUFFICIENT_RESOURCES;
        memcpy(tmp2, tmp, Str1->MaximumLength);
        ExFreePool(tmp);
        tmp = tmp2;
        Str1->MaximumLength = STRING_BUFFER_ALIGN(i + sizeof(WCHAR))*2;
        Str1->Buffer = tmp;
    }
    RtlCopyMemory(((PCHAR)tmp)+Str1->Length, Str2, i);
    i+=Str1->Length;
    tmp[(i / sizeof(WCHAR))] = 0;
    Str1->Length = i;

    return STATUS_SUCCESS;

#undef UDF_UNC_STR_TAG

} // end RtlAppendUnicodeToString()

#endif //NT_NATIVE_MODE

#ifdef CDRW_W32
NTSTATUS
MyInitUnicodeString(
    IN PUNICODE_STRING Str1,
    IN PCWSTR Str2
    )
{

    USHORT i;

#ifdef _X86_

    __asm push  ebx
    __asm push  esi
    __asm xor   ebx,ebx
    __asm mov   esi,Str2
Scan_1:
    __asm cmp   [word ptr esi+ebx],0
    __asm je    EO_Scan
    __asm add   ebx,2
    __asm jmp   Scan_1
EO_Scan:
    __asm mov   i,bx
    __asm pop   esi
    __asm pop   ebx

#else   // NO X86 optimization, use generic C/C++

    i=0;
    while(Str2[i]) {
       i++;
    }
    i *= sizeof(WCHAR);

#endif // _X86_

    Str1->MaximumLength = STRING_BUFFER_ALIGN((Str1->Length = i) + sizeof(WCHAR));
    Str1->Buffer = (PWCHAR)MyAllocatePool__(NonPagedPool, Str1->MaximumLength);
    if(!Str1->Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(Str1->Buffer, Str2, i);
    Str1->Buffer[i/sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;

} // end MyInitUnicodeString()
#endif //CDRW_W32
