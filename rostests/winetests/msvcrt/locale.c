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

#include "wine/test.h"
#include "winnls.h"

static BOOL (__cdecl *p__crtGetStringTypeW)(DWORD, DWORD, const wchar_t*, int, WORD*);
static int (__cdecl *pmemcpy_s)(void *, size_t, void*, size_t);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__crtGetStringTypeW = (void*)GetProcAddress(hmod, "__crtGetStringTypeW");
    pmemcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
}

static void test_setlocale(void)
{
    static const char lc_all[] = "LC_COLLATE=C;LC_CTYPE=C;"
        "LC_MONETARY=Greek_Greece.1253;LC_NUMERIC=Polish_Poland.1250;LC_TIME=C";

    char *ret, buf[100];

    ret = setlocale(20, "C");
    ok(ret == NULL, "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "");
    ok(ret != NULL, "ret == NULL\n");

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
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "chinese-simplified");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")
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
        || broken(!strcmp(ret, "Chinese_People's Republic of China.936")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "cht");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Chinese (Traditional)_Taiwan.950")
        || broken(!strcmp(ret, "Chinese_Taiwan.950")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "csy");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Czech_Czech Republic.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "czech");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Czech_Czech Republic.1250"), "ret = %s\n", ret);

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
        ok(!strcmp( ret, "Norwegian-Nynorsk_Norway.1252")
        || !strcmp(ret, "Norwegian (Nynorsk)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252"))
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "nor");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252")
        || !strcmp(ret, "Norwegian (Bokmal)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "norwegian-bokmal");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252")
        || !strcmp(ret, "Norwegian (Bokmal)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "norwegian-nynorsk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Norwegian-Nynorsk_Norway.1252")
        || !strcmp(ret, "Norwegian (Nynorsk)_Norway.1252")
        || broken(!strcmp(ret, "Norwegian_Norway.1252"))
        || broken(!strcmp(ret, "Norwegian (Bokmal)_Norway.1252"))
        || broken(!strcmp(ret, "Norwegian (Bokm\xe5l)_Norway.1252")), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "plk");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "polish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "portugese");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Portuguese_Brazil.1252"), "ret = %s\n", ret);

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
        ok(!strcmp(ret, "Turkish_Turkey.1254"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "turkish");
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "Turkish_Turkey.1254"), "ret = %s\n", ret);

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
}

static void test_crtGetStringTypeW(void)
{
    static const wchar_t str0[] = { '0', '\0' };
    static const wchar_t strA[] = { 'A', '\0' };
    static const wchar_t str_space[] = { ' ', '\0' };
    static const wchar_t str_null[] = { '\0', '\0' };
    static const wchar_t str_rand[] = { 1234, '\0' };

    const wchar_t *str[] = { str0, strA, str_space, str_null, str_rand };

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

    for(i=0; i<sizeof(str)/sizeof(*str); i++) {
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

START_TEST(locale)
{
    init();

    test_crtGetStringTypeW();
    test_setlocale();
}
