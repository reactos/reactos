/*
 * PROJECT:     ReactOS certutil
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CertUtil asn implementation
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 *
 * NOTES:
 *  To keep it simple, Tag and Class are combined in one identifier
 *  See for more details:
 *      https://en.wikipedia.org/wiki/X.690#BER_encoding
 *      https://www.strozhevsky.com/free_docs/asn1_by_simple_words.pdf
 *      http://mikk.net/~chris/asn1.pdf
 *
 *  And for a test suite:
 *      https://github.com/YuryStrozhevsky/asn1-test-suite
 */

#include "precomp.h"
#include <math.h>
#include <wincrypt.h>
#include <stdlib.h>


#define ASN_TAG_IS_CONSTRUCTED          0x20


#define ASN_TAG_BITSTRING               0x03
#define ASN_TAG_OCTET_STRING            0x04
#define ASN_TAG_OBJECT_ID               0x06

#define ASN_TAG_SEQUENCE_RAW            0x10
#define ASN_TAG_SET_RAW                 0x11

#define ASN_TAG_SEQUENCE                0x30
#define ASN_TAG_SET                     0x31


#define ASN_TAG_CONTEXT_SPECIFIC        0x80
#define ASN_TAG_CONTEXT_SPECIFIC_N(n)   (ASN_TAG_CONTEXT_SPECIFIC | (n))

#define ASN_TAG_OPTIONAL                0xA0
#define ASN_TAG_OPTIONAL_N(n)           (ASN_TAG_OPTIONAL | (n))

/* NOTE: These names are not the names listed in f.e. the wikipedia pages,
   they are made to look like MS's names for this */
LPCWSTR TagToName(DWORD dwTag)
{
    switch (dwTag)
    {
    case 0x0: return L"EOC";
    case 0x1: return L"BOOL";
    case 0x2: return L"INTEGER";
    case ASN_TAG_BITSTRING: return L"BIT_STRING";
    case ASN_TAG_OCTET_STRING: return L"OCTET_STRING";
    case 0x5: return L"NULL";
    case ASN_TAG_OBJECT_ID: return L"OBJECT_ID";
    case 0x7: return L"Object Descriptor";
    case 0x8: return L"EXTERNAL";
    case 0x9: return L"REAL";
    case 0xA: return L"ENUMERATED";
    case 0xB: return L"EMBEDDED PDV";
    case 0xC: return L"UTF8String";
    case 0xD: return L"RELATIVE-OID";
    case 0xE: return L"TIME";
    case 0xF: return L"Reserved";
    case ASN_TAG_SEQUENCE_RAW: __debugbreak(); return L"SEQUENCE_RAW";
    case ASN_TAG_SET_RAW: __debugbreak(); return L"SET_RAW";
    case 0x12: return L"NumericString";
    case 0x13: return L"PRINTABLE_STRING";
    case 0x14: return L"T61String";
    case 0x15: return L"VideotexString";
    case 0x16: return L"IA5String";
    case 0x17: return L"UTC_TIME";
    case 0x18: return L"GeneralizedTime";
    case 0x19: return L"GraphicString";
    case 0x1A: return L"VisibleString";
    case 0x1B: return L"GeneralString";
    case 0x1C: return L"UniversalString";
    case 0x1D: return L"CHARACTER STRING";
    case 0x1E: return L"BMPString";
    case 0x1F: return L"DATE";
    case 0x20: return L"CONSTRUCTED";

    case ASN_TAG_SEQUENCE: return L"SEQUENCE";
    case ASN_TAG_SET: return L"SET";


    case ASN_TAG_CONTEXT_SPECIFIC_N(0): return L"CONTEXT_SPECIFIC[0]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(1): return L"CONTEXT_SPECIFIC[1]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(2): return L"CONTEXT_SPECIFIC[2]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(3): return L"CONTEXT_SPECIFIC[3]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(4): return L"CONTEXT_SPECIFIC[4]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(5): return L"CONTEXT_SPECIFIC[5]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(6): return L"CONTEXT_SPECIFIC[6]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(7): return L"CONTEXT_SPECIFIC[7]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(8): return L"CONTEXT_SPECIFIC[8]";
    case ASN_TAG_CONTEXT_SPECIFIC_N(9): return L"CONTEXT_SPECIFIC[9]";
    /* Experiments show that Windows' certutil only goes up to 9 */


    case ASN_TAG_OPTIONAL_N(0): return L"OPTIONAL[0]";
    case ASN_TAG_OPTIONAL_N(1): return L"OPTIONAL[1]";
    case ASN_TAG_OPTIONAL_N(2): return L"OPTIONAL[2]";
    case ASN_TAG_OPTIONAL_N(3): return L"OPTIONAL[3]";
    case ASN_TAG_OPTIONAL_N(4): return L"OPTIONAL[4]";
    case ASN_TAG_OPTIONAL_N(5): return L"OPTIONAL[5]";
    case ASN_TAG_OPTIONAL_N(6): return L"OPTIONAL[6]";
    case ASN_TAG_OPTIONAL_N(7): return L"OPTIONAL[7]";
    case ASN_TAG_OPTIONAL_N(8): return L"OPTIONAL[8]";
    case ASN_TAG_OPTIONAL_N(9): return L"OPTIONAL[9]";
    /* Experiments show that Windows' certutil only goes up to 9 */

    default:
        return L"???";
    }
}

