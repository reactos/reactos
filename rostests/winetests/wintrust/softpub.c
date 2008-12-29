/*
 * wintrust softpub functions tests
 *
 * Copyright 2007 Juan Lang
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
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wintrust.h>
#include <softpub.h>
#include <mssip.h>
#include <winuser.h>
#include "winnls.h"

#include "wine/test.h"

/* Just in case we're being built with borked headers, redefine function
 * pointers to have the correct calling convention.
 */
typedef void   *(WINAPI *SAFE_MEM_ALLOC)(DWORD);
typedef void    (WINAPI *SAFE_MEM_FREE)(void *);
typedef BOOL    (WINAPI *SAFE_ADD_STORE)(CRYPT_PROVIDER_DATA *,
 HCERTSTORE);
typedef BOOL    (WINAPI *SAFE_ADD_SGNR)(CRYPT_PROVIDER_DATA *,
 BOOL, DWORD, struct _CRYPT_PROVIDER_SGNR *);
typedef BOOL    (WINAPI *SAFE_ADD_CERT)(CRYPT_PROVIDER_DATA *,
 DWORD, BOOL, DWORD, PCCERT_CONTEXT);
typedef BOOL    (WINAPI *SAFE_ADD_PRIVDATA)(CRYPT_PROVIDER_DATA *,
 CRYPT_PROVIDER_PRIVDATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_INIT_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_OBJTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_SIGTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_CERTTRUST_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_FINALPOLICY_CALL)(CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_TESTFINALPOLICY_CALL)(
 CRYPT_PROVIDER_DATA *);
typedef HRESULT (WINAPI *SAFE_PROVIDER_CLEANUP_CALL)(CRYPT_PROVIDER_DATA *);
typedef BOOL    (WINAPI *SAFE_PROVIDER_CERTCHKPOLICY_CALL)(
 CRYPT_PROVIDER_DATA *, DWORD, BOOL, DWORD);

typedef struct _SAFE_PROVIDER_FUNCTIONS
{
    DWORD                              cbStruct;
    SAFE_MEM_ALLOC                     pfnAlloc;
    SAFE_MEM_FREE                      pfnFree;
    SAFE_ADD_STORE                     pfnAddStore2Chain;
    SAFE_ADD_SGNR                      pfnAddSgnr2Chain;
    SAFE_ADD_CERT                      pfnAddCert2Chain;
    SAFE_ADD_PRIVDATA                  pfnAddPrivData2Chain;
    SAFE_PROVIDER_INIT_CALL            pfnInitialize;
    SAFE_PROVIDER_OBJTRUST_CALL        pfnObjectTrust;
    SAFE_PROVIDER_SIGTRUST_CALL        pfnSignatureTrust;
    SAFE_PROVIDER_CERTTRUST_CALL       pfnCertificateTrust;
    SAFE_PROVIDER_FINALPOLICY_CALL     pfnFinalPolicy;
    SAFE_PROVIDER_CERTCHKPOLICY_CALL   pfnCertCheckPolicy;
    SAFE_PROVIDER_TESTFINALPOLICY_CALL pfnTestFinalPolicy;
    struct _CRYPT_PROVUI_FUNCS        *psUIpfns;
    SAFE_PROVIDER_CLEANUP_CALL         pfnCleanupPolicy;
} SAFE_PROVIDER_FUNCTIONS;

static const BYTE v1CertWithPubKey[] = {
0x30,0x81,0x95,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x22,0x30,0x0d,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x01,0x01,0x05,0x00,0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,
0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,
0x01,0x01 };

