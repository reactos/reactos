/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Runtime Library
 * PURPOSE:           Network Address Translation implementation
 * PROGRAMMER:        Alex Ionescu (alexi@tinykrnl.org)
 *                    Thomas Faber (thomas.faber@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <ntstrsafe.h>
#define NDEBUG
#include <debug.h>

/* maximum length of an ipv4 address expressed as a string */
#define IPV4_ADDR_STRING_MAX_LEN 16

/* maximum length of an ipv4 port expressed as a string */
#define IPV4_PORT_STRING_MAX_LEN 7 /* with the leading ':' */

/* maximum length of an ipv6 string for RtlIpv6AddressToString */
#define RTLIPV6A2S_MAX_LEN 46

/* maximum length of an ipv6 string with scope and port for RtlIpv6AddressToStringEx */
#define RTLIPV6A2SEX_MAX_LEN 65

/* network to host order conversion for little endian machines */
#define WN2H(w) (((w & 0xFF00) >> 8) | ((w & 0x00FF) << 8))

/* PRIVATE FUNCTIONS **********************************************************/

/* decode a string with given Base (8, 10 or 16) */
static
NTSTATUS
RtlpStringToUlongBase(
    _In_ PCWSTR String,
    _In_ ULONG Base,
    _Out_ PCWSTR *Terminator,
    _Out_ PULONG Out)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    ULONG Result = 0;
    ULONG Digit;

    while (1)
    {
        Digit = towlower(*String);
        if (isdigit(Digit) && (Base >= 10 || Digit <= L'7'))
            Digit -= L'0';
        else if (Digit >= L'a' && Digit <= L'f' && Base >= 16)
            Digit -= (L'a' - 10);
        else
            break;

        Status = RtlULongMult(Result, Base, &Result);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        Status = RtlULongAdd(Result, Digit, &Result);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        String++;
    }

    *Terminator = String;
    *Out = Result;
    return Status;
}


static
NTSTATUS
RtlpStringToUlong(
    _In_ PCWSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR *Terminator,
    _Out_ PULONG Out)
{
    ULONG Base = 10;

    if (String[0] == L'0')
    {
        if (String[1] == L'x' || String[1] == L'X')
        {
            /* 0x/0X prefix -- hex */
            String += 2;
            Base = 16;
        }
        else if (String[1] >= L'0' && String[1] <= L'9')
        {
            /* 0 prefix -- octal */
            String++;
            Base = 8;
        }
    }

    /* Strict forbids anything but decimal */
    if (Strict && Base != 10)
    {
        *Terminator = String;
        return STATUS_INVALID_PARAMETER;
    }
    return RtlpStringToUlongBase(String, Base, Terminator, Out);
}

