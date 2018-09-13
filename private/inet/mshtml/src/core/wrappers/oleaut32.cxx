//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       oleaut32.cxx
//
//  Contents:   Dynamic wrappers for OLE Automation monikers.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

DYNLIB g_dynlibOLEAUT32 = { NULL, NULL, "oleaut32.dll" };

#define WRAP_HR(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEAUT32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

#define WRAP_HR_NOTHR(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEAUT32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = (*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2;\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

#define WRAP(t, fn, a1, a2)\
STDAPI_(t) fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEAUT32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        return 0;\
    return (*(t (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2;\
}

#define WRAP_VOID(fn, a1, a2)\
STDAPI_(void) fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibOLEAUT32, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        return;\
    (*(void (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2;\
}

WINOLEAUTAPI_(void )
VariantInit(VARIANTARG *pvarg)
{
	pvarg->vt = VT_EMPTY;
}

WINOLEAUTAPI
VariantClear(VARIANTARG *pvarg)
{
    HRESULT hr;
    static DYNPROC s_dynprocVariantClear = { NULL, &g_dynlibOLEAUT32, "VariantClear" };

	switch (pvarg->vt)
	{
	case VT_I1:
	case VT_I2:
	case VT_I4:
	case VT_R4:
	case VT_R8:
	case VT_CY:
	case VT_NULL:
	case VT_EMPTY:
	case VT_BOOL:
		break;
		
	case VT_DISPATCH:
	case VT_UNKNOWN:
		ReleaseInterface(V_UNKNOWN(pvarg));
		break;

    case VT_SAFEARRAY:
        THR(SafeArrayDestroy(V_ARRAY(pvarg)));
        break;
		
	default:
		hr = THR(LoadProcedure(&s_dynprocVariantClear));
		if (hr)
			RRETURN(hr);
		hr = THR((*(HRESULT (STDAPICALLTYPE *) (VARIANTARG *))s_dynprocVariantClear.pfn)(pvarg));
		RRETURN(hr);
	}

	V_VT(pvarg) = VT_EMPTY;
	RRETURN(S_OK);
}

WRAP_HR(RegisterTypeLib,
    (ITypeLib *ptlib, OLECHAR *szFullPath, OLECHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir))

WRAP_HR(LoadTypeLib,
    (const OLECHAR *szFile, ITypeLib **pptlib), 
    (szFile, pptlib))

WRAP_HR(SetErrorInfo,
   (unsigned long dwReserved, IErrorInfo*perrinfo), 
   (dwReserved, perrinfo))

WRAP_HR(GetErrorInfo, 
    (ULONG dwReserved, IErrorInfo ** pperrinfo), 
    (dwReserved, pperrinfo))

WRAP_HR(LoadRegTypeLib,
    (REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib **pptlib),
    (rguid, wVerMajor, wVerMinor, lcid, pptlib))

WINOLEAUTAPI
VariantCopy(VARIANTARG *pvargDest, VARIANTARG *pvargSrc)
{
    HRESULT hr;
    static DYNPROC s_dynprocVariantCopy = { NULL, &g_dynlibOLEAUT32, "VariantCopy" };

	switch (pvargSrc->vt)
	{
	case VT_I1:
	case VT_I2:
	case VT_I4:
	case VT_R4:
	case VT_R8:
	case VT_CY:
	case VT_NULL:
	case VT_EMPTY:
	case VT_BOOL:
        hr = S_OK;
		break;
		
	case VT_DISPATCH:
	case VT_UNKNOWN:
        hr = S_OK;
		if ( V_UNKNOWN(pvargSrc) )
			V_UNKNOWN(pvargSrc)->AddRef();
		break;

    case VT_SAFEARRAY:
        hr = S_OK;
        pvargDest->vt = VT_SAFEARRAY;
        if (V_ARRAY(pvargSrc))
            hr = THR(SafeArrayCopy(V_ARRAY(pvargSrc), &V_ARRAY(pvargDest)));
        else
            V_ARRAY(pvargDest) = NULL;
        RRETURN(hr);
        break;

	default:
		hr = THR(LoadProcedure(&s_dynprocVariantCopy));
		if (hr)
			RRETURN(hr);
		hr = THR((*(HRESULT (STDAPICALLTYPE *) (VARIANTARG *, VARIANTARG *))s_dynprocVariantCopy.pfn)(pvargDest, pvargSrc));
		RRETURN(hr);
	}

    VariantClear(pvargDest);
    *pvargDest = *pvargSrc;
	RRETURN(hr);
}

WRAP_HR(VariantChangeType,
    (VARIANTARG *pvargDest, VARIANTARG *pvarSrc, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, wFlags, vt))

WRAP(BSTR, SysAllocStringLen,
    (const OLECHAR*pch, unsigned int i), (pch, i))

WRAP(BSTR, SysAllocString,
    (const OLECHAR*pch), (pch))

WRAP(BSTR, SysAllocStringByteLen, (LPCSTR psz, UINT len), (psz, len))

STDAPI_(void) SysFreeString(BSTR bs)
{
    if (bs)
    {
        HRESULT hr;
        static DYNPROC s_dynprocSysFreeString = { NULL, &g_dynlibOLEAUT32, "SysFreeString" };
        hr = THR(LoadProcedure(&s_dynprocSysFreeString));
        if (hr)
            return;
        (*(void (STDAPICALLTYPE *) (BSTR bs))s_dynprocSysFreeString.pfn)(bs);
    }
}

WRAP_HR(DispGetIDsOfNames,
    (ITypeInfo*ptinfo, OLECHAR **rgszNames, UINT cNames, DISPID*rgdispid),
    (ptinfo, rgszNames, cNames, rgdispid))

WRAP_HR(DispInvoke, 
        (void * _this, ITypeInfo * ptinfo, DISPID dispidMember, WORD wFlags, 
         DISPPARAMS * pparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr),
        ( _this, ptinfo, dispidMember, wFlags, pparams, pvarResult, pexcepinfo, puArgErr))

WRAP_HR(CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo))

WRAP(SAFEARRAY *, SafeArrayCreateVector,
    (VARTYPE vt, long iBound, ULONG cElements), (vt, iBound, cElements) );

WRAP_HR(SafeArrayAccessData,
    (SAFEARRAY * psa, void HUGEP** ppvData), (psa, ppvData))

WRAP_HR(SafeArrayUnaccessData,
    (SAFEARRAY * psa), (psa) );

WRAP(UINT, SafeArrayGetElemsize,
    (SAFEARRAY * psa), (psa) );

WRAP_HR(SafeArrayGetUBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plUBound),
    (psa,nDim,plUBound))

WRAP_HR(SafeArrayGetElement,
    (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv))

WRAP(UINT, SafeArrayGetDim,
    (SAFEARRAY * psa), (psa))

WRAP(UINT, SysStringLen,
    (BSTR bstr), (bstr))

WRAP_HR(SafeArrayDestroy,
    (SAFEARRAY * psa), (psa))

WRAP(INT, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime))

WRAP(UINT, SysStringByteLen, (BSTR bstr), (bstr))

WRAP_HR_NOTHR(VariantChangeTypeEx, 
    (VARIANTARG * pvargDest, VARIANTARG * pvarSrc, LCID lcid, USHORT wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, lcid, wFlags, vt))

WRAP_HR(CreateTypeLib2,
    (SYSKIND syskind, LPCOLESTR szFile, ICreateTypeLib2 **ppctlib),
    (syskind, szFile, ppctlib))

WRAP(SAFEARRAY *, SafeArrayCreate,
        (VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound),
        (vt, cDims, rgsabound))

WRAP_HR(SafeArrayPutElement,
        (SAFEARRAY * psa, LONG * rgIndices, void * pv),
        (psa, rgIndices, pv))

WRAP_HR(UnRegisterTypeLib,
    (REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind),
    (libID, wVerMajor, wVerMinor, lcid, syskind))

WRAP_HR(SafeArrayDestroyData,
    (SAFEARRAY * psa),
    (psa))

WRAP_HR(SafeArrayGetLBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plLbound),
    (psa, nDim, plLbound))

