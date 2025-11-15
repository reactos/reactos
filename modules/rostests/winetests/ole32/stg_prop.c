/* IPropertyStorage unit tests
 * Copyright 2005 Juan Lang
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
#include <stdio.h>
#define COBJMACROS
#include "objbase.h"
#include "wine/test.h"

#ifndef PID_BEHAVIOR
#define PID_BEHAVIOR 0x80000003
#endif

/* FIXME: this creates an ANSI storage, try to find conditions under which
 * Unicode translation fails
 */
static void testPropsHelper(IPropertySetStorage **propSetStorage)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szSummaryInfo[] = { 5,'S','u','m','m','a','r','y',
        'I','n','f','o','r','m','a','t','i','o','n',0 };
    static WCHAR propName[] = { 'p','r','o','p',0 };
    static char val[] = "l33t auth0r";
    WCHAR filename[MAX_PATH];
    HRESULT hr;
    IStorage *storage = NULL;
    IStream *stream = NULL;
    IPropertyStorage *propertyStorage = NULL;
    PROPSPEC spec;
    PROPVARIANT var;
    CLIPDATA clipdata;
    unsigned char clipcontent[] = "foobar";
    GUID anyOldGuid = { 0x12345678,0xdead,0xbeef, {
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07 } };

    if(propSetStorage)
        trace("Testing property storage with a set...\n");
    else
        trace("Testing property storage without a set...\n");

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    hr = StgCreateDocfile(filename,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "StgCreateDocfile failed: 0x%08lx\n", hr);

    if(propSetStorage)
    {
        hr = StgCreatePropSetStg(storage, 0, propSetStorage);
        ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

        hr = IPropertySetStorage_Create(*propSetStorage,
         &FMTID_SummaryInformation, NULL, PROPSETFLAG_ANSI,
         STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
         &propertyStorage);
        ok(hr == S_OK, "IPropertySetStorage_Create failed: 0x%08lx\n", hr);
    }
    else
    {
        hr = IStorage_CreateStream(storage, szSummaryInfo,
         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
        ok(hr == S_OK, "IStorage_CreateStream failed: 0x%08lx\n", hr);

        hr = StgCreatePropStg((IUnknown *)stream, &FMTID_SummaryInformation,
         NULL, PROPSETFLAG_ANSI, 0, &propertyStorage);
        ok(hr == S_OK, "StgCreatePropStg failed: 0x%08lx\n", hr);
    }

    hr = IPropertyStorage_WriteMultiple(propertyStorage, 0, NULL, NULL, 0);
    ok(hr == S_OK, "WriteMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);

    /* test setting one that I can't set */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_DICTIONARY;
    var.vt = VT_I4;
    var.lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* test setting one by name with an invalid propidNameFirst */
    spec.ulKind = PRSPEC_LPWSTR;
    spec.lpwstr = propName;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var,
     PID_DICTIONARY);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* test setting behavior (case-sensitive) */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_BEHAVIOR;
    var.lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* set one by value.. */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    var.lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);

    /* set one by name */
    spec.ulKind = PRSPEC_LPWSTR;
    spec.lpwstr = propName;
    var.lVal = 2;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var,
     PID_FIRST_USABLE);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);

    /* set a string value */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PIDSI_AUTHOR;
    var.vt = VT_LPSTR;
    var.pszVal = val;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);

    /* set a clipboard value */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PIDSI_THUMBNAIL;
    var.vt = VT_CF;
    clipdata.cbSize = sizeof clipcontent + sizeof (ULONG);
    clipdata.ulClipFmt = CF_ENHMETAFILE;
    clipdata.pClipData = clipcontent;
    var.pclipdata = &clipdata;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);


    /* check reading */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 0, NULL, NULL);
    ok(hr == S_FALSE, "ReadMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* read by propid */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && var.lVal == 1,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, var.lVal);
    /* read by name */
    spec.ulKind = PRSPEC_LPWSTR;
    spec.lpwstr = propName;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && var.lVal == 2,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, var.lVal);
    /* read string value */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PIDSI_AUTHOR;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !lstrcmpA(var.pszVal, val),
     "Didn't get expected type or value for property (got type %d, value %s)\n",
     var.vt, var.pszVal);
    PropVariantClear(&var);

    /* read clipboard format */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PIDSI_THUMBNAIL;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_CF, "variant type wrong\n");
    ok(var.pclipdata->ulClipFmt == CF_ENHMETAFILE,
        "clipboard type wrong\n");
    ok(var.pclipdata->cbSize == sizeof clipcontent + sizeof (ULONG),
        "clipboard size wrong\n");
    ok(!memcmp(var.pclipdata->pClipData, clipcontent, sizeof clipcontent),
        "clipboard contents wrong\n");
    ok(S_OK == PropVariantClear(&var), "failed to clear variant\n");

    /* check deleting */
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 0, NULL);
    ok(hr == S_OK, "DeleteMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* contrary to what the docs say, you can't delete the dictionary */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_DICTIONARY;
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, &spec);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);
    /* now delete the first value.. */
    spec.propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, &spec);
    ok(hr == S_OK, "DeleteMultiple failed: 0x%08lx\n", hr);
    /* and check that it's no longer readable */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_FALSE, "Expected S_FALSE, got 0x%08lx\n", hr);

    hr = IPropertyStorage_Commit(propertyStorage, STGC_DEFAULT);
    ok(hr == S_OK, "Commit failed: 0x%08lx\n", hr);

    /* check reverting */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_Revert(propertyStorage);
    ok(hr == S_OK, "Revert failed: 0x%08lx\n", hr);
    /* now check that it's still not there */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_FALSE, "Expected S_FALSE, got 0x%08lx\n", hr);
    /* set an integer value again */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    var.vt = VT_I4;
    var.lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* commit it */
    hr = IPropertyStorage_Commit(propertyStorage, STGC_DEFAULT);
    ok(hr == S_OK, "Commit failed: 0x%08lx\n", hr);
    /* set it to a string value */
    var.vt = VT_LPSTR;
    var.pszVal = val;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* revert it */
    hr = IPropertyStorage_Revert(propertyStorage);
    ok(hr == S_OK, "Revert failed: 0x%08lx\n", hr);
    /* Oddly enough, there's no guarantee that a successful revert actually
     * implies the value wasn't saved.  Maybe transactional mode needs to be
     * used for that?
     */

    IPropertyStorage_Release(propertyStorage);
    if(propSetStorage) IPropertySetStorage_Release(*propSetStorage);
    IStorage_Release(storage);
    if(stream) IUnknown_Release(stream);

    /* now open it again */
    hr = StgOpenStorage(filename, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
     NULL, 0, &storage);
    ok(hr == S_OK, "StgOpenStorage failed: 0x%08lx\n", hr);

    if(propSetStorage)
    {
        hr = StgCreatePropSetStg(storage, 0, propSetStorage);
        ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

        hr = IPropertySetStorage_Open(*propSetStorage, &FMTID_SummaryInformation,
         STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &propertyStorage);
        ok(hr == S_OK, "IPropertySetStorage_Open failed: 0x%08lx\n", hr);
    }
    else
    {
        hr = IStorage_OpenStream(storage, szSummaryInfo,
         0, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stream);
        ok(hr == S_OK, "IStorage_OpenStream failed: 0x%08lx\n", hr);

        hr = StgOpenPropStg((IUnknown *)stream, &FMTID_SummaryInformation,
         PROPSETFLAG_DEFAULT, 0, &propertyStorage);
        ok(hr == S_OK, "StgOpenPropStg failed: 0x%08lx\n", hr);
    }

    /* check properties again */
    spec.ulKind = PRSPEC_LPWSTR;
    spec.lpwstr = propName;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && var.lVal == 2,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, var.lVal);
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PIDSI_AUTHOR;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !lstrcmpA(var.pszVal, val),
     "Didn't get expected type or value for property (got type %d, value %s)\n",
     var.vt, var.pszVal);
    PropVariantClear(&var);

    IPropertyStorage_Release(propertyStorage);
    if(propSetStorage) IPropertySetStorage_Release(*propSetStorage);
    IStorage_Release(storage);
    if(stream) IUnknown_Release(stream);

    DeleteFileW(filename);

    /* Test creating a property set storage with a random GUID */
    hr = StgCreateDocfile(filename,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "StgCreateDocfile failed: 0x%08lx\n", hr);

    if(propSetStorage)
    {
        hr = StgCreatePropSetStg(storage, 0, propSetStorage);
        ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

        hr = IPropertySetStorage_Create(*propSetStorage,
         &anyOldGuid, NULL, PROPSETFLAG_ANSI,
         STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
         &propertyStorage);
        ok(hr == S_OK, "IPropertySetStorage_Create failed: 0x%08lx\n", hr);
    }
    else
    {
        hr = IStorage_CreateStream(storage, szSummaryInfo,
         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
        ok(hr == S_OK, "IStorage_CreateStream failed: 0x%08lx\n", hr);

        hr = StgCreatePropStg((IUnknown *)stream, &anyOldGuid, NULL,
         PROPSETFLAG_DEFAULT, 0, &propertyStorage);
        ok(hr == S_OK, "StgCreatePropStg failed: 0x%08lx\n", hr);
    }

    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    var.vt = VT_I4;
    var.lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);

    hr = IPropertyStorage_Commit(propertyStorage, STGC_DEFAULT);
    ok(hr == S_OK, "Commit failed: 0x%08lx\n", hr);

    IPropertyStorage_Release(propertyStorage);
    if(propSetStorage) IPropertySetStorage_Release(*propSetStorage);
    IStorage_Release(storage);
    if(stream) IUnknown_Release(stream);

    /* now open it again */
    hr = StgOpenStorage(filename, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
     NULL, 0, &storage);
    ok(hr == S_OK, "StgOpenStorage failed: 0x%08lx\n", hr);

    if(propSetStorage)
    {
        hr = StgCreatePropSetStg(storage, 0, propSetStorage);
        ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

        hr = IPropertySetStorage_Open(*propSetStorage, &anyOldGuid,
         STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &propertyStorage);
        ok(hr == S_OK, "IPropertySetStorage_Open failed: 0x%08lx\n", hr);
    }
    else
    {
        hr = IStorage_OpenStream(storage, szSummaryInfo,
         0, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stream);
        ok(hr == S_OK, "IStorage_OpenStream failed: 0x%08lx\n", hr);

        hr = StgOpenPropStg((IUnknown *)stream, &anyOldGuid,
         PROPSETFLAG_DEFAULT, 0, &propertyStorage);
        ok(hr == S_OK, "StgOpenPropStg failed: 0x%08lx\n", hr);
    }

    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);

    ok(var.vt == VT_I4 && var.lVal == 1,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, var.lVal);

    IPropertyStorage_Release(propertyStorage);
    if(propSetStorage) IPropertySetStorage_Release(*propSetStorage);
    IStorage_Release(storage);
    if(stream) IUnknown_Release(stream);

    DeleteFileW(filename);
}

