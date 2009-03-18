/*
 * Copyright 2008 Juan Lang
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

#include "config.h"
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "mssip.h"
#define COBJMACROS
#include "objbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msisip);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}

static GUID mySubject = { 0x000c10f1, 0x0000, 0x0000,
 { 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 }};

/***********************************************************************
 *              DllRegisterServer (MSISIP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static WCHAR msisip[] = { 'M','S','I','S','I','P','.','D','L','L',0 };
    static WCHAR getSignedDataMsg[] = { 'M','s','i','S','I','P','G','e','t',
     'S','i','g','n','e','d','D','a','t','a','M','s','g',0 };
    static WCHAR putSignedDataMsg[] = { 'M','s','i','S','I','P','P','u','t',
     'S','i','g','n','e','d','D','a','t','a','M','s','g',0 };
    static WCHAR createIndirectData[] = { 'M','s','i','S','I','P',
     'C','r','e','a','t','e','I','n','d','i','r','e','c','t','D','a','t','a',
     0 };
    static WCHAR verifyIndirectData[] = { 'M','s','i','S','I','P',
     'V','e','r','i','f','y','I','n','d','i','r','e','c','t','D','a','t','a',
     0 };
    static WCHAR removeSignedDataMsg[] = { 'M','s','i','S','I','P','R','e','m',
     'o','v','e','S','i','g','n','e','d','D','a','t','a','M','s','g', 0 };
    static WCHAR isMyTypeOfFile[] = { 'M','s','i','S','I','P',
     'I','s','M','y','T','y','p','e','O','f','F','i','l','e',0 };

    SIP_ADD_NEWPROVIDER prov;

    memset(&prov, 0, sizeof(prov));
    prov.cbStruct = sizeof(prov);
    prov.pwszDLLFileName = msisip;
    prov.pgSubject = &mySubject;
    prov.pwszGetFuncName = getSignedDataMsg;
    prov.pwszPutFuncName = putSignedDataMsg;
    prov.pwszCreateFuncName = createIndirectData;
    prov.pwszVerifyFuncName = verifyIndirectData;
    prov.pwszRemoveFuncName = removeSignedDataMsg;
    prov.pwszIsFunctionNameFmt2 = isMyTypeOfFile;
    return CryptSIPAddProvider(&prov) ? S_OK : S_FALSE;
}

/***********************************************************************
 *              DllUnregisterServer (MSISIP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    CryptSIPRemoveProvider(&mySubject);
    return S_OK;
}

/***********************************************************************
 *              MsiSIPGetSignedDataMsg  (MSISIP.@)
 */
BOOL WINAPI MsiSIPGetSignedDataMsg(SIP_SUBJECTINFO *pSubjectInfo,
 DWORD *pdwEncodingType, DWORD dwIndex, DWORD *pcbSignedDataMsg,
 BYTE *pbSignedDataMsg)
{
    static const WCHAR digitalSig[] = { 5,'D','i','g','i','t','a','l',
     'S','i','g','n','a','t','u','r','e',0 };
    BOOL ret = FALSE;
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    BYTE hdr[2], len[sizeof(DWORD)];
    DWORD count, lenBytes, dataBytes;

    TRACE("(%p %p %d %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    r = StgOpenStorage(pSubjectInfo->pwsFileName, NULL,
     STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    if (FAILED(r))
    {
        TRACE("couldn't open %s\n", debugstr_w(pSubjectInfo->pwsFileName));
        goto end;
    }

    r = IStorage_OpenStream(stg, digitalSig, 0,
     STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &stm);
    if (FAILED(r))
    {
        TRACE("couldn't find digital signature stream\n");
        goto freestorage;
    }

    r = IStream_Read(stm, hdr, sizeof(hdr), &count);
    if (FAILED(r) || count != sizeof(hdr))
        goto freestream;
    if (hdr[0] != 0x30)
    {
        WARN("unexpected data in digital sig: 0x%02x%02x\n", hdr[0], hdr[1]);
        goto freestream;
    }

    /* Read the asn.1 length from the stream.  Only supports definite-length
     * values, which DER-encoded signatures should be.
     */
    if (hdr[1] == 0x80)
    {
        WARN("indefinite-length encoding not supported!\n");
        goto freestream;
    }
    else if (hdr[1] & 0x80)
    {
        DWORD temp;
        LPBYTE ptr;

        lenBytes = hdr[1] & 0x7f;
        if (lenBytes > sizeof(DWORD))
        {
            WARN("asn.1 length too long (%d)\n", lenBytes);
            goto freestream;
        }
        r = IStream_Read(stm, len, lenBytes, &count);
        if (FAILED(r) || count != lenBytes)
            goto freestream;
        dataBytes = 0;
        temp = lenBytes;
        ptr = len;
        while (temp--)
        {
            dataBytes <<= 8;
            dataBytes |= *ptr++;
        }
    }
    else
    {
        lenBytes = 0;
        dataBytes = hdr[1];
    }

    if (!pbSignedDataMsg)
    {
        *pcbSignedDataMsg = 2 + lenBytes + dataBytes;
        ret = TRUE;
    }
    else if (*pcbSignedDataMsg < 2 + lenBytes + dataBytes)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *pcbSignedDataMsg = 2 + lenBytes + dataBytes;
    }
    else
    {
        LPBYTE ptr = pbSignedDataMsg;

        memcpy(ptr, hdr, sizeof(hdr));
        ptr += sizeof(hdr);
        if (lenBytes)
        {
            memcpy(ptr, len, lenBytes);
            ptr += lenBytes;
        }
        r = IStream_Read(stm, ptr, dataBytes, &count);
        if (SUCCEEDED(r) && count == dataBytes)
        {
            *pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
            *pcbSignedDataMsg = 2 + lenBytes + dataBytes;
            ret = TRUE;
        }
    }

freestream:
    IStream_Release(stm);
freestorage:
    IStorage_Release(stg);
end:

    TRACE("returning %d\n", ret);
    return ret;
}

/***********************************************************************
 *              MsiSIPIsMyTypeOfFile (MSISIP.@)
 */
BOOL WINAPI MsiSIPIsMyTypeOfFile(WCHAR *name, GUID *subject)
{
    static const WCHAR msi[] = { '.','m','s','i',0 };
    static const WCHAR msp[] = { '.','m','s','p',0 };
    BOOL ret = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(name), subject);

    if (lstrlenW(name) < lstrlenW(msi))
        return FALSE;
    else if (lstrcmpiW(name + lstrlenW(name) - lstrlenW(msi), msi) &&
     lstrcmpiW(name + lstrlenW(name) - lstrlenW(msp), msp))
        return FALSE;
    else
    {
        IStorage *stg = NULL;
        HRESULT r = StgOpenStorage(name, NULL,
         STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);

        if (SUCCEEDED(r))
        {
            IStorage_Release(stg);
            *subject = mySubject;
            ret = TRUE;
        }
    }
    return ret;
}
