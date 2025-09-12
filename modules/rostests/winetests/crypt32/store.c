/*
 * crypt32 cert store function tests
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

#include <stdio.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winreg.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

struct CertPropIDHeader
{
    DWORD propID;
    DWORD unknown1;
    DWORD cb;
};

static const BYTE emptyCert[] = { 0x30, 0x00 };
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
static const BYTE signedBigCert[] = {
 0x30, 0x81, 0x93, 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3,
 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07,
 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };
static const BYTE serializedCert[] = { 0x20, 0x00, 0x00, 0x00,
 0x01, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x30, 0x7a, 0x02, 0x01, 0x01,
 0x30, 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67,
 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15,
 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75,
 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02,
 0x01, 0x01 };
static const BYTE signedCRL[] = { 0x30, 0x45, 0x30, 0x2c, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x02, 0x06, 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c,
 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };
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
static const BYTE signedCTLWithCTLInnerContent[] = {
0x30,0x82,0x01,0x0f,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,
0xa0,0x82,0x01,0x00,0x30,0x81,0xfd,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,
0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x30,0x06,0x09,
0x2b,0x06,0x01,0x04,0x01,0x82,0x37,0x0a,0x01,0xa0,0x23,0x30,0x21,0x30,0x00,
0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
0x30,0x5a,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,
0x00,0x31,0x81,0xb5,0x30,0x81,0xb2,0x02,0x01,0x01,0x30,0x1a,0x30,0x15,0x31,
0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,
0x4c,0x61,0x6e,0x67,0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,
0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0xa0,0x3b,0x30,0x18,0x06,0x09,0x2a,0x86,
0x48,0x86,0xf7,0x0d,0x01,0x09,0x03,0x31,0x0b,0x06,0x09,0x2b,0x06,0x01,0x04,
0x01,0x82,0x37,0x0a,0x01,0x30,0x1f,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x01,0x09,0x04,0x31,0x12,0x04,0x10,0x54,0x71,0xbc,0xe1,0x56,0x31,0xa2,0xf9,
0x65,0x70,0x34,0xf8,0xe2,0xe9,0xb4,0xf4,0x30,0x04,0x06,0x00,0x05,0x00,0x04,
0x40,0x2f,0x1b,0x9f,0x5a,0x4a,0x15,0x73,0xfa,0xb1,0x93,0x3d,0x09,0x52,0xdf,
0x6b,0x98,0x4b,0x13,0x5e,0xe7,0xbf,0x65,0xf4,0x9c,0xc2,0xb1,0x77,0x09,0xb1,
0x66,0x4d,0x72,0x0d,0xb1,0x1a,0x50,0x20,0xe0,0x57,0xa2,0x39,0xc7,0xcd,0x7f,
0x8e,0xe7,0x5f,0x76,0x2b,0xd1,0x6a,0x82,0xb3,0x30,0x25,0x61,0xf6,0x25,0x23,
0x57,0x6c,0x0b,0x47,0xb8 };

#define test_store_is_empty(store) _test_store_is_empty(__LINE__,store)
static void _test_store_is_empty(unsigned line, HCERTSTORE store)
{
    const CERT_CONTEXT *cert;

    cert = CertEnumCertificatesInStore(store, NULL);
    ok_(__FILE__,line)(!cert && GetLastError() == CRYPT_E_NOT_FOUND, "store is not empty\n");
}

static void testMemStore(void)
{
    HCERTSTORE store1, store2;
    PCCERT_CONTEXT context;
    BOOL ret;
    DWORD GLE;

    /* NULL provider */
    store1 = CertOpenStore(0, 0, 0, 0, NULL);
    ok(!store1 && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* weird flags */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_DELETE_FLAG, NULL);
    ok(!store1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %d\n", GetLastError());

    /* normal */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %d\n", GetLastError());
    /* open existing doesn't */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, NULL);
    ok(store2 != NULL, "CertOpenStore failed: %d\n", GetLastError());
    ok(store1 != store2, "Expected different stores\n");

    /* add a bogus (empty) cert */
    context = NULL;
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, emptyCert,
     sizeof(emptyCert), CERT_STORE_ADD_ALWAYS, &context);
    /* Windows returns CRYPT_E_ASN1_EOD or OSS_DATA_ERROR, but accept
     * CRYPT_E_ASN1_CORRUPT as well (because matching errors is tough in this
     * case)
     */
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_EOD || GLE == CRYPT_E_ASN1_CORRUPT ||
     GLE == OSS_DATA_ERROR),
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GLE);
    /* add a "signed" cert--the signature isn't a real signature, so this adds
     * without any check of the signature's validity
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedBigCert, sizeof(signedBigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(signedBigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, signedBigCert,
         sizeof(signedBigCert)), "Unexpected encoded cert in context\n");
        /* remove it, the rest of the tests will work on an unsigned cert */
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
    }
    /* try adding a "signed" CRL as a cert */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_BADTAG || GLE == CRYPT_E_ASN1_CORRUPT ||
     GLE == OSS_DATA_ERROR),
     "Expected CRYPT_E_ASN1_BADTAG or CRYPT_E_ASN1_CORRUPT or OSS_DATA_ERROR, got %08x\n",
     GLE);
    /* add a cert to store1 */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        DWORD size;
        BYTE *buf;

        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");

        /* check serializing this element */
        /* These crash
        ret = CertSerializeCertificateStoreElement(NULL, 0, NULL, NULL);
        ret = CertSerializeCertificateStoreElement(context, 0, NULL, NULL);
        ret = CertSerializeCertificateStoreElement(NULL, 0, NULL, &size);
         */
        /* apparently flags are ignored */
        ret = CertSerializeCertificateStoreElement(context, 1, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(context, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n", GetLastError());
            ok(size == sizeof(serializedCert), "Wrong size %d\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }

        ret = CertFreeCertificateContext(context);
        ok(ret, "CertFreeCertificateContext failed: %08x\n", GetLastError());
    }
    /* verify the cert's in store1 */
    context = CertEnumCertificatesInStore(store1, NULL);
    ok(context != NULL, "Expected a valid context\n");
    context = CertEnumCertificatesInStore(store1, context);
    ok(!context && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    /* verify store2 (the "open existing" mem store) is still empty */
    context = CertEnumCertificatesInStore(store2, NULL);
    ok(!context, "Expected an empty store\n");
    /* delete the cert from store1, and check it's empty */
    context = CertEnumCertificatesInStore(store1, NULL);
    if (context)
    {
        /* Deleting a bitwise copy crashes with an access to an uninitialized
         * pointer, so a cert context has some special data out there in memory
         * someplace
        CERT_CONTEXT copy;
        memcpy(&copy, context, sizeof(copy));
        ret = CertDeleteCertificateFromStore(&copy);
         */
        PCCERT_CONTEXT copy = CertDuplicateCertificateContext(context);

        ok(copy != NULL, "CertDuplicateCertificateContext failed: %08x\n",
         GetLastError());
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
        /* try deleting a copy */
        ret = CertDeleteCertificateFromStore(copy);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
        /* check that the store is empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Expected an empty store\n");
    }

    /* close an empty store */
    ret = CertCloseStore(NULL, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());
    ret = CertCloseStore(store1, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());
    ret = CertCloseStore(store2, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());

    /* This seems nonsensical, but you can open a read-only mem store, only
     * it isn't read-only
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_READONLY_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %d\n", GetLastError());
    /* yep, this succeeds */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
    }
    CertCloseStore(store1, 0);
}

static void compareStore(HCERTSTORE store, LPCSTR name, const BYTE *pb,
 DWORD cb, BOOL todo)
{
    BOOL ret;
    CRYPT_DATA_BLOB blob = { 0, NULL };

    ret = CertSaveStore(store, X509_ASN_ENCODING, CERT_STORE_SAVE_AS_STORE,
     CERT_STORE_SAVE_TO_MEMORY, &blob, 0);
    ok(ret, "CertSaveStore failed: %08x\n", GetLastError());
    todo_wine_if (todo)
        ok(blob.cbData == cb, "%s: expected size %d, got %d\n", name, cb,
         blob.cbData);
    blob.pbData = HeapAlloc(GetProcessHeap(), 0, blob.cbData);
    if (blob.pbData)
    {
        ret = CertSaveStore(store, X509_ASN_ENCODING, CERT_STORE_SAVE_AS_STORE,
         CERT_STORE_SAVE_TO_MEMORY, &blob, 0);
        ok(ret, "CertSaveStore failed: %08x\n", GetLastError());
        todo_wine_if (todo)
            ok(!memcmp(pb, blob.pbData, cb), "%s: unexpected value\n", name);
        HeapFree(GetProcessHeap(), 0, blob.pbData);
    }
}

static const BYTE serializedStoreWithCert[] = {
 0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x20,0x00,0x00,0x00,0x01,0x00,0x00,
 0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,
 0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,
 0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,
 0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00 };

static const struct
{
    HKEY key;
    DWORD cert_store;
    BOOL appdata_file;
    WCHAR store_name[16];
    const WCHAR *base_reg_path;
} reg_store_saved_certs[] = {
    { HKEY_LOCAL_MACHINE, CERT_SYSTEM_STORE_LOCAL_MACHINE, FALSE,
        {'R','O','O','T',0}, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH },
    { HKEY_LOCAL_MACHINE, CERT_SYSTEM_STORE_LOCAL_MACHINE, FALSE,
        {'M','Y',0}, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH },
    { HKEY_LOCAL_MACHINE, CERT_SYSTEM_STORE_LOCAL_MACHINE, FALSE,
        {'C','A',0}, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH },
    /* Adding to HKCU\Root triggers safety warning. */
    { HKEY_CURRENT_USER, CERT_SYSTEM_STORE_CURRENT_USER, TRUE,
        {'M','Y',0}, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH },
    { HKEY_CURRENT_USER, CERT_SYSTEM_STORE_CURRENT_USER, FALSE,
        {'C','A',0}, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH }
};

/* Testing whether system stores are available for adding new certs
 * and checking directly in the registry whether they are actually saved or deleted.
 * Windows treats HKCU\My (at least) as a special case and uses AppData directory
 * for storing certs, not registry.
 */
static void testRegStoreSavedCerts(void)
{
    static const WCHAR fmt[] =
        { '%','s','\\','%','s','\\','%','s','\\','%','s',0},
    ms_certs[] =
        { 'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s',0},
    certs[] =
        {'C','e','r','t','i','f','i','c','a','t','e','s',0},
    bigCert_hash[] = {
        '6','E','3','0','9','0','7','1','5','F','D','9','2','3',
        '5','6','E','B','A','E','2','5','4','0','E','6','2','2',
        'D','A','1','9','2','6','0','2','A','6','0','8',0};
    PCCERT_CONTEXT cert1, cert2;
    HCERTSTORE store;
    HANDLE cert_file;
    HRESULT pathres;
    WCHAR key_name[MAX_PATH], appdata_path[MAX_PATH];
    HKEY key;
    BOOL ret;
    DWORD res,i;

    for (i = 0; i < ARRAY_SIZE(reg_store_saved_certs); i++)
    {
        DWORD err;

        store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
            reg_store_saved_certs[i].cert_store, reg_store_saved_certs[i].store_name);

        err = GetLastError();
        if (!store)
        {
            ok (err == ERROR_ACCESS_DENIED, "Failed to create store at %d (%08x)\n", i, err);
            skip("Insufficient privileges for the test %d\n", i);
            continue;
        }
        ok (store!=NULL, "Failed to open the store at %d, %x\n", i, GetLastError());
        cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert1 != NULL, "Create cert context failed at %d, %x\n", i, GetLastError());
        ret = CertAddCertificateContextToStore(store, cert1, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
        /* Addittional skip per Win7, it allows opening HKLM store, but disallows adding certs */
        err = GetLastError();
        if (!ret)
        {
            ok (err == ERROR_ACCESS_DENIED, "Failed to add certificate to store at %d (%08x)\n", i, err);
            skip("Insufficient privileges for the test %d\n", i);
            continue;
        }
        ok (ret, "Adding to the store failed at %d, %x\n", i, err);
        CertFreeCertificateContext(cert1);
        CertCloseStore(store, 0);

        wsprintfW(key_name, fmt, reg_store_saved_certs[i].base_reg_path,
            reg_store_saved_certs[i].store_name, certs, bigCert_hash);

        if (!reg_store_saved_certs[i].appdata_file)
        {
            res = RegOpenKeyExW(reg_store_saved_certs[i].key, key_name, 0, KEY_ALL_ACCESS, &key);
            ok (!res, "The cert hasn't been saved at %d, %x\n", i, GetLastError());
            if (!res) RegCloseKey(key);
        } else
        {
            pathres = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata_path);
            ok (pathres == S_OK,
                "Failed to get app data path at %d (%x)\n", pathres, GetLastError());
            if (pathres == S_OK)
            {
                PathAppendW(appdata_path, ms_certs);
                PathAppendW(appdata_path, reg_store_saved_certs[i].store_name);
                PathAppendW(appdata_path, certs);
                PathAppendW(appdata_path, bigCert_hash);

                cert_file = CreateFileW(appdata_path, GENERIC_READ, 0, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                todo_wine ok (cert_file != INVALID_HANDLE_VALUE,
                        "Cert was not saved in AppData at %d (%x)\n", i, GetLastError());
                CloseHandle(cert_file);
            }
        }

        /* deleting cert from store */
        store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
            reg_store_saved_certs[i].cert_store, reg_store_saved_certs[i].store_name);
        ok (store!=NULL, "Failed to open the store at %d, %x\n", i, GetLastError());

        cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert1 != NULL, "Create cert context failed at %d, %x\n", i, GetLastError());

        cert2 = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
            CERT_FIND_EXISTING, cert1, NULL);
        ok (cert2 != NULL, "Failed to find cert in the store at %d, %x\n", i, GetLastError());

        ret = CertDeleteCertificateFromStore(cert2);
        ok (ret, "Failed to delete certificate from store at %d, %x\n", i, GetLastError());

        CertFreeCertificateContext(cert1);
        CertFreeCertificateContext(cert2);
        CertCloseStore(store, 0);

        res = RegOpenKeyExW(reg_store_saved_certs[i].key, key_name, 0, KEY_ALL_ACCESS, &key);
        ok (res, "The cert's registry entry should be absent at %i, %x\n", i, GetLastError());
        if (!res) RegCloseKey(key);

        if (reg_store_saved_certs[i].appdata_file)
        {
            cert_file = CreateFileW(appdata_path, GENERIC_READ, 0, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            ok (cert_file == INVALID_HANDLE_VALUE,
                "Cert should have been absent in AppData %d\n", i);

            CloseHandle(cert_file);
        }
    }
}

