/*
 * Unit test suite for virtual substituted drive functions.
 *
 * Copyright 2017 Giannis Adamopoulos
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

#include "precomp.h"

#define STRSECTION_MAGIC   0x64487353 /* dHsS */

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

struct wndclass_redirect_data
{
    ULONG size;
    DWORD res;
    ULONG name_len;
    ULONG name_offset;   /* versioned name offset */
    ULONG module_len;
    ULONG module_offset; /* container name offset to the section base */
};

struct dllredirect_data
{
    ULONG size;
    ULONG unk;
    DWORD res[3];
};

#include <pshpack1.h>

struct assemply_data
{
    ULONG size;
    DWORD ulFlags;
    DWORD ulEncodedAssemblyIdentityLength;
    DWORD ulEncodedAssemblyIdentityOffset; /* offset to the section base */
    DWORD ulManifestPathType;
    DWORD ulManifestPathLength;
    DWORD ulManifestPathOffset;            /* offset to the section base */
    LARGE_INTEGER liManifestLastWriteTime;
    DWORD unk3[11];
    DWORD ulAssemblyDirectoryNameLength;
    DWORD ulAssemblyDirectoryNameOffset;   /* offset to the section base */
    DWORD unk4[3];  /* In win10 there are two more fields */
};

#include <poppack.h>

HANDLE _CreateActCtxFromFile(LPCWSTR FileName, int line)
{
    ACTCTXW ActCtx = {sizeof(ACTCTX)};
    HANDLE h;
    WCHAR buffer[MAX_PATH] , *separator;

    ok (GetModuleFileNameW(NULL, buffer, MAX_PATH), "GetModuleFileName failed\n");
    separator = wcsrchr(buffer, L'\\');
    if (separator)
        wcscpy(separator + 1, FileName);

    ActCtx.lpSource = buffer;

    SetLastError(0xdeaddead);
    h = CreateActCtxW(&ActCtx);
    ok_(__FILE__, line)(h != INVALID_HANDLE_VALUE, "CreateActCtx failed for %S\n", FileName);
    // In win10 last error is unchanged and in win2k3 it is ERROR_BAD_EXE_FORMAT
    ok_(__FILE__, line)(GetLastError() == ERROR_BAD_EXE_FORMAT || GetLastError() == 0xdeaddead, "Wrong last error %lu\n", GetLastError());

    return h;
}

VOID _ActivateCtx(HANDLE h, ULONG_PTR *cookie, int line)
{
    BOOL res;

    SetLastError(0xdeaddead);
    res = ActivateActCtx(h, cookie);
    ok_(__FILE__, line)(res == TRUE, "ActivateActCtx failed\n");
    ok_(__FILE__, line)(GetLastError() == 0xdeaddead, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(0xdeaddead), GetLastError());
}

VOID _DeactivateCtx(ULONG_PTR cookie, int line)
{
    BOOL res;

    SetLastError(0xdeaddead);
    res = DeactivateActCtx(0, cookie);
    ok_(__FILE__, line)(res == TRUE, "DeactivateActCtx failed\n");
    ok_(__FILE__, line)(GetLastError() == 0xdeaddead, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(0xdeaddead), GetLastError());
}

