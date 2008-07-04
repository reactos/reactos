#include "precomp.h"

#define NDEBUG
#include <debug.h>

static BOOL
DnsIntNameContainsDots(LPCWSTR Name)
{
    return wcschr(Name, '.') ? TRUE : FALSE;
}

static BOOL
DnsIntTwoConsecutiveDots(LPCWSTR Name)
{
    return wcsstr(Name, L"..") ? TRUE : FALSE;
}

static BOOL
DnsIntContainsUnderscore(LPCWSTR Name)
{
    return wcschr(Name, '_') ? TRUE : FALSE;
}

/* DnsValidateName *********************
 * Use some different algorithms to validate the given name as suitable for
 * use with DNS.
 *
 * Name      -- The name to evaluate.
 * Format    -- Format to use:
 *               DnsNameDomain
 *               DnsNameDomainLabel
 *               DnsNameHostnameFull
 *               DnsNameHostnameLabel
 *               DnsNameWildcard
 *               DnsNameSrvRecord
 * RETURNS:
 * ERROR_SUCCESS                -- All good
 * ERROR_INVALID_NAME           --
 *  Name greater than 255 chars.
 *  Label greater than 63 chars.
 *  Two consecutive dots, or starts with dot.
 *  Contains a dot, but a Label check was specified.
 * DNS_ERROR_INVALID_NAME_CHAR
 *  Contains any invalid char: " {|}~[\]^':;<=>?@!"#$%^`()+/,"
 *  Contains an *, except when it is the first label and Wildcard was
 *  specified.
 * DNS_ERROR_NUMERIC_NAME
 *  Set if the name contains only numerics, unless Domain is specified.
 * DNS_ERROR_NON_RFC_NAME
 *  If the name contains underscore.
 *  If there is an underscore in any position but the first in the SrvRecord
 *   case.
 *  If the name contains a non-ascii character.
 */

DNS_STATUS WINAPI
DnsValidateName_W(LPCWSTR Name,
                  DNS_NAME_FORMAT Format)
{
    BOOL AllowDot = FALSE;
    BOOL AllowLeadingAst = FALSE;
    BOOL AllowLeadingUnderscore = FALSE;
    BOOL AllowAllDigits = FALSE;
    const WCHAR *NextLabel, *CurrentLabel, *CurrentChar;

    switch(Format)
    {
        case DnsNameDomain:
            AllowAllDigits = TRUE;
            AllowDot = TRUE;
            break;

        case DnsNameDomainLabel:
            AllowAllDigits = TRUE;
            break;

        case DnsNameHostnameFull:
            AllowDot = TRUE;
            break;

        case DnsNameHostnameLabel:
            break;

        case DnsNameWildcard:
            AllowLeadingAst = TRUE;
            AllowDot = TRUE;
            break;

        case DnsNameSrvRecord:
            AllowLeadingUnderscore = TRUE;
            break;

        default:
            break;
    }

    /* Preliminary checks */
    if(Name[0] == 0)
        return ERROR_INVALID_NAME; /* XXX arty: Check this */

    /* Name too long */
    if(wcslen(Name) > 255)
        return ERROR_INVALID_NAME;

    /* Violations about dots */
    if((!AllowDot && DnsIntNameContainsDots(Name)) || Name[0] == '.' || DnsIntTwoConsecutiveDots(Name))
        return ERROR_INVALID_NAME;

    /* Check component sizes */
    CurrentLabel = Name;

    do
    {
        NextLabel = CurrentLabel;
        while(*NextLabel && *NextLabel != '.')
            NextLabel++;

        if(NextLabel - CurrentLabel > 63)
            return ERROR_INVALID_NAME;

        CurrentLabel = NextLabel;
    } while(*CurrentLabel);

    CurrentChar = Name;

    while(*CurrentChar)
    {
        if(wcschr(L" {|}~[\\]^':;<=>?@!\"#$%^`()+/,",*CurrentChar))
            return DNS_ERROR_INVALID_NAME_CHAR;

        CurrentChar++;
    }

    if((!AllowLeadingAst && Name[0] == '*') || (AllowLeadingAst && Name[0] == '*' && Name[1] && Name[1] != '.'))
        return DNS_ERROR_INVALID_NAME_CHAR;

    if(wcschr(Name + 1, '*'))
        return DNS_ERROR_INVALID_NAME_CHAR;

    CurrentChar = Name;

    while(!AllowAllDigits && *CurrentChar)
    {
        if(*CurrentChar == '.' || (*CurrentChar >= '0' && *CurrentChar <= '9'))
            return DNS_ERROR_NUMERIC_NAME;
    }

    if(((AllowLeadingUnderscore && Name[0] == '_') || Name[0] != '_') && !DnsIntContainsUnderscore(Name + 1))
        return DNS_ERROR_NON_RFC_NAME;

    return ERROR_SUCCESS;
}

DNS_STATUS WINAPI
DnsValidateName_UTF8(LPCSTR Name,
                     DNS_NAME_FORMAT Format)
{
    PWCHAR Buffer;
    int StrLenWc;
    DNS_STATUS Status;

    StrLenWc = mbstowcs(NULL, Name, 0);
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(WCHAR) * (StrLenWc + 1));
    mbstowcs(Buffer, Name, StrLenWc + 1);
    Status = DnsValidateName_W(Buffer, Format);
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

    return Status;
}

DNS_STATUS WINAPI
DnsValidateName_A(LPCSTR Name,
                  DNS_NAME_FORMAT Format)
{
    return DnsValidateName_UTF8(Name, Format);
}

/* DnsNameCompare **********************
 * Return TRUE if the names are identical.
 *
 * Name1 & Name2 -- Names.
 */
BOOL WINAPI
DnsNameCompare_W(LPWSTR Name1,
                 LPWSTR Name2)
{
    int offset = 0;

    while(Name1[offset] && Name2[offset] && towupper(Name1[offset]) == towupper(Name2[offset]))
        offset++;

    return
        (!Name1[offset] && !Name2[offset]) ||
        (!Name1[offset] && !wcscmp(Name2 + offset, L".")) ||
        (!Name2[offset] && !wcscmp(Name1 + offset, L"."));
}

BOOL WINAPI
DnsNameCompare_UTF8(LPCSTR Name1,
                    LPCSTR Name2)
{
    int offset = 0;

    while(Name1[offset] && Name2[offset] && toupper(Name1[offset]) == toupper(Name2[offset]))
        offset++;

    return
        (!Name1[offset] && !Name2[offset]) ||
        (!Name1[offset] && !strcmp(Name2 + offset, ".")) ||
        (!Name2[offset] && !strcmp(Name1 + offset, "."));
}

BOOL WINAPI
DnsNameCompare_A(LPSTR Name1,
                 LPSTR Name2)
{
    return DnsNameCompare_UTF8(Name1, Name2);
}