/**
 * This test checks that certificate falls into correct store of a collection
 * depending on the access flags and priorities.
 */
static void testStoresInCollection(void)
{
    PCCERT_CONTEXT cert1, cert2, tcert1;
    HCERTSTORE collection, ro_store, rw_store, rw_store_2, tstore;
    static const WCHAR WineTestRO_W[] = { 'W','i','n','e','T','e','s','t','_','R','O',0 },
                       WineTestRW_W[] = { 'W','i','n','e','T','e','s','t','_','R','W',0 },
                       WineTestRW2_W[]= { 'W','i','n','e','T','e','s','t','_','R','W','2',0 };
    BOOL ret;

    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
        CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != NULL, "Failed to init collection store, last error %x\n", GetLastError());
    /* Add read-only store to collection with very high priority*/
    ro_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, WineTestRO_W);
    ok(ro_store != NULL, "Failed to init ro store %x\n", GetLastError());

    ret = CertAddStoreToCollection(collection, ro_store, 0, 1000);
    ok (ret, "Failed to add read-only store to collection %x\n", GetLastError());

    cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    ok (cert1 != NULL, "Create cert context failed %x\n", GetLastError());
    ret = CertAddCertificateContextToStore(collection, cert1, CERT_STORE_ADD_ALWAYS, NULL);
    ok (!ret, "Added cert to collection with single read-only store %x\n", GetLastError());

    /* Add read-write store to collection with the lowest priority*/
    rw_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, WineTestRW_W);
    ok (rw_store != NULL, "Failed to open rw store %x\n", GetLastError());
    ret = CertAddStoreToCollection(collection, rw_store, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok (ret, "Failed to add rw store to collection %x\n", GetLastError());
    /** Adding certificate to collection should fall into rw store,
     *  even though prioirty of the ro_store is higher */
    ret = CertAddCertificateContextToStore(collection, cert1, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
    ok (ret, "Failed to add cert to the collection %x\n", GetLastError());

    tcert1 = CertEnumCertificatesInStore(ro_store, NULL);
    ok (!tcert1, "Read-only ro_store contains cert\n");

    tcert1 = CertEnumCertificatesInStore(rw_store, NULL);
    ok (cert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "Unexpected cert in the rw store\n");
    CertFreeCertificateContext(tcert1);

    tcert1 = CertEnumCertificatesInStore(collection, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "Unexpected cert in the collection\n");
    CertFreeCertificateContext(tcert1);

    /** adding one more rw store with higher priority*/
    rw_store_2 = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, WineTestRW2_W);
    ok (rw_store_2 != NULL, "Failed to init second rw store %x\n", GetLastError());
    ret = CertAddStoreToCollection(collection, rw_store_2, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 50);
    ok (ret, "Failed to add rw_store_2 to collection %x\n",GetLastError());

    cert2 = CertCreateCertificateContext(X509_ASN_ENCODING, signedBigCert, sizeof(signedBigCert));
    ok (cert2 != NULL, "Failed to create cert context %x\n", GetLastError());
    ret = CertAddCertificateContextToStore(collection, cert2, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
    ok (ret, "Failed to add cert2 to the store %x\n",GetLastError());

    /** checking certificates in the stores */
    tcert1 = CertEnumCertificatesInStore(ro_store, 0);
    ok (tcert1 == NULL, "Read-only store not empty\n");

    tcert1 = CertEnumCertificatesInStore(rw_store, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "Unexpected cert in the rw_store\n");
    CertFreeCertificateContext(tcert1);

    tcert1 = CertEnumCertificatesInStore(rw_store_2, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert2->cbCertEncoded,
        "Unexpected cert in the rw_store_2\n");
    CertFreeCertificateContext(tcert1);

    /** checking certificates in the collection */
    tcert1 = CertEnumCertificatesInStore(collection, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert2->cbCertEncoded,
        "cert2 expected in the collection got %p, %x\n",tcert1, GetLastError());
    tcert1 = CertEnumCertificatesInStore(collection, tcert1);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "cert1 expected in the collection got %p, %x\n",tcert1, GetLastError());
    tcert1 = CertEnumCertificatesInStore(collection, tcert1);
    ok (tcert1==NULL,"Unexpected cert in the collection %p %x\n",tcert1, GetLastError());

    /* checking whether certs had been saved */
    tstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, WineTestRW_W);
    ok (tstore!=NULL, "Failed to open existing rw store\n");
    tcert1 = CertEnumCertificatesInStore(tstore, NULL);
    todo_wine
        ok(tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded, "cert1 wasn't saved\n");
    CertFreeCertificateContext(tcert1);
    CertCloseStore(tstore,0);

    tstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, WineTestRW2_W);
    ok (tstore!=NULL, "Failed to open existing rw2 store\n");
    tcert1 = CertEnumCertificatesInStore(tstore, NULL);
    todo_wine
        ok (tcert1 && tcert1->cbCertEncoded == cert2->cbCertEncoded, "cert2 wasn't saved\n");
    CertFreeCertificateContext(tcert1);
    CertCloseStore(tstore,0);

    CertCloseStore(collection,0);
    CertCloseStore(ro_store,0);
    CertCloseStore(rw_store,0);
    CertCloseStore(rw_store_2,0);

    /* reopening registry stores to check whether certs had been saved */
    rw_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_SYSTEM_STORE_CURRENT_USER, WineTestRW_W);
    tcert1 = CertEnumCertificatesInStore(rw_store, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "Unexpected cert in store %p\n", tcert1);
    CertFreeCertificateContext(tcert1);
    CertCloseStore(rw_store,0);

    rw_store_2 = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_SYSTEM_STORE_CURRENT_USER, WineTestRW2_W);
    tcert1 = CertEnumCertificatesInStore(rw_store_2, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert2->cbCertEncoded,
        "Unexpected cert in store %p\n", tcert1);
    CertFreeCertificateContext(tcert1);
    CertCloseStore(rw_store_2,0);

    CertFreeCertificateContext(cert1);
    CertFreeCertificateContext(cert2);
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER,WineTestRO_W);
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER,WineTestRW_W);
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER,WineTestRW2_W);

}

