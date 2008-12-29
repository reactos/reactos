/*
 * crypt32 CRL functions tests
 *
 * Copyright 2005-2006 Juan Lang
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
#include <winreg.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"


static const BYTE bigCert[] = { 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22,
 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30,
 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20,
 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01,
 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE bigCert2[] = { 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22,
 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30,
 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20,
 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01,
 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE bigCertWithDifferentIssuer[] = { 0x30, 0x7a, 0x02, 0x01,
 0x01, 0x30, 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61, 0x6e,
 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30,
 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02,
 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03,
 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff,
 0x02, 0x01, 0x01 };
static const BYTE CRL[] = { 0x30, 0x2c, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a,
 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f, 0x31,
 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x5a };
static const BYTE newerCRL[] = { 0x30, 0x2a, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x17, 0x0d, 0x30, 0x36,
 0x30, 0x35, 0x31, 0x36, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE signedCRL[] = { 0x30, 0x45, 0x30, 0x2c, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x02, 0x06, 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c,
 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

static BOOL (WINAPI *pCertFindCertificateInCRL)(PCCERT_CONTEXT,PCCRL_CONTEXT,DWORD,void*,PCRL_ENTRY*);
static PCCRL_CONTEXT (WINAPI *pCertFindCRLInStore)(HCERTSTORE,DWORD,DWORD,DWORD,const void*,PCCRL_CONTEXT);
static BOOL (WINAPI *pCertIsValidCRLForCertificate)(PCCERT_CONTEXT, PCCRL_CONTEXT, DWORD, void*);

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("crypt32.dll");
    pCertFindCertificateInCRL = (void*)GetProcAddress(hdll, "CertFindCertificateInCRL");
    pCertFindCRLInStore = (void*)GetProcAddress(hdll, "CertFindCRLInStore");
    pCertIsValidCRLForCertificate = (void*)GetProcAddress(hdll, "CertIsValidCRLForCertificate");
}

static void testCreateCRL(void)
{
    PCCRL_CONTEXT context;
    DWORD GLE;

    context = CertCreateCRLContext(0, NULL, 0);
    ok(!context && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    context = CertCreateCRLContext(X509_ASN_ENCODING, NULL, 0);
    GLE = GetLastError();
    ok(!context && (GLE == CRYPT_E_ASN1_EOD || GLE == OSS_MORE_INPUT),
     "Expected CRYPT_E_ASN1_EOD or OSS_MORE_INPUT, got %08x\n", GLE);
    context = CertCreateCRLContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    GLE = GetLastError();
    ok(!context, "Expected failure\n");
    context = CertCreateCRLContext(X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL) - 1);
    ok(!context, "Expected failure\n");
    context = CertCreateCRLContext(X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL));
    ok(context != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);
    context = CertCreateCRLContext(X509_ASN_ENCODING, CRL, sizeof(CRL));
    ok(context != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);
}

static void testAddCRL(void)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    PCCRL_CONTEXT context;
    BOOL ret;
    DWORD GLE;

    if (!store) return;

    /* Bad CRL encoding type */
    ret = CertAddEncodedCRLToStore(0, 0, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddEncodedCRLToStore(0, 0, signedCRL, sizeof(signedCRL), 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, signedCRL, sizeof(signedCRL), 0,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddEncodedCRLToStore(0, 0, signedCRL, sizeof(signedCRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, signedCRL, sizeof(signedCRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());

    /* No CRL */
    ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, NULL, 0, 0, NULL);
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_EOD || GLE == OSS_MORE_INPUT),
     "Expected CRYPT_E_ASN1_EOD or OSS_MORE_INPUT, got %08x\n", GLE);
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, NULL, 0, 0, NULL);
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_EOD || GLE == OSS_MORE_INPUT),
     "Expected CRYPT_E_ASN1_EOD or OSS_MORE_INPUT, got %08x\n", GLE);

    /* Weird--bad add disposition leads to an access violation in Windows.
     * Both tests crash on some win9x boxes.
     */
    if (0)
    {
        ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, signedCRL,
         sizeof(signedCRL), 0, NULL);
        ok(!ret && (GetLastError() == STATUS_ACCESS_VIOLATION ||
                    GetLastError() == E_INVALIDARG /* Vista */),
         "Expected STATUS_ACCESS_VIOLATION or E_INVALIDARG, got %08x\n", GetLastError());
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
         sizeof(signedCRL), 0, NULL);
        ok(!ret && (GetLastError() == STATUS_ACCESS_VIOLATION ||
                    GetLastError() == E_INVALIDARG /* Vista */),
         "Expected STATUS_ACCESS_VIOLATION or E_INVALIDARG, got %08x\n", GetLastError());
    }

    /* Weird--can add a CRL to the NULL store (does this have special meaning?)
     */
    context = NULL;
    ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);

    /* Normal cases: a "signed" CRL is okay.. */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    /* and an unsigned one is too. */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, CRL, sizeof(CRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, newerCRL,
     sizeof(newerCRL), CERT_STORE_ADD_NEW, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08x\n", GetLastError());

    /* This should replace (one of) the existing CRL(s). */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, newerCRL,
     sizeof(newerCRL), CERT_STORE_ADD_NEWER, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());

    CertCloseStore(store, 0);
}

