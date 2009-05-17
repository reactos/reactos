/*
 * Unit tests for DPA functions
 *
 * Copyright 2003 Uwe Bonnes
 * Copyright 2005 Felix Nawothnig
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "objidl.h"

#include "wine/test.h"

#define DPAM_NOSORT 0x1
#define DPAM_INSERT 0x4
#define DPAM_DELETE 0x8

typedef struct _ITEMDATA
{
    INT   iPos;
    PVOID pvData;
} ITEMDATA, *LPITEMDATA;

typedef PVOID   (CALLBACK *PFNDPAMERGE)(UINT,PVOID,PVOID,LPARAM);
typedef HRESULT (CALLBACK *PFNDPASTM)(LPITEMDATA,IStream*,LPARAM);

static HDPA    (WINAPI *pDPA_Clone)(const HDPA,const HDPA);
static HDPA    (WINAPI *pDPA_Create)(INT);
static HDPA    (WINAPI *pDPA_CreateEx)(INT,HANDLE);
static PVOID   (WINAPI *pDPA_DeleteAllPtrs)(const HDPA);
static PVOID   (WINAPI *pDPA_DeletePtr)(const HDPA,INT);
static BOOL    (WINAPI *pDPA_Destroy)(const HDPA);
static VOID    (WINAPI *pDPA_DestroyCallback)(HDPA,PFNDPAENUMCALLBACK,PVOID);
static VOID    (WINAPI *pDPA_EnumCallback)(HDPA,PFNDPAENUMCALLBACK,PVOID); 
static INT     (WINAPI *pDPA_GetPtr)(const HDPA,INT);
static INT     (WINAPI *pDPA_GetPtrIndex)(const HDPA,PVOID);
static BOOL    (WINAPI *pDPA_Grow)(HDPA,INT);
static INT     (WINAPI *pDPA_InsertPtr)(const HDPA,INT,PVOID);
static HRESULT (WINAPI *pDPA_LoadStream)(HDPA*,PFNDPASTM,IStream*,LPARAM);
static BOOL    (WINAPI *pDPA_Merge)(const HDPA,const HDPA,DWORD,PFNDPACOMPARE,PFNDPAMERGE,LPARAM);
static HRESULT (WINAPI *pDPA_SaveStream)(HDPA,PFNDPASTM,IStream*,LPARAM);
static INT     (WINAPI *pDPA_Search)(HDPA,PVOID,INT,PFNDPACOMPARE,LPARAM,UINT);
static BOOL    (WINAPI *pDPA_SetPtr)(const HDPA,INT,PVOID);
static BOOL    (WINAPI *pDPA_Sort)(const HDPA,PFNDPACOMPARE,LPARAM);

#define COMCTL32_GET_PROC(func, ord) \
  ((p ## func = (PVOID)GetProcAddress(hcomctl32,(LPCSTR)ord)) ? 1 \
   : (trace( #func " not exported\n"), 0)) 

static BOOL InitFunctionPtrs(HMODULE hcomctl32)
{
    /* 4.00+ */
    if(COMCTL32_GET_PROC(DPA_Clone, 331) &&
       COMCTL32_GET_PROC(DPA_Create, 328) &&
       COMCTL32_GET_PROC(DPA_CreateEx, 340) &&
       COMCTL32_GET_PROC(DPA_DeleteAllPtrs, 337) &&
       COMCTL32_GET_PROC(DPA_DeletePtr, 336) &&
       COMCTL32_GET_PROC(DPA_Destroy, 329) &&
       COMCTL32_GET_PROC(DPA_GetPtr, 332) &&
       COMCTL32_GET_PROC(DPA_GetPtrIndex, 333) &&
       COMCTL32_GET_PROC(DPA_Grow, 330) &&
       COMCTL32_GET_PROC(DPA_InsertPtr, 334) &&
       COMCTL32_GET_PROC(DPA_Search, 339) &&
       COMCTL32_GET_PROC(DPA_SetPtr, 335) &&
       COMCTL32_GET_PROC(DPA_Sort, 338))
    {
        /* 4.71+ */
        COMCTL32_GET_PROC(DPA_DestroyCallback, 386) &&
        COMCTL32_GET_PROC(DPA_EnumCallback, 385) &&
        COMCTL32_GET_PROC(DPA_LoadStream, 9) &&
        COMCTL32_GET_PROC(DPA_Merge, 11) &&
        COMCTL32_GET_PROC(DPA_SaveStream, 10);

        return TRUE;
    }

    return FALSE;
}

