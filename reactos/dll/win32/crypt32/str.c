/*
 * Copyright 2006 Juan Lang for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

DWORD WINAPI CertRDNValueToStrA(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
 LPSTR psz, DWORD csz)
{
    DWORD ret = 0;

    TRACE("(%d, %p, %p, %d)\n", dwValueType, pValue, psz, csz);

    switch (dwValueType)
    {
    case CERT_RDN_ANY_TYPE:
        break;
    case CERT_RDN_NUMERIC_STRING:
    case CERT_RDN_PRINTABLE_STRING:
    case CERT_RDN_TELETEX_STRING:
    case CERT_RDN_VIDEOTEX_STRING:
    case CERT_RDN_IA5_STRING:
    case CERT_RDN_GRAPHIC_STRING:
    case CERT_RDN_VISIBLE_STRING:
    case CERT_RDN_GENERAL_STRING:
        if (!psz || !csz)
            ret = pValue->cbData;
        else
        {
            DWORD chars = min(pValue->cbData, csz - 1);

            if (chars)
            {
                memcpy(psz, pValue->pbData, chars);
                ret += chars;
                csz -= chars;
            }
        }
        break;
    default:
        FIXME("string type %d unimplemented\n", dwValueType);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
        csz--;
        ret++;
    }
    else
        ret++;
    TRACE("returning %d (%s)\n", ret, debugstr_a(psz));
    return ret;
}

DWORD WINAPI CertRDNValueToStrW(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
 LPWSTR psz, DWORD csz)
{
    DWORD ret = 0;

    TRACE("(%d, %p, %p, %d)\n", dwValueType, pValue, psz, csz);

    switch (dwValueType)
    {
    case CERT_RDN_ANY_TYPE:
        break;
    case CERT_RDN_NUMERIC_STRING:
    case CERT_RDN_PRINTABLE_STRING:
    case CERT_RDN_TELETEX_STRING:
    case CERT_RDN_VIDEOTEX_STRING:
    case CERT_RDN_IA5_STRING:
    case CERT_RDN_GRAPHIC_STRING:
    case CERT_RDN_VISIBLE_STRING:
    case CERT_RDN_GENERAL_STRING:
        if (!psz || !csz)
            ret = pValue->cbData;
        else
        {
            DWORD chars = min(pValue->cbData, csz - 1);

            if (chars)
            {
                DWORD i;

                for (i = 0; i < chars; i++)
                    psz[i] = pValue->pbData[i];
                ret += chars;
                csz -= chars;
            }
        }
        break;
    default:
        FIXME("string type %d unimplemented\n", dwValueType);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
        csz--;
        ret++;
    }
    else
        ret++;
    TRACE("returning %d (%s)\n", ret, debugstr_w(psz));
    return ret;
}

/* Adds the prefix prefix to the string pointed to by psz, followed by the
 * character '='.  Copies no more than csz characters.  Returns the number of
 * characters copied.  If psz is NULL, returns the number of characters that
 * would be copied.
 */
static DWORD CRYPT_AddPrefixA(LPCSTR prefix, LPSTR psz, DWORD csz)
{
    DWORD chars;

    TRACE("(%s, %p, %d)\n", debugstr_a(prefix), psz, csz);

    if (psz)
    {
        chars = min(lstrlenA(prefix), csz);
        memcpy(psz, prefix, chars);
        csz -= chars;
        *(psz + chars) = '=';
        chars++;
        csz--;
    }
    else
        chars = lstrlenA(prefix) + 1;
    return chars;
}

