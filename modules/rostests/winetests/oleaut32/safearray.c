/*
 * SafeArray test program
 *
 * Copyright 2002 Marcus Meissner
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#define COBJMACROS
#define CONST_VTABLE
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winsock2.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

#ifndef FADF_CREATEVECTOR
  const USHORT FADF_CREATEVECTOR = 0x2000;
#endif

static HMODULE hOleaut32;

static HRESULT (WINAPI *pSafeArrayAllocDescriptorEx)(VARTYPE,UINT,SAFEARRAY**);
static HRESULT (WINAPI *pSafeArrayGetVartype)(SAFEARRAY*,VARTYPE*);
static HRESULT (WINAPI *pSafeArrayGetRecordInfo)(SAFEARRAY*,IRecordInfo**);
static SAFEARRAY* (WINAPI *pSafeArrayCreateEx)(VARTYPE,UINT,SAFEARRAYBOUND*,LPVOID);

#define GETPTR(func) p##func = (void*)GetProcAddress(hOleaut32, #func)

/* Has I8/UI8 data type? */
static BOOL has_i8;
/* Has INT_PTR/UINT_PTR type? */
static BOOL has_int_ptr;

static const USHORT ignored_copy_features[] =
    {
        FADF_AUTO,
        FADF_STATIC,
        FADF_EMBEDDED,
        FADF_FIXEDSIZE
    };

#define START_REF_COUNT 1
#define RECORD_SIZE 64
#define RECORD_SIZE_FAIL 17
/************************************************************************
 * Dummy IRecordInfo Implementation
 */
typedef struct IRecordInfoImpl
{
  IRecordInfo IRecordInfo_iface;
  LONG ref;
  unsigned int sizeCalled;
  unsigned int clearCalled;
  unsigned int recordcopy;
} IRecordInfoImpl;

static inline IRecordInfoImpl *impl_from_IRecordInfo(IRecordInfo *iface)
{
  return CONTAINING_RECORD(iface, IRecordInfoImpl, IRecordInfo_iface);
}

static HRESULT WINAPI RecordInfo_QueryInterface(IRecordInfo *iface, REFIID riid, void **obj)
{
  *obj = NULL;

  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_IRecordInfo))
  {
      *obj = iface;
      IRecordInfo_AddRef(iface);
      return S_OK;
  }

  return E_NOINTERFACE;
}

