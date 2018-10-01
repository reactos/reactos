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

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wincrypt.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

DWORD WINAPI CertRDNValueToStrA(DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue,
 LPSTR psz, DWORD csz)
{
    DWORD ret = 0, len;

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
        len = pValue->cbData;
        if (!psz || !csz)
            ret = len;
        else
        {
            DWORD chars = min(len, csz - 1);

            if (chars)
            {
                memcpy(psz, pValue->pbData, chars);
                ret += chars;
                csz -= chars;
            }
        }
        break;
    case CERT_RDN_BMP_STRING:
    case CERT_RDN_UTF8_STRING:
        len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pValue->pbData,
         pValue->cbData / sizeof(WCHAR), NULL, 0, NULL, NULL);
        if (!psz || !csz)
            ret = len;
        else
        {
            DWORD chars = min(pValue->cbData / sizeof(WCHAR), csz - 1);

            if (chars)
            {
                ret = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pValue->pbData,
                 chars, psz, csz - 1, NULL, NULL);
                csz -= ret;
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
    DWORD ret = 0, len, i, strLen;

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
        len = pValue->cbData;
        if (!psz || !csz)
            ret = len;
        else
        {
            WCHAR *ptr = psz;

            for (i = 0; i < pValue->cbData && ptr - psz < csz; ptr++, i++)
                *ptr = pValue->pbData[i];
            ret = ptr - psz;
        }
        break;
    case CERT_RDN_BMP_STRING:
    case CERT_RDN_UTF8_STRING:
        strLen = len = pValue->cbData / sizeof(WCHAR);
        if (!psz || !csz)
            ret = len;
        else
        {
            WCHAR *ptr = psz;

            for (i = 0; i < strLen && ptr - psz < csz; ptr++, i++)
                *ptr = ((LPCWSTR)pValue->pbData)[i];
            ret = ptr - psz;
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

static inline BOOL is_quotable_char(char c)
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

static DWORD quote_rdn_value_to_str_a(DWORD dwValueType,
 PCERT_RDN_VALUE_BLOB pValue, LPSTR psz, DWORD csz)
{
    DWORD ret = 0, len, i;
    BOOL needsQuotes = FALSE;

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
        len = pValue->cbData;
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
        if (!psz || !csz)
            ret = len;
        else
        {
            char *ptr = psz;

            if (needsQuotes)
                *ptr++ = '"';
            for (i = 0; i < pValue->cbData && ptr - psz < csz; ptr++, i++)
            {
                *ptr = pValue->pbData[i];
                if (pValue->pbData[i] == '"' && ptr - psz < csz - 1)
                    *(++ptr) = '"';
            }
            if (needsQuotes && ptr - psz < csz)
                *ptr++ = '"';
            ret = ptr - psz;
        }
        break;
    case CERT_RDN_BMP_STRING:
    case CERT_RDN_UTF8_STRING:
        len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pValue->pbData,
         pValue->cbData / sizeof(WCHAR), NULL, 0, NULL, NULL);
        if (pValue->cbData && isspaceW(((LPCWSTR)pValue->pbData)[0]))
            needsQuotes = TRUE;
        if (pValue->cbData &&
         isspaceW(((LPCWSTR)pValue->pbData)[pValue->cbData / sizeof(WCHAR)-1]))
            needsQuotes = TRUE;
        for (i = 0; i < pValue->cbData / sizeof(WCHAR); i++)
        {
            if (is_quotable_char(((LPCWSTR)pValue->pbData)[i]))
                needsQuotes = TRUE;
            if (((LPCWSTR)pValue->pbData)[i] == '"')
                len += 1;
        }
        if (needsQuotes)
            len += 2;
        if (!psz || !csz)
            ret = len;
        else
        {
            char *dst = psz;

            if (needsQuotes)
                *dst++ = '"';
            for (i = 0; i < pValue->cbData / sizeof(WCHAR) &&
             dst - psz < csz; dst++, i++)
            {
                LPCWSTR src = (LPCWSTR)pValue->pbData + i;

                WideCharToMultiByte(CP_ACP, 0, src, 1, dst,
                 csz - (dst - psz) - 1, NULL, NULL);
                if (*src == '"' && dst - psz < csz - 1)
                    *(++dst) = '"';
            }
            if (needsQuotes && dst - psz < csz)
                *dst++ = '"';
            ret = dst - psz;
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

static DWORD quote_rdn_value_to_str_w(DWORD dwValueType,
 PCERT_RDN_VALUE_BLOB pValue, LPWSTR psz, DWORD csz)
{
    DWORD ret = 0, len, i, strLen;
    BOOL needsQuotes = FALSE;

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
        len = pValue->cbData;
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
                if (pValue->pbData[i] == '"' && ptr - psz < csz - 1)
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
        if (pValue->cbData && isspace(pValue->pbData[0]))
            needsQuotes = TRUE;
        if (pValue->cbData && isspace(pValue->pbData[strLen - 1]))
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
                if (((LPCWSTR)pValue->pbData)[i] == '"' && ptr - psz < csz - 1)
                    *(++ptr) = '"';
            }
            if (needsQuotes && ptr - psz < csz)
                *ptr++ = '"';
            ret = ptr - psz;
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
        chars = min(strlen(prefix), csz);
        memcpy(psz, prefix, chars);
        *(psz + chars) = '=';
        chars++;
    }
    else
        chars = lstrlenA(prefix) + 1;
    return chars;
}

DWORD WINAPI CertNameToStrA(DWORD dwCertEncodingType, PCERT_NAME_BLOB pName,
 DWORD dwStrType, LPSTR psz, DWORD csz)
{
    static const DWORD unsupportedFlags = CERT_NAME_STR_NO_QUOTING_FLAG |
     CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
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
        BOOL reverse = dwStrType & CERT_NAME_STR_REVERSE_FLAG;
        const CERT_RDN *rdn = info->rgRDN;

        if(reverse && info->cRDN > 1) rdn += (info->cRDN - 1);

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
            for (j = 0; (!psz || ret < csz) && j < rdn->cRDNAttr; j++)
            {
                DWORD chars;
                char prefixBuf[13]; /* big enough for SERIALNUMBER */
                LPCSTR prefix = NULL;

                if ((dwStrType & 0x000000ff) == CERT_OID_NAME_STR)
                    prefix = rdn->rgRDNAttr[j].pszObjId;
                else if ((dwStrType & 0x000000ff) == CERT_X500_NAME_STR)
                {
                    PCCRYPT_OID_INFO oidInfo = CryptFindOIDInfo(
                     CRYPT_OID_INFO_OID_KEY,
                     rdn->rgRDNAttr[j].pszObjId,
                     CRYPT_RDN_ATTR_OID_GROUP_ID);

                    if (oidInfo)
                    {
                        WideCharToMultiByte(CP_ACP, 0, oidInfo->pwszName, -1,
                         prefixBuf, sizeof(prefixBuf), NULL, NULL);
                        prefix = prefixBuf;
                    }
                    else
                        prefix = rdn->rgRDNAttr[j].pszObjId;
                }
                if (prefix)
                {
                    /* - 1 is needed to account for the NULL terminator. */
                    chars = CRYPT_AddPrefixA(prefix,
                     psz ? psz + ret : NULL, psz ? csz - ret - 1 : 0);
                    ret += chars;
                }
                chars = quote_rdn_value_to_str_a(
                 rdn->rgRDNAttr[j].dwValueType,
                 &rdn->rgRDNAttr[j].Value, psz ? psz + ret : NULL,
                 psz ? csz - ret : 0);
                if (chars)
                    ret += chars - 1;
                if (j < rdn->cRDNAttr - 1)
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
            if(reverse) rdn--;
            else rdn++;
        }
        LocalFree(info);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
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

    TRACE("(%s, %p, %d)\n", debugstr_w(prefix), psz, csz);

    if (psz)
    {
        chars = min(strlenW(prefix), csz);
        memcpy(psz, prefix, chars * sizeof(WCHAR));
        *(psz + chars) = '=';
        chars++;
    }
    else
        chars = lstrlenW(prefix) + 1;
    return chars;
}

static const WCHAR indent[] = { ' ',' ',' ',' ',' ',0 };

DWORD cert_name_to_str_with_indent(DWORD dwCertEncodingType, DWORD indentLevel,
 const CERT_NAME_BLOB *pName, DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    static const DWORD unsupportedFlags = CERT_NAME_STR_NO_QUOTING_FLAG |
     CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG;
    static const WCHAR commaSep[] = { ',',' ',0 };
    static const WCHAR semiSep[] = { ';',' ',0 };
    static const WCHAR crlfSep[] = { '\r','\n',0 };
    static const WCHAR plusSep[] = { ' ','+',' ',0 };
    static const WCHAR spaceSep[] = { ' ',0 };
    DWORD ret = 0, bytes = 0;
    BOOL bRet;
    CERT_NAME_INFO *info;

    if (dwStrType & unsupportedFlags)
        FIXME("unsupported flags: %08x\n", dwStrType & unsupportedFlags);

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
            for (j = 0; (!psz || ret < csz) && j < rdn->cRDNAttr; j++)
            {
                DWORD chars;
                LPCSTR prefixA = NULL;
                LPCWSTR prefixW = NULL;

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
                            chars = min(strlenW(indent), csz - ret - 1);
                            memcpy(psz + ret, indent, chars * sizeof(WCHAR));
                        }
                        else
                            chars = strlenW(indent);
                        ret += chars;
                    }
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
                chars = quote_rdn_value_to_str_w(
                 rdn->rgRDNAttr[j].dwValueType,
                 &rdn->rgRDNAttr[j].Value, psz ? psz + ret : NULL,
                 psz ? csz - ret : 0);
                if (chars)
                    ret += chars - 1;
                if (j < rdn->cRDNAttr - 1)
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
            if(reverse) rdn--;
            else rdn++;
        }
        LocalFree(info);
    }
    if (psz && csz)
    {
        *(psz + ret) = '\0';
        ret++;
    }
    else
        ret++;
    return ret;
}