static void test_utils(SAFE_PROVIDER_FUNCTIONS *funcs)
{
    CRYPT_PROVIDER_DATA data = { 0 };
    HCERTSTORE store;
    CRYPT_PROVIDER_SGNR sgnr = { 0 };
    BOOL ret;

    /* Crash
    ret = funcs->pfnAddStore2Chain(NULL, NULL);
    ret = funcs->pfnAddStore2Chain(&data, NULL);
     */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (store)
    {
        ret = funcs->pfnAddStore2Chain(&data, store);
        ok(ret, "pfnAddStore2Chain failed: %08x\n", GetLastError());
        ok(data.chStores == 1, "Expected 1 store, got %d\n", data.chStores);
        ok(data.pahStores != NULL, "Expected pahStores to be allocated\n");
        if (data.pahStores)
        {
            ok(data.pahStores[0] == store, "Unexpected store\n");
            CertCloseStore(data.pahStores[0], 0);
            funcs->pfnFree(data.pahStores);
            data.pahStores = NULL;
            data.chStores = 0;
            CertCloseStore(store, 0);
            store = NULL;
        }
    }
    else
        skip("CertOpenStore failed: %08x\n", GetLastError());

    /* Crash
    ret = funcs->pfnAddSgnr2Chain(NULL, FALSE, 0, NULL);
    ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, NULL);
     */
    ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
    ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
    ok(data.csSigners == 1, "Expected 1 signer, got %d\n", data.csSigners);
    ok(data.pasSigners != NULL, "Expected pasSigners to be allocated\n");
    if (data.pasSigners)
    {
        PCCERT_CONTEXT cert;

        ok(!memcmp(&data.pasSigners[0], &sgnr, sizeof(sgnr)),
         "Unexpected data in signer\n");
        /* Adds into the location specified by the index */
        sgnr.cbStruct = sizeof(CRYPT_PROVIDER_SGNR);
        sgnr.sftVerifyAsOf.dwLowDateTime = 0xdeadbeef;
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 1, &sgnr);
        ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
        ok(data.csSigners == 2, "Expected 2 signers, got %d\n", data.csSigners);
        ok(!memcmp(&data.pasSigners[1], &sgnr, sizeof(sgnr)),
         "Unexpected data in signer\n");
        /* This also adds, but the data aren't copied */
        sgnr.cbStruct = sizeof(DWORD);
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
        ok(ret, "pfnAddSgnr2Chain failed: %08x\n", GetLastError());
        ok(data.csSigners == 3, "Expected 3 signers, got %d\n", data.csSigners);
        ok(data.pasSigners[0].cbStruct == 0, "Unexpected data size %d\n",
         data.pasSigners[0].cbStruct);
        ok(data.pasSigners[0].sftVerifyAsOf.dwLowDateTime == 0,
         "Unexpected verify time %d\n",
         data.pasSigners[0].sftVerifyAsOf.dwLowDateTime);
        /* But too large a thing isn't added */
        sgnr.cbStruct = sizeof(sgnr) + sizeof(DWORD);
        SetLastError(0xdeadbeef);
        ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

        /* Crash
        ret = funcs->pfnAddCert2Chain(NULL, 0, FALSE, 0, NULL);
        ret = funcs->pfnAddCert2Chain(&data, 0, FALSE, 0, NULL);
         */
        cert = CertCreateCertificateContext(X509_ASN_ENCODING, v1CertWithPubKey,
         sizeof(v1CertWithPubKey));
        if (cert)
        {
            /* Notes on behavior that are hard to test:
             * 1. If pasSigners is invalid, pfnAddCert2Chain crashes
             * 2. An invalid signer index isn't checked.
             */
            ret = funcs->pfnAddCert2Chain(&data, 0, FALSE, 0, cert);
            ok(ret, "pfnAddCert2Chain failed: %08x\n", GetLastError());
            ok(data.pasSigners[0].csCertChain == 1, "Expected 1 cert, got %d\n",
             data.pasSigners[0].csCertChain);
            ok(data.pasSigners[0].pasCertChain != NULL,
             "Expected pasCertChain to be allocated\n");
            if (data.pasSigners[0].pasCertChain)
                ok(data.pasSigners[0].pasCertChain[0].pCert == cert,
                 "Unexpected cert\n");
            CertFreeCertificateContext(cert);
        }
        else
            skip("CertCreateCertificateContext failed: %08x\n", GetLastError());
    }
}

static void testInitialize(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    HRESULT ret;
    CRYPT_PROVIDER_DATA data = { 0 };
    WINTRUST_DATA wintrust_data = { 0 };

    if (!funcs->pfnInitialize)
    {
        skip("missing pfnInitialize\n");
        return;
    }

    /* Crashes
    ret = funcs->pfnInitialize(NULL);
     */
    memset(&data, 0, sizeof(data));
    ret = funcs->pfnInitialize(&data);
    ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
    data.padwTrustStepErrors =
     funcs->pfnAlloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    /* Without wintrust data set, crashes when padwTrustStepErrors is set */
    data.pWintrustData = &wintrust_data;
    if (data.padwTrustStepErrors)
    {
        /* Apparently, cdwTrustStepErrors does not need to be set. */
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        data.cdwTrustStepErrors = 1;
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        memset(data.padwTrustStepErrors, 0xba,
         TRUSTERROR_MAX_STEPS * sizeof(DWORD));
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] = 0;
        ret = funcs->pfnInitialize(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        funcs->pfnFree(data.padwTrustStepErrors);
    }
}

