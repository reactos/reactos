/*
 * Unit tests for Windows property system
 *
 * Copyright 2006 Paul Vriens
 * Copyright 2010 Andrew Nguyen
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

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "initguid.h"
#include "propsys.h"
#include "propvarutil.h"
#include "strsafe.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(dummy_guid, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe);
DEFINE_GUID(expect_guid, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12);

#define GUID_MEMBERS(g) {(g).Data1, (g).Data2, (g).Data3, {(g).Data4[0], (g).Data4[1], (g).Data4[2], (g).Data4[3], (g).Data4[4], (g).Data4[5], (g).Data4[6], (g).Data4[7]}}

static const char topic[] = "wine topic";
static const WCHAR topicW[] = {'w','i','n','e',' ','t','o','p','i','c',0};
static const WCHAR emptyW[] = {0};

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
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
           "[%d] Expected PSStringFromPropertyKey to return 0x%08x, got 0x%08x\n",
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
    };

    int i;

    for (i = 0; i < ARRAY_SIZE(testcases); i++)
    {
        if (testcases[i].pkey)
            *testcases[i].pkey = out_init;

        ret = PSPropertyKeyFromString(testcases[i].pwzString, testcases[i].pkey);
        ok(ret == testcases[i].hr_expect,
           "[%d] Expected PSPropertyKeyFromString to return 0x%08x, got 0x%08x\n",
           i, testcases[i].hr_expect, ret);

        if (testcases[i].pkey)
        {
            ok(IsEqualGUID(&testcases[i].pkey->fmtid, &testcases[i].pkey_expect.fmtid),
               "[%d] Expected GUID %s, got %s\n",
               i, wine_dbgstr_guid(&testcases[i].pkey_expect.fmtid), wine_dbgstr_guid(&testcases[i].pkey->fmtid));
            ok(testcases[i].pkey->pid == testcases[i].pkey_expect.pid,
               "[%d] Expected property ID %u, got %u\n",
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
       "Expected PSRefreshPropertySchema to return CO_E_NOTINITIALIZED, got 0x%08x\n", ret);

    CoInitialize(NULL);

    ret = PSRefreshPropertySchema();
    ok(ret == S_OK,
       "Expected PSRefreshPropertySchema to return S_OK, got 0x%08x\n", ret);

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
        const char *str;
    } testcases[] = {
        {&IID_NULL,             "{00000000-0000-0000-0000-000000000000}" },
        {&dummy_guid,           "{DEADBEEF-DEAD-BEEF-DEAD-BEEFCAFEBABE}" },
    };

    hres = InitPropVariantFromGUIDAsString(NULL, &propvar);
    ok(hres == E_FAIL, "InitPropVariantFromGUIDAsString returned %x\n", hres);

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
        ok(hres == S_OK, "%d) InitPropVariantFromGUIDAsString returned %x\n", i, hres);
        ok(propvar.vt == VT_LPWSTR, "%d) propvar.vt = %d\n", i, propvar.vt);
        ok(!strcmp_wa(propvar.u.pwszVal, testcases[i].str), "%d) propvar.u.pwszVal = %s\n",
                i, wine_dbgstr_w(propvar.u.pwszVal));
        CoTaskMemFree(propvar.u.pwszVal);

        memset(&var, 0, sizeof(VARIANT));
        hres = InitVariantFromGUIDAsString(testcases[i].guid, &var);
        ok(hres == S_OK, "%d) InitVariantFromGUIDAsString returned %x\n", i, hres);
        ok(V_VT(&var) == VT_BSTR, "%d) V_VT(&var) = %d\n", i, V_VT(&var));
        ok(SysStringLen(V_BSTR(&var)) == 38, "SysStringLen returned %d\n",
                SysStringLen(V_BSTR(&var)));
        ok(!strcmp_wa(V_BSTR(&var), testcases[i].str), "%d) V_BSTR(&var) = %s\n",
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
    ok(hres == S_OK, "InitPropVariantFromBuffer returned %x\n", hres);
    ok(propvar.vt == (VT_VECTOR|VT_UI1), "propvar.vt = %d\n", propvar.vt);
    ok(propvar.u.caub.cElems == 0, "cElems = %d\n", propvar.u.caub.cElems == 0);
    PropVariantClear(&propvar);

    hres = InitPropVariantFromBuffer(data_in, 4, &propvar);
    ok(hres == S_OK, "InitPropVariantFromBuffer returned %x\n", hres);
    ok(propvar.vt == (VT_VECTOR|VT_UI1), "propvar.vt = %d\n", propvar.vt);
    ok(propvar.u.caub.cElems == 4, "cElems = %d\n", propvar.u.caub.cElems == 0);
    ok(!memcmp(propvar.u.caub.pElems, data_in, 4), "Data inside array is incorrect\n");
    PropVariantClear(&propvar);

    hres = InitVariantFromBuffer(NULL, 0, &var);
    ok(hres == S_OK, "InitVariantFromBuffer returned %x\n", hres);
    ok(V_VT(&var) == (VT_ARRAY|VT_UI1), "V_VT(&var) = %d\n", V_VT(&var));
    size = SafeArrayGetDim(V_ARRAY(&var));
    ok(size == 1, "SafeArrayGetDim returned %d\n", size);
    hres = SafeArrayGetLBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetLBound returned %x\n", hres);
    ok(size == 0, "LBound = %d\n", size);
    hres = SafeArrayGetUBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetUBound returned %x\n", hres);
    ok(size == -1, "UBound = %d\n", size);
    VariantClear(&var);

    hres = InitVariantFromBuffer(data_in, 4, &var);
    ok(hres == S_OK, "InitVariantFromBuffer returned %x\n", hres);
    ok(V_VT(&var) == (VT_ARRAY|VT_UI1), "V_VT(&var) = %d\n", V_VT(&var));
    size = SafeArrayGetDim(V_ARRAY(&var));
    ok(size == 1, "SafeArrayGetDim returned %d\n", size);
    hres = SafeArrayGetLBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetLBound returned %x\n", hres);
    ok(size == 0, "LBound = %d\n", size);
    hres = SafeArrayGetUBound(V_ARRAY(&var), 1, &size);
    ok(hres == S_OK, "SafeArrayGetUBound returned %x\n", hres);
    ok(size == 3, "UBound = %d\n", size);
    hres = SafeArrayAccessData(V_ARRAY(&var), &data_out);
    ok(hres == S_OK, "SafeArrayAccessData failed %x\n", hres);
    ok(!memcmp(data_in, data_out, 4), "Data inside safe array is incorrect\n");
    hres = SafeArrayUnaccessData(V_ARRAY(&var));
    ok(hres == S_OK, "SafeArrayUnaccessData failed %x\n", hres);
    VariantClear(&var);
}

static void test_PropVariantToGUID(void)
{
    PROPVARIANT propvar;
    VARIANT var;
    GUID guid;
    HRESULT hres;

    hres = InitPropVariantFromGUIDAsString(&IID_NULL, &propvar);
    ok(hres == S_OK, "InitPropVariantFromGUIDAsString failed %x\n", hres);

    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    PropVariantClear(&propvar);

    hres = InitPropVariantFromGUIDAsString(&dummy_guid, &propvar);
    ok(hres == S_OK, "InitPropVariantFromGUIDAsString failed %x\n", hres);

    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    ok(propvar.vt == VT_LPWSTR, "incorrect PROPVARIANT type: %d\n", propvar.vt);
    propvar.u.pwszVal[1] = 'd';
    propvar.u.pwszVal[2] = 'E';
    propvar.u.pwszVal[3] = 'a';
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    propvar.u.pwszVal[1] = 'z';
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == E_INVALIDARG, "PropVariantToGUID returned %x\n", hres);
    PropVariantClear(&propvar);


    hres = InitVariantFromGUIDAsString(&IID_NULL, &var);
    ok(hres == S_OK, "InitVariantFromGUIDAsString failed %x\n", hres);

    hres = VariantToGUID(&var, &guid);
    ok(hres == S_OK, "VariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    VariantClear(&var);

    hres = InitVariantFromGUIDAsString(&dummy_guid, &var);
    ok(hres == S_OK, "InitVariantFromGUIDAsString failed %x\n", hres);

    hres = VariantToGUID(&var, &guid);
    ok(hres == S_OK, "VariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));

    ok(V_VT(&var) == VT_BSTR, "incorrect VARIANT type: %d\n", V_VT(&var));
    V_BSTR(&var)[1] = 'z';
    hres = VariantToGUID(&var, &guid);
    ok(hres == E_FAIL, "VariantToGUID returned %x\n", hres);

    V_BSTR(&var)[1] = 'd';
    propvar.vt = V_VT(&var);
    propvar.u.bstrVal = V_BSTR(&var);
    V_VT(&var) = VT_EMPTY;
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %x\n", hres);
    ok(IsEqualGUID(&dummy_guid, &guid), "incorrect GUID created: %s\n", wine_dbgstr_guid(&guid));
    PropVariantClear(&propvar);

    memset(&guid, 0, sizeof(guid));
    InitPropVariantFromCLSID(&dummy_guid, &propvar);
    hres = PropVariantToGUID(&propvar, &guid);
    ok(hres == S_OK, "PropVariantToGUID failed %x\n", hres);
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
    ok(hres == S_OK, "returned %x\n", hres);
    ok(!lstrcmpW(str, emptyW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    prop.vt = VT_LPSTR;
    prop.u.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.u.pszVal, topic);
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "returned %x\n", hres);
    ok(!lstrcmpW(str, topicW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
    PropVariantClear(&prop);

    prop.vt = VT_EMPTY;
    hres = PropVariantToStringAlloc(&prop, &str);
    ok(hres == S_OK, "returned %x\n", hres);
    ok(!lstrcmpW(str, emptyW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);
}

static void test_PropVariantCompare(void)
{
    PROPVARIANT empty, null, emptyarray, i2_0, i2_2, i4_large, i4_largeneg, i4_2, str_2, str_02, str_b;
    PROPVARIANT clsid_null, clsid, clsid2, r4_0, r4_2, r8_0, r8_2;
    INT res;
    static const WCHAR str_2W[] = {'2', 0};
    static const WCHAR str_02W[] = {'0', '2', 0};
    static const WCHAR str_bW[] = {'b', 0};
    SAFEARRAY emptysafearray;

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
    emptyarray.u.parray = &emptysafearray;
    emptysafearray.cDims = 1;
    emptysafearray.fFeatures = FADF_FIXEDSIZE;
    emptysafearray.cbElements = 4;
    emptysafearray.cLocks = 0;
    emptysafearray.pvData = NULL;
    emptysafearray.rgsabound[0].cElements = 0;
    emptysafearray.rgsabound[0].lLbound = 0;
    i2_0.vt = VT_I2;
    i2_0.u.iVal = 0;
    i2_2.vt = VT_I2;
    i2_2.u.iVal = 2;
    i4_large.vt = VT_I4;
    i4_large.u.lVal = 65536;
    i4_largeneg.vt = VT_I4;
    i4_largeneg.u.lVal = -65536;
    i4_2.vt = VT_I4;
    i4_2.u.lVal = 2;
    str_2.vt = VT_BSTR;
    str_2.u.bstrVal = SysAllocString(str_2W);
    str_02.vt = VT_BSTR;
    str_02.u.bstrVal = SysAllocString(str_02W);
    str_b.vt = VT_BSTR;
    str_b.u.bstrVal = SysAllocString(str_bW);
    clsid_null.vt = VT_CLSID;
    clsid_null.u.puuid = NULL;
    clsid.vt = VT_CLSID;
    clsid.u.puuid = (GUID *)&dummy_guid;
    clsid2.vt = VT_CLSID;
    clsid2.u.puuid = (GUID *)&GUID_NULL;
    r4_0.vt = VT_R4;
    r4_0.u.fltVal = 0.0f;
    r4_2.vt = VT_R4;
    r4_2.u.fltVal = 2.0f;
    r8_0.vt = VT_R8;
    r8_0.u.dblVal = 0.0;
    r8_2.vt = VT_R8;
    r8_2.u.dblVal = 2.0;

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

    SysFreeString(str_2.u.bstrVal);
    SysFreeString(str_02.u.bstrVal);
    SysFreeString(str_b.u.bstrVal);
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
    propvar.u.hVal.QuadPart = (ULONGLONG)1 << 63;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == (ULONGLONG)1 << 63, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    propvar.vt = VT_UI8;
    propvar.u.uhVal.QuadPart = 5;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == 5, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(ullval == 5, "got wrong value %s\n", wine_dbgstr_longlong(ullval));

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(lval == 5, "got wrong value %d\n", lval);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(ulval == 5, "got wrong value %d\n", ulval);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(sval == 5, "got wrong value %d\n", sval);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(usval == 5, "got wrong value %d\n", usval);

    propvar.vt = VT_I8;
    propvar.u.hVal.QuadPart = -5;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == -5, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    hr = PropVariantToUInt64(&propvar, &ullval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToInt32(&propvar, &lval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(lval == -5, "got wrong value %d\n", lval);

    hr = PropVariantToUInt32(&propvar, &ulval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    hr = PropVariantToInt16(&propvar, &sval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(sval == -5, "got wrong value %d\n", sval);

    hr = PropVariantToUInt16(&propvar, &usval);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "hr=%x\n", hr);

    propvar.vt = VT_UI4;
    propvar.u.ulVal = 6;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == 6, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_I4;
    propvar.u.lVal = -6;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == -6, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_UI2;
    propvar.u.uiVal = 7;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(llval == 7, "got wrong value %s\n", wine_dbgstr_longlong(llval));

    propvar.vt = VT_I2;
    propvar.u.iVal = -7;

    hr = PropVariantToInt64(&propvar, &llval);
    ok(hr == S_OK, "hr=%x\n", hr);
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
    propvar.u.boolVal = VARIANT_FALSE;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_BOOL;
    propvar.u.boolVal = 1;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_BOOL;
    propvar.u.boolVal = VARIANT_TRUE;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    /* VT_EMPTY */
    propvar.vt = VT_EMPTY;
    propvar.u.boolVal = VARIANT_TRUE;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    /* test integer conversion */
    propvar.vt = VT_I4;
    propvar.u.lVal = 0;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.u.lVal = 1;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.u.lVal = 67;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_I4;
    propvar.u.lVal = -67;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    /* test string conversion */
    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_0;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_1;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_7;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_n7;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_true;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_true_case;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_true2;
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_false;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_false2;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_true_space;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = str_yes;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = NULL;
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    /* VT_LPSTR */
    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"#TruE#";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == DISP_E_TYPEMISMATCH, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"#TRUE#";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"tRUe";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"#FALSE#";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"fALSe";
    val = TRUE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == FALSE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"1";
    val = FALSE;
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(val == TRUE, "Unexpected value %d\n", val);

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = (char *)"-1";
    hr = PropVariantToBoolean(&propvar, &val);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
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
    propvar.u.boolVal = VARIANT_TRUE;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_I4;
    propvar.u.lVal = 15;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* VT_LPWSTR */

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = wstr_empty;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_empty, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = wstr_space;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_space, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPWSTR;
    propvar.u.pwszVal = wstr_test2;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == wstr_test2, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* VT_LPSTR */

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = str_empty;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = str_space;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_LPSTR;
    propvar.u.pszVal = str_test2;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(result == default_value, "Unexpected value %s\n", wine_dbgstr_w(result));

    /* VT_BSTR */

    propvar.vt = VT_BSTR;
    propvar.u.bstrVal = NULL;
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_empty), "Unexpected value %s\n", wine_dbgstr_w(result));

    propvar.vt = VT_BSTR;
    propvar.u.bstrVal = SysAllocString(wstr_empty);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_empty), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.u.bstrVal);

    propvar.vt = VT_BSTR;
    propvar.u.bstrVal = SysAllocString(wstr_space);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_space), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.u.bstrVal);

    propvar.vt = VT_BSTR;
    propvar.u.bstrVal = SysAllocString(wstr_test2);
    result = PropVariantToStringWithDefault(&propvar, default_value);
    ok(!lstrcmpW(result, wstr_test2), "Unexpected value %s\n", wine_dbgstr_w(result));
    SysFreeString(propvar.u.bstrVal);
}

