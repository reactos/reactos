/*
 * Marshaling Tests
 *
 * Copyright 2004 Robert Shearman
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
#define CONST_VTABLE

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propidl.h" /* for LPSAFEARRAY_User* routines */

#include "wine/test.h"

#if (__STDC__ && !defined(_FORCENAMELESSUNION)) || defined(NONAMELESSUNION)
# define V_U2(A)  ((A)->n1.n2)
#else
# define V_U2(A)  (*(A))
#endif

#ifdef __REACTOS__
typedef struct
{
    IUnknown IUnknown_iface;
    ULONG refs;
} HeapUnknown;

static const IUnknownVtbl HeapUnknown_Vtbl;
#endif

static HRESULT (WINAPI *pSafeArrayGetIID)(SAFEARRAY*,GUID*);
static HRESULT (WINAPI *pSafeArrayGetVartype)(SAFEARRAY*,VARTYPE*);
static HRESULT (WINAPI *pVarBstrCmp)(BSTR,BSTR,LCID,ULONG);

static inline SF_TYPE get_union_type(SAFEARRAY *psa)
{
    VARTYPE vt;
    HRESULT hr;

    hr = pSafeArrayGetVartype(psa, &vt);
    if (FAILED(hr))
    {
        if(psa->fFeatures & FADF_VARIANT) return SF_VARIANT;

        switch(psa->cbElements)
        {
        case 1: vt = VT_I1; break;
        case 2: vt = VT_I2; break;
        case 4: vt = VT_I4; break;
        case 8: vt = VT_I8; break;
        default: return 0;
        }
    }

    if (psa->fFeatures & FADF_HAVEIID)
        return SF_HAVEIID;

    switch (vt)
    {
    case VT_I1:
    case VT_UI1:      return SF_I1;
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return SF_I2;
    case VT_INT:
    case VT_UINT:
    case VT_I4:
    case VT_UI4:
    case VT_R4:       return SF_I4;
    case VT_DATE:
    case VT_CY:
    case VT_R8:
    case VT_I8:
    case VT_UI8:      return SF_I8;
    case VT_INT_PTR:
    case VT_UINT_PTR: return (sizeof(UINT_PTR) == 4 ? SF_I4 : SF_I8);
    case VT_BSTR:     return SF_BSTR;
    case VT_DISPATCH: return SF_DISPATCH;
    case VT_VARIANT:  return SF_VARIANT;
    case VT_UNKNOWN:  return SF_UNKNOWN;
    /* Note: Return a non-zero size to indicate vt is valid. The actual size
     * of a UDT is taken from the result of IRecordInfo_GetSize().
     */
    case VT_RECORD:   return SF_RECORD;
    default:          return SF_ERROR;
    }
}

static ULONG get_cell_count(const SAFEARRAY *psa)
{
    const SAFEARRAYBOUND* psab = psa->rgsabound;
    USHORT cCount = psa->cDims;
    ULONG ulNumCells = 1;

    while (cCount--)
    {
         if (!psab->cElements)
            return 0;
        ulNumCells *= psab->cElements;
        psab++;
    }
    return ulNumCells;
}

static DWORD elem_wire_size(LPSAFEARRAY lpsa, SF_TYPE sftype)
{
#ifdef __REACTOS__
    switch (sftype)
    {
    case SF_HAVEIID:
    case SF_UNKNOWN:
    case SF_DISPATCH:
    case SF_BSTR:
#else
    if (sftype == SF_BSTR)
#endif
        return sizeof(DWORD);
#ifdef __REACTOS__

    default:
#else
    else
#endif
        return lpsa->cbElements;
#ifdef __REACTOS__
    }
#endif
}

