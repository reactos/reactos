/*
 * OLEFONT test program
 *
 * Copyright 2003 Marcus Meissner
 * Copyright 2006 (Google) Benjamin Arai
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
#include <math.h>
#include <float.h>
#include <time.h>

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

static WCHAR MSSansSerif_font[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0};
static WCHAR system_font[] = { 'S','y','s','t','e','m',0 };
static WCHAR arial_font[] = { 'A','r','i','a','l',0 };

static HMODULE hOleaut32;

static HRESULT (WINAPI *pOleCreateFontIndirect)(LPFONTDESC,REFIID,LPVOID*);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

/* Create a font with cySize given by lo_size, hi_size,  */
/* SetRatio to ratio_logical, ratio_himetric,            */
/* check that resulting hfont has height hfont_height.   */
/* Various checks along the way.                         */

static void test_ifont_sizes(long lo_size, long hi_size, 
	long ratio_logical, long ratio_himetric,
	long hfont_height, const char * test_name)
{
	FONTDESC fd;
	LPVOID pvObj = NULL;
	IFont* ifnt = NULL;
	HFONT hfont;
	LOGFONT lf;
	CY psize;
	HRESULT hres;

	fd.cbSizeofstruct = sizeof(FONTDESC);
	fd.lpstrName      = system_font;
	S(fd.cySize).Lo   = lo_size;
	S(fd.cySize).Hi   = hi_size;
	fd.sWeight        = 0;
	fd.sCharset       = 0;
	fd.fItalic        = 0;
	fd.fUnderline     = 0;
	fd.fStrikethrough = 0;

	/* Create font, test that it worked. */
	hres = pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj);
	ifnt = pvObj;
	ok(hres == S_OK,"%s: OCFI returns 0x%08x instead of S_OK.\n",
		test_name, hres);
	ok(pvObj != NULL,"%s: OCFI returns NULL.\n", test_name);

	/* Read back size.  Hi part was ignored. */
	hres = IFont_get_Size(ifnt, &psize);
	ok(hres == S_OK,"%s: IFont_get_size returns 0x%08x instead of S_OK.\n",
		test_name, hres);
	ok(S(psize).Lo == lo_size && S(psize).Hi == 0,
		"%s: get_Size: Lo=%d, Hi=%d; expected Lo=%ld, Hi=%ld.\n",
		test_name, S(psize).Lo, S(psize).Hi, lo_size, 0L);

	/* Change ratio, check size unchanged.  Standard is 72, 2540. */
	hres = IFont_SetRatio(ifnt, ratio_logical, ratio_himetric);
	ok(hres == S_OK,"%s: IFont_SR returns 0x%08x instead of S_OK.\n",
		test_name, hres);
	hres = IFont_get_Size(ifnt, &psize);
	ok(hres == S_OK,"%s: IFont_get_size returns 0x%08x instead of S_OK.\n",
                test_name, hres);
	ok(S(psize).Lo == lo_size && S(psize).Hi == 0,
		"%s: gS after SR: Lo=%d, Hi=%d; expected Lo=%ld, Hi=%ld.\n",
		test_name, S(psize).Lo, S(psize).Hi, lo_size, 0L);

	/* Check hFont size with this ratio.  This tests an important 	*/
	/* conversion for which MSDN is very wrong.			*/
	hres = IFont_get_hFont (ifnt, &hfont);
	ok(hres == S_OK, "%s: IFont_get_hFont returns 0x%08x instead of S_OK.\n",
		test_name, hres);
	hres = GetObject (hfont, sizeof(LOGFONT), &lf);
	ok(lf.lfHeight == hfont_height,
		"%s: hFont has lf.lfHeight=%d, expected %ld.\n",
		test_name, lf.lfHeight, hfont_height);

	/* Free IFont. */
	IFont_Release(ifnt);
}

static void test_QueryInterface(void)
{
        LPVOID pvObj = NULL;
        HRESULT hres;
        IFont*  font = NULL;
        LONG ret;

        hres = pOleCreateFontIndirect(NULL, &IID_IFont, &pvObj);
        font = pvObj;

        ok(hres == S_OK,"OCFI (NULL,..) does not return 0, but 0x%08x\n",hres);
        ok(font != NULL,"OCFI (NULL,..) returns NULL, instead of !NULL\n");

        pvObj = NULL;
        hres = IFont_QueryInterface( font, &IID_IFont, &pvObj);

        /* Test if QueryInterface increments ref counter for IFONTs */
        ret = IFont_AddRef(font);
        ok(ret == 3, "IFont_QI expected ref value 3 but instead got %12u\n",ret);
        IFont_Release(font);

        ok(hres == S_OK,"IFont_QI does not return S_OK, but 0x%08x\n", hres);
        ok(pvObj != NULL,"IFont_QI does return NULL, instead of a ptr\n");

        /* Orignial ref and QueryInterface ref both have to be released */
        IFont_Release(font);
        IFont_Release(font);
}

