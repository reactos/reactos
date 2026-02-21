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
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

DWORD WINAPI CertRDNValueToStrA(DWORD type, PCERT_RDN_VALUE_BLOB value_blob,
                                LPSTR value, DWORD value_len)
{
    DWORD len, len_mb, ret;
    LPWSTR valueW;

    TRACE("(%ld, %p, %p, %ld)\n", type, value_blob, value, value_len);

    len = CertRDNValueToStrW(type, value_blob, NULL, 0);

    if (!(valueW = CryptMemAlloc(len * sizeof(*valueW))))
    {
        ERR("No memory.\n");
        if (value && value_len) *value = 0;
        return 1;
    }

    len = CertRDNValueToStrW(type, value_blob, valueW, len);
    len_mb = WideCharToMultiByte(CP_ACP, 0, valueW, len, NULL, 0, NULL, NULL);
    if (!value || !value_len)
    {
        CryptMemFree(valueW);
        return len_mb;
    }

    ret = WideCharToMultiByte(CP_ACP, 0, valueW, len, value, value_len, NULL, NULL);
    if (ret < len_mb)
    {
        value[0] = 0;
        ret = 1;
    }
    CryptMemFree(valueW);
    return ret;
}

static DWORD rdn_value_to_strW(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
                               LPWSTR psz, DWORD csz, BOOL partial_copy)
{
    DWORD ret = 0, len, i;

    TRACE("(%ld, %p, %p, %ld)\n", dwValueType, pValue, psz, csz);

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
        len = pValue->cbData;
        if (!psz || !csz) ret = len;
        else if (len < csz || partial_copy)
        {
            len = min(len, csz - 1);
            for (i = 0; i < len; ++i)
                psz[i] = pValue->pbData[i];
            ret = len;
        }
        break;
    case CERT_RDN_BMP_STRING:
    case CERT_RDN_UTF8_STRING:
        len = pValue->cbData / sizeof(WCHAR);
        if (!psz || !csz)
            ret = len;
        else if (len < csz || partial_copy)
        {
            WCHAR *ptr = psz;

            len = min(len, csz - 1);
            for (i = 0; i < len; ++i)
                ptr[i] = ((LPCWSTR)pValue->pbData)[i];
            ret = len;
        }
        break;
    default:
        FIXME("string type %ld unimplemented\n", dwValueType);
    }
    if (psz && csz) psz[ret] = 0;
    TRACE("returning %ld (%s)\n", ret + 1, debugstr_w(psz));
    return ret + 1;
}

DWORD WINAPI CertRDNValueToStrW(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
                                LPWSTR psz, DWORD csz)
{
    return rdn_value_to_strW(dwValueType, pValue, psz, csz, FALSE);
}

static inline BOOL is_quotable_char(WCHAR c)
{
    switch(c)
    {
    case '+':
    case ',':
    case '"':
    case '=':
    case '<':
    case '>':
    case ';':
    case '#':
    case '\n':
        return TRUE;
    default:
        return FALSE;
    }
}

static inline BOOL is_spaceW(WCHAR c)
{
    return c <= 0x7f && isspace((char)c);
}