static void check_safearray(void *buffer, LPSAFEARRAY lpsa)
{
    unsigned char *wiresa = buffer;
    const SAFEARRAYBOUND *bounds;
    VARTYPE vt;
    SF_TYPE sftype;
    ULONG cell_count;
    int i;

    if(!lpsa)
    {
        ok(*(DWORD *)wiresa == 0, "wiresa + 0x0 should be NULL instead of 0x%08x\n", *(DWORD *)wiresa);
        return;
    }

    if (!pSafeArrayGetVartype || !pSafeArrayGetIID)
        return;

#ifdef __REACTOS__
    /* If FADF_HAVEIID is set, VT will be 0. */
    if((lpsa->fFeatures & FADF_HAVEIID) || FAILED(SafeArrayGetVartype(lpsa, &vt)))
#else
    if(FAILED(pSafeArrayGetVartype(lpsa, &vt)))
#endif
        vt = 0;

    sftype = get_union_type(lpsa);
    cell_count = get_cell_count(lpsa);

    ok(*(DWORD *)wiresa, "wiresa + 0x0 should be non-NULL instead of 0x%08x\n", *(DWORD *)wiresa); /* win2k: this is lpsa. winxp: this is 0x00000001 */
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == lpsa->cDims, "wiresa + 0x4 should be lpsa->cDims instead of 0x%08x\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cDims, "wiresa + 0x8 should be lpsa->cDims instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == lpsa->fFeatures, "wiresa + 0xa should be lpsa->fFeatures instead of 0x%08x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == elem_wire_size(lpsa, sftype), "wiresa + 0xc should be 0x%08x instead of 0x%08x\n", elem_wire_size(lpsa, sftype), *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(WORD *)wiresa == lpsa->cLocks, "wiresa + 0x10 should be lpsa->cLocks instead of 0x%04x\n", *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(WORD *)wiresa == vt, "wiresa + 0x12 should be %04x instead of 0x%04x\n", vt, *(WORD *)wiresa);
    wiresa += sizeof(WORD);
    ok(*(DWORD *)wiresa == sftype, "wiresa + 0x14 should be %08x instead of 0x%08x\n", (DWORD)sftype, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x18 should be %u instead of %u\n", cell_count, *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    ok(*(DWORD *)wiresa, "wiresa + 0x1c should be non-zero instead of 0x%08x\n", *(DWORD *)wiresa);
    wiresa += sizeof(DWORD);
    if(sftype == SF_HAVEIID)
    {
        GUID guid;
        pSafeArrayGetIID(lpsa, &guid);
        ok(IsEqualGUID(&guid, wiresa), "guid mismatch\n");
        wiresa += sizeof(GUID);
    }

    /* bounds are marshaled in natural dimensions order */
    bounds = (SAFEARRAYBOUND*)wiresa;
    for(i=0; i<lpsa->cDims; i++)
    {
        ok(memcmp(bounds, &lpsa->rgsabound[lpsa->cDims-i-1], sizeof(SAFEARRAYBOUND)) == 0,
           "bounds mismatch for dimension %d, got (%d,%d), expected (%d,%d)\n", i,
            bounds->lLbound, bounds->cElements, lpsa->rgsabound[lpsa->cDims-i-1].lLbound,
            lpsa->rgsabound[lpsa->cDims-i-1].cElements);
        bounds++;
    }

    wiresa += sizeof(lpsa->rgsabound[0]) * lpsa->cDims;

    ok(*(DWORD *)wiresa == cell_count, "wiresa + 0x28 should be %u instead of %u\n", cell_count, *(DWORD*)wiresa);
    wiresa += sizeof(DWORD);
    /* elements are now pointed to by wiresa */
}

static void * WINAPI user_allocate(SIZE_T size)
{
    ok(0, "unexpected user_allocate call\n");
    return CoTaskMemAlloc(size);
}

static void WINAPI user_free(void *p)
{
    ok(0, "unexpected user_free call\n");
    CoTaskMemFree(p);
}

static void init_user_marshal_cb(USER_MARSHAL_CB *umcb,
                                 PMIDL_STUB_MESSAGE stub_msg,
                                 PRPC_MESSAGE rpc_msg, unsigned char *buffer,
                                 unsigned int size, MSHCTX context)
{
    memset(rpc_msg, 0, sizeof(*rpc_msg));
    rpc_msg->Buffer = buffer;
    rpc_msg->BufferLength = size;

    memset(stub_msg, 0, sizeof(*stub_msg));
    stub_msg->RpcMsg = rpc_msg;
    stub_msg->Buffer = buffer;
    stub_msg->pfnAllocate = user_allocate;
    stub_msg->pfnFree = user_free;

    memset(umcb, 0, sizeof(*umcb));
    umcb->Flags = MAKELONG(context, NDR_LOCAL_DATA_REPRESENTATION);
    umcb->pStubMsg = stub_msg;
    umcb->Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb->CBType = buffer ? USER_MARSHAL_CB_UNMARSHALL : USER_MARSHAL_CB_BUFFER_SIZE;
}

static void test_marshal_LPSAFEARRAY(void)
{
#ifdef __REACTOS__
    HeapUnknown *heap_unknown[10];
#endif
    unsigned char *buffer, *next;
#ifdef __REACTOS__
    ULONG size, expected, size2;
#else
    ULONG size, expected;
#endif
    LPSAFEARRAY lpsa;
    LPSAFEARRAY lpsa2 = NULL;
    SAFEARRAYBOUND sab[2];
    RPC_MESSAGE rpc_msg;
    MIDL_STUB_MESSAGE stub_msg;
    USER_MARSHAL_CB umcb;
    HRESULT hr;
    VARTYPE vt, vt2;
    OLECHAR *values[10];
    int expected_bstr_size;
    int i;
    LONG indices[1];

    sab[0].lLbound = 5;
    sab[0].cElements = 10;

    lpsa = SafeArrayCreate(VT_I2, 1, sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;

    lpsa->cLocks = 7;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = (44 + 1 + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1);
    expected += sab[0].cElements * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 44 + sab[0].cElements * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok(next - buffer == expected, "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    ok(lpsa->cLocks == 7, "got lock count %u\n", lpsa->cLocks);

    check_safearray(buffer, lpsa);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal\n");
    if (pSafeArrayGetVartype)
    {
        pSafeArrayGetVartype(lpsa, &vt);
        pSafeArrayGetVartype(lpsa2, &vt2);
        ok(vt == vt2, "vts differ %x %x\n", vt, vt2);
    }
    ok(lpsa2->cLocks == 0, "got lock count %u, expected 0\n", lpsa2->cLocks);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    ok(!lpsa2, "lpsa2 was not set to 0 by LPSAFEARRAY_UserFree\n");
    HeapFree(GetProcessHeap(), 0, buffer);
    lpsa->cLocks = 0;
    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* use two dimensions */
    sab[0].lLbound = 5;
    sab[0].cElements = 10;
    sab[1].lLbound = 1;
    sab[1].cElements = 2;

    lpsa = SafeArrayCreate(VT_I2, 2, sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;

    lpsa->cLocks = 7;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = (44 + 1 + +sizeof(SAFEARRAYBOUND) + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1);
    expected += max(sab[0].cElements, sab[1].cElements) * lpsa->cDims * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 52 + max(sab[0].cElements, sab[1].cElements) * lpsa->cDims * sizeof(USHORT);
    ok(size == expected || size == expected + 12, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok(next - buffer == expected, "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    ok(lpsa->cLocks == 7, "got lock count %u\n", lpsa->cLocks);

    check_safearray(buffer, lpsa);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal\n");
    if (pSafeArrayGetVartype)
    {
        pSafeArrayGetVartype(lpsa, &vt);
        pSafeArrayGetVartype(lpsa2, &vt2);
        ok(vt == vt2, "vts differ %x %x\n", vt, vt2);
    }
    ok(lpsa2->cLocks == 0, "got lock count %u, expected 0\n", lpsa2->cLocks);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    HeapFree(GetProcessHeap(), 0, buffer);
    lpsa->cLocks = 0;
    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* test NULL safe array */
    lpsa = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 4;
    ok(size == expected, "size should be 4 bytes, not %d\n", size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok(next - buffer == expected, "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    check_safearray(buffer, lpsa);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    ok(lpsa2 == NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);
    HeapFree(GetProcessHeap(), 0, buffer);

    sab[0].lLbound = 5;
    sab[0].cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, sab);
    *(double *)lpsa->pvData = 3.1415;

    lpsa->cLocks = 7;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = (44 + 1 + (sizeof(double) - 1)) & ~(sizeof(double) - 1);
    expected += sab[0].cElements * sizeof(double);
    ok(size == expected || size == expected + 16, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    expected = (44 + (sizeof(double) - 1)) & ~(sizeof(double) - 1);
    expected += sab[0].cElements * sizeof(double);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size == expected || size == expected + 8, /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok(next - buffer == expected || broken(next - buffer + sizeof(DWORD) == expected),
            "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);

    check_safearray(buffer, lpsa);

    HeapFree(GetProcessHeap(), 0, buffer);
    lpsa->cLocks = 0;
    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* VARTYPE-less arrays can be marshaled if cbElements is 1,2,4 or 8 as type SF_In */
    hr = SafeArrayAllocDescriptor(1, &lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);
    lpsa->cbElements = 8;
    lpsa->rgsabound[0].lLbound = 2;
    lpsa->rgsabound[0].cElements = 48;
    hr = SafeArrayAllocData(lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);

    if (pSafeArrayGetVartype)
    {
        hr = pSafeArrayGetVartype(lpsa, &vt);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr);
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = (44 + lpsa->cbElements - 1) & ~(lpsa->cbElements - 1);
    expected += lpsa->cbElements * lpsa->rgsabound[0].cElements;
    ok(size == expected || size == expected + 8,  /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok(next - buffer == expected || broken(next - buffer + sizeof(DWORD) == expected),
            "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    check_safearray(buffer, lpsa);
    HeapFree(GetProcessHeap(), 0, buffer);
    hr = SafeArrayDestroyData(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = SafeArrayDestroyDescriptor(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Test an array of VT_BSTR */
    sab[0].lLbound = 3;
    sab[0].cElements = ARRAY_SIZE(values);

    lpsa = SafeArrayCreate(VT_BSTR, 1, sab);
    expected_bstr_size = 0;
    for (i = 0; i < sab[0].cElements; i++)
    {
        int j;
        WCHAR buf[128];
        for (j = 0; j <= i; j++)
            buf[j] = 'a' + j;
        buf[j] = 0;
        indices[0] = i + sab[0].lLbound;
        values[i] = SysAllocString(buf);
        hr = SafeArrayPutElement(lpsa, indices, values[i]);
        ok(hr == S_OK, "Failed to put bstr element hr 0x%x\n", hr);
        expected_bstr_size += (j * sizeof(WCHAR)) + (3 * sizeof(DWORD));
        if (i % 2 == 0) /* Account for DWORD padding.  Works so long as cElements is even */
            expected_bstr_size += sizeof(WCHAR);
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    expected = 44 + (sab[0].cElements * sizeof(DWORD)) + expected_bstr_size;
    todo_wine
    ok(size == expected + sizeof(DWORD) || size  == (expected + sizeof(DWORD) + 12 /* win64 */),
            "size should be %u bytes, not %u\n", expected + (ULONG) sizeof(DWORD), size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    todo_wine
    ok(size == expected || size  == (expected + 12 /* win64 */),
        "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    todo_wine
    ok(next - buffer == expected, "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);

    check_safearray(buffer, lpsa);

    lpsa2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    todo_wine
    ok(next - buffer == expected, "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal, result %p\n", next);

    for (i = 0; i < ARRAY_SIZE(values); i++)
    {
        BSTR gotvalue = NULL;

        if (lpsa2)
        {
            indices[0] = i + sab[0].lLbound;
            hr = SafeArrayGetElement(lpsa2, indices, &gotvalue);
            ok(hr == S_OK, "Failed to get bstr element at hres 0x%x\n", hr);
            if (hr == S_OK)
            {
                if (pVarBstrCmp)
                    ok(pVarBstrCmp(values[i], gotvalue, 0, 0) == VARCMP_EQ, "String %d does not match\n", i);
                SysFreeString(gotvalue);
            }
        }

        SysFreeString(values[i]);
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);

    HeapFree(GetProcessHeap(), 0, buffer);
    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* VARTYPE-less arrays with FADF_VARIANT */
    hr = SafeArrayAllocDescriptor(1, &lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);
    lpsa->cbElements = sizeof(VARIANT);
    lpsa->fFeatures = FADF_VARIANT;
    lpsa->rgsabound[0].lLbound = 2;
    lpsa->rgsabound[0].cElements = 48;
    hr = SafeArrayAllocData(lpsa);
    ok(hr == S_OK, "saad failed %08x\n", hr);

    if (pSafeArrayGetVartype)
    {
        hr = pSafeArrayGetVartype(lpsa, &vt);
        ok(hr == E_INVALIDARG, "ret %08x\n", hr);
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    expected = 44 + 28 * lpsa->rgsabound[0].cElements;
    todo_wine
    ok(size == expected || size == expected + 8,  /* win64 */
       "size should be %u bytes, not %u\n", expected, size);
    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    todo_wine
    ok(next - buffer == expected || broken(next - buffer + sizeof(DWORD) == expected),
            "Marshaled %u bytes, expected %u\n", (ULONG) (next - buffer), expected);
    lpsa->cbElements = 16;  /* VARIANT wire size */
    check_safearray(buffer, lpsa);
    HeapFree(GetProcessHeap(), 0, buffer);
    hr = SafeArrayDestroyData(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = SafeArrayDestroyDescriptor(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

#ifdef __REACTOS__
    /* Test an array of VT_UNKNOWN */
    sab[0].lLbound = 3;
    sab[0].cElements = ARRAY_SIZE(heap_unknown);

    lpsa = SafeArrayCreate(VT_UNKNOWN, 1, sab);

    /*
     * Calculate approximate expected size. Sizes are different between Windows
     * versions, so this should calculate the smallest size that seems sane.
     */
    expected = 60;
    for (i = 0; i < sab[0].cElements; i++)
    {
        HeapUnknown *unk;
        VARIANT v;

        unk = HeapAlloc(GetProcessHeap(), 0, sizeof(*unk));
        unk->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
        unk->refs = 1;

        indices[0] = i + sab[0].lLbound;
        heap_unknown[i] = unk;
        hr = SafeArrayPutElement(lpsa, indices, &heap_unknown[i]->IUnknown_iface);
        ok(hr == S_OK, "Failed to put unknown element hr 0x%x\n", hr);
        ok(unk->refs == 2, "VT_UNKNOWN safearray elem %d, refcount %d\n", i, unk->refs);

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = &unk->IUnknown_iface;
        expected += VARIANT_UserSize(&umcb.Flags, 0, &v) - 20;
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size >= expected || size  >= (expected + 12 ),
        "size should be at least %u bytes, not %u\n", expected, size);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size2 = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    ok(size2 == (size + sizeof(DWORD)) || size2  == (size + sizeof(DWORD) + 12),
            "size should be %u bytes, not %u\n", size + (ULONG) sizeof(DWORD), size2);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok((next - buffer) <= size, "Marshaled %u bytes, expected at most %u\n", (ULONG) (next - buffer), size);
    check_safearray(buffer, lpsa);
todo_wine
    ok(heap_unknown[0]->refs == 3, "Unexpected refcount %d\n", heap_unknown[0]->refs);

    lpsa2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    ok((next - buffer) <= size, "Marshaled %u bytes, expected at most %u\n", (ULONG) (next - buffer), size);
    ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal, result %p\n", next);

    for (i = 0; i < ARRAY_SIZE(heap_unknown); i++)
    {
        IUnknown *gotvalue = NULL;

        if (lpsa2)
        {
            indices[0] = i + sab[0].lLbound;
            hr = SafeArrayGetElement(lpsa2, indices, &gotvalue);
            ok(hr == S_OK, "Failed to get unk element at %d, hres 0x%x\n", i, hr);
            if (hr == S_OK)
            {
                ok(gotvalue == &heap_unknown[i]->IUnknown_iface, "Interface %d mismatch, expected %p, got %p\n",
                        i, &heap_unknown[i]->IUnknown_iface, gotvalue);
                IUnknown_Release(gotvalue);
            }
        }
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);

    /* Set one of the elements to NULL, see how this effects size. */
    indices[0] = 3 + sab[0].lLbound;
    hr = SafeArrayPutElement(lpsa, indices, NULL);
    ok(hr == S_OK, "Failed to put unknown element hr 0x%x\n", hr);

    expected = 60;
    for (i = 0; i < sab[0].cElements; i++)
    {
        VARIANT v;

        V_VT(&v) = VT_UNKNOWN;
        V_UNKNOWN(&v) = (i != 3) ? &heap_unknown[i]->IUnknown_iface : NULL;
        expected += VARIANT_UserSize(&umcb.Flags, 0, &v) - 20;
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = LPSAFEARRAY_UserSize(&umcb.Flags, 0, &lpsa);
    ok(size >= expected || size  >= (expected + 12 ),
        "size should be at least %u bytes, not %u\n", expected, size);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size2 = LPSAFEARRAY_UserSize(&umcb.Flags, 1, &lpsa);
    ok(size2 == (size + sizeof(DWORD)) || size2 == (size + sizeof(DWORD) + 12),
            "size should be %u bytes, not %u\n", size + (ULONG) sizeof(DWORD), size2);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserMarshal(&umcb.Flags, buffer, &lpsa);
    ok((next - buffer) <= expected, "Marshaled %u bytes, expected at most %u bytes\n", (ULONG) (next - buffer), expected);
    check_safearray(buffer, lpsa);

    lpsa2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = LPSAFEARRAY_UserUnmarshal(&umcb.Flags, buffer, &lpsa2);
    ok((next - buffer) <= expected, "Marshaled %u bytes, expected at most %u bytes\n", (ULONG) (next - buffer), expected);
    ok(lpsa2 != NULL, "LPSAFEARRAY didn't unmarshal, result %p\n", next);

    for (i = 0; i < ARRAY_SIZE(heap_unknown); i++)
    {
        IUnknown *gotvalue = NULL;

        if (lpsa2)
        {
            indices[0] = i + sab[0].lLbound;
            hr = SafeArrayGetElement(lpsa2, indices, &gotvalue);
            ok(hr == S_OK, "Failed to get unk element at %d, hres 0x%x\n", i, hr);
            if (hr == S_OK)
            {
                /* Our NULL interface. */
                if (i == 3)
                    ok(gotvalue == NULL, "Interface %d expected NULL, got %p\n", i, gotvalue);
                else
                {
                    ok(gotvalue == &heap_unknown[i]->IUnknown_iface, "Interface %d mismatch, expected %p, got %p\n",
                            i, &heap_unknown[i]->IUnknown_iface, gotvalue);
                    IUnknown_Release(gotvalue);
                }
            }
        }
        IUnknown_Release(&heap_unknown[i]->IUnknown_iface);
    }

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    LPSAFEARRAY_UserFree(&umcb.Flags, &lpsa2);

    ok(heap_unknown[0]->refs == 1, "Unexpected refcount %d\n", heap_unknown[0]->refs);

    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);
#endif
}

static void check_bstr(void *buffer, BSTR b)
{
    DWORD *wireb = buffer;
    DWORD len = SysStringByteLen(b);

    ok(*wireb == (len + 1) / 2, "wv[0] %08x\n", *wireb);
    wireb++;
    if(b)
        ok(*wireb == len, "wv[1] %08x\n", *wireb);
    else
        ok(*wireb == 0xffffffff, "wv[1] %08x\n", *wireb);
    wireb++;
    ok(*wireb == (len + 1) / 2, "wv[2] %08x\n", *wireb);
    if(len)
    {
        wireb++;
        ok(!memcmp(wireb, b, (len + 1) & ~1), "strings differ\n");
    }
    return;
}

static void test_marshal_BSTR(void)
{
    ULONG size;
    RPC_MESSAGE rpc_msg;
    MIDL_STUB_MESSAGE stub_msg;
    USER_MARSHAL_CB umcb;
    unsigned char *buffer, *next;
    BSTR b, b2;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t','1',0};
    DWORD len;

    b = SysAllocString(str);
    len = SysStringLen(b);
    ok(len == 13, "get %d\n", len);

    /* BSTRs are DWORD aligned */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 1, &b);
    ok(size == 42, "size %d\n", size);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 38, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    b2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    ok(b2 != NULL, "BSTR didn't unmarshal\n");
    ok(!memcmp(b, b2, (len + 1) * 2), "strings differ\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    BSTR_UserFree(&umcb.Flags, &b2);

    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);

    check_bstr(buffer, b);
    b2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    ok(b2 == NULL, "NULL BSTR didn't unmarshal\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    BSTR_UserFree(&umcb.Flags, &b2);
    HeapFree(GetProcessHeap(), 0, buffer);

    b = SysAllocStringByteLen("abc", 3);
    *(((char*)b) + 3) = 'd';
    len = SysStringLen(b);
    ok(len == 1, "get %d\n", len);
    len = SysStringByteLen(b);
    ok(len == 3, "get %d\n", len);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 16, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);
    ok(buffer[15] == 'd', "buffer[15] %02x\n", buffer[15]);

    b2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    ok(b2 != NULL, "BSTR didn't unmarshal\n");
    ok(!memcmp(b, b2, len), "strings differ\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    BSTR_UserFree(&umcb.Flags, &b2);
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);

    b = SysAllocStringByteLen("", 0);
    len = SysStringLen(b);
    ok(len == 0, "get %d\n", len);
    len = SysStringByteLen(b);
    ok(len == 0, "get %d\n", len);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = BSTR_UserSize(&umcb.Flags, 0, &b);
    ok(size == 12, "size %d\n", size);

    buffer = HeapAlloc(GetProcessHeap(), 0, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserMarshal(&umcb.Flags, buffer, &b);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    check_bstr(buffer, b);

    b2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    next = BSTR_UserUnmarshal(&umcb.Flags, buffer, &b2);
    ok(next == buffer + size, "got %p expect %p\n", next, buffer + size);
    ok(b2 != NULL, "NULL LPSAFEARRAY didn't unmarshal\n");
    len = SysStringByteLen(b2);
    ok(len == 0, "byte len %d\n", len);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    BSTR_UserFree(&umcb.Flags, &b2);
    HeapFree(GetProcessHeap(), 0, buffer);
    SysFreeString(b);
}

#ifndef __REACTOS__
typedef struct
{
    IUnknown IUnknown_iface;
    ULONG refs;
} HeapUnknown;
#endif

static inline HeapUnknown *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HeapUnknown, IUnknown_iface);
}

static HRESULT WINAPI HeapUnknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI HeapUnknown_AddRef(IUnknown *iface)
{
    HeapUnknown *This = impl_from_IUnknown(iface);
    return InterlockedIncrement((LONG*)&This->refs);
}

static ULONG WINAPI HeapUnknown_Release(IUnknown *iface)
{
    HeapUnknown *This = impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement((LONG*)&This->refs);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static const IUnknownVtbl HeapUnknown_Vtbl =
{
    HeapUnknown_QueryInterface,
    HeapUnknown_AddRef,
    HeapUnknown_Release
};

typedef struct
{
    DWORD clSize;
    DWORD rpcReserved;
    USHORT vt;
    USHORT wReserved1;
    USHORT wReserved2;
    USHORT wReserved3;
    DWORD switch_is;
} variant_wire_t;

static DWORD *check_variant_header(DWORD *wirev, VARIANT *v, ULONG size)
{
    const variant_wire_t *header = (const variant_wire_t*)wirev;
    DWORD switch_is;

    ok(header->clSize == (size + 7) >> 3, "wv[0] %08x, expected %08x\n", header->clSize, (size + 7) >> 3);
    ok(header->rpcReserved == 0, "wv[1] %08x\n", header->rpcReserved);
    ok(header->vt == V_VT(v), "vt %04x expected %04x\n", header->vt, V_VT(v));
    ok(header->wReserved1 == V_U2(v).wReserved1, "res1 %04x expected %04x\n", header->wReserved1, V_U2(v).wReserved1);
    ok(header->wReserved2 == V_U2(v).wReserved2, "res2 %04x expected %04x\n", header->wReserved2, V_U2(v).wReserved2);
    ok(header->wReserved3 == V_U2(v).wReserved3, "res3 %04x expected %04x\n", header->wReserved3, V_U2(v).wReserved3);

    switch_is = V_VT(v);
    if(switch_is & VT_ARRAY)
        switch_is &= ~VT_TYPEMASK;
    ok(header->switch_is == switch_is, "switch_is %08x expected %08x\n", header->switch_is, switch_is);

    return (DWORD*)((unsigned char*)wirev + sizeof(variant_wire_t));
}

/* Win9x and WinME don't always align as needed. Variants have
 * an alignment of 8.
 */
static void *alloc_aligned(SIZE_T size, void **buf)
{
    *buf = HeapAlloc(GetProcessHeap(), 0, size + 7);
    return (void *)(((UINT_PTR)*buf + 7) & ~7);
}

static void test_marshal_VARIANT(void)
{
    VARIANT v, v2, v3;
    MIDL_STUB_MESSAGE stubMsg = { 0 };
    RPC_MESSAGE rpcMsg = { 0 };
    USER_MARSHAL_CB umcb = { 0 };
    unsigned char *buffer, *next;
    void *oldbuffer;
    ULONG ul;
    short s;
    double d;
    void *mem;
    DWORD *wirev;
    BSTR b, b2;
    WCHAR str[] = {'m','a','r','s','h','a','l',' ','t','e','s','t',0};
    SAFEARRAYBOUND sab;
    LPSAFEARRAY lpsa, lpsa2, lpsa_copy;
    DECIMAL dec, dec2;
    HeapUnknown *heap_unknown;
    DWORD expected;
    HRESULT hr;
    LONG bound, bound2;
    VARTYPE vt, vt2;
    IUnknown *unk;

    stubMsg.RpcMsg = &rpcMsg;

    umcb.Flags = MAKELONG(MSHCTX_DIFFERENTMACHINE, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pStubMsg = &stubMsg;
    umcb.pReserve = NULL;
    umcb.Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb.CBType = USER_MARSHAL_CB_UNMARSHALL;

    /*** I1 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I1;
    V_I1(&v) = 0x12;

    /* check_variant_header tests wReserved[123], so initialize to unique values.
     * (Could probably also do this by setting the variant to a known DECIMAL.)
     */
    V_U2(&v).wReserved1 = 0x1234;
    V_U2(&v).wReserved2 = 0x5678;
    V_U2(&v).wReserved3 = 0x9abc;

    /* Variants have an alignment of 8 */
    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 1, &v);
    ok(stubMsg.BufferLength == 29, "size %d\n", stubMsg.BufferLength);

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 21, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*(char*)wirev == V_I1(&v), "wv[5] %08x\n", *wirev);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_I1(&v) == V_I1(&v2), "got i1 %x expect %x\n", V_I1(&v), V_I1(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I2 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2;
    V_I2(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 22, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*(short*)wirev == V_I2(&v), "wv[5] %08x\n", *wirev);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_I2(&v) == V_I2(&v2), "got i2 %x expect %x\n", V_I2(&v), V_I2(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I2 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_I2 | VT_BYREF;
    s = 0x1234;
    V_I2REF(&v) = &s;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 26, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*(short*)wirev == s, "wv[6] %08x\n", *wirev);
    VariantInit(&v2);
    V_VT(&v2) = VT_I2 | VT_BYREF;
    V_BYREF(&v2) = mem = CoTaskMemAlloc(sizeof(V_I2(&v2)));
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_BYREF(&v2) == mem, "didn't reuse existing memory\n");
    ok(*V_I2REF(&v) == *V_I2REF(&v2), "got i2 ref %x expect ui4 ref %x\n", *V_I2REF(&v), *V_I2REF(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == V_I4(&v), "wv[5] %08x\n", *wirev);

    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_I4(&v) == V_I4(&v2), "got i4 %x expect %x\n", V_I4(&v), V_I4(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UI4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4;
    V_UI4(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0x1234, "wv[5] %08x\n", *wirev);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_UI4(&v) == V_UI4(&v2), "got ui4 %x expect %x\n", V_UI4(&v), V_UI4(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UI4 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_BYREF;
    ul = 0x1234;
    V_UI4REF(&v) = &ul;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 28, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == ul, "wv[6] %08x\n", *wirev);

    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(*V_UI4REF(&v) == *V_UI4REF(&v2), "got ui4 ref %x expect ui4 ref %x\n", *V_UI4REF(&v), *V_UI4REF(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** I8 ***/
    VariantInit(&v);
    V_VT(&v) = VT_I8;
    V_I8(&v) = (LONGLONG)1000000 * 1000000;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 32, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0xcccccccc, "wv[5] %08x\n", *wirev); /* pad */
    wirev++;
    ok(*(LONGLONG *)wirev == V_I8(&v), "wv[6] %s\n", wine_dbgstr_longlong(*(LONGLONG *)wirev));
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_I8(&v) == V_I8(&v2), "got i8 %s expect %s\n",
            wine_dbgstr_longlong(V_I8(&v)), wine_dbgstr_longlong(V_I8(&v2)));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R4 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R4;
    V_R8(&v) = 3.1415;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 24, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
     
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*(float*)wirev == V_R4(&v), "wv[5] %08x\n", *wirev);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_R4(&v) == V_R4(&v2), "got r4 %f expect %f\n", V_R4(&v), V_R4(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R8 ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8;
    V_R8(&v) = 3.1415;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 32, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0xcccccccc, "wv[5] %08x\n", *wirev); /* pad */
    wirev++;
    ok(*(double*)wirev == V_R8(&v), "wv[6] %08x, wv[7] %08x\n", *wirev, *(wirev+1));
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_R8(&v) == V_R8(&v2), "got r8 %f expect %f\n", V_R8(&v), V_R8(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** R8 BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_R8 | VT_BYREF;
    d = 3.1415;
    V_R8REF(&v) = &d;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 32, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 8, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*(double*)wirev == d, "wv[6] %08x wv[7] %08x\n", *wirev, *(wirev+1));
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(*V_R8REF(&v) == *V_R8REF(&v2), "got r8 ref %f expect %f\n", *V_R8REF(&v), *V_R8REF(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** VARIANT_BOOL ***/
    VariantInit(&v);
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = 0x1234;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 22, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*(short*)wirev == V_BOOL(&v), "wv[5] %04x\n", *(WORD*)wirev);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_BOOL(&v) == V_BOOL(&v2), "got bool %x expect %x\n", V_BOOL(&v), V_BOOL(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** DECIMAL ***/
    VarDecFromI4(0x12345678, &dec);
    dec.wReserved = 0xfedc;          /* Also initialize reserved field, as we check it later */
    VariantInit(&v);
    V_DECIMAL(&v) = dec;
    V_VT(&v) = VT_DECIMAL;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 40, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0xcccccccc, "wirev[5] %08x\n", *wirev); /* pad */
    wirev++;
    dec2 = dec;
    dec2.wReserved = VT_DECIMAL;
    ok(!memcmp(wirev, &dec2, sizeof(dec2)), "wirev[6] %08x wirev[7] %08x wirev[8] %08x wirev[9] %08x\n",
       *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(!memcmp(&V_DECIMAL(&v), & V_DECIMAL(&v2), sizeof(DECIMAL)), "decimals differ\n");

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** DECIMAL BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_DECIMAL | VT_BYREF;
    V_DECIMALREF(&v) = &dec;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 40, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 16, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(!memcmp(wirev, &dec, sizeof(dec)), "wirev[6] %08x wirev[7] %08x wirev[8] %08x wirev[9] %08x\n", *wirev, *(wirev + 1), *(wirev + 2), *(wirev + 3));
    VariantInit(&v2);
    /* check_variant_header tests wReserved[123], so initialize to unique values.
     * (Could probably also do this by setting the variant to a known DECIMAL.)
     */
    V_U2(&v2).wReserved1 = 0x0123;
    V_U2(&v2).wReserved2 = 0x4567;
    V_U2(&v2).wReserved3 = 0x89ab;

    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(!memcmp(V_DECIMALREF(&v), V_DECIMALREF(&v2), sizeof(DECIMAL)), "decimals differ\n");

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** EMPTY ***/
    VariantInit(&v);
    V_VT(&v) = VT_EMPTY;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 20, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** NULL ***/
    VariantInit(&v);
    V_VT(&v) = VT_NULL;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 20, "size %d\n", stubMsg.BufferLength);

    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;

    check_variant_header(wirev, &v, stubMsg.BufferLength);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** BSTR ***/
    b = SysAllocString(str);
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = b;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 60, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev, "wv[5] %08x\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, V_BSTR(&v));
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(SysStringByteLen(V_BSTR(&v)) == SysStringByteLen(V_BSTR(&v2)), "bstr string lens differ\n");
    ok(!memcmp(V_BSTR(&v), V_BSTR(&v2), SysStringByteLen(V_BSTR(&v))), "bstrs differ\n");

    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** BSTR BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&v) = &b;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 64, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);
    ok(*wirev == 0x4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08x\n", *wirev); /* win2k: this is b. winxp: this is (char*)b + 1 */
    wirev++;
    check_bstr(wirev, b);
    b2 = SysAllocString(str);
    b2[0] = 0;
    V_VT(&v2) = VT_BSTR | VT_BYREF;
    V_BSTRREF(&v2) = &b2;
    mem = b2;
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(mem == b2, "BSTR should be reused\n");
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(SysStringByteLen(*V_BSTRREF(&v)) == SysStringByteLen(*V_BSTRREF(&v2)), "bstr string lens differ\n");
    ok(!memcmp(*V_BSTRREF(&v), *V_BSTRREF(&v2), SysStringByteLen(*V_BSTRREF(&v))), "bstrs differ\n");

    SysFreeString(b2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);
    SysFreeString(b);

    /*** ARRAY ***/
    sab.lLbound = 5;
    sab.cElements = 10;

    lpsa = SafeArrayCreate(VT_R8, 1, &sab);
    *(DWORD *)lpsa->pvData = 0xcafebabe;
    *((DWORD *)lpsa->pvData + 1) = 0xdeadbeef;

    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY;
    V_ARRAY(&v) = lpsa;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    expected = 152;
    ok(stubMsg.BufferLength == expected || stubMsg.BufferLength == expected + 8, /* win64 */
       "size %u instead of %u\n", stubMsg.BufferLength, expected);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    wirev = (DWORD*)buffer;
    
    wirev = check_variant_header(wirev, &v, expected);
    ok(*wirev, "wv[5] %08x\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(SafeArrayGetDim(V_ARRAY(&v)) == SafeArrayGetDim(V_ARRAY(&v2)), "array dims differ\n");
    SafeArrayGetLBound(V_ARRAY(&v), 1, &bound);
    SafeArrayGetLBound(V_ARRAY(&v2), 1, &bound2);
    ok(bound == bound2, "array lbounds differ\n");
    SafeArrayGetUBound(V_ARRAY(&v), 1, &bound);
    SafeArrayGetUBound(V_ARRAY(&v2), 1, &bound2);
    ok(bound == bound2, "array ubounds differ\n");
    if (pSafeArrayGetVartype)
    {
        pSafeArrayGetVartype(V_ARRAY(&v), &vt);
        pSafeArrayGetVartype(V_ARRAY(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
    }
    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** ARRAY BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY | VT_BYREF;
    V_ARRAYREF(&v) = &lpsa;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    expected = 152;
    ok(stubMsg.BufferLength == expected || stubMsg.BufferLength == expected + 16, /* win64 */
       "size %u instead of %u\n", stubMsg.BufferLength, expected);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, expected);
    ok(*wirev == 4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08x\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(SafeArrayGetDim(*V_ARRAYREF(&v)) == SafeArrayGetDim(*V_ARRAYREF(&v2)), "array dims differ\n");
    SafeArrayGetLBound(*V_ARRAYREF(&v), 1, &bound);
    SafeArrayGetLBound(*V_ARRAYREF(&v2), 1, &bound2);
    ok(bound == bound2, "array lbounds differ\n");
    SafeArrayGetUBound(*V_ARRAYREF(&v), 1, &bound);
    SafeArrayGetUBound(*V_ARRAYREF(&v2), 1, &bound2);
    ok(bound == bound2, "array ubounds differ\n");
    if (pSafeArrayGetVartype)
    {
        pSafeArrayGetVartype(*V_ARRAYREF(&v), &vt);
        pSafeArrayGetVartype(*V_ARRAYREF(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
    }
    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** ARRAY BYREF ***/
    VariantInit(&v);
    V_VT(&v) = VT_UI4 | VT_ARRAY | VT_BYREF;
    V_ARRAYREF(&v) = &lpsa;
    lpsa->fFeatures |= FADF_STATIC;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    expected = 152;
    ok(stubMsg.BufferLength == expected || stubMsg.BufferLength == expected + 16, /* win64 */
       "size %u instead of %u\n", stubMsg.BufferLength, expected);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    wirev = (DWORD*)buffer;

    wirev = check_variant_header(wirev, &v, expected);
    ok(*wirev == 4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev, "wv[6] %08x\n", *wirev); /* win2k: this is lpsa. winxp: this is (char*)lpsa + 1 */
    wirev++;
    check_safearray(wirev, lpsa);
    lpsa_copy = lpsa2 = SafeArrayCreate(VT_I8, 1, &sab);
    /* set FADF_STATIC feature to make sure lpsa2->pvData pointer changes if new data buffer is allocated */
    lpsa2->fFeatures |= FADF_STATIC;
    mem = lpsa2->pvData;
    V_VT(&v2) = VT_UI4 | VT_ARRAY | VT_BYREF;
    V_ARRAYREF(&v2) = &lpsa2;
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + expected, "got %p expect %p\n", next, buffer + expected);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(lpsa2 == lpsa_copy, "safearray should be reused\n");
    ok(mem == lpsa2->pvData, "safearray data should be reused\n");
    ok(SafeArrayGetDim(*V_ARRAYREF(&v)) == SafeArrayGetDim(*V_ARRAYREF(&v2)), "array dims differ\n");
    SafeArrayGetLBound(*V_ARRAYREF(&v), 1, &bound);
    SafeArrayGetLBound(*V_ARRAYREF(&v2), 1, &bound2);
    ok(bound == bound2, "array lbounds differ\n");
    SafeArrayGetUBound(*V_ARRAYREF(&v), 1, &bound);
    SafeArrayGetUBound(*V_ARRAYREF(&v2), 1, &bound2);
    ok(bound == bound2, "array ubounds differ\n");
    if (pSafeArrayGetVartype)
    {
        pSafeArrayGetVartype(*V_ARRAYREF(&v), &vt);
        pSafeArrayGetVartype(*V_ARRAYREF(&v2), &vt2);
        ok(vt == vt2, "array vts differ %x %x\n", vt, vt2);
    }
    lpsa2->fFeatures &= ~FADF_STATIC;
    hr = SafeArrayDestroy(*V_ARRAYREF(&v2));
    ok(hr == S_OK, "got 0x%08x\n", hr);
    HeapFree(GetProcessHeap(), 0, oldbuffer);
    lpsa->fFeatures &= ~FADF_STATIC;
    hr = SafeArrayDestroy(lpsa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /*** VARIANT BYREF ***/
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v2) = VT_R8;
    V_R8(&v2) = 3.1415;
    V_VT(&v) = VT_VARIANT | VT_BYREF;
    V_VARIANTREF(&v) = &v2;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength == 64, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);

    ok(*wirev == sizeof(VARIANT), "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == ('U' | 's' << 8 | 'e' << 16 | 'r' << 24), "wv[6] %08x\n", *wirev); /* 'User' */
    wirev++;
    ok(*wirev == 0xcccccccc, "wv[7] %08x\n", *wirev); /* pad */
    wirev++;
    wirev = check_variant_header(wirev, &v2, stubMsg.BufferLength - 32);
    ok(*wirev == 0xcccccccc, "wv[13] %08x\n", *wirev); /* pad for VT_R8 */
    wirev++;
    ok(*(double*)wirev == V_R8(&v2), "wv[6] %08x wv[7] %08x\n", *wirev, *(wirev+1));
    VariantInit(&v3);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
    ok(V_VT(V_VARIANTREF(&v)) == V_VT(V_VARIANTREF(&v3)), "vts differ %x %x\n",
       V_VT(V_VARIANTREF(&v)), V_VT(V_VARIANTREF(&v3))); 
    ok(V_R8(V_VARIANTREF(&v)) == V_R8(V_VARIANTREF(&v3)), "r8s differ\n"); 
    VARIANT_UserFree(&umcb.Flags, &v3);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UNKNOWN ***/
    heap_unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*heap_unknown));
    heap_unknown->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
    heap_unknown->refs = 1;
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = &heap_unknown->IUnknown_iface;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength > 40, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
todo_wine
    ok(heap_unknown->refs == 2, "got refcount %d\n", heap_unknown->refs);
    wirev = (DWORD*)buffer;
    wirev = check_variant_header(wirev, &v, next - buffer);

    ok(*wirev == (DWORD_PTR)V_UNKNOWN(&v) /* Win9x */ ||
       *wirev == (DWORD_PTR)V_UNKNOWN(&v) + 1 /* NT */, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == next - buffer - 0x20, "wv[6] %08x\n", *wirev);
    wirev++;
    ok(*wirev == next - buffer - 0x20, "wv[7] %08x\n", *wirev);
    wirev++;
    ok(*wirev == 0x574f454d, "wv[8] %08x\n", *wirev);
    VariantInit(&v3);
    V_VT(&v3) = VT_UNKNOWN;
    V_UNKNOWN(&v3) = &heap_unknown->IUnknown_iface;
    IUnknown_AddRef(V_UNKNOWN(&v3));
    stubMsg.Buffer = buffer;
todo_wine
    ok(heap_unknown->refs == 3, "got refcount %d\n", heap_unknown->refs);
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
    ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
    ok(V_UNKNOWN(&v) == V_UNKNOWN(&v3), "got %p expect %p\n", V_UNKNOWN(&v), V_UNKNOWN(&v3));
    VARIANT_UserFree(&umcb.Flags, &v3);
    ok(heap_unknown->refs == 1, "%d refcounts of IUnknown leaked\n", heap_unknown->refs - 1);
    IUnknown_Release(&heap_unknown->IUnknown_iface);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** NULL UNKNOWN ***/
    VariantInit(&v);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = NULL;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength >= 24, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    wirev = (DWORD*)buffer;
    wirev = check_variant_header(wirev, &v, next - buffer);
    ok(*wirev == 0, "wv[5] %08x\n", *wirev);

    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v), V_VT(&v2));
    ok(V_UNKNOWN(&v2) == NULL, "got %p expect NULL\n", V_UNKNOWN(&v2));
    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    /*** UNKNOWN BYREF ***/
    heap_unknown = HeapAlloc(GetProcessHeap(), 0, sizeof(*heap_unknown));
    heap_unknown->IUnknown_iface.lpVtbl = &HeapUnknown_Vtbl;
    heap_unknown->refs = 1;
    VariantInit(&v);
    VariantInit(&v2);
    V_VT(&v) = VT_UNKNOWN | VT_BYREF;
    V_UNKNOWNREF(&v) = (IUnknown **)&heap_unknown;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength >= 44, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    ok(heap_unknown->refs == 1, "got refcount %d\n", heap_unknown->refs);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
todo_wine
    ok(heap_unknown->refs == 2, "got refcount %d\n", heap_unknown->refs);
    wirev = (DWORD*)buffer;
    wirev = check_variant_header(wirev, &v, next - buffer);

    ok(*wirev == 4, "wv[5] %08x\n", *wirev);
    wirev++;
    ok(*wirev == (DWORD_PTR)heap_unknown /* Win9x, Win2000 */ ||
       *wirev == (DWORD_PTR)heap_unknown + 1 /* XP */, "wv[6] %08x\n", *wirev);
    wirev++;
    ok(*wirev == next - buffer - 0x24, "wv[7] %08x\n", *wirev);
    wirev++;
    ok(*wirev == next - buffer - 0x24, "wv[8] %08x\n", *wirev);
    wirev++;
    ok(*wirev == 0x574f454d, "wv[9] %08x\n", *wirev);

    VariantInit(&v3);
    V_VT(&v3) = VT_UNKNOWN;
    V_UNKNOWN(&v3) = &heap_unknown->IUnknown_iface;
    IUnknown_AddRef(V_UNKNOWN(&v3));
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v3);
    ok(heap_unknown->refs == 2, "got refcount %d\n", heap_unknown->refs);
    ok(V_VT(&v) == V_VT(&v3), "got vt %d expect %d\n", V_VT(&v), V_VT(&v3));
    ok(*V_UNKNOWNREF(&v) == *V_UNKNOWNREF(&v3), "got %p expect %p\n", *V_UNKNOWNREF(&v), *V_UNKNOWNREF(&v3));
    VARIANT_UserFree(&umcb.Flags, &v3);
    ok(heap_unknown->refs == 1, "%d refcounts of IUnknown leaked\n", heap_unknown->refs - 1);
    IUnknown_Release(&heap_unknown->IUnknown_iface);
    HeapFree(GetProcessHeap(), 0, oldbuffer);

    unk = NULL;
    VariantInit(&v);
    V_VT(&v) = VT_UNKNOWN | VT_BYREF;
    V_UNKNOWNREF(&v) = &unk;

    rpcMsg.BufferLength = stubMsg.BufferLength = VARIANT_UserSize(&umcb.Flags, 0, &v);
    ok(stubMsg.BufferLength >= 28, "size %d\n", stubMsg.BufferLength);
    buffer = rpcMsg.Buffer = stubMsg.Buffer = stubMsg.BufferStart = alloc_aligned(stubMsg.BufferLength, &oldbuffer);
    stubMsg.BufferEnd = stubMsg.Buffer + stubMsg.BufferLength;
    memset(buffer, 0xcc, stubMsg.BufferLength);
    next = VARIANT_UserMarshal(&umcb.Flags, buffer, &v);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    wirev = (DWORD*)buffer;
    wirev = check_variant_header(wirev, &v, stubMsg.BufferLength);

    ok(*wirev == 4, "wv[5] %08x\n", *wirev);

    VariantInit(&v2);
    stubMsg.Buffer = buffer;
    next = VARIANT_UserUnmarshal(&umcb.Flags, buffer, &v2);
    ok(next == buffer + stubMsg.BufferLength, "got %p expect %p\n", next, buffer + stubMsg.BufferLength);
    ok(V_VT(&v) == V_VT(&v2), "got vt %d expect %d\n", V_VT(&v2), V_VT(&v));
    ok(!*V_UNKNOWNREF(&v2), "got %p expect NULL\n", *V_UNKNOWNREF(&v2));
    VARIANT_UserFree(&umcb.Flags, &v2);
    HeapFree(GetProcessHeap(), 0, oldbuffer);
}


START_TEST(usrmarshal)
{
    HANDLE hOleaut32 = GetModuleHandleA("oleaut32.dll");
#define GETPTR(func) p##func = (void*)GetProcAddress(hOleaut32, #func)
    GETPTR(SafeArrayGetIID);
    GETPTR(SafeArrayGetVartype);
    GETPTR(VarBstrCmp);
#undef GETPTR

    if (!pSafeArrayGetIID || !pSafeArrayGetVartype)
        win_skip("SafeArrayGetIID and/or SafeArrayGetVartype is not available, some tests will be skipped\n");

    CoInitialize(NULL);

    test_marshal_LPSAFEARRAY();
    test_marshal_BSTR();
    test_marshal_VARIANT();

    CoUninitialize();
}
