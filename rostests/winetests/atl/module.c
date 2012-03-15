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

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <wtypes.h>
#include <olectl.h>
#include <ocidl.h>

struct _ATL_OBJMAP_ENTRYW;
struct _AtlCreateWndData;
struct _ATL_TERMFUNC_ELEM;

struct _ATL_MODULEW
{
    UINT cbSize;
    HINSTANCE m_hInst;
    HINSTANCE m_hInstResource;
    HINSTANCE m_hInstTypeLib;
    struct _ATL_OBJMAP_ENTRYW* m_pObjMap;
    LONG m_nLockCnt;
    HANDLE m_hHeap;
    union
    {
        CRITICAL_SECTION m_csTypeInfoHolder;
        CRITICAL_SECTION m_csStaticDataInit;
    } u;
    CRITICAL_SECTION m_csWindowCreate;
    CRITICAL_SECTION m_csObjMap;

    DWORD dwAtlBuildVer;
    struct _AtlCreateWndData* m_pCreateWndList;
    BOOL m_bDestroyHeap;
    GUID* pguidVer;
    DWORD m_dwHeaps;
    HANDLE* m_phHeaps;
    int m_nHeap;
    struct _ATL_TERMFUNC_ELEM* m_pTermFuncs;
};

HRESULT WINAPI AtlModuleInit(struct _ATL_MODULEW* pM, struct _ATL_OBJMAP_ENTRYW* p, HINSTANCE h);

#define MAXSIZE 512
static void test_StructSize(void)
{
        struct _ATL_MODULEW  *tst;
        HRESULT              hres;
	int i;

        tst = HeapAlloc (GetProcessHeap(),HEAP_ZERO_MEMORY,MAXSIZE);

	for (i=0;i<MAXSIZE;i++) {
		tst->cbSize = i;
		hres = AtlModuleInit(tst, NULL, NULL);

		switch (i)  {
                case FIELD_OFFSET( struct _ATL_MODULEW, dwAtlBuildVer ):
		case sizeof(struct _ATL_MODULEW):
#ifdef _WIN64
		case sizeof(struct _ATL_MODULEW) + sizeof(void *):
#endif
			ok (hres == S_OK, "AtlModuleInit with %d failed (0x%x).\n", i, (int)hres);
			break;
		default:
			ok (FAILED(hres) ||
                            broken((i > FIELD_OFFSET( struct _ATL_MODULEW, dwAtlBuildVer )) && (hres == S_OK)), /* Win95 */
                            "AtlModuleInit with %d succeeded? (0x%x).\n", i, (int)hres);
			break;
		}
	}

        HeapFree (GetProcessHeap(), 0, tst);
}

START_TEST(module)
{
        test_StructSize();
}