static DWORD quote_rdn_value_to_str_w(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
                                      DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    DWORD ret = 0, len, i, strLen;
    BOOL needsQuotes = FALSE;

    TRACE("(%ld, %p, %p, %ld)\n", dwValueType, pValue, psz, csz);

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
        len = pValue->cbData;
        if (!(dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG))
        {
            if (pValue->cbData && isspace(pValue->pbData[0]))
                needsQuotes = TRUE;
            if (pValue->cbData && isspace(pValue->pbData[pValue->cbData - 1]))
                needsQuotes = TRUE;
            for (i = 0; i < pValue->cbData; i++)
            {
                if (is_quotable_char(pValue->pbData[i]))
                    needsQuotes = TRUE;
                if (pValue->pbData[i] == '"')
                    len += 1;
            }
            if (needsQuotes)
                len += 2;
        }
        if (!psz || !csz)
            ret = len;
        else
        {
            WCHAR *ptr = psz;

            if (needsQuotes)
                *ptr++ = '"';
            for (i = 0; i < pValue->cbData && ptr - psz < csz; ptr++, i++)
            {
                *ptr = pValue->pbData[i];
                if (!(dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG) &&
                    pValue->pbData[i] == '"' && ptr - psz < csz - 1)
                    *(++ptr) = '"';
            }
            if (needsQuotes && ptr - psz < csz)
                *ptr++ = '"';
            ret = ptr - psz;
        }
        break;
    case CERT_RDN_BMP_STRING:
    case CERT_RDN_UTF8_STRING:
        strLen = len = pValue->cbData / sizeof(WCHAR);
        if (!(dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG))
        {
            if (strLen && is_spaceW(((LPCWSTR)pValue->pbData)[0]))
                needsQuotes = TRUE;
            if (strLen && is_spaceW(((LPCWSTR)pValue->pbData)[strLen - 1]))
                needsQuotes = TRUE;
            for (i = 0; i < strLen; i++)
            {
                if (is_quotable_char(((LPCWSTR)pValue->pbData)[i]))
                    needsQuotes = TRUE;
                if (((LPCWSTR)pValue->pbData)[i] == '"')
                    len += 1;
            }
            if (needsQuotes)
                len += 2;
        }
        if (!psz || !csz)
            ret = len;
        else
        {
            WCHAR *ptr = psz;

            if (needsQuotes)
                *ptr++ = '"';
            for (i = 0; i < strLen && ptr - psz < csz; ptr++, i++)
            {
                *ptr = ((LPCWSTR)pValue->pbData)[i];
                if (!(dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG) &&
                    ((LPCWSTR)pValue->pbData)[i] == '"' && ptr - psz < csz - 1)
                    *(++ptr) = '"';
            }
            if (needsQuotes && ptr - psz < csz)
                *ptr++ = '"';
            ret = ptr - psz;
        }
        break;
    default:
        FIXME("string type %ld unimplemented\n", dwValueType);
    }
    TRACE("returning %ld (%s)\n", ret, debugstr_w(psz));
    return ret;
}

