/*
 * Unit test suite for locale functions.
 *
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <locale.h>
#include <process.h>

#include "wine/test.h"
#include "winnls.h"

static BOOL (__cdecl *p__crtGetStringTypeW)(DWORD, DWORD, const wchar_t*, int, WORD*);
static int (__cdecl *pmemcpy_s)(void *, size_t, void*, size_t);
static int (__cdecl *p___mb_cur_max_func)(void);
static int *(__cdecl *p__p___mb_cur_max)(void);
static _locale_t(__cdecl *p_create_locale)(int, const char*);
static void(__cdecl *p_free_locale)(_locale_t);
static int (__cdecl *p_wcsicmp_l)(const wchar_t*, const wchar_t*, _locale_t);
void* __cdecl _Gettnames(void);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__crtGetStringTypeW = (void*)GetProcAddress(hmod, "__crtGetStringTypeW");
    pmemcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
    p___mb_cur_max_func = (void*)GetProcAddress(hmod, "___mb_cur_max_func");
    p__p___mb_cur_max = (void*)GetProcAddress(hmod, "__p___mb_cur_max");
    p_create_locale = (void*)GetProcAddress(hmod, "_create_locale");
    p_free_locale = (void*)GetProcAddress(hmod, "_free_locale");
    p_wcsicmp_l = (void*)GetProcAddress(hmod, "_wcsicmp_l");
}

static void test_setlocale(void)
{
    static const char lc_all[] = "LC_COLLATE=C;LC_CTYPE=C;"
        "LC_MONETARY=Greek_Greece.1253;LC_NUMERIC=Polish_Poland.1250;LC_TIME=C";

    char *ret, buf[100];
    char *ptr;
    int len;

    ret = setlocale(20, "C");
    ok(ret == NULL, "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "C");
    ok(!strcmp(ret, "C"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, NULL);
    ok(!strcmp(ret, "C"), "ret = %s\n", ret);

    if(!setlocale(LC_NUMERIC, "Polish")
            || !setlocale(LC_NUMERIC, "Greek")
            || !setlocale(LC_NUMERIC, "German")
            || !setlocale(LC_NUMERIC, "English")) {
        win_skip("System with limited locales\n");
        return;
    }

    ret = setlocale(LC_NUMERIC, "Polish");
    ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_MONETARY, "Greek");
    ok(!strcmp(ret, "Greek_Greece.1253"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, NULL);
    ok(!strcmp(ret, lc_all), "ret = %s\n", ret);

    strcpy(buf, ret);
    ret = setlocale(LC_ALL, buf);
    ok(!strcmp(ret, lc_all), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "German");
    ok(!strcmp(ret, "German_Germany.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "american");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "american english");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "american-english");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "australian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Australia.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "belgian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Dutch_Belgium.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "canadian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Canada.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "chinese");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")
        || !strcmp(ret, "Chinese (Simplified)_China.936")
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "chinese-simplified");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")
        || !strcmp(ret, "Chinese (Simplified)_China.936")
        || broken(!strcmp(ret, "Chinese_People's Republic of China.936"))
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "chinese-traditional");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Traditional)_Taiwan.950")
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "chs");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")
        || !strcmp(ret, "Chinese (Simplified)_China.936")
        || broken(!strcmp(ret, "Chinese_People's Republic of China.936")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "cht");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Traditional)_Taiwan.950")
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "Chinese_China.936");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
    {
        trace("Chinese_China.936=%s\n", ret);
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936") /* Vista - Win7 */
        || !strcmp(ret, "Chinese (Simplified)_China.936") /* Win8 - Win10 */
        || broken(!strcmp(ret, "Chinese_People's Republic of China.936")), "ret = %s\n", ret);
    }

    ret = setlocale(LC_ALL, "csy");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Czech_Czech Republic.1250")
        || !strcmp(ret, "Czech_Czechia.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "czech");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Czech_Czech Republic.1250")
        || !strcmp(ret, "Czech_Czechia.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "dan");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Danish_Denmark.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "danish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Danish_Denmark.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "dea");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Austria.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "des");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Switzerland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "deu");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Germany.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "dutch");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Dutch_Netherlands.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "dutch-belgian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Dutch_Belgium.1252")
        || broken(!strcmp(ret, "Dutch_Netherlands.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "ena");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Australia.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "ell");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Greek_Greece.1253"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "enc");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Canada.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "eng");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United Kingdom.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "enu");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "enz");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_New Zealand.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-american");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-aus");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Australia.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-can");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_Canada.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-nz");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_New Zealand.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-uk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United Kingdom.1252")
        || broken(!strcmp(ret, "English_United States.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-us");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "english-usa");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "esm");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish_Mexico.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "esn");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish_Spain.1252")
        || broken(!strcmp(ret, "Spanish - Modern Sort_Spain.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "esp");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish_Spain.1252")
        || broken(!strcmp(ret, "Spanish - Traditional Sort_Spain.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "fin");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Finnish_Finland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "finnish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Finnish_Finland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "fra");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_France.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "frb");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Belgium.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "frc");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Canada.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "french");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_France.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "french-belgian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Belgium.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "french-canadian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Canada.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "french-swiss");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Switzerland.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "frs");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "French_Switzerland.1252")
        || broken(!strcmp(ret, "French_France.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "german");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Germany.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "german-austrian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Austria.1252")
        || broken(!strcmp(ret, "German_Germany.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "german-swiss");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Switzerland.1252")
        || broken(!strcmp(ret, "German_Germany.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "greek");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Greek_Greece.1253"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "hun");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Hungarian_Hungary.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "hungarian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Hungarian_Hungary.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "icelandic");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Icelandic_Iceland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "isl");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Icelandic_Iceland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "ita");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Italian_Italy.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "italian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Italian_Italy.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "italian-swiss");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Italian_Switzerland.1252")
        || broken(!strcmp(ret, "Italian_Italy.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "its");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Italian_Switzerland.1252")
        || broken(!strcmp(ret, "Italian_Italy.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "japanese");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Japanese_Japan.932"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "jpn");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Japanese_Japan.932"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "korean");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Korean_Korea.949"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "korean");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Korean_Korea.949"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "nlb");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Dutch_Belgium.1252")
        || broken(!strcmp(ret, "Dutch_Netherlands.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "nld");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Dutch_Netherlands.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "non");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp( ret, "Norwegian-Nynorsk_Norway.1252") /* XP - Win10 */
        || !strcmp(ret, "Norwegian (Nynorsk)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252"))
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), /* WinME */
           "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "nor");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252") /* XP - Win8 */
        || !strcmp(ret, "Norwegian Bokm\xe5l_Norway.1252") /* Win10 */
        || !strcmp(ret, "Norwegian (Bokmal)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), /* WinME */
           "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "norwegian-bokmal");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252") /* XP - Win8 */
        || !strcmp(ret, "Norwegian Bokm\xe5l_Norway.1252") /* Win10 */
        || !strcmp(ret, "Norwegian (Bokmal)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), /* WinME */
           "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "norwegian-nynorsk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian-Nynorsk_Norway.1252") /* Vista - Win10 */
        || !strcmp(ret, "Norwegian (Nynorsk)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252")) /* WinME */
        || broken(!strcmp(ret, "Norwegian (Bokmal)_Norway.1252"))
        || broken(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252")) /* XP & 2003 */,
           "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "plk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "polish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "portuguese");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Portuguese_Brazil.1252")
        || broken(!strcmp(ret, "Portuguese_Portugal.1252")) /* NT4 */, "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "portuguese-brazil");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Portuguese_Brazil.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "ptb");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Portuguese_Brazil.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "ptg");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Portuguese_Portugal.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "rus");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Russian_Russia.1251"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "russian");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Russian_Russia.1251"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "sky");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Slovak_Slovakia.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "slovak");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Slovak_Slovakia.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "spanish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish_Spain.1252")
        || broken(!strcmp(ret, "Spanish - Traditional Sort_Spain.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "spanish-mexican");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish_Mexico.1252")
        || broken(!strcmp(ret, "Spanish_Spain.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "spanish-modern");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Spanish - Modern Sort_Spain.1252")
           || !strcmp(ret, "Spanish_Spain.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "sve");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Swedish_Sweden.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "swedish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Swedish_Sweden.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "swiss");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "German_Switzerland.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "trk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Turkish_Turkey.1254")
        || !strcmp(ret, "Turkish_T\xfcrkiye.1254"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "turkish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Turkish_Turkey.1254")
        || !strcmp(ret, "Turkish_T\xfcrkiye.1254"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "uk");
    ok(ret != NULL, "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United Kingdom.1252")
        || broken(!strcmp(ret, "Ukrainian_Ukraine.1251")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "us");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "usa");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "English_United States.ACP");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret) {
        strcpy(buf, "English_United States.");
        GetLocaleInfoA(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                LOCALE_IDEFAULTANSICODEPAGE, buf+strlen(buf), 80);
        ok(!strcmp(ret, buf), "ret = %s, expected %s\n", ret, buf);
    }

    ret = setlocale(LC_ALL, "English_United States.OCP");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret) {
        strcpy(buf, "English_United States.");
        GetLocaleInfoA(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                LOCALE_IDEFAULTCODEPAGE, buf+strlen(buf), 80);
        ok(!strcmp(ret, buf), "ret = %s, expected %s\n", ret, buf);
    }

    GetLocaleInfoA(GetUserDefaultLCID(), LOCALE_IDEFAULTCODEPAGE, buf, sizeof(buf));
    if(IsValidCodePage(atoi(buf))) {
        ret = setlocale(LC_ALL, ".OCP");
        ok(ret != NULL, "ret == NULL\n");
        ptr = strchr(ret, '.');
        ok(ptr && !strcmp(ptr + 1, buf), "ret %s, buf %s.\n", ret, buf);
    }

    len = GetLocaleInfoA(GetUserDefaultLCID(), LOCALE_IDEFAULTANSICODEPAGE, buf, sizeof(buf)) - 1;
    if(buf[0] == '0' && !buf[1])
        len = sprintf(buf, "%d", GetACP());
    ret = setlocale(LC_ALL, ".ACP");
    ok(ret != NULL, "ret == NULL\n");
    ptr = strchr(ret, '.');
    ok(ptr && !strncmp(ptr + 1, buf, len), "ret %s, buf %s.\n", ret, buf);

    ret = setlocale(LC_ALL, ".1250");
    ok(ret != NULL, "ret == NULL\n");
    ptr = strchr(ret, '.');
    ok(ptr && !strcmp(ptr, ".1250"), "ret %s, buf %s.\n", ret, buf);

    ret = setlocale(LC_ALL, "English_United States.UTF8");
    ok(ret == NULL, "ret != NULL\n");

    ret = setlocale(LC_ALL, "en-US");
    ok(ret == NULL || broken (ret != NULL), "ret != NULL\n"); /* XP & 2003 */
}