DWORD WINAPI CertNameToStrA(DWORD dwCertEncodingType, PCERT_NAME_BLOB pName,
 DWORD dwStrType, LPSTR psz, DWORD csz)
{
    static const DWORD unsupportedFlags = CERT_NAME_STR_NO_QUOTING_FLAG |
     CERT_NAME_STR_REVERSE_FLAG | CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
    static const char commaSep[] = ", ";
    static const char semiSep[] = "; ";
    static const char crlfSep[] = "\r\n";
    static const char plusSep[] = " + ";
    static const char spaceSep[] = " ";
    DWORD ret = 0, bytes = 0;
    BOOL bRet;
    CERT_NAME_INFO *info;

    TRACE("(%d, %p, %08x, %p, %d)\n", dwCertEncodingType, pName, dwStrType,
     psz, csz);
    if (dwStrType & unsupportedFlags)
        FIXME("unsupported flags: %08x\n", dwStrType & unsupportedFlags);

    bRet = CryptDecodeObjectEx(dwCertEncodingType, X509_NAME, pName->pbData,
     pName->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &bytes);
    if (bRet)
    {
        DWORD i, j, sepLen, rdnSepLen;
        LPCSTR sep, rdnSep;

        if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
            sep = semiSep;
        else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
            sep = crlfSep;
        else
            sep = commaSep;
        sepLen = strlen(sep);
        if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
            rdnSep = spaceSep;
        else
            rdnSep = plusSep;
        rdnSepLen = strlen(rdnSep);
        for (i = 0; (!psz || ret < csz) && i < info->cRDN; i++)
        {
            for (j = 0; (!psz || ret < csz) && j < info->rgRDN[i].cRDNAttr; j++)
            {
                DWORD chars;
                char prefixBuf[10]; /* big enough for GivenName */
                LPCSTR prefix = NULL;

                if ((dwStrType & 0x000000ff) == CERT_OID_NAME_STR)
                    prefix = info->rgRDN[i].rgRDNAttr[j].pszObjId;
                else if ((dwStrType & 0x000000ff) == CERT_X500_NAME_STR)
                {
                    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(
                     CRYPT_OID_INFO_OID_KEY,
                     info->rgRDN[i].rgRDNAttr[j].pszObjId,
                     CRYPT_RDN_ATTR_OID_GROUP_ID);

                    if (oidInfo)
                    {
                        WideCharToMultiByte(CP_ACP, 0, oidInfo->pwszName, -1,
                         prefixBuf, sizeof(prefixBuf), NULL, NULL);
                        prefix = prefixBuf;
                    }
                    else
                        prefix = info->rgRDN[i].rgRDNAttr[j].pszObjId;
                }
                if (prefix)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixA(prefix,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                    csz -= chars;
                }
                /* FIXME: handle quoting */
                chars = CertRDNValueToStrA(
                 info->rgRDN[i].rgRDNAttr[j].dwValueType,
                 &info->rgRDN[i].rgRDNAttr[j].Value, psz ? psz + ret : NULL,
                 psz ? csz - ret : 0);
                if (chars)
                    ret += chars - 1;
                if (j < info->rgRDN[i].cRDNAttr - 1)
                {
                    if (psz && ret < csz - rdnSepLen - 1)
                        memcpy(psz + ret, rdnSep, rdnSepLen);
                    ret += rdnSepLen;
                }
            }
            if (i < info->cRDN - 1)
            {
                if (psz && ret < csz - sepLen - 1)
                    memcpy(psz + ret, sep, sepLen);
                ret += sepLen;
            }
        }
        LocalFree(info);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
        csz--;
        ret++;
    }
    else
        ret++;
    TRACE("Returning %s\n", debugstr_a(psz));
    return ret;
}

/* Adds the prefix prefix to the wide-character string pointed to by psz,
 * followed by the character '='.  Copies no more than csz characters.  Returns
 * the number of characters copied.  If psz is NULL, returns the number of
 * characters that would be copied.
 * Assumes the characters in prefix are ASCII (not multibyte characters.)
 */
static DWORD CRYPT_AddPrefixAToW(LPCSTR prefix, LPWSTR psz, DWORD csz)
{
    DWORD chars;

    TRACE("(%s, %p, %d)\n", debugstr_a(prefix), psz, csz);

    if (psz)
    {
        DWORD i;

        chars = min(lstrlenA(prefix), csz);
        for (i = 0; i < chars; i++)
            *(psz + i) = prefix[i];
        csz -= chars;
        *(psz + chars) = '=';
        chars++;
        csz--;
    }
    else
        chars = lstrlenA(prefix) + 1;
    return chars;
}

/* Adds the prefix prefix to the string pointed to by psz, followed by the
 * character '='.  Copies no more than csz characters.  Returns the number of
 * characters copied.  If psz is NULL, returns the number of characters that
 * would be copied.
 */
