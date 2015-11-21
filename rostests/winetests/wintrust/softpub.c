/*
 * wintrust softpub functions tests
 *
 * Copyright 2007,2010 Juan Lang
 * Copyright 2010 Andrey Turkin
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

static BOOL (WINAPI * pWTHelperGetKnownUsages)(DWORD action, PCCRYPT_OID_INFO **usages);
static BOOL (WINAPI * CryptSIPCreateIndirectData_p)(SIP_SUBJECTINFO *, DWORD *, SIP_INDIRECT_DATA *);
static VOID (WINAPI * CertFreeCertificateChain_p)(PCCERT_CHAIN_CONTEXT);

static void InitFunctionPtrs(void)
{
    HMODULE hWintrust = GetModuleHandleA("wintrust.dll");
    HMODULE hCrypt32 = GetModuleHandleA("crypt32.dll");

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

    WINTRUST_GET_PROC(WTHelperGetKnownUsages)

#undef WINTRUST_GET_PROC

#define CRYPT32_GET_PROC(func) \
    func ## _p = (void*)GetProcAddress(hCrypt32, #func); \
    if(!func ## _p) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

    CRYPT32_GET_PROC(CryptSIPCreateIndirectData)
    CRYPT32_GET_PROC(CertFreeCertificateChain)

#undef CRYPT32_GET_PROC
}

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
            {
                ok(data.pasSigners[0].pasCertChain[0].pCert == cert,
                 "Unexpected cert\n");
                CertFreeCertificateContext(
                 data.pasSigners[0].pasCertChain[0].pCert);
            }
            CertFreeCertificateContext(cert);
        }
        else
            skip("CertCreateCertificateContext failed: %08x\n", GetLastError());
        funcs->pfnFree(data.pasSigners);
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
    MultiByteToWideChar(CP_ACP, 0, notepadPath, -1, notepadPathW, size);
}

/* Creates a test file and returns a handle to it.  The file's path is returned
 * in temp_file, which must be at least MAX_PATH characters in length.
 */