static void test_type_info(void)
{
        LPVOID pvObj = NULL;
        HRESULT hres;
        IFontDisp*  fontdisp = NULL;
	ITypeInfo* pTInfo;
	WCHAR name_Name[] = {'N','a','m','e',0};
	BSTR names[3];
	UINT n;
        LCID en_us = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
                SORT_DEFAULT);
        DISPPARAMS dispparams;
        VARIANT varresult;

        pOleCreateFontIndirect(NULL, &IID_IFontDisp, &pvObj);
        fontdisp = pvObj;

	hres = IFontDisp_GetTypeInfo(fontdisp, 0, en_us, &pTInfo);
	ok(hres == S_OK, "GTI returned 0x%08x instead of S_OK.\n", hres);
	ok(pTInfo != NULL, "GTI returned NULL.\n");

	hres = ITypeInfo_GetNames(pTInfo, DISPID_FONT_NAME, names, 3, &n);
	ok(hres == S_OK, "GetNames returned 0x%08x instead of S_OK.\n", hres);
	ok(n == 1, "GetNames returned %d names instead of 1.\n", n);
	ok(!lstrcmpiW(names[0],name_Name), "DISPID_FONT_NAME doesn't get 'Names'.\n");

	ITypeInfo_Release(pTInfo);

        dispparams.cNamedArgs = 0;
        dispparams.rgdispidNamedArgs = NULL;
        dispparams.cArgs = 0;
        dispparams.rgvarg = NULL;
        VariantInit(&varresult);
        hres = IFontDisp_Invoke(fontdisp, DISPID_FONT_NAME, &IID_NULL,
            LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult,
            NULL, NULL);
        ok(hres == S_OK, "IFontDisp_Invoke return 0x%08x instead of S_OK.\n", hres);
        VariantClear(&varresult);

	IFontDisp_Release(fontdisp);
}