static DWORD CRYPT_AddPrefixW(LPCWSTR prefix, LPWSTR psz, DWORD csz)
{
    DWORD chars;

    TRACE("(%s, %p, %d)\n", debugstr_w(prefix), psz, csz);

    if (psz)
    {
        chars = min(lstrlenW(prefix), csz);
        memcpy(psz, prefix, chars * sizeof(WCHAR));
        csz -= chars;
        *(psz + chars) = '=';
        chars++;
        csz--;
    }
    else
        chars = lstrlenW(prefix) + 1;
    return chars;
}

DWORD WINAPI CertNameToStrW(DWORD dwCertEncodingType, PCERT_NAME_BLOB pName,
 DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    static const DWORD unsupportedFlags = CERT_NAME_STR_NO_QUOTING_FLAG |
     CERT_NAME_STR_REVERSE_FLAG | CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
    static const WCHAR commaSep[] = { ',',' ',0 };
    static const WCHAR semiSep[] = { ';',' ',0 };
    static const WCHAR crlfSep[] = { '\r','\n',0 };
    static const WCHAR plusSep[] = { ' ','+',' ',0 };
    static const WCHAR spaceSep[] = { ' ',0 };
    DWORD ret = 0, bytes = 0;
    BOOL bRet;
    CERT_NAME_INFO *info;

    TRACE("(%d, %p, %08x, %p, %d)\n", dwCertEncodingType, pName, dwStrType,
     psz, csz);
    if (dwStrType & unsupportedFlags)
        FIXME("unsupported flags: %08x\n", dwStrType & unsupportedFlags);

    bRet = CryptDecodeObjectEx(dwCertEncodingType, X509_NAME, pName->pbData,
     pName->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &bytes);
    if (bRet)
    {
        DWORD i, j, sepLen, rdnSepLen;
        LPCWSTR sep, rdnSep;

        if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
            sep = semiSep;
        else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
            sep = crlfSep;
        else
            sep = commaSep;
        sepLen = lstrlenW(sep);
        if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
            rdnSep = spaceSep;
        else
            rdnSep = plusSep;
        rdnSepLen = lstrlenW(rdnSep);
        for (i = 0; (!psz || ret < csz) && i < info->cRDN; i++)
        {
            for (j = 0; (!psz || ret < csz) && j < info->rgRDN[i].cRDNAttr; j++)
            {
                DWORD chars;
                LPCSTR prefixA = NULL;
                LPCWSTR prefixW = NULL;

                if ((dwStrType & 0x000000ff) == CERT_OID_NAME_STR)
                    prefixA = info->rgRDN[i].rgRDNAttr[j].pszObjId;
                else if ((dwStrType & 0x000000ff) == CERT_X500_NAME_STR)
                {
                    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(
                     CRYPT_OID_INFO_OID_KEY,
                     info->rgRDN[i].rgRDNAttr[j].pszObjId,
                     CRYPT_RDN_ATTR_OID_GROUP_ID);

                    if (oidInfo)
                        prefixW = oidInfo->pwszName;
                    else
                        prefixA = info->rgRDN[i].rgRDNAttr[j].pszObjId;
                }
                if (prefixW)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixW(prefixW,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                    csz -= chars;
                }
                else if (prefixA)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixAToW(prefixA,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                    csz -= chars;
                }
                /* FIXME: handle quoting */
                chars = CertRDNValueToStrW(
                 info->rgRDN[i].rgRDNAttr[j].dwValueType,
                 &info->rgRDN[i].rgRDNAttr[j].Value, psz ? psz + ret : NULL,
                 psz ? csz - ret : 0);
                if (chars)
                    ret += chars - 1;
                if (j < info->rgRDN[i].cRDNAttr - 1)
                {
                    if (psz && ret < csz - rdnSepLen - 1)
                        memcpy(psz + ret, rdnSep, rdnSepLen * sizeof(WCHAR));
                    ret += rdnSepLen;
                }
            }
            if (i < info->cRDN - 1)
            {
                if (psz && ret < csz - sepLen - 1)
                    memcpy(psz + ret, sep, sepLen * sizeof(WCHAR));
                ret += sepLen;
            }
        }
        LocalFree(info);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
        csz--;
        ret++;
    }
    else
        ret++;
    TRACE("Returning %s\n", debugstr_w(psz));
    return ret;
}

