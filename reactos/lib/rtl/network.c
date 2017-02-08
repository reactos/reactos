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

/* Tell us what possible base the string could be in, 10 or 16 by looking at the characters.
   Invalid characters break the operation */
static
ULONG
RtlpClassifyChars(PCWSTR S, PULONG Base)
{
    ULONG Len = 0;
    *Base = 0;
    for (Len = 0; S[Len]; ++Len)
    {
        if (iswascii(S[Len]) && isdigit(S[Len]))
            *Base = max(*Base, 10);
        else if (iswascii(S[Len]) && isxdigit(S[Len]))
            *Base = 16;
        else
            break;
    }
    return Len;
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

    Status = RtlUnicodeToMultiByteN(S, RTLIPV6A2S_MAX_LEN, NULL, Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));
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

    Status = RtlUnicodeToMultiByteN(AddressString, RTLIPV6A2SEX_MAX_LEN, NULL, Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));
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
* @implemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    _In_ PCWSTR String,
    _Out_ PCWSTR *Terminator,
    _Out_ struct in6_addr *Addr)
{
    INT n, j;
    INT StartSkip = -1, Parts = 0;
    ULONG Base, Len;
    NTSTATUS Status = STATUS_SUCCESS;
    enum { None, Number, Colon, DoubleColon } Last = None;
    BOOLEAN SkipoutLastHex = FALSE;

    if (!String || !Terminator || !Addr)
        return STATUS_INVALID_PARAMETER;

    for (n = 0; n < 8;)
    {
        Len = RtlpClassifyChars(String, &Base);
        if (Len == 0)
        {
            /* not a number, and no ':' or already encountered an ':' */
            if (String[0] != ':' || Last == Colon)
                break;

            /* double colon, 1 or more fields set to 0. mark the position, and move on. */
            if (StartSkip == -1 && String[1] == ':')
            {
                /* this case was observed in windows, but it does not seem correct. */
                if (!n)
                {
                    Addr->s6_words[n++] = 0;
                    Addr->s6_words[n++] = 0;
                }
                StartSkip = n;
                String += 2;
                Last = DoubleColon;
            }
            else if (String[1] == ':' || Last != Number)
            {
                /* a double colon after we already encountered one, or a the last parsed item was not a number. */
                break;
            }
            else
            {
                ++String;
                Last = Colon;
            }
        }
        else if (Len > 4)
        {
            /* it seems that when encountering more than 4 chars, the terminator is not updated,
                unless the previously encountered item is a double colon.... */
            Status = STATUS_INVALID_PARAMETER;
            if (Last != DoubleColon)
                return Status;
            String += Len;
            break;
        }
        else
        {
            ULONG Value;
            if (String[Len] == '.' && n <= 6)
            {
                ULONG Values[4];
                INT PartsV4 = 0;
                /* this could be an ipv4 address inside an ipv6 address http://tools.ietf.org/html/rfc2765 */
                Last = Number;
                Status = RtlpIpv4StringToAddressParserW(String, TRUE, &String, Values, &PartsV4);
                for(j = 0; j < PartsV4; ++j)
                {
                    if (Values[j] > 255)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        if (j != 3)
                            return Status;
                        break;
                    }
                    if ((j == PartsV4 - 1) &&
                        (j < 3 ||
                         (*String == ':' && StartSkip == -1) ||
                         (StartSkip == -1 && n < 6) ||
                         Status == STATUS_INVALID_PARAMETER))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                    Addr->s6_bytes[n * 2 + j] = Values[j] & 0xff;
                }
                /* mark enough parts as converted in case we are the last item to be converted */
                n += 2;
                /* mark 2 parts as converted in case we are not the last item, and we encountered a double colon. */
                Parts+=2;
                break;
            }

            if (String[Len] != ':' && n < 7 && StartSkip == -1)
            {
                /* if we decoded atleast some numbers, update the terminator to point to the first invalid char */
                if (Base)
                    String += Len;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (Len == 1 && towlower(String[Len]) == 'x' && String[0] == '0')
            {
                Len = RtlpClassifyChars(String + 2, &Base);
                if (Len > 0 && Len <= 4)
                {
                    *Terminator = String + 1;
                    String += 2;
                    SkipoutLastHex = TRUE;
                }
            }

            Status = RtlpStringToUlongBase(String, 16, &String, &Value);
            if (!NT_SUCCESS(Status))
                break;

            if (StartSkip != -1)
                Parts++;
            Addr->s6_words[n++] = WN2H(Value);
            Last = Number;
            if (SkipoutLastHex)
                break;
        }
    }

    if (StartSkip != -1 && Status != STATUS_INVALID_PARAMETER && Last != Colon)
    {
        /* we found a '::' somewhere, so expand that. */
        memmove(&Addr->s6_words[8-Parts], &Addr->s6_words[StartSkip], Parts * sizeof(Addr->s6_words[0]));
        memset(&Addr->s6_words[StartSkip], 0, (8-StartSkip-Parts) * sizeof(Addr->s6_words[0]));
        n = 8;
    }

    /* we have already set the terminator */
    if (SkipoutLastHex)
        return n < 8 ? STATUS_INVALID_PARAMETER : Status;
    *Terminator = String;
    return n < 8 ? STATUS_INVALID_PARAMETER : Status;
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
    NTSTATUS Status;
    ULONG ConvertedPort = 0, ConvertedScope = 0;
    if (!AddressString || !Address || !ScopeId || !Port)
        return STATUS_INVALID_PARAMETER;

    if (*AddressString == '[')
    {
        ConvertedPort = 1;
        ++AddressString;
    }
    Status = RtlIpv6StringToAddressW(AddressString, &AddressString, Address);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    if (*AddressString == '%')
    {
        ++AddressString;
        Status = RtlpStringToUlongBase(AddressString, 10, &AddressString, &ConvertedScope);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }
    else if (*AddressString && !(ConvertedPort && *AddressString == ']'))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (ConvertedPort)
    {
        if (*AddressString++ !=']')
            return STATUS_INVALID_PARAMETER;
        if (*AddressString ==':')
        {
            ULONG Base = 10;
            if (*++AddressString == '0')
            {
                if (towlower(*++AddressString) != 'x')
                    return STATUS_INVALID_PARAMETER;
                ++AddressString;
                Base = 16;
            }
            Status = RtlpStringToUlongBase(AddressString, Base, &AddressString, &ConvertedPort);
            if (!NT_SUCCESS(Status) || ConvertedPort > 0xffff)
                return STATUS_INVALID_PARAMETER;
        }
        else
        {
            ConvertedPort = 0;
        }
    }

    if (*AddressString == 0)
    {
        *ScopeId = ConvertedScope;
        *Port = WN2H(ConvertedPort);
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_PARAMETER;
}

/* EOF */
