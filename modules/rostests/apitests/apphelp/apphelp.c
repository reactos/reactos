/*
 * PROJECT:     apphelp_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Misc apphelp tests
 * COPYRIGHT:   Copyright 2012 Detlef Riekenberg
 *              Copyright 2013 Mislav Blažević
 *              Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
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
#include <initguid.h>
#include <shlguid.h>
#include <shobjidl.h>

#include "wine/test.h"

#include "apphelp_apitest.h"


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
#define TAG_NULL 0x0
#define TAG_SIZE (0x1 | TAG_TYPE_DWORD)
#define TAG_CHECKSUM (0x3 | TAG_TYPE_DWORD)
#define TAG_MODULE_TYPE (0x6 | TAG_TYPE_DWORD)
#define TAG_VERDATEHI (0x7 | TAG_TYPE_DWORD)
#define TAG_VERDATELO (0x8 | TAG_TYPE_DWORD)
#define TAG_VERFILEOS (0x9 | TAG_TYPE_DWORD)
#define TAG_VERFILETYPE (0xA | TAG_TYPE_DWORD)
#define TAG_PE_CHECKSUM (0xB | TAG_TYPE_DWORD)
#define TAG_VER_LANGUAGE (0x12 | TAG_TYPE_DWORD)
#define TAG_LINKER_VERSION (0x1C | TAG_TYPE_DWORD)
#define TAG_LINK_DATE (0x1D | TAG_TYPE_DWORD)
#define TAG_UPTO_LINK_DATE (0x1E | TAG_TYPE_DWORD)
#define TAG_EXE_WRAPPER (0x31 | TAG_TYPE_DWORD)
#define TAG_BIN_FILE_VERSION (0x2 | TAG_TYPE_QWORD)
#define TAG_BIN_PRODUCT_VERSION (0x3 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_PRODUCT_VERSION (0x6 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_FILE_VERSION (0xD | TAG_TYPE_QWORD)
#define TAG_NAME (0x1 | TAG_TYPE_STRINGREF)
#define TAG_COMPANY_NAME (0x9 | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_NAME (0x10 | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_VERSION (0x11 | TAG_TYPE_STRINGREF)
#define TAG_FILE_DESCRIPTION (0x12 | TAG_TYPE_STRINGREF)
#define TAG_FILE_VERSION (0x13 | TAG_TYPE_STRINGREF)
#define TAG_ORIGINAL_FILENAME (0x14 | TAG_TYPE_STRINGREF)
#define TAG_INTERNAL_NAME (0x15 | TAG_TYPE_STRINGREF)
#define TAG_LEGAL_COPYRIGHT (0x16 | TAG_TYPE_STRINGREF)
#define TAG_16BIT_DESCRIPTION (0x17 | TAG_TYPE_STRINGREF)
#define TAG_16BIT_MODULE_NAME (0x20 | TAG_TYPE_STRINGREF)
#define TAG_EXPORT_NAME (0x24 | TAG_TYPE_STRINGREF)


#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

typedef struct tagATTRINFO {
  TAG   type;
  DWORD flags;  /* ATTRIBUTE_AVAILABLE, ATTRIBUTE_FAILED */
  union {
    QWORD qwattr;
    DWORD dwattr;
    WCHAR *lpattr;
  };
} ATTRINFO, *PATTRINFO;

static HMODULE hdll;
static BOOL (WINAPI *pApphelpCheckShellObject)(REFCLSID, BOOL, ULONGLONG *);
static LPCWSTR (WINAPI *pSdbTagToString)(TAG tag);
static BOOL (WINAPI *pSdbGUIDToString)(REFGUID Guid, PWSTR GuidString, SIZE_T Length);
static BOOL (WINAPI *pSdbIsNullGUID)(REFGUID Guid);
static BOOL (WINAPI *pSdbGetStandardDatabaseGUID)(DWORD Flags, GUID* Guid);
static BOOL (WINAPI *pSdbGetFileAttributes)(LPCWSTR wszPath, PATTRINFO *ppAttrInfo, LPDWORD pdwAttrCount);
static BOOL (WINAPI *pSdbFreeFileAttributes)(PATTRINFO AttrInfo);
static HRESULT (WINAPI* pSdbGetAppPatchDir)(PVOID hsdb, LPWSTR path, DWORD size);
static DWORD g_AttrInfoSize;

/* 'Known' database guids */
DEFINE_GUID(GUID_DATABASE_MSI,0xd8ff6d16,0x6a3a,0x468a,0x8b,0x44,0x01,0x71,0x4d,0xdc,0x49,0xea);
DEFINE_GUID(GUID_DATABASE_SHIM,0x11111111,0x1111,0x1111,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11);
DEFINE_GUID(GUID_DATABASE_DRIVERS,0xf9ab2228,0x3312,0x4a73,0xb6,0xf9,0x93,0x6d,0x70,0xe1,0x12,0xef);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(test_Microsoft_Browser_Architecture, 0xa5e46e3a, 0x8849, 0x11d1, 0x9d, 0x8c, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x61);
DEFINE_GUID(test_UserAssist, 0xdd313e04, 0xfeff, 0x11d1, 0x8e, 0xcd, 0x00, 0x00, 0xf8, 0x7a, 0x47, 0x0c);
DEFINE_GUID(CLSID_InternetSecurityManager, 0x7b8a2d94, 0x0ac9, 0x11d1, 0x89, 0x6c, 0x00, 0xc0, 0x4f, 0xB6, 0xbf, 0xc4);