static ULONG WINAPI RecordInfo_AddRef(IRecordInfo *iface)
{
  IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI RecordInfo_Release(IRecordInfo *iface)
{
  IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  if (!ref)
      HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

static HRESULT WINAPI RecordInfo_RecordInit(IRecordInfo *iface, PVOID pvNew)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static BOOL fail_GetSize; /* Whether to fail the GetSize call */

static HRESULT WINAPI RecordInfo_RecordClear(IRecordInfo *iface, PVOID pvExisting)
{
  IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
  This->clearCalled++;
  return S_OK;
}

static HRESULT WINAPI RecordInfo_RecordCopy(IRecordInfo *iface, PVOID pvExisting, PVOID pvNew)
{
  IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
  This->recordcopy++;
  return S_OK;
}

static HRESULT WINAPI RecordInfo_GetGuid(IRecordInfo *iface, GUID *pguid)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetName(IRecordInfo *iface, BSTR *pbstrName)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetSize(IRecordInfo *iface, ULONG* size)
{
  IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
  This->sizeCalled++;
  if (fail_GetSize)
  {
    *size = RECORD_SIZE_FAIL;
    return E_UNEXPECTED;
  }
  *size = RECORD_SIZE;
  return S_OK;
}

static HRESULT WINAPI RecordInfo_GetTypeInfo(IRecordInfo *iface, ITypeInfo **ppTypeInfo)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetField(IRecordInfo *iface, PVOID pvData,
                                                LPCOLESTR szFieldName, VARIANT *pvarField)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetFieldNoCopy(IRecordInfo *iface, PVOID pvData,
                            LPCOLESTR szFieldName, VARIANT *pvarField, PVOID *ppvDataCArray)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_PutField(IRecordInfo *iface, ULONG wFlags, PVOID pvData,
                                            LPCOLESTR szFieldName, VARIANT *pvarField)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_PutFieldNoCopy(IRecordInfo *iface, ULONG wFlags,
                PVOID pvData, LPCOLESTR szFieldName, VARIANT *pvarField)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetFieldNames(IRecordInfo *iface, ULONG *pcNames,
                                                BSTR *rgBstrNames)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static BOOL WINAPI RecordInfo_IsMatchingType(IRecordInfo *iface, IRecordInfo *info2)
{
  ok(0, "unexpected call\n");
  return FALSE;
}

static PVOID WINAPI RecordInfo_RecordCreate(IRecordInfo *iface)
{
  ok(0, "unexpected call\n");
  return NULL;
}

static HRESULT WINAPI RecordInfo_RecordCreateCopy(IRecordInfo *iface, PVOID pvSource,
                                                    PVOID *ppvDest)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_RecordDestroy(IRecordInfo *iface, PVOID pvRecord)
{
  ok(0, "unexpected call\n");
  return E_NOTIMPL;
}

static const IRecordInfoVtbl RecordInfoVtbl =
{
  RecordInfo_QueryInterface,
  RecordInfo_AddRef,
  RecordInfo_Release,
  RecordInfo_RecordInit,
  RecordInfo_RecordClear,
  RecordInfo_RecordCopy,
  RecordInfo_GetGuid,
  RecordInfo_GetName,
  RecordInfo_GetSize,
  RecordInfo_GetTypeInfo,
  RecordInfo_GetField,
  RecordInfo_GetFieldNoCopy,
  RecordInfo_PutField,
  RecordInfo_PutFieldNoCopy,
  RecordInfo_GetFieldNames,
  RecordInfo_IsMatchingType,
  RecordInfo_RecordCreate,
  RecordInfo_RecordCreateCopy,
  RecordInfo_RecordDestroy
};

static IRecordInfoImpl *IRecordInfoImpl_Construct(void)
{
  IRecordInfoImpl *rec;

  rec = HeapAlloc(GetProcessHeap(), 0, sizeof(IRecordInfoImpl));
  rec->IRecordInfo_iface.lpVtbl = &RecordInfoVtbl;
  rec->ref = START_REF_COUNT;
  rec->clearCalled = 0;
  rec->sizeCalled = 0;
  return rec;
}

static DWORD SAFEARRAY_GetVTSize(VARTYPE vt)
{
  switch (vt)
  {
    case VT_I1:
    case VT_UI1:      return sizeof(BYTE);
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return sizeof(SHORT);
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:    return sizeof(LONG);
    case VT_R8:       return sizeof(LONG64);
    case VT_I8:
    case VT_UI8:
      if (has_i8)
        return sizeof(LONG64);
      break;
    case VT_INT:
    case VT_UINT:     return sizeof(INT);
    case VT_INT_PTR:
    case VT_UINT_PTR: 
      if (has_int_ptr)
        return sizeof(UINT_PTR);
      break;
    case VT_CY:       return sizeof(CY);
    case VT_DATE:     return sizeof(DATE);
    case VT_BSTR:     return sizeof(BSTR);
    case VT_DISPATCH: return sizeof(LPDISPATCH);
    case VT_VARIANT:  return sizeof(VARIANT);
    case VT_UNKNOWN:  return sizeof(LPUNKNOWN);
    case VT_DECIMAL:  return sizeof(DECIMAL);
  }
  return 0;
}

static void check_for_VT_INT_PTR(void)
{
    /* Set a global flag if VT_INT_PTR is supported */

    SAFEARRAY* a;
    SAFEARRAYBOUND bound;
    bound.cElements	= 0;
    bound.lLbound	= 0;
    a = SafeArrayCreate(VT_INT_PTR, 1, &bound);
    if (a) {
        HRESULT hres;
        trace("VT_INT_PTR is supported\n");
        has_int_ptr = TRUE;
        hres = SafeArrayDestroy(a);
        ok(hres == S_OK, "got 0x%08lx\n", hres);
    }
    else {
        trace("VT_INT_PTR is not supported\n");
        has_int_ptr = FALSE;
    }        
}

#define VARTYPE_NOT_SUPPORTED 0
static struct {
	VARTYPE vt;    /* VT */
	UINT elemsize; /* elementsize by VT */
	UINT expflags; /* fFeatures from SafeArrayAllocDescriptorEx */
	UINT addflags; /* additional fFeatures from SafeArrayCreate */
} vttypes[] = {
{VT_EMPTY,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_NULL,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_I2,       2,                    FADF_HAVEVARTYPE,0},
{VT_I4,       4,                    FADF_HAVEVARTYPE,0},
{VT_R4,       4,                    FADF_HAVEVARTYPE,0},
{VT_R8,       8,                    FADF_HAVEVARTYPE,0},
{VT_CY,       8,                    FADF_HAVEVARTYPE,0},
{VT_DATE,     8,                    FADF_HAVEVARTYPE,0},
{VT_BSTR,     sizeof(BSTR),         FADF_HAVEVARTYPE,FADF_BSTR},
{VT_DISPATCH, sizeof(LPDISPATCH),   FADF_HAVEIID,    FADF_DISPATCH},
{VT_ERROR,    4,                    FADF_HAVEVARTYPE,0},
{VT_BOOL,     2,                    FADF_HAVEVARTYPE,0},
{VT_VARIANT,  sizeof(VARIANT),      FADF_HAVEVARTYPE,FADF_VARIANT},
{VT_UNKNOWN,  sizeof(LPUNKNOWN),    FADF_HAVEIID,    FADF_UNKNOWN},
{VT_DECIMAL,  sizeof(DECIMAL),      FADF_HAVEVARTYPE,0},
{15,          VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0}, /* no VT_xxx */
{VT_I1,       1,	            FADF_HAVEVARTYPE,0},
{VT_UI1,      1,		    FADF_HAVEVARTYPE,0},
{VT_UI2,      2,                    FADF_HAVEVARTYPE,0},
{VT_UI4,      4,                    FADF_HAVEVARTYPE,0},
{VT_I8,       VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_UI8,      VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_INT,      sizeof(INT),          FADF_HAVEVARTYPE,0},
{VT_UINT,     sizeof(UINT),         FADF_HAVEVARTYPE,0},
{VT_VOID,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_HRESULT,  VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_PTR,      VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_SAFEARRAY,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CARRAY,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_USERDEFINED,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_LPSTR,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_LPWSTR,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_FILETIME, VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_RECORD,   VARTYPE_NOT_SUPPORTED,FADF_RECORD,0},
{VT_BLOB,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STREAM,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STORAGE,  VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STREAMED_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STORED_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_BLOB_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CF,       VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CLSID,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
};

static void test_safearray(void)
{
	SAFEARRAY 	*a, b, *c;
	unsigned int 	i, diff;
	LONG		indices[2];
	HRESULT 	hres;
	SAFEARRAYBOUND	bound, bounds[2];
	VARIANT		v,d;
	LPVOID		data;
	IID		iid;
	VARTYPE		vt;
	LONG		l;
	unsigned char	*ptr1, *ptr2;

	hres = SafeArrayDestroy( NULL);
	ok( hres == S_OK, "SafeArrayDestroy( NULL) returned 0x%lx\n", hres);

	bound.cElements	= 1;
	bound.lLbound	= 0;
	a = SafeArrayCreate(-1, 1, &bound);
	ok(NULL == a,"SAC(-1,1,[1,0]) not failed?\n");

	bound.cElements	= 0;
	bound.lLbound	= 42;
	a = SafeArrayCreate(VT_I4, 1, &bound);
	ok(NULL != a,"SAC(VT_I4,1,[0,0]) failed.\n");

        hres = SafeArrayGetLBound(a, 1, &l);
	ok(hres == S_OK, "SAGLB of 0 size dimensioned array failed with %lx\n",hres);
	ok(l == 42, "SAGLB of 0 size dimensioned array failed to return 42, but returned %ld\n",l);
        hres = SafeArrayGetUBound(a, 1, &l);
	ok(hres == S_OK, "SAGUB of 0 size dimensioned array failed with %lx\n",hres);
	ok(l == 41, "SAGUB of 0 size dimensioned array failed to return 41, but returned %ld\n",l);
        
        hres = SafeArrayAccessData(a, &data);
        ok(hres == S_OK, "SafeArrayAccessData of 0 size dimensioned array failed with %lx\n", hres);
        SafeArrayUnaccessData(a);

	bound.cElements = 2;
        hres = SafeArrayRedim(a, &bound);
	ok(hres == S_OK,"SAR of a 0 elements dimension failed with hres %lx\n", hres);
	bound.cElements = 0;
        hres = SafeArrayRedim(a, &bound);
	ok(hres == S_OK || hres == E_OUTOFMEMORY,
          "SAR to a 0 elements dimension failed with hres %lx\n", hres);
	hres = SafeArrayDestroy(a);
	ok(hres == S_OK,"SAD of 0 dim array failed with hres %lx\n", hres);

        SafeArrayAllocDescriptor(2, &a);
        a->rgsabound[0].cElements = 2;
        a->rgsabound[0].lLbound = 1;
        a->rgsabound[1].cElements = 4;
        a->rgsabound[1].lLbound = 1;
        a->cbElements = 2;
        hres = SafeArrayAllocData(a);
        ok(hres == S_OK, "SafeArrayAllocData failed with hres %lx\n", hres);

        indices[0] = 4;
        indices[1] = 2;
        hres = SafeArrayPtrOfIndex(a, indices, (void **)&ptr1);
        ok(hres == S_OK, "SAPOI failed with hres %lx\n", hres);
        SafeArrayAccessData(a, (void **)&ptr2);
        ok(ptr1 - ptr2 == 14, "SAPOI got wrong ptr\n");
        *(WORD *)ptr1 = 0x55aa;
        SafeArrayUnaccessData(a);

        bound.cElements = 10;
        bound.lLbound = 1;
        SafeArrayRedim(a, &bound);
        ptr1 = NULL;
        SafeArrayPtrOfIndex(a, indices, (void **)&ptr1);
        ok(*(WORD *)ptr1 == 0x55aa, "Data not preserved when resizing array\n");

        bound.cElements = 10;
        bound.lLbound = 0;
        SafeArrayRedim(a, &bound);
        SafeArrayPtrOfIndex(a, indices, (void **)&ptr1);
        ok(*(WORD *)ptr1 == 0 ||
           broken(*(WORD *)ptr1 != 0), /* Win 2003 */
           "Expanded area not zero-initialized\n");

        indices[1] = 1;
        SafeArrayPtrOfIndex(a, indices, (void **)&ptr1);
        ok(*(WORD *)ptr1 == 0x55aa ||
           broken(*(WORD *)ptr1 != 0x55aa), /* Win 2003 */
           "Data not preserved when resizing array\n");

        hres = SafeArrayDestroy(a);
        ok(hres == S_OK,"SAD failed with hres %lx\n", hres);

	bounds[0].cElements = 0;	bounds[0].lLbound =  1;
	bounds[1].cElements =  2;	bounds[1].lLbound = 23;
    	a = SafeArrayCreate(VT_I4,2,bounds);
    	ok(a != NULL,"SAC(VT_INT32,2,...) with 0 element dim failed.\n");

        hres = SafeArrayDestroy(a);
        ok(hres == S_OK,"SAD failed with hres %lx\n", hres);
	bounds[0].cElements = 1;	bounds[0].lLbound =  1;
	bounds[1].cElements = 0;	bounds[1].lLbound = 23;
    	a = SafeArrayCreate(VT_I4,2,bounds);
    	ok(a != NULL,"SAC(VT_INT32,2,...) with 0 element dim failed.\n");

        hres = SafeArrayDestroy(a);
        ok(hres == S_OK,"SAD failed with hres %lx\n", hres);

	bounds[0].cElements = 42;	bounds[0].lLbound =  1;
	bounds[1].cElements =  2;	bounds[1].lLbound = 23;
    a = SafeArrayCreate(VT_I4,2,bounds);
    ok(a != NULL,"SAC(VT_INT32,2,...) failed.\n");

	hres = SafeArrayGetLBound (a, 0, &l);
	ok (hres == DISP_E_BADINDEX, "SAGLB 0 failed with %lx\n", hres);
	hres = SafeArrayGetLBound (a, 1, &l);
	ok (hres == S_OK, "SAGLB 1 failed with %lx\n", hres);
	ok (l == 1, "SAGLB 1 returned %ld instead of 1\n", l);
	hres = SafeArrayGetLBound (a, 2, &l);
	ok (hres == S_OK, "SAGLB 2 failed with %lx\n", hres);
	ok (l == 23, "SAGLB 2 returned %ld instead of 23\n", l);
	hres = SafeArrayGetLBound (a, 3, &l);
	ok (hres == DISP_E_BADINDEX, "SAGLB 3 failed with %lx\n", hres);

	hres = SafeArrayGetUBound (a, 0, &l);
	ok (hres == DISP_E_BADINDEX, "SAGUB 0 failed with %lx\n", hres);
	hres = SafeArrayGetUBound (a, 1, &l);
	ok (hres == S_OK, "SAGUB 1 failed with %lx\n", hres);
	ok (l == 42, "SAGUB 1 returned %ld instead of 42\n", l);
	hres = SafeArrayGetUBound (a, 2, &l);
	ok (hres == S_OK, "SAGUB 2 failed with %lx\n", hres);
	ok (l == 24, "SAGUB 2 returned %ld instead of 24\n", l);
	hres = SafeArrayGetUBound (a, 3, &l);
	ok (hres == DISP_E_BADINDEX, "SAGUB 3 failed with %lx\n", hres);

	i = SafeArrayGetDim(a);
	ok(i == 2, "getdims of 2 din array returned %d\n",i);

	indices[0] = 0;
	indices[1] = 23;
	hres = SafeArrayGetElement(a, indices, &i);
	ok(DISP_E_BADINDEX == hres,"SAGE failed [0,23], hres 0x%lx\n",hres);

	indices[0] = 1;
	indices[1] = 22;
	hres = SafeArrayGetElement(a, indices, &i);
	ok(DISP_E_BADINDEX == hres,"SAGE failed [1,22], hres 0x%lx\n",hres);

	indices[0] = 1;
	indices[1] = 23;
	hres = SafeArrayGetElement(a, indices, &i);
	ok(S_OK == hres,"SAGE failed [1,23], hres 0x%lx\n",hres);

	indices[0] = 1;
	indices[1] = 25;
	hres = SafeArrayGetElement(a, indices, &i);
	ok(DISP_E_BADINDEX == hres,"SAGE failed [1,24], hres 0x%lx\n",hres);

	indices[0] = 3;
	indices[1] = 23;
	hres = SafeArrayGetElement(a, indices, &i);
	ok(S_OK == hres,"SAGE failed [42,23], hres 0x%lx\n",hres);

	hres = SafeArrayAccessData(a, (void**)&ptr1);
	ok(S_OK == hres, "SAAD failed with 0x%lx\n", hres);

	indices[0] = 3;
	indices[1] = 23;
	hres = SafeArrayPtrOfIndex(a, indices, (void**)&ptr2);
	ok(S_OK == hres,"SAPOI failed [1,23], hres 0x%lx\n",hres);
        diff = ptr2 - ptr1;
	ok(diff == 8,"ptr difference is not 8, but %d (%p vs %p)\n", diff, ptr2, ptr1);

	indices[0] = 3;
	indices[1] = 24;
	hres = SafeArrayPtrOfIndex(a, indices, (void**)&ptr2);
	ok(S_OK == hres,"SAPOI failed [5,24], hres 0x%lx\n",hres);
        diff = ptr2 - ptr1;
	ok(diff == 176,"ptr difference is not 176, but %d (%p vs %p)\n", diff, ptr2, ptr1);

	indices[0] = 20;
	indices[1] = 23;
	hres = SafeArrayPtrOfIndex(a, indices, (void**)&ptr2);
	ok(S_OK == hres,"SAPOI failed [20,23], hres 0x%lx\n",hres);
        diff = ptr2 - ptr1;
	ok(diff == 76,"ptr difference is not 76, but %d (%p vs %p)\n", diff, ptr2, ptr1);

	hres = SafeArrayUnaccessData(a);
	ok(S_OK == hres, "SAUAD failed with 0x%lx\n", hres);

	hres = SafeArrayDestroy(a);
	ok(hres == S_OK,"SAD failed with hres %lx\n", hres);

        for (i = 0; i < ARRAY_SIZE(vttypes); i++) {
	if ((i == VT_I8 || i == VT_UI8) && has_i8)
	{
	  vttypes[i].elemsize = sizeof(LONG64);
	}

	a = SafeArrayCreate(vttypes[i].vt, 1, &bound);

	ok((!a && !vttypes[i].elemsize) ||
	   (a && vttypes[i].elemsize == a->cbElements),
	   "SAC(%d,1,[1,0]), %p result %ld, expected %d\n",
	   vttypes[i].vt,a,(a?a->cbElements:0),vttypes[i].elemsize);

	if (a)
	{
	  ok(a->fFeatures == (vttypes[i].expflags | vttypes[i].addflags),
	     "SAC of %d returned feature flags %x, expected %x\n",
	  vttypes[i].vt, a->fFeatures,
	  vttypes[i].expflags|vttypes[i].addflags);
	  ok(SafeArrayGetElemsize(a) == vttypes[i].elemsize,
	     "SAGE for vt %d returned elemsize %d instead of expected %d\n",
	     vttypes[i].vt, SafeArrayGetElemsize(a),vttypes[i].elemsize);
	}

		if (!a) continue;

        if (pSafeArrayGetVartype)
        {
            hres = pSafeArrayGetVartype(a, &vt);
            ok(hres == S_OK, "SAGVT of arra y with vt %d failed with %lx\n", vttypes[i].vt, hres);
            /* Windows prior to Vista returns VT_UNKNOWN instead of VT_DISPATCH */
            ok(broken(vt == VT_UNKNOWN) || vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d\n", vttypes[i].vt, vt);
        }

		hres = SafeArrayCopy(a, &c);
		ok(hres == S_OK, "failed to copy safearray of vt %d with hres %lx\n", vttypes[i].vt, hres);

		ok(vttypes[i].elemsize == c->cbElements,"copy of SAC(%d,1,[1,0]), result %ld, expected %d\n",vttypes[i].vt,(c?c->cbElements:0),vttypes[i].elemsize
		);
		ok(c->fFeatures == (vttypes[i].expflags | vttypes[i].addflags),"SAC of %d returned feature flags %x, expected %x\n", vttypes[i].vt, c->fFeatures, vttypes[i].expflags|vttypes[i].addflags);
		ok(SafeArrayGetElemsize(c) == vttypes[i].elemsize,"SAGE for vt %d returned elemsize %d instead of expected %d\n",vttypes[i].vt, SafeArrayGetElemsize(c),vttypes[i].elemsize);

        if (pSafeArrayGetVartype) {
            hres = pSafeArrayGetVartype(c, &vt);
            ok(hres == S_OK, "SAGVT of array with vt %d failed with %lx\n", vttypes[i].vt, hres);
            /* Windows prior to Vista returns VT_UNKNOWN instead of VT_DISPATCH */
            ok(broken(vt == VT_UNKNOWN) || vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d\n", vttypes[i].vt, vt);
        }

        hres = SafeArrayCopyData(a, c);
        ok(hres == S_OK, "failed to copy safearray data of vt %d with hres %lx\n", vttypes[i].vt, hres);

        hres = SafeArrayDestroyData(c);
        ok(hres == S_OK,"SADD of copy of array with vt %d failed with hres %lx\n", vttypes[i].vt, hres);

		hres = SafeArrayDestroy(c);
		ok(hres == S_OK,"SAD failed with hres %lx\n", hres);

		hres = SafeArrayDestroy(a);
		ok(hres == S_OK,"SAD of array with vt %d failed with hres %lx\n", vttypes[i].vt, hres);
	}

	/* Test conversion of type|VT_ARRAY <-> VT_BSTR */
	bound.lLbound = 0;
	bound.cElements = 10;
	a = SafeArrayCreate(VT_UI1, 1, &bound);
	ok(a != NULL, "SAC failed.\n");
	ok(S_OK == SafeArrayAccessData(a, &data),"SACD failed\n");
	memcpy(data,"Hello World\n",10);
	ok(S_OK == SafeArrayUnaccessData(a),"SAUD failed\n");
	V_VT(&v) = VT_ARRAY|VT_UI1;
	V_ARRAY(&v) = a;
	hres = VariantChangeTypeEx(&v, &v, 0, 0, VT_BSTR);
	ok(hres==S_OK, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR failed with %lx\n",hres);
	ok(V_VT(&v) == VT_BSTR,"CTE VT_ARRAY|VT_UI1 -> VT_BSTR did not return VT_BSTR, but %d.v\n",V_VT(&v));
	ok(V_BSTR(&v)[0] == 0x6548,"First letter are not 'He', but %x\n", V_BSTR(&v)[0]);
	VariantClear(&v);

	VariantInit(&d);
	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocStringLen(NULL, 0);
	hres = VariantChangeTypeEx(&d, &v, 0, 0, VT_UI1|VT_ARRAY);
	ok(hres==S_OK, "CTE VT_BSTR -> VT_UI1|VT_ARRAY failed with %lx\n",hres);
	ok(V_VT(&d) == (VT_UI1|VT_ARRAY),"CTE BSTR -> VT_UI1|VT_ARRAY did not return VT_UI1|VT_ARRAY, but %d.v\n",V_VT(&v));
	VariantClear(&v);
	VariantClear(&d);

	/* check locking functions */
	a = SafeArrayCreate(VT_I4, 1, &bound);
	ok(a!=NULL,"SAC should not fail\n");

	hres = SafeArrayAccessData(a, &data);
	ok(hres == S_OK,"SAAD failed with hres %lx\n",hres);

	hres = SafeArrayDestroy(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy not failed with DISP_E_ARRAYISLOCKED, but with hres %lx\n", hres);

	hres = SafeArrayDestroyData(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy data not failed with DISP_E_ARRAYISLOCKED, but with hres %lx\n", hres);

	hres = SafeArrayDestroyDescriptor(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy descriptor not failed with DISP_E_ARRAYISLOCKED, but with hres %lx\n", hres);

	hres = SafeArrayUnaccessData(a);
	ok(hres == S_OK,"SAUD failed after lock/destroy test\n");

	hres = SafeArrayDestroy(a);
	ok(hres == S_OK,"SAD failed after lock/destroy test\n");

	/* Test if we need to destroy data before descriptor */
	a = SafeArrayCreate(VT_I4, 1, &bound);
	ok(a!=NULL,"SAC should not fail\n");
	hres = SafeArrayDestroyDescriptor(a);
	ok(hres == S_OK,"SADD with data in array failed with hres %lx\n",hres);

    /* IID functions */
    /* init a small stack safearray */
    memset(&b, 0, sizeof(b));
    b.cDims = 1;
    memset(&iid, 0x42, sizeof(IID));
    hres = SafeArraySetIID(&b, &iid);
    ok(hres == E_INVALIDARG, "Unexpected ret value %#lx.\n", hres);

    hres = SafeArrayAllocDescriptor(1, &a);
    ok(hres == S_OK, "Failed to allocate array descriptor, hr %#lx.\n", hres);
    ok((a->fFeatures & FADF_HAVEIID) == 0, "Unexpected features mask %#x.\n", a->fFeatures);
    hres = SafeArraySetIID(a, &iid);
    ok(hres == E_INVALIDARG, "Unexpected ret value %#lx.\n", hres);

    hres = SafeArrayDestroyDescriptor(a);
    ok(hres == S_OK,"SADD failed with hres %lx\n",hres);

    if (!pSafeArrayAllocDescriptorEx)
        return;

    for (i = 0; i < ARRAY_SIZE(vttypes); i++) {
		a = NULL;
		hres = pSafeArrayAllocDescriptorEx(vttypes[i].vt,1,&a);
		ok(hres == S_OK, "SafeArrayAllocDescriptorEx gave hres 0x%lx\n", hres);
		ok(a->fFeatures == vttypes[i].expflags,"SAADE(%d) resulted with flags %x, expected %x\n", vttypes[i].vt, a->fFeatures, vttypes[i].expflags);
		if (a->fFeatures & FADF_HAVEIID) {
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == S_OK,"SAGIID failed for vt %d with hres %lx\n", vttypes[i].vt,hres);
			switch (vttypes[i].vt) {
			case VT_UNKNOWN:
				ok(IsEqualGUID(((GUID*)a)-1,&IID_IUnknown),"guid for VT_UNKNOWN is not IID_IUnknown\n");
				ok(IsEqualGUID(&iid, &IID_IUnknown),"SAGIID returned wrong GUID for IUnknown\n");
				break;
			case VT_DISPATCH:
				ok(IsEqualGUID(((GUID*)a)-1,&IID_IDispatch),"guid for VT_UNKNOWN is not IID_IDispatch\n");
				ok(IsEqualGUID(&iid, &IID_IDispatch),"SAGIID returned wrong GUID for IDispatch\n");
				break;
			default:
				ok(FALSE,"unknown vt %d with FADF_HAVEIID\n",vttypes[i].vt);
				break;
			}
		} else {
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == E_INVALIDARG,"SAGIID did not fail for vt %d with hres %lx\n", vttypes[i].vt,hres);
		}
		if (a->fFeatures & FADF_RECORD) {
			ok(vttypes[i].vt == VT_RECORD,"FADF_RECORD for non record %d\n",vttypes[i].vt);
		}
		if (a->fFeatures & FADF_HAVEVARTYPE) {
			ok(vttypes[i].vt == ((DWORD*)a)[-1], "FADF_HAVEVARTYPE set, but vt %d mismatch stored %ld\n",vttypes[i].vt,((DWORD*)a)[-1]);
		}

		hres = pSafeArrayGetVartype(a, &vt);
		ok(hres == S_OK, "SAGVT of array with vt %d failed with %lx\n", vttypes[i].vt, hres);

		if (vttypes[i].vt == VT_DISPATCH) {
			/* Special case. Checked against Windows. */
			ok(vt == VT_UNKNOWN, "SAGVT of array with VT_DISPATCH returned not VT_UNKNOWN, but %d\n", vt);
		} else {
			ok(vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d\n", vttypes[i].vt, vt);
		}

		if (a->fFeatures & FADF_HAVEIID) {
			hres = SafeArraySetIID(a, &IID_IStorage); /* random IID */
			ok(hres == S_OK,"SASIID failed with FADF_HAVEIID set for vt %d with %lx\n", vttypes[i].vt, hres);
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == S_OK,"SAGIID failed with FADF_HAVEIID set for vt %d with %lx\n", vttypes[i].vt, hres);
			ok(IsEqualGUID(&iid, &IID_IStorage),"returned iid is not IID_IStorage\n");
		} else {
			hres = SafeArraySetIID(a, &IID_IStorage); /* random IID */
			ok(hres == E_INVALIDARG,"SASIID did not failed with !FADF_HAVEIID set for vt %d with %lx\n", vttypes[i].vt, hres);
		}
		hres = SafeArrayDestroyDescriptor(a);
		ok(hres == S_OK,"SADD failed with hres %lx\n",hres);
    }
}

static void test_SafeArrayAllocDestroyDescriptor(void)
{
  SAFEARRAY *sa;
  HRESULT hres;
  UINT i;

  /* Failure cases */
  hres = SafeArrayAllocDescriptor(0, &sa);
  ok(hres == E_INVALIDARG, "0 dimensions gave hres 0x%lx\n", hres);

  hres = SafeArrayAllocDescriptor(65536, &sa);
  ok(hres == E_INVALIDARG, "65536 dimensions gave hres 0x%lx\n", hres);

  if (0)
  {
  /* Crashes on 95: XP & Wine return E_POINTER */
  hres=SafeArrayAllocDescriptor(1, NULL);
  ok(hres == E_POINTER,"NULL parm gave hres 0x%lx\n", hres);
  }

  /* Test up to the dimension boundary case */
  for (i = 5; i <= 65535; i += 30)
  {
    hres = SafeArrayAllocDescriptor(i, &sa);
    ok(hres == S_OK, "%d dimensions failed; hres 0x%lx\n", i, hres);

    if (hres == S_OK)
    {
      ok(SafeArrayGetDim(sa) == i, "Dimension is %d; should be %d\n",
         SafeArrayGetDim(sa), i);

      hres = SafeArrayDestroyDescriptor(sa);
      ok(hres == S_OK, "destroy failed; hres 0x%lx\n", hres);
    }
  }

  if (!pSafeArrayAllocDescriptorEx)
    return;

  hres = pSafeArrayAllocDescriptorEx(VT_UI1, 0, &sa);
  ok(hres == E_INVALIDARG, "0 dimensions gave hres 0x%lx\n", hres);

  hres = pSafeArrayAllocDescriptorEx(VT_UI1, 65536, &sa);
  ok(hres == E_INVALIDARG, "65536 dimensions gave hres 0x%lx\n", hres);

  hres = pSafeArrayAllocDescriptorEx(VT_UI1, 1, NULL);
  ok(hres == E_POINTER,"NULL parm gave hres 0x%lx\n", hres);

  hres = pSafeArrayAllocDescriptorEx(-1, 1, &sa);
  ok(hres == S_OK, "VT = -1 gave hres 0x%lx\n", hres);

  sa->rgsabound[0].cElements = 0;
  sa->rgsabound[0].lLbound = 1;

  hres = SafeArrayAllocData(sa);
  ok(hres == S_OK, "SafeArrayAllocData gave hres 0x%lx\n", hres);

  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK,"SafeArrayDestroy failed with hres %lx\n",hres);
}

static void test_SafeArrayCreateLockDestroy(void)
{
  SAFEARRAYBOUND sab[4];
  SAFEARRAY *sa;
  HRESULT hres;
  VARTYPE vt;
  UINT dimension;

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
  {
    sab[dimension].lLbound = 0;
    sab[dimension].cElements = 8;
  }

  /* Failure cases */
/* This test crashes very early versions with no error checking...
  sa = SafeArrayCreate(VT_UI1, 1, NULL);
  ok(sa == NULL, "NULL bounds didn't fail\n");
*/
  sa = SafeArrayCreate(VT_UI1, 65536, sab);
  ok(!sa, "Max bounds didn't fail\n");

  memset(sab, 0, sizeof(sab));

  /* Don't test 0 sized dimensions, as Windows has a bug which allows this */

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
    sab[dimension].cElements = 8;

  /* Test all VARTYPES in 1-4 dimensions */
  for (dimension = 1; dimension < 4; dimension++)
  {
    for (vt = VT_EMPTY; vt < VT_CLSID; vt++)
    {
      DWORD dwLen = SAFEARRAY_GetVTSize(vt);

      sa = SafeArrayCreate(vt, dimension, sab);

      if (dwLen)
        ok(sa != NULL, "VARTYPE %d (@%d dimensions) failed\n", vt, dimension);
      else
        ok(sa == NULL || vt == VT_R8,
           "VARTYPE %d (@%d dimensions) succeeded!\n", vt, dimension);

      if (sa)
      {
        ok(SafeArrayGetDim(sa) == dimension,
           "VARTYPE %d (@%d dimensions) cDims is %d, expected %d\n",
           vt, dimension, SafeArrayGetDim(sa), dimension);
        ok(SafeArrayGetElemsize(sa) == dwLen || vt == VT_R8,
           "VARTYPE %d (@%d dimensions) cbElements is %d, expected %ld\n",
           vt, dimension, SafeArrayGetElemsize(sa), dwLen);

        if (vt != VT_UNKNOWN && vt != VT_DISPATCH)
        {
          ok((sa->fFeatures & FADF_HAVEIID) == 0,
             "Non interface type should not have FADF_HAVEIID\n");
          hres = SafeArraySetIID(sa, &IID_IUnknown);
          ok(hres == E_INVALIDARG, "Unexpected ret value %#lx.\n", hres);
          if (vt != VT_RECORD)
          {
            VARTYPE aVt;

            ok(sa->fFeatures & FADF_HAVEVARTYPE,
               "Non interface type should have FADF_HAVEVARTYPE\n");
            if (pSafeArrayGetVartype)
            {
              hres = pSafeArrayGetVartype(sa, &aVt);
              ok(hres == S_OK && aVt == vt,
                 "Non interface type %d: bad type %d, hres %lx\n", vt, aVt, hres);
            }
          }
        }
        else
        {
          ok(sa->fFeatures & FADF_HAVEIID, "Interface type should have FADF_HAVEIID\n");
          hres = SafeArraySetIID(sa, &IID_IUnknown);
          ok(hres == S_OK, "Failed to set array IID, hres %#lx.\n", hres);
          ok((sa->fFeatures & FADF_HAVEVARTYPE) == 0,
             "Interface type %d should not have FADF_HAVEVARTYPE\n", vt);
        }

        hres = SafeArrayLock(sa);
        ok(hres == S_OK, "Lock VARTYPE %d (@%d dimensions) failed; hres 0x%lx\n",
           vt, dimension, hres);

        if (hres == S_OK)
        {
          hres = SafeArrayDestroy(sa);
          ok(hres == DISP_E_ARRAYISLOCKED,"Destroy() got hres %lx\n", hres);

          hres = SafeArrayDestroyData(sa);
          ok(hres == DISP_E_ARRAYISLOCKED,"DestroyData() got hres %lx\n", hres);

          hres = SafeArrayDestroyDescriptor(sa);
          ok(hres == DISP_E_ARRAYISLOCKED,"DestroyDescriptor() got hres %lx\n", hres);

          hres = SafeArrayUnlock(sa);
          ok(hres == S_OK, "Unlock VARTYPE %d (@%d dims) hres 0x%lx\n",
             vt, dimension, hres);

          hres = SafeArrayDestroy(sa);
          ok(hres == S_OK, "destroy VARTYPE %d (@%d dims) hres 0x%lx\n",
             vt, dimension, hres);
        }
      }
    }
  }
}

static void test_VectorCreateLockDestroy(void)
{
  SAFEARRAY *sa;
  HRESULT hres;
  VARTYPE vt;
  int element;

  sa = SafeArrayCreateVector(VT_UI1, 0, 0);
  ok(sa != NULL, "SACV with 0 elements failed.\n");

  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "SafeArrayDestroy failed with hres %lx\n",hres);

  /* Test all VARTYPES in different lengths */
  for (element = 1; element <= 101; element += 10)
  {
    for (vt = VT_EMPTY; vt < VT_CLSID; vt++)
    {
      DWORD dwLen = SAFEARRAY_GetVTSize(vt);

      sa = SafeArrayCreateVector(vt, 0, element);

      if (dwLen)
        ok(sa != NULL, "VARTYPE %d (@%d elements) failed\n", vt, element);
      else
        ok(sa == NULL, "VARTYPE %d (@%d elements) succeeded!\n", vt, element);

      if (sa)
      {
        ok(SafeArrayGetDim(sa) == 1, "VARTYPE %d (@%d elements) cDims %d, not 1\n",
           vt, element, SafeArrayGetDim(sa));
        ok(SafeArrayGetElemsize(sa) == dwLen,
           "VARTYPE %d (@%d elements) cbElements is %d, expected %ld\n",
           vt, element, SafeArrayGetElemsize(sa), dwLen);

        hres = SafeArrayLock(sa);
        ok(hres == S_OK, "Lock VARTYPE %d (@%d elements) failed; hres 0x%lx\n",
           vt, element, hres);

        if (hres == S_OK)
        {
          hres = SafeArrayUnlock(sa);
          ok(hres == S_OK, "Unlock VARTYPE %d (@%d elements) failed; hres 0x%lx\n",
             vt, element, hres);

          hres = SafeArrayDestroy(sa);
          ok(hres == S_OK, "destroy VARTYPE %d (@%d elements) failed; hres 0x%lx\n",
             vt, element, hres);
        }
      }
    }
  }
}

static void test_LockUnlock(void)
{
  SAFEARRAYBOUND sab[4];
  SAFEARRAY *sa;
  HRESULT hres;
  BOOL bVector = FALSE;
  int dimension;

  /* Failure cases */
  hres = SafeArrayLock(NULL);
  ok(hres == E_INVALIDARG, "Lock NULL array hres 0x%lx\n", hres);
  hres = SafeArrayUnlock(NULL);
  ok(hres == E_INVALIDARG, "Lock NULL array hres 0x%lx\n", hres);

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
  {
    sab[dimension].lLbound = 0;
    sab[dimension].cElements = 8;
  }

  sa = SafeArrayCreate(VT_UI1, ARRAY_SIZE(sab), sab);

  /* Test maximum locks */
test_LockUnlock_Vector:
  if (sa)
  {
    int count = 0;

    hres = SafeArrayUnlock(sa);
    ok (hres == E_UNEXPECTED, "Bad %sUnlock gave hres 0x%lx\n",
        bVector ? "vector " : "\n", hres);

    while ((hres = SafeArrayLock(sa)) == S_OK)
      count++;
    ok (count == 65535 && hres == E_UNEXPECTED, "Lock %sfailed at %d; hres 0x%lx\n",
        bVector ? "vector " : "\n", count, hres);

    if (count == 65535 && hres == E_UNEXPECTED)
    {
      while ((hres = SafeArrayUnlock(sa)) == S_OK)
        count--;
      ok (count == 0 && hres == E_UNEXPECTED, "Unlock %sfailed at %d; hres 0x%lx\n",
          bVector ? "vector " : "\n", count, hres);
    }

    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
  }

  if (bVector == FALSE)
  {
    /* Test again with a vector */
    sa = SafeArrayCreateVector(VT_UI1, 0, 100);
    bVector = TRUE;
    goto test_LockUnlock_Vector;
  }
}

static void test_SafeArrayGetPutElement(void)
{
  SAFEARRAYBOUND sab[4];
  LONG indices[ARRAY_SIZE(sab)], index;
  SAFEARRAY *sa;
  HRESULT hres;
  int value = 0, gotvalue, dimension;
  IRecordInfoImpl *irec;
  unsigned int x,y,z,a;

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
  {
    sab[dimension].lLbound = dimension * 2 + 1;
    sab[dimension].cElements = dimension * 3 + 1;
  }

  sa = SafeArrayCreate(VT_INT, ARRAY_SIZE(sab), sab);
  if (!sa)
    return; /* Some early versions can't handle > 3 dims */

  ok(sa->cbElements == sizeof(value), "int size mismatch\n");

  /* Failure cases */
  for (x = 0; x < ARRAY_SIZE(sab); x++)
  {
    indices[0] = sab[0].lLbound;
    indices[1] = sab[1].lLbound;
    indices[2] = sab[2].lLbound;
    indices[3] = sab[3].lLbound;

    indices[x] = indices[x] - 1;
    hres = SafeArrayPutElement(sa, indices, &value);
    ok(hres == DISP_E_BADINDEX, "Put allowed too small index in dimension %d\n", x);
    hres = SafeArrayGetElement(sa, indices, &value);
    ok(hres == DISP_E_BADINDEX, "Get allowed too small index in dimension %d\n", x);

    indices[x] = sab[x].lLbound + sab[x].cElements;
    hres = SafeArrayPutElement(sa, indices, &value);
    ok(hres == DISP_E_BADINDEX, "Put allowed too big index in dimension %d\n", x);
    hres = SafeArrayGetElement(sa, indices, &value);
    ok(hres == DISP_E_BADINDEX, "Get allowed too big index in dimension %d\n", x);
  }

  indices[0] = sab[0].lLbound;
  indices[1] = sab[1].lLbound;
  indices[2] = sab[2].lLbound;
  indices[3] = sab[3].lLbound;

  hres = SafeArrayPutElement(NULL, indices, &value);
  ok(hres == E_INVALIDARG, "Put NULL array hres 0x%lx\n", hres);
  hres = SafeArrayGetElement(NULL, indices, &value);
  ok(hres == E_INVALIDARG, "Get NULL array hres 0x%lx\n", hres);

  hres = SafeArrayPutElement(sa, NULL, &value);
  ok(hres == E_INVALIDARG, "Put NULL indices hres 0x%lx\n", hres);
  hres = SafeArrayGetElement(sa, NULL, &value);
  ok(hres == E_INVALIDARG, "Get NULL indices hres 0x%lx\n", hres);

  if (0)
  {
  /* This is retarded. Windows checks every case of invalid parameters
   * except the following, which crashes. We ERR this in Wine.
   */
  hres = SafeArrayPutElement(sa, indices, NULL);
  ok(hres == E_INVALIDARG, "Put NULL value hres 0x%lx\n", hres);
  }

  hres = SafeArrayGetElement(sa, indices, NULL);
  ok(hres == E_INVALIDARG, "Get NULL value hres 0x%lx\n", hres);

  value = 0;

  /* Make sure we can read and get back the correct values in 4 dimensions,
   * Each with a different size and lower bound.
   */
  for (x = 0; x < sab[0].cElements; x++)
  {
    indices[0] = sab[0].lLbound + x;
    for (y = 0; y < sab[1].cElements; y++)
    {
      indices[1] = sab[1].lLbound + y;
      for (z = 0; z < sab[2].cElements; z++)
      {
        indices[2] = sab[2].lLbound + z;
        for (a = 0; a < sab[3].cElements; a++)
        {
          indices[3] = sab[3].lLbound + a;
          hres = SafeArrayPutElement(sa, indices, &value);
          ok(hres == S_OK, "Failed to put element at (%d,%d,%d,%d) hres 0x%lx\n",
             x, y, z, a, hres);
          value++;
        }
      }
    }
  }

  value = 0;

  for (x = 0; x < sab[0].cElements; x++)
  {
    indices[0] = sab[0].lLbound + x;
    for (y = 0; y < sab[1].cElements; y++)
    {
      indices[1] = sab[1].lLbound + y;
      for (z = 0; z < sab[2].cElements; z++)
      {
        indices[2] = sab[2].lLbound + z;
        for (a = 0; a < sab[3].cElements; a++)
        {
          indices[3] = sab[3].lLbound + a;
          gotvalue = value / 3;
          hres = SafeArrayGetElement(sa, indices, &gotvalue);
          ok(hres == S_OK, "Failed to get element at (%d,%d,%d,%d) hres 0x%lx\n",
             x, y, z, a, hres);
          if (hres == S_OK)
            ok(value == gotvalue, "Got value %d instead of %d at (%d,%d,%d,%d)\n",
               gotvalue, value, x, y, z, a);
          value++;
        }
      }
    }
  }
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* VT_RECORD array */
  irec = IRecordInfoImpl_Construct();
  irec->ref = 1;

  sab[0].lLbound = 0;
  sab[0].cElements = 8;

  sa = pSafeArrayCreateEx(VT_RECORD, 1, sab, &irec->IRecordInfo_iface);
  ok(sa != NULL, "failed to create array\n");
  ok(irec->ref == 2, "got %ld\n", irec->ref);

  index = 0;
  irec->recordcopy = 0;
  hres = SafeArrayPutElement(sa, &index, (void*)0xdeadbeef);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(irec->recordcopy == 1, "got %d\n", irec->recordcopy);

  index = 0;
  irec->recordcopy = 0;
  hres = SafeArrayGetElement(sa, &index, (void*)0xdeadbeef);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(irec->recordcopy == 1, "got %d\n", irec->recordcopy);

  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(irec->ref == 1, "got %ld\n", irec->ref);
  IRecordInfo_Release(&irec->IRecordInfo_iface);
}

static void test_SafeArrayGetPutElement_BSTR(void)
{
  SAFEARRAYBOUND sab;
  LONG indices[1];
  SAFEARRAY *sa;
  HRESULT hres;
  BSTR value = 0, gotvalue;
  const OLECHAR szTest[5] = { 'T','e','s','t','\0' };

  sab.lLbound = 1;
  sab.cElements = 1;

  sa = SafeArrayCreate(VT_BSTR, 1, &sab);
  ok(sa != NULL, "BSTR test couldn't create array\n");
  if (!sa)
    return;

  ok(sa->cbElements == sizeof(BSTR), "BSTR size mismatch\n");

  indices[0] = sab.lLbound;
  value = SysAllocString(szTest);
  ok (value != NULL, "Expected non-NULL\n");
  hres = SafeArrayPutElement(sa, indices, value);
  ok(hres == S_OK, "Failed to put bstr element hres 0x%lx\n", hres);
  gotvalue = NULL;
  hres = SafeArrayGetElement(sa, indices, &gotvalue);
  ok(hres == S_OK, "Failed to get bstr element at hres 0x%lx\n", hres);
  if (hres == S_OK)
    ok(SysStringLen(value) == SysStringLen(gotvalue), "Got len %d instead of %d\n", SysStringLen(gotvalue), SysStringLen(value));
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  SysFreeString(value);
  SysFreeString(gotvalue);
}

struct xtunk_impl {
  IUnknown IUnknown_iface;
  LONG ref;
};
static const IUnknownVtbl xtunk_vtbl;

static struct xtunk_impl xtunk = {{&xtunk_vtbl}, 0};

static HRESULT WINAPI tunk_QueryInterface(IUnknown *punk, REFIID riid, void **x)
{
  return E_FAIL;
}

static ULONG WINAPI tunk_AddRef(IUnknown *punk)
{
  return ++xtunk.ref;
}

static ULONG WINAPI tunk_Release(IUnknown *punk)
{
  return --xtunk.ref;
}

static const IUnknownVtbl xtunk_vtbl = {
	tunk_QueryInterface,
	tunk_AddRef,
	tunk_Release
};

static void test_SafeArrayGetPutElement_IUnknown(void)
{
  SAFEARRAYBOUND sab;
  LONG indices[1];
  SAFEARRAY *sa;
  HRESULT hres;
  IUnknown *gotvalue;

  sab.lLbound = 1;
  sab.cElements = 1;
  sa = SafeArrayCreate(VT_UNKNOWN, 1, &sab);
  ok(sa != NULL, "UNKNOWN test couldn't create array\n");
  if (!sa)
    return;

  ok(sa->cbElements == sizeof(LPUNKNOWN), "LPUNKNOWN size mismatch\n");

  indices[0] = sab.lLbound;
  xtunk.ref = 1;
  hres = SafeArrayPutElement(sa, indices, &xtunk.IUnknown_iface);
  ok(hres == S_OK, "Failed to put bstr element hres 0x%lx\n", hres);
  ok(xtunk.ref == 2,"Failed to increment refcount of iface.\n");
  gotvalue = NULL;
  hres = SafeArrayGetElement(sa, indices, &gotvalue);
  ok(xtunk.ref == 3,"Failed to increment refcount of iface.\n");
  ok(hres == S_OK, "Failed to get bstr element at hres 0x%lx\n", hres);
  if (hres == S_OK)
    ok(gotvalue == &xtunk.IUnknown_iface, "Got %p instead of %p\n", gotvalue, &xtunk.IUnknown_iface);
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(xtunk.ref == 2,"Failed to decrement refcount of iface.\n");
}

static void test_SafeArrayGetPutElement_VARIANT(void)
{
  SAFEARRAYBOUND sab;
  LONG indices[1];
  SAFEARRAY *sa;
  HRESULT hres;
  VARIANT value, gotvalue;

  sab.lLbound = 1;
  sab.cElements = 1;
  sa = SafeArrayCreate(VT_VARIANT, 1, &sab);
  ok(sa != NULL, "VARIANT test couldn't create array\n");
  if (!sa)
    return;

  ok(sa->cbElements == sizeof(VARIANT), "VARIANT size mismatch\n");

  indices[0] = sab.lLbound;
  V_VT(&value) = VT_I4;
  V_I4(&value) = 0x42424242;
  hres = SafeArrayPutElement(sa, indices, &value);
  ok(hres == S_OK, "Failed to put Variant I4 element hres 0x%lx\n", hres);

  V_VT(&gotvalue) = 0xdead;
  hres = SafeArrayGetElement(sa, indices, &gotvalue);
  ok(hres == S_OK, "Failed to get variant element at hres 0x%lx\n", hres);

  V_VT(&gotvalue) = VT_EMPTY;
  hres = SafeArrayGetElement(sa, indices, &gotvalue);
  ok(hres == S_OK, "Failed to get variant element at hres 0x%lx\n", hres);
  if (hres == S_OK) {
    ok(V_VT(&value) == V_VT(&gotvalue), "Got type 0x%x instead of 0x%x\n", V_VT(&value), V_VT(&gotvalue));
    if (V_VT(&value) == V_VT(&gotvalue))
        ok(V_I4(&value) == V_I4(&gotvalue), "Got %ld instead of %d\n", V_I4(&value), V_VT(&gotvalue));
  }
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
}

static void test_SafeArrayCopyData(void)
{
  SAFEARRAYBOUND sab[4];
  SAFEARRAY *sa;
  SAFEARRAY *sacopy;
  HRESULT hres;
  int dimension, size = 1, i;

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
  {
    sab[dimension].lLbound = dimension * 2 + 2;
    sab[dimension].cElements = dimension * 3 + 1;
    size *= sab[dimension].cElements;
  }

  sa = SafeArrayCreate(VT_INT, ARRAY_SIZE(sab), sab);
  ok(sa != NULL, "Copy test couldn't create array\n");
  sacopy = SafeArrayCreate(VT_INT, ARRAY_SIZE(sab), sab);
  ok(sacopy != NULL, "Copy test couldn't create copy array\n");

  if (!sa || !sacopy)
    return;

  ok(sa->cbElements == sizeof(int), "int size mismatch\n");

  /* Fill the source array with some data; it doesn't matter what */
  for (dimension = 0; dimension < size; dimension++)
  {
    int* data = sa->pvData;
    data[dimension] = dimension;
  }

  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == S_OK, "copy data failed hres 0x%lx\n", hres);
  if (hres == S_OK)
  {
    ok(!memcmp(sa->pvData, sacopy->pvData, size * sizeof(int)), "compared different\n");
  }

  /* Failure cases */
  hres = SafeArrayCopyData(NULL, sacopy);
  ok(hres == E_INVALIDARG, "Null copy source hres 0x%lx\n", hres);
  hres = SafeArrayCopyData(sa, NULL);
  ok(hres == E_INVALIDARG, "Null copy hres 0x%lx\n", hres);

  sacopy->rgsabound[0].cElements += 1;
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "Bigger copy first dimension hres 0x%lx\n", hres);

  sacopy->rgsabound[0].cElements -= 2;
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "Smaller copy first dimension hres 0x%lx\n", hres);
  sacopy->rgsabound[0].cElements += 1;

  sacopy->rgsabound[3].cElements += 1;
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "Bigger copy last dimension hres 0x%lx\n", hres);

  sacopy->rgsabound[3].cElements -= 2;
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "Smaller copy last dimension hres 0x%lx\n", hres);
  sacopy->rgsabound[3].cElements += 1;

  hres = SafeArrayDestroy(sacopy);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  sacopy = NULL;
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "->Null copy hres 0x%lx\n", hres);

  hres = SafeArrayCopy(sa, &sacopy);
  ok(hres == S_OK, "copy failed hres 0x%lx\n", hres);
  ok(SafeArrayGetElemsize(sa) == SafeArrayGetElemsize(sacopy),"elemsize wrong\n");
  ok(SafeArrayGetDim(sa) == SafeArrayGetDim(sacopy),"dimensions wrong\n");
  ok(!memcmp(sa->pvData, sacopy->pvData, size * sizeof(int)), "compared different\n");
  hres = SafeArrayDestroy(sacopy);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  sacopy = SafeArrayCreate(VT_INT, ARRAY_SIZE(sab), sab);
  ok(sacopy != NULL, "Copy test couldn't create copy array\n");
  ok(sacopy->fFeatures == FADF_HAVEVARTYPE, "0x%04x\n", sacopy->fFeatures);

  for (i = 0; i < ARRAY_SIZE(ignored_copy_features); i++)
  {
      USHORT feature = ignored_copy_features[i];
      USHORT orig = sacopy->fFeatures;

      sa->fFeatures |= feature;
      hres = SafeArrayCopyData(sa, sacopy);
      ok(hres == S_OK, "got 0x%08lx\n", hres);
      ok(sacopy->fFeatures == orig && orig == FADF_HAVEVARTYPE, "got features 0x%04x\n", sacopy->fFeatures);
      sa->fFeatures &= ~feature;
  }

  hres = SafeArrayDestroy(sacopy);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* copy data from a vector */
  sa = SafeArrayCreateVector(VT_UI1, 0, 2);

  sacopy = SafeArrayCreateVector(VT_UI1, 0, 2);
  ok(sa->fFeatures == (FADF_HAVEVARTYPE|FADF_CREATEVECTOR) ||
     broken(sa->fFeatures == FADF_CREATEVECTOR /* W2k */),
     "got 0x%08x\n", sa->fFeatures);
  ok(sacopy->fFeatures == (FADF_HAVEVARTYPE|FADF_CREATEVECTOR) ||
     broken(sacopy->fFeatures == FADF_CREATEVECTOR /* W2k */),
     "got 0x%08x\n", sacopy->fFeatures);
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sacopy->fFeatures == (FADF_HAVEVARTYPE|FADF_CREATEVECTOR) ||
     broken(sacopy->fFeatures == FADF_CREATEVECTOR /* W2k */),
     "got 0x%04x\n", sacopy->fFeatures);
  SafeArrayDestroy(sacopy);

  sacopy = SafeArrayCreate(VT_UI1, ARRAY_SIZE(sab), sab);
  ok(sacopy != NULL, "Copy test couldn't create copy array\n");
  ok(sacopy->fFeatures == FADF_HAVEVARTYPE, "0x%04x\n", sacopy->fFeatures);
  hres = SafeArrayCopyData(sa, sacopy);
  ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);
  SafeArrayDestroy(sacopy);

  SafeArrayDestroy(sa);
}