/* Callbacks */
static INT CALLBACK CB_CmpLT(PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    return p1 < p2 ? -1 : p1 > p2 ? 1 : 0;
}

static INT CALLBACK CB_CmpGT(PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    return p1 > p2 ? -1 : p1 < p2 ? 1 : 0;
}

static PVOID CALLBACK CB_MergeInsertSrc(UINT op, PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    return p1;
}        

static PVOID CALLBACK CB_MergeDeleteOddSrc(UINT op, PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    return ((PCHAR)p2)+1;
}

static INT nEnum;

static INT CALLBACK CB_EnumFirstThree(PVOID pItem, PVOID lp)
{   
    INT i;

    i = pDPA_GetPtrIndex(lp, pItem);
    ok(i == nEnum, "i=%d nEnum=%d\n", i, nEnum);
    nEnum++;
    pDPA_SetPtr(lp, i, (PVOID)7);
    return pItem != (PVOID)3;
}

static HRESULT CALLBACK CB_Save(LPITEMDATA pInfo, IStream *pStm, LPARAM lp)
{
    HRESULT hRes;
    
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    hRes = IStream_Write(pStm, &pInfo->iPos, sizeof(INT), NULL);
    ok(hRes == S_OK, "hRes=0x%x\n", hRes);
    hRes = IStream_Write(pStm, &pInfo->pvData, sizeof(PVOID), NULL);
    ok(hRes == S_OK, "hRes=0x%x\n", hRes);
    return S_OK;
}

static HRESULT CALLBACK CB_Load(LPITEMDATA pInfo, IStream *pStm, LPARAM lp)
{
    HRESULT hRes;
    INT iOldPos;
    
    iOldPos = pInfo->iPos;
    ok(lp == 0xdeadbeef, "lp=%ld\n", lp);
    hRes = IStream_Read(pStm, &pInfo->iPos, sizeof(INT), NULL);
    ok(hRes == S_OK, "hRes=0x%x\n", hRes);
    ok(pInfo->iPos == iOldPos, "iPos=%d iOldPos=%d\n", pInfo->iPos, iOldPos);
    hRes = IStream_Read(pStm, &pInfo->pvData, sizeof(PVOID), NULL);
    ok(hRes == S_OK, "hRes=0x%x\n", hRes);
    return S_OK;
}

static BOOL CheckDPA(HDPA dpa, DWORD dwIn, PDWORD pdwOut)
{
    DWORD dwOut = 0;
    INT i;

    for(i = 0; i < 8;)
    {
        ULONG_PTR ulItem = (ULONG_PTR)pDPA_GetPtr(dpa, i++);
        if(!ulItem) break;
        dwOut = dwOut << 4 | (ulItem & 0xf);
    }
    
    *pdwOut = dwOut;

    if(dwOut != dwIn)
    {
        pDPA_DeleteAllPtrs(dpa);
        
        do
        {
            pDPA_InsertPtr(dpa, 0, (PVOID)(ULONG_PTR)(dwIn & 0xf));
            dwIn >>= 4;
        }
        while(dwIn);
        
        return FALSE;
    }
    
    return TRUE;
}

