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

#define COBJMACROS

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <initguid.h>
#include <wtypes.h>
#include <olectl.h>
#include <ocidl.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static WCHAR MSSansSerif_font[] = {'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0};
static WCHAR system_font[] = { 'S','y','s','t','e','m',0 };
static WCHAR arial_font[] = { 'A','r','i','a','l',0 };
static WCHAR marlett_font[] = { 'M','a','r','l','e','t','t',0 };

static HMODULE hOleaut32;

static HRESULT (WINAPI *pOleCreateFontIndirect)(LPFONTDESC,REFIID,LPVOID*);

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08lx, expected 0x%08lx\n", hr, hr_exp)

/* Create a font with cySize given by lo_size, hi_size,  */
/* SetRatio to ratio_logical, ratio_himetric,            */
/* check that resulting hfont has height hfont_height.   */
/* Various checks along the way.                         */
static void test_ifont_size(LONGLONG size, LONG ratio_logical, LONG ratio_himetric,
                            LONG hfont_height, const char * test_name)
{
	FONTDESC fd;
	LPVOID pvObj = NULL;
	IFont* ifnt = NULL;
	HFONT hfont;
	LOGFONTA lf;
	CY psize;
	HRESULT hres;
        DWORD rtnval;

	fd.cbSizeofstruct = sizeof(FONTDESC);
	fd.lpstrName      = arial_font; /* using scalable instead of bitmap font reduces errors due to font realization */
	fd.cySize.int64   = size;
	fd.sWeight        = 0;
	fd.sCharset       = 0;
        fd.fItalic        = FALSE;
        fd.fUnderline     = FALSE;
        fd.fStrikethrough = FALSE;

	/* Create font, test that it worked. */
	hres = pOleCreateFontIndirect(&fd, &IID_IFont, &pvObj);
	ifnt = pvObj;
	ok(hres == S_OK,"%s: OCFI returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	ok(pvObj != NULL,"%s: OCFI returns NULL.\n", test_name);

        /* Change the scaling ratio */
        hres = IFont_SetRatio(ifnt, ratio_logical, ratio_himetric);
        ok((ratio_logical && ratio_himetric) ? hres == S_OK : hres == E_FAIL,
           "%s: IFont_SetRatio unexpectedly returned 0x%08lx.\n", test_name, hres);

	/* Read back size. */
	hres = IFont_get_Size(ifnt, &psize);
	ok(hres == S_OK,"%s: IFont_get_size returns 0x%08lx instead of S_OK.\n",
		test_name, hres);

        /* Check returned size - allow for errors due to rounding & font realization. */
	ok((psize.int64 - size) < 10000 && (psize.int64 - size) > -10000,
		"%s: IFont_get_Size: Lo=%ld, Hi=%ld; expected Lo=%ld, Hi=%ld.\n",
		test_name, psize.Lo, psize.Hi, fd.cySize.Lo, fd.cySize.Hi);

	/* Check hFont size. */
	hres = IFont_get_hFont (ifnt, &hfont);
	ok(hres == S_OK, "%s: IFont_get_hFont returns 0x%08lx instead of S_OK.\n",
		test_name, hres);
	rtnval = GetObjectA(hfont, sizeof(LOGFONTA), &lf);
        ok(rtnval > 0, "GetObject(hfont) failed\n");

        /* Since font scaling may encounter rounding errors, allow 1 pixel deviation. */
	ok(abs(lf.lfHeight - hfont_height) <= 1,
		"%s: hFont has lf.lfHeight=%ld, expected %ld.\n",
		test_name, lf.lfHeight, hfont_height);

	/* Free IFont. */
	IFont_Release(ifnt);
}

static void test_ifont_sizes(void)
{
  /* Test various size operations and conversions. */
  /* Add more as needed. */

  /* Results of first 2 tests depend on display resolution. */
  HDC hdc = GetDC(0);
  LONG dpi = GetDeviceCaps(hdc, LOGPIXELSY); /* expected results depend on display DPI */
  ReleaseDC(0, hdc);
  if(dpi == 96) /* normal resolution display */
  {
    test_ifont_size(180000, 0, 0, -24, "default");     /* normal font */
    test_ifont_size(186000, 0, 0, -25, "rounding");    /* test rounding */
  } else if(dpi == 72) /* low resolution display */
  {
    test_ifont_size(180000, 0, 0, -18, "default");     /* normal font */
    test_ifont_size(186000, 0, 0, -19, "rounding");    /* test rounding */
  } else if(dpi == 120) /* high resolution display */
  {
    test_ifont_size(180000, 0, 0, -30, "default");     /* normal font */
    test_ifont_size(186000, 0, 0, -31, "rounding");    /* test rounding */
  } else
    skip("Skipping resolution dependent font size tests - display resolution is %ld\n", dpi);

  /* Next 4 tests specify a scaling ratio, so display resolution is not a factor. */
    test_ifont_size(180000, 72,  2540, -18, "ratio1");  /* change ratio */
    test_ifont_size(180000, 144, 2540, -36, "ratio2");  /* another ratio */
    test_ifont_size(180000, 72,  1270, -36, "ratio3");  /* yet another ratio */
    test_ifont_size(186000, 72,  2540, -19, "rounding+ratio"); /* test rounding with ratio */

    /* test various combinations of logical == himetric */
    test_ifont_size(180000, 10, 10, -635, "identical ratio 1");
    test_ifont_size(240000, 10, 10, -848, "identical ratio 2");
    test_ifont_size(300000, 10, 10, -1058, "identical ratio 3");

    /* test various combinations of logical and himetric both set to 1 */
    test_ifont_size(180000, 1, 1, -24, "1:1 ratio 1");
    test_ifont_size(240000, 1, 1, -32, "1:1 ratio 2");
    test_ifont_size(300000, 1, 1, -40, "1:1 ratio 3");

    /* test various combinations of logical set to 1 */
    test_ifont_size(180000, 1, 0, -24, "1:0 ratio 1");
    test_ifont_size(240000, 1, 0, -32, "1:0 ratio 2");
    test_ifont_size(300000, 1, 0, -40, "1:0 ratio 3");

    /* test various combinations of himetric set to 1 */
    test_ifont_size(180000, 0, 1, -24, "0:1 ratio 1");
    test_ifont_size(240000, 0, 1, -32, "0:1 ratio 2");
    test_ifont_size(300000, 0, 1, -40, "0:1 ratio 3");

    /* test various combinations of 2:1 logical:himetric */
    test_ifont_size(180000, 2, 1, -1270, "2:1 ratio 1");
    test_ifont_size(240000, 2, 1, -1694, "2:1 ratio 2");
    test_ifont_size(300000, 2, 1, -2117, "2:1 ratio 3");

    /* test various combinations of 1:2 logical:himetric */
    test_ifont_size(180000, 1, 2, -318, "1:2 ratio 1");
    test_ifont_size(240000, 1, 2, -424, "1:2 ratio 2");
    test_ifont_size(300000, 1, 2, -529, "1:2 ratio 3");

    /* test various combinations of logical and himetric both set to 2 */
    test_ifont_size(180000, 2, 2, -635, "2:2 ratio 1");
    test_ifont_size(240000, 2, 2, -848, "2:2 ratio 2");
    test_ifont_size(300000, 2, 2, -1058, "2:2 ratio 3");
}

static void test_interfaces(void)
{
    HRESULT hr;
    IFont *font = NULL;

    hr = pOleCreateFontIndirect(NULL, &IID_IFont, NULL);
    EXPECT_HR(hr, E_POINTER);

    hr = pOleCreateFontIndirect(NULL, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);
    ok(font != NULL,"OCFI (NULL,..) returns NULL, instead of !NULL\n");

    check_interface(font, &IID_IFont, TRUE);
    check_interface(font, &IID_IFontDisp, TRUE);
    check_interface(font, &IID_IDispatch, TRUE);
    check_interface(font, &IID_IPersist, TRUE);
    check_interface(font, &IID_IPersistStream, TRUE);
    check_interface(font, &IID_IConnectionPointContainer, TRUE);
    check_interface(font, &IID_IPersistPropertyBag, TRUE);
    check_interface(font, &IID_IPersistStreamInit, FALSE);

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
	ok(hres == S_OK, "GTI returned 0x%08lx instead of S_OK.\n", hres);
	ok(pTInfo != NULL, "GTI returned NULL.\n");

	hres = ITypeInfo_GetNames(pTInfo, DISPID_FONT_NAME, names, 3, &n);
	ok(hres == S_OK, "GetNames returned 0x%08lx instead of S_OK.\n", hres);
	ok(n == 1, "GetNames returned %d names instead of 1.\n", n);
	ok(!lstrcmpiW(names[0],name_Name), "DISPID_FONT_NAME doesn't get 'Names'.\n");
	SysFreeString(names[0]);

	ITypeInfo_Release(pTInfo);

        dispparams.cNamedArgs = 0;
        dispparams.rgdispidNamedArgs = NULL;
        dispparams.cArgs = 0;
        dispparams.rgvarg = NULL;
        VariantInit(&varresult);
        hres = IFontDisp_Invoke(fontdisp, DISPID_FONT_NAME, &IID_NULL,
            LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult,
            NULL, NULL);
        ok(hres == S_OK, "IFontDisp_Invoke return 0x%08lx instead of S_OK.\n", hres);
        VariantClear(&varresult);

	IFontDisp_Release(fontdisp);
}

static HRESULT WINAPI FontEventsDisp_QueryInterface(IFontEventsDisp *iface, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IFontEventsDisp) || IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
    {
        IFontEventsDisp_AddRef(iface);
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

static HRESULT WINAPI FontEventsDisp_GetTypeInfoCount(IFontEventsDisp *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FontEventsDisp_GetTypeInfo(IFontEventsDisp *iface, UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI FontEventsDisp_GetIDsOfNames(IFontEventsDisp *iface, REFIID riid, LPOLESTR *names, UINT cNames, LCID lcid,
    DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static int fonteventsdisp_invoke_called;
static BSTR fonteventsdisp_invoke_arg0;

static HRESULT WINAPI FontEventsDisp_Invoke(
        IFontEventsDisp *iface,
        DISPID dispid,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    VARIANTARG *arg0 = &pDispParams->rgvarg[0];

    ok(dispid == DISPID_FONT_CHANGED, "expected DISPID_FONT_CHANGED instead of 0x%lx\n", dispid);
    ok(IsEqualGUID(riid, &GUID_NULL), "got riid %s\n", wine_dbgstr_guid(riid));
    ok(wFlags == INVOKE_FUNC, "expected INVOKE_FUNC instead of 0x%x\n", wFlags);
    ok(pDispParams->cArgs == 1, "expected arg count 1, got %d\n", pDispParams->cArgs);
    ok(V_VT(arg0) == VT_BSTR, "expected VT_BSTR, got %d\n", V_VT(arg0));

    fonteventsdisp_invoke_arg0 = SysAllocString(V_BSTR(arg0));
    fonteventsdisp_invoke_called++;
    return S_OK;
}

static IFontEventsDispVtbl FontEventsDisp_Vtbl =
{
    FontEventsDisp_QueryInterface,
    FontEventsDisp_AddRef,
    FontEventsDisp_Release,
    FontEventsDisp_GetTypeInfoCount,
    FontEventsDisp_GetTypeInfo,
    FontEventsDisp_GetIDsOfNames,
    FontEventsDisp_Invoke
};

static IFontEventsDisp FontEventsDisp = { &FontEventsDisp_Vtbl };

    struct font_dispid
    {
        DISPID dispid;
        const WCHAR *name;
    };

static void test_font_events_disp(void)
{
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR sizeW[] = {'S','i','z','e',0};
    static const WCHAR boldW[] = {'B','o','l','d',0};
    static const WCHAR italicW[] = {'I','t','a','l','i','c',0};
    static const WCHAR underlineW[] = {'U','n','d','e','r','l','i','n','e',0};
    static const WCHAR strikeW[] = {'S','t','r','i','k','e','t','h','r','o','u','g','h',0};
    static const WCHAR weightW[] = {'W','e','i','g','h','t',0};
    static const WCHAR charsetW[] = {'C','h','a','r','s','e','t',0};

    static const struct font_dispid font_dispids[] =
    {
        { DISPID_FONT_NAME, nameW },
        { DISPID_FONT_SIZE, sizeW },
        { DISPID_FONT_BOLD, boldW },
        { DISPID_FONT_ITALIC, italicW },
        { DISPID_FONT_UNDER, underlineW },
        { DISPID_FONT_STRIKE, strikeW },
        { DISPID_FONT_WEIGHT, weightW },
        { DISPID_FONT_CHARSET, charsetW }
    };

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
    INT i;

    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = MSSansSerif_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = 0;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&pFont);
    EXPECT_HR(hr, S_OK);

    hr = IFont_QueryInterface(pFont, &IID_IConnectionPointContainer, (void **)&pCPC);
    EXPECT_HR(hr, S_OK);

    hr = IConnectionPointContainer_FindConnectionPoint(pCPC, &IID_IFontEventsDisp, &pCP);
    EXPECT_HR(hr, S_OK);
    IConnectionPointContainer_Release(pCPC);

    hr = IConnectionPoint_Advise(pCP, (IUnknown *)&FontEventsDisp, &dwCookie);
    EXPECT_HR(hr, S_OK);
    IConnectionPoint_Release(pCP);

    fonteventsdisp_invoke_called = 0;
    fonteventsdisp_invoke_arg0 = NULL;
    hr = IFont_put_Bold(pFont, TRUE);
    EXPECT_HR(hr, S_OK);

    ok(fonteventsdisp_invoke_called == 1, "IFontEventDisp::Invoke wasn't called once\n");
    SysFreeString(fonteventsdisp_invoke_arg0);

    hr = IFont_QueryInterface(pFont, &IID_IFontDisp, (void **)&pFontDisp);
    EXPECT_HR(hr, S_OK);

    for (i = 0; i < ARRAY_SIZE(font_dispids); i++)
    {
        switch (font_dispids[i].dispid)
        {
        case DISPID_FONT_NAME:
        {
            static const WCHAR arialW[] = {'A','r','i','a','l',0};
            V_VT(&vararg) = VT_BSTR;
            V_BSTR(&vararg) = SysAllocString(arialW);
            break;
        }
        case DISPID_FONT_SIZE:
            V_VT(&vararg) = VT_CY;
            V_CY(&vararg).Lo = 25;
            V_CY(&vararg).Hi = 0;
            break;
        case DISPID_FONT_BOLD:
            V_VT(&vararg) = VT_BOOL;
            V_BOOL(&vararg) = VARIANT_FALSE;
            break;
        case DISPID_FONT_ITALIC:
        case DISPID_FONT_UNDER:
        case DISPID_FONT_STRIKE:
            V_VT(&vararg) = VT_BOOL;
            V_BOOL(&vararg) = VARIANT_TRUE;
            break;
        case DISPID_FONT_WEIGHT:
            V_VT(&vararg) = VT_I2;
            V_I2(&vararg) = FW_BLACK;
            break;
        case DISPID_FONT_CHARSET:
            V_VT(&vararg) = VT_I2;
            V_I2(&vararg) = 1;
            break;
        default:
            ;
        }

        dispparams.cNamedArgs = 0;
        dispparams.rgdispidNamedArgs = NULL;
        dispparams.cArgs = 1;
        dispparams.rgvarg = &vararg;
        fonteventsdisp_invoke_called = 0;
        hr = IFontDisp_Invoke(pFontDisp, font_dispids[i].dispid, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
        ok(hr == S_OK, "dispid=%ld, got 0x%08lx\n", font_dispids[i].dispid, hr);
        ok(fonteventsdisp_invoke_called == 1, "dispid=%ld, DISPID_FONT_CHANGED not called, got %d\n", font_dispids[i].dispid,
            fonteventsdisp_invoke_called);
        if (hr == S_OK)
        {
            ok(!lstrcmpW(font_dispids[i].name, fonteventsdisp_invoke_arg0), "dispid=%ld, got %s, expected %s\n",
                font_dispids[i].dispid, wine_dbgstr_w(fonteventsdisp_invoke_arg0), wine_dbgstr_w(font_dispids[i].name));
            SysFreeString(fonteventsdisp_invoke_arg0);
        }
        VariantClear(&vararg);
    }

    IFontDisp_Release(pFontDisp);

    hr = IFont_Clone(pFont, &pFont2);
    EXPECT_HR(hr, S_OK);
    IFont_Release(pFont);

    /* this test shows that the notification routine isn't called again */
    fonteventsdisp_invoke_called = 0;
    hr = IFont_put_Bold(pFont2, FALSE);
    EXPECT_HR(hr, S_OK);
    ok(fonteventsdisp_invoke_called == 0, "got %d\n", fonteventsdisp_invoke_called);

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
        "GetIDsOfNames: \"%s\", \"%s\" returns 0x%08lx, expected 0x%08lx.\n",
        a_name_1, a_name_2, hres, hres_expect);

    /* test first DISPID */
    ok(rgDispId[0]==id_1,
        "GetIDsOfNames: \"%s\" gets DISPID 0x%08lx, expected 0x%08lx.\n",
        a_name_1, rgDispId[0], id_1);

    /* test second DISPID is present */
    if (numnames == 2)
    {
        ok(rgDispId[1]==id_2,
            "GetIDsOfNames: ..., \"%s\" gets DISPID 0x%08lx, expected 0x%08lx.\n",
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
    EXPECT_HR(hr, S_OK);

    V_VT(&vararg) = VT_BOOL;
    V_BOOL(&vararg) = VARIANT_FALSE;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_IFontDisp, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    EXPECT_HR(hr, DISP_E_UNKNOWNINTERFACE);

    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    EXPECT_HR(hr, DISP_E_BADPARAMCOUNT);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYPUT, NULL, NULL, NULL, NULL);
    EXPECT_HR(hr, DISP_E_PARAMNOTOPTIONAL);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    EXPECT_HR(hr, DISP_E_PARAMNOTOPTIONAL);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    EXPECT_HR(hr, S_OK);

    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_METHOD, NULL, &varresult, NULL, NULL);
    EXPECT_HR(hr, DISP_E_MEMBERNOTFOUND);

    hr = IFontDisp_Invoke(fontdisp, 0xdeadbeef, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    EXPECT_HR(hr, DISP_E_MEMBERNOTFOUND);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IFontDisp_Invoke(fontdisp, DISPID_FONT_BOLD, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    EXPECT_HR(hr, S_OK);

    IFontDisp_Release(fontdisp);
}

static void test_IsEqual(void)
{
    FONTDESC fd;
    IFont* ifnt = NULL;
    IFont* ifnt2 = NULL;
    HRESULT hres;

    /* Basic font description */
    fd.cbSizeofstruct = sizeof(FONTDESC);
    fd.lpstrName      = system_font;
    fd.cySize.Lo      = 100;
    fd.cySize.Hi      = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = FALSE;
    fd.fUnderline     = FALSE;
    fd.fStrikethrough = FALSE;

    /* Create font */
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt);

    /* Test equal fonts */
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_OK,
        "IFont_IsEqual: (EQUAL) Expected S_OK but got 0x%08lx\n",hres);
    IFont_Release(ifnt2);

    /* Check for bad pointer */
    hres = IFont_IsEqual(ifnt,NULL);
    ok(hres == E_POINTER,
        "IFont_IsEqual: (NULL) Expected 0x80004003 but got 0x%08lx\n",hres);

    /* Test strName */
    fd.lpstrName = arial_font;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (strName) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.lpstrName = system_font;
    IFont_Release(ifnt2);

    /* Test lo font size */
    fd.cySize.Lo = 10000;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Lo font size) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.cySize.Lo = 100;
    IFont_Release(ifnt2);

    /* Test hi font size */
    fd.cySize.Hi = 10000;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Hi font size) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.cySize.Hi = 100;
    IFont_Release(ifnt2);

    /* Test font weight  */
    fd.sWeight = 100;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Weight) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.sWeight = 0;
    IFont_Release(ifnt2);

    /* Test charset */
    fd.sCharset = 1;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Charset) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.sCharset = 0;
    IFont_Release(ifnt2);

    /* Test italic setting */
    fd.fItalic = TRUE;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Italic) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.fItalic = FALSE;
    IFont_Release(ifnt2);

    /* Test underline setting */
    fd.fUnderline = TRUE;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Underline) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.fUnderline = FALSE;
    IFont_Release(ifnt2);

    /* Test strikethrough setting */
    fd.fStrikethrough = TRUE;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    hres = IFont_IsEqual(ifnt,ifnt2);
    ok(hres == S_FALSE,
        "IFont_IsEqual: (Strikethrough) Expected S_FALSE but got 0x%08lx\n",hres);
    fd.fStrikethrough = FALSE;
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
    fd.cySize.Lo      = 100;
    fd.cySize.Hi      = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = FALSE;
    fd.fUnderline     = FALSE;
    fd.fStrikethrough = FALSE;

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
        "IFont_ReleaseHfont: (Bad HFONT) Expected E_INVALIDARG but got 0x%08lx\n",
        hres);

    /* Try to add a bad HFONT */
    hres = IFont_ReleaseHfont(ifnt1,(HFONT)32);
    ok(hres == S_FALSE,
        "IFont_ReleaseHfont: (Bad HFONT) Expected S_FALSE but got 0x%08lx\n",
        hres);

    /* Release all refs */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Check that both lists are empty */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08lx\n",
        hres);

    /* The list should be empty */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08lx\n",
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
    fd.cySize.Lo      = 100;
    fd.cySize.Hi      = 100;
    fd.sWeight        = 0;
    fd.sCharset       = 0;
    fd.fItalic        = FALSE;
    fd.fUnderline     = FALSE;
    fd.fStrikethrough = FALSE;

    /* Create HFONTs and IFONTs */
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt1);
    IFont_get_hFont(ifnt1,&hfnt1);
    fd.lpstrName = arial_font;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt2);
    IFont_get_hFont(ifnt2,&hfnt2);

    /* Try invalid HFONT */
    hres = IFont_AddRefHfont(ifnt1,NULL);
    ok(hres == E_INVALIDARG,
        "IFont_AddRefHfont: (Bad HFONT) Expected E_INVALIDARG but got 0x%08lx\n",
        hres);

    /* Try to add a bad HFONT */
    hres = IFont_AddRefHfont(ifnt1,(HFONT)32);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Bad HFONT) Expected S_FALSE but got 0x%08lx\n",
        hres);

    /* Add simple IFONT HFONT pair */
    hres = IFont_AddRefHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* IFONT and HFONT do not have to be the same (always looks at HFONT) */
    hres = IFont_AddRefHfont(ifnt2,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Release all hfnt1 refs */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Check if hfnt1 is empty */
    hres = IFont_ReleaseHfont(ifnt1,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08lx\n",
        hres);

    /* Release all hfnt2 refs */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Check if hfnt2 is empty */
    hres = IFont_ReleaseHfont(ifnt2,hfnt2);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_FALSE but got 0x%08lx\n",
        hres);

    /* Show that releasing an IFONT does not always release it from the HFONT cache. */

    IFont_Release(ifnt1);

    /* Add a reference for destroyed hfnt1 */
    hres = IFont_AddRefHfont(ifnt2,hfnt1);
    ok(hres == S_OK,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Decrement reference for destroyed hfnt1 */
    hres = IFont_ReleaseHfont(ifnt2,hfnt1);
    ok(hres == S_OK ||
       hres == S_FALSE, /* <= win2k */
        "IFont_AddRefHfont: (Release ref) Expected S_OK or S_FALSE but got 0x%08lx\n",
        hres);

    /* Shows that releasing all IFONT's does clear the HFONT cache. */

    IFont_Release(ifnt2);

    /* Need to make a new IFONT for testing */
    fd.fUnderline = TRUE;
    pOleCreateFontIndirect(&fd, &IID_IFont, (void **)&ifnt3);
    IFont_get_hFont(ifnt3,&hfnt3);

    /* Add a reference for destroyed hfnt1 */
    hres = IFont_AddRefHfont(ifnt3,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Add ref) Expected S_OK but got 0x%08lx\n",
        hres);

    /* Decrement reference for destroyed hfnt1 */
    hres = IFont_ReleaseHfont(ifnt3,hfnt1);
    ok(hres == S_FALSE,
        "IFont_AddRefHfont: (Release ref) Expected S_OK but got 0x%08lx\n",
        hres);

    IFont_Release(ifnt3);
}