static void testCollectionStore(void)
{
    HCERTSTORE store1, store2, collection, collection2;
    PCCERT_CONTEXT context;
    BOOL ret;
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    HANDLE file;

    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    /* Try adding a cert to any empty collection */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_ACCESSDENIED,
     "Expected E_ACCESSDENIED, got %08x\n", GetLastError());

    /* Create and add a cert to a memory store */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    /* Add the memory store to the collection, without allowing adding */
    ret = CertAddStoreToCollection(collection, store1, 0, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* Verify the cert is in the collection */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        CertFreeCertificateContext(context);
    }
    /* Check that adding to the collection isn't allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_ACCESSDENIED,
     "Expected E_ACCESSDENIED, got %08x\n", GetLastError());

    /* Create a new memory store */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Try adding a store to a non-collection store */
    ret = CertAddStoreToCollection(store1, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Try adding some bogus stores */
    /* This crashes in Windows
    ret = pCertAddStoreToCollection(0, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */
    /* This "succeeds"... */
    ret = CertAddStoreToCollection(collection, 0,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* while this crashes.
    ret = pCertAddStoreToCollection(collection, 1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */

    /* Add it to the collection, this time allowing adding */
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* Check that adding to the collection is allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    /* Now check that it was actually added to store2 */
    context = CertEnumCertificatesInStore(store2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == store2, "Unexpected store\n");
        CertFreeCertificateContext(context);
    }
    /* Check that the collection has both bigCert and bigCert2.  bigCert comes
     * first because store1 was added first.
     */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(!context, "Unexpected cert\n");
        }
    }
    /* close store2, and check that the collection is unmodified */
    CertCloseStore(store2, 0);
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(!context, "Unexpected cert\n");
        }
    }

    /* Adding a collection to a collection is legal */
    collection2 = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddStoreToCollection(collection2, collection,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* check the contents of collection2 */
    context = CertEnumCertificatesInStore(collection2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection2, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection2, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection2, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection2, context);
            ok(!context, "Unexpected cert\n");
        }
    }

    /* I'd like to test closing the collection in the middle of enumeration,
     * but my tests have been inconsistent.  The first time calling
     * CertEnumCertificatesInStore on a closed collection succeeded, while the
     * second crashed.  So anything appears to be fair game.
     * I'd also like to test removing a store from a collection in the middle
     * of an enumeration, but my tests in Windows have been inconclusive.
     * In one scenario it worked.  In another scenario, about a third of the
     * time this leads to "random" crashes elsewhere in the code.  This
     * probably means this is not allowed.
     */

    CertCloseStore(store1, 0);
    CertCloseStore(collection, 0);
    CertCloseStore(collection2, 0);

    /* Add the same cert to two memory stores, then put them in a collection */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != 0, "CertOpenStore failed: %08x\n", GetLastError());
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store2 != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ret = CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    ret = CertAddStoreToCollection(collection, store2, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());

    /* Check that the collection has two copies of the same cert */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Wrong size %d\n", context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
             "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(context == NULL, "Unexpected cert\n");
        }
    }

    /* The following would check whether I can delete an identical cert, rather
     * than one enumerated from the store.  It crashes, so that means I must
     * only call CertDeleteCertificateFromStore with contexts enumerated from
     * the store.
    context = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    if (context)
    {
        ret = CertDeleteCertificateFromStore(collection, context);
        printf("ret is %d, GetLastError is %08x\n", ret, GetLastError());
        CertFreeCertificateContext(context);
    }
     */

    /* Now check deleting from the collection. */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        CertDeleteCertificateFromStore(context);
        /* store1 should now be empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Unexpected cert\n");
        /* and there should be one certificate in the collection */
        context = CertEnumCertificatesInStore(collection, NULL);
        ok(context != NULL, "Expected a valid cert\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Wrong size %d\n", context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
             "Unexpected cert\n");
        }
        context = CertEnumCertificatesInStore(collection, context);
        ok(context == NULL, "Unexpected cert\n");
    }

    /* Finally, test removing stores from the collection.  No return
     *  value, so it's a bit funny to test.
     */
    /* This crashes
     * CertRemoveStoreFromCollection(NULL, NULL);
     */
    /* This "succeeds," no crash, no last error set */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(store2, collection);
    ok(GetLastError() == 0xdeadbeef,
       "Didn't expect an error to be set: %08x\n", GetLastError());

    /* After removing store2, the collection should be empty */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(collection, store2);
    ok(GetLastError() == 0xdeadbeef,
       "Didn't expect an error to be set: %08x\n", GetLastError());
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(!context, "Unexpected cert\n");

    CertCloseStore(collection, 0);
    CertCloseStore(store2, 0);
    CertCloseStore(store1, 0);

    /* Test adding certificates to and deleting certificates from collections.
     */
    store1 = CertOpenSystemStoreA(0, "My");
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    CertDeleteCertificateFromStore(context);

    CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);

    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    CertDeleteCertificateFromStore(context);

    CertCloseStore(collection, 0);
    CertCloseStore(store1, 0);

    /* Test whether a collection store can be committed */
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    SetLastError(0xdeadbeef);
    ret = CertControlStore(collection, 0, CERT_STORE_CTRL_COMMIT, NULL);
    ok(ret, "CertControlStore failed: %08x\n", GetLastError());

    /* Adding a mem store that can't be committed prevents a successful commit.
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddStoreToCollection(collection, store1, 0, 0);
    SetLastError(0xdeadbeef);
    ret = CertControlStore(collection, 0, CERT_STORE_CTRL_COMMIT, NULL);
    ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "expected ERROR_CALL_NOT_IMPLEMENTED, got %d\n", GetLastError());
    CertRemoveStoreFromCollection(collection, store1);
    CertCloseStore(store1, 0);

    /* Test adding a cert to a collection with a file store, committing the
     * change to the collection, and comparing the resulting file.
     */
    if (!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store1 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store1 != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CloseHandle(file);
    CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    CertCloseStore(store1, 0);

    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
     GetLastError());
    ret = CertControlStore(collection, 0, CERT_STORE_CTRL_COMMIT, NULL);
    ok(ret, "CertControlStore failed: %d\n", ret);
    compareStore(collection, "serialized store with cert",
     serializedStoreWithCert, sizeof(serializedStoreWithCert), FALSE);
    CertCloseStore(collection, 0);

    DeleteFileW(filename);
}

/* Looks for the property with ID propID in the buffer buf.  Returns a pointer
 * to its header if found, NULL if not.
 */
static const struct CertPropIDHeader *findPropID(const BYTE *buf, DWORD size,
 DWORD propID)
{
    const struct CertPropIDHeader *ret = NULL;
    BOOL failed = FALSE;

    while (size && !ret && !failed)
    {
        if (size < sizeof(struct CertPropIDHeader))
            failed = TRUE;
        else
        {
            const struct CertPropIDHeader *hdr =
             (const struct CertPropIDHeader *)buf;

            size -= sizeof(struct CertPropIDHeader);
            buf += sizeof(struct CertPropIDHeader);
            if (size < hdr->cb)
                failed = TRUE;
            else if (hdr->propID == propID)
                ret = hdr;
            else
            {
                buf += hdr->cb;
                size -= hdr->cb;
            }
        }
    }
    return ret;
}