static HRESULT WINAPI FontEventsDisp_QueryInterface(
        IFontEventsDisp *iface,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (IsEqualIID(riid, &IID_IFontEventsDisp) || IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
    {
        IUnknown_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI FontEventsDisp_AddRef(
    IFontEventsDisp *iface)
{
    return 2;
}

static ULONG WINAPI FontEventsDisp_Release(
        IFontEventsDisp *iface)
{
    return 1;
}

static int fonteventsdisp_invoke_called = 0;

static HRESULT WINAPI FontEventsDisp_Invoke(
        IFontEventsDisp __RPC_FAR * iface,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    static const WCHAR wszBold[] = {'B','o','l','d',0};
    ok(wFlags == INVOKE_FUNC, "invoke flags should have been INVOKE_FUNC instead of 0x%x\n", wFlags);
    ok(dispIdMember == DISPID_FONT_CHANGED, "dispIdMember should have been DISPID_FONT_CHANGED instead of 0x%x\n", dispIdMember);
    ok(pDispParams->cArgs == 1, "pDispParams->cArgs should have been 1 instead of %d\n", pDispParams->cArgs);
    ok(V_VT(&pDispParams->rgvarg[0]) == VT_BSTR, "VT of first param should have been VT_BSTR instead of %d\n", V_VT(&pDispParams->rgvarg[0]));
    ok(!lstrcmpW(V_BSTR(&pDispParams->rgvarg[0]), wszBold), "String in first param should have been \"Bold\"\n");

    fonteventsdisp_invoke_called++;
    return S_OK;
}

static IFontEventsDispVtbl FontEventsDisp_Vtbl =
{
    FontEventsDisp_QueryInterface,
    FontEventsDisp_AddRef,
    FontEventsDisp_Release,
    NULL,
    NULL,
    NULL,
    FontEventsDisp_Invoke
};

static IFontEventsDisp FontEventsDisp = { &FontEventsDisp_Vtbl };

static void test_font_events_disp(void)
{
    IFont *pFont;
    IFont *pFont2;
    IConnectionPointContainer *pCPC;
    IConnectionPoint *pCP;
    FONTDESC fontdesc;
    HRESULT hr;
    DWORD dwCookie;
    IFontDisp *pFontDisp;
    DISPPARAMS dispparams;
    VARIANTARG vararg;

    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = MSSansSerif_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = 0;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&pFont);
    ok_ole_success(hr, "OleCreateFontIndirect");

    hr = IFont_QueryInterface(pFont, &IID_IConnectionPointContainer, (void **)&pCPC);
    ok_ole_success(hr, "IFont_QueryInterface");

    hr = IConnectionPointContainer_FindConnectionPoint(pCPC, &IID_IFontEventsDisp, &pCP);
    ok_ole_success(hr, "IConnectionPointContainer_FindConnectionPoint");
    IConnectionPointContainer_Release(pCPC);

    hr = IConnectionPoint_Advise(pCP, (IUnknown *)&FontEventsDisp, &dwCookie);
    ok_ole_success(hr, "IConnectionPoint_Advise");
    IConnectionPoint_Release(pCP);

    hr = IFont_put_Bold(pFont, TRUE);
    ok_ole_success(hr, "IFont_put_Bold");

    ok(fonteventsdisp_invoke_called == 1, "IFontEventDisp::Invoke wasn't called once\n");

    hr = IFont_QueryInterface(pFont, &IID_IFontDisp, (void **)&pFontDisp);
    ok_ole_success(hr, "IFont_QueryInterface");

    V_VT(&vararg) = VT_BOOL;
    V_BOOL(&vararg) = VARIANT_FALSE;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IFontDisp_Invoke(pFontDisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);

    IFontDisp_Release(pFontDisp);

    ok(fonteventsdisp_invoke_called == 2, "IFontEventDisp::Invoke was called %d times instead of twice\n",
        fonteventsdisp_invoke_called);

    hr = IFont_Clone(pFont, &pFont2);
    ok_ole_success(hr, "IFont_Clone");
    IFont_Release(pFont);

    hr = IFont_put_Bold(pFont2, FALSE);
    ok_ole_success(hr, "IFont_put_Bold");

    /* this test shows that the notification routine isn't called again */
    ok(fonteventsdisp_invoke_called == 2, "IFontEventDisp::Invoke was called %d times instead of twice\n",
        fonteventsdisp_invoke_called);

    IFont_Release(pFont2);
}

static void test_names_ids(WCHAR* w_name_1, const char* a_name_1,
                    WCHAR* w_name_2, const char* a_name_2,
                    LCID lcid, DISPID id_1, DISPID id_2,
                    HRESULT hres_expect, int numnames)
{
    LPVOID pvObj = NULL;
    IFontDisp *fontdisp = NULL;
    HRESULT hres;
    DISPID rgDispId[2] = {0xdeadbeef, 0xdeadbeef};
    LPOLESTR names[2] = {w_name_1, w_name_2};

    pOleCreateFontIndirect(NULL, &IID_IFontDisp, &pvObj);
    fontdisp = pvObj;

    hres = IFontDisp_GetIDsOfNames(fontdisp, &IID_NULL, names, numnames,
                                   lcid, rgDispId);

    /* test hres */
    ok(hres == hres_expect,
        "GetIDsOfNames: \"%s\", \"%s\" returns 0x%08x, expected 0x%08x.\n",
        a_name_1, a_name_2, hres, hres_expect);

    /* test first DISPID */
    ok(rgDispId[0]==id_1,
        "GetIDsOfNames: \"%s\" gets DISPID 0x%08x, expected 0x%08x.\n",
        a_name_1, rgDispId[0], id_1);

    /* test second DISPID is present */
    if (numnames == 2)
    {
        ok(rgDispId[1]==id_2,
            "GetIDsOfNames: ..., \"%s\" gets DISPID 0x%08x, expected 0x%08x.\n",
            a_name_2, rgDispId[1], id_2);
    }

   IFontDisp_Release(fontdisp);
}

static void test_GetIDsOfNames(void)
{
    WCHAR name_Name[] = {'N','a','m','e',0};
    WCHAR name_Italic[] = {'I','t','a','l','i','c',0};
    WCHAR name_Size[] = {'S','i','z','e',0};
    WCHAR name_Bold[] = {'B','o','l','d',0};
    WCHAR name_Underline[] = {'U','n','d','e','r','l','i','n','e',0};
    WCHAR name_Strikethrough[] = {'S','t','r','i','k','e','t','h','r','o','u','g','h',0};
    WCHAR name_Weight[] = {'W','e','i','g','h','t',0};
    WCHAR name_Charset[] = {'C','h','a','r','s','e','t',0};
    WCHAR name_Foo[] = {'F','o','o',0};
    WCHAR name_nAmE[] = {'n','A','m','E',0};
    WCHAR name_Nom[] = {'N','o','m',0};

    LCID en_us = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
                          SORT_DEFAULT);
    LCID fr_fr = MAKELCID(MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH),
                          SORT_DEFAULT);

    /* Test DISPID_FONTs for the various properties. */
    test_names_ids(name_Name, "Name", NULL, "", en_us,
                   DISPID_FONT_NAME, 0, S_OK,1);
    test_names_ids(name_Size, "Size", NULL, "", en_us,
                   DISPID_FONT_SIZE, 0, S_OK,1);
    test_names_ids(name_Bold, "Bold", NULL, "", en_us,
                   DISPID_FONT_BOLD, 0, S_OK,1);
    test_names_ids(name_Italic, "Italic", NULL, "", en_us,
                   DISPID_FONT_ITALIC, 0, S_OK,1);
    test_names_ids(name_Underline, "Underline", NULL, "", en_us,
                   DISPID_FONT_UNDER, 0, S_OK,1);
    test_names_ids(name_Strikethrough, "Strikethrough", NULL, "", en_us,
                   DISPID_FONT_STRIKE, 0, S_OK,1);
    test_names_ids(name_Weight, "Weight", NULL, "", en_us,
                   DISPID_FONT_WEIGHT, 0, S_OK,1);
    test_names_ids(name_Charset, "Charset", NULL, "", en_us,
                   DISPID_FONT_CHARSET, 0, S_OK,1);

    /* Capitalization doesn't matter. */
    test_names_ids(name_nAmE, "nAmE", NULL, "", en_us,
                   DISPID_FONT_NAME, 0, S_OK,1);

    /* Unknown name. */
    test_names_ids(name_Foo, "Foo", NULL, "", en_us,
                   DISPID_UNKNOWN, 0, DISP_E_UNKNOWNNAME,1);

    /* Pass several names: first is processed,                */
    /* second gets DISPID_UNKNOWN and doesn't affect retval.  */
    test_names_ids(name_Italic, "Italic", name_Name, "Name", en_us,
                   DISPID_FONT_ITALIC, DISPID_UNKNOWN, S_OK,2);
    test_names_ids(name_Italic, "Italic", name_Foo, "Foo", en_us,
                   DISPID_FONT_ITALIC, DISPID_UNKNOWN, S_OK,2);

    /* Locale ID has no effect. */
    test_names_ids(name_Name, "Name", NULL, "", fr_fr,
                   DISPID_FONT_NAME, 0, S_OK,1);
    test_names_ids(name_Nom, "This is not a font", NULL, "", fr_fr,
                   DISPID_UNKNOWN, 0, DISP_E_UNKNOWNNAME,1);

    /* One of the arguments are invalid */
    test_names_ids(name_Name, "Name", NULL, "", en_us,
                   0xdeadbeef, 0xdeadbeef, E_INVALIDARG,0);
    test_names_ids(name_Italic, "Italic", NULL, "", en_us,
                   0xdeadbeef, 0xdeadbeef, E_INVALIDARG,0);
    test_names_ids(name_Foo, "Foo", NULL, "", en_us,
                   0xdeadbeef, 0xdeadbeef, E_INVALIDARG,0);

    /* Crazy locale ID? */
    test_names_ids(name_Name, "Name", NULL, "", -1,
                   DISPID_FONT_NAME, 0, S_OK,1);
}