static void testFindCRL(void)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    PCCRL_CONTEXT context;
    PCCERT_CONTEXT cert;
    BOOL ret;

    if (!store) return;
    if (!pCertFindCRLInStore)
    {
        skip("CertFindCRLInStore() is not available\n");
        return;
    }

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());

    /* Crashes
    context = pCertFindCRLInStore(NULL, 0, 0, 0, NULL, NULL);
     */

    /* Find any context */
    context = pCertFindCRLInStore(store, 0, 0, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    /* Bogus flags are ignored */
    context = pCertFindCRLInStore(store, 0, 1234, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    /* CRL encoding type is ignored too */
    context = pCertFindCRLInStore(store, 1234, 0, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);

    /* This appears to match any cert */
    context = pCertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);

    /* Try to match an issuer that isn't in the store */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2,
     sizeof(bigCert2));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    context = pCertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, cert, NULL);
    ok(context == NULL, "Expected no matching context\n");
    CertFreeCertificateContext(cert);

    /* Match an issuer that is in the store */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    context = pCertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, cert, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    CertFreeCertificateContext(cert);

    CertCloseStore(store, 0);
}

static void testGetCRLFromStore(void)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    PCCRL_CONTEXT context;
    PCCERT_CONTEXT cert;
    DWORD flags;
    BOOL ret;

    if (!store) return;

    /* Crash
    context = CertGetCRLFromStore(NULL, NULL, NULL, NULL);
    context = CertGetCRLFromStore(store, NULL, NULL, NULL);
     */

    /* Bogus flags */
    flags = 0xffffffff;
    context = CertGetCRLFromStore(store, NULL, NULL, &flags);
    ok(!context && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());

    /* Test an empty store */
    flags = 0;
    context = CertGetCRLFromStore(store, NULL, NULL, &flags);
    ok(context == NULL && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());

    /* NULL matches any CRL */
    flags = 0;
    context = CertGetCRLFromStore(store, NULL, NULL, &flags);
    ok(context != NULL, "Expected a context\n");
    CertFreeCRLContext(context);

    /* This cert's issuer isn't in */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2,
     sizeof(bigCert2));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    context = CertGetCRLFromStore(store, cert, NULL, &flags);
    ok(context == NULL && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertFreeCertificateContext(cert);

    /* But this one is */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    context = CertGetCRLFromStore(store, cert, NULL, &flags);
    ok(context != NULL, "Expected a context\n");
    CertFreeCRLContext(context);
    CertFreeCertificateContext(cert);

    CertCloseStore(store, 0);
}

static void checkCRLHash(const BYTE *data, DWORD dataLen, ALG_ID algID,
 PCCRL_CONTEXT context, DWORD propID)
{
    BYTE hash[20] = { 0 }, hashProperty[20];
    BOOL ret;
    DWORD size;

    memset(hash, 0, sizeof(hash));
    memset(hashProperty, 0, sizeof(hashProperty));
    size = sizeof(hash);
    ret = CryptHashCertificate(0, algID, 0, data, dataLen, hash, &size);
    ok(ret, "CryptHashCertificate failed: %08x\n", GetLastError());
    ret = CertGetCRLContextProperty(context, propID, hashProperty, &size);
    ok(ret, "CertGetCRLContextProperty failed: %08x\n", GetLastError());
    ok(!memcmp(hash, hashProperty, size), "Unexpected hash for property %d\n",
     propID);
}