static void testRegStore(void)
{
    static const char tempKey[] = "Software\\Wine\\CryptTemp";
    HCERTSTORE store;
    LONG rc;
    HKEY key = NULL;
    DWORD disp, GLE;

    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, NULL);
    GLE = GetLastError();
    ok(!store && (GLE == ERROR_INVALID_HANDLE || GLE == ERROR_BADKEY),
     "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", GLE);
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    GLE = GetLastError();
    ok(!store && (GLE == ERROR_INVALID_HANDLE || GLE == ERROR_BADKEY),
     "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %d\n", GLE);

    /* Opening up any old key works.. */
    key = HKEY_CURRENT_USER;
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    /* Not sure if this is a bug in DuplicateHandle, marking todo_wine for now
     */
    todo_wine ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    CertCloseStore(store, 0);

    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
    if (key)
    {
        BOOL ret;
        BYTE hash[20];
        DWORD size, i;
        static const char certificates[] = "Certificates\\";
        char subKeyName[sizeof(certificates) + 20 * 2 + 1], *ptr;
        HKEY subKey;
        PCCERT_CONTEXT context;

        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
        ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
        /* Add a certificate.  It isn't persisted right away, since it's only
         * added to the cache..
         */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        /* so flush the cache to force a commit.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08x\n", GetLastError());
        /* and check that the expected subkey was written. */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2, sizeof(bigCert2),
         hash, &size);
        ok(ret, "CryptHashCertificate failed: %d\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1; i < size;
         i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
        if (subKey)
        {
            LPBYTE buf;

            size = 0;
            RegQueryValueExA(subKey, "Blob", NULL, NULL, NULL, &size);
            buf = HeapAlloc(GetProcessHeap(), 0, size);
            if (buf)
            {
                rc = RegQueryValueExA(subKey, "Blob", NULL, NULL, buf, &size);
                ok(!rc, "RegQueryValueExA failed: %d\n", rc);
                if (!rc)
                {
                    const struct CertPropIDHeader *hdr;

                    /* Both the hash and the cert should be present */
                    hdr = findPropID(buf, size, CERT_CERT_PROP_ID);
                    ok(hdr != NULL, "Expected to find a cert property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(bigCert2),
                           "Wrong size %d of cert property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), bigCert2,
                         hdr->cb), "Unexpected cert in cert property\n");
                    }
                    hdr = findPropID(buf, size, CERT_HASH_PROP_ID);
                    ok(hdr != NULL, "Expected to find a hash property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(hash),
                           "Wrong size %d of hash property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), hash,
                         hdr->cb), "Unexpected hash in cert property\n");
                    }
                }
                HeapFree(GetProcessHeap(), 0, buf);
            }
            RegCloseKey(subKey);
        }

        /* Remove the existing context */
        context = CertEnumCertificatesInStore(store, NULL);
        ok(context != NULL, "Expected a cert context\n");
        if (context)
            CertDeleteCertificateFromStore(context);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08x\n", GetLastError());

        /* Add a serialized cert with a bogus hash directly to the registry */
        memset(hash, 0, sizeof(hash));
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
        if (subKey)
        {
            BYTE buf[sizeof(struct CertPropIDHeader) * 2 + sizeof(hash) +
             sizeof(bigCert)], *ptr;
            DWORD certCount = 0;
            struct CertPropIDHeader *hdr;

            hdr = (struct CertPropIDHeader *)buf;
            hdr->propID = CERT_HASH_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(hash);
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));
            ptr += sizeof(hash);
            hdr = (struct CertPropIDHeader *)ptr;
            hdr->propID = CERT_CERT_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(bigCert);
            ptr += sizeof(*hdr);
            memcpy(ptr, bigCert, sizeof(bigCert));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* Make sure the bogus hash cert gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %d\n", certCount);

            RegCloseKey(subKey);
        }

        /* Add another serialized cert directly to the registry, this time
         * under the correct key name (named with the correct hash value).
         */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2,
         sizeof(bigCert2), hash, &size);
        ok(ret, "CryptHashCertificate failed: %d\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
        if (subKey)
        {
            BYTE buf[sizeof(struct CertPropIDHeader) * 2 + sizeof(hash) +
             sizeof(bigCert2)], *ptr;
            DWORD certCount = 0;
            PCCERT_CONTEXT context;
            struct CertPropIDHeader *hdr;

            /* First try with a bogus hash... */
            hdr = (struct CertPropIDHeader *)buf;
            hdr->propID = CERT_HASH_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(hash);
            ptr = buf + sizeof(*hdr);
            memset(ptr, 0, sizeof(hash));
            ptr += sizeof(hash);
            hdr = (struct CertPropIDHeader *)ptr;
            hdr->propID = CERT_CERT_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(bigCert2);
            ptr += sizeof(*hdr);
            memcpy(ptr, bigCert2, sizeof(bigCert2));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* and make sure just one cert still gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificate, got %d\n", certCount);

            /* Try again with the correct hash... */
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* and make sure two certs get loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 2, "Expected 2 certificates, got %d\n", certCount);

            RegCloseKey(subKey);
        }
        CertCloseStore(store, 0);
        /* Is delete allowed on a reg store? */
        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0,
         CERT_STORE_DELETE_FLAG, key);
        ok(store == NULL, "Expected NULL return from CERT_STORE_DELETE_FLAG\n");
        ok(GetLastError() == 0, "CertOpenStore failed: %08x\n",
         GetLastError());

        RegCloseKey(key);
    }
    /* The CertOpenStore with CERT_STORE_DELETE_FLAG above will delete the
     * contents of the key, but not the key itself.
     */
    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, &disp);
    ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
    ok(disp == REG_OPENED_EXISTING_KEY,
     "Expected REG_OPENED_EXISTING_KEY, got %d\n", disp);
    if (!rc)
    {
        RegCloseKey(key);
        rc = RegDeleteKeyA(HKEY_CURRENT_USER, tempKey);
        if (rc)
        {
            /* Use shlwapi's SHDeleteKeyA to _really_ blow away the key,
             * otherwise subsequent tests will fail.
             */
            SHDeleteKeyA(HKEY_CURRENT_USER, tempKey);
        }
    }
}

static const char MyA[] = { 'M','y',0,0 };
static const WCHAR MyW[] = { 'M','y',0 };
static const WCHAR BogusW[] = { 'B','o','g','u','s',0 };
static const WCHAR BogusPathW[] = { 'S','o','f','t','w','a','r','e','\\',
 'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',
 't','i','f','i','c','a','t','e','s','\\','B','o','g','u','s',0 };

static void testSystemRegStore(void)
{
    HCERTSTORE store, memStore;

    /* Check with a UNICODE name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyW);
    /* Not all OSes support CERT_STORE_PROV_SYSTEM_REGISTRY, so don't continue
     * testing if they don't.
     */
    if (!store)
        return;

    /* Check that it isn't a collection store */
    memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (memStore)
    {
        BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);
        ok(!ret && GetLastError() == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", GetLastError());
        CertCloseStore(memStore, 0);
    }
    CertCloseStore(store, 0);

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    ok(!store, "CertOpenStore failed: %08x\n", GetLastError());
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0, 0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* The name is expected to be UNICODE, check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
}

static void testSystemStore(void)
{
    static const WCHAR baskslashW[] = { '\\',0 };
    HCERTSTORE store;
    WCHAR keyName[MAX_PATH];
    HKEY key;
    LONG rc;

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* The name is expected to be UNICODE, first check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Create the expected key */
    lstrcpyW(keyName, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH);
    lstrcatW(keyName, baskslashW);
    lstrcatW(keyName, MyW);
    rc = RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_READ,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyEx failed: %d\n", rc);
    if (!rc)
        RegCloseKey(key);
    /* Check opening with a UNICODE name, specifying the create new flag */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_CREATE_NEW_FLAG, MyW);
    ok(!store && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08x\n", GetLastError());
    /* Now check opening with a UNICODE name, this time opening existing */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        /* Check that it's a collection store */
        if (memStore)
        {
            BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);
            ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
            CertCloseStore(memStore, 0);
        }
        CertCloseStore(store, 0);
    }

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store, "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    ok(!store, "Didn't expect a store to be returned when deleting\n");
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

static const BYTE serializedStoreWithCertAndCRL[] = {
 0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x20,0x00,0x00,0x00,0x01,0x00,0x00,
 0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,
 0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,
 0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,
 0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x21,0x00,0x00,0x00,0x01,0x00,
 0x00,0x00,0x47,0x00,0x00,0x00,0x30,0x45,0x30,0x2c,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
 0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00,0x03,0x11,
 0x00,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,
 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static void testFileStore(void)
{
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;
    PCCERT_CONTEXT cert;
    HANDLE file;

    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());

    if (!GetTempFileNameW(szDot, szPrefix, 0, filename))
       return;

    DeleteFileW(filename);
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, CERT_STORE_DELETE_FLAG,
     file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());

    /* A "read-only" file store.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        DWORD size;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        /* apparently allows adding certificates.. */
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        /* but not commits.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        /* It still has certs in memory.. */
        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        CertFreeCertificateContext(cert);
        /* but the file size is still 0. */
        size = GetFileSize(file, NULL);
        ok(size == 0, "Expected size 0, got %d\n", size);
        CertCloseStore(store, 0);
    }

    /* The create new flag is allowed.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        CertCloseStore(store, 0);
    }
    /* as is the open existing flag. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        CertCloseStore(store, 0);
    }
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        /* with commits enabled, commit is allowed */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %d\n", ret);
        compareStore(store, "serialized store with cert",
         serializedStoreWithCert, sizeof(serializedStoreWithCert), FALSE);
        CertCloseStore(store, 0);
    }
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
         sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());
        compareStore(store, "serialized store with cert and CRL",
         serializedStoreWithCertAndCRL, sizeof(serializedStoreWithCertAndCRL),
         FALSE);
        CertCloseStore(store, 0);
    }

    DeleteFileW(filename);
}

static BOOL initFileFromData(LPCWSTR filename, const BYTE *pb, DWORD cb)
{
    HANDLE file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    BOOL ret;

    if (file != INVALID_HANDLE_VALUE)
    {
        DWORD written;

        ret = WriteFile(file, pb, cb, &written, NULL);
        CloseHandle(file);
    }
    else
        ret = FALSE;
    return ret;
}

