/*
 * Unit tests for Windows property system
 *
 * Copyright 2006 Paul Vriens
 * Copyright 2010 Andrew Nguyen
 * Copyright 2012 Vincent Povirk
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "objbase.h"
#include "propsys.h"
#include "propvarutil.h"
#include "strsafe.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(dummy_guid, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe);
DEFINE_GUID(expect_guid, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12);
DEFINE_GUID(PKEY_WineTest, 0x7b317433, 0xdfa3, 0x4c44, 0xad, 0x3e, 0x2f, 0x80, 0x4b, 0x90, 0xdb, 0xf4);
DEFINE_GUID(DUMMY_GUID1, 0x12345678, 0x1234,0x1234, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19);

#define GUID_MEMBERS(g) {(g).Data1, (g).Data2, (g).Data3, {(g).Data4[0], (g).Data4[1], (g).Data4[2], (g).Data4[3], (g).Data4[4], (g).Data4[5], (g).Data4[6], (g).Data4[7]}}

static const char topic[] = "wine topic";
static const WCHAR topicW[] = {'w','i','n','e',' ','t','o','p','i','c',0};
static const WCHAR emptyW[] = {0};
static const WCHAR dummy_guid_str[] = L"{DEADBEEF-DEAD-BEEF-DEAD-BEEFCAFEBABE}";

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown *)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static void test_PSStringFromPropertyKey(void)
{
    static const WCHAR fillerW[] = {'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                    'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                    'X','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_fillerW[] = {'\0','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                         'X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
                                         'X','X','X','X','X','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_truncatedW[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                            '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                            '0','0','0','}',' ','\0','9','X','X','X','X','X','X','X','X','X'};
    static const WCHAR zero_truncated2W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                             '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                             '0','0','0','}',' ','\0','9','2','7','6','9','4','9','2','X','X'};
    static const WCHAR zero_truncated3W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                            '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                            '0','0','0','}',' ','\0','9','2','7','6','9','4','9','2','4','X'};
    static const WCHAR zero_truncated4W[] = {'\0','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0',
                                             '0','0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0',
                                             '0','0','0','}',' ','\0','7','X','X','X','X','X','X','X','X','X'};
    static const WCHAR truncatedW[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','9','X','X','X','X','X','X','X','X','X'};
    static const WCHAR truncated2W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','9','2','7','6','9','4','9','2','X','X'};
    static const WCHAR truncated3W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','\0','9','2','7','6','9','4','9','2','4','X'};
    static const WCHAR truncated4W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                        '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                        '0','}',' ','\0','7','X','X','X','X','X','X','X','X','X'};
    static const WCHAR expectedW[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                      '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                      '0','}',' ','4','2','9','4','9','6','7','2','9','5',0};
    static const WCHAR expected2W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','1','3','5','7','9','\0','X','X','X','X','X'};
    static const WCHAR expected3W[] = {'{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0',
                                       '0','-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0',
                                       '0','}',' ','0','\0','X','X','X','X','X','X','X','X','X'};
    PROPERTYKEY prop = {GUID_MEMBERS(GUID_NULL), ~0U};
    PROPERTYKEY prop2 = {GUID_MEMBERS(GUID_NULL), 13579};
    PROPERTYKEY prop3 = {GUID_MEMBERS(GUID_NULL), 0};
    WCHAR out[PKEYSTR_MAX];
    HRESULT ret;

    const struct
    {
        REFPROPERTYKEY pkey;
        LPWSTR psz;
        UINT cch;
        HRESULT hr_expect;
        const WCHAR *buf_expect;
        BOOL hr_broken;
        HRESULT hr2;
        BOOL buf_broken;
        const WCHAR *buf2;
    } testcases[] =
    {
        {NULL, NULL, 0, E_POINTER},
        {&prop, NULL, 0, E_POINTER},
        {&prop, NULL, PKEYSTR_MAX, E_POINTER},
        {NULL, out, 0, E_NOT_SUFFICIENT_BUFFER, fillerW},
        {NULL, out, PKEYSTR_MAX, E_NOT_SUFFICIENT_BUFFER, zero_fillerW, FALSE, 0, TRUE, fillerW},
        {&prop, out, 0, E_NOT_SUFFICIENT_BUFFER, fillerW},
        {&prop, out, GUIDSTRING_MAX, E_NOT_SUFFICIENT_BUFFER, fillerW},
        {&prop, out, GUIDSTRING_MAX + 1, E_NOT_SUFFICIENT_BUFFER, fillerW},
        {&prop, out, GUIDSTRING_MAX + 2, E_NOT_SUFFICIENT_BUFFER, zero_truncatedW, TRUE, S_OK, TRUE, truncatedW},
        {&prop, out, PKEYSTR_MAX - 2, E_NOT_SUFFICIENT_BUFFER, zero_truncated2W, TRUE, S_OK, TRUE, truncated2W},
        {&prop, out, PKEYSTR_MAX - 1, E_NOT_SUFFICIENT_BUFFER, zero_truncated3W, TRUE, S_OK, TRUE, truncated3W},
        {&prop, out, PKEYSTR_MAX, S_OK, expectedW},
        {&prop2, out, GUIDSTRING_MAX + 2, E_NOT_SUFFICIENT_BUFFER, zero_truncated4W, TRUE, S_OK, TRUE, truncated4W},
        {&prop2, out, GUIDSTRING_MAX + 6, S_OK, expected2W},
        {&prop2, out, PKEYSTR_MAX, S_OK, expected2W},
        {&prop3, out, GUIDSTRING_MAX + 1, E_NOT_SUFFICIENT_BUFFER, fillerW},
        {&prop3, out, GUIDSTRING_MAX + 2, S_OK, expected3W},
        {&prop3, out, PKEYSTR_MAX, S_OK, expected3W},
    };

    int i;

    for (i = 0; i < ARRAY_SIZE(testcases); i++)
    {
        if (testcases[i].psz)
            memcpy(testcases[i].psz, fillerW, PKEYSTR_MAX * sizeof(WCHAR));

        ret = PSStringFromPropertyKey(testcases[i].pkey,
                                      testcases[i].psz,
                                      testcases[i].cch);
        ok(ret == testcases[i].hr_expect ||
           broken(testcases[i].hr_broken && ret == testcases[i].hr2), /* Vista/Win2k8 */
           "[%d] Expected PSStringFromPropertyKey to return 0x%08lx, got 0x%08lx\n",
           i, testcases[i].hr_expect, ret);

        if (testcases[i].psz)
            ok(!memcmp(testcases[i].psz, testcases[i].buf_expect, PKEYSTR_MAX * sizeof(WCHAR)) ||
                broken(testcases[i].buf_broken &&
                       !memcmp(testcases[i].psz, testcases[i].buf2, PKEYSTR_MAX * sizeof(WCHAR))), /* Vista/Win2k8 */
               "[%d] Unexpected output contents\n", i);
    }
}