static void testCRLProperties(void)
{
    PCCRL_CONTEXT context = CertCreateCRLContext(X509_ASN_ENCODING,
     CRL, sizeof(CRL));

    ok(context != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());
    if (context)
    {
        DWORD propID, numProps, access, size;
        BOOL ret;
        BYTE hash[20] = { 0 }, hashProperty[20];
        CRYPT_DATA_BLOB blob;

        /* This crashes
        propID = CertEnumCRLContextProperties(NULL, 0);
         */

        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCRLContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 0, "Expected 0 properties, got %d\n", numProps);

        /* Tests with a NULL cert context.  Prop ID 0 fails.. */
        ret = CertSetCRLContextProperty(NULL, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
        /* while this just crashes.
        ret = CertSetCRLContextProperty(NULL, CERT_KEY_PROV_HANDLE_PROP_ID, 0,
         NULL);
         */

        ret = CertSetCRLContextProperty(context, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
        /* Can't set the cert property directly, this crashes.
        ret = CertSetCRLContextProperty(context, CERT_CRL_PROP_ID, 0, CRL);
         */

        /* These all crash.
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID, 0,
         NULL);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID, NULL, NULL);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID, 
         hashProperty, NULL);
         */
        /* A missing prop */
        size = 0;
        ret = CertGetCRLContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID,
         NULL, &size);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
        /* And, an implicit property */
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID,
         NULL, &size);
        ok(ret, "CertGetCRLContextProperty failed: %08x\n", GetLastError());
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID,
         &access, &size);
        ok(ret, "CertGetCRLContextProperty failed: %08x\n", GetLastError());
        ok(!(access & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG),
         "Didn't expect a persisted crl\n");
        /* Trying to set this "read only" property crashes.
        access |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
        ret = CertSetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID, 0,
         &access);
         */

        /* Can I set the hash to an invalid hash? */
        blob.pbData = hash;
        blob.cbData = sizeof(hash);
        ret = CertSetCRLContextProperty(context, CERT_HASH_PROP_ID, 0, &blob);
        ok(ret, "CertSetCRLContextProperty failed: %08x\n",
         GetLastError());
        size = sizeof(hashProperty);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID,
         hashProperty, &size);
        ok(!memcmp(hashProperty, hash, sizeof(hash)), "Unexpected hash\n");
        /* Delete the (bogus) hash, and get the real one */
        ret = CertSetCRLContextProperty(context, CERT_HASH_PROP_ID, 0, NULL);
        ok(ret, "CertSetCRLContextProperty failed: %08x\n", GetLastError());
        checkCRLHash(CRL, sizeof(CRL), CALG_SHA1, context, CERT_HASH_PROP_ID);

        /* Now that the hash property is set, we should get one property when
         * enumerating.
         */
        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCRLContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 1, "Expected 1 properties, got %d\n", numProps);

        /* Check a few other implicit properties */
        checkCRLHash(CRL, sizeof(CRL), CALG_MD5, context,
         CERT_MD5_HASH_PROP_ID);

        CertFreeCRLContext(context);
    }
}