BOOL WINAPI CertStrToNameA(DWORD dwCertEncodingType, LPCSTR pszX500,
 DWORD dwStrType, void *pvReserved, BYTE *pbEncoded, DWORD *pcbEncoded,
 LPCSTR *ppszError)
{
    LPWSTR x500, errorStr;
    BOOL ret;
    int len;

    TRACE("(%08x, %s, %08x, %p, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(pszX500), dwStrType, pvReserved, pbEncoded, pcbEncoded,
     ppszError);

    len = MultiByteToWideChar(CP_ACP, 0, pszX500, -1, NULL, 0);
    x500 = CryptMemAlloc(len * sizeof(WCHAR));
    if (x500)
    {
        MultiByteToWideChar(CP_ACP, 0, pszX500, -1, x500, len);
        ret = CertStrToNameW(dwCertEncodingType, x500, dwStrType, pvReserved,
         pbEncoded, pcbEncoded, ppszError ? (LPCWSTR *)&errorStr : NULL);
        if (ppszError)
        {
            DWORD i;

            *ppszError = pszX500;
            for (i = 0; i < errorStr - x500; i++)
                CharNextA(*ppszError);
        }
        CryptMemFree(x500);
    }
    else
        ret = FALSE;
    return ret;
}

struct KeynameKeeper
{
    WCHAR  buf[10]; /* big enough for L"GivenName" */
    LPWSTR keyName; /* usually = buf, but may be allocated */
    DWORD  keyLen;
};

static void CRYPT_InitializeKeynameKeeper(struct KeynameKeeper *keeper)
{
    keeper->keyName = keeper->buf;
    keeper->keyLen = sizeof(keeper->buf) / sizeof(keeper->buf[0]);
}

static void CRYPT_FreeKeynameKeeper(struct KeynameKeeper *keeper)
{
    if (keeper->keyName != keeper->buf)
        CryptMemFree(keeper->keyName);
}

struct X500TokenW
{
    LPCWSTR start;
    LPCWSTR end;
};

static void CRYPT_KeynameKeeperFromTokenW(struct KeynameKeeper *keeper,
 const struct X500TokenW *key)
{
    DWORD len = key->end - key->start;

    if (len > keeper->keyLen)
    {
        if (keeper->keyName == keeper->buf)
            keeper->keyName = CryptMemAlloc(len * sizeof(WCHAR));
        else
            keeper->keyName = CryptMemRealloc(keeper->keyName,
             len * sizeof(WCHAR));
        keeper->keyLen = len;
    }
    memcpy(keeper->keyName, key->start, (key->end - key->start) *
     sizeof(WCHAR));
    keeper->keyName[len] = '\0';
    TRACE("Keyname is %s\n", debugstr_w(keeper->keyName));
}

static DWORD CRYPT_GetNextKeyW(LPCWSTR str, struct X500TokenW *token,
 LPCWSTR *ppszError)
{
    DWORD ret = ERROR_SUCCESS;

    while (*str && isspaceW(*str))
        str++;
    if (*str)
    {
        token->start = str;
        while (*str && *str != '=' && !isspaceW(*str))
            str++;
        if (*str && (*str == '=' || isspaceW(*str)))
            token->end = str;
        else
        {
            TRACE("missing equals char at %s\n", debugstr_w(token->start));
            if (ppszError)
                *ppszError = token->start;
            ret = CRYPT_E_INVALID_X500_STRING;
        }
    }
    else
        token->start = NULL;
    return ret;
}

/* Assumes separators are characters in the 0-255 range */
static DWORD CRYPT_GetNextValueW(LPCWSTR str, DWORD dwFlags, LPCWSTR separators,
 struct X500TokenW *token, LPCWSTR *ppszError)
{
    DWORD ret = ERROR_SUCCESS;

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(str), debugstr_w(separators), token,
     ppszError);

    while (*str && isspaceW(*str))
        str++;
    if (*str)
    {
        token->start = str;
        if (!(dwFlags & CERT_NAME_STR_NO_QUOTING_FLAG) && *str == '"')
        {
            token->end = NULL;
            str++;
            while (!token->end && !ret)
            {
                while (*str && *str != '"')
                    str++;
                if (*str == '"')
                {
                    if (*(str + 1) != '"')
                        token->end = str + 1;
                    else
                        str += 2;
                }
                else
                {
                    TRACE("unterminated quote at %s\n", debugstr_w(str));
                    if (ppszError)
                        *ppszError = str;
                    ret = CRYPT_E_INVALID_X500_STRING;
                }
            }
        }
        else
        {
            WCHAR map[256] = { 0 };

            while (*separators)
                map[*separators++] = 1;
            while (*str && (*str >= 0xff || !map[*(const unsigned short *)str]))
                str++;
            token->end = str;
        }
    }
    else
    {
        TRACE("missing value at %s\n", debugstr_w(str));
        if (ppszError)
            *ppszError = str;
        ret = CRYPT_E_INVALID_X500_STRING;
    }
    return ret;
}

