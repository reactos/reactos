/*
 * Utility routines for comctl32 v6 tests
 *
 * Copyright 2006 Mike McCormack for CodeWeavers
 * Copyright 2007 George Gov
 * Copyright 2009 Owen Rudge for CodeWeavers
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

static const CHAR manifest_name[] = "cc6.manifest";

static const CHAR manifest[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
    "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\n"
    "  <assemblyIdentity\n"
    "      type=\"win32\"\n"
    "      name=\"Wine.ComCtl32.Tests\"\n"
    "      version=\"1.0.0.0\"\n"
    "      processorArchitecture=\"" ARCH "\"\n"
    "  />\n"
    "<description>Wine comctl32 test suite</description>\n"
    "<dependency>\n"
    "  <dependentAssembly>\n"
    "    <assemblyIdentity\n"
    "        type=\"win32\"\n"
    "        name=\"microsoft.windows.common-controls\"\n"
    "        version=\"6.0.0.0\"\n"
    "        processorArchitecture=\"" ARCH "\"\n"
    "        publicKeyToken=\"6595b64144ccf1df\"\n"
    "        language=\"*\"\n"
    "    />\n"
    "</dependentAssembly>\n"
    "</dependency>\n"
    "</assembly>\n";

static void unload_v6_module(ULONG_PTR cookie, HANDLE hCtx)
{
    DeactivateActCtx(0, cookie);
    ReleaseActCtx(hCtx);

    DeleteFileA(manifest_name);
}

static BOOL load_v6_module(ULONG_PTR *pcookie, HANDLE *hCtx)
{
    ACTCTX_SECTION_KEYED_DATA data;
    DWORD written;
    HMODULE hmod;
    ACTCTXA ctx;
    HANDLE file;
    BOOL ret;

    /* create manifest */
    file = CreateFileA( manifest_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if (file != INVALID_HANDLE_VALUE)
    {
        ret = (WriteFile( file, manifest, sizeof(manifest)-1, &written, NULL ) &&
               written == sizeof(manifest)-1);
        CloseHandle( file );
        if (!ret)
        {
            DeleteFileA( manifest_name );
            skip("Failed to fill manifest file. Skipping comctl32 V6 tests.\n");
            return FALSE;
        }
        else
            trace("created %s\n", manifest_name);
    }
    else
    {
        skip("Failed to create manifest file. Skipping comctl32 V6 tests.\n");
        return FALSE;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.cbSize = sizeof(ctx);
    ctx.lpSource = manifest_name;

    *hCtx = CreateActCtxA(&ctx);
    ok(*hCtx != 0, "Expected context handle\n");

    hmod = GetModuleHandleA("comctl32.dll");

    ret = ActivateActCtx(*hCtx, pcookie);
    ok(ret, "Failed to activate context, error %ld.\n", GetLastError());

    if (!ret)
    {
        win_skip("A problem during context activation occurred.\n");
        DeleteFileA(manifest_name);
    }

    data.cbSize = sizeof(data);
    ret = FindActCtxSectionStringA(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
        "comctl32.dll", &data);
    ok(ret, "failed to find comctl32.dll in active context, %lu\n", GetLastError());
    if (ret)
    {
        FreeLibrary(hmod);
        LoadLibraryA("comctl32.dll");
    }

    return ret;
}

#undef ARCH
