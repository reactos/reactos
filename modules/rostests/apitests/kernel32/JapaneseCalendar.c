/*
 * Japanese Calendar Testcase
 *
 * Copyright 2019 Katayama Hirofumi MZ
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

#include "precomp.h"

#ifndef CAL_SABBREVERASTRING
    #define CAL_SABBREVERASTRING 0x00000039
#endif

START_TEST(JapaneseCalendar)
{
    CHAR szTextA[64];
    WCHAR szTextW[64];
    SYSTEMTIME st;
    DWORD langid = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    LCID lcid = MAKELCID(langid, SORT_DEFAULT);
    DWORD dwValue;
    CALTYPE type;
    int ret;
    static const WCHAR s_szSeireki19[] = {0x897F, 0x66A6, '1', '9', 0}; // L"\u897F\u66A6" L"19"
    static const WCHAR s_szHeisei[] = {0x5E73, 0x6210, 0};  // L"\u5E73\u6210"
    static const WCHAR s_szHeisei31[] = {0x5E73, 0x6210, '3', '1', 0};  // L"\u5E73\u6210" L"31"
    static const WCHAR s_szReiwa[] = {0x4EE4, 0x548C, 0};   // L"\u4EE4\u548C"
    static const WCHAR s_szOneCharHeisei1[] = {0x337B, 0};  // L"\u337B"
    static const WCHAR s_szOneCharHeisei2[] = {0x5E73, 0};  // L"\u5E73"
    static const WCHAR s_szOneCharReiwa1[] = {0x32FF, 0};   // L"\u32FF"
    static const WCHAR s_szOneCharReiwa2[] = {0x4EE4, 0};   // L"\u4EE4"
    static const WCHAR s_szOneCharReiwa3[] = {0xF9A8, 0};   // L"\uF9A8"
    static const WCHAR s_szWareki[] = {0x548C, 0x66A6, 0};  // L"\u548C\u66A6"
    static const WCHAR s_szNen[] = {0x5E74, 0};             // L"\u5E74"

    trace("lcid: 0x%08lX\n", lcid);
    trace("langid: 0x%04lX\n", langid);

    ZeroMemory(&st, sizeof(st));
    st.wYear = 2019;
    st.wMonth = 4;
    st.wDayOfWeek = 4;
    st.wDay = 4;

    /* Standard Date Formatting */
    {
        DWORD dwFlags = 0;

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        ret = GetDateFormatA(lcid, dwFlags, &st, "gyy", szTextA, ARRAYSIZE(szTextA));
        ok(ret != 0, "ret: %d\n", ret);
        ok(/* WinXP */ lstrcmpiA(szTextA, "19") == 0 ||
           /* Win10 */ lstrcmpiA(szTextA, "\x90\xBC\x97\xEF" "19") == 0, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        ret = GetDateFormatA(lcid, dwFlags, &st, "ggyy", szTextA, ARRAYSIZE(szTextA));
        ok(ret != 0, "ret: %d\n", ret);
        ok(/* WinXP */ lstrcmpiA(szTextA, "19") == 0 ||
           /* Win10 */ lstrcmpiA(szTextA, "\x90\xBC\x97\xEF" "19") == 0, "szTextA: %s\n", szTextA);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        ret = GetDateFormatW(lcid, dwFlags, &st, L"gyy", szTextW, ARRAYSIZE(szTextW));
        ok(ret != 0, "ret: %d\n", ret);
        ok(/* WinXP */ lstrcmpiW(szTextW, L"19") == 0 ||
           /* Win10 */ lstrcmpiW(szTextW, s_szSeireki19) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        ret = GetDateFormatW(lcid, dwFlags, &st, L"ggyy", szTextW, ARRAYSIZE(szTextW));
        ok(ret != 0, "ret: %d\n", ret);
        ok(/* WinXP */ lstrcmpiW(szTextW, L"19") == 0 ||
           /* Win10 */ lstrcmpiW(szTextW, s_szSeireki19) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);
    }

    /* Alternative Date Formatting (Wareki) */
    {
        DWORD dwFlags = DATE_USE_ALT_CALENDAR;

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        ret = GetDateFormatA(lcid, dwFlags, &st, "gyy", szTextA, ARRAYSIZE(szTextA));
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC" "31") == 0, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        ret = GetDateFormatA(lcid, dwFlags, &st, "ggyy", szTextA, ARRAYSIZE(szTextA));
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC" "31") == 0, "szTextA: %s\n", szTextA);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        ret = GetDateFormatW(lcid, dwFlags, &st, L"gyy", szTextW, ARRAYSIZE(szTextW));
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiW(szTextW, s_szHeisei31) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        ret = GetDateFormatW(lcid, dwFlags, &st, L"ggyy", szTextW, ARRAYSIZE(szTextW));
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiW(szTextW, s_szHeisei31) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);
    }

    /* Japanese calendar-related locale info (MBCS) */
    {
        type = CAL_ICALINTVALUE | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok_long(dwValue, 3);

        type = CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok_long(dwValue, 99);

        type = CAL_IYEAROFFSETRANGE | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok(dwValue == 1989 || dwValue == 2019, "dwValue was %ld\n", dwValue);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        type = CAL_SABBREVERASTRING;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok_int(ret, FALSE);
        ok(lstrcmpiA(szTextA, "\x7F") == 0, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        type = CAL_SCALNAME;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiA(szTextA, "\x98\x61\x97\xEF") == 0, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        type = CAL_SERASTRING;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC") == 0, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        type = CAL_SLONGDATE;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(strstr(szTextA, "\x94\x4E") != NULL, "szTextA: %s\n", szTextA);

        szTextA[0] = 0x7F;
        szTextA[1] = 0;
        type = CAL_SSHORTDATE;
        ret = GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(strstr(szTextA, "/") != NULL, "szTextA: %s\n", szTextA);
    }

    /* Japanese calendar-related locale info (Unicode) */
    {
        type = CAL_ICALINTVALUE | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok_long(dwValue, 3);

        type = CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok_long(dwValue, 99);

        type = CAL_IYEAROFFSETRANGE | CAL_RETURN_NUMBER;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(ret != 0, "ret: %d\n", ret);
        ok(dwValue == 1989 || dwValue == 2019, "dwValue was %ld\n", dwValue);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        type = CAL_SABBREVERASTRING;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(/* WinXP */ ret == 0 || /* Win10 */ ret != 0, "ret: %d\n", ret);
        ok(/* WinXP */ (szTextW[0] == 0x7F && szTextW[1] == 0) ||
           /* Win10? */ lstrcmpiW(szTextW, s_szOneCharHeisei1) == 0 ||
           /* Win10? */ lstrcmpiW(szTextW, s_szOneCharHeisei2) == 0 ||
           /* Win10? */ lstrcmpiW(szTextW, s_szOneCharReiwa1) == 0 ||
           /* Win10? */ lstrcmpiW(szTextW, s_szOneCharReiwa2) == 0 ||
           /* Win10? */ lstrcmpiW(szTextW, s_szOneCharReiwa3) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        type = CAL_SCALNAME;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(lstrcmpiW(szTextW, s_szWareki) == 0, "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        type = CAL_SERASTRING;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(wcsstr(szTextW, s_szHeisei) != NULL ||
           wcsstr(szTextW, s_szReiwa) != NULL, "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        type = CAL_SLONGDATE;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(wcsstr(szTextW, s_szNen) != NULL, "\n");

        szTextW[0] = 0x7F;
        szTextW[1] = 0;
        type = CAL_SSHORTDATE;
        ret = GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(ret != 0, "ret: %d\n", ret);
        ok(wcsstr(szTextW, L"/") != NULL, "\n");
    }
}