static void test_PSPropertyKeyFromString(void)
{
    static const WCHAR fmtid_clsidW[] = {'S','t','d','F','o','n','t',' ','1',0};
    static const WCHAR fmtid_truncatedW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-',0};
    static const WCHAR fmtid_nobracketsW[] = {'1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                              '1','2','3','4','-','1','2','3','4','-',
                                              '1','2','3','4','5','6','7','8','9','0','1','2',0};
    static const WCHAR fmtid_badbracketW[] = {'X','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                              '1','2','3','4','-','1','2','3','4','-',
                                              '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badcharW[] = {'{','X','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                           '1','2','3','4','-','1','2','3','4','-',
                                           '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar2W[] = {'{','1','2','3','4','5','6','7','X','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddashW[] = {'{','1','2','3','4','5','6','7','8','X','1','2','3','4','-',
                                           '1','2','3','4','-','1','2','3','4','-',
                                           '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar3W[] = {'{','1','2','3','4','5','6','7','8','-','X','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar4W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','X','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash2W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','X',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar5W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            'X','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar6W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','X','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash3W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','X','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar7W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','X','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar8W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','X','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_baddash4W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','X',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar9W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            'X','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar9_adjW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                '1','2','3','4','-','1','2','3','4','-',
                                                '1','X','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar10W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','X','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar11W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','X','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar12W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','X','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badchar13W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','X','0','1','2','}',0};
    static const WCHAR fmtid_badchar14W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','X','2','}',0};
    static const WCHAR fmtid_badbracket2W[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','X',0};
    static const WCHAR fmtid_spaceW[] = {' ','{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                         '1','2','3','4','-','1','2','3','4','-',
                                         '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_spaceendW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',0};
    static const WCHAR fmtid_spacesendW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',' ',' ',0};
    static const WCHAR fmtid_nopidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                         '1','2','3','4','-','1','2','3','4','-',
                                         '1','2','3','4','5','6','7','8','9','0','1','2','}',0};
    static const WCHAR fmtid_badpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','D','E','A','D',0};
    static const WCHAR fmtid_adjpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}','1','3','5','7','9',0};
    static const WCHAR fmtid_spacespidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',' ',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negnegnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                '1','2','3','4','-','1','2','3','4','-',
                                                '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-','-','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacepidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacenegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_negspacespidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','-',' ','-',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_pospidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','1','3','5','7','9',0};
    static const WCHAR fmtid_posnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','-','+','-','1','3','5','7','9',0};
    static const WCHAR fmtid_symbolpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','+','/','$','-','1','3','5','7','9',0};
    static const WCHAR fmtid_letterpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','A','B','C','D','1','3','5','7','9',0};
    static const WCHAR fmtid_spacepadpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','3','5','7','9',' ',' ',' ',0};
    static const WCHAR fmtid_spacemixpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1',' ','3',' ','5','7','9',' ',' ',' ',0};
    static const WCHAR fmtid_tabpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}','\t','1','3','5','7','9',0};
    static const WCHAR fmtid_hexpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','0','x','D','E','A','D',0};
    static const WCHAR fmtid_mixedpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                            '1','2','3','4','-','1','2','3','4','-',
                                            '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','A','9','B','5','C','3','D','1',0};
    static const WCHAR fmtid_overflowpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','2','3','4','5','6','7','8','9','0','1',0};
    static const WCHAR fmtid_commapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',',',',',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspacepidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                 '1','2','3','4','-','1','2','3','4','-',
                                                 '1','2','3','4','5','6','7','8','9','0','1','2','}',',',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_spacecommapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                 '1','2','3','4','-','1','2','3','4','-',
                                                 '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommaspcpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                  '1','2','3','4','-','1','2','3','4','-',
                                                  '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_spacescommaspidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                                   '1','2','3','4','-','1','2','3','4','-',
                                                   '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ',',','1','3','5','7','9',0};
    static const WCHAR fmtid_commanegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',','-','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommanegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',','-','1','3','5','7','9',0};
    static const WCHAR fmtid_commaspcnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_spccommaspcnegpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',' ',',',' ','-','1','3','5','7','9',0};
    static const WCHAR fmtid_commanegspcpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}',',','-',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_negcommapidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                               '1','2','3','4','-','1','2','3','4','-',
                                               '1','2','3','4','5','6','7','8','9','0','1','2','}','-',',','1','3','5','7','9',0};
    static const WCHAR fmtid_normalpidW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                             '1','2','3','4','-','1','2','3','4','-',
                                             '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','3','5','7','9',0};
    static const WCHAR fmtid_udigitW[] = {'{','1','2','3','4','5','6','7','8','-','1','2','3','4','-',
                                          '1','2','3','4','-','1','2','3','4','-',
                                          '1','2','3','4','5','6','7','8','9','0','1','2','}',' ','1','2','3',0x661,'5','7','9',0};
    PROPERTYKEY out_init = {GUID_MEMBERS(dummy_guid), 0xdeadbeef};
    PROPERTYKEY out;
    HRESULT ret;

    const struct
    {
        LPCWSTR pwzString;
        PROPERTYKEY *pkey;
        HRESULT hr_expect;
        PROPERTYKEY pkey_expect;
    } testcases[] =
    {
        {NULL, NULL, E_POINTER},
        {NULL, &out, E_POINTER, {GUID_MEMBERS(out_init.fmtid), out_init.pid}},
        {emptyW, NULL, E_POINTER},
        {emptyW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_clsidW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_truncatedW, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_nobracketsW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_badbracketW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_badcharW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_badchar2W, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0}},
        {fmtid_baddashW, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar3W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar4W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash2W, &out, E_INVALIDARG, { {0x12345678,0,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar5W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar6W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash3W, &out, E_INVALIDARG, { {0x12345678,0x1234,0,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar7W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar8W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0,0,0,0,0,0,0}}, 0}},
        {fmtid_baddash4W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar9W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar9_adjW, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0,0,0,0,0,0}}, 0}},
        {fmtid_badchar10W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0,0,0,0,0}}, 0}},
        {fmtid_badchar11W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0,0,0,0}}, 0}},
        {fmtid_badchar12W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0,0,0}}, 0}},
        {fmtid_badchar13W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0,0}}, 0}},
        {fmtid_badchar14W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0x90,0}}, 0}},
        {fmtid_badbracket2W, &out, E_INVALIDARG, { {0x12345678,0x1234,0x1234,{0x12,0x34,0x12,0x34,0x56,0x78,0x90,0x00}}, 0 }},
        {fmtid_spaceW, &out, E_INVALIDARG, {GUID_MEMBERS(GUID_NULL), 0 }},
        {fmtid_spaceendW, &out, E_INVALIDARG, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_spacesendW, &out, E_INVALIDARG, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_nopidW, &out, E_INVALIDARG, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_badpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_adjpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_spacespidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_negpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_negnegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_negnegnegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_negspacepidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_negspacenegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_negspacespidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_pospidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_posnegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_symbolpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_letterpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_spacepadpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_spacemixpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 1}},
        {fmtid_tabpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_hexpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_mixedpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_overflowpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 3755744309U}},
        {fmtid_commapidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_commaspidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_commaspacepidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_spacecommapidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_spccommaspcpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_spacescommaspidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_commanegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_spccommanegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_commaspcnegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_spccommaspcnegpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 4294953717U}},
        {fmtid_commanegspcpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0U}},
        {fmtid_negcommapidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 0}},
        {fmtid_normalpidW, &out, S_OK, {GUID_MEMBERS(expect_guid), 13579}},
        {fmtid_udigitW, &out, S_OK, {GUID_MEMBERS(expect_guid), 123}},
    };

    int i;

    for (i = 0; i < ARRAY_SIZE(testcases); i++)
    {
        if (testcases[i].pkey)
            *testcases[i].pkey = out_init;

        ret = PSPropertyKeyFromString(testcases[i].pwzString, testcases[i].pkey);
        ok(ret == testcases[i].hr_expect,
           "[%d] Expected PSPropertyKeyFromString to return 0x%08lx, got 0x%08lx\n",
           i, testcases[i].hr_expect, ret);

        if (testcases[i].pkey)
        {
            ok(IsEqualGUID(&testcases[i].pkey->fmtid, &testcases[i].pkey_expect.fmtid),
               "[%d] Expected GUID %s, got %s\n",
               i, wine_dbgstr_guid(&testcases[i].pkey_expect.fmtid), wine_dbgstr_guid(&testcases[i].pkey->fmtid));
            ok(testcases[i].pkey->pid == testcases[i].pkey_expect.pid,
               "[%d] Expected property ID %lu, got %lu\n",
               i, testcases[i].pkey_expect.pid, testcases[i].pkey->pid);
        }
    }
}

static void test_PSRefreshPropertySchema(void)
{
    HRESULT ret;

    ret = PSRefreshPropertySchema();
    todo_wine
    ok(ret == CO_E_NOTINITIALIZED,
       "Expected PSRefreshPropertySchema to return CO_E_NOTINITIALIZED, got 0x%08lx\n", ret);

    CoInitialize(NULL);

    ret = PSRefreshPropertySchema();
    ok(ret == S_OK,
       "Expected PSRefreshPropertySchema to return S_OK, got 0x%08lx\n", ret);

    CoUninitialize();
}

static void test_InitPropVariantFromGUIDAsString(void)
{
    PROPVARIANT propvar;
    VARIANT var;
    HRESULT hres;
    int i;

    const struct {
        REFGUID guid;
        const WCHAR *str;
    } testcases[] = {
        {&IID_NULL,             L"{00000000-0000-0000-0000-000000000000}" },
        {&dummy_guid,           dummy_guid_str },
    };

    hres = InitPropVariantFromGUIDAsString(NULL, &propvar);
    ok(hres == E_FAIL, "InitPropVariantFromGUIDAsString returned %lx\n", hres);

    if(0) {
        /* Returns strange data on Win7, crashes on older systems */
        InitVariantFromGUIDAsString(NULL, &var);

        /* Crashes on windows */
        InitPropVariantFromGUIDAsString(&IID_NULL, NULL);
        InitVariantFromGUIDAsString(&IID_NULL, NULL);
    }

    for(i=0; i < ARRAY_SIZE(testcases); i++) {
        memset(&propvar, 0, sizeof(PROPVARIANT));
        hres = InitPropVariantFromGUIDAsString(testcases[i].guid, &propvar);
        ok(hres == S_OK, "%d) InitPropVariantFromGUIDAsString returned %lx\n", i, hres);
        ok(propvar.vt == VT_LPWSTR, "%d) propvar.vt = %d\n", i, propvar.vt);
        ok(!lstrcmpW(propvar.pwszVal, testcases[i].str), "%d) propvar.pwszVal = %s\n",
                i, wine_dbgstr_w(propvar.pwszVal));
        CoTaskMemFree(propvar.pwszVal);

        memset(&var, 0, sizeof(VARIANT));
        hres = InitVariantFromGUIDAsString(testcases[i].guid, &var);
        ok(hres == S_OK, "%d) InitVariantFromGUIDAsString returned %lx\n", i, hres);
        ok(V_VT(&var) == VT_BSTR, "%d) V_VT(&var) = %d\n", i, V_VT(&var));
        ok(SysStringLen(V_BSTR(&var)) == 38, "SysStringLen returned %d\n",
                SysStringLen(V_BSTR(&var)));
        ok(!lstrcmpW(V_BSTR(&var), testcases[i].str), "%d) V_BSTR(&var) = %s\n",
                i, wine_dbgstr_w(V_BSTR(&var)));
        VariantClear(&var);
    }
}

static void test_InitPropVariantFromBuffer(void)
{
    static const char data_in[] = "test";
    PROPVARIANT propvar;
    VARIANT var;
    HRESULT hres;
    void *data_out;
    LONG size;

    hres = InitPropVariantFromBuffer(NULL, 0, &propvar);
    ok(hres == S_OK, "InitPropVariantFromBuffer returned %lx\n", hres);
    ok(propvar.vt == (VT_VECTOR|VT_UI1), "propvar.vt = %d\n", propvar.vt);
    ok(propvar.caub.cElems == 0, "cElems = %d\n", propvar.caub.cElems == 0);
    PropVariantClear(&propvar);

    hres = InitPropVariantFromBuffer(data_in, 4, &propvar);
    ok(hres == S_OK, "InitPropVariantFromBuffer returned %lx\n", hres);
    ok(propvar.vt == (VT_VECTOR|VT_UI1), "propvar.vt = %d\n", propvar.vt);
    ok(propvar.caub.cElems == 4, "cElems = %d\n", propvar.caub.cElems == 0);
    ok(!memcmp(propvar.caub.pElems, data_in, 4), "Data inside array is incorrect\n");
    PropVariantClear(&propvar);

    hres = InitVariantFromBuffer(NULL, 0, &var);
    ok(hres == S_OK, "InitVariantFromBuffer returned %lx\n", hres);
    ok(V_VT(&var) == (VT_ARRAY|VT_UI1), "V_VT(&var) = %d\n", V_VT(&var));
    size = SafeArrayGetDim(V_ARRAY(&var));
    ok(size == 1, "SafeArrayGetDim returned %ld\n", size);
    hres = SafeArrayGetLBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetLBound returned %lx\n", hres);
    ok(size == 0, "LBound = %ld\n", size);
    hres = SafeArrayGetUBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetUBound returned %lx\n", hres);
    ok(size == -1, "UBound = %ld\n", size);
    VariantClear(&var);

    hres = InitVariantFromBuffer(data_in, 4, &var);
    ok(hres == S_OK, "InitVariantFromBuffer returned %lx\n", hres);
    ok(V_VT(&var) == (VT_ARRAY|VT_UI1), "V_VT(&var) = %d\n", V_VT(&var));
    size = SafeArrayGetDim(V_ARRAY(&var));
    ok(size == 1, "SafeArrayGetDim returned %ld\n", size);
    hres = SafeArrayGetLBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetLBound returned %lx\n", hres);
    ok(size == 0, "LBound = %ld\n", size);
    hres = SafeArrayGetUBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetUBound returned %lx\n", hres);
    ok(size == 3, "UBound = %ld\n", size);
    hres = SafeArrayAccessData(V_ARRAY(&var), &data_out);
    ok(hres == S_OK, "SafeArrayAccessData failed %lx\n", hres);
    ok(!memcmp(data_in, data_out, 4), "Data inside safe array is incorrect\n");
    hres = SafeArrayUnaccessData(V_ARRAY(&var));
    ok(hres == S_OK, "SafeArrayUnaccessData failed %lx\n", hres);
    VariantClear(&var);
}

