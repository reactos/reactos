/*
 * Unit test suite for crypt32.dll's Cert*ToStr and CertStrToName functions.
 *
 * Copyright 2006 Juan Lang, Aric Stewart for CodeWeavers
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
#include <wincrypt.h>
#include <winnls.h>

#include "wine/test.h"

typedef struct _CertRDNAttrEncoding {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCSTR str;
} CertRDNAttrEncoding, *PCertRDNAttrEncoding;

typedef struct _CertRDNAttrEncodingW {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCWSTR str;
} CertRDNAttrEncodingW, *PCertRDNAttrEncodingW;

static BYTE bin1[] = { 0x55, 0x53 };
static BYTE bin2[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f, 0x74,
 0x61 };
static BYTE bin3[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70, 0x6f,
 0x6c, 0x69, 0x73 };
static BYTE bin4[] = { 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61, 0x76,
 0x65, 0x72, 0x73 };
static BYTE bin5[] = { 0x57, 0x69, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x76,
 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74 };
static BYTE bin6[] = { 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
 0x74 };
static BYTE bin7[] = { 0x61, 0x72, 0x69, 0x63, 0x40, 0x63, 0x6f, 0x64,
 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d };
static BYTE bin8[] = {
0x65,0x00,0x50,0x00,0x4b,0x00,0x49,0x00,0x20,0x00,0x52,0x00,0x6f,0x00,0x6f,
0x00,0x74,0x00,0x20,0x00,0x43,0x00,0x65,0x00,0x72,0x00,0x74,0x00,0x69,0x00,
0x66,0x00,0x69,0x00,0x63,0x00,0x61,0x00,0x74,0x00,0x69,0x00,0x6f,0x00,0x6e,
0x00,0x20,0x00,0x41,0x00,0x75,0x00,0x74,0x00,0x68,0x00,0x6f,0x00,0x72,0x00,
0x69,0x00,0x74,0x00,0x79,0x00 };
static BYTE bin9[] = { 0x61, 0x62, 0x63, 0x22, 0x64, 0x65, 0x66 };
static BYTE bin10[] = { 0x61, 0x62, 0x63, 0x27, 0x64, 0x65, 0x66 };
static BYTE bin11[] = { 0x61, 0x62, 0x63, 0x2c, 0x20, 0x64, 0x65, 0x66 };
static BYTE bin12[] = { 0x20, 0x61, 0x62, 0x63, 0x20 };
static BYTE bin13[] = { 0x22, 0x64, 0x65, 0x66, 0x22 };
static BYTE bin14[] = { 0x31, 0x3b, 0x33 };

/*
Certificate:
    Data:
        Version: 1 (0x0)
        Serial Number:
            e3:5a:10:f1:fc:4b:f3:a2
        Signature Algorithm: md5WithRSAEncryption
        Issuer: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, OU = Wine Development, CN = localhost, emailAddress = aric@codeweavers.com
        Validity
            Not Before: Jan 25 13:57:24 2006 GMT
            Not After : Feb 24 13:57:24 2006 GMT
        Subject: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, OU = Wine Development, CN = localhost, emailAddress = aric@codeweavers.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (1024 bit)
                Modulus:
...
                Exponent: 65537 (0x10001)
    Signature Algorithm: md5WithRSAEncryption
...
*/
static const BYTE cert[] = 
{0x30,0x82,0x2,0xbb,0x30,0x82,0x2,0x24,0x2,0x9,0x0,0xe3,0x5a,0x10,0xf1,0xfc,
 0x4b,0xf3,0xa2,0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x4,0x5,
 0x0,0x30,0x81,0xa1,0x31,0xb,0x30,0x9,0x6,0x3,0x55,0x4,0x6,0x13,0x2,0x55,0x53,
 0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x8,0x13,0x9,0x4d,0x69,0x6e,0x6e,0x65,
 0x73,0x6f,0x74,0x61,0x31,0x14,0x30,0x12,0x6,0x3,0x55,0x4,0x7,0x13,0xb,0x4d,
 0x69,0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,0x73,0x31,0x14,0x30,0x12,0x6,0x3,
 0x55,0x4,0xa,0x13,0xb,0x43,0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,
 0x31,0x19,0x30,0x17,0x6,0x3,0x55,0x4,0xb,0x13,0x10,0x57,0x69,0x6e,0x65,0x20,
 0x44,0x65,0x76,0x65,0x6c,0x6f,0x70,0x6d,0x65,0x6e,0x74,0x31,0x12,0x30,0x10,
 0x6,0x3,0x55,0x4,0x3,0x13,0x9,0x6c,0x6f,0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,
 0x31,0x23,0x30,0x21,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x9,0x1,0x16,
 0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,0x65,0x77,0x65,0x61,0x76,0x65,
 0x72,0x73,0x2e,0x63,0x6f,0x6d,0x30,0x1e,0x17,0xd,0x30,0x36,0x30,0x31,0x32,
 0x35,0x31,0x33,0x35,0x37,0x32,0x34,0x5a,0x17,0xd,0x30,0x36,0x30,0x32,0x32,
 0x34,0x31,0x33,0x35,0x37,0x32,0x34,0x5a,0x30,0x81,0xa1,0x31,0xb,0x30,0x9,0x6,
 0x3,0x55,0x4,0x6,0x13,0x2,0x55,0x53,0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x8,
 0x13,0x9,0x4d,0x69,0x6e,0x6e,0x65,0x73,0x6f,0x74,0x61,0x31,0x14,0x30,0x12,0x6,
 0x3,0x55,0x4,0x7,0x13,0xb,0x4d,0x69,0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,
 0x73,0x31,0x14,0x30,0x12,0x6,0x3,0x55,0x4,0xa,0x13,0xb,0x43,0x6f,0x64,0x65,
 0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x31,0x19,0x30,0x17,0x6,0x3,0x55,0x4,0xb,
 0x13,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,0x6c,0x6f,0x70,0x6d,
 0x65,0x6e,0x74,0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x3,0x13,0x9,0x6c,0x6f,
 0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,0x31,0x23,0x30,0x21,0x6,0x9,0x2a,0x86,0x48,
 0x86,0xf7,0xd,0x1,0x9,0x1,0x16,0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,
 0x65,0x77,0x65,0x61,0x76,0x65,0x72,0x73,0x2e,0x63,0x6f,0x6d,0x30,0x81,0x9f,
 0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x1,0x5,0x0,0x3,0x81,
 0x8d,0x0,0x30,0x81,0x89,0x2,0x81,0x81,0x0,0x9b,0xb5,0x8f,0xaf,0xfb,0x9a,0xaf,
 0xdc,0xa2,0x4d,0xb1,0xc8,0x72,0x44,0xef,0x79,0x7f,0x28,0xb6,0xfe,0x50,0xdc,
 0x8a,0xf7,0x11,0x2f,0x90,0x70,0xed,0xa4,0xa9,0xd,0xbf,0x82,0x3e,0x56,0xd8,
 0x36,0xb6,0x9,0x52,0x83,0xab,0x65,0x95,0x0,0xe2,0xea,0x3c,0x4f,0x85,0xb8,0xc,
 0x41,0x42,0x77,0x5c,0x9d,0x44,0xeb,0xcf,0x7d,0x60,0x64,0x7a,0x6c,0x4c,0xac,
 0x4a,0x9a,0x23,0x25,0x15,0xd7,0x92,0xb4,0x10,0xe7,0x95,0xad,0x4b,0x93,0xda,
 0x6a,0x76,0xe0,0xa5,0xd2,0x13,0x8,0x12,0x30,0x68,0xde,0xb9,0x5b,0x6e,0x2a,
 0x97,0x43,0xaa,0x7b,0x22,0x33,0x34,0xb1,0xca,0x5d,0x19,0xd8,0x42,0x26,0x45,
 0xc6,0xe9,0x1d,0xee,0x7,0xc2,0x27,0x95,0x87,0xd8,0x12,0xec,0x4b,0x16,0x9f,0x2,
 0x3,0x1,0x0,0x1,0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x4,0x5,
 0x0,0x3,0x81,0x81,0x0,0x96,0xf9,0xf6,0x6a,0x3d,0xd9,0xca,0x6e,0xd5,0x76,0x73,
 0xab,0x75,0xc1,0xcc,0x98,0x44,0xc3,0xa9,0x90,0x68,0x88,0x76,0xb9,0xeb,0xb6,
 0xbe,0x60,0x62,0xb9,0x67,0x1e,0xcc,0xf4,0xe1,0xe7,0x6c,0xc8,0x67,0x3f,0x1d,
 0xf3,0x68,0x86,0x30,0xee,0xaa,0x92,0x61,0x37,0xd7,0x82,0x90,0x28,0xaa,0x7a,
 0x18,0x88,0x60,0x14,0x88,0x75,0xc0,0x4a,0x4e,0x7d,0x48,0xe7,0x3,0xa6,0xfd,
 0xd7,0xce,0x3c,0xe5,0x9b,0xaf,0x2f,0xdc,0xbb,0x7c,0xbd,0x20,0x49,0xd9,0x68,
 0x37,0xeb,0x5d,0xbb,0xe2,0x6d,0x66,0xe3,0x11,0xc1,0xa7,0x88,0x49,0xc6,0x6f,
 0x65,0xd3,0xce,0xae,0x26,0x19,0x3,0x2e,0x4f,0x78,0xa5,0xa,0x97,0x7e,0x4f,0xc4,
 0x91,0x8a,0xf8,0x5,0xef,0x5b,0x3b,0x49,0xbf,0x5f,0x2b};