static const BYTE base64SPC[] =
"MIICJQYJKoZIhvcNAQcCoIICFjCCAhICAQExADALBgkqhkiG9w0BBwGgggH6MIIB"
"9jCCAV+gAwIBAgIQnP8+EF4opr9OxH7h4uBPWTANBgkqhkiG9w0BAQQFADAUMRIw"
"EAYDVQQDEwlKdWFuIExhbmcwHhcNMDgxMjEyMTcxMDE0WhcNMzkxMjMxMjM1OTU5"
"WjAUMRIwEAYDVQQDEwlKdWFuIExhbmcwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJ"
"AoGBALCgNjyNvOic0FOfjxvi43HbM+D5joDkhiGSXe+gbZlf8f16k07kkObFEunz"
"mdB5coscmA7gyqiWNN4ZUyr2cA3lCbnpGPA/0IblyyOcuGIFmmCzeZaVa5ZG6xZP"
"K7L7o+73Qo6jXVbGhBGnMZ7Q9sAn6s2933olnStnejnqwV0NAgMBAAGjSTBHMEUG"
"A1UdAQQ+MDyAEFKbKEdXYyx+CWKcV6vxM6ShFjAUMRIwEAYDVQQDEwlKdWFuIExh"
"bmeCEJz/PhBeKKa/TsR+4eLgT1kwDQYJKoZIhvcNAQEEBQADgYEALpkgLgW3mEaK"
"idPQ3iPJYLG0Ub1wraqEl9bd42hrhzIdcDzlQgxnm8/5cHYVxIF/C20x/HJplb1R"
"G6U1ipFe/q8byWD/9JpiBKMGPi9YlUTgXHfS9d4S/QWO1h9Z7KeipBYhoslQpHXu"
"y9bUr8Adqi6SzgHpCnMu53dxgxUD1r4xAA==";
/* Same as base64SPC, but as a wide-char string */
static const WCHAR utf16Base64SPC[] = {
'M','I','I','C','J','Q','Y','J','K','o','Z','I','h','v','c','N','A',
'Q','c','C','o','I','I','C','F','j','C','C','A','h','I','C','A','Q',
'E','x','A','D','A','L','B','g','k','q','h','k','i','G','9','w','0',
'B','B','w','G','g','g','g','H','6','M','I','I','B','9','j','C','C',
'A','V','+','g','A','w','I','B','A','g','I','Q','n','P','8','+','E',
'F','4','o','p','r','9','O','x','H','7','h','4','u','B','P','W','T',
'A','N','B','g','k','q','h','k','i','G','9','w','0','B','A','Q','Q',
'F','A','D','A','U','M','R','I','w','E','A','Y','D','V','Q','Q','D',
'E','w','l','K','d','W','F','u','I','E','x','h','b','m','c','w','H',
'h','c','N','M','D','g','x','M','j','E','y','M','T','c','x','M','D',
'E','0','W','h','c','N','M','z','k','x','M','j','M','x','M','j','M',
'1','O','T','U','5','W','j','A','U','M','R','I','w','E','A','Y','D',
'V','Q','Q','D','E','w','l','K','d','W','F','u','I','E','x','h','b',
'm','c','w','g','Z','8','w','D','Q','Y','J','K','o','Z','I','h','v',
'c','N','A','Q','E','B','B','Q','A','D','g','Y','0','A','M','I','G',
'J','A','o','G','B','A','L','C','g','N','j','y','N','v','O','i','c',
'0','F','O','f','j','x','v','i','4','3','H','b','M','+','D','5','j',
'o','D','k','h','i','G','S','X','e','+','g','b','Z','l','f','8','f',
'1','6','k','0','7','k','k','O','b','F','E','u','n','z','m','d','B',
'5','c','o','s','c','m','A','7','g','y','q','i','W','N','N','4','Z',
'U','y','r','2','c','A','3','l','C','b','n','p','G','P','A','/','0',
'I','b','l','y','y','O','c','u','G','I','F','m','m','C','z','e','Z',
'a','V','a','5','Z','G','6','x','Z','P','K','7','L','7','o','+','7',
'3','Q','o','6','j','X','V','b','G','h','B','G','n','M','Z','7','Q',
'9','s','A','n','6','s','2','9','3','3','o','l','n','S','t','n','e',
'j','n','q','w','V','0','N','A','g','M','B','A','A','G','j','S','T',
'B','H','M','E','U','G','A','1','U','d','A','Q','Q','+','M','D','y',
'A','E','F','K','b','K','E','d','X','Y','y','x','+','C','W','K','c',
'V','6','v','x','M','6','S','h','F','j','A','U','M','R','I','w','E',
'A','Y','D','V','Q','Q','D','E','w','l','K','d','W','F','u','I','E',
'x','h','b','m','e','C','E','J','z','/','P','h','B','e','K','K','a',
'/','T','s','R','+','4','e','L','g','T','1','k','w','D','Q','Y','J',
'K','o','Z','I','h','v','c','N','A','Q','E','E','B','Q','A','D','g',
'Y','E','A','L','p','k','g','L','g','W','3','m','E','a','K','i','d',
'P','Q','3','i','P','J','Y','L','G','0','U','b','1','w','r','a','q',
'E','l','9','b','d','4','2','h','r','h','z','I','d','c','D','z','l',
'Q','g','x','n','m','8','/','5','c','H','Y','V','x','I','F','/','C',
'2','0','x','/','H','J','p','l','b','1','R','G','6','U','1','i','p',
'F','e','/','q','8','b','y','W','D','/','9','J','p','i','B','K','M',
'G','P','i','9','Y','l','U','T','g','X','H','f','S','9','d','4','S',
'/','Q','W','O','1','h','9','Z','7','K','e','i','p','B','Y','h','o',
's','l','Q','p','H','X','u','y','9','b','U','r','8','A','d','q','i',
'6','S','z','g','H','p','C','n','M','u','5','3','d','x','g','x','U',
'D','1','r','4','x','A','A','=','=',0 };

static void testFileNameStore(void)
{
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR spcPrefix[] = { 's','p','c',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;
    DWORD GLE;

    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0, 0, NULL);
    GLE = GetLastError();
    ok(!store && (GLE == ERROR_PATH_NOT_FOUND || GLE == ERROR_INVALID_PARAMETER),
     "Expected ERROR_PATH_NOT_FOUND or ERROR_INVALID_PARAMETER, got %08x\n",
     GLE);

    if (!GetTempFileNameW(szDot, szPrefix, 0, filename))
       return;
    DeleteFileW(filename);

    /* The two flags are mutually exclusive */
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG, filename);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());

    /* In all of the following tests, the encoding type seems to be ignored */
    if (initFileFromData(filename, bigCert, sizeof(bigCert)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
    if (initFileFromData(filename, serializedStoreWithCert,
     sizeof(serializedStoreWithCert)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
    if (initFileFromData(filename, serializedStoreWithCertAndCRL,
     sizeof(serializedStoreWithCertAndCRL)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(crl != NULL, "CertEnumCRLsInStore failed: %08x\n", GetLastError());
        crl = CertEnumCRLsInStore(store, crl);
        ok(!crl, "Expected only one CRL\n");

        CertCloseStore(store, 0);
        /* Don't delete it this time, the next test uses it */
    }
    /* Now that the file exists, we can open it read-only */
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_STORE_READONLY_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CertCloseStore(store, 0);
    DeleteFileW(filename);

    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_CREATE_NEW_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        compareStore(store, "serialized store with cert",
         serializedStoreWithCert, sizeof(serializedStoreWithCert), FALSE);
        CertCloseStore(store, 0);
    }
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING,
         signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());
        compareStore(store, "serialized store with cert and CRL",
         serializedStoreWithCertAndCRL, sizeof(serializedStoreWithCertAndCRL),
         FALSE);
        CertCloseStore(store, 0);
    }
    DeleteFileW(filename);

    if (!GetTempFileNameW(szDot, spcPrefix, 0, filename))
       return;
    DeleteFileW(filename);

    if (initFileFromData(filename, base64SPC, sizeof(base64SPC)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
    if (initFileFromData(filename, (BYTE *)utf16Base64SPC,
     sizeof(utf16Base64SPC)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
}

static const BYTE signedContent[] = {
0x30,0x81,0xb2,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x02,0xa0,
0x81,0xa4,0x30,0x81,0xa1,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,0x08,0x2a,
0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x13,0x06,0x09,0x2a,0x86,
0x48,0x86,0xf7,0x0d,0x01,0x07,0x01,0xa0,0x06,0x04,0x04,0x01,0x02,0x03,0x04,
0x31,0x77,0x30,0x75,0x02,0x01,0x01,0x30,0x1a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x02,0x01,0x01,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x02,0x05,0x05,0x00,0x30,0x04,0x06,0x00,0x05,0x00,0x04,0x40,0x81,0xa6,0x70,
0xb3,0xef,0x59,0xd1,0x66,0xd1,0x9b,0xc0,0x9a,0xb6,0x9a,0x5e,0x6d,0x6f,0x6d,
0x0d,0x59,0xa9,0xaa,0x6e,0xe9,0x2c,0xa0,0x1e,0xee,0xc2,0x60,0xbc,0x59,0xbe,
0x3f,0x63,0x06,0x8d,0xc9,0x11,0x1d,0x23,0x64,0x92,0xef,0x2e,0xfc,0x57,0x29,
0xa4,0xaf,0xe0,0xee,0x93,0x19,0x39,0x51,0xe4,0x44,0xb8,0x0b,0x28,0xf4,0xa8,
0x0d };
static const BYTE signedWithCertAndCrlBareContent[] = {
0x30,0x82,0x01,0x4f,0x02,0x01,0x01,0x31,0x0e,0x30,0x0c,0x06,0x08,0x2a,0x86,
0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x13,0x06,0x09,0x2a,0x86,0x48,
0x86,0xf7,0x0d,0x01,0x07,0x01,0xa0,0x06,0x04,0x04,0x01,0x02,0x03,0x04,0xa0,
0x7c,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,
0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,
0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,
0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x16,0x30,0x14,
0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,
0x01,0xff,0x02,0x01,0x01,0xa1,0x2e,0x30,0x2c,0x30,0x02,0x06,0x00,0x30,0x15,
0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x31,0x77,0x30,0x75,0x02,0x01,0x01,
0x30,0x1a,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,
0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x02,0x01,0x01,0x30,0x0c,
0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x30,0x04,0x06,
0x00,0x05,0x00,0x04,0x40,0x81,0xa6,0x70,0xb3,0xef,0x59,0xd1,0x66,0xd1,0x9b,
0xc0,0x9a,0xb6,0x9a,0x5e,0x6d,0x6f,0x6d,0x0d,0x59,0xa9,0xaa,0x6e,0xe9,0x2c,
0xa0,0x1e,0xee,0xc2,0x60,0xbc,0x59,0xbe,0x3f,0x63,0x06,0x8d,0xc9,0x11,0x1d,
0x23,0x64,0x92,0xef,0x2e,0xfc,0x57,0x29,0xa4,0xaf,0xe0,0xee,0x93,0x19,0x39,
0x51,0xe4,0x44,0xb8,0x0b,0x28,0xf4,0xa8,0x0d };
static const BYTE hashContent[] = {
0x30,0x47,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x07,0x05,0xa0,0x3a,
0x30,0x38,0x02,0x01,0x00,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x02,0x05,0x05,0x00,0x30,0x13,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x07,0x01,0xa0,0x06,0x04,0x04,0x01,0x02,0x03,0x04,0x04,0x10,0x08,0xd6,0xc0,
0x5a,0x21,0x51,0x2a,0x79,0xa1,0xdf,0xeb,0x9d,0x2a,0x8f,0x26,0x2f };
static const BYTE hashBareContent[] = {
0x30,0x38,0x02,0x01,0x00,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x02,0x05,0x05,0x00,0x30,0x13,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,
0x07,0x01,0xa0,0x06,0x04,0x04,0x01,0x02,0x03,0x04,0x04,0x10,0x08,0xd6,0xc0,
0x5a,0x21,0x51,0x2a,0x79,0xa1,0xdf,0xeb,0x9d,0x2a,0x8f,0x26,0x2f };

static void testMessageStore(void)
{
    HCERTSTORE store;
    HCRYPTMSG msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 0, 0, NULL,
     NULL);
    CRYPT_DATA_BLOB blob = { sizeof(signedWithCertAndCrlBareContent),
     (LPBYTE)signedWithCertAndCrlBareContent };
    DWORD count, size;
    BOOL ret;

    /* Crashes
    store = CertOpenStore(CERT_STORE_PROV_MSG, 0, 0, 0, NULL);
     */
    SetLastError(0xdeadbeef);
    store = CertOpenStore(CERT_STORE_PROV_MSG, 0, 0, 0, msg);
    ok(!store && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %08x\n", GetLastError());
    CryptMsgUpdate(msg, signedContent, sizeof(signedContent), TRUE);
    store = CertOpenStore(CERT_STORE_PROV_MSG, 0, 0, 0, msg);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT cert = NULL;
        PCCRL_CONTEXT crl = NULL;

        count = 0;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 0, "Expected 0 certificates, got %d\n", count);

        count = 0;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 0, "Expected 0 CRLs, got %d\n", count);

        /* Can add certs to a message store */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        count = 0;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 1, "Expected 1 certificate, got %d\n", count);

        CertCloseStore(store, 0);
    }
    /* but the added certs weren't actually added to the message */
    size = sizeof(count);
    ret = CryptMsgGetParam(msg, CMSG_CERT_COUNT_PARAM, 0, &count, &size);
    ok(ret, "CryptMsgGetParam failed: %08x\n", GetLastError());
    ok(count == 0, "Expected 0 certificates, got %d\n", count);
    CryptMsgClose(msg);

    /* Crashes
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, NULL);
     */
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        DWORD count = 0;
        PCCERT_CONTEXT cert = NULL;
        PCCRL_CONTEXT crl = NULL;

        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 1, "Expected 1 certificate, got %d\n", count);

        count = 0;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 1, "Expected 1 CRL, got %d\n", count);

        CertCloseStore(store, 0);
    }
    /* Encoding appears to be ignored */
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, X509_ASN_ENCODING, 0, 0,
     &blob);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Messages other than signed messages aren't allowed */
    blob.cbData = sizeof(hashContent);
    blob.pbData = (LPBYTE)hashContent;
    SetLastError(0xdeadbeef);
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(!store && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %08x\n", GetLastError());
    blob.cbData = sizeof(hashBareContent);
    blob.pbData = (LPBYTE)hashBareContent;
    SetLastError(0xdeadbeef);
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(!store && GetLastError() == CRYPT_E_ASN1_BADTAG,
     "Expected CRYPT_E_ASN1_BADTAG, got %08x\n", GetLastError());
}