static void test_PropVariantToGUID(void)
{
    PROPVARIANT propvar;
    VARIANT var;
    GUID guid;
    HRESULT hres;

    hres = InitPropVariantFromGUIDAsString(&IID_NULL, &propvar);
    ok(hres == S_OK, "InitPropVariantFromGUIDAsString failed %lx\n", hres);

    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    PropVariantClear(&propvar);

    hres = InitPropVariantFromGUIDAsString(&dummy_guid, &propvar);
    ok(hres == S_OK, "InitPropVariantFromGUIDAsString failed %lx\n", hres);

    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    ok(propvar.vt == VT_LPWSTR, "incorrect PROPVARIANT type: %d\n", propvar.vt);
    propvar.pwszVal[1] = 'd';
    propvar.pwszVal[2] = 'E';
    propvar.pwszVal[3] = 'a';
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    propvar.pwszVal[1] = 'z';
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == E_INVALIDARG, "PropVariantToGUID returned %lx\n", hres);
    PropVariantClear(&propvar);


    hres = InitVariantFromGUIDAsString(&IID_NULL, &var);
    ok(hres == S_OK, "InitVariantFromGUIDAsString failed %lx\n", hres);

    hres = VariantToGUID(&var, &guid);
    ok(hres == S_OK, "VariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    VariantClear(&var);

    hres = InitVariantFromGUIDAsString(&dummy_guid, &var);
    ok(hres == S_OK, "InitVariantFromGUIDAsString failed %lx\n", hres);

    hres = VariantToGUID(&var, &guid);
    ok(hres == S_OK, "VariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    ok(V_VT(&var) == VT_BSTR, "incorrect VARIANT type: %d\n", V_VT(&var));
    V_BSTR(&var)[1] = 'z';
    hres = VariantToGUID(&var, &guid);
    ok(hres == E_FAIL, "VariantToGUID returned %lx\n", hres);

    V_BSTR(&var)[1] = 'd';
    propvar.vt = V_VT(&var);
    propvar.bstrVal = V_BSTR(&var);
    V_VT(&var) = VT_EMPTY;
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    PropVariantClear(&propvar);

    memset(&guid, 0, sizeof(guid));
    InitPropVariantFromCLSID(&dummy_guid, &propvar);
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %lx\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    PropVariantClear(&propvar);
}

static void test_PropVariantToStringAlloc(void)
{
    PROPVARIANT prop;
    WCHAR *str;
    HRESULT hres;

    prop.vt = VT_NULL;
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "returned %lx\n", hres);
    ok(!lstrcmpW(str, emptyW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "returned %lx\n", hres);
    ok(!lstrcmpW(str, topicW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    PropVariantClear(&prop);

    prop.vt = VT_EMPTY;
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "returned %lx\n", hres);
    ok(!lstrcmpW(str, emptyW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    prop.vt = VT_CLSID;
    prop.puuid = (CLSID *)&dummy_guid;
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "PropVariantToStringAlloc returned %#lx.\n", hres);
    ok(!wcscmp(str, dummy_guid_str), "Unexpected str %s.\n", debugstr_w(str));
    CoTaskMemFree(str);
}

static void test_PropVariantCompareEx(void)
{
    PROPVARIANT empty, null, emptyarray, i2_0, i2_2, i4_large, i4_largeneg, i4_2, str_2, str_02, str_b;
    PROPVARIANT clsid_null, clsid, clsid2, r4_0, r4_2, r8_0, r8_2;
    PROPVARIANT ui4, ui4_large;
    PROPVARIANT var1, var2;
    INT res;
    static const WCHAR str_2W[] = {'2', 0};
    static const WCHAR str_02W[] = {'0', '2', 0};
    static const WCHAR str_bW[] = {'b', 0};
    SAFEARRAY emptysafearray;
    unsigned char bytevector1[] = {1,2,3};
    unsigned char bytevector2[] = {4,5,6};

    PropVariantInit(&empty);
    PropVariantInit(&null);
    PropVariantInit(&emptyarray);
    PropVariantInit(&i2_0);
    PropVariantInit(&i2_2);
    PropVariantInit(&i4_large);
    PropVariantInit(&i4_largeneg);
    PropVariantInit(&i4_2);
    PropVariantInit(&str_2);
    PropVariantInit(&str_b);

    empty.vt = VT_EMPTY;
    null.vt = VT_NULL;
    emptyarray.vt = VT_ARRAY | VT_I4;
    emptyarray.parray = &emptysafearray;
    emptysafearray.cDims = 1;
    emptysafearray.fFeatures = FADF_FIXEDSIZE;
    emptysafearray.cbElements = 4;
    emptysafearray.cLocks = 0;
    emptysafearray.pvData = NULL;
    emptysafearray.rgsabound[0].cElements = 0;
    emptysafearray.rgsabound[0].lLbound = 0;
    i2_0.vt = VT_I2;
    i2_0.iVal = 0;
    i2_2.vt = VT_I2;
    i2_2.iVal = 2;
    i4_large.vt = VT_I4;
    i4_large.lVal = 65536;
    i4_largeneg.vt = VT_I4;
    i4_largeneg.lVal = -65536;
    i4_2.vt = VT_I4;
    i4_2.lVal = 2;
    ui4.vt = VT_UI4;
    ui4.ulVal = 2;
    ui4_large.vt = VT_UI4;
    ui4_large.ulVal = 65536;
    str_2.vt = VT_BSTR;
    str_2.bstrVal = SysAllocString(str_2W);
    str_02.vt = VT_BSTR;
    str_02.bstrVal = SysAllocString(str_02W);
    str_b.vt = VT_BSTR;
    str_b.bstrVal = SysAllocString(str_bW);
    clsid_null.vt = VT_CLSID;
    clsid_null.puuid = NULL;
    clsid.vt = VT_CLSID;
    clsid.puuid = (GUID *)&dummy_guid;
    clsid2.vt = VT_CLSID;
    clsid2.puuid = (GUID *)&GUID_NULL;
    r4_0.vt = VT_R4;
    r4_0.fltVal = 0.0f;
    r4_2.vt = VT_R4;
    r4_2.fltVal = 2.0f;
    r8_0.vt = VT_R8;
    r8_0.dblVal = 0.0;
    r8_2.vt = VT_R8;
    r8_2.dblVal = 2.0;

    res = PropVariantCompareEx(&empty, &empty, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&empty, &null, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&null, &emptyarray, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&null, &i2_0, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_0, &null, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&null, &i2_0, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_0, &null, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_2, &i2_0, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_0, &i2_2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&ui4, &ui4_large, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&ui4_large, &ui4, 0, 0);
    ok(res == 1, "res=%i\n", res);

    /* Always return -1 if second value cannot be converted to first type */
    res = PropVariantCompareEx(&i2_0, &i4_large, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_0, &i4_largeneg, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&i4_large, &i2_0, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&i4_largeneg, &i2_0, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_2, &i4_2, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_2, &str_2, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&i2_2, &str_02, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&str_2, &i2_2, 0, 0);
    todo_wine ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&str_02, &i2_2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&str_02, &str_2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&str_02, &str_b, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&str_2, &str_02, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&i4_large, &str_b, 0, 0);
    todo_wine ok(res == -5 /* ??? */, "res=%i\n", res);

    /* VT_CLSID */
    res = PropVariantCompareEx(&clsid_null, &clsid_null, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid_null, &clsid_null, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid, &clsid, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid, &clsid2, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid2, &clsid, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid_null, &clsid, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid, &clsid_null, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid_null, &clsid, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&clsid, &clsid_null, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == -1, "res=%i\n", res);

    /* VT_R4/VT_R8 */
    res = PropVariantCompareEx(&r4_0, &r8_0, 0, 0);
    todo_wine
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&r4_0, &r4_0, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&r4_0, &r4_2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&r4_2, &r4_0, 0, 0);
    ok(res == 1, "res=%i\n", res);

    res = PropVariantCompareEx(&r8_0, &r8_0, 0, 0);
    ok(res == 0, "res=%i\n", res);

    res = PropVariantCompareEx(&r8_0, &r8_2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&r8_2, &r8_0, 0, 0);
    ok(res == 1, "res=%i\n", res);

    /* VT_VECTOR | VT_UI1 */
    var1.vt = VT_VECTOR | VT_UI1;
    var1.caub.cElems = 1;
    var1.caub.pElems = bytevector1;
    var2.vt = VT_VECTOR | VT_UI1;
    var2.caub.cElems = 1;
    var2.caub.pElems = bytevector2;

    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&var2, &var1, 0, 0);
    ok(res == 1, "res=%i\n", res);

    /* Vector length mismatch */
    var1.caub.cElems = 2;
    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&var2, &var1, 0, 0);
    ok(res == 1, "res=%i\n", res);

    var1.caub.pElems = bytevector2;
    var2.caub.pElems = bytevector1;
    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == 1, "res=%i\n", res);

    var1.caub.pElems = bytevector1;
    var2.caub.pElems = bytevector2;

    var1.caub.cElems = 1;
    var2.caub.cElems = 2;
    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&var2, &var1, 0, 0);
    ok(res == 1, "res=%i\n", res);

    /* Length mismatch over same data */
    var1.caub.pElems = bytevector1;
    var2.caub.pElems = bytevector1;

    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&var2, &var1, 0, 0);
    ok(res == 1, "res=%i\n", res);

    var1.caub.cElems = 1;
    var2.caub.cElems = 1;
    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == 0, "res=%i\n", res);

    var1.caub.cElems = 0;
    res = PropVariantCompareEx(&var1, &var2, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == 1, "res=%i\n", res);
    res = PropVariantCompareEx(&var2, &var1, 0, PVCF_TREATEMPTYASGREATERTHAN);
    ok(res == -1, "res=%i\n", res);

    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == -1, "res=%i\n", res);
    res = PropVariantCompareEx(&var2, &var1, 0, 0);
    ok(res == 1, "res=%i\n", res);

    var2.caub.cElems = 0;
    res = PropVariantCompareEx(&var1, &var2, 0, 0);
    ok(res == 0, "res=%i\n", res);

    SysFreeString(str_2.bstrVal);
    SysFreeString(str_02.bstrVal);
    SysFreeString(str_b.bstrVal);
}