static void test_returns(void)
{
    IFont *pFont;
    FONTDESC fontdesc;
    HRESULT hr;

    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = MSSansSerif_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = 0;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&pFont);
    EXPECT_HR(hr, S_OK);

    hr = IFont_put_Name(pFont, NULL);
    EXPECT_HR(hr, CTL_E_INVALIDPROPERTYVALUE);

    hr = IFont_get_Name(pFont, NULL);
    EXPECT_HR(hr, E_POINTER);

    hr = IFont_get_Size(pFont, NULL);
    EXPECT_HR(hr, E_POINTER);

    hr = IFont_get_Bold(pFont, NULL);
    EXPECT_HR(hr, E_POINTER);

    IFont_Release(pFont);
}

static void test_hfont_lifetime(void)
{
    IFont *font, *font2;
    FONTDESC fontdesc;
    HRESULT hr;
    HFONT hfont, first_hfont = NULL;
    CY size;
    DWORD obj_type;
    int i;

    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = arial_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = ANSI_CHARSET;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);

    hr = IFont_get_hFont(font, &hfont);
    EXPECT_HR(hr, S_OK);

    /* show that if the font is updated the old hfont is deleted when the
       new font is realized */
    for(i = 0; i < 100; i++)
    {
        HFONT last_hfont = hfont;

        size.int64 = (i + 10) * 20000;

        obj_type = GetObjectType(hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_put_Size(font, size);
        EXPECT_HR(hr, S_OK);

        /* put_Size doesn't cause the new font to be realized */
        obj_type = GetObjectType(last_hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_get_hFont(font, &hfont);
        EXPECT_HR(hr, S_OK);
    }

    /* now show that if we take a reference on the hfont, it persists
       until the font object is released */
    for(i = 0; i < 100; i++)
    {
        size.int64 = (i + 10) * 20000;

        obj_type = GetObjectType(hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_put_Size(font, size);
        EXPECT_HR(hr, S_OK);

        hr = IFont_get_hFont(font, &hfont);
        EXPECT_HR(hr, S_OK);

        hr = IFont_AddRefHfont(font, hfont);
        EXPECT_HR(hr, S_OK);

        if(i == 0) first_hfont = hfont;
        obj_type = GetObjectType(first_hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);
    }

    IFont_Release(font);

    /* An AddRefHfont followed by a ReleaseHfont means the font doesn't not persist
       through re-realization */

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);

    hr = IFont_get_hFont(font, &hfont);
    EXPECT_HR(hr, S_OK);

    for(i = 0; i < 100; i++)
    {
        HFONT last_hfont = hfont;

        size.int64 = (i + 10) * 20000;

        obj_type = GetObjectType(hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_put_Size(font, size);
        EXPECT_HR(hr, S_OK);

        /* put_Size doesn't cause the new font to be realized */
        obj_type = GetObjectType(last_hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_get_hFont(font, &hfont);
        EXPECT_HR(hr, S_OK);

        hr = IFont_AddRefHfont(font, hfont);
        EXPECT_HR(hr, S_OK);

        hr = IFont_ReleaseHfont(font, hfont);
        EXPECT_HR(hr, S_OK);
    }

    /* Interestingly if we release a nonexistent reference on the hfont,
     * it persists until the font object is released
     */
    for(i = 0; i < 100; i++)
    {
        size.int64 = (i + 10) * 20000;

        obj_type = GetObjectType(hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

        hr = IFont_put_Size(font, size);
        EXPECT_HR(hr, S_OK);

        hr = IFont_get_hFont(font, &hfont);
        EXPECT_HR(hr, S_OK);

        hr = IFont_ReleaseHfont(font, hfont);
        EXPECT_HR(hr, S_OK);

        if(i == 0) first_hfont = hfont;
        obj_type = GetObjectType(first_hfont);
        ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);
    }

    IFont_Release(font);

    /* If we take two internal references on a hfont then we can release
       it twice.  So it looks like there's a total reference count
       that includes internal and external references */

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font2);
    EXPECT_HR(hr, S_OK);

    hr = IFont_get_hFont(font, &hfont);
    EXPECT_HR(hr, S_OK);
    hr = IFont_get_hFont(font2, &first_hfont);
    EXPECT_HR(hr, S_OK);
    todo_wine
    ok(hfont == first_hfont, "fonts differ\n");
    hr = IFont_ReleaseHfont(font, hfont);
    EXPECT_HR(hr, S_OK);
    hr = IFont_ReleaseHfont(font, hfont);
    todo_wine
    EXPECT_HR(hr, S_OK);
    hr = IFont_ReleaseHfont(font, hfont);
    EXPECT_HR(hr, S_FALSE);

    obj_type = GetObjectType(hfont);
    ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

    IFont_Release(font);

    obj_type = GetObjectType(hfont);
    ok(obj_type == OBJ_FONT, "got obj type %ld\n", obj_type);

    IFont_Release(font2);
}

static void test_realization(void)
{
    IFont *font;
    FONTDESC fontdesc;
    HRESULT hr;
    BSTR name;
    SHORT cs;

    /* Try to create a symbol only font (marlett) with charset
       set to ANSI.  This will result in another, ANSI, font
       being selected */
    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = marlett_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = ANSI_CHARSET;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);

    hr = IFont_get_Charset(font, &cs);
    EXPECT_HR(hr, S_OK);
    ok(cs == ANSI_CHARSET, "got charset %d\n", cs);

    IFont_Release(font);

    /* Now create an ANSI font and change the name to marlett */

    fontdesc.lpstrName = arial_font;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void **)&font);
    EXPECT_HR(hr, S_OK);

    hr = IFont_get_Charset(font, &cs);
    EXPECT_HR(hr, S_OK);
    ok(cs == ANSI_CHARSET, "got charset %d\n", cs);

    name = SysAllocString(marlett_font);
    hr = IFont_put_Name(font, name);
    EXPECT_HR(hr, S_OK);
    SysFreeString(name);

    hr = IFont_get_Name(font, &name);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpiW(name, marlett_font), "got name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);

    hr = IFont_get_Charset(font, &cs);
    EXPECT_HR(hr, S_OK);
    ok(cs == SYMBOL_CHARSET, "got charset %d\n", cs);

    IFont_Release(font);
}