DWORD WINAPI CertNameToStrA(DWORD encoding_type, PCERT_NAME_BLOB name_blob, DWORD str_type, LPSTR str, DWORD str_len)
{
    DWORD len, len_mb, ret;
    LPWSTR strW;

    TRACE("(%ld, %p, %08lx, %p, %ld)\n", encoding_type, name_blob, str_type, str, str_len);

    len = CertNameToStrW(encoding_type, name_blob, str_type, NULL, 0);

    if (!(strW = CryptMemAlloc(len * sizeof(*strW))))
    {
        ERR("No memory.\n");
        if (str && str_len) *str = 0;
        return 1;
    }

    len = CertNameToStrW(encoding_type, name_blob, str_type, strW, len);
    len_mb = WideCharToMultiByte(CP_ACP, 0, strW, len, NULL, 0, NULL, NULL);
    if (!str || !str_len)
    {
        CryptMemFree(strW);
        return len_mb;
    }

    ret = WideCharToMultiByte(CP_ACP, 0, strW, len, str, str_len, NULL, NULL);
    if (ret < len_mb)
    {
        str[0] = 0;
        ret = 1;
    }
    CryptMemFree(strW);
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

    TRACE("(%s, %p, %ld)\n", debugstr_a(prefix), psz, csz);

    if (psz)
    {
        DWORD i;

        chars = min(strlen(prefix), csz);
        for (i = 0; i < chars; i++)
            *(psz + i) = prefix[i];
        *(psz + chars) = '=';
        chars++;
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

    TRACE("(%s, %p, %ld)\n", debugstr_w(prefix), psz, csz);

    if (psz)
    {
        chars = min(lstrlenW(prefix), csz);
        memcpy(psz, prefix, chars * sizeof(WCHAR));
        *(psz + chars) = '=';
        chars++;
    }
    else
        chars = lstrlenW(prefix) + 1;
    return chars;
}

static const WCHAR indent[] = L"     ";

DWORD cert_name_to_str_with_indent(DWORD dwCertEncodingType, DWORD indentLevel,
 const CERT_NAME_BLOB *pName, DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    static const DWORD unsupportedFlags = CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
    DWORD ret = 0, bytes = 0;
    BOOL bRet;
    CERT_NAME_INFO *info;
    DWORD chars;

    if (dwStrType & unsupportedFlags)
        FIXME("unsupported flags: %08lx\n", dwStrType & unsupportedFlags);

    bRet = CryptDecodeObjectEx(dwCertEncodingType, X509_NAME, pName->pbData,
     pName->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &info, &bytes);
    if (bRet)
    {
        DWORD i, j, sepLen, rdnSepLen;
        LPCWSTR sep, rdnSep;
        BOOL reverse = dwStrType & CERT_NAME_STR_REVERSE_FLAG;
        const CERT_RDN *rdn = info->rgRDN;

        if(reverse && info->cRDN > 1) rdn += (info->cRDN - 1);

        if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
            sep = L"; ";
        else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
            sep = L"\r\n";
        else
            sep = L", ";
        sepLen = lstrlenW(sep);
        if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
            rdnSep = L" ";
        else
            rdnSep = L" + ";
        rdnSepLen = lstrlenW(rdnSep);
        if (!csz) psz = NULL;
        for (i = 0; i < info->cRDN; i++)
        {
            if (psz && ret + 1 == csz) break;
            for (j = 0; j < rdn->cRDNAttr; j++)
            {
                LPCSTR prefixA = NULL;
                LPCWSTR prefixW = NULL;

                if (psz && ret + 1 == csz) break;

                if ((dwStrType & 0x000000ff) == CERT_OID_NAME_STR)
                    prefixA = rdn->rgRDNAttr[j].pszObjId;
                else if ((dwStrType & 0x000000ff) == CERT_X500_NAME_STR)
                {
                    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(
                     CRYPT_OID_INFO_OID_KEY,
                     rdn->rgRDNAttr[j].pszObjId,
                     CRYPT_RDN_ATTR_OID_GROUP_ID);

                    if (oidInfo)
                        prefixW = oidInfo->pwszName;
                    else
                        prefixA = rdn->rgRDNAttr[j].pszObjId;
                }
                if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
                {
                    DWORD k;

                    for (k = 0; k < indentLevel; k++)
                    {
                        if (psz)
                        {
                            chars = min(lstrlenW(indent), csz - ret - 1);
                            memcpy(psz + ret, indent, chars * sizeof(WCHAR));
                        }
                        else
                            chars = lstrlenW(indent);
                        ret += chars;
                    }
                    if (psz && ret + 1 == csz) break;
                }
                if (prefixW)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixW(prefixW,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                }
                else if (prefixA)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixAToW(prefixA,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                }
                if (psz && ret + 1 == csz) break;

                chars = quote_rdn_value_to_str_w(rdn->rgRDNAttr[j].dwValueType, &rdn->rgRDNAttr[j].Value, dwStrType,
                                                 psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                ret += chars;
                if (j < rdn->cRDNAttr - 1)
                {
                    if (psz)
                    {
                        chars = min(rdnSepLen, csz - ret - 1);
                        memcpy(psz + ret, rdnSep, chars * sizeof(WCHAR));
                        ret += chars;
                    }
                    else ret += rdnSepLen;
                }
            }
            if (psz && ret + 1 == csz) break;
            if (i < info->cRDN - 1)
            {
                if (psz)
                {
                    chars = min(sepLen, csz - ret - 1);
                    memcpy(psz + ret, sep, chars * sizeof(WCHAR));
                    ret += chars;
                }
                else ret += sepLen;
            }
            if(reverse) rdn--;
            else rdn++;
        }
        LocalFree(info);
    }
    if (psz && csz) psz[ret] = 0;
    return ret + 1;
}

DWORD WINAPI CertNameToStrW(DWORD dwCertEncodingType, PCERT_NAME_BLOB pName,
 DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    BOOL ret;

    TRACE("(%ld, %p, %08lx, %p, %ld)\n", dwCertEncodingType, pName, dwStrType,
     psz, csz);

    ret = cert_name_to_str_with_indent(dwCertEncodingType, 0, pName, dwStrType,
     psz, csz);
    TRACE("Returning %s\n", debugstr_w(psz));
    return ret;
}

BOOL WINAPI CertStrToNameA(DWORD dwCertEncodingType, LPCSTR pszX500,
 DWORD dwStrType, void *pvReserved, BYTE *pbEncoded, DWORD *pcbEncoded,
 LPCSTR *ppszError)
{
    BOOL ret;
    int len;

    TRACE("(%08lx, %s, %08lx, %p, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(pszX500), dwStrType, pvReserved, pbEncoded, pcbEncoded,
     ppszError);

    len = MultiByteToWideChar(CP_ACP, 0, pszX500, -1, NULL, 0);
    if (len)
    {
        LPWSTR x500, errorStr;

        if ((x500 = CryptMemAlloc(len * sizeof(WCHAR))))
        {
            MultiByteToWideChar(CP_ACP, 0, pszX500, -1, x500, len);
            ret = CertStrToNameW(dwCertEncodingType, x500, dwStrType,
             pvReserved, pbEncoded, pcbEncoded,
             ppszError ? (LPCWSTR *)&errorStr : NULL);
            if (ppszError)
            {
                if (!ret)
                {
                    LONG i;

                    *ppszError = pszX500;
                    for (i = 0; i < errorStr - x500; i++)
                        *ppszError = CharNextA(*ppszError);
                }
                else
                    *ppszError = NULL;
            }
            CryptMemFree(x500);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ret = FALSE;
        }
    }
    else
    {
        SetLastError(CRYPT_E_INVALID_X500_STRING);
        if (ppszError)
            *ppszError = pszX500;
        ret = FALSE;
    }
    return ret;
}

struct KeynameKeeper
{
    WCHAR  buf[10]; /* big enough for L"GivenName" */
    LPWSTR keyName; /* usually = buf, but may be allocated */
    DWORD  keyLen;  /* full available buffer size in WCHARs */
};

static void CRYPT_InitializeKeynameKeeper(struct KeynameKeeper *keeper)
{
    keeper->keyName = keeper->buf;
    keeper->keyLen = ARRAY_SIZE(keeper->buf);
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

    if (len >= keeper->keyLen)
    {
        CRYPT_FreeKeynameKeeper( keeper );
        keeper->keyLen = len + 1;
        keeper->keyName = CryptMemAlloc(keeper->keyLen * sizeof(WCHAR));
    }
    memcpy(keeper->keyName, key->start, len * sizeof(WCHAR));
    keeper->keyName[len] = '\0';
    TRACE("Keyname is %s\n", debugstr_w(keeper->keyName));
}

static BOOL CRYPT_GetNextKeyW(LPCWSTR str, struct X500TokenW *token,
 LPCWSTR *ppszError)
{
    BOOL ret = TRUE;

    while (*str && iswspace(*str))
        str++;
    if (*str)
    {
        token->start = str;
        while (*str && *str != '=' && !iswspace(*str))
            str++;
        if (*str && (*str == '=' || iswspace(*str)))
            token->end = str;
        else
        {
            TRACE("missing equals char at %s\n", debugstr_w(token->start));
            if (ppszError)
                *ppszError = token->start;
            SetLastError(CRYPT_E_INVALID_X500_STRING);
            ret = FALSE;
        }
    }
    else
        token->start = NULL;
    return ret;
}

/* Assumes separators are characters in the 0-255 range */
static BOOL CRYPT_GetNextValueW(LPCWSTR str, DWORD dwFlags, LPCWSTR separators,
 WCHAR *separator_used, struct X500TokenW *token, LPCWSTR *ppszError)
{
    BOOL ret = TRUE;

    TRACE("(%s, %s, %p, %p)\n", debugstr_w(str), debugstr_w(separators), token,
     ppszError);

    *separator_used = 0;
    while (*str && iswspace(*str))
        str++;
    if (*str)
    {
        token->start = str;
        if (!(dwFlags & CERT_NAME_STR_NO_QUOTING_FLAG) && *str == '"')
        {
            token->end = NULL;
            str++;
            while (!token->end && ret)
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
                    SetLastError(CRYPT_E_INVALID_X500_STRING);
                    ret = FALSE;
                }
            }
        }
        else
        {
            WCHAR map[256] = { 0 };

            while (*separators)
                map[*separators++] = 1;
            while (*str && (*str >= 0xff || !map[*str]))
                str++;
            token->end = str;
            if (map[*str]) *separator_used = *str;
        }
    }
    else
    {
        TRACE("missing value at %s\n", debugstr_w(str));
        if (ppszError)
            *ppszError = str;
        SetLastError(CRYPT_E_INVALID_X500_STRING);
        ret = FALSE;
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
    BOOL ret = TRUE;

    if (value->end > value->start)
    {
        LONG i;
        LPWSTR ptr;

        nameValue.Value.pbData = CryptMemAlloc((value->end - value->start + 1) *
         sizeof(WCHAR));
        if (!nameValue.Value.pbData)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        ptr = (LPWSTR)nameValue.Value.pbData;
        for (i = 0; i < value->end - value->start; i++)
        {
            *ptr++ = value->start[i];
            if (value->start[i] == '"')
                i++;
        }
        /* The string is NULL terminated because of a quirk in encoding
         * unicode names values:  if the length is given as 0, the value is
         * assumed to be a NULL-terminated string.
         */
        *ptr = 0;
        nameValue.Value.cbData = (LPBYTE)ptr - nameValue.Value.pbData;
    }
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
 PCCRYPT_OID_INFO keyOID, struct X500TokenW *value, DWORD dwStrType, LPCWSTR *ppszError)
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
            if (value->start[0] == '"' && !(dwStrType & CERT_NAME_STR_NO_QUOTING_FLAG))
            {
                value->start++;
                value->end--;
            }
            ret = CRYPT_EncodeValue(dwCertEncodingType, value,
             &info->rgRDN[info->cRDN].rgRDNAttr[0].Value, types, ppszError);
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
        info->cRDN++;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);
    return ret;
}