static void test_intconversions(void)
{
    PROPVARIANT propvar;
    SHORT sval;
    USHORT usval;
    LONG lval;
    ULONG ulval;
    LONGLONG llval;
    ULONGLONG ullval;
    HRESULT hr;

    propvar.vt = 0xdead;
    hr = PropVariantClear(&propvar);
    ok (FAILED(hr), "PropVariantClear fails on invalid vt.\n");

    propvar.vt = VT_I8;
    PropVariantClear(&propvar);

    propvar.vt = VT_I8;
    propvar.hVal.QuadPart = (ULONGLONG)1 << 63;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == (ULONGLONG)1 << 63, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    ulval = PropVariantToUInt32WithDefault(&propvar, 77);
    ok(ulval == 77, "ulval=%lu\n", ulval);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    propvar.vt = VT_UI8;
    propvar.uhVal.QuadPart = 5;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == 5, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(ullval == 5, "got wrong value %s\n", wine_dbgstr_longlong(ullval));

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(lval == 5, "got wrong value %ld\n", lval);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(ulval == 5, "got wrong value %ld\n", ulval);

    ulval = PropVariantToUInt32WithDefault(&propvar, 77);
    ok(ulval == 5, "got wrong value %lu\n", ulval);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(sval == 5, "got wrong value %d\n", sval);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(usval == 5, "got wrong value %d\n", usval);

    propvar.vt = VT_I8;
    propvar.hVal.QuadPart = -5;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == -5, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(lval == -5, "got wrong value %ld\n", lval);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    ulval = PropVariantToUInt32WithDefault(&propvar, 77);
    ok(ulval == 77, "ulval=%lu\n", ulval);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(sval == -5, "got wrong value %d\n", sval);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%lx\n", hr);

    propvar.vt = VT_UI4;
    propvar.ulVal = 6;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == 6, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_I4;
    propvar.lVal = -6;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == -6, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_UI2;
    propvar.uiVal = 7;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == 7, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_I2;
    propvar.iVal = -7;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(llval == -7, "got wrong value %s\n", wine_dbgstr_longlong(llval));
}

static void test_PropVariantToBoolean(void)
{
    static WCHAR str_0[] = {'0',0};
    static WCHAR str_1[] = {'1',0};
    static WCHAR str_7[] = {'7',0};
    static WCHAR str_n7[] = {'-','7',0};
    static WCHAR str_true[] = {'t','r','u','e',0};
    static WCHAR str_true2[] = {'#','T','R','U','E','#',0};
    static WCHAR str_true_case[] = {'t','R','U','e',0};
    static WCHAR str_false[] = {'f','a','l','s','e',0};
    static WCHAR str_false2[] = {'#','F','A','L','S','E','#',0};
    static WCHAR str_true_space[] = {'t','r','u','e',' ',0};
    static WCHAR str_yes[] = {'y','e','s',0};
    PROPVARIANT propvar;
    HRESULT hr;
    BOOL val;

    /* VT_BOOL */
    propvar.vt = VT_BOOL;
    propvar.boolVal = VARIANT_FALSE;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_BOOL;
    propvar.boolVal = 1;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_BOOL;
    propvar.boolVal = VARIANT_TRUE;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    /* VT_EMPTY */
    propvar.vt = VT_EMPTY;
    propvar.boolVal = VARIANT_TRUE;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    /* test integer conversion */
    propvar.vt = VT_I4;
    propvar.lVal = 0;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.lVal = 1;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.lVal = 67;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.lVal = -67;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    /* test string conversion */
    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_0;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_1;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_7;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_n7;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_true;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_true_case;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_true2;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_false;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_false2;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_true_space;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = str_yes;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = NULL;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    /* VT_LPSTR */
    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"#TruE#";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"#TRUE#";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"tRUe";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"#FALSE#";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"fALSe";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"1";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)"-1";
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);
}

static void test_PropVariantToStringWithDefault(void)
{
    PROPVARIANT propvar;
    static WCHAR default_value[] = {'t', 'e', 's', 't', 0};
    static WCHAR wstr_test2[] =  {'t', 'e', 's', 't', '2', 0};
    static WCHAR wstr_empty[] = {0};
    static WCHAR wstr_space[] = {' ', 0};
    static CHAR str_test2[] =  "test2";
    static CHAR str_empty[] = "";
    static CHAR str_space[] = " ";
    LPCWSTR result;

    propvar.vt = VT_EMPTY;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_BOOL;
    propvar.boolVal = VARIANT_TRUE;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_I4;
    propvar.lVal = 15;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_CLSID;
    propvar.puuid = (CLSID *)&dummy_guid;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s.\n", debugstr_w(result));

    /* VT_LPWSTR */

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = wstr_empty;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_empty, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = wstr_space;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_space, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = wstr_test2;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_test2, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* VT_LPSTR */

    propvar.vt = VT_LPSTR;
    propvar.pszVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.pszVal = str_empty;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.pszVal = str_space;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.pszVal = str_test2;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* VT_BSTR */

    propvar.vt = VT_BSTR;
    propvar.bstrVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_empty), "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocString(wstr_empty);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_empty), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.bstrVal);

    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocString(wstr_space);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_space), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.bstrVal);

    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocString(wstr_test2);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_test2), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.bstrVal);
}

static void test_PropVariantChangeType_LPWSTR(void)
{
    PROPVARIANT dest, src;
    HRESULT hr;

    PropVariantInit(&dest);

    src.vt = VT_NULL;
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.pwszVal, emptyW), "got %s\n", wine_dbgstr_w(dest.pwszVal));
    PropVariantClear(&dest);
    PropVariantClear(&src);

    src.vt = VT_LPSTR;
    src.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(src.pszVal, topic);
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.pwszVal, topicW), "got %s\n", wine_dbgstr_w(dest.pwszVal));
    PropVariantClear(&dest);
    PropVariantClear(&src);

    src.vt = VT_LPWSTR;
    src.pwszVal = CoTaskMemAlloc( (lstrlenW(topicW)+1) * sizeof(WCHAR));
    lstrcpyW(src.pwszVal, topicW);
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%lx\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.pwszVal, topicW), "got %s\n", wine_dbgstr_w(dest.pwszVal));
    PropVariantClear(&dest);
    PropVariantClear(&src);
}

static void test_InitPropVariantFromCLSID(void)
{
    PROPVARIANT propvar;
    GUID clsid;
    HRESULT hr;

    memset(&propvar, 0, sizeof(propvar));
    propvar.vt = VT_I4;
    propvar.lVal = 15;

    memset(&clsid, 0xcc, sizeof(clsid));
    hr = InitPropVariantFromCLSID(&clsid, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == VT_CLSID, "Unexpected type %d.\n", propvar.vt);
    ok(IsEqualGUID(propvar.puuid, &clsid), "Unexpected puuid value.\n");
    PropVariantClear(&propvar);
}

static void test_InitPropVariantFromStringVector(void)
{
    static const WCHAR *strs[2] = { L"abc", L"def" };
    PROPVARIANT propvar;
    HRESULT hr;

    memset(&propvar, 0, sizeof(propvar));
    propvar.vt = VT_I4;
    propvar.lVal = 15;

    hr = InitPropVariantFromStringVector(NULL, 0, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == (VT_LPWSTR|VT_VECTOR), "Unexpected type %#x.\n", propvar.vt);
    ok(!propvar.calpwstr.cElems, "Unexpected number of elements.\n");
    ok(!!propvar.calpwstr.pElems, "Unexpected vector pointer.\n");
    PropVariantClear(&propvar);

    hr = InitPropVariantFromStringVector(strs, 2, &propvar);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(propvar.vt == (VT_LPWSTR|VT_VECTOR), "Unexpected type %#x.\n", propvar.vt);
    ok(propvar.calpwstr.cElems == 2, "Unexpected number of elements.\n");
    ok(!!propvar.calpwstr.pElems, "Unexpected vector pointer.\n");
    ok(propvar.calpwstr.pElems[0] != strs[0], "Unexpected string pointer.\n");
    ok(!wcscmp(propvar.calpwstr.pElems[0], strs[0]), "Unexpected string %s.\n", debugstr_w(propvar.calpwstr.pElems[0]));
    ok(propvar.calpwstr.pElems[1] != strs[1], "Unexpected string pointer.\n");
    ok(!wcscmp(propvar.calpwstr.pElems[1], strs[1]), "Unexpected string %s.\n", debugstr_w(propvar.calpwstr.pElems[1]));
    PropVariantClear(&propvar);
}