static void testProps(void)
{
    IPropertySetStorage *propSetStorage = NULL;

    testPropsHelper(&propSetStorage);
    testPropsHelper(NULL);
}

static void testCodepage(void)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static CHAR aval[] = "hi";
    static WCHAR wval[] = { 'h','i',0 };
    HRESULT hr;
    IStorage *storage = NULL;
    IPropertySetStorage *propSetStorage = NULL;
    IPropertyStorage *propertyStorage = NULL;
    PROPSPEC spec;
    PROPVARIANT var;
    WCHAR fileName[MAX_PATH];

    if(!GetTempFileNameW(szDot, szPrefix, 0, fileName))
        return;

    hr = StgCreateDocfile(fileName,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "StgCreateDocfile failed: 0x%08lx\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Create(propSetStorage,
     &FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT,
     STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
     &propertyStorage);
    ok(hr == S_OK, "IPropertySetStorage_Create failed: 0x%08lx\n", hr);

    PropVariantInit(&var);
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_CODEPAGE;
    /* check code page before it's been explicitly set */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && var.iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* Set the code page to ascii */
    var.vt = VT_I2;
    var.iVal = 1252;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && var.iVal == 1252,
     "Didn't get expected type or value for property\n");
    /* Set code page to Unicode */
    var.iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && var.iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* Set a string value */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    var.vt = VT_LPSTR;
    var.pszVal = aval;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(var.pszVal, "hi"),
     "Didn't get expected type or value for property\n");
    PropVariantClear(&var);
    /* This seemingly non-sensical test is to show that the string is indeed
     * interpreted according to the current system code page, not according to
     * the property set's code page.  (If the latter were true, the whole
     * string would be maintained.  As it is, only the first character is.)
     */
    var.vt = VT_LPSTR;
    var.pszVal = (LPSTR)wval;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(var.pszVal, "h"),
     "Didn't get expected type or value for property\n");
    PropVariantClear(&var);

    /* now that a property's been set, you can't change the code page */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_CODEPAGE;
    var.vt = VT_I2;
    var.iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    IPropertyStorage_Release(propertyStorage);
    IPropertySetStorage_Release(propSetStorage);
    IStorage_Release(storage);

    DeleteFileW(fileName);

    /* same tests, but with PROPSETFLAG_ANSI */
    hr = StgCreateDocfile(fileName,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "StgCreateDocfile failed: 0x%08lx\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(hr == S_OK, "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Create(propSetStorage,
     &FMTID_SummaryInformation, NULL, PROPSETFLAG_ANSI,
     STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
     &propertyStorage);
    ok(hr == S_OK, "IPropertySetStorage_Create failed: 0x%08lx\n", hr);

    /* check code page before it's been explicitly set */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2, "Didn't get expected type for property (%u)\n", var.vt);
    /* Set code page to Unicode */
    var.iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && var.iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* This test is commented out for documentation.  It fails under Wine,
     * and I expect it would under Windows as well, yet it succeeds.  There's
     * obviously something about string conversion I don't understand.
     */
    if(0) {
    static unsigned char strVal[] = { 0x81, 0xff, 0x04, 0 };
    /* Set code page to 950 (Traditional Chinese) */
    var.iVal = 950;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* Try writing an invalid string: lead byte 0x81 is unused in Traditional
     * Chinese.
     */
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = PID_FIRST_USABLE;
    var.vt = VT_LPSTR;
    var.pszVal = (LPSTR)strVal;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == S_OK, "WriteMultiple failed: 0x%08lx\n", hr);
    /* Check returned string */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_OK, "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(var.pszVal, (LPCSTR)strVal),
     "Didn't get expected type or value for property\n");
    }

    IPropertyStorage_Release(propertyStorage);
    IPropertySetStorage_Release(propSetStorage);
    IStorage_Release(storage);

    DeleteFileW(fileName);
}

