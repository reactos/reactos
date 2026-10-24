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
#include "v6util.h"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %ld, got %ld\n", expected, got);
}

typedef struct _STREAMDATA
{
    DWORD dwSize;
    DWORD dwData2;
    DWORD dwItems;
} STREAMDATA, *PSTREAMDATA;

static HDPA    (WINAPI *pDPA_Clone)(const HDPA,HDPA);
static HDPA    (WINAPI *pDPA_Create)(INT);
static HDPA    (WINAPI *pDPA_CreateEx)(INT,HANDLE);
static PVOID   (WINAPI *pDPA_DeleteAllPtrs)(HDPA);
static PVOID   (WINAPI *pDPA_DeletePtr)(HDPA,INT);
static BOOL    (WINAPI *pDPA_Destroy)(HDPA);
static VOID    (WINAPI *pDPA_DestroyCallback)(HDPA,PFNDPAENUMCALLBACK,PVOID);
static VOID    (WINAPI *pDPA_EnumCallback)(HDPA,PFNDPAENUMCALLBACK,PVOID); 
static PVOID   (WINAPI *pDPA_GetPtr)(HDPA,INT_PTR);
static INT     (WINAPI *pDPA_GetPtrIndex)(HDPA,PVOID);
static BOOL    (WINAPI *pDPA_Grow)(HDPA,INT);
static INT     (WINAPI *pDPA_InsertPtr)(HDPA,INT,PVOID);
static HRESULT (WINAPI *pDPA_LoadStream)(HDPA*,PFNDPASTREAM,IStream*,LPVOID);
static BOOL    (WINAPI *pDPA_Merge)(HDPA,HDPA,DWORD,PFNDPACOMPARE,PFNDPAMERGE,LPARAM);
static HRESULT (WINAPI *pDPA_SaveStream)(HDPA,PFNDPASTREAM,IStream*,LPVOID);
static INT     (WINAPI *pDPA_Search)(HDPA,PVOID,INT,PFNDPACOMPARE,LPARAM,UINT);
static BOOL    (WINAPI *pDPA_SetPtr)(HDPA,INT,PVOID);
static BOOL    (WINAPI *pDPA_Sort)(HDPA,PFNDPACOMPARE,LPARAM);

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X2(f, ord) p##f = (void*)GetProcAddress(hComCtl32, (const char *)ord);
    /* 4.00+ */
    X2(DPA_Clone, 331);
    X2(DPA_Create, 328);
    X2(DPA_CreateEx, 340);
    X2(DPA_DeleteAllPtrs, 337);
    X2(DPA_DeletePtr, 336);
    X2(DPA_Destroy, 329);
    X2(DPA_GetPtr, 332);
    X2(DPA_GetPtrIndex, 333);
    X2(DPA_Grow, 330);
    X2(DPA_InsertPtr, 334);
    X2(DPA_Search, 339);
    X2(DPA_SetPtr, 335);
    X2(DPA_Sort, 338);

    /* 4.71+ */
    X2(DPA_DestroyCallback, 386);
    X2(DPA_EnumCallback, 385);
    X2(DPA_LoadStream, 9);
    X2(DPA_Merge, 11);
    X2(DPA_SaveStream, 10);
#undef X2
}

/* Callbacks */
static INT CALLBACK CB_CmpLT(PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0x1abe11ed, "lp=%Id\n", lp);
    return p1 < p2 ? -1 : p1 > p2 ? 1 : 0;
}

static INT CALLBACK CB_CmpGT(PVOID p1, PVOID p2, LPARAM lp)
{
    ok(lp == 0x1abe11ed, "lp=%Id\n", lp);
    return p1 > p2 ? -1 : p1 < p2 ? 1 : 0;
}

/* merge callback messages counter
   DPAMM_MERGE     1
   DPAMM_DELETE    2
   DPAMM_INSERT    3  */
static INT nMessages[4];

static PVOID CALLBACK CB_MergeInsertSrc(UINT op, PVOID p1, PVOID p2, LPARAM lp)
{
    nMessages[op]++;
    ok(lp == 0x1abe11ed, "lp=%Id\n", lp);
    return p1;
}        