static void test_PropVariantToDouble(void)
{
    PROPVARIANT propvar;
    double value;
    HRESULT hr;

    PropVariantInit(&propvar);
    propvar.vt = VT_R8;
    propvar.dblVal = 15.0;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 15.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I4;
    propvar.lVal = 123;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 123.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I4;
    propvar.lVal = -256;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == -256, "Unexpected value: %f\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I8;
    propvar.lVal = 65536;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 65536.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I8;
    propvar.lVal = -321;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 4294966975.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_UI4;
    propvar.ulVal = 6;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 6.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_UI8;
    propvar.uhVal.QuadPart = 8;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08lx.\n", hr);
    ok(value == 8.0, "Unexpected value: %f.\n", value);
}

static void test_PropVariantToString(void)
{
    static WCHAR stringW[] = L"Wine";
    static CHAR string[] = "Wine";
    WCHAR bufferW[256] = {0};
    PROPVARIANT propvar;
    HRESULT hr;

    PropVariantInit(&propvar);
    propvar.vt = VT_EMPTY;
    propvar.pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, 0);
    ok(hr == E_INVALIDARG, "PropVariantToString should fail: 0x%08lx.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_EMPTY;
    propvar.pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_NULL;
    propvar.pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    propvar.lVal = 22;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    todo_wine ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    todo_wine ok(!lstrcmpW(bufferW, L"22"), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = stringW;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPSTR;
    propvar.pszVal = string;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    /*  Result string will be truncated if output buffer is too small. */
    PropVariantInit(&propvar);
    propvar.vt = VT_UI4;
    propvar.lVal = 123456;
    hr = PropVariantToString(&propvar, bufferW, 4);
    todo_wine
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "PropVariantToString returned: %#lx.\n", hr);
    todo_wine
    ok(!wcscmp(bufferW, L"123"), "Unexpected string %s.\n", debugstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = stringW;
    hr = PropVariantToString(&propvar, bufferW, 4);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "PropVariantToString returned: 0x%08lx.\n", hr);
    ok(!wcscmp(bufferW, L"Win"), "Unexpected string %s.\n", debugstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPSTR;
    propvar.pszVal = string;
    hr = PropVariantToString(&propvar, bufferW, 4);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "PropVariantToString returned: 0x%08lx.\n", hr);
    ok(!wcscmp(bufferW, L"Win"), "Unexpected string %s.\n", debugstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocString(stringW);
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    SysFreeString(propvar.bstrVal);

    PropVariantInit(&propvar);
    propvar.vt = VT_CLSID;
    propvar.puuid = (CLSID *)&dummy_guid;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString returned %#lx.\n", hr);
    ok(!wcscmp(bufferW, dummy_guid_str), "Unexpected string %s.\n", debugstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
}

#define check_PropVariantToBSTR(type, member, value, expect_str)    \
do                                                                  \
{                                                                   \
    PROPVARIANT check_propvar_ = {.vt = (type), .member = (value)}; \
    HRESULT check_hr_;                                              \
    BSTR check_bstr_;                                               \
                                                                    \
    check_hr_ = PropVariantToBSTR(&check_propvar_, &check_bstr_);   \
    ok_(__FILE__, __LINE__)(check_hr_ == S_OK,                      \
            "PropVariantToBSTR returned %#lx.\n", check_hr_);       \
                                                                    \
    if (check_hr_ == S_OK)                                          \
    {                                                               \
        ok_(__FILE__, __LINE__)(!wcscmp(check_bstr_, (expect_str)), \
                "Unexpected bstr %s.\n", debugstr_w(check_bstr_));  \
        SysFreeString(check_bstr_);                                 \
    }                                                               \
} while (0)

static void test_PropVariantToBSTR(void)
{
    unsigned char test_bytes[] = {1, 20, 30, 4};
    WCHAR test_bstr[] = {'a', 0, 'b', 0, 'c'};
    PROPVARIANT propvar;
    UINT length;
    HRESULT hr;
    BSTR bstr;

    if (0) /* Crashes. */
    {
        hr = PropVariantToBSTR(&propvar, NULL);
        hr = PropVariantToBSTR(NULL, &bstr);
    }

    todo_wine
    {
    check_PropVariantToBSTR(VT_I1,     cVal,           -123,                 L"-123");
    check_PropVariantToBSTR(VT_I2,     iVal,           -456,                 L"-456");
    check_PropVariantToBSTR(VT_I4,     lVal,           -789,                 L"-789");
    check_PropVariantToBSTR(VT_I8,     hVal.QuadPart,  -101112,              L"-101112");
    check_PropVariantToBSTR(VT_UI1,    bVal,           0xcd,                 L"205");
    check_PropVariantToBSTR(VT_UI2,    uiVal,          0xdead,               L"57005");
    check_PropVariantToBSTR(VT_UI4,    ulVal,          0xdeadbeef,           L"3735928559");
    check_PropVariantToBSTR(VT_UI8,    uhVal.QuadPart, 0xdeadbeefdeadbeef,   L"16045690984833335023");
    check_PropVariantToBSTR(VT_BOOL,   boolVal,        TRUE,                 L"1");
    check_PropVariantToBSTR(VT_R4,     fltVal,         0.125f,               L"0.125");
    check_PropVariantToBSTR(VT_R8,     dblVal,         0.456,                L"0.456");
    }
    check_PropVariantToBSTR(VT_CLSID,  puuid,          (CLSID *)&dummy_guid, dummy_guid_str);
    check_PropVariantToBSTR(VT_LPSTR,  pszVal,         (char *)topic,        topicW);
    check_PropVariantToBSTR(VT_LPWSTR, pwszVal,        (WCHAR *)topicW,      topicW);

    PropVariantInit(&propvar);
    propvar.vt = VT_FILETIME;
    propvar.filetime.dwLowDateTime = 0xdead;
    propvar.filetime.dwHighDateTime = 0xbeef;
    hr = PropVariantToBSTR(&propvar, &bstr);
    todo_wine
    ok(hr == S_OK, "PropVariantToBSTR returned %#lx.\n", hr);
    if (hr == S_OK)
    {
    ok(!wcscmp(bstr, L"1601/08/31:23:29:30.651"), "Unexpected bstr %s.\n", debugstr_w(bstr));
    SysFreeString(bstr);
    }

    PropVariantInit(&propvar);
    propvar.vt = VT_DATE;
    propvar.date = 123.123f;
    hr = PropVariantToBSTR(&propvar, &bstr);
    todo_wine
    ok(hr == S_OK, "PropVariantToBSTR returned %#lx.\n", hr);
    if (hr == S_OK)
    {
    ok(!wcscmp(bstr, L"1900/05/02:02:57:07.000"), "Unexpected bstr %s.\n", debugstr_w(bstr));
    SysFreeString(bstr);
    }

    PropVariantInit(&propvar);
    propvar.vt = VT_VECTOR | VT_I1;
    propvar.caub.cElems = ARRAY_SIZE(test_bytes);
    propvar.caub.pElems = test_bytes;
    hr = PropVariantToBSTR(&propvar, &bstr);
    todo_wine
    ok(hr == S_OK, "PropVariantToBSTR returned %#lx.\n", hr);
    if (hr == S_OK)
    {
    ok(!wcscmp(bstr, L"1; 20; 30; 4"), "Unexpected bstr %s.\n", debugstr_w(bstr));
    SysFreeString(bstr);
    }

    PropVariantInit(&propvar);
    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocStringLen(test_bstr, ARRAY_SIZE(test_bstr));
    hr = PropVariantToBSTR(&propvar, &bstr);
    ok(hr == S_OK, "PropVariantToBSTR returned %#lx.\n", hr);
    length = SysStringLen(bstr);
    ok(length == wcslen(test_bstr), "Unexpected length %u.\n", length);
    ok(!wcscmp(bstr, test_bstr), "Unexpected bstr %s.", debugstr_wn(bstr, ARRAY_SIZE(test_bstr)));
    SysFreeString(bstr);
    PropVariantClear(&propvar);
}

static void test_PropVariantToBuffer(void)
{
    PROPVARIANT propvar;
    HRESULT hr;
    UINT8 data[] = {1,2,3,4,5,6,7,8,9,10};
    INT8 data_int8[] = {1,2,3,4,5,6,7,8,9,10};
    SAFEARRAY *sa;
    SAFEARRAYBOUND sabound;
    void *pdata;
    UINT8 buffer[256] = {0};

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08lx.\n", hr);
    hr = PropVariantToBuffer(&propvar, NULL, 0); /* crash when cb isn't zero */
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08lx.\n", hr);
    hr = PropVariantToBuffer(&propvar, buffer, 10);
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    ok(!memcmp(buffer, data, 10) && !buffer[10], "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08lx.\n", hr);
    buffer[0] = 99;
    hr = PropVariantToBuffer(&propvar, buffer, 11);
    ok(hr == E_FAIL, "PropVariantToBuffer returned: 0x%08lx.\n", hr);
    ok(buffer[0] == 99, "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08lx.\n", hr);
    hr = PropVariantToBuffer(&propvar, buffer, 9);
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    ok(!memcmp(buffer, data, 9) && !buffer[9], "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_ARRAY|VT_UI1;
    sabound.lLbound = 0;
    sabound.cElements = sizeof(data);
    sa = NULL;
    sa = SafeArrayCreate(VT_UI1, 1, &sabound);
    ok(sa != NULL, "SafeArrayCreate failed.\n");
    hr = SafeArrayAccessData(sa, &pdata);
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08lx.\n", hr);
    memcpy(pdata, data, sizeof(data));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08lx.\n", hr);
    propvar.parray = sa;
    buffer[0] = 99;
    hr = PropVariantToBuffer(&propvar, buffer, 11);
    todo_wine ok(hr == E_FAIL, "PropVariantToBuffer returned: 0x%08lx.\n", hr);
    ok(buffer[0] == 99, "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_ARRAY|VT_UI1;
    sabound.lLbound = 0;
    sabound.cElements = sizeof(data);
    sa = NULL;
    sa = SafeArrayCreate(VT_UI1, 1, &sabound);
    ok(sa != NULL, "SafeArrayCreate failed.\n");
    hr = SafeArrayAccessData(sa, &pdata);
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08lx.\n", hr);
    memcpy(pdata, data, sizeof(data));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08lx.\n", hr);
    propvar.parray = sa;
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data));
    todo_wine ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    todo_wine ok(!memcmp(buffer, data, 10) && !buffer[10], "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_VECTOR|VT_I1;
    propvar.caub.pElems = CoTaskMemAlloc(sizeof(data_int8));
    propvar.caub.cElems = sizeof(data_int8);
    memcpy(propvar.caub.pElems, data_int8, sizeof(data_int8));
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data_int8));
    ok(hr == E_INVALIDARG, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_ARRAY|VT_I1;
    sabound.lLbound = 0;
    sabound.cElements = sizeof(data_int8);
    sa = NULL;
    sa = SafeArrayCreate(VT_I1, 1, &sabound);
    ok(sa != NULL, "SafeArrayCreate failed.\n");
    hr = SafeArrayAccessData(sa, &pdata);
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08lx.\n", hr);
    memcpy(pdata, data_int8, sizeof(data_int8));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08lx.\n", hr);
    propvar.parray = sa;
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data_int8));
    ok(hr == E_INVALIDARG, "PropVariantToBuffer failed: 0x%08lx.\n", hr);
    PropVariantClear(&propvar);
}