static HANDLE create_temp_file(WCHAR *temp_file)
{
    HANDLE file = INVALID_HANDLE_VALUE;
    WCHAR temp_path[MAX_PATH];

    if (GetTempPathW(sizeof(temp_path) / sizeof(temp_path[0]), temp_path))
    {
        static const WCHAR img[] = { 'i','m','g',0 };

        if (GetTempFileNameW(temp_path, img, 0, temp_file))
            file = CreateFileW(temp_file, GENERIC_READ | GENERIC_WRITE, 0, NULL,
             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return file;
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
        WCHAR pathW[MAX_PATH];
        PROVDATA_SIP provDataSIP = { 0 };
        static const GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
         0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
        static GUID bogusGuid = { 0xdeadbeef, 0xbaad, 0xf00d, { 0x00,0x00,0x00,
         0x00,0x00,0x00,0x00,0x00 } };

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
        /* Create and test with an empty file */
        fileInfo.hFile = create_temp_file(pathW);
        /* pfnObjectTrust now crashes unless both pPDSip and psPfns are set */
        U(data).pPDSip = &provDataSIP;
        data.psPfns = (CRYPT_PROVIDER_FUNCTIONS *)funcs;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_SUBJECT_FORM_UNKNOWN,
         "expected TRUST_E_SUBJECT_FORM_UNKNOWN, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        CloseHandle(fileInfo.hFile);
        fileInfo.hFile = NULL;
        fileInfo.pcwszFilePath = pathW;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_SUBJECT_FORM_UNKNOWN,
         "expected TRUST_E_SUBJECT_FORM_UNKNOWN, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        DeleteFileW(pathW);
        /* Test again with a file we expect to exist, and to contain no
         * signature.
         */
        getNotepadPath(pathW, MAX_PATH);
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE ||
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_SUBJECT_FORM_UNKNOWN,
         "Expected TRUST_E_NOSIGNATURE or TRUST_E_SUBJECT_FORM_UNKNOWN, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        if (data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE)
        {
            ok(!memcmp(&provDataSIP.gSubject, &unknown, sizeof(unknown)),
             "Unexpected subject GUID\n");
            ok(provDataSIP.pSip != NULL, "Expected a SIP\n");
            ok(provDataSIP.psSipSubjectInfo != NULL,
             "Expected a subject info\n");
        }
        /* Specifying the GUID results in that GUID being the subject GUID */
        fileInfo.pgKnownSubject = &bogusGuid;
        ret = funcs->pfnObjectTrust(&data);
        ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        ok(data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE ||
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_SUBJECT_FORM_UNKNOWN ||
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_PROVIDER_UNKNOWN,
         "Expected TRUST_E_NOSIGNATURE or TRUST_E_SUBJECT_FORM_UNKNOWN or TRUST_E_PROVIDER_UNKNOWN, got %08x\n",
         data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
        if (data.padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] ==
         TRUST_E_NOSIGNATURE)
        {
            ok(!memcmp(&provDataSIP.gSubject, &bogusGuid, sizeof(bogusGuid)),
             "unexpected subject GUID\n");
        }
        /* Specifying a bogus GUID pointer crashes */
        if (0)
        {
            fileInfo.pgKnownSubject = (GUID *)0xdeadbeef;
            ret = funcs->pfnObjectTrust(&data);
            ok(ret == S_FALSE, "Expected S_FALSE, got %08x\n", ret);
        }
        funcs->pfnFree(data.padwTrustStepErrors);
    }
}

static const BYTE selfSignedCert[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x43,
  0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45, 0x2d, 0x2d,
  0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x44, 0x70, 0x7a, 0x43, 0x43,
  0x41, 0x6f, 0x2b, 0x67, 0x41, 0x77, 0x49, 0x42, 0x41, 0x67, 0x49, 0x4a,
  0x41, 0x4c, 0x59, 0x51, 0x67, 0x65, 0x66, 0x7a, 0x51, 0x41, 0x61, 0x43,
  0x4d, 0x41, 0x30, 0x47, 0x43, 0x53, 0x71, 0x47, 0x53, 0x49, 0x62, 0x33,
  0x44, 0x51, 0x45, 0x42, 0x42, 0x51, 0x55, 0x41, 0x4d, 0x47, 0x6f, 0x78,
  0x43, 0x7a, 0x41, 0x4a, 0x42, 0x67, 0x4e, 0x56, 0x0a, 0x42, 0x41, 0x59,
  0x54, 0x41, 0x6b, 0x46, 0x56, 0x4d, 0x52, 0x4d, 0x77, 0x45, 0x51, 0x59,
  0x44, 0x56, 0x51, 0x51, 0x49, 0x44, 0x41, 0x70, 0x54, 0x62, 0x32, 0x31,
  0x6c, 0x4c, 0x56, 0x4e, 0x30, 0x59, 0x58, 0x52, 0x6c, 0x4d, 0x53, 0x45,
  0x77, 0x48, 0x77, 0x59, 0x44, 0x56, 0x51, 0x51, 0x4b, 0x44, 0x42, 0x68,
  0x4a, 0x62, 0x6e, 0x52, 0x6c, 0x63, 0x6d, 0x35, 0x6c, 0x64, 0x43, 0x42,
  0x58, 0x0a, 0x61, 0x57, 0x52, 0x6e, 0x61, 0x58, 0x52, 0x7a, 0x49, 0x46,
  0x42, 0x30, 0x65, 0x53, 0x42, 0x4d, 0x64, 0x47, 0x51, 0x78, 0x49, 0x7a,
  0x41, 0x68, 0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x4d, 0x4d, 0x47, 0x6e,
  0x4e, 0x6c, 0x62, 0x47, 0x5a, 0x7a, 0x61, 0x57, 0x64, 0x75, 0x5a, 0x57,
  0x51, 0x75, 0x64, 0x47, 0x56, 0x7a, 0x64, 0x43, 0x35, 0x33, 0x61, 0x57,
  0x35, 0x6c, 0x61, 0x48, 0x45, 0x75, 0x0a, 0x62, 0x33, 0x4a, 0x6e, 0x4d,
  0x42, 0x34, 0x58, 0x44, 0x54, 0x45, 0x7a, 0x4d, 0x44, 0x59, 0x79, 0x4d,
  0x54, 0x45, 0x78, 0x4d, 0x6a, 0x55, 0x78, 0x4d, 0x46, 0x6f, 0x58, 0x44,
  0x54, 0x49, 0x7a, 0x4d, 0x44, 0x59, 0x78, 0x4f, 0x54, 0x45, 0x78, 0x4d,
  0x6a, 0x55, 0x78, 0x4d, 0x46, 0x6f, 0x77, 0x61, 0x6a, 0x45, 0x4c, 0x4d,
  0x41, 0x6b, 0x47, 0x41, 0x31, 0x55, 0x45, 0x42, 0x68, 0x4d, 0x43, 0x0a,
  0x51, 0x56, 0x55, 0x78, 0x45, 0x7a, 0x41, 0x52, 0x42, 0x67, 0x4e, 0x56,
  0x42, 0x41, 0x67, 0x4d, 0x43, 0x6c, 0x4e, 0x76, 0x62, 0x57, 0x55, 0x74,
  0x55, 0x33, 0x52, 0x68, 0x64, 0x47, 0x55, 0x78, 0x49, 0x54, 0x41, 0x66,
  0x42, 0x67, 0x4e, 0x56, 0x42, 0x41, 0x6f, 0x4d, 0x47, 0x45, 0x6c, 0x75,
  0x64, 0x47, 0x56, 0x79, 0x62, 0x6d, 0x56, 0x30, 0x49, 0x46, 0x64, 0x70,
  0x5a, 0x47, 0x64, 0x70, 0x0a, 0x64, 0x48, 0x4d, 0x67, 0x55, 0x48, 0x52,
  0x35, 0x49, 0x45, 0x78, 0x30, 0x5a, 0x44, 0x45, 0x6a, 0x4d, 0x43, 0x45,
  0x47, 0x41, 0x31, 0x55, 0x45, 0x41, 0x77, 0x77, 0x61, 0x63, 0x32, 0x56,
  0x73, 0x5a, 0x6e, 0x4e, 0x70, 0x5a, 0x32, 0x35, 0x6c, 0x5a, 0x43, 0x35,
  0x30, 0x5a, 0x58, 0x4e, 0x30, 0x4c, 0x6e, 0x64, 0x70, 0x62, 0x6d, 0x56,
  0x6f, 0x63, 0x53, 0x35, 0x76, 0x63, 0x6d, 0x63, 0x77, 0x0a, 0x67, 0x67,
  0x45, 0x69, 0x4d, 0x41, 0x30, 0x47, 0x43, 0x53, 0x71, 0x47, 0x53, 0x49,
  0x62, 0x33, 0x44, 0x51, 0x45, 0x42, 0x41, 0x51, 0x55, 0x41, 0x41, 0x34,
  0x49, 0x42, 0x44, 0x77, 0x41, 0x77, 0x67, 0x67, 0x45, 0x4b, 0x41, 0x6f,
  0x49, 0x42, 0x41, 0x51, 0x44, 0x77, 0x4e, 0x6d, 0x2b, 0x46, 0x7a, 0x78,
  0x6e, 0x6b, 0x48, 0x57, 0x2f, 0x4e, 0x70, 0x37, 0x59, 0x48, 0x34, 0x4d,
  0x79, 0x45, 0x0a, 0x77, 0x4d, 0x6c, 0x49, 0x67, 0x71, 0x30, 0x66, 0x45,
  0x77, 0x70, 0x47, 0x6f, 0x41, 0x75, 0x78, 0x44, 0x64, 0x61, 0x46, 0x55,
  0x32, 0x6f, 0x70, 0x76, 0x41, 0x51, 0x56, 0x61, 0x2b, 0x41, 0x43, 0x46,
  0x38, 0x63, 0x6f, 0x38, 0x4d, 0x4a, 0x6c, 0x33, 0x78, 0x77, 0x76, 0x46,
  0x44, 0x2b, 0x67, 0x61, 0x46, 0x45, 0x7a, 0x59, 0x78, 0x53, 0x58, 0x30,
  0x43, 0x47, 0x72, 0x4a, 0x45, 0x4c, 0x63, 0x0a, 0x74, 0x34, 0x4d, 0x69,
  0x30, 0x68, 0x4b, 0x50, 0x76, 0x42, 0x70, 0x65, 0x73, 0x59, 0x6c, 0x46,
  0x4d, 0x51, 0x65, 0x6b, 0x2b, 0x63, 0x70, 0x51, 0x50, 0x33, 0x4b, 0x35,
  0x75, 0x36, 0x71, 0x58, 0x5a, 0x52, 0x49, 0x67, 0x48, 0x75, 0x59, 0x45,
  0x4c, 0x2f, 0x73, 0x55, 0x6f, 0x39, 0x32, 0x70, 0x44, 0x30, 0x7a, 0x4a,
  0x65, 0x4c, 0x47, 0x41, 0x31, 0x49, 0x30, 0x4b, 0x5a, 0x34, 0x73, 0x2f,
  0x0a, 0x51, 0x7a, 0x77, 0x61, 0x4f, 0x38, 0x62, 0x62, 0x4b, 0x6d, 0x37,
  0x42, 0x72, 0x6e, 0x56, 0x77, 0x30, 0x6e, 0x5a, 0x2f, 0x4b, 0x41, 0x5a,
  0x6a, 0x75, 0x78, 0x75, 0x6f, 0x4e, 0x33, 0x52, 0x64, 0x72, 0x69, 0x30,
  0x4a, 0x48, 0x77, 0x7a, 0x6a, 0x41, 0x55, 0x34, 0x2b, 0x71, 0x57, 0x65,
  0x55, 0x63, 0x2f, 0x64, 0x33, 0x45, 0x70, 0x4f, 0x47, 0x78, 0x69, 0x42,
  0x77, 0x5a, 0x4e, 0x61, 0x7a, 0x0a, 0x39, 0x6f, 0x4a, 0x41, 0x37, 0x54,
  0x2f, 0x51, 0x6f, 0x62, 0x75, 0x61, 0x4e, 0x53, 0x6b, 0x65, 0x55, 0x48,
  0x43, 0x61, 0x50, 0x53, 0x6a, 0x44, 0x37, 0x71, 0x7a, 0x6c, 0x43, 0x4f,
  0x52, 0x48, 0x47, 0x68, 0x75, 0x31, 0x76, 0x79, 0x79, 0x35, 0x31, 0x45,
  0x36, 0x79, 0x46, 0x43, 0x4e, 0x47, 0x66, 0x65, 0x7a, 0x71, 0x2f, 0x4d,
  0x59, 0x34, 0x4e, 0x4b, 0x68, 0x77, 0x72, 0x61, 0x59, 0x64, 0x0a, 0x62,
  0x79, 0x49, 0x2f, 0x6c, 0x42, 0x46, 0x62, 0x36, 0x35, 0x6b, 0x5a, 0x45,
  0x66, 0x49, 0x4b, 0x4b, 0x54, 0x7a, 0x79, 0x36, 0x76, 0x30, 0x44, 0x65,
  0x79, 0x50, 0x37, 0x52, 0x6b, 0x34, 0x75, 0x48, 0x44, 0x38, 0x77, 0x62,
  0x49, 0x79, 0x50, 0x32, 0x47, 0x6c, 0x42, 0x30, 0x67, 0x37, 0x2f, 0x69,
  0x79, 0x33, 0x4c, 0x61, 0x74, 0x49, 0x74, 0x49, 0x70, 0x2b, 0x49, 0x35,
  0x53, 0x50, 0x56, 0x0a, 0x41, 0x67, 0x4d, 0x42, 0x41, 0x41, 0x47, 0x6a,
  0x55, 0x44, 0x42, 0x4f, 0x4d, 0x42, 0x30, 0x47, 0x41, 0x31, 0x55, 0x64,
  0x44, 0x67, 0x51, 0x57, 0x42, 0x42, 0x53, 0x36, 0x49, 0x4c, 0x5a, 0x2f,
  0x71, 0x38, 0x66, 0x2f, 0x4b, 0x45, 0x68, 0x4b, 0x76, 0x68, 0x69, 0x2b,
  0x73, 0x6b, 0x59, 0x45, 0x31, 0x79, 0x48, 0x71, 0x39, 0x7a, 0x41, 0x66,
  0x42, 0x67, 0x4e, 0x56, 0x48, 0x53, 0x4d, 0x45, 0x0a, 0x47, 0x44, 0x41,
  0x57, 0x67, 0x42, 0x53, 0x36, 0x49, 0x4c, 0x5a, 0x2f, 0x71, 0x38, 0x66,
  0x2f, 0x4b, 0x45, 0x68, 0x4b, 0x76, 0x68, 0x69, 0x2b, 0x73, 0x6b, 0x59,
  0x45, 0x31, 0x79, 0x48, 0x71, 0x39, 0x7a, 0x41, 0x4d, 0x42, 0x67, 0x4e,
  0x56, 0x48, 0x52, 0x4d, 0x45, 0x42, 0x54, 0x41, 0x44, 0x41, 0x51, 0x48,
  0x2f, 0x4d, 0x41, 0x30, 0x47, 0x43, 0x53, 0x71, 0x47, 0x53, 0x49, 0x62,
  0x33, 0x0a, 0x44, 0x51, 0x45, 0x42, 0x42, 0x51, 0x55, 0x41, 0x41, 0x34,
  0x49, 0x42, 0x41, 0x51, 0x41, 0x79, 0x5a, 0x59, 0x77, 0x47, 0x4b, 0x46,
  0x34, 0x34, 0x43, 0x68, 0x47, 0x51, 0x72, 0x6e, 0x74, 0x57, 0x6c, 0x38,
  0x48, 0x53, 0x4a, 0x30, 0x63, 0x69, 0x55, 0x58, 0x4d, 0x44, 0x4b, 0x32,
  0x46, 0x6c, 0x6f, 0x74, 0x47, 0x49, 0x6a, 0x30, 0x32, 0x6c, 0x4d, 0x39,
  0x38, 0x71, 0x45, 0x49, 0x65, 0x68, 0x0a, 0x56, 0x67, 0x66, 0x41, 0x34,
  0x7a, 0x69, 0x37, 0x4d, 0x45, 0x6c, 0x51, 0x61, 0x76, 0x6b, 0x52, 0x76,
  0x32, 0x54, 0x43, 0x50, 0x50, 0x55, 0x51, 0x62, 0x35, 0x51, 0x64, 0x61,
  0x6f, 0x37, 0x57, 0x78, 0x37, 0x6c, 0x66, 0x61, 0x54, 0x6f, 0x5a, 0x68,
  0x4f, 0x54, 0x2b, 0x4e, 0x52, 0x68, 0x32, 0x6b, 0x35, 0x78, 0x2b, 0x6b,
  0x6a, 0x5a, 0x46, 0x77, 0x38, 0x70, 0x45, 0x48, 0x74, 0x35, 0x51, 0x0a,
  0x69, 0x68, 0x62, 0x46, 0x4c, 0x35, 0x58, 0x2b, 0x57, 0x7a, 0x6f, 0x2b,
  0x42, 0x36, 0x36, 0x59, 0x79, 0x49, 0x76, 0x68, 0x77, 0x54, 0x63, 0x48,
  0x30, 0x46, 0x2b, 0x6e, 0x66, 0x55, 0x71, 0x66, 0x74, 0x38, 0x59, 0x74,
  0x72, 0x2f, 0x38, 0x37, 0x47, 0x45, 0x62, 0x73, 0x41, 0x48, 0x6a, 0x48,
  0x43, 0x36, 0x4c, 0x2b, 0x77, 0x6b, 0x31, 0x76, 0x4e, 0x6e, 0x64, 0x49,
  0x59, 0x47, 0x30, 0x51, 0x0a, 0x79, 0x62, 0x73, 0x7a, 0x78, 0x49, 0x72,
  0x32, 0x6d, 0x46, 0x45, 0x49, 0x4a, 0x6f, 0x69, 0x51, 0x44, 0x44, 0x67,
  0x66, 0x6c, 0x71, 0x67, 0x64, 0x76, 0x4c, 0x54, 0x32, 0x79, 0x64, 0x46,
  0x6d, 0x79, 0x33, 0x73, 0x32, 0x68, 0x49, 0x74, 0x51, 0x6c, 0x49, 0x71,
  0x4b, 0x4c, 0x42, 0x36, 0x49, 0x4a, 0x51, 0x49, 0x75, 0x69, 0x37, 0x72,
  0x37, 0x34, 0x76, 0x64, 0x72, 0x63, 0x58, 0x71, 0x58, 0x0a, 0x44, 0x7a,
  0x68, 0x6d, 0x4c, 0x66, 0x67, 0x6a, 0x67, 0x4c, 0x77, 0x33, 0x2b, 0x55,
  0x79, 0x69, 0x59, 0x74, 0x44, 0x54, 0x76, 0x63, 0x78, 0x65, 0x7a, 0x62,
  0x4c, 0x73, 0x76, 0x51, 0x6f, 0x52, 0x6b, 0x74, 0x77, 0x4b, 0x5a, 0x4c,
  0x44, 0x54, 0x42, 0x42, 0x35, 0x76, 0x59, 0x32, 0x78, 0x4b, 0x36, 0x6b,
  0x4f, 0x4f, 0x44, 0x70, 0x7a, 0x50, 0x48, 0x73, 0x4b, 0x67, 0x30, 0x42,
  0x59, 0x77, 0x0a, 0x4d, 0x6b, 0x48, 0x56, 0x56, 0x54, 0x34, 0x79, 0x2f,
  0x4d, 0x59, 0x36, 0x63, 0x63, 0x4b, 0x51, 0x2f, 0x4c, 0x56, 0x74, 0x32,
  0x66, 0x4a, 0x49, 0x74, 0x69, 0x41, 0x71, 0x49, 0x47, 0x32, 0x38, 0x64,
  0x37, 0x31, 0x53, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e, 0x44,
  0x20, 0x43, 0x45, 0x52, 0x54, 0x49, 0x46, 0x49, 0x43, 0x41, 0x54, 0x45,
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
};

static void testCertTrust(SAFE_PROVIDER_FUNCTIONS *funcs, GUID *actionID)
{
    CRYPT_PROVIDER_DATA data = { 0 };
    CRYPT_PROVIDER_SGNR sgnr = { sizeof(sgnr), { 0 } };
    HRESULT ret;
    BOOL b;

    if (!CertFreeCertificateChain_p)
    {
        win_skip("CertFreeCertificateChain not found\n");
        return;
    }

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
    b = funcs->pfnAddSgnr2Chain(&data, FALSE, 0, &sgnr);
    if (b)
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

            b = funcs->pfnAddCert2Chain(&data, 0, FALSE, 0, cert);
            ok(b == TRUE, "Expected TRUE, got %d\n", b);

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
            CertFreeCertificateContext(
             data.pasSigners[0].pasCertChain[0].pCert);
            CertFreeCertificateChain_p(data.pasSigners[0].pChainContext);
            CertFreeCertificateContext(cert);
        }
    }
    funcs->pfnFree(data.padwTrustStepErrors);
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