static void test_SafeArrayCreateEx(void)
{
  IRecordInfoImpl* iRec;
  SAFEARRAYBOUND sab[4];
  SAFEARRAY *sa;
  HRESULT hres;
  UINT dimension;

  if (!pSafeArrayCreateEx)
  {
    win_skip("SafeArrayCreateEx not supported\n");
    return;
  }

  for (dimension = 0; dimension < ARRAY_SIZE(sab); dimension++)
  {
    sab[dimension].lLbound = 0;
    sab[dimension].cElements = 8;
  }

  /* Failure cases */
  sa = pSafeArrayCreateEx(VT_UI1, 1, NULL, NULL);
  ok(sa == NULL, "CreateEx NULL bounds didn't fail\n");

  /* test IID storage & defaulting */
  sa = pSafeArrayCreateEx(VT_DISPATCH, 1, sab, (PVOID)&IID_ITypeInfo);
  ok(sa != NULL, "CreateEx (ITypeInfo) failed\n");

  if (sa)
  {
    GUID guid;

    hres = SafeArrayGetIID(sa, &guid);
    ok(hres == S_OK, "Failed to get array IID, hres %#lx.\n", hres);
    ok(IsEqualGUID(&guid, &IID_ITypeInfo), "CreateEx (ITypeInfo) bad IID\n");
    hres = SafeArraySetIID(sa, &IID_IUnknown);
    ok(hres == S_OK, "Failed to set IID, hres = %8lx\n", hres);
    hres = SafeArrayGetIID(sa, &guid);
    ok(hres == S_OK && IsEqualGUID(&guid, &IID_IUnknown), "Set bad IID\n");
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
  }

  sa = pSafeArrayCreateEx(VT_DISPATCH, 1, sab, NULL);
  ok(sa != NULL, "CreateEx (NULL) failed\n");

  if (sa)
  {
    GUID guid;

    hres = SafeArrayGetIID(sa, &guid);
    ok(hres == S_OK, "Failed to get array IID, hres %#lx.\n", hres);
    ok(IsEqualGUID(&guid, &IID_IDispatch), "CreateEx (NULL) bad IID\n");
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
  }

  sa = pSafeArrayCreateEx(VT_UNKNOWN, 1, sab, NULL);
  ok(sa != NULL, "CreateEx (NULL-Unk) failed\n");

  if (sa)
  {
    GUID guid;

    hres = SafeArrayGetIID(sa, &guid);
    ok(hres == S_OK, "Failed to get array IID, hres %#lx.\n", hres);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "CreateEx (NULL-Unk) bad IID\n");
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
  }

  /* VT_RECORD failure case */
  sa = pSafeArrayCreateEx(VT_RECORD, 1, sab, NULL);
  ok(sa == NULL, "CreateEx (NULL-Rec) succeeded\n");

  iRec = IRecordInfoImpl_Construct();

  /* Win32 doesn't care if GetSize fails */
  fail_GetSize = TRUE;
  sa = pSafeArrayCreateEx(VT_RECORD, 1, sab, &iRec->IRecordInfo_iface);
  ok(sa != NULL, "CreateEx (Fail Size) failed\n");
  ok(iRec->ref == START_REF_COUNT + 1, "Wrong iRec refcount %ld\n", iRec->ref);
  ok(iRec->sizeCalled == 1, "GetSize called %d times\n", iRec->sizeCalled);
  ok(iRec->clearCalled == 0, "Clear called %d times\n", iRec->clearCalled);
  if (sa)
  {
    ok(sa->cbElements == RECORD_SIZE_FAIL, "Altered size to %ld\n", sa->cbElements);
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(iRec->clearCalled == sab[0].cElements, "Destroy->Clear called %d times\n", iRec->clearCalled);
    ok(iRec->ref == START_REF_COUNT, "got %ld, expected %d\n", iRec->ref, START_REF_COUNT);
  }

  /* Test VT_RECORD array */
  fail_GetSize = FALSE;
  iRec->ref = START_REF_COUNT;
  iRec->sizeCalled = 0;
  iRec->clearCalled = 0;
  sa = pSafeArrayCreateEx(VT_RECORD, 1, sab, &iRec->IRecordInfo_iface);
  ok(sa != NULL, "CreateEx (Rec) failed\n");
  ok(iRec->ref == START_REF_COUNT + 1, "Wrong iRec refcount %ld\n", iRec->ref);
  ok(iRec->sizeCalled == 1, "GetSize called %d times\n", iRec->sizeCalled);
  ok(iRec->clearCalled == 0, "Clear called %d times\n", iRec->clearCalled);
  if (sa && pSafeArrayGetRecordInfo)
  {
    IRecordInfo* saRec = NULL;
    SAFEARRAY *sacopy;

    hres = pSafeArrayGetRecordInfo(sa, &saRec);
    ok(hres == S_OK,"GRI failed\n");
    ok(saRec == &iRec->IRecordInfo_iface, "Different saRec\n");
    ok(iRec->ref == START_REF_COUNT + 2, "Didn't AddRef %ld\n", iRec->ref);
    IRecordInfo_Release(saRec);

    ok(sa->cbElements == RECORD_SIZE,"Elemsize is %ld\n", sa->cbElements);

    /* try to copy record based arrays */
    sacopy = pSafeArrayCreateEx(VT_RECORD, 1, sab, &iRec->IRecordInfo_iface);
    iRec->recordcopy = 0;
    iRec->clearCalled = 0;
    /* array copy code doesn't explicitly clear a record */
    hres = SafeArrayCopyData(sa, sacopy);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(iRec->recordcopy == sab[0].cElements, "got %d\n", iRec->recordcopy);
    ok(iRec->clearCalled == 0, "got %d\n", iRec->clearCalled);

    hres = SafeArrayDestroy(sacopy);
    ok(hres == S_OK, "got 0x%08lx\n", hres);

    iRec->clearCalled = 0;
    iRec->sizeCalled = 0;
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(iRec->sizeCalled == 0, "Destroy->GetSize called %d times\n", iRec->sizeCalled);
    ok(iRec->clearCalled == sab[0].cElements, "Destroy->Clear called %d times\n", iRec->clearCalled);
    ok(iRec->ref == START_REF_COUNT, "Wrong iRec refcount %ld\n", iRec->ref);
  }
  else
  {
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
  }

  IRecordInfo_Release(&iRec->IRecordInfo_iface);
}