static const CLSID * objects[] = {
    &GUID_NULL,
    /* used by IE */
    &test_Microsoft_Browser_Architecture,
    &CLSID_MenuBand,
    &CLSID_ShellLink,
    &CLSID_ShellWindows,
    &CLSID_InternetSecurityManager,
    &test_UserAssist,
    (const CLSID *)NULL
};

static void test_ApphelpCheckShellObject(void)
{
    ULONGLONG flags;
    BOOL res;
    int i;

    if (!pApphelpCheckShellObject)
    {
        win_skip("ApphelpCheckShellObject not available\n");
        return;
    }

    for (i = 0; objects[i]; i++)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], FALSE, &flags);
        ok(res && (flags == 0), "%s 0: got %d and 0x%x%08x with 0x%x (expected TRUE and 0)\n",
            wine_dbgstr_guid(objects[i]), res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());

        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(objects[i], TRUE, &flags);
        ok(res && (flags == 0), "%s 1: got %d and 0x%x%08x with 0x%x (expected TRUE and 0)\n",
            wine_dbgstr_guid(objects[i]), res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());

    }

    /* NULL as pointer to flags is checked */
    SetLastError(0xdeadbeef);
    res = pApphelpCheckShellObject(&GUID_NULL, FALSE, NULL);
    ok(res, "%s 0: got %d with 0x%x (expected != FALSE)\n", wine_dbgstr_guid(&GUID_NULL), res, GetLastError());

    /* NULL as CLSID* crash on Windows */
    if (0)
    {
        flags = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pApphelpCheckShellObject(NULL, FALSE, &flags);
        trace("NULL as CLSID*: got %d and 0x%x%08x with 0x%x\n", res, (ULONG)(flags >> 32), (ULONG)flags, GetLastError());
    }
}

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
        TAG_TYPE_NULL, 0x1000, __LINE__, WINVER_ANY, WINVER_2003,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", NULL
        }
    },
    {
        TAG_TYPE_NULL, 0x1000, __LINE__, WINVER_VISTA, WINVER_VISTA,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", "MITIGATION_OS", "BLOCK_UPGRADE",
            "INCLUDEEXCLUDEDLL", NULL
        }
    },
    {
        TAG_TYPE_NULL, 0x1000, __LINE__, WINVER_WIN7, WINVER_ANY,
        {
            "InvalidTag", "INCLUDE", "GENERAL", "MATCH_LOGIC_NOT", "APPLY_ALL_SHIMS", "USE_SERVICE_PACK_FILES", "MITIGATION_OS", "BLOCK_UPGRADE",
            "INCLUDEEXCLUDEDLL", "RAC_EVENT_OFF", "TELEMETRY_OFF", "SHIM_ENGINE_OFF", "LAYER_PROPAGATION_OFF", "REINSTALL_UPGRADE", NULL
        }
    },

    {
        TAG_TYPE_BYTE, 0x1000, __LINE__, WINVER_ANY, WINVER_ANY,
        {
            "InvalidTag", NULL
        }
    },

    {
        TAG_TYPE_WORD, 0x800, __LINE__, WINVER_ANY, WINVER_WIN7,
        {
            "InvalidTag", "MATCH_MODE", NULL
        }
    },
    {
        TAG_TYPE_WORD, 0x800, __LINE__, WINVER_WIN8, WINVER_ANY,
        {
            "InvalidTag", "MATCH_MODE", "QUIRK_COMPONENT_CODE_ID", "QUIRK_CODE_ID", NULL
        }
    },
    {
        TAG_TYPE_WORD | 0x800, 0x800, __LINE__, WINVER_ANY, WINVER_ANY,
        {
            "InvalidTag", "TAG", "INDEX_TAG", "INDEX_KEY", NULL
        }
    },

    {
        TAG_TYPE_DWORD, 0x800, __LINE__, WINVER_ANY, WINVER_WINXP,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERFILEDATEHI",
            "VERFILEDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVERSION", "PREVOSMINORVERSION", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEM_SEVERITY", "APPHELP_LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEXFLAGS", "FLAGS",
            "VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", NULL
        }
    },
    {
        TAG_TYPE_DWORD, 0x800, __LINE__, WINVER_2003, WINVER_2003,
        {
            "InvalidTag", "SIZE", "OFFSET", "CHECKSUM", "SHIM_TAGID", "PATCH_TAGID", "MODULE_TYPE", "VERFILEDATEHI",
            "VERFILEDATELO", "VERFILEOS", "VERFILETYPE", "PE_CHECKSUM", "PREVOSMAJORVERSION", "PREVOSMINORVERSION", "PREVOSPLATFORMID", "PREVOSBUILDNO",
            "PROBLEM_SEVERITY", "APPHELP_LANGID", "VER_LANGUAGE", "InvalidTag", "ENGINE", "HTMLHELPID", "INDEXFLAGS", "FLAGS",
            "VALUETYPE", "DATA_DWORD", "LAYER_TAGID", "MSI_TRANSFORM_TAGID", "LINKER_VERSION", "LINK_DATE", "UPTO_LINK_DATE", "OS_SERVICE_PACK",
            "FLAG_TAGID", "RUNTIME_PLATFORM", "OS_SKU", "OS_PLATFORM", NULL
        }
    },
    {
        TAG_TYPE_DWORD, 0x800, __LINE__, WINVER_VISTA, WINVER_VISTA,
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
        TAG_TYPE_DWORD, 0x800, __LINE__, WINVER_WIN7, WINVER_ANY,
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
        TAG_TYPE_DWORD | 0x800, 0x800, __LINE__, WINVER_ANY, WINVER_ANY,
        {
            "InvalidTag", "TAGID", NULL
        }
    },

    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, WINVER_ANY, WINVER_WINXP,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", NULL
        }
    },
    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, WINVER_2003, WINVER_2003,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", "FLAG_MASK_FUSION", "FLAGS_PROCESSPARAM",
            NULL
        }
    },
    {
        TAG_TYPE_QWORD, 0x1000, __LINE__, WINVER_VISTA, WINVER_ANY,
        {
            "InvalidTag", "TIME", "BIN_FILE_VERSION", "BIN_PRODUCT_VERSION", "MODTIME", "FLAG_MASK_KERNEL", "UPTO_BIN_PRODUCT_VERSION", "DATA_QWORD",
            "FLAG_MASK_USER", "FLAGS_NTVDM1", "FLAGS_NTVDM2", "FLAGS_NTVDM3", "FLAG_MASK_SHELL", "UPTO_BIN_FILE_VERSION", "FLAG_MASK_FUSION", "FLAG_PROCESSPARAM",
            "FLAG_LUA", "FLAG_INSTALL", NULL
        }
    },

    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, WINVER_ANY, WINVER_2003,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "S16BIT_DESCRIPTION",
            "PROBLEM_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "S16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", NULL
        }
    },
    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, WINVER_VISTA, WINVER_VISTA,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "16BIT_DESCRIPTION",
            "APPHELP_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", "EXPORT_NAME", NULL
        }
    },
    {
        TAG_TYPE_STRINGREF, 0x1000, __LINE__, WINVER_WIN7, WINVER_ANY,
        {
            "InvalidTag", "NAME", "DESCRIPTION", "MODULE", "API", "VENDOR", "APP_NAME", "InvalidTag",
            "COMMAND_LINE", "COMPANY_NAME", "DLLFILE", "WILDCARD_NAME", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "PRODUCT_NAME", "PRODUCT_VERSION", "FILE_DESCRIPTION", "FILE_VERSION", "ORIGINAL_FILENAME", "INTERNAL_NAME", "LEGAL_COPYRIGHT", "16BIT_DESCRIPTION",
            "APPHELP_DETAILS", "LINK_URL", "LINK_TEXT", "APPHELP_TITLE", "APPHELP_CONTACT", "SXS_MANIFEST", "DATA_STRING", "MSI_TRANSFORM_FILE",
            "16BIT_MODULE_NAME", "LAYER_DISPLAYNAME", "COMPILER_VERSION", "ACTION_TYPE", "EXPORT_NAME", "URL", NULL
        }
    },

    {
        TAG_TYPE_LIST, 0x800, __LINE__, WINVER_ANY, WINVER_2003,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI TRANSFORM", "MSI TRANSFORM REF", "MSI PACKAGE", "FLAG", "MSI CUSTOM ACTION", "FLAG_REF", "ACTION", NULL
        }
    },
    {
        TAG_TYPE_LIST, 0x800, __LINE__, WINVER_VISTA, WINVER_VISTA,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI_TRANSFORM", "MSI_TRANSFORM_REF", "MSI_PACKAGE", "FLAG", "MSI_CUSTOM_ACTION", "FLAG_REF", "ACTION", "LOOKUP",
            NULL
        }
    },
    {
        TAG_TYPE_LIST, 0x800, __LINE__, WINVER_WIN7, WINVER_ANY,
        {
            "InvalidTag", "DATABASE", "LIBRARY", "INEXCLUDE", "SHIM", "PATCH", "APP", "EXE",
            "MATCHING_FILE", "SHIM_REF", "PATCH_REF", "LAYER", "FILE", "APPHELP", "LINK", "DATA",
            "MSI_TRANSFORM", "MSI_TRANSFORM_REF", "MSI_PACKAGE", "FLAG", "MSI_CUSTOM_ACTION", "FLAG_REF", "ACTION", "LOOKUP",
            "CONTEXT", "CONTEXT_REF", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "SPC", NULL
        }
    },
    {
        TAG_TYPE_LIST | 0x800, 0x800, __LINE__, WINVER_ANY, WINVER_ANY,
        {
            "InvalidTag", "STRINGTABLE", "INDEXES", "INDEX", NULL
        }
    },

    {
        TAG_TYPE_STRING, 0x800, __LINE__, WINVER_ANY, WINVER_ANY,
        {
            "InvalidTag", NULL
        }
    },
    {
        TAG_TYPE_STRING | 0x800, 0x800, __LINE__, WINVER_ANY, WINVER_2003,
        {
            "InvalidTag", "STRTAB_ITEM", NULL
        }
    },
    {
        TAG_TYPE_STRING | 0x800, 0x800, __LINE__, WINVER_VISTA, WINVER_ANY,
        {
            "InvalidTag", "STRINGTABLE_ITEM", NULL
        }
    },


    {
        TAG_TYPE_BINARY, 0x800, __LINE__, WINVER_ANY, WINVER_2003,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID(GUID)", "DATA_BITS", "MSI_PACKAGE_ID(GUID)", "DATABASE_ID(GUID)",
            NULL
        }
    },
    {
        TAG_TYPE_BINARY, 0x800, __LINE__, WINVER_VISTA, WINVER_VISTA,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID", "DATA_BITS", "MSI_PACKAGE_ID", "DATABASE_ID",
            NULL
        }
    },
    {
        TAG_TYPE_BINARY, 0x800, __LINE__, WINVER_WIN7, WINVER_ANY,
        {
            "InvalidTag", "InvalidTag", "PATCH_BITS", "FILE_BITS", "EXE_ID", "DATA_BITS", "MSI_PACKAGE_ID", "DATABASE_ID",
            "CONTEXT_PLATFORM_ID", "CONTEXT_BRANCH_ID", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag", "InvalidTag",
            "FIX_ID", "APP_ID", NULL
        }
    },
    {
        TAG_TYPE_BINARY | 0x800, 0x800, __LINE__, WINVER_ANY, WINVER_ANY,
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
        if ((data[n].min_ver == WINVER_ANY || g_WinVersion >= data[n].min_ver) &&
            (data[n].max_ver == WINVER_ANY || g_WinVersion <= data[n].max_ver))
        {
            test_tag(data[n].base, data[n].tags, data[n].upper_limit, data[n].line);
        }
    }
}

