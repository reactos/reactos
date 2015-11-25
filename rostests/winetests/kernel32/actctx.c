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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <wine/test.h>
#include <winbase.h>
#include <windef.h>
#include <winnt.h>
#include <wine/winternl.h>
#include <winnls.h>
#include <stdio.h>

#include <oaidl.h>
#include <initguid.h>

static BOOL   (WINAPI *pActivateActCtx)(HANDLE,ULONG_PTR*);
static HANDLE (WINAPI *pCreateActCtxA)(PCACTCTXA);
static HANDLE (WINAPI *pCreateActCtxW)(PCACTCTXW);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD,ULONG_PTR);
static BOOL   (WINAPI *pFindActCtxSectionStringA)(DWORD,const GUID *,ULONG,LPCSTR,PACTCTX_SECTION_KEYED_DATA);
static BOOL   (WINAPI *pFindActCtxSectionStringW)(DWORD,const GUID *,ULONG,LPCWSTR,PACTCTX_SECTION_KEYED_DATA);
static BOOL   (WINAPI *pGetCurrentActCtx)(HANDLE *);
static BOOL   (WINAPI *pIsDebuggerPresent)(void);
static BOOL   (WINAPI *pQueryActCtxW)(DWORD,HANDLE,PVOID,ULONG,PVOID,SIZE_T,SIZE_T*);
static VOID   (WINAPI *pReleaseActCtx)(HANDLE);
static BOOL   (WINAPI *pFindActCtxSectionGuid)(DWORD,const GUID*,ULONG,const GUID*,PACTCTX_SECTION_KEYED_DATA);

static NTSTATUS(NTAPI *pRtlFindActivationContextSectionString)(DWORD,const GUID *,ULONG,PUNICODE_STRING,PACTCTX_SECTION_KEYED_DATA);
static BOOLEAN (NTAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, PCSZ);
static VOID    (NTAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);

static const char* strw(LPCWSTR x)
{
    static char buffer[1024];
    char*       p = buffer;

    if (!x) return "(nil)";
    else while ((*p++ = *x++));
    return buffer;
}

#ifdef __i386__
#define ARCH "x86"
#elif defined __x86_64__
#define ARCH "amd64"
#elif defined __arm__
#define ARCH "arm"
#elif defined __aarch64__
#define ARCH "arm64"
#else
#define ARCH "none"
#endif

static const char manifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.0.0.0\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"</assembly>";

static const char manifest1_1[] =
"<assembly xmlns = \"urn:schemas-microsoft-com:asm.v1\" manifestVersion = \"1.0\">"
"<assemblyIdentity version = \"1.0.0.0\" name = \"Wine.Test\" type = \"win32\"></assemblyIdentity>"
"</assembly>";

static const char manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\">"
"</assemblyIdentity>"
"<dependency>"
"<dependentAssembly>"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"" ARCH "\">"
"</assemblyIdentity>"
"</dependentAssembly>"
"</dependency>"
"</assembly>";