static void getNotepadPath(WCHAR *notepadPathW, DWORD size)
{
    static const CHAR notepad[] = "\\notepad.exe";
    CHAR notepadPath[MAX_PATH];

    /* Workaround missing W-functions for win9x */
    GetWindowsDirectoryA(notepadPath, MAX_PATH);
    lstrcatA(notepadPath, notepad);
    MultiByteToWideChar(0, 0, notepadPath, -1, notepadPathW, size);
}

static void testObjTrust(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    HRESULT ret;
    CRYPT_PROVIDER_DATA data = { 0 };
    WINTRUST_DATA wintrust_data = { 0 };
    WINTRUST_CERT_INFO certInfo = { sizeof(WINTRUST_CERT_INFO), 0 };
    WINTRUST_FILE_INFO fileInfo = { sizeof(WINTRUST_FILE_INFO), 0 };

    if (!funcs->pfnObjectTrust)
    {
        skip("missing pfnObjectTrust\n");
        return;
    }

    /* Crashes
    ret = funcs->pfnObjectTrust(NULL);
     */
    data.pWintrustData = &wintrust_data;
    data.padwTrustStepErrors =
     funcs->pfnAlloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    if (data.padwTrustStepErrors)
    {
        WCHAR notepadPathW[MAX_PATH];
        PROVDATA_SIP provDataSIP = { 0 };
        static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
         0x00,0xC0,0x4F,0xC2,0x95,0xEE } };

        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        U(wintrust_data).pCert = &certInfo;
        wintrust_data.dwUnionChoice = WTD_CHOICE_CERT;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        certInfo.psCertContext = (PCERT_CONTEXT)CertCreateCertificateContext(
         X509_ASN_ENCODING, v1CertWithPubKey, sizeof(v1CertWithPubKey));
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        CertFreeCertificateContext(certInfo.psCertContext);
        certInfo.psCertContext = NULL;
        wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
        U(wintrust_data).pFile = NULL;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        U(wintrust_data).pFile = &fileInfo;
        /* Crashes
        ret = funcs->pfnObjectTrust(&data);
         */
        getNotepadPath(notepadPathW, MAX_PATH);
        fileInfo.pcwszFilePath = notepadPathW;
        /* pfnObjectTrust now crashes unless both pPDSip and psPfns are set */
        U(data).pPDSip = &provDataSIP;
        data.psPfns = (CRYPT_PROVIDER_FUNCTIONS *)funcs;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE, "Expected TRUST_E_NOSIGNATURE, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        ok(!memcmp(&provDataSIP.gSubject, &unknown, sizeof(unknown)),
         "Unexpected subject GUID\n");
        ok(provDataSIP.pSip != NULL, "Expected a SIP\n");
        ok(provDataSIP.psSipSubjectInfo != NULL, "Expected a subject info\n");
        funcs->pfnFree(data.padwTrustStepErrors);
    }
}

