/*
 * Copyright 2007 Hans Leidekker
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

#include <wine/test.h>

#include <windef.h>
#include <snmp.h>

static INT  (WINAPI *pSnmpUtilAsnAnyCpy)(AsnAny*, AsnAny*);
static VOID (WINAPI *pSnmpUtilAsnAnyFree)(AsnAny*);
static INT  (WINAPI *pSnmpUtilOctetsCmp)(AsnOctetString*, AsnOctetString*);
static INT  (WINAPI *pSnmpUtilOctetsCpy)(AsnOctetString*, AsnOctetString*);
static VOID (WINAPI *pSnmpUtilOctetsFree)(AsnOctetString*);
static INT  (WINAPI *pSnmpUtilOctetsNCmp)(AsnOctetString*, AsnOctetString*, UINT);

static void InitFunctionPtrs(void)
{
    HMODULE hSnmpapi = GetModuleHandleA("snmpapi.dll");

#define SNMPAPI_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hSnmpapi, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    SNMPAPI_GET_PROC(SnmpUtilAsnAnyCpy)
    SNMPAPI_GET_PROC(SnmpUtilAsnAnyFree)
    SNMPAPI_GET_PROC(SnmpUtilOctetsCmp)
    SNMPAPI_GET_PROC(SnmpUtilOctetsCpy)
    SNMPAPI_GET_PROC(SnmpUtilOctetsFree)
    SNMPAPI_GET_PROC(SnmpUtilOctetsNCmp)

#undef SNMPAPI_GET_PROC
}

static void test_SnmpUtilOidToA(void)
{
    LPSTR ret;
    static UINT ids1[] = { 1,3,6,1,4,1,311 };
    static UINT ids2[] = {
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    static UINT ids3[] = { 0xffffffff };
    static AsnObjectIdentifier oid0 = { 0, ids1 };
    static AsnObjectIdentifier oid1 = { 7, ids1 };
    static AsnObjectIdentifier oid2 = { 256, ids2 };
    static AsnObjectIdentifier oid3 = { 257, ids2 };
    static AsnObjectIdentifier oid4 = { 258, ids2 };
    static AsnObjectIdentifier oid5 = { 1, ids3 };
    static const char expect0[] = "<null oid>";
    static const char expect0_alt[] = "NUL";
    static const char expect1[] = "1.3.6.1.4.1.311";
    static const char expect2[] =
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1";
    static const char expect3[] =
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1";
    static const char expect3_alt[] =
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1";
    static const char expect4[] = "-1";

    /* This crashes under win98 */
    if(0)
    {
        ret = SnmpUtilOidToA(NULL);
        ok(ret != NULL, "SnmpUtilOidToA failed\n");
        ok(!strcmp(ret, expect0), "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
           ret, expect1);
    }

    ret = SnmpUtilOidToA(&oid0);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect0) ||
       broken(!strcmp(ret, expect0_alt)), /* Win98, WinMe, NT4 */
       "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect0);

    ret = SnmpUtilOidToA(&oid1);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect1), "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect1);

    ret = SnmpUtilOidToA(&oid2);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect2), "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect2);

    ret = SnmpUtilOidToA(&oid3);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect3), "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect3);

    ret = SnmpUtilOidToA(&oid4);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect3) ||
       broken(!strcmp(ret, expect3_alt)), /* Win98, WinMe, NT4 */
       "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect3);

    ret = SnmpUtilOidToA(&oid5);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect4), "SnmpUtilOidToA failed got\n%s\n expected\n%s\n",
       ret, expect4);
}

static void test_SnmpUtilAsnAnyCpyFree(void)
{
    INT ret;
    static AsnAny dst, src = { ASN_INTEGER, { 1 } };

    if (0) { /* these crash on XP */
    ret = pSnmpUtilAsnAnyCpy(NULL, NULL);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");

    ret = pSnmpUtilAsnAnyCpy(&dst, NULL);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");

    ret = pSnmpUtilAsnAnyCpy(NULL, &src);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");
    }

    ret = pSnmpUtilAsnAnyCpy(&dst, &src);
    ok(ret, "SnmpUtilAsnAnyCpy failed\n");
    ok(!memcmp(&src, &dst, sizeof(AsnAny)), "SnmpUtilAsnAnyCpy failed\n");

    if (0) { /* crashes on XP */
    pSnmpUtilAsnAnyFree(NULL);
    }
    pSnmpUtilAsnAnyFree(&dst);
    ok(dst.asnType == ASN_NULL, "SnmpUtilAsnAnyFree failed\n");
    ok(dst.asnValue.number == 1, "SnmpUtilAsnAnyFree failed\n");
}