BOOL Move(DWORD dwLen, PBYTE& pData, DWORD& dwSize)
{
    if (dwSize < dwLen)
        return FALSE;

    pData += dwLen;
    dwSize -= dwLen;

    return TRUE;
}

BOOL ParseTag(PBYTE& pData, DWORD& dwSize, DWORD& dwTagAndClass)
{
    if (dwSize == 0)
        return FALSE;

    /* Is this a long form? */
    if ((pData[0] & 0x1f) != 0x1f)
    {
        /* No, so extract the tag and class (in one identifier) */
        dwTagAndClass = pData[0];
        return Move(1, pData, dwSize);
    }

    DWORD dwClass = (pData[0] & 0xE0) >> 5;
    dwTagAndClass = 0;
    DWORD n;
    for (n = 1; n < dwSize; ++n)
    {
        dwTagAndClass <<= 7;
        dwTagAndClass |= (pData[n] & 0x7f);

        if (!(pData[n] & 0x80))
        {
            break;
        }
    }

    Move(n, pData, dwSize);

    /* Any number bigger than this, we shift data out! */
    if (n > 4)
        return FALSE;

    /* Just drop this in the hightest bits*/
    dwTagAndClass |= (dwClass << (32-3));

    return TRUE;
}

BOOL ParseLength(PBYTE& pData, DWORD& dwSize, DWORD& dwLength)
{
    if (dwSize == 0)
        return FALSE;

    if (!(pData[0] & 0x80))
    {
        dwLength = pData[0];
        return Move(1, pData, dwSize);
    }

    DWORD dwBytes = pData[0] & 0x7f;
    if (dwBytes == 0 || dwBytes > 8 || dwBytes + 1 > dwSize)
    {
        return FALSE;
    }

    dwLength = 0;
    for (DWORD n = 0; n < dwBytes; ++n)
    {
        dwLength <<= 8;
        dwLength += pData[1 + n];
    }

    return Move(dwBytes + 1, pData, dwSize);
}


DWORD HexDump(PBYTE pRoot, PBYTE pData, DWORD dwSize, PWSTR wszPrefix)
{
    while (TRUE)
    {
        SIZE_T Address = pData - pRoot;
        ConPrintf(StdOut, L"%04x: ", Address);
        ConPuts(StdOut, wszPrefix);

        for (DWORD n = 0; n < min(dwSize, 0x10); ++n)
        {
            ConPrintf(StdOut, L"%02x ", pData[n]);
        }

        if (dwSize <= 0x10)
            break;

        Move(0x10, pData, dwSize);
        ConPuts(StdOut, L"\n");
    }

    return 3 * dwSize;
}

void PrintTag(PBYTE pRoot, PBYTE pHeader, DWORD dwTag, DWORD dwTagLength, PBYTE pData, PWSTR wszPrefix)
{
    DWORD dwRemainder = HexDump(pRoot, pHeader, pData - pHeader, wszPrefix);

    LPCWSTR wszTag = TagToName(dwTag);
    DWORD dwPadding = dwRemainder + wcslen(wszPrefix);
    while (dwPadding > 50)
        dwPadding -= 50;
    ConPrintf(StdOut, L"%*s; %s (%x Bytes)\n", 50 - dwPadding, L"", wszTag, dwTagLength);
}

