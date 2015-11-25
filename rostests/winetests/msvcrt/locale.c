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
static int (__cdecl *p___mb_cur_max_func)(void);
static int *(__cdecl *p__p___mb_cur_max)(void);
void* __cdecl _Gettnames(void);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__crtGetStringTypeW = (void*)GetProcAddress(hmod, "__crtGetStringTypeW");
    pmemcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
    p___mb_cur_max_func = (void*)GetProcAddress(hmod, "___mb_cur_max_func");
    p__p___mb_cur_max = (void*)GetProcAddress(hmod, "__p___mb_cur_max");
}

static void test_setlocale(void)
{
    static const char lc_all[] = "LC_COLLATE=C;LC_CTYPE=C;"
        "LC_MONETARY=Greek_Greece.1253;LC_NUMERIC=Polish_Poland.1250;LC_TIME=C";

    char *ret, buf[100];

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
todo_wine
    ok(ret != NULL || broken (ret == NULL), "ret == NULL\n");
    if(ret)
    {
todo_wine
        ok(!strcmp(ret, "Chinese (Simplified)_People's Republic of China.936")
        || !strcmp(ret, "Chinese (Simplified)_China.936")
        || broken(!strcmp(ret, "Chinese_People's Republic of China.936")), "ret = %s\n", ret);
        trace("ret is %s\n", ret);
    }

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

    ret = setlocale(LC_ALL, "English_United States.UTF8");
    ok(ret == NULL, "ret != NULL\n");
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

static void test__Gettnames(void)
{
    struct {
        char *str[43];
        LCID lcid;
        int  unk[2];
        wchar_t *wstr[43];
        char data[1];
    } *ret;
    int size;
    char buf[64];

    if(!setlocale(LC_ALL, "english"))
        return;

    ret = _Gettnames();
    size = ret->data-(char*)ret;
    /* Newer version of the structure stores both ascii and unicode strings.
     * Unicode strings are only initialized on Windows 7
     */
    if(sizeof(void*) == 8)
        ok(size==0x2c0 || broken(size==0x170), "structure size: %x\n", size);
    else
        ok(size==0x164 || broken(size==0xb8), "structure size: %x\n", size);

    ok(!strcmp(ret->str[0], "Sun"), "ret->str[0] = %s\n", ret->str[0]);
    ok(!strcmp(ret->str[1], "Mon"), "ret->str[1] = %s\n", ret->str[1]);
    ok(!strcmp(ret->str[2], "Tue"), "ret->str[2] = %s\n", ret->str[2]);
    ok(!strcmp(ret->str[3], "Wed"), "ret->str[3] = %s\n", ret->str[3]);
    ok(!strcmp(ret->str[4], "Thu"), "ret->str[4] = %s\n", ret->str[4]);
    ok(!strcmp(ret->str[5], "Fri"), "ret->str[5] = %s\n", ret->str[5]);
    ok(!strcmp(ret->str[6], "Sat"), "ret->str[6] = %s\n", ret->str[6]);
    ok(!strcmp(ret->str[7], "Sunday"), "ret->str[7] = %s\n", ret->str[7]);
    ok(!strcmp(ret->str[8], "Monday"), "ret->str[8] = %s\n", ret->str[8]);
    ok(!strcmp(ret->str[9], "Tuesday"), "ret->str[9] = %s\n", ret->str[9]);
    ok(!strcmp(ret->str[10], "Wednesday"), "ret->str[10] = %s\n", ret->str[10]);
    ok(!strcmp(ret->str[11], "Thursday"), "ret->str[11] = %s\n", ret->str[11]);
    ok(!strcmp(ret->str[12], "Friday"), "ret->str[12] = %s\n", ret->str[12]);
    ok(!strcmp(ret->str[13], "Saturday"), "ret->str[13] = %s\n", ret->str[13]);
    ok(!strcmp(ret->str[14], "Jan"), "ret->str[14] = %s\n", ret->str[14]);
    ok(!strcmp(ret->str[15], "Feb"), "ret->str[15] = %s\n", ret->str[15]);
    ok(!strcmp(ret->str[16], "Mar"), "ret->str[16] = %s\n", ret->str[16]);
    ok(!strcmp(ret->str[17], "Apr"), "ret->str[17] = %s\n", ret->str[17]);
    ok(!strcmp(ret->str[18], "May"), "ret->str[18] = %s\n", ret->str[18]);
    ok(!strcmp(ret->str[19], "Jun"), "ret->str[19] = %s\n", ret->str[19]);
    ok(!strcmp(ret->str[20], "Jul"), "ret->str[20] = %s\n", ret->str[20]);
    ok(!strcmp(ret->str[21], "Aug"), "ret->str[21] = %s\n", ret->str[21]);
    ok(!strcmp(ret->str[22], "Sep"), "ret->str[22] = %s\n", ret->str[22]);
    ok(!strcmp(ret->str[23], "Oct"), "ret->str[23] = %s\n", ret->str[23]);
    ok(!strcmp(ret->str[24], "Nov"), "ret->str[24] = %s\n", ret->str[24]);
    ok(!strcmp(ret->str[25], "Dec"), "ret->str[25] = %s\n", ret->str[25]);
    ok(!strcmp(ret->str[26], "January"), "ret->str[26] = %s\n", ret->str[26]);
    ok(!strcmp(ret->str[27], "February"), "ret->str[27] = %s\n", ret->str[27]);
    ok(!strcmp(ret->str[28], "March"), "ret->str[28] = %s\n", ret->str[28]);
    ok(!strcmp(ret->str[29], "April"), "ret->str[29] = %s\n", ret->str[29]);
    ok(!strcmp(ret->str[30], "May"), "ret->str[30] = %s\n", ret->str[30]);
    ok(!strcmp(ret->str[31], "June"), "ret->str[31] = %s\n", ret->str[31]);
    ok(!strcmp(ret->str[32], "July"), "ret->str[32] = %s\n", ret->str[32]);
    ok(!strcmp(ret->str[33], "August"), "ret->str[33] = %s\n", ret->str[33]);
    ok(!strcmp(ret->str[34], "September"), "ret->str[34] = %s\n", ret->str[34]);
    ok(!strcmp(ret->str[35], "October"), "ret->str[35] = %s\n", ret->str[35]);
    ok(!strcmp(ret->str[36], "November"), "ret->str[36] = %s\n", ret->str[36]);
    ok(!strcmp(ret->str[37], "December"), "ret->str[37] = %s\n", ret->str[37]);
    ok(!strcmp(ret->str[38], "AM"), "ret->str[38] = %s\n", ret->str[38]);
    ok(!strcmp(ret->str[39], "PM"), "ret->str[39] = %s\n", ret->str[39]);
    ok(!strcmp(ret->str[40], "M/d/yyyy") || broken(!strcmp(ret->str[40], "M/d/yy"))/*NT*/,
            "ret->str[40] = %s\n", ret->str[40]);
    size = GetLocaleInfoA(MAKELCID(LANG_ENGLISH, SORT_DEFAULT),
           LOCALE_SLONGDATE|LOCALE_NOUSEROVERRIDE, buf, sizeof(buf));
    ok(size, "GetLocaleInfo failed: %x\n", GetLastError());
    ok(!strcmp(ret->str[41], buf), "ret->str[41] = %s, expected %s\n", ret->str[41], buf);
    free(ret);

    if(!setlocale(LC_TIME, "german"))
        return;

    ret = _Gettnames();
    ok(!strcmp(ret->str[0], "So"), "ret->str[0] = %s\n", ret->str[0]);
    ok(!strcmp(ret->str[1], "Mo"), "ret->str[1] = %s\n", ret->str[1]);
    ok(!strcmp(ret->str[2], "Di"), "ret->str[2] = %s\n", ret->str[2]);
    ok(!strcmp(ret->str[3], "Mi"), "ret->str[3] = %s\n", ret->str[3]);
    ok(!strcmp(ret->str[4], "Do"), "ret->str[4] = %s\n", ret->str[4]);
    ok(!strcmp(ret->str[5], "Fr"), "ret->str[5] = %s\n", ret->str[5]);
    ok(!strcmp(ret->str[6], "Sa"), "ret->str[6] = %s\n", ret->str[6]);
    ok(!strcmp(ret->str[7], "Sonntag"), "ret->str[7] = %s\n", ret->str[7]);
    ok(!strcmp(ret->str[8], "Montag"), "ret->str[8] = %s\n", ret->str[8]);
    ok(!strcmp(ret->str[9], "Dienstag"), "ret->str[9] = %s\n", ret->str[9]);
    ok(!strcmp(ret->str[10], "Mittwoch"), "ret->str[10] = %s\n", ret->str[10]);
    ok(!strcmp(ret->str[11], "Donnerstag"), "ret->str[11] = %s\n", ret->str[11]);
    ok(!strcmp(ret->str[12], "Freitag"), "ret->str[12] = %s\n", ret->str[12]);
    ok(!strcmp(ret->str[13], "Samstag"), "ret->str[13] = %s\n", ret->str[13]);
    ok(!strcmp(ret->str[14], "Jan"), "ret->str[14] = %s\n", ret->str[14]);
    ok(!strcmp(ret->str[15], "Feb"), "ret->str[15] = %s\n", ret->str[15]);
    ok(!strcmp(ret->str[16], "Mrz"), "ret->str[16] = %s\n", ret->str[16]);
    ok(!strcmp(ret->str[17], "Apr"), "ret->str[17] = %s\n", ret->str[17]);
    ok(!strcmp(ret->str[18], "Mai"), "ret->str[18] = %s\n", ret->str[18]);
    ok(!strcmp(ret->str[19], "Jun"), "ret->str[19] = %s\n", ret->str[19]);
    ok(!strcmp(ret->str[20], "Jul"), "ret->str[20] = %s\n", ret->str[20]);
    ok(!strcmp(ret->str[21], "Aug"), "ret->str[21] = %s\n", ret->str[21]);
    ok(!strcmp(ret->str[22], "Sep"), "ret->str[22] = %s\n", ret->str[22]);
    ok(!strcmp(ret->str[23], "Okt"), "ret->str[23] = %s\n", ret->str[23]);
    ok(!strcmp(ret->str[24], "Nov"), "ret->str[24] = %s\n", ret->str[24]);
    ok(!strcmp(ret->str[25], "Dez"), "ret->str[25] = %s\n", ret->str[25]);
    ok(!strcmp(ret->str[26], "Januar"), "ret->str[26] = %s\n", ret->str[26]);
    ok(!strcmp(ret->str[27], "Februar"), "ret->str[27] = %s\n", ret->str[27]);
    ok(!strcmp(ret->str[29], "April"), "ret->str[29] = %s\n", ret->str[29]);
    ok(!strcmp(ret->str[30], "Mai"), "ret->str[30] = %s\n", ret->str[30]);
    ok(!strcmp(ret->str[31], "Juni"), "ret->str[31] = %s\n", ret->str[31]);
    ok(!strcmp(ret->str[32], "Juli"), "ret->str[32] = %s\n", ret->str[32]);
    ok(!strcmp(ret->str[33], "August"), "ret->str[33] = %s\n", ret->str[33]);
    ok(!strcmp(ret->str[34], "September"), "ret->str[34] = %s\n", ret->str[34]);
    ok(!strcmp(ret->str[35], "Oktober"), "ret->str[35] = %s\n", ret->str[35]);
    ok(!strcmp(ret->str[36], "November"), "ret->str[36] = %s\n", ret->str[36]);
    ok(!strcmp(ret->str[37], "Dezember"), "ret->str[37] = %s\n", ret->str[37]);
    ok(!strcmp(ret->str[38], ""), "ret->str[38] = %s\n", ret->str[38]);
    ok(!strcmp(ret->str[39], ""), "ret->str[39] = %s\n", ret->str[39]);
    ok(!strcmp(ret->str[40], "dd.MM.yyyy") || broken(!strcmp(ret->str[40], "dd.MM.yy"))/*NT*/,
            "ret->str[40] = %s\n", ret->str[40]);
    ok(!strcmp(ret->str[41], "dddd, d. MMMM yyyy"), "ret->str[41] = %s\n", ret->str[41]);
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
        win_skip("Skipping __p___mb_cur_max tests\n");
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

START_TEST(locale)
{
    init();

    test_crtGetStringTypeW();
    test_setlocale();
    test__Gettnames();
    test___mb_cur_max_func();
}