static void test_GuidFunctions(void)
{
    GUID guid;
    ok(pSdbIsNullGUID(&GUID_NULL), "expected GUID_NULL to be recognized as NULL GUID\n");
    ok(pSdbIsNullGUID(NULL), "expected NULL to be recognized as NULL GUID\n");
    ok(pSdbIsNullGUID(&test_UserAssist) == 0, "expected a set GUID not to be recognized as NULL GUID\n");

    memset(&guid, 0, sizeof(guid));
    ok(pSdbGetStandardDatabaseGUID(0, &guid) == 0,"Expected SdbGetStandardDatabaseGUID to fail\n");
    ok(IsEqualGUID(&GUID_NULL, &guid), "Expected guid not to be changed\n");

    ok(pSdbGetStandardDatabaseGUID(0x80020000, NULL),"Expected SdbGetStandardDatabaseGUID to succeed\n");

    memset(&guid, 0, sizeof(guid));
    ok(pSdbGetStandardDatabaseGUID(0x80020000, &guid),"Expected SdbGetStandardDatabaseGUID to succeed\n");
    ok(IsEqualGUID(&GUID_DATABASE_MSI, &guid), "Expected guid to equal GUID_DATABASE_MSI, was: %s\n", wine_dbgstr_guid(&guid));

    memset(&guid, 0, sizeof(guid));
    ok(pSdbGetStandardDatabaseGUID(0x80030000, &guid),"Expected SdbGetStandardDatabaseGUID to succeed\n");
    ok(IsEqualGUID(&GUID_DATABASE_SHIM, &guid), "Expected guid to equal GUID_DATABASE_SHIM, was: %s\n", wine_dbgstr_guid(&guid));

    memset(&guid, 0, sizeof(guid));
    ok(pSdbGetStandardDatabaseGUID(0x80040000, &guid),"Expected SdbGetStandardDatabaseGUID to succeed\n");
    ok(IsEqualGUID(&GUID_DATABASE_DRIVERS, &guid), "Expected guid to equal GUID_DATABASE_DRIVERS, was: %s\n", wine_dbgstr_guid(&guid));
}

