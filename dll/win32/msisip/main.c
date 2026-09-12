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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "mssip.h"
#define COBJMACROS
#include "objbase.h"
#include "initguid.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msisip);

static GUID mySubject = { 0x000c10f1, 0x0000, 0x0000,
 { 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46 }};

/***********************************************************************
 *              DllRegisterServer (MSISIP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static WCHAR msisip[] = L"MSISIP.DLL";
    static WCHAR getSignedDataMsg[] = L"MsiSIPGetSignedDataMsg";
    static WCHAR putSignedDataMsg[] = L"MsiSIPPutSignedDataMsg";
    static WCHAR createIndirectData[] = L"MsiSIPCreateIndirectData";
    static WCHAR verifyIndirectData[] = L"MsiSIPVerifyIndirectData";
    static WCHAR removeSignedDataMsg[] = L"MsiSIPRemoveSignedDataMsg";
    static WCHAR isMyTypeOfFile[] = L"MsiSIPIsMyTypeOfFile";

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
    prov.pwszGetCapFuncName = NULL;
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
    BOOL ret = FALSE;
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    BYTE hdr[2], len[sizeof(DWORD)];
    DWORD count, lenBytes, dataBytes;

    TRACE("(%p %p %ld %p %p)\n", pSubjectInfo, pdwEncodingType, dwIndex,
          pcbSignedDataMsg, pbSignedDataMsg);

    r = StgOpenStorage(pSubjectInfo->pwsFileName, NULL,
     STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    if (FAILED(r))
    {
        TRACE("couldn't open %s\n", debugstr_w(pSubjectInfo->pwsFileName));
        goto end;
    }

    r = IStorage_OpenStream(stg, L"\5DigitalSignature", 0,
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
            WARN("asn.1 length too long (%ld)\n", lenBytes);
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

DEFINE_GUID(CLSID_MsiTransform, 0x000c1082,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_MsiDatabase,  0x000c1084,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_MsiPatch,     0x000c1086,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

/***********************************************************************
 *              MsiSIPIsMyTypeOfFile (MSISIP.@)
 */
BOOL WINAPI MsiSIPIsMyTypeOfFile(WCHAR *name, GUID *subject)
{
    BOOL ret = FALSE;
    IStorage *stg = NULL;
    HRESULT r;

    TRACE("(%s, %p)\n", debugstr_w(name), subject);

    r = StgOpenStorage(name, NULL, STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE,
     NULL, 0, &stg);
    if (SUCCEEDED(r))
    {
        STATSTG stat;

        r = IStorage_Stat(stg, &stat, STATFLAG_NONAME);
        if (SUCCEEDED(r))
        {
            if (IsEqualGUID(&stat.clsid, &CLSID_MsiDatabase) ||
             IsEqualGUID(&stat.clsid, &CLSID_MsiPatch) ||
             IsEqualGUID(&stat.clsid, &CLSID_MsiTransform))
            {
                ret = TRUE;
                *subject = mySubject;
            }
        }
        IStorage_Release(stg);
    }
    return ret;
}
