/*
 * Copyright 2016 Daniel Lehman (Esri)
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <verrsrc.h>
#include <dbghelp.h>
#include "wine/test.h"

typedef unsigned char MSVCRT_bool;

typedef struct {
    const char  *what;
    MSVCRT_bool  dofree;
} __std_exception_data;

typedef struct
{
    char *name;
    char mangled[32];
} type_info140;

typedef struct _type_info_list
{
    SLIST_ENTRY entry;
    char name[1];
} type_info_list;

static void* (CDECL *p_malloc)(size_t);
static void (CDECL *p___std_exception_copy)(const __std_exception_data*, __std_exception_data*);
static void (CDECL *p___std_exception_destroy)(__std_exception_data*);
static int (CDECL *p___std_type_info_compare)(const type_info140*, const type_info140*);
static const char* (CDECL *p___std_type_info_name)(type_info140*, SLIST_HEADER*);
static void (CDECL *p___std_type_info_destroy_list)(SLIST_HEADER*);
static size_t (CDECL *p___std_type_info_hash)(type_info140*);
static char* (__cdecl *p___unDName)(char*,const char*,int,void*,void*,unsigned short int);

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("ucrtbase.dll");
    if (!module)
    {
        win_skip("ucrtbase.dll not installed\n");
        return FALSE;
    }

    p_malloc = (void*)GetProcAddress(module, "malloc");
    p___std_exception_copy = (void*)GetProcAddress(module, "__std_exception_copy");
    p___std_exception_destroy = (void*)GetProcAddress(module, "__std_exception_destroy");
    p___std_type_info_compare = (void*)GetProcAddress(module, "__std_type_info_compare");
    p___std_type_info_name = (void*)GetProcAddress(module, "__std_type_info_name");
    p___std_type_info_destroy_list = (void*)GetProcAddress(module, "__std_type_info_destroy_list");
    p___std_type_info_hash = (void*)GetProcAddress(module, "__std_type_info_hash");
    p___unDName = (void*)GetProcAddress(module, "__unDName");
    return TRUE;
}

static void test___std_exception(void)
{
    __std_exception_data src;
    __std_exception_data dst;

    if (0) /* crash on Windows */
    {
        p___std_exception_copy(NULL, &src);
        p___std_exception_copy(&dst, NULL);

        src.what   = "invalid free";
        src.dofree = 1;
        p___std_exception_destroy(&src);
        p___std_exception_destroy(NULL);
    }

    src.what   = "what";
    src.dofree = 0;
    p___std_exception_copy(&src, &dst);
    ok(dst.what == src.what, "expected what to be same, got src %p dst %p\n", src.what, dst.what);
    ok(!dst.dofree, "expected 0, got %d\n", dst.dofree);

    src.dofree = 0x42;
    p___std_exception_copy(&src, &dst);
    ok(dst.what != src.what, "expected what to be different, got src %p dst %p\n", src.what, dst.what);
    ok(dst.dofree == 1, "expected 1, got %d\n", dst.dofree);

    p___std_exception_destroy(&dst);
    ok(!dst.what, "expected NULL, got %p\n", dst.what);
    ok(!dst.dofree, "expected 0, got %d\n", dst.dofree);

    src.what = NULL;
    src.dofree = 0;
    p___std_exception_copy(&src, &dst);
    ok(!dst.what, "dst.what != NULL\n");
    ok(!dst.dofree, "dst.dofree != FALSE\n");

    src.what = NULL;
    src.dofree = 1;
    p___std_exception_copy(&src, &dst);
    ok(!dst.what, "dst.what != NULL\n");
    ok(!dst.dofree, "dst.dofree != FALSE\n");
}