static void test_crtGetStringTypeW(void)
{
    const wchar_t *str[] = { L"0", L"A", L" ", L"\0", L"\x04d2" };

    WORD out_crt, out;
    BOOL ret_crt, ret;
    int i;

    if(!p__crtGetStringTypeW) {
        win_skip("Skipping __crtGetStringTypeW tests\n");
        return;
    }

    if(!pmemcpy_s) {
        win_skip("Too old version of msvcrt.dll\n");
        return;
    }

    for(i=0; i<ARRAY_SIZE(str); i++) {
        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE1, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE1, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);

        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE2, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE2, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);

        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE3, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE3, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);
    }

    ret = p__crtGetStringTypeW(0, 3, str[0], 1, &out);
    ok(!ret, "ret == TRUE\n");
}

static void test__Gettnames(void)
{
    static const DWORD time_data[] = {
        LOCALE_SABBREVDAYNAME7, LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
        LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
        LOCALE_SABBREVDAYNAME6,
        LOCALE_SDAYNAME7, LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
        LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6,
        LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
        LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
        LOCALE_S1159, LOCALE_S2359,
        LOCALE_SSHORTDATE, LOCALE_SLONGDATE,
        LOCALE_STIMEFORMAT
    };

    struct {
        char *str[43];
        LCID lcid;
        int  unk[2];
        wchar_t *wstr[43];
        char data[1];
    } *ret;
    int size;
    char buf[64];
    int i;

    if(!setlocale(LC_ALL, "english"))
        return;

    ret = _Gettnames();
    size = ret->str[0]-(char*)ret;
    /* Newer version of the structure stores both ascii and unicode strings.
     * Unicode strings are only initialized on Windows 7
     */
    if(sizeof(void*) == 8)
        ok(size==0x2c0 || broken(size==0x168), "structure size: %x\n", size);
    else
        ok(size==0x164 || broken(size==0xb8), "structure size: %x\n", size);

    for (i = 0; i < ARRAY_SIZE(time_data); i++)
    {
        size = GetLocaleInfoA(MAKELCID(LANG_ENGLISH, SORT_DEFAULT),
                              time_data[i], buf, sizeof(buf));
        ok(size, "GetLocaleInfo failed: %lx\n", GetLastError());
        ok(!strcmp(ret->str[i], buf), "ret->str[%i] = %s, expected %s\n", i, ret->str[i], buf);
    }

    ok(ret->wstr[0] != NULL, "ret->wstr[0] = NULL\n");
    ok(ret->str[42] + strlen(ret->str[42])+1 != (char*)ret->wstr[0],
            "ret->str[42] = %p len = %Id, ret->wstr[0] = %p\n",
            ret->str[42], strlen(ret->str[42]), ret->wstr[0]);
    free(ret);

    if(!setlocale(LC_TIME, "german"))
        return;

    ret = _Gettnames();
    for (i = 0; i < ARRAY_SIZE(time_data); i++)
    {
        size = GetLocaleInfoA(MAKELCID(LANG_GERMAN, SORT_DEFAULT),
                              time_data[i], buf, sizeof(buf));
        ok(size, "GetLocaleInfo failed: %lx\n", GetLastError());
        ok(!strcmp(ret->str[i], buf), "ret->str[%i] = %s, expected %s\n", i, ret->str[i], buf);
    }
    free(ret);

    setlocale(LC_ALL, "C");
}