static void testFmtId(void)
{
    WCHAR szSummaryInfo[] = { 5,'S','u','m','m','a','r','y',
        'I','n','f','o','r','m','a','t','i','o','n',0 };
    WCHAR szDocSummaryInfo[] = { 5,'D','o','c','u','m','e','n','t',
        'S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',
        0 };
    WCHAR szIID_IPropSetStg[] = { 5,'0','j','a','a','a','a','a',
        'a','A','a','a','a','a','a','d','a','A','a','a','a','a','a','a','a','G',
        'c',0 };
    WCHAR name[32];
    FMTID fmtid;
    HRESULT hr;

    hr = FmtIdToPropStgName(NULL, name);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = FmtIdToPropStgName(&FMTID_SummaryInformation, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = FmtIdToPropStgName(&FMTID_SummaryInformation, name);
    ok(hr == S_OK, "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szSummaryInfo, (lstrlenW(szSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_SummaryInformation\n");
    hr = FmtIdToPropStgName(&FMTID_DocSummaryInformation, name);
    ok(hr == S_OK, "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szDocSummaryInfo, (lstrlenW(szDocSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_DocSummaryInformation\n");
    hr = FmtIdToPropStgName(&FMTID_UserDefinedProperties, name);
    ok(hr == S_OK, "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szDocSummaryInfo, (lstrlenW(szDocSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_DocSummaryInformation\n");
    hr = FmtIdToPropStgName(&IID_IPropertySetStorage, name);
    ok(hr == S_OK, "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szIID_IPropSetStg, (lstrlenW(szIID_IPropSetStg) + 1) *
     sizeof(WCHAR)), "Got wrong name for IID_IPropertySetStorage\n");

    /* test args first */
    hr = PropStgNameToFmtId(NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = PropStgNameToFmtId(NULL, &fmtid);
    ok(hr == STG_E_INVALIDNAME, "Expected STG_E_INVALIDNAME, got 0x%08lx\n",
     hr);
    hr = PropStgNameToFmtId(szDocSummaryInfo, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* test the known format IDs */
    hr = PropStgNameToFmtId(szSummaryInfo, &fmtid);
    ok(hr == S_OK, "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_SummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_SummaryInformation\n");
    hr = PropStgNameToFmtId(szDocSummaryInfo, &fmtid);
    ok(hr == S_OK, "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_DocSummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_DocSummaryInformation\n");
    /* test another GUID */
    hr = PropStgNameToFmtId(szIID_IPropSetStg, &fmtid);
    ok(hr == S_OK, "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &IID_IPropertySetStorage, sizeof(fmtid)),
     "Got unexpected FMTID, expected IID_IPropertySetStorage\n");
    /* now check case matching */
    CharUpperW(szDocSummaryInfo + 1);
    hr = PropStgNameToFmtId(szDocSummaryInfo, &fmtid);
    ok(hr == S_OK, "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_DocSummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_DocSummaryInformation\n");
    CharUpperW(szIID_IPropSetStg + 1);
    hr = PropStgNameToFmtId(szIID_IPropSetStg, &fmtid);
    ok(hr == S_OK, "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &IID_IPropertySetStorage, sizeof(fmtid)),
     "Got unexpected FMTID, expected IID_IPropertySetStorage\n");
}

typedef struct
{
    WORD byte_order;
    WORD format;
    DWORD os_ver;
    CLSID clsid;
    DWORD reserved;
} PROPERTYSETHEADER;

typedef struct
{
    FMTID fmtid;
    DWORD offset;
} FORMATIDOFFSET;

typedef struct
{
    DWORD size;
    DWORD properties;
} PROPERTYSECTIONHEADER;

typedef struct
{
    DWORD propid;
    DWORD offset;
} PROPERTYIDOFFSET;

typedef struct
{
    DWORD type;
    DWORD data;
} PROPVARIANT_DWORD;

struct test_prop_data
{
    PROPERTYSETHEADER header;
    FORMATIDOFFSET doc_summary;
    FORMATIDOFFSET user_def_props;
    PROPERTYSECTIONHEADER doc_summary_header;
    PROPERTYIDOFFSET prop1;
    PROPVARIANT_DWORD prop1_val;
    PROPERTYSECTIONHEADER user_def_props_header;
    PROPERTYIDOFFSET prop2;
    PROPVARIANT_DWORD prop2_val;
} test_prop_data = {
    {
        0xfffe, 0, 0x2000a00,
        /* IID_IUnknown */
        {0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}},
        2
    },
    {
        /* FMTID_DocSummaryInformation */
        {0xd5cdd502, 0x2e9c, 0x101b, {0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae}},
        FIELD_OFFSET(struct test_prop_data, doc_summary_header)
    },
    {
        /* FMTID_UserDefinedProperties */
        {0xd5cdd505, 0x2e9c, 0x101b, {0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae}},
        FIELD_OFFSET(struct test_prop_data, user_def_props_header)
    },
    {
        sizeof(PROPERTYSECTIONHEADER) + sizeof(PROPERTYIDOFFSET) + sizeof(PROPVARIANT_DWORD),
        1
    },
    { 0x10, sizeof(PROPERTYSECTIONHEADER) + sizeof(PROPERTYIDOFFSET) },
    { VT_UI4, 11 },
    {
        sizeof(PROPERTYSECTIONHEADER) + sizeof(PROPERTYIDOFFSET) + sizeof(PROPVARIANT_DWORD),
        1
    },
    { 0x10, sizeof(PROPERTYSECTIONHEADER) + sizeof(PROPERTYIDOFFSET) },
    { VT_I4, 10 }
};

static void test_propertyset_storage_enum(void)
{
    IPropertyStorage *prop_storage, *prop_storage2;
    IPropertySetStorage *ps_storage;
    IEnumSTATPROPSETSTG *ps_enum;
    IEnumSTATPROPSTG *prop_enum;
    WCHAR filename[MAX_PATH];
    STATPROPSETSTG psstg;
    STATPROPSTG pstg;
    DWORD ret, fetched;
    IStorage *storage;
    IStream *stream;
    FILETIME ftime;
    PROPVARIANT pv;
    PROPSPEC ps;
    HRESULT hr;

    ret = GetTempFileNameW(L".", L"stg", 0, filename);
    ok(ret, "Failed to get temporary file name.\n");

    hr = StgCreateDocfile(filename, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "Failed to crate storage, hr %#lx.\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &ps_storage);
    ok(hr == S_OK, "Failed to create property set storage, hr %#lx.\n", hr);

    hr = IPropertySetStorage_Create(ps_storage, &FMTID_SummaryInformation, &IID_IUnknown, PROPSETFLAG_ANSI,
            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to create property storage, hr %#lx.\n", hr);

    hr = IPropertyStorage_Stat(prop_storage, &psstg);
    ok(hr == S_OK, "Failed to get prop storage stats, hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualCLSID(&psstg.clsid, &IID_IUnknown), "Unexpected storage clsid %s.\n", wine_dbgstr_guid(&psstg.clsid));

    hr = IPropertySetStorage_Enum(ps_storage, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IPropertySetStorage_Enum(ps_storage, &ps_enum);
    ok(hr == S_OK, "Failed to get enum object, hr %#lx.\n", hr);

    memset(&psstg, 0, sizeof(psstg));
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_OK, "Failed to get enum item, hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected fetched count.\n");
    ok(IsEqualCLSID(&psstg.fmtid, &FMTID_SummaryInformation), "Unexpected fmtid %s.\n",
            wine_dbgstr_guid(&psstg.fmtid));
    ok(psstg.mtime.dwHighDateTime == 0 && psstg.mtime.dwLowDateTime == 0, "Unexpected mtime %#lx / %#lx.\n",
            psstg.mtime.dwHighDateTime, psstg.mtime.dwLowDateTime);

    memset(&ftime, 0, sizeof(ftime));
    ftime.dwLowDateTime = 1;
    hr = IPropertyStorage_SetTimes(prop_storage, NULL, NULL, &ftime);
    todo_wine
    ok(hr == S_OK, "Failed to set storage times, hr %#lx.\n", hr);

    hr = IEnumSTATPROPSETSTG_Reset(ps_enum);
    ok(hr == S_OK, "Failed to reset enumerator, hr %#lx.\n", hr);
    memset(&psstg, 0, sizeof(psstg));
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_OK, "Failed to get enum item, hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected fetched count.\n");
    ok(IsEqualCLSID(&psstg.fmtid, &FMTID_SummaryInformation), "Unexpected fmtid %s.\n",
            wine_dbgstr_guid(&psstg.fmtid));
    ok(psstg.mtime.dwHighDateTime == 0 && psstg.mtime.dwLowDateTime == 0, "Unexpected mtime %#lx / %#lx.\n",
            psstg.mtime.dwHighDateTime, psstg.mtime.dwLowDateTime);
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IPropertySetStorage_Create(ps_storage, &FMTID_SummaryInformation, &IID_IUnknown, PROPSETFLAG_ANSI,
            STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &prop_storage2);
    ok(hr == S_OK, "Failed to create property storage, hr %#lx.\n", hr);

    hr = IEnumSTATPROPSETSTG_Reset(ps_enum);
    ok(hr == S_OK, "Failed to reset enumerator, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_OK, "Failed to get enum item, hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected fetched count.\n");
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_FALSE, "Failed to get enum item, hr %#lx.\n", hr);

    /* Skipping. */
    hr = IEnumSTATPROPSETSTG_Reset(ps_enum);
    ok(hr == S_OK, "Failed to reset enumerator, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Skip(ps_enum, 2);
    todo_wine
    ok(hr == S_FALSE, "Failed to skip, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    todo_wine
    ok(hr == S_FALSE, "Failed to get enum item, hr %#lx.\n", hr);

    hr = IEnumSTATPROPSETSTG_Reset(ps_enum);
    ok(hr == S_OK, "Failed to reset enumerator, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Skip(ps_enum, 1);
    ok(hr == S_OK, "Failed to skip, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    todo_wine
    ok(hr == S_FALSE, "Failed to get enum item, hr %#lx.\n", hr);

    hr = IEnumSTATPROPSETSTG_Reset(ps_enum);
    ok(hr == S_OK, "Failed to reset enumerator, hr %#lx.\n", hr);
todo_wine {
    hr = IEnumSTATPROPSETSTG_Skip(ps_enum, 0);
    ok(hr == S_FALSE, "Failed to skip, hr %#lx.\n", hr);
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_FALSE, "Failed to get enum item, hr %#lx.\n", hr);
}
    IEnumSTATPROPSETSTG_Release(ps_enum);

    IPropertyStorage_Release(prop_storage2);
    IPropertyStorage_Release(prop_storage);

    IPropertySetStorage_Release(ps_storage);
    IStorage_Release(storage);

    /* test FMTID_UserDefinedProperties */
    hr = StgCreateDocfile(filename, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(hr == S_OK, "Failed to crate storage, hr %#lx.\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &ps_storage);
    ok(hr == S_OK, "Failed to create property set storage, hr %#lx.\n", hr);

    hr = IPropertySetStorage_Create(ps_storage, &FMTID_UserDefinedProperties, &IID_IUnknown,
            PROPSETFLAG_ANSI, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to create property storage, hr %#lx.\n", hr);
    ps.ulKind = PRSPEC_PROPID;
    ps.propid = 0x10;
    pv.vt = VT_I4;
    pv.lVal = 10;
    hr = IPropertyStorage_WriteMultiple(prop_storage, 1, &ps, &pv, 0x10);
    ok(hr == S_OK, "Failed to add property, hr %#lx.\n", hr);
    IPropertyStorage_Release(prop_storage);

    hr = IPropertySetStorage_Open(ps_storage, &FMTID_DocSummaryInformation,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to open FMTID_DocSummaryInformation, hr %#lx.\n", hr);
    pv.vt = VT_UI4;
    pv.lVal = 11;
    hr = IPropertyStorage_WriteMultiple(prop_storage, 1, &ps, &pv, 0x10);
    ok(hr == S_OK, "Failed to add property, hr %#lx.\n", hr);
    IPropertyStorage_Release(prop_storage);

    hr = IPropertySetStorage_Enum(ps_storage, &ps_enum);
    ok(hr == S_OK, "Failed to get enum object, hr %#lx.\n", hr);
    memset(&psstg, 0, sizeof(psstg));
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_OK, "Failed to get enum item, hr %#lx.\n", hr);
    ok(fetched == 1, "Unexpected fetched count.\n");
    ok(IsEqualCLSID(&psstg.fmtid, &FMTID_DocSummaryInformation), "Unexpected fmtid %s.\n",
            wine_dbgstr_guid(&psstg.fmtid));
    memset(&psstg, 0, sizeof(psstg));
    hr = IEnumSTATPROPSETSTG_Next(ps_enum, 1, &psstg, &fetched);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    IEnumSTATPROPSETSTG_Release(ps_enum);

    hr = IPropertySetStorage_Open(ps_storage, &FMTID_DocSummaryInformation,
            STGM_READ | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to open FMTID_DocSummaryInformation, hr %#lx.\n", hr);
    hr = IPropertyStorage_Enum(prop_storage, &prop_enum);
    ok(hr == S_OK, "IPropertyStorage_Enum failed, hr %#lx.\n", hr);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_OK, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    ok(pstg.propid == 0x10, "pstg.propid = %lx\n", pstg.propid);
    ok(pstg.vt == VT_UI4, "pstg.vt = %d\n", pstg.vt);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_FALSE, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    IEnumSTATPROPSTG_Release(prop_enum);
    IPropertyStorage_Release(prop_storage);

    hr = IPropertySetStorage_Open(ps_storage, &FMTID_UserDefinedProperties,
            STGM_READ | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to open FMTID_DocSummaryInformation, hr %#lx.\n", hr);
    hr = IPropertyStorage_Enum(prop_storage, &prop_enum);
    ok(hr == S_OK, "IPropertyStorage_Enum failed, hr %#lx.\n", hr);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_OK, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    ok(pstg.propid == 0x10, "pstg.propid = %lx\n", pstg.propid);
    todo_wine ok(pstg.vt == VT_I4, "pstg.vt = %d\n", pstg.vt);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_FALSE, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    IEnumSTATPROPSTG_Release(prop_enum);
    IPropertyStorage_Release(prop_storage);

    hr = IStorage_OpenStream(storage, L"\5DocumentSummaryInformation", NULL,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stream);
    ok(hr == S_OK, "IStorage_CreateStream failed, hr %#lx.\n", hr);
    hr = IStream_Write(stream, &test_prop_data, sizeof(test_prop_data), NULL);
    ok(hr == S_OK, "IStream_Write failed, hr %#lx.\n", hr);
    IStream_Release(stream);

    hr = IPropertySetStorage_Open(ps_storage, &FMTID_DocSummaryInformation,
            STGM_READ | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to open FMTID_DocSummaryInformation, hr %#lx.\n", hr);
    hr = IPropertyStorage_Enum(prop_storage, &prop_enum);
    ok(hr == S_OK, "IPropertyStorage_Enum failed, hr %#lx.\n", hr);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_OK, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    ok(pstg.propid == 0x10, "pstg.propid = %lx\n", pstg.propid);
    ok(pstg.vt == VT_UI4, "pstg.vt = %d\n", pstg.vt);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_FALSE, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    IEnumSTATPROPSTG_Release(prop_enum);
    IPropertyStorage_Release(prop_storage);

    hr = IPropertySetStorage_Open(ps_storage, &FMTID_UserDefinedProperties,
            STGM_READ | STGM_SHARE_EXCLUSIVE, &prop_storage);
    ok(hr == S_OK, "Failed to open FMTID_DocSummaryInformation, hr %#lx.\n", hr);
    hr = IPropertyStorage_Enum(prop_storage, &prop_enum);
    ok(hr == S_OK, "IPropertyStorage_Enum failed, hr %#lx.\n", hr);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_OK, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    ok(pstg.propid == 0x10, "pstg.propid = %lx\n", pstg.propid);
    todo_wine ok(pstg.vt == VT_I4, "pstg.vt = %d\n", pstg.vt);
    memset(&pstg, 0, sizeof(pstg));
    hr = IEnumSTATPROPSTG_Next(prop_enum, 1, &pstg, &fetched);
    ok(hr == S_FALSE, "IEnumSTATPROPSTG_Next failed, hr %#lx.\n", hr);
    IEnumSTATPROPSTG_Release(prop_enum);
    IPropertyStorage_Release(prop_storage);

    IPropertySetStorage_Release(ps_storage);
    IStorage_Release(storage);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete storage file.\n");
}

START_TEST(stg_prop)
{
    testProps();
    testCodepage();
    testFmtId();
    test_propertyset_storage_enum();
}