BOOL WINAPI CertStrToNameW(DWORD dwCertEncodingType, LPCWSTR pszX500,
 DWORD dwStrType, void *pvReserved, BYTE *pbEncoded, DWORD *pcbEncoded,
 LPCWSTR *ppszError)
{
    CERT_NAME_INFO info = { 0, NULL };
    LPCWSTR str;
    struct KeynameKeeper keeper;
    DWORD i;
    BOOL ret = TRUE;

    TRACE("(%08lx, %s, %08lx, %p, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_w(pszX500), dwStrType, pvReserved, pbEncoded, pcbEncoded,
     ppszError);

    CRYPT_InitializeKeynameKeeper(&keeper);
    str = pszX500;
    while (str && *str && ret)
    {
        struct X500TokenW token;

        ret = CRYPT_GetNextKeyW(str, &token, ppszError);
        if (ret && token.start)
        {
            PCCRYPT_OID_INFO keyOID;

            CRYPT_KeynameKeeperFromTokenW(&keeper, &token);
            keyOID = CryptFindOIDInfo(CRYPT_OID_INFO_NAME_KEY, keeper.keyName,
             CRYPT_RDN_ATTR_OID_GROUP_ID);
            if (!keyOID)
            {
                if (ppszError)
                    *ppszError = token.start;
                SetLastError(CRYPT_E_INVALID_X500_STRING);
                ret = FALSE;
            }
            else
            {
                str = token.end;
                while (iswspace(*str))
                    str++;
                if (*str != '=')
                {
                    if (ppszError)
                        *ppszError = str;
                    SetLastError(CRYPT_E_INVALID_X500_STRING);
                    ret = FALSE;
                }
                else
                {
                    LPCWSTR sep;
                    WCHAR sep_used;

                    str++;
                    if (dwStrType & CERT_NAME_STR_COMMA_FLAG)
                        sep = L",";
                    else if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
                        sep = L";";
                    else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
                        sep = L"\r\n";
                    else if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
                        sep = L",;\r\n";
                    else
                        sep = L"+,;\r\n";
                    ret = CRYPT_GetNextValueW(str, dwStrType, sep, &sep_used, &token,
                     ppszError);
                    if (ret)
                    {
                        str = token.end;
                        /* if token.end points to the separator, skip it */
                        if (str && sep_used && *str == sep_used) str++;

                        ret = CRYPT_ValueToRDN(dwCertEncodingType, &info,
                         keyOID, &token, dwStrType, ppszError);
                    }
                }
            }
        }
    }
    CRYPT_FreeKeynameKeeper(&keeper);
    if (ret)
    {
        if (ppszError)
            *ppszError = NULL;
        ret = CryptEncodeObjectEx(dwCertEncodingType, X509_NAME, &info,
         0, NULL, pbEncoded, pcbEncoded);
    }
    for (i = 0; i < info.cRDN; i++)
    {
        DWORD j;

        for (j = 0; j < info.rgRDN[i].cRDNAttr; j++)
            LocalFree(info.rgRDN[i].rgRDNAttr[j].Value.pbData);
        CryptMemFree(info.rgRDN[i].rgRDNAttr);
    }
    CryptMemFree(info.rgRDN);
    return ret;
}