static void test___mb_cur_max_func(void)
{
    int mb_cur_max;

    setlocale(LC_ALL, "C");

    /* for newer Windows */
    if(!p___mb_cur_max_func)
        win_skip("Skipping ___mb_cur_max_func tests\n");
    else {
        mb_cur_max = p___mb_cur_max_func();
        ok(mb_cur_max == 1, "mb_cur_max = %d, expected 1\n", mb_cur_max);

        /* some old Windows don't set chinese */
        if (!setlocale(LC_ALL, "chinese"))
            win_skip("Skipping test with chinese locale\n");
        else {
            mb_cur_max = p___mb_cur_max_func();
            ok(mb_cur_max == 2, "mb_cur_max = %d, expected 2\n", mb_cur_max);
            setlocale(LC_ALL, "C");
        }
    }

    /* for older Windows */
    if (!p__p___mb_cur_max)
        skip("Skipping __p___mb_cur_max tests\n");
    else {
        mb_cur_max = *p__p___mb_cur_max();
        ok(mb_cur_max == 1, "mb_cur_max = %d, expected 1\n", mb_cur_max);

        /* some old Windows don't set chinese */
        if (!setlocale(LC_ALL, "chinese"))
            win_skip("Skipping test with chinese locale\n");
        else {
            mb_cur_max = *p__p___mb_cur_max();
            ok(mb_cur_max == 2, "mb_cur_max = %d, expected 2\n", mb_cur_max);
            setlocale(LC_ALL, "C");
        }
    }
}