static void test_SafeArrayClear(void)
{
  SAFEARRAYBOUND sab;
  SAFEARRAY *sa;
  VARIANTARG v;
  HRESULT hres;

  sab.lLbound = 0;
  sab.cElements = 10;
  sa = SafeArrayCreate(VT_UI1, 1, &sab);
  ok(sa != NULL, "Create() failed.\n");
  if (!sa)
    return;

  /* Test clearing non-NULL variants containing arrays */
  V_VT(&v) = VT_ARRAY|VT_UI1;
  V_ARRAY(&v) = sa;
  hres = VariantClear(&v);
  ok(hres == S_OK && V_VT(&v) == VT_EMPTY, "VariantClear: hres 0x%lx, Type %d\n", hres, V_VT(&v));
  ok(V_ARRAY(&v) == sa, "VariantClear: Overwrote value\n");

  sa = SafeArrayCreate(VT_UI1, 1, &sab);
  ok(sa != NULL, "Create() failed.\n");
  if (!sa)
    return;

  V_VT(&v) = VT_SAFEARRAY;
  V_ARRAY(&v) = sa;
  hres = VariantClear(&v);
  ok(hres == DISP_E_BADVARTYPE, "VariantClear: hres 0x%lx\n", hres);

  V_VT(&v) = VT_SAFEARRAY|VT_BYREF;
  V_ARRAYREF(&v) = &sa;
  hres = VariantClear(&v);
  ok(hres == DISP_E_BADVARTYPE, "VariantClear: hres 0x%lx\n", hres);

  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
}

