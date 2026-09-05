/*
 * Unit test suite for MAPI property functions
 *
 * Copyright 2004 Jon Griffiths
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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnt.h"
#include "initguid.h"
#include "mapiutil.h"
#include "mapitags.h"
#include "mapi32_test.h"

static HMODULE hMapi32 = 0;

static SCODE        (WINAPI *pScInitMapiUtil)(ULONG);
static void         (WINAPI *pDeinitMapiUtil)(void);
static SCODE        (WINAPI *pPropCopyMore)(LPSPropValue,LPSPropValue,ALLOCATEMORE*,LPVOID);
static ULONG        (WINAPI *pUlPropSize)(LPSPropValue);
static BOOL         (WINAPI *pFPropContainsProp)(LPSPropValue,LPSPropValue,ULONG);
static BOOL         (WINAPI *pFPropCompareProp)(LPSPropValue,ULONG,LPSPropValue);
static LONG         (WINAPI *pLPropCompareProp)(LPSPropValue,LPSPropValue);
static LPSPropValue (WINAPI *pPpropFindProp)(LPSPropValue,ULONG,ULONG);
static SCODE        (WINAPI *pScCountProps)(INT,LPSPropValue,ULONG*);
static SCODE        (WINAPI *pScCopyProps)(int,LPSPropValue,LPVOID,ULONG*);
static SCODE        (WINAPI *pScRelocProps)(int,LPSPropValue,LPVOID,LPVOID,ULONG*);
static LPSPropValue (WINAPI *pLpValFindProp)(ULONG,ULONG,LPSPropValue);
static BOOL         (WINAPI *pFBadRglpszA)(LPSTR*,ULONG);
static BOOL         (WINAPI *pFBadRglpszW)(LPWSTR*,ULONG);
static BOOL         (WINAPI *pFBadRowSet)(LPSRowSet);
static ULONG        (WINAPI *pFBadPropTag)(ULONG);
static ULONG        (WINAPI *pFBadRow)(LPSRow);
static ULONG        (WINAPI *pFBadProp)(LPSPropValue);
static ULONG        (WINAPI *pFBadColumnSet)(LPSPropTagArray);
static SCODE        (WINAPI *pCreateIProp)(LPCIID,ALLOCATEBUFFER*,ALLOCATEMORE*,
                                           FREEBUFFER*,LPVOID,LPPROPDATA*);
static SCODE        (WINAPI *pMAPIAllocateBuffer)(ULONG, LPVOID);
static SCODE        (WINAPI *pMAPIAllocateMore)(ULONG, LPVOID, LPVOID);
static SCODE        (WINAPI *pMAPIInitialize)(LPVOID);
static SCODE        (WINAPI *pMAPIFreeBuffer)(LPVOID);
static void         (WINAPI *pMAPIUninitialize)(void);

static BOOL InitFuncPtrs(void)
{
    hMapi32 = LoadLibraryA("mapi32.dll");

    pPropCopyMore = (void*)GetProcAddress(hMapi32, "PropCopyMore@16");
    pUlPropSize = (void*)GetProcAddress(hMapi32, "UlPropSize@4");
    pFPropContainsProp = (void*)GetProcAddress(hMapi32, "FPropContainsProp@12");
    pFPropCompareProp = (void*)GetProcAddress(hMapi32, "FPropCompareProp@12");
    pLPropCompareProp = (void*)GetProcAddress(hMapi32, "LPropCompareProp@8");
    pPpropFindProp = (void*)GetProcAddress(hMapi32, "PpropFindProp@12");
    pScCountProps = (void*)GetProcAddress(hMapi32, "ScCountProps@12");
    pScCopyProps = (void*)GetProcAddress(hMapi32, "ScCopyProps@16");
    pScRelocProps = (void*)GetProcAddress(hMapi32, "ScRelocProps@20");
    pLpValFindProp = (void*)GetProcAddress(hMapi32, "LpValFindProp@12");
    pFBadRglpszA = (void*)GetProcAddress(hMapi32, "FBadRglpszA@8");
    pFBadRglpszW = (void*)GetProcAddress(hMapi32, "FBadRglpszW@8");
    pFBadRowSet = (void*)GetProcAddress(hMapi32, "FBadRowSet@4");
    pFBadPropTag = (void*)GetProcAddress(hMapi32, "FBadPropTag@4");
    pFBadRow = (void*)GetProcAddress(hMapi32, "FBadRow@4");
    pFBadProp = (void*)GetProcAddress(hMapi32, "FBadProp@4");
    pFBadColumnSet = (void*)GetProcAddress(hMapi32, "FBadColumnSet@4");
    pCreateIProp = (void*)GetProcAddress(hMapi32, "CreateIProp@24");

    pScInitMapiUtil = (void*)GetProcAddress(hMapi32, "ScInitMapiUtil@4");
    pDeinitMapiUtil = (void*)GetProcAddress(hMapi32, "DeinitMapiUtil@0");
    pMAPIAllocateBuffer = (void*)GetProcAddress(hMapi32, "MAPIAllocateBuffer");
    pMAPIAllocateMore = (void*)GetProcAddress(hMapi32, "MAPIAllocateMore");
    pMAPIFreeBuffer = (void*)GetProcAddress(hMapi32, "MAPIFreeBuffer");
    pMAPIInitialize = (void*)GetProcAddress(hMapi32, "MAPIInitialize");
    pMAPIUninitialize = (void*)GetProcAddress(hMapi32, "MAPIUninitialize");

    return pMAPIAllocateBuffer && pMAPIAllocateMore && pMAPIFreeBuffer &&
           pScInitMapiUtil && pDeinitMapiUtil;
}

/* FIXME: Test PT_I2, PT_I4, PT_R4, PT_R8, PT_CURRENCY, PT_APPTIME, PT_SYSTIME,
 * PT_ERROR, PT_BOOLEAN, PT_I8, and PT_CLSID. */
static ULONG ptTypes[] = {
    PT_STRING8, PT_BINARY, PT_UNICODE
};