static const BYTE selfSignedCert[] = {
 0x30, 0x82, 0x01, 0x1f, 0x30, 0x81, 0xce, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02,
 0x10, 0xeb, 0x0d, 0x57, 0x2a, 0x9c, 0x09, 0xba, 0xa4, 0x4a, 0xb7, 0x25, 0x49,
 0xd9, 0x3e, 0xb5, 0x73, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d,
 0x05, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03,
 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30,
 0x1e, 0x17, 0x0d, 0x30, 0x36, 0x30, 0x36, 0x32, 0x39, 0x30, 0x35, 0x30, 0x30,
 0x34, 0x36, 0x5a, 0x17, 0x0d, 0x30, 0x37, 0x30, 0x36, 0x32, 0x39, 0x31, 0x31,
 0x30, 0x30, 0x34, 0x36, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e,
 0x67, 0x00, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xe2, 0x54, 0x3a, 0xa7, 0x83, 0xb1, 0x27, 0x14, 0x3e, 0x59, 0xbb, 0xb4,
 0x53, 0xe6, 0x1f, 0xe7, 0x5d, 0xf1, 0x21, 0x68, 0xad, 0x85, 0x53, 0xdb, 0x6b,
 0x1e, 0xeb, 0x65, 0x97, 0x03, 0x86, 0x60, 0xde, 0xf3, 0x6c, 0x38, 0x75, 0xe0,
 0x4c, 0x61, 0xbb, 0xbc, 0x62, 0x17, 0xa9, 0xcd, 0x79, 0x3f, 0x21, 0x4e, 0x96,
 0xcb, 0x0e, 0xdc, 0x61, 0x94, 0x30, 0x18, 0x10, 0x6b, 0xd0, 0x1c, 0x10, 0x79,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x25, 0x90, 0x53, 0x34, 0xd9, 0x56, 0x41,
 0x5e, 0xdb, 0x7e, 0x01, 0x36, 0xec, 0x27, 0x61, 0x5e, 0xb7, 0x4d, 0x90, 0x66,
 0xa2, 0xe1, 0x9d, 0x58, 0x76, 0xd4, 0x9c, 0xba, 0x2c, 0x84, 0xc6, 0x83, 0x7a,
 0x22, 0x0d, 0x03, 0x69, 0x32, 0x1a, 0x6d, 0xcb, 0x0c, 0x15, 0xb3, 0x6b, 0xc7,
 0x0a, 0x8c, 0xb4, 0x5c, 0x34, 0x78, 0xe0, 0x3c, 0x9c, 0xe9, 0xf3, 0x30, 0x9f,
 0xa8, 0x76, 0x57, 0x92, 0x36 };

static void testCertTrust(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    CRYPT_PROVIDER_DATA data = { 0 };
    CRYPT_PROVIDER_SGNR sgnr = { sizeof(sgnr), { 0 } };
    HRESULT ret;

    data.padwTrustStepErrors =
     funcs->pfnAlloc(TRUSTERROR_MAX_STEPS * sizeof(DWORD));
    if (!data.padwTrustStepErrors)
    {
        skip("pfnAlloc failed\n");
        return;
    }
    ret = funcs->pfnCertificateTrust(&data);
    ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
    ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] ==
     TRUST_E_NOSIGNATURE, "Expected TRUST_E_NOSIGNATURE, got %08x\n",
     data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    ret = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
    if (ret)
    {
        PCCERT_CONTEXT cert;

        /* An empty signer "succeeds," even though there's no cert */
        ret = funcs->pfnCertificateTrust(&data);
        ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
        cert = CertCreateCertificateContext(X509_ASN_ENCODING, selfSignedCert,
         sizeof(selfSignedCert));
        if (cert)
        {
            WINTRUST_DATA wintrust_data = { 0 };

            ret = funcs->pfnAddCert2Chain(&data, 0, FALSE, 0, cert);
            /* If pWintrustData isn't set, crashes attempting to access
             * pWintrustData->fdwRevocationChecks
             */
            data.pWintrustData = &wintrust_data;
            /* If psPfns isn't set, crashes attempting to access
             * psPfns->pfnCertCheckPolicy
             */
            data.psPfns = (CRYPT_PROVIDER_FUNCTIONS *)funcs;
            ret = funcs->pfnCertificateTrust(&data);
            ok(ret == S_OK, "Expected S_OK, got %08x\n", ret);
            ok(data.csSigners == 1, "Unexpected number of signers %d\n",
             data.csSigners);
            ok(data.pasSigners[0].pChainContext != NULL,
             "Expected a certificate chain\n");
            ok(data.pasSigners[0].csCertChain == 1,
             "Unexpected number of chain elements %d\n",
             data.pasSigners[0].csCertChain);
            /* pasSigners and pasSigners[0].pasCertChain are guaranteed to be
             * initialized, see tests for pfnAddSgnr2Chain and pfnAddCert2Chain
             */
            ok(!data.pasSigners[0].pasCertChain[0].fTrustedRoot,
             "Didn't expect cert to be trusted\n");
            ok(data.pasSigners[0].pasCertChain[0].fSelfSigned,
             "Expected cert to be self-signed\n");
            ok(data.pasSigners[0].pasCertChain[0].dwConfidence ==
             (CERT_CONFIDENCE_SIG | CERT_CONFIDENCE_TIMENEST),
             "Expected CERT_CONFIDENCE_SIG | CERT_CONFIDENCE_TIMENEST, got %08x\n",
             data.pasSigners[0].pasCertChain[0].dwConfidence);
            CertFreeCertificateContext(cert);
        }
    }
}

