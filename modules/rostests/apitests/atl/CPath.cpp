/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CPath
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include <atlpath.h>
#include "resource.h"

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

#undef ok
#undef _T

#define TEST_NAMEX(name)        void test_##name##W()
#define CPathX                  CPathW
#define CStringX                CStringW
#define _X(x)                   L ## x
#define XCHAR                   WCHAR
#define dbgstrx(x)              wine_dbgstr_w(x)
#define ok                      ok_("CPathW:\n" __FILE__, __LINE__)
#define GetModuleFileNameX      GetModuleFileNameW
#include "CPath.inl"

#undef CPathX
#undef CStringX
#undef TEST_NAMEX
#undef _X
#undef XCHAR
#undef dbgstrx
#undef ok
#undef GetModuleFileNameX

#define TEST_NAMEX(name)        void test_##name##A()
#define CPathX                  CPathA
#define CStringX                CStringA
#define _X(x)                   x
#define XCHAR                   CHAR
#define dbgstrx(x)              (const char*)x
#define ok                      ok_("CPathA:\n" __FILE__, __LINE__)
#define GetModuleFileNameX      GetModuleFileNameA
#include "CPath.inl"

START_TEST(CPath)
{
    test_initW();
    test_initA();

    test_modifyW();
    test_modifyA();

    test_is_somethingW();
    test_is_somethingA();
}