DWORD WINAPI CertGetNameStringA(PCCERT_CONTEXT cert, DWORD type,
                                DWORD flags, void *type_para, LPSTR name, DWORD name_len)
{
    DWORD len, len_mb, ret;
    LPWSTR nameW;

    TRACE("(%p, %ld, %08lx, %p, %p, %ld)\n", cert, type, flags, type_para, name, name_len);

    len = CertGetNameStringW(cert, type, flags, type_para, NULL, 0);

    if (!(nameW = CryptMemAlloc(len * sizeof(*nameW))))
    {
        ERR("No memory.\n");
        if (name && name_len) *name = 0;
        return 1;
    }

    len = CertGetNameStringW(cert, type, flags, type_para, nameW, len);
    len_mb = WideCharToMultiByte(CP_ACP, 0, nameW, len, NULL, 0, NULL, NULL);
    if (!name || !name_len)
    {
        CryptMemFree(nameW);
        return len_mb;
    }

    ret = WideCharToMultiByte(CP_ACP, 0, nameW, len, name, name_len, NULL, NULL);
    if (ret < len_mb)
    {
        name[0] = 0;
        ret = 1;
    }
    CryptMemFree(nameW);
    return ret;
}

static BOOL cert_get_alt_name_info(PCCERT_CONTEXT cert, BOOL alt_name_issuer, PCERT_ALT_NAME_INFO *info)
{
    static const char *oids[][2] =
    {
        { szOID_SUBJECT_ALT_NAME2, szOID_SUBJECT_ALT_NAME },
        { szOID_ISSUER_ALT_NAME2, szOID_ISSUER_ALT_NAME },
    };
    PCERT_EXTENSION ext;
    DWORD bytes = 0;

    ext = CertFindExtension(oids[!!alt_name_issuer][0], cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);
    if (!ext)
        ext = CertFindExtension(oids[!!alt_name_issuer][1], cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);
    if (!ext) return FALSE;

    return CryptDecodeObjectEx(cert->dwCertEncodingType, X509_ALTERNATE_NAME, ext->Value.pbData, ext->Value.cbData,
                             CRYPT_DECODE_ALLOC_FLAG, NULL, info, &bytes);
}