static TAG g_Tags_26[] = {
    TAG_SIZE,
    TAG_CHECKSUM,
    TAG_BIN_FILE_VERSION,
    TAG_BIN_PRODUCT_VERSION,
    TAG_PRODUCT_VERSION,
    TAG_FILE_DESCRIPTION,
    TAG_COMPANY_NAME,
    TAG_PRODUCT_NAME,
    TAG_FILE_VERSION,
    TAG_ORIGINAL_FILENAME,
    TAG_INTERNAL_NAME,
    TAG_LEGAL_COPYRIGHT,
    TAG_VERDATEHI,  /* TAG_VERFILEDATEHI */
    TAG_VERDATELO,  /* TAG_VERFILEDATELO */
    TAG_VERFILEOS,
    TAG_VERFILETYPE,
    TAG_MODULE_TYPE,
    TAG_PE_CHECKSUM,
    TAG_LINKER_VERSION,
    TAG_16BIT_DESCRIPTION,  /* CHECKME! */
    TAG_16BIT_MODULE_NAME,  /* CHECKME! */
    TAG_UPTO_BIN_FILE_VERSION,
    TAG_UPTO_BIN_PRODUCT_VERSION,
    TAG_LINK_DATE,
    TAG_UPTO_LINK_DATE,
    TAG_VER_LANGUAGE,
    0
};

static TAG g_Tags_28[] = {
    TAG_SIZE,
    TAG_CHECKSUM,
    TAG_BIN_FILE_VERSION,
    TAG_BIN_PRODUCT_VERSION,
    TAG_PRODUCT_VERSION,
    TAG_FILE_DESCRIPTION,
    TAG_COMPANY_NAME,
    TAG_PRODUCT_NAME,
    TAG_FILE_VERSION,
    TAG_ORIGINAL_FILENAME,
    TAG_INTERNAL_NAME,
    TAG_LEGAL_COPYRIGHT,
    TAG_VERDATEHI,
    TAG_VERDATELO,
    TAG_VERFILEOS,
    TAG_VERFILETYPE,
    TAG_MODULE_TYPE,
    TAG_PE_CHECKSUM,
    TAG_LINKER_VERSION,
    TAG_16BIT_DESCRIPTION,
    TAG_16BIT_MODULE_NAME,
    TAG_UPTO_BIN_FILE_VERSION,
    TAG_UPTO_BIN_PRODUCT_VERSION,
    TAG_LINK_DATE,
    TAG_UPTO_LINK_DATE,
    TAG_EXPORT_NAME,
    TAG_VER_LANGUAGE,
    TAG_EXE_WRAPPER,
    0
};

static DWORD find_tag(TAG tag)
{
    DWORD n;
    TAG* allTags;
    switch (g_AttrInfoSize)
    {
    case 26:
        allTags = g_Tags_26;
        break;
    case 28:
        allTags = g_Tags_28;
        break;
    default:
        return ~0;
    }

    for (n = 0; n < allTags[n]; ++n)
    {
        if (allTags[n] == tag)
            return n;
    }
    return ~0;
}

