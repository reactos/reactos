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
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    /* weird flags */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_DELETE_FLAG, NULL);
    ok(!store1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %ld\n", GetLastError());

    /* normal */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    /* open existing doesn't */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, NULL);
    ok(store2 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    ok(store1 != store2, "Expected different stores\n");

    /* add a bogus (empty) cert */
    context = NULL;
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, emptyCert,
     sizeof(emptyCert), CERT_STORE_ADD_ALWAYS, &context);
    /* Windows returns CRYPT_E_ASN1_EOD, but accept
     * CRYPT_E_ASN1_CORRUPT as well (because matching errors is tough in this
     * case)
     */
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_EOD || GLE == CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %08lx\n",
     GLE);
    /* add a "signed" cert--the signature isn't a real signature, so this adds
     * without any check of the signature's validity
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedBigCert, sizeof(signedBigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(signedBigCert),
         "Wrong cert size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, signedBigCert,
         sizeof(signedBigCert)), "Unexpected encoded cert in context\n");
        /* remove it, the rest of the tests will work on an unsigned cert */
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
    }
    /* try adding a "signed" CRL as a cert */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    GLE = GetLastError();
    ok(!ret && (GLE == CRYPT_E_ASN1_BADTAG || GLE == CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_BADTAG or CRYPT_E_ASN1_CORRUPT, got %08lx\n",
     GLE);
    /* add a cert to store1 */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        DWORD size;
        BYTE *buf;

        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %ld\n", context->cbCertEncoded);
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
        ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(context, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n", GetLastError());
            ok(size == sizeof(serializedCert), "Wrong size %ld\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            free(buf);
        }

        ret = CertFreeCertificateContext(context);
        ok(ret, "CertFreeCertificateContext failed: %08lx\n", GetLastError());
    }
    /* verify the cert's in store1 */
    context = CertEnumCertificatesInStore(store1, NULL);
    ok(context != NULL, "Expected a valid context\n");
    context = CertEnumCertificatesInStore(store1, context);
    ok(!context && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
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

        ok(copy != NULL, "CertDuplicateCertificateContext failed: %08lx\n",
         GetLastError());
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
        /* try deleting a copy */
        ret = CertDeleteCertificateFromStore(copy);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
        /* check that the store is empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Expected an empty store\n");
    }

    /* close an empty store */
    ret = CertCloseStore(NULL, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());
    ret = CertCloseStore(store1, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());
    ret = CertCloseStore(store2, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());

    /* This seems nonsensical, but you can open a read-only mem store, only
     * it isn't read-only
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_READONLY_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    /* yep, this succeeds */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
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
    ok(ret, "CertSaveStore failed: %08lx\n", GetLastError());
    todo_wine_if (todo)
        ok(blob.cbData == cb, "%s: expected size %ld, got %ld\n", name, cb,
         blob.cbData);
    blob.pbData = malloc(blob.cbData);
    if (blob.pbData)
    {
        ret = CertSaveStore(store, X509_ASN_ENCODING, CERT_STORE_SAVE_AS_STORE,
         CERT_STORE_SAVE_TO_MEMORY, &blob, 0);
        ok(ret, "CertSaveStore failed: %08lx\n", GetLastError());
        todo_wine_if (todo)
            ok(!memcmp(pb, blob.pbData, cb), "%s: unexpected value\n", name);
        free(blob.pbData);
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
            ok (err == ERROR_ACCESS_DENIED, "Failed to create store at %ld (%08lx)\n", i, err);
            skip("Insufficient privileges for the test %ld\n", i);
            continue;
        }
        ok (store!=NULL, "Failed to open the store at %ld, %lx\n", i, GetLastError());
        cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert1 != NULL, "Create cert context failed at %ld, %lx\n", i, GetLastError());
        ret = CertAddCertificateContextToStore(store, cert1, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
        /* Additional skip per Win7, it allows opening HKLM store, but disallows adding certs */
        err = GetLastError();
        if (!ret)
        {
            ok (err == ERROR_ACCESS_DENIED, "Failed to add certificate to store at %ld (%08lx)\n", i, err);
            skip("Insufficient privileges for the test %ld\n", i);
            continue;
        }
        ok (ret, "Adding to the store failed at %ld, %lx\n", i, err);
        CertFreeCertificateContext(cert1);
        CertCloseStore(store, 0);

        wsprintfW(key_name, L"%s\\%s\\%s\\%s", reg_store_saved_certs[i].base_reg_path,
            reg_store_saved_certs[i].store_name, L"Certificates", L"6E3090715FD92356EBAE2540E622DA192602A608");

        if (!reg_store_saved_certs[i].appdata_file)
        {
            res = RegOpenKeyExW(reg_store_saved_certs[i].key, key_name, 0, KEY_ALL_ACCESS, &key);
            ok (!res, "The cert hasn't been saved at %ld, %lx\n", i, GetLastError());
            if (!res) RegCloseKey(key);
        } else
        {
            pathres = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdata_path);
            ok (pathres == S_OK,
                "Failed to get app data path at %ld (%lx)\n", pathres, GetLastError());
            if (pathres == S_OK)
            {
                PathAppendW(appdata_path, L"Microsoft\\SystemCertificates");
                PathAppendW(appdata_path, reg_store_saved_certs[i].store_name);
                PathAppendW(appdata_path, L"Certificates");
                PathAppendW(appdata_path, L"6E3090715FD92356EBAE2540E622DA192602A608");

                cert_file = CreateFileW(appdata_path, GENERIC_READ, 0, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                todo_wine ok (cert_file != INVALID_HANDLE_VALUE,
                        "Cert was not saved in AppData at %ld (%lx)\n", i, GetLastError());
                CloseHandle(cert_file);
            }
        }

        /* deleting cert from store */
        store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W,0,0,
            reg_store_saved_certs[i].cert_store, reg_store_saved_certs[i].store_name);
        ok (store!=NULL, "Failed to open the store at %ld, %lx\n", i, GetLastError());

        cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert1 != NULL, "Create cert context failed at %ld, %lx\n", i, GetLastError());

        cert2 = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
            CERT_FIND_EXISTING, cert1, NULL);
        ok (cert2 != NULL, "Failed to find cert in the store at %ld, %lx\n", i, GetLastError());

        ret = CertDeleteCertificateFromStore(cert2);
        ok (ret, "Failed to delete certificate from store at %ld, %lx\n", i, GetLastError());

        CertFreeCertificateContext(cert1);
        CertFreeCertificateContext(cert2);
        CertCloseStore(store, 0);

        res = RegOpenKeyExW(reg_store_saved_certs[i].key, key_name, 0, KEY_ALL_ACCESS, &key);
        ok (res, "The cert's registry entry should be absent at %li, %lx\n", i, GetLastError());
        if (!res) RegCloseKey(key);

        if (reg_store_saved_certs[i].appdata_file)
        {
            cert_file = CreateFileW(appdata_path, GENERIC_READ, 0, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            ok (cert_file == INVALID_HANDLE_VALUE,
                "Cert should have been absent in AppData %ld\n", i);

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
    BOOL ret;

    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
        CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != NULL, "Failed to init collection store, last error %lx\n", GetLastError());
    /* Add read-only store to collection with very high priority*/
    ro_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RO");
    ok(ro_store != NULL, "Failed to init ro store %lx\n", GetLastError());

    ret = CertAddStoreToCollection(collection, ro_store, 0, 1000);
    ok (ret, "Failed to add read-only store to collection %lx\n", GetLastError());

    cert1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    ok (cert1 != NULL, "Create cert context failed %lx\n", GetLastError());
    ret = CertAddCertificateContextToStore(collection, cert1, CERT_STORE_ADD_ALWAYS, NULL);
    ok (!ret, "Added cert to collection with single read-only store %lx\n", GetLastError());

    /* Add read-write store to collection with the lowest priority*/
    rw_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW");
    ok (rw_store != NULL, "Failed to open rw store %lx\n", GetLastError());
    ret = CertAddStoreToCollection(collection, rw_store, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok (ret, "Failed to add rw store to collection %lx\n", GetLastError());
    /** Adding certificate to collection should fall into rw store,
     *  even though prioirty of the ro_store is higher */
    ret = CertAddCertificateContextToStore(collection, cert1, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
    ok (ret, "Failed to add cert to the collection %lx\n", GetLastError());

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
        CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW2");
    ok (rw_store_2 != NULL, "Failed to init second rw store %lx\n", GetLastError());
    ret = CertAddStoreToCollection(collection, rw_store_2, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 50);
    ok (ret, "Failed to add rw_store_2 to collection %lx\n",GetLastError());

    cert2 = CertCreateCertificateContext(X509_ASN_ENCODING, signedBigCert, sizeof(signedBigCert));
    ok (cert2 != NULL, "Failed to create cert context %lx\n", GetLastError());
    ret = CertAddCertificateContextToStore(collection, cert2, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
    ok (ret, "Failed to add cert2 to the store %lx\n",GetLastError());

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
        "cert2 expected in the collection got %p, %lx\n",tcert1, GetLastError());
    tcert1 = CertEnumCertificatesInStore(collection, tcert1);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "cert1 expected in the collection got %p, %lx\n",tcert1, GetLastError());
    tcert1 = CertEnumCertificatesInStore(collection, tcert1);
    ok (tcert1==NULL,"Unexpected cert in the collection %p %lx\n",tcert1, GetLastError());

    /* checking whether certs had been saved */
    tstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"WineTest_RW");
    ok (tstore!=NULL, "Failed to open existing rw store\n");
    tcert1 = CertEnumCertificatesInStore(tstore, NULL);
    todo_wine
        ok(tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded, "cert1 wasn't saved\n");
    CertFreeCertificateContext(tcert1);
    CertCloseStore(tstore,0);

    tstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"WineTest_RW2");
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
    rw_store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW");
    tcert1 = CertEnumCertificatesInStore(rw_store, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert1->cbCertEncoded,
        "Unexpected cert in store %p\n", tcert1);
    CertFreeCertificateContext(tcert1);
    CertCloseStore(rw_store,0);

    rw_store_2 = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW2");
    tcert1 = CertEnumCertificatesInStore(rw_store_2, NULL);
    ok (tcert1 && tcert1->cbCertEncoded == cert2->cbCertEncoded,
        "Unexpected cert in store %p\n", tcert1);
    CertFreeCertificateContext(tcert1);
    CertCloseStore(rw_store_2,0);

    CertFreeCertificateContext(cert1);
    CertFreeCertificateContext(cert2);
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RO");
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW");
    CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
        CERT_STORE_DELETE_FLAG|CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest_RW2");

}