/* Encodes the string represented by value as the string type type into the
 * CERT_NAME_BLOB output.  If there is an error and ppszError is not NULL,
 * *ppszError is set to the first failing character.  If there is no error,
 * output's pbData must be freed with LocalFree.
 */
static BOOL CRYPT_EncodeValueWithType(DWORD dwCertEncodingType,
 const struct X500TokenW *value, PCERT_NAME_BLOB output, DWORD type,
 LPCWSTR *ppszError)
{
    CERT_NAME_VALUE nameValue = { type, { 0, NULL } };
    BOOL ret = FALSE;

    nameValue.Value.pbData = CryptMemAlloc((value->end - value->start) *
     sizeof(WCHAR));
    if (nameValue.Value.pbData)
    {
        DWORD i;
        LPWSTR ptr = (LPWSTR)nameValue.Value.pbData;

        for (i = 0; i < value->end - value->start; i++)
        {
            *ptr++ = value->start[i];
            if (value->start[i] == '"')
                i++;
        }
        nameValue.Value.cbData = (LPBYTE)ptr - nameValue.Value.pbData;
        ret = CryptEncodeObjectEx(dwCertEncodingType, X509_UNICODE_NAME_VALUE,
         &nameValue, CRYPT_ENCODE_ALLOC_FLAG, NULL, &output->pbData,
         &output->cbData);
        if (!ret && ppszError)
        {
            if (type == CERT_RDN_NUMERIC_STRING &&
             GetLastError() == CRYPT_E_INVALID_NUMERIC_STRING)
                *ppszError = value->start + output->cbData;
            else if (type == CERT_RDN_PRINTABLE_STRING &&
             GetLastError() == CRYPT_E_INVALID_PRINTABLE_STRING)
                *ppszError = value->start + output->cbData;
            else if (type == CERT_RDN_IA5_STRING &&
             GetLastError() == CRYPT_E_INVALID_IA5_STRING)
                *ppszError = value->start + output->cbData;
        }
        CryptMemFree(nameValue.Value.pbData);
    }
    return ret;
}

static BOOL CRYPT_EncodeValue(DWORD dwCertEncodingType,
 const struct X500TokenW *value, PCERT_NAME_BLOB output, const DWORD *types,
 LPCWSTR *ppszError)
{
    DWORD i;
    BOOL ret;

    ret = FALSE;
    for (i = 0; !ret && types[i]; i++)
        ret = CRYPT_EncodeValueWithType(dwCertEncodingType, value, output,
         types[i], ppszError);
    return ret;
}

static BOOL CRYPT_ValueToRDN(DWORD dwCertEncodingType, PCERT_NAME_INFO info,
 PCCRYPT_OID_INFO keyOID, struct X500TokenW *value, LPCWSTR *ppszError)
{
    BOOL ret = FALSE;

    TRACE("OID %s, value %s\n", debugstr_a(keyOID->pszOID),
     debugstr_wn(value->start, value->end - value->start));

    if (!info->rgRDN)
        info->rgRDN = CryptMemAlloc(sizeof(CERT_RDN));
    else
        info->rgRDN = CryptMemRealloc(info->rgRDN,
         (info->cRDN + 1) * sizeof(CERT_RDN));
    if (info->rgRDN)
    {
        /* FIXME: support multiple RDN attrs */
        info->rgRDN[info->cRDN].rgRDNAttr =
         CryptMemAlloc(sizeof(CERT_RDN_ATTR));
        if (info->rgRDN[info->cRDN].rgRDNAttr)
        {
            static const DWORD defaultTypes[] = { CERT_RDN_PRINTABLE_STRING,
             CERT_RDN_BMP_STRING, 0 };
            const DWORD *types;

            info->rgRDN[info->cRDN].cRDNAttr = 1;
            info->rgRDN[info->cRDN].rgRDNAttr[0].pszObjId =
             (LPSTR)keyOID->pszOID;
            info->rgRDN[info->cRDN].rgRDNAttr[0].dwValueType =
             CERT_RDN_ENCODED_BLOB;
            if (keyOID->ExtraInfo.cbData)
                types = (const DWORD *)keyOID->ExtraInfo.pbData;
            else
                types = defaultTypes;

            /* Remove surrounding quotes */
            if (value->start[0] == '"')
            {
                value->start++;
                value->end--;
            }
            ret = CRYPT_EncodeValue(dwCertEncodingType, value,
             &info->rgRDN[info->cRDN].rgRDNAttr[0].Value, types, ppszError);
        }
    }
    if (ret)
        info->cRDN++;
    return ret;
}