/*
Certificate:
    Data:
        Version: 1 (0x0)
        Serial Number:
            59:9e:db:44:80:da:6c:92:f9:38:be:d8:fe:7a:20:77:57:c7:71:5b
        Signature Algorithm: md5WithRSAEncryption
        Issuer: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, OU = Wine Development, CN = localhost
        Validity
            Not Before: Mar 17 22:20:44 2023 GMT
            Not After : Apr 16 22:20:44 2023 GMT
        Subject: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, OU = Wine Development, CN = localhost
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                Public-Key: (1024 bit)
                Modulus:
...
                Exponent: 65537 (0x10001)
    Signature Algorithm: md5WithRSAEncryption
...
*/
static const BYTE cert_no_email[] = {
    0x30,0x82,0x02,0x7a,0x30,0x82,0x01,0xe3,0x02,0x14,0x59,0x9e,0xdb,0x44,0x80,0xda,
    0x6c,0x92,0xf9,0x38,0xbe,0xd8,0xfe,0x7a,0x20,0x77,0x57,0xc7,0x71,0x5b,0x30,0x0d,
    0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x04,0x05,0x00,0x30,0x7c,0x31,
    0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x12,0x30,0x10,
    0x06,0x03,0x55,0x04,0x08,0x0c,0x09,0x4d,0x69,0x6e,0x6e,0x65,0x73,0x6f,0x74,0x61,
    0x31,0x14,0x30,0x12,0x06,0x03,0x55,0x04,0x07,0x0c,0x0b,0x4d,0x69,0x6e,0x6e,0x65,
    0x61,0x70,0x6f,0x6c,0x69,0x73,0x31,0x14,0x30,0x12,0x06,0x03,0x55,0x04,0x0a,0x0c,
    0x0b,0x43,0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x31,0x19,0x30,0x17,
    0x06,0x03,0x55,0x04,0x0b,0x0c,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,
    0x6c,0x6f,0x70,0x6d,0x65,0x6e,0x74,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,
    0x0c,0x09,0x6c,0x6f,0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,0x30,0x1e,0x17,0x0d,0x32,
    0x33,0x30,0x33,0x31,0x37,0x32,0x32,0x32,0x30,0x34,0x34,0x5a,0x17,0x0d,0x32,0x33,
    0x30,0x34,0x31,0x36,0x32,0x32,0x32,0x30,0x34,0x34,0x5a,0x30,0x7c,0x31,0x0b,0x30,
    0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,0x53,0x31,0x12,0x30,0x10,0x06,0x03,
    0x55,0x04,0x08,0x0c,0x09,0x4d,0x69,0x6e,0x6e,0x65,0x73,0x6f,0x74,0x61,0x31,0x14,
    0x30,0x12,0x06,0x03,0x55,0x04,0x07,0x0c,0x0b,0x4d,0x69,0x6e,0x6e,0x65,0x61,0x70,
    0x6f,0x6c,0x69,0x73,0x31,0x14,0x30,0x12,0x06,0x03,0x55,0x04,0x0a,0x0c,0x0b,0x43,
    0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x31,0x19,0x30,0x17,0x06,0x03,
    0x55,0x04,0x0b,0x0c,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,0x6c,0x6f,
    0x70,0x6d,0x65,0x6e,0x74,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x0c,0x09,
    0x6c,0x6f,0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,0x30,0x81,0x9f,0x30,0x0d,0x06,0x09,
    0x2a,0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x01,0x05,0x00,0x03,0x81,0x8d,0x00,0x30,
    0x81,0x89,0x02,0x81,0x81,0x00,0xc4,0xdd,0x00,0xa7,0xdb,0xec,0x95,0x68,0xee,0xf0,
    0x3f,0xed,0xb2,0xcb,0x6f,0xf4,0x34,0x2f,0xbe,0x13,0xa9,0x24,0x95,0xf3,0xca,0x3c,
    0x2b,0xd3,0x41,0x7c,0x32,0xe7,0x95,0x4e,0xdd,0xef,0xcc,0x45,0x0d,0xf2,0x71,0x42,
    0x12,0x78,0xb1,0x17,0x88,0xf4,0x12,0xba,0x92,0x2d,0x5c,0xfc,0x2c,0x8a,0x53,0xbf,
    0xee,0x23,0x3f,0x7b,0x11,0x46,0x5e,0x1d,0xb8,0xff,0xa3,0x70,0x5c,0x5f,0x6b,0xa8,
    0x3c,0x47,0x75,0xa5,0x3a,0x80,0x61,0x15,0x10,0x56,0x57,0x1f,0x82,0x6a,0xb2,0xb8,
    0xdc,0x3a,0xe0,0x1c,0x9c,0x83,0xd9,0x11,0x26,0xa6,0xb6,0x85,0x0a,0x27,0x45,0xb7,
    0xff,0xfa,0x26,0xbd,0x11,0x29,0x23,0x59,0xaa,0x19,0x77,0x3f,0x86,0x32,0x9f,0x48,
    0x43,0x4f,0xd0,0x03,0x7a,0x09,0x02,0x03,0x01,0x00,0x01,0x30,0x0d,0x06,0x09,0x2a,
    0x86,0x48,0x86,0xf7,0x0d,0x01,0x01,0x04,0x05,0x00,0x03,0x81,0x81,0x00,0xa3,0xf0,
    0x23,0xfc,0x80,0x05,0xac,0x76,0x26,0xbb,0xfc,0x79,0x03,0x10,0xa0,0xfb,0x7a,0x3e,
    0xf9,0xa7,0xdd,0xb1,0x9e,0x7c,0x22,0x83,0xa6,0xee,0x77,0x88,0xa2,0x74,0x64,0x35,
    0x4f,0x66,0x82,0x88,0x4a,0x83,0xc9,0xda,0x7e,0xc4,0xa0,0xd1,0xfb,0xe1,0x3e,0x22,
    0x1e,0xa8,0xdc,0x1b,0xd4,0xda,0x64,0x63,0xfc,0x1b,0x61,0x4f,0x52,0x1b,0xab,0x61,
    0x05,0xcd,0xb8,0x2d,0xb0,0x73,0xa7,0x5d,0x78,0xff,0x3f,0x4d,0x12,0x3a,0x38,0x69,
    0xc4,0x9f,0x77,0x35,0xce,0xe2,0xf9,0xd6,0x23,0x47,0xc2,0x15,0xff,0xbf,0x3e,0x65,
    0xf3,0xc0,0x0a,0x58,0x76,0x10,0x8e,0xd5,0xa9,0x30,0x3e,0x25,0x4b,0x6d,0xb7,0xb2,
    0x64,0x96,0x0e,0x27,0x88,0x55,0xfc,0xaa,0x18,0x65,0x2a,0xe9,0xf4,0x23
};