static void testCollectionStore(void)
{
    HCERTSTORE store1, store2, collection, collection2;
    PCCERT_CONTEXT context;
    BOOL ret;
    WCHAR filename[MAX_PATH];
    HANDLE file;

    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    /* Try adding a cert to any empty collection */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_ACCESSDENIED,
     "Expected E_ACCESSDENIED, got %08lx\n", GetLastError());

    /* Create and add a cert to a memory store */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    /* Add the memory store to the collection, without allowing adding */
    ret = CertAddStoreToCollection(collection, store1, 0, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
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
     "Expected E_ACCESSDENIED, got %08lx\n", GetLastError());

    /* Create a new memory store */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Try adding a store to a non-collection store */
    ret = CertAddStoreToCollection(store1, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Try adding some bogus stores */
    /* This crashes in Windows
    ret = pCertAddStoreToCollection(0, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */
    /* This "succeeds"... */
    ret = CertAddStoreToCollection(collection, 0,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* while this crashes.
    ret = pCertAddStoreToCollection(collection, 1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */

    /* Add it to the collection, this time allowing adding */
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* Check that adding to the collection is allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
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
         "Wrong size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %ld\n", context->cbCertEncoded);
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
         "Wrong size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %ld\n", context->cbCertEncoded);
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
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* check the contents of collection2 */
    context = CertEnumCertificatesInStore(collection2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection2, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection2, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection2, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %ld\n", context->cbCertEncoded);
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
    ok(store1 != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store2 != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ret = CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    ret = CertAddStoreToCollection(collection, store2, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());

    /* Check that the collection has two copies of the same cert */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %ld\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Wrong size %ld\n", context->cbCertEncoded);
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
             "Wrong size %ld\n", context->cbCertEncoded);
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
       "Didn't expect an error to be set: %08lx\n", GetLastError());

    /* After removing store2, the collection should be empty */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(collection, store2);
    ok(GetLastError() == 0xdeadbeef,
       "Didn't expect an error to be set: %08lx\n", GetLastError());
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
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    CertDeleteCertificateFromStore(context);

    CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);

    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    CertDeleteCertificateFromStore(context);

    CertCloseStore(collection, 0);
    CertCloseStore(store1, 0);

    /* Test whether a collection store can be committed */
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    SetLastError(0xdeadbeef);
    ret = CertControlStore(collection, 0, CERT_STORE_CTRL_COMMIT, NULL);
    ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

    /* Adding a mem store that can't be committed prevents a successful commit.
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    CertAddStoreToCollection(collection, store1, 0, 0);
    SetLastError(0xdeadbeef);
    ret = CertControlStore(collection, 0, CERT_STORE_CTRL_COMMIT, NULL);
    ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "expected ERROR_CALL_NOT_IMPLEMENTED, got %ld\n", GetLastError());
    CertRemoveStoreFromCollection(collection, store1);
    CertCloseStore(store1, 0);

    /* Test adding a cert to a collection with a file store, committing the
     * change to the collection, and comparing the resulting file.
     */
    if (!GetTempFileNameW(L".", L"cer", 0, filename))
        return;

    DeleteFileW(filename);
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store1 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store1 != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    CloseHandle(file);
    CertAddStoreToCollection(collection, store1, CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    CertCloseStore(store1, 0);

    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
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
     "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", GLE);
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    GLE = GetLastError();
    ok(!store && (GLE == ERROR_INVALID_HANDLE || GLE == ERROR_BADKEY),
     "Expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", GLE);

    /* Opening up any old key works.. */
    key = HKEY_CURRENT_USER;
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    /* Not sure if this is a bug in DuplicateHandle, marking todo_wine for now
     */
    todo_wine ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    CertCloseStore(store, 0);

    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
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
        ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
        /* Add a certificate.  It isn't persisted right away, since it's only
         * added to the cache..
         */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        /* so flush the cache to force a commit.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08lx\n", GetLastError());
        /* and check that the expected subkey was written. */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2, sizeof(bigCert2),
         hash, &size);
        ok(ret, "CryptHashCertificate failed: %ld\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1; i < size;
         i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
        if (subKey)
        {
            LPBYTE buf;

            size = 0;
            RegQueryValueExA(subKey, "Blob", NULL, NULL, NULL, &size);
            buf = malloc(size);
            if (buf)
            {
                rc = RegQueryValueExA(subKey, "Blob", NULL, NULL, buf, &size);
                ok(!rc, "RegQueryValueExA failed: %ld\n", rc);
                if (!rc)
                {
                    const struct CertPropIDHeader *hdr;

                    /* Both the hash and the cert should be present */
                    hdr = findPropID(buf, size, CERT_CERT_PROP_ID);
                    ok(hdr != NULL, "Expected to find a cert property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(bigCert2),
                           "Wrong size %ld of cert property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), bigCert2,
                         hdr->cb), "Unexpected cert in cert property\n");
                    }
                    hdr = findPropID(buf, size, CERT_HASH_PROP_ID);
                    ok(hdr != NULL, "Expected to find a hash property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(hash),
                           "Wrong size %ld of hash property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), hash,
                         hdr->cb), "Unexpected hash in cert property\n");
                    }
                }
                free(buf);
            }
            RegCloseKey(subKey);
        }

        /* Remove the existing context */
        context = CertEnumCertificatesInStore(store, NULL);
        ok(context != NULL, "Expected a cert context\n");
        if (context)
            CertDeleteCertificateFromStore(context);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

        /* Add a serialized cert with a bogus hash directly to the registry */
        memset(hash, 0, sizeof(hash));
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
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
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* Make sure the bogus hash cert gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %ld\n", certCount);

            RegCloseKey(subKey);
        }

        /* Add another serialized cert directly to the registry, this time
         * under the correct key name (named with the correct hash value).
         */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2,
         sizeof(bigCert2), hash, &size);
        ok(ret, "CryptHashCertificate failed: %ld\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
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
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* and make sure just one cert still gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificate, got %ld\n", certCount);

            /* Try again with the correct hash... */
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* and make sure two certs get loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 2, "Expected 2 certificates, got %ld\n", certCount);

            RegCloseKey(subKey);
        }
        CertCloseStore(store, 0);
        /* Is delete allowed on a reg store? */
        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0,
         CERT_STORE_DELETE_FLAG, key);
        ok(store == NULL, "Expected NULL return from CERT_STORE_DELETE_FLAG\n");
        ok(GetLastError() == 0, "CertOpenStore failed: %08lx\n",
         GetLastError());

        RegCloseKey(key);
    }
    /* The CertOpenStore with CERT_STORE_DELETE_FLAG above will delete the
     * contents of the key, but not the key itself.
     */
    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, &disp);
    ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
    ok(disp == REG_OPENED_EXISTING_KEY,
     "Expected REG_OPENED_EXISTING_KEY, got %ld\n", disp);
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