static void test_inmemorystore(void)
{
    IPropertyStoreCache *propcache;
    HRESULT hr;
    PROPERTYKEY pkey;
    PROPVARIANT propvar;
    DWORD count;
    PSC_STATE state;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER,
        &IID_IPropertyStoreCache, (void**)&propcache);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    if (FAILED(hr))
    {
        win_skip("CLSID_InMemoryPropertyStore not supported\n");
        CoUninitialize();
        return;
    }

    hr = IPropertyStoreCache_GetCount(propcache, NULL);
    ok(hr == E_POINTER, "GetCount failed, hr=%lx\n", hr);

    hr = IPropertyStoreCache_GetCount(propcache, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 0, "GetCount returned %li, expected 0\n", count);

    hr = IPropertyStoreCache_Commit(propcache);
    ok(hr == S_OK, "Commit failed, hr=%lx\n", hr);

    hr = IPropertyStoreCache_Commit(propcache);
    ok(hr == S_OK, "Commit failed, hr=%lx\n", hr);

    hr = IPropertyStoreCache_GetAt(propcache, 0, &pkey);
    ok(hr == E_INVALIDARG, "GetAt failed, hr=%lx\n", hr);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    memset(&propvar, 0, sizeof(propvar));
    propvar.vt = VT_I4;
    propvar.lVal = 12345;

    if (0)
    {
        /* Crashes on Windows 7 */
        hr = IPropertyStoreCache_SetValue(propcache, NULL, &propvar);
        ok(hr == E_POINTER, "SetValue failed, hr=%lx\n", hr);

        hr = IPropertyStoreCache_SetValue(propcache, &pkey, NULL);
        ok(hr == E_POINTER, "SetValue failed, hr=%lx\n", hr);
    }

    hr = IPropertyStoreCache_SetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "SetValue failed, hr=%lx\n", hr);

    hr = IPropertyStoreCache_GetCount(propcache, &count);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(count == 1, "GetCount returned %li, expected 0\n", count);

    memset(&pkey, 0, sizeof(pkey));

    hr = IPropertyStoreCache_GetAt(propcache, 0, &pkey);
    ok(hr == S_OK, "GetAt failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&pkey.fmtid, &PKEY_WineTest), "got wrong pkey\n");
    ok(pkey.pid == 4, "got pid of %li, expected 4\n", pkey.pid);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    memset(&propvar, 0, sizeof(propvar));

    if (0)
    {
        /* Crashes on Windows 7 */
        hr = IPropertyStoreCache_GetValue(propcache, NULL, &propvar);
        ok(hr == E_POINTER, "GetValue failed, hr=%lx\n", hr);
    }

    hr = IPropertyStoreCache_GetValue(propcache, &pkey, NULL);
    ok(hr == E_POINTER, "GetValue failed, hr=%lx\n", hr);

    hr = IPropertyStoreCache_GetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.lVal == 12345, "expected 12345, got %ld\n", propvar.lVal);

    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 10;

    /* Get information for field that isn't set yet */
    propvar.vt = VT_I2;
    hr = IPropertyStoreCache_GetValue(propcache, &pkey, &propvar);
    ok(hr == S_OK, "GetValue failed, hr=%lx\n", hr);
    ok(propvar.vt == VT_EMPTY, "expected VT_EMPTY, got %d\n", propvar.vt);

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetState failed, hr=%lx\n", hr);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    propvar.vt = VT_I2;
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetValueAndState failed, hr=%lx\n", hr);
    ok(propvar.vt == VT_EMPTY, "expected VT_EMPTY, got %d\n", propvar.vt);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    /* Set state on an unset field */
    hr = IPropertyStoreCache_SetState(propcache, &pkey, PSC_NORMAL);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "SetState failed, hr=%lx\n", hr);

    /* Manipulate state on already set field */
    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 4;

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == S_OK, "GetState failed, hr=%lx\n", hr);
    ok(state == PSC_NORMAL, "expected PSC_NORMAL, got %d\n", state);

    hr = IPropertyStoreCache_SetState(propcache, &pkey, 10);
    ok(hr == S_OK, "SetState failed, hr=%lx\n", hr);

    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetState(propcache, &pkey, &state);
    ok(hr == S_OK, "GetState failed, hr=%lx\n", hr);
    ok(state == 10, "expected 10, got %d\n", state);

    propvar.vt = VT_I4;
    propvar.lVal = 12346;
    hr = IPropertyStoreCache_SetValueAndState(propcache, &pkey, &propvar, 5);
    ok(hr == S_OK, "SetValueAndState failed, hr=%lx\n", hr);

    memset(&propvar, 0, sizeof(propvar));
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == S_OK, "GetValueAndState failed, hr=%lx\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.lVal == 12346, "expected 12346, got %d\n", propvar.vt);
    ok(state == 5, "expected 5, got %d\n", state);

    /* Set new field with state */
    pkey.fmtid = PKEY_WineTest;
    pkey.pid = 8;

    propvar.vt = VT_I4;
    propvar.lVal = 12347;
    hr = IPropertyStoreCache_SetValueAndState(propcache, &pkey, &propvar, PSC_DIRTY);
    ok(hr == S_OK, "SetValueAndState failed, hr=%lx\n", hr);

    memset(&propvar, 0, sizeof(propvar));
    state = 0xdeadbeef;
    hr = IPropertyStoreCache_GetValueAndState(propcache, &pkey, &propvar, &state);
    ok(hr == S_OK, "GetValueAndState failed, hr=%lx\n", hr);
    ok(propvar.vt == VT_I4, "expected VT_I4, got %d\n", propvar.vt);
    ok(propvar.lVal == 12347, "expected 12347, got %d\n", propvar.vt);
    ok(state == PSC_DIRTY, "expected PSC_DIRTY, got %d\n", state);

    IPropertyStoreCache_Release(propcache);

    CoUninitialize();
}

static void test_persistserialized(void)
{
    IPropertyStore *propstore;
    IPersistSerializedPropStorage *serialized;
    HRESULT hr;
    SERIALIZEDPROPSTORAGE *result;
    DWORD result_size;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_InMemoryPropertyStore, NULL, CLSCTX_INPROC_SERVER,
        &IID_IPropertyStore, (void**)&propstore);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    hr = IPropertyStore_QueryInterface(propstore, &IID_IPersistSerializedPropStorage,
        (void**)&serialized);
    todo_wine ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    if (FAILED(hr))
    {
        IPropertyStore_Release(propstore);
        skip("IPersistSerializedPropStorage not supported\n");
        CoUninitialize();
        return;
    }

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, NULL, &result_size);
    ok(hr == E_POINTER, "GetPropertyStorage failed, hr=%lx\n", hr);

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, &result, NULL);
    ok(hr == E_POINTER, "GetPropertyStorage failed, hr=%lx\n", hr);

    hr = IPersistSerializedPropStorage_GetPropertyStorage(serialized, &result, &result_size);
    ok(hr == S_OK, "GetPropertyStorage failed, hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        ok(result_size == 0, "expected 0 bytes, got %li\n", result_size);

        CoTaskMemFree(result);
    }

    hr = IPersistSerializedPropStorage_SetPropertyStorage(serialized, NULL, 4);
    ok(hr == E_POINTER, "SetPropertyStorage failed, hr=%lx\n", hr);

    hr = IPersistSerializedPropStorage_SetPropertyStorage(serialized, NULL, 0);
    ok(hr == S_OK, "SetPropertyStorage failed, hr=%lx\n", hr);

    hr = IPropertyStore_GetCount(propstore, &result_size);
    ok(hr == S_OK, "GetCount failed, hr=%lx\n", hr);
    ok(result_size == 0, "expecting 0, got %ld\n", result_size);

    IPropertyStore_Release(propstore);
    IPersistSerializedPropStorage_Release(serialized);

    CoUninitialize();
}