/* minimal PE file image */
#define VA_START 0x400000
#define FILE_PE_START 0x50
#define NUM_SECTIONS 3
#define FILE_TEXT 0x200
#define RVA_TEXT 0x1000
#define RVA_BSS 0x2000
#define FILE_IDATA 0x400
#define RVA_IDATA 0x3000
#define FILE_TOTAL 0x600
#define RVA_TOTAL 0x4000
#include <pshpack1.h>
struct Imports {
    IMAGE_IMPORT_DESCRIPTOR descriptors[2];
    IMAGE_THUNK_DATA32 original_thunks[2];
    IMAGE_THUNK_DATA32 thunks[2];
    struct __IMPORT_BY_NAME {
        WORD hint;
        char funcname[0x20];
    } ibn;
    char dllname[0x10];
};
#define EXIT_PROCESS (VA_START+RVA_IDATA+FIELD_OFFSET(struct Imports, thunks))

static struct _PeImage {
    IMAGE_DOS_HEADER dos_header;
    char __alignment1[FILE_PE_START - sizeof(IMAGE_DOS_HEADER)];
    IMAGE_NT_HEADERS32 nt_headers;
    IMAGE_SECTION_HEADER sections[NUM_SECTIONS];
    char __alignment2[FILE_TEXT - FILE_PE_START - sizeof(IMAGE_NT_HEADERS32) -
        NUM_SECTIONS * sizeof(IMAGE_SECTION_HEADER)];
    unsigned char text_section[FILE_IDATA-FILE_TEXT];
    struct Imports idata_section;
    char __alignment3[FILE_TOTAL-FILE_IDATA-sizeof(struct Imports)];
} bin = {
    /* dos header */
    {IMAGE_DOS_SIGNATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, {0}, 0, 0, {0}, FILE_PE_START},
    /* alignment before PE header */
    {0},
    /* nt headers */
    {IMAGE_NT_SIGNATURE,
        /* basic headers - 3 sections, no symbols, EXE file */
        {IMAGE_FILE_MACHINE_I386, NUM_SECTIONS, 0, 0, 0, sizeof(IMAGE_OPTIONAL_HEADER32),
            IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_EXECUTABLE_IMAGE},
        /* optional header */
        {IMAGE_NT_OPTIONAL_HDR32_MAGIC, 4, 0, FILE_IDATA-FILE_TEXT,
            FILE_TOTAL-FILE_IDATA + FILE_IDATA-FILE_TEXT, 0x400,
            RVA_TEXT, RVA_TEXT, RVA_BSS, VA_START, 0x1000, 0x200, 4, 0, 1, 0, 4, 0, 0,
            RVA_TOTAL, FILE_TEXT, 0, IMAGE_SUBSYSTEM_WINDOWS_GUI, 0,
            0x200000, 0x1000, 0x100000, 0x1000, 0, 0x10,
            {{0, 0},
             {RVA_IDATA, sizeof(struct Imports)}
            }
        }
    },
    /* sections */
    {
        {".text", {0x100}, RVA_TEXT, FILE_IDATA-FILE_TEXT, FILE_TEXT,
            0, 0, 0, 0, IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ},
        {".bss", {0x400}, RVA_BSS, 0, 0, 0, 0, 0, 0,
            IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE},
        {".idata", {sizeof(struct Imports)}, RVA_IDATA, FILE_TOTAL-FILE_IDATA, FILE_IDATA, 0,
            0, 0, 0, IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE}
    },
    /* alignment before first section */
    {0},
    /* .text section */
    {
        0x31, 0xC0, /* xor eax, eax */
        0xFF, 0x25, EXIT_PROCESS&0xFF, (EXIT_PROCESS>>8)&0xFF, (EXIT_PROCESS>>16)&0xFF,
            (EXIT_PROCESS>>24)&0xFF, /* jmp ExitProcess */
        0
    },
    /* .idata section */
    {
        {
            {{RVA_IDATA + FIELD_OFFSET(struct Imports, original_thunks)}, 0, 0,
            RVA_IDATA + FIELD_OFFSET(struct Imports, dllname),
            RVA_IDATA + FIELD_OFFSET(struct Imports, thunks)
            },
            {{0}, 0, 0, 0, 0}
        },
        {{{RVA_IDATA+FIELD_OFFSET(struct Imports, ibn)}}, {{0}}},
        {{{RVA_IDATA+FIELD_OFFSET(struct Imports, ibn)}}, {{0}}},
        {0,"ExitProcess"},
        "KERNEL32.DLL"
    },
    /* final alignment */
    {0}
};
#include <poppack.h>