static void test_Invoke(void)
{
    IFontDisp *fontdisp;
    HRESULT hr;
    VARIANTARG vararg;
    DISPPARAMS dispparams;
    VARIANT varresult;

    hr = pOleCreateFontIndirect(NULL, &IID_IFontDisp, (void **)&fontdisp);
    ok_ole_success(hr, "OleCreateFontIndirect");

    V_VT(&vararg) = VT_BOOL;
    V_BOOL(&vararg) = VARIANT_FALSE;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_IFontDisp, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNINTERFACE, "IFontDisp_Invoke should have returned DISP_E_UNKNOWNINTERFACE instead of 0x%08x\n", hr);

    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IFontDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYPUT, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IFontDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08x\n", hr);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IFontDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08x\n", hr);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    ok_ole_success(hr, "IFontDisp_Invoke");

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_METHOD, NULL, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IFontDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    hr = IFontDisp_Invoke(fontdisp, 0xdeadbeef, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IFontDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08x\n", hr);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok_ole_success(hr, "IFontDisp_Invoke");

    IFontDisp_Release(fontdisp);
}

static void test_IsEqual(void)
{
    FONTDESC fd;
    LPVOID pvObj = NULL;
    LPVOID pvObj2 = NULL;
    IFont* ifnt = NULL;
    IFont* ifnt2 = NULL;
    HRESULT hres;

    /* Basic font description */
    fd.cbSizeofstruct = sizeof(FONTDESC);
    fd.lpstrName      = system_font;
    S(fd.cySize).Lo   = 100;
    S(fd.cySize).Hi   = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = 0;
    fd.fUnderline     = 0;
    fd.fStrikethrough = 0;

    /* Create font */
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj);
    ifnt = pvObj;

    /* Test equal fonts */
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    ifnt2 = pvObj2;
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_OK,
        "IFont_IsEqual: (EQUAL) Expected S_OK but got 0x%08x\n",hres);
    IFont_Release(ifnt2);

    /* Check for bad pointer */
    hres = IFont_IsEqual(ifnt,NULL);
    ok(hres == E_POINTER,
        "IFont_IsEqual: (NULL) Expected 0x80004003 but got 0x%08x\n",hres);

    /* Test strName */
    fd.lpstrName = arial_font;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (strName) Expected S_FALSE but got 0x%08x\n",hres);
    fd.lpstrName = system_font;
    IFont_Release(ifnt2);

    /* Test lo font size */
    S(fd.cySize).Lo = 10000;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    ifnt2 = pvObj2;
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Lo font size) Expected S_FALSE but got 0x%08x\n",hres);
    S(fd.cySize).Lo = 100;
    IFont_Release(ifnt2);

    /* Test hi font size */
    S(fd.cySize).Hi = 10000;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    ifnt2 = pvObj2;
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Hi font size) Expected S_FALSE but got 0x%08x\n",hres);
    S(fd.cySize).Hi = 100;
    IFont_Release(ifnt2);

    /* Test font weight  */
    fd.sWeight = 100;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    ifnt2 = pvObj2;
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Weight) Expected S_FALSE but got 0x%08x\n",hres);
    fd.sWeight = 0;
    IFont_Release(ifnt2);

    /* Test charset */
    fd.sCharset = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Charset) Expected S_FALSE but got 0x%08x\n",hres);
    fd.sCharset = 0;
    IFont_Release(ifnt2);

    /* Test italic setting */
    fd.fItalic = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Italic) Expected S_FALSE but got 0x%08x\n",hres);
    fd.fItalic = 0;
    IFont_Release(ifnt2);

    /* Test underline setting */
    fd.fUnderline = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Underline) Expected S_FALSE but got 0x%08x\n",hres);
    fd.fUnderline = 0;
    IFont_Release(ifnt2);

    /* Test strikethrough setting */
    fd.fStrikethrough = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Strikethrough) Expected S_FALSE but got 0x%08x\n",hres);
    fd.fStrikethrough = 0;
    IFont_Release(ifnt2);

    /* Free IFont. */
    IFont_Release(ifnt);
}