WRAP_HR(SafeArrayCopy,
    (SAFEARRAY * psa, SAFEARRAY ** ppsaOut),
    (psa, ppsaOut))

WRAP(INT, SysReAllocString, (BSTR *a, const OLECHAR *b), (a, b))
WRAP(INT, SysReAllocStringLen, (BSTR *a, const OLECHAR *b, UINT c), (a, b, c))

WRAP_HR(VarUI1FromI2, (SHORT sIn, BYTE * pbOut), (sIn, pbOut))
WRAP_HR(VarUI1FromI4, (LONG lIn, BYTE * pbOut), (lIn, pbOut))
WRAP_HR(VarUI1FromR4, (FLOAT fltIn, BYTE * pbOut), (fltIn, pbOut))
WRAP_HR(VarUI1FromR8, (DOUBLE dblIn, BYTE * pbOut), (dblIn, pbOut))
WRAP_HR(VarUI1FromCy, (CY cyIn, BYTE * pbOut), (cyIn, pbOut))
WRAP_HR(VarUI1FromDate, (DATE dateIn, BYTE * pbOut), (dateIn, pbOut))
WRAP_HR(VarUI1FromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, BYTE * pbOut), (strIn, lcid, dwFlags, pbOut))
WRAP_HR(VarUI1FromDisp, (IDispatch * pdispIn, LCID lcid, BYTE * pbOut), (pdispIn, lcid, pbOut))
WRAP_HR(VarUI1FromBool, (VARIANT_BOOL boolIn, BYTE * pbOut), (boolIn, pbOut))
WRAP_HR(VarUI1FromI1, (CHAR cIn, BYTE *pbOut), (cIn, pbOut))
WRAP_HR(VarUI1FromUI2, (USHORT uiIn, BYTE *pbOut), (uiIn, pbOut))
WRAP_HR(VarUI1FromUI4, (ULONG ulIn, BYTE *pbOut), (ulIn, pbOut))
WRAP_HR(VarUI1FromDec, (DECIMAL *pdecIn, BYTE *pbOut), (pdecIn, pbOut))
WRAP_HR(VarI2FromUI1, (BYTE bIn, SHORT * psOut), (bIn, psOut))
WRAP_HR(VarI2FromI4, (LONG lIn, SHORT * psOut), (lIn, psOut))
WRAP_HR(VarI2FromR4, (FLOAT fltIn, SHORT * psOut), (fltIn, psOut))
WRAP_HR(VarI2FromR8, (DOUBLE dblIn, SHORT * psOut), (dblIn, psOut))
WRAP_HR(VarI2FromCy, (CY cyIn, SHORT * psOut), (cyIn, psOut))
WRAP_HR(VarI2FromDate, (DATE dateIn, SHORT * psOut), (dateIn, psOut))
WRAP_HR(VarI2FromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, SHORT * psOut), (strIn, lcid, dwFlags, psOut))
WRAP_HR(VarI2FromDisp, (IDispatch * pdispIn, LCID lcid, SHORT * psOut), (pdispIn, lcid, psOut))
WRAP_HR(VarI2FromBool, (VARIANT_BOOL boolIn, SHORT * psOut), (boolIn, psOut))
WRAP_HR(VarI2FromI1, (CHAR cIn, SHORT *psOut), (cIn, psOut))
WRAP_HR(VarI2FromUI2, (USHORT uiIn, SHORT *psOut), (uiIn, psOut))
WRAP_HR(VarI2FromUI4, (ULONG ulIn, SHORT *psOut), (ulIn, psOut))
WRAP_HR(VarI2FromDec, (DECIMAL *pdecIn, SHORT *psOut), (pdecIn, psOut))
WRAP_HR(VarI4FromUI1, (BYTE bIn, LONG * plOut), (bIn, plOut))
WRAP_HR(VarI4FromI2, (SHORT sIn, LONG * plOut), (sIn, plOut))
WRAP_HR(VarI4FromR4, (FLOAT fltIn, LONG * plOut), (fltIn, plOut))
WRAP_HR(VarI4FromR8, (DOUBLE dblIn, LONG * plOut), (dblIn, plOut))
WRAP_HR(VarI4FromCy, (CY cyIn, LONG * plOut), (cyIn, plOut))
WRAP_HR(VarI4FromDate, (DATE dateIn, LONG * plOut), (dateIn, plOut))
WRAP_HR(VarI4FromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut))
WRAP_HR(VarI4FromDisp, (IDispatch * pdispIn, LCID lcid, LONG * plOut), (pdispIn, lcid, plOut))
WRAP_HR(VarI4FromBool, (VARIANT_BOOL boolIn, LONG * plOut), (boolIn, plOut))
WRAP_HR(VarI4FromI1, (CHAR cIn, LONG *plOut), (cIn, plOut))
WRAP_HR(VarI4FromUI2, (USHORT uiIn, LONG *plOut), (uiIn, plOut))
WRAP_HR(VarI4FromUI4, (ULONG ulIn, LONG *plOut), (ulIn, plOut))
WRAP_HR(VarI4FromDec, (DECIMAL *pdecIn, LONG *plOut), (pdecIn, plOut))
//WRAP_HR(VarI4FromInt, (INT intIn, LONG *plOut), (intIn, plOut))
WRAP_HR(VarR4FromUI1, (BYTE bIn, FLOAT * pfltOut), (bIn, pfltOut))
WRAP_HR(VarR4FromI2, (SHORT sIn, FLOAT * pfltOut), (sIn, pfltOut))
WRAP_HR(VarR4FromI4, (LONG lIn, FLOAT * pfltOut), (lIn, pfltOut))
WRAP_HR(VarR4FromR8, (DOUBLE dblIn, FLOAT * pfltOut), (dblIn, pfltOut))
WRAP_HR(VarR4FromCy, (CY cyIn, FLOAT * pfltOut), (cyIn, pfltOut))
WRAP_HR(VarR4FromDate, (DATE dateIn, FLOAT * pfltOut), (dateIn, pfltOut))
WRAP_HR(VarR4FromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, FLOAT *pfltOut), (strIn, lcid, dwFlags, pfltOut))
WRAP_HR(VarR4FromDisp, (IDispatch * pdispIn, LCID lcid, FLOAT * pfltOut), (pdispIn, lcid, pfltOut))
WRAP_HR(VarR4FromBool, (VARIANT_BOOL boolIn, FLOAT * pfltOut), (boolIn, pfltOut))
WRAP_HR(VarR4FromI1, (CHAR cIn, FLOAT *pfltOut), (cIn, pfltOut))
WRAP_HR(VarR4FromUI2, (USHORT uiIn, FLOAT *pfltOut), (uiIn, pfltOut))
WRAP_HR(VarR4FromUI4, (ULONG ulIn, FLOAT *pfltOut), (ulIn, pfltOut))
WRAP_HR(VarR4FromDec, (DECIMAL *pdecIn, FLOAT *pfltOut), (pdecIn, pfltOut))
WRAP_HR(VarR8FromUI1, (BYTE bIn, DOUBLE * pdblOut), (bIn, pdblOut))
WRAP_HR(VarR8FromI2, (SHORT sIn, DOUBLE * pdblOut), (sIn, pdblOut))
WRAP_HR(VarR8FromI4, (LONG lIn, DOUBLE * pdblOut), (lIn, pdblOut))
WRAP_HR(VarR8FromR4, (FLOAT fltIn, DOUBLE * pdblOut), (fltIn, pdblOut))
WRAP_HR(VarR8FromCy, (CY cyIn, DOUBLE * pdblOut), (cyIn, pdblOut))
WRAP_HR(VarR8FromDate, (DATE dateIn, DOUBLE * pdblOut), (dateIn, pdblOut))
WRAP_HR(VarR8FromStr, (OLECHAR *strIn, LCID lcid, ULONG dwFlags, DOUBLE *pdblOut), (strIn, lcid, dwFlags, pdblOut))
WRAP_HR(VarR8FromDisp, (IDispatch * pdispIn, LCID lcid, DOUBLE * pdblOut), (pdispIn, lcid, pdblOut))
WRAP_HR(VarR8FromBool, (VARIANT_BOOL boolIn, DOUBLE * pdblOut), (boolIn, pdblOut))
WRAP_HR(VarR8FromI1, (CHAR cIn, DOUBLE *pdblOut), (cIn, pdblOut))
WRAP_HR(VarR8FromUI2, (USHORT uiIn, DOUBLE *pdblOut), (uiIn, pdblOut))
WRAP_HR(VarR8FromUI4, (ULONG ulIn, DOUBLE *pdblOut), (ulIn, pdblOut))
WRAP_HR(VarR8FromDec, (DECIMAL *pdecIn, DOUBLE *pdblOut), (pdecIn, pdblOut))
WRAP_HR(VarDateFromUI1, (BYTE bIn, DATE * pdateOut), (bIn, pdateOut))
WRAP_HR(VarDateFromI2, (SHORT sIn, DATE * pdateOut), (sIn, pdateOut))
WRAP_HR(VarDateFromI4, (LONG lIn, DATE * pdateOut), (lIn, pdateOut))
WRAP_HR(VarDateFromR4, (FLOAT fltIn, DATE * pdateOut), (fltIn, pdateOut))
WRAP_HR(VarDateFromR8, (DOUBLE dblIn, DATE * pdateOut), (dblIn, pdateOut))
WRAP_HR(VarDateFromCy, (CY cyIn, DATE * pdateOut), (cyIn, pdateOut))
WRAP_HR(VarDateFromStr, (OLECHAR *strIn, LCID lcid, ULONG dwFlags, DATE *pdateOut), (strIn, lcid, dwFlags, pdateOut))
WRAP_HR(VarDateFromDisp, (IDispatch * pdispIn, LCID lcid, DATE * pdateOut), (pdispIn, lcid, pdateOut))
WRAP_HR(VarDateFromBool, (VARIANT_BOOL boolIn, DATE * pdateOut), (boolIn, pdateOut))
WRAP_HR(VarDateFromI1, (CHAR cIn, DATE *pdateOut), (cIn, pdateOut))
WRAP_HR(VarDateFromUI2, (USHORT uiIn, DATE *pdateOut), (uiIn, pdateOut))
WRAP_HR(VarDateFromUI4, (ULONG ulIn, DATE *pdateOut), (ulIn, pdateOut))
WRAP_HR(VarDateFromDec, (DECIMAL *pdecIn, DATE *pdateOut), (pdecIn, pdateOut))
WRAP_HR(VarCyFromUI1, (BYTE bIn, CY * pcyOut), (bIn, pcyOut))
WRAP_HR(VarCyFromI2, (SHORT sIn, CY * pcyOut), (sIn, pcyOut))
WRAP_HR(VarCyFromI4, (LONG lIn, CY * pcyOut), (lIn, pcyOut))
WRAP_HR(VarCyFromR4, (FLOAT fltIn, CY * pcyOut), (fltIn, pcyOut))
WRAP_HR(VarCyFromR8, (DOUBLE dblIn, CY * pcyOut), (dblIn, pcyOut))
WRAP_HR(VarCyFromDate, (DATE dateIn, CY * pcyOut), (dateIn, pcyOut))
WRAP_HR(VarCyFromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, CY * pcyOut), (strIn, lcid, dwFlags, pcyOut))
WRAP_HR(VarCyFromDisp, (IDispatch * pdispIn, LCID lcid, CY * pcyOut), (pdispIn, lcid, pcyOut))
WRAP_HR(VarCyFromBool, (VARIANT_BOOL boolIn, CY * pcyOut), (boolIn, pcyOut))
WRAP_HR(VarCyFromI1, (CHAR cIn, CY *pcyOut), (cIn, pcyOut))
WRAP_HR(VarCyFromUI2, (USHORT uiIn, CY *pcyOut), (uiIn, pcyOut))
WRAP_HR(VarCyFromUI4, (ULONG ulIn, CY *pcyOut), (ulIn, pcyOut))
WRAP_HR(VarCyFromDec, (DECIMAL *pdecIn, CY *pcyOut), (pdecIn, pcyOut))
WRAP_HR(VarBstrFromUI1, (BYTE bVal, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (bVal, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromI2, (SHORT iVal, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (iVal, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromI4, (LONG lIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (lIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromR4, (FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (fltIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromR8, (DOUBLE dblIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (dblIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromCy, (CY cyIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (cyIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromDate, (DATE dateIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (dateIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromDisp, (IDispatch * pdispIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (pdispIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromBool, (VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR * pbstrOut), (boolIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromI1, (CHAR cIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut), (cIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromUI2, (USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut), (uiIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromUI4, (ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut), (ulIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBstrFromDec, (DECIMAL *pdecIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut), (pdecIn, lcid, dwFlags, pbstrOut))
WRAP_HR(VarBoolFromUI1, (BYTE bIn, VARIANT_BOOL * pboolOut), (bIn, pboolOut))
WRAP_HR(VarBoolFromI2, (SHORT sIn, VARIANT_BOOL * pboolOut), (sIn, pboolOut))
WRAP_HR(VarBoolFromI4, (LONG lIn, VARIANT_BOOL * pboolOut), (lIn, pboolOut))
WRAP_HR(VarBoolFromR4, (FLOAT fltIn, VARIANT_BOOL * pboolOut), (fltIn, pboolOut))
WRAP_HR(VarBoolFromR8, (DOUBLE dblIn, VARIANT_BOOL * pboolOut), (dblIn, pboolOut))
WRAP_HR(VarBoolFromDate, (DATE dateIn, VARIANT_BOOL * pboolOut), (dateIn, pboolOut))
WRAP_HR(VarBoolFromCy, (CY cyIn, VARIANT_BOOL * pboolOut), (cyIn, pboolOut))
WRAP_HR(VarBoolFromStr, (OLECHAR * strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL * pboolOut), (strIn, lcid, dwFlags, pboolOut))
WRAP_HR(VarBoolFromDisp, (IDispatch * pdispIn, LCID lcid, VARIANT_BOOL * pboolOut), (pdispIn, lcid, pboolOut))
WRAP_HR(VarBoolFromI1, (CHAR cIn, VARIANT_BOOL *pboolOut), (cIn, pboolOut))
WRAP_HR(VarBoolFromUI2, (USHORT uiIn, VARIANT_BOOL *pboolOut), (uiIn, pboolOut))
WRAP_HR(VarBoolFromUI4, (ULONG ulIn, VARIANT_BOOL *pboolOut), (ulIn, pboolOut))
WRAP_HR(VarBoolFromDec, (DECIMAL *pdecIn, VARIANT_BOOL *pboolOut), (pdecIn, pboolOut))
WRAP_HR(VarI1FromUI1, (BYTE bIn, CHAR *pcOut), (bIn, pcOut))
WRAP_HR(VarI1FromI2, (SHORT uiIn, CHAR *pcOut), (uiIn, pcOut))
WRAP_HR(VarI1FromI4, (LONG lIn, CHAR *pcOut), (lIn, pcOut))
WRAP_HR(VarI1FromR4, (FLOAT fltIn, CHAR *pcOut), (fltIn, pcOut))
WRAP_HR(VarI1FromR8, (DOUBLE dblIn, CHAR *pcOut), (dblIn, pcOut))
WRAP_HR(VarI1FromDate, (DATE dateIn, CHAR *pcOut), (dateIn, pcOut))
WRAP_HR(VarI1FromCy, (CY cyIn, CHAR *pcOut), (cyIn, pcOut))
WRAP_HR(VarI1FromStr, (OLECHAR *strIn, LCID lcid, ULONG dwFlags, CHAR *pcOut), (strIn, lcid, dwFlags, pcOut))
WRAP_HR(VarI1FromDisp, (IDispatch *pdispIn, LCID lcid, CHAR *pcOut), (pdispIn, lcid, pcOut))
WRAP_HR(VarI1FromBool, (VARIANT_BOOL boolIn, CHAR *pcOut), (boolIn, pcOut))
WRAP_HR(VarI1FromUI2, (USHORT uiIn, CHAR *pcOut), (uiIn, pcOut))
WRAP_HR(VarI1FromUI4, (ULONG ulIn, CHAR *pcOut), (ulIn, pcOut))
WRAP_HR(VarI1FromDec, (DECIMAL *pdecIn, CHAR *pcOut), (pdecIn, pcOut))
WRAP_HR(VarUI2FromUI1, (BYTE bIn, USHORT *puiOut), (bIn, puiOut))
WRAP_HR(VarUI2FromI2, (SHORT uiIn, USHORT *puiOut), (uiIn, puiOut))
WRAP_HR(VarUI2FromI4, (LONG lIn, USHORT *puiOut), (lIn, puiOut))
WRAP_HR(VarUI2FromR4, (FLOAT fltIn, USHORT *puiOut), (fltIn, puiOut))
WRAP_HR(VarUI2FromR8, (DOUBLE dblIn, USHORT *puiOut), (dblIn, puiOut))
WRAP_HR(VarUI2FromDate, (DATE dateIn, USHORT *puiOut), (dateIn, puiOut))
WRAP_HR(VarUI2FromCy, (CY cyIn, USHORT *puiOut), (cyIn, puiOut))
WRAP_HR(VarUI2FromStr, (OLECHAR *strIn, LCID lcid, ULONG dwFlags, USHORT *puiOut), (strIn, lcid, dwFlags, puiOut))
WRAP_HR(VarUI2FromDisp, (IDispatch *pdispIn, LCID lcid, USHORT *puiOut), (pdispIn, lcid, puiOut))
WRAP_HR(VarUI2FromBool, (VARIANT_BOOL boolIn, USHORT *puiOut), (boolIn, puiOut))
WRAP_HR(VarUI2FromI1, (CHAR cIn, USHORT *puiOut), (cIn, puiOut))
WRAP_HR(VarUI2FromUI4, (ULONG ulIn, USHORT *puiOut), (ulIn, puiOut))
WRAP_HR(VarUI2FromDec, (DECIMAL *pdecIn, USHORT *puiOut), (pdecIn, puiOut))
WRAP_HR(VarUI4FromUI1, (BYTE bIn, ULONG *pulOut), (bIn, pulOut))
WRAP_HR(VarUI4FromI2, (SHORT uiIn, ULONG *pulOut), (uiIn, pulOut))
WRAP_HR(VarUI4FromI4, (LONG lIn, ULONG *pulOut), (lIn, pulOut))
WRAP_HR(VarUI4FromR4, (FLOAT fltIn, ULONG *pulOut), (fltIn, pulOut))
WRAP_HR(VarUI4FromR8, (DOUBLE dblIn, ULONG *pulOut), (dblIn, pulOut))
WRAP_HR(VarUI4FromDate, (DATE dateIn, ULONG *pulOut), (dateIn, pulOut))
WRAP_HR(VarUI4FromCy, (CY cyIn, ULONG *pulOut), (cyIn, pulOut))
WRAP_HR(VarUI4FromStr, (OLECHAR *strIn, LCID lcid, ULONG dwFlags, ULONG *pulOut), (strIn, lcid, dwFlags, pulOut))
WRAP_HR(VarUI4FromDisp, (IDispatch *pdispIn, LCID lcid, ULONG *pulOut), (pdispIn, lcid, pulOut))
WRAP_HR(VarUI4FromBool, (VARIANT_BOOL boolIn, ULONG *pulOut), (boolIn, pulOut))
WRAP_HR(VarUI4FromI1, (CHAR cIn, ULONG *pulOut), (cIn, pulOut))
WRAP_HR(VarUI4FromUI2, (USHORT uiIn, ULONG *pulOut), (uiIn, pulOut))
WRAP_HR(VarUI4FromDec, (DECIMAL *pdecIn, ULONG *pulOut), (pdecIn, pulOut))
WRAP_HR(VarFormatCurrency, 
    (LPVARIANT pvarIn, int iNumDig, int iIncLead, int iUseParens, int iGroup, ULONG dwFlags, BSTR *pbstrOut),
    (pvarIn, iNumDig, iIncLead, iUseParens, iGroup, dwFlags, pbstrOut))