static void testSystemRegStore(void)
{
    HCERTSTORE store, memStore;

    /* Check with a UNICODE name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"My");
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
        ok(!ret && GetLastError() == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        CertCloseStore(memStore, 0);
    }
    CertCloseStore(store, 0);

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"Bogus");
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, L"Bogus");
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, L"Bogus");
    ok(!store, "CertOpenStore failed: %08lx\n", GetLastError());
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\SystemCertificates\\Bogus");

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0, 0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, "My");
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, L"My");
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* The name is expected to be UNICODE, check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, "My");
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
}

static void testSystemStore(void)
{
    HCERTSTORE store;
    WCHAR keyName[MAX_PATH];
    HKEY key;
    LONG rc;

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, "My");
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, L"My");
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* The name is expected to be UNICODE, first check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, "My");
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* Create the expected key */
    lstrcpyW(keyName, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH);
    lstrcatW(keyName, L"\\My");
    rc = RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_READ,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyEx failed: %ld\n", rc);
    if (!rc)
        RegCloseKey(key);
    /* Check opening with a UNICODE name, specifying the create new flag */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_CREATE_NEW_FLAG, L"My");
    ok(!store && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08lx\n", GetLastError());
    /* Now check opening with a UNICODE name, this time opening existing */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"My");
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        /* Check that it's a collection store */
        if (memStore)
        {
            BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);
            ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
            CertCloseStore(memStore, 0);
        }
        CertCloseStore(store, 0);
    }

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"Bogus");
    ok(!store, "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, L"Bogus");
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, L"Bogus");
    ok(!store, "Didn't expect a store to be returned when deleting\n");
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\SystemCertificates\\Bogus");
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
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;
    PCCERT_CONTEXT cert;
    HANDLE file;

    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08lx\n", GetLastError());

    if (!GetTempFileNameW(L".", L"cer", 0, filename))
       return;

    DeleteFileW(filename);
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, CERT_STORE_DELETE_FLAG,
     file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());

    /* A "read-only" file store.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
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
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08lx\n", GetLastError());
        /* It still has certs in memory.. */
        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
         GetLastError());
        CertFreeCertificateContext(cert);
        /* but the file size is still 0. */
        size = GetFileSize(file, NULL);
        ok(size == 0, "Expected size 0, got %ld\n", size);
        CertCloseStore(store, 0);
    }

    /* The create new flag is allowed.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08lx\n", GetLastError());
        CertCloseStore(store, 0);
    }
    /* as is the open existing flag. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08lx\n", GetLastError());
        CertCloseStore(store, 0);
    }
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
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
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
         sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());
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
static const WCHAR utf16Base64SPC[] =
L"MIICJQYJKoZIhvcNAQcCoIICFjCCAhICAQExADALBgkqhkiG9w0BBwGgggH6MIIB"
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

static void testFileNameStore(void)
{
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;
    DWORD GLE;

    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0, 0, NULL);
    GLE = GetLastError();
    ok(!store && (GLE == ERROR_PATH_NOT_FOUND || GLE == ERROR_INVALID_PARAMETER),
     "Expected ERROR_PATH_NOT_FOUND or ERROR_INVALID_PARAMETER, got %08lx\n",
     GLE);

    if (!GetTempFileNameW(L".", L"cer", 0, filename))
       return;
    DeleteFileW(filename);

    /* The two flags are mutually exclusive */
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG, filename);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());

    /* In all of the following tests, the encoding type seems to be ignored */
    if (initFileFromData(filename, bigCert, sizeof(bigCert)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
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
        ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
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
        ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(crl != NULL, "CertEnumCRLsInStore failed: %08lx\n", GetLastError());
        crl = CertEnumCRLsInStore(store, crl);
        ok(!crl, "Expected only one CRL\n");

        CertCloseStore(store, 0);
        /* Don't delete it this time, the next test uses it */
    }
    /* Now that the file exists, we can open it read-only */
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_STORE_READONLY_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    CertCloseStore(store, 0);
    DeleteFileW(filename);

    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_CREATE_NEW_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        compareStore(store, "serialized store with cert",
         serializedStoreWithCert, sizeof(serializedStoreWithCert), FALSE);
        CertCloseStore(store, 0);
    }
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING,
         signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());
        compareStore(store, "serialized store with cert and CRL",
         serializedStoreWithCertAndCRL, sizeof(serializedStoreWithCertAndCRL),
         FALSE);
        CertCloseStore(store, 0);
    }
    DeleteFileW(filename);

    if (!GetTempFileNameW(L".", L"spc", 0, filename))
       return;
    DeleteFileW(filename);

    if (initFileFromData(filename, base64SPC, sizeof(base64SPC)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
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
        ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
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
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %08lx\n", GetLastError());
    CryptMsgUpdate(msg, signedContent, sizeof(signedContent), TRUE);
    store = CertOpenStore(CERT_STORE_PROV_MSG, 0, 0, 0, msg);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
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
        ok(count == 0, "Expected 0 certificates, got %ld\n", count);

        count = 0;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 0, "Expected 0 CRLs, got %ld\n", count);

        /* Can add certs to a message store */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        count = 0;
        do {
            cert = CertEnumCertificatesInStore(store, cert);
            if (cert)
                count++;
        } while (cert);
        ok(count == 1, "Expected 1 certificate, got %ld\n", count);

        CertCloseStore(store, 0);
    }
    /* but the added certs weren't actually added to the message */
    size = sizeof(count);
    ret = CryptMsgGetParam(msg, CMSG_CERT_COUNT_PARAM, 0, &count, &size);
    ok(ret, "CryptMsgGetParam failed: %08lx\n", GetLastError());
    ok(count == 0, "Expected 0 certificates, got %ld\n", count);
    CryptMsgClose(msg);

    /* Crashes
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, NULL);
     */
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
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
        ok(count == 1, "Expected 1 certificate, got %ld\n", count);

        count = 0;
        do {
            crl = CertEnumCRLsInStore(store, crl);
            if (crl)
                count++;
        } while (crl);
        ok(count == 1, "Expected 1 CRL, got %ld\n", count);

        CertCloseStore(store, 0);
    }
    /* Encoding appears to be ignored */
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, X509_ASN_ENCODING, 0, 0,
     &blob);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Messages other than signed messages aren't allowed */
    blob.cbData = sizeof(hashContent);
    blob.pbData = (LPBYTE)hashContent;
    SetLastError(0xdeadbeef);
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(!store && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %08lx\n", GetLastError());
    blob.cbData = sizeof(hashBareContent);
    blob.pbData = (LPBYTE)hashBareContent;
    SetLastError(0xdeadbeef);
    store = CertOpenStore(CERT_STORE_PROV_PKCS7, 0, 0, 0, &blob);
    ok(!store && GetLastError() == CRYPT_E_ASN1_BADTAG,
     "Expected CRYPT_E_ASN1_BADTAG, got %08lx\n", GetLastError());
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
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SERIALIZED, 0, 0, 0, &blob);
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
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
    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08lx\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(crl != NULL, "CertEnumCRLsInStore failed: %08lx\n",
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
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* This succeeds, and on WinXP at least, the Bogus key is created under
     * HKCU (but not under HKLM, even when run as an administrator.)
     */
    store = CertOpenSystemStoreW(0, L"Bogus");
    ok(store != 0, "CertOpenSystemStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Delete it so other tests succeed next time around */
    CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, L"Bogus");
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\SystemCertificates\\Bogus");
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
    const CERT_CONTEXT *cert, *cert2;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(reg_system_store_test_data); i++) {
        cur_flag = reg_system_store_test_data[i].cert_store;
        ret = CertRegisterSystemStore(L"WineTest", cur_flag, NULL, NULL);
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
                "Store registration (dwFlags=%08x) failed, last error %lx\n", cur_flag, err);
        if (!ret)
        {
            skip("Nothing to test without registered store at %08x\n", cur_flag);
            continue;
        }

        hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_STORE_OPEN_EXISTING_FLAG | cur_flag, L"WineTest");
        ok (hstore != NULL, "Opening just registered store at %08x failed, last error %lx\n", cur_flag, GetLastError());

        cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
        ok (cert != NULL, "Failed creating cert at %08x, last error: %lx\n", cur_flag, GetLastError());
        if (cert)
        {
            ret = CertAddCertificateContextToStore(hstore, cert, CERT_STORE_ADD_NEW, NULL);
            ok (ret, "Failed to add cert at %08x, last error: %lx\n", cur_flag, GetLastError());

            cert2 = CertEnumCertificatesInStore(hstore, NULL);
            ok (cert2 != NULL && cert2->cbCertEncoded == cert->cbCertEncoded,
                "Unexpected cert encoded size at %08x, last error: %lx\n", cur_flag, GetLastError());

            ret = CertDeleteCertificateFromStore(cert2);
            ok (ret, "Failed to delete certificate from the new store at %08x, last error: %lx\n", cur_flag, GetLastError());

            CertFreeCertificateContext(cert);
        }

        ret = CertCloseStore(hstore, 0);
        ok (ret, "CertCloseStore failed at %08x, last error %lx\n", cur_flag, GetLastError());

        ret = CertUnregisterSystemStore(L"WineTest", cur_flag );
        todo_wine_if (reg_system_store_test_data[i].todo)
            ok( ret == reg_system_store_test_data[i].expected,
                "Unregistering failed at %08x, last error %ld\n", cur_flag, GetLastError());
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
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
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
    ok(ret, "CertEnumSystemStore failed: %08lx\n", GetLastError());
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
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
    /* Contrary to MSDN, CERT_ACCESS_STATE_PROP_ID is supported for stores.. */
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08lx\n",
     GetLastError());
    ok(!state, "Expected a non-persisted store\n");
    /* and CERT_STORE_LOCALIZED_NAME_PROP_ID isn't supported by default. */
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
    /* Delete an arbitrary property on a store */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08lx\n", GetLastError());
    /* Set an arbitrary property on a store */
    blob.pbData = (LPBYTE)&state;
    blob.cbData = sizeof(state);
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, &blob);
    ok(ret, "CertSetStoreProperty failed: %08lx\n", GetLastError());
    /* Get an arbitrary property that's been set */
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(ret, "CertGetStoreProperty failed: %08lx\n", GetLastError());
    ok(size == sizeof(state), "Unexpected data size %ld\n", size);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, &propID, &size);
    ok(ret, "CertGetStoreProperty failed: %08lx\n", GetLastError());
    ok(propID == state, "CertGetStoreProperty got the wrong value\n");
    /* Delete it again */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08lx\n", GetLastError());
    /* And check that it's missing */
    SetLastError(0xdeadbeef);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
    CertCloseStore(store, 0);

    /* Recheck on the My store.. */
    store = CertOpenSystemStoreW(0, L"My");
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08lx\n",
     GetLastError());
    ok(state, "Expected a persisted store\n");
    SetLastError(0xdeadbeef);
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
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
     "Expected ERROR_END_OF_MEDIA, got %08lx\n", GetLastError());

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddSerializedElementToStore(store, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08lx\n", GetLastError());

    /* Test with an empty property */
    hdr = (struct CertPropIDHeader *)buf;
    hdr->propID = CERT_CERT_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = 0;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Test with a bad size in property header */
    hdr->cb = sizeof(bigCert) - 1;
    memcpy(buf + sizeof(struct CertPropIDHeader), bigCert, sizeof(bigCert));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Kosher size in property header, but no context type */
    hdr->cb = sizeof(bigCert);
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* With a bad context type */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0,
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0,
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Bad unknown field, good type */
    hdr->unknown1 = 2;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0,
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
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
    ok(ret, "CertAddSerializedElementToStore failed: %08lx\n", GetLastError());
    /* Everything okay, check it's not re-added */
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08lx\n", GetLastError());

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
    ok(ret, "CertAddSerializedElementToStore failed: %08lx\n", GetLastError());
    if (context)
    {
        BYTE hashVal[20], realHash[20];
        DWORD size = sizeof(hashVal);

        ret = CryptHashCertificate(0, 0, 0, bigCert, sizeof(bigCert),
         realHash, &size);
        ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID,
         hashVal, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
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
    WCHAR subkey_name[32];
    DWORD num_subkeys, subkey_name_len;
    int idx;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\SystemCertificates", 0, KEY_READ, &root_key))
        return;
    if (RegOpenKeyExW(root_key, L"WineTest", 0, KEY_READ, &test_key))
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
    RegDeleteKeyW(root_key, L"WineTest");
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
     "expected E_INVALIDARG, got %08lx\n", GetLastError());
    source = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    SetLastError(0xdeadbeef);
    ret = CertAddCertificateLinkToStore(store1, source, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddCertificateLinkToStore(store1, source, CERT_STORE_ADD_ALWAYS,
     NULL);
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
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
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store1, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
             GetLastError());
            /* The serialized linked certificate is identical to the serialized
             * original certificate.
             */
            ok(size == sizeof(serializedCert), "Wrong size %ld\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            free(buf);
        }
        /* Set a friendly name on the source certificate... */
        blob.pbData = (LPBYTE)L"WineTest";
        blob.cbData = sizeof(L"WineTest");
        ret = CertSetCertificateContextProperty(source,
         CERT_FRIENDLY_NAME_PROP_ID, 0, &blob);
        ok(ret, "CertSetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        /* and the linked certificate has the same friendly name. */
        ret = CertGetCertificateContextProperty(linked,
         CERT_FRIENDLY_NAME_PROP_ID, NULL, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertGetCertificateContextProperty(linked,
             CERT_FRIENDLY_NAME_PROP_ID, buf, &size);
            ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
             GetLastError());
            ok(!lstrcmpW((LPCWSTR)buf, L"WineTest"),
             "unexpected friendly name\n");
            free(buf);
        }
        CertFreeCertificateContext(linked);
    }
    CertFreeCertificateContext(source);
    CertCloseStore(store1, 0);

    /* Test adding a cert to a file store, committing the change to the store,
     * and creating a link to the resulting cert.
     */
    if (!GetTempFileNameW(L".", L"cer", 0, filename1))
       return;

    DeleteFileW(filename1);
    file = CreateFileW(filename1, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store1 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store1 != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    CloseHandle(file);

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &source);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
     GetLastError());

    /* Test adding a link to a memory store. */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            /* The serialized linked certificate is identical to the serialized
             * original certificate.
             */
            ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n", GetLastError());
            ok(size == sizeof(serializedCert), "Wrong size %ld\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            free(buf);
        }
        /* Set a friendly name on the source certificate... */
        blob.pbData = (LPBYTE)L"WineTest";
        blob.cbData = sizeof(L"WineTest");
        ret = CertSetCertificateContextProperty(source,
         CERT_FRIENDLY_NAME_PROP_ID, 0, &blob);
        ok(ret, "CertSetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        /* and the linked certificate has the same friendly name. */
        ret = CertGetCertificateContextProperty(linked,
         CERT_FRIENDLY_NAME_PROP_ID, NULL, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertGetCertificateContextProperty(linked,
             CERT_FRIENDLY_NAME_PROP_ID, buf, &size);
            ok(ret, "CertGetCertificateContextProperty failed: %08lx\n", GetLastError());
            ok(!lstrcmpW((LPCWSTR)buf, L"WineTest"),
             "unexpected friendly name\n");
            free(buf);
        }
        CertFreeCertificateContext(linked);
    }
    CertCloseStore(store2, 0);

    if (!GetTempFileNameW(L".", L"cer", 0, filename2))
       return;

    DeleteFileW(filename2);
    file = CreateFileW(filename2, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store2 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store2 != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    CloseHandle(file);
    /* Test adding a link to a file store. */
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(linked->hCertStore == store2, "unexpected store\n");
        ret = CertSerializeCertificateStoreElement(linked, 0, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
         GetLastError());
        buf = malloc(size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(linked, 0, buf, &size);
            ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
             GetLastError());
            /* The serialized linked certificate now contains the friendly
             * name property.
             */
            ok(size == sizeof(serializedCertWithFriendlyName),
             "Wrong size %ld\n", size);
            ok(!memcmp(serializedCertWithFriendlyName, buf, size),
             "Unexpected serialized cert\n");
            free(buf);
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
     "expected E_INVALIDARG, got %08lx\n", GetLastError());
    CertFreeCertificateContext(source);

    /* Test adding a link to a file store, where the linked certificate is
     * in a system store.
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &source);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
     GetLastError());
    if (!GetTempFileNameW(L".", L"cer", 0, filename1))
       return;

    DeleteFileW(filename1);
    file = CreateFileW(filename1, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store2 = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store2 != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    CloseHandle(file);

    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
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
     CERT_SYSTEM_STORE_CURRENT_USER, L"WineTest");
    ok(store2 != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
    ret = CertAddCertificateLinkToStore(store2, source, CERT_STORE_ADD_ALWAYS,
     &linked);
    ok(ret, "CertAddCertificateLinkToStore failed: %08lx\n", GetLastError());
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
    ok(!res && GetLastError() == E_UNEXPECTED, "CertCloseStore returned: %x(%lx)\n", res, GetLastError());

    res = CertCloseStore(cert->hCertStore, 0);
    ok(!res && GetLastError() == E_UNEXPECTED, "CertCloseStore returned: %x(%lx)\n", res, GetLastError());

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
    ok(!res && GetLastError() == CRYPT_E_PENDING_CLOSE, "CertCloseStore returned: %x(%lu)\n", res, GetLastError());

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
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());

    CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &cert);
    /* I_CertUpdateStore adds the contexts from store2 to store1 */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %ld\n", certs);
    /* Calling it a second time has no effect */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %ld\n", certs);

    /* The last parameters to I_CertUpdateStore appear to be ignored */
    ret = pI_CertUpdatestore(store1, store2, 1, 0);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());
    ret = pI_CertUpdatestore(store1, store2, 0, 1);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());

    CertAddEncodedCRLToStore(store2, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);

    /* I_CertUpdateStore also adds the CRLs from store2 to store1 */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 1, "Expected 1 cert, got %ld\n", certs);
    certs = countCRLsInStore(store1);
    ok(certs == 1, "Expected 1 CRL, got %ld\n", certs);

    CertDeleteCertificateFromStore(cert);
    /* If a context is deleted from store2, I_CertUpdateStore deletes it
     * from store1
     */
    ret = pI_CertUpdatestore(store1, store2, 0, 0);
    ok(ret, "I_CertUpdateStore failed: %08lx\n", GetLastError());
    certs = countCertsInStore(store1);
    ok(certs == 0, "Expected 0 certs, got %ld\n", certs);

    CertCloseStore(store1, 0);
    CertCloseStore(store2, 0);
}