static void test_OleCreateFontIndirect(void)
{
    FONTDESC fontdesc;
    IUnknown *unk, *unk2;
    IFont *font;
    HRESULT hr;
    WCHAR str_empty[] = {0};

    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = arial_font;
    fontdesc.cySize.int64 = 12 * 10000; /* 12 pt */
    fontdesc.sWeight = FW_NORMAL;
    fontdesc.sCharset = ANSI_CHARSET;
    fontdesc.fItalic = FALSE;
    fontdesc.fUnderline = FALSE;
    fontdesc.fStrikethrough = FALSE;

    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, S_OK);
    IFont_Release(font);

    /* play with cbSizeofstruct value */
    fontdesc.cbSizeofstruct = sizeof(fontdesc)-1;
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, S_OK);
    IFont_Release(font);

    fontdesc.cbSizeofstruct = sizeof(fontdesc)+1;
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, S_OK);
    IFont_Release(font);

    fontdesc.cbSizeofstruct = 0;
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, S_OK);
    IFont_Release(font);

    /* Test NULL name */
    fontdesc.cbSizeofstruct = sizeof(fontdesc);
    fontdesc.lpstrName = NULL;
    font = (IFont*)0xdeadbeef;
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, CTL_E_INVALIDPROPERTYVALUE);
    ok(font == 0, "Got %p\n", font);

    /* Test empty Name */
    fontdesc.lpstrName = str_empty;
    font = NULL;
    hr = pOleCreateFontIndirect(&fontdesc, &IID_IFont, (void**)&font);
    EXPECT_HR(hr, S_OK);
    ok(font != 0, "Got NULL font\n");
    IFont_Release(font);

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoGetClassObject(&CLSID_StdFont, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&unk);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void**)&unk2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IUnknown_Release(unk);
    IUnknown_Release(unk2);

    OleUninitialize();
}

START_TEST(olefont)
{
    hOleaut32 = GetModuleHandleA("oleaut32.dll");
    pOleCreateFontIndirect = (void*)GetProcAddress(hOleaut32, "OleCreateFontIndirect");
    if (!pOleCreateFontIndirect)
    {
        win_skip("OleCreateFontIndirect not available\n");
        return;
    }

    test_interfaces();
    test_type_info();
    test_ifont_sizes();
    test_font_events_disp();
    test_GetIDsOfNames();
    test_Invoke();
    test_IsEqual();
    test_ReleaseHfont();
    test_AddRefHfont();
    test_returns();
    test_hfont_lifetime();
    test_realization();
    test_OleCreateFontIndirect();
}
