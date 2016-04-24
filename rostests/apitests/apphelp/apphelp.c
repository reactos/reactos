/*
 * Copyright 2012 Detlef Riekenberg
 * Copyright 2013 Mislav Blažević
 * Copyright 2016 Mark Jansen
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


#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <shlwapi.h>
#include <winnt.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif

#include <winerror.h>
#include <stdio.h>

#include "wine/test.h"


static DWORD g_Version;

#define VERSION_ANY     0
#define VERSION_WINXP   0x0501
#define VERSION_2003    0x0502
#define VERSION_VISTA   0x0600
#define VERSION_WIN7    0x0601
#define VERSION_WIN8    0x0602
#define VERSION_WIN10   0x1000


typedef WORD TAG;

#define TAG_TYPE_MASK 0xF000

#define TAG_TYPE_NULL 0x1000
#define TAG_TYPE_BYTE 0x2000
#define TAG_TYPE_WORD 0x3000
#define TAG_TYPE_DWORD 0x4000
#define TAG_TYPE_QWORD 0x5000
#define TAG_TYPE_STRINGREF 0x6000
#define TAG_TYPE_LIST 0x7000
#define TAG_TYPE_STRING 0x8000
#define TAG_TYPE_BINARY 0x9000


static HMODULE hdll;
static LPCWSTR (WINAPI *pSdbTagToString)(TAG);

static void test_SdbTagToString(void)
{
    static const TAG invalid_values[] = {
        1, TAG_TYPE_WORD, TAG_TYPE_MASK,
        TAG_TYPE_DWORD | 0xFF,
        TAG_TYPE_DWORD | (0x800 + 0xEE),
        0x900, 0xFFFF, 0xDEAD, 0xBEEF
    };
    static const WCHAR invalid[] = {'I','n','v','a','l','i','d','T','a','g',0};
    LPCWSTR ret;
    WORD i;

    for (i = 0; i < 9; ++i)
    {
        ret = pSdbTagToString(invalid_values[i]);
        ok(lstrcmpW(ret, invalid) == 0, "unexpected string %s, should be %s\n",
           wine_dbgstr_w(ret), wine_dbgstr_w(invalid));
    }
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), 0, 0);
    return lstrcmpA(buf, stra);
}

void test_tag(TAG base, const char* names[], size_t upperlimit, int line)
{
    TAG n;
    for (n = 0; names[n]; ++n)
    {
        LPCWSTR tagstr = pSdbTagToString(base | n);
        ok_(__FILE__, line + 2)(!strcmp_wa(tagstr, names[n]), "Got %s instead of '%s' for %x\n", wine_dbgstr_w(tagstr), names[n], base | n);
    }
    for (; n < upperlimit; ++n)
    {
        LPCWSTR tagstr = pSdbTagToString(base | n);
        ok_(__FILE__, line + 2)(!strcmp_wa(tagstr, "InvalidTag"), "Got %s instead of 'InvalidTag' for %x\n", wine_dbgstr_w(tagstr), base | n);
    }
}

static struct
{
    TAG base;
    DWORD upper_limit;
    DWORD line;
    DWORD min_ver;
    DWORD max_ver;
    const char* tags[7*8];
} data[] = {
    {
        TAG_TYPE_NULL, 0x1000, __LINE__, VERSION_ANY, VERSION_2003,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", NULL
        }
    },
    {
        TAG_TYPE_NULL, 0x1000, __LINE__, VERSION_VISTA, VERSION_VISTA,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", "MITIGATION_OS", "BLOCK_UPGRADE",
            "INCLUDEEXCLUDEDLL", NULL
        }
    },
    {
        TAG_TYPE_NULL, 0x1000, __LINE__, VERSION_WIN7, VERSION_ANY,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", "MITIGATION_OS", "BLOCK_UPGRADE",
            "INCLUDEEXCLUDEDLL", "RAC_EVENT_OFF", "TELEMETRY_OFF", "SHIM_ENGINE_OFF", "LAYER_PROPAGATION_OFF", "REINSTALL_UPGRADE", NULL
        }
    },

    {
        TAG_TYPE_BYTE, 0x1000, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", NULL
        }
    },

    {
        TAG_TYPE_WORD, 0x800, __LINE__, VERSION_ANY, VERSION_WIN7,
        {
            "InvalidTag", "MATCH_MODE", NULL
        }
    },
    {
        TAG_TYPE_WORD, 0x800, __LINE__, VERSION_WIN8, VERSION_ANY,
        {
            "InvalidTag", "MATCH_MODE", "QUIRK_COMPONENT_CODE_ID", "QUIRK_CODE_ID", NULL
        }
    },
    {
        TAG_TYPE_WORD | 0x800, 0x800, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", "TAG", "INDEX_TAG", "INDEX_KEY", NULL
        }
    },

    {
        TAG_TYPE_DWORD, 0x800, __LINE__, VERSION_ANY, VERSION_WINXP,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERFILEDATEHI",
            "VERFILEDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVERSION", "PREVOSMINORVERSION", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEM_SEVERITY", "APPHELP_LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEXFLAGS", "FLAGS",
            "VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", NULL
        }
    },
    {
        TAG_TYPE_DWORD, 0x800, __LINE__, VERSION_2003, VERSION_2003,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERFILEDATEHI",
            "VERFILEDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVERSION", "PREVOSMINORVERSION", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEM_SEVERITY", "APPHELP_LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEXFLAGS", "FLAGS",
            "VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", "OS_PLATFORM", NULL
        }
    },
    {
        TAG_TYPE_DWORD, 0x800, __LINE__, VERSION_VISTA, VERSION_VISTA,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERDATEHI",
            "VERDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVER", "PREVOSMINORVER", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEMSEVERITY", "LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEX_FLAGS", "FLAGS",
            "DATA_VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", "OS_PLATFORM", "APP_NAME_RC_ID", "VENDOR_NAME_RC_ID", "SUMMARY_MSG_RC_ID", "VISTA_SKU",
            NULL
        }
    },
    {
        TAG_TYPE_DWORD, 0x800, __LINE__, VERSION_WIN7, VERSION_ANY,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERDATEHI",
            "VERDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVER", "PREVOSMINORVER", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEMSEVERITY", "LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEX_FLAGS", "FLAGS",
            "DATA_VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", "OS_PLATFORM", "APP_NAME_RC_ID", "VENDOR_NAME_RC_ID", "SUMMARY_MSG_RC_ID", "VISTA_SKU",
            "DESCRIPTION_RC_ID", "PARAMETER1_RC_ID", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", 
            "CONTEXT_TAGID", "EXE_WRAPPER", "URL_ID", NULL
        }
    },
    {
        TAG_TYPE_DWORD | 0x800, 0x800, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", "TAGID", NULL
        }
    },

    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, VERSION_ANY, VERSION_WINXP,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", NULL
        }
    },
    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, VERSION_2003, VERSION_2003,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", "FLAG_MASK_FUSION", "FLAGS_PROCESSPARAM",
            NULL
        }
    },
    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, VERSION_VISTA, VERSION_ANY,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", "FLAG_MASK_FUSION", "FLAG_PROCESSPARAM",
            "FLAG_LUA", "FLAG_INSTALL", NULL
        }
    },

    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, VERSION_ANY, VERSION_2003,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "S16BIT_DESCRIPTION",
            "PROBLEM_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "S16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", NULL
        }
    },
    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, VERSION_VISTA, VERSION_VISTA,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "16BIT_DESCRIPTION",
            "APPHELP_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", "EXPORT_NAME", NULL
        }
    },
    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, VERSION_WIN7, VERSION_ANY,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "16BIT_DESCRIPTION",
            "APPHELP_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", "EXPORT_NAME", "URL", NULL
        }
    },

    {
        TAG_TYPE_LIST, 0x800, __LINE__, VERSION_ANY, VERSION_2003,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI TRANSFORM", "MSI TRANSFORM REF", "MSI PACKAGE", "FLAG", "MSI CUSTOM ACTION", "FLAG_REF", "ACTION", NULL
        }
    },
    {
        TAG_TYPE_LIST, 0x800, __LINE__, VERSION_VISTA, VERSION_VISTA,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI_TRANSFORM", "MSI_TRANSFORM_REF", "MSI_PACKAGE", "FLAG", "MSI_CUSTOM_ACTION", "FLAG_REF", "ACTION", "LOOKUP",
            NULL
        }
    },
    {
        TAG_TYPE_LIST, 0x800, __LINE__, VERSION_WIN7, VERSION_ANY,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI_TRANSFORM", "MSI_TRANSFORM_REF", "MSI_PACKAGE", "FLAG", "MSI_CUSTOM_ACTION", "FLAG_REF", "ACTION", "LOOKUP",
            "CONTEXT", "CONTEXT_REF", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "SPC", NULL
        }
    },
    {
        TAG_TYPE_LIST | 0x800, 0x800, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", "STRINGTABLE", "INDEXES", "INDEX", NULL
        }
    },

    {
        TAG_TYPE_STRING, 0x800, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", NULL
        }
    },
    {
        TAG_TYPE_STRING | 0x800, 0x800, __LINE__, VERSION_ANY, VERSION_2003,
        {
            "InvalidTag", "STRTAB_ITEM", NULL
        }
    },
    {
        TAG_TYPE_STRING | 0x800, 0x800, __LINE__, VERSION_VISTA, VERSION_ANY,
        {
            "InvalidTag", "STRINGTABLE_ITEM", NULL
        }
    },


    {
        TAG_TYPE_BINARY, 0x800, __LINE__, VERSION_ANY, VERSION_2003,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID(GUID)", "DATA_BITS", "MSI_PACKAGE_ID(GUID)", "DATABASE_ID(GUID)",
            NULL
        }
    },
    {
        TAG_TYPE_BINARY, 0x800, __LINE__, VERSION_VISTA, VERSION_VISTA,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID", "DATA_BITS", "MSI_PACKAGE_ID", "DATABASE_ID",
            NULL
        }
    },
    {
        TAG_TYPE_BINARY, 0x800, __LINE__, VERSION_WIN7, VERSION_ANY,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID", "DATA_BITS", "MSI_PACKAGE_ID", "DATABASE_ID",
            "CONTEXT_PLATFORM_ID", "CONTEXT_BRANCH_ID", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "FIX_ID", "APP_ID", NULL
        }
    },
    {
        TAG_TYPE_BINARY | 0x800, 0x800, __LINE__, VERSION_ANY, VERSION_ANY,
        {
            "InvalidTag", "INDEX_BITS", NULL
        }
    },

    { 0, 0, 0, 0, 0, { NULL } }
};


static void test_SdbTagToStringAllTags(void)
{
    int n;
    for (n = 0; data[n].base; ++n)
    {
        if ((data[n].min_ver == VERSION_ANY || g_Version >= data[n].min_ver) &&
            (data[n].max_ver == VERSION_ANY || g_Version <= data[n].max_ver))
        {
            test_tag(data[n].base, data[n].tags, data[n].upper_limit, data[n].line);
        }
    }
}

START_TEST(apphelp)
{
    RTL_OSVERSIONINFOEXW rtlinfo;
    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
#ifdef __REACTOS__
    RtlGetVersion((PRTL_OSVERSIONINFOW)&rtlinfo);
#else
    RtlGetVersion(&rtlinfo);
#endif
    g_Version = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;
    trace("Detected version: 0x%x\n", g_Version);
    //SetEnvironmentVariable("SHIM_DEBUG_LEVEL", "4");
    //SetEnvironmentVariable("DEBUGCHANNEL", "+apphelp");
    hdll = LoadLibraryA("apphelp.dll");
    pSdbTagToString = (void *) GetProcAddress(hdll, "SdbTagToString");
    test_SdbTagToString();
#ifdef __REACTOS__
    if (g_Version < VERSION_WIN7)
    {
        g_Version = VERSION_WIN7;
        trace("Using version 0x%x for SdbTagToString tests\n", g_Version);
    }
#endif
    test_SdbTagToStringAllTags();
}