static void test_PropVariantChangeType_LPWSTR(void)
{
    PROPVARIANT dest, src;
    HRESULT hr;

    PropVariantInit(&dest);

    src.vt = VT_NULL;
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.u.pwszVal, emptyW), "got %s\n", wine_dbgstr_w(dest.u.pwszVal));
    PropVariantClear(&dest);
    PropVariantClear(&src);

    src.vt = VT_LPSTR;
    src.u.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(src.u.pszVal, topic);
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.u.pwszVal, topicW), "got %s\n", wine_dbgstr_w(dest.u.pwszVal));
    PropVariantClear(&dest);
    PropVariantClear(&src);

    src.vt = VT_LPWSTR;
    src.u.pwszVal = CoTaskMemAlloc( (lstrlenW(topicW)+1) * sizeof(WCHAR));
    lstrcpyW(src.u.pwszVal, topicW);
    hr = PropVariantChangeType(&dest, &src, 0, VT_LPWSTR);
    ok(hr == S_OK, "hr=%x\n", hr);
    ok(dest.vt == VT_LPWSTR, "got %d\n", dest.vt);
    ok(!lstrcmpW(dest.u.pwszVal, topicW), "got %s\n", wine_dbgstr_w(dest.u.pwszVal));
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
    propvar.u.lVal = 15;

    memset(&clsid, 0xcc, sizeof(clsid));
    hr = InitPropVariantFromCLSID(&clsid, &propvar);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(propvar.vt == VT_CLSID, "Unexpected type %d.\n", propvar.vt);
    ok(IsEqualGUID(propvar.u.puuid, &clsid), "Unexpected puuid value.\n");
    PropVariantClear(&propvar);
}