static const BYTE v1CRLWithIssuerAndEntry[] = { 0x30, 0x44, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x16, 0x30, 0x14, 0x02, 0x01, 0x01, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v2CRLWithIssuingDistPoint[] = { 0x30,0x5c,0x02,0x01,0x01,
 0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
 0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,
 0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,
 0x16,0x30,0x14,0x02,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
 0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0xa0,0x13,0x30,0x11,0x30,0x0f,0x06,
 0x03,0x55,0x1d,0x13,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE verisignCRL[] = { 0x30, 0x82, 0x01, 0xb1, 0x30, 0x82, 0x01,
 0x1a, 0x02, 0x01, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x30, 0x61, 0x31, 0x11, 0x30, 0x0f, 0x06,
 0x03, 0x55, 0x04, 0x07, 0x13, 0x08, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65,
 0x74, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0e, 0x56,
 0x65, 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e,
 0x31, 0x33, 0x30, 0x31, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x2a, 0x56, 0x65,
 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x20, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x72,
 0x63, 0x69, 0x61, 0x6c, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65,
 0x20, 0x50, 0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x65, 0x72, 0x73, 0x20, 0x43,
 0x41, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x33, 0x32, 0x34, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x30, 0x34, 0x30, 0x31, 0x30, 0x37, 0x32, 0x33,
 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x69, 0x30, 0x21, 0x02, 0x10, 0x1b, 0x51,
 0x90, 0xf7, 0x37, 0x24, 0x39, 0x9c, 0x92, 0x54, 0xcd, 0x42, 0x46, 0x37, 0x99,
 0x6a, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x31, 0x33, 0x30, 0x30, 0x30, 0x30, 0x31,
 0x32, 0x34, 0x5a, 0x30, 0x21, 0x02, 0x10, 0x75, 0x0e, 0x40, 0xff, 0x97, 0xf0,
 0x47, 0xed, 0xf5, 0x56, 0xc7, 0x08, 0x4e, 0xb1, 0xab, 0xfd, 0x17, 0x0d, 0x30,
 0x31, 0x30, 0x31, 0x33, 0x31, 0x30, 0x30, 0x30, 0x30, 0x34, 0x39, 0x5a, 0x30,
 0x21, 0x02, 0x10, 0x77, 0xe6, 0x5a, 0x43, 0x59, 0x93, 0x5d, 0x5f, 0x7a, 0x75,
 0x80, 0x1a, 0xcd, 0xad, 0xc2, 0x22, 0x17, 0x0d, 0x30, 0x30, 0x30, 0x38, 0x33,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x35, 0x36, 0x5a, 0xa0, 0x1a, 0x30, 0x18, 0x30,
 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0b, 0x06,
 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03, 0x02, 0x05, 0xa0, 0x30, 0x0d, 0x06,
 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x03,
 0x81, 0x81, 0x00, 0x18, 0x2c, 0xe8, 0xfc, 0x16, 0x6d, 0x91, 0x4a, 0x3d, 0x88,
 0x54, 0x48, 0x5d, 0xb8, 0x11, 0xbf, 0x64, 0xbb, 0xf9, 0xda, 0x59, 0x19, 0xdd,
 0x0e, 0x65, 0xab, 0xc0, 0x0c, 0xfa, 0x67, 0x7e, 0x21, 0x1e, 0x83, 0x0e, 0xcf,
 0x9b, 0x89, 0x8a, 0xcf, 0x0c, 0x4b, 0xc1, 0x39, 0x9d, 0xe7, 0x6a, 0xac, 0x46,
 0x74, 0x6a, 0x91, 0x62, 0x22, 0x0d, 0xc4, 0x08, 0xbd, 0xf5, 0x0a, 0x90, 0x7f,
 0x06, 0x21, 0x3d, 0x7e, 0xa7, 0xaa, 0x5e, 0xcd, 0x22, 0x15, 0xe6, 0x0c, 0x75,
 0x8e, 0x6e, 0xad, 0xf1, 0x84, 0xe4, 0x22, 0xb4, 0x30, 0x6f, 0xfb, 0x64, 0x8f,
 0xd7, 0x80, 0x43, 0xf5, 0x19, 0x18, 0x66, 0x1d, 0x72, 0xa3, 0xe3, 0x94, 0x82,
 0x28, 0x52, 0xa0, 0x06, 0x4e, 0xb1, 0xc8, 0x92, 0x0c, 0x97, 0xbe, 0x15, 0x07,
 0xab, 0x7a, 0xc9, 0xea, 0x08, 0x67, 0x43, 0x4d, 0x51, 0x63, 0x3b, 0x9c, 0x9c,
 0xcd };

static void testIsValidCRLForCert(void)
{
    BOOL ret;
    PCCERT_CONTEXT cert1, cert2;
    PCCRL_CONTEXT crl;
    HCERTSTORE store;

    if(!pCertIsValidCRLForCertificate) return;

    crl = CertCreateCRLContext(X509_ASN_ENCODING, v1CRLWithIssuerAndEntry,
     sizeof(v1CRLWithIssuerAndEntry));
    ok(crl != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());
    cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(cert1 != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());

    /* Crash
    ret = CertIsValidCRLForCertificate(NULL, NULL, 0, NULL);
    ret = CertIsValidCRLForCertificate(cert1, NULL, 0, NULL);
     */

    /* Curiously, any CRL is valid for the NULL certificate */
    ret = pCertIsValidCRLForCertificate(NULL, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    /* Same issuer for both cert and CRL, this CRL is valid for that cert */
    ret = pCertIsValidCRLForCertificate(cert1, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    cert2 = CertCreateCertificateContext(X509_ASN_ENCODING,
     bigCertWithDifferentIssuer, sizeof(bigCertWithDifferentIssuer));
    ok(cert2 != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());

    /* Yet more curious: different issuers for these, yet the CRL is valid for
     * that cert.  According to MSDN, the relevant bit to check is whether the
     * CRL has a CRL_ISSUING_DIST_POINT extension.
     */
    ret = pCertIsValidCRLForCertificate(cert2, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    CertFreeCRLContext(crl);

    /* Yet with a CRL_ISSUING_DIST_POINT in the CRL, I still can't get this
     * to say the CRL is not valid for either cert.
     */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, v2CRLWithIssuingDistPoint,
     sizeof(v2CRLWithIssuingDistPoint));
    ok(crl != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());

    ret = pCertIsValidCRLForCertificate(cert1, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());
    ret = pCertIsValidCRLForCertificate(cert2, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    CertFreeCRLContext(crl);

    /* And again, with a real CRL, the CRL is valid for both certs. */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, verisignCRL,
     sizeof(verisignCRL));
    ok(crl != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());

    ret = pCertIsValidCRLForCertificate(cert1, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());
    ret = pCertIsValidCRLForCertificate(cert2, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    CertFreeCRLContext(crl);

    /* One last test: a CRL in a different store than the cert is also valid
     * for the cert, so CertIsValidCRLForCertificate must always return TRUE?
     */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, verisignCRL,
     sizeof(verisignCRL), CERT_STORE_ADD_ALWAYS, &crl);
    ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());

    ret = pCertIsValidCRLForCertificate(cert1, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());
    ret = pCertIsValidCRLForCertificate(cert2, crl, 0, NULL);
    ok(ret, "CertIsValidCRLForCertificate failed: %08x\n", GetLastError());

    CertFreeCRLContext(crl);

    CertCloseStore(store, 0);

    CertFreeCertificateContext(cert2);
    CertFreeCertificateContext(cert1);
}

static const BYTE crlWithDifferentIssuer[] = {
 0x30,0x47,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,
 0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x41,0x6c,0x65,0x78,0x20,0x4c,0x61,0x6e,
 0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,
 0x30,0x30,0x30,0x5a,0x30,0x16,0x30,0x14,0x02,0x01,0x01,0x18,0x0f,0x31,0x36,
 0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a };

static void testFindCertInCRL(void)
{
    BOOL ret;
    PCCERT_CONTEXT cert;
    PCCRL_CONTEXT crl;
    PCRL_ENTRY entry;

    if (!pCertFindCertificateInCRL)
    {
        skip("CertFindCertificateInCRL() is not available\n");
        return;
    }

    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());

    /* Crash
    ret = pCertFindCertificateInCRL(NULL, NULL, 0, NULL, NULL);
    ret = pCertFindCertificateInCRL(NULL, crl, 0, NULL, NULL);
    ret = pCertFindCertificateInCRL(cert, NULL, 0, NULL, NULL);
    ret = pCertFindCertificateInCRL(cert, crl, 0, NULL, NULL);
    ret = pCertFindCertificateInCRL(NULL, NULL, 0, NULL, &entry);
    ret = pCertFindCertificateInCRL(NULL, crl, 0, NULL, &entry);
    ret = pCertFindCertificateInCRL(cert, NULL, 0, NULL, &entry);
     */

    crl = CertCreateCRLContext(X509_ASN_ENCODING, verisignCRL,
     sizeof(verisignCRL));
    ret = pCertFindCertificateInCRL(cert, crl, 0, NULL, &entry);
    ok(ret, "CertFindCertificateInCRL failed: %08x\n", GetLastError());
    ok(entry == NULL, "Expected not to find an entry in CRL\n");
    CertFreeCRLContext(crl);

    crl = CertCreateCRLContext(X509_ASN_ENCODING, v1CRLWithIssuerAndEntry,
     sizeof(v1CRLWithIssuerAndEntry));
    ret = pCertFindCertificateInCRL(cert, crl, 0, NULL, &entry);
    ok(ret, "CertFindCertificateInCRL failed: %08x\n", GetLastError());
    ok(entry != NULL, "Expected to find an entry in CRL\n");
    CertFreeCRLContext(crl);

    /* Entry found even though CRL issuer doesn't match cert issuer */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, crlWithDifferentIssuer,
     sizeof(crlWithDifferentIssuer));
    ret = pCertFindCertificateInCRL(cert, crl, 0, NULL, &entry);
    ok(ret, "CertFindCertificateInCRL failed: %08x\n", GetLastError());
    ok(entry != NULL, "Expected to find an entry in CRL\n");
    CertFreeCRLContext(crl);

    CertFreeCertificateContext(cert);
}

static void testVerifyCRLRevocation(void)
{
    BOOL ret;
    PCCERT_CONTEXT cert;
    PCCRL_CONTEXT crl;

    ret = CertVerifyCRLRevocation(0, NULL, 0, NULL);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());
    ret = CertVerifyCRLRevocation(X509_ASN_ENCODING, NULL, 0, NULL);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());

    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));

    /* Check against no CRL */
    ret = CertVerifyCRLRevocation(0, cert->pCertInfo, 0, NULL);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());
    ret = CertVerifyCRLRevocation(X509_ASN_ENCODING, cert->pCertInfo, 0, NULL);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());

    /* Check against CRL with entry for the cert */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, v1CRLWithIssuerAndEntry,
     sizeof(v1CRLWithIssuerAndEntry));
    ret = CertVerifyCRLRevocation(0, cert->pCertInfo, 1,
     (PCRL_INFO *)&crl->pCrlInfo);
    ok(!ret, "CertVerifyCRLRevocation should have been revoked\n");
    ret = CertVerifyCRLRevocation(X509_ASN_ENCODING, cert->pCertInfo, 1,
     (PCRL_INFO *)&crl->pCrlInfo);
    ok(!ret, "CertVerifyCRLRevocation should have been revoked\n");
    CertFreeCRLContext(crl);

    /* Check against CRL with different issuer and entry for the cert */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, v1CRLWithIssuerAndEntry,
     sizeof(v1CRLWithIssuerAndEntry));
    ok(crl != NULL, "CertCreateCRLContext failed: %08x\n", GetLastError());
    ret = CertVerifyCRLRevocation(X509_ASN_ENCODING, cert->pCertInfo, 1,
     (PCRL_INFO *)&crl->pCrlInfo);
    ok(!ret, "CertVerifyCRLRevocation should have been revoked\n");
    CertFreeCRLContext(crl);

    /* Check against CRL without entry for the cert */
    crl = CertCreateCRLContext(X509_ASN_ENCODING, verisignCRL,
     sizeof(verisignCRL));
    ret = CertVerifyCRLRevocation(0, cert->pCertInfo, 1,
     (PCRL_INFO *)&crl->pCrlInfo);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());
    ret = CertVerifyCRLRevocation(X509_ASN_ENCODING, cert->pCertInfo, 1,
     (PCRL_INFO *)&crl->pCrlInfo);
    ok(ret, "CertVerifyCRLRevocation failed: %08x\n", GetLastError());
    CertFreeCRLContext(crl);

    CertFreeCertificateContext(cert);
}

START_TEST(crl)
{
    init_function_pointers();

    testCreateCRL();
    testAddCRL();
    testFindCRL();
    testGetCRLFromStore();

    testCRLProperties();

    testIsValidCRLForCert();
    testFindCertInCRL();
    testVerifyCRLRevocation();
}