static const BYTE pfxdata[] =
{
  0x30, 0x82, 0x0b, 0x1d, 0x02, 0x01, 0x03, 0x30, 0x82, 0x0a, 0xe3, 0x06,
  0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x82,
  0x0a, 0xd4, 0x04, 0x82, 0x0a, 0xd0, 0x30, 0x82, 0x0a, 0xcc, 0x30, 0x82,
  0x05, 0x07, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07,
  0x06, 0xa0, 0x82, 0x04, 0xf8, 0x30, 0x82, 0x04, 0xf4, 0x02, 0x01, 0x00,
  0x30, 0x82, 0x04, 0xed, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
  0x01, 0x07, 0x01, 0x30, 0x1c, 0x06, 0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x01, 0x0c, 0x01, 0x06, 0x30, 0x0e, 0x04, 0x08, 0xac, 0x3e, 0x35,
  0xa8, 0xed, 0x0d, 0x50, 0x07, 0x02, 0x02, 0x08, 0x00, 0x80, 0x82, 0x04,
  0xc0, 0x5a, 0x62, 0x55, 0x25, 0xf6, 0x2c, 0xf1, 0x78, 0x6c, 0x63, 0x96,
  0x8a, 0xea, 0x04, 0x64, 0xb3, 0x99, 0x3b, 0x80, 0x50, 0x05, 0x37, 0x55,
  0xa3, 0x5e, 0x9f, 0x35, 0xc3, 0x3c, 0xdc, 0xf6, 0xc4, 0xc1, 0x39, 0xa2,
  0xd7, 0x50, 0xad, 0xf9, 0x29, 0x3c, 0x51, 0xea, 0x15, 0x20, 0x25, 0xd3,
  0x4d, 0x69, 0xdf, 0x10, 0xd8, 0x9d, 0x60, 0x78, 0x8a, 0x70, 0x44, 0x7f,
  0x01, 0x4f, 0x4a, 0xfa, 0xab, 0xfd, 0x46, 0x48, 0x96, 0x2b, 0x69, 0xfc,
  0x11, 0xf8, 0x3f, 0xd3, 0x79, 0x09, 0x75, 0x81, 0x47, 0xdf, 0xce, 0xfe,
  0x07, 0x2f, 0x0a, 0xd8, 0xac, 0x87, 0x14, 0x1f, 0x7b, 0x95, 0x70, 0xee,
  0x7e, 0x52, 0x90, 0x11, 0xd6, 0x69, 0xf4, 0xd5, 0x38, 0x85, 0xc9, 0xc1,
  0x07, 0x01, 0xe8, 0xbb, 0xfb, 0xe2, 0x08, 0xa8, 0xfa, 0xbf, 0xf0, 0x92,
  0x63, 0x1d, 0xbb, 0x2b, 0x45, 0x6f, 0xce, 0x97, 0x01, 0xd7, 0x95, 0xf0,
  0x9c, 0x9a, 0x6b, 0x73, 0x01, 0xbf, 0xf9, 0x3d, 0xc8, 0x2b, 0x86, 0x7a,
  0xd5, 0x65, 0x84, 0xd7, 0xff, 0xb2, 0xf9, 0x20, 0x52, 0x35, 0xc5, 0x60,
  0x33, 0x70, 0x1d, 0x2f, 0x26, 0x09, 0x1c, 0x22, 0x17, 0xd8, 0x08, 0x4e,
  0x69, 0x20, 0xe2, 0x71, 0xe4, 0x07, 0xb1, 0x48, 0x5f, 0x20, 0x08, 0x7a,
  0xbf, 0x65, 0x53, 0x23, 0x07, 0xf9, 0x6c, 0xde, 0x3e, 0x29, 0xbf, 0x6b,
  0xef, 0xbb, 0x6a, 0x5f, 0x79, 0xa1, 0x72, 0xa1, 0x10, 0x24, 0x80, 0xb4,
  0x44, 0xb8, 0xc9, 0xfc, 0xa3, 0x36, 0x7e, 0x23, 0x37, 0x58, 0xc6, 0x1e,
  0xe8, 0x42, 0x4d, 0xb5, 0xf5, 0x58, 0x93, 0x21, 0x38, 0xa2, 0xc4, 0xa9,
  0x01, 0x96, 0xf9, 0x61, 0xac, 0x55, 0xb3, 0x3d, 0xe4, 0x54, 0x8b, 0x6c,
  0xc3, 0x83, 0xff, 0x50, 0x87, 0x94, 0xe8, 0x35, 0x3c, 0x26, 0x0d, 0x20,
  0x8a, 0x25, 0x0e, 0xb6, 0x67, 0x78, 0x29, 0xc7, 0xbf, 0x76, 0x8e, 0x62,
  0x62, 0xc4, 0x50, 0xd6, 0xc5, 0x3c, 0xb4, 0x7a, 0x35, 0xbe, 0x53, 0x52,
  0xc4, 0xe4, 0x10, 0xb3, 0xe0, 0x73, 0xb0, 0xd1, 0xc1, 0x5a, 0x4f, 0x4e,
  0x64, 0x0d, 0x92, 0x51, 0x2d, 0x4d, 0xec, 0xb0, 0xc6, 0x40, 0x1b, 0x03,
  0x89, 0x7f, 0xc2, 0x2c, 0xe3, 0x2c, 0xbd, 0x8c, 0x9c, 0xd9, 0xe0, 0x08,
  0x59, 0xd3, 0xaf, 0x48, 0x56, 0x89, 0x60, 0x85, 0x76, 0xe0, 0xd8, 0x7c,
  0xcf, 0x02, 0x8f, 0xfd, 0xb2, 0x8f, 0x2b, 0x61, 0xcf, 0x28, 0x56, 0x8b,
  0x6b, 0x03, 0x2b, 0x2f, 0x83, 0x31, 0xa0, 0x1c, 0xd1, 0x6c, 0x87, 0x49,
  0xc4, 0x77, 0x55, 0x1f, 0x61, 0x45, 0x58, 0x88, 0x9f, 0x01, 0xc3, 0x63,
  0x62, 0x30, 0x35, 0xdf, 0x61, 0x74, 0x55, 0x63, 0x3f, 0xae, 0x41, 0xc1,
  0xb8, 0xf0, 0x9f, 0xab, 0x25, 0xad, 0x41, 0x5c, 0x1f, 0x00, 0x0d, 0xef,
  0xf0, 0xcf, 0xaf, 0x41, 0x23, 0xca, 0x8c, 0x38, 0xea, 0x5a, 0xe4, 0x8b,
  0xb4, 0x89, 0xd0, 0x76, 0x7f, 0x2b, 0x77, 0x8f, 0xe4, 0x44, 0xd5, 0x37,
  0xac, 0xc2, 0x09, 0x7e, 0x7e, 0x7e, 0x02, 0x5c, 0x27, 0x01, 0xcb, 0x4d,
  0xea, 0xb3, 0x97, 0x36, 0x35, 0xd2, 0x05, 0x3c, 0x4e, 0xb8, 0x04, 0x5c,
  0xb8, 0x95, 0x3f, 0xc6, 0xbf, 0xd4, 0x20, 0x01, 0xfb, 0xed, 0x37, 0x5a,
  0xad, 0x4c, 0x61, 0x93, 0xfe, 0x95, 0x7c, 0x34, 0x11, 0x15, 0x9d, 0x00,
  0x0b, 0x99, 0x69, 0xcb, 0x7e, 0xb9, 0x53, 0x46, 0x57, 0x39, 0x3f, 0x59,
  0x4b, 0x30, 0x8d, 0xfb, 0x84, 0x66, 0x2d, 0x06, 0xc9, 0x88, 0xa6, 0x18,
  0xd7, 0x36, 0xc6, 0xf6, 0xf7, 0x47, 0x85, 0x38, 0xc8, 0x3d, 0x37, 0xea,
  0x57, 0x4c, 0xb0, 0x7c, 0x95, 0x29, 0x84, 0xab, 0xbb, 0x19, 0x86, 0xc2,
  0xc5, 0x99, 0x01, 0x38, 0x6b, 0xf1, 0xd3, 0x1d, 0xa8, 0x02, 0xf9, 0x6f,
  0xaa, 0xf1, 0x57, 0xd0, 0x88, 0x68, 0x62, 0x5f, 0x9f, 0x7a, 0x63, 0xba,
  0x3a, 0xc9, 0x95, 0x11, 0x3c, 0xf9, 0xa1, 0xc1, 0x35, 0xfe, 0xd5, 0x12,
  0x49, 0x88, 0x0d, 0x5c, 0xe2, 0xd1, 0x15, 0x18, 0xfb, 0xd5, 0x7f, 0x19,
  0x3f, 0xaf, 0xa0, 0xcb, 0x31, 0x20, 0x9e, 0x03, 0x93, 0xa4, 0x66, 0xbd,
  0x83, 0xe8, 0x60, 0x34, 0x55, 0x0d, 0x97, 0x10, 0x23, 0x24, 0x7a, 0x45,
  0x36, 0xb4, 0xc4, 0xee, 0x60, 0x6f, 0xd8, 0x46, 0xc5, 0xac, 0x2b, 0xa9,
  0x18, 0x74, 0x83, 0x1e, 0xdf, 0x7c, 0x1a, 0x5a, 0xe8, 0x5f, 0x8b, 0x4f,
  0x9f, 0x40, 0x3e, 0x5e, 0xfb, 0xd3, 0x68, 0xac, 0x34, 0x62, 0x30, 0x23,
  0xb6, 0xbc, 0xdf, 0xbc, 0xc7, 0x25, 0xd2, 0x1b, 0x57, 0x33, 0xfb, 0x78,
  0x22, 0x21, 0x1e, 0x3a, 0xf6, 0x44, 0x18, 0x7e, 0x12, 0x36, 0x47, 0x58,
  0xd0, 0x59, 0x26, 0x98, 0x98, 0x95, 0xf4, 0xd1, 0xaa, 0x45, 0xaa, 0xe7,
  0xd1, 0xe6, 0x2d, 0x78, 0xf0, 0x8b, 0x1c, 0xfd, 0xf8, 0x50, 0x60, 0xa2,
  0x1e, 0x7f, 0xe3, 0x31, 0x77, 0x31, 0x58, 0x99, 0x0f, 0xda, 0x0e, 0xa3,
  0xc6, 0x7a, 0x30, 0x45, 0x55, 0x11, 0x91, 0x77, 0x41, 0x79, 0xd3, 0x56,
  0xb2, 0x07, 0x00, 0x61, 0xab, 0xec, 0x27, 0xc7, 0x9f, 0xfa, 0x89, 0x08,
  0xc2, 0x87, 0xcf, 0xe9, 0xdc, 0x9e, 0x29, 0x22, 0xfb, 0x23, 0x7f, 0x9d,
  0x89, 0xd5, 0x6e, 0x75, 0x20, 0xd8, 0x00, 0x5b, 0xc4, 0x94, 0xbb, 0xc5,
  0xb2, 0xba, 0x77, 0x2b, 0xf6, 0x3c, 0x88, 0xb0, 0x4c, 0x38, 0x46, 0x55,
  0xee, 0x8b, 0x03, 0x15, 0xbc, 0x0a, 0x1d, 0x47, 0x87, 0x44, 0xaf, 0xb1,
  0x2a, 0xa7, 0x4d, 0x08, 0xdf, 0x3b, 0x2d, 0x70, 0xa1, 0x67, 0x31, 0x76,
  0x6e, 0x6f, 0x40, 0x3b, 0x3b, 0xe8, 0xf9, 0xdf, 0x90, 0xa4, 0xce, 0x7f,
  0xb8, 0x2d, 0x69, 0xcb, 0x1c, 0x1e, 0x94, 0xcd, 0xb1, 0xd8, 0x43, 0x22,
  0xb8, 0x4f, 0x98, 0x92, 0x74, 0xb3, 0xde, 0xeb, 0x7a, 0xcb, 0xfa, 0xd0,
  0x36, 0xe4, 0x5d, 0xfa, 0xd3, 0xce, 0xf9, 0xba, 0x3e, 0x0f, 0x6c, 0xc3,
  0x5b, 0xb3, 0x81, 0x84, 0x6e, 0x5d, 0xc1, 0x21, 0x89, 0xec, 0x67, 0x9a,
  0xfd, 0x55, 0x20, 0xb0, 0x71, 0x53, 0xae, 0xf8, 0xa4, 0x8d, 0xd5, 0xe5,
  0x2d, 0x3a, 0xce, 0x89, 0x55, 0x8c, 0x4f, 0x3b, 0x37, 0x95, 0x4e, 0x15,
  0xbe, 0xe7, 0xd1, 0x7a, 0x36, 0x82, 0x45, 0x69, 0x7c, 0x27, 0x4f, 0xb9,
  0x4b, 0x7d, 0xcd, 0x59, 0xc8, 0xf4, 0x8b, 0x0f, 0x4f, 0x75, 0x23, 0xd3,
  0xd0, 0xc7, 0x10, 0x79, 0xc0, 0xf1, 0xac, 0x14, 0xf7, 0x0d, 0xc8, 0x5e,
  0xfc, 0xff, 0x1a, 0x2b, 0x10, 0x88, 0x7e, 0x7e, 0x2f, 0xfa, 0x7b, 0x9f,
  0x47, 0x23, 0x34, 0xfc, 0xf5, 0xde, 0xd9, 0xa3, 0x05, 0x99, 0x2a, 0x96,
  0x83, 0x3d, 0xa4, 0x7f, 0x6a, 0x66, 0x9b, 0xe7, 0xf1, 0x00, 0x4e, 0x9a,
  0xfc, 0x68, 0xd2, 0x74, 0x17, 0xba, 0xc9, 0xc8, 0x20, 0x39, 0xa1, 0xa8,
  0x85, 0xc6, 0x10, 0x2b, 0xab, 0x97, 0x34, 0x2d, 0x49, 0x68, 0x57, 0xb0,
  0x43, 0xee, 0x25, 0xbb, 0x35, 0x1b, 0x03, 0x99, 0xa3, 0x21, 0x68, 0x66,
  0x86, 0x3f, 0xc6, 0xfc, 0x49, 0xf0, 0xba, 0x5f, 0x00, 0xc6, 0xe3, 0x1c,
  0xb2, 0x9f, 0x16, 0x7f, 0xc7, 0x40, 0x4a, 0x9a, 0x39, 0xc1, 0x95, 0x69,
  0xa2, 0x87, 0xba, 0x58, 0xc6, 0xf2, 0xd6, 0x66, 0xa6, 0x4c, 0x6d, 0x29,
  0x9c, 0xa8, 0x6e, 0xa9, 0xd2, 0xe4, 0x54, 0x17, 0x89, 0xe2, 0x43, 0xf0,
  0xe1, 0x8b, 0x57, 0x84, 0x6c, 0x87, 0x63, 0x17, 0xbb, 0xf6, 0x33, 0x1b,
  0xe4, 0x34, 0x6a, 0x80, 0x70, 0x7b, 0x1b, 0xfd, 0xf8, 0x79, 0x28, 0xc8,
  0x3c, 0x8e, 0xa4, 0xd5, 0xb8, 0x96, 0x54, 0xd4, 0xec, 0x72, 0xe5, 0x40,
  0x8f, 0x56, 0xde, 0x82, 0x15, 0x72, 0x4d, 0xd8, 0x0c, 0x07, 0xea, 0xe6,
  0x44, 0xcd, 0x94, 0x73, 0x5c, 0x04, 0xe8, 0x8e, 0xb7, 0xc7, 0xc9, 0x29,
  0xdc, 0x04, 0xef, 0x7c, 0x31, 0x9b, 0x50, 0xbc, 0xea, 0x71, 0x1f, 0x28,
  0x22, 0xb6, 0x04, 0x53, 0x2e, 0x71, 0xc4, 0xf6, 0xbb, 0x88, 0x51, 0xee,
  0x3e, 0x76, 0x65, 0xb4, 0x4b, 0x1b, 0xa3, 0xec, 0x7b, 0xa7, 0x9d, 0x31,
  0x5d, 0xb8, 0x9f, 0xab, 0x6b, 0x54, 0x7d, 0xbd, 0xc1, 0x2c, 0x55, 0xb0,
  0x23, 0x8c, 0x06, 0x60, 0x01, 0x4f, 0x60, 0x85, 0x56, 0x7f, 0xfb, 0x99,
  0x0c, 0xdc, 0x8c, 0x09, 0x37, 0x46, 0x5b, 0x97, 0x5d, 0xe8, 0x31, 0x00,
  0x1b, 0x30, 0x9b, 0x02, 0x92, 0x29, 0xb5, 0x20, 0xce, 0x4b, 0x90, 0xfb,
  0x91, 0x07, 0x5a, 0xd3, 0xf5, 0xa0, 0xe6, 0x8f, 0xf8, 0x73, 0xc5, 0x4b,
  0xbb, 0xad, 0x2a, 0xeb, 0xa8, 0xb7, 0x68, 0x34, 0x36, 0x47, 0xd5, 0x4b,
  0x61, 0x89, 0x53, 0xe6, 0xb6, 0xb1, 0x07, 0xe4, 0x08, 0x2e, 0xed, 0x50,
  0xd4, 0x1e, 0xed, 0x7f, 0xbf, 0x35, 0x68, 0x04, 0x45, 0x72, 0x86, 0x71,
  0x15, 0x55, 0xdf, 0xe6, 0x30, 0xc0, 0x8b, 0x8a, 0xb0, 0x6c, 0xd0, 0x35,
  0x57, 0x8f, 0x04, 0x37, 0xbc, 0xe1, 0xb8, 0xbf, 0x27, 0x37, 0x3d, 0xd0,
  0xc8, 0x46, 0x67, 0x42, 0x51, 0x30, 0x82, 0x05, 0xbd, 0x06, 0x09, 0x2a,
  0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x82, 0x05, 0xae,
  0x04, 0x82, 0x05, 0xaa, 0x30, 0x82, 0x05, 0xa6, 0x30, 0x82, 0x05, 0xa2,
  0x06, 0x0b, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x0a, 0x01,
  0x02, 0xa0, 0x82, 0x04, 0xee, 0x30, 0x82, 0x04, 0xea, 0x30, 0x1c, 0x06,
  0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x01, 0x03, 0x30,
  0x0e, 0x04, 0x08, 0x9f, 0xa4, 0x72, 0x2b, 0x6b, 0x0e, 0xcb, 0x9f, 0x02,
  0x02, 0x08, 0x00, 0x04, 0x82, 0x04, 0xc8, 0xe5, 0x35, 0xb9, 0x72, 0x28,
  0x20, 0x28, 0xad, 0xe3, 0x01, 0xd7, 0x0b, 0xe0, 0x4e, 0x36, 0xc3, 0x73,
  0x06, 0xd5, 0xf6, 0x75, 0x1a, 0x78, 0xb2, 0xd8, 0xf6, 0x5a, 0x85, 0x8e,
  0x50, 0xa3, 0x05, 0x49, 0x02, 0x2d, 0xf8, 0xa3, 0x2f, 0xe6, 0x02, 0x7a,
  0xd5, 0x0b, 0x1d, 0xf1, 0xd1, 0xe4, 0x16, 0xaa, 0x70, 0x2e, 0x34, 0xdb,
  0x56, 0xd9, 0x33, 0x94, 0x11, 0xaa, 0x60, 0xd4, 0xfa, 0x5b, 0xd1, 0xb3,
  0x2e, 0x86, 0x6a, 0x5a, 0x69, 0xdf, 0x11, 0x91, 0xb0, 0xca, 0x82, 0xff,
  0x63, 0xad, 0x6a, 0x0b, 0x90, 0xa6, 0xc7, 0x9b, 0xef, 0x9a, 0xf8, 0x96,
  0xec, 0xe4, 0xc4, 0xdf, 0x55, 0x4c, 0x12, 0x07, 0xab, 0x7c, 0x5c, 0x68,
  0x47, 0xf2, 0x92, 0xfb, 0x94, 0xab, 0xc3, 0x64, 0xd3, 0xfe, 0xb2, 0x16,
  0xb4, 0x78, 0x80, 0x52, 0xe9, 0x32, 0x39, 0x3b, 0x8d, 0x12, 0x91, 0x36,
  0xfd, 0xa1, 0x97, 0xc2, 0x0a, 0x4a, 0xf1, 0xb3, 0x8a, 0xe4, 0x01, 0xed,
  0x0a, 0xda, 0x2e, 0xa0, 0x38, 0xa9, 0x47, 0x3d, 0x3a, 0x64, 0x87, 0x06,
  0xc3, 0x83, 0x60, 0xaf, 0x84, 0xdb, 0x87, 0xff, 0x70, 0x61, 0x43, 0x7d,
  0x2d, 0x61, 0x9a, 0xf7, 0x0d, 0xca, 0x0c, 0x0f, 0xbe, 0x43, 0x5b, 0x99,
  0xe1, 0x90, 0x64, 0x1f, 0xa7, 0x1b, 0xa6, 0xa6, 0x5c, 0x13, 0x70, 0xa3,
  0xdb, 0xd7, 0xf0, 0xe8, 0x7a, 0xb0, 0xd1, 0x9b, 0x52, 0xa6, 0x4f, 0xd6,
  0xff, 0x54, 0x4d, 0xa6, 0x15, 0x05, 0x5c, 0xe9, 0x04, 0x6a, 0xc3, 0x49,
  0x12, 0x2f, 0x24, 0x03, 0xc3, 0x80, 0x06, 0xa6, 0x07, 0x8b, 0x96, 0xe7,
  0x39, 0x31, 0x6d, 0xd3, 0x1b, 0xa5, 0x45, 0x58, 0x04, 0xe7, 0x87, 0xdf,
  0x26, 0xfb, 0x1b, 0x9f, 0x92, 0x93, 0x32, 0x12, 0x9a, 0xc9, 0xe6, 0xcb,
  0x88, 0x14, 0x9f, 0x23, 0x0b, 0x52, 0xa2, 0xb8, 0x32, 0x6c, 0xa9, 0x33,
  0xa1, 0x17, 0xe8, 0x4a, 0xd4, 0x5c, 0x7d, 0xb3, 0xa3, 0x64, 0x86, 0x03,
  0x7c, 0x7c, 0x3f, 0x99, 0xdc, 0x21, 0x9f, 0x93, 0xc6, 0xb9, 0x1d, 0xe0,
  0x21, 0x79, 0x78, 0x35, 0xdc, 0x1e, 0x27, 0x3c, 0x73, 0x7f, 0x0f, 0xd6,
  0x4f, 0xde, 0xe9, 0xb4, 0xb7, 0xe3, 0xf5, 0x72, 0xce, 0x42, 0xf3, 0x91,
  0x5b, 0x84, 0xba, 0xbb, 0xae, 0xf0, 0x87, 0x0f, 0x50, 0xa4, 0x5e, 0x80,
  0x23, 0x57, 0x2b, 0xa0, 0xa3, 0xc3, 0x8a, 0x2f, 0xa8, 0x7a, 0x1a, 0x65,
  0x8f, 0x62, 0xf8, 0x3e, 0xe2, 0xcd, 0xbc, 0x63, 0x56, 0x8e, 0x77, 0xf3,
  0xf9, 0x69, 0x10, 0x57, 0xa8, 0xaf, 0x67, 0x2a, 0x9f, 0x7f, 0x7e, 0xeb,
  0x1d, 0x99, 0xa6, 0x67, 0xcd, 0x9e, 0x42, 0x2e, 0x5e, 0x4e, 0x61, 0x24,
  0xfa, 0xca, 0x2a, 0xeb, 0x62, 0x1f, 0xa3, 0x14, 0x0a, 0x06, 0x4b, 0x77,
  0x78, 0x77, 0x9b, 0xf1, 0x03, 0xcc, 0xb5, 0xfe, 0xfb, 0x7a, 0x77, 0xa6,
  0x82, 0x9f, 0xe5, 0xde, 0x9d, 0x0d, 0x4d, 0x37, 0xc6, 0x12, 0x73, 0x6d,
  0xea, 0xbb, 0x48, 0xf0, 0xd2, 0x81, 0xcc, 0x1a, 0x47, 0xfa, 0xa4, 0xd2,
  0xb2, 0x27, 0xa0, 0xfc, 0x30, 0x04, 0xdb, 0x05, 0xd3, 0x0b, 0xbc, 0x4d,
  0x7a, 0x99, 0xef, 0x7f, 0x26, 0x01, 0xd4, 0x07, 0x0b, 0x1e, 0x99, 0x06,
  0x3c, 0xde, 0x3d, 0x1c, 0x21, 0x82, 0x68, 0x46, 0x35, 0x38, 0x61, 0xea,
  0xd4, 0xc2, 0x65, 0x09, 0x39, 0x87, 0xb4, 0xd3, 0x5d, 0x3c, 0xa3, 0x79,
  0xe4, 0x01, 0x4e, 0xbf, 0x18, 0xba, 0x57, 0x3f, 0xdd, 0xea, 0x0a, 0x6b,
  0x99, 0xfb, 0x93, 0xfa, 0xab, 0xee, 0x08, 0xdf, 0x38, 0x23, 0xae, 0x8d,
  0xa8, 0x03, 0x13, 0xfe, 0x83, 0x88, 0xb0, 0xc2, 0xf9, 0x90, 0xa5, 0x1c,
  0x01, 0x6f, 0x71, 0x91, 0x42, 0x35, 0x81, 0x74, 0x71, 0x6c, 0xba, 0x86,
  0x48, 0xfe, 0x96, 0xd2, 0x88, 0x12, 0x36, 0x4e, 0xa6, 0x2f, 0xd1, 0xdb,
  0xfa, 0xbf, 0xdb, 0x84, 0x01, 0xfc, 0x7d, 0x7a, 0xac, 0x20, 0xae, 0xf5,
  0x95, 0xc9, 0xdc, 0x10, 0x5f, 0x4c, 0xae, 0x85, 0x01, 0x8b, 0xfe, 0x77,
  0x13, 0x01, 0xae, 0x39, 0x59, 0x7e, 0xbc, 0xfd, 0xc9, 0x42, 0xe4, 0x13,
  0x07, 0x3f, 0xa9, 0x74, 0xd9, 0xd5, 0xfc, 0xb9, 0x78, 0xbe, 0x97, 0xf5,
  0xe7, 0x36, 0x7f, 0xfa, 0x23, 0x30, 0xeb, 0xab, 0x92, 0xd3, 0xdc, 0x3f,
  0x7f, 0xc0, 0x77, 0x93, 0xf9, 0x88, 0xe3, 0x4e, 0x13, 0x53, 0x6d, 0x71,
  0x87, 0xe9, 0x24, 0x2b, 0xae, 0x26, 0xbf, 0x62, 0x51, 0x04, 0x42, 0xe1,
  0x13, 0x9d, 0xd8, 0x9f, 0x59, 0x87, 0x3f, 0xfc, 0x94, 0xff, 0xcf, 0x88,
  0x88, 0xe6, 0xeb, 0x6e, 0xc1, 0x96, 0x04, 0x27, 0xc8, 0xda, 0xfa, 0xe8,
  0x2e, 0xbb, 0x2c, 0x6e, 0xf4, 0xb4, 0x00, 0x7d, 0x8d, 0x3b, 0xef, 0x8b,
  0x18, 0xa9, 0x5f, 0x32, 0xa9, 0xf2, 0x3a, 0x7e, 0x65, 0x2d, 0x6e, 0x8d,
  0x75, 0x77, 0xf6, 0xa6, 0xd8, 0xf9, 0x6b, 0x51, 0xe6, 0x66, 0x52, 0x59,
  0x39, 0x97, 0x22, 0xda, 0xb2, 0xd6, 0x82, 0x5a, 0x6e, 0x61, 0x60, 0x16,
  0x48, 0x7b, 0xf1, 0xc3, 0x4d, 0x7f, 0x50, 0xfa, 0x4d, 0x58, 0x27, 0x30,
  0xc8, 0x96, 0xe0, 0x41, 0x4f, 0x6b, 0xeb, 0x88, 0xa2, 0x7a, 0xef, 0x8a,
  0x88, 0xc8, 0x50, 0x4b, 0x55, 0x66, 0xee, 0xbf, 0xc4, 0x01, 0x82, 0x4c,
  0xec, 0xde, 0x37, 0x64, 0xd6, 0x1e, 0xcf, 0x3e, 0x2e, 0xfe, 0x84, 0x68,
  0xbf, 0xa3, 0x68, 0x77, 0xa9, 0x03, 0xe4, 0xf8, 0xd7, 0xb2, 0x6e, 0xa3,
  0xc4, 0xc3, 0x36, 0x53, 0xf3, 0xdd, 0x7e, 0x4c, 0xf0, 0xe9, 0xb2, 0x44,
  0xe6, 0x60, 0x3d, 0x00, 0x9a, 0x08, 0xc3, 0x21, 0x17, 0x49, 0xda, 0x49,
  0xfb, 0x4c, 0x8b, 0xe9, 0x10, 0x66, 0xfe, 0xb7, 0xe0, 0xf9, 0xdd, 0xbf,
  0x41, 0xfe, 0x04, 0x9b, 0x7f, 0xe8, 0xd6, 0x2e, 0x4d, 0x0f, 0x7b, 0x10,
  0x73, 0x4c, 0xa1, 0x3e, 0x43, 0xb7, 0xcf, 0x94, 0x97, 0x7e, 0x24, 0xbb,
  0x87, 0xbf, 0x22, 0xb8, 0x3e, 0xeb, 0x9a, 0x3f, 0xe3, 0x86, 0xee, 0x21,
  0xbc, 0xf5, 0x44, 0xeb, 0x60, 0x2e, 0xe7, 0x8f, 0x89, 0xa4, 0x91, 0x61,
  0x28, 0x90, 0x85, 0x68, 0xe0, 0xa9, 0x62, 0x93, 0x86, 0x5a, 0x15, 0xbe,
  0xb2, 0x76, 0x83, 0xf2, 0x0f, 0x00, 0xc7, 0xb6, 0x57, 0xe9, 0x1f, 0x92,
  0x49, 0xfe, 0x50, 0x85, 0xbf, 0x39, 0x3d, 0xe4, 0x8b, 0x72, 0x2d, 0x49,
  0xbe, 0x05, 0x0a, 0x34, 0x56, 0x80, 0xc6, 0x1f, 0x46, 0x59, 0xc9, 0xfe,
  0x40, 0xfb, 0x78, 0x6d, 0x7a, 0xe5, 0x30, 0xe9, 0x81, 0x55, 0x75, 0x05,
  0x63, 0xd2, 0x22, 0xee, 0x2e, 0x6e, 0xb9, 0x18, 0xe5, 0x8a, 0x5a, 0x66,
  0xbd, 0x74, 0x30, 0xe3, 0x8b, 0x76, 0x22, 0x18, 0x1e, 0xef, 0x69, 0xe8,
  0x9d, 0x07, 0xa7, 0x9a, 0x87, 0x6c, 0x04, 0x4b, 0x74, 0x2b, 0xbe, 0x37,
  0x2f, 0x29, 0x9b, 0x60, 0x9d, 0x8b, 0x57, 0x55, 0x34, 0xca, 0x41, 0x25,
  0xae, 0x56, 0x92, 0x34, 0x1b, 0x9e, 0xbd, 0xfe, 0x74, 0xbd, 0x4e, 0x29,
  0xf0, 0x5e, 0x27, 0x94, 0xb0, 0x9e, 0x23, 0x9f, 0x4a, 0x0f, 0xa1, 0xdf,
  0xe7, 0xc4, 0xdb, 0xbe, 0x0f, 0x1a, 0x0b, 0x6c, 0xb0, 0xe1, 0x06, 0x7c,
  0x5a, 0x5b, 0x81, 0x1c, 0xb6, 0x12, 0xec, 0x6f, 0x3b, 0xbb, 0x84, 0x36,
  0xd5, 0x28, 0x16, 0xea, 0x51, 0xa8, 0x99, 0x24, 0x8f, 0xe7, 0xf8, 0xe9,
  0xce, 0xa1, 0x65, 0x96, 0x6f, 0x4e, 0x2f, 0xb7, 0x6f, 0x65, 0x39, 0xad,
  0xfd, 0x2e, 0xa0, 0x37, 0x32, 0x2f, 0xf3, 0x95, 0xa1, 0x3a, 0xa1, 0x9d,
  0x2c, 0x9e, 0xa1, 0x4b, 0x7e, 0xc9, 0x7e, 0x86, 0xaa, 0x16, 0x00, 0x82,
  0x1d, 0x36, 0xbf, 0x98, 0x0a, 0x82, 0x5b, 0xcc, 0xc4, 0x6a, 0xad, 0xa0,
  0x1f, 0x47, 0x98, 0xde, 0x8d, 0x68, 0x38, 0x3f, 0x33, 0xe2, 0x08, 0x3b,
  0x2a, 0x65, 0xd9, 0x2f, 0x53, 0x68, 0xb8, 0x78, 0xd0, 0x1d, 0xbb, 0x2a,
  0x73, 0x19, 0xba, 0x58, 0xea, 0xf1, 0x0a, 0xaa, 0xa6, 0xbe, 0x27, 0xd6,
  0x00, 0x6b, 0x4e, 0x43, 0x8e, 0x5b, 0x19, 0xc1, 0x37, 0x0f, 0xfb, 0x81,
  0x72, 0x10, 0xb6, 0x20, 0x32, 0xcd, 0xa2, 0x7c, 0x90, 0xd4, 0xf5, 0xcf,
  0x1c, 0xcb, 0x14, 0x24, 0x7a, 0x4d, 0xf5, 0xd5, 0xd9, 0xce, 0x6a, 0x64,
  0xc9, 0xd3, 0xa7, 0x36, 0x6f, 0x1d, 0xf1, 0xe9, 0x71, 0x6c, 0x3d, 0x02,
  0xa4, 0x62, 0xb1, 0x82, 0x5c, 0x13, 0x4b, 0x6b, 0x68, 0xe2, 0x31, 0xef,
  0xe4, 0x46, 0xfd, 0xe5, 0xa8, 0x29, 0xe9, 0x1e, 0xad, 0xff, 0x33, 0xdb,
  0x0b, 0xc0, 0x92, 0xb1, 0xef, 0xeb, 0xb3, 0x6f, 0x96, 0x7b, 0xdf, 0xcd,
  0x07, 0x19, 0x86, 0x60, 0x98, 0xcf, 0x95, 0xfe, 0x98, 0xdd, 0x29, 0xa6,
  0x35, 0x7b, 0x46, 0x13, 0x03, 0xa8, 0xd9, 0x7c, 0xb3, 0xdf, 0x9f, 0x14,
  0xb7, 0x34, 0x5a, 0xc4, 0x12, 0x81, 0xc5, 0x98, 0x25, 0x8d, 0x3e, 0xe3,
  0xd8, 0x2d, 0xe4, 0x54, 0xab, 0xb0, 0x13, 0xfd, 0xd1, 0x3f, 0x3b, 0xbf,
  0xa9, 0x45, 0x28, 0x8a, 0x2f, 0x9c, 0x1e, 0x2d, 0xe5, 0xab, 0x13, 0x95,
  0x97, 0xc3, 0x34, 0x37, 0x8d, 0x93, 0x66, 0x31, 0x81, 0xa0, 0x30, 0x23,
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x15, 0x31,
  0x16, 0x04, 0x14, 0xa5, 0x23, 0x9b, 0x7e, 0xe6, 0x45, 0x71, 0xbf, 0x48,
  0xc6, 0x27, 0x3c, 0x96, 0x87, 0x63, 0xbd, 0x1f, 0xde, 0x72, 0x12, 0x30,
  0x79, 0x06, 0x09, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x11, 0x01,
  0x31, 0x6c, 0x1e, 0x6a, 0x00, 0x4d, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72,
  0x00, 0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00, 0x74, 0x00, 0x20,
  0x00, 0x45, 0x00, 0x6e, 0x00, 0x68, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x63,
  0x00, 0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x52, 0x00, 0x53, 0x00, 0x41,
  0x00, 0x20, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x64, 0x00, 0x20, 0x00, 0x41,
  0x00, 0x45, 0x00, 0x53, 0x00, 0x20, 0x00, 0x43, 0x00, 0x72, 0x00, 0x79,
  0x00, 0x70, 0x00, 0x74, 0x00, 0x6f, 0x00, 0x67, 0x00, 0x72, 0x00, 0x61,
  0x00, 0x70, 0x00, 0x68, 0x00, 0x69, 0x00, 0x63, 0x00, 0x20, 0x00, 0x50,
  0x00, 0x72, 0x00, 0x6f, 0x00, 0x76, 0x00, 0x69, 0x00, 0x64, 0x00, 0x65,
  0x00, 0x72, 0x30, 0x31, 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
  0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14, 0x93, 0xa8, 0xb2, 0x7e, 0xb7,
  0xab, 0xf1, 0x1c, 0x3c, 0x36, 0x58, 0xdc, 0x67, 0x6d, 0x42, 0xa6, 0xfc,
  0x53, 0x01, 0xe6, 0x04, 0x08, 0x77, 0x57, 0x22, 0xa1, 0x7d, 0xb9, 0xa2,
  0x69, 0x02, 0x02, 0x08, 0x00
};