static void testSerializedStore(void)
{
    HCERTSTORE store;
    CRYPT_DATA_BLOB blob;

    if (0)
    {
        /* Crash */
        store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0, 0, NULL);
        store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0,
         CERT_STORE_DELETE_FLAG, NULL);
    }
    blob.cbData = sizeof(serializedStoreWithCert);
    blob.pbData = (BYTE *)serializedStoreWithCert;
    store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0,
     CERT_STORE_DELETE_FLAG, &blob);
    ok(!store && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0, 0, &blob);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
    }
    blob.cbData = sizeof(serializedStoreWithCertAndCRL);
    blob.pbData = (BYTE *)serializedStoreWithCertAndCRL;
    store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0, 0, &blob);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(crl != NULL, "CertEnumCRLsInStore failed: %08x\n",
         GetLastError());
        crl = CertEnumCRLsInStore(store, crl);
        ok(!crl, "Expected only one CRL\n");

        CertCloseStore(store, 0);
    }
}

static void testCertOpenSystemStore(void)
{
    HCERTSTORE store;

    store = CertOpenSystemStoreW(0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* This succeeds, and on WinXP at least, the Bogus key is created under
     * HKCU (but not under HKLM, even when run as an administrator.)
     */
    store = CertOpenSystemStoreW(0, BogusW);
    ok(store != 0, "CertOpenSystemStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Delete it so other tests succeed next time around */
    CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

static const struct
{
    DWORD cert_store;
    BOOL expected;
    BOOL todo;
} reg_system_store_test_data[] = {
    { CERT_SYSTEM_STORE_CURRENT_USER,  TRUE, 0},
    /* Following tests could require administrator privileges and thus could be skipped */
    { CERT_SYSTEM_STORE_CURRENT_SERVICE, TRUE, 1},
    { CERT_SYSTEM_STORE_LOCAL_MACHINE, TRUE, 0},
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY, TRUE, 0},
    { CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY, TRUE, 0},
    { CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, TRUE, 1}
};

static void testCertRegisterSystemStore(void)
{
    BOOL ret, cur_flag;
    DWORD err = 0;
    HCERTSTORE hstore;
    static const WCHAR WineTestW[] = {'W','i','n','e','T','e','s','t',0};
    const CERT_CONTEXT *cert, *cert2;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(reg_system_store_test_data); i++) {
        cur_flag = reg_system_store_test_data[i].cert_store;
        ret = CertRegisterSystemStore(WineTestW, cur_flag, NULL, NULL);
        if (!ret)
        {
            err = GetLastError();
            if (err == ERROR_ACCESS_DENIED)
            {
                win_skip("Insufficient privileges for the flag %08x test\n", cur_flag);
                continue;
            }
        }
        todo_wine_if (reg_system_store_test_data[i].todo)
            ok (ret == reg_system_store_test_data[i].expected,
                "Store registration (dwFlags=%08x) failed, last error %x\n", cur_flag, err);
        if (!ret)
        {
            skip("Nothing to test without registered store at %08x\n", cur_flag);
            continue;
        }

        hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_STORE_OPEN_EXISTING_FLAG | cur_flag, WineTestW);
        ok (hstore != NULL, "Opening just registered store at %08x failed, last error %x\n", cur_flag, GetLastError());

        cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert != NULL, "Failed creating cert at %08x, last error: %x\n", cur_flag, GetLastError());
        if (cert)
        {
            ret = CertAddCertificateContextToStore(hstore, cert, CERT_STORE_ADD_NEW, NULL);
            ok (ret, "Failed to add cert at %08x, last error: %x\n", cur_flag, GetLastError());

            cert2 = CertEnumCertificatesInStore(hstore, NULL);
            ok (cert2 != NULL && cert2->cbCertEncoded == cert->cbCertEncoded,
                "Unexpected cert encoded size at %08x, last error: %x\n", cur_flag, GetLastError());

            ret = CertDeleteCertificateFromStore(cert2);
            ok (ret, "Failed to delete certificate from the new store at %08x, last error: %x\n", cur_flag, GetLastError());

            CertFreeCertificateContext(cert);
        }

        ret = CertCloseStore(hstore, 0);
        ok (ret, "CertCloseStore failed at %08x, last error %x\n", cur_flag, GetLastError());

        ret = CertUnregisterSystemStore(WineTestW, cur_flag );
        todo_wine_if (reg_system_store_test_data[i].todo)
            ok( ret == reg_system_store_test_data[i].expected,
                "Unregistering failed at %08x, last error %d\n", cur_flag, GetLastError());
     }

}

struct EnumSystemStoreInfo
{
    BOOL  goOn;
    DWORD storeCount;
};

static BOOL CALLBACK enumSystemStoreCB(const void *systemStore, DWORD dwFlags,
 PCERT_SYSTEM_STORE_INFO pStoreInfo, void *pvReserved, void *pvArg)
{
    struct EnumSystemStoreInfo *info = pvArg;

    info->storeCount++;
    return info->goOn;
}

static void testCertEnumSystemStore(void)
{
    BOOL ret;
    struct EnumSystemStoreInfo info = { FALSE, 0 };

    SetLastError(0xdeadbeef);
    ret = CertEnumSystemStore(0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Crashes
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, NULL,
     NULL);
     */

    SetLastError(0xdeadbeef);
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, &info,
     enumSystemStoreCB);
    /* Callback returning FALSE stops enumeration */
    ok(!ret, "Expected CertEnumSystemStore to stop\n");
    ok(info.storeCount == 0 || info.storeCount == 1,
     "Expected 0 or 1 stores\n");

    info.goOn = TRUE;
    info.storeCount = 0;
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, &info,
     enumSystemStoreCB);
    ok(ret, "CertEnumSystemStore failed: %08x\n", GetLastError());
    /* There should always be at least My, Root, and CA stores */
    ok(info.storeCount == 0 || info.storeCount >= 3,
     "Expected at least 3 stores\n");
}

static void testStoreProperty(void)
{
    HCERTSTORE store;
    BOOL ret;
    DWORD propID, size = 0, state;
    CRYPT_DATA_BLOB blob;

    /* Crash
    ret = CertGetStoreProperty(NULL, 0, NULL, NULL);
    ret = CertGetStoreProperty(NULL, 0, NULL, &size);
    ret = CertGetStoreProperty(store, 0, NULL, NULL);
     */

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Check a missing prop ID */
    SetLastError(0xdeadbeef);
    ret = CertGetStoreProperty(store, 0, NULL, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    /* Contrary to MSDN, CERT_ACCESS_STATE_PROP_ID is supported for stores.. */
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08x\n",
     GetLastError());
    ok(!state, "Expected a non-persisted store\n");
    /* and CERT_STORE_LOCALIZED_NAME_PROP_ID isn't supported by default. */
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    /* Delete an arbitrary property on a store */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* Set an arbitrary property on a store */
    blob.pbData = (LPBYTE)&state;
    blob.cbData = sizeof(state);
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, &blob);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* Get an arbitrary property that's been set */
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(ret, "CertGetStoreProperty failed: %08x\n", GetLastError());
    ok(size == sizeof(state), "Unexpected data size %d\n", size);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, &propID, &size);
    ok(ret, "CertGetStoreProperty failed: %08x\n", GetLastError());
    ok(propID == state, "CertGetStoreProperty got the wrong value\n");
    /* Delete it again */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* And check that it's missing */
    SetLastError(0xdeadbeef);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertCloseStore(store, 0);

    /* Recheck on the My store.. */
    store = CertOpenSystemStoreW(0, MyW);
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08x\n",
     GetLastError());
    ok(state, "Expected a persisted store\n");
    SetLastError(0xdeadbeef);
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertCloseStore(store, 0);
}