static void test_SnmpUtilOctetsCpyFree(void)
{
    INT ret;
    static BYTE stream[] = { '1', '2', '3', '4' };
    static AsnOctetString dst, src = { stream, 4, TRUE };

    ret = pSnmpUtilOctetsCpy(NULL, NULL);
    ok(!ret, "SnmpUtilOctetsCpy succeeded\n");

    memset(&dst, 1, sizeof(AsnOctetString));
    ret = pSnmpUtilOctetsCpy(&dst, NULL);
    ok(ret, "SnmpUtilOctetsCpy failed\n");
    ok(dst.length == 0, "SnmpUtilOctetsCpy failed\n");
    ok(dst.stream == NULL, "SnmpUtilOctetsCpy failed\n");
    ok(dst.dynamic == FALSE, "SnmpUtilOctetsCpy failed\n");

    ret = pSnmpUtilOctetsCpy(NULL, &src);
    ok(!ret, "SnmpUtilOctetsCpy succeeded\n");

    memset(&dst, 0, sizeof(AsnOctetString));
    ret = pSnmpUtilOctetsCpy(&dst, &src);
    ok(ret, "SnmpUtilOctetsCpy failed\n");
    ok(src.length == dst.length, "SnmpUtilOctetsCpy failed\n");
    ok(!memcmp(src.stream, dst.stream, dst.length), "SnmpUtilOctetsCpy failed\n");
    ok(dst.dynamic == TRUE, "SnmpUtilOctetsCpy failed\n");

    pSnmpUtilOctetsFree(NULL);
    pSnmpUtilOctetsFree(&dst);
    ok(dst.stream == NULL, "SnmpUtilOctetsFree failed\n");
    ok(dst.length == 0, "SnmpUtilOctetsFree failed\n");
    ok(dst.dynamic == FALSE, "SnmpUtilOctetsFree failed\n");
}

static void test_SnmpUtilOidCpyFree(void)
{
    INT ret;
    static UINT ids[] = { 1, 3, 6, 1, 4, 1, 311 };
    static AsnObjectIdentifier dst, src = { ARRAY_SIZE(ids), ids };

    /* These crashes under win98 */
    if(0)
    {
        ret = SnmpUtilOidCpy(NULL, NULL);
        ok(!ret, "SnmpUtilOidCpy succeeded\n");

        memset(&dst, 1, sizeof(AsnObjectIdentifier));
        ret = SnmpUtilOidCpy(&dst, NULL);
        ok(ret, "SnmpUtilOidCpy failed\n");
        ok(dst.idLength == 0, "SnmpUtilOidCpy failed\n");
        ok(dst.ids == NULL, "SnmpUtilOidCpy failed\n");

        ret = SnmpUtilOidCpy(NULL, &src);
        ok(!ret, "SnmpUtilOidCpy succeeded\n");
    }

    memset(&dst, 0, sizeof(AsnObjectIdentifier));
    ret = SnmpUtilOidCpy(&dst, &src);
    ok(ret, "SnmpUtilOidCpy failed\n");
    ok(src.idLength == dst.idLength, "SnmpUtilOidCpy failed\n");
    ok(!memcmp(src.ids, dst.ids, dst.idLength * sizeof(UINT)), "SnmpUtilOidCpy failed\n");
    SnmpUtilOidFree(&dst);

    /* These crashes under win98 */
    if(0)
    {
        ret = SnmpUtilOidCpy(NULL, NULL);
        ok(!ret, "SnmpUtilOidCpy succeeded\n");

        memset(&dst, 1, sizeof(AsnObjectIdentifier));
        ret = SnmpUtilOidCpy(&dst, NULL);
        ok(ret, "SnmpUtilOidCpy failed\n");
        ok(dst.idLength == 0, "SnmpUtilOidCpy failed\n");
        ok(dst.ids == NULL, "SnmpUtilOidCpy failed\n");

        ret = SnmpUtilOidCpy(NULL, &src);
        ok(!ret, "SnmpUtilOidCpy succeeded\n");
    }

    memset(&dst, 0, sizeof(AsnObjectIdentifier));
    ret = SnmpUtilOidCpy(&dst, &src);
    ok(ret, "SnmpUtilOidCpy failed\n");
    ok(src.idLength == dst.idLength, "SnmpUtilOidCpy failed\n");
    ok(!memcmp(src.ids, dst.ids, dst.idLength * sizeof(UINT)), "SnmpUtilOidCpy failed\n");

    /* This crashes under win98 */
    if(0)
    {
        SnmpUtilOidFree(NULL);
    }
    SnmpUtilOidFree(&dst);
    ok(dst.idLength == 0, "SnmpUtilOidFree failed\n");
    ok(dst.ids == NULL, "SnmpUtilOidFree failed\n");
}