static void test_PropCopyMore(void)
{
    static char szHiA[] = "Hi!";
    static WCHAR szHiW[] = L"Hi!";
    SPropValue *lpDest = NULL, *lpSrc = NULL;
    ULONG i;
    SCODE scode;

    if (!pPropCopyMore)
    {
        win_skip("PropCopyMore is not available\n");
        return;
    }

    scode = pMAPIAllocateBuffer(sizeof(SPropValue), &lpDest);
    ok(scode == S_OK, "Expected MAPIAllocateBuffer to return S_OK, got 0x%lx\n", scode);
    if (FAILED(scode))
    {
        skip("MAPIAllocateBuffer failed\n");
        return;
    }

    scode = pMAPIAllocateMore(sizeof(SPropValue), lpDest, &lpSrc);
    ok(scode == S_OK, "Expected MAPIAllocateMore to return S_OK, got 0x%lx\n", scode);
    if (FAILED(scode))
    {
        skip("MAPIAllocateMore failed\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(ptTypes); i++)
    {
        lpSrc->ulPropTag = ptTypes[i];

        switch (ptTypes[i])
        {
        case PT_STRING8:
            lpSrc->Value.lpszA = szHiA;
            break;
        case PT_UNICODE:
            lpSrc->Value.lpszW = szHiW;
            break;
        case PT_BINARY:
            lpSrc->Value.bin.cb = 4;
            lpSrc->Value.bin.lpb = (LPBYTE)szHiA;
            break;
        }

        memset(lpDest, 0xff, sizeof(SPropValue));

        scode = pPropCopyMore(lpDest, lpSrc, (ALLOCATEMORE*)pMAPIAllocateMore, lpDest);
        ok(!scode && lpDest->ulPropTag == lpSrc->ulPropTag,
           "PropCopyMore: Expected 0x0,%ld, got 0x%08lx,%ld\n",
           lpSrc->ulPropTag, scode, lpDest->ulPropTag);
        if (SUCCEEDED(scode))
        {
            switch (ptTypes[i])
            {
            case PT_STRING8:
                ok(lstrcmpA(lpDest->Value.lpszA, lpSrc->Value.lpszA) == 0,
                   "PropCopyMore: Ascii string differs\n");
                break;
            case PT_UNICODE:
                ok(wcscmp(lpDest->Value.lpszW, lpSrc->Value.lpszW) == 0,
                   "PropCopyMore: Unicode string differs\n");
                break;
            case PT_BINARY:
                ok(lpDest->Value.bin.cb == 4 &&
                   !memcmp(lpSrc->Value.bin.lpb, lpDest->Value.bin.lpb, 4),
                   "PropCopyMore: Binary array  differs\n");
                break;
            }
        }
    }

    /* Since all allocations are linked, freeing lpDest frees everything */
    scode = pMAPIFreeBuffer(lpDest);
    ok(scode == S_OK, "Expected MAPIFreeBuffer to return S_OK, got 0x%lx\n", scode);
}

static void test_UlPropSize(void)
{
    static char szHiA[] = "Hi!";
    static WCHAR szHiW[] = L"Hi!";
    LPSTR  buffa[2];
    LPWSTR buffw[2];
    SBinary buffbin[2];
    ULONG pt, exp, res;

    if (!pUlPropSize)
    {
        win_skip("UlPropSize is not available\n");
        return;
    }

    for (pt = 0; pt < PROP_ID_INVALID; pt++)
    {
        SPropValue pv;

        memset(&pv, 0 ,sizeof(pv));
        pv.ulPropTag = pt;

        exp = 1u; /* Default to one item for non-MV properties */

        switch (PROP_TYPE(pt))
        {
        case PT_MV_I2:       pv.Value.MVi.cValues = exp = 2;
        case PT_I2:          exp *= sizeof(USHORT); break;
        case PT_MV_I4:       pv.Value.MVl.cValues = exp = 2;
        case PT_I4:          exp *= sizeof(LONG); break;
        case PT_MV_R4:       pv.Value.MVflt.cValues = exp = 2;
        case PT_R4:          exp *= sizeof(float); break;
        case PT_MV_DOUBLE:   pv.Value.MVdbl.cValues = exp = 2;
        case PT_R8:          exp *= sizeof(double); break;
        case PT_MV_CURRENCY: pv.Value.MVcur.cValues = exp = 2;
        case PT_CURRENCY:    exp *= sizeof(CY); break;
        case PT_MV_APPTIME:  pv.Value.MVat.cValues = exp = 2;
        case PT_APPTIME:     exp *= sizeof(double); break;
        case PT_MV_SYSTIME:  pv.Value.MVft.cValues = exp = 2;
        case PT_SYSTIME:     exp *= sizeof(FILETIME); break;
        case PT_ERROR:       exp = sizeof(SCODE); break;
        case PT_BOOLEAN:     exp = sizeof(USHORT); break;
        case PT_OBJECT:      exp = 0; break;
        case PT_MV_I8:       pv.Value.MVli.cValues = exp = 2;
        case PT_I8:          exp *= sizeof(LONG64); break;
#if 0
        /* My version of native mapi returns 0 for PT_MV_CLSID even if a valid
         * array is given. This _has_ to be a bug, so Wine does
         * the right thing(tm) and we don't test it here.
         */
        case PT_MV_CLSID:    pv.Value.MVguid.cValues = exp = 2;
#endif
        case PT_CLSID:       exp *= sizeof(GUID); break;
        case PT_STRING8:
            pv.Value.lpszA = szHiA;
            exp = 4;
            break;
        case PT_UNICODE:
            pv.Value.lpszW = szHiW;
            exp = 4 * sizeof(WCHAR);
            break;
        case PT_BINARY:
            pv.Value.bin.cb = exp = 19;
            break;
        case PT_MV_STRING8:
            pv.Value.MVszA.cValues = 2;
            pv.Value.MVszA.lppszA = buffa;
            buffa[0] = szHiA;
            buffa[1] = szHiA;
            exp = 8;
            break;
        case PT_MV_UNICODE:
            pv.Value.MVszW.cValues = 2;
            pv.Value.MVszW.lppszW = buffw;
            buffw[0] = szHiW;
            buffw[1] = szHiW;
            exp = 8 * sizeof(WCHAR);
            break;
        case PT_MV_BINARY:
            pv.Value.MVbin.cValues = 2;
            pv.Value.MVbin.lpbin = buffbin;
            buffbin[0].cb = 19;
            buffbin[1].cb = 1;
            exp = 20;
            break;
        default:
            exp = 0;
        }

        res = pUlPropSize(&pv);
        ok(res == exp,
           "pt= %ld: Expected %ld, got %ld\n", pt, exp, res);
    }
}

static void test_FPropContainsProp(void)
{
    static char szFull[] = "Full String";
    static char szFullLower[] = "full string";
    static char szPrefix[] = "Full";
    static char szPrefixLower[] = "full";
    static char szSubstring[] = "ll St";
    static char szSubstringLower[] = "ll st";
    SPropValue pvLeft, pvRight;
    ULONG pt;
    BOOL bRet;

    if (!pFPropContainsProp)
    {
        win_skip("FPropContainsProp is not available\n");
        return;
    }

    /* Ensure that only PT_STRING8 and PT_BINARY are handled */
    for (pt = 0; pt < PROP_ID_INVALID; pt++)
    {
        if (pt == PT_STRING8 || pt == PT_BINARY)
            continue; /* test these later */

        memset(&pvLeft, 0 ,sizeof(pvLeft));
        memset(&pvRight, 0 ,sizeof(pvRight));
        pvLeft.ulPropTag = pvRight.ulPropTag = pt;

        bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
        ok(bRet == FALSE, "pt= %ld: Expected FALSE, got %d\n", pt, bRet);
    }

    /* test the various flag combinations */
    pvLeft.ulPropTag = pvRight.ulPropTag = PT_STRING8;
    pvLeft.Value.lpszA = szFull;
    pvRight.Value.lpszA = szFull;

    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == TRUE, "(full,full)[] match failed\n");
    pvRight.Value.lpszA = szPrefix;
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == FALSE, "(full,prefix)[] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == TRUE, "(full,prefix)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == TRUE, "(full,prefix)[SUBSTRING] match failed\n");
    pvRight.Value.lpszA = szPrefixLower;
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "(full,prefixlow)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == FALSE, "(full,prefixlow)[SUBSTRING] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX|FL_IGNORECASE);
    ok(bRet == TRUE, "(full,prefixlow)[PREFIX|IGNORECASE] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING|FL_IGNORECASE);
    ok(bRet == TRUE, "(full,prefixlow)[SUBSTRING|IGNORECASE] match failed\n");
    pvRight.Value.lpszA = szSubstring;
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == FALSE, "(full,substr)[] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "(full,substr)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == TRUE, "(full,substr)[SUBSTRING] match failed\n");
    pvRight.Value.lpszA = szSubstringLower;
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "(full,substrlow)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == FALSE, "(full,substrlow)[SUBSTRING] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX|FL_IGNORECASE);
    ok(bRet == FALSE, "(full,substrlow)[PREFIX|IGNORECASE] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING|FL_IGNORECASE);
    ok(bRet == TRUE, "(full,substrlow)[SUBSTRING|IGNORECASE] match failed\n");
    pvRight.Value.lpszA = szFullLower;
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING|FL_IGNORECASE);
    ok(bRet == TRUE, "(full,fulllow)[IGNORECASE] match failed\n");

    pvLeft.ulPropTag = pvRight.ulPropTag = PT_BINARY;
    pvLeft.Value.bin.lpb = (LPBYTE)szFull;
    pvRight.Value.bin.lpb = (LPBYTE)szFull;
    pvLeft.Value.bin.cb = pvRight.Value.bin.cb = strlen(szFull);

    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == TRUE, "bin(full,full)[] match failed\n");
    pvRight.Value.bin.lpb = (LPBYTE)szPrefix;
    pvRight.Value.bin.cb = strlen(szPrefix);
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == FALSE, "bin(full,prefix)[] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == TRUE, "bin(full,prefix)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == TRUE, "bin(full,prefix)[SUBSTRING] match failed\n");
    pvRight.Value.bin.lpb = (LPBYTE)szPrefixLower;
    pvRight.Value.bin.cb = strlen(szPrefixLower);
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "bin(full,prefixlow)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == FALSE, "bin(full,prefixlow)[SUBSTRING] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX|FL_IGNORECASE);
    ok(bRet == FALSE, "bin(full,prefixlow)[PREFIX|IGNORECASE] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING|FL_IGNORECASE);
    ok(bRet == FALSE, "bin(full,prefixlow)[SUBSTRING|IGNORECASE] match failed\n");
    pvRight.Value.bin.lpb = (LPBYTE)szSubstring;
    pvRight.Value.bin.cb = strlen(szSubstring);
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING);
    ok(bRet == FALSE, "bin(full,substr)[] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "bin(full,substr)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == TRUE, "bin(full,substr)[SUBSTRING] match failed\n");
    pvRight.Value.bin.lpb = (LPBYTE)szSubstringLower;
    pvRight.Value.bin.cb = strlen(szSubstringLower);
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX);
    ok(bRet == FALSE, "bin(full,substrlow)[PREFIX] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING);
    ok(bRet == FALSE, "bin(full,substrlow)[SUBSTRING] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_PREFIX|FL_IGNORECASE);
    ok(bRet == FALSE, "bin(full,substrlow)[PREFIX|IGNORECASE] match failed\n");
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_SUBSTRING|FL_IGNORECASE);
    ok(bRet == FALSE, "bin(full,substrlow)[SUBSTRING|IGNORECASE] match failed\n");
    pvRight.Value.bin.lpb = (LPBYTE)szFullLower;
    pvRight.Value.bin.cb = strlen(szFullLower);
    bRet = pFPropContainsProp(&pvLeft, &pvRight, FL_FULLSTRING|FL_IGNORECASE);
    ok(bRet == FALSE, "bin(full,fulllow)[IGNORECASE] match failed\n");
}