static void test_PropVariantToDouble(void)
{
    PROPVARIANT propvar;
    double value;
    HRESULT hr;

    PropVariantInit(&propvar);
    propvar.vt = VT_R8;
    propvar.u.dblVal = 15.0;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 15.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I4;
    propvar.u.lVal = 123;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 123.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I4;
    propvar.u.lVal = -256;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == -256, "Unexpected value: %f\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I8;
    propvar.u.lVal = 65536;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 65536.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_I8;
    propvar.u.lVal = -321;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 4294966975.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_UI4;
    propvar.u.ulVal = 6;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 6.0, "Unexpected value: %f.\n", value);

    PropVariantClear(&propvar);
    propvar.vt = VT_UI8;
    propvar.u.uhVal.QuadPart = 8;
    hr = PropVariantToDouble(&propvar, &value);
    ok(hr == S_OK, "PropVariantToDouble failed: 0x%08x.\n", hr);
    ok(value == 8.0, "Unexpected value: %f.\n", value);
}

static void test_PropVariantToString(void)
{
    PROPVARIANT propvar;
    static CHAR string[] = "Wine";
    static WCHAR stringW[] = {'W','i','n','e',0};
    WCHAR bufferW[256] = {0};
    HRESULT hr;

    PropVariantInit(&propvar);
    propvar.vt = VT_EMPTY;
    U(propvar).pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, 0);
    ok(hr == E_INVALIDARG, "PropVariantToString should fail: 0x%08x.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_EMPTY;
    U(propvar).pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_NULL;
    U(propvar).pwszVal = stringW;
    bufferW[0] = 65;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    U(propvar).lVal = 22;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    todo_wine ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    todo_wine ok(!strcmp_wa(bufferW, "22"), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_LPWSTR;
    U(propvar).pwszVal = stringW;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPSTR;
    U(propvar).pszVal = string;
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPWSTR;
    U(propvar).pwszVal = stringW;
    hr = PropVariantToString(&propvar, bufferW, 4);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "PropVariantToString returned: 0x%08x.\n", hr);
    ok(!memcmp(bufferW, stringW, 4), "got wrong string.\n");
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_LPSTR;
    U(propvar).pszVal = string;
    hr = PropVariantToString(&propvar, bufferW, 4);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "PropVariantToString returned: 0x%08x.\n", hr);
    ok(!memcmp(bufferW, stringW, 4), "got wrong string.\n");
    memset(bufferW, 0, sizeof(bufferW));

    PropVariantInit(&propvar);
    propvar.vt = VT_BSTR;
    propvar.u.bstrVal = SysAllocString(stringW);
    hr = PropVariantToString(&propvar, bufferW, ARRAY_SIZE(bufferW));
    ok(hr == S_OK, "PropVariantToString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));
    SysFreeString(propvar.u.bstrVal);
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
    UINT8 buffer[256];

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08x.\n", hr);
    hr = PropVariantToBuffer(&propvar, NULL, 0); /* crash when cb isn't zero */
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08x.\n", hr);
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08x.\n", hr);
    hr = PropVariantToBuffer(&propvar, buffer, 10);
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08x.\n", hr);
    ok(!memcmp(buffer, data, 10) && !buffer[10], "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08x.\n", hr);
    buffer[0] = 99;
    hr = PropVariantToBuffer(&propvar, buffer, 11);
    ok(hr == E_FAIL, "PropVariantToBuffer returned: 0x%08x.\n", hr);
    ok(buffer[0] == 99, "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    hr = InitPropVariantFromBuffer(data, 10, &propvar);
    ok(hr == S_OK, "InitPropVariantFromBuffer failed 0x%08x.\n", hr);
    hr = PropVariantToBuffer(&propvar, buffer, 9);
    ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08x.\n", hr);
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
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08x.\n", hr);
    memcpy(pdata, data, sizeof(data));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08x.\n", hr);
    U(propvar).parray = sa;
    buffer[0] = 99;
    hr = PropVariantToBuffer(&propvar, buffer, 11);
    todo_wine ok(hr == E_FAIL, "PropVariantToBuffer returned: 0x%08x.\n", hr);
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
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08x.\n", hr);
    memcpy(pdata, data, sizeof(data));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08x.\n", hr);
    U(propvar).parray = sa;
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data));
    todo_wine ok(hr == S_OK, "PropVariantToBuffer failed: 0x%08x.\n", hr);
    todo_wine ok(!memcmp(buffer, data, 10) && !buffer[10], "got wrong buffer.\n");
    memset(buffer, 0, sizeof(buffer));
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_VECTOR|VT_I1;
    U(propvar).caub.pElems = CoTaskMemAlloc(sizeof(data_int8));
    U(propvar).caub.cElems = sizeof(data_int8);
    memcpy(U(propvar).caub.pElems, data_int8, sizeof(data_int8));
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data_int8));
    ok(hr == E_INVALIDARG, "PropVariantToBuffer failed: 0x%08x.\n", hr);
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_ARRAY|VT_I1;
    sabound.lLbound = 0;
    sabound.cElements = sizeof(data_int8);
    sa = NULL;
    sa = SafeArrayCreate(VT_I1, 1, &sabound);
    ok(sa != NULL, "SafeArrayCreate failed.\n");
    hr = SafeArrayAccessData(sa, &pdata);
    ok(hr == S_OK, "SafeArrayAccessData failed: 0x%08x.\n", hr);
    memcpy(pdata, data_int8, sizeof(data_int8));
    hr = SafeArrayUnaccessData(sa);
    ok(hr == S_OK, "SafeArrayUnaccessData failed: 0x%08x.\n", hr);
    U(propvar).parray = sa;
    hr = PropVariantToBuffer(&propvar, buffer, sizeof(data_int8));
    ok(hr == E_INVALIDARG, "PropVariantToBuffer failed: 0x%08x.\n", hr);
    PropVariantClear(&propvar);
}

START_TEST(propsys)
{
    test_PSStringFromPropertyKey();
    test_PSPropertyKeyFromString();
    test_PSRefreshPropertySchema();
    test_InitPropVariantFromGUIDAsString();
    test_InitPropVariantFromBuffer();
    test_PropVariantToGUID();
    test_PropVariantToStringAlloc();
    test_PropVariantCompare();
    test_intconversions();
    test_PropVariantChangeType_LPWSTR();
    test_PropVariantToBoolean();
    test_PropVariantToStringWithDefault();
    test_InitPropVariantFromCLSID();
    test_PropVariantToDouble();
    test_PropVariantToString();
    test_PropVariantToBuffer();
}