static void test_SnmpUtilOctetsNCmp(void)
{
    INT ret;
    static BYTE stream1[] = { '1', '2', '3', '4' };
    static BYTE stream2[] = { '5', '6', '7', '8' };
    static AsnOctetString octets1 = { stream1, 4, FALSE };
    static AsnOctetString octets2 = { stream2, 4, FALSE };

    ret = pSnmpUtilOctetsNCmp(NULL, NULL, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(NULL, NULL, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(&octets1, NULL, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(&octets1, NULL, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(NULL, &octets2, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(NULL, &octets2, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = pSnmpUtilOctetsNCmp(&octets1, &octets1, 0);
    ok(!ret, "SnmpUtilOctetsNCmp failed\n");

    ret = pSnmpUtilOctetsNCmp(&octets1, &octets1, 4);
    ok(!ret, "SnmpUtilOctetsNCmp failed\n");

    ret = pSnmpUtilOctetsNCmp(&octets1, &octets2, 4);
    ok(ret == -4, "SnmpUtilOctetsNCmp failed\n");

    ret = pSnmpUtilOctetsNCmp(&octets2, &octets1, 4);
    ok(ret == 4, "SnmpUtilOctetsNCmp failed\n");
}

static void test_SnmpUtilOctetsCmp(void)
{
    INT ret;
    static BYTE stream1[] = { '1', '2', '3' };
    static BYTE stream2[] = { '1', '2', '3', '4' };
    static AsnOctetString octets1 = { stream1, 3, FALSE };
    static AsnOctetString octets2 = { stream2, 4, FALSE };

    if (0) { /* these crash on XP */
    ret = pSnmpUtilOctetsCmp(NULL, NULL);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");

    ret = pSnmpUtilOctetsCmp(&octets1, NULL);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");

    ret = pSnmpUtilOctetsCmp(NULL, &octets2);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");
    }

    ret = pSnmpUtilOctetsCmp(&octets2, &octets1);
    ok(ret == 1, "SnmpUtilOctetsCmp failed\n");

    ret = pSnmpUtilOctetsCmp(&octets1, &octets2);
    ok(ret < 0, "SnmpUtilOctetsCmp failed\n");
}

static void test_SnmpUtilOidNCmp(void)
{
    INT ret;
    static UINT ids1[] = { 1, 2, 3, 4 };
    static UINT ids2[] = { 5, 6, 7, 8 };
    static AsnObjectIdentifier oid1 = { 4, ids1 };
    static AsnObjectIdentifier oid2 = { 4, ids2 };

    /* This crashes under win98 */
    if(0)
    {
        ret = SnmpUtilOidNCmp(NULL, NULL, 0);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");

        ret = SnmpUtilOidNCmp(NULL, NULL, 1);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");

        ret = SnmpUtilOidNCmp(&oid1, NULL, 0);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");

        ret = SnmpUtilOidNCmp(&oid1, NULL, 1);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");

        ret = SnmpUtilOidNCmp(NULL, &oid2, 0);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");

        ret = SnmpUtilOidNCmp(NULL, &oid2, 1);
        ok(!ret, "SnmpUtilOidNCmp succeeded\n");
    }

    ret = SnmpUtilOidNCmp(&oid1, &oid1, 0);
    ok(!ret, "SnmpUtilOidNCmp failed\n");

    ret = SnmpUtilOidNCmp(&oid1, &oid1, 4);
    ok(!ret, "SnmpUtilOidNCmp failed\n");

    ret = SnmpUtilOidNCmp(&oid1, &oid2, 4);
    ok(ret < 0, "SnmpUtilOidNCmp failed: %d\n", ret);

    ret = SnmpUtilOidNCmp(&oid2, &oid1, 4);
    ok(ret > 0, "SnmpUtilOidNCmp failed: %d\n", ret);

    oid1.idLength = 3;
    memcpy(oid1.ids, oid2.ids, sizeof(UINT) * 4);
    ret = SnmpUtilOidNCmp(&oid1, &oid1, 4);
    ok(!ret, "SnmpUtilOidNCmp failed: %d\n", ret);
    ret = SnmpUtilOidNCmp(&oid2, &oid1, 4);
    ok(ret > 0, "SnmpUtilOidNCmp failed: %d\n", ret);
    ret = SnmpUtilOidNCmp(&oid1, &oid2, 4);
    ok(ret < 0, "SnmpUtilOidNCmp failed: %d\n", ret);

    ret = SnmpUtilOidNCmp(&oid1, &oid2, 2);
    ok(!ret, "SnmpUtilOidNCmp failed: %d\n", ret);
    ret = SnmpUtilOidNCmp(&oid2, &oid1, 2);
    ok(!ret, "SnmpUtilOidNCmp failed: %d\n", ret);
}

static void test_SnmpUtilOidCmp(void)
{
    INT ret;
    static UINT ids1[] = { 1, 2, 3 };
    static UINT ids2[] = { 1, 2, 3, 4 };
    static AsnObjectIdentifier oid1 = { 3, ids1 };
    static AsnObjectIdentifier oid2 = { 4, ids2 };

    if (0) { /* these crash on XP */
    ret = SnmpUtilOidCmp(NULL, NULL);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");

    ret = SnmpUtilOidCmp(&oid1, NULL);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");

    ret = SnmpUtilOidCmp(NULL, &oid2);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");
    }

    ret = SnmpUtilOidCmp(&oid2, &oid1);
    ok(ret > 0, "SnmpUtilOidCmp failed\n");

    ret = SnmpUtilOidCmp(&oid1, &oid2);
    ok(ret < 0, "SnmpUtilOidCmp failed\n");
}

static void test_SnmpUtilOidAppend(void)
{
    INT ret;
    UINT *ids1;
    static UINT ids2[] = { 4, 5, 6 };
    static AsnObjectIdentifier oid1;
    static AsnObjectIdentifier oid2 = { 3, ids2 };

    ids1 = SnmpUtilMemAlloc(3 * sizeof(UINT));
    ids1[0] = 1;
    ids1[1] = 2;
    ids1[2] = 3;

    oid1.idLength = 3;
    oid1.ids = ids1;

    /* This crashes under win98 */
    if(0)
    {
        ret = SnmpUtilOidAppend(NULL, NULL);
        ok(!ret, "SnmpUtilOidAppend succeeded\n");

        ret = SnmpUtilOidAppend(&oid1, NULL);
        ok(ret, "SnmpUtilOidAppend failed\n");

        ret = SnmpUtilOidAppend(NULL, &oid2);
        ok(!ret, "SnmpUtilOidAppend succeeded\n");
    }

    ret = SnmpUtilOidAppend(&oid1, &oid2);
    ok(ret, "SnmpUtilOidAppend failed\n");
    ok(oid1.idLength == 6, "SnmpUtilOidAppend failed\n");
    ok(!memcmp(&oid1.ids[3], ids2, 3 * sizeof(UINT)),
       "SnmpUtilOidAppend failed\n");

    SnmpUtilOidFree(&oid1);
}

static void test_SnmpUtilVarBindCpyFree(void)
{
    INT ret;
    static UINT ids[] = { 1, 3, 6, 1, 4, 1, 311 };
    static SnmpVarBind dst, src = { { 7, ids }, { ASN_INTEGER, { 1 } } };

    /* This crashes under win98 */
    if(0)
    {
        ret = SnmpUtilVarBindCpy(NULL, NULL);
        ok(!ret, "SnmpUtilVarBindCpy succeeded\n");

        memset(&dst, 0, sizeof(SnmpVarBind));
        ret = SnmpUtilVarBindCpy(&dst, NULL);
        ok(ret, "SnmpUtilVarBindCpy failed\n");
        ok(dst.name.idLength == 0, "SnmpUtilVarBindCpy failed\n");
        ok(dst.name.ids == NULL, "SnmpUtilVarBindCpy failed\n");
        ok(dst.value.asnType == ASN_NULL, "SnmpUtilVarBindCpy failed\n");
        ok(dst.value.asnValue.number == 0, "SnmpUtilVarBindCpy failed\n");

        ret = SnmpUtilVarBindCpy(NULL, &src);
        ok(!ret, "SnmpUtilVarBindCpy succeeded\n");
    }

    memset(&dst, 0, sizeof(SnmpVarBind));
    ret = SnmpUtilVarBindCpy(&dst, &src);
    ok(ret, "SnmpUtilVarBindCpy failed\n");
    ok(src.name.idLength == dst.name.idLength, "SnmpUtilVarBindCpy failed\n");
    ok(!memcmp(src.name.ids, dst.name.ids, dst.name.idLength * sizeof(UINT)),
       "SnmpUtilVarBindCpy failed\n");
    ok(!memcmp(&src.value, &dst.value, sizeof(AsnObjectSyntax)),
       "SnmpUtilVarBindCpy failed\n");

    /* This crashes under win98 */
    if(0)
    {
        SnmpUtilVarBindFree(NULL);
    }
    SnmpUtilVarBindFree(&dst);
    ok(dst.name.idLength == 0, "SnmpUtilVarBindFree failed\n");
    ok(dst.name.ids == NULL, "SnmpUtilVarBindFree failed\n");
    ok(dst.value.asnType == ASN_NULL, "SnmpUtilVarBindFree failed\n");
    ok(dst.value.asnValue.number == 1, "SnmpUtilVarBindFree failed\n");
}

static void test_SnmpUtilVarBindListCpyFree(void)
{
    INT ret;
    static UINT ids[] = { 1, 3, 6, 1, 4, 1, 311 };
    static SnmpVarBind src = { { 7, ids }, { ASN_INTEGER, { 1 } } };
    static SnmpVarBindList dst_list, src_list = { &src, 1 };

    if (0) { /* these crash on XP */
    ret = SnmpUtilVarBindListCpy(NULL, NULL);
    ok(!ret, "SnmpUtilVarBindCpy succeeded\n");

    ret = SnmpUtilVarBindListCpy(NULL, &src_list);
    ok(!ret, "SnmpUtilVarBindListCpy succeeded\n");
    }

    /* This crashes under win98 */
    if(0)
    {
        memset(&dst_list, 0xff, sizeof(SnmpVarBindList));
        ret = SnmpUtilVarBindListCpy(&dst_list, NULL);
        ok(ret, "SnmpUtilVarBindListCpy failed\n");
        ok(dst_list.list == NULL, "SnmpUtilVarBindListCpy failed\n");
        ok(dst_list.len == 0, "SnmpUtilVarBindListCpy failed\n");
    }

    ret = SnmpUtilVarBindListCpy(&dst_list, &src_list);
    ok(ret, "SnmpUtilVarBindListCpy failed\n");
    ok(src_list.len == dst_list.len, "SnmpUtilVarBindListCpy failed\n");
    ok(src_list.list->name.idLength == dst_list.list->name.idLength,
       "SnmpUtilVarBindListCpy failed\n");
    ok(!memcmp(src_list.list->name.ids, dst_list.list->name.ids,
               dst_list.list->name.idLength * sizeof(UINT)),
       "SnmpUtilVarBindListCpy failed\n");
    ok(!memcmp(&src_list.list->value, &dst_list.list->value, sizeof(AsnAny)),
       "SnmpUtilVarBindListCpy failed\n");

    if (0) { /* crashes on XP */
    SnmpUtilVarBindListFree(NULL);
    }
    SnmpUtilVarBindListFree(&dst_list);
    ok(dst_list.list == NULL, "SnmpUtilVarBindListFree failed\n");
    ok(dst_list.len == 0, "SnmpUtilVarBindListFree failed\n");
}

START_TEST(util)
{
    InitFunctionPtrs();

    test_SnmpUtilOidToA();

    if (!pSnmpUtilAsnAnyCpy || !pSnmpUtilAsnAnyFree)
        win_skip("SnmpUtilAsnAnyCpy and/or SnmpUtilAsnAnyFree not available\n");
    else
        test_SnmpUtilAsnAnyCpyFree();

    if (!pSnmpUtilOctetsCpy || !pSnmpUtilOctetsFree)
        win_skip("SnmpUtilOctetsCpy and/or SnmpUtilOctetsFree not available\n");
    else
        test_SnmpUtilOctetsCpyFree();

    test_SnmpUtilOidCpyFree();

    if (!pSnmpUtilOctetsNCmp)
        win_skip("SnmpUtilOctetsNCmp not available\n");
    else
        test_SnmpUtilOctetsNCmp();

    if (!pSnmpUtilOctetsCmp)
        win_skip("SnmpUtilOctetsCmp not available\n");
    else
        test_SnmpUtilOctetsCmp();

    test_SnmpUtilOidCmp();
    test_SnmpUtilOidNCmp();
    test_SnmpUtilOidAppend();
    test_SnmpUtilVarBindCpyFree();
    test_SnmpUtilVarBindListCpyFree();
}