/* Worker function to extract the ipv4 part of a string. */
NTSTATUS
NTAPI
RtlpIpv4StringToAddressParserW(
    _In_ PCWSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR *Terminator,
    _Out_writes_(4) ULONG *Values,
    _Out_ INT *Parts)
{
    NTSTATUS Status;
    *Parts = 0;
    do
    {
        Status = RtlpStringToUlong(String, Strict, &String, &Values[*Parts]);
        (*Parts)++;

        if (*String != L'.')
            break;

        /* Already four parts, but a dot follows? */
        if (*Parts == 4)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        /* Skip the dot */
        String++;
    } while (NT_SUCCESS(Status));

    *Terminator = String;
    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PSTR
NTAPI
RtlIpv4AddressToStringA(
    _In_ const struct in_addr *Addr,
    _Out_writes_(IPV4_ADDR_STRING_MAX_LEN) PCHAR S)
{
    NTSTATUS Status;
    PSTR End;

    if (!S)
        return (PSTR)~0;

    Status = RtlStringCchPrintfExA(S,
                                   IPV4_ADDR_STRING_MAX_LEN,
                                   &End,
                                   NULL,
                                   0,
                                   "%u.%u.%u.%u",
                                   Addr->S_un.S_un_b.s_b1,
                                   Addr->S_un.S_un_b.s_b2,
                                   Addr->S_un.S_un_b.s_b3,
                                   Addr->S_un.S_un_b.s_b4);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return (PSTR)~0;

    return End;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(
    _In_ const struct in_addr *Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PCHAR AddressString,
    _Inout_ PULONG AddressStringLength)
{
    CHAR Buffer[IPV4_ADDR_STRING_MAX_LEN + IPV4_PORT_STRING_MAX_LEN];
    NTSTATUS Status;
    ULONG Length;
    PSTR End;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    Status = RtlStringCchPrintfExA(Buffer,
                                   RTL_NUMBER_OF(Buffer),
                                   &End,
                                   NULL,
                                   0,
                                   Port ? "%u.%u.%u.%u:%u"
                                        : "%u.%u.%u.%u",
                                   Address->S_un.S_un_b.s_b1,
                                   Address->S_un.S_un_b.s_b2,
                                   Address->S_un.S_un_b.s_b3,
                                   Address->S_un.S_un_b.s_b4,
                                   WN2H(Port));
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    Length = End - Buffer;
    if (*AddressStringLength > Length)
    {
        Status = RtlStringCchCopyA(AddressString,
                                   *AddressStringLength,
                                   Buffer);
        ASSERT(Status == STATUS_SUCCESS);
        *AddressStringLength = Length + 1;
        return STATUS_SUCCESS;
    }

    *AddressStringLength = Length + 1;
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
PWSTR
NTAPI
RtlIpv4AddressToStringW(
    _In_ const struct in_addr *Addr,
    _Out_writes_(IPV4_ADDR_STRING_MAX_LEN) PWCHAR S)
{
    NTSTATUS Status;
    PWSTR End;

    if (!S)
        return (PWSTR)~0;

    Status = RtlStringCchPrintfExW(S,
                                   IPV4_ADDR_STRING_MAX_LEN,
                                   &End,
                                   NULL,
                                   0,
                                   L"%u.%u.%u.%u",
                                   Addr->S_un.S_un_b.s_b1,
                                   Addr->S_un.S_un_b.s_b2,
                                   Addr->S_un.S_un_b.s_b3,
                                   Addr->S_un.S_un_b.s_b4);
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return (PWSTR)~0;

    return End;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(
    _In_ const struct in_addr *Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWCHAR AddressString,
    _Inout_ PULONG AddressStringLength)
{
    WCHAR Buffer[IPV4_ADDR_STRING_MAX_LEN + IPV4_PORT_STRING_MAX_LEN];
    NTSTATUS Status;
    ULONG Length;
    PWSTR End;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    Status = RtlStringCchPrintfExW(Buffer,
                                   RTL_NUMBER_OF(Buffer),
                                   &End,
                                   NULL,
                                   0,
                                   Port ? L"%u.%u.%u.%u:%u"
                                        : L"%u.%u.%u.%u",
                                   Address->S_un.S_un_b.s_b1,
                                   Address->S_un.S_un_b.s_b2,
                                   Address->S_un.S_un_b.s_b3,
                                   Address->S_un.S_un_b.s_b4,
                                   WN2H(Port));
    ASSERT(Status == STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    Length = End - AddressString;
    if (*AddressStringLength > Length)
    {
        Status = RtlStringCchCopyW(AddressString,
                                   *AddressStringLength,
                                   Buffer);
        ASSERT(Status == STATUS_SUCCESS);
        *AddressStringLength = Length + 1;
        return STATUS_SUCCESS;
    }

    *AddressStringLength = Length + 1;
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4StringToAddressA(
    _In_ PCSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCSTR *Terminator,
    _Out_ struct in_addr *Addr)
{
    NTSTATUS Status;
    ANSI_STRING AddressA;
    UNICODE_STRING AddressW;
    PCWSTR TerminatorW = NULL;

    Status = RtlInitAnsiStringEx(&AddressA, String);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&AddressW, &AddressA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv4StringToAddressW(AddressW.Buffer,
                                     Strict,
                                     &TerminatorW,
                                     Addr);

    ASSERT(TerminatorW >= AddressW.Buffer);
    *Terminator = String + (TerminatorW - AddressW.Buffer);

    RtlFreeUnicodeString(&AddressW);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4StringToAddressExA(
    _In_ PCSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ struct in_addr *Address,
    _Out_ PUSHORT Port)
{
    NTSTATUS Status;
    ANSI_STRING AddressA;
    UNICODE_STRING AddressW;

    Status = RtlInitAnsiStringEx(&AddressA, AddressString);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&AddressW, &AddressA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv4StringToAddressExW(AddressW.Buffer, Strict, Address, Port);

    RtlFreeUnicodeString(&AddressW);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
    _In_ PCWSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR *Terminator,
    _Out_ struct in_addr *Addr)
{
    NTSTATUS Status;
    ULONG Values[4];
    ULONG Result;
    INT Parts = 0;
    INT i;

    Status = RtlpIpv4StringToAddressParserW(String,
                                            Strict,
                                            Terminator,
                                            Values,
                                            &Parts);
    if (Strict && Parts < 4)
        Status = STATUS_INVALID_PARAMETER;

    if (!NT_SUCCESS(Status))
        return Status;

    /* Combine the parts */
    Result = Values[Parts - 1];
    for (i = 0; i < Parts - 1; i++)
    {
        INT Shift = CHAR_BIT * (3 - i);

        if (Values[i] > 0xFF || (Result & (0xFF << Shift)) != 0)
        {
            return STATUS_INVALID_PARAMETER;
        }
        Result |= Values[i] << Shift;
    }

    Addr->S_un.S_addr = RtlUlongByteSwap(Result);
    return Status;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ struct in_addr *Address,
    _Out_ PUSHORT Port)
{
    PCWSTR CurrentChar;
    ULONG ConvertedPort;
    NTSTATUS Status;

    if (!AddressString || !Address || !Port)
        return STATUS_INVALID_PARAMETER;

    Status = RtlIpv4StringToAddressW(AddressString,
                                     Strict,
                                     &CurrentChar,
                                     Address);
    if (!NT_SUCCESS(Status))
        return Status;

    if (!*CurrentChar)
    {
        *Port = 0;
        return STATUS_SUCCESS;
    }

    if (*CurrentChar != L':')
        return STATUS_INVALID_PARAMETER;
    ++CurrentChar;

    Status = RtlpStringToUlong(CurrentChar,
                               FALSE,
                               &CurrentChar,
                               &ConvertedPort);
    if (!NT_SUCCESS(Status))
        return Status;

    if (*CurrentChar || !ConvertedPort || ConvertedPort > 0xffff)
        return STATUS_INVALID_PARAMETER;

    *Port = WN2H(ConvertedPort);
    return STATUS_SUCCESS;
}

/*
* @implemented
*/
PSTR
NTAPI
RtlIpv6AddressToStringA(
    _In_ const struct in6_addr *Addr,
    _Out_writes_(RTLIPV6A2S_MAX_LEN) PSTR S)
{
    WCHAR Buffer[RTLIPV6A2S_MAX_LEN];
    PWSTR Result;
    NTSTATUS Status;

    if (!S)
        return (PSTR)~0;

    Buffer[0] = 0;
    Result = RtlIpv6AddressToStringW(Addr, Buffer);
    if (Result == (PWSTR)~0)
        return (PSTR)~0;

    ASSERT(Result >= Buffer);
    ASSERT(Result < Buffer + RTL_NUMBER_OF(Buffer));

    Status = RtlUnicodeToMultiByteN(S, RTLIPV6A2S_MAX_LEN, NULL, Buffer, (ULONG)(wcslen(Buffer) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        return (PSTR)~0;

    return S + strlen(S);
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExA(
    _In_ const struct in6_addr *Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PSTR AddressString,
    _Inout_ PULONG AddressStringLength)
{
    WCHAR Buffer[RTLIPV6A2SEX_MAX_LEN];
    NTSTATUS Status;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    Status = RtlIpv6AddressToStringExW(Address, ScopeId, Port, Buffer, AddressStringLength);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlUnicodeToMultiByteN(AddressString, RTLIPV6A2SEX_MAX_LEN, NULL, Buffer, (*AddressStringLength + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

/*
* @implemented
*/
PWSTR
NTAPI
RtlIpv6AddressToStringW(
    _In_ const struct in6_addr *Addr,
    _Out_writes_(RTLIPV6A2S_MAX_LEN) PWSTR S)
{
    NTSTATUS Status;
    UINT Parts = 8, n;
    BOOLEAN SkipOnce = TRUE;
    PWSTR End;
    size_t Remaining;

    if (!S)
        return (PWSTR)~0;

    Remaining = RTLIPV6A2S_MAX_LEN;
    /* does it look like an ipv4 address contained in an ipv6? http://tools.ietf.org/html/rfc2765 */
    if (!Addr->s6_words[0] && !Addr->s6_words[1] && !Addr->s6_words[2] && !Addr->s6_words[3] && Addr->s6_words[6])
    {
        PWSTR Prefix = NULL;
        if (Addr->s6_words[4] == 0xffff && !Addr->s6_words[5])
            Prefix = L"ffff:0:";
        else if (!Addr->s6_words[4] && Addr->s6_words[5] == 0xffff)
            Prefix = L"ffff:";
        else if (!Addr->s6_words[4] && !Addr->s6_words[5])
            Prefix = L"";
        if (Prefix != NULL)
        {
            Status = RtlStringCchPrintfExW(S,
                                           Remaining,
                                           &End,
                                           NULL,
                                           0,
                                           L"::%ls%u.%u.%u.%u",
                                           Prefix,
                                           Addr->s6_bytes[12],
                                           Addr->s6_bytes[13],
                                           Addr->s6_bytes[14],
                                           Addr->s6_bytes[15]);
            ASSERT(Status == STATUS_SUCCESS);
            if (!NT_SUCCESS(Status))
                return (PWSTR)~0;
            return End;
        }
    }

    /* does it look like an ISATAP address? http://tools.ietf.org/html/rfc5214 */
    if (!(Addr->s6_words[4] & 0xfffd) && Addr->s6_words[5] == 0xfe5e)
        Parts = 6;

    for (n = 0; n < Parts; ++n)
    {
        if (SkipOnce && ((n + 1) < Parts) && !Addr->s6_words[n] && !Addr->s6_words[n + 1])
        {
            SkipOnce = FALSE;
            while (!Addr->s6_words[n + 1] && (n + 1) < Parts)
                ++n;
            *S++ = ':';
            Remaining--;
            if ((n + 1) >= Parts)
            {
                *S++ = ':';
                Remaining--;
            }
        }
        else
        {
            if (n)
            {
                *S++ = ':';
                Remaining--;
            }
            Status = RtlStringCchPrintfExW(S,
                                           Remaining,
                                           &End,
                                           &Remaining,
                                           0,
                                           L"%x",
                                           WN2H(Addr->s6_words[n]));
            ASSERT(Status == STATUS_SUCCESS);
            if (!NT_SUCCESS(Status))
                return (PWSTR)~0;
            S = End;
        }
    }
    if (Parts < 8)
    {
        Status = RtlStringCchPrintfExW(S,
                                       Remaining,
                                       &End,
                                       NULL,
                                       0,
                                       L":%u.%u.%u.%u",
                                       Addr->s6_bytes[12],
                                       Addr->s6_bytes[13],
                                       Addr->s6_bytes[14],
                                       Addr->s6_bytes[15]);
        ASSERT(Status == STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            return (PWSTR)~0;

        return End;
    }
    *S = UNICODE_NULL;
    return S;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(
    _In_ const struct in6_addr *Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWCHAR AddressString,
    _Inout_ PULONG AddressStringLength)
{
    WCHAR Buffer[RTLIPV6A2SEX_MAX_LEN];
    PWCHAR S = Buffer;
    NTSTATUS Status;
    ULONG Length;
    size_t Remaining;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    if (Port)
        *S++ = L'[';

    S = RtlIpv6AddressToStringW(Address, S);
    ASSERT(S != (PCWSTR)~0);
    if (S == (PCWSTR)~0)
        return STATUS_INVALID_PARAMETER;

    ASSERT(S >= Buffer);
    ASSERT(S <= Buffer + RTLIPV6A2S_MAX_LEN + 1);
    Remaining = RTL_NUMBER_OF(Buffer) - (S - Buffer);
    ASSERT(Remaining >= RTLIPV6A2SEX_MAX_LEN - RTLIPV6A2S_MAX_LEN);

    if (ScopeId)
    {
        Status = RtlStringCchPrintfExW(S,
                                       Remaining,
                                       &S,
                                       &Remaining,
                                       0,
                                       L"%%%u",
                                       ScopeId);
        ASSERT(Status == STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }

    if (Port)
    {
        Status = RtlStringCchPrintfExW(S,
                                       Remaining,
                                       &S,
                                       &Remaining,
                                       0,
                                       L"]:%u",
                                       WN2H(Port));
        ASSERT(Status == STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }

    Length = S - Buffer;
    ASSERT(Buffer[Length] == UNICODE_NULL);
    if (*AddressStringLength > Length)
    {
        Status = RtlStringCchCopyW(AddressString, *AddressStringLength, Buffer);
        ASSERT(Status == STATUS_SUCCESS);
        *AddressStringLength = Length + 1;
        return STATUS_SUCCESS;
    }

    *AddressStringLength = Length + 1;
    return STATUS_INVALID_PARAMETER;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(
    _In_ PCSTR String,
    _Out_ PCSTR *Terminator,
    _Out_ struct in6_addr *Addr)
{
    NTSTATUS Status;
    ANSI_STRING StringA;
    UNICODE_STRING StringW;
    PCWSTR TerminatorW = NULL;

    Status = RtlInitAnsiStringEx(&StringA, String);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&StringW, &StringA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv6StringToAddressW(StringW.Buffer, &TerminatorW, Addr);
    /* on windows the terminator is not always written, so we mimic that behavior. */
    if (TerminatorW)
        *Terminator = String + (TerminatorW - StringW.Buffer);

    RtlFreeUnicodeString(&StringW);
    return Status;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(
    _In_ PCSTR AddressString,
    _Out_ struct in6_addr *Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port)
{
    NTSTATUS Status;
    ANSI_STRING AddressA;
    UNICODE_STRING AddressW;

    Status = RtlInitAnsiStringEx(&AddressA, AddressString);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&AddressW, &AddressA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv6StringToAddressExW(AddressW.Buffer, Address, ScopeId, Port);

    RtlFreeUnicodeString(&AddressW);
    return Status;
}

/* 
 * RtlIpv6StringToAddress[Ex]W implementation was imported from Wine 9.6 at 20/4/2024
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * For full text of the license see COPYING.LIB in the root of this project.
 *
 * Copyright 2020 Alex Henrie
 */

static const int hex_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x20-0x2F */
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x40-0x4F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x50-0x5F */
    -1, 10, 11, 12, 13, 14, 15                                      /* 0x60-0x66 */
};

static BOOL parse_ipv4_component(const WCHAR **str, BOOL strict, ULONG *value)
{
    int base = 10, d;
    WCHAR c;
    ULONG cur_value, prev_value = 0;
    BOOL success = FALSE;

    if (**str == '.')
    {
        *str += 1;
        return FALSE;
    }

    if ((*str)[0] == '0')
    {
        if ((*str)[1] == 'x' || (*str)[1] == 'X')
        {
            *str += 2;
            if (strict) return FALSE;
            base = 16;
        }
        else if ((*str)[1] >= '0' && (*str)[1] <= '9')
        {
            *str += 1;
            if (strict) return FALSE;
            base = 8;
        }
    }

    for (cur_value = 0; **str; *str += 1)
    {
        c = **str;
        if (c >= ARRAYSIZE(hex_table)) break;
        d = hex_table[c];
        if (d == -1 || d >= base) break;
        cur_value = cur_value * base + d;
        success = TRUE;
        if (cur_value < prev_value) return FALSE; /* overflow */
        prev_value = cur_value;
    }

    if (success) *value = cur_value;
    return success;
}

static BOOL parse_ipv6_component(const WCHAR **str, int base, ULONG *value)
{
    WCHAR *terminator;
    if (**str >= ARRAYSIZE(hex_table) || hex_table[**str] == -1) return FALSE;
    *value = min(wcstoul(*str, &terminator, base), 0x7FFFFFFF);
    if (*terminator == '0') terminator++; /* "0x" but nothing valid after */
    else if (terminator == *str) return FALSE;
    *str = terminator;
    return TRUE;
}

static NTSTATUS ipv6_string_to_address(const WCHAR *str, BOOL ex,
                                       const WCHAR **terminator, IN6_ADDR *address, ULONG *scope, USHORT *port)
{
    BOOL expecting_port = FALSE, has_0x = FALSE, too_big = FALSE;
    int n_bytes = 0, n_ipv4_bytes = 0, gap = -1;
    ULONG ip_component, scope_component = 0, port_component = 0;
    const WCHAR *prev_str;

    if (str[0] == '[')
    {
        if (!ex) goto error;
        expecting_port = TRUE;
        str++;
    }

    if (str[0] == ':')
    {
        if (str[1] != ':') goto error;
        str++;
        address->u.Word[0] = 0;
    }

    for (;;)
    {
        if (!n_ipv4_bytes && *str == ':')
        {
            /* double colon */
            if (gap != -1) goto error;
            str++;
            prev_str = str;
            gap = n_bytes;
            if (n_bytes == 14 || !parse_ipv6_component(&str, 16, &ip_component)) break;
            str = prev_str;
        }
        else
        {
            prev_str = str;
        }

        if (!n_ipv4_bytes && n_bytes <= (gap != -1 ? 10 : 12))
        {
            if (parse_ipv6_component(&str, 10, &ip_component) && *str == '.')
                n_ipv4_bytes = 1;
            str = prev_str;
        }

        if (n_ipv4_bytes)
        {
            /* IPv4 component */
            if (!parse_ipv6_component(&str, 10, &ip_component)) goto error;
            if (str - prev_str > 3 || ip_component > 255)
            {
                too_big = TRUE;
            }
            else
            {
                if (*str != '.' && (n_ipv4_bytes < 4 || (n_bytes < 15 && gap == -1))) goto error;
                address->u.Byte[n_bytes] = ip_component;
                n_bytes++;
            }
            if (n_ipv4_bytes == 4 || *str != '.') break;
            n_ipv4_bytes++;
        }
        else
        {
            /* IPv6 component */
            if (!parse_ipv6_component(&str, 16, &ip_component)) goto error;
            if (prev_str[0] == '0' && (prev_str[1] == 'x' || prev_str[1] == 'X'))
            {
                /* Windows "feature": the last IPv6 component can start with "0x" and be longer than 4 digits */
                if (terminator) *terminator = prev_str + 1; /* Windows says that the "x" is the terminator */
                if (n_bytes < 14 && gap == -1) return STATUS_INVALID_PARAMETER;
                address->u.Word[n_bytes/2] = RtlUshortByteSwap(ip_component);
                n_bytes += 2;
                has_0x = TRUE;
                goto fill_gap;
            }
            if (*str != ':' && n_bytes < 14 && gap == -1) goto error;
            if (str - prev_str > 4)
                too_big = TRUE;
            else
                address->u.Word[n_bytes/2] = RtlUshortByteSwap(ip_component);
            n_bytes += 2;
            if (*str != ':' || (gap != -1 && str[1] == ':')) break;
        }
        if (n_bytes == (gap != -1 ? 14 : 16)) break;
        if (too_big) return STATUS_INVALID_PARAMETER;
        str++;
    }

    if (terminator) *terminator = str;
    if (too_big) return STATUS_INVALID_PARAMETER;

fill_gap:
    if (gap == -1)
    {
        if (n_bytes < 16) goto error;
    }
    else
    {
        memmove(address->u.Byte + 16 - (n_bytes - gap), address->u.Byte + gap, n_bytes - gap);
        memset(address->u.Byte + gap, 0, 16 - n_bytes);
    }

    if (ex)
    {
        if (has_0x) goto error;

        if (*str == '%')
        {
            str++;
            if (!parse_ipv4_component(&str, TRUE, &scope_component)) goto error;
        }

        if (expecting_port)
        {
            if (*str != ']') goto error;
            str++;
            if (*str == ':')
            {
                str++;
                if (!parse_ipv4_component(&str, FALSE, &port_component)) goto error;
                if (!port_component || port_component > 0xFFFF || *str) goto error;
                port_component = RtlUshortByteSwap(port_component);
            }
        }
    }

    if (!terminator && *str) return STATUS_INVALID_PARAMETER;

    if (scope) *scope = scope_component;
    if (port) *port = port_component;

    return STATUS_SUCCESS;

error:
    if (terminator) *terminator = str;
    return STATUS_INVALID_PARAMETER;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    _In_ PCWSTR String,
    _Out_ PCWSTR *Terminator,
    _Out_ struct in6_addr *Addr)
{
    if (!String || !Terminator || !Addr)
        return STATUS_INVALID_PARAMETER;

    return ipv6_string_to_address(String, FALSE, Terminator, Addr, NULL, NULL);
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
    _In_ PCWSTR AddressString,
    _Out_ struct in6_addr *Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port)
{
    if (!AddressString || !Address || !ScopeId || !Port) 
        return STATUS_INVALID_PARAMETER;

    return ipv6_string_to_address(AddressString, TRUE, NULL, Address, ScopeId, Port);
}

/* End of Wine implementation */

/* EOF */