static void testAddSerialized(void)
{
    BOOL ret;
    HCERTSTORE store;
    BYTE buf[sizeof(struct CertPropIDHeader) * 2 + 20 + sizeof(bigCert)] =
     { 0 };
    BYTE hash[20];
    struct CertPropIDHeader *hdr;
    PCCERT_CONTEXT context;

    ret = CertAddSerializedElementToStore(0, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08x\n", GetLastError());

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddSerializedElementToStore(store, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08x\n", GetLastError());

    /* Test with an empty property */
    hdr = (struct CertPropIDHeader *)buf;
    hdr->propID = CERT_CERT_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = 0;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test with a bad size in property header */
    hdr->cb = sizeof(bigCert) - 1;
    memcpy(buf + sizeof(struct CertPropIDHeader), bigCert, sizeof(bigCert));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Kosher size in property header, but no context type */
    hdr->cb = sizeof(bigCert);
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* With a bad context type */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0,
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0,
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Bad unknown field, good type */
    hdr->unknown1 = 2;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
    /* Most everything okay, but bad add disposition */
    hdr->unknown1 = 1;
    /* This crashes
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
     * as does this
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
     */
    /* Everything okay, but buffer's too big */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf),
     CERT_STORE_ADD_NEW, 0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(ret, "CertAddSerializedElementToStore failed: %08x\n", GetLastError());
    /* Everything okay, check it's not re-added */
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08x\n", GetLastError());

    context = CertEnumCertificatesInStore(store, NULL);
    ok(context != NULL, "Expected a cert\n");
    if (context)
        CertDeleteCertificateFromStore(context);

    /* Try adding with a bogus hash.  Oddly enough, it succeeds, and the hash,
     * when queried, is the real hash rather than the bogus hash.
     */
    hdr = (struct CertPropIDHeader *)(buf + sizeof(struct CertPropIDHeader) +
     sizeof(bigCert));
    hdr->propID = CERT_HASH_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = sizeof(hash);
    memset(hash, 0xc, sizeof(hash));
    memcpy((LPBYTE)hdr + sizeof(struct CertPropIDHeader), hash, sizeof(hash));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf),
     CERT_STORE_ADD_NEW, 0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL,
     (const void **)&context);
    ok(ret, "CertAddSerializedElementToStore failed: %08x\n", GetLastError());
    if (context)
    {
        BYTE hashVal[20], realHash[20];
        DWORD size = sizeof(hashVal);

        ret = CryptHashCertificate(0, 0, 0, bigCert, sizeof(bigCert),
         realHash, &size);
        ok(ret, "CryptHashCertificate failed: %08x\n", GetLastError());
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID,
         hashVal, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08x\n",
         GetLastError());
        ok(!memcmp(hashVal, realHash, size), "Unexpected hash\n");
        CertFreeCertificateContext(context);
    }

    CertCloseStore(store, 0);
}

static const BYTE serializedCertWithFriendlyName[] = {
0x0b,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x57,0x00,0x69,
0x00,0x6e,0x00,0x65,0x00,0x54,0x00,0x65,0x00,0x73,0x00,0x74,0x00,0x00,0x00,
0x20,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,
0x01,0x01,0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,
0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,
0x22,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,
0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,
0x30,0x30,0x30,0x30,0x5a,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,
0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,
0x30,0x02,0x06,0x00,0x03,0x01,0x00,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,
0x55,0x1d,0x13,0x01,0x01,0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,
0x01 };
static const BYTE serializedStoreWithCertWithFriendlyName[] = {
0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x0b,0x00,0x00,0x00,0x01,0x00,0x00,
0x00,0x12,0x00,0x00,0x00,0x57,0x00,0x69,0x00,0x6e,0x00,0x65,0x00,0x54,0x00,
0x65,0x00,0x73,0x00,0x74,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x01,0x00,0x00,
0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,
0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,
0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,
0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,
0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,
0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,
0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00 };
static const BYTE serializedStoreWithCertAndHash[] = {
0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x03,0x00,0x00,0x00,0x01,0x00,0x00,
0x00,0x14,0x00,0x00,0x00,0x6e,0x30,0x90,0x71,0x5f,0xd9,0x23,0x56,0xeb,0xae,
0x25,0x40,0xe6,0x22,0xda,0x19,0x26,0x02,0xa6,0x08,0x20,0x00,0x00,0x00,0x01,
0x00,0x00,0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,
0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,
0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,
0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,
0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,
0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,
0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,
0x01,0x00,0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static void delete_test_key(void)
{
    HKEY root_key, test_key;
    static const WCHAR SysCertW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
        'S','y','s','t','e','m','C','e','r','t','i','f','i','c','a','t','e','s',0};
    static const WCHAR WineTestW[] = {'W','i','n','e','T','e','s','t',0};
    WCHAR subkey_name[32];
    DWORD num_subkeys, subkey_name_len;
    int idx;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, SysCertW, 0, KEY_READ, &root_key))
        return;
    if (RegOpenKeyExW(root_key, WineTestW, 0, KEY_READ, &test_key))
    {
        RegCloseKey(root_key);
        return;
    }
    RegQueryInfoKeyW(test_key, NULL, NULL, NULL, &num_subkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    for (idx = num_subkeys; idx-- > 0;)
    {
        subkey_name_len = ARRAY_SIZE(subkey_name);
        RegEnumKeyExW(test_key, idx, subkey_name, &subkey_name_len, NULL, NULL, NULL, NULL);
        RegDeleteKeyW(test_key, subkey_name);
    }
    RegCloseKey(test_key);
    RegDeleteKeyW(root_key, WineTestW);
    RegCloseKey(root_key);
}