/*
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            5d:79:35:fd:d3:8f:6b:e2:28:3e:94:f4:14:bf:d4:b5:c2:3a:ac:38
        Signature Algorithm: md5WithRSAEncryption
        Issuer: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, CN = server_cn.org, emailAddress = test@codeweavers.com
        Validity
            Not Before: Apr 14 18:56:22 2022 GMT
            Not After : Apr 11 18:56:22 2032 GMT
        Subject: C = US, ST = Minnesota, L = Minneapolis, O = CodeWeavers, CN = server_cn.org, emailAddress = test@codeweavers.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (1024 bit)
                Modulus:
...
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Subject Alternative Name:
                DNS:ex1.org, DNS:*.ex2.org
            X509v3 Issuer Alternative Name:
                DNS:ex3.org, DNS:*.ex4.org
    Signature Algorithm: md5WithRSAEncryption
...
*/
static BYTE cert_v3[] = {
    0x30, 0x82, 0x02, 0xdf, 0x30, 0x82, 0x02, 0x48, 0xa0, 0x03, 0x02, 0x01,
    0x02, 0x02, 0x14, 0x5d, 0x79, 0x35, 0xfd, 0xd3, 0x8f, 0x6b, 0xe2, 0x28,
    0x3e, 0x94, 0xf4, 0x14, 0xbf, 0xd4, 0xb5, 0xc2, 0x3a, 0xac, 0x38, 0x30,
    0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04,
    0x05, 0x00, 0x30, 0x81, 0x8a, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
    0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03,
    0x55, 0x04, 0x08, 0x0c, 0x09, 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f,
    0x74, 0x61, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c,
    0x0b, 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70, 0x6f, 0x6c, 0x69, 0x73,
    0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x0b, 0x43,
    0x6f, 0x64, 0x65, 0x57, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x31, 0x16,
    0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0d, 0x73, 0x65, 0x72,
    0x76, 0x65, 0x72, 0x5f, 0x63, 0x6e, 0x2e, 0x6f, 0x72, 0x67, 0x31, 0x23,
    0x30, 0x21, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09,
    0x01, 0x16, 0x14, 0x74, 0x65, 0x73, 0x74, 0x40, 0x63, 0x6f, 0x64, 0x65,
    0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x30,
    0x1e, 0x17, 0x0d, 0x32, 0x32, 0x30, 0x34, 0x31, 0x34, 0x31, 0x38, 0x35,
    0x36, 0x32, 0x32, 0x5a, 0x17, 0x0d, 0x33, 0x32, 0x30, 0x34, 0x31, 0x31,
    0x31, 0x38, 0x35, 0x36, 0x32, 0x32, 0x5a, 0x30, 0x81, 0x8a, 0x31, 0x0b,
    0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31,
    0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x09, 0x4d, 0x69,
    0x6e, 0x6e, 0x65, 0x73, 0x6f, 0x74, 0x61, 0x31, 0x14, 0x30, 0x12, 0x06,
    0x03, 0x55, 0x04, 0x07, 0x0c, 0x0b, 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61,
    0x70, 0x6f, 0x6c, 0x69, 0x73, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
    0x04, 0x0a, 0x0c, 0x0b, 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61, 0x76,
    0x65, 0x72, 0x73, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x03,
    0x0c, 0x0d, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x5f, 0x63, 0x6e, 0x2e,
    0x6f, 0x72, 0x67, 0x31, 0x23, 0x30, 0x21, 0x06, 0x09, 0x2a, 0x86, 0x48,
    0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x14, 0x74, 0x65, 0x73, 0x74,
    0x40, 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73,
    0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x81, 0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x81,
    0x8d, 0x00, 0x30, 0x81, 0x89, 0x02, 0x81, 0x81, 0x00, 0xcd, 0x7c, 0x05,
    0xba, 0xad, 0xd0, 0xb0, 0x43, 0xcc, 0x47, 0x7d, 0x87, 0xaa, 0xb5, 0x89,
    0x9f, 0x43, 0x94, 0xa0, 0x84, 0xc0, 0xc0, 0x5e, 0x05, 0x6d, 0x2f, 0x05,
    0x21, 0x6b, 0x20, 0x39, 0x88, 0x06, 0x4e, 0xce, 0x76, 0xa7, 0x24, 0x77,
    0x13, 0x71, 0x9b, 0x2a, 0x53, 0x04, 0x4f, 0x0f, 0xfc, 0x3f, 0x4f, 0xb1,
    0x4e, 0xdc, 0xed, 0x96, 0xd4, 0x55, 0xbd, 0xcf, 0x25, 0xa6, 0x7c, 0xe3,
    0x35, 0xbf, 0xeb, 0x30, 0xec, 0xef, 0x7f, 0x8e, 0xa1, 0xc6, 0xd3, 0xb2,
    0x03, 0x62, 0x0a, 0x92, 0x87, 0x17, 0x52, 0x2d, 0x45, 0x2a, 0xdc, 0xdb,
    0x87, 0xa5, 0x32, 0x4a, 0x78, 0x28, 0x4a, 0x51, 0xff, 0xdb, 0xd5, 0x20,
    0x47, 0x7e, 0xc5, 0xbe, 0x1d, 0x01, 0x55, 0x13, 0x9f, 0xfb, 0x8e, 0x39,
    0xd9, 0x1b, 0xe0, 0x34, 0x93, 0x43, 0x9c, 0x02, 0xa3, 0x0f, 0xb5, 0xdc,
    0x9d, 0x86, 0x45, 0xc5, 0x4d, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x40,
    0x30, 0x3e, 0x30, 0x1d, 0x06, 0x03,
    0x55, 0x1d, 0x11, /* Subject Alternative Name OID */
    0x04, 0x16, 0x30, 0x14, 0x82, 0x07, 0x65, 0x78, 0x31, 0x2e, 0x6f, 0x72,
    0x67, 0x82, 0x09, 0x2a, 0x2e, 0x65, 0x78, 0x32, 0x2e, 0x6f, 0x72, 0x67,
    0x30, 0x1d, 0x06, 0x03,
    0x55, 0x1d, 0x12, /* Issuer Alternative Name OID */
    0x04, 0x16, 0x30, 0x14, 0x82, 0x07, 0x65, 0x78, 0x33, 0x2e, 0x6f, 0x72,
    0x67, 0x82, 0x09, 0x2a, 0x2e, 0x65, 0x78, 0x34, 0x2e, 0x6f, 0x72, 0x67,
    0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
    0x04, 0x05, 0x00, 0x03, 0x81, 0x81, 0x00, 0xcc, 0xa3, 0x75, 0x67, 0x61,
    0x63, 0x1d, 0x99, 0x16, 0xc6, 0x93, 0x35, 0xa4, 0x31, 0xb6, 0x05, 0x05,
    0x77, 0x12, 0x15, 0x16, 0x78, 0xb3, 0xba, 0x6e, 0xde, 0xfc, 0x73, 0x7c,
    0x5c, 0xdd, 0xdf, 0x92, 0xde, 0xa0, 0x86, 0xff, 0x77, 0x60, 0x99, 0x8f,
    0x4a, 0x40, 0xa8, 0x6a, 0xdb, 0x6f, 0x30, 0xe5, 0xce, 0x82, 0x2f, 0xf7,
    0x09, 0x17, 0xb2, 0xd3, 0x3a, 0x29, 0x9a, 0xd0, 0x73, 0x9c, 0x44, 0xa2,
    0x19, 0xf3, 0x1d, 0x16, 0x1a, 0x45, 0x2c, 0x4b, 0x94, 0xf1, 0xb8, 0xb6,
    0xc9, 0x82, 0x6c, 0x1f, 0xae, 0xbc, 0xd1, 0xbe, 0x78, 0xc9, 0x23, 0xf5,
    0x51, 0x6c, 0x90, 0xbf, 0xa3, 0x5c, 0xa1, 0x3a, 0xd8, 0xe3, 0xcf, 0x82,
    0x31, 0x78, 0x2b, 0xda, 0x99, 0xff, 0x23, 0x5b, 0xea, 0x59, 0xe0, 0x6d,
    0xd1, 0x30, 0xfd, 0x96, 0x6a, 0x4d, 0x36, 0x72, 0x96, 0xd7, 0x4f, 0x01,
    0xa9, 0x4d, 0x8f
};

#define CERT_V3_SAN_OID_OFFSET 534
#define CERT_V3_IAN_OID_OFFSET 565

static char issuerStr[] =
 "US, Minnesota, Minneapolis, CodeWeavers, Wine Development, localhost, aric@codeweavers.com";
static char issuerStrSemicolon[] =
 "US; Minnesota; Minneapolis; CodeWeavers; Wine Development; localhost; aric@codeweavers.com";
static char issuerStrCRLF[] =
 "US\r\nMinnesota\r\nMinneapolis\r\nCodeWeavers\r\nWine Development\r\nlocalhost\r\naric@codeweavers.com";