typedef struct tagFPropCompareProp_Result
{
    SHORT lVal;
    SHORT rVal;
    ULONG relOp;
    BOOL  bRet;
} FPropCompareProp_Result;

static const FPropCompareProp_Result FPCProp_Results[] =
{
    { 1, 2, RELOP_LT, TRUE },
    { 1, 1, RELOP_LT, FALSE },
    { 2, 1, RELOP_LT, FALSE },
    { 1, 2, RELOP_LE, TRUE },
    { 1, 1, RELOP_LE, TRUE },
    { 2, 1, RELOP_LE, FALSE },
    { 1, 2, RELOP_GT, FALSE },
    { 1, 1, RELOP_GT, FALSE },
    { 2, 1, RELOP_GT, TRUE },
    { 1, 2, RELOP_GE, FALSE },
    { 1, 1, RELOP_GE, TRUE },
    { 2, 1, RELOP_GE, TRUE },
    { 1, 2, RELOP_EQ, FALSE },
    { 1, 1, RELOP_EQ, TRUE },
    { 2, 1, RELOP_EQ, FALSE }
};

static const char *relops[] = { "RELOP_LT", "RELOP_LE", "RELOP_GT", "RELOP_GE", "RELOP_EQ" };

static void test_FPropCompareProp(void)
{
    SPropValue pvLeft, pvRight;
    GUID lguid, rguid;
    char lbuffa[2], rbuffa[2];
    WCHAR lbuffw[2], rbuffw[2];
    ULONG i, j;
    BOOL bRet, bExp;

    if (!pFPropCompareProp)
    {
        win_skip("FPropCompareProp is not available\n");
        return;
    }

    lbuffa[1] = '\0';
    rbuffa[1] = '\0';
    lbuffw[1] = '\0';
    rbuffw[1] = '\0';

    for (i = 0; i < ARRAY_SIZE(ptTypes); i++)
    {
        pvLeft.ulPropTag = pvRight.ulPropTag = ptTypes[i];

        for (j = 0; j < ARRAY_SIZE(FPCProp_Results); j++)
        {
            SHORT lVal = FPCProp_Results[j].lVal;
            SHORT rVal = FPCProp_Results[j].rVal;

            bExp = FPCProp_Results[j].bRet;

            switch (ptTypes[i])
            {
            case PT_BOOLEAN:
                /* Boolean values have no concept of less or greater than, only equality */
                if ((lVal == 1 && rVal == 2 && FPCProp_Results[j].relOp == RELOP_LT) ||
                    (lVal == 2 && rVal == 1 && FPCProp_Results[j].relOp == RELOP_LE)||
                    (lVal == 2 && rVal == 1 && FPCProp_Results[j].relOp == RELOP_GT)||
                    (lVal == 1 && rVal == 2 && FPCProp_Results[j].relOp == RELOP_GE)||
                    (lVal == 1 && rVal == 2 && FPCProp_Results[j].relOp == RELOP_EQ)||
                    (lVal == 2 && rVal == 1 && FPCProp_Results[j].relOp == RELOP_EQ))
                    bExp = !bExp;
                    /* Fall through ... */
            case PT_I2:
                pvLeft.Value.i = lVal;
                pvRight.Value.i = rVal;
                break;
            case PT_ERROR:
            case PT_I4:
                pvLeft.Value.l = lVal;
                pvRight.Value.l = rVal;
                break;
            case PT_R4:
                pvLeft.Value.flt = lVal;
                pvRight.Value.flt = rVal;
                break;
            case PT_APPTIME:
            case PT_R8:
                pvLeft.Value.dbl = lVal;
                pvRight.Value.dbl = rVal;
                break;
            case PT_CURRENCY:
                pvLeft.Value.cur.int64 = lVal;
                pvRight.Value.cur.int64 = rVal;
                break;
            case PT_SYSTIME:
                pvLeft.Value.ft.dwLowDateTime = lVal;
                pvLeft.Value.ft.dwHighDateTime = 0;
                pvRight.Value.ft.dwLowDateTime = rVal;
                pvRight.Value.ft.dwHighDateTime = 0;
                break;
            case PT_I8:
                pvLeft.Value.li.u.LowPart = lVal;
                pvLeft.Value.li.u.HighPart = 0;
                pvRight.Value.li.u.LowPart = rVal;
                pvRight.Value.li.u.HighPart = 0;
                break;
            case PT_CLSID:
                memset(&lguid, 0, sizeof(GUID));
                memset(&rguid, 0, sizeof(GUID));
                lguid.Data4[7] = lVal;
                rguid.Data4[7] = rVal;
                pvLeft.Value.lpguid = &lguid;
                pvRight.Value.lpguid = &rguid;
                break;
            case PT_STRING8:
                pvLeft.Value.lpszA = lbuffa;
                pvRight.Value.lpszA = rbuffa;
                lbuffa[0] = '0' + lVal;
                rbuffa[0] = '0' + rVal;
                break;
            case PT_UNICODE:
                pvLeft.Value.lpszW = lbuffw;
                pvRight.Value.lpszW = rbuffw;
                lbuffw[0] = '0' + lVal;
                rbuffw[0] = '0' + rVal;
                break;
            case PT_BINARY:
                pvLeft.Value.bin.cb = 1;
                pvRight.Value.bin.cb = 1;
                pvLeft.Value.bin.lpb = (LPBYTE)lbuffa;
                pvRight.Value.bin.lpb = (LPBYTE)rbuffa;
                lbuffa[0] = lVal;
                rbuffa[0] = rVal;
                break;
            }

            bRet = pFPropCompareProp(&pvLeft, FPCProp_Results[j].relOp, &pvRight);
            ok(bRet == bExp,
               "pt %ld (%d,%d,%s): expected %d, got %d\n", ptTypes[i],
               FPCProp_Results[j].lVal, FPCProp_Results[j].rVal,
               relops[FPCProp_Results[j].relOp], bExp, bRet);
        }
    }
}

