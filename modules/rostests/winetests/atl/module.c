/*
 * ATL test program
 *
 * Copyright 2010 Marcus Meissner
 *
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include <wine/atlbase.h>

#include <wine/test.h>

#define MAXSIZE 512
static void test_StructSize(void)
{
    _ATL_MODULEW *tst;
    HRESULT hres;
    int i;

    tst = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAXSIZE);

    for (i=0;i<MAXSIZE;i++) {
        tst->cbSize = i;
        hres = AtlModuleInit(tst, NULL, NULL);

        switch (i)  {
        case FIELD_OFFSET(_ATL_MODULEW, dwAtlBuildVer):
        case sizeof(_ATL_MODULEW):
#ifdef _WIN64
        case sizeof(_ATL_MODULEW) + sizeof(void *):
#endif
            ok (hres == S_OK, "AtlModuleInit with %d failed (0x%x).\n", i, (int)hres);
            break;
        default:
            ok (FAILED(hres), "AtlModuleInit with %d succeeded? (0x%x).\n", i, (int)hres);
            break;
        }
    }

    HeapFree (GetProcessHeap(), 0, tst);
}

static void test_winmodule(void)
{
    _AtlCreateWndData create_data[3];
    _ATL_MODULEW winmod;
    void *p;
    HRESULT hres;

    winmod.cbSize = sizeof(winmod);
    winmod.m_pCreateWndList = (void*)0xdeadbeef;
    winmod.m_csWindowCreate.LockCount = 0xdeadbeef;
    hres = AtlModuleInit(&winmod, NULL, NULL);
    ok(hres == S_OK, "AtlModuleInit failed: %08x\n", hres);
    ok(!winmod.m_pCreateWndList, "winmod.m_pCreateWndList = %p\n", winmod.m_pCreateWndList);
    ok(winmod.m_csWindowCreate.LockCount == -1, "winmod.m_csWindowCreate.LockCount = %d\n",
       winmod.m_csWindowCreate.LockCount);

    AtlModuleAddCreateWndData(&winmod, create_data, (void*)0xdead0001);
    ok(winmod.m_pCreateWndList == create_data, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[0].m_pThis == (void*)0xdead0001, "unexpected create_data[0].m_pThis %p\n", create_data[0].m_pThis);
    ok(create_data[0].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[0].m_dwThreadID %x\n",
       create_data[0].m_dwThreadID);
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);

    AtlModuleAddCreateWndData(&winmod, create_data+1, (void*)0xdead0002);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[1].m_pThis == (void*)0xdead0002, "unexpected create_data[1].m_pThis %p\n", create_data[1].m_pThis);
    ok(create_data[1].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[1].m_dwThreadID %x\n",
       create_data[1].m_dwThreadID);
    ok(create_data[1].m_pNext == create_data, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    AtlModuleAddCreateWndData(&winmod, create_data+2, (void*)0xdead0003);
    ok(winmod.m_pCreateWndList == create_data+2, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pThis == (void*)0xdead0003, "unexpected create_data[2].m_pThis %p\n", create_data[2].m_pThis);
    ok(create_data[2].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[2].m_dwThreadID %x\n",
       create_data[2].m_dwThreadID);
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    p = AtlModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0003, "unexpected AtlModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    create_data[1].m_dwThreadID = 0xdeadbeef;

    p = AtlModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0001, "unexpected AtlModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);
    ok(!create_data[1].m_pNext, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    p = AtlModuleExtractCreateWndData(&winmod);
    ok(!p, "unexpected AtlModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
}

static DWORD cb_val;

static void WINAPI term_callback(DWORD dw)
{
    cb_val = dw;
}

static void test_term(void)
{
    _ATL_MODULEW test;
    HRESULT hres;

    test.cbSize = sizeof(_ATL_MODULEW);

    hres = AtlModuleInit(&test, NULL, NULL);
    ok (hres == S_OK, "AtlModuleInit failed (0x%x).\n", (int)hres);

    hres = AtlModuleAddTermFunc(&test, term_callback, 0x22);
    ok (hres == S_OK, "AtlModuleAddTermFunc failed (0x%x).\n", (int)hres);

    cb_val = 0xdeadbeef;
    hres = AtlModuleTerm(&test);
    ok (hres == S_OK, "AtlModuleTerm failed (0x%x).\n", (int)hres);
    ok (cb_val == 0x22, "wrong callback value (0x%x).\n", (int)cb_val);

    test.cbSize = FIELD_OFFSET(_ATL_MODULEW, dwAtlBuildVer);

    hres = AtlModuleInit(&test, NULL, NULL);
    ok (hres == S_OK, "AtlModuleInit failed (0x%x).\n", (int)hres);

    hres = AtlModuleAddTermFunc(&test, term_callback, 0x23);
    ok (hres == S_OK, "AtlModuleAddTermFunc failed (0x%x).\n", (int)hres);

    cb_val = 0xdeadbeef;
    hres = AtlModuleTerm(&test);
    ok (hres == S_OK, "AtlModuleTerm failed (0x%x).\n", (int)hres);
    ok (cb_val == 0xdeadbeef, "wrong callback value (0x%x).\n", (int)cb_val);
}

START_TEST(module)
{
    test_StructSize();
    test_winmodule();
    test_term();
}