DEFINE_GUID(IID_CoTest,    0x12345678, 0x1234, 0x5678, 0x12, 0x34, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33);
DEFINE_GUID(IID_CoTest2,   0x12345678, 0x1234, 0x5678, 0x12, 0x34, 0x11, 0x11, 0x22, 0x22, 0x33, 0x34);
DEFINE_GUID(CLSID_clrclass,0x22345678, 0x1234, 0x5678, 0x12, 0x34, 0x11, 0x11, 0x22, 0x22, 0x33, 0x33);
DEFINE_GUID(IID_TlibTest,  0x99999999, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
DEFINE_GUID(IID_TlibTest2, 0x99999999, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x56);
DEFINE_GUID(IID_TlibTest3, 0x99999999, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x57);
DEFINE_GUID(IID_TlibTest4, 0x99999999, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x58);
DEFINE_GUID(IID_Iifaceps,  0x66666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
DEFINE_GUID(IID_Ibifaceps, 0x66666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x57);
DEFINE_GUID(IID_Iifaceps2, 0x76666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
DEFINE_GUID(IID_Iifaceps3, 0x86666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
DEFINE_GUID(IID_Iiface,    0x96666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55);
DEFINE_GUID(IID_PS32,      0x66666666, 0x8888, 0x7777, 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x56);

static const char manifest3[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\""
" publicKeyToken=\"6595b6414666f1df\" />"
"<description />"
"<file name=\"testlib.dll\">"
"<windowClass>wndClass</windowClass>"
"    <comClass description=\"Test com class\""
"              clsid=\"{12345678-1234-5678-1234-111122223333}\""
"              tlbid=\"{99999999-8888-7777-6666-555555555555}\""
"              threadingModel=\"Neutral\""
"              progid=\"ProgId.ProgId\""
"              miscStatus=\"cantlinkinside\""
"              miscStatusIcon=\"recomposeonresize\""
"              miscStatusContent=\"insideout\""
"              miscStatusThumbnail=\"alignable\""
"              miscStatusDocPrint=\"simpleframe,setclientsitefirst\""
"    >"
"        <progid>ProgId.ProgId.1</progid>"
"        <progid>ProgId.ProgId.2</progid>"
"        <progid>ProgId.ProgId.3</progid>"
"        <progid>ProgId.ProgId.4</progid>"
"        <progid>ProgId.ProgId.5</progid>"
"        <progid>ProgId.ProgId.6</progid>"
"    </comClass>"
"    <comClass clsid=\"{12345678-1234-5678-1234-111122223334}\" threadingModel=\"Neutral\" >"
"        <progid>ProgId.ProgId.7</progid>"
"    </comClass>"
"    <comInterfaceProxyStub "
"        name=\"Iifaceps\""
"        tlbid=\"{99999999-8888-7777-6666-555555555558}\""
"        iid=\"{66666666-8888-7777-6666-555555555555}\""
"        proxyStubClsid32=\"{66666666-8888-7777-6666-555555555556}\""
"        threadingModel=\"Free\""
"        numMethods=\"10\""
"        baseInterface=\"{66666666-8888-7777-6666-555555555557}\""
"    />"
"</file>"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps2\""
"        tlbid=\"{99999999-8888-7777-6666-555555555558}\""
"        iid=\"{76666666-8888-7777-6666-555555555555}\""
"        proxyStubClsid32=\"{66666666-8888-7777-6666-555555555556}\""
"        numMethods=\"10\""
"        baseInterface=\"{66666666-8888-7777-6666-555555555557}\""
"    />"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps3\""
"        tlbid=\"{99999999-8888-7777-6666-555555555558}\""
"        iid=\"{86666666-8888-7777-6666-555555555555}\""
"        numMethods=\"10\""
"        baseInterface=\"{66666666-8888-7777-6666-555555555557}\""
"    />"
"    <clrSurrogate "
"        clsid=\"{96666666-8888-7777-6666-555555555555}\""
"        name=\"testsurrogate\""
"        runtimeVersion=\"v2.0.50727\""
"    />"
"    <clrClass "
"        clsid=\"{22345678-1234-5678-1234-111122223333}\""
"        name=\"clrclass\""
"        progid=\"clrprogid\""
"        description=\"test description\""
"        tlbid=\"{99999999-8888-7777-6666-555555555555}\""
"        runtimeVersion=\"1.2.3.4\""
"        threadingModel=\"Neutral\""
"    >"
"        <progid>clrprogid.1</progid>"
"        <progid>clrprogid.2</progid>"
"        <progid>clrprogid.3</progid>"
"        <progid>clrprogid.4</progid>"
"        <progid>clrprogid.5</progid>"
"        <progid>clrprogid.6</progid>"
"    </clrClass>"
"</assembly>";

static const char manifest_wndcls1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"testdep1\" type=\"win32\" processorArchitecture=\"" ARCH "\"/>"
"<file name=\"testlib1.dll\">"
"<windowClass versioned=\"yes\">wndClass1</windowClass>"
"<windowClass>wndClass2</windowClass>"
" <typelib tlbid=\"{99999999-8888-7777-6666-555555555558}\" version=\"1.0\" helpdir=\"\" />"
"</file>"
"<file name=\"testlib1_2.dll\" />"
"</assembly>";

static const char manifest_wndcls2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"4.3.2.1\"  name=\"testdep2\" type=\"win32\" processorArchitecture=\"" ARCH "\" />"
"<file name=\"testlib2.dll\">"
" <windowClass versioned=\"no\">wndClass3</windowClass>"
" <windowClass>wndClass4</windowClass>"
" <typelib tlbid=\"{99999999-8888-7777-6666-555555555555}\" version=\"1.0\" helpdir=\"help\" resourceid=\"409\""
"          flags=\"HiddeN,CoNTROL,rESTRICTED\" />"
" <typelib tlbid=\"{99999999-8888-7777-6666-555555555556}\" version=\"1.0\" helpdir=\"help1\" resourceid=\"409\" />"
" <typelib tlbid=\"{99999999-8888-7777-6666-555555555557}\" version=\"1.0\" helpdir=\"\" />"
"</file>"
"<file name=\"testlib2_2.dll\" />"
"</assembly>";

static const char manifest_wndcls_main[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\" />"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep1\" version=\"1.2.3.4\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep2\" version=\"4.3.2.1\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"</assembly>";

static const char manifest4[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\">"
"</assemblyIdentity>"
"<dependency>"
"<dependentAssembly>"
"<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" "
    "version=\"6.0.1.0\" processorArchitecture=\"" ARCH "\" publicKeyToken=\"6595b64144ccf1df\">"
"</assemblyIdentity>"
"</dependentAssembly>"
"</dependency>"
"</assembly>";

static const char manifest5[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\">"
"</assemblyIdentity>"
"<dependency>"
"    <dependentAssembly dependencyType=\"preRequisite\" allowDelayedBinding=\"true\">"
"        <assemblyIdentity name=\"Missing.Assembly\" version=\"1.0.0.0\" />"
"    </dependentAssembly>"
"</dependency>"
"</assembly>";

static const char testdep_manifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"" ARCH "\"/>"
"</assembly>";

static const char testdep_manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"" ARCH "\" />"
"<file name=\"testlib.dll\"></file>"
"<file name=\"testlib2.dll\" hash=\"63c978c2b53d6cf72b42fb7308f9af12ab19ec53\" hashalg=\"SHA1\" />"
"</assembly>";

static const char testdep_manifest3[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\"> "
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"" ARCH "\"/>"
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
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.3\" processorArchitecture=\"" ARCH "\" />"
"<file name=\"testlib.dll\" hash=\"63c978c2b53d6cf72b42fb7308f9af12ab19ec5\" hashalg=\"SHA1\" />"
"</assembly>";

static const char wrong_manifest8[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\"></assemblyIdentity>"
"<file></file>"
"</assembly>";

static const char wrong_depmanifest1[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity type=\"win32\" name=\"testdep\" version=\"6.5.4.4\" processorArchitecture=\"" ARCH "\" />"
"</assembly>";

static const WCHAR testlib_dll[] =
    {'t','e','s','t','l','i','b','.','d','l','l',0};
static const WCHAR testlib2_dll[] =
    {'t','e','s','t','l','i','b','2','.','d','l','l',0};
static const WCHAR wndClassW[] =
    {'w','n','d','C','l','a','s','s',0};
static const WCHAR wndClass1W[] =
    {'w','n','d','C','l','a','s','s','1',0};
static const WCHAR wndClass2W[] =
    {'w','n','d','C','l','a','s','s','2',0};
static const WCHAR wndClass3W[] =
    {'w','n','d','C','l','a','s','s','3',0};

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
    ULONG assembly_cnt_min;
    ULONG assembly_cnt_max;
    ULONG root_manifest_type;
    LPWSTR root_manifest_path;
    ULONG root_config_type;
    ULONG app_dir_type;
    LPCWSTR app_dir;
} detailed_info_t;

static const detailed_info_t detailed_info0 = {
    0, 0, 0, 0, NULL, 0, 0, NULL
};

static const detailed_info_t detailed_info1 = {
    1, 1, 1, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    work_dir,
};

static const detailed_info_t detailed_info1_child = {
    1, 1, 1, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, app_manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    app_dir,
};

/* On Vista+, there's an extra assembly for Microsoft.Windows.Common-Controls.Resources */
static const detailed_info_t detailed_info2 = {
    1, 2, 3, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE, manifest_path,
    ACTIVATION_CONTEXT_PATH_TYPE_NONE, ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
    work_dir,
};

static void test_detailed_info(HANDLE handle, const detailed_info_t *exinfo, int line)
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
        ok_(__FILE__, line)(!b, "QueryActCtx succeeded\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());
        ok_(__FILE__, line)(size == exsize, "size=%ld, expected %ld\n", size, exsize);
    }else {
        size = sizeof(ACTIVATION_CONTEXT_DETAILED_INFORMATION);
    }

    detailed_info = HeapAlloc(GetProcessHeap(), 0, size);
    memset(detailed_info, 0xfe, size);
    b = pQueryActCtxW(0, handle, NULL,
                      ActivationContextDetailedInformation, detailed_info,
                      size, &retsize);
    ok_(__FILE__, line)(b, "QueryActCtx failed: %u\n", GetLastError());
    ok_(__FILE__, line)(retsize == exsize, "size=%ld, expected %ld\n", retsize, exsize);

    ok_(__FILE__, line)(detailed_info->dwFlags == 0, "detailed_info->dwFlags=%x\n", detailed_info->dwFlags);
    ok_(__FILE__, line)(detailed_info->ulFormatVersion == exinfo->format_version,
       "detailed_info->ulFormatVersion=%u, expected %u\n", detailed_info->ulFormatVersion,
       exinfo->format_version);
    ok_(__FILE__, line)(exinfo->assembly_cnt_min <= detailed_info->ulAssemblyCount &&
       detailed_info->ulAssemblyCount <= exinfo->assembly_cnt_max,
       "detailed_info->ulAssemblyCount=%u, expected between %u and %u\n", detailed_info->ulAssemblyCount,
       exinfo->assembly_cnt_min, exinfo->assembly_cnt_max);
    ok_(__FILE__, line)(detailed_info->ulRootManifestPathType == exinfo->root_manifest_type,
       "detailed_info->ulRootManifestPathType=%u, expected %u\n",
       detailed_info->ulRootManifestPathType, exinfo->root_manifest_type);
    ok_(__FILE__, line)(detailed_info->ulRootManifestPathChars ==
       (exinfo->root_manifest_path ? lstrlenW(exinfo->root_manifest_path) : 0),
       "detailed_info->ulRootManifestPathChars=%u, expected %u\n",
       detailed_info->ulRootManifestPathChars,
       exinfo->root_manifest_path ?lstrlenW(exinfo->root_manifest_path) : 0);
    ok_(__FILE__, line)(detailed_info->ulRootConfigurationPathType == exinfo->root_config_type,
       "detailed_info->ulRootConfigurationPathType=%u, expected %u\n",
       detailed_info->ulRootConfigurationPathType, exinfo->root_config_type);
    ok_(__FILE__, line)(detailed_info->ulRootConfigurationPathChars == 0,
       "detailed_info->ulRootConfigurationPathChars=%d\n", detailed_info->ulRootConfigurationPathChars);
    ok_(__FILE__, line)(detailed_info->ulAppDirPathType == exinfo->app_dir_type,
       "detailed_info->ulAppDirPathType=%u, expected %u\n", detailed_info->ulAppDirPathType,
       exinfo->app_dir_type);
    ok_(__FILE__, line)(detailed_info->ulAppDirPathChars == (exinfo->app_dir ? lstrlenW(exinfo->app_dir) : 0),
       "detailed_info->ulAppDirPathChars=%u, expected %u\n",
       detailed_info->ulAppDirPathChars, exinfo->app_dir ? lstrlenW(exinfo->app_dir) : 0);
    if(exinfo->root_manifest_path) {
        ok_(__FILE__, line)(detailed_info->lpRootManifestPath != NULL, "detailed_info->lpRootManifestPath == NULL\n");
        if(detailed_info->lpRootManifestPath)
            ok_(__FILE__, line)(!lstrcmpiW(detailed_info->lpRootManifestPath, exinfo->root_manifest_path),
               "unexpected detailed_info->lpRootManifestPath\n");
    }else {
        ok_(__FILE__, line)(detailed_info->lpRootManifestPath == NULL, "detailed_info->lpRootManifestPath != NULL\n");
    }
    ok_(__FILE__, line)(detailed_info->lpRootConfigurationPath == NULL,
       "detailed_info->lpRootConfigurationPath=%p\n", detailed_info->lpRootConfigurationPath);
    if(exinfo->app_dir) {
        ok_(__FILE__, line)(detailed_info->lpAppDirPath != NULL, "detailed_info->lpAppDirPath == NULL\n");
        if(detailed_info->lpAppDirPath)
            ok_(__FILE__, line)(!lstrcmpiW(exinfo->app_dir, detailed_info->lpAppDirPath),
               "unexpected detailed_info->lpAppDirPath\n%s\n",strw(detailed_info->lpAppDirPath));
    }else {
        ok_(__FILE__, line)(detailed_info->lpAppDirPath == NULL, "detailed_info->lpAppDirPath != NULL\n");
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
    "testdep,processorArchitecture=\"" ARCH "\","
    "type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly depmanifest2_info = {
    0x10, depmanifest_path,
    "testdep,processorArchitecture=\"" ARCH "\","
    "type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly depmanifest3_info = {
    0x10, depmanifest_path,
    "testdep,processorArchitecture=\"" ARCH "\",type=\"win32\",version=\"6.5.4.3\"",
    TRUE
};

static const info_in_assembly manifest_comctrl_info = {
    0, NULL, NULL, TRUE /* These values may differ between Windows installations */
};

static void test_info_in_assembly(HANDLE handle, DWORD id, const info_in_assembly *exinfo, int line)
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
    ok_(__FILE__, line)(!b, "QueryActCtx succeeded\n");
    ok_(__FILE__, line)(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());

    ok_(__FILE__, line)(size >= exsize, "size=%lu, expected %lu\n", size, exsize);

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
    ok_(__FILE__, line)(b, "QueryActCtx failed: %u\n", GetLastError());
    if (!exinfo->manifest_path)
        exsize += info->ulManifestPathLength + sizeof(WCHAR);
    if (!exinfo->encoded_assembly_id)
        exsize += info->ulEncodedAssemblyIdentityLength + sizeof(WCHAR);
    if (exinfo->has_assembly_dir)
        exsize += info->ulAssemblyDirectoryNameLength + sizeof(WCHAR);
    ok_(__FILE__, line)(size == exsize, "size=%lu, expected %lu\n", size, exsize);

    if (0)  /* FIXME: flags meaning unknown */
    {
        ok_(__FILE__, line)((info->ulFlags) == exinfo->flags, "info->ulFlags = %x, expected %x\n",
           info->ulFlags, exinfo->flags);
    }
    if(exinfo->encoded_assembly_id) {
        len = strlen_aw(exinfo->encoded_assembly_id)*sizeof(WCHAR);
        ok_(__FILE__, line)(info->ulEncodedAssemblyIdentityLength == len,
           "info->ulEncodedAssemblyIdentityLength = %u, expected %u\n",
           info->ulEncodedAssemblyIdentityLength, len);
    } else {
        ok_(__FILE__, line)(info->ulEncodedAssemblyIdentityLength != 0,
           "info->ulEncodedAssemblyIdentityLength == 0\n");
    }
    ok_(__FILE__, line)(info->ulManifestPathType == ACTIVATION_CONTEXT_PATH_TYPE_WIN32_FILE,
       "info->ulManifestPathType = %x\n", info->ulManifestPathType);
    if(exinfo->manifest_path) {
        len = lstrlenW(exinfo->manifest_path)*sizeof(WCHAR);
        ok_(__FILE__, line)(info->ulManifestPathLength == len, "info->ulManifestPathLength = %u, expected %u\n",
           info->ulManifestPathLength, len);
    } else {
        ok_(__FILE__, line)(info->ulManifestPathLength != 0, "info->ulManifestPathLength == 0\n");
    }

    ok_(__FILE__, line)(info->ulPolicyPathType == ACTIVATION_CONTEXT_PATH_TYPE_NONE,
       "info->ulPolicyPathType = %x\n", info->ulPolicyPathType);
    ok_(__FILE__, line)(info->ulPolicyPathLength == 0,
       "info->ulPolicyPathLength = %u, expected 0\n", info->ulPolicyPathLength);
    ok_(__FILE__, line)(info->ulMetadataSatelliteRosterIndex == 0, "info->ulMetadataSatelliteRosterIndex = %x\n",
       info->ulMetadataSatelliteRosterIndex);
    ok_(__FILE__, line)(info->ulManifestVersionMajor == 1,"info->ulManifestVersionMajor = %x\n",
       info->ulManifestVersionMajor);
    ok_(__FILE__, line)(info->ulManifestVersionMinor == 0, "info->ulManifestVersionMinor = %x\n",
       info->ulManifestVersionMinor);
    ok_(__FILE__, line)(info->ulPolicyVersionMajor == 0, "info->ulPolicyVersionMajor = %x\n",
       info->ulPolicyVersionMajor);
    ok_(__FILE__, line)(info->ulPolicyVersionMinor == 0, "info->ulPolicyVersionMinor = %x\n",
       info->ulPolicyVersionMinor);
    if(exinfo->has_assembly_dir)
        ok_(__FILE__, line)(info->ulAssemblyDirectoryNameLength != 0,
           "info->ulAssemblyDirectoryNameLength == 0\n");
    else
        ok_(__FILE__, line)(info->ulAssemblyDirectoryNameLength == 0,
           "info->ulAssemblyDirectoryNameLength != 0\n");

    ok_(__FILE__, line)(info->lpAssemblyEncodedAssemblyIdentity != NULL,
       "info->lpAssemblyEncodedAssemblyIdentity == NULL\n");
    if(info->lpAssemblyEncodedAssemblyIdentity && exinfo->encoded_assembly_id) {
        ok_(__FILE__, line)(!strcmp_aw(info->lpAssemblyEncodedAssemblyIdentity, exinfo->encoded_assembly_id),
           "unexpected info->lpAssemblyEncodedAssemblyIdentity %s / %s\n",
           strw(info->lpAssemblyEncodedAssemblyIdentity), exinfo->encoded_assembly_id);
    }
    if(exinfo->manifest_path) {
        ok_(__FILE__, line)(info->lpAssemblyManifestPath != NULL, "info->lpAssemblyManifestPath == NULL\n");
        if(info->lpAssemblyManifestPath)
            ok_(__FILE__, line)(!lstrcmpiW(info->lpAssemblyManifestPath, exinfo->manifest_path),
               "unexpected info->lpAssemblyManifestPath\n");
    }else {
        ok_(__FILE__, line)(info->lpAssemblyManifestPath != NULL, "info->lpAssemblyManifestPath == NULL\n");
    }

    ok_(__FILE__, line)(info->lpAssemblyPolicyPath == NULL, "info->lpAssemblyPolicyPath != NULL\n");
    if(info->lpAssemblyPolicyPath)
        ok_(__FILE__, line)(*(WORD*)info->lpAssemblyPolicyPath == 0, "info->lpAssemblyPolicyPath is not empty\n");
    if(exinfo->has_assembly_dir)
        ok_(__FILE__, line)(info->lpAssemblyDirectoryName != NULL, "info->lpAssemblyDirectoryName == NULL\n");
    else
        ok_(__FILE__, line)(info->lpAssemblyDirectoryName == NULL, "info->lpAssemblyDirectoryName = %s\n",
           strw(info->lpAssemblyDirectoryName));
    HeapFree(GetProcessHeap(), 0, info);
}

static void test_file_info(HANDLE handle, ULONG assid, ULONG fileid, LPCWSTR filename, int line)
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
    ok_(__FILE__, line)(!b, "QueryActCtx succeeded\n");
    ok_(__FILE__, line)(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetLastError() = %u\n", GetLastError());
    ok_(__FILE__, line)(size == exsize, "size=%lu, expected %lu\n", size, exsize);

    if(size == 0xdeadbeef)
    {
        skip("bad size\n");
        return;
    }

    info = HeapAlloc(GetProcessHeap(), 0, size);
    memset(info, 0xfe, size);

    b = pQueryActCtxW(0, handle, &index,
                      FileInformationInAssemblyOfAssemblyInActivationContext, info, size, &size);
    ok_(__FILE__, line)(b, "QueryActCtx failed: %u\n", GetLastError());
    ok_(__FILE__, line)(!size, "size=%lu, expected 0\n", size);

    ok_(__FILE__, line)(info->ulFlags == 2, "info->ulFlags=%x, expected 2\n", info->ulFlags);
    ok_(__FILE__, line)(info->ulFilenameLength == lstrlenW(filename)*sizeof(WCHAR),
       "info->ulFilenameLength=%u, expected %u*sizeof(WCHAR)\n",
       info->ulFilenameLength, lstrlenW(filename));
    ok_(__FILE__, line)(info->ulPathLength == 0, "info->ulPathLength=%u\n", info->ulPathLength);
    ok_(__FILE__, line)(info->lpFileName != NULL, "info->lpFileName == NULL\n");
    if(info->lpFileName)
        ok_(__FILE__, line)(!lstrcmpiW(info->lpFileName, filename), "unexpected info->lpFileName\n");
    ok_(__FILE__, line)(info->lpFilePath == NULL, "info->lpFilePath != NULL\n");
    HeapFree(GetProcessHeap(), 0, info);
}

static HANDLE test_create(const char *file)
{
    ACTCTXW actctx;
    HANDLE handle;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, file, -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = pCreateActCtxW(&actctx);
    /* to be tested outside of this helper, including last error */
    if (handle == INVALID_HANDLE_VALUE) return handle;

    ok(actctx.cbSize == sizeof(actctx), "actctx.cbSize=%d\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "actctx.dwFlags=%d\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "actctx.lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0,
       "actctx.wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "actctx.wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL,
       "actctx.lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "actctx.lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "actctx.lpApplicationName=%p\n",
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

struct strsection_header
{
    DWORD magic;
    ULONG size;
    DWORD unk1[3];
    ULONG count;
    ULONG index_offset;
    DWORD unk2[2];
    ULONG global_offset;
    ULONG global_len;
};

struct string_index
{
    ULONG hash;
    ULONG name_offset;
    ULONG name_len;
    ULONG data_offset;
    ULONG data_len;
    ULONG rosterindex;
};

struct guidsection_header
{
    DWORD magic;
    ULONG size;
    DWORD unk[3];
    ULONG count;
    ULONG index_offset;
    DWORD unk2;
    ULONG names_offset;
    ULONG names_len;
};

struct guid_index
{
    GUID  guid;
    ULONG data_offset;
    ULONG data_len;
    ULONG rosterindex;
};

struct wndclass_redirect_data
{
    ULONG size;
    DWORD res;
    ULONG name_len;
    ULONG name_offset;  /* versioned name offset */
    ULONG module_len;
    ULONG module_offset;/* container name offset */
};

struct dllredirect_data
{
    ULONG size;
    ULONG unk;
    DWORD res[3];
};

struct tlibredirect_data
{
    ULONG  size;
    DWORD  res;
    ULONG  name_len;
    ULONG  name_offset;
    LANGID langid;
    WORD   flags;
    ULONG  help_len;
    ULONG  help_offset;
    WORD   major_version;
    WORD   minor_version;
};

struct progidredirect_data
{
    ULONG size;
    DWORD reserved;
    ULONG clsid_offset;
};

static void test_find_dll_redirection(HANDLE handle, LPCWSTR libname, ULONG exid, int line)
{
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    libname, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if (!ret) return;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(data.ulLength == 20, "data.ulLength=%u\n", data.ulLength);

    if (data.lpData)
    {
        struct dllredirect_data *dlldata = (struct dllredirect_data*)data.lpData;
        ok_(__FILE__, line)(dlldata->size == data.ulLength, "got wrong size %d\n", dlldata->size);
        ok_(__FILE__, line)(dlldata->unk == 2, "got wrong field value %d\n", dlldata->unk);
        ok_(__FILE__, line)(dlldata->res[0] == 0, "got wrong res[0] value %d\n", dlldata->res[0]);
        ok_(__FILE__, line)(dlldata->res[1] == 0, "got wrong res[1] value %d\n", dlldata->res[1]);
        ok_(__FILE__, line)(dlldata->res[2] == 0, "got wrong res[2] value %d\n", dlldata->res[2]);
    }

    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    libname, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionStringW failed: %u\n", GetLastError());
    if (!ret) return;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(data.ulLength == 20, "data.ulLength=%u\n", data.ulLength);
    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == handle, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    pReleaseActCtx(handle);
}

static void test_find_window_class(HANDLE handle, LPCWSTR clsname, ULONG exid, int line)
{
    struct wndclass_redirect_data *wnddata;
    struct strsection_header *header;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    clsname, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionStringW failed: %u, class %s\n", GetLastError(),
        wine_dbgstr_w(clsname));
    if (!ret) return;

    header = (struct strsection_header*)data.lpSectionBase;
    wnddata = (struct wndclass_redirect_data*)data.lpData;

    ok_(__FILE__, line)(header->magic == 0x64487353, "got wrong magic 0x%08x\n", header->magic);
    ok_(__FILE__, line)(header->count > 0, "got count %d\n", header->count);
    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(wnddata->size == sizeof(*wnddata), "got %d for header size\n", wnddata->size);
    if (data.lpData && wnddata->size == sizeof(*wnddata))
    {
        static const WCHAR verW[] = {'6','.','5','.','4','.','3','!',0};
        WCHAR buff[50];
        WCHAR *ptr;
        ULONG len;

        ok_(__FILE__, line)(wnddata->res == 0, "got reserved as %d\n", wnddata->res);
        /* redirect class name (versioned or not) is stored just after header data */
        ok_(__FILE__, line)(wnddata->name_offset == wnddata->size, "got name offset as %d\n", wnddata->name_offset);
        ok_(__FILE__, line)(wnddata->module_len > 0, "got module name length as %d\n", wnddata->module_len);

        /* expected versioned name */
        lstrcpyW(buff, verW);
        lstrcatW(buff, clsname);
        ptr = (WCHAR*)((BYTE*)wnddata + wnddata->name_offset);
        ok_(__FILE__, line)(!lstrcmpW(ptr, buff), "got wrong class name %s, expected %s\n", wine_dbgstr_w(ptr), wine_dbgstr_w(buff));
        ok_(__FILE__, line)(lstrlenW(ptr)*sizeof(WCHAR) == wnddata->name_len,
            "got wrong class name length %d, expected %d\n", wnddata->name_len, lstrlenW(ptr));

        /* data length is simply header length + string data length including nulls */
        len = wnddata->size + wnddata->name_len + wnddata->module_len + 2*sizeof(WCHAR);
        ok_(__FILE__, line)(data.ulLength == len, "got wrong data length %d, expected %d\n", data.ulLength, len);

        if (data.ulSectionTotalLength > wnddata->module_offset)
        {
            WCHAR *modulename, *sectionptr;

            /* just compare pointers */
            modulename = (WCHAR*)((BYTE*)wnddata + wnddata->size + wnddata->name_len + sizeof(WCHAR));
            sectionptr = (WCHAR*)((BYTE*)data.lpSectionBase + wnddata->module_offset);
            ok_(__FILE__, line)(modulename == sectionptr, "got wrong name offset %p, expected %p\n", sectionptr, modulename);
        }
    }

    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    clsname, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionStringW failed: %u, class %s\n", GetLastError(),
        wine_dbgstr_w(clsname));
    if (!ret) return;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(data.ulLength > 0, "data.ulLength=%u\n", data.ulLength);
    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n", data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == handle, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
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


static void test_basic_info(HANDLE handle, int line)
{
    ACTIVATION_CONTEXT_BASIC_INFORMATION basic;
    SIZE_T size;
    BOOL b;

    b = pQueryActCtxW(QUERY_ACTCTX_FLAG_NO_ADDREF, handle, NULL,
                          ActivationContextBasicInformation, &basic,
                          sizeof(basic), &size);

    ok_(__FILE__, line) (b,"ActivationContextBasicInformation failed\n");
    ok_(__FILE__, line) (size == sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION),"size mismatch\n");
    ok_(__FILE__, line) (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
    ok_(__FILE__, line) (basic.hActCtx == handle, "unexpected handle\n");

    b = pQueryActCtxW(QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX |
                      QUERY_ACTCTX_FLAG_NO_ADDREF, handle, NULL,
                          ActivationContextBasicInformation, &basic,
                          sizeof(basic), &size);
    if (handle)
    {
        ok_(__FILE__, line) (!b,"ActivationContextBasicInformation succeeded\n");
        ok_(__FILE__, line) (size == 0,"size mismatch\n");
        ok_(__FILE__, line) (GetLastError() == ERROR_INVALID_PARAMETER, "Wrong last error\n");
        ok_(__FILE__, line) (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
        ok_(__FILE__, line) (basic.hActCtx == handle, "unexpected handle\n");
    }
    else
    {
        ok_(__FILE__, line) (b,"ActivationContextBasicInformation failed\n");
        ok_(__FILE__, line) (size == sizeof(ACTIVATION_CONTEXT_BASIC_INFORMATION),"size mismatch\n");
        ok_(__FILE__, line) (basic.dwFlags == 0, "unexpected flags %x\n",basic.dwFlags);
        ok_(__FILE__, line) (basic.hActCtx == handle, "unexpected handle\n");
    }
}

enum comclass_threadingmodel {
    ThreadingModel_Apartment = 1,
    ThreadingModel_Free      = 2,
    ThreadingModel_No        = 3,
    ThreadingModel_Both      = 4,
    ThreadingModel_Neutral   = 5
};

enum comclass_miscfields {
    MiscStatus          = 1,
    MiscStatusIcon      = 2,
    MiscStatusContent   = 4,
    MiscStatusThumbnail = 8,
    MiscStatusDocPrint  = 16
};

struct comclassredirect_data {
    ULONG size;
    BYTE  res;
    BYTE  miscmask;
    BYTE  res1[2];
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

struct clrclass_data {
    ULONG size;
    DWORD res[2];
    ULONG module_len;
    ULONG module_offset;
    ULONG name_len;
    ULONG name_offset;
    ULONG version_len;
    ULONG version_offset;
    DWORD res2[2];
};

static void test_find_com_redirection(HANDLE handle, const GUID *clsid, const GUID *tlid, const WCHAR *progid, ULONG exid, int line)
{
    struct comclassredirect_data *comclass, *comclass2;
    ACTCTX_SECTION_KEYED_DATA data, data2;
    struct guidsection_header *header;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionGuid(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                                    clsid, &data);
    if (!ret)
    {
        skip("failed for guid %s\n", wine_dbgstr_guid(clsid));
        return;
    }
    ok_(__FILE__, line)(ret, "FindActCtxSectionGuid failed: %u\n", GetLastError());

    comclass = (struct comclassredirect_data*)data.lpData;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(comclass->size == sizeof(*comclass), "got %d for header size\n", comclass->size);
    if (data.lpData && comclass->size == sizeof(*comclass))
    {
        WCHAR *ptr;
        ULONG len;

        ok_(__FILE__, line)(comclass->res == 0, "got res as %d\n", comclass->res);
        ok_(__FILE__, line)(comclass->res1[0] == 0, "got res1[0] as %02x\n", comclass->res1[0]);
        ok_(__FILE__, line)(comclass->res1[1] == 0, "got res1[1] as %02x\n", comclass->res1[1]);
        ok_(__FILE__, line)(comclass->model == ThreadingModel_Neutral, "got model %d\n", comclass->model);
        ok_(__FILE__, line)(IsEqualGUID(&comclass->clsid, clsid), "got wrong clsid %s\n", wine_dbgstr_guid(&comclass->clsid));
        ok_(__FILE__, line)(IsEqualGUID(&comclass->clsid2, clsid), "got wrong clsid2 %s\n", wine_dbgstr_guid(&comclass->clsid2));
        if (tlid)
            ok_(__FILE__, line)(IsEqualGUID(&comclass->tlid, tlid), "got wrong tlid %s\n", wine_dbgstr_guid(&comclass->tlid));
        ok_(__FILE__, line)(comclass->name_len > 0, "got modulename len %d\n", comclass->name_len);

        if (progid)
        {
            len = comclass->size + comclass->clrdata_len;
            ok_(__FILE__, line)(comclass->progid_offset == len, "got progid offset %d, expected %d\n", comclass->progid_offset, len);
        }
        else
            ok_(__FILE__, line)(comclass->progid_offset == 0, "got progid offset %d, expected 0\n", comclass->progid_offset);

        if (comclass->progid_offset)
        {
            ptr = (WCHAR*)((BYTE*)comclass + comclass->progid_offset);
            ok_(__FILE__, line)(!lstrcmpW(ptr, progid), "got wrong progid %s, expected %s\n", wine_dbgstr_w(ptr), wine_dbgstr_w(progid));
            ok_(__FILE__, line)(lstrlenW(progid)*sizeof(WCHAR) == comclass->progid_len,
                "got progid name length %d\n", comclass->progid_len);
        }

        /* data length is simply header length + string data length including nulls */
        len = comclass->size + comclass->clrdata_len;
        if (comclass->progid_len) len += comclass->progid_len + sizeof(WCHAR);
        ok_(__FILE__, line)(data.ulLength == len, "got wrong data length %d, expected %d\n", data.ulLength, len);

        /* keyed data structure doesn't include module name, it's available from section data */
        ok_(__FILE__, line)(data.ulSectionTotalLength > comclass->name_offset, "got wrong offset %d\n", comclass->name_offset);

        /* check misc fields are set */
        if (comclass->miscmask)
        {
            if (comclass->miscmask & MiscStatus)
                ok_(__FILE__, line)(comclass->miscstatus != 0, "got miscstatus 0x%08x\n", comclass->miscstatus);
            if (comclass->miscmask & MiscStatusIcon)
                ok_(__FILE__, line)(comclass->miscstatusicon != 0, "got miscstatusicon 0x%08x\n", comclass->miscstatusicon);
            if (comclass->miscmask & MiscStatusContent)
                ok_(__FILE__, line)(comclass->miscstatuscontent != 0, "got miscstatuscontent 0x%08x\n", comclass->miscstatuscontent);
            if (comclass->miscmask & MiscStatusThumbnail)
                ok_(__FILE__, line)(comclass->miscstatusthumbnail != 0, "got miscstatusthumbnail 0x%08x\n", comclass->miscstatusthumbnail);
            if (comclass->miscmask & MiscStatusDocPrint)
                ok_(__FILE__, line)(comclass->miscstatusdocprint != 0, "got miscstatusdocprint 0x%08x\n", comclass->miscstatusdocprint);
        }

        /* part used for clrClass only */
        if (comclass->clrdata_len)
        {
            static const WCHAR mscoreeW[] = {'M','S','C','O','R','E','E','.','D','L','L',0};
            static const WCHAR mscoree2W[] = {'m','s','c','o','r','e','e','.','d','l','l',0};
            struct clrclass_data *clrclass;
            WCHAR *ptrW;

            clrclass = (struct clrclass_data*)((BYTE*)data.lpData + comclass->clrdata_offset);
            ok_(__FILE__, line)(clrclass->size == sizeof(*clrclass), "clrclass: got size %d\n", clrclass->size);
            ok_(__FILE__, line)(clrclass->res[0] == 0, "clrclass: got res[0]=0x%08x\n", clrclass->res[0]);
            ok_(__FILE__, line)(clrclass->res[1] == 2, "clrclass: got res[1]=0x%08x\n", clrclass->res[1]);
            ok_(__FILE__, line)(clrclass->module_len == lstrlenW(mscoreeW)*sizeof(WCHAR), "clrclass: got module len %d\n", clrclass->module_len);
            ok_(__FILE__, line)(clrclass->module_offset > 0, "clrclass: got module offset %d\n", clrclass->module_offset);

            ok_(__FILE__, line)(clrclass->name_len > 0, "clrclass: got name len %d\n", clrclass->name_len);
            ok_(__FILE__, line)(clrclass->name_offset == clrclass->size, "clrclass: got name offset %d\n", clrclass->name_offset);
            ok_(__FILE__, line)(clrclass->version_len > 0, "clrclass: got version len %d\n", clrclass->version_len);
            ok_(__FILE__, line)(clrclass->version_offset > 0, "clrclass: got version offset %d\n", clrclass->version_offset);

            ok_(__FILE__, line)(clrclass->res2[0] == 0, "clrclass: got res2[0]=0x%08x\n", clrclass->res2[0]);
            ok_(__FILE__, line)(clrclass->res2[1] == 0, "clrclass: got res2[1]=0x%08x\n", clrclass->res2[1]);

            /* clrClass uses mscoree.dll as module name, but in two variants - comclass data points to module name
               in lower case, clsclass subsection - in upper case */
            ok_(__FILE__, line)(comclass->name_len == lstrlenW(mscoree2W)*sizeof(WCHAR), "clrclass: got com name len %d\n", comclass->name_len);
            ok_(__FILE__, line)(comclass->name_offset > 0, "clrclass: got name offset %d\n", clrclass->name_offset);

            ptrW = (WCHAR*)((BYTE*)data.lpSectionBase + comclass->name_offset);
            ok_(__FILE__, line)(!lstrcmpW(ptrW, mscoreeW), "clrclass: module name %s\n", wine_dbgstr_w(ptrW));

            ptrW = (WCHAR*)((BYTE*)data.lpSectionBase + clrclass->module_offset);
            ok_(__FILE__, line)(!lstrcmpW(ptrW, mscoree2W), "clrclass: module name2 %s\n", wine_dbgstr_w(ptrW));
        }
    }

    header = (struct guidsection_header*)data.lpSectionBase;
    ok_(__FILE__, line)(data.lpSectionGlobalData == ((BYTE*)header + header->names_offset), "data.lpSectionGlobalData == NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == header->names_len, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);

    /* generated guid for this class works as key guid in search */
    memset(&data2, 0xfe, sizeof(data2));
    data2.cbSize = sizeof(data2);
    ret = pFindActCtxSectionGuid(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                                    &comclass->alias, &data2);
    ok_(__FILE__, line)(ret, "FindActCtxSectionGuid failed: %u\n", GetLastError());

    comclass2 = (struct comclassredirect_data*)data2.lpData;
    ok_(__FILE__, line)(comclass->size == comclass2->size, "got wrong data length %d, expected %d\n", comclass2->size, comclass->size);
    ok_(__FILE__, line)(!memcmp(comclass, comclass2, comclass->size), "got wrong data\n");
}

enum ifaceps_mask
{
    NumMethods = 1,
    BaseIface  = 2
};

struct ifacepsredirect_data
{
    ULONG size;
    DWORD mask;
    GUID  iid;
    ULONG nummethods;
    GUID  tlbid;
    GUID  base;
    ULONG name_len;
    ULONG name_offset;
};

static void test_find_ifaceps_redirection(HANDLE handle, const GUID *iid, const GUID *tlbid, const GUID *base,
    const GUID *ps32, ULONG exid, int line)
{
    struct ifacepsredirect_data *ifaceps;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionGuid(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
                                    iid, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionGuid failed: %u\n", GetLastError());

    ifaceps = (struct ifacepsredirect_data*)data.lpData;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(ifaceps->size == sizeof(*ifaceps), "got %d for header size\n", ifaceps->size);
    if (data.lpData && ifaceps->size == sizeof(*ifaceps))
    {
        ULONG len;

        /* for external proxy stubs it contains a value from 'proxyStubClsid32' */
        if (ps32)
        {
            ok_(__FILE__, line)(IsEqualGUID(&ifaceps->iid, ps32), "got wrong iid %s\n", wine_dbgstr_guid(&ifaceps->iid));
        }
        else
            ok_(__FILE__, line)(IsEqualGUID(&ifaceps->iid, iid), "got wrong iid %s\n", wine_dbgstr_guid(&ifaceps->iid));

        ok_(__FILE__, line)(IsEqualGUID(&ifaceps->tlbid, tlbid), "got wrong tlid %s\n", wine_dbgstr_guid(&ifaceps->tlbid));
        ok_(__FILE__, line)(ifaceps->name_len > 0, "got modulename len %d\n", ifaceps->name_len);
        ok_(__FILE__, line)(ifaceps->name_offset == ifaceps->size, "got progid offset %d\n", ifaceps->name_offset);

        /* data length is simply header length + string data length including nulls */
        len = ifaceps->size + ifaceps->name_len + sizeof(WCHAR);
        ok_(__FILE__, line)(data.ulLength == len, "got wrong data length %d, expected %d\n", data.ulLength, len);

        /* mask purpose is to indicate if attribute was specified, for testing purposes assume that manifest
           always has non-zero value for it */
        if (ifaceps->mask & NumMethods)
            ok_(__FILE__, line)(ifaceps->nummethods != 0, "got nummethods %d\n", ifaceps->nummethods);
        if (ifaceps->mask & BaseIface)
            ok_(__FILE__, line)(IsEqualGUID(&ifaceps->base, base), "got base %s\n", wine_dbgstr_guid(&ifaceps->base));
    }

    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);
}

struct clrsurrogate_data
{
    ULONG size;
    DWORD res;
    GUID  clsid;
    ULONG version_offset;
    ULONG version_len;
    ULONG name_offset;
    ULONG name_len;
};

static void test_find_surrogate(HANDLE handle, const GUID *clsid, const WCHAR *name, const WCHAR *version,
    ULONG exid, int line)
{
    struct clrsurrogate_data *surrogate;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionGuid(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_CLR_SURROGATES,
                                    clsid, &data);
    if (!ret)
    {
        skip("surrogate sections are not supported\n");
        return;
    }
    ok_(__FILE__, line)(ret, "FindActCtxSectionGuid failed: %u\n", GetLastError());

    surrogate = (struct clrsurrogate_data*)data.lpData;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(surrogate->size == sizeof(*surrogate), "got %d for header size\n", surrogate->size);
    if (data.lpData && surrogate->size == sizeof(*surrogate))
    {
        WCHAR *ptrW;
        ULONG len;

        ok_(__FILE__, line)(surrogate->res == 0, "invalid res value %d\n", surrogate->res);
        ok_(__FILE__, line)(IsEqualGUID(&surrogate->clsid, clsid), "got wrong clsid %s\n", wine_dbgstr_guid(&surrogate->clsid));

        ok_(__FILE__, line)(surrogate->version_len == lstrlenW(version)*sizeof(WCHAR), "got version len %d\n", surrogate->version_len);
        ok_(__FILE__, line)(surrogate->version_offset == surrogate->size, "got version offset %d\n", surrogate->version_offset);

        ok_(__FILE__, line)(surrogate->name_len == lstrlenW(name)*sizeof(WCHAR), "got name len %d\n", surrogate->name_len);
        ok_(__FILE__, line)(surrogate->name_offset > surrogate->version_offset, "got name offset %d\n", surrogate->name_offset);

        len = surrogate->size + surrogate->name_len + surrogate->version_len + 2*sizeof(WCHAR);
        ok_(__FILE__, line)(data.ulLength == len, "got wrong data length %d, expected %d\n", data.ulLength, len);

        ptrW = (WCHAR*)((BYTE*)surrogate + surrogate->name_offset);
        ok(!lstrcmpW(ptrW, name), "got wrong name %s\n", wine_dbgstr_w(ptrW));

        ptrW = (WCHAR*)((BYTE*)surrogate + surrogate->version_offset);
        ok(!lstrcmpW(ptrW, version), "got wrong name %s\n", wine_dbgstr_w(ptrW));
    }

    ok_(__FILE__, line)(data.lpSectionGlobalData == NULL, "data.lpSectionGlobalData != NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == 0, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n",
       data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
       data.ulAssemblyRosterIndex, exid);
}

static void test_find_progid_redirection(HANDLE handle, const GUID *clsid, const char *progid, ULONG exid, int line)
{
    struct progidredirect_data *progiddata;
    struct comclassredirect_data *comclass;
    ACTCTX_SECTION_KEYED_DATA data, data2;
    struct strsection_header *header;
    BOOL ret;

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pFindActCtxSectionStringA(0, NULL,
                                       ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION,
                                       progid, &data);
    ok_(__FILE__, line)(ret, "FindActCtxSectionStringA failed: %u\n", GetLastError());

    progiddata = (struct progidredirect_data*)data.lpData;

    ok_(__FILE__, line)(data.cbSize == sizeof(data), "data.cbSize=%u\n", data.cbSize);
    ok_(__FILE__, line)(data.ulDataFormatVersion == 1, "data.ulDataFormatVersion=%u\n", data.ulDataFormatVersion);
    ok_(__FILE__, line)(data.lpData != NULL, "data.lpData == NULL\n");
    ok_(__FILE__, line)(progiddata->size == sizeof(*progiddata), "got %d for header size\n", progiddata->size);
    if (data.lpData && progiddata->size == sizeof(*progiddata))
    {
        GUID *guid;

        ok_(__FILE__, line)(progiddata->reserved == 0, "got reserved as %d\n", progiddata->reserved);
        ok_(__FILE__, line)(progiddata->clsid_offset > 0, "got clsid_offset as %d\n", progiddata->clsid_offset);

        /* progid data points to generated alias guid */
        guid = (GUID*)((BYTE*)data.lpSectionBase + progiddata->clsid_offset);

        memset(&data2, 0, sizeof(data2));
        data2.cbSize = sizeof(data2);
        ret = pFindActCtxSectionGuid(0, NULL,
                                        ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                                        guid, &data2);
        ok_(__FILE__, line)(ret, "FindActCtxSectionGuid failed: %u\n", GetLastError());

        comclass = (struct comclassredirect_data*)data2.lpData;
        ok_(__FILE__, line)(IsEqualGUID(guid, &comclass->alias), "got wrong alias referenced from progid %s, %s\n", progid, wine_dbgstr_guid(guid));
        ok_(__FILE__, line)(IsEqualGUID(clsid, &comclass->clsid), "got wrong class referenced from progid %s, %s\n", progid, wine_dbgstr_guid(clsid));
    }

    header = (struct strsection_header*)data.lpSectionBase;
    ok_(__FILE__, line)(data.lpSectionGlobalData == (BYTE*)header + header->global_offset, "data.lpSectionGlobalData == NULL\n");
    ok_(__FILE__, line)(data.ulSectionGlobalDataLength == header->global_len, "data.ulSectionGlobalDataLength=%u\n", data.ulSectionGlobalDataLength);
    ok_(__FILE__, line)(data.lpSectionBase != NULL, "data.lpSectionBase == NULL\n");
    ok_(__FILE__, line)(data.ulSectionTotalLength > 0, "data.ulSectionTotalLength=%u\n", data.ulSectionTotalLength);
    ok_(__FILE__, line)(data.hActCtx == NULL, "data.hActCtx=%p\n", data.hActCtx);
    ok_(__FILE__, line)(data.ulAssemblyRosterIndex == exid, "data.ulAssemblyRosterIndex=%u, expected %u\n",
        data.ulAssemblyRosterIndex, exid);
}

static void test_wndclass_section(void)
{
    static const WCHAR cls1W[] = {'1','.','2','.','3','.','4','!','w','n','d','C','l','a','s','s','1',0};
    ACTCTX_SECTION_KEYED_DATA data, data2;
    struct wndclass_redirect_data *classdata;
    struct strsection_header *section;
    ULONG_PTR cookie;
    HANDLE handle;
    WCHAR *ptrW;
    BOOL ret;

    /* use two dependent manifests, each defines 2 window class redirects */
    create_manifest_file("testdep1.manifest", manifest_wndcls1, -1, NULL, NULL);
    create_manifest_file("testdep2.manifest", manifest_wndcls2, -1, NULL, NULL);
    create_manifest_file("main_wndcls.manifest", manifest_wndcls_main, -1, NULL, NULL);

    handle = test_create("main_wndcls.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());

    DeleteFileA("testdep1.manifest");
    DeleteFileA("testdep2.manifest");
    DeleteFileA("main_wndcls.manifest");

    ret = pActivateActCtx(handle, &cookie);
    ok(ret, "ActivateActCtx failed: %u\n", GetLastError());

    memset(&data, 0, sizeof(data));
    memset(&data2, 0, sizeof(data2));
    data.cbSize = sizeof(data);
    data2.cbSize = sizeof(data2);

    /* get data for two classes from different assemblies */
    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    wndClass1W, &data);
    ok(ret, "got %d\n", ret);
    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                    wndClass3W, &data2);
    ok(ret, "got %d\n", ret);

    section = (struct strsection_header*)data.lpSectionBase;
    ok(section->count == 4, "got %d\n", section->count);
    ok(section->size == sizeof(*section), "got %d\n", section->size);

    /* For both string same section is returned, meaning it's one wndclass section per context */
    ok(data.lpSectionBase == data2.lpSectionBase, "got %p, %p\n", data.lpSectionBase, data2.lpSectionBase);
    ok(data.ulSectionTotalLength == data2.ulSectionTotalLength, "got %u, %u\n", data.ulSectionTotalLength,
        data2.ulSectionTotalLength);

    /* wndClass1 is versioned, wndClass3 is not */
    classdata = (struct wndclass_redirect_data*)data.lpData;
    ptrW = (WCHAR*)((BYTE*)data.lpData + classdata->name_offset);
    ok(!lstrcmpW(ptrW, cls1W), "got %s\n", wine_dbgstr_w(ptrW));

    classdata = (struct wndclass_redirect_data*)data2.lpData;
    ptrW = (WCHAR*)((BYTE*)data2.lpData + classdata->name_offset);
    ok(!lstrcmpW(ptrW, wndClass3W), "got %s\n", wine_dbgstr_w(ptrW));

    ret = pDeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %u\n", GetLastError());

    pReleaseActCtx(handle);
}

static void test_dllredirect_section(void)
{
    static const WCHAR testlib1W[] = {'t','e','s','t','l','i','b','1','.','d','l','l',0};
    static const WCHAR testlib2W[] = {'t','e','s','t','l','i','b','2','.','d','l','l',0};
    ACTCTX_SECTION_KEYED_DATA data, data2;
    struct strsection_header *section;
    ULONG_PTR cookie;
    HANDLE handle;
    BOOL ret;

    /* use two dependent manifests, 4 'files' total */
    create_manifest_file("testdep1.manifest", manifest_wndcls1, -1, NULL, NULL);
    create_manifest_file("testdep2.manifest", manifest_wndcls2, -1, NULL, NULL);
    create_manifest_file("main_wndcls.manifest", manifest_wndcls_main, -1, NULL, NULL);

    handle = test_create("main_wndcls.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());

    DeleteFileA("testdep1.manifest");
    DeleteFileA("testdep2.manifest");
    DeleteFileA("main_wndcls.manifest");

    ret = pActivateActCtx(handle, &cookie);
    ok(ret, "ActivateActCtx failed: %u\n", GetLastError());

    memset(&data, 0, sizeof(data));
    memset(&data2, 0, sizeof(data2));
    data.cbSize = sizeof(data);
    data2.cbSize = sizeof(data2);

    /* get data for two files from different assemblies */
    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib1W, &data);
    ok(ret, "got %d\n", ret);
    ret = pFindActCtxSectionStringW(0, NULL,
                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                    testlib2W, &data2);
    ok(ret, "got %d\n", ret);

    section = (struct strsection_header*)data.lpSectionBase;
    ok(section->count == 4, "got %d\n", section->count);
    ok(section->size == sizeof(*section), "got %d\n", section->size);

    /* For both string same section is returned, meaning it's one dll redirect section per context */
    ok(data.lpSectionBase == data2.lpSectionBase, "got %p, %p\n", data.lpSectionBase, data2.lpSectionBase);
    ok(data.ulSectionTotalLength == data2.ulSectionTotalLength, "got %u, %u\n", data.ulSectionTotalLength,
        data2.ulSectionTotalLength);

    ret = pDeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %u\n", GetLastError());

    pReleaseActCtx(handle);
}

static void test_typelib_section(void)
{
    static const WCHAR helpW[] = {'h','e','l','p'};
    ACTCTX_SECTION_KEYED_DATA data, data2;
    struct guidsection_header *section;
    struct tlibredirect_data *tlib;
    ULONG_PTR cookie;
    HANDLE handle;
    BOOL ret;

    /* use two dependent manifests, 4 'files' total */
    create_manifest_file("testdep1.manifest", manifest_wndcls1, -1, NULL, NULL);
    create_manifest_file("testdep2.manifest", manifest_wndcls2, -1, NULL, NULL);
    create_manifest_file("main_wndcls.manifest", manifest_wndcls_main, -1, NULL, NULL);

    handle = test_create("main_wndcls.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());

    DeleteFileA("testdep1.manifest");
    DeleteFileA("testdep2.manifest");
    DeleteFileA("main_wndcls.manifest");

    ret = pActivateActCtx(handle, &cookie);
    ok(ret, "ActivateActCtx failed: %u\n", GetLastError());

    memset(&data, 0, sizeof(data));
    memset(&data2, 0, sizeof(data2));
    data.cbSize = sizeof(data);
    data2.cbSize = sizeof(data2);

    /* get data for two typelibs from different assemblies */
    ret = pFindActCtxSectionGuid(0, NULL,
                                 ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
                                 &IID_TlibTest, &data);
    ok(ret, "got %d\n", ret);

    ret = pFindActCtxSectionGuid(0, NULL,
                                 ACTIVATION_CONTEXT_SECTION_COM_TYPE_LIBRARY_REDIRECTION,
                                 &IID_TlibTest4, &data2);
    ok(ret, "got %d\n", ret);

    section = (struct guidsection_header*)data.lpSectionBase;
    ok(section->count == 4, "got %d\n", section->count);
    ok(section->size == sizeof(*section), "got %d\n", section->size);

    /* For both GUIDs same section is returned */
    ok(data.lpSectionBase == data2.lpSectionBase, "got %p, %p\n", data.lpSectionBase, data2.lpSectionBase);
    ok(data.ulSectionTotalLength == data2.ulSectionTotalLength, "got %u, %u\n", data.ulSectionTotalLength,
        data2.ulSectionTotalLength);

    ok(data.lpSectionGlobalData == ((BYTE*)section + section->names_offset), "data.lpSectionGlobalData == NULL\n");
    ok(data.ulSectionGlobalDataLength == section->names_len, "data.ulSectionGlobalDataLength=%u\n",
       data.ulSectionGlobalDataLength);

    /* test some actual data */
    tlib = (struct tlibredirect_data*)data.lpData;
    ok(tlib->size == sizeof(*tlib), "got %d\n", tlib->size);
    ok(tlib->major_version == 1, "got %d\n", tlib->major_version);
    ok(tlib->minor_version == 0, "got %d\n", tlib->minor_version);
    ok(tlib->help_offset > 0, "got %d\n", tlib->help_offset);
    ok(tlib->help_len == sizeof(helpW), "got %d\n", tlib->help_len);
    ok(tlib->flags == (LIBFLAG_FHIDDEN|LIBFLAG_FCONTROL|LIBFLAG_FRESTRICTED), "got %x\n", tlib->flags);

    ret = pDeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %u\n", GetLastError());

    pReleaseActCtx(handle);
}

static void test_allowDelayedBinding(void)
{
    HANDLE handle;

    if (!create_manifest_file("test5.manifest", manifest5, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test5.manifest");
    if (handle == INVALID_HANDLE_VALUE) {
        win_skip("allowDelayedBinding attribute is not supported.\n");
        return;
    }

    DeleteFileA("test5.manifest");
    DeleteFileA("testdep.manifest");
    if (handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        pReleaseActCtx(handle);
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
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info0, __LINE__);
        pReleaseActCtx(handle);
    }

    /* test for whitespace handling in Eq ::= S? '=' S? */
    create_manifest_file("test1_1.manifest", manifest1_1, -1, NULL, NULL);
    handle = test_create("test1_1.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test1_1.manifest");
    pReleaseActCtx(handle);

    if(!create_manifest_file("test1.manifest", manifest1, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    trace("manifest1\n");

    handle = test_create("test1.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test1.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info1, __LINE__);
        test_info_in_assembly(handle, 1, &manifest1_info, __LINE__);

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

    handle = test_create("test2.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test2.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info2, __LINE__);
        test_info_in_assembly(handle, 1, &manifest2_info, __LINE__);
        test_info_in_assembly(handle, 2, &depmanifest1_info, __LINE__);
        pReleaseActCtx(handle);
    }

    if(!create_manifest_file("test2-2.manifest", manifest2, -1, "testdep.manifest", testdep_manifest2)) {
        skip("Could not create manifest file\n");
        return;
    }

    trace("manifest2 depmanifest2\n");

    handle = test_create("test2-2.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test2-2.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info2, __LINE__);
        test_info_in_assembly(handle, 1, &manifest2_info, __LINE__);
        test_info_in_assembly(handle, 2, &depmanifest2_info, __LINE__);
        test_file_info(handle, 1, 0, testlib_dll, __LINE__);
        test_file_info(handle, 1, 1, testlib2_dll, __LINE__);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 2, __LINE__);
        test_find_dll_redirection(handle, testlib2_dll, 2, __LINE__);
        b = pDeactivateActCtx(0, cookie);
        ok(b, "DeactivateActCtx failed: %u\n", GetLastError());

        pReleaseActCtx(handle);
    }

    trace("manifest2 depmanifest3\n");

    if(!create_manifest_file("test2-3.manifest", manifest2, -1, "testdep.manifest", testdep_manifest3)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test2-3.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test2-3.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info2, __LINE__);
        test_info_in_assembly(handle, 1, &manifest2_info, __LINE__);
        test_info_in_assembly(handle, 2, &depmanifest3_info, __LINE__);
        test_file_info(handle, 1, 0, testlib_dll, __LINE__);
        test_file_info(handle, 1, 1, testlib2_dll, __LINE__);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 2, __LINE__);
        test_find_dll_redirection(handle, testlib2_dll, 2, __LINE__);
        test_find_window_class(handle, wndClassW, 2, __LINE__);
        test_find_window_class(handle, wndClass2W, 2, __LINE__);
        b = pDeactivateActCtx(0, cookie);
        ok(b, "DeactivateActCtx failed: %u\n", GetLastError());

        pReleaseActCtx(handle);
    }

    trace("manifest3\n");

    if(!create_manifest_file("test3.manifest", manifest3, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test3.manifest");
    ok(handle != INVALID_HANDLE_VALUE || broken(handle == INVALID_HANDLE_VALUE) /* XP pre-SP2, win2k3 w/o SP */,
        "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    if (handle == INVALID_HANDLE_VALUE)
        win_skip("Some activation context features not supported, skipping a test (possibly old XP/Win2k3 system\n");
    DeleteFileA("test3.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        static const WCHAR nameW[] = {'t','e','s','t','s','u','r','r','o','g','a','t','e',0};
        static const WCHAR versionW[] = {'v','2','.','0','.','5','0','7','2','7',0};
        static const WCHAR progidW[] = {'P','r','o','g','I','d','.','P','r','o','g','I','d',0};
        static const WCHAR clrprogidW[] = {'c','l','r','p','r','o','g','i','d',0};

        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info1, __LINE__);
        test_info_in_assembly(handle, 1, &manifest3_info, __LINE__);
        test_file_info(handle, 0, 0, testlib_dll, __LINE__);

        b = pActivateActCtx(handle, &cookie);
        ok(b, "ActivateActCtx failed: %u\n", GetLastError());
        test_find_dll_redirection(handle, testlib_dll, 1, __LINE__);
        test_find_dll_redirection(handle, testlib_dll, 1, __LINE__);
        test_find_com_redirection(handle, &IID_CoTest, &IID_TlibTest, progidW, 1, __LINE__);
        test_find_com_redirection(handle, &IID_CoTest2, NULL, NULL, 1, __LINE__);
        test_find_com_redirection(handle, &CLSID_clrclass, &IID_TlibTest, clrprogidW, 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.1", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.2", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.3", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.4", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.5", 1, __LINE__);
        test_find_progid_redirection(handle, &IID_CoTest, "ProgId.ProgId.6", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.1", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.2", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.3", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.4", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.5", 1, __LINE__);
        test_find_progid_redirection(handle, &CLSID_clrclass, "clrprogid.6", 1, __LINE__);
        test_find_surrogate(handle, &IID_Iiface, nameW, versionW, 1, __LINE__);
        test_find_ifaceps_redirection(handle, &IID_Iifaceps, &IID_TlibTest4, &IID_Ibifaceps, NULL, 1, __LINE__);
        test_find_ifaceps_redirection(handle, &IID_Iifaceps2, &IID_TlibTest4, &IID_Ibifaceps, &IID_PS32, 1, __LINE__);
        test_find_ifaceps_redirection(handle, &IID_Iifaceps3, &IID_TlibTest4, &IID_Ibifaceps, NULL, 1, __LINE__);
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

    handle = test_create("test4.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test4.manifest");
    DeleteFileA("testdep.manifest");
    if(handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info2, __LINE__);
        test_info_in_assembly(handle, 1, &manifest4_info, __LINE__);
        test_info_in_assembly(handle, 2, &manifest_comctrl_info, __LINE__);
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
        handle = test_create("..\\test1.manifest");
        ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
        DeleteFileA("..\\test1.manifest");
        if(handle != INVALID_HANDLE_VALUE) {
            test_basic_info(handle, __LINE__);
            test_detailed_info(handle, &detailed_info1, __LINE__);
            test_info_in_assembly(handle, 1, &manifest1_info, __LINE__);
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

    handle = test_create("test1.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test1.manifest");
    if (handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info1, __LINE__);
        test_info_in_assembly(handle, 1, &manifest1_info, __LINE__);
        pReleaseActCtx(handle);
    }

    trace("UTF-16 manifest1, reverse endian, with BOM\n");
    if(!create_wide_manifest("test1.manifest", manifest1, TRUE, TRUE)) {
        skip("Could not create manifest file\n");
        return;
    }

    handle = test_create("test1.manifest");
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    DeleteFileA("test1.manifest");
    if (handle != INVALID_HANDLE_VALUE) {
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info1, __LINE__);
        test_info_in_assembly(handle, 1, &manifest1_info, __LINE__);
        pReleaseActCtx(handle);
    }

    test_wndclass_section();
    test_dllredirect_section();
    test_typelib_section();
    test_allowDelayedBinding();
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
        test_basic_info(handle, __LINE__);
        test_detailed_info(handle, &detailed_info1_child, __LINE__);
        test_info_in_assembly(handle, 1, &manifest1_child_info, __LINE__);
        pReleaseActCtx(handle);
    }
}

static HANDLE create_manifest(const char *filename, const char *data, int line)
{
    HANDLE handle;
    create_manifest_file(filename, data, -1, NULL, NULL);

    handle = test_create(filename);
    ok_(__FILE__, line)(handle != INVALID_HANDLE_VALUE,
        "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());

    DeleteFileA(filename);
    return handle;
}

static void kernel32_find(ULONG section, const char *string_to_find, BOOL should_find, int line)
{
    UNICODE_STRING string_to_findW;
    ACTCTX_SECTION_KEYED_DATA data;
    BOOL ret;
    DWORD err;

    pRtlCreateUnicodeStringFromAsciiz(&string_to_findW, string_to_find);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    SetLastError(0);
    ret = pFindActCtxSectionStringA(0, NULL, section, string_to_find, &data);
    err = GetLastError();
    ok_(__FILE__, line)(ret == should_find,
        "FindActCtxSectionStringA: expected ret = %u, got %u\n", should_find, ret);
    ok_(__FILE__, line)(err == (should_find ? ERROR_SUCCESS : ERROR_SXS_KEY_NOT_FOUND),
        "FindActCtxSectionStringA: unexpected error %u\n", err);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    SetLastError(0);
    ret = pFindActCtxSectionStringW(0, NULL, section, string_to_findW.Buffer, &data);
    err = GetLastError();
    ok_(__FILE__, line)(ret == should_find,
        "FindActCtxSectionStringW: expected ret = %u, got %u\n", should_find, ret);
    ok_(__FILE__, line)(err == (should_find ? ERROR_SUCCESS : ERROR_SXS_KEY_NOT_FOUND),
        "FindActCtxSectionStringW: unexpected error %u\n", err);

    SetLastError(0);
    ret = pFindActCtxSectionStringA(0, NULL, section, string_to_find, NULL);
    err = GetLastError();
    ok_(__FILE__, line)(!ret,
        "FindActCtxSectionStringA: expected failure, got %u\n", ret);
    ok_(__FILE__, line)(err == ERROR_INVALID_PARAMETER,
        "FindActCtxSectionStringA: unexpected error %u\n", err);

    SetLastError(0);
    ret = pFindActCtxSectionStringW(0, NULL, section, string_to_findW.Buffer, NULL);
    err = GetLastError();
    ok_(__FILE__, line)(!ret,
        "FindActCtxSectionStringW: expected failure, got %u\n", ret);
    ok_(__FILE__, line)(err == ERROR_INVALID_PARAMETER,
        "FindActCtxSectionStringW: unexpected error %u\n", err);

    pRtlFreeUnicodeString(&string_to_findW);
}

static void ntdll_find(ULONG section, const char *string_to_find, BOOL should_find, int line)
{
    UNICODE_STRING string_to_findW;
    ACTCTX_SECTION_KEYED_DATA data;
    NTSTATUS ret;

    pRtlCreateUnicodeStringFromAsciiz(&string_to_findW, string_to_find);

    memset(&data, 0xfe, sizeof(data));
    data.cbSize = sizeof(data);

    ret = pRtlFindActivationContextSectionString(0, NULL, section, &string_to_findW, &data);
    ok_(__FILE__, line)(ret == (should_find ? STATUS_SUCCESS : STATUS_SXS_KEY_NOT_FOUND),
        "RtlFindActivationContextSectionString: unexpected status 0x%x\n", ret);

    ret = pRtlFindActivationContextSectionString(0, NULL, section, &string_to_findW, NULL);
    ok_(__FILE__, line)(ret == (should_find ? STATUS_SUCCESS : STATUS_SXS_KEY_NOT_FOUND),
        "RtlFindActivationContextSectionString: unexpected status 0x%x\n", ret);

    pRtlFreeUnicodeString(&string_to_findW);
}

static void test_findsectionstring(void)
{
    HANDLE handle;
    BOOL ret;
    ULONG_PTR cookie;

    handle = create_manifest("test.manifest", testdep_manifest3, __LINE__);
    ret = pActivateActCtx(handle, &cookie);
    ok(ret, "ActivateActCtx failed: %u\n", GetLastError());

    /* first we show the parameter validation from kernel32 */
    kernel32_find(ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION, "testdep", FALSE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib.dll", TRUE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib2.dll", TRUE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib3.dll", FALSE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass", TRUE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass2", TRUE, __LINE__);
    kernel32_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass3", FALSE, __LINE__);

    /* then we show that ntdll plays by different rules */
    ntdll_find(ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION, "testdep", FALSE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib.dll", TRUE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib2.dll", TRUE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, "testlib3.dll", FALSE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass", TRUE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass2", TRUE, __LINE__);
    ntdll_find(ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION, "wndClass3", FALSE, __LINE__);

    ret = pDeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %u\n", GetLastError());
    pReleaseActCtx(handle);
}

static void run_child_process(void)
{
    char cmdline[MAX_PATH];
    char path[MAX_PATH];
    char **argv;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    HANDLE file;
    FILETIME now;
    BOOL ret;

    GetModuleFileNameA(NULL, path, MAX_PATH);
    strcat(path, ".manifest");
    if(!create_manifest_file(path, manifest1, -1, NULL, NULL)) {
        skip("Could not create manifest file\n");
        return;
    }

    si.cb = sizeof(si);
    winetest_get_mainargs( &argv );
    /* Vista+ seems to cache presence of .manifest files. Change last modified
       date to defeat the cache */
    file = CreateFileA(argv[0], FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        GetSystemTimeAsFileTime(&now);
        SetFileTime(file, NULL, NULL, &now);
        CloseHandle(file);
    }
    sprintf(cmdline, "\"%s\" %s manifest1", argv[0], argv[1]);
    ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %u\n", GetLastError());
    winetest_wait_child_process( pi.hProcess );
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteFileA(path);
}

static void init_paths(void)
{
    LPWSTR ptr;

    static const WCHAR dot_manifest[] = {'.','M','a','n','i','f','e','s','t',0};
    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR subdir[] = {'T','e','s','t','S','u','b','d','i','r','\\',0};

    GetModuleFileNameW(NULL, exe_path, sizeof(exe_path)/sizeof(WCHAR));
    lstrcpyW(app_dir, exe_path);
    for(ptr=app_dir+lstrlenW(app_dir); *ptr != '\\' && *ptr != '/'; ptr--);
    ptr[1] = 0;

    GetCurrentDirectoryW(MAX_PATH, work_dir);
    ptr = work_dir + lstrlenW( work_dir ) - 1;
    if (*ptr != '\\' && *ptr != '/')
        lstrcatW(work_dir, backslash);
    lstrcpyW(work_dir_subdir, work_dir);
    lstrcatW(work_dir_subdir, subdir);

    GetModuleFileNameW(NULL, app_manifest_path, sizeof(app_manifest_path)/sizeof(WCHAR));
    lstrcpyW(app_manifest_path+lstrlenW(app_manifest_path), dot_manifest);
}

static void write_manifest(const char *filename, const char *manifest)
{
    HANDLE file;
    DWORD size;
    CHAR path[MAX_PATH];

    GetTempPathA(sizeof(path)/sizeof(CHAR), path);
    strcat(path, filename);

    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
    WriteFile(file, manifest, strlen(manifest), &size, NULL);
    CloseHandle(file);
}

static void delete_manifest_file(const char *filename)
{
    CHAR path[MAX_PATH];

    GetTempPathA(sizeof(path)/sizeof(CHAR), path);
    strcat(path, filename);
    DeleteFileA(path);
}

static void test_CreateActCtx(void)
{
    CHAR path[MAX_PATH], dir[MAX_PATH];
    ACTCTXA actctx;
    HANDLE handle;

    GetTempPathA(sizeof(path)/sizeof(CHAR), path);
    strcat(path, "main_wndcls.manifest");

    write_manifest("testdep1.manifest", manifest_wndcls1);
    write_manifest("testdep2.manifest", manifest_wndcls2);
    write_manifest("main_wndcls.manifest", manifest_wndcls_main);

    memset(&actctx, 0, sizeof(ACTCTXA));
    actctx.cbSize = sizeof(ACTCTXA);
    actctx.lpSource = path;

    /* create using lpSource without specified directory */
    handle = pCreateActCtxA(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "failed to generate context, error %u\n", GetLastError());
    pReleaseActCtx(handle);

    /* with specified directory, that doesn't contain dependent assembly */
    GetWindowsDirectoryA(dir, sizeof(dir)/sizeof(CHAR));

    memset(&actctx, 0, sizeof(ACTCTXA));
    actctx.cbSize = sizeof(ACTCTXA);
    actctx.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    actctx.lpAssemblyDirectory = dir;
    actctx.lpSource = path;

    SetLastError(0xdeadbeef);
    handle = pCreateActCtxA(&actctx);
todo_wine {
    ok(handle == INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    ok(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX, "got error %d\n", GetLastError());
}
    if (handle != INVALID_HANDLE_VALUE) pReleaseActCtx(handle);

    delete_manifest_file("main_wndcls.manifest");
    delete_manifest_file("testdep1.manifest");
    delete_manifest_file("testdep2.manifest");

    /* ACTCTX_FLAG_HMODULE_VALID but hModule is not set */
    memset(&actctx, 0, sizeof(ACTCTXA));
    actctx.cbSize = sizeof(ACTCTXA);
    actctx.dwFlags = ACTCTX_FLAG_HMODULE_VALID;
    SetLastError(0xdeadbeef);
    handle = pCreateActCtxA(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "got handle %p\n", handle);
todo_wine
    ok(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX || broken(GetLastError() == ERROR_NOT_ENOUGH_MEMORY) /* XP, win2k3 */,
        "got error %d\n", GetLastError());

    /* create from HMODULE - resource doesn't exist, lpSource is set */
    memset(&actctx, 0, sizeof(ACTCTXA));
    actctx.cbSize = sizeof(ACTCTXA);
    actctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
    actctx.lpSource = "dummyfile.dll";
    actctx.lpResourceName = MAKEINTRESOURCEA(20);
    actctx.hModule = GetModuleHandleA(NULL);

    SetLastError(0xdeadbeef);
    handle = pCreateActCtxA(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    ok(GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND, "got error %d\n", GetLastError());

    /* load manifest from lpAssemblyDirectory directory */
    write_manifest("testdir.manifest", manifest1);
    GetTempPathA(sizeof(path)/sizeof(path[0]), path);
    SetCurrentDirectoryA(path);
    strcat(path, "assembly_dir");
    strcpy(dir, path);
    strcat(path, "\\testdir.manifest");

    memset(&actctx, 0, sizeof(actctx));
    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    actctx.lpSource = "testdir.manifest";
    actctx.lpAssemblyDirectory = dir;

    SetLastError(0xdeadbeef);
    handle = pCreateActCtxA(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    ok(GetLastError()==ERROR_PATH_NOT_FOUND ||
            broken(GetLastError()==ERROR_FILE_NOT_FOUND) /* WinXP */,
            "got error %d\n", GetLastError());

    CreateDirectoryA(dir, NULL);
    memset(&actctx, 0, sizeof(actctx));
    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    actctx.lpSource = "testdir.manifest";
    actctx.lpAssemblyDirectory = dir;

    SetLastError(0xdeadbeef);
    handle = pCreateActCtxA(&actctx);
    ok(handle == INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %d\n", GetLastError());
    SetCurrentDirectoryW(work_dir);

    write_manifest("assembly_dir\\testdir.manifest", manifest1);
    memset(&actctx, 0, sizeof(actctx));
    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    actctx.lpSource = "testdir.manifest";
    actctx.lpAssemblyDirectory = dir;

    handle = pCreateActCtxA(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    pReleaseActCtx(handle);

    memset(&actctx, 0, sizeof(actctx));
    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
    actctx.lpSource = path;
    actctx.lpAssemblyDirectory = dir;

    handle = pCreateActCtxA(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "got handle %p\n", handle);
    pReleaseActCtx(handle);

    delete_manifest_file("testdir.manifest");
    delete_manifest_file("assembly_dir\\testdir.manifest");
    RemoveDirectoryA(dir);
}

static BOOL init_funcs(void)
{
    HMODULE hLibrary = GetModuleHandleA("kernel32.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(hLibrary, #f))) return FALSE;
    X(ActivateActCtx);
    X(CreateActCtxA);
    X(CreateActCtxW);
    X(DeactivateActCtx);
    X(FindActCtxSectionStringA);
    X(FindActCtxSectionStringW);
    X(GetCurrentActCtx);
    X(IsDebuggerPresent);
    X(QueryActCtxW);
    X(ReleaseActCtx);
    X(FindActCtxSectionGuid);

    hLibrary = GetModuleHandleA("ntdll.dll");
    X(RtlFindActivationContextSectionString);
    X(RtlCreateUnicodeStringFromAsciiz);
    X(RtlFreeUnicodeString);
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
        win_skip("Needed functions are not available\n");
        return;
    }
    init_paths();

    if(argc > 2 && !strcmp(argv[2], "manifest1")) {
        test_app_manifest();
        return;
    }

    test_actctx();
    test_CreateActCtx();
    test_findsectionstring();
    run_child_process();
}