static void test__wcsicmp_l(void)
{
    const struct {
        const wchar_t *str1;
        const wchar_t *str2;
        int exp;
        const char *loc;
    } tests[] = {
        { L"i", L"i",  0 },
        { L"I", L"i",  0 },
        { L"I", L"i",  0, "Turkish" },
        { L"i", L"a",  8 },
        { L"a", L"i", -8 },
        { L"i", L"a",  8, "Turkish" },
    };
    int ret, i;

    if (!p_wcsicmp_l || !p_create_locale)
    {
        win_skip("_wcsicmp_l or _create_locale not available\n");
        return;
    }
    ok(!!p_free_locale, "_free_locale not available\n");

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        _locale_t loc = NULL;

        if(tests[i].loc && !(loc = p_create_locale(LC_ALL, tests[i].loc))) {
            win_skip("locale %s not available.  skipping\n", tests[i].loc);
            continue;
        }

        ret = p_wcsicmp_l(tests[i].str1, tests[i].str2, loc);
        ok(ret == tests[i].exp, "_wcsicmp_l = %d, expected %d for test %d '%ls' vs '%ls' using %s locale\n",
            ret, tests[i].exp, i, tests[i].str1, tests[i].str2, loc ? tests[i].loc : "current");

        if(loc)
            p_free_locale(loc);
    }
}

static unsigned __stdcall test_thread_setlocale_func(void *arg)
{
    char *ret;

    ret = setlocale(LC_ALL, NULL);
    ok(!strcmp(ret, "C"), "expected ret=C, but received ret=%s\n", ret);

    ret = setlocale(LC_ALL, "");
    ok(strcmp(ret, "Invariant Language_Invariant Country.0"), "expected valid locale\n");

    return 0;
}