static void test_sip_create_indirect_data(void)
{
    static GUID unknown = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,
     0x00,0xC0,0x4F,0xC2,0x95,0xEE } };
    static char oid_sha1[] = szOID_OIWSEC_sha1;
    BOOL ret;
    SIP_SUBJECTINFO subjinfo = { 0 };
    WCHAR temp_file[MAX_PATH];
    HANDLE file;
    DWORD count;

    if (!CryptSIPCreateIndirectData_p)
    {
        skip("Missing CryptSIPCreateIndirectData\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(&subjinfo, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    subjinfo.cbSize = sizeof(subjinfo);
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(&subjinfo, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    file = create_temp_file(temp_file);
    if (file == INVALID_HANDLE_VALUE)
    {
        skip("couldn't create temp file\n");
        return;
    }
    WriteFile(file, &bin, sizeof(bin), &count, NULL);
    FlushFileBuffers(file);

    subjinfo.hFile = file;
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(&subjinfo, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    subjinfo.pgSubjectType = &unknown;
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(&subjinfo, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = CryptSIPCreateIndirectData_p(&subjinfo, &count, NULL);
    todo_wine
    ok(!ret && (GetLastError() == NTE_BAD_ALGID ||
                GetLastError() == ERROR_INVALID_PARAMETER /* Win7 */),
       "expected NTE_BAD_ALGID or ERROR_INVALID_PARAMETER, got %08x\n",
       GetLastError());
    ok(count == 0xdeadbeef, "expected count to be unmodified, got %d\n", count);
    subjinfo.DigestAlgorithm.pszObjId = oid_sha1;
    count = 0xdeadbeef;
    ret = CryptSIPCreateIndirectData_p(&subjinfo, &count, NULL);
    todo_wine
    ok(ret, "CryptSIPCreateIndirectData failed: %d\n", GetLastError());
    ok(count, "expected a positive count\n");
    if (ret)
    {
        SIP_INDIRECT_DATA *indirect = HeapAlloc(GetProcessHeap(), 0, count);

        count = 256;
        ret = CryptSIPCreateIndirectData_p(&subjinfo, &count, indirect);
        ok(ret, "CryptSIPCreateIndirectData failed: %d\n", GetLastError());
        /* If the count is larger than needed, it's unmodified */
        ok(count == 256, "unexpected count %d\n", count);
        ok(!strcmp(indirect->Data.pszObjId, SPC_PE_IMAGE_DATA_OBJID),
           "unexpected data oid %s\n",
           indirect->Data.pszObjId);
        ok(!strcmp(indirect->DigestAlgorithm.pszObjId, oid_sha1),
           "unexpected digest algorithm oid %s\n",
           indirect->DigestAlgorithm.pszObjId);
        ok(indirect->Digest.cbData == 20, "unexpected hash size %d\n",
           indirect->Digest.cbData);
        if (indirect->Digest.cbData == 20)
        {
            const BYTE hash[20] = {
                0x8a,0xd5,0x45,0x53,0x3d,0x67,0xdf,0x2f,0x78,0xe0,
                0x55,0x0a,0xe0,0xd9,0x7a,0x28,0x3e,0xbf,0x45,0x2b };

            ok(!memcmp(indirect->Digest.pbData, hash, 20),
               "unexpected value\n");
        }

        HeapFree(GetProcessHeap(), 0, indirect);
    }
    CloseHandle(file);
    DeleteFileW(temp_file);
}

static void test_wintrust(void)
{
    static GUID generic_action_v2 = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA wtd;
    WINTRUST_FILE_INFO file;
    LONG r;
    HRESULT hr;
    WCHAR pathW[MAX_PATH];

    memset(&wtd, 0, sizeof(wtd));
    wtd.cbStruct = sizeof(wtd);
    wtd.dwUIChoice = WTD_UI_NONE;
    wtd.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
    wtd.dwUnionChoice = WTD_CHOICE_FILE;
    U(wtd).pFile = &file;
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    memset(&file, 0, sizeof(file));
    file.cbStruct = sizeof(file);
    file.pcwszFilePath = pathW;
    /* Test with an empty file */
    file.hFile = create_temp_file(pathW);
    r = WinVerifyTrust(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(r == TRUST_E_SUBJECT_FORM_UNKNOWN,
     "expected TRUST_E_SUBJECT_FORM_UNKNOWN, got %08x\n", r);
    CloseHandle(file.hFile);
    DeleteFileW(pathW);
    file.hFile = NULL;
    /* Test with a known file path, which we expect not have a signature */
    getNotepadPath(pathW, MAX_PATH);
    r = WinVerifyTrust(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(r == TRUST_E_NOSIGNATURE || r == CRYPT_E_FILE_ERROR,
     "expected TRUST_E_NOSIGNATURE or CRYPT_E_FILE_ERROR, got %08x\n", r);
    wtd.dwStateAction = WTD_STATEACTION_CLOSE;
    r = WinVerifyTrust(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(r == S_OK, "WinVerifyTrust failed: %08x\n", r);
    wtd.dwStateAction = WTD_STATEACTION_VERIFY;
    hr = WinVerifyTrustEx(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(hr == TRUST_E_NOSIGNATURE || hr == CRYPT_E_FILE_ERROR,
     "expected TRUST_E_NOSIGNATURE or CRYPT_E_FILE_ERROR, got %08x\n", hr);
    wtd.dwStateAction = WTD_STATEACTION_CLOSE;
    r = WinVerifyTrust(INVALID_HANDLE_VALUE, &generic_action_v2, &wtd);
    ok(r == S_OK, "WinVerifyTrust failed: %08x\n", r);
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
    test_sip_create_indirect_data();
    test_wintrust();
    test_get_known_usages();
}
