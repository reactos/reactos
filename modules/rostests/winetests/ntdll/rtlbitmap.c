/* Unit test suite for Rtl bitmap functions
 *
 * Copyright 2002 Jon Griffiths
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
 *
 * NOTES
 * We use function pointers here as some of the bitmap functions exist only
 * in later versions of ntdll.
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

#ifdef __WINE_WINTERNL_H

/* Function ptrs for ordinal calls */
static HMODULE hntdll = 0;
static VOID (WINAPI *pRtlInitializeBitMap)(PRTL_BITMAP,LPBYTE,ULONG);
static VOID (WINAPI *pRtlSetAllBits)(PRTL_BITMAP);
static VOID (WINAPI *pRtlClearAllBits)(PRTL_BITMAP);
static VOID (WINAPI *pRtlSetBits)(PRTL_BITMAP,ULONG,ULONG);
static VOID (WINAPI *pRtlClearBits)(PRTL_BITMAP,ULONG,ULONG);
static BOOLEAN (WINAPI *pRtlAreBitsSet)(PRTL_BITMAP,ULONG,ULONG);
static BOOLEAN (WINAPI *pRtlAreBitsClear)(PRTL_BITMAP,ULONG,ULONG);
static ULONG (WINAPI *pRtlFindSetBitsAndClear)(PRTL_BITMAP,ULONG,ULONG);
static ULONG (WINAPI *pRtlFindClearBitsAndSet)(PRTL_BITMAP,ULONG,ULONG);
static CCHAR (WINAPI *pRtlFindMostSignificantBit)(ULONGLONG);
static CCHAR (WINAPI *pRtlFindLeastSignificantBit)(ULONGLONG);
static ULONG (WINAPI *pRtlFindSetRuns)(PRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
static ULONG (WINAPI *pRtlFindClearRuns)(PRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
static ULONG (WINAPI *pRtlFindNextForwardRunSet)(PRTL_BITMAP,ULONG,PULONG);
static ULONG (WINAPI *pRtlFindNextForwardRunClear)(PRTL_BITMAP,ULONG,PULONG);
static ULONG (WINAPI *pRtlNumberOfSetBits)(PRTL_BITMAP);
static ULONG (WINAPI *pRtlNumberOfClearBits)(PRTL_BITMAP);
static ULONG (WINAPI *pRtlFindLongestRunSet)(PRTL_BITMAP,PULONG);
static ULONG (WINAPI *pRtlFindLongestRunClear)(PRTL_BITMAP,PULONG);

static BYTE buff[256];
static RTL_BITMAP bm;

static void InitFunctionPtrs(void)
{
  hntdll = LoadLibraryA("ntdll.dll");
  ok(hntdll != 0, "LoadLibrary failed\n");
  if (hntdll)
  {
    pRtlInitializeBitMap = (void *)GetProcAddress(hntdll, "RtlInitializeBitMap");
    pRtlSetAllBits = (void *)GetProcAddress(hntdll, "RtlSetAllBits");
    pRtlClearAllBits = (void *)GetProcAddress(hntdll, "RtlClearAllBits");
    pRtlSetBits = (void *)GetProcAddress(hntdll, "RtlSetBits");
    pRtlClearBits = (void *)GetProcAddress(hntdll, "RtlClearBits");
    pRtlAreBitsSet = (void *)GetProcAddress(hntdll, "RtlAreBitsSet");
    pRtlAreBitsClear = (void *)GetProcAddress(hntdll, "RtlAreBitsClear");
    pRtlNumberOfSetBits = (void *)GetProcAddress(hntdll, "RtlNumberOfSetBits");
    pRtlNumberOfClearBits = (void *)GetProcAddress(hntdll, "RtlNumberOfClearBits");
    pRtlFindSetBitsAndClear = (void *)GetProcAddress(hntdll, "RtlFindSetBitsAndClear");
    pRtlFindClearBitsAndSet = (void *)GetProcAddress(hntdll, "RtlFindClearBitsAndSet");
    pRtlFindMostSignificantBit = (void *)GetProcAddress(hntdll, "RtlFindMostSignificantBit");
    pRtlFindLeastSignificantBit = (void *)GetProcAddress(hntdll, "RtlFindLeastSignificantBit");
    pRtlFindSetRuns = (void *)GetProcAddress(hntdll, "RtlFindSetRuns");
    pRtlFindClearRuns = (void *)GetProcAddress(hntdll, "RtlFindClearRuns");
    pRtlFindNextForwardRunSet = (void *)GetProcAddress(hntdll, "RtlFindNextForwardRunSet");
    pRtlFindNextForwardRunClear = (void *)GetProcAddress(hntdll, "RtlFindNextForwardRunClear");
    pRtlFindLongestRunSet = (void *)GetProcAddress(hntdll, "RtlFindLongestRunSet");
    pRtlFindLongestRunClear = (void *)GetProcAddress(hntdll, "RtlFindLongestRunClear");
  }
}

static void test_RtlInitializeBitMap(void)
{
  bm.SizeOfBitMap = 0;
  bm.Buffer = 0;

  memset(buff, 0, sizeof(buff));
  buff[0] = 77; /* Check buffer is not written to during init */
  buff[79] = 77;

  pRtlInitializeBitMap(&bm, buff, 800);
  ok(bm.SizeOfBitMap == 800, "size uninitialised\n");
  ok(bm.Buffer == (PULONG)buff,"buffer uninitialised\n");
  ok(buff[0] == 77 && buff[79] == 77, "wrote to buffer\n");
}

static void test_RtlSetAllBits(void)
{
  if (!pRtlSetAllBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, 1);

  pRtlSetAllBits(&bm);
  ok(buff[0] == 0xff && buff[1] == 0xff && buff[2] == 0xff &&
     buff[3] == 0xff, "didn't round up size\n");
  ok(buff[4] == 0, "set more than rounded size\n");
}

static void test_RtlClearAllBits(void)
{
  if (!pRtlClearAllBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, 1);

  pRtlClearAllBits(&bm);
  ok(!buff[0] && !buff[1] && !buff[2] && !buff[3], "didn't round up size\n");
  ok(buff[4] == 0xff, "cleared more than rounded size\n");
}

static void test_RtlSetBits(void)
{
  if (!pRtlSetBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlSetBits(&bm, 0, 1);
  ok(buff[0] == 1, "didn't set 1st bit\n");

  buff[0] = 0;
  pRtlSetBits(&bm, 7, 2);
  ok(buff[0] == 0x80 && buff[1] == 1, "didn't span w/len < 8\n");

  buff[0] = buff[1] = 0;
  pRtlSetBits(&bm, 7, 10);
  ok(buff[0] == 0x80 && buff[1] == 0xff && buff[2] == 1, "didn't span w/len > 8\n");

  buff[0] = buff[1] = buff[2] = 0;
  pRtlSetBits(&bm, 0, 8); /* 1st byte */
  ok(buff[0] == 0xff, "didn't set all bits\n");
  ok(!buff[1], "set too many bits\n");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1); /* last bit */
  ok(buff[sizeof(buff)-1] == 0x80, "didn't set last bit\n");
}

static void test_RtlClearBits(void)
{
  if (!pRtlClearBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlClearBits(&bm, 0, 1);
  ok(buff[0] == 0xfe, "didn't clear 1st bit\n");

  buff[0] = 0xff;
  pRtlClearBits(&bm, 7, 2);
  ok(buff[0] == 0x7f && buff[1] == 0xfe, "didn't span w/len < 8\n");

  buff[0] = buff[1] = 0xff;
  pRtlClearBits(&bm, 7, 10);
  ok(buff[0] == 0x7f && buff[1] == 0 && buff[2] == 0xfe, "didn't span w/len > 8\n");

  buff[0] = buff[1] = buff[2] = 0xff;
  pRtlClearBits(&bm, 0, 8);  /* 1st byte */
  ok(!buff[0], "didn't clear all bits\n");
  ok(buff[1] == 0xff, "cleared too many bits\n");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1);
  ok(buff[sizeof(buff)-1] == 0x7f, "didn't set last bit\n");
}

static void test_RtlCheckBit(void)
{
  BOOLEAN bRet;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);
  pRtlSetBits(&bm, 0, 1);
  pRtlSetBits(&bm, 7, 2);
  pRtlSetBits(&bm, sizeof(buff)*8-1, 1);

  bRet = RtlCheckBit(&bm, 0);
  ok (bRet, "didn't find set bit\n");
  bRet = RtlCheckBit(&bm, 7);
  ok (bRet, "didn't find set bit\n");
  bRet = RtlCheckBit(&bm, 8);
  ok (bRet, "didn't find set bit\n");
  bRet = RtlCheckBit(&bm, sizeof(buff)*8-1);
  ok (bRet, "didn't find set bit\n");
  bRet = RtlCheckBit(&bm, 1);
  ok (!bRet, "found non set bit\n");
  bRet = RtlCheckBit(&bm, sizeof(buff)*8-2);
  ok (!bRet, "found non set bit\n");
}

static void test_RtlAreBitsSet(void)
{
  BOOLEAN bRet;

  if (!pRtlAreBitsSet)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  bRet = pRtlAreBitsSet(&bm, 0, 1);
  ok (!bRet, "found set bits after init\n");

  pRtlSetBits(&bm, 0, 1);
  bRet = pRtlAreBitsSet(&bm, 0, 1);
  ok (bRet, "didn't find set bits\n");

  buff[0] = 0;
  pRtlSetBits(&bm, 7, 2);
  bRet = pRtlAreBitsSet(&bm, 7, 2);
  ok(bRet, "didn't find w/len < 8\n");
  bRet = pRtlAreBitsSet(&bm, 6, 3);
  ok(!bRet, "found non set bit\n");
  bRet = pRtlAreBitsSet(&bm, 7, 3);
  ok(!bRet, "found non set bit\n");

  buff[0] = buff[1] = 0;
  pRtlSetBits(&bm, 7, 10);
  bRet = pRtlAreBitsSet(&bm, 7, 10);
  ok(bRet, "didn't find w/len < 8\n");
  bRet = pRtlAreBitsSet(&bm, 6, 11);
  ok(!bRet, "found non set bit\n");
  bRet = pRtlAreBitsSet(&bm, 7, 11);
  ok(!bRet, "found non set bit\n");

  buff[0] = buff[1] = buff[2] = 0;
  pRtlSetBits(&bm, 0, 8); /* 1st byte */
  bRet = pRtlAreBitsSet(&bm, 0, 8);
  ok(bRet, "didn't find whole byte\n");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1);
  bRet = pRtlAreBitsSet(&bm, sizeof(buff)*8-1, 1);
  ok(bRet, "didn't find last bit\n");
}