static void test_SafeArrayCopy(void)
{
  SAFEARRAYBOUND sab;
  SAFEARRAY *sa, *sa2;
  VARIANTARG vSrc, vDst;
  HRESULT hres;
  int i;

  sab.lLbound = 0;
  sab.cElements = 10;
  sa = SafeArrayCreate(VT_UI1, 1, &sab);
  ok(sa != NULL, "Create() failed.\n");
  if (!sa)
    return;

  /* Test copying non-NULL variants containing arrays */
  V_VT(&vSrc) = (VT_ARRAY|VT_BYREF|VT_UI1);
  V_ARRAYREF(&vSrc) = &sa;
  V_VT(&vDst) = VT_EMPTY;

  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == (VT_ARRAY|VT_BYREF|VT_UI1),
     "VariantCopy: hres 0x%lx, Type %d\n", hres, V_VT(&vDst));
  ok(V_ARRAYREF(&vDst) == &sa, "VariantClear: Performed deep copy\n");

  V_VT(&vSrc) = (VT_ARRAY|VT_UI1);
  V_ARRAY(&vSrc) = sa;
  V_VT(&vDst) = VT_EMPTY;

  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK && V_VT(&vDst) == (VT_ARRAY|VT_UI1),
     "VariantCopy: hres 0x%lx, Type %d\n", hres, V_VT(&vDst));
  ok(V_ARRAY(&vDst) != sa, "VariantClear: Performed shallow copy\n");

  hres = SafeArrayDestroy(V_ARRAY(&vSrc));
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = SafeArrayDestroy(V_ARRAY(&vDst));
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = SafeArrayAllocDescriptor(1, &sa);
  ok(hres == S_OK, "SafeArrayAllocDescriptor failed with error 0x%08lx\n", hres);

  sa->cbElements = 16;
  hres = SafeArrayCopy(sa, &sa2);
  ok(hres == S_OK, "SafeArrayCopy failed with error 0x%08lx\n", hres);
  ok(sa != sa2, "SafeArrayCopy performed shallow copy\n");

  hres = SafeArrayDestroy(sa2);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  sa2 = (void*)0xdeadbeef;
  hres = SafeArrayCopy(NULL, &sa2);
  ok(hres == S_OK, "SafeArrayCopy failed with error 0x%08lx\n", hres);
  ok(!sa2, "SafeArrayCopy didn't return NULL for output array\n");

  hres = SafeArrayAllocDescriptor(1, &sa);
  ok(hres == S_OK, "SafeArrayAllocDescriptor failed with error 0x%08lx\n", hres);

  sa2 = (void*)0xdeadbeef;
  hres = SafeArrayCopy(sa, &sa2);
  ok(hres == E_INVALIDARG,
    "SafeArrayCopy with empty array should have failed with error E_INVALIDARG instead of 0x%08lx\n",
    hres);
  ok(!sa2, "SafeArrayCopy didn't return NULL for output array\n");

  hres = SafeArrayDestroy(sa2);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* test feature copy */
  hres = SafeArrayAllocDescriptor(1, &sa);
  ok(hres == S_OK, "SafeArrayAllocDescriptor failed with error 0x%08lx\n", hres);
  ok(sa->fFeatures == 0, "got src features 0x%04x\n", sa->fFeatures);
  sa->cbElements = 16;

  for (i = 0; i < ARRAY_SIZE(ignored_copy_features); i++)
  {
      USHORT feature = ignored_copy_features[i];

      sa->fFeatures |= feature;
      hres = SafeArrayCopy(sa, &sa2);
      ok(hres == S_OK, "got 0x%08lx\n", hres);
      ok(sa2->fFeatures == 0, "got features 0x%04x\n", sa2->fFeatures);
      hres = SafeArrayDestroy(sa2);
      ok(hres == S_OK, "got 0x%08lx\n", hres);
      sa->fFeatures &= ~feature;
  }

  SafeArrayDestroy(sa);

  /* copy from a vector */
  sa = SafeArrayCreateVector(VT_UI1, 0, 2);
  ok(sa->fFeatures == (FADF_HAVEVARTYPE|FADF_CREATEVECTOR) ||
     broken(sa->fFeatures == FADF_CREATEVECTOR /* W2k */),
     "got 0x%08x\n", sa->fFeatures);
  hres = SafeArrayCopy(sa, &sa2);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sa2->fFeatures == FADF_HAVEVARTYPE ||
     broken(!sa2->fFeatures /* W2k */), "got 0x%04x\n",
     sa2->fFeatures);

  SafeArrayDestroy(sa2);
  SafeArrayDestroy(sa);
}