static void test_PFXImportCertStore(void)
{
    HCERTSTORE store;
    CRYPT_DATA_BLOB pfx;
    const CERT_CONTEXT *cert;
    CERT_KEY_CONTEXT key;
    char buf[512];
    CRYPT_KEY_PROV_INFO *keyprov = (CRYPT_KEY_PROV_INFO *)buf;
    CERT_INFO *info;
    DWORD count, size;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    store = PFXImportCertStore( NULL, NULL, 0 );
    ok( store == NULL, "got %p\n", store );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError() );

    pfx.pbData = (BYTE *)pfxdata;
    pfx.cbData = sizeof(pfxdata);
    store = PFXImportCertStore( &pfx, NULL, CRYPT_EXPORTABLE|CRYPT_USER_KEYSET|PKCS12_NO_PERSIST_KEY );
    ok( store != NULL, "got %lu\n", GetLastError() );
    count = countCertsInStore( store );
    ok( count == 1, "got %lu\n", count );

    cert = CertFindCertificateInStore( store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );
    ok( cert != NULL, "got %lu\n", GetLastError() );
    ok( cert->dwCertEncodingType == X509_ASN_ENCODING, "got %lu\n", cert->dwCertEncodingType );
    ok( cert->pbCertEncoded != NULL, "pbCertEncoded not set\n" );
    ok( cert->cbCertEncoded == 1123, "got %lu\n", cert->cbCertEncoded );
    ok( cert->pCertInfo != NULL, "pCertInfo not set\n" );
    ok( cert->hCertStore == store, "got %p\n", cert->hCertStore );

    info = cert->pCertInfo;
    ok( info->dwVersion == CERT_V1, "got %lu\n", info->dwVersion );
    ok( !strcmp(info->SignatureAlgorithm.pszObjId, szOID_RSA_SHA256RSA),
        "got \"%s\"\n", info->SignatureAlgorithm.pszObjId );

    size = sizeof(key);
    ret = CertGetCertificateContextProperty( cert, CERT_KEY_CONTEXT_PROP_ID, &key, &size );
    ok( ret, "got %08lx\n", GetLastError() );
    ok( key.cbSize == sizeof(key), "got %lu\n", key.cbSize );
    ok( key.hCryptProv, "hCryptProv not set\n" );
    ok( key.dwKeySpec == AT_KEYEXCHANGE, "got %lu\n", key.dwKeySpec );

    size = sizeof(buf);
    SetLastError( 0xdeadbeef );
    ret = CertGetCertificateContextProperty( cert, CERT_KEY_PROV_INFO_PROP_ID, keyprov, &size );
    ok( !ret && GetLastError() == CRYPT_E_NOT_FOUND, "got %08lx\n", GetLastError() );
    CertFreeCertificateContext( cert );
    CertCloseStore( store, 0 );

    /* without PKCS12_NO_PERSIST_KEY */
    store = PFXImportCertStore( &pfx, NULL, CRYPT_EXPORTABLE|CRYPT_USER_KEYSET );
    ok( store != NULL, "got %lu\n", GetLastError() );

    cert = CertFindCertificateInStore( store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );
    ok( cert != NULL, "got %08lx\n", GetLastError() );

    size = sizeof(key);
    ret = CertGetCertificateContextProperty( cert, CERT_KEY_CONTEXT_PROP_ID, &key, &size );
    ok( !ret && GetLastError() == CRYPT_E_NOT_FOUND, "got %08lx\n", GetLastError() );

    size = sizeof(buf);
    ret = CertGetCertificateContextProperty( cert, CERT_KEY_PROV_INFO_PROP_ID, buf, &size );
    ok(ret, "got %lu\n", GetLastError());
    CertFreeCertificateContext( cert );
    CertCloseStore( store, 0 );

    /* CRYPT_MACHINE_KEYSET */
    store = PFXImportCertStore( &pfx, NULL, CRYPT_MACHINE_KEYSET );
    ok( store != NULL, "got %lu\n", GetLastError() );

    cert = CertFindCertificateInStore( store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );
    ok( cert != NULL, "got %08lx\n", GetLastError() );

    CertFreeCertificateContext( cert );
    CertCloseStore( store, 0 );
}