DWORD WINAPI CertNameToStrW(DWORD dwCertEncodingType, PCERT_NAME_BLOB pName,
 DWORD dwStrType, LPWSTR psz, DWORD csz)
{
    BOOL ret;

    TRACE("(%d, %p, %08x, %p, %d)\n", dwCertEncodingType, pName, dwStrType,
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

    TRACE("(%08x, %s, %08x, %p, %p, %p, %p)\n", dwCertEncodingType,
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
    while (*str && isspaceW(*str))
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

    TRACE("(%08x, %s, %08x, %p, %p, %p, %p)\n", dwCertEncodingType,
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
                while (isspaceW(*str))
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
                    static const WCHAR commaSep[] = { ',',0 };
                    static const WCHAR semiSep[] = { ';',0 };
                    static const WCHAR crlfSep[] = { '\r','\n',0 };
                    static const WCHAR allSepsWithoutPlus[] = { ',',';','\r','\n',0 };
                    static const WCHAR allSeps[] = { '+',',',';','\r','\n',0 };
                    LPCWSTR sep;
                    WCHAR sep_used;

                    str++;
                    if (dwStrType & CERT_NAME_STR_COMMA_FLAG)
                        sep = commaSep;
                    else if (dwStrType & CERT_NAME_STR_SEMICOLON_FLAG)
                        sep = semiSep;
                    else if (dwStrType & CERT_NAME_STR_CRLF_FLAG)
                        sep = crlfSep;
                    else if (dwStrType & CERT_NAME_STR_NO_PLUS_FLAG)
                        sep = allSepsWithoutPlus;
                    else
                        sep = allSeps;
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

/* Searches cert's extensions for the alternate name extension with OID
 * altNameOID, and if found, searches it for the alternate name type entryType.
 * If found, returns a pointer to the entry, otherwise returns NULL.
 * Regardless of whether an entry of the desired type is found, if the
 * alternate name extension is present, sets *info to the decoded alternate
 * name extension, which you must free using LocalFree.
 * The return value is a pointer within *info, so don't free *info before
 * you're done with the return value.
 */
static PCERT_ALT_NAME_ENTRY cert_find_alt_name_entry(PCCERT_CONTEXT cert,
 LPCSTR altNameOID, DWORD entryType, PCERT_ALT_NAME_INFO *info)
{
    PCERT_ALT_NAME_ENTRY entry = NULL;
    PCERT_EXTENSION ext = CertFindExtension(altNameOID,
     cert->pCertInfo->cExtension, cert->pCertInfo->rgExtension);

    if (ext)
    {
        DWORD bytes = 0;

        if (CryptDecodeObjectEx(cert->dwCertEncodingType, X509_ALTERNATE_NAME,
         ext->Value.pbData, ext->Value.cbData, CRYPT_DECODE_ALLOC_FLAG, NULL,
         info, &bytes))
        {
            DWORD i;

            for (i = 0; !entry && i < (*info)->cAltEntry; i++)
                if ((*info)->rgAltEntry[i].dwAltNameChoice == entryType)
                    entry = &(*info)->rgAltEntry[i];
        }
    }
    else
        *info = NULL;
    return entry;
}

static DWORD cert_get_name_from_rdn_attr(DWORD encodingType,
 const CERT_NAME_BLOB *name, LPCSTR oid, LPWSTR pszNameString, DWORD cchNameString)
{
    CERT_NAME_INFO *nameInfo;
    DWORD bytes = 0, ret = 0;

    if (CryptDecodeObjectEx(encodingType, X509_NAME, name->pbData,
     name->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &nameInfo, &bytes))
    {
        PCERT_RDN_ATTR nameAttr;

        if (!oid)
            oid = szOID_RSA_emailAddr;
        nameAttr = CertFindRDNAttr(oid, nameInfo);
        if (nameAttr)
            ret = CertRDNValueToStrW(nameAttr->dwValueType, &nameAttr->Value,
             pszNameString, cchNameString);
        LocalFree(nameInfo);
    }
    return ret;
}

DWORD WINAPI CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType,
 DWORD dwFlags, void *pvTypePara, LPWSTR pszNameString, DWORD cchNameString)
{
    DWORD ret = 0;
    PCERT_NAME_BLOB name;
    LPCSTR altNameOID;

    TRACE("(%p, %d, %08x, %p, %p, %d)\n", pCertContext, dwType,
     dwFlags, pvTypePara, pszNameString, cchNameString);

    if (!pCertContext)
        goto done;

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
    case CERT_NAME_EMAIL_TYPE:
    {
        CERT_ALT_NAME_INFO *info;
        PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
         altNameOID, CERT_ALT_NAME_RFC822_NAME, &info);

        if (entry)
        {
            if (!pszNameString)
                ret = strlenW(entry->u.pwszRfc822Name) + 1;
            else if (cchNameString)
            {
                ret = min(strlenW(entry->u.pwszRfc822Name), cchNameString - 1);
                memcpy(pszNameString, entry->u.pwszRfc822Name,
                 ret * sizeof(WCHAR));
                pszNameString[ret++] = 0;
            }
        }
        if (info)
            LocalFree(info);
        if (!ret)
            ret = cert_get_name_from_rdn_attr(pCertContext->dwCertEncodingType,
             name, szOID_RSA_emailAddr, pszNameString, cchNameString);
        break;
    }
    case CERT_NAME_RDN_TYPE:
    {
        DWORD type = pvTypePara ? *(DWORD *)pvTypePara : 0;

        if (name->cbData)
            ret = CertNameToStrW(pCertContext->dwCertEncodingType, name,
             type, pszNameString, cchNameString);
        else
        {
            CERT_ALT_NAME_INFO *info;
            PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
             altNameOID, CERT_ALT_NAME_DIRECTORY_NAME, &info);

            if (entry)
                ret = CertNameToStrW(pCertContext->dwCertEncodingType,
                 &entry->u.DirectoryName, type, pszNameString, cchNameString);
            if (info)
                LocalFree(info);
        }
        break;
    }
    case CERT_NAME_ATTR_TYPE:
        ret = cert_get_name_from_rdn_attr(pCertContext->dwCertEncodingType,
         name, pvTypePara, pszNameString, cchNameString);
        if (!ret)
        {
            CERT_ALT_NAME_INFO *altInfo;
            PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
             altNameOID, CERT_ALT_NAME_DIRECTORY_NAME, &altInfo);

            if (entry)
                ret = cert_name_to_str_with_indent(X509_ASN_ENCODING, 0,
                 &entry->u.DirectoryName, 0, pszNameString, cchNameString);
            if (altInfo)
                LocalFree(altInfo);
        }
        break;
    case CERT_NAME_SIMPLE_DISPLAY_TYPE:
    {
        static const LPCSTR simpleAttributeOIDs[] = { szOID_COMMON_NAME,
         szOID_ORGANIZATIONAL_UNIT_NAME, szOID_ORGANIZATION_NAME,
         szOID_RSA_emailAddr };
        CERT_NAME_INFO *nameInfo = NULL;
        DWORD bytes = 0, i;

        if (CryptDecodeObjectEx(pCertContext->dwCertEncodingType, X509_NAME,
         name->pbData, name->cbData, CRYPT_DECODE_ALLOC_FLAG, NULL, &nameInfo,
         &bytes))
        {
            PCERT_RDN_ATTR nameAttr = NULL;

            for (i = 0; !nameAttr && i < ARRAY_SIZE(simpleAttributeOIDs); i++)
                nameAttr = CertFindRDNAttr(simpleAttributeOIDs[i], nameInfo);
            if (nameAttr)
                ret = CertRDNValueToStrW(nameAttr->dwValueType,
                 &nameAttr->Value, pszNameString, cchNameString);
            LocalFree(nameInfo);
        }
        if (!ret)
        {
            CERT_ALT_NAME_INFO *altInfo;
            PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
             altNameOID, CERT_ALT_NAME_RFC822_NAME, &altInfo);

            if (altInfo)
            {
                if (!entry && altInfo->cAltEntry)
                    entry = &altInfo->rgAltEntry[0];
                if (entry)
                {
                    if (!pszNameString)
                        ret = strlenW(entry->u.pwszRfc822Name) + 1;
                    else if (cchNameString)
                    {
                        ret = min(strlenW(entry->u.pwszRfc822Name),
                         cchNameString - 1);
                        memcpy(pszNameString, entry->u.pwszRfc822Name,
                         ret * sizeof(WCHAR));
                        pszNameString[ret++] = 0;
                    }
                }
                LocalFree(altInfo);
            }
        }
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
    case CERT_NAME_DNS_TYPE:
    {
        CERT_ALT_NAME_INFO *info;
        PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
         altNameOID, CERT_ALT_NAME_DNS_NAME, &info);

        if (entry)
        {
            if (!pszNameString)
                ret = strlenW(entry->u.pwszDNSName) + 1;
            else if (cchNameString)
            {
                ret = min(strlenW(entry->u.pwszDNSName), cchNameString - 1);
                memcpy(pszNameString, entry->u.pwszDNSName, ret * sizeof(WCHAR));
                pszNameString[ret++] = 0;
            }
        }
        if (info)
            LocalFree(info);
        if (!ret)
            ret = cert_get_name_from_rdn_attr(pCertContext->dwCertEncodingType,
             name, szOID_COMMON_NAME, pszNameString, cchNameString);
        break;
    }
    case CERT_NAME_URL_TYPE:
    {
        CERT_ALT_NAME_INFO *info;
        PCERT_ALT_NAME_ENTRY entry = cert_find_alt_name_entry(pCertContext,
         altNameOID, CERT_ALT_NAME_URL, &info);

        if (entry)
        {
            if (!pszNameString)
                ret = strlenW(entry->u.pwszURL) + 1;
            else if (cchNameString)
            {
                ret = min(strlenW(entry->u.pwszURL), cchNameString - 1);
                memcpy(pszNameString, entry->u.pwszURL, ret * sizeof(WCHAR));
                pszNameString[ret++] = 0;
            }
        }
        if (info)
            LocalFree(info);
        break;
    }
    default:
        FIXME("unimplemented for type %d\n", dwType);
        ret = 0;
    }
done:
    if (!ret)
    {
        if (!pszNameString)
            ret = 1;
        else if (cchNameString)
        {
            pszNameString[0] = 0;
            ret = 1;
        }
    }
    return ret;
}