#define MKARRAY(low,num,typ) sab.lLbound = low; sab.cElements = num; \
  sa = SafeArrayCreate(typ, 1, &sab); ok(sa != NULL, "Create() failed.\n"); \
  if (!sa) return; \
  V_VT(&v) = VT_ARRAY|typ; V_ARRAY(&v) = sa; VariantInit(&v2)

#define MKARRAYCONT(low,num,typ) sab.lLbound = low; sab.cElements = num; \
  sa = SafeArrayCreate(typ, 1, &sab); if (!sa) continue; \
  V_VT(&v) = VT_ARRAY|typ; V_ARRAY(&v) = sa; VariantInit(&v2)

static void test_SafeArrayChangeTypeEx(void)
{
  static const char *szHello = "Hello World";
  SAFEARRAYBOUND sab;
  SAFEARRAY *sa;
  VARIANTARG v,v2;
  VARTYPE vt;
  HRESULT hres;

  /* VT_ARRAY|VT_UI1 -> VT_BSTR */
  MKARRAY(0,strlen(szHello)+1,VT_UI1);
  memcpy(sa->pvData, szHello, strlen(szHello)+1);

  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_BSTR);
  ok(hres == S_OK, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR failed with %lx\n", hres);
  if (hres == S_OK)
  {
    ok(V_VT(&v2) == VT_BSTR, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR did not return VT_BSTR, but %d.\n",V_VT(&v2));
    ok(strcmp((char*)V_BSTR(&v2),szHello) == 0,"Expected string '%s', got '%s'\n", szHello,
       (char*)V_BSTR(&v2));
    VariantClear(&v2);
  }

  /* VT_VECTOR|VT_UI1 -> VT_BSTR */
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  sa = SafeArrayCreateVector(VT_UI1, 0, strlen(szHello)+1);
  ok(sa != NULL, "CreateVector() failed.\n");
  if (!sa)
    return;

  memcpy(sa->pvData, szHello, strlen(szHello)+1);
  V_VT(&v) = VT_VECTOR|VT_UI1;
  V_ARRAY(&v) = sa;
  VariantInit(&v2);

  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_BSTR);
  ok(hres == DISP_E_BADVARTYPE, "CTE VT_VECTOR|VT_UI1 returned %lx\n", hres);

  /* (vector)VT_ARRAY|VT_UI1 -> VT_BSTR (In place) */
  V_VT(&v) = VT_ARRAY|VT_UI1;
  hres = VariantChangeTypeEx(&v, &v, 0, 0, VT_BSTR);
  ok(hres == S_OK, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR failed with %lx\n", hres);
  if (hres == S_OK)
  {
    ok(V_VT(&v) == VT_BSTR, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR did not return VT_BSTR, but %d.\n",V_VT(&v));
    ok(strcmp((char*)V_BSTR(&v),szHello) == 0,"Expected string '%s', got '%s'\n", szHello,
            (char*)V_BSTR(&v));
    VariantClear(&v);
  }

  /* To/from BSTR only works with arrays of VT_UI1 */
  for (vt = VT_EMPTY; vt <= VT_CLSID; vt++)
  {
    if (vt == VT_UI1)
      continue;

    sab.lLbound = 0;
    sab.cElements = 1;
    sa = SafeArrayCreate(vt, 1, &sab);
    if (!sa) continue;

    V_VT(&v) = VT_ARRAY|vt;
    V_ARRAY(&v) = sa;
    VariantInit(&v2);

    hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_BSTR);
    if (vt == VT_INT_PTR || vt == VT_UINT_PTR)
    {
        ok(hres == DISP_E_BADVARTYPE, "expected DISP_E_BADVARTYPE, got 0x%08lx\n", hres);
        SafeArrayDestroy(sa);
    }
    else
    {
        ok(hres == DISP_E_TYPEMISMATCH, "got 0x%08lx for vt=%d, instead of DISP_E_TYPEMISMATCH\n", hres, vt);
        hres = VariantClear(&v);
        ok(hres == S_OK, "expected S_OK, got 0x%08lx\n", hres);
    }
    VariantClear(&v2);
  }

  /* Can't change an array of one type into array of another type , even
   * if the other type is the same size
   */
  sa = SafeArrayCreateVector(VT_UI1, 0, 1);
  ok(sa != NULL, "CreateVector() failed.\n");
  if (!sa)
    return;

  V_VT(&v) = VT_ARRAY|VT_UI1;
  V_ARRAY(&v) = sa;
  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_ARRAY|VT_I1);
  ok(hres == DISP_E_TYPEMISMATCH, "CTE VT_ARRAY|VT_UI1->VT_ARRAY|VT_I1 returned %lx\n", hres);

  /* But can change to the same array type */
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  sa = SafeArrayCreateVector(VT_UI1, 0, 1);
  ok(sa != NULL, "CreateVector() failed.\n");
  if (!sa)
    return;
  V_VT(&v) = VT_ARRAY|VT_UI1;
  V_ARRAY(&v) = sa;
  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_ARRAY|VT_UI1);
  ok(hres == S_OK, "CTE VT_ARRAY|VT_UI1->VT_ARRAY|VT_UI1 returned %lx\n", hres);
  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  VariantClear(&v2);

  /* NULL/EMPTY */
  MKARRAY(0,1,VT_UI1);
  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_NULL);
  ok(hres == DISP_E_TYPEMISMATCH, "CTE VT_ARRAY|VT_UI1 returned %lx\n", hres);
  VariantClear(&v);
  MKARRAY(0,1,VT_UI1);
  hres = VariantChangeTypeEx(&v2, &v, 0, 0, VT_EMPTY);
  ok(hres == DISP_E_TYPEMISMATCH, "CTE VT_ARRAY|VT_UI1 returned %lx\n", hres);
  VariantClear(&v);
}