static void test_ReleaseHfont(void)
{
    FONTDESC fd;
    LPVOID pvObj1 = NULL;
    LPVOID pvObj2 = NULL;
    IFont* ifnt1 = NULL;
    IFont* ifnt2 = NULL;
    HFONT hfnt1 = 0;
    HFONT hfnt2 = 0;
    HRESULT hres;

    /* Basic font description */
    fd.cbSizeofstruct = sizeof(FONTDESC);
    fd.lpstrName      = system_font;
    S(fd.cySize).Lo   = 100;
    S(fd.cySize).Hi   = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = 0;
    fd.fUnderline     = 0;
    fd.fStrikethrough = 0;

    /* Create HFONTs and IFONTs */
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj1);
    ifnt1 = pvObj1;
    IFont_get_hFont(ifnt1,&hfnt1);
    fd.lpstrName = arial_font;
    pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj2);
    ifnt2 = pvObj2;
    IFont_get_hFont(ifnt2,&hfnt2);

    /* Try invalid HFONT */
    hres = IFont_ReleaseHfont(ifnt1,NULL);
    ok(hres == E_INVALIDARG,
        "IFont_ReleaseHfont: (Bad HFONT) Expected E_INVALIDARG but got 0x%08x\n",
        hres);

    /* Try to add a bad HFONT */
    hres = IFont_ReleaseHfont(ifnt1,(HFONT)32);
    ok(hres == S_FALSE,
        "IFont_ReleaseHfont: (Bad HFONT) Expected S_FALSE but got 0x%08x\n",
        hres);

    /* Release all refs */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Check that both lists are empty */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08x\n",
        hres);

    /* The list should be empty */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08x\n",
        hres);

    IFont_Release(ifnt1);
    IFont_Release(ifnt2);
}