static char subjectStr[] =
 "2.5.4.6=US, 2.5.4.8=Minnesota, 2.5.4.7=Minneapolis, 2.5.4.10=CodeWeavers, 2.5.4.11=Wine Development, 2.5.4.3=localhost, 1.2.840.113549.1.9.1=aric@codeweavers.com";
static char subjectStrSemicolon[] =
 "2.5.4.6=US; 2.5.4.8=Minnesota; 2.5.4.7=Minneapolis; 2.5.4.10=CodeWeavers; 2.5.4.11=Wine Development; 2.5.4.3=localhost; 1.2.840.113549.1.9.1=aric@codeweavers.com";
static char subjectStrCRLF[] =
 "2.5.4.6=US\r\n2.5.4.8=Minnesota\r\n2.5.4.7=Minneapolis\r\n2.5.4.10=CodeWeavers\r\n2.5.4.11=Wine Development\r\n2.5.4.3=localhost\r\n1.2.840.113549.1.9.1=aric@codeweavers.com";
static char x500SubjectStr[] = "C=US, S=Minnesota, L=Minneapolis, O=CodeWeavers, OU=Wine Development, CN=localhost, E=aric@codeweavers.com";
static char x500SubjectStrSemicolonReverse[] = "E=aric@codeweavers.com; CN=localhost; OU=Wine Development; O=CodeWeavers; L=Minneapolis; S=Minnesota; C=US";

static void test_CertRDNValueToStrA(void)
{
    CertRDNAttrEncoding attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), bin1 }, "US" },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), bin2 }, "Minnesota" },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), bin3 }, "Minneapolis" },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), bin4 }, "CodeWeavers" },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), bin5 }, "Wine Development" },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), bin6 }, "localhost" },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), bin7 }, "aric@codeweavers.com" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin9), bin9 }, "abc\"def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin10), bin10 }, "abc'def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin11), bin11 }, "abc, def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin12), bin12 }, " abc " },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin13), bin13 }, "\"def\"" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin14), bin14 }, "1;3" },
    };
    unsigned int i;
    DWORD ret, len;
    char buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };
    static const char ePKI[] = "ePKI Root Certification Authority";

    /* This crashes
    ret = CertRDNValueToStrA(0, NULL, NULL, 0);
     */
    /* With empty input, it generates the empty string */
    SetLastError(0xdeadbeef);
    ret = CertRDNValueToStrA(0, &blob, NULL, 0);
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ret = CertRDNValueToStrA(0, &blob, buffer, sizeof(buffer));
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ok(!buffer[0], "Expected empty string\n");

    for (i = 0; i < ARRAY_SIZE(attrs); i++)
    {
        len = CertRDNValueToStrA(attrs[i].dwValueType, &attrs[i].Value,
         buffer, sizeof(buffer));
        ok(len == strlen(attrs[i].str) + 1, "Expected length %d, got %ld\n",
         lstrlenA(attrs[i].str) + 1, ret);
        ok(!strcmp(buffer, attrs[i].str), "Expected %s, got %s\n",
         attrs[i].str, buffer);
        memset(buffer, 0xcc, sizeof(buffer));
        ret = CertRDNValueToStrA(attrs[i].dwValueType, &attrs[i].Value, buffer, len - 1);
        ok(ret == 1, "Unexpected ret %lu, expected 1, test %u.\n", ret, i);
        ok(!buffer[0], "Unexpected value %#x, test %u.\n", buffer[0], i);
        ok(!strncmp(buffer + 1, attrs[i].str + 1, len - 2), "Strings do not match, test %u.\n", i);
        memset(buffer, 0xcc, sizeof(buffer));
        ret = CertRDNValueToStrA(attrs[i].dwValueType, &attrs[i].Value, buffer, 0);
        ok(ret == len, "Unexpected ret %lu, expected %lu, test %u.\n", ret, len, i);
        ok((unsigned char)buffer[0] == 0xcc, "Unexpected value %#x, test %u.\n", buffer[0], i);
    }
    blob.pbData = bin8;
    blob.cbData = sizeof(bin8);
    ret = CertRDNValueToStrA(CERT_RDN_UTF8_STRING, &blob, buffer,
     sizeof(buffer));
    ok(ret == strlen(ePKI) + 1, "Expected length %d, got %ld\n", lstrlenA(ePKI), ret);
    if (ret == strlen(ePKI) + 1)
        ok(!strcmp(buffer, ePKI), "Expected %s, got %s\n", ePKI, buffer);
}

static void test_CertRDNValueToStrW(void)
{
    static const WCHAR ePKIW[] = L"ePKI Root Certification Authority";
    CertRDNAttrEncodingW attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), bin1 }, L"US" },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), bin2 }, L"Minnesota" },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), bin3 }, L"Minneapolis" },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), bin4 }, L"CodeWeavers" },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), bin5 }, L"Wine Development" },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), bin6 }, L"localhost" },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), bin7 }, L"aric@codeweavers.com" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin9), bin9 }, L"abc\"def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin10), bin10 }, L"abc'def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin11), bin11 }, L"abc, def" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin12), bin12 }, L" abc " },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin13), bin13 }, L"\"def\"" },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin14), bin14 }, L"1;3" },
    };
    unsigned int i;
    DWORD ret, len;
    WCHAR buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };

    /* This crashes
    ret = CertRDNValueToStrW(0, NULL, NULL, 0);
     */
    /* With empty input, it generates the empty string */
    SetLastError(0xdeadbeef);
    ret = CertRDNValueToStrW(0, &blob, NULL, 0);
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ret = CertRDNValueToStrW(0, &blob, buffer, ARRAY_SIZE(buffer));
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ok(!buffer[0], "Expected empty string\n");

    for (i = 0; i < ARRAY_SIZE(attrs); i++)
    {
        len = CertRDNValueToStrW(attrs[i].dwValueType, &attrs[i].Value, buffer, ARRAY_SIZE(buffer));
        ok(len == lstrlenW(attrs[i].str) + 1,
         "Expected length %d, got %ld\n", lstrlenW(attrs[i].str) + 1, ret);
        ok(!lstrcmpW(buffer, attrs[i].str), "Expected %s, got %s\n",
         wine_dbgstr_w(attrs[i].str), wine_dbgstr_w(buffer));
        memset(buffer, 0xcc, sizeof(buffer));
        ret = CertRDNValueToStrW(attrs[i].dwValueType, &attrs[i].Value, buffer, len - 1);
        ok(ret == 1, "Unexpected ret %lu, expected 1, test %u.\n", ret, i);
        ok(!buffer[0], "Unexpected value %#x, test %u.\n", buffer[0], i);
        ok(buffer[1] == 0xcccc, "Unexpected value %#x, test %u.\n", buffer[1], i);
        memset(buffer, 0xcc, sizeof(buffer));
        ret = CertRDNValueToStrW(attrs[i].dwValueType, &attrs[i].Value, buffer, 0);
        ok(ret == len, "Unexpected ret %lu, expected %lu, test %u.\n", ret, len, i);
        ok(buffer[0] == 0xcccc, "Unexpected value %#x, test %u.\n", buffer[0], i);
    }
    blob.pbData = bin8;
    blob.cbData = sizeof(bin8);
    ret = CertRDNValueToStrW(CERT_RDN_UTF8_STRING, &blob, buffer,
     sizeof(buffer));
    ok(ret == lstrlenW(ePKIW) + 1, "Expected length %d, got %ld\n", lstrlenW(ePKIW), ret);
    if (ret == lstrlenW(ePKIW) + 1)
        ok(!lstrcmpW(buffer, ePKIW), "Expected %s, got %s\n",
         wine_dbgstr_w(ePKIW), wine_dbgstr_w(buffer));
}