static void test_SafeArrayDestroyData (void)
{
  SAFEARRAYBOUND sab[2];
  SAFEARRAY *sa;
  HRESULT hres;
  int value = 0xdeadbeef;
  LONG index[1];
  void *temp_pvData;
  USHORT features;

  sab[0].lLbound = 0;
  sab[0].cElements = 10;
  sa = SafeArrayCreate(VT_INT, 1, sab);
  ok(sa != NULL, "Create() failed.\n");
  ok(sa->fFeatures == FADF_HAVEVARTYPE, "got 0x%x\n", sa->fFeatures);

  index[0] = 1;
  SafeArrayPutElement (sa, index, &value);

/* SafeArrayDestroyData shouldn't free pvData if FADF_STATIC is set. */
  features = (sa->fFeatures |= FADF_STATIC);
  temp_pvData = sa->pvData;
  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "SADData FADF_STATIC failed, error code %lx.\n",hres);
  ok(features == sa->fFeatures, "got 0x%x\n", sa->fFeatures);
  ok(sa->pvData == temp_pvData, "SADData FADF_STATIC: pvData=%p, expected %p (fFeatures = %d).\n",
    sa->pvData, temp_pvData, sa->fFeatures);
  SafeArrayGetElement (sa, index, &value);
  ok(value == 0, "Data not cleared after SADData\n");

