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
    static const WCHAR s_szSeireki[] = {0x897F, 0x66A6, 0}; // L"\u897F\u66A6"
    static const WCHAR s_szHeisei[] = {0x5E73, 0x6210, 0};  // L"\u5E73\u6210"
    static const WCHAR s_szOneCharHeisei[] = {0x5E73, 0};   // L"\u5E73"
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
        GetDateFormatA(lcid, dwFlags, &st, "gg", szTextA, ARRAYSIZE(szTextA));
        ok(/* WinXP */ szTextA[0] == 0 ||
           /* Win10 */ lstrcmpiA(szTextA, "\x90\xBC\x97\xEF") == 0, "szTextA: %s\n", szTextA);

        GetDateFormatA(lcid, dwFlags, &st, "g", szTextA, ARRAYSIZE(szTextA));
        ok(/* WinXP */ szTextA[0] == 0 ||
           /* Win10 */ lstrcmpiA(szTextA, "\x90\xBC\x97\xEF") == 0, "szTextA: %s\n", szTextA);

        GetDateFormatW(lcid, dwFlags, &st, L"gg", szTextW, ARRAYSIZE(szTextW));
        ok(/* WinXP */ szTextW[0] == 0 ||
           /* Win10 */ lstrcmpiW(szTextW, s_szSeireki) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        GetDateFormatW(lcid, dwFlags, &st, L"g", szTextW, ARRAYSIZE(szTextW));
        ok(/* WinXP */ szTextW[0] == 0 ||
           /* Win10 */ lstrcmpiW(szTextW, s_szSeireki) == 0,
           "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);
    }

    /* Alternative Date Formatting (Wareki) */
    {
        DWORD dwFlags = DATE_USE_ALT_CALENDAR;
        GetDateFormatA(lcid, dwFlags, &st, "gg", szTextA, ARRAYSIZE(szTextA));
        ok_int(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC"), 0);

        GetDateFormatA(lcid, dwFlags, &st, "g", szTextA, ARRAYSIZE(szTextA));
        ok_int(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC"), 0);

        GetDateFormatW(lcid, dwFlags, &st, L"gg", szTextW, ARRAYSIZE(szTextW));
        ok_int(lstrcmpiW(szTextW, s_szHeisei), 0);

        GetDateFormatW(lcid, dwFlags, &st, L"g", szTextW, ARRAYSIZE(szTextW));
        ok_int(lstrcmpiW(szTextW, s_szHeisei), 0);
    }

    /* Japanese calendar-related locale info (MBCS) */
    {
        type = CAL_ICALINTVALUE | CAL_RETURN_NUMBER;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok_long(dwValue, 3);

        type = CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok_long(dwValue, 99);

        type = CAL_IYEAROFFSETRANGE | CAL_RETURN_NUMBER;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(dwValue == 1989 || dwValue == 2019, "dwValue was %ld\n", dwValue);

        type = CAL_SABBREVERASTRING;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC") == 0, "szTextA: %s\n", szTextA);

        type = CAL_SCALNAME;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(lstrcmpiA(szTextA, "\x98\x61\x97\xEF") == 0, "szTextA: %s\n", szTextA);

        type = CAL_SERASTRING;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(lstrcmpiA(szTextA, "\x95\xBD\x90\xAC") == 0, "szTextA: %s\n", szTextA);

        type = CAL_SLONGDATE;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(strstr(szTextA, "\x94\x4E") != NULL, "szTextA: %s\n", szTextA);

        type = CAL_SSHORTDATE;
        GetCalendarInfoA(lcid, CAL_JAPAN, type, szTextA, ARRAYSIZE(szTextA), NULL);
        ok(strstr(szTextA, "/") != NULL, "szTextA: %s\n", szTextA);
    }

    /* Japanese calendar-related locale info (Unicode) */
    {
        type = CAL_ICALINTVALUE | CAL_RETURN_NUMBER;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok_long(dwValue, 3);

        type = CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok_long(dwValue, 99);

        type = CAL_IYEAROFFSETRANGE | CAL_RETURN_NUMBER;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, NULL, 0, &dwValue);
        ok(dwValue == 1989 || dwValue == 2019, "dwValue was %ld\n", dwValue);

        type = CAL_SABBREVERASTRING;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(/* WinXP */ lstrcmpiW(szTextW, s_szHeisei) == 0 ||
           /* Win10 */ lstrcmpiW(szTextW, s_szOneCharHeisei) == 0, "\n");

        type = CAL_SCALNAME;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(lstrcmpiW(szTextW, s_szWareki) == 0, "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        type = CAL_SERASTRING;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(wcsstr(szTextW, s_szHeisei) != NULL, "szTextW: %04X %04X %04X\n", szTextW[0], szTextW[1], szTextW[2]);

        type = CAL_SLONGDATE;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(wcsstr(szTextW, s_szNen) != NULL, "\n");

        type = CAL_SSHORTDATE;
        GetCalendarInfoW(lcid, CAL_JAPAN, type, szTextW, ARRAYSIZE(szTextW), NULL);
        ok(wcsstr(szTextW, L"/") != NULL, "\n");
    }
}