static void expect_tag_skip_imp(PATTRINFO pattr, TAG tag)
{
    DWORD num = find_tag(tag);
    PATTRINFO p;

    if (num == ~0)
        return;

    p = &pattr[num];
    winetest_ok(p->type == TAG_NULL, "expected entry #%d to be TAG_NULL, was %x\n", num, p->type);
    winetest_ok(p->flags == ATTRIBUTE_FAILED, "expected entry #%d to be failed, was %d\n", num, p->flags);
    winetest_ok(p->qwattr == 0, "expected entry #%d to be 0, was 0x%I64x\n", num, p->qwattr);
}
static void expect_tag_empty_imp(PATTRINFO pattr, TAG tag)
{
    DWORD num = find_tag(tag);
    PATTRINFO p;

    if (num == ~0)
        return;

    p = &pattr[num];
    winetest_ok(p->type == TAG_NULL, "expected entry #%d to be TAG_NULL, was %x\n", num, p->type);
    winetest_ok(p->flags == 0, "expected entry #%d to be 0, was %d\n", num, p->flags);
    winetest_ok(p->qwattr == 0, "expected entry #%d to be 0, was 0x%I64x\n", num, p->qwattr);
}

static void expect_tag_dword_imp(PATTRINFO pattr, TAG tag, DWORD value)
{
    DWORD num = find_tag(tag);
    PATTRINFO p;

    if (num == ~0)
        return;

    p = &pattr[num];
    winetest_ok(p->type == tag, "expected entry #%d to be %x, was %x\n", num, tag, p->type);
    winetest_ok(p->flags == ATTRIBUTE_AVAILABLE, "expected entry #%d to be available, was %d\n", num, p->flags);
    winetest_ok(p->dwattr == value, "expected entry #%d to be 0x%x, was 0x%x\n", num, value, p->dwattr);
}

static void expect_tag_qword_imp(PATTRINFO pattr, TAG tag, QWORD value)
{
    DWORD num = find_tag(tag);
    PATTRINFO p;

    if (num == ~0)
        return;

    p = &pattr[num];
    winetest_ok(p->type == tag, "expected entry #%d to be %x, was %x\n", num, tag, p->type);
    winetest_ok(p->flags == ATTRIBUTE_AVAILABLE, "expected entry #%d to be available, was %d\n", num, p->flags);
    winetest_ok(p->qwattr == value, "expected entry #%d to be 0x%I64x, was 0x%I64x\n", num, value, p->qwattr);
}

static void expect_tag_str_imp(PATTRINFO pattr, TAG tag, const WCHAR* value)
{
    DWORD num = find_tag(tag);
    PATTRINFO p;

    if (num == ~0)
        return;

    p = &pattr[num];
    winetest_ok(p->type == tag, "expected entry #%d to be %x, was %x\n", num, tag, p->type);
    winetest_ok(p->flags == ATTRIBUTE_AVAILABLE, "expected entry #%d to be available, was %d\n", num, p->flags);
    winetest_ok(p->lpattr && wcscmp(p->lpattr, value) == 0, "expected entry #%d to be %s, was %s\n", num, wine_dbgstr_w(value), wine_dbgstr_w(p->lpattr));
}

#define expect_tag_skip     (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_tag_skip_imp
#define expect_tag_empty    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_tag_empty_imp
#define expect_tag_dword    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_tag_dword_imp
#define expect_tag_qword    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_tag_qword_imp
#define expect_tag_str      (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_tag_str_imp
#define expect_tag_skip_range(ptr, from, to) \
    do { \
        int n = (from), n_end = (to); \
        winetest_set_location(__FILE__, __LINE__); \
        for ( ; n < n_end; ++n) \
            expect_tag_skip_imp((ptr), n); \
    } while (0)
#define test_crc            (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_crc_imp
#define test_crc2           (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_crc2_imp

void test_onefile(WCHAR* filename)
{
    PATTRINFO pattrinfo;
    DWORD num;

    if (!pSdbFreeFileAttributes)
    {
        hdll = LoadLibraryA("apphelp.dll");
        pSdbTagToString = (void *)GetProcAddress(hdll, "SdbTagToString");
        pSdbGetFileAttributes = (void *)GetProcAddress(hdll, "SdbGetFileAttributes");
        pSdbFreeFileAttributes = (void *)GetProcAddress(hdll, "SdbFreeFileAttributes");
    }

    if (pSdbGetFileAttributes(filename, &pattrinfo, &num))
    {
        if (pattrinfo[16].flags == ATTRIBUTE_AVAILABLE)
        {
            if (pattrinfo[16].type != TAG_MODULE_TYPE)//SdbpSetAttrFail(&attr_info[16]); /* TAG_MODULE_TYPE (1: WIN16?) (3: WIN32?) (WIN64?), Win32VersionValue? */)
                printf("FAIL TAG_MODULE_TYPE (%S)\n", filename);
            if (pattrinfo[16].dwattr != 3 && pattrinfo[16].dwattr != 2)
                printf("TAG_MODULE_TYPE(%S): %d\n", filename, pattrinfo[16].dwattr);    // C:\Program Files (x86)\Windows Kits\8.1\Lib\win7\stub512.com
            if (pattrinfo[16].dwattr == 2)
            {
                printf("TAG_MODULE_TYPE(%S): %d, %d\n", filename, pattrinfo[16].dwattr, pattrinfo[0].dwattr);
            }
        }

        if (pattrinfo[27].flags == ATTRIBUTE_AVAILABLE)
        {
            if (pattrinfo[27].type != TAG_EXE_WRAPPER)
                printf("FAIL TAG_EXE_WRAPPER (%S)\n", filename);
            if (pattrinfo[27].dwattr != 0)
                printf("TAG_EXE_WRAPPER(%S): %d\n", filename, pattrinfo[27].dwattr);
        }

        pSdbFreeFileAttributes(pattrinfo);
    }
}