/* Clear FADF_STATIC, now really destroy the data. */
  features = (sa->fFeatures ^= FADF_STATIC);
  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "SADData !FADF_STATIC failed, error code %lx.\n",hres);
  ok(features == sa->fFeatures, "got 0x%x\n", sa->fFeatures);
  ok(sa->pvData == NULL, "SADData !FADF_STATIC: pvData=%p, expected NULL.\n", sa->pvData);

  hres = SafeArrayDestroy(sa);
  ok(hres == S_OK, "SAD failed, error code %lx.\n", hres);

  /* two dimensions */
  sab[0].lLbound = 0;
  sab[0].cElements = 10;
  sab[1].lLbound = 0;
  sab[1].cElements = 10;

  sa = SafeArrayCreate(VT_INT, 2, sab);
  ok(sa != NULL, "Create() failed.\n");
  ok(sa->fFeatures == FADF_HAVEVARTYPE, "got 0x%x\n", sa->fFeatures);

  features = sa->fFeatures;
  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "got 0x%08lx\n",hres);
  ok(features == sa->fFeatures, "got 0x%x\n", sa->fFeatures);

  SafeArrayDestroy(sa);

  /* try to destroy data from descriptor */
  hres = SafeArrayAllocDescriptor(1, &sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sa->fFeatures == 0, "got 0x%x\n", sa->fFeatures);

  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sa->fFeatures == 0, "got 0x%x\n", sa->fFeatures);

  hres = SafeArrayDestroyDescriptor(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = SafeArrayAllocDescriptor(2, &sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sa->fFeatures == 0, "got 0x%x\n", sa->fFeatures);

  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(sa->fFeatures == 0, "got 0x%x\n", sa->fFeatures);

  hres = SafeArrayDestroyDescriptor(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* vector case */
  sa = SafeArrayCreateVector(VT_I4, 0, 10);
  ok(sa != NULL, "got %p\n", sa);
  ok(sa->fFeatures == (FADF_CREATEVECTOR|FADF_HAVEVARTYPE), "got 0x%x\n", sa->fFeatures);

  ok(sa->pvData != NULL, "got %p\n", sa->pvData);
  hres = SafeArrayDestroyData(sa);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  todo_wine
  ok(sa->fFeatures == FADF_HAVEVARTYPE, "got 0x%x\n", sa->fFeatures);
  todo_wine
  ok(sa->pvData == NULL || broken(sa->pvData != NULL), "got %p\n", sa->pvData);
  /* There was a bug on windows, especially visible on 64bit systems,
     probably double-free or similar issue. */
  sa->pvData = NULL;
  SafeArrayDestroy(sa);
}

static void test_safearray_layout(void)
{
    IRecordInfoImpl *irec;
    IRecordInfo *record;
    GUID guid, *guidptr;
    SAFEARRAYBOUND sab;
    SAFEARRAY *sa;
    DWORD *dwptr;
    HRESULT hr;

    sab.lLbound = 0;
    sab.cElements = 10;

    /* GUID field */
    sa = SafeArrayCreate(VT_UNKNOWN, 1, &sab);
    ok(sa != NULL, "got %p\n", sa);

    guidptr = (GUID*)sa - 1;
    ok(IsEqualIID(guidptr, &IID_IUnknown), "got %s\n", wine_dbgstr_guid(guidptr));

    hr = SafeArraySetIID(sa, &IID_IDispatch);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(guidptr, &IID_IDispatch), "got %s\n", wine_dbgstr_guid(guidptr));

    memcpy(guidptr, &IID_IUnknown, sizeof(GUID));
    hr = SafeArrayGetIID(sa, &guid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&guid, &IID_IUnknown), "got %s\n", wine_dbgstr_guid(&guid));

    hr = SafeArrayDestroy(sa);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* VARTYPE field */
    sa = SafeArrayCreate(VT_UI1, 1, &sab);
    ok(sa != NULL, "got %p\n", sa);

    dwptr = (DWORD*)sa - 1;
    ok(*dwptr == VT_UI1, "got %ld\n", *dwptr);

    hr = SafeArrayDestroy(sa);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* IRecordInfo pointer */
    irec = IRecordInfoImpl_Construct();
    irec->ref = 1;

    sa = pSafeArrayCreateEx(VT_RECORD, 1, &sab, &irec->IRecordInfo_iface);
    ok(sa != NULL, "failed to create array\n");

    record = *((IRecordInfo**)sa - 1);
    ok(record == &irec->IRecordInfo_iface, "got %p\n", record);

    hr = SafeArrayDestroy(sa);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IRecordInfo_Release(&irec->IRecordInfo_iface);
}

static void test_SafeArrayRedim(void)
{
    SAFEARRAYBOUND sab;
    LONG indices[1];
    SAFEARRAY *sa;
    HRESULT hres;

    sab.lLbound = 1;
    sab.cElements = 2;
    hres = SafeArrayRedim(NULL, &sab);
    ok(hres == E_INVALIDARG, "Unexpected hr %#lx.\n", hres);
    sa = SafeArrayCreate(VT_UNKNOWN, 1, &sab);
    ok(!!sa, "Failed to create an array.\n");
    hres = SafeArrayRedim(sa, NULL);
    ok(hres == E_INVALIDARG, "Unexpected hr %#lx.\n", hres);
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);

    /* VT_UNKNOWN */
    sab.lLbound = 1;
    sab.cElements = 2;
    sa = SafeArrayCreate(VT_UNKNOWN, 1, &sab);
    ok(!!sa, "Failed to create an array.\n");
    ok(sa->cbElements == sizeof(IUnknown *), "Unexpected element size.\n");

    indices[0] = 2;
    xtunk.ref = 1;
    hres = SafeArrayPutElement(sa, indices, &xtunk.IUnknown_iface);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    ok(xtunk.ref == 2,"Failed to increment refcount of iface.\n");
    sab.cElements = 1;
    hres = SafeArrayRedim(sa, &sab);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    ok(xtunk.ref == 1, "Failed to decrement refcount\n");
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);

    /* FADF_FIXEDSIZE */
    sab.lLbound = 1;
    sab.cElements = 2;
    sa = SafeArrayCreate(VT_I4, 1, &sab);
    ok(!!sa, "Failed to create an array.\n");
    sab.cElements = 3;
    hres = SafeArrayRedim(sa, &sab);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    sab.cElements = 4;
    sa->fFeatures |= FADF_FIXEDSIZE;
    hres = SafeArrayRedim(sa, &sab);
    ok(hres == DISP_E_ARRAYISLOCKED, "Unexpected hr %#lx.\n", hres);
    sa->fFeatures &= ~FADF_FIXEDSIZE;
    hres = SafeArrayRedim(sa, &sab);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);

    /* Locked array */
    sab.lLbound = 1;
    sab.cElements = 2;
    sa = SafeArrayCreate(VT_I4, 1, &sab);
    ok(!!sa, "Failed to create an array.\n");
    hres = SafeArrayLock(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    sab.cElements = 3;
    hres = SafeArrayRedim(sa, &sab);
    ok(hres == DISP_E_ARRAYISLOCKED, "Unexpected hr %#lx.\n", hres);
    hres = SafeArrayUnlock(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = SafeArrayDestroy(sa);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
}

START_TEST(safearray)
{
    hOleaut32 = GetModuleHandleA("oleaut32.dll");

    has_i8 = GetProcAddress(hOleaut32, "VarI8FromI1") != NULL;

    GETPTR(SafeArrayAllocDescriptorEx);
    GETPTR(SafeArrayGetVartype);
    GETPTR(SafeArrayCreateEx);
    GETPTR(SafeArrayGetRecordInfo);

    check_for_VT_INT_PTR();
    test_safearray();
    test_SafeArrayAllocDestroyDescriptor();
    test_SafeArrayCreateLockDestroy();
    test_VectorCreateLockDestroy();
    test_LockUnlock();
    test_SafeArrayChangeTypeEx();
    test_SafeArrayCopy();
    test_SafeArrayClear();
    test_SafeArrayCreateEx();
    test_SafeArrayCopyData();
    test_SafeArrayDestroyData();
    test_SafeArrayGetPutElement();
    test_SafeArrayGetPutElement_BSTR();
    test_SafeArrayGetPutElement_IUnknown();
    test_SafeArrayRedim();
    test_SafeArrayGetPutElement_VARIANT();
    test_safearray_layout();
}