void TestClassRedirection(HANDLE h, LPCWSTR ClassToTest, LPCWSTR ExpectedClassPart, LPCWSTR ExpectedModule, ULONG ExpectedClassCount)
{
    ACTCTX_SECTION_KEYED_DATA KeyedData = { 0 };
    BOOL res;
    struct strsection_header *header;
    struct wndclass_redirect_data *classData;
    LPCWSTR VersionedClass, ClassLib;
    int data_lenght;

    SetLastError(0xdeaddead);
    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_WINDOW_CLASS_REDIRECTION,
                                   ClassToTest,
                                   &KeyedData);
    ok(res == TRUE, "FindActCtxSectionString failed\n");
    ok(GetLastError() == 0xdeaddead, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(0xdeaddead), GetLastError());

    ok(KeyedData.ulDataFormatVersion == 1, "Wrong format version: %lu\n", KeyedData.ulDataFormatVersion);
    ok(KeyedData.hActCtx == h, "Wrong handle\n");
    ok(KeyedData.lpSectionBase != NULL, "Expected non null lpSectionBase\n");
    ok(KeyedData.lpData != NULL, "Expected non null lpData\n");
    header = (struct strsection_header*)KeyedData.lpSectionBase;
    classData = (struct wndclass_redirect_data*)KeyedData.lpData;

    if(res == FALSE || KeyedData.ulDataFormatVersion != 1 || header == NULL || classData == NULL)
    {
        skip("Can't read data for class. Skipping\n");
    }
    else
    {
        ok(header->magic == STRSECTION_MAGIC, "%lu\n", header->magic );
        ok(header->size == sizeof(*header), "Got %lu instead of %d\n", header->size, sizeof(*header));
        ok(header->count == ExpectedClassCount, "Expected %lu classes, got %lu\n", ExpectedClassCount, header->count );

        VersionedClass = (WCHAR*)((BYTE*)classData + classData->name_offset);
        ClassLib = (WCHAR*)((BYTE*)header + classData->module_offset);
        data_lenght = classData->size + classData->name_len + classData->module_len + 2*sizeof(WCHAR);
        ok(KeyedData.ulLength == data_lenght, "Got lenght %lu instead of %d\n", KeyedData.ulLength, data_lenght);
        ok(classData->size == sizeof(*classData), "Got %lu instead of %d\n", classData->size, sizeof(*classData));
        ok(classData->res == 0, "Got res %lu\n", classData->res);
        ok(classData->module_len == wcslen(ExpectedModule) * 2, "Got name len %lu, expected %d\n", classData->module_len, wcslen(ExpectedModule) *2);
        ok(wcscmp(ClassLib, ExpectedModule) == 0, "Got %S, expected %S\n", ClassLib, ExpectedModule);
        /* compare only if VersionedClass starts with ExpectedClassPart */
        ok(memcmp(VersionedClass, ExpectedClassPart, sizeof(WCHAR) * wcslen(ExpectedClassPart)) == 0, "Expected %S to start with %S\n", VersionedClass, ExpectedClassPart);
    }
}