static void test_crc_imp(size_t len, DWORD expected)
{
    static const WCHAR path[] = {'t','e','s','t','x','x','.','e','x','e',0};
    static char crc_test[] = {4, 4, 4, 4, 1, 1, 1, 1, 4, 4, 4, 4, 2, 2, 2, 2};

    PATTRINFO pattrinfo = (PATTRINFO)0xdead;
    DWORD num = 333;
    BOOL ret;

    test_create_file_imp(L"testxx.exe", crc_test, len);
    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    winetest_ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    winetest_ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    winetest_ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword_imp(pattrinfo, TAG_CHECKSUM, expected);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);
}

static void test_crc2_imp(DWORD len, int fill, DWORD expected)
{
    static const WCHAR path[] = {'t','e','s','t','x','x','.','e','x','e',0};

    PATTRINFO pattrinfo = (PATTRINFO)0xdead;
    DWORD num = 333;
    BOOL ret;
    size_t n;
    char* crc_test = malloc(len);
    for (n = 0; n < len; ++n)
        crc_test[n] = (char)(fill ? fill : n);

    test_create_file_imp(L"testxx.exe", crc_test, len);
    free(crc_test);
    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    winetest_ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    winetest_ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    winetest_ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword_imp(pattrinfo, TAG_SIZE, len);
        expect_tag_dword_imp(pattrinfo, TAG_CHECKSUM, expected);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);
}