static void test_AddRefHfont(void)
{
    FONTDESC fd;
    IFont* ifnt1 = NULL;
    IFont* ifnt2 = NULL;
    IFont* ifnt3 = NULL;
    HFONT hfnt1 = 0;
    HFONT hfnt2 = 0;
    HFONT hfnt3 = 0;
    HRESULT hres;

    /* Basic font description */
    fd.cbSizeofstruct = sizeof(FONTDESC);
    fd.lpstrName      = system_font;
    S(fd.cySize).Lo   = 100;
    S(fd.cySize).Hi   = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = 0;
    fd.fUnderline     = 0;
    fd.fStrikethrough = 0;

    /* Create HFONTs and IFONTs */
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt1);
    IFont_get_hFont(ifnt1,&hfnt1);
    fd.lpstrName = arial_font;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    IFont_get_hFont(ifnt2,&hfnt2);

    /* Try invalid HFONT */
    hres = IFont_AddRefHfont(ifnt1,NULL);
    ok(hres == E_INVALIDARG,
        "IFont_AddRefHfont: (Bad HFONT) Expected E_INVALIDARG but got 0x%08x\n",
        hres);

    /* Try to add a bad HFONT */
    hres = IFont_AddRefHfont(ifnt1,(HFONT)32);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Bad HFONT) Expected S_FALSE but got 0x%08x\n",
        hres);

    /* Add simple IFONT HFONT pair */
    hres = IFont_AddRefHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* IFONT and HFONT do not have to be the same (always looks at HFONT) */
    hres = IFont_AddRefHfont(ifnt2,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Release all hfnt1 refs */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Check if hfnt1 is empty */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08x\n",
        hres);

    /* Release all hfnt2 refs */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Check if hfnt2 is empty */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08x\n",
        hres);

    /* Show that releasing an IFONT does not always release it from the HFONT cache. */

    IFont_Release(ifnt1);

    /* Add a reference for destroyed hfnt1 */
    hres = IFont_AddRefHfont(ifnt2,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Decrement reference for destroyed hfnt1 */
    hres = IFont_ReleaseHfont(ifnt2,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Shows that releasing all IFONT's does clear the HFONT cache. */

    IFont_Release(ifnt2);

    /* Need to make a new IFONT for testing */
    fd.fUnderline = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt3);
    IFont_get_hFont(ifnt3,&hfnt3);

    /* Add a reference for destroyed hfnt1 */
    hres = IFont_AddRefHfont(ifnt3,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08x\n",
        hres);

    /* Decrement reference for destroyed hfnt1 */
    hres = IFont_ReleaseHfont(ifnt3,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08x\n",
        hres);

    IFont_Release(ifnt3);
}

START_TEST(olefont)
{
	hOleaut32 = GetModuleHandleA("oleaut32.dll");
	pOleCreateFontIndirect = (void*)GetProcAddress(hOleaut32, "OleCreateFontIndirect");
	if (!pOleCreateFontIndirect)
	{
	    skip("OleCreateFontIndirect not available\n");
	    return;
	}

	test_QueryInterface();
	test_type_info();

	/* Test various size operations and conversions. */
	/* Add more as needed. */
	test_ifont_sizes(180000, 0, 72, 2540, -18, "default");
	test_ifont_sizes(180000, 0, 144, 2540, -36, "ratio1");		/* change ratio */
	test_ifont_sizes(180000, 0, 72, 1270, -36, "ratio2");		/* 2nd part of ratio */

	/* These depend on details of how IFont rounds sizes internally. */
	test_ifont_sizes(0, 0, 72, 2540, 0, "zero size");          /* zero size */
	test_ifont_sizes(186000, 0, 72, 2540, -19, "rounding");   /* test rounding */

	test_font_events_disp();
	test_GetIDsOfNames();
	test_Invoke();
	test_IsEqual();
	test_ReleaseHfont();
	test_AddRefHfont();
}