typedef struct tagLPropCompareProp_Result
{
    SHORT lVal;
    SHORT rVal;
    INT   iRet;
} LPropCompareProp_Result;

static const LPropCompareProp_Result LPCProp_Results[] =
{
    { 1, 2, -1 },
    { 1, 1, 0 },
    { 2, 1, 1 },
};

static void test_LPropCompareProp(void)
{
    SPropValue pvLeft, pvRight;
    GUID lguid, rguid;
    char lbuffa[2], rbuffa[2];
    WCHAR lbuffw[2], rbuffw[2];
    ULONG i, j;
    INT iRet, iExp;

    if (!pLPropCompareProp)
    {
        win_skip("LPropCompareProp is not available\n");
        return;
    }

    lbuffa[1] = '\0';
    rbuffa[1] = '\0';
    lbuffw[1] = '\0';
    rbuffw[1] = '\0';

    for (i = 0; i < ARRAY_SIZE(ptTypes); i++)
    {
        pvLeft.ulPropTag = pvRight.ulPropTag = ptTypes[i];

        for (j = 0; j < ARRAY_SIZE(LPCProp_Results); j++)
        {
            SHORT lVal = LPCProp_Results[j].lVal;
            SHORT rVal = LPCProp_Results[j].rVal;

            iExp = LPCProp_Results[j].iRet;

            switch (ptTypes[i])
            {
            case PT_BOOLEAN:
                /* Boolean values have no concept of less or greater than, only equality */
                if (lVal && rVal)
                    iExp = 0;
                    /* Fall through ... */
            case PT_I2:
                pvLeft.Value.i = lVal;
                pvRight.Value.i = rVal;
                break;
            case PT_ERROR:
            case PT_I4:
                pvLeft.Value.l = lVal;
                pvRight.Value.l = rVal;
                break;
            case PT_R4:
                pvLeft.Value.flt = lVal;
                pvRight.Value.flt = rVal;
                break;
            case PT_APPTIME:
            case PT_R8:
                pvLeft.Value.dbl = lVal;
                pvRight.Value.dbl = rVal;
                break;
            case PT_CURRENCY:
                pvLeft.Value.cur.int64 = lVal;
                pvRight.Value.cur.int64 = rVal;
                break;
            case PT_SYSTIME:
                pvLeft.Value.ft.dwLowDateTime = lVal;
                pvLeft.Value.ft.dwHighDateTime = 0;
                pvRight.Value.ft.dwLowDateTime = rVal;
                pvRight.Value.ft.dwHighDateTime = 0;
                break;
            case PT_I8:
                pvLeft.Value.li.u.LowPart = lVal;
                pvLeft.Value.li.u.HighPart = 0;
                pvRight.Value.li.u.LowPart = rVal;
                pvRight.Value.li.u.HighPart = 0;
                break;
            case PT_CLSID:
                memset(&lguid, 0, sizeof(GUID));
                memset(&rguid, 0, sizeof(GUID));
                lguid.Data4[7] = lVal;
                rguid.Data4[7] = rVal;
                pvLeft.Value.lpguid = &lguid;
                pvRight.Value.lpguid = &rguid;
                break;
            case PT_STRING8:
                pvLeft.Value.lpszA = lbuffa;
                pvRight.Value.lpszA = rbuffa;
                lbuffa[0] = '0' + lVal;
                rbuffa[0] = '0' + rVal;
                break;
            case PT_UNICODE:
                pvLeft.Value.lpszW = lbuffw;
                pvRight.Value.lpszW = rbuffw;
                lbuffw[0] = '0' + lVal;
                rbuffw[0] = '0' + rVal;
                break;
            case PT_BINARY:
                pvLeft.Value.bin.cb = 1;
                pvRight.Value.bin.cb = 1;
                pvLeft.Value.bin.lpb = (LPBYTE)lbuffa;
                pvRight.Value.bin.lpb = (LPBYTE)rbuffa;
                lbuffa[0] = lVal;
                rbuffa[0] = rVal;
                break;
            }

            iRet = pLPropCompareProp(&pvLeft, &pvRight);
            ok(iRet == iExp,
               "pt %ld (%d,%d): expected %d, got %d\n", ptTypes[i],
               LPCProp_Results[j].lVal, LPCProp_Results[j].rVal, iExp, iRet);
        }
    }
}