static PCERT_ALT_NAME_ENTRY cert_find_next_alt_name_entry(PCERT_ALT_NAME_INFO info, DWORD entry_type,
                                                          unsigned int *index)
{
    unsigned int i;

    for (i = *index; i < info->cAltEntry; ++i)
        if (info->rgAltEntry[i].dwAltNameChoice == entry_type)
        {
            *index = i + 1;
            return &info->rgAltEntry[i];
        }
    return NULL;
}

/* Searches cert's extensions for the alternate name extension with OID
 * altNameOID, and if found, searches it for the alternate name type entryType.
 * If found, returns a pointer to the entry, otherwise returns NULL.
 * Regardless of whether an entry of the desired type is found, if the
 * alternate name extension is present, sets *info to the decoded alternate
 * name extension, which you must free using LocalFree.
 * The return value is a pointer within *info, so don't free *info before
 * you're done with the return value.
 */
static PCERT_ALT_NAME_ENTRY cert_find_alt_name_entry(PCCERT_CONTEXT cert, BOOL alt_name_issuer,
                                                     DWORD entry_type, PCERT_ALT_NAME_INFO *info)
{
    unsigned int index = 0;

    if (!cert_get_alt_name_info(cert, alt_name_issuer, info)) return NULL;
    return cert_find_next_alt_name_entry(*info, entry_type, &index);
}