static void test_ApplicationAttributes(void)
{
    static const WCHAR path[] = {'t','e','s','t','x','x','.','e','x','e',0};
    static const WCHAR PRODUCT_VERSION[] = {'1','.','0','.','0','.','1',0};
    static const WCHAR FILE_DESCRIPTION[] = {'F','i','l','e','D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR COMPANY_NAME[] = {'C','o','m','p','a','n','y','N','a','m','e',0};
    static const WCHAR PRODUCT_NAME[] = {'P','r','o','d','u','c','t','N','a','m','e',0};
    static const WCHAR FILE_VERSION[] = {'1','.','0','.','0','.','0',0};
    static const WCHAR ORIGINAL_FILENAME[] = {'O','r','i','g','i','n','a','l','F','i','l','e','n','a','m','e',0};
    static const WCHAR INTERNAL_NAME[] = {'I','n','t','e','r','n','a','l','N','a','m','e',0};
    static const WCHAR LEGAL_COPYRIGHT[] = {'L','e','g','a','l','C','o','p','y','r','i','g','h','t',0};
    static const WCHAR EXPORT_NAME[] = {'T','e','S','t','2','.','e','x','e',0};
    static const WCHAR OS2_DESCRIPTION[] = {'M','O','D',' ','D','E','S','C','R','I','P','T','I','O','N',' ','H','E','R','E',0};
    static const WCHAR OS2_EXPORT_NAME[] = {'T','E','S','T','M','O','D','.','h','X','x',0};
    static const WCHAR OS2_DESCRIPTION_broken[] = {'Z',0};
    static const WCHAR OS2_EXPORT_NAME_broken[] = {'E',0};

    PATTRINFO pattrinfo = (PATTRINFO)0xdead;
    DWORD num = 333;
    BOOL ret;

    /* ensure the file is not there. */
    DeleteFileA("testxx.exe");
    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret == FALSE, "expected SdbGetFileAttributes to fail.\n");
    ok(pattrinfo == (PATTRINFO)0xdead, "expected the pointer not to change.\n");
    ok(num == 333, "expected the number of items not to change.\n");
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    /* Test a file with as much features as possible */
    test_create_exe(L"testxx.exe", 0);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");

    //for (UINT n = 0; n < num; ++n)
    //{
    //    trace("%S\n", pSdbTagToString(pattrinfo[n].type));
    //}

    switch (num)
    {
    case 26:
        // 2k3
        g_AttrInfoSize = 26;
        break;
    case 28:
        // Win7+ (and maybe vista, but who cares about that?)
        g_AttrInfoSize = 28;
        break;
    default:
        ok(0, "Unknown attrinfo size: %u\n", num);
        break;
    }

    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0x800);
        expect_tag_dword(pattrinfo, TAG_CHECKSUM, 0x178bd629);
        expect_tag_qword(pattrinfo, TAG_BIN_FILE_VERSION, 0x1000000000000ull);
        expect_tag_qword(pattrinfo, TAG_BIN_PRODUCT_VERSION, 0x1000000000001ull);
        expect_tag_str(pattrinfo, TAG_PRODUCT_VERSION, PRODUCT_VERSION);
        expect_tag_str(pattrinfo, TAG_FILE_DESCRIPTION, FILE_DESCRIPTION);
        expect_tag_str(pattrinfo, TAG_COMPANY_NAME, COMPANY_NAME);
        expect_tag_str(pattrinfo, TAG_PRODUCT_NAME, PRODUCT_NAME);
        expect_tag_str(pattrinfo, TAG_FILE_VERSION, FILE_VERSION);
        expect_tag_str(pattrinfo, TAG_ORIGINAL_FILENAME, ORIGINAL_FILENAME);
        expect_tag_str(pattrinfo, TAG_INTERNAL_NAME, INTERNAL_NAME);
        expect_tag_str(pattrinfo, TAG_LEGAL_COPYRIGHT, LEGAL_COPYRIGHT);
        expect_tag_dword(pattrinfo, TAG_VERDATEHI, 0x1d1a019);
        expect_tag_dword(pattrinfo, TAG_VERDATELO, 0xac754c50);
        expect_tag_dword(pattrinfo, TAG_VERFILEOS, VOS__WINDOWS32);
        expect_tag_dword(pattrinfo, TAG_VERFILETYPE, VFT_APP);
        expect_tag_dword(pattrinfo, TAG_MODULE_TYPE, 0x3); /* Win32 */
        expect_tag_dword(pattrinfo, TAG_PE_CHECKSUM, 0xBAAD);
        expect_tag_dword(pattrinfo, TAG_LINKER_VERSION, 0x40002);
        expect_tag_skip(pattrinfo, TAG_16BIT_DESCRIPTION);
        expect_tag_skip(pattrinfo, TAG_16BIT_MODULE_NAME);
        expect_tag_qword(pattrinfo, TAG_UPTO_BIN_FILE_VERSION, 0x1000000000000ull);
        expect_tag_qword(pattrinfo, TAG_UPTO_BIN_PRODUCT_VERSION, 0x1000000000001ull);
        expect_tag_dword(pattrinfo, TAG_LINK_DATE, 0x12345);
        expect_tag_dword(pattrinfo, TAG_UPTO_LINK_DATE, 0x12345);
        expect_tag_str(pattrinfo, TAG_EXPORT_NAME, EXPORT_NAME);
        expect_tag_dword(pattrinfo, TAG_VER_LANGUAGE, 0xffff);
        expect_tag_dword(pattrinfo, TAG_EXE_WRAPPER, 0x0);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);


    /* Disable resource and exports */
    test_create_exe(L"testxx.exe", 1);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0x800);
        expect_tag_dword(pattrinfo, TAG_CHECKSUM, 0xea7caffd);
        //expect_tag_skip_range(pattrinfo, 2, 16);
        expect_tag_dword(pattrinfo, TAG_MODULE_TYPE, 0x3); /* Win32 */
        expect_tag_dword(pattrinfo, TAG_PE_CHECKSUM, 0xBAAD);
        expect_tag_dword(pattrinfo, TAG_LINKER_VERSION, 0x40002);
        //expect_tag_skip_range(pattrinfo, 19, 23);
        expect_tag_dword(pattrinfo, TAG_LINK_DATE, 0x12345);
        expect_tag_dword(pattrinfo, TAG_UPTO_LINK_DATE, 0x12345);
        expect_tag_skip(pattrinfo, TAG_EXPORT_NAME);
        expect_tag_empty(pattrinfo, TAG_VER_LANGUAGE);
        expect_tag_dword(pattrinfo, TAG_EXE_WRAPPER, 0x0);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    /* A file with just 'MZ' */
    test_create_file(L"testxx.exe", "MZ", 2);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0x2);
        expect_tag_dword(pattrinfo, TAG_CHECKSUM, 0);
        //expect_tag_skip_range(pattrinfo, 2, 16);
        expect_tag_dword(pattrinfo, TAG_MODULE_TYPE, 0x1);
        //expect_tag_skip_range(pattrinfo, 17, 26);
        expect_tag_empty(pattrinfo, TAG_VER_LANGUAGE);
        expect_tag_skip(pattrinfo, TAG_EXE_WRAPPER);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    /* Empty file */
    test_create_file(L"testxx.exe", NULL, 0);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0);
        //expect_tag_skip_range(pattrinfo, 1, 26);
        expect_tag_empty(pattrinfo, TAG_VER_LANGUAGE);
        expect_tag_skip(pattrinfo, TAG_EXE_WRAPPER);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    /* minimal NE executable */
    test_create_ne(L"testxx.exe", 0);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0xa8);
        expect_tag_dword(pattrinfo, TAG_CHECKSUM, 0xf2abe4e9);
        //expect_tag_skip_range(pattrinfo, 2, 16);
        expect_tag_dword(pattrinfo, TAG_MODULE_TYPE, 0x2);
        expect_tag_skip(pattrinfo, TAG_PE_CHECKSUM);
        expect_tag_skip(pattrinfo, TAG_LINKER_VERSION);
        expect_tag_str(pattrinfo, TAG_16BIT_DESCRIPTION, OS2_DESCRIPTION);
        expect_tag_str(pattrinfo, TAG_16BIT_MODULE_NAME, OS2_EXPORT_NAME);
        //expect_tag_skip_range(pattrinfo, 21, 26);
        expect_tag_empty(pattrinfo, TAG_VER_LANGUAGE);
        expect_tag_skip(pattrinfo, TAG_EXE_WRAPPER);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    /* NE executable with description / module name pointers zero, to show they are always used */
    test_create_ne(L"testxx.exe", 1);

    ret = pSdbGetFileAttributes(path, &pattrinfo, &num);
    ok(ret != FALSE, "expected SdbGetFileAttributes to succeed.\n");
    ok(pattrinfo != (PATTRINFO)0xdead, "expected a valid pointer.\n");
    ok(num == g_AttrInfoSize, "expected %u items, got %d.\n", g_AttrInfoSize, num);

    if (num == g_AttrInfoSize && ret)
    {
        expect_tag_dword(pattrinfo, TAG_SIZE, 0xa8);
        expect_tag_dword(pattrinfo, TAG_CHECKSUM, 0xddcbe4c9);
        //expect_tag_skip_range(pattrinfo, 2, 16);
        expect_tag_dword(pattrinfo, TAG_MODULE_TYPE, 0x2);
        expect_tag_skip(pattrinfo, TAG_PE_CHECKSUM);
        expect_tag_skip(pattrinfo, TAG_LINKER_VERSION);
        expect_tag_str(pattrinfo, TAG_16BIT_DESCRIPTION, OS2_DESCRIPTION_broken);   /* the 'Z' from 'MZ' */
        expect_tag_str(pattrinfo, TAG_16BIT_MODULE_NAME, OS2_EXPORT_NAME_broken);   /* the 'E' from 'NE' */
        //expect_tag_skip_range(pattrinfo, 21, 26);
        expect_tag_empty(pattrinfo, TAG_VER_LANGUAGE);
        expect_tag_skip(pattrinfo, TAG_EXE_WRAPPER);
    }
    if (ret)
        pSdbFreeFileAttributes(pattrinfo);

    test_crc(1, 0);
    test_crc(2, 0);
    test_crc(3, 0);
    test_crc(4, 0x2020202);
    test_crc(5, 0x2020202);
    test_crc(6, 0x2020202);
    test_crc(7, 0x2020202);
    test_crc(8, 0x81818181);
    test_crc(9, 0x81818181);
    test_crc(10, 0x81818181);
    test_crc(11, 0x81818181);
    test_crc(12, 0xc2c2c2c2);
    test_crc(16, 0x62626262);

    /* This seems to be the cutoff point */
    test_crc2(0xffc, 4, 0xfbfbfcfc);
    test_crc2(0xffc, 8, 0x7070717);
    test_crc2(0xffc, 0xcc, 0xc8eba002);
    test_crc2(0xffc, 0, 0x4622028d);

    test_crc2(0x1000, 4, 0x80);
    test_crc2(0x1000, 8, 0x8787878f);
    test_crc2(0x1000, 0xcc, 0x4adc3667);
    test_crc2(0x1000, 0, 0xa3108044);

    /* Here is another cutoff point */
    test_crc2(0x11fc, 4, 0x80);
    test_crc2(0x11fc, 8, 0x8787878f);
    test_crc2(0x11fc, 0xcc, 0x4adc3667);
    test_crc2(0x11fc, 0, 0xf03e0800);

    test_crc2(0x1200, 4, 0x80);
    test_crc2(0x1200, 8, 0x8787878f);
    test_crc2(0x1200, 0xcc, 0x4adc3667);
    test_crc2(0x1200, 0, 0xa3108044);

    /* After that, it stays the same for all sizes */
    test_crc2(0xf000, 4, 0x80);
    test_crc2(0xf000, 8, 0x8787878f);
    test_crc2(0xf000, 0xcc, 0x4adc3667);
    test_crc2(0xf000, 0, 0xa3108044);


    DeleteFileA("testxx.exe");
}