static void test_PpropFindProp(void)
{
    SPropValue pvProp, *pRet;
    ULONG i;

    if (!pPpropFindProp)
    {
        win_skip("PpropFindProp is not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(ptTypes); i++)
    {
        pvProp.ulPropTag = ptTypes[i];

        pRet = pPpropFindProp(&pvProp, 1u, ptTypes[i]);
        ok(pRet == &pvProp,
           "PpropFindProp[%ld]: Didn't find existing property\n",
           ptTypes[i]);

        pRet = pPpropFindProp(&pvProp, 1u, i ? ptTypes[i-1] : ptTypes[i+1]);
        ok(pRet == NULL, "PpropFindProp[%ld]: Found nonexistent property\n",
           ptTypes[i]);
    }

    pvProp.ulPropTag = PROP_TAG(PT_I2, 1u);
    pRet = pPpropFindProp(&pvProp, 1u, PROP_TAG(PT_UNSPECIFIED, 0u));
    ok(pRet == NULL, "PpropFindProp[UNSPECIFIED]: Matched on different id\n");
    pRet = pPpropFindProp(&pvProp, 1u, PROP_TAG(PT_UNSPECIFIED, 1u));
    ok(pRet == &pvProp, "PpropFindProp[UNSPECIFIED]: Didn't match id\n");
}

static void test_ScCountProps(void)
{
    static char szHiA[] = "Hi!";
    static WCHAR szHiW[] = L"Hi!";
    static const ULONG ULHILEN = 4; /* chars in szHiA/W incl. NUL */
    LPSTR  buffa[3];
    LPWSTR buffw[3];
    SBinary buffbin[3];
    GUID iids[4], *iid = iids;
    SCODE res;
    ULONG pt, exp, ulRet;
    BOOL success = TRUE;

    if (!pScCountProps)
    {
        win_skip("ScCountProps is not available\n");
        return;
    }

    for (pt = 0; pt < PROP_ID_INVALID && success; pt++)
    {
        SPropValue pv;

        memset(&pv, 0 ,sizeof(pv));
        pv.ulPropTag = PROP_TAG(pt, 1u);

        switch (PROP_TYPE(pt))
        {
        case PT_I2:
        case PT_I4:
        case PT_R4:
        case PT_R8:
        case PT_CURRENCY:
        case PT_APPTIME:
        case PT_SYSTIME:
        case PT_ERROR:
        case PT_BOOLEAN:
        case PT_OBJECT:
        case PT_I8:
            exp = sizeof(pv);
            break;
        case PT_CLSID:
            pv.Value.lpguid = iid;
            exp = sizeof(GUID) + sizeof(pv);
            break;
        case PT_STRING8:
            pv.Value.lpszA = szHiA;
            exp = 4 + sizeof(pv);
            break;
        case PT_UNICODE:
            pv.Value.lpszW = szHiW;
            exp = 4 * sizeof(WCHAR) + sizeof(pv);
            break;
        case PT_BINARY:
            pv.Value.bin.cb = 2;
            pv.Value.bin.lpb = (LPBYTE)iid;
            exp = 2 + sizeof(pv);
            break;
        case PT_MV_I2:
            pv.Value.MVi.cValues = 3;
            pv.Value.MVi.lpi = (SHORT*)iid;
            exp = 3 * sizeof(SHORT) + sizeof(pv);
            break;
        case PT_MV_I4:
            pv.Value.MVl.cValues = 3;
            pv.Value.MVl.lpl = (LONG*)iid;
            exp = 3 * sizeof(LONG) + sizeof(pv);
            break;
        case PT_MV_I8:
            pv.Value.MVli.cValues = 3;
            pv.Value.MVli.lpli = (LARGE_INTEGER*)iid;
            exp = 3 * sizeof(LARGE_INTEGER) + sizeof(pv);
            break;
        case PT_MV_R4:
            pv.Value.MVflt.cValues = 3;
            pv.Value.MVflt.lpflt = (float*)iid;
            exp = 3 * sizeof(float) + sizeof(pv);
            break;
        case PT_MV_APPTIME:
        case PT_MV_R8:
            pv.Value.MVdbl.cValues = 3;
            pv.Value.MVdbl.lpdbl = (double*)iid;
            exp = 3 * sizeof(double) + sizeof(pv);
            break;
        case PT_MV_CURRENCY:
            pv.Value.MVcur.cValues = 3;
            pv.Value.MVcur.lpcur = (CY*)iid;
            exp = 3 * sizeof(CY) + sizeof(pv);
            break;
        case PT_MV_SYSTIME:
            pv.Value.MVft.cValues = 3;
            pv.Value.MVft.lpft = (FILETIME*)iid;
            exp = 3 * sizeof(CY) + sizeof(pv);
            break;
        case PT_MV_STRING8:
            pv.Value.MVszA.cValues = 3;
            pv.Value.MVszA.lppszA = buffa;
            buffa[0] = szHiA;
            buffa[1] = szHiA;
            buffa[2] = szHiA;
            exp = ULHILEN * 3 + 3 * sizeof(char*) + sizeof(pv);
            break;
        case PT_MV_UNICODE:
            pv.Value.MVszW.cValues = 3;
            pv.Value.MVszW.lppszW = buffw;
            buffw[0] = szHiW;
            buffw[1] = szHiW;
            buffw[2] = szHiW;
            exp = ULHILEN * 3 * sizeof(WCHAR) + 3 * sizeof(WCHAR*) + sizeof(pv);
            break;
        case PT_MV_BINARY:
            pv.Value.MVbin.cValues = 3;
            pv.Value.MVbin.lpbin = buffbin;
            buffbin[0].cb = 17;
            buffbin[0].lpb = (LPBYTE)&iid;
            buffbin[1].cb = 2;
            buffbin[1].lpb = (LPBYTE)&iid;
            buffbin[2].cb = 1;
            buffbin[2].lpb = (LPBYTE)&iid;
            exp = 20 + sizeof(pv) + sizeof(SBinary) * 3;
            break;
        default:
            exp = 0;
        }

        ulRet = 0xffffffff;
        res = pScCountProps(1, &pv, &ulRet);
        if (!exp) {
            success = res == MAPI_E_INVALID_PARAMETER && ulRet == 0xffffffff;
            ok(success, "pt= %ld: Expected failure, got %ld, ret=0x%08lX\n",
               pt, ulRet, res);
        }
        else {
            success = res == S_OK && ulRet == exp;
            ok(success, "pt= %ld: Expected %ld, got %ld, ret=0x%08lX\n",
               pt, exp, ulRet, res);
        }
    }

}

static void test_ScCopyRelocProps(void)
{
    static char szTestA[] = "Test";
    char buffer[512], buffer2[512], *lppszA[1];
    SPropValue pvProp, *lpResProp = (LPSPropValue)buffer;
    ULONG ulCount;
    SCODE sc;

    if (!pScCopyProps || !pScRelocProps)
    {
        win_skip("SPropValue copy functions are not available\n");
        return;
    }

    pvProp.ulPropTag = PROP_TAG(PT_MV_STRING8, 1u);

    lppszA[0] = szTestA;
    pvProp.Value.MVszA.cValues = 1;
    pvProp.Value.MVszA.lppszA = lppszA;
    ulCount = 0;

    sc = pScCopyProps(1, &pvProp, buffer, &ulCount);
    ok(sc == S_OK, "wrong ret %ld\n", sc);
    if(sc == S_OK)
    {
        ok(lpResProp->ulPropTag == pvProp.ulPropTag, "wrong tag %lx\n",lpResProp->ulPropTag);
        ok(lpResProp->Value.MVszA.cValues == 1, "wrong cValues %ld\n", lpResProp->Value.MVszA.cValues);
        ok(lpResProp->Value.MVszA.lppszA[0] == buffer + sizeof(SPropValue) + sizeof(char*),
           "wrong lppszA[0] %p\n",lpResProp->Value.MVszA.lppszA[0]);
        ok(ulCount == sizeof(SPropValue) + sizeof(char*) + 5, "wrong count %ld\n", ulCount);
        ok(!strcmp(lpResProp->Value.MVszA.lppszA[0], szTestA),
           "wrong string '%s'\n", lpResProp->Value.MVszA.lppszA[0]);
    }

    memcpy(buffer2, buffer, sizeof(buffer));

    /* Clear the data in the source buffer. Since pointers in the copied buffer
     * refer to the source buffer, this proves that native always assumes that
     * the copied buffers pointers are bad (needing to be relocated first).
     */
    memset(buffer, 0, sizeof(buffer));
    ulCount = 0;

    sc = pScRelocProps(1, (LPSPropValue)buffer2, buffer, buffer2, &ulCount);
    lpResProp = (LPSPropValue)buffer2;

    ok(sc == S_OK, "wrong ret %ld\n", sc);
    if(sc == S_OK)
    {
        ok(lpResProp->ulPropTag == pvProp.ulPropTag, "wrong tag %lx\n",lpResProp->ulPropTag);
        ok(lpResProp->Value.MVszA.cValues == 1, "wrong cValues %ld\n", lpResProp->Value.MVszA.cValues);
        ok(lpResProp->Value.MVszA.lppszA[0] == buffer2 + sizeof(SPropValue) + sizeof(char*),
           "wrong lppszA[0] %p\n",lpResProp->Value.MVszA.lppszA[0]);
        /* Native has a bug whereby it calculates the size correctly when copying
         * but when relocating does not (presumably it uses UlPropSize() which
         * ignores multivalue pointers). Wine returns the correct value.
         */
        ok(ulCount == sizeof(SPropValue) + sizeof(char*) + 5 || ulCount == sizeof(SPropValue) + 5,
           "wrong count %ld\n", ulCount);
        ok(!strcmp(lpResProp->Value.MVszA.lppszA[0], szTestA),
           "wrong string '%s'\n", lpResProp->Value.MVszA.lppszA[0]);
    }

    /* Native crashes with lpNew or lpOld set to NULL so skip testing this */
}

static void test_LpValFindProp(void)
{
    SPropValue pvProp, *pRet;
    ULONG i;

    if (!pLpValFindProp)
    {
        win_skip("LpValFindProp is not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(ptTypes); i++)
    {
        pvProp.ulPropTag = PROP_TAG(ptTypes[i], 1u);

        pRet = pLpValFindProp(PROP_TAG(ptTypes[i], 1u), 1u, &pvProp);
        ok(pRet == &pvProp,
           "LpValFindProp[%ld]: Didn't find existing property id/type\n",
           ptTypes[i]);

        pRet = pLpValFindProp(PROP_TAG(ptTypes[i], 0u), 1u, &pvProp);
        ok(pRet == NULL, "LpValFindProp[%ld]: Found nonexistent property id\n",
           ptTypes[i]);

        pRet = pLpValFindProp(PROP_TAG(PT_NULL, 0u), 1u, &pvProp);
        ok(pRet == NULL, "LpValFindProp[%ld]: Found nonexistent property id/type\n",
           ptTypes[i]);

        pRet = pLpValFindProp(PROP_TAG(PT_NULL, 1u), 1u, &pvProp);
        ok(pRet == &pvProp,
           "LpValFindProp[%ld]: Didn't find existing property id\n",
           ptTypes[i]);
    }
}

static void test_FBadRglpszA(void)
{
    LPSTR lpStrs[4];
    static CHAR szString[] = "A String";
    BOOL bRet;

    if (!pFBadRglpszA)
    {
        win_skip("FBadRglpszA is not available\n");
        return;
    }

    bRet = pFBadRglpszA(NULL, 10);
    ok(bRet == TRUE, "FBadRglpszA(Null): expected TRUE, got FALSE\n");

    lpStrs[0] = lpStrs[1] = lpStrs[2] = lpStrs[3] = NULL;
    bRet = pFBadRglpszA(lpStrs, 4);
    ok(bRet == TRUE, "FBadRglpszA(Nulls): expected TRUE, got FALSE\n");

    lpStrs[0] = lpStrs[1] = lpStrs[2] = szString;
    bRet = pFBadRglpszA(lpStrs, 3);
    ok(bRet == FALSE, "FBadRglpszA(valid): expected FALSE, got TRUE\n");

    bRet = pFBadRglpszA(lpStrs, 4);
    ok(bRet == TRUE, "FBadRglpszA(1 invalid): expected TRUE, got FALSE\n");
}

static void test_FBadRglpszW(void)
{
    LPWSTR lpStrs[4];
    static WCHAR szString[] = L"A String";
    BOOL bRet;

    if (!pFBadRglpszW)
    {
        win_skip("FBadRglpszW is not available\n");
        return;
    }

    bRet = pFBadRglpszW(NULL, 10);
    ok(bRet == TRUE, "FBadRglpszW(Null): expected TRUE, got FALSE\n");

    lpStrs[0] = lpStrs[1] = lpStrs[2] = lpStrs[3] = NULL;
    bRet = pFBadRglpszW(lpStrs, 4);
    ok(bRet == TRUE, "FBadRglpszW(Nulls): expected TRUE, got FALSE\n");

    lpStrs[0] = lpStrs[1] = lpStrs[2] = szString;
    bRet = pFBadRglpszW(lpStrs, 3);
    ok(bRet == FALSE, "FBadRglpszW(valid): expected FALSE, got TRUE\n");

    bRet = pFBadRglpszW(lpStrs, 4);
    ok(bRet == TRUE, "FBadRglpszW(1 invalid): expected TRUE, got FALSE\n");
}

static void test_FBadRowSet(void)
{
    ULONG ulRet;

    if (!pFBadRowSet)
    {
        win_skip("FBadRowSet is not available\n");
        return;
    }

    ulRet = pFBadRowSet(NULL);
    ok(ulRet != 0, "FBadRow(null): Expected non-zero, got 0\n");

    /* FIXME */
}

static void test_FBadPropTag(void)
{
    ULONG pt, res;

    if (!pFBadPropTag)
    {
        win_skip("FBadPropTag is not available\n");
        return;
    }

    for (pt = 0; pt < PROP_ID_INVALID; pt++)
    {
        BOOL bBad = TRUE;

        switch (pt & (~MV_FLAG & PROP_TYPE_MASK))
        {
        case PT_UNSPECIFIED:
        case PT_NULL: case PT_I2: case PT_I4: case PT_R4:
        case PT_R8: case PT_CURRENCY: case PT_APPTIME:
        case PT_ERROR: case PT_BOOLEAN: case PT_OBJECT:
        case PT_I8: case PT_STRING8: case PT_UNICODE:
        case PT_SYSTIME: case PT_CLSID: case PT_BINARY:
            bBad = FALSE;
        }

        res = pFBadPropTag(pt);
        if (bBad)
            ok(res != 0, "pt= %ld: Expected non-zero, got 0\n", pt);
        else
            ok(res == 0,
               "pt= %ld: Expected zero, got %ld\n", pt, res);
    }
}

static void test_FBadRow(void)
{
    ULONG ulRet;

    if (!pFBadRow)
    {
        win_skip("FBadRow is not available\n");
        return;
    }

    ulRet = pFBadRow(NULL);
    ok(ulRet != 0, "FBadRow(null): Expected non-zero, got 0\n");

    /* FIXME */
}

static void test_FBadProp(void)
{
    static WCHAR szEmpty[] = L"";
    GUID iid;
    ULONG pt, res;
    SPropValue pv;

    if (!pFBadProp)
    {
        win_skip("FBadProp is not available\n");
        return;
    }

    for (pt = 0; pt < PROP_ID_INVALID; pt++)
    {
        BOOL bBad = TRUE;

        memset(&pv, 0, sizeof(pv));
        pv.ulPropTag = pt;

        /* Note that MV values are valid below because their array count is 0,
         * so no pointers are validated.
         */
        switch (PROP_TYPE(pt))
        {
        case (MV_FLAG|PT_UNSPECIFIED):
        case PT_UNSPECIFIED:
        case (MV_FLAG|PT_NULL):
        case PT_NULL:
        case PT_MV_I2:
        case PT_I2:
        case PT_MV_I4:
        case PT_I4:
        case PT_MV_I8:
        case PT_I8:
        case PT_MV_R4:
        case PT_R4:
        case PT_MV_R8:
        case PT_R8:
        case PT_MV_CURRENCY:
        case PT_CURRENCY:
        case PT_MV_APPTIME:
        case PT_APPTIME:
        case (MV_FLAG|PT_ERROR):
        case PT_ERROR:
        case (MV_FLAG|PT_BOOLEAN):
        case PT_BOOLEAN:
        case (MV_FLAG|PT_OBJECT):
        case PT_OBJECT:
        case PT_MV_STRING8:
        case PT_MV_UNICODE:
        case PT_MV_SYSTIME:
        case PT_SYSTIME:
        case PT_MV_BINARY:
        case PT_BINARY:
        case PT_MV_CLSID:
            bBad = FALSE;
            break;
        case PT_STRING8:
        case PT_UNICODE:
            pv.Value.lpszW = szEmpty;
            bBad = FALSE;
            break;
        case PT_CLSID:
            pv.Value.lpguid = &iid;
            bBad = FALSE;
            break;
        }

        res = pFBadProp(&pv);
        if (bBad)
            ok(res != 0, "pt= %ld: Expected non-zero, got 0\n", pt);
        else
            ok(res == 0,
               "pt= %ld: Expected zero, got %ld\n", pt, res);
    }
}

static void test_FBadColumnSet(void)
{
    SPropTagArray pta;
    ULONG pt, res;

    if (!pFBadColumnSet)
    {
        win_skip("FBadColumnSet is not available\n");
        return;
    }

    res = pFBadColumnSet(NULL);
    ok(res != 0, "(null): Expected non-zero, got 0\n");

    pta.cValues = 1;

    for (pt = 0; pt < PROP_ID_INVALID; pt++)
    {
        BOOL bBad = TRUE;

        pta.aulPropTag[0] = pt;

        switch (pt & (~MV_FLAG & PROP_TYPE_MASK))
        {
        case PT_UNSPECIFIED:
        case PT_NULL:
        case PT_I2:
        case PT_I4:
        case PT_R4:
        case PT_R8:
        case PT_CURRENCY:
        case PT_APPTIME:
        case PT_BOOLEAN:
        case PT_OBJECT:
        case PT_I8:
        case PT_STRING8:
        case PT_UNICODE:
        case PT_SYSTIME:
        case PT_CLSID:
        case PT_BINARY:
            bBad = FALSE;
        }
        if (pt == (MV_FLAG|PT_ERROR))
            bBad = FALSE;

        res = pFBadColumnSet(&pta);
        if (bBad)
            ok(res != 0, "pt= %ld: Expected non-zero, got 0\n", pt);
        else
            ok(res == 0,
               "pt= %ld: Expected zero, got %ld\n", pt, res);
    }
}


static void test_IProp(void)
{
    IPropData *lpIProp;
    LPMAPIERROR lpError;
    LPSPropProblemArray lpProbs;
    LPSPropValue lpProps;
    LPSPropTagArray lpTags;
    SPropValue pvs[2];
    SizedSPropTagArray(2,tags);
    ULONG access[2], count;
    SCODE sc;

    if (!pCreateIProp)
    {
        win_skip("CreateIProp is not available\n");
        return;
    }

    memset(&tags, 0 , sizeof(tags));

    /* Create the object */
    lpIProp = NULL;
    sc = pCreateIProp(&IID_IMAPIPropData, (ALLOCATEBUFFER *)pMAPIAllocateBuffer, (ALLOCATEMORE*)pMAPIAllocateMore,
                      (FREEBUFFER *)pMAPIFreeBuffer, NULL, &lpIProp);
    ok(sc == S_OK && lpIProp,
       "CreateIProp: expected S_OK, non-null, got 0x%08lX,%p\n", sc, lpIProp);

    if (sc != S_OK || !lpIProp)
        return;

    /* GetLastError - No errors set */
    lpError = NULL;
    sc = IPropData_GetLastError(lpIProp, E_INVALIDARG, 0, &lpError);
    ok(sc == S_OK && !lpError,
       "GetLastError: Expected S_OK, null, got 0x%08lX,%p\n", sc, lpError);

    /* Get prop tags - succeeds returning 0 items */
    lpTags = NULL;
    sc = IPropData_GetPropList(lpIProp, 0, &lpTags);
    ok(sc == S_OK && lpTags && lpTags->cValues == 0,
       "GetPropList(empty): Expected S_OK, non-null, 0, got 0x%08lX,%p,%ld\n",
        sc, lpTags, lpTags ? lpTags->cValues : 0);
    if (lpTags)
        pMAPIFreeBuffer(lpTags);

    /* Get props - succeeds returning 0 items */
    lpProps = NULL;
    count = 0;
    tags.cValues = 1;
    tags.aulPropTag[0] = PR_IMPORTANCE;
    sc = IPropData_GetProps(lpIProp, (LPSPropTagArray)&tags, 0, &count, &lpProps);
    ok(sc == MAPI_W_ERRORS_RETURNED && lpProps && count == 1,
       "GetProps(empty): Expected ERRORS_RETURNED, non-null, 1, got 0x%08lX,%p,%ld\n",
       sc, lpProps, count);
    if (lpProps && count > 0)
    {
        ok(lpProps[0].ulPropTag == CHANGE_PROP_TYPE(PR_IMPORTANCE,PT_ERROR),
           "GetProps(empty): Expected %x, got %lx\n",
           CHANGE_PROP_TYPE(PR_IMPORTANCE,PT_ERROR), lpProps[0].ulPropTag);

        pMAPIFreeBuffer(lpProps);
    }

    /* Add (NULL) - Can't add NULLs */
    lpProbs = NULL;
    pvs[0].ulPropTag = PROP_TAG(PT_NULL,0x01);
    sc = IPropData_SetProps(lpIProp, 1, pvs, &lpProbs);
    ok(sc == MAPI_E_INVALID_PARAMETER && !lpProbs,
       "SetProps(): Expected INVALID_PARAMETER, null, got 0x%08lX,%p\n",
       sc, lpProbs);

    /* Add (OBJECT) - Can't add OBJECTs */
    lpProbs = NULL;
    pvs[0].ulPropTag = PROP_TAG(PT_OBJECT,0x01);
    sc = IPropData_SetProps(lpIProp, 1, pvs, &lpProbs);
    ok(sc == MAPI_E_INVALID_PARAMETER && !lpProbs,
       "SetProps(OBJECT): Expected INVALID_PARAMETER, null, got 0x%08lX,%p\n",
       sc, lpProbs);

    /* Add - Adds value */
    lpProbs = NULL;
    pvs[0].ulPropTag = PR_IMPORTANCE;
    sc = IPropData_SetProps(lpIProp, 1, pvs, &lpProbs);
    ok(sc == S_OK && !lpProbs,
       "SetProps(ERROR): Expected S_OK, null, got 0x%08lX,%p\n", sc, lpProbs);

    /* Get prop list - returns 1 item */
    lpTags = NULL;
    IPropData_GetPropList(lpIProp, 0, &lpTags);
    ok(sc == S_OK && lpTags && lpTags->cValues == 1,
       "GetPropList: Expected S_OK, non-null, 1, got 0x%08lX,%p,%ld\n",
        sc, lpTags, lpTags ? lpTags->cValues : 0);
    if (lpTags && lpTags->cValues > 0)
    {
        ok(lpTags->aulPropTag[0] == PR_IMPORTANCE,
           "GetPropList: Expected %x, got %lx\n",
           PR_IMPORTANCE, lpTags->aulPropTag[0]);
        pMAPIFreeBuffer(lpTags);
    }

    /* Set access to read and write */
    sc = IPropData_HrSetObjAccess(lpIProp, IPROP_READWRITE);
    ok(sc == S_OK, "SetObjAccess(WRITE): Expected S_OK got 0x%08lX\n", sc);

    tags.cValues = 1;
    tags.aulPropTag[0] = PR_IMPORTANCE;

    /* Set item access (bad access) - Fails */
    access[0] = 0;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == MAPI_E_INVALID_PARAMETER,
       "SetPropAccess(0): Expected INVALID_PARAMETER got 0x%08lX\n",sc);
    access[0] = IPROP_READWRITE;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == MAPI_E_INVALID_PARAMETER,
       "SetPropAccess(RW): Expected INVALID_PARAMETER got 0x%08lX\n",sc);
    access[0] = IPROP_CLEAN;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == MAPI_E_INVALID_PARAMETER,
       "SetPropAccess(C): Expected INVALID_PARAMETER got 0x%08lX\n",sc);

    /* Set item access to read/write/clean */
    tags.cValues = 1;
    tags.aulPropTag[0] = PR_IMPORTANCE;
    access[0] = IPROP_READWRITE|IPROP_CLEAN;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == S_OK, "SetPropAccess(RW/C): Expected S_OK got 0x%08lX\n",sc);

    /* Set object access to read only */
    sc = IPropData_HrSetObjAccess(lpIProp, IPROP_READONLY);
    ok(sc == S_OK, "SetObjAccess(READ): Expected S_OK got 0x%08lX\n", sc);

    /* Set item access to read/write/dirty - doesn't care about RO object */
    access[0] = IPROP_READONLY|IPROP_DIRTY;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == S_OK, "SetPropAccess(WRITE): Expected S_OK got 0x%08lX\n", sc);

    /* Delete any item when set to read only - Error */
    lpProbs = NULL;
    tags.aulPropTag[0] = PR_RESPONSE_REQUESTED;
    sc = IPropData_DeleteProps(lpIProp, (LPSPropTagArray)&tags, &lpProbs);
    ok(sc == E_ACCESSDENIED && !lpProbs,
       "DeleteProps(nonexistent): Expected E_ACCESSDENIED null got 0x%08lX %p\n",
       sc, lpProbs);

    /* Set access to read and write */
    sc = IPropData_HrSetObjAccess(lpIProp, IPROP_READWRITE);
    ok(sc == S_OK, "SetObjAccess(WRITE): Expected S_OK got 0x%08lX\n", sc);

    /* Delete nonexistent item - No error */
    lpProbs = NULL;
    tags.aulPropTag[0] = PR_RESPONSE_REQUESTED;
    sc = IPropData_DeleteProps(lpIProp, (LPSPropTagArray)&tags, &lpProbs);
    ok(sc == S_OK && !lpProbs,
       "DeleteProps(nonexistent): Expected S_OK null got 0x%08lX %p\n",
       sc, lpProbs);

    /* Delete existing item (r/o) - No error, but lpProbs populated */
    lpProbs = NULL;
    tags.aulPropTag[0] = PR_IMPORTANCE;
    sc = IPropData_DeleteProps(lpIProp, (LPSPropTagArray)&tags, &lpProbs);
    ok(sc == S_OK && lpProbs,
       "DeleteProps(RO): Expected S_OK non-null got 0x%08lX %p\n", sc, lpProbs);

    if (lpProbs && lpProbs->cProblem > 0)
    {
        ok(lpProbs->cProblem == 1 &&
           lpProbs->aProblem[0].ulIndex == 0 &&
           lpProbs->aProblem[0].ulPropTag == PR_IMPORTANCE &&
           lpProbs->aProblem[0].scode == E_ACCESSDENIED,
           "DeleteProps(RO): Expected (1,0,%x,%lx) got (%ld,%lx,%lx)\n",
            PR_IMPORTANCE, E_ACCESSDENIED,
            lpProbs->aProblem[0].ulIndex, lpProbs->aProblem[0].ulPropTag,
            lpProbs->aProblem[0].scode);
        pMAPIFreeBuffer(lpProbs);
    }

    lpProbs = NULL;
    tags.cValues = 1;
    tags.aulPropTag[0] = PR_RESPONSE_REQUESTED;
    IPropData_HrAddObjProps(lpIProp, (LPSPropTagArray)&tags, &lpProbs);
    ok(sc == S_OK && !lpProbs,
       "AddObjProps(RO): Expected S_OK null got 0x%08lX %p\n", sc, lpProbs);

    /* Get prop list - returns 1 item */
    lpTags = NULL;
    IPropData_GetPropList(lpIProp, 0, &lpTags);
    ok(sc == S_OK && lpTags && lpTags->cValues == 1,
       "GetPropList: Expected S_OK, non-null, 1, got 0x%08lX,%p,%ld\n",
        sc, lpTags, lpTags ? lpTags->cValues : 0);
    if (lpTags && lpTags->cValues > 0)
    {
        ok(lpTags->aulPropTag[0] == PR_IMPORTANCE,
           "GetPropList: Expected %x, got %lx\n",
           PR_IMPORTANCE, lpTags->aulPropTag[0]);
        pMAPIFreeBuffer(lpTags);
    }

    /* Set item to r/w again */
    access[0] = IPROP_READWRITE|IPROP_DIRTY;
    sc = IPropData_HrSetPropAccess(lpIProp, (LPSPropTagArray)&tags, access);
    ok(sc == S_OK, "SetPropAccess(WRITE): Expected S_OK got 0x%08lX\n", sc);

    /* Delete existing item (r/w) - No error, no problems */
    lpProbs = NULL;
    sc = IPropData_DeleteProps(lpIProp, (LPSPropTagArray)&tags, &lpProbs);
    ok(sc == S_OK && !lpProbs,
       "DeleteProps(RO): Expected S_OK null got 0x%08lX %p\n", sc, lpProbs);

    /* Free the list */
    IPropData_Release(lpIProp);
}