static void testAddCertificateLink(void)
{
    BOOL ret;
    HCERTSTORE store1, store2;
    PCCERT_CONTEXT source, linked;
    DWORD size;
    LPBYTE buf;
    CERT_NAME_BLOB blob;
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR WineTestW[] = { 'W','i','n','e','T','e','s','t',0 };
    WCHAR filename1[MAX_PATH], filename2[MAX_PATH];
    HANDLE file;

    if (0)
    {
        /* Crashes, i.e. the store is dereferenced without checking. */
        ret = CertAddCertificateLinkToStore(NULL, NULL, 0, NULL);
    }

    /* Adding a certificate link to a store requires a valid add disposition */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    SetLastError(0xdeadbeef);
    ret = CertAddCertificateLinkToStore(store1, NULL, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    source = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    SetLastError(0xdeadbeef);
    ret = CertAddCertificateLinkToStore(store1, source, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddCertificateLinkToStore(store1, source, CERT_STORE_ADD_ALWAYS,
     NULL);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (0)
    {
        /* Crashes, i.e. the source certificate is dereferenced without
         * checking when a valid add disposition is given.
         */
        ret = CertAddCertificateLinkToStore(store1, NULL, CERT_STORE_ADD_ALWAYS,
         NULL);
    }
    CertCloseStore(store1, 0);

    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddCertificateLinkToStore(store1, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store1, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
             GetLastError());
            /* The serialized linked certificate is identical to the serialized
             * original certificate.
             */
            ok(size == sizeof(serializedCert), "Wrong size %d\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }
        /* Set a friendly name on the source certificate... */
        blob.pbData = (LPBYTE)WineTestW;
        blob.cbData = sizeof(WineTestW);
        ret = CertSetCertificateContextProperty(source,
         CERT_FRIENDLY_NAME_PROP_ID, 0, &blob);
        ok(ret, "CertSetCertificateContextProperty failed: %08x\n",
         GetLastError());
        /* and the linked certificate has the same friendly name. */
        ret = CertGetCertificateContextProperty(linked,
         CERT_FRIENDLY_NAME_PROP_ID, NULL, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertGetCertificateContextProperty(linked,
             CERT_FRIENDLY_NAME_PROP_ID, buf, &size);
            ok(ret, "CertGetCertificateContextProperty failed: %08x\n",
             GetLastError());
            ok(!lstrcmpW((LPCWSTR)buf, WineTestW),
             "unexpected friendly name\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }
        CertFreeCertificateContext(linked);
    }
    CertFreeCertificateContext(source);
    CertCloseStore(store1, 0);

    /* Test adding a cert to a file store, committing the change to the store,
     * and creating a link to the resulting cert.
     */
    if (!GetTempFileNameW(szDot, szPrefix, 0, filename1))
       return;

    DeleteFileW(filename1);
    file = CreateFileW(filename1, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store1 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store1 != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CloseHandle(file);

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &source);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
     GetLastError());

    /* Test adding a link to a memory store. */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            /* The serialized linked certificate is identical to the serialized
             * original certificate.
             */
            ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n", GetLastError());
            ok(size == sizeof(serializedCert), "Wrong size %d\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }
        /* Set a friendly name on the source certificate... */
        blob.pbData = (LPBYTE)WineTestW;
        blob.cbData = sizeof(WineTestW);
        ret = CertSetCertificateContextProperty(source,
         CERT_FRIENDLY_NAME_PROP_ID, 0, &blob);
        ok(ret, "CertSetCertificateContextProperty failed: %08x\n",
         GetLastError());
        /* and the linked certificate has the same friendly name. */
        ret = CertGetCertificateContextProperty(linked,
         CERT_FRIENDLY_NAME_PROP_ID, NULL, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertGetCertificateContextProperty(linked,
             CERT_FRIENDLY_NAME_PROP_ID, buf, &size);
            ok(ret, "CertGetCertificateContextProperty failed: %08x\n", GetLastError());
            ok(!lstrcmpW((LPCWSTR)buf, WineTestW),
             "unexpected friendly name\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }
        CertFreeCertificateContext(linked);
    }
    CertCloseStore(store2, 0);

    if (!GetTempFileNameW(szDot, szPrefix, 0, filename2))
       return;

    DeleteFileW(filename2);
    file = CreateFileW(filename2, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store2 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store2 != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CloseHandle(file);
    /* Test adding a link to a file store. */
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
             GetLastError());
            /* The serialized linked certificate now contains the friendly
             * name property.
             */
            ok(size == sizeof(serializedCertWithFriendlyName),
             "Wrong size %d\n", size);
            ok(!memcmp(serializedCertWithFriendlyName, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }
        CertFreeCertificateContext(linked);
        compareStore(store2, "file store -> file store",
         serializedStoreWithCertWithFriendlyName,
         sizeof(serializedStoreWithCertWithFriendlyName), FALSE);
    }
    CertCloseStore(store2, 0);
    DeleteFileW(filename2);

    CertFreeCertificateContext(source);

    CertCloseStore(store1, 0);
    DeleteFileW(filename1);

    /* Test adding a link to a system store (which is a collection store.) */
    store1 = CertOpenSystemStoreA(0, "My");
    source = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    SetLastError(0xdeadbeef);
    ret = CertAddCertificateLinkToStore(store1, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08x\n", GetLastError());
    CertFreeCertificateContext(source);

    /* Test adding a link to a file store, where the linked certificate is
     * in a system store.
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &source);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
     GetLastError());
    if (!GetTempFileNameW(szDot, szPrefix, 0, filename1))
       return;

    DeleteFileW(filename1);
    file = CreateFileW(filename1, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store2 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store2 != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CloseHandle(file);

    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        ret = CertControlStore(store2, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %d\n", ret);
        compareStore(store2, "file store -> system store",
         serializedStoreWithCertAndHash,
         sizeof(serializedStoreWithCertAndHash), TRUE);
        CertFreeCertificateContext(linked);
    }

    CertCloseStore(store2, 0);
    DeleteFileW(filename1);

    /* Test adding a link to a registry store, where the linked certificate is
     * in a system store.
     */
    store2 = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, WineTestW);
    ok(store2 != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        CertDeleteCertificateFromStore(linked);
    }
    CertCloseStore(store2, 0);

    CertFreeCertificateContext(source);
    CertCloseStore(store1, 0);

    delete_test_key();
}

static DWORD countCertsInStore(HCERTSTORE store)
{
    PCCERT_CONTEXT cert = NULL;
    DWORD certs = 0;

    do {
        cert = CertEnumCertificatesInStore(store, cert);
        if (cert)
            certs++;
    } while (cert);
    return certs;
}

static DWORD countCRLsInStore(HCERTSTORE store)
{
    PCCRL_CONTEXT crl = NULL;
    DWORD crls = 0;

    do {
        crl = CertEnumCRLsInStore(store, crl);
        if (crl)
            crls++;
    } while (crl);
    return crls;
}

static void testEmptyStore(void)
{
    const CERT_CONTEXT *cert, *cert2, *cert3;
    const CRL_CONTEXT *crl;
    const CTL_CONTEXT *ctl;
    HCERTSTORE store;
    BOOL res;

    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    ok(cert != NULL, "CertCreateCertificateContext failed\n");
    ok(cert->hCertStore != NULL, "cert->hCertStore == NULL\n");
    if(!cert->hCertStore) {
        CertFreeCertificateContext(cert);
        return;
    }

    test_store_is_empty(cert->hCertStore);

    cert2 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2, sizeof(bigCert2));
    ok(cert2 != NULL, "CertCreateCertificateContext failed\n");
    ok(cert2->hCertStore == cert->hCertStore, "Unexpected hCertStore\n");

    test_store_is_empty(cert2->hCertStore);

    res = CertAddCertificateContextToStore(cert->hCertStore, cert2, CERT_STORE_ADD_NEW, &cert3);
    ok(res, "CertAddCertificateContextToStore failed\n");
    todo_wine
    ok(cert3 && cert3 != cert2, "Unexpected cert3\n");
    ok(cert3->hCertStore == cert->hCertStore, "Unexpected hCertStore\n");

    test_store_is_empty(cert->hCertStore);

    res = CertDeleteCertificateFromStore(cert3);
    ok(res, "CertDeleteCertificateContextFromStore failed\n");
    ok(cert3->hCertStore == cert->hCertStore, "Unexpected hCertStore\n");

    CertFreeCertificateContext(cert3);

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertAddCertificateContextToStore(store, cert2, CERT_STORE_ADD_NEW, &cert3);
    ok(res, "CertAddCertificateContextToStore failed\n");
    ok(cert3 && cert3 != cert2, "Unexpected cert3\n");
    ok(cert3->hCertStore == store, "Unexpected hCertStore\n");

    res = CertDeleteCertificateFromStore(cert3);
    ok(res, "CertDeleteCertificateContextFromStore failed\n");
    ok(cert3->hCertStore == store, "Unexpected hCertStore\n");

    CertCloseStore(store, 0);
    CertFreeCertificateContext(cert3);

    res = CertCloseStore(cert->hCertStore, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(!res && GetLastError() == E_UNEXPECTED, "CertCloseStore returned: %x(%x)\n", res, GetLastError());

    res = CertCloseStore(cert->hCertStore, 0);
    ok(!res && GetLastError() == E_UNEXPECTED, "CertCloseStore returned: %x(%x)\n", res, GetLastError());

    CertFreeCertificateContext(cert2);

    crl = CertCreateCRLContext(X509_ASN_ENCODING, signedCRL, sizeof(signedCRL));
    ok(crl != NULL, "CertCreateCRLContext failed\n");
    ok(crl->hCertStore == cert->hCertStore, "unexpected hCertStore\n");

    CertFreeCRLContext(crl);

    ctl = CertCreateCTLContext(X509_ASN_ENCODING, signedCTLWithCTLInnerContent, sizeof(signedCTLWithCTLInnerContent));
    ok(ctl != NULL, "CertCreateCTLContext failed\n");
    ok(ctl->hCertStore == cert->hCertStore, "unexpected hCertStore\n");

    CertFreeCTLContext(ctl);

    CertFreeCertificateContext(cert);
}

static void testCloseStore(void)
{
    const CERT_CONTEXT *cert;
    const CRL_CONTEXT *crl;
    const CTL_CONTEXT *ctl;
    HCERTSTORE store, store2;
    BOOL res;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(res, "CertCloseStore failed\n");

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    store2 = CertDuplicateStore(store);
    ok(store2 != NULL, "CertCloneStore failed\n");
    ok(store2 == store, "unexpected store2\n");

    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(!res && GetLastError() == CRYPT_E_PENDING_CLOSE, "CertCloseStore failed\n");

    res = CertCloseStore(store2, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(res, "CertCloseStore failed\n");

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &cert);
    ok(res, "CertAddEncodedCertificateToStore failed\n");

    /* There is still a reference from cert */
    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(!res && GetLastError() == CRYPT_E_PENDING_CLOSE, "CertCloseStore failed\n");

    res = CertFreeCertificateContext(cert);
    ok(res, "CertFreeCertificateContext failed\n");

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &crl);
    ok(res, "CertAddEncodedCRLToStore failed\n");

    /* There is still a reference from CRL */
    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(!res && GetLastError() == CRYPT_E_PENDING_CLOSE, "CertCloseStore failed\n");

    res = CertFreeCRLContext(crl);
    ok(res, "CertFreeCRLContext failed\n");

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertAddEncodedCTLToStore(store, X509_ASN_ENCODING, signedCTLWithCTLInnerContent,
     sizeof(signedCTLWithCTLInnerContent), CERT_STORE_ADD_ALWAYS, &ctl);
    ok(res, "CertAddEncodedCTLToStore failed\n");

    /* There is still a reference from CTL */
    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(!res && GetLastError() == CRYPT_E_PENDING_CLOSE, "CertCloseStore returned: %x(%u)\n", res, GetLastError());

    res = CertFreeCTLContext(ctl);
    ok(res, "CertFreeCTLContext failed\n");

    /* Add all kinds of contexts, then release external references and make sure that store is properly closed. */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed\n");

    res = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &cert);
    ok(res, "CertAddEncodedCertificateToStore failed\n");

    res = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &crl);
    ok(res, "CertAddEncodedCRLToStore failed\n");

    res = CertAddEncodedCTLToStore(store, X509_ASN_ENCODING, signedCTLWithCTLInnerContent,
     sizeof(signedCTLWithCTLInnerContent), CERT_STORE_ADD_ALWAYS, &ctl);
    ok(res, "CertAddEncodedCTLToStore failed\n");

    CertFreeCertificateContext(cert);
    CertFreeCRLContext(crl);
    CertFreeCTLContext(ctl);

    res = CertCloseStore(store, CERT_CLOSE_STORE_CHECK_FLAG);
    ok(res, "CertCloseStore failed\n");
}

static void test_I_UpdateStore(void)
{
    HMODULE lib = GetModuleHandleA("crypt32");
    BOOL (WINAPI *pI_CertUpdatestore)(HCERTSTORE, HCERTSTORE, DWORD, DWORD) =
     (void *)GetProcAddress(lib, "I_CertUpdateStore");
    BOOL ret;
    HCERTSTORE store1, store2;
    PCCERT_CONTEXT cert;
    DWORD certs;

    if (!pI_CertUpdatestore)
    {
        win_skip("No I_CertUpdateStore\n");
        return;
    }
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    /* Crash
    ret = pI_CertUpdatestore(NULL, NULL, 0, 0);
    ret = pI_CertUpdatestore(store1, NULL, 0, 0);
    ret = pI_CertUpdatestore(NULL, store2, 0, 0);
     */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());

    CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &cert);
    /* I_CertUpdateStore adds the contexts from store2 to store1 */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %d\n", certs);
    /* Calling it a second time has no effect */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %d\n", certs);

    /* The last parameters to I_CertUpdateStore appear to be ignored */
    ret = pI_CertUpdatestore(store1, store2, 1, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());
    ret = pI_CertUpdatestore(store1, store2, 0, 1);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());

    CertAddEncodedCRLToStore(store2, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);

    /* I_CertUpdateStore also adds the CRLs from store2 to store1 */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %d\n", certs);
    certs = countCRLsInStore(store1);
    ok(certs == 1, "Expected 1 CRL, got %d\n", certs);

    CertDeleteCertificateFromStore(cert);
    /* If a context is deleted from store2, I_CertUpdateStore deletes it
     * from store1
     */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08x\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 0, "Expected 0 certs, got %d\n", certs);

    CertCloseStore(store1, 0);
    CertCloseStore(store2, 0);
}

START_TEST(store)
{
    /* various combinations of CertOpenStore */
    testMemStore();
    testCollectionStore();
    testStoresInCollection();

    testRegStore();
    testRegStoreSavedCerts();

    testSystemRegStore();
    testSystemStore();
    testFileStore();
    testFileNameStore();
    testMessageStore();
    testSerializedStore();
    testCloseStore();

    testCertRegisterSystemStore();

    testCertOpenSystemStore();
    testCertEnumSystemStore();
    testStoreProperty();

    testAddSerialized();
    testAddCertificateLink();

    testEmptyStore();

    test_I_UpdateStore();
}
