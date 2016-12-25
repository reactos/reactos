/*
 * Unit test suite for cpu functions
 *
 * Copyright 2014 Michael Müller
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

#include "wine/test.h"
#include "winbase.h"
#include "winnls.h"

static BOOL (WINAPI *pGetNumaProcessorNode)(UCHAR, PUCHAR);

static void InitFunctionPointers(void)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    pGetNumaProcessorNode = (void *)GetProcAddress(hkernel32, "GetNumaProcessorNode");
}

static void test_GetNumaProcessorNode(void)
{
    SYSTEM_INFO si;
    UCHAR node;
    BOOL ret;
    int i;

    if (!pGetNumaProcessorNode)
    {
        win_skip("GetNumaProcessorNode() is missing\n");
        return;
    }

    GetSystemInfo(&si);

    for (i = 0; i < 256; i++)
    {
        ret = pGetNumaProcessorNode(i, &node);
        if (i < si.dwNumberOfProcessors)
        {
            ok(ret, "expected TRUE, got FALSE for processor %d\n", i);
            ok(node != 0xFF, "expected node != 0xFF, but got 0xFF\n");
        }
        else
        {
            ok(!ret, "expected FALSE, got TRUE for processor %d\n", i);
            ok(node == 0xFF, "expected node == 0xFF, but got %x\n", node);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        }
    }

    /* crashes on windows */
    if (0)
    {
        ok(!pGetNumaProcessorNode(0, NULL), "expected return value FALSE, got TRUE\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }
}

START_TEST(cpu)
{
    InitFunctionPointers();
    test_GetNumaProcessorNode();
}