START_TEST(prop)
{
    SCODE ret;

    if (!HaveDefaultMailClient())
    {
        win_skip("No default mail client installed\n");
        return;
    }

    if(!InitFuncPtrs())
    {
        win_skip("Needed functions are not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pScInitMapiUtil(0);
    if ((ret != S_OK) && (GetLastError() == ERROR_PROC_NOT_FOUND))
    {
        win_skip("ScInitMapiUtil is not implemented\n");
        FreeLibrary(hMapi32);
        return;
    }
    else if ((ret == E_FAIL) && (GetLastError() == ERROR_INVALID_HANDLE))
    {
        win_skip("ScInitMapiUtil doesn't work on some Win98 and WinME systems\n");
        FreeLibrary(hMapi32);
        return;
    }

    test_PropCopyMore();
    test_UlPropSize();

    /* We call MAPIInitialize here for the benefit of native extended MAPI
     * providers which crash in the FPropContainsProp tests when MAPIInitialize
     * has not been called. Since MAPIInitialize is irrelevant for FPropContainsProp
     * on Wine, we do not care whether MAPIInitialize succeeds. */
    if (pMAPIInitialize)
        ret = pMAPIInitialize(NULL);
    test_FPropContainsProp();
    if (pMAPIUninitialize && ret == S_OK)
        pMAPIUninitialize();

    test_FPropCompareProp();
    test_LPropCompareProp();
    test_PpropFindProp();
    test_ScCountProps();
    test_ScCopyRelocProps();
    test_LpValFindProp();
    test_FBadRglpszA();
    test_FBadRglpszW();
    test_FBadRowSet();
    test_FBadPropTag();
    test_FBadRow();
    test_FBadProp();
    test_FBadColumnSet();

    test_IProp();

    pDeinitMapiUtil();
    FreeLibrary(hMapi32);
}
