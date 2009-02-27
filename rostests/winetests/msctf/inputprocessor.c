/*
 * Unit tests for ITfInputProcessor
 *
 * Copyright 2009 Aric Stewart, CodeWeavers
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
#include "wine/test.h"
#include "winuser.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "comcat.h"
#include "initguid.h"
#include "msctf.h"

static ITfInputProcessorProfiles* g_ipp;
static LANGID gLangid;
static ITfCategoryMgr * g_cm;

DEFINE_GUID(CLSID_FakeService, 0xEDE1A7AD,0x66DE,0x47E0,0xB6,0x20,0x3E,0x92,0xF8,0x24,0x6B,0xF3);
DEFINE_GUID(CLSID_TF_InputProcessorProfiles, 0x33c53a50,0xf456,0x4884,0xb0,0x49,0x85,0xfd,0x64,0x3e,0xcf,0xed);
DEFINE_GUID(CLSID_TF_CategoryMgr,         0xA4B544A1,0x438D,0x4B41,0x93,0x25,0x86,0x95,0x23,0xE2,0xD6,0xC7);
DEFINE_GUID(GUID_TFCAT_TIP_KEYBOARD,     0x34745c63,0xb2f0,0x4784,0x8b,0x67,0x5e,0x12,0xc8,0x70,0x1a,0x31);
DEFINE_GUID(GUID_TFCAT_TIP_SPEECH,       0xB5A73CD1,0x8355,0x426B,0xA1,0x61,0x25,0x98,0x08,0xF2,0x6B,0x14);
DEFINE_GUID(GUID_TFCAT_TIP_HANDWRITING,  0x246ecb87,0xc2f2,0x4abe,0x90,0x5b,0xc8,0xb3,0x8a,0xdd,0x2c,0x43);
DEFINE_GUID (GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,  0x046B8C80,0x1647,0x40F7,0x9B,0x21,0xB9,0x3B,0x81,0xAA,0xBC,0x1B);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);


static HRESULT initialize(void)
{
    HRESULT hr;
    CoInitialize(NULL);
    hr = CoCreateInstance (&CLSID_TF_InputProcessorProfiles, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfInputProcessorProfiles, (void**)&g_ipp);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance (&CLSID_TF_CategoryMgr, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfCategoryMgr, (void**)&g_cm);
    return hr;
}

static void cleanup(void)
{
    if (g_ipp)
        ITfInputProcessorProfiles_Release(g_ipp);
    if (g_cm)
        ITfCategoryMgr_Release(g_cm);
    CoUninitialize();
}

static void test_Register(void)
{
    HRESULT hr;

    static const WCHAR szDesc[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',0};
    static const WCHAR szFile[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',' ','F','i','l','e',0};

    hr = ITfInputProcessorProfiles_Register(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register text service(%x)\n",hr);
    hr = ITfInputProcessorProfiles_AddLanguageProfile(g_ipp, &CLSID_FakeService, gLangid, &CLSID_FakeService, szDesc, sizeof(szDesc)/sizeof(WCHAR), szFile, sizeof(szFile)/sizeof(WCHAR), 1);
    ok(SUCCEEDED(hr),"Unable to add Language Profile (%x)\n",hr);
}

static void test_Unregister(void)
{
    HRESULT hr;
    hr = ITfInputProcessorProfiles_Unregister(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to unregister text service(%x)\n",hr);
}

static void test_EnumInputProcessorInfo(void)
{
    IEnumGUID *ppEnum;
    BOOL found = FALSE;

    if (SUCCEEDED(ITfInputProcessorProfiles_EnumInputProcessorInfo(g_ipp, &ppEnum)))
    {
        ULONG fetched;
        GUID g;
        while (IEnumGUID_Next(ppEnum, 1, &g, &fetched) == S_OK)
        {
            if(IsEqualGUID(&g,&CLSID_FakeService))
                found = TRUE;
        }
    }
    ok(found,"Did not find registered text service\n");
}

static void test_EnumLanguageProfiles(void)
{
    BOOL found = FALSE;
    IEnumTfLanguageProfiles *ppEnum;
    if (SUCCEEDED(ITfInputProcessorProfiles_EnumLanguageProfiles(g_ipp,gLangid,&ppEnum)))
    {
        TF_LANGUAGEPROFILE profile;
        while (IEnumTfLanguageProfiles_Next(ppEnum,1,&profile,NULL)==S_OK)
        {
            if (IsEqualGUID(&profile.clsid,&CLSID_FakeService))
            {
                found = TRUE;
                ok(profile.langid == gLangid, "LangId Incorrect\n");
                todo_wine ok(IsEqualGUID(&profile.catid,&GUID_TFCAT_TIP_KEYBOARD), "CatId Incorrect\n");
                ok(IsEqualGUID(&profile.guidProfile,&CLSID_FakeService), "guidProfile Incorrect\n");
            }
        }
    }
    ok(found,"Registered text service not found\n");
}

static void test_RegisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_RegisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"ITfCategoryMgr_RegisterCategory failed\n");
}

static void test_UnregisterCategory(void)
{
    HRESULT hr;
    hr = ITfCategoryMgr_UnregisterCategory(g_cm, &CLSID_FakeService, &GUID_TFCAT_TIP_KEYBOARD, &CLSID_FakeService);
    todo_wine ok(SUCCEEDED(hr),"ITfCategoryMgr_UnregisterCategory failed\n");
}

START_TEST(inputprocessor)
{
    if (SUCCEEDED(initialize()))
    {
        gLangid = GetUserDefaultLCID();
        test_Register();
        test_RegisterCategory();
        test_EnumInputProcessorInfo();
        test_EnumLanguageProfiles();
        test_UnregisterCategory();
        test_Unregister();
    }
    else
        skip("Unable to create InputProcessor\n");
    cleanup();
}