struct OID_NAMES
{
    CHAR* Oid;
    LPCWSTR Names[20];
    DWORD NumberOfNames;
};

BOOL WINAPI CryptOIDEnumCallback(_In_ PCCRYPT_OID_INFO pInfo, _Inout_opt_ void *pvArg)
{
    OID_NAMES* Names = (OID_NAMES*)pvArg;

    if (pInfo && pInfo->pszOID && !_stricmp(pInfo->pszOID, Names->Oid))
    {
        if (Names->NumberOfNames < RTL_NUMBER_OF(Names->Names))
        {
            for (DWORD n = 0; n < Names->NumberOfNames; ++n)
            {
                // We already have this..
                if (!_wcsicmp(Names->Names[n], pInfo->pwszName))
                    return TRUE;
            }

            Names->Names[Names->NumberOfNames++] = pInfo->pwszName;
        }
    }

    return TRUE;
}

void PrintOID(PBYTE pRoot, PBYTE pHeader, PBYTE pData, DWORD dwSize, PWSTR wszPrefix)
{
    /* CryptFindOIDInfo expects the OID to be in ANSI.. */
    CHAR szOID[250];
    CHAR* ptr = szOID;
    size_t cchRemaining = RTL_NUMBER_OF(szOID);

    /* CryptFindOIDInfo just returns the first, we want multiple */
    OID_NAMES Names = {0};

    if (dwSize == 0)
        return;

    DWORD dwValue = 0, count = 0;
    for (DWORD n = 0; n < dwSize; ++n)
    {
        dwValue <<= 7;
        dwValue |= pData[n] & 0x7f;

        if (pData[n] & 0x80)
        {
            if (++count >= 4)
                break;
            continue;
        }
        count = 0;

        /* First & second octet have a special encoding */
        if (ptr == szOID)
        {
            DWORD id1 = dwValue / 40;
            DWORD id2 = dwValue % 40;

            /* The first one can only be 0, 1 or 2, so handle special case: tc24.ber */
            if (id1 > 2)
            {
                id2 += (id1 - 2) * 40;
                id1 = 2;
            }
            StringCchPrintfExA(ptr, cchRemaining, &ptr, &cchRemaining, 0, "%d.%d", id1, id2);
        }
        else
        {
            StringCchPrintfExA(ptr, cchRemaining, &ptr, &cchRemaining, 0, ".%d", dwValue);
        }

        dwValue = 0;
    }

    if (dwValue || count)
    {
        /* We cannot format this, so just add abort */
        return;
    }

    SIZE_T Address = pData - pRoot;
    /* Pad with spaces instead of printing the address again */
    DWORD addrDigits = (DWORD)log10((double)Address) + 1;
    ConPrintf(StdOut, L"%*s  ", max(addrDigits, 4), L"");
    ConPrintf(StdOut, L"%s; %S", wszPrefix, szOID);

    Names.Oid = szOID;

    /* The order does not match a naive call with '0'... */
    CryptEnumOIDInfo(0, 0, &Names, CryptOIDEnumCallback);

    for (DWORD n = 0; n < Names.NumberOfNames; ++n)
    {
        if (n == 0)
            ConPrintf(StdOut, L" %s", Names.Names[n]);
        else if (n == 1)
            ConPrintf(StdOut, L" (%s", Names.Names[n]);
        else
            ConPrintf(StdOut, L" / %s", Names.Names[n]);
    }

    ConPrintf(StdOut, L"%s\n", Names.NumberOfNames > 1 ? L")" : L"");
}