static void test_RtlAreBitsClear(void)
{
  BOOLEAN bRet;

  if (!pRtlAreBitsClear)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  bRet = pRtlAreBitsClear(&bm, 0, 1);
  ok (!bRet, "found clear bits after init\n");

  pRtlClearBits(&bm, 0, 1);
  bRet = pRtlAreBitsClear(&bm, 0, 1);
  ok (bRet, "didn't find set bits\n");

  buff[0] = 0xff;
  pRtlClearBits(&bm, 7, 2);
  bRet = pRtlAreBitsClear(&bm, 7, 2);
  ok(bRet, "didn't find w/len < 8\n");
  bRet = pRtlAreBitsClear(&bm, 6, 3);
  ok(!bRet, "found non clear bit\n");
  bRet = pRtlAreBitsClear(&bm, 7, 3);
  ok(!bRet, "found non clear bit\n");

  buff[0] = buff[1] = 0xff;
  pRtlClearBits(&bm, 7, 10);
  bRet = pRtlAreBitsClear(&bm, 7, 10);
  ok(bRet, "didn't find w/len < 8\n");
  bRet = pRtlAreBitsClear(&bm, 6, 11);
  ok(!bRet, "found non clear bit\n");
  bRet = pRtlAreBitsClear(&bm, 7, 11);
  ok(!bRet, "found non clear bit\n");

  buff[0] = buff[1] = buff[2] = 0xff;
  pRtlClearBits(&bm, 0, 8); /* 1st byte */
  bRet = pRtlAreBitsClear(&bm, 0, 8);
  ok(bRet, "didn't find whole byte\n");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1);
  bRet = pRtlAreBitsClear(&bm, sizeof(buff)*8-1, 1);
  ok(bRet, "didn't find last bit\n");
}