static void test_CryptQueryObject(void)
{
    CRYPT_DATA_BLOB pfx;
    DWORD encoding_type, content_type, format_type;
    HCERTSTORE store;
    HCRYPTMSG msg;
    const void *ctx;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = CryptQueryObject( CERT_QUERY_OBJECT_BLOB, NULL, CERT_QUERY_CONTENT_FLAG_ALL,
                            CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( !ret, "success\n" );
    ok( GetLastError() == E_INVALIDARG, "got %lu\n", GetLastError() );

    pfx.pbData = (BYTE *)pfxdata;
    pfx.cbData = sizeof(pfxdata);
    encoding_type = content_type = format_type = 0xdeadbeef;
    store = (HCERTSTORE *)0xdeadbeef;
    msg = (HCRYPTMSG *)0xdeadbeef;
    ctx = (void *)0xdeadbeef;
    ret = CryptQueryObject( CERT_QUERY_OBJECT_BLOB, &pfx, CERT_QUERY_CONTENT_FLAG_ALL,
                            CERT_QUERY_FORMAT_FLAG_BINARY, 0, &encoding_type, &content_type, &format_type,
                            &store, &msg, &ctx );
    ok( ret, "got %lu\n", GetLastError() );
    ok( encoding_type == X509_ASN_ENCODING, "got %08lx\n", encoding_type );
    ok( content_type == CERT_QUERY_CONTENT_PFX, "got %08lx\n", content_type );
    ok( format_type == CERT_QUERY_FORMAT_BINARY, "got %08lx\n", format_type );
    ok( store == NULL, "got %p\n", store );
    ok( msg == NULL, "got %p\n", msg );
    ok( ctx == NULL, "got %p\n", ctx );
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
#ifdef __REACTOS__
    if ((GetVersion() & 0xFF) > 5) // test_PFXImportCertStore() crashes on Server 2003
#endif
    test_PFXImportCertStore();
    test_CryptQueryObject();
}