static PVOID CALLBACK CB_MergeDeleteOddSrc(UINT op, PVOID p1, PVOID p2, LPARAM lp)
{
    nMessages[op]++;
    ok(lp == 0x1abe11ed, "lp=%Id\n", lp);
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

static HRESULT CALLBACK CB_Save(DPASTREAMINFO *pInfo, IStream *pStm, LPVOID lp)
{
    HRESULT hRes;

    ok(lp == (LPVOID)0xdeadbeef, "lp=%p\n", lp);
    hRes = IStream_Write(pStm, &pInfo->iPos, sizeof(INT), NULL);
    expect(S_OK, hRes);
    hRes = IStream_Write(pStm, &pInfo->pvItem, sizeof(PVOID), NULL);
    expect(S_OK, hRes);
    return S_OK;
}

static HRESULT CALLBACK CB_Load(DPASTREAMINFO *pInfo, IStream *pStm, LPVOID lp)
{
    HRESULT hRes;
    INT iOldPos;
    
    iOldPos = pInfo->iPos;
    ok(lp == (LPVOID)0xdeadbeef, "lp=%p\n", lp);
    hRes = IStream_Read(pStm, &pInfo->iPos, sizeof(INT), NULL);
    expect(S_OK, hRes);
    ok(pInfo->iPos == iOldPos, "iPos=%d iOldPos=%d\n", pInfo->iPos, iOldPos);
    hRes = IStream_Read(pStm, &pInfo->pvItem, sizeof(PVOID), NULL);
    expect(S_OK, hRes);
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
    BOOL rc;
    
    GetSystemInfo(&si);
    hHeap = HeapCreate(0, 1, 2);
    ok(hHeap != NULL, "error=%ld\n", GetLastError());
    dpa3 = pDPA_CreateEx(0, hHeap);
    ok(dpa3 != NULL, "\n");
    ret = pDPA_Grow(dpa3, si.dwPageSize + 1);
    ok(!ret && GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
       "ret=%d error=%ld\n", ret, GetLastError());

    dpa = pDPA_Create(0);
    ok(dpa != NULL, "\n");

    /* Set item with out of bound index */
    ok(pDPA_SetPtr(dpa, 1, (PVOID)6), "\n");
    /* Fill the created gap */
    ok(pDPA_SetPtr(dpa, 0, (PVOID)5), "\n");
    rc=CheckDPA(dpa, 0x56, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    
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
    ok(rc, "dw=0x%lx\n", dw);

    for(i = 1; i <= 6; i++)
    {
        INT j, k;
        k = pDPA_GetPtrIndex(dpa, (PVOID)(INT_PTR)i);
        /* Linear searches should work on unsorted DPAs */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpLT, 0x1abe11ed, 0);
        ok(j == k, "j=%d k=%d\n", j, k);
    }

    /* Sort DPA */
    ok(pDPA_Sort(dpa, CB_CmpGT, 0x1abe11ed), "\n");
    rc=CheckDPA(dpa, 0x654321, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    
    /* Clone into a new DPA */
    dpa2 = pDPA_Clone(dpa, NULL);
    ok(dpa2 != NULL, "\n");
    /* The old data should have been preserved */
    rc=CheckDPA(dpa2, 0x654321, &dw2);
    ok(rc, "dw=0x%lx\n", dw2);
    ok(pDPA_Sort(dpa, CB_CmpLT, 0x1abe11ed), "\n");
    
    /* Test if the DPA itself was really copied */
    rc=CheckDPA(dpa,  0x123456, &dw);
    ok(rc, "dw=0x%lx\n",  dw );
    rc=CheckDPA(dpa2, 0x654321, &dw2);
    ok(rc, "dw2=0x%lx\n", dw2);

    /* Clone into an old DPA */
    SetLastError(ERROR_SUCCESS);
    p = pDPA_Clone(dpa, dpa3);
    ok(p == dpa3, "p=%p\n", p);
    rc=CheckDPA(dpa3, 0x123456, &dw3);
    ok(rc, "dw3=0x%lx\n", dw3);

    for(i = 1; i <= 6; i++)
    {
        INT j;

        /* The array is in order so ptr == index+1 */
        j = pDPA_GetPtrIndex(dpa, (PVOID)(INT_PTR)i);
        ok(j+1 == i, "j=%d i=%d\n", j, i);
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpLT, 0x1abe11ed, DPAS_SORTED);
        ok(j+1 == i, "j=%d i=%d\n", j, i);

        /* Linear searches respect iStart ... */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, i+1, CB_CmpLT, 0x1abe11ed, 0);
        ok(j == DPA_ERR, "j=%d\n", j);
        /* ... but for a binary search it's ignored */
        j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, i+1, CB_CmpLT, 0x1abe11ed, DPAS_SORTED);
        ok(j+1 == i, "j=%d i=%d\n", j, i);
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
    ok(rc, "dw=0x%lx\n", dw);

    /* Check where to re-insert the deleted item */
    i = pDPA_Search(dpa, (PVOID)3, 0, 
                    CB_CmpLT, 0x1abe11ed, DPAS_SORTED|DPAS_INSERTAFTER);
    ok(i == 2, "i=%d\n", i);
    /* DPAS_INSERTBEFORE works just like DPAS_INSERTAFTER */
    i = pDPA_Search(dpa, (PVOID)3, 0,
                    CB_CmpLT, 0x1abe11ed, DPAS_SORTED|DPAS_INSERTBEFORE);
    ok(i == 2, "i=%d\n", i);
    /* without DPAS_INSERTBEFORE/AFTER */
    i = pDPA_Search(dpa, (PVOID)3, 0,
                    CB_CmpLT, 0x1abe11ed, DPAS_SORTED);
    ok(i == -1, "i=%d\n", i);

    /* Re-insert the item */
    ret = pDPA_InsertPtr(dpa, 2, (PVOID)3);
    ok(ret == 2, "ret=%d i=%d\n", ret, 2);
    rc=CheckDPA(dpa, 0x123456, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    
    /* When doing a binary search while claiming reverse order all indexes
     * should be bogus */
    for(i = 0; i < 6; i++)
    {
        INT j = pDPA_Search(dpa, (PVOID)(INT_PTR)i, 0, CB_CmpGT, 0x1abe11ed,
                            DPAS_SORTED|DPAS_INSERTBEFORE);
        ok(j != i, "i=%d\n", i);
    }

    /* Setting item with huge index should work */
    ok(pDPA_SetPtr(dpa2, 0x12345, (PVOID)0xdeadbeef), "\n");
    ret = pDPA_GetPtrIndex(dpa2, (PVOID)0xdeadbeef);
    ok(ret == 0x12345, "ret=%d\n", ret);
          
    pDPA_DeleteAllPtrs(dpa2);
    rc=CheckDPA(dpa2, 0, &dw2);
    ok(rc, "dw2=0x%lx\n", dw2);

    pDPA_Destroy(dpa);
    pDPA_Destroy(dpa2);
    pDPA_Destroy(dpa3);
}

static void test_DPA_Merge(void)
{
    HDPA dpa, dpa2, dpa3;
    INT ret, i;
    DWORD dw;
    BOOL rc;

    dpa  = pDPA_Create(0);
    dpa2 = pDPA_Create(0);
    dpa3 = pDPA_Create(0);

    ret = pDPA_InsertPtr(dpa, 0, (PVOID)1);
    ok(ret == 0, "ret=%d\n", ret);
    ret = pDPA_InsertPtr(dpa, 1, (PVOID)3);
    ok(ret == 1, "ret=%d\n", ret);
    ret = pDPA_InsertPtr(dpa, 2, (PVOID)5);
    ok(ret == 2, "ret=%d\n", ret);

    rc = CheckDPA(dpa, 0x135, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    for (i = 0; i < 6; i++)
    {
        ret = pDPA_InsertPtr(dpa2, i, (PVOID)(INT_PTR)(6-i));
        ok(ret == i, "ret=%d\n", ret);
        ret = pDPA_InsertPtr(dpa3, i, (PVOID)(INT_PTR)(i+1));
        ok(ret == i, "ret=%d\n", ret);
    }

    rc = CheckDPA(dpa2, 0x654321, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    rc = CheckDPA(dpa3, 0x123456, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    /* Delete all odd entries from dpa2 */
    memset(nMessages, 0, sizeof(nMessages));
    pDPA_Merge(dpa2, dpa, DPAM_INTERSECT,
               CB_CmpLT, CB_MergeDeleteOddSrc, 0x1abe11ed);
    rc = CheckDPA(dpa2, 0x246, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    expect(3, nMessages[DPAMM_MERGE]);
    expect(3, nMessages[DPAMM_DELETE]);
    expect(0, nMessages[DPAMM_INSERT]);

    for (i = 0; i < 6; i++)
    {
        ret = pDPA_InsertPtr(dpa2, i, (PVOID)(INT_PTR)(6-i));
        ok(ret == i, "ret=%d\n", ret);
    }

    /* DPAM_INTERSECT - returning source while merging */
    memset(nMessages, 0, sizeof(nMessages));
    pDPA_Merge(dpa2, dpa, DPAM_INTERSECT,
               CB_CmpLT, CB_MergeInsertSrc, 0x1abe11ed);
    rc = CheckDPA(dpa2, 0x135, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    expect(3, nMessages[DPAMM_MERGE]);
    expect(6, nMessages[DPAMM_DELETE]);
    expect(0, nMessages[DPAMM_INSERT]);

    /* DPAM_UNION */
    pDPA_DeleteAllPtrs(dpa);
    pDPA_InsertPtr(dpa, 0, (PVOID)1);
    pDPA_InsertPtr(dpa, 1, (PVOID)3);
    pDPA_InsertPtr(dpa, 2, (PVOID)5);
    pDPA_DeleteAllPtrs(dpa2);
    pDPA_InsertPtr(dpa2, 0, (PVOID)2);
    pDPA_InsertPtr(dpa2, 1, (PVOID)4);
    pDPA_InsertPtr(dpa2, 2, (PVOID)6);

    memset(nMessages, 0, sizeof(nMessages));
    pDPA_Merge(dpa2, dpa, DPAM_UNION,
               CB_CmpLT, CB_MergeInsertSrc, 0x1abe11ed);
    rc = CheckDPA(dpa2, 0x123456, &dw);
    ok(rc ||
       broken(!rc && dw == 0x23456), /* 4.7x */
       "dw=0x%lx\n", dw);

    expect(0, nMessages[DPAMM_MERGE]);
    expect(0, nMessages[DPAMM_DELETE]);
    ok(nMessages[DPAMM_INSERT] == 3 ||
       broken(nMessages[DPAMM_INSERT] == 2), /* 4.7x */
       "Expected 3, got %d\n", nMessages[DPAMM_INSERT]);

    /* Merge dpa3 into dpa2 and dpa */
    memset(nMessages, 0, sizeof(nMessages));
    pDPA_Merge(dpa, dpa3, DPAM_UNION|DPAM_SORTED,
               CB_CmpLT, CB_MergeInsertSrc, 0x1abe11ed);
    expect(3, nMessages[DPAMM_MERGE]);
    expect(0, nMessages[DPAMM_DELETE]);
    expect(3, nMessages[DPAMM_INSERT]);


    pDPA_DeleteAllPtrs(dpa2);
    pDPA_InsertPtr(dpa2, 0, (PVOID)2);
    pDPA_InsertPtr(dpa2, 1, (PVOID)4);
    pDPA_InsertPtr(dpa2, 2, (PVOID)6);

    memset(nMessages, 0, sizeof(nMessages));
    pDPA_Merge(dpa2, dpa3, DPAM_UNION|DPAM_SORTED,
               CB_CmpLT, CB_MergeInsertSrc, 0x1abe11ed);
    expect(3, nMessages[DPAMM_MERGE]);
    expect(0, nMessages[DPAMM_DELETE]);
    ok(nMessages[DPAMM_INSERT] == 3 ||
       broken(nMessages[DPAMM_INSERT] == 2), /* 4.7x */
       "Expected 3, got %d\n", nMessages[DPAMM_INSERT]);

    rc = CheckDPA(dpa,  0x123456, &dw);
    ok(rc, "dw=0x%lx\n",  dw);
    rc = CheckDPA(dpa2, 0x123456, &dw);
    ok(rc ||
       broken(!rc), /* win98 */
       "dw=0x%lx\n", dw);
    rc = CheckDPA(dpa3, 0x123456, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    pDPA_Destroy(dpa);
    pDPA_Destroy(dpa2);
    pDPA_Destroy(dpa3);
}

static void test_DPA_EnumCallback(void)
{
    HDPA dpa;
    BOOL rc;
    DWORD dw;
    INT i, ret;

    dpa = pDPA_Create(0);

    for (i = 0; i < 6; i++)
    {
        ret = pDPA_InsertPtr(dpa, i, (PVOID)(INT_PTR)(i+1));
        ok(ret == i, "ret=%d\n", ret);
    }

    rc = CheckDPA(dpa, 0x123456, &dw);
    ok(rc, "dw=0x%lx\n", dw);

    nEnum = 0;
    /* test callback sets first 3 items to 7 */
    pDPA_EnumCallback(dpa, CB_EnumFirstThree, dpa);
    rc = CheckDPA(dpa, 0x777456, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    ok(nEnum == 3, "nEnum=%d\n", nEnum);

    pDPA_Destroy(dpa);
}

static void test_DPA_DestroyCallback(void)
{
    HDPA dpa;
    INT i, ret;

    dpa = pDPA_Create(0);

    for (i = 0; i < 3; i++)
    {
        ret = pDPA_InsertPtr(dpa, i, (PVOID)(INT_PTR)(i+1));
        ok(ret == i, "ret=%d\n", ret);
    }

    nEnum = 0;
    pDPA_DestroyCallback(dpa, CB_EnumFirstThree, dpa);
    ok(nEnum == 3, "nEnum=%d\n", nEnum);
}

static void test_DPA_LoadStream(void)
{
    IStorage* pStg = NULL;
    IStream* pStm = NULL;
    LARGE_INTEGER li;
    ULARGE_INTEGER uli;
    DWORD dwMode;
    HRESULT hRes;
    STREAMDATA header;
    ULONG written, ret;
    HDPA dpa;

    hRes = CoInitialize(NULL);
    ok(hRes == S_OK, "Failed to initialize COM, hr %#lx.\n", hRes);

    dwMode = STGM_DIRECT|STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
    hRes = StgCreateDocfile(NULL, dwMode|STGM_DELETEONRELEASE, 0, &pStg);
    expect(S_OK, hRes);

    hRes = IStorage_CreateStream(pStg, L"Stg", dwMode, 0, 0, &pStm);
    expect(S_OK, hRes);

    /* write less than header size */
    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    memset(&header, 0, sizeof(header));
    written = 0;
    uli.QuadPart = sizeof(header)-1;
    hRes = IStream_SetSize(pStm, uli);
    expect(S_OK, hRes);
    hRes = IStream_Write(pStm, &header, sizeof(header)-1, &written);
    expect(S_OK, hRes);
    written -= sizeof(header)-1;
    expect(0, written);

    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    hRes = pDPA_LoadStream(&dpa, CB_Load, pStm, NULL);
    expect(E_FAIL, hRes);

    /* check stream position after header read failed */
    li.QuadPart = 0;
    uli.QuadPart = 1;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_CUR, &uli);
    expect(S_OK, hRes);
    ok(uli.QuadPart == 0, "Expected to position reset\n");

    /* write valid header for empty DPA */
    header.dwSize = sizeof(header);
    header.dwData2 = 1;
    header.dwItems = 0;
    written = 0;

    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    uli.QuadPart = sizeof(header);
    hRes = IStream_SetSize(pStm, uli);
    expect(S_OK, hRes);

    hRes = IStream_Write(pStm, &header, sizeof(header), &written);
    expect(S_OK, hRes);
    written -= sizeof(header);
    expect(0, written);

    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    dpa = NULL;
    hRes = pDPA_LoadStream(&dpa, CB_Load, pStm, NULL);
    expect(S_OK, hRes);
    pDPA_Destroy(dpa);

    /* try with altered dwData2 field */
    header.dwSize = sizeof(header);
    header.dwData2 = 2;
    header.dwItems = 0;

    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);
    hRes = IStream_Write(pStm, &header, sizeof(header), &written);
    expect(S_OK, hRes);
    written -= sizeof(header);
    expect(0, written);

    li.QuadPart = 0;
    hRes = IStream_Seek(pStm, li, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    hRes = pDPA_LoadStream(&dpa, CB_Load, pStm, (void*)0xdeadbeef);
    expect(E_FAIL, hRes);

    ret = IStream_Release(pStm);
    ok(!ret, "ret=%ld\n", ret);

    ret = IStorage_Release(pStg);
    ok(!ret, "ret=%ld\n", ret);

    CoUninitialize();
}

static void test_DPA_SaveStream(void)
{
    HDPA dpa;
    IStorage* pStg = NULL;
    IStream* pStm = NULL;
    DWORD dwMode, dw;
    HRESULT hRes;
    INT ret;
    INT i;
    BOOL rc;
    LARGE_INTEGER liZero;

    hRes = CoInitialize(NULL);
    ok(hRes == S_OK, "Failed to initialize COM, hr %#lx.\n", hRes);

    dwMode = STGM_DIRECT|STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;
    hRes = StgCreateDocfile(NULL, dwMode|STGM_DELETEONRELEASE, 0, &pStg);
    expect(S_OK, hRes);

    hRes = IStorage_CreateStream(pStg, L"Stg", dwMode, 0, 0, &pStm);
    expect(S_OK, hRes);

    dpa = pDPA_Create(0);

    /* simple parameter check */
    hRes = pDPA_SaveStream(dpa, NULL, pStm, NULL);
    ok(hRes == E_INVALIDARG ||
       broken(hRes == S_OK) /* XP and below */, "Wrong result, %ld\n", hRes);
if (0) {
    /* crashes on XP */
    hRes = pDPA_SaveStream(NULL, CB_Save, pStm, NULL);
    expect(E_INVALIDARG, hRes);

    hRes = pDPA_SaveStream(dpa, CB_Save, NULL, NULL);
    expect(E_INVALIDARG, hRes);
}

    /* saving/loading */
    for (i = 0; i < 6; i++)
    {
        ret = pDPA_InsertPtr(dpa, i, (PVOID)(INT_PTR)(i+1));
        ok(ret == i, "ret=%d\n", ret);
    }

    liZero.QuadPart = 0;
    hRes = IStream_Seek(pStm, liZero, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);

    hRes = pDPA_SaveStream(dpa, CB_Save, pStm, (void*)0xdeadbeef);
    expect(S_OK, hRes);
    pDPA_Destroy(dpa);

    liZero.QuadPart = 0;
    hRes = IStream_Seek(pStm, liZero, STREAM_SEEK_SET, NULL);
    expect(S_OK, hRes);
    hRes = pDPA_LoadStream(&dpa, CB_Load, pStm, (void*)0xdeadbeef);
    expect(S_OK, hRes);
    rc = CheckDPA(dpa, 0x123456, &dw);
    ok(rc, "dw=0x%lx\n", dw);
    pDPA_Destroy(dpa);

    ret = IStream_Release(pStm);
    ok(!ret, "ret=%d\n", ret);

    ret = IStorage_Release(pStg);
    ok(!ret, "ret=%d\n", ret);

    CoUninitialize();
}

START_TEST(dpa)
{
    ULONG_PTR cookie;
    HANDLE ctxt;

    init_functions();

    test_dpa();
    test_DPA_Merge();
    test_DPA_EnumCallback();
    test_DPA_DestroyCallback();
    test_DPA_LoadStream();
    test_DPA_SaveStream();

    if (!load_v6_module(&cookie, &ctxt))
        return;

    init_functions();

    test_dpa();
    test_DPA_Merge();
    test_DPA_EnumCallback();
    test_DPA_DestroyCallback();
    test_DPA_LoadStream();
    test_DPA_SaveStream();

    unload_v6_module(cookie, ctxt);
}