static void test_RtlNumberOfSetBits(void)
{
  ULONG ulCount;

  if (!pRtlNumberOfSetBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 0, "set bits after init\n");

  pRtlSetBits(&bm, 0, 1); /* Set 1st bit */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 1, "count wrong\n");

  pRtlSetBits(&bm, 7, 8); /* 8 more, spanning bytes 1-2 */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1, "count wrong\n");

  pRtlSetBits(&bm, 17, 33); /* 33 more crossing ULONG boundary */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1+33, "count wrong\n");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1); /* Set last bit */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1+33+1, "count wrong\n");
}

static void test_RtlNumberOfClearBits(void)
{
  ULONG ulCount;

  if (!pRtlNumberOfClearBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 0, "cleared bits after init\n");

  pRtlClearBits(&bm, 0, 1); /* Set 1st bit */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 1, "count wrong\n");

  pRtlClearBits(&bm, 7, 8); /* 8 more, spanning bytes 1-2 */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1, "count wrong\n");

  pRtlClearBits(&bm, 17, 33); /* 33 more crossing ULONG boundary */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1+33, "count wrong\n");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1); /* Set last bit */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1+33+1, "count wrong\n");
}

/* Note: this tests RtlFindSetBits also */
static void test_RtlFindSetBitsAndClear(void)
{
  BOOLEAN bRet;
  ULONG ulPos;

  if (!pRtlFindSetBitsAndClear)
    return;

  memset(buff, 0, sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlSetBits(&bm, 0, 32);
  ulPos = pRtlFindSetBitsAndClear(&bm, 32, 0);
  ok (ulPos == 0, "didn't find bits\n");
  if(ulPos == 0)
  {
    bRet = pRtlAreBitsClear(&bm, 0, 32);
    ok (bRet, "found but didn't clear\n");
  }

  memset(buff, 0 , sizeof(buff));
  pRtlSetBits(&bm, 40, 77);
  ulPos = pRtlFindSetBitsAndClear(&bm, 77, 0);
  ok (ulPos == 40, "didn't find bits\n");
  if(ulPos == 40)
  {
    bRet = pRtlAreBitsClear(&bm, 40, 77);
    ok (bRet, "found but didn't clear\n");
  }
}

/* Note: this tests RtlFindClearBits also */
static void test_RtlFindClearBitsAndSet(void)
{
  BOOLEAN bRet;
  ULONG ulPos;

  if (!pRtlFindClearBitsAndSet)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0xff, sizeof(buff));
  pRtlSetBits(&bm, 0, 32);
  ulPos = pRtlFindSetBitsAndClear(&bm, 32, 0);
  ok (ulPos == 0, "didn't find bits\n");
  if(ulPos == 0)
  {
      bRet = pRtlAreBitsClear(&bm, 0, 32);
      ok (bRet, "found but didn't clear\n");
  }

  memset(buff, 0xff , sizeof(buff));
  pRtlClearBits(&bm, 40, 77);
  ulPos = pRtlFindClearBitsAndSet(&bm, 77, 50);
  ok (ulPos == 40, "didn't find bits\n");
  if(ulPos == 40)
  {
    bRet = pRtlAreBitsSet(&bm, 40, 77);
    ok (bRet, "found but didn't set\n");
  }
}

