/*
 * Unit test suite for debug functions.
 *
 * Copyright 2004 Patrik Stridvall
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

#include "windef.h"
#include "winbase.h"
#include "winnt.h"

#include "crtdbg.h"

#include "wine/test.h"

/**********************************************************************/

static void * (__cdecl *pMSVCRTD_operator_new_dbg)(size_t, int, const char *, int) = NULL;

/* Some exports are only available in later versions */
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hModule,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init_functions(void)
{
  HMODULE hModule = LoadLibraryA("msvcrtd.dll");

  if (!hModule) {
    trace("LoadLibraryA failed to load msvcrtd.dll with GLE=%d\n", GetLastError());
    return FALSE;
  }

  if (sizeof(void *) > sizeof(int))  /* 64-bit has a different mangled name */
      SET(pMSVCRTD_operator_new_dbg, "??2@YAPEAX_KHPEBDH@Z");
  else
      SET(pMSVCRTD_operator_new_dbg, "??2@YAPAXIHPBDH@Z");

  if (pMSVCRTD_operator_new_dbg == NULL)
    return FALSE;

  return TRUE;
}

/**********************************************************************/

static void test_new(void)
{
  void *mem;

  mem = pMSVCRTD_operator_new_dbg(42, _NORMAL_BLOCK, __FILE__, __LINE__);
  ok(mem != NULL, "memory not allocated\n");
  HeapFree(GetProcessHeap(), 0, mem);
}

/**********************************************************************/

START_TEST(debug)
{
  if (!init_functions()) 
    return;

  test_new();
}