static DWORD cert_get_name_from_rdn_attr(DWORD encodingType,
 const CERT_NAME_BLOB *name, LPCSTR oid, LPWSTR pszNameString, DWORD cchNameString)
{
    CERT_NAME_INFO *nameInfo;
    DWORD bytes = 0, ret = 0;

    if (CryptDecodeObjectEx(encodingType, X509_NAME, name->pbData,
     name->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &nameInfo, &bytes))
    {
        PCERT_RDN_ATTR nameAttr = NULL;

        if (oid)
            nameAttr = CertFindRDNAttr(oid, nameInfo);
        else
        {
            static const LPCSTR attributeOIDs[] =
            {
                szOID_RSA_emailAddr, szOID_COMMON_NAME,
                szOID_ORGANIZATIONAL_UNIT_NAME, szOID_ORGANIZATION_NAME
            };
            DWORD i;

            for (i = 0; !nameAttr && i < ARRAY_SIZE(attributeOIDs); i++)
                nameAttr = CertFindRDNAttr(attributeOIDs[i], nameInfo);
        }
        if (nameAttr)
            ret = rdn_value_to_strW(nameAttr->dwValueType, &nameAttr->Value,
             pszNameString, cchNameString, TRUE);
        LocalFree(nameInfo);
    }
    return ret;
}

static DWORD copy_output_str(WCHAR *dst, const WCHAR *src, DWORD dst_size)
{
    DWORD len = wcslen(src);

    if (!dst || !dst_size) return len + 1;
    len = min(len, dst_size - 1);
    memcpy(dst, src, len * sizeof(*dst));
    dst[len] = 0;
    return len + 1;
}