/* Showing that SdbGetAppPatchDir returns HRESULT */
static void test_SdbGetAppPatchDir(void)
{
    WCHAR Buffer[MAX_PATH];
    HRESULT hr, expect_hr;
    int n;


    _SEH2_TRY
    {
        hr = pSdbGetAppPatchDir(NULL, NULL, 0);
        ok_hex(hr, S_FALSE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Some versions accept it, some don't */
        trace("SdbGetAppPatchDir did not handle a NULL pointer very gracefully.\n");
    }
    _SEH2_END;



    memset(Buffer, 0xbb, sizeof(Buffer));
    hr = pSdbGetAppPatchDir(NULL, Buffer, 0);
    if (g_WinVersion < WINVER_WIN7)
        expect_hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    else if (g_WinVersion < WINVER_WIN10)
        expect_hr = S_OK;
    else
        expect_hr = S_FALSE;
    ok_hex(hr, expect_hr);

    if (g_WinVersion < WINVER_WIN7)
        expect_hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    else if (g_WinVersion < WINVER_WIN10)
        expect_hr = S_OK;
    else
        expect_hr = TRUE;

    memset(Buffer, 0xbb, sizeof(Buffer));
    hr = pSdbGetAppPatchDir(NULL, Buffer, 1);
    ok_hex(hr, expect_hr);


    for (n = 2; n < _countof(Buffer) - 1; ++n)
    {
        memset(Buffer, 0xbb, sizeof(Buffer));
        hr = pSdbGetAppPatchDir(NULL, Buffer, n);
        ok(Buffer[n] == 0xbbbb, "Expected SdbGetAppPatchDir to leave WCHAR at %d untouched, was: %d\n",
           n, Buffer[n]);
        ok(hr == S_OK || hr == expect_hr, "Expected S_OK or 0x%x, was: 0x%x (at %d)\n", expect_hr, hr, n);
    }
}
START_TEST(apphelp)
{
    //SetEnvironmentVariable("SHIM_DEBUG_LEVEL", "4");
    //SetEnvironmentVariable("DEBUGCHANNEL", "+apphelp");
    silence_debug_output();

    hdll = LoadLibraryA("apphelp.dll");
    g_WinVersion = get_module_version(hdll);
    trace("Detected apphelp.dll version: 0x%x\n", g_WinVersion);

#define RESOLVE(fnc)    do { p##fnc = (void *) GetProcAddress(hdll, #fnc); ok(!!p##fnc, #fnc " not found.\n"); } while (0)
    RESOLVE(ApphelpCheckShellObject);
    RESOLVE(SdbTagToString);
    RESOLVE(SdbGUIDToString);
    RESOLVE(SdbIsNullGUID);
    RESOLVE(SdbGetStandardDatabaseGUID);
    RESOLVE(SdbGetFileAttributes);
    RESOLVE(SdbFreeFileAttributes);
    RESOLVE(SdbGetAppPatchDir);
#undef RESOLVE

    test_ApphelpCheckShellObject();
    test_GuidFunctions();
    test_ApplicationAttributes();
    test_SdbTagToString();
    test_SdbTagToStringAllTags();
    if (pSdbGetAppPatchDir)
        test_SdbGetAppPatchDir();
}