BOOL WINAPI CertStrToNameW(DWORD dwCertEncodingType, LPCWSTR pszX500,
 DWORD dwStrType, void *pvReserved, BYTE *pbEncoded, DWORD *pcbEncoded,
 LPCWSTR *ppszError)
{
    CERT_NAME_INFO info = { 0, NULL };
    LPCWSTR str;
    struct KeynameKeeper keeper;
    DWORD i, error = ERROR_SUCCESS;
    BOOL ret = TRUE;

    TRACE("(%08x, %s, %08x, %p, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_w(pszX500), dwStrType, pvReserved, pbEncoded, pcbEncoded,
     ppszError);

    CRYPT_InitializeKeynameKeeper(&keeper);
    str = pszX500;
    while (str && *str && !error && ret)
    {
        struct X500TokenW token;

        error = CRYPT_GetNextKeyW(str, &token, ppszError);
        if (!error && token.start)
        {
            PCCRYPT_OID_INFO keyOID;

            CRYPT_KeynameKeeperFromTokenW(&keeper, &token);
            keyOID = CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY, keeper.keyName,
             CRYPT_RDN_ATTR_OID_GROUP_ID);
            if (!keyOID)
            {
                if (ppszError)
                    *ppszError = token.start;
                error = CRYPT_E_INVALID_X500_STRING;
            }
            else
            {
                str = token.end;
                while (isspace(*str))
                    str++;
                if (*str != '=')
                {
                    if (ppszError)
                        *ppszError = str;
                    error = CRYPT_E_INVALID_X500_STRING;
                }
                else
                {
                    static const WCHAR commaSep[] = { ',',0 };
                    static const WCHAR semiSep[] = { ';',0 };
                    static const WCHAR crlfSep[] = { '\r','\n',0 };
                    static const WCHAR allSeps[] = { ',',';','\r','\n',0 };
                    LPCWSTR sep;

                    str++;
                    if (dwStrType & CERT_NAME_STR_COMMA_FLAG)
                        sep = commaSep;
                    else if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
                        sep = semiSep;
                    else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
                        sep = crlfSep;
                    else
                        sep = allSeps;
                    error = CRYPT_GetNextValueW(str, dwStrType, sep, &token,
                     ppszError);
                    if (!error)
                    {
                        str = token.end;
                        ret = CRYPT_ValueToRDN(dwCertEncodingType, &info,
                         keyOID, &token, ppszError);
                    }
                }
            }
        }
    }
    CRYPT_FreeKeynameKeeper(&keeper);
    if (!error)
    {
        ret = CryptEncodeObjectEx(dwCertEncodingType, X509_NAME, &info,
         0, NULL, pbEncoded, pcbEncoded);
        for (i = 0; i < info.cRDN; i++)
        {
            DWORD j;

            for (j = 0; j < info.rgRDN[i].cRDNAttr; j++)
                LocalFree(info.rgRDN[i].rgRDNAttr[j].Value.pbData);
            CryptMemFree(info.rgRDN[i].rgRDNAttr);
        }
        CryptMemFree(info.rgRDN);
    }
    else
    {
        SetLastError(error);
        ret = FALSE;
    }
    return ret;
}