DWORD WINAPI CertGetNameStringW(PCCERT_CONTEXT cert, DWORD type, DWORD flags, void *type_para,
                                LPWSTR name_string, DWORD name_len)
{
    static const DWORD supported_flags = CERT_NAME_ISSUER_FLAG | CERT_NAME_SEARCH_ALL_NAMES_FLAG;
    BOOL alt_name_issuer, search_all_names;
    CERT_ALT_NAME_INFO *info = NULL;
    PCERT_ALT_NAME_ENTRY entry;
    PCERT_NAME_BLOB name;
    DWORD ret = 0;

    TRACE("(%p, %ld, %08lx, %p, %p, %ld)\n", cert, type, flags, type_para, name_string, name_len);

    if (!cert)
        goto done;

    if (flags & ~supported_flags)
        FIXME("Unsupported flags %#lx.\n", flags);

    search_all_names = flags & CERT_NAME_SEARCH_ALL_NAMES_FLAG;
    if (search_all_names && type != CERT_NAME_DNS_TYPE)
    {
        WARN("CERT_NAME_SEARCH_ALL_NAMES_FLAG used with type %lu.\n", type);
        goto done;
    }

    alt_name_issuer = flags & CERT_NAME_ISSUER_FLAG;
    name = alt_name_issuer ? &cert->pCertInfo->Issuer : &cert->pCertInfo->Subject;

    switch (type)
    {
    case CERT_NAME_EMAIL_TYPE:
    {
        entry = cert_find_alt_name_entry(cert, alt_name_issuer, CERT_ALT_NAME_RFC822_NAME, &info);

        if (entry)
        {
            ret = copy_output_str(name_string, entry->pwszRfc822Name, name_len);
            break;
        }
        ret = cert_get_name_from_rdn_attr(cert->dwCertEncodingType, name, szOID_RSA_emailAddr,
                                          name_string, name_len);
        break;
    }
    case CERT_NAME_RDN_TYPE:
    {
        DWORD param = type_para ? *(DWORD *)type_para : 0;

        if (name->cbData)
        {
            ret = CertNameToStrW(cert->dwCertEncodingType, name, param, name_string, name_len);
        }
        else
        {
            entry = cert_find_alt_name_entry(cert, alt_name_issuer, CERT_ALT_NAME_DIRECTORY_NAME, &info);

            if (entry)
                ret = CertNameToStrW(cert->dwCertEncodingType, &entry->DirectoryName,
                                     param, name_string, name_len);
        }
        break;
    }
    case CERT_NAME_ATTR_TYPE:
        ret = cert_get_name_from_rdn_attr(cert->dwCertEncodingType, name, type_para,
                                          name_string, name_len);
        if (ret) break;

        entry = cert_find_alt_name_entry(cert, alt_name_issuer, CERT_ALT_NAME_DIRECTORY_NAME, &info);

        if (entry)
            ret = cert_name_to_str_with_indent(X509_ASN_ENCODING, 0, &entry->DirectoryName,
                                               0, name_string, name_len);
        break;
    case CERT_NAME_SIMPLE_DISPLAY_TYPE:
    {
        static const LPCSTR simpleAttributeOIDs[] =
        {
            szOID_COMMON_NAME, szOID_ORGANIZATIONAL_UNIT_NAME, szOID_ORGANIZATION_NAME, szOID_RSA_emailAddr
        };
        CERT_NAME_INFO *nameInfo = NULL;
        DWORD bytes = 0, i;

        if (CryptDecodeObjectEx(cert->dwCertEncodingType, X509_NAME, name->pbData, name->cbData,
                                CRYPT_DECODE_ALLOC_FLAG, NULL, &nameInfo, &bytes))
        {
            PCERT_RDN_ATTR nameAttr = NULL;

            for (i = 0; !nameAttr && i < ARRAY_SIZE(simpleAttributeOIDs); i++)
                nameAttr = CertFindRDNAttr(simpleAttributeOIDs[i], nameInfo);
            if (nameAttr)
                ret = rdn_value_to_strW(nameAttr->dwValueType, &nameAttr->Value, name_string, name_len, TRUE);
            LocalFree(nameInfo);
        }
        if (ret) break;
        entry = cert_find_alt_name_entry(cert, alt_name_issuer, CERT_ALT_NAME_RFC822_NAME, &info);
        if (!info) break;
        if (!entry && info->cAltEntry)
            entry = &info->rgAltEntry[0];
        if (entry) ret = copy_output_str(name_string, entry->pwszRfc822Name, name_len);
        break;
    }
    case CERT_NAME_FRIENDLY_DISPLAY_TYPE:
    {
        DWORD len = name_len;

        if (CertGetCertificateContextProperty(cert, CERT_FRIENDLY_NAME_PROP_ID, name_string, &len))
            ret = len;
        else
            ret = CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, flags,
                                     type_para, name_string, name_len);
        break;
    }
    case CERT_NAME_DNS_TYPE:
    {
        unsigned int index = 0, len;

        if (cert_get_alt_name_info(cert, alt_name_issuer, &info)
            && (entry = cert_find_next_alt_name_entry(info, CERT_ALT_NAME_DNS_NAME, &index)))
        {
            if (search_all_names)
            {
                do
                {
                    if (name_string && name_len == 1) break;
                    ret += len = copy_output_str(name_string, entry->pwszDNSName, name_len ? name_len - 1 : 0);
                    if (name_string && name_len)
                    {
                        name_string += len;
                        name_len -= len;
                    }
                }
                while ((entry = cert_find_next_alt_name_entry(info, CERT_ALT_NAME_DNS_NAME, &index)));
            }
            else ret = copy_output_str(name_string, entry->pwszDNSName, name_len);
        }
        else
        {
            if (!search_all_names || name_len != 1)
            {
                len = search_all_names && name_len ? name_len - 1 : name_len;
                ret = cert_get_name_from_rdn_attr(cert->dwCertEncodingType, name, szOID_COMMON_NAME,
                                                  name_string, len);
                if (name_string) name_string += ret;
            }
        }

        if (search_all_names)
        {
            if (name_string && name_len) *name_string = 0;
            ++ret;
        }
        break;
    }
    case CERT_NAME_URL_TYPE:
    {
        if ((entry = cert_find_alt_name_entry(cert, alt_name_issuer, CERT_ALT_NAME_URL, &info)))
            ret = copy_output_str(name_string, entry->pwszURL, name_len);
        break;
    }
    default:
        FIXME("unimplemented for type %lu.\n", type);
        ret = 0;
        break;
    }
done:
    if (info)
        LocalFree(info);

    if (!ret)
    {
        ret = 1;
        if (name_string && name_len) name_string[0] = 0;
    }
    return ret;
}