VOID TestLibDependency(HANDLE h)
{
    ACTCTX_SECTION_KEYED_DATA KeyedData = { 0 };
    BOOL res;
    struct strsection_header *SectionHeader;
    struct dllredirect_data *redirData;
    struct assemply_data *assemplyData;

    SetLastError(0xdeaddead);
    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                   L"dep1.dll",
                                   &KeyedData);
    ok(res == TRUE, "FindActCtxSectionString failed\n");
    ok(GetLastError() == 0xdeaddead, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(0xdeaddead), GetLastError());

    ok(KeyedData.ulDataFormatVersion == 1, "Wrong format version: %lu", KeyedData.ulDataFormatVersion);
    ok(KeyedData.hActCtx == h, "Wrong handle\n");
    ok(KeyedData.lpSectionBase != NULL, "Expected non null lpSectionBase\n");
    ok(KeyedData.lpData != NULL, "Expected non null lpData\n");
    SectionHeader = (struct strsection_header*)KeyedData.lpSectionBase;
    redirData = (struct dllredirect_data *)KeyedData.lpData;

    if(res == FALSE || KeyedData.ulDataFormatVersion != 1 || SectionHeader == NULL || redirData == NULL)
    {
        skip("Can't read data for dep1.dll. Skipping\n");
    }
    else
    {
        ok(SectionHeader->magic == STRSECTION_MAGIC, "%lu\n", SectionHeader->magic );
        ok(SectionHeader->size == sizeof(*SectionHeader), "Got %lu instead of %d\n", SectionHeader->size, sizeof(*SectionHeader));
        ok(SectionHeader->count == 2, "%lu\n", SectionHeader->count ); /* 2 dlls? */
        ok(redirData->size == sizeof(*redirData), "Got %lu instead of %d\n", redirData->size, sizeof(*redirData));
    }

    SetLastError(0xdeaddead);
    KeyedData.cbSize = sizeof(KeyedData);
    res = FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX,
                                   NULL,
                                   ACTIVATION_CONTEXT_SECTION_ASSEMBLY_INFORMATION,
                                   L"dep1",
                                   &KeyedData);
    ok(res == TRUE, "FindActCtxSectionString failed\n");
    ok(GetLastError() == 0xdeaddead, "Wrong last error. Expected %lu, got %lu\n", (DWORD)(0xdeaddead), GetLastError());
    ok(KeyedData.ulDataFormatVersion == 1, "Wrong format version: %lu", KeyedData.ulDataFormatVersion);
    ok(KeyedData.hActCtx == h, "Wrong handle\n");
    ok(KeyedData.lpSectionBase != NULL, "Expected non null lpSectionBase\n");
    ok(KeyedData.lpData != NULL, "Expected non null lpData\n");
    SectionHeader = (struct strsection_header*)KeyedData.lpSectionBase;
    assemplyData = (struct assemply_data*)KeyedData.lpData;;

    if(res == FALSE || KeyedData.ulDataFormatVersion != 1 || SectionHeader == NULL || assemplyData == NULL)
    {
        skip("Can't read data for dep1. Skipping\n");
    }
    else
    {
        LPCWSTR AssemblyIdentity, ManifestPath, AssemblyDirectory;
        int data_lenght;
        DWORD buffer[256];
        PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION details = (PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION)buffer;

        ok(SectionHeader->magic == STRSECTION_MAGIC, "%lu\n", SectionHeader->magic );
        ok(SectionHeader->size == sizeof(*SectionHeader), "Got %lu instead of %d\n", SectionHeader->size, sizeof(*SectionHeader));
        ok(SectionHeader->count == 2, "%lu\n", SectionHeader->count ); /* 2 dlls? */

        data_lenght = assemplyData->size +
                      assemplyData->ulEncodedAssemblyIdentityLength +
                      assemplyData->ulManifestPathLength +
                      assemplyData->ulAssemblyDirectoryNameLength + 2 * sizeof(WCHAR);
        ok(assemplyData->size == sizeof(*assemplyData), "Got %lu instead of %d\n", assemplyData->size, sizeof(*assemplyData));
        ok(KeyedData.ulLength == data_lenght, "Got lenght %lu instead of %d\n", KeyedData.ulLength, data_lenght);

        AssemblyIdentity = (WCHAR*)((BYTE*)SectionHeader + assemplyData->ulEncodedAssemblyIdentityOffset);
        ManifestPath = (WCHAR*)((BYTE*)SectionHeader + assemplyData->ulManifestPathOffset);
        AssemblyDirectory = (WCHAR*)((BYTE*)SectionHeader + assemplyData->ulAssemblyDirectoryNameOffset);

        /* Use AssemblyDetailedInformationInActivationContext so as to infer the contents of assemplyData */
        res = QueryActCtxW(0, h, &KeyedData.ulAssemblyRosterIndex,
                           AssemblyDetailedInformationInActivationContext,
                           &buffer, sizeof(buffer), NULL);
        ok(res == TRUE, "QueryActCtxW failed\n");
        ok(assemplyData->ulFlags == details->ulFlags, "\n");
        ok(assemplyData->ulEncodedAssemblyIdentityLength == details->ulEncodedAssemblyIdentityLength, "\n");
        ok(assemplyData->ulManifestPathType == details->ulManifestPathType, "\n");
        ok(assemplyData->ulManifestPathLength == details->ulManifestPathLength, "\n");
        ok(assemplyData->ulAssemblyDirectoryNameLength == details->ulAssemblyDirectoryNameLength, "\n");
        ok(assemplyData->liManifestLastWriteTime.QuadPart == details->liManifestLastWriteTime.QuadPart, "\n");

        ok(wcscmp(ManifestPath, details->lpAssemblyManifestPath) == 0, "Expected path %S, got %S\n", details->lpAssemblyManifestPath, ManifestPath);
        ok(wcscmp(AssemblyDirectory, details->lpAssemblyDirectoryName) == 0, "Expected path %S, got %S\n", details->lpAssemblyManifestPath, ManifestPath);

        /* It looks like that AssemblyIdentity isn't null terminated */
        ok(memcmp(AssemblyIdentity, details->lpAssemblyEncodedAssemblyIdentity, assemplyData->ulEncodedAssemblyIdentityLength) == 0, "Got wrong AssemblyIdentity\n");
    }
}