#define test_NameToStrConversionA(a, b, c) test_NameToStrConversionA_(__LINE__, a, b, c)
static void test_NameToStrConversionA_(unsigned int line, PCERT_NAME_BLOB pName, DWORD dwStrType, LPCSTR expected)
{
    char buffer[2000];
    DWORD len, retlen;

    len = CertNameToStrA(X509_ASN_ENCODING, pName, dwStrType, NULL, 0);
    ok(len == strlen(expected) + 1, "line %u: Expected %d chars, got %ld.\n", line, lstrlenA(expected) + 1, len);
    len = CertNameToStrA(X509_ASN_ENCODING,pName, dwStrType, buffer, sizeof(buffer));
    ok(len == strlen(expected) + 1, "line %u: Expected %d chars, got %ld.\n",  line, lstrlenA(expected) + 1, len);
    ok(!strcmp(buffer, expected), "line %u: Expected %s, got %s.\n", line, expected, buffer);

    memset(buffer, 0xcc, sizeof(buffer));
    retlen = CertNameToStrA(X509_ASN_ENCODING, pName, dwStrType, buffer, len - 1);
    ok(retlen == 1, "line %u: expected 1, got %lu\n", line, retlen);
    ok(!buffer[0], "line %u: string is not zero terminated.\n", line);

    memset(buffer, 0xcc, sizeof(buffer));
    retlen = CertNameToStrA(X509_ASN_ENCODING, pName, dwStrType, buffer, 0);
    ok(retlen == len, "line %u: expected %lu chars, got %lu\n", line, len - 1, retlen);
    ok((unsigned char)buffer[0] == 0xcc, "line %u: got %s\n", line, wine_dbgstr_a(buffer));
}

static BYTE encodedSimpleCN[] = {
0x30,0x0c,0x31,0x0a,0x30,0x08,0x06,0x03,0x55,0x04,0x03,0x13,0x01,0x31 };
static BYTE encodedSingleQuotedCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,
 0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x27,0x31,0x27 };
static BYTE encodedSpacedCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,
 0x55,0x04,0x03,0x13,0x03,0x20,0x31,0x20 };
static BYTE encodedQuotedCN[] = { 0x30,0x11,0x31,0x0f,0x30,0x0d,0x06,0x03,
 0x55, 0x04,0x03,0x1e,0x06,0x00,0x22,0x00,0x31,0x00,0x22, };
static BYTE encodedMultipleAttrCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,
 0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x31,0x2b,0x32 };
static BYTE encodedCommaCN[] = {
0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x61,0x2c,
0x62 };
static BYTE encodedEqualCN[] = {
0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x61,0x3d,
0x62 };
static BYTE encodedLessThanCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3c
};
static BYTE encodedGreaterThanCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3e
};
static BYTE encodedHashCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x23
};
static BYTE encodedSemiCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3b
};
static BYTE encodedNewlineCN[] = {
0x30,0x11,0x31,0x0f,0x30,0x0d,0x06,0x03,0x55,0x04,0x03,0x1e,0x06,0x00,0x61,
0x00,0x0a,0x00,0x62 };
static BYTE encodedDummyCN[] = {
0x30,0x1F,0x31,0x0E,0x30,0x0C,0x06,0x03,0x55,0x04,0x03,0x13,0x05,0x64,0x75,
0x6D,0x6D,0x79,0x31,0x0D,0x30,0x0B,0x06,0x03,0x55,0x04,0x0C,0x13,0x04,0x74,
0x65,0x73,0x74 };
static BYTE encodedFields[] = {
0x30,0x2F,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x57,0x69,
0x6E,0x65,0x20,0x54,0x65,0x73,0x74,0x31,0x0C,0x30,0x0A,0x06,0x03,0x55,0x04,
0x0C,0x13,0x03,0x31,0x32,0x33,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,
0x13,0x02,0x42,0x52 };

