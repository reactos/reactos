/*
 * Unit tests for DPA functions
 *
 * Copyright 2003 Uwe Bonnes
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"

#include "wine/test.h"

static HDPA (WINAPI *pDPA_Create)(int);
static BOOL (WINAPI *pDPA_Grow)(const HDPA hdpa, INT nGrow);
static BOOL (WINAPI *pDPA_Destroy)(const HDPA hdpa);
static BOOL (WINAPI *pDPA_SetPtr)(const HDPA hdpa, INT i, LPVOID p);

static INT CALLBACK dpa_strcmp(LPVOID pvstr1, LPVOID pvstr2, LPARAM flags)
{
  LPCSTR str1 = (LPCSTR)pvstr1;
  LPCSTR str2 = (LPCSTR)pvstr2;

  return lstrcmpA (str1, str2);
}

void DPA_test()
{
  HDPA dpa_ret;
  INT  int_ret;
  CHAR test_str0[]="test0";

  if (!pDPA_Create)
      return;

  dpa_ret = pDPA_Create(0);
  ok((dpa_ret !=0), "DPA_Create failed\n");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED);
  ok((int_ret == -1), "DPA_Search found invalid item\n");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED|DPAS_INSERTBEFORE);
  ok((int_ret == 0), "DPA_Search proposed bad item\n");
  int_ret = DPA_Search(dpa_ret,test_str0,0, dpa_strcmp,0, DPAS_SORTED|DPAS_INSERTAFTER);
  ok((int_ret == 0), "DPA_Search proposed bad item\n");
  int_ret = pDPA_Grow(dpa_ret,0);
  ok(int_ret != 0, "DPA_Grow failed\n");
  int_ret = pDPA_SetPtr(dpa_ret, 0, (void*)0xdeadbeef);
  ok(int_ret != 0, "DPA_SetPtr failed\n");
  int_ret = pDPA_Destroy(dpa_ret);
  ok(int_ret != 0, "DPA_Destory failed\n");
}

START_TEST(dpa)
{
    HMODULE hdll;

    hdll=GetModuleHandleA("comctl32.dll");
    pDPA_Create=(void*)GetProcAddress(hdll,(LPCSTR)328);
    pDPA_Destroy=(void*)GetProcAddress(hdll,(LPCSTR)329);
    pDPA_Grow=(void*)GetProcAddress(hdll,(LPCSTR)330);
    pDPA_SetPtr=(void*)GetProcAddress(hdll,(LPCSTR)335);

    DPA_test();
}