static void test_thread_setlocale(void)
{
    HANDLE hThread;

    hThread = (HANDLE)_beginthreadex(NULL, 0, test_thread_setlocale_func, NULL, 0, NULL);
    ok(hThread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
}

static void test_locale_info(void)
{
    pthreadlocinfo locinfo, locinfo2;
    _locale_t locale, locale2;
    int ret;

    if (!p_create_locale)
    {
        win_skip("_create_locale isn't available.\n");
        return;
    }

    if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
        skip("Skip language-specific tests on Japanese system.\n");
    else
    {
        locale = p_create_locale(LC_ALL, "Japanese_Japan.932");
        locale2 = p_create_locale(LC_ALL, ".932");
        locinfo = locale->locinfo;
        locinfo2 = locale2->locinfo;

        ok(locinfo->mb_cur_max == locinfo2->mb_cur_max, "Got wrong max char size %d %d.\n",
                locinfo->mb_cur_max, locinfo2->mb_cur_max);
        ok(locinfo->ctype1_refcount != locinfo2->ctype1_refcount, "Got wrong refcount pointer %p vs %p.\n",
                locinfo->ctype1_refcount, locinfo2->ctype1_refcount);
        ok(locinfo->lc_codepage == 932 && locinfo->lc_codepage == locinfo2->lc_codepage,
                "Got wrong codepage %d vs %d.\n", locinfo->lc_codepage, locinfo2->lc_codepage);
        ok(locinfo->lc_id[LC_CTYPE].wCodePage == 932
                && locinfo->lc_id[LC_CTYPE].wCodePage == locinfo2->lc_id[LC_CTYPE].wCodePage,
                "Got wrong LC_CTYPE codepage %d vs %d.\n", locinfo->lc_id[LC_CTYPE].wCodePage,
                locinfo2->lc_id[LC_CTYPE].wCodePage);
        ret = strcmp(locinfo->lc_category[LC_CTYPE].locale, locinfo2->lc_category[LC_CTYPE].locale);
        ok(!!ret, "Got locale name %s vs %s.\n", locinfo->lc_category[LC_CTYPE].locale,
                locinfo2->lc_category[LC_CTYPE].locale);
        ret = memcmp(locinfo->ctype1, locinfo2->ctype1, 257 * sizeof(*locinfo->ctype1));
        ok(!ret, "Got wrong ctype1 data.\n");
        ret = memcmp(locinfo->pclmap, locinfo2->pclmap, 256 * sizeof(*locinfo->pclmap));
        ok(!ret, "Got wrong pclmap data.\n");
        ret = memcmp(locinfo->pcumap, locinfo2->pcumap, 256 * sizeof(*locinfo->pcumap));
        ok(!ret, "Got wrong pcumap data.\n");
        ok(locinfo->lc_handle[LC_CTYPE] != locinfo2->lc_handle[LC_CTYPE],
                "Got wrong LC_CTYPE %#lx vs %#lx.\n", locinfo->lc_handle[LC_CTYPE], locinfo2->lc_handle[LC_CTYPE]);

        p_free_locale(locale2);
        locale2 = p_create_locale(LC_ALL, "Japanese_Japan.1252");
        locinfo2 = locale2->locinfo;

        ok(locinfo->mb_cur_max != locinfo2->mb_cur_max, "Got wrong max char size %d %d.\n",
                locinfo->mb_cur_max, locinfo2->mb_cur_max);
        ok(locinfo->ctype1_refcount != locinfo2->ctype1_refcount, "Got wrong refcount pointer %p vs %p.\n",
                locinfo->ctype1_refcount, locinfo2->ctype1_refcount);
        ok(locinfo2->lc_codepage == 1252, "Got wrong codepage %d.\n", locinfo2->lc_codepage);
        ok(locinfo2->lc_id[LC_CTYPE].wCodePage == 1252, "Got wrong LC_CTYPE codepage %d.\n",
                locinfo2->lc_id[LC_CTYPE].wCodePage);
        ok(locinfo->lc_codepage != locinfo2->lc_codepage, "Got wrong codepage %d vs %d.\n",
                locinfo->lc_codepage, locinfo2->lc_codepage);
        ok(locinfo->lc_id[LC_CTYPE].wCodePage != locinfo2->lc_id[LC_CTYPE].wCodePage,
                "Got wrong LC_CTYPE codepage %d vs %d.\n", locinfo->lc_id[LC_CTYPE].wCodePage,
                locinfo2->lc_id[LC_CTYPE].wCodePage);
        ret = strcmp(locinfo->lc_category[LC_CTYPE].locale, locinfo2->lc_category[LC_CTYPE].locale);
        ok(!!ret, "Got locale name %s vs %s.\n", locinfo->lc_category[LC_CTYPE].locale,
                locinfo2->lc_category[LC_CTYPE].locale);
        ret = memcmp(locinfo->ctype1, locinfo2->ctype1, 257 * sizeof(*locinfo->ctype1));
        ok(!!ret, "Got wrong ctype1 data.\n");
        ret = memcmp(locinfo->pclmap, locinfo2->pclmap, 256 * sizeof(*locinfo->pclmap));
        ok(!!ret, "Got wrong pclmap data.\n");
        ret = memcmp(locinfo->pcumap, locinfo2->pcumap, 256 * sizeof(*locinfo->pcumap));
        ok(!!ret, "Got wrong pcumap data.\n");
        ok(locinfo->lc_handle[LC_CTYPE] == locinfo2->lc_handle[LC_CTYPE],
                "Got wrong LC_CTYPE %#lx vs %#lx.\n", locinfo->lc_handle[LC_CTYPE], locinfo2->lc_handle[LC_CTYPE]);

        p_free_locale(locale2);
        locale2 = p_create_locale(LC_ALL, "Japanese_Japan.3000"); /* an invalid codepage */
        ok(!locale2, "Got %p.\n", locale2);

        p_free_locale(locale);
    }

    locale = p_create_locale(LC_ALL, "German_Germany.437");
    locale2 = p_create_locale(LC_ALL, "German_Germany.1252");
    locinfo = locale->locinfo;
    locinfo2 = locale2->locinfo;

    ok(locinfo->mb_cur_max == locinfo2->mb_cur_max, "Got wrong max char size %d %d.\n",
            locinfo->mb_cur_max, locinfo2->mb_cur_max);
    ok(locinfo->ctype1_refcount != locinfo2->ctype1_refcount, "Got wrong refcount pointer %p vs %p.\n",
            locinfo->ctype1_refcount, locinfo2->ctype1_refcount);
    ok(locinfo->lc_codepage != locinfo2->lc_codepage, "Got wrong codepage %d vs %d.\n",
            locinfo->lc_codepage, locinfo2->lc_codepage);
    ok(locinfo->lc_id[LC_CTYPE].wCodePage != locinfo2->lc_id[LC_CTYPE].wCodePage,
            "Got wrong LC_CTYPE codepage %d vs %d.\n", locinfo->lc_id[LC_CTYPE].wCodePage,
            locinfo2->lc_id[LC_CTYPE].wCodePage);
    ret = strcmp(locinfo->lc_category[LC_CTYPE].locale, locinfo2->lc_category[LC_CTYPE].locale);
    ok(!!ret, "Got locale name %s vs %s.\n", locinfo->lc_category[LC_CTYPE].locale,
            locinfo2->lc_category[LC_CTYPE].locale);
    ret = memcmp(locinfo->ctype1, locinfo2->ctype1, 257 * sizeof(*locinfo->ctype1));
    ok(!!ret, "Got wrong ctype1 data.\n");
    ret = memcmp(locinfo->pclmap, locinfo2->pclmap, 256 * sizeof(*locinfo->pclmap));
    ok(!!ret, "Got wrong pclmap data.\n");
    ret = memcmp(locinfo->pcumap, locinfo2->pcumap, 256 * sizeof(*locinfo->pcumap));
    ok(!!ret, "Got wrong pcumap data.\n");
    ok(locinfo->lc_handle[LC_CTYPE] == locinfo2->lc_handle[LC_CTYPE],
            "Got wrong LC_CTYPE %#lx vs %#lx.\n", locinfo->lc_handle[LC_CTYPE], locinfo2->lc_handle[LC_CTYPE]);

    p_free_locale(locale2);
    p_free_locale(locale);
}

START_TEST(locale)
{
    init();

    test_crtGetStringTypeW();
    test_setlocale();
    test__Gettnames();
    test___mb_cur_max_func();
    test__wcsicmp_l();
    test_thread_setlocale();
    test_locale_info();
}