static void test_CertNameToStrA(void)
{
    PCCERT_CONTEXT context;
    CERT_NAME_BLOB blob;

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        DWORD ret;

        /* This crashes
        ret = CertNameToStrA(0, NULL, 0, NULL, 0);
         */
        /* Test with a bogus encoding type */
        SetLastError(0xdeadbeef);
        ret = CertNameToStrA(0, &context->pCertInfo->Issuer, 0, NULL, 0);
        ok(ret == 1 && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected retval 1 and ERROR_FILE_NOT_FOUND, got %ld - %08lx\n",
         ret, GetLastError());
        SetLastError(0xdeadbeef);
        ret = CertNameToStrA(X509_ASN_ENCODING, &context->pCertInfo->Issuer,
         0, NULL, 0);
        ok(ret && GetLastError() == ERROR_SUCCESS,
         "Expected positive return and ERROR_SUCCESS, got %ld - %08lx\n",
         ret, GetLastError());

        test_NameToStrConversionA(&context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR, issuerStr);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, issuerStrSemicolon);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG, issuerStrCRLF);
        test_NameToStrConversionA(&context->pCertInfo->Subject, CERT_OID_NAME_STR, subjectStr);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG, subjectStrSemicolon);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG, subjectStrCRLF);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR, x500SubjectStr);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG | CERT_NAME_STR_REVERSE_FLAG,
         x500SubjectStrSemicolonReverse);

        CertFreeCertificateContext(context);
    }
    blob.pbData = encodedSimpleCN;
    blob.cbData = sizeof(encodedSimpleCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=1");
    blob.pbData = encodedSingleQuotedCN;
    blob.cbData = sizeof(encodedSingleQuotedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN='1'");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "'1'");
    blob.pbData = encodedSpacedCN;
    blob.cbData = sizeof(encodedSpacedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\" 1 \"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\" 1 \"");
    blob.pbData = encodedQuotedCN;
    blob.cbData = sizeof(encodedQuotedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"\"\"1\"\"\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"\"\"1\"\"\"");
    blob.pbData = encodedMultipleAttrCN;
    blob.cbData = sizeof(encodedMultipleAttrCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"1+2\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"1+2\"");
    blob.pbData = encodedCommaCN;
    blob.cbData = sizeof(encodedCommaCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a,b\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a,b\"");
    blob.pbData = encodedEqualCN;
    blob.cbData = sizeof(encodedEqualCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a=b\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a=b\"");
    blob.pbData = encodedLessThanCN;
    blob.cbData = sizeof(encodedLessThanCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"<\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"<\"");
    blob.pbData = encodedGreaterThanCN;
    blob.cbData = sizeof(encodedGreaterThanCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\">\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\">\"");
    blob.pbData = encodedHashCN;
    blob.cbData = sizeof(encodedHashCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"#\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"#\"");
    blob.pbData = encodedSemiCN;
    blob.cbData = sizeof(encodedSemiCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\";\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\";\"");
    blob.pbData = encodedNewlineCN;
    blob.cbData = sizeof(encodedNewlineCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a\nb\"");
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a\nb\"");
}

#define test_NameToStrConversionW(a, b, c) test_NameToStrConversionW_(__LINE__, a, b, c)
static void test_NameToStrConversionW_(unsigned int line, PCERT_NAME_BLOB pName, DWORD dwStrType, LPCWSTR expected)
{
    DWORD len, retlen, expected_len;
    WCHAR buffer[2000];

    expected_len = wcslen(expected) + 1;
    memset(buffer, 0xcc, sizeof(buffer));
    len = CertNameToStrW(X509_ASN_ENCODING, pName, dwStrType, NULL, 0);
    ok(len == expected_len, "line %u: expected %lu chars, got %lu\n", line, expected_len, len);
    retlen = CertNameToStrW(X509_ASN_ENCODING, pName, dwStrType, buffer, ARRAY_SIZE(buffer));
    ok(retlen == len, "line %u: expected %lu chars, got %lu.\n", line, len, retlen);
    ok(!wcscmp(buffer, expected), "Expected %s, got %s\n", wine_dbgstr_w(expected), wine_dbgstr_w(buffer));

    memset(buffer, 0xcc, sizeof(buffer));
    retlen = CertNameToStrW(X509_ASN_ENCODING, pName, dwStrType, buffer, len - 1);
    ok(retlen == len - 1, "line %u: expected %lu chars, got %lu\n", line, len - 1, retlen);
    ok(!wcsncmp(buffer, expected, retlen - 1), "line %u: expected %s, got %s\n",
            line, wine_dbgstr_w(expected), wine_dbgstr_w(buffer));
    ok(!buffer[retlen - 1], "line %u: string is not zero terminated.\n", line);

    memset(buffer, 0xcc, sizeof(buffer));
    retlen = CertNameToStrW(X509_ASN_ENCODING, pName, dwStrType, buffer, 0);
    ok(retlen == len, "line %u: expected %lu chars, got %lu\n", line, len - 1, retlen);
    ok(buffer[0] == 0xcccc, "line %u: got %s\n", line, wine_dbgstr_w(buffer));
}

static void test_CertNameToStrW(void)
{
    PCCERT_CONTEXT context;
    CERT_NAME_BLOB blob;

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        DWORD ret;

        /* This crashes
        ret = CertNameToStrW(0, NULL, 0, NULL, 0);
         */
        /* Test with a bogus encoding type */
        SetLastError(0xdeadbeef);
        ret = CertNameToStrW(0, &context->pCertInfo->Issuer, 0, NULL, 0);
        ok(ret == 1 && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected retval 1 and ERROR_FILE_NOT_FOUND, got %ld - %08lx\n",
         ret, GetLastError());
        SetLastError(0xdeadbeef);
        ret = CertNameToStrW(X509_ASN_ENCODING, &context->pCertInfo->Issuer,
         0, NULL, 0);
        ok(ret && GetLastError() == ERROR_SUCCESS,
         "Expected positive return and ERROR_SUCCESS, got %ld - %08lx\n",
         ret, GetLastError());

        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR,
         L"US, Minnesota, Minneapolis, CodeWeavers, Wine Development, localhost, aric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         L"US; Minnesota; Minneapolis; CodeWeavers; Wine Development; localhost; aric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         L"US\r\nMinnesota\r\nMinneapolis\r\nCodeWeavers\r\nWine Development\r\nlocalhost\r\naric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR,
         L"2.5.4.6=US, 2.5.4.8=Minnesota, 2.5.4.7=Minneapolis, 2.5.4.10=CodeWeavers, 2.5.4.11=Wine Development,"
          " 2.5.4.3=localhost, 1.2.840.113549.1.9.1=aric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         L"2.5.4.6=US; 2.5.4.8=Minnesota; 2.5.4.7=Minneapolis; 2.5.4.10=CodeWeavers; 2.5.4.11=Wine Development;"
          " 2.5.4.3=localhost; 1.2.840.113549.1.9.1=aric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         L"2.5.4.6=US\r\n2.5.4.8=Minnesota\r\n2.5.4.7=Minneapolis\r\n2.5.4.10=CodeWeavers\r\n2.5.4.11=Wine "
          "Development\r\n2.5.4.3=localhost\r\n1.2.840.113549.1.9.1=aric@codeweavers.com");
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG | CERT_NAME_STR_REVERSE_FLAG,
         L"E=aric@codeweavers.com; CN=localhost; OU=Wine Development; O=CodeWeavers; L=Minneapolis; S=Minnesota; "
          "C=US");

        CertFreeCertificateContext(context);
    }
    blob.pbData = encodedSimpleCN;
    blob.cbData = sizeof(encodedSimpleCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=1");
    blob.pbData = encodedSingleQuotedCN;
    blob.cbData = sizeof(encodedSingleQuotedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN='1'");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"'1'");
    blob.pbData = encodedSpacedCN;
    blob.cbData = sizeof(encodedSpacedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\" 1 \"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\" 1 \"");
    blob.pbData = encodedQuotedCN;
    blob.cbData = sizeof(encodedQuotedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"\"\"1\"\"\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"\"\"1\"\"\"");
    blob.pbData = encodedMultipleAttrCN;
    blob.cbData = sizeof(encodedMultipleAttrCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"1+2\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"1+2\"");
    blob.pbData = encodedCommaCN;
    blob.cbData = sizeof(encodedCommaCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a,b\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a,b\"");
    blob.pbData = encodedEqualCN;
    blob.cbData = sizeof(encodedEqualCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a=b\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a=b\"");
    blob.pbData = encodedLessThanCN;
    blob.cbData = sizeof(encodedLessThanCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"<\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"<\"");
    blob.pbData = encodedGreaterThanCN;
    blob.cbData = sizeof(encodedGreaterThanCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\">\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\">\"");
    blob.pbData = encodedHashCN;
    blob.cbData = sizeof(encodedHashCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"#\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"#\"");
    blob.pbData = encodedSemiCN;
    blob.cbData = sizeof(encodedSemiCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\";\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\";\"");
    blob.pbData = encodedNewlineCN;
    blob.cbData = sizeof(encodedNewlineCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a\nb\"");
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a\nb\"");
}

struct StrToNameA
{
    LPCSTR x500;
    DWORD encodedSize;
    const BYTE *encoded;
};

static const struct StrToNameA namesA[] = {
 { "CN=1", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN=\"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN = \"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN='1'", sizeof(encodedSingleQuotedCN), encodedSingleQuotedCN },
 { "CN=\" 1 \"", sizeof(encodedSpacedCN), encodedSpacedCN },
 { "CN=\"\"\"1\"\"\"", sizeof(encodedQuotedCN), encodedQuotedCN },
 { "CN=\"1+2\"", sizeof(encodedMultipleAttrCN), encodedMultipleAttrCN },
 { "CN=\"a,b\"", sizeof(encodedCommaCN), encodedCommaCN },
 { "CN=\"a=b\"", sizeof(encodedEqualCN), encodedEqualCN },
 { "CN=\"<\"", sizeof(encodedLessThanCN), encodedLessThanCN },
 { "CN=\">\"", sizeof(encodedGreaterThanCN), encodedGreaterThanCN },
 { "CN=\"#\"", sizeof(encodedHashCN), encodedHashCN },
 { "CN=\";\"", sizeof(encodedSemiCN), encodedSemiCN },
 { "CN=dummy,T=test", sizeof(encodedDummyCN), encodedDummyCN },
 { " CN =   Wine Test,T = 123, C = BR", sizeof(encodedFields), encodedFields },
};

static void test_CertStrToNameA(void)
{
    BOOL ret;
    DWORD size, i;
    BYTE buf[100];

    /* Crash
    ret = CertStrToNameA(0, NULL, 0, NULL, NULL, NULL, NULL);
     */
    ret = CertStrToNameA(0, NULL, 0, NULL, NULL, &size, NULL);
    ok(!ret, "Expected failure\n");
    ret = CertStrToNameA(0, "bogus", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(0, "foo=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(0, "CN=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=1", 0, NULL, NULL, &size, NULL);
    ok(ret, "CertStrToNameA failed: %08lx\n", GetLastError());
    size = sizeof(buf);
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=\"\"1\"\"", 0, NULL, buf, &size,
     NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=1+2", 0, NULL, buf,
     &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=1+2", CERT_NAME_STR_NO_PLUS_FLAG, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=1,2", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
                 "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=\"1,2;3,4\"", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
                 "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=abc", 0, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=abc", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=\"abc\"", 0, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = CertStrToNameA(X509_ASN_ENCODING, "CN=\"abc\"", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == ERROR_MORE_DATA,
                 "Expected ERROR_MORE_DATA, got %08lx\n", GetLastError());
    for (i = 0; i < ARRAY_SIZE(namesA); i++)
    {
        size = sizeof(buf);
        ret = CertStrToNameA(X509_ASN_ENCODING, namesA[i].x500, 0, NULL, buf,
         &size, NULL);
        ok(ret, "CertStrToNameA failed on string %s: %08lx\n", namesA[i].x500,
         GetLastError());
        ok(size == namesA[i].encodedSize,
         "Expected size %ld, got %ld\n", namesA[i].encodedSize, size);
        if (ret)
            ok(!memcmp(buf, namesA[i].encoded, namesA[i].encodedSize),
             "Unexpected value for string %s\n", namesA[i].x500);
    }
}

struct StrToNameW
{
    LPCWSTR x500;
    DWORD encodedSize;
    const BYTE *encoded;
};

static const BYTE encodedJapaneseCN[] = { 0x30,0x0f,0x31,0x0d,0x30,0x0b,0x06,
 0x03,0x55,0x04,0x03,0x1e,0x04,0x22,0x6f,0x57,0x5b };

static const struct StrToNameW namesW[] = {
 { L"CN=1", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN=\"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN = \"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN='1'", sizeof(encodedSingleQuotedCN), encodedSingleQuotedCN },
 { L"CN=\" 1 \"", sizeof(encodedSpacedCN), encodedSpacedCN },
 { L"CN=\"\"\"1\"\"\"", sizeof(encodedQuotedCN), encodedQuotedCN },
 { L"CN=\"1+2\"", sizeof(encodedMultipleAttrCN), encodedMultipleAttrCN },
 { L"CN=\x226f\x575b", sizeof(encodedJapaneseCN), encodedJapaneseCN },
 { L"CN=\"a,b\"", sizeof(encodedCommaCN), encodedCommaCN },
 { L"CN=\"a=b\"", sizeof(encodedEqualCN), encodedEqualCN },
 { L"CN=\"<\"", sizeof(encodedLessThanCN), encodedLessThanCN },
 { L"CN=\">\"", sizeof(encodedGreaterThanCN), encodedGreaterThanCN },
 { L"CN=\"#\"", sizeof(encodedHashCN), encodedHashCN },
 { L"CN=\";\"", sizeof(encodedSemiCN), encodedSemiCN },
 { L"CN=dummy,T=test", sizeof(encodedDummyCN), encodedDummyCN },
 { L" CN =   Wine Test,T = 123, C = BR", sizeof(encodedFields), encodedFields },
};

static void test_CertStrToNameW(void)
{
    BOOL ret;
    DWORD size, i;
    LPCWSTR errorPtr;
    BYTE buf[100];

    /* Crash
    ret = CertStrToNameW(0, NULL, 0, NULL, NULL, NULL, NULL);
     */
    ret = CertStrToNameW(0, NULL, 0, NULL, NULL, &size, NULL);
    ok(!ret, "Expected failure\n");
    ret = CertStrToNameW(0, L"bogus", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameW(0, L"foo=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameW(0, L"CN=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CertStrToNameW(X509_ASN_ENCODING, L"CN=1", 0, NULL, NULL, &size,
     NULL);
    ok(ret, "CertStrToNameW failed: %08lx\n", GetLastError());
    size = sizeof(buf);
    ret = CertStrToNameW(X509_ASN_ENCODING, L"CN=\"\"1\"\"", 0, NULL, buf,
     &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = CertStrToNameW(X509_ASN_ENCODING, L"CN=\"\"1\"\"", 0, NULL, buf,
     &size, &errorPtr);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ok(errorPtr && *errorPtr == '1', "Expected first error character was 1\n");
    for (i = 0; i < ARRAY_SIZE(namesW); i++)
    {
        size = sizeof(buf);
        ret = CertStrToNameW(X509_ASN_ENCODING, namesW[i].x500, 0, NULL, buf,
         &size, NULL);
        ok(ret, "Index %ld: CertStrToNameW failed: %08lx\n", i, GetLastError());
        ok(size == namesW[i].encodedSize,
         "Index %ld: expected size %ld, got %ld\n", i, namesW[i].encodedSize,
         size);
        if (ret)
            ok(!memcmp(buf, namesW[i].encoded, size),
             "Index %ld: unexpected value for string %s\n", i, wine_dbgstr_w(namesW[i].x500));
    }
}

#define test_CertGetNameString_value(a, b, c, d, e) test_CertGetNameString_value_(__LINE__, a, b, c, d, e)
static void test_CertGetNameString_value_(unsigned int line, PCCERT_CONTEXT context, DWORD type, DWORD flags,
        void *type_para, const char *expected)
{
    DWORD len, retlen, expected_len;
    WCHAR expectedW[512];
    WCHAR strW[512];
    char str[512];

    expected_len = 0;
    while(expected[expected_len])
    {
        while((expectedW[expected_len] = expected[expected_len]))
            ++expected_len;
        if (!(flags & CERT_NAME_SEARCH_ALL_NAMES_FLAG))
            break;
        expectedW[expected_len++] = 0;
    }
    expectedW[expected_len++] = 0;

    len = CertGetNameStringA(context, type, flags, type_para, NULL, 0);
    if (flags & CERT_NAME_SEARCH_ALL_NAMES_FLAG && ((type == CERT_NAME_DNS_TYPE && len < expected_len)
        || (type != CERT_NAME_DNS_TYPE && len > expected_len)))
    {
        /* Supported since Win8. */
        win_skip("line %u: CERT_NAME_SEARCH_ALL_NAMES_FLAG is not supported.\n", line);
        return;
    }
    ok(len == expected_len, "line %u: unexpected length %ld, expected %ld.\n", line, len, expected_len);
    memset(str, 0xcc, len);
    retlen = CertGetNameStringA(context, type, flags, type_para, str, len);
    ok(retlen == len, "line %u: unexpected len %lu, expected %lu.\n", line, retlen, len);
    ok(!memcmp(str, expected, expected_len), "line %u: unexpected value %s.\n", line, debugstr_an(str, expected_len));
    str[0] = str[1] = 0xcc;
    retlen = CertGetNameStringA(context, type, flags, type_para, str, len - 1);
    ok(retlen == 1, "line %u: Unexpected len %lu, expected 1.\n", line, retlen);
    if (len == 1) return;
    ok(!str[0], "line %u: unexpected str[0] %#x.\n", line, str[0]);
    ok(str[1] == expected[1], "line %u: unexpected str[1] %#x.\n", line, str[1]);
    ok(!memcmp(str + 1, expected + 1, len - 2),
            "line %u: str %s, string data mismatch.\n", line, debugstr_a(str + 1));
    retlen = CertGetNameStringA(context, type, flags, type_para, str, 0);
    ok(retlen == len, "line %u: Unexpected len %lu, expected 1.\n", line, retlen);

    memset(strW, 0xcc, len * sizeof(*strW));
    retlen = CertGetNameStringW(context, type, flags, type_para, strW, len);
    ok(retlen == expected_len, "line %u: unexpected len %lu, expected %lu.\n", line, retlen, expected_len);
    ok(!memcmp(strW, expectedW, len * sizeof(*strW)), "line %u: unexpected value %s.\n", line, debugstr_wn(strW, len));
    strW[0] = strW[1] = 0xcccc;
    retlen = CertGetNameStringW(context, type, flags, type_para, strW, len - 1);
    ok(retlen == len - 1, "line %u: unexpected len %lu, expected %lu.\n", line, retlen, len - 1);
    if (flags & CERT_NAME_SEARCH_ALL_NAMES_FLAG)
    {
        ok(!memcmp(strW, expectedW, (retlen - 2) * sizeof(*strW)),
                "line %u: str %s, string data mismatch.\n", line, debugstr_wn(strW, retlen - 2));
        ok(!strW[retlen - 2], "line %u: string is not zero terminated.\n", line);
        ok(!strW[retlen - 1], "line %u: string sequence is not zero terminated.\n", line);

        retlen = CertGetNameStringW(context, type, flags, type_para, strW, 1);
        ok(retlen == 1, "line %u: unexpected len %lu, expected %lu.\n", line, retlen, len - 1);
        ok(!strW[retlen - 1], "line %u: string sequence is not zero terminated.\n", line);
    }
    else
    {
        ok(!memcmp(strW, expectedW, (retlen - 1) * sizeof(*strW)),
                "line %u: str %s, string data mismatch.\n", line, debugstr_wn(strW, retlen - 1));
        ok(!strW[retlen - 1], "line %u: string is not zero terminated.\n", line);
    }
    retlen = CertGetNameStringA(context, type, flags, type_para, NULL, len - 1);
    ok(retlen == len, "line %u: unexpected len %lu, expected %lu\n", line, retlen, len);
    retlen = CertGetNameStringW(context, type, flags, type_para, NULL, len - 1);
    ok(retlen == len, "line %u: unexpected len %lu, expected %lu\n", line, retlen, len);
}

static void test_CertGetNameString(void)
{
    static const char aric[] = "aric@codeweavers.com";
    static const char localhost[] = "localhost";
    PCCERT_CONTEXT context;
    DWORD len, type;

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(!!context, "CertCreateCertificateContext failed, err %lu\n", GetLastError());

    /* Bad string types/types missing from the cert */
    len = CertGetNameStringA(NULL, 0, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);
    len = CertGetNameStringA(context, 0, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);
    len = CertGetNameStringA(context, CERT_NAME_URL_TYPE, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);

    len = CertGetNameStringW(NULL, 0, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);
    len = CertGetNameStringW(context, 0, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);
    len = CertGetNameStringW(context, CERT_NAME_URL_TYPE, 0, NULL, NULL, 0);
    ok(len == 1, "expected 1, got %lu\n", len);

    test_CertGetNameString_value(context, CERT_NAME_EMAIL_TYPE, 0, NULL, aric);
    test_CertGetNameString_value(context, CERT_NAME_RDN_TYPE, 0, NULL, issuerStr);
    type = 0;
    test_CertGetNameString_value(context, CERT_NAME_RDN_TYPE, 0, &type, issuerStr);
    type = CERT_OID_NAME_STR;
    test_CertGetNameString_value(context, CERT_NAME_RDN_TYPE, 0, &type, subjectStr);
    test_CertGetNameString_value(context, CERT_NAME_ATTR_TYPE, 0, NULL, aric);
    test_CertGetNameString_value(context, CERT_NAME_ATTR_TYPE, 0, (void *)szOID_RSA_emailAddr, aric);
    test_CertGetNameString_value(context, CERT_NAME_ATTR_TYPE, 0, (void *)szOID_COMMON_NAME, localhost);
    test_CertGetNameString_value(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, localhost);
    test_CertGetNameString_value(context, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, localhost);
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, 0, NULL, localhost);
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, CERT_NAME_SEARCH_ALL_NAMES_FLAG, NULL, "localhost\0");
    test_CertGetNameString_value(context, CERT_NAME_EMAIL_TYPE, CERT_NAME_SEARCH_ALL_NAMES_FLAG, NULL, "");
    test_CertGetNameString_value(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_SEARCH_ALL_NAMES_FLAG, NULL, "");

    CertFreeCertificateContext(context);

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert_no_email,
     sizeof(cert_no_email));
    ok(!!context, "CertCreateCertificateContext failed, err %lu\n", GetLastError());

    test_CertGetNameString_value(context, CERT_NAME_ATTR_TYPE, 0, NULL, localhost);

    CertFreeCertificateContext(context);

    ok(cert_v3[CERT_V3_SAN_OID_OFFSET] == 0x55, "Incorrect CERT_V3_SAN_OID_OFFSET.\n");
    ok(cert_v3[CERT_V3_IAN_OID_OFFSET] == 0x55, "Incorrect CERT_V3_IAN_OID_OFFSET.\n");
    cert_v3[CERT_V3_SAN_OID_OFFSET + 2] = 7; /* legacy OID_SUBJECT_ALT_NAME */
    cert_v3[CERT_V3_IAN_OID_OFFSET + 2] = 8; /* legacy OID_ISSUER_ALT_NAME */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert_v3, sizeof(cert_v3));
    ok(!!context, "CertCreateCertificateContext failed, err %lu\n", GetLastError());
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, 0, NULL, "ex1.org");
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, CERT_NAME_ISSUER_FLAG, NULL, "ex3.org");
    CertFreeCertificateContext(context);

    cert_v3[CERT_V3_SAN_OID_OFFSET + 2] = 17; /* OID_SUBJECT_ALT_NAME2 */
    cert_v3[CERT_V3_IAN_OID_OFFSET + 2] = 18; /* OID_ISSUER_ALT_NAME2 */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert_v3, sizeof(cert_v3));
    ok(!!context, "CertCreateCertificateContext failed, err %lu\n", GetLastError());
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, 0, NULL, "ex1.org");
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, CERT_NAME_ISSUER_FLAG, NULL, "ex3.org");
    test_CertGetNameString_value(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, "server_cn.org");
    test_CertGetNameString_value(context, CERT_NAME_ATTR_TYPE, 0, (void *)szOID_SUR_NAME, "");
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, CERT_NAME_SEARCH_ALL_NAMES_FLAG,
            NULL, "ex1.org\0*.ex2.org\0");
    test_CertGetNameString_value(context, CERT_NAME_DNS_TYPE, CERT_NAME_SEARCH_ALL_NAMES_FLAG | CERT_NAME_ISSUER_FLAG,
            NULL, "ex3.org\0*.ex4.org\0");
    CertFreeCertificateContext(context);
}