DWORD WINAPI CertGetNameStringA(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, void *pvTypePara, LPSTR pszNameString, DWORD cchNameString)
{
    DWORD ret;

    TRACE("(%p, %d, %08x, %p, %p, %d)\n", pCertContext, dwType, dwFlags,
     pvTypePara, pszNameString, cchNameString);

    if (pszNameString)
    {
        LPWSTR wideName;
        DWORD nameLen;

        nameLen = CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara,
         NULL, 0);
        wideName = CryptMemAlloc(nameLen * sizeof(WCHAR));
        if (wideName)
        {
            CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara,
             wideName, nameLen);
            nameLen = WideCharToMultiByte(CP_ACP, 0, wideName, nameLen,
             pszNameString, cchNameString, NULL, NULL);
            if (nameLen <= cchNameString)
                ret = nameLen;
            else
            {
                pszNameString[cchNameString - 1] = '\0';
                ret = cchNameString;
            }
            CryptMemFree(wideName);
        }
        else
        {
            *pszNameString = '\0';
            ret = 1;
        }
    }
    else
        ret = CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara,
         NULL, 0);
    return ret;
}

DWORD WINAPI CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, void *pvTypePara, LPWSTR pszNameString, DWORD cchNameString)
{
    DWORD ret;
    PCERT_NAME_BLOB name;
    LPCSTR altNameOID;

    TRACE("(%p, %d, %08x, %p, %p, %d)\n", pCertContext, dwType,
     dwFlags, pvTypePara, pszNameString, cchNameString);

    if (dwFlags & CERT_NAME_ISSUER_FLAG)
    {
        name = &pCertContext->pCertInfo->Issuer;
        altNameOID = szOID_ISSUER_ALT_NAME;
    }
    else
    {
        name = &pCertContext->pCertInfo->Subject;
        altNameOID = szOID_SUBJECT_ALT_NAME;
    }

    switch (dwType)
    {
    case CERT_NAME_SIMPLE_DISPLAY_TYPE:
    {
        static const LPCSTR simpleAttributeOIDs[] = { szOID_COMMON_NAME,
         szOID_ORGANIZATIONAL_UNIT_NAME, szOID_ORGANIZATION_NAME,
         szOID_RSA_emailAddr };
        CERT_NAME_INFO *info = NULL;
        PCERT_RDN_ATTR nameAttr = NULL;
        DWORD bytes = 0, i;

        if (CryptDecodeObjectEx(pCertContext->dwCertEncodingType, X509_NAME,
         name->pbData, name->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &info,
         &bytes))
        {
            for (i = 0; !nameAttr && i < sizeof(simpleAttributeOIDs) /
             sizeof(simpleAttributeOIDs[0]); i++)
                nameAttr = CertFindRDNAttr(simpleAttributeOIDs[i], info);
        }
        else
            ret = 0;
        if (!nameAttr)
        {
            PCERT_EXTENSION ext = CertFindExtension(altNameOID,
             pCertContext->pCertInfo->cExtension,
             pCertContext->pCertInfo->rgExtension);

            if (ext)
            {
                for (i = 0; !nameAttr && i < sizeof(simpleAttributeOIDs) /
                 sizeof(simpleAttributeOIDs[0]); i++)
                    nameAttr = CertFindRDNAttr(simpleAttributeOIDs[i], info);
                if (!nameAttr)
                {
                    /* FIXME: gotta then look for a rfc822Name choice in ext.
                     * Failing that, look for the first attribute.
                     */
                    FIXME("CERT_NAME_SIMPLE_DISPLAY_TYPE: stub\n");
                    ret = 0;
                }
            }
        }
        ret = CertRDNValueToStrW(nameAttr->dwValueType, &nameAttr->Value,
         pszNameString, cchNameString);
        if (info)
            LocalFree(info);
        break;
    }
    case CERT_NAME_FRIENDLY_DISPLAY_TYPE:
    {
        DWORD cch = cchNameString;

        if (CertGetCertificateContextProperty(pCertContext,
         CERT_FRIENDLY_NAME_PROP_ID, pszNameString, &cch))
            ret = cch;
        else
            ret = CertGetNameStringW(pCertContext,
             CERT_NAME_SIMPLE_DISPLAY_TYPE, dwFlags, pvTypePara, pszNameString,
             cchNameString);
        break;
    }
    default:
        FIXME("unimplemented for type %d\n", dwType);
        ret = 0;
    }
    return ret;
}