static void test_PSCreateMemoryPropertyStore(void)
{
    IPropertyStore *propstore, *propstore1;
    IPersistSerializedPropStorage *serialized;
    IPropertyStoreCache *propstorecache;
    HRESULT hr;

    /* PSCreateMemoryPropertyStore(&IID_IPropertyStore, NULL); crashes */

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08lx.\n", hr);
    ok(propstore != NULL, "got %p.\n", propstore);
    EXPECT_REF(propstore, 1);

    hr = PSCreateMemoryPropertyStore(&IID_IPersistSerializedPropStorage, (void **)&serialized);
    todo_wine ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08lx.\n", hr);
    todo_wine ok(serialized != NULL, "got %p.\n", serialized);
    EXPECT_REF(propstore, 1);
    if(serialized)
    {
        EXPECT_REF(serialized, 1);
        IPersistSerializedPropStorage_Release(serialized);
    }

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStoreCache, (void **)&propstorecache);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08lx.\n", hr);
    ok(propstorecache != NULL, "got %p.\n", propstore);
    ok(propstorecache != (IPropertyStoreCache *)propstore, "pointer are equal: %p, %p.\n", propstorecache, propstore);
    EXPECT_REF(propstore, 1);
    EXPECT_REF(propstorecache, 1);

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore1);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08lx.\n", hr);
    ok(propstore1 != NULL, "got %p.\n", propstore);
    ok(propstore1 != propstore, "pointer are equal: %p, %p.\n", propstore1, propstore);
    EXPECT_REF(propstore, 1);
    EXPECT_REF(propstore1, 1);
    EXPECT_REF(propstorecache, 1);

    IPropertyStore_Release(propstore1);
    IPropertyStore_Release(propstore);
    IPropertyStoreCache_Release(propstorecache);
}

static void  test_propertystore(void)
{
    IPropertyStore *propstore;
    HRESULT hr;
    PROPVARIANT propvar, ret_propvar;
    PROPERTYKEY propkey;
    DWORD count = 0;

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "PSCreateMemoryPropertyStore failed: 0x%08lx.\n", hr);
    ok(propstore != NULL, "got %p.\n", propstore);

    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08lx.\n", hr);
    ok(!count, "got wrong property count: %ld, expected 0.\n", count);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    propvar.lVal = 123;
    propkey.fmtid = DUMMY_GUID1;
    propkey.pid = PID_FIRST_USABLE;
    hr = IPropertyStore_SetValue(propstore, &propkey, &propvar);
    ok(hr == S_OK, "IPropertyStore_SetValue failed: 0x%08lx.\n", hr);
    hr = IPropertyStore_Commit(propstore);
    ok(hr == S_OK, "IPropertyStore_Commit failed: 0x%08lx.\n", hr);
    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08lx.\n", hr);
    ok(count == 1, "got wrong property count: %ld, expected 1.\n", count);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = VT_I4;
    hr = IPropertyStore_GetValue(propstore, &propkey, &ret_propvar);
    ok(hr == S_OK, "IPropertyStore_GetValue failed: 0x%08lx.\n", hr);
    ok(ret_propvar.vt == VT_I4, "got wrong property type: %x.\n", ret_propvar.vt);
    ok(ret_propvar.lVal == 123, "got wrong value: %ld, expected 123.\n", ret_propvar.lVal);
    PropVariantClear(&propvar);
    PropVariantClear(&ret_propvar);

    PropVariantInit(&propvar);
    propkey.fmtid = DUMMY_GUID1;
    propkey.pid = PID_FIRST_USABLE;
    hr = IPropertyStore_SetValue(propstore, &propkey, &propvar);
    ok(hr == S_OK, "IPropertyStore_SetValue failed: 0x%08lx.\n", hr);
    hr = IPropertyStore_Commit(propstore);
    ok(hr == S_OK, "IPropertyStore_Commit failed: 0x%08lx.\n", hr);
    hr = IPropertyStore_GetCount(propstore, &count);
    ok(hr == S_OK, "IPropertyStore_GetCount failed: 0x%08lx.\n", hr);
    ok(count == 1, "got wrong property count: %ld, expected 1.\n", count);
    PropVariantInit(&ret_propvar);
    hr = IPropertyStore_GetValue(propstore, &propkey, &ret_propvar);
    ok(hr == S_OK, "IPropertyStore_GetValue failed: 0x%08lx.\n", hr);
    ok(ret_propvar.vt == VT_EMPTY, "got wrong property type: %x.\n", ret_propvar.vt);
    ok(!ret_propvar.lVal, "got wrong value: %ld, expected 0.\n", ret_propvar.lVal);
    PropVariantClear(&propvar);
    PropVariantClear(&ret_propvar);

    IPropertyStore_Release(propstore);
}

static void test_PSCreatePropertyStoreFromObject(void)
{
    IPropertyStore *propstore;
    IUnknown *unk;
    HRESULT hr;

    hr = PSCreateMemoryPropertyStore(&IID_IPropertyStore, (void **)&propstore);
    ok(hr == S_OK, "Failed to create property store, hr %#lx.\n", hr);

    hr = PSCreatePropertyStoreFromObject(NULL, STGM_READWRITE, &IID_IUnknown, (void **)&unk);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = PSCreatePropertyStoreFromObject((IUnknown *)propstore, STGM_READWRITE, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = PSCreatePropertyStoreFromObject((IUnknown *)propstore, STGM_READWRITE, &IID_IUnknown, (void **)&unk);
    todo_wine
    ok(hr == S_OK, "Failed to create wrapper, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(unk != (IUnknown *)propstore, "Unexpected object returned.\n");
        IUnknown_Release(unk);
    }

    hr = PSCreatePropertyStoreFromObject((IUnknown *)propstore, STGM_READWRITE, &IID_IPropertyStore, (void **)&unk);
    ok(hr == S_OK, "Failed to create wrapper, hr %#lx.\n", hr);
    ok(unk == (IUnknown *)propstore, "Unexpected object returned.\n");
    IUnknown_Release(unk);

    IPropertyStore_Release(propstore);
}

static void test_InitVariantFromFileTime(void)
{
    FILETIME ft = {0};
    SYSTEMTIME st;
    VARIANT var;
    HRESULT hr;
    double d;

    VariantInit(&var);
    if (0) /* crash on Windows */
    {
        InitVariantFromFileTime(&ft, NULL);
        InitVariantFromFileTime(NULL, &var);
    }

    ft.dwHighDateTime = -1;
    ft.dwLowDateTime = -1;
    V_VT(&var) = 0xdead;
    V_DATE(&var) = 42.0;
    hr = InitVariantFromFileTime(&ft, &var);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&var) == VT_EMPTY, "Unexpected VT %d\n", V_VT(&var));
    ok(V_DATE(&var) == 0.0, "got wrong value: %f, expected 0.0\n", V_DATE(&var));

    GetSystemTimeAsFileTime(&ft);
    hr = InitVariantFromFileTime(&ft, &var);
    ok(V_VT(&var) == VT_DATE, "Unexpected VT %d\n", V_VT(&var));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    FileTimeToSystemTime(&ft, &st);
    SystemTimeToVariantTime(&st, &d);
    ok(V_DATE(&var) == d, "got wrong value: %f, expected %f\n", V_DATE(&var), d);
}

static void test_VariantToStringWithDefault(void)
{
    static WCHAR default_value[] = L"test";
    VARIANT var, var2;
    PCWSTR result;
    BSTR b;

    V_VT(&var) = VT_EMPTY;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_NULL;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_BOOL;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    V_BOOL(&var) = VARIANT_TRUE;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_CY;
    V_CY(&var).int64 = 100000;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_DATE;
    V_DATE(&var) = 42.0;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_ERROR;
    V_ERROR(&var) = DISP_E_PARAMNOTFOUND;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_I4;
    V_I4(&var) = 15;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_I1;
    V_I1(&var) = 1;
    result = VariantToStringWithDefault(&var, NULL);
    ok(result == NULL, "Unexpected value %s\n", wine_dbgstr_w(result));
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* V_BSTR */

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = NULL;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result[0] == '\0', "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"");
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L" ");
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"test1");
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var);

    /* V_BSTRREF */

    V_VT(&var) = VT_BYREF | VT_BSTR;
    b = NULL;
    V_BSTRREF(&var) = &b;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result[0] == '\0', "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L"");
    V_BSTRREF(&var) = &b;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);

    V_VT(&var) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L" ");
    V_BSTRREF(&var) = &b;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);

    V_VT(&var) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L"test1");
    V_BSTRREF(&var) = &b;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);

    /* Nested V_BSTR */

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BSTR;
    V_BSTR(&var2) = NULL;
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result[0] == '\0', "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BSTR;
    V_BSTR(&var2) = SysAllocString(L"");
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var2), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var2);

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BSTR;
    V_BSTR(&var2) = SysAllocString(L" ");
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var2), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var2);

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BSTR;
    V_BSTR(&var2) = SysAllocString(L"test1");
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == V_BSTR(&var2), "Unexpected value %s\n", wine_dbgstr_w(result));
    VariantClear(&var2);

    /* Nested V_BSTRREF */

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BYREF | VT_BSTR;
    b = NULL;
    V_BSTRREF(&var2) = &b;
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result[0] == '\0', "Unexpected value %s\n", wine_dbgstr_w(result));

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L"");
    V_BSTRREF(&var2) = &b;
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L" ");
    V_BSTRREF(&var2) = &b;
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);

    V_VT(&var) = VT_BYREF | VT_VARIANT;
    V_VT(&var2) = VT_BYREF | VT_BSTR;
    b = SysAllocString(L"test1");
    V_BSTRREF(&var2) = &b;
    V_VARIANTREF(&var) = &var2;
    result = VariantToStringWithDefault(&var, default_value);
    ok(result == b, "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(b);
}

static void test_VariantToString(void)
{
    HRESULT hr;
    VARIANT v;
    WCHAR buff[64];

    buff[0] = 1;
    V_VT(&v) = VT_EMPTY;
    hr = VariantToString(&v, buff, 64);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!buff[0], "Unexpected buffer.\n");

    buff[0] = 0;
    V_VT(&v) = VT_I4;
    V_I4(&v) = 567;
    hr = VariantToString(&v, buff, 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buff, L"567"), "Unexpected buffer %s.\n", wine_dbgstr_w(buff));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(L"test1");

    buff[0] = 1;
    hr = VariantToString(&v, buff, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(!buff[0], "Unexpected buffer.\n");

    hr = VariantToString(&v, buff, 5);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    hr = VariantToString(&v, buff, 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!wcscmp(buff, L"test1"), "Unexpected string.\n");
    VariantClear(&v);
}