static void test_quoted_RDN(void)
{
    static const WCHAR str1[] = { '1',0x00a0,0 };
    static const WCHAR str2[] = { '1',0x3000,0 };
    static const struct
    {
        const WCHAR *CN;
        const WCHAR *X500_CN;
    } test[] =
    {
        { L"1", L"1" },
        { L" 1", L"\" 1\"" },
        { L"1 ", L"\"1 \"" },
        { L"\"1\"", L"\"\"\"1\"\"\"" },
        { L"\" 1 \"", L"\"\"\" 1 \"\"\"" },
        { L"\"\"\"1\"\"\"", L"\"\"\"\"\"\"\"1\"\"\"\"\"\"\"" },
        { L"1+", L"\"1+\"" },
        { L"1=", L"\"1=\"" },
        { L"1\"", L"\"1\"\"\"" },
        { L"1<", L"\"1<\"" },
        { L"1>", L"\"1>\"" },
        { L"1#", L"\"1#\"" },
        { L"1+", L"\"1+\"" },
        { L"1\t", L"\"1\t\"" },
        { L"1\r", L"\"1\r\"" },
        { L"1\n", L"\"1\n\"" },
        { str1, str1 },
        { str2, str2 },
    };
    CERT_RDN_ATTR attr;
    CERT_RDN rdn;
    CERT_NAME_INFO info;
    CERT_NAME_BLOB blob;
    BYTE *buf;
    WCHAR str[256];
    DWORD size, ret, i;

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        winetest_push_context("%lu", i);

        attr.pszObjId = (LPSTR)szOID_COMMON_NAME;
        attr.dwValueType = CERT_RDN_UNICODE_STRING;
        attr.Value.cbData = wcslen(test[i].CN) * sizeof(WCHAR);
        attr.Value.pbData = (BYTE *)test[i].CN;
        rdn.cRDNAttr = 1;
        rdn.rgRDNAttr = &attr;
        info.cRDN = 1;
        info.rgRDN = &rdn;
        buf = NULL;
        size = 0;
        ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_UNICODE_NAME, &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret, "CryptEncodeObjectEx error %08lx\n", GetLastError());

        blob.pbData = buf;
        blob.cbData = size;

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, 0, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcscmp(str, test[i].X500_CN), "got %s, expected %s\n", debugstr_w(str), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_SIMPLE_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcscmp(str, test[i].X500_CN), "got %s, expected %s\n", debugstr_w(str), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_OID_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"2.5.4.3=", 8), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[8], test[i].X500_CN), "got %s, expected %s\n", debugstr_w(&str[8]), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_X500_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"CN=", 3), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[3], test[i].X500_CN), "got %s, expected %s\n", debugstr_w(&str[3]), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_X500_NAME_STR | CERT_NAME_STR_NO_QUOTING_FLAG, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"CN=", 3), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[3], test[i].CN), "got %s, expected %s\n", debugstr_w(&str[3]), debugstr_w(test[i].CN));

        LocalFree(buf);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        winetest_push_context("%lu", i);

        attr.pszObjId = (LPSTR)szOID_COMMON_NAME;
        attr.dwValueType = CERT_RDN_UTF8_STRING;
        attr.Value.cbData = wcslen(test[i].CN) * sizeof(WCHAR);
        attr.Value.pbData = (BYTE *)test[i].CN;
        rdn.cRDNAttr = 1;
        rdn.rgRDNAttr = &attr;
        info.cRDN = 1;
        info.rgRDN = &rdn;
        buf = NULL;
        size = 0;
        ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_UNICODE_NAME, &info, CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
        ok(ret, "CryptEncodeObjectEx error %08lx\n", GetLastError());

        blob.pbData = buf;
        blob.cbData = size;

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, 0, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcscmp(str, test[i].X500_CN), "got %s, expected %s\n", debugstr_w(str), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_SIMPLE_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcscmp(str, test[i].X500_CN), "got %s, expected %s\n", debugstr_w(str), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_OID_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"2.5.4.3=", 8), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[8], test[i].X500_CN), "got %s, expected %s\n", debugstr_w(&str[8]), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_X500_NAME_STR, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"CN=", 3), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[3], test[i].X500_CN), "got %s, expected %s\n", debugstr_w(&str[3]), debugstr_w(test[i].X500_CN));

        str[0] = 0;
        ret = CertNameToStrW(X509_ASN_ENCODING, &blob, CERT_X500_NAME_STR | CERT_NAME_STR_NO_QUOTING_FLAG, str, ARRAY_SIZE(str));
        ok(ret, "CertNameToStr error %08lx\n", GetLastError());
        ok(!wcsncmp(str, L"CN=", 3), "got %s\n", debugstr_w(str));
        ok(!wcscmp(&str[3], test[i].CN), "got %s, expected %s\n", debugstr_w(&str[3]), debugstr_w(test[i].CN));

        LocalFree(buf);

        winetest_pop_context();
    }
}

START_TEST(str)
{
    test_quoted_RDN();
    test_CertRDNValueToStrA();
    test_CertRDNValueToStrW();
    test_CertNameToStrA();
    test_CertNameToStrW();
    test_CertStrToNameA();
    test_CertStrToNameW();
    test_CertGetNameString();
}