START_TEST(FindActCtxSectionStringW)
{
    HANDLE h, h2;
    ULONG_PTR cookie, cookie2;

    /*First run the redirection tests without using our own actctx */
    TestClassRedirection(NULL, L"Button", L"Button", L"comctl32.dll", 27);
    /* Something activates an activation context that mentions comctl32 but comctl32 is not loaded */
    ok( GetModuleHandleW(L"comctl32.dll") == NULL, "Expected comctl32 not to be loaded\n");
    ok( GetModuleHandleW(L"user32.dll") == NULL, "Expected user32 not to be loaded\n");

    /* Class redirection tests */
    h = _CreateActCtxFromFile(L"classtest.manifest", __LINE__);
    if (h != INVALID_HANDLE_VALUE)
    {
        _ActivateCtx(h, &cookie, __LINE__);
        TestClassRedirection(h, L"Button", L"2.2.2.2!Button", L"testlib.dll", 5);
        _ActivateCtx(NULL, &cookie2, __LINE__);
        TestClassRedirection(NULL, L"Button", L"Button", L"comctl32.dll", 27);
        _DeactivateCtx(cookie2, __LINE__);
        _DeactivateCtx(cookie, __LINE__);
    }
    else
    {
        skip("Failed to create context for classtest.manifest\n");
    }

    /* Class redirection tests with multiple contexts in the activation stack */
    h2 = _CreateActCtxFromFile(L"classtest2.manifest", __LINE__);
    if (h != INVALID_HANDLE_VALUE && h2 != INVALID_HANDLE_VALUE)
    {
        _ActivateCtx(h, &cookie, __LINE__);
        _ActivateCtx(h2, &cookie2, __LINE__);
        TestClassRedirection(NULL, L"Button", L"Button", L"comctl32.dll", 27);
        TestClassRedirection(h2, L"MyClass", L"1.1.1.1!MyClass", L"testlib.dll", 5);
        _DeactivateCtx(cookie2, __LINE__);
        TestClassRedirection(h, L"Button", L"2.2.2.2!Button", L"testlib.dll", 5);
        _DeactivateCtx(cookie, __LINE__);
    }
    else
    {
        skip("Failed to create context for classtest.manifest\n");
    }

    /* Dependency tests */
    h = _CreateActCtxFromFile(L"deptest.manifest", __LINE__);
    if (h != INVALID_HANDLE_VALUE)
    {
        _ActivateCtx(h, &cookie, __LINE__);
        TestLibDependency(h);
        _DeactivateCtx(cookie, __LINE__);
    }
    else
    {
        skip("Failed to create context for deptest.manifest\n");
    }

    /* Activate a context that depends on comctl32 v6 and run class tests again */
    h = _CreateActCtxFromFile(L"comctl32dep.manifest", __LINE__);
    if (h != INVALID_HANDLE_VALUE)
    {
        _ActivateCtx(h, &cookie, __LINE__);
        TestClassRedirection(h, L"Button", L"6.0.", L"comctl32.dll", 29);
        ok( GetModuleHandleW(L"comctl32.dll") == NULL, "Expected comctl32 not to be loaded\n");
        ok( GetModuleHandleW(L"user32.dll") == NULL, "Expected user32 not to be loaded\n");
        _DeactivateCtx(cookie, __LINE__);
    }
    else
    {
        skip("Failed to create context for comctl32dep.manifest\n");
    }
}