static void test___std_type_info(void)
{
    type_info140 ti1 = { NULL, ".?AVa@@" };
    type_info140 ti2 = { NULL, ".?AVb@@" };
    type_info140 ti3 = ti1;
    SLIST_HEADER header;
    type_info_list *elem;
    const char *ret;
    size_t hash1, hash2;
    int eq;


    InitializeSListHead(&header);
    p___std_type_info_destroy_list(&header);

    elem = p_malloc(sizeof(*elem));
    memset(elem, 0, sizeof(*elem));
    InterlockedPushEntrySList(&header, &elem->entry);
    p___std_type_info_destroy_list(&header);
    ok(!InterlockedPopEntrySList(&header), "list is not empty\n");

    ret = p___std_type_info_name(&ti1, &header);
    ok(!strcmp(ret, "class a"), "__std_type_info_name(&ti1) = %s\n", ret);
    ok(ti1.name == ret, "ti1.name = %p, ret = %p\n", ti1.name, ret);

    p___std_type_info_destroy_list(&header);
    ok(!InterlockedPopEntrySList(&header), "list is not empty\n");
    ok(ti1.name == ret, "ti1.name = %p, ret = %p\n", ti1.name, ret);
    ti1.name = NULL;

    eq = p___std_type_info_compare(&ti1, &ti1);
    ok(eq == 0, "__std_type_info_compare(&ti1, &ti1) = %d\n", eq);

    eq = p___std_type_info_compare(&ti1, &ti2);
    ok(eq == -1, "__std_type_info_compare(&ti1, &ti2) = %d\n", eq);

    eq = p___std_type_info_compare(&ti1, &ti3);
    ok(eq == 0, "__std_type_info_compare(&ti1, &ti3) = %d\n", eq);

    ti1.mangled[0] = 0;
    ti1.mangled[1] = 0;
    ti1.mangled[2] = 0;
    hash1 = p___std_type_info_hash(&ti1);
#ifdef _WIN64
    ok(hash1 == 0xcbf29ce44fd0bfc1, "hash = %p\n", (void*)hash1);
#else
    ok(hash1 == 0x811c9dc5, "hash = %p\n", (void*)hash1);
#endif

    ti1.mangled[0] = 1;
    hash2 = p___std_type_info_hash(&ti1);
    ok(hash1 == hash2, "hash1 != hash2 (first char not ignored)\n");

    ti1.mangled[1] = 1;
    hash1 = p___std_type_info_hash(&ti1);
#ifdef _WIN64
    ok(hash1 == 0xaf63bc4c29620a60, "hash = %p\n", (void*)hash1);
#else
    ok(hash1 == 0x40c5b8c, "hash = %p\n", (void*)hash1);
#endif
    ok(hash1 != hash2, "hash1 == hash2 for different strings\n");

    ti1.mangled[1] = 2;
    hash2 = p___std_type_info_hash(&ti1);
    ok(hash1 != hash2, "hash1 == hash2 for different strings\n");

    hash1 = p___std_type_info_hash(&ti2);
    ok(hash1 != hash2, "hash1 == hash2 for different strings\n");
}

static void test___unDName(void)
{
    static struct {const char *in; const char *out; const char *broken; unsigned int flags;} und_tests[] =
    {
/*   0 */ {"??4QDnsDomainNameRecord@@QAEAAV0@$$QAV0@@Z",
           "public: class QDnsDomainNameRecord & __thiscall QDnsDomainNameRecord::operator=(class QDnsDomainNameRecord &&)"},
/*   1 */ {"??4QDnsDomainNameRecord@@QAEAAV0@$$QEAV0@@Z",
          "public: class QDnsDomainNameRecord & __thiscall QDnsDomainNameRecord::operator=(class QDnsDomainNameRecord && __ptr64)"},
/*   2 */ {"??__K_l@@YA?AUCC@@I@Z", "struct CC __cdecl operator \"\" _l(unsigned int)",
           "??__K_l@@YA?AUCC@@I@Z" /* W10 1507 fails on this :-( */},
/*   3 */ {"?meth@Q@@QEGBA?AV1@XZ",
           "public: class Q __cdecl Q::meth(void)const __ptr64& ",
           "public: ?? :: ?? ::XZ::V1" /* W10 1507 fails on this :-( */},
/*   4 */ {"?meth@Q@@QEHAA?AV1@XZ",
           "public: class Q __cdecl Q::meth(void) __ptr64&& ",
           "public: ?? :: ?? ::XZ::V1" /* W10 1507 fails on this :-( */},
/*   5 */ {"?meth@Q@@QEGBA?AV1@XZ",
           "public: class Q Q::meth(void)const & ",
           "public: ?? :: ?? ::XZ::V1" /* W10 1507 fails on this :-( */,
           UNDNAME_NO_MS_KEYWORDS},
/*   6 */ {"?meth@Q@@QEHAA?AV1@XZ",
           "public: class Q Q::meth(void)&& ",
           "public: ?? :: ?? ::XZ::V1" /* W10 1507 fails on this :-( */,
           UNDNAME_NO_MS_KEYWORDS},
/*   7 */ {"?AU?$my_iter@H$0A@$$V@@",
           "struct my_iter<int,0>",
           NULL,
           UNDNAME_NO_ARGUMENTS},
/*   8 */ {"??$foo@J_W$$T@bar@@YAJQB_W$$THQAUgod@@@Z",
           "long __cdecl bar::foo<long,wchar_t,std::nullptr_t>(wchar_t const * const,std::nullptr_t,int,struct god * const)"},

    };
    unsigned i;
    for (i = 0; i < ARRAY_SIZE(und_tests); i++)
    {
        char *name = p___unDName(0, und_tests[i].in, 0, malloc, free, und_tests[i].flags);
        ok(!strcmp(name, und_tests[i].out) ||
           broken(und_tests[i].broken && !strcmp(und_tests[i].broken, name)),
           "unDName returned %s for #%u\n", wine_dbgstr_a(name), i);
        free(name);
    }
}

START_TEST(cpp)
{
    if (!init()) return;
    test___std_exception();
    test___std_type_info();
    test___unDName();
}