static void test_RtlFindMostSignificantBit(void)
{
  int i;
  signed char cPos;
  ULONGLONG ulLong;

  if (!pRtlFindMostSignificantBit)
    return;

  for (i = 0; i < 64; i++)
  {
    ulLong = 1ul;
    ulLong <<= i;

    cPos = pRtlFindMostSignificantBit(ulLong);
    ok (cPos == i, "didn't find MSB 0x%s %d %d\n",
        wine_dbgstr_longlong(ulLong ), i, cPos);

    /* Set all bits lower than bit i */
    ulLong = ((ulLong - 1) << 1) | 1;

    cPos = pRtlFindMostSignificantBit(ulLong);
    ok (cPos == i, "didn't find MSB 0x%s %d %d\n",
        wine_dbgstr_longlong(ulLong ), i, cPos);
  }
  cPos = pRtlFindMostSignificantBit(0);
  ok (cPos == -1, "found bit when not set\n");
}

static void test_RtlFindLeastSignificantBit(void)
{
  int i;
  signed char cPos;
  ULONGLONG ulLong;

  if (!pRtlFindLeastSignificantBit)
    return;

  for (i = 0; i < 64; i++)
  {
    ulLong = (ULONGLONG)1 << i;

    cPos = pRtlFindLeastSignificantBit(ulLong);
    ok (cPos == i, "didn't find LSB 0x%s %d %d\n",
        wine_dbgstr_longlong(ulLong ), i, cPos);

    ulLong = ~((ULONGLONG)0) << i;

    cPos = pRtlFindLeastSignificantBit(ulLong);
    ok (cPos == i, "didn't find LSB 0x%s %d %d\n",
        wine_dbgstr_longlong(ulLong ), i, cPos);
  }
  cPos = pRtlFindLeastSignificantBit(0);
  ok (cPos == -1, "found bit when not set\n");
}

