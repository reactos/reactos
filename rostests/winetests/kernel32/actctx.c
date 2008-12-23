/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include <winbase.h>
#include <windef.h>
#include <winnt.h>
#include <winternl.h>
#include <winnls.h>
#include <stdio.h>

static BOOL   (WINAPI *pActivateActCtx)(HANDLE,ULONG_PTR*);
static HANDLE (WINAPI *pCreateActCtxW)(PCACTCTXW);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD,ULONG_PTR);
static BOOL   (WINAPI *pFindActCtxSectionStringW)(DWORD,const GUID *,ULONG,LPCWSTR,PACTCTX_SECTION_KEYED_DATA);
static BOOL   (WINAPI *pGetCurrentActCtx)(HANDLE *);
static BOOL   (WINAPI *pIsDebuggerPresent)(void);
static BOOL   (WINAPI *pQueryActCtxW)(DWORD,HANDLE,PVOID,ULONG,PVOID,SIZE_T,SIZE_T*);
static VOID   (WINAPI *pReleaseActCtx)(HANDLE);

static const char* strw(LPCWSTR x)
{
    static char buffer[1024];
    char*       p = buffer;

    if (!x) return "(nil)";
    else while ((*p++ = *x++));
    return buffer;
}

static const char manifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\">"
"</assemblyIdentity>"
"<dependency>"
"<dependentAssembly>"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"x86\">"
"</assemblyIdentity>"
"</dependentAssembly>"
"</dependency>"
"</assembly>";