BOOL ParseAsn(PBYTE pRoot, PBYTE pData, DWORD dwSize, PWSTR wszPrefix, BOOL fPrint)
{
    while (dwSize)
    {
        PBYTE pHeader = pData;
        DWORD dwTagAndClass;

        if (!ParseTag(pData, dwSize, dwTagAndClass))
        {
            if (fPrint)
                ConPrintf(StdOut, L"CertUtil: -asn command failed to parse tag near 0x%x\n", pHeader - pRoot);
            return FALSE;
        }

        DWORD dwTagLength;
        if (!ParseLength(pData, dwSize, dwTagLength))
        {
            if (fPrint)
                ConPrintf(StdOut, L"CertUtil: -asn command failed to parse tag length near 0x%x\n", pHeader - pRoot);
            return FALSE;
        }

        if (dwTagLength > dwSize)
        {
            if (fPrint)
                ConPrintf(StdOut, L"CertUtil: -asn command malformed tag length near 0x%x\n", pHeader - pRoot);
            return FALSE;
        }


        if (fPrint)
            PrintTag(pRoot, pHeader, dwTagAndClass, dwTagLength, pData, wszPrefix);

        size_t len = wcslen(wszPrefix);
        StringCchCatW(wszPrefix, MAX_PATH, dwTagLength != dwSize ? L"|  " : L"   ");

        if (dwTagAndClass & ASN_TAG_IS_CONSTRUCTED)
        {
            if (!ParseAsn(pRoot, pData, dwTagLength, wszPrefix, fPrint))
            {
                return FALSE;
            }
        }
        else
        {
            if (fPrint)
            {
                /* Special case for a bit string / octet string */
                if ((dwTagAndClass == ASN_TAG_BITSTRING || dwTagAndClass == ASN_TAG_OCTET_STRING) && dwTagLength)
                {
                    if (dwTagAndClass == ASN_TAG_BITSTRING)
                    {
                        /* First, we print the 'unused bits' field of the bit string */
                        HexDump(pRoot, pData, 1, wszPrefix);
                        ConPuts(StdOut, L"\n");

                        /* Move past it */
                        Move(1, pData, dwSize);
                        dwTagLength--;
                    }

                    /* Do we have any data left? */
                    if (dwTagLength)
                    {
                        /* Try to parse this as ASN */
                        if (ParseAsn(pRoot, pData, dwTagLength, wszPrefix, FALSE))
                        {
                            /* We succeeded, this _could_ be ASN, so display it as if it is */
                            if (!ParseAsn(pRoot, pData, dwTagLength, wszPrefix, TRUE))
                            {
                                /* Uhhh, did someone edit the data? */
                                ConPrintf(StdOut, L"CertUtil: -asn command unexpected failure parsing tag near 0x%x\n", pData - pRoot);
                                return FALSE;
                            }

                            /* Move past what we just parsed */
                            Move(dwTagLength, pData, dwSize);
                            /* Lie about this so that we don't also print a hexdump */
                            dwTagLength = 0;
                        }
                    }
                }

                /* Is there any data (left) to print? */
                if (dwTagLength)
                {
                    HexDump(pRoot, pData, dwTagLength, wszPrefix);
                    ConPuts(StdOut, L"\n");

                    StringCchCatW(wszPrefix, MAX_PATH, L"   ");

                    /* Do we have additional formatters? */
                    switch (dwTagAndClass)
                    {
                    case ASN_TAG_OBJECT_ID:
                        PrintOID(pRoot, pHeader, pData, dwTagLength, wszPrefix);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        wszPrefix[len] = '\0';

        if (!Move(dwTagLength, pData, dwSize))
        {
            /* This should not be possible, it was checked before! */
            return FALSE;
        }
    }

    return TRUE;
}


BOOL asn_dump(LPCWSTR Filename)
{
    HANDLE hFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ConPrintf(StdOut, L"CertUtil: -asn command failed to open: %d\n", GetLastError());
        return FALSE;
    }

    DWORD dwSize = GetFileSize(hFile, NULL);
    if (dwSize == INVALID_FILE_SIZE)
    {
        ConPrintf(StdOut, L"CertUtil: -asn command failed to get file size: %d\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    if (dwSize == 0)
    {
        ConPrintf(StdOut, L"CertUtil: -asn command got an empty file\n");
        CloseHandle(hFile);
        return FALSE;
    }

    PBYTE pData = (PBYTE)LocalAlloc(0, dwSize);
    if (!pData)
    {
        ConPrintf(StdOut, L"CertUtil: -asn command failed to allocate: %d\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD cbRead;
    BOOL fRead = ReadFile(hFile, pData, dwSize, &cbRead, NULL);
    DWORD dwErr = GetLastError();
    CloseHandle(hFile);

    if (!fRead || cbRead != dwSize)
    {
        ConPrintf(StdOut, L"CertUtil: -asn command failed to read: %d\n", dwErr);
        LocalFree(pData);
        return FALSE;
    }

    WCHAR Buffer[MAX_PATH] = {0};
    BOOL fSucceeded = ParseAsn(pData, pData, dwSize, Buffer, TRUE);

    LocalFree(pData);
    return fSucceeded;
}