#define check_VariantToPropVariant(var, propvar, type, member, value, format) do                     \
{                                                                                                    \
    V_VT(&(var)) = VT_##type;                                                                        \
    V_##type(&(var)) = (value);                                                                      \
    hr = VariantToPropVariant(&(var), &(propvar));                                                   \
    ok_(__FILE__, __LINE__)(hr == S_OK, "VariantToPropVariant returned %#lx.\n", hr);                \
    ok_(__FILE__, __LINE__)((propvar).vt == VT_##type, "Unexpected propvar.vt %d.\n", (propvar).vt); \
    ok_(__FILE__, __LINE__)((propvar).member == (value),                                             \
            "Unexpected propvar."#member" "format".\n", (propvar).member);                           \
} while (0)

#define check_PropVariantToVariant(propvar, var, type, member, value, format) do                     \
{                                                                                                    \
    (propvar).vt = VT_##type;                                                                        \
    (propvar).member = (value);                                                                      \
    hr = PropVariantToVariant(&(propvar), &(var));                                                   \
    ok_(__FILE__, __LINE__)(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);                \
    ok_(__FILE__, __LINE__)(V_VT(&(var)) == VT_##type, "Unexpected vt %d.\n", V_VT(&(var)));         \
    ok_(__FILE__, __LINE__)(V_##type(&(var)) == (value),                                             \
            "Unexpected V_"#type"(&var) "format".\n", (propvar).member);                             \
} while (0)

static void test_VariantToPropVariant(void)
{
    PROPVARIANT propvar;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    PropVariantInit(&propvar);

    hr = VariantToPropVariant(NULL, &propvar);
    ok(hr == E_INVALIDARG, "VariantToPropVariant returned %#lx.\n", hr);
    hr = VariantToPropVariant(&var, NULL);
    ok(hr == E_INVALIDARG, "VariantToPropVariant returned %#lx.\n", hr);

    V_VT(&var) = 0xdead;
    hr = VariantToPropVariant(&var, &propvar);
    ok(hr == DISP_E_BADVARTYPE, "VariantToPropVariant returned %#lx.\n", hr);
    V_VT(&var) = VT_ILLEGAL;
    hr = VariantToPropVariant(&var, &propvar);
    todo_wine
    ok(hr == TYPE_E_TYPEMISMATCH, "VariantToPropVariant returned %#lx.\n", hr);
    V_VT(&var) = VT_CLSID;
    hr = VariantToPropVariant(&var, &propvar);
    todo_wine
    ok(hr == DISP_E_BADVARTYPE, "VariantToPropVariant returned %#lx.\n", hr);

    V_VT(&var) = VT_EMPTY;
    hr = VariantToPropVariant(&var, &propvar);
    ok(hr == S_OK, "VariantToPropVariant returned %#lx.\n", hr);
    ok(propvar.vt == VT_EMPTY, "Unexpected propvar.vt %d.\n", propvar.vt);

    V_VT(&var) = VT_NULL;
    hr = VariantToPropVariant(&var, &propvar);
    ok(hr == S_OK, "VariantToPropVariant returned %#lx.\n", hr);
    ok(propvar.vt == VT_NULL, "Unexpected propvar.vt %d.\n", propvar.vt);

    check_VariantToPropVariant(var, propvar, I1,  cVal,          -123,    "%c");
    check_VariantToPropVariant(var, propvar, I2,  iVal,          -456,    "%d");
    check_VariantToPropVariant(var, propvar, I4,  lVal,          -789,    "%ld");
    check_VariantToPropVariant(var, propvar, I8,  hVal.QuadPart, -101112, "%I64d");

    check_VariantToPropVariant(var, propvar, UI1, bVal,           0xcd,               "%#x");
    check_VariantToPropVariant(var, propvar, UI2, uiVal,          0xdead,             "%#x");
    check_VariantToPropVariant(var, propvar, UI4, ulVal,          0xdeadbeef,         "%#lx");
    check_VariantToPropVariant(var, propvar, UI8, uhVal.QuadPart, 0xdeadbeefdeadbeef, "%I64x");

    check_VariantToPropVariant(var, propvar, BOOL, boolVal, TRUE, "%d");

    check_VariantToPropVariant(var, propvar, R4, fltVal, 0.123f, "%f");
    check_VariantToPropVariant(var, propvar, R8, dblVal, 0.456f, "%f");

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"test");
    hr = VariantToPropVariant(&var, &propvar);
    ok(hr == S_OK, "VariantToPropVariant returned %#lx.\n", hr);
    ok(propvar.vt == VT_BSTR, "Unexpected propvar.vt %d.\n", propvar.vt);
    ok(propvar.bstrVal != V_BSTR(&var), "Got same string pointer.\n");
    ok(!wcscmp(propvar.bstrVal, V_BSTR(&var)), "Unexpected propvar.bstrVal %s.\n", debugstr_w(propvar.bstrVal));

    PropVariantClear(&propvar);
    VariantClear(&var);
}

static void test_PropVariantToVariant(void)
{
    PROPVARIANT propvar;
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    PropVariantInit(&propvar);

    hr = PropVariantToVariant(NULL, &var);
    ok(hr == E_INVALIDARG, "PropVariantToVariant returned %#lx.\n", hr);
    hr = PropVariantToVariant(&propvar, NULL);
    ok(hr == E_INVALIDARG, "PropVariantToVariant returned %#lx.\n", hr);

    propvar.vt = 0xdead;
    hr = PropVariantToVariant(&propvar, &var);
    todo_wine
    ok(hr == E_OUTOFMEMORY, "PropVariantToVariant returned %#lx.\n", hr);
    propvar.vt = VT_ILLEGAL;
    hr = PropVariantToVariant(&propvar, &var);
    todo_wine
    ok(hr == E_OUTOFMEMORY, "PropVariantToVariant returned %#lx.\n", hr);

    propvar.vt = VT_EMPTY;
    hr = PropVariantToVariant(&propvar, &var);
    ok(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_EMPTY, "Unexpected V_VT(&var) %d.\n", V_VT(&var));

    propvar.vt = VT_NULL;
    hr = PropVariantToVariant(&propvar, &var);
    ok(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_NULL, "Unexpected V_VT(&var) %d.\n", V_VT(&var));

    check_PropVariantToVariant(propvar, var, I1,  cVal,          'X',     "%c");
    check_PropVariantToVariant(propvar, var, I2,  iVal,          -456,    "%d");
    check_PropVariantToVariant(propvar, var, I4,  lVal,          -789,    "%ld");
    check_PropVariantToVariant(propvar, var, I8,  hVal.QuadPart, -101112, "%I64d");

    check_PropVariantToVariant(propvar, var, UI1, bVal,           0xcd,               "%#x");
    check_PropVariantToVariant(propvar, var, UI2, uiVal,          0xdead,             "%#x");
    check_PropVariantToVariant(propvar, var, UI4, ulVal,          0xdeadbeef,         "%#lx");
    check_PropVariantToVariant(propvar, var, UI8, uhVal.QuadPart, 0xdeadbeefdeadbeef, "%I64x");

    check_PropVariantToVariant(propvar, var, BOOL, boolVal, TRUE, "%d");

    check_PropVariantToVariant(propvar, var, R4, fltVal, 0.123f, "%f");
    check_PropVariantToVariant(propvar, var, R8, dblVal, 0.456f, "%f");

    propvar.vt = VT_BSTR;
    propvar.bstrVal = SysAllocString(L"test");
    hr = PropVariantToVariant(&propvar, &var);
    ok(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Unexpected V_VT(&var) %d.\n", V_VT(&var));
    ok(V_BSTR(&var) != propvar.bstrVal, "Got same string pointer.\n");
    ok(!wcscmp(V_BSTR(&var), propvar.bstrVal), "Unexpected V_BSTR(&var) %s.\n", debugstr_w(V_BSTR(&var)));
    PropVariantClear(&propvar);
    VariantClear(&var);

    propvar.vt = VT_CLSID;
    propvar.puuid = (GUID *)&dummy_guid;
    hr = PropVariantToVariant(&propvar, &var);
    todo_wine
    ok(hr == 39, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Unexpected V_VT(&var) %d.\n", V_VT(&var));
    ok(!wcscmp(V_BSTR(&var), dummy_guid_str), "Unexpected V_BSTR(&var) %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    propvar.vt = VT_LPSTR;
    propvar.pszVal = (char *)topic;
    hr = PropVariantToVariant(&propvar, &var);
    ok(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Unexpected V_VT(&var) %d.\n", V_VT(&var));
    ok(!wcscmp(V_BSTR(&var), topicW), "Unexpected V_BSTR(&var) %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = (WCHAR *)topicW;
    hr = PropVariantToVariant(&propvar, &var);
    ok(hr == S_OK, "PropVariantToVariant returned %#lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "Unexpected V_VT(&var) %d.\n", V_VT(&var));
    ok(V_BSTR(&var) != topicW, "Got same string pointer.\n");
    ok(!wcscmp(V_BSTR(&var), topicW), "Unexpected V_BSTR(&var) %s.\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);
}

START_TEST(propsys)
{
    test_InitPropVariantFromGUIDAsString();
    test_InitPropVariantFromBuffer();
    test_InitPropVariantFromCLSID();
    test_InitPropVariantFromStringVector();
    test_InitVariantFromFileTime();

    test_PSStringFromPropertyKey();
    test_PSPropertyKeyFromString();
    test_PSRefreshPropertySchema();
    test_PropVariantToGUID();
    test_PropVariantToStringAlloc();
    test_PropVariantCompareEx();
    test_intconversions();
    test_PropVariantChangeType_LPWSTR();
    test_PropVariantToBoolean();
    test_PropVariantToStringWithDefault();
    test_PropVariantToDouble();
    test_PropVariantToString();
    test_PropVariantToBSTR();
    test_PropVariantToBuffer();
    test_inmemorystore();
    test_persistserialized();
    test_PSCreateMemoryPropertyStore();
    test_propertystore();
    test_PSCreatePropertyStoreFromObject();
    test_VariantToStringWithDefault();
    test_VariantToString();
    test_VariantToPropVariant();
    test_PropVariantToVariant();
}