static const char manifest3[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\""
" publicKeyToken=\"6595b6414666f1df\" />"
"<file name=\"testlib.dll\">"
"<windowClass>wndClass</windowClass>"
"</file>"
"</assembly>";

static const char manifest4[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\">"
"</assemblyIdentity>"
"<dependency>"
"<dependentAssembly>"
"<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" "
    "version=\"6.0.1.0\" processorArchitecture=\"x86\" publicKeyToken=\"6595b64144ccf1df\">"
"</assemblyIdentity>"
"</dependentAssembly>"
"</dependency>"
"</assembly>";

static const char testdep_manifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"x86\"/>"
"</assembly>";

static const char testdep_manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"x86\" />"
"<file name=\"testlib.dll\"></file>"
"<file name=\"testlib2.dll\" hash=\"63c978c2b53d6cf72b42fb7308f9af12ab19ec53\" hashalg=\"SHA1\" />"
"</assembly>";

static const char testdep_manifest3[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\"> "
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"x86\"/>"
"<file name=\"testlib.dll\"/>"
"<file name=\"testlib2.dll\" hash=\"63c978c2b53d6cf72b42fb7308f9af12ab19ec53\" hashalg=\"SHA1\">"
"<windowClass>wndClass</windowClass>"
"<windowClass>wndClass2</windowClass>"
"</file>"
"</assembly>";

static const char wrong_manifest1[] =
"<assembly manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char wrong_manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char wrong_manifest3[] =
"<assembly test=\"test\" xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char wrong_manifest4[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"<test></test>"
"</assembly>";

static const char wrong_manifest5[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>"
"<test></test>";

static const char wrong_manifest6[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v5\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char wrong_manifest7[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"x86\" />"
"<file name=\"testlib.dll\" hash=\"63c978c2b53d6cf72b42fb7308f9af12ab19ec5\" hashalg=\"SHA1\" />"
"</assembly>";

static const char wrong_manifest8[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"<file></file>"
"</assembly>";

static const char wrong_depmanifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.4\" processorArchitecture=\"x86\" />"
"</assembly>";

static const WCHAR testlib_dll[] =
    {'t','e','s','t','l','i','b','.','d','l','l',0};
static const WCHAR testlib2_dll[] =
    {'t','e','s','t','l','i','b','2','.','d','l','l',0};
static const WCHAR wndClassW[] =
    {'w','n','d','C','l','a','s','s',0};
static const WCHAR wndClass2W[] =
    {'w','n','d','C','l','a','s','s','2',0};
static const WCHAR acr_manifest[] =
    {'a','c','r','.','m','a','n','i','f','e','s','t',0};

static WCHAR app_dir[MAX_PATH], exe_path[MAX_PATH], work_dir[MAX_PATH], work_dir_subdir[MAX_PATH];
static WCHAR app_manifest_path[MAX_PATH], manifest_path[MAX_PATH], depmanifest_path[MAX_PATH];

static int strcmp_aw(LPCWSTR strw, const char *stra)
{
    WCHAR buf[1024];

    if (!stra) return 1;
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, sizeof(buf)/sizeof(WCHAR));
    return lstrcmpW(strw, buf);
}

static DWORD strlen_aw(const char *str)
{
    return MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0) - 1;
}

static BOOL create_manifest_file(const char *filename, const char *manifest, int manifest_len,
                                 const char *depfile, const char *depmanifest)
{
    DWORD size;
    HANDLE file;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, filename, -1, path, MAX_PATH );
    GetFullPathNameW(path, sizeof(manifest_path)/sizeof(WCHAR), manifest_path, NULL);

    if (manifest_len == -1)
        manifest_len = strlen(manifest);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;
    WriteFile(file, manifest, manifest_len, &size, NULL);
    CloseHandle(file);

    if (depmanifest)
    {
        MultiByteToWideChar( CP_ACP, 0, depfile, -1, path, MAX_PATH );
        GetFullPathNameW(path, sizeof(depmanifest_path)/sizeof(WCHAR), depmanifest_path, NULL);
        file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
        if(file == INVALID_HANDLE_VALUE)
            return FALSE;
        WriteFile(file, depmanifest, strlen(depmanifest), &size, NULL);
        CloseHandle(file);
    }
    return TRUE;
}

static BOOL create_wide_manifest(const char *filename, const char *manifest, BOOL fBOM, BOOL fReverse)
{
    WCHAR *wmanifest = HeapAlloc(GetProcessHeap(), 0, (strlen(manifest)+2) * sizeof(WCHAR));
    BOOL ret;
    int offset = (fBOM ? 0 : 1);

    MultiByteToWideChar(CP_ACP, 0, manifest, -1, &wmanifest[1], (strlen(manifest)+1));
    wmanifest[0] = 0xfeff;
    if (fReverse)
    {
        size_t i;
        for (i = 0; i < strlen(manifest)+1; i++)
            wmanifest[i] = (wmanifest[i] << 8) | ((wmanifest[i] >> 8) & 0xff);
    }
    ret = create_manifest_file(filename, (char *)&wmanifest[offset], (strlen(manifest)+1-offset) * sizeof(WCHAR), NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wmanifest);
    return ret;
}

typedef struct {
    ULONG format_version;
    ULONG assembly_cnt;
    ULONG root_manifest_type;
    LPWSTR root_manifest_path;
    ULONG root_config_type;
    ULONG app_dir_type;
    LPCWSTR app_dir;
} detailed_info_t;

static const detailed_info_t detailed_info0 = {
    0, 0, 0, NULL, 0, 0, NULL
};

static const detailed_info_t detailed_info1 = {
    1, 1, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    work_dir,
};

static const detailed_info_t detailed_info1_child = {
    1, 1, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, app_manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    app_dir,
};

static const detailed_info_t detailed_info2 = {
    1, 2, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    work_dir,
};

static void test_detailed_info(HANDLE handle, const detailed_info_t *exinfo)
{
    ACTIVATION_CONTEXT_DETAILED_INFORMATION detailed_info_tmp, *detailed_info;
    SIZE_T size, exsize, retsize;
    BOOL b;

    exsize = sizeof(ACTIVATION_CONTEXT_DETAILED_INFORMATION)
        + (exinfo->root_manifest_path ? (lstrlenW(exinfo->root_manifest_path)+1)*sizeof(WCHAR):0)
        + (exinfo->app_dir ? (lstrlenW(exinfo->app_dir)+1)*sizeof(WCHAR) : 0);

    if(exsize != sizeof(ACTIVATION_CONTEXT_DETAILED_INFORMATION)) {
        size = 0xdeadbeef;
        b = pQueryActCtxW(0, handle, NULL,
                          ActivationContextDetailedInformation, &detailed_info_tmp,
                          sizeof(detailed_info_tmp), &size);
        ok(!b, "QueryActCtx succeeded\n");
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());
        ok(size == exsize, "size=%ld, expected %ld\n", size, exsize);
    }else {
        size = sizeof(ACTIVATION_CONTEXT_DETAILED_INFORMATION);
    }

    detailed_info = HeapAlloc(GetProcessHeap(), 0, size);
    memset(detailed_info, 0xfe, size);
    b = pQueryActCtxW(0, handle, NULL,
                      ActivationContextDetailedInformation, detailed_info,
                      size, &retsize);
    ok(b, "QueryActCtx failed: %u\n", GetLastError());
    ok(retsize == exsize, "size=%ld, expected %ld\n", retsize, exsize);

    ok(detailed_info->dwFlags == 0, "detailed_info->dwFlags=%x\n", detailed_info->dwFlags);
    ok(detailed_info->ulFormatVersion == exinfo->format_version,
       "detailed_info->ulFormatVersion=%u, expected %u\n", detailed_info->ulFormatVersion,
       exinfo->format_version);
    ok(detailed_info->ulAssemblyCount == exinfo->assembly_cnt,
       "detailed_info->ulAssemblyCount=%u, expected %u\n", detailed_info->ulAssemblyCount,
       exinfo->assembly_cnt);
    ok(detailed_info->ulRootManifestPathType == exinfo->root_manifest_type,
       "detailed_info->ulRootManifestPathType=%u, expected %u\n",
       detailed_info->ulRootManifestPathType, exinfo->root_manifest_type);
    ok(detailed_info->ulRootManifestPathChars ==
       (exinfo->root_manifest_path ? lstrlenW(exinfo->root_manifest_path) : 0),
       "detailed_info->ulRootManifestPathChars=%u, expected %u\n",
       detailed_info->ulRootManifestPathChars,
       exinfo->root_manifest_path ?lstrlenW(exinfo->root_manifest_path) : 0);
    ok(detailed_info->ulRootConfigurationPathType == exinfo->root_config_type,
       "detailed_info->ulRootConfigurationPathType=%u, expected %u\n",
       detailed_info->ulRootConfigurationPathType, exinfo->root_config_type);
    ok(detailed_info->ulRootConfigurationPathChars == 0,
       "detailed_info->ulRootConfigurationPathChars=%d\n", detailed_info->ulRootConfigurationPathChars);
    ok(detailed_info->ulAppDirPathType == exinfo->app_dir_type,
       "detailed_info->ulAppDirPathType=%u, expected %u\n", detailed_info->ulAppDirPathType,
       exinfo->app_dir_type);
    ok(detailed_info->ulAppDirPathChars == (exinfo->app_dir ? lstrlenW(exinfo->app_dir) : 0),
       "detailed_info->ulAppDirPathChars=%u, expected %u\n",
       detailed_info->ulAppDirPathChars, exinfo->app_dir ? lstrlenW(exinfo->app_dir) : 0);
    if(exinfo->root_manifest_path) {
        ok(detailed_info->lpRootManifestPath != NULL, "detailed_info->lpRootManifestPath == NULL\n");
        if(detailed_info->lpRootManifestPath)
            ok(!lstrcmpiW(detailed_info->lpRootManifestPath, exinfo->root_manifest_path),
               "unexpected detailed_info->lpRootManifestPath\n");
    }else {
        ok(detailed_info->lpRootManifestPath == NULL, "detailed_info->lpRootManifestPath != NULL\n");
    }
    ok(detailed_info->lpRootConfigurationPath == NULL,
       "detailed_info->lpRootConfigurationPath=%p\n", detailed_info->lpRootConfigurationPath);
    if(exinfo->app_dir) {
        ok(detailed_info->lpAppDirPath != NULL, "detailed_info->lpAppDirPath == NULL\n");
        if(detailed_info->lpAppDirPath)
            ok(!lstrcmpiW(exinfo->app_dir, detailed_info->lpAppDirPath),
               "unexpected detailed_info->lpAppDirPath\n%s\n",strw(detailed_info->lpAppDirPath));
    }else {
        ok(detailed_info->lpAppDirPath == NULL, "detailed_info->lpAppDirPath != NULL\n");
    }

    HeapFree(GetProcessHeap(), 0, detailed_info);
}

typedef struct {
    ULONG flags;
/*    ULONG manifest_path_type; FIXME */
    LPCWSTR manifest_path;
    LPCSTR encoded_assembly_id;
    BOOL has_assembly_dir;
} info_in_assembly;

static const info_in_assembly manifest1_info = {
    1, manifest_path,
    "Wine.Test,type=\"win32\",version=\"1.0.0.0\"",
    FALSE
};

static const info_in_assembly manifest1_child_info = {
    1, app_manifest_path,
    "Wine.Test,type=\"win32\",version=\"1.0.0.0\"",
    FALSE
};

static const info_in_assembly manifest2_info = {
    1, manifest_path,
    "Wine.Test,type=\"win32\",version=\"1.2.3.4\"",
    FALSE
};

static const info_in_assembly manifest3_info = {
    1, manifest_path,
    "Wine.Test,publicKeyToken=\"6595b6414666f1df\",type=\"win32\",version=\"1.2.3.4\"",
    FALSE
};

static const info_in_assembly manifest4_info = {
    1, manifest_path,
    "Wine.Test,type=\"win32\",version=\"1.2.3.4\"",
    FALSE
};

static const info_in_assembly depmanifest1_info = {
    0x10, depmanifest_path,
    "testdep,processorArchitecture=\"x86\","
    "type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly depmanifest2_info = {
    0x10, depmanifest_path,
    "testdep,processorArchitecture=\"x86\","
    "type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly depmanifest3_info = {
    0x10, depmanifest_path,
    "testdep,processorArchitecture=\"x86\",type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly manifest_comctrl_info = {
    0, NULL, NULL, TRUE /* These values may differ between Windows installations */
};

static void test_info_in_assembly(HANDLE handle, DWORD id, const info_in_assembly *exinfo)
{
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info, info_tmp;
    SIZE_T size, exsize;
    ULONG len;
    BOOL b;

    exsize = sizeof(ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION);
    if (exinfo->manifest_path) exsize += (lstrlenW(exinfo->manifest_path)+1) * sizeof(WCHAR);
    if (exinfo->encoded_assembly_id) exsize += (strlen_aw(exinfo->encoded_assembly_id) + 1) * sizeof(WCHAR);

    size = 0xdeadbeef;
    b = pQueryActCtxW(0, handle, &id,
                      AssemblyDetailedInformationInActivationContext, &info_tmp,
                      sizeof(info_tmp), &size);
    ok(!b, "QueryActCtx succeeded\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());

    ok(size >= exsize, "size=%lu, expected %lu\n", size, exsize);

    if (size == 0xdeadbeef)
    {
        skip("bad size\n");
        return;
    }

    info = HeapAlloc(GetProcessHeap(), 0, size);
    memset(info, 0xfe, size);

    size = 0xdeadbeef;
    b = pQueryActCtxW(0, handle, &id,
                      AssemblyDetailedInformationInActivationContext, info, size, &size);
    ok(b, "QueryActCtx failed: %u\n", GetLastError());
    if (!exinfo->manifest_path)
        exsize += info->ulManifestPathLength + sizeof(WCHAR);
    if (!exinfo->encoded_assembly_id)
        exsize += info->ulEncodedAssemblyIdentityLength + sizeof(WCHAR);
    if (exinfo->has_assembly_dir)
        exsize += info->ulAssemblyDirectoryNameLength + sizeof(WCHAR);
    ok(size == exsize, "size=%lu, expected %lu\n", size, exsize);

    if (0)  /* FIXME: flags meaning unknown */
    {
        ok((info->ulFlags) == exinfo->flags, "info->ulFlags = %x, expected %x\n",
           info->ulFlags, exinfo->flags);
    }
    if(exinfo->encoded_assembly_id) {
        len = strlen_aw(exinfo->encoded_assembly_id)*sizeof(WCHAR);
        ok(info->ulEncodedAssemblyIdentityLength == len,
           "info->ulEncodedAssemblyIdentityLength = %u, expected %u\n",
           info->ulEncodedAssemblyIdentityLength, len);
    } else {
        ok(info->ulEncodedAssemblyIdentityLength != 0,
           "info->ulEncodedAssemblyIdentityLength == 0\n");
    }
    ok(info->ulManifestPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
       "info->ulManifestPathType = %x\n", info->ulManifestPathType);
    if(exinfo->manifest_path) {
        len = lstrlenW(exinfo->manifest_path)*sizeof(WCHAR);
        ok(info->ulManifestPathLength == len, "info->ulManifestPathLength = %u, expected %u\n",
           info->ulManifestPathLength, len);
    } else {
        ok(info->ulManifestPathLength != 0, "info->ulManifestPathLength == 0\n");
    }

    ok(info->ulPolicyPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE,
       "info->ulPolicyPathType = %x\n", info->ulPolicyPathType);
    ok(info->ulPolicyPathLength == 0,
       "info->ulPolicyPathLength = %u, expected 0\n", info->ulPolicyPathLength);
    ok(info->ulMetadataSatelliteRosterIndex == 0, "info->ulMetadataSatelliteRosterIndex = %x\n",
       info->ulMetadataSatelliteRosterIndex);
    ok(info->ulManifestVersionMajor == 1,"info->ulManifestVersionMajor = %x\n",
       info->ulManifestVersionMajor);
    ok(info->ulManifestVersionMinor == 0, "info->ulManifestVersionMinor = %x\n",
       info->ulManifestVersionMinor);
    ok(info->ulPolicyVersionMajor == 0, "info->ulPolicyVersionMajor = %x\n",
       info->ulPolicyVersionMajor);
    ok(info->ulPolicyVersionMinor == 0, "info->ulPolicyVersionMinor = %x\n",
       info->ulPolicyVersionMinor);
    if(exinfo->has_assembly_dir)
        ok(info->ulAssemblyDirectoryNameLength != 0,
           "info->ulAssemblyDirectoryNameLength == 0\n");
    else
        ok(info->ulAssemblyDirectoryNameLength == 0,
           "info->ulAssemblyDirectoryNameLength != 0\n");

    ok(info->lpAssemblyEncodedAssemblyIdentity != NULL,
       "info->lpAssemblyEncodedAssemblyIdentity == NULL\n");
    if(info->lpAssemblyEncodedAssemblyIdentity && exinfo->encoded_assembly_id) {
        ok(!strcmp_aw(info->lpAssemblyEncodedAssemblyIdentity, exinfo->encoded_assembly_id),
           "unexpected info->lpAssemblyEncodedAssemblyIdentity %s / %s\n",
           strw(info->lpAssemblyEncodedAssemblyIdentity), exinfo->encoded_assembly_id);
    }
    if(exinfo->manifest_path) {
        ok(info->lpAssemblyManifestPath != NULL, "info->lpAssemblyManifestPath == NULL\n");
        if(info->lpAssemblyManifestPath)
            ok(!lstrcmpiW(info->lpAssemblyManifestPath, exinfo->manifest_path),
               "unexpected info->lpAssemblyManifestPath\n");
    }else {
        ok(info->lpAssemblyManifestPath != NULL, "info->lpAssemblyManifestPath == NULL\n");
    }

    ok(info->lpAssemblyPolicyPath == NULL, "info->lpAssemblyPolicyPath != NULL\n");
    if(info->lpAssemblyPolicyPath)
        ok(*(WORD*)info->lpAssemblyPolicyPath == 0, "info->lpAssemblyPolicyPath is not empty\n");
    if(exinfo->has_assembly_dir)
        ok(info->lpAssemblyDirectoryName != NULL, "info->lpAssemblyDirectoryName == NULL\n");
    else
        ok(info->lpAssemblyDirectoryName == NULL, "info->lpAssemblyDirectoryName = %s\n",
           strw(info->lpAssemblyDirectoryName));
    HeapFree(GetProcessHeap(), 0, info);
}

static void test_file_info(HANDLE handle, ULONG assid, ULONG fileid, LPCWSTR filename)
{
    ASSEMBLY_FILE_DETAILED_INFORMATION *info, info_tmp;
    ACTIVATION_CONTEXT_QUERY_INDEX index = {assid, fileid};
    SIZE_T size, exsize;
    BOOL b;

    exsize = sizeof(ASSEMBLY_FILE_DETAILED_INFORMATION)
        +(lstrlenW(filename)+1)*sizeof(WCHAR);

    size = 0xdeadbeef;
    b = pQueryActCtxW(0, handle, &index,
                      FileInformationInAssemblyOfAssemblyInActivationContext, &info_tmp,
                      sizeof(info_tmp), &size);
    ok(!b, "QueryActCtx succeeded\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());
    ok(size == exsize, "size=%lu, expected %lu\n", size, exsize);

    if(size == 0xdeadbeef)
    {
        skip("bad size\n");
        return;
    }

    info = HeapAlloc(GetProcessHeap(), 0, size);
    memset(info, 0xfe, size);

    b = pQueryActCtxW(0, handle, &index,
                      FileInformationInAssemblyOfAssemblyInActivationContext, info, size, &size);
    ok(b, "QueryActCtx failed: %u\n", GetLastError());
    ok(!size, "size=%lu, expected 0\n", size);

    ok(info->ulFlags == 2, "info->ulFlags=%x, expected 2\n", info->ulFlags);
    ok(info->ulFilenameLength == lstrlenW(filename)*sizeof(WCHAR),
       "info->ulFilenameLength=%u, expected %u*sizeof(WCHAR)\n",
       info->ulFilenameLength, lstrlenW(filename));
    ok(info->ulPathLength == 0, "info->ulPathLength=%u\n", info->ulPathLength);
    ok(info->lpFileName != NULL, "info->lpFileName == NULL\n");
    if(info->lpFileName)
        ok(!lstrcmpiW(info->lpFileName, filename), "unexpected info->lpFileName\n");
    ok(info->lpFilePath == NULL, "info->lpFilePath != NULL\n");
    HeapFree(GetProcessHeap(), 0, info);
}

static HANDLE test_create(const char *file, const char *manifest)
{
    ACTCTXW actctx;
    HANDLE handle;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, file, -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = pCreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "actctx.cbSize=%d\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "actctx.=%d\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "actctx.lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0,
       "actctx.wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "actctx.wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL,
       "actctx.lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "actctx.lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "actctx.lpApplocationName=%p\n",
       actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "actctx.hModule=%p\n", actctx.hModule);

    return handle;
}

static void test_create_and_fail(const char *manifest, const char *depmanifest, int todo)
{
    ACTCTXW actctx;
    HANDLE handle;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, "bad.manifest", -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    create_manifest_file("bad.manifest", manifest, -1, "testdep.manifest", depmanifest);
    handle = pCreateActCtxW(&actctx);
    if (todo) todo_wine
    {
        ok(handle == INVALID_HANDLE_VALUE, "handle != INVALID_HANDLE_VALUE\n");
        ok(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX, "GetLastError == %u\n", GetLastError());
    }
    else
    {
        ok(handle == INVALID_HANDLE_VALUE, "handle != INVALID_HANDLE_VALUE\n");
        ok(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX, "GetLastError == %u\n", GetLastError());
    }
    if (handle != INVALID_HANDLE_VALUE) pReleaseActCtx( handle );
    DeleteFileA("bad.manifest");
    DeleteFileA("testdep.manifest");
}

static void test_create_wide_and_fail(const char *manifest, BOOL fBOM)
{
    ACTCTXW actctx;
    HANDLE handle;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, "bad.manifest", -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    create_wide_manifest("bad.manifest", manifest, fBOM, FALSE);
    handle = pCreateActCtxW(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "handle != INVALID_HANDLE_VALUE\n");
    ok(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX, "GetLastError == %u\n", GetLastError());

    if (handle != INVALID_HANDLE_VALUE) pReleaseActCtx( handle );
    DeleteFileA("bad.manifest");
}

static void test_create_fail(void)
{
    ACTCTXW actctx;
    HANDLE handle;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, "nonexistent.manifest", -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = pCreateActCtxW(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "handle != INVALID_HANDLE_VALUE\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError == %u\n", GetLastError());

    trace("wrong_manifest1\n");
    test_create_and_fail(wrong_manifest1, NULL, 0 );
    trace("wrong_manifest2\n");
    test_create_and_fail(wrong_manifest2, NULL, 0 );
    trace("wrong_manifest3\n");
    test_create_and_fail(wrong_manifest3, NULL, 1 );
    trace("wrong_manifest4\n");
    test_create_and_fail(wrong_manifest4, NULL, 1 );
    trace("wrong_manifest5\n");
    test_create_and_fail(wrong_manifest5, NULL, 0 );
    trace("wrong_manifest6\n");
    test_create_and_fail(wrong_manifest6, NULL, 0 );
    trace("wrong_manifest7\n");
    test_create_and_fail(wrong_manifest7, NULL, 1 );
    trace("wrong_manifest8\n");
    test_create_and_fail(wrong_manifest8, NULL, 0 );
    trace("UTF-16 manifest1 without BOM\n");
    test_create_wide_and_fail(manifest1, FALSE );
    trace("manifest2\n");
    test_create_and_fail(manifest2, NULL, 0 );
    trace("manifest2+depmanifest1\n");
    test_create_and_fail(manifest2, wrong_depmanifest1, 0 );
}

static void test_find_dll_redirection(HANDLE handle, LPCWSTR libname, ULONG exid)
{
    ACTCTX_SECTION_KEYED_DATA data;
    DWORD *p;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    libname, &data);
    ok(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if(!ret)
    {
        skip("couldn't find %s\n",strw(libname));
        return;
    }

    ok(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok(data.lpData != NULL, "data.lpData == NULL\n");
    ok(data.ulLength == 20, "data.ulLength=%u\n", data.ulLength);

    p = data.lpData;
    if(ret && p) todo_wine {
        ok(p[0] == 20 && p[1] == 2 && p[2] == 0 && p[3] == 0 && p[4] == 0,
           "wrong data %u,%u,%u,%u,%u\n",p[0], p[1], p[2], p[3], p[4]);
    }

    ok(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    /* ok(data.ulSectionTotalLength == ??, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength); */
    ok(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    libname, &data);
    ok(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if(!ret)
    {
        skip("couldn't find\n");
        return;
    }

    ok(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok(data.lpData != NULL, "data.lpData == NULL\n");
    ok(data.ulLength == 20, "data.ulLength=%u\n", data.ulLength);
    ok(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    /* ok(data.ulSectionTotalLength == ?? , "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength); */
    ok(data.hActCtx == handle, "data.hActCtx=%p\n", data.hActCtx);
    ok(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    pReleaseActCtx(handle);
}

static void test_find_window_class(HANDLE handle, LPCWSTR clsname, ULONG exid)
{
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    clsname, &data);
    ok(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if(!ret)
    {
        skip("couldn't find\n");
        return;
    }

    ok(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok(data.lpData != NULL, "data.lpData == NULL\n");
    /* ok(data.ulLength == ??, "data.ulLength=%u\n", data.ulLength); */
    ok(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    /* ok(data.ulSectionTotalLength == 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength); FIXME */
    ok(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    clsname, &data);
    ok(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if(!ret)
    {
        skip("couldn't find\n");
        return;
    }

    ok(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok(data.lpData != NULL, "data.lpData == NULL\n");
    /* ok(data.ulLength == ??, "data.ulLength=%u\n", data.ulLength); FIXME */
    ok(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    /* ok(data.ulSectionTotalLength == 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength); FIXME */
    ok(data.hActCtx == handle, "data.hActCtx=%p\n", data.hActCtx);
    ok(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    pReleaseActCtx(handle);
}

static void test_find_string_fail(void)
{
    ACTCTX_SECTION_KEYED_DATA data = {sizeof(data)};
    BOOL ret;

    ret = pFindActCtxSectionStringW(0, NULL, 100, testlib_dll, &data);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_SXS_SECTION_NOT_FOUND, "GetLastError()=%u\n", GetLastError());

    ret = pFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib2_dll, &data);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_SXS_KEY_NOT_FOUND, "GetLastError()=%u\n", GetLastError());

    ret = pFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib_dll, NULL);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%u\n", GetLastError());

    ret = pFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    NULL, &data);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%u\n", GetLastError());

    data.cbSize = 0;
    ret = pFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib_dll, &data);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%u\n", GetLastError());

    data.cbSize = 35;
    ret = pFindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib_dll, &data);
    ok(!ret, "FindActCtxSectionStringW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError()=%u\n", GetLastError());
}


static void test_basic_info(HANDLE handle)
{
    ACTIVATION_CONTEXT_BASIC_INFORMATION basic;
    SIZE_T size;
    BOOL b;

    b = pQueryActCtxW(0, handle, NULL,
                          ActivationContextBasicInformation, &basic,
                          sizeof(basic), &size);

    ok (b,"ActivationContextBasicInformation failed\n");
    ok (size == sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION),"size mismatch\n");
    ok (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
    ok (basic.hActCtx == handle, "unexpected handle\n");

    b = pQueryActCtxW(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX, handle, NULL,
                          ActivationContextBasicInformation, &basic,
                          sizeof(basic), &size);
    if (handle)
    {
        ok (!b,"ActivationContextBasicInformation succeeded\n");
        ok (size == 0,"size mismatch\n");
        ok (GetLastError() == ERROR_INVALID_PARAMETER, "Wrong last error\n");
        ok (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
        ok (basic.hActCtx == handle, "unexpected handle\n");
    }
    else
    {
        ok (b,"ActivationContextBasicInformation failed\n");
        ok (size == sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION),"size mismatch\n");
        ok (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
        ok (basic.hActCtx == handle, "unexpected handle\n");
    }
}

static void test_actctx(void)
{
    ULONG_PTR cookie;
    HANDLE handle;
    BOOL b;

    test_create_fail();

    trace("default actctx\n");

    b = pGetCurrentActCtx(&handle);
    ok(handle == NULL, "handle = %p, expected NULL\n", handle);
    ok(b, "GetCurrentActCtx failed: %u\n", GetLastError());
    if(b) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info0);
        pReleaseActCtx(handle);
    }

    if(!create_manifest_file("test1.manifest", manifest1, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    trace("manifest1\n");

    handle = test_create("test1.manifest", manifest1);
    DeleteFileA("test1.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info1);
        test_info_in_assembly(handle, 1, &manifest1_info);

        if (pIsDebuggerPresent && !pIsDebuggerPresent())
        {
            /* CloseHandle will generate an exception if a debugger is present */
            b = CloseHandle(handle);
            ok(!b, "CloseHandle succeeded\n");
            ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError() == %u\n", GetLastError());
        }

        pReleaseActCtx(handle);
    }

    if(!create_manifest_file("test2.manifest", manifest2, -1, "testdep.manifest", testdep_manifest1)) {
        skip("Could not create manifest file\n");
        return;
    }

    trace("manifest2 depmanifest1\n");

    handle = test_create("test2.manifest", manifest2);
    DeleteFileA("test2.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info2);
        test_info_in_assembly(handle, 1, &manifest2_info);
        test_info_in_assembly(handle, 2, &depmanifest1_info);
        pReleaseActCtx(handle);
    }

    if(!create_manifest_file("test3.manifest", manifest2, -1, "testdep.manifest", testdep_manifest2)) {
        skip("Could not create manifest file\n");
        return;
    }

    trace("manifest2 depmanifest2\n");

    handle = test_create("test3.manifest", manifest2);
    DeleteFileA("test3.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info2);
        test_info_in_assembly(handle, 1, &manifest2_info);
        test_info_in_assembly(handle, 2, &depmanifest2_info);
        test_file_info(handle, 1, 0, testlib_dll);
        test_file_info(handle, 1, 1, testlib2_dll);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 2);
        test_find_dll_redirection(handle, testlib2_dll, 2);
        b = pDeactivateActCtx(0, cookie);
        ok(b, "DeactivateActCtx failed: %u\n", GetLastError());

        pReleaseActCtx(handle);
    }

    trace("manifest2 depmanifest3\n");

    if(!create_manifest_file("test2-3.manifest", manifest2, -1, "testdep.manifest", testdep_manifest3)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test2-3.manifest", manifest2);
    DeleteFileA("test2-3.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info2);
        test_info_in_assembly(handle, 1, &manifest2_info);
        test_info_in_assembly(handle, 2, &depmanifest3_info);
        test_file_info(handle, 1, 0, testlib_dll);
        test_file_info(handle, 1, 1, testlib2_dll);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 2);
        test_find_dll_redirection(handle, testlib2_dll, 2);
        test_find_window_class(handle, wndClassW, 2);
        test_find_window_class(handle, wndClass2W, 2);
        b = pDeactivateActCtx(0, cookie);
        ok(b, "DeactivateActCtx failed: %u\n", GetLastError());

        pReleaseActCtx(handle);
    }

    trace("manifest3\n");

    if(!create_manifest_file("test3.manifest", manifest3, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test3.manifest", manifest3);
    DeleteFileA("test3.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info1);
        test_info_in_assembly(handle, 1, &manifest3_info);
        test_file_info(handle, 0, 0, testlib_dll);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 1);
        test_find_dll_redirection(handle, testlib_dll, 1);
        test_find_string_fail();
        b = pDeactivateActCtx(0, cookie);
        ok(b, "DeactivateActCtx failed: %u\n", GetLastError());

        pReleaseActCtx(handle);
    }

    trace("manifest4\n");

    if(!create_manifest_file("test4.manifest", manifest4, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test4.manifest", manifest4);
    DeleteFileA("test4.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info2);
        test_info_in_assembly(handle, 1, &manifest4_info);
        test_info_in_assembly(handle, 2, &manifest_comctrl_info);
        pReleaseActCtx(handle);
    }

    trace("manifest1 in subdir\n");

    CreateDirectoryW(work_dir_subdir, NULL);
    if (SetCurrentDirectoryW(work_dir_subdir))
    {
        if(!create_manifest_file("..\\test1.manifest", manifest1, -1, NULL, NULL)) {
            skip("Could not create manifest file\n");
            return;
        }
        handle = test_create("..\\test1.manifest", manifest1);
        DeleteFileA("..\\test1.manifest");
        if(handle != INVALID_HANDLE_VALUE) {
            test_basic_info(handle);
            test_detailed_info(handle, &detailed_info1);
            test_info_in_assembly(handle, 1, &manifest1_info);
            pReleaseActCtx(handle);
        }
        SetCurrentDirectoryW(work_dir);
    }
    else
        skip("Couldn't change directory\n");
    RemoveDirectoryW(work_dir_subdir);

    trace("UTF-16 manifest1, with BOM\n");
    if(!create_wide_manifest("test1.manifest", manifest1, TRUE, FALSE)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test1.manifest", manifest1);
    DeleteFileA("test1.manifest");
    if (handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info1);
        test_info_in_assembly(handle, 1, &manifest1_info);
        pReleaseActCtx(handle);
    }

    trace("UTF-16 manifest1, reverse endian, with BOM\n");
    if(!create_wide_manifest("test1.manifest", manifest1, TRUE, TRUE)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test1.manifest", manifest1);
    DeleteFileA("test1.manifest");
    if (handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info1);
        test_info_in_assembly(handle, 1, &manifest1_info);
        pReleaseActCtx(handle);
    }

}

static void test_app_manifest(void)
{
    HANDLE handle;
    BOOL b;

    trace("child process manifest1\n");

    b = pGetCurrentActCtx(&handle);
    ok(handle == NULL, "handle != NULL\n");
    ok(b, "GetCurrentActCtx failed: %u\n", GetLastError());
    if(b) {
        test_basic_info(handle);
        test_detailed_info(handle, &detailed_info1_child);
        test_info_in_assembly(handle, 1, &manifest1_child_info);
        pReleaseActCtx(handle);
    }
}

static void run_child_process(void)
{
    char cmdline[MAX_PATH];
    char path[MAX_PATH];
    char **argv;
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { 0 };

    GetModuleFileNameA(NULL, path, MAX_PATH);
    strcat(path, ".manifest");
    if(!create_manifest_file(path, manifest1, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    si.cb = sizeof(si);
    winetest_get_mainargs( &argv );
    sprintf(cmdline, "\"%s\" %s manifest1", argv[0], argv[1]);
    ok(CreateProcess(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL,
                     &si, &pi) != 0, "Could not create process: %u\n", GetLastError());
    winetest_wait_child_process( pi.hProcess );
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileA(path);
}

static void init_paths(void)
{
    LPWSTR ptr;
    WCHAR last;

    static const WCHAR dot_manifest[] = {'.','M','a','n','i','f','e','s','t',0};
    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR subdir[] = {'T','e','s','t','S','u','b','d','i','r','\\',0};

    GetModuleFileNameW(NULL, exe_path, sizeof(exe_path)/sizeof(WCHAR));
    lstrcpyW(app_dir, exe_path);
    for(ptr=app_dir+lstrlenW(app_dir); *ptr != '\\' && *ptr != '/'; ptr--);
    ptr[1] = 0;

    GetCurrentDirectoryW(MAX_PATH, work_dir);
    last = work_dir[lstrlenW(work_dir) - 1];
    if (last != '\\' && last != '/')
        lstrcatW(work_dir, backslash);
    lstrcpyW(work_dir_subdir, work_dir);
    lstrcatW(work_dir_subdir, subdir);

    GetModuleFileNameW(NULL, app_manifest_path, sizeof(app_manifest_path)/sizeof(WCHAR));
    lstrcpyW(app_manifest_path+lstrlenW(app_manifest_path), dot_manifest);
}

static BOOL init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandle("kernel32");

#define X(f) if (!(p##f = (void*)GetProcAddress(hKernel32, #f))) return FALSE;
    X(ActivateActCtx);
    X(CreateActCtxW);
    X(DeactivateActCtx);
    X(FindActCtxSectionStringW);
    X(GetCurrentActCtx);
    X(IsDebuggerPresent);
    X(QueryActCtxW);
    X(ReleaseActCtx);
#undef X

    return TRUE;
}

START_TEST(actctx)
{
    int argc;
    char **argv;

    argc = winetest_get_mainargs(&argv);

    if (!init_funcs())
    {
        skip("Needed functions are not available\n");
        return;
    }
    init_paths();

    if(argc > 2 && !strcmp(argv[2], "manifest1")) {
        test_app_manifest();
        return;
    }

    test_actctx();
    run_child_process();
}