static void test_dpa(void)
{
    SYSTEM_INFO si;
    HANDLE hHeap;
    HDPA dpa, dpa2, dpa3;
    INT ret, i;
    PVOID p;
    DWORD dw, dw2, dw3;
    HRESULT hRes;
    BOOL rc;
    
    GetSystemInfo(&si);
    hHeap = HeapCreate(0, 1, 2);
    ok(hHeap != NULL, "error=%d\n", GetLastError());
    dpa3 = pDPA_CreateEx(0, hHeap);
    ok(dpa3 != NULL, "\n");
    ret = pDPA_Grow(dpa3, si.dwPageSize + 1);
    todo_wine ok(!ret && GetLastError() == ERROR_NOT_ENOUGH_MEMORY, 
       "ret=%d error=%d\n", ret, GetLastError());
        
    dpa = pDPA_Create(0);
    ok(dpa != NULL, "\n");

    /* Set item with out of bound index */
    ok(pDPA_SetPtr(dpa, 1, (PVOID)6), "\n");
    /* Fill the created gap */
    ok(pDPA_SetPtr(dpa, 0, (PVOID)5), "\n");
    rc=CheckDPA(dpa, 0x56, &dw);
    ok(rc, "dw=0x%x\n", dw);
    
    /* Prepend item */
    ret = pDPA_InsertPtr(dpa, 1, (PVOID)1);
    ok(ret == 1, "ret=%d\n", ret);
    /* Append item using correct index */
    ret = pDPA_InsertPtr(dpa, 3, (PVOID)3);
    ok(ret == 3, "ret=%d\n", ret);
    /* Append item using out of bound index */
    ret = pDPA_InsertPtr(dpa, 5, (PVOID)2);
    ok(ret == 4, "ret=%d\n", ret);
    /* Append item using DPA_APPEND */ 
    ret = pDPA_InsertPtr(dpa, DPA_APPEND, (PVOID)4);
    ok(ret == 5, "ret=%d\n", ret);

    rc=CheckDPA(dpa, 0x516324, &dw);
    ok(rc, "dw=0x%x\n", dw);

    for(i = 1; i <= 6; i++)
    {
        INT j, k;
        k = pDPA_GetPtrIndex(dpa, (PVOID)(INT_PTR)i);
        /* Linear searches should work on unsorted DPAs */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpLT, 0xdeadbeef, 0);
        ok(j == k, "j=%d k=%d\n", j, k);
    }

    /* Sort DPA */
    ok(pDPA_Sort(dpa, CB_CmpGT, 0xdeadbeef), "\n");
    rc=CheckDPA(dpa, 0x654321, &dw);
    ok(rc, "dw=0x%x\n", dw);
    
    /* Clone into a new DPA */
    dpa2 = pDPA_Clone(dpa, NULL);
    ok(dpa2 != NULL, "\n");
    /* The old data should have been preserved */
    rc=CheckDPA(dpa2, 0x654321, &dw2);
    ok(rc, "dw=0x%x\n", dw2);
    ok(pDPA_Sort(dpa, CB_CmpLT, 0xdeadbeef), "\n");
    
    /* Test if the DPA itself was really copied */
    rc=CheckDPA(dpa,  0x123456, &dw);
    ok(rc, "dw=0x%x\n",  dw );
    rc=CheckDPA(dpa2, 0x654321, &dw2);
    ok(rc, "dw2=0x%x\n", dw2);

    /* Clone into an old DPA */
    p = NULL; SetLastError(ERROR_SUCCESS);
    p = pDPA_Clone(dpa, dpa3);
    ok(p == dpa3, "p=%p\n", p);
    rc=CheckDPA(dpa3, 0x123456, &dw3);
    ok(rc, "dw3=0x%x\n", dw3);

    for(i = 1; i <= 6; i++)
    {
        INT j;

        /* The array is in order so ptr == index+1 */
        j = pDPA_GetPtrIndex(dpa, (PVOID)(INT_PTR)i);
        ok(j+1 == i, "j=%d i=%d\n", j, i);
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpLT, 0xdeadbeef, DPAS_SORTED);
        ok(j+1 == i, "j=%d i=%d\n", j, i);

        /* Linear searches respect iStart ... */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, i+1, CB_CmpLT, 0xdeadbeef, 0);
        ok(j == DPA_ERR, "j=%d\n", j);
        /* ... but for a binary search it's ignored */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, i+1, CB_CmpLT, 0xdeadbeef, DPAS_SORTED);
        todo_wine ok(j+1 == i, "j=%d i=%d\n", j, i);
    }
    
    /* Try to get the index of a nonexistent item */
    i = pDPA_GetPtrIndex(dpa, (PVOID)7);
    ok(i == DPA_ERR, "i=%d\n", i);
    
    /* Try to delete out of bound indexes */
    p = pDPA_DeletePtr(dpa, -1);
    ok(p == NULL, "p=%p\n", p);
    p = pDPA_DeletePtr(dpa, 6);
    ok(p == NULL, "p=%p\n", p);

    /* Delete the third item */
    p = pDPA_DeletePtr(dpa, 2);
    ok(p == (PVOID)3, "p=%p\n", p);
    rc=CheckDPA(dpa, 0x12456, &dw);
    ok(rc, "dw=0x%x\n", dw);

    /* Check where to re-insert the deleted item */
    i = pDPA_Search(dpa, (PVOID)3, 0, 
                    CB_CmpLT, 0xdeadbeef, DPAS_SORTED|DPAS_INSERTAFTER);
    ok(i == 2, "i=%d\n", i);
    /* DPAS_INSERTBEFORE works just like DPAS_INSERTAFTER */
    i = pDPA_Search(dpa, (PVOID)3, 0,
                    CB_CmpLT, 0xdeadbeef, DPAS_SORTED|DPAS_INSERTBEFORE);
    ok(i == 2, "i=%d\n", i);
    /* without DPAS_INSERTBEFORE/AFTER */
    i = pDPA_Search(dpa, (PVOID)3, 0,
                    CB_CmpLT, 0xdeadbeef, DPAS_SORTED);
    ok(i == -1, "i=%d\n", i);

    /* Re-insert the item */
    ret = pDPA_InsertPtr(dpa, 2, (PVOID)3);
    ok(ret == 2, "ret=%d i=%d\n", ret, 2);
    rc=CheckDPA(dpa, 0x123456, &dw);
    ok(rc, "dw=0x%x\n", dw);
    
    /* When doing a binary search while claiming reverse order all indexes
     * should be bogus */
    for(i = 0; i < 6; i++)
    {
        INT j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpGT, 0xdeadbeef,
                            DPAS_SORTED|DPAS_INSERTBEFORE);
        ok(j != i, "i=%d\n", i);
    }

    if(pDPA_Merge)
    {
        /* Delete all even entries from dpa */
        p = pDPA_DeletePtr(dpa, 1);
        p = pDPA_DeletePtr(dpa, 2);
        p = pDPA_DeletePtr(dpa, 3);
        rc=CheckDPA(dpa, 0x135, &dw);
        ok(rc, "dw=0x%x\n", dw);
    
        /* Delete all odd entries from dpa2 */
        pDPA_Merge(dpa2, dpa, DPAM_DELETE, 
                   CB_CmpLT, CB_MergeDeleteOddSrc, 0xdeadbeef);
        todo_wine
        {
            rc=CheckDPA(dpa2, 0x246, &dw2);
            ok(rc, "dw=0x%x\n", dw2);
        }
    
        /* Merge dpa3 into dpa2 and dpa */
        pDPA_Merge(dpa, dpa3, DPAM_INSERT|DPAM_NOSORT, 
                   CB_CmpLT, CB_MergeInsertSrc, 0xdeadbeef);
        pDPA_Merge(dpa2, dpa3, DPAM_INSERT|DPAM_NOSORT, 
                   CB_CmpLT, CB_MergeInsertSrc, 0xdeadbeef);
    
        rc=CheckDPA(dpa,  0x123456, &dw);
        ok(rc, "dw=0x%x\n",  dw);
        rc=CheckDPA(dpa2, 0x123456, &dw2);
        ok(rc ||
           broken(!rc), /* win98 */
           "dw2=0x%x\n", dw2);
        rc=CheckDPA(dpa3, 0x123456, &dw3);
        ok(rc, "dw3=0x%x\n", dw3);
    }

    if(pDPA_EnumCallback)
    {
        nEnum = 0;
        pDPA_EnumCallback(dpa2, CB_EnumFirstThree, dpa2);
        rc=CheckDPA(dpa2, 0x777456, &dw2);
        ok(rc, "dw=0x%x\n", dw2);
        ok(nEnum == 3, "nEnum=%d\n", nEnum);
    }
    
    /* Setting item with huge index should work */
    ok(pDPA_SetPtr(dpa2, 0x12345, (PVOID)0xdeadbeef), "\n");
    ret = pDPA_GetPtrIndex(dpa2, (PVOID)0xdeadbeef);
    ok(ret == 0x12345, "ret=%d\n", ret);
          
    pDPA_DeleteAllPtrs(dpa2);
    rc=CheckDPA(dpa2, 0, &dw2);
    ok(rc, "dw2=0x%x\n", dw2);
    pDPA_Destroy(dpa2);

    if(pDPA_DestroyCallback)
    {
        nEnum = 0;
        pDPA_DestroyCallback(dpa3, CB_EnumFirstThree, dpa3);
        ok(nEnum == 3, "nEnum=%d\n", nEnum);
    }
    else pDPA_Destroy(dpa3);

    if(!pDPA_SaveStream)
        goto skip_stream_tests;

    hRes = CoInitialize(NULL);
    if(hRes == S_OK)
    {
        static const WCHAR szStg[] = { 'S','t','g',0 };
        IStorage* pStg = NULL;
        IStream* pStm = NULL;
        LARGE_INTEGER liZero;
        DWORD dwMode;
        liZero.QuadPart = 0;

        dwMode = STGM_DIRECT|STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
        hRes = StgCreateDocfile(NULL, dwMode|STGM_DELETEONRELEASE, 0, &pStg);
        ok(hRes == S_OK, "hRes=0x%x\n", hRes);

        hRes = IStorage_CreateStream(pStg, szStg, dwMode, 0, 0, &pStm);
        ok(hRes == S_OK, "hRes=0x%x\n", hRes);

        hRes = pDPA_SaveStream(dpa, CB_Save, pStm, 0xdeadbeef);
        todo_wine ok(hRes == S_OK, "hRes=0x%x\n", hRes);
        pDPA_Destroy(dpa);
        
        hRes = IStream_Seek(pStm, liZero, STREAM_SEEK_SET, NULL);
        ok(hRes == S_OK, "hRes=0x%x\n", hRes);
        hRes = pDPA_LoadStream(&dpa, CB_Load, pStm, 0xdeadbeef);
        todo_wine
        {
            ok(hRes == S_OK, "hRes=0x%x\n", hRes);
            rc=CheckDPA(dpa, 0x123456, &dw);
            ok(rc, "dw=0x%x\n", dw);
        }

        ret = IStream_Release(pStm);
        ok(!ret, "ret=%d\n", ret);
	
        ret = IStorage_Release(pStg);
        ok(!ret, "ret=%d\n", ret);

        CoUninitialize();
    }
    else ok(0, "hResult: %d\n", hRes);

skip_stream_tests:
    pDPA_Destroy(dpa);
}

START_TEST(dpa)
{
    HMODULE hcomctl32;

    hcomctl32 = GetModuleHandleA("comctl32.dll");

    if(InitFunctionPtrs(hcomctl32))
        test_dpa();
    else
        win_skip("Needed functions are not available\n");
}