/* Note: Also tests RtlFindLongestRunSet() */
static void test_RtlFindSetRuns(void)
{
  RTL_BITMAP_RUN runs[16];
  ULONG ulCount;

  if (!pRtlFindSetRuns)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0, sizeof(buff));
  ulCount = pRtlFindSetRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 0, "found set bits in empty bitmap\n");

  memset(runs, 0, sizeof(runs));
  memset(buff, 0xff, sizeof(buff));
  ulCount = pRtlFindSetRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 1, "didn't find set bits\n");
  ok (runs[0].StartingIndex == 0,"bad start\n");
  ok (runs[0].NumberOfBits == sizeof(buff)*8,"bad size\n");

  /* Set up 3 runs */
  memset(runs, 0, sizeof(runs));
  memset(buff, 0, sizeof(buff));
  pRtlSetBits(&bm, 7, 19);
  pRtlSetBits(&bm, 101, 3);
  pRtlSetBits(&bm, 1877, 33);

  /* Get first 2 */
  ulCount = pRtlFindSetRuns(&bm, runs, 2, FALSE);
  ok(ulCount == 2, "RtlFindClearRuns returned %ld, expected 2\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 101,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 101,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits == 19 + 3,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[2].StartingIndex == 0,"found extra run\n");

  /* Get longest 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindSetRuns(&bm, runs, 2, TRUE);
  ok(ulCount == 2, "RtlFindClearRuns returned %ld, expected 2\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 1877,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 1877,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits == 33 + 19,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[2].StartingIndex == 0,"found extra run\n");

  /* Get all 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindSetRuns(&bm, runs, 3, TRUE);
  ok(ulCount == 3, "RtlFindClearRuns returned %ld, expected 3\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 101 ||
      runs[0].StartingIndex == 1877,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 101 ||
      runs[1].StartingIndex == 1877,"bad find\n");
  ok (runs[2].StartingIndex == 7 || runs[2].StartingIndex == 101 ||
      runs[2].StartingIndex == 1877,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits
      + runs[2].NumberOfBits == 19 + 3 + 33,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[1].StartingIndex != runs[2].StartingIndex,"found run twice\n");
  ok (runs[3].StartingIndex == 0,"found extra run\n");

  if (pRtlFindLongestRunSet)
  {
    ULONG ulStart = 0;

    ulCount = pRtlFindLongestRunSet(&bm, &ulStart);
    ok(ulCount == 33 && ulStart == 1877,"didn't find longest %ld %ld\n",ulCount,ulStart);

    memset(buff, 0, sizeof(buff));
    ulCount = pRtlFindLongestRunSet(&bm, &ulStart);
    ok(ulCount == 0,"found longest when none set\n");
  }
}

/* Note: Also tests RtlFindLongestRunClear() */
static void test_RtlFindClearRuns(void)
{
  RTL_BITMAP_RUN runs[16];
  ULONG ulCount;

  if (!pRtlFindClearRuns)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0xff, sizeof(buff));
  ulCount = pRtlFindClearRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 0, "found clear bits in full bitmap\n");

  memset(runs, 0, sizeof(runs));
  memset(buff, 0, sizeof(buff));
  ulCount = pRtlFindClearRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 1, "didn't find clear bits\n");
  ok (runs[0].StartingIndex == 0,"bad start\n");
  ok (runs[0].NumberOfBits == sizeof(buff)*8,"bad size\n");

  /* Set up 3 runs */
  memset(runs, 0, sizeof(runs));
  memset(buff, 0xff, sizeof(buff));
  pRtlClearBits(&bm, 7, 19);
  pRtlClearBits(&bm, 101, 3);
  pRtlClearBits(&bm, 1877, 33);

  /* Get first 2 */
  ulCount = pRtlFindClearRuns(&bm, runs, 2, FALSE);
  ok(ulCount == 2, "RtlFindClearRuns returned %ld, expected 2\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 101,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 101,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits == 19 + 3,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[2].StartingIndex == 0,"found extra run\n");

  /* Get longest 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindClearRuns(&bm, runs, 2, TRUE);
  ok(ulCount == 2, "RtlFindClearRuns returned %ld, expected 2\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 1877,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 1877,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits == 33 + 19,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[2].StartingIndex == 0,"found extra run\n");

  /* Get all 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindClearRuns(&bm, runs, 3, TRUE);
  ok(ulCount == 3, "RtlFindClearRuns returned %ld, expected 3\n", ulCount);
  ok (runs[0].StartingIndex == 7 || runs[0].StartingIndex == 101 ||
      runs[0].StartingIndex == 1877,"bad find\n");
  ok (runs[1].StartingIndex == 7 || runs[1].StartingIndex == 101 ||
      runs[1].StartingIndex == 1877,"bad find\n");
  ok (runs[2].StartingIndex == 7 || runs[2].StartingIndex == 101 ||
      runs[2].StartingIndex == 1877,"bad find\n");
  ok (runs[0].NumberOfBits + runs[1].NumberOfBits
      + runs[2].NumberOfBits == 19 + 3 + 33,"bad size\n");
  ok (runs[0].StartingIndex != runs[1].StartingIndex,"found run twice\n");
  ok (runs[1].StartingIndex != runs[2].StartingIndex,"found run twice\n");
  ok (runs[3].StartingIndex == 0,"found extra run\n");

  if (pRtlFindLongestRunClear)
  {
    ULONG ulStart = 0;

    ulCount = pRtlFindLongestRunClear(&bm, &ulStart);
    ok(ulCount == 33 && ulStart == 1877,"didn't find longest\n");

    memset(buff, 0xff, sizeof(buff));
    ulCount = pRtlFindLongestRunClear(&bm, &ulStart);
    ok(ulCount == 0,"found longest when none clear\n");
  }

}

static void test_RtlFindNextForwardRunSet(void)
{
  BYTE mask[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff };
  ULONG ulStart = 0;
  ULONG ulCount, lpPos;
  if (!pRtlFindNextForwardRunSet)
    return;

  pRtlInitializeBitMap(&bm, mask, 62);
  ulCount = pRtlFindNextForwardRunSet(&bm, ulStart, &lpPos);
  ok(ulCount == 6, "Invalid length of found set run: %ld, expected 6\n", ulCount);
  ok(lpPos == 56, "Invalid position of found set run: %ld, expected 56\n", lpPos);
}

static void test_RtlFindNextForwardRunClear(void)
{
  BYTE mask[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 };
  ULONG ulStart = 0;
  ULONG ulCount, lpPos;
  if (!pRtlFindNextForwardRunClear)
    return;

  pRtlInitializeBitMap(&bm, mask, 62);
  ulCount = pRtlFindNextForwardRunClear(&bm, ulStart, &lpPos);
  ok(ulCount == 6, "Invalid length of found clear run: %ld, expected 6\n", ulCount);
  ok(lpPos == 56, "Invalid position of found clear run: %ld, expected 56\n", lpPos);
}

#endif

START_TEST(rtlbitmap)
{
#ifdef __WINE_WINTERNL_H
  InitFunctionPtrs();

  if (pRtlInitializeBitMap)
  {
    test_RtlInitializeBitMap();
    test_RtlSetAllBits();
    test_RtlClearAllBits();
    test_RtlSetBits();
    test_RtlClearBits();
    test_RtlCheckBit();
    test_RtlAreBitsSet();
    test_RtlAreBitsClear();
    test_RtlNumberOfSetBits();
    test_RtlNumberOfClearBits();
    test_RtlFindSetBitsAndClear();
    test_RtlFindClearBitsAndSet();
    test_RtlFindMostSignificantBit();
    test_RtlFindLeastSignificantBit();
    test_RtlFindSetRuns();
    test_RtlFindClearRuns();
    test_RtlFindNextForwardRunSet();
    test_RtlFindNextForwardRunClear();
  }
#endif
}