static void test_provider_funcs(void)
{
    static GUID generic_verify_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    SAFE_PROVIDER_FUNCTIONS funcs = { sizeof(SAFE_PROVIDER_FUNCTIONS), 0 };
    BOOL ret;

    ret = WintrustLoadFunctionPointers(&generic_verify_v2,
     (CRYPT_PROVIDER_FUNCTIONS *)&funcs);
    if (!ret)
        skip("WintrustLoadFunctionPointers failed\n");
    else
    {
        test_utils(&funcs);
        testInitialize(&funcs, &generic_verify_v2);
        testObjTrust(&funcs, &generic_verify_v2);
        testCertTrust(&funcs, &generic_verify_v2);
    }
}

static void test_wintrust(void)
{
    static GUID generic_action_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA wtd;
    WINTRUST_FILE_INFO file;
    LONG r;
    HRESULT hr;
    WCHAR notepadPathW[MAX_PATH];

    memset(&wtd, 0, sizeof(wtd));
    wtd.cbStruct = sizeof(wtd);
    wtd.dwUIChoice = WTD_UI_NONE;
    wtd.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    wtd.dwUnionChoice = WTD_CHOICE_FILE;
    U(wtd).pFile = &file;
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    memset(&file, 0, sizeof(file));
    file.cbStruct = sizeof(file);
    getNotepadPath(notepadPathW, MAX_PATH);
    file.pcwszFilePath = notepadPathW;
    r = WinVerifyTrust(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(r == TRUST_E_NOSIGNATURE, "expected TRUST_E_NOSIGNATURE, got %08x\n", r);
    hr = WinVerifyTrustEx(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(hr == TRUST_E_NOSIGNATURE, "expected TRUST_E_NOSIGNATURE, got %08x\n",
     hr);
}

static BOOL (WINAPI * pWTHelperGetKnownUsages)(DWORD action, PCCRYPT_OID_INFO **usages);

static void InitFunctionPtrs(void)
{
    HMODULE hWintrust = GetModuleHandleA("wintrust.dll");

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

    WINTRUST_GET_PROC(WTHelperGetKnownUsages)

#undef WINTRUST_GET_PROC
}

static void test_get_known_usages(void)
{
    BOOL ret;
    PCCRYPT_OID_INFO *usages;

    if (!pWTHelperGetKnownUsages)
    {
        skip("missing WTHelperGetKnownUsages\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pWTHelperGetKnownUsages(0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pWTHelperGetKnownUsages(1, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pWTHelperGetKnownUsages(0, &usages);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    /* A value of 1 for the first parameter seems to imply the value is
     * allocated
     */
    SetLastError(0xdeadbeef);
    usages = NULL;
    ret = pWTHelperGetKnownUsages(1, &usages);
    ok(ret, "WTHelperGetKnownUsages failed: %d\n", GetLastError());
    ok(usages != NULL, "expected a pointer\n");
    if (ret && usages)
    {
        PCCRYPT_OID_INFO *ptr;

        /* The returned usages are an array of PCCRYPT_OID_INFOs, terminated with a
         * NULL pointer.
         */
        for (ptr = usages; *ptr; ptr++)
        {
            ok((*ptr)->cbSize == sizeof(CRYPT_OID_INFO) ||
             (*ptr)->cbSize == (sizeof(CRYPT_OID_INFO) + 2 * sizeof(LPCWSTR)), /* Vista */
             "unexpected size %d\n", (*ptr)->cbSize);
            /* Each returned usage is in the CRYPT_ENHKEY_USAGE_OID_GROUP_ID group */
            ok((*ptr)->dwGroupId == CRYPT_ENHKEY_USAGE_OID_GROUP_ID,
             "expected group CRYPT_ENHKEY_USAGE_OID_GROUP_ID, got %d\n",
             (*ptr)->dwGroupId);
        }
    }
    /* A value of 2 for the second parameter seems to imply the value is freed
     */
    SetLastError(0xdeadbeef);
    ret = pWTHelperGetKnownUsages(2, &usages);
    ok(ret, "WTHelperGetKnownUsages failed: %d\n", GetLastError());
    ok(usages == NULL, "expected pointer to be cleared\n");
    SetLastError(0xdeadbeef);
    usages = NULL;
    ret = pWTHelperGetKnownUsages(2, &usages);
    ok(ret, "WTHelperGetKnownUsages failed: %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pWTHelperGetKnownUsages(2, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
}

START_TEST(softpub)
{
    InitFunctionPtrs();
    test_provider_funcs();
    test_wintrust();
    test_get_known_usages();
}
