/*
 * PROJECT:     apphelp_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests showing shim artifacts in the environment
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <shlwapi.h>
#include <winnt.h>
#include <userenv.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif
#include <winerror.h>
#include <stdio.h>

#include "wine/test.h"

#include "apphelp_apitest.h"

typedef void* HSDB;
typedef void* PDB;
typedef DWORD TAGREF;
typedef WORD TAG;



static HMODULE hdll;

BOOL(WINAPI *pSdbGetMatchingExe)(HSDB hsdb, LPCWSTR szPath, LPCWSTR szModuleName, LPCWSTR pszEnvironment, DWORD dwFlags, PSDBQUERYRESULT_VISTA pQueryResult);
HSDB (WINAPI *pSdbInitDatabase)(DWORD dwFlags, LPCWSTR pszDatabasePath);
void (WINAPI *pSdbReleaseDatabase)(HSDB hsdb);
BOOL (WINAPI *pSdbTagRefToTagID)(HSDB hsdb, TAGREF trWhich, PDB* ppdb, TAGID* ptiWhich);
TAG (WINAPI *pSdbGetTagFromTagID)(PDB pdb, TAGID tiWhich);
TAGREF (WINAPI *pSdbGetLayerTagRef)(HSDB hsdb, LPCWSTR layerName);


/* TODO: Investigate ApphelpCheckRunApp, for some reason there is not AppCompatData generated... */

BOOL (WINAPI *pApphelpCheckRunAppEx_w7)(HANDLE FileHandle, PVOID Unk1, PVOID Unk2, PWCHAR ApplicationName, PVOID Environment, USHORT ExeType, PULONG Reason,
                                   PVOID* SdbQueryAppCompatData, PULONG SdbQueryAppCompatDataSize, PVOID* SxsData, PULONG SxsDataSize,
                                   PULONG FusionFlags, PULONG64 SomeFlag1, PULONG SomeFlag2);

BOOL (WINAPI *pApphelpCheckRunAppEx_w10)(HANDLE FileHandle, PVOID Unk1, PVOID Unk2, PWCHAR ApplicationName, PVOID Environment, PVOID Unk3, USHORT ExeType, PULONG Reason,
                                        PVOID* SdbQueryAppCompatData, PULONG SdbQueryAppCompatDataSize, PVOID* SxsData, PULONG SxsDataSize,
                                        PULONG FusionFlags, PULONG64 SomeFlag1, PULONG SomeFlag2);


BOOL (WINAPI *pSdbPackAppCompatData)(HSDB hsdb, PSDBQUERYRESULT_VISTA pQueryResult, PVOID* ppData, DWORD *dwSize);
BOOL (WINAPI *pSdbUnpackAppCompatData)(HSDB hsdb, LPCWSTR pszImageName, PVOID pData, PSDBQUERYRESULT_VISTA pQueryResult);
DWORD (WINAPI *pSdbGetAppCompatDataSize)(PVOID pData);


static HSDB g_LayerDB;
static DWORD g_ShimDataSize;
static DWORD g_ModuleVersion;
static const SDBQUERYRESULT_VISTA empty_result = { { 0 } };
static const SDBQUERYRESULT_VISTA almost_empty = { { 0 }, { 0 }, { 0 }, 0, 0, 0, 0, { 0 }, SHIMREG_DISABLE_LAYER, 0 };


#define SHIMDATA_MAGIC  0xAC0DEDAB
#define MAX_LAYER_LENGTH            256


typedef struct ShimData_Win2k3
{
    WCHAR szModule[34];
    DWORD dwSize;
    DWORD dwMagic;

    TAGREF atrExes[SDB_MAX_EXES_2k3];
    TAGREF atrLayers[SDB_MAX_LAYERS];
    DWORD dwUnk0;
    DWORD dwUnk1;
    DWORD dwCustomSDBMap;
    GUID rgGuidDB[SDB_MAX_SDBS];
} ShimData_Win2k3;



typedef struct ShimData_Win7
{
    WCHAR szModule[260];
    DWORD dwSize;
    DWORD dwMagic;
    SDBQUERYRESULT_VISTA Query;
    WCHAR szLayer[MAX_LAYER_LENGTH];
    DWORD unknown;  // 0x14c
} ShimData_Win7;

typedef struct ShimData_Win10_v1
{
    WCHAR szModule[260];
    DWORD dwSize;
    DWORD dwMagic;
    DWORD unk1;
    SDBQUERYRESULT_VISTA Query;
    WCHAR szLayer[MAX_LAYER_LENGTH];
    char padding1[0x200];
    char padding2[0x404];   // Contains some data at the start
    DWORD unk2;
    DWORD unk3;
    WCHAR processname[MAX_PATH];
    WCHAR szLayerEnv[MAX_LAYER_LENGTH];
    WCHAR unk4[MAX_LAYER_LENGTH];
    char padding4[120];
} ShimData_Win10_v1;

typedef struct ShimData_Win10_v2
{
    DWORD dwSize;
    DWORD dwMagic;
    DWORD unk1;
    DWORD unk2;
    SDBQUERYRESULT_VISTA Query;
    WCHAR szLayer[MAX_LAYER_LENGTH];
    char padding1[0x200];
    char padding2[0x2ae + 0x54 + 0x2a + 0x16 + 0x16];
    DWORD unk3;
    DWORD unk4;
    WCHAR processname[MAX_PATH-2];
    WCHAR szLayerEnv[MAX_LAYER_LENGTH];
    WCHAR unk5[MAX_LAYER_LENGTH];
    char padding4[76];
} ShimData_Win10_v2;

typedef struct ShimData_QueryOffset
{
    DWORD dwSize_10_v2;
    DWORD dwMagic_10_v2;

    char spacing1[60];

    DWORD dwSize_2k3;
    DWORD dwMagic_2k3;

    char spacing2[444];

    DWORD dwSize_7_10;
    DWORD dwMagic_7_10;
} ShimData_QueryOffset;


C_ASSERT(sizeof(ShimData_Win2k3) == 392);
C_ASSERT(sizeof(ShimData_Win7) == 1500);
C_ASSERT(sizeof(ShimData_Win10_v1) == 4712);
C_ASSERT(sizeof(ShimData_Win10_v2) == 3976);

C_ASSERT(offsetof(ShimData_Win10_v2, dwSize) == 0);
C_ASSERT(offsetof(ShimData_Win2k3, dwSize) == 68);
C_ASSERT(offsetof(ShimData_Win7, dwSize) == 520);
C_ASSERT(offsetof(ShimData_Win10_v1, dwSize) == 520);

C_ASSERT(offsetof(ShimData_Win10_v2, dwMagic) == 4);
C_ASSERT(offsetof(ShimData_Win2k3, dwMagic) == 72);
C_ASSERT(offsetof(ShimData_Win7, dwMagic) == 524);
C_ASSERT(offsetof(ShimData_Win10_v1, dwMagic) == 524);

C_ASSERT(offsetof(ShimData_QueryOffset, dwSize_10_v2) == 0);
C_ASSERT(offsetof(ShimData_QueryOffset, dwSize_2k3) == 68);
C_ASSERT(offsetof(ShimData_QueryOffset, dwSize_7_10) == 520);

C_ASSERT(offsetof(ShimData_QueryOffset, dwMagic_10_v2) == 4);
C_ASSERT(offsetof(ShimData_QueryOffset, dwMagic_2k3) == 72);
C_ASSERT(offsetof(ShimData_QueryOffset, dwMagic_7_10) == 524);



#define SDB_DATABASE_MAIN_SHIM  0x80030000

#define SDBGMEF_IGNORE_ENVIRONMENT 0x1


typedef struct test_RemoteShimInfo
{
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    DWORD ShimDataSize;
    PVOID AppCompatInfo;
} test_RemoteShimInfo;


static BOOL readproc(HANDLE proc, LPVOID address, PVOID target, DWORD size)
{
    SIZE_T dwRead;
    if (ReadProcessMemory(proc, address, target, size, &dwRead))
    {
        ok(dwRead == size, "Expected to read %u bytes, got %lu\n", size, dwRead);
        return dwRead == size;
    }
    ok(0, "RPM failed with %u\n", GetLastError());
    return FALSE;
}

static BOOL get_shiminfo(HANDLE proc, test_RemoteShimInfo* info)
{
    PROCESS_BASIC_INFORMATION pbi = { 0 };
    ULONG sizeOut = 0;
    NTSTATUS status = NtQueryInformationProcess(proc, ProcessBasicInformation, &pbi, sizeof(pbi), &sizeOut);
    ok(NT_SUCCESS(status), "Expected NtQI to succeed, but failed with: %x\n", status);
    memset(info, 0, sizeof(*info));
    if (NT_SUCCESS(status))
    {
        PEB peb = { 0 };
        if (readproc(proc, pbi.PebBaseAddress, &peb, sizeof(peb)))
        {
            MEMORY_BASIC_INFORMATION mbi = { 0 };
            SIZE_T dwRead;

            info->AppCompatFlags = peb.AppCompatFlags;
            info->AppCompatFlagsUser = peb.AppCompatFlagsUser;
            info->AppCompatInfo = peb.AppCompatInfo;
            if (peb.pShimData == NULL)
                return TRUE;

            dwRead = VirtualQueryEx(proc, (LPCVOID)peb.pShimData, &mbi, sizeof(mbi));
            ok(dwRead == sizeof(mbi), "Expected VQE to return %u, got %lu\n", sizeof(mbi), dwRead);
            if (dwRead == sizeof(mbi) || peb.pShimData == NULL)
            {
                info->ShimDataSize = mbi.RegionSize;
                info->pShimData = malloc(mbi.RegionSize);
                if (readproc(proc, peb.pShimData, info->pShimData, mbi.RegionSize))
                    return TRUE;
                free(info->pShimData);
                info->pShimData = NULL;
            }
        }
    }
    return FALSE;
}

static HANDLE create_proc(BOOL suspended)
{
    static char proc_name[MAX_PATH] = { 0 };
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    BOOL res;
    if (!proc_name[0])
    {
        GetModuleFileNameA(NULL, proc_name, MAX_PATH);
    }

    res = CreateProcessA(NULL, proc_name, NULL, NULL, TRUE, suspended ? CREATE_SUSPENDED : 0, NULL, NULL, &si, &pi);
    if (!res)
        return NULL;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

static void create_environ(const char* layers[], size_t num)
{
    char buf[256] = { 0 };
    size_t n;
    for (n = 0; n < num; ++n)
    {
        if (n)
            strcat(buf, " ");
        strcat(buf, layers[n]);
    }
    SetEnvironmentVariableA("__COMPAT_LAYER", buf);
}

static void ValidateShim(TAGREF trLayer, const char* name)
{
    HSDB hsdb = pSdbInitDatabase(SDB_DATABASE_MAIN_SHIM, NULL);
    ok(hsdb != NULL, "Expected a valid handle\n");
    if (hsdb)
    {
        PDB pdb = NULL;
        TAGID tagid = 0xdeadbeef;
        WCHAR nameW[256] = { 0 };
        BOOL ret;

        mbstowcs(nameW, name, strlen(name));

        ret = pSdbTagRefToTagID(hsdb, trLayer, &pdb, &tagid);
        ok(ret == TRUE, "Expected pSdbTagRefToTagID to succeed\n");
        if (ret)
        {
            TAG tag;
            ok(pdb != NULL, "Expected pdb to be a valid pointer\n");
            ok(tagid != 0 && tagid != 0xdeadbeef, "Expected tagid to be a valid tag id, was: 0x%x\n", tagid);
            tag = pSdbGetTagFromTagID(pdb, tagid);
            ok(tag == 0x700b, "Expected tag to be 0x700b, was 0x%x\n", (DWORD)tag);
        }

        pSdbReleaseDatabase(hsdb);
    }
}


static void Validate_ShimData_Win2k3(PVOID data, size_t count, const char* layers[])
{
    //size_t n;
    ShimData_Win2k3* pShimData = (ShimData_Win2k3*)data;

    ok(!lstrcmpW(pShimData->szModule, L"ShimEng.dll"), "Expected pShimData->Module to be %s, was %s\n", wine_dbgstr_w(L"ShimEng.dll"), wine_dbgstr_w(pShimData->szModule));
    ok(pShimData->dwMagic == SHIMDATA_MAGIC, "Expected pShimData->dwMagic to be 0x%x, was 0x%x\n", SHIMDATA_MAGIC, pShimData->dwMagic);
    ok(pShimData->dwSize == sizeof(ShimData_Win2k3), "Expected pShimData->dwSize to be %u, was %u\n", sizeof(ShimData_Win2k3), pShimData->dwSize);
    ok(pShimData->dwCustomSDBMap == 1, "Expected pShimData->dwCustomSDBMap to be 1, was %u\n", pShimData->dwCustomSDBMap);
}

static void Validate_ShimData_Win7(PVOID data, WCHAR szApphelp[256], size_t count, const char* layers[])
{
    size_t n;
    ShimData_Win7* pShimData = (ShimData_Win7*)data;

    ok(!lstrcmpW(pShimData->szModule, szApphelp), "Expected pShimData->Module to be %s, was %s\n",
        wine_dbgstr_w(szApphelp), wine_dbgstr_w(pShimData->szModule));
    ok(pShimData->dwMagic == SHIMDATA_MAGIC, "Expected pShimData->dwMagic to be 0x%x, was 0x%x\n",
        SHIMDATA_MAGIC, pShimData->dwMagic);
    ok(pShimData->dwSize == sizeof(ShimData_Win7), "Expected pShimData->dwSize to be %u, was %u\n",
        sizeof(ShimData_Win7), pShimData->dwSize);
    if (pShimData->Query.dwLayerCount != min(count, SDB_MAX_LAYERS))
    {
        char buf[250] = {0};
        GetEnvironmentVariableA("__COMPAT_LAYER", buf, _countof(buf));
        trace("At test: %s\n", buf);
    }
    ok(pShimData->Query.dwLayerCount == min(count, SDB_MAX_LAYERS),
        "Expected LayerCount to be %u, was %u\n", min(count, SDB_MAX_LAYERS), pShimData->Query.dwLayerCount);
    for (n = 0; n < SDB_MAX_LAYERS; ++n)
    {
        if (n < count)
        {
            ok(pShimData->Query.atrLayers[n] != 0, "Expected to find a valid layer in index %u / %u\n", n, count);
            ValidateShim(pShimData->Query.atrLayers[n], layers[n]);
        }
        else
            ok(pShimData->Query.atrLayers[n] == 0, "Expected to find an empty layer in index %u / %u\n", n, count);
    }
    ok(pShimData->unknown == 0x14c, "Expected pShimData->unknown to be 0x14c, was 0x%x\n", pShimData->unknown);
}

static void Validate_ShimData_Win10_v2(PVOID data, WCHAR szApphelp[256], size_t count, const char* layers[])
{
    size_t n;
    ShimData_Win10_v2* pShimData = (ShimData_Win10_v2*)data;

    if (pShimData->dwMagic != SHIMDATA_MAGIC)
    {
        skip("Yet another unknown shimdata variant...\n");
        return;
    }

    ok(pShimData->dwSize == sizeof(ShimData_Win10_v2), "Expected pShimData->dwSize to be %u, was %u\n",
       sizeof(ShimData_Win10_v2), pShimData->dwSize);
    if (pShimData->Query.dwLayerCount != min(count, SDB_MAX_LAYERS))
    {
        char buf[250] = {0};
        GetEnvironmentVariableA("__COMPAT_LAYER", buf, _countof(buf));
        trace("At test: %s\n", buf);
    }
    ok(pShimData->Query.dwLayerCount == min(count, SDB_MAX_LAYERS),
       "Expected LayerCount to be %u, was %u\n", min(count, SDB_MAX_LAYERS), pShimData->Query.dwLayerCount);
    for (n = 0; n < SDB_MAX_LAYERS; ++n)
    {
        if (n < count)
        {
            ok(pShimData->Query.atrLayers[n] != 0, "Expected to find a valid layer in index %u / %u\n", n, count);
            ValidateShim(pShimData->Query.atrLayers[n], layers[n]);
        }
        else
            ok(pShimData->Query.atrLayers[n] == 0, "Expected to find an empty layer in index %u / %u\n", n, count);
    }

}

static void Validate_ShimData_Win10(PVOID data, WCHAR szApphelp[256], size_t count, const char* layers[])
{
    size_t n;
    ShimData_Win10_v1* pShimData = (ShimData_Win10_v1*)data;

    if (pShimData->dwMagic != SHIMDATA_MAGIC)
    {
        Validate_ShimData_Win10_v2(data, szApphelp, count, layers);
        return;
    }


    ok(!lstrcmpiW(pShimData->szModule, szApphelp), "Expected pShimData->Module to be %s, was %s\n",
        wine_dbgstr_w(szApphelp), wine_dbgstr_w(pShimData->szModule));
    ok(pShimData->dwSize == sizeof(ShimData_Win10_v1), "Expected pShimData->dwSize to be %u, was %u\n",
        sizeof(ShimData_Win10_v1), pShimData->dwSize);
    if (pShimData->Query.dwLayerCount != min(count, SDB_MAX_LAYERS))
    {
        char buf[250] = {0};
        GetEnvironmentVariableA("__COMPAT_LAYER", buf, _countof(buf));
        trace("At test: %s\n", buf);
    }
    ok(pShimData->Query.dwLayerCount == min(count, SDB_MAX_LAYERS),
        "Expected LayerCount to be %u, was %u\n", min(count, SDB_MAX_LAYERS), pShimData->Query.dwLayerCount);
    for (n = 0; n < SDB_MAX_LAYERS; ++n)
    {
        if (n < count)
        {
            ok(pShimData->Query.atrLayers[n] != 0, "Expected to find a valid layer in index %u / %u\n", n, count);
            ValidateShim(pShimData->Query.atrLayers[n], layers[n]);
        }
        else
            ok(pShimData->Query.atrLayers[n] == 0, "Expected to find an empty layer in index %u / %u\n", n, count);
    }
}

static void Validate_EmptyShimData_Win10(PVOID data)
{
    ShimData_Win10_v1* pShimData = (ShimData_Win10_v1*)data;
    ok(pShimData != NULL, "Expected pShimData\n");
    if (!pShimData)
        return;

    if (pShimData->dwMagic != SHIMDATA_MAGIC)
    {
        ShimData_Win10_v2* pShimData2 = (ShimData_Win10_v2*)data;
        if (pShimData2->dwMagic != SHIMDATA_MAGIC)
        {
            skip("Unknown shimdata (win10)\n");
            return;
        }

        ok(!lstrcmpiW(pShimData2->szLayer, L""), "Expected pShimData->szLayer to be '', was %s\n", wine_dbgstr_w(pShimData2->szLayer));
        ok(pShimData2->dwSize == sizeof(ShimData_Win10_v2), "Expected pShimData->dwSize to be %u, was %u\n", sizeof(ShimData_Win10_v2), pShimData2->dwSize);
        ok(!memcmp(&pShimData2->Query, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
    }
    else
    {
        ok(!lstrcmpiW(pShimData->szModule, L""), "Expected pShimData->Module to be '', was %s\n", wine_dbgstr_w(pShimData->szModule));
        ok(!lstrcmpiW(pShimData->szLayer, L""), "Expected pShimData->szLayer to be '', was %s\n", wine_dbgstr_w(pShimData->szLayer));
        ok(pShimData->dwSize == sizeof(ShimData_Win10_v1), "Expected pShimData->dwSize to be %u, was %u\n", sizeof(ShimData_Win10_v1), pShimData->dwSize);
        ok(!memcmp(&pShimData->Query, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
    }
}

static void Test_layers(WCHAR szApphelp[256])
{
    static const char* layers[] = {
        "256Color", "NT4SP5", "DisableNXHideUI", "DisableNXShowUI",
        "WIN2000SP3", "640X480", /*"DISABLEDWM",*/ "HIGHDPIAWARE",
        /*"RUNASADMIN",*/ "DISABLETHEMES" /*, "Layer_Win95VersionLie"*/ };

    size_t n;
    HANDLE proc;
    test_RemoteShimInfo info;
    BOOL res;

    for (n = 0; n <= (sizeof(layers) / sizeof(layers[0])); ++n)
    {
        create_environ(layers, n);

        proc = create_proc(TRUE);
        res = get_shiminfo(proc, &info);
        TerminateProcess(proc, 0);
        CloseHandle(proc);

        if (!res)
        {
            ok(0, "Unable to get process info (%u)!\n", n);
            continue;
        }

        if (n == 0)
        {
            ok(info.AppCompatFlags.QuadPart == 0, "Expected AppCompatFlags to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlags.QuadPart));
            ok(info.AppCompatFlagsUser.QuadPart == 0, "Expected AppCompatFlagsUser to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlagsUser.QuadPart));
            ok(info.AppCompatInfo == NULL, "Expected AppCompatInfo to be NULL, was: %p\n", info.AppCompatInfo);
            if (g_WinVersion < WINVER_WIN10)
            {
                ok(info.pShimData == NULL, "Expected pShimData to be NULL, was: %p\n", info.pShimData);
            }
            else
            {
                Validate_EmptyShimData_Win10(info.pShimData);
            }
        }
        else
        {
            ok(info.AppCompatFlags.QuadPart == 0, "Expected AppCompatFlags to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlags.QuadPart));
            ok(info.AppCompatFlagsUser.QuadPart == 0, "Expected AppCompatFlagsUser to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlagsUser.QuadPart));
            ok(info.AppCompatInfo == NULL, "Expected AppCompatInfo to be NULL, was: %p\n", info.AppCompatInfo);
            ok(info.pShimData != NULL, "Expected pShimData to be valid, was NULL\n");
            ok(info.ShimDataSize == g_ShimDataSize, "Expected ShimDataSize to be %u, was: %u\n", g_ShimDataSize, info.ShimDataSize);
            if (info.pShimData)
            {
                if (g_WinVersion < WINVER_VISTA)
                    Validate_ShimData_Win2k3(info.pShimData, n, layers);
                else if (g_WinVersion < WINVER_WIN10)
                    Validate_ShimData_Win7(info.pShimData, szApphelp, n, layers);
                else
                    Validate_ShimData_Win10(info.pShimData, szApphelp, n, layers);
            }
        }
        free(info.pShimData);
    }
}


/*
[Warn][SdbGetMatchingExe   ] __COMPAT_LAYER name cannot exceed 256 characters.
[Info][SdbpGetPermLayersInternal] Failed to read value info from Key "\Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" Status 0xc0000034
[Info][SdbpGetPermLayersInternal] Failed to read value info from Key "\REGISTRY\USER\S-1-5-21-4051718696-421402927-393408651-2638\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers" Status 0xc0000034
[Warn][SdbpEnumUserSdb     ] Failed to open key "\Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Custom\NotepadReplacer.exe" Status 0xc0000034
*/
static void Test_repeatlayer(WCHAR szApphelp[256])
{
    static const char* layers[] = {
        "256Color", "256Color", "256Color", "256Color",
        "256Color", "256Color", "256Color", "256Color" };

    HANDLE proc;
    test_RemoteShimInfo info;
    BOOL res;

    SetEnvironmentVariableA("__COMPAT_LAYER", "256Color 256Color 256Color 256Color 256Color 256Color 256Color 256Color");


    proc = create_proc(TRUE);
    res = get_shiminfo(proc, &info);
    TerminateProcess(proc, 0);
    CloseHandle(proc);

    if (res)
    {
        ok(info.AppCompatFlags.QuadPart == 0, "Expected AppCompatFlags to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlags.QuadPart));
        ok(info.AppCompatFlagsUser.QuadPart == 0, "Expected AppCompatFlagsUser to be 0, was: %s\n", wine_dbgstr_longlong(info.AppCompatFlagsUser.QuadPart));
        ok(info.AppCompatInfo == 0, "Expected AppCompatInfo to be 0, was: %p\n", info.AppCompatInfo);
        ok(info.pShimData != NULL, "Expected pShimData to be valid, was NULL\n");
        ok(info.ShimDataSize == g_ShimDataSize, "Expected ShimDataSize to be %u, was: %u\n", g_ShimDataSize, info.ShimDataSize);
        if (info.pShimData)
        {
            /* Win10 only 'loads' one layer */
            if (g_WinVersion < WINVER_VISTA)
                Validate_ShimData_Win2k3(info.pShimData, SDB_MAX_LAYERS, layers);
            else if (g_WinVersion < WINVER_WIN10)
                Validate_ShimData_Win7(info.pShimData, szApphelp, SDB_MAX_LAYERS, layers);
            else
                Validate_ShimData_Win10(info.pShimData, szApphelp, 1, layers);
        }
    }
    else
    {
        ok(0, "Unable to get process info!\n");
    }

}


TAGREF find_layer(const char* szLayerStart, const char* szLayerEnd)
{
    char layer[100] = { 0 };
    WCHAR layerW[100] = { 0 };
    strncpy(layer, szLayerStart, szLayerEnd - szLayerStart);

    if (!g_LayerDB)
    {
        g_LayerDB = pSdbInitDatabase(SDB_DATABASE_MAIN_SHIM, 0);
    }

    mbstowcs(layerW, layer, strlen(layer));
    return pSdbGetLayerTagRef(g_LayerDB, layerW);
}

static void expect_layeronly_imp(SDBQUERYRESULT_VISTA* result, const char* layers, DWORD flags)
{
    DWORD dwLayerCount = 0, n;
    TAGREF atrLayers[SDB_MAX_LAYERS] = { 0 };

    const char* layer = layers, *nextlayer;
    while (layer && *layer)
    {
        nextlayer = strchr(layer, ' ');
        atrLayers[dwLayerCount++] = find_layer(layer, nextlayer ? nextlayer : (layer + strlen(layer)));
        layer = nextlayer ? (nextlayer+1) : NULL;
    }

    if (g_ModuleVersion < WINVER_VISTA)
    {
        SDBQUERYRESULT_2k3* result2 = (SDBQUERYRESULT_2k3*)result;
        result = NULL;

        winetest_ok(!memcmp(&result2->atrExes, &empty_result.atrExes, sizeof(result2->atrExes)), "Expected atrExes to be empty\n");
        winetest_ok(!memcmp(&result2->atrLayers[dwLayerCount], &empty_result.atrLayers[dwLayerCount], sizeof(result2->atrLayers) - dwLayerCount * sizeof(result2->atrLayers[0])), "Expected atrLayers[+1] to be empty\n");
        for (n = 0; n < dwLayerCount; ++n)
        {
            winetest_ok(result2->atrLayers[n] == atrLayers[n], "Expected atrLayers[%u] to be %x, was %x\n",
                n, atrLayers[n], result2->atrLayers[n]);
        }
        winetest_ok(result2->dwLayerFlags == 0, "Expected dwLayerFlags to be 0, was %u\n", result2->dwLayerFlags);
        winetest_ok(result2->trApphelp == 0, "Expected trApphelp to be 0, was %u\n", result2->trApphelp);
        winetest_ok(result2->dwExeCount == 0, "Expected dwExeCount to be 0, was %u\n", result2->dwExeCount);
        winetest_ok(result2->dwLayerCount == dwLayerCount, "Expected dwLayerCount to be %u, was %u\n", dwLayerCount, result2->dwLayerCount);
        winetest_ok(!memcmp(&result2->guidID, &empty_result.guidID, sizeof(result2->guidID)), "Expected guidID to be empty\n");
        winetest_ok(result2->dwFlags == flags, "Expected dwFlags to be 0x%x, was 0x%x\n", flags, result2->dwFlags);
        winetest_ok(result2->dwCustomSDBMap == 1, "Expected dwCustomSDBMap to be 1, was %u\n", result2->dwCustomSDBMap);
        winetest_ok(!memcmp(&result2->rgGuidDB[1], &empty_result.rgGuidDB[1], sizeof(result2->rgGuidDB) - sizeof(result2->rgGuidDB[0])), "Expected rgGuidDB[+1] to be empty\n");
    }
    else
    {
        winetest_ok(!memcmp(&result->atrExes, &empty_result.atrExes, sizeof(empty_result.atrExes)), "Expected atrExes to be empty\n");
        winetest_ok(!memcmp(&result->adwExeFlags, &empty_result.adwExeFlags, sizeof(empty_result.adwExeFlags)), "Expected adwExeFlags to be empty\n");
        winetest_ok(!memcmp(&result->atrLayers[dwLayerCount], &empty_result.atrLayers[dwLayerCount], sizeof(empty_result.atrLayers) - dwLayerCount * sizeof(empty_result.atrLayers[0])), "Expected atrLayers[+1] to be empty\n");
        for (n = 0; n < dwLayerCount; ++n)
        {
            winetest_ok(result->atrLayers[n] == atrLayers[n], "Expected atrLayers[%u] to be %x, was %x\n",
                n, atrLayers[n], result->atrLayers[n]);
        }
        winetest_ok(result->dwLayerFlags == 0, "Expected dwLayerFlags to be 0, was %u\n", result->dwLayerFlags);
        winetest_ok(result->trApphelp == 0, "Expected trApphelp to be 0, was %u\n", result->trApphelp);
        winetest_ok(result->dwExeCount == 0, "Expected dwExeCount to be 0, was %u\n", result->dwExeCount);
        winetest_ok(result->dwLayerCount == dwLayerCount, "Expected dwLayerCount to be %u, was %u\n", dwLayerCount, result->dwLayerCount);
        winetest_ok(!memcmp(&result->guidID, &empty_result.guidID, sizeof(empty_result.guidID)), "Expected guidID to be empty\n");
        winetest_ok(result->dwFlags == flags, "Expected dwFlags to be 0x%x, was 0x%x\n", flags, result->dwFlags);
        winetest_ok(result->dwCustomSDBMap == 1, "Expected dwCustomSDBMap to be 1, was %u\n", result->dwCustomSDBMap);
        winetest_ok(!memcmp(&result->rgGuidDB[1], &empty_result.rgGuidDB[1], sizeof(empty_result.rgGuidDB) - sizeof(empty_result.rgGuidDB[0])), "Expected rgGuidDB[+1] to be empty\n");
    }
}

static void Test_Shimdata(SDBQUERYRESULT_VISTA* result, const WCHAR* szLayer)
{
    BOOL ret;
    PVOID pData;
    DWORD dwSize;

    pData = NULL;
    dwSize = 0;
    ret = pSdbPackAppCompatData(g_LayerDB, result, &pData, &dwSize);
    ok(ret == TRUE, "Expected ret to be TRUE\n");

    if (pData)
    {
        ShimData_Win2k3* pWin2k3;
        ShimData_Win7* pWin7;
        ShimData_Win10_v1* pWin10;
        ShimData_Win10_v2* pWin10_v2;
        SDBQUERYRESULT_VISTA result2 = { { 0 } };

        DWORD res = pSdbGetAppCompatDataSize(pData);
        ok_int(dwSize, res);
        switch(dwSize)
        {
        case sizeof(ShimData_Win2k3):
            pWin2k3 = (ShimData_Win2k3*)pData;
            ok_hex(pWin2k3->dwMagic, SHIMDATA_MAGIC);
            ok_int(pWin2k3->dwSize, dwSize);
            ok(pWin2k3->dwCustomSDBMap == 1, "Expected pWin2k3->dwCustomSDBMap to equal 1, was %u for %s\n", pWin2k3->dwCustomSDBMap, wine_dbgstr_w(szLayer));
            //ok(!memcmp(&pWin2k3->Query, result, sizeof(SDBQUERYRESULT_2k3)), "Expected pWin2k3->Query to equal result\n");
            //ok_wstr(pWin7->szLayer, szLayer);
            break;
        case sizeof(ShimData_Win7):
            pWin7 = (ShimData_Win7*)pData;
            ok_hex(pWin7->dwMagic, SHIMDATA_MAGIC);
            ok_int(pWin7->dwSize, dwSize);
            ok(!memcmp(&pWin7->Query, result, sizeof(*result)), "Expected pWin7->Query to equal result\n");
            ok_wstr(pWin7->szLayer, szLayer);
            break;
        case sizeof(ShimData_Win10_v1):
            pWin10 = (ShimData_Win10_v1*)pData;
            ok_hex(pWin10->dwMagic, SHIMDATA_MAGIC);
            ok_int(pWin10->dwSize, dwSize);
            ok(!memcmp(&pWin10->Query, result, sizeof(*result)), "Expected pWin10->Query to equal result\n");
            ok_wstr(pWin10->szLayerEnv, szLayer);
            ok_wstr(pWin10->szLayer, L"");
            break;
        case sizeof(ShimData_Win10_v2):
            pWin10_v2 = (ShimData_Win10_v2*)pData;
            ok_hex(pWin10_v2->dwMagic, SHIMDATA_MAGIC);
            ok_int(pWin10_v2->dwSize, dwSize);
            ok(!memcmp(&pWin10_v2->Query, result, sizeof(*result)), "Expected pWin10->Query to equal result\n");
            ok_wstr(pWin10_v2->szLayerEnv, szLayer);
            ok_wstr(pWin10_v2->szLayer, L"");
            break;
        default:
            skip("Unknown size %d\n", dwSize);
            break;
        }

        ret = pSdbUnpackAppCompatData(g_LayerDB, NULL, pData, &result2);
        ok(ret == TRUE, "Expected ret to be TRUE\n");
        /* TODO: For some reason 2k3 does not seem to output the database GUIDs,
            investigate when we have support for multible db's! */
        if (dwSize == sizeof(ShimData_Win2k3))
        {
            SDBQUERYRESULT_2k3* input = (SDBQUERYRESULT_2k3*)result;
            SDBQUERYRESULT_2k3* output = (SDBQUERYRESULT_2k3*)&result2;
            const GUID rgGuidDB0 = {0x11111111, 0x1111, 0x1111, {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}};

            // Check expected data.
            ok(input->dwLayerCount == 1,
               "Expected input->dwLayerCount to be 1, was %u for %s\n",
               input->dwLayerCount, wine_dbgstr_w(szLayer));
            ok(input->dwCustomSDBMap == 1,
               "Expected input->dwCustomSDBMap to be 1, was %u for %s\n",
               input->dwCustomSDBMap, wine_dbgstr_w(szLayer));
            ok(IsEqualGUID(&input->rgGuidDB[0], &rgGuidDB0),
               "Expected input->rgGuidDB[0] to be %s, was %s for %s\n",
               wine_dbgstr_guid(&rgGuidDB0), wine_dbgstr_guid(&input->rgGuidDB[0]), wine_dbgstr_w(szLayer));

            // Check missing data.
            ok(output->dwLayerCount == 0,
               "Expected output->dwLayerCount to be 0, was %u for %s\n",
               output->dwLayerCount, wine_dbgstr_w(szLayer));
            ok(output->dwCustomSDBMap == 0,
               "Expected output->dwCustomSDBMap to be 0, was %u for %s\n",
               output->dwCustomSDBMap, wine_dbgstr_w(szLayer));
            ok(IsEqualGUID(&output->rgGuidDB[0], &empty_result.rgGuidDB[0]),
               "Expected output->rgGuidDB[0] to be empty, was %s for %s\n",
               wine_dbgstr_guid(&output->rgGuidDB[0]), wine_dbgstr_w(szLayer));

            // Fake it for now, so the memcmp works.
            output->dwLayerCount = input->dwLayerCount;
            output->dwCustomSDBMap = input->dwCustomSDBMap;
            output->rgGuidDB[0] = input->rgGuidDB[0];
        }
        ok(!memcmp(&result2, result, sizeof(*result)), "Expected result2 to equal result for %s\n", wine_dbgstr_w(szLayer));

        RtlFreeHeap(GetProcessHeap(), 0, pData);
    }
}


#define expect_layeronly    (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_layeronly_imp


static void Test_GetMatchingExe(void)
{
    BOOL ret;
    SDBQUERYRESULT_VISTA result = { { 0 } };
    WCHAR self[MAX_PATH];
    DWORD flags = (g_WinVersion < WINVER_VISTA) ? 0 : ((g_WinVersion < WINVER_WIN10) ? 1 : 0x21);

    GetModuleFileNameW(NULL, self, MAX_PATH);
    SetEnvironmentVariableA("__COMPAT_LAYER", NULL);

    /* szPath cannot be NULL! */
    ret = pSdbGetMatchingExe(NULL, L"", NULL, NULL, 0, &result);
    ok(ret == FALSE, "Expected ret to be FALSE\n");
    ok(!memcmp(&result, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");

    result = empty_result;

    ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
    ok(ret == FALSE, "Expected ret to be FALSE\n");
    ok(!memcmp(&result, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");

    result = empty_result;
    SetEnvironmentVariableA("__COMPAT_LAYER", "Some_invalid_layer_name");

    ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
    ok(ret == FALSE, "Expected ret to be FALSE\n");
    if (g_WinVersion < WINVER_WIN10)
        ok(!memcmp(&result, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
    else
        ok(!memcmp(&result, &almost_empty, sizeof(almost_empty)), "Expected result to be almost empty\n");

    result = empty_result;
    SetEnvironmentVariableA("__COMPAT_LAYER", "256Color");

    ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
    ok(ret == TRUE, "Expected ret to be TRUE\n");
    expect_layeronly(&result, "256Color", flags);

    Test_Shimdata(&result, L"256Color");

    result = empty_result;
    SetEnvironmentVariableA("__COMPAT_LAYER", "640X480");

    ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
    ok(ret == TRUE, "Expected ret to be TRUE\n");
    expect_layeronly(&result, "640X480", flags);

    Test_Shimdata(&result, L"640X480");

    /* HIGHDPIAWARE does not exist in 2k3 */
    if (g_WinVersion > WINVER_2003)
    {
        result = empty_result;
        SetEnvironmentVariableA("__COMPAT_LAYER", "HIGHDPIAWARE");

        ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
        ok(ret == TRUE, "Expected ret to be TRUE\n");
        expect_layeronly(&result, "HIGHDPIAWARE", flags);

        Test_Shimdata(&result, L"HIGHDPIAWARE");

        result = empty_result;
        SetEnvironmentVariableA("__COMPAT_LAYER", "256Color HIGHDPIAWARE   640X480");

        ret = pSdbGetMatchingExe(NULL, self, NULL, NULL, 0, &result);
        ok(ret == TRUE, "Expected ret to be TRUE\n");
        expect_layeronly(&result, "256Color HIGHDPIAWARE 640X480", flags);

        Test_Shimdata(&result, L"256Color HIGHDPIAWARE   640X480");
    }
}


HANDLE xOpenFile(WCHAR* ApplicationName)
{
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE FileHandle;

    if (!RtlDosPathNameToNtPathName_U(ApplicationName, &FileName, NULL, NULL))
    {
        skip("Unable to translate %s to Nt path\n", wine_dbgstr_w(ApplicationName));
        return NULL;
    }


    InitializeObjectAttributes(&ObjectAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&FileHandle,
                        SYNCHRONIZE |
                        FILE_READ_ATTRIBUTES |
                        FILE_READ_DATA |
                        FILE_EXECUTE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT |
                        FILE_NON_DIRECTORY_FILE);

    RtlFreeUnicodeString(&FileName);

    if (!NT_SUCCESS(Status))
        return NULL;

    return FileHandle;
}


#define RESET_CHECKRUNAPP_VARS()\
    do { \
        if (AppCompatData && AppCompatData != &Query) { RtlFreeHeap(RtlGetProcessHeap(), 0, AppCompatData); } \
        ExeType = IMAGE_FILE_MACHINE_I386; \
        SxsDataSize = FusionFlags = Reason = 0; \
        SxsData = NULL; \
        memset(&Query, 0, sizeof(Query)); \
        AppCompatData = &Query; \
        AppCompatDataSize = 123456; \
    } while (0)

#define CHECK_BASICS()\
    do { \
        ok_hex(ret, TRUE); \
        ok(AppCompatData != NULL && AppCompatData != &Query, "Expected the pointer to be valid\n"); \
        ok_hex(AppCompatDataSize, sizeof(SDBQUERYRESULT_VISTA)); \
        ok(SxsData == NULL, "Expected the pointer to be NULL\n"); \
        ok_hex(SxsDataSize, 0); \
        ok_hex(FusionFlags, 0); \
    } while (0)

/* W10 does not seem to use the flags at all, so with this macro we can still test it below 10. */
#define CHECKREASON(value, w10dum) (g_ModuleVersion < WINVER_WIN10 ? value : w10dum)


static BOOL call_ApphelpCheckRunApp(HANDLE FileHandle, PWCHAR ApplicationName, PVOID Environment, USHORT ExeType,
                                    PULONG Reason, PVOID* SdbQueryAppCompatData, PULONG SdbQueryAppCompatDataSize,
                                    PVOID* SxsData, PULONG SxsDataSize, PULONG FusionFlags)
{
    ULONG64 SomeFlag1 = 0;
    ULONG SomeFlag2 = 0;

    if (pApphelpCheckRunAppEx_w7)
    {
        return pApphelpCheckRunAppEx_w7(FileHandle, NULL, NULL, ApplicationName, Environment, ExeType, Reason,
                                        SdbQueryAppCompatData, SdbQueryAppCompatDataSize, SxsData, SxsDataSize,
                                        FusionFlags, &SomeFlag1, &SomeFlag2);
    }

    if (pApphelpCheckRunAppEx_w10)
    {
        return pApphelpCheckRunAppEx_w10(FileHandle, NULL, NULL, ApplicationName, Environment, NULL, ExeType, Reason,
                                        SdbQueryAppCompatData, SdbQueryAppCompatDataSize, SxsData, SxsDataSize,
                                        FusionFlags, &SomeFlag1, &SomeFlag2);
    }

    return FALSE;
}





static void Test_ApphelpCheckRunApp(WCHAR szApphelp[256])
{
    BOOL ret;
    HANDLE FileHandle = NULL;
    WCHAR ApplicationName[MAX_PATH], EmptyName[1] = { 0 };
    DWORD expect_flags = (g_WinVersion < WINVER_WIN10) ? 1 : 0x21;

    USHORT ExeType;
    PVOID AppCompatData = NULL, SxsData, DuplicatedEnv, Environment;
    ULONG AppCompatDataSize, SxsDataSize, FusionFlags;
    ULONG Reason;
    SDBQUERYRESULT_VISTA Query;
    int n;
    /* this are the only interesting bits (with the exception of '1', which is there to invoke the 'default' case) */
    const int kTestBits = 0x70503;

    if (!pApphelpCheckRunAppEx_w7 && !pApphelpCheckRunAppEx_w10)
    {
        skip("No usable ApphelpCheckRunAppEx\n");
        return;
    }

    GetModuleFileNameW(NULL, ApplicationName, MAX_PATH);

    FileHandle = xOpenFile(ApplicationName);
    SetEnvironmentVariableA("__COMPAT_LAYER", NULL);
    if (!CreateEnvironmentBlock(&DuplicatedEnv, NULL, TRUE))
        DuplicatedEnv = NULL;
    ok(DuplicatedEnv != NULL, "Invalid env (%u)\n", GetLastError());

    /* First with the environment without __COMPAT_LAYER */
    RESET_CHECKRUNAPP_VARS();

    ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, DuplicatedEnv, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x30000, 0));
    ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");

    /* We need this to be set for tests later on. */
    SetEnvironmentVariableA("__COMPAT_LAYER", "256Color");

    if (g_ModuleVersion < WINVER_WIN10)
    {
        /* Showing that when an environment is passed in, that is used instead of the current.
           In Win10 this behavior is no longer observed */

        RESET_CHECKRUNAPP_VARS();

        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, DuplicatedEnv, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        ok_hex(Reason, CHECKREASON(0x30000, 0));
        ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
    }

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        if (!(kTestBits & (1<<n)))
            continue;
        RESET_CHECKRUNAPP_VARS();
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, DuplicatedEnv, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 2)
            ExpectedReason = 2;
        else if (ExpectedReason == 0x100)
            ExpectedReason = 0x30000;
        else
            ExpectedReason |= 0x30000;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
    }

    if (g_ModuleVersion < WINVER_WIN10)
    {
        /* Now, using a NULL env, showing that the current environment is used.
           W10 does no longer do this, so we skip this test here. */
        Environment = NULL;

        RESET_CHECKRUNAPP_VARS();

        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        ok_hex(Reason, CHECKREASON(0x50000, 0));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
            expect_layeronly(AppCompatData, "256Color", expect_flags);

        for (n = 0; n < 32; ++n)
        {
            ULONG ExpectedReason;
            RESET_CHECKRUNAPP_VARS();
            if (!(kTestBits & (1<<n)))
                continue;
            ExpectedReason = Reason = (1 << n);
            ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
                &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

            CHECK_BASICS();
            if (ExpectedReason == 2)
                ExpectedReason = 2;
            else if (ExpectedReason == 0x100)
                ExpectedReason = 0x50000;
            else if (ExpectedReason == 0x400)
                ExpectedReason = 0x30400;
            else
                ExpectedReason |= 0x50000;
            ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
            if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
            {
                if (ExpectedReason != 0x30400)
                    expect_layeronly(AppCompatData, "256Color", expect_flags);
                else
                    ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
            }
        }
    }

    /* Passing in an environment with __COMPAT_LAYER set */
    Environment = GetEnvironmentStringsW();

    RESET_CHECKRUNAPP_VARS();

    ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x50000, 0));
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        expect_layeronly(AppCompatData, "256Color", expect_flags);

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        if (!(kTestBits & (1<<n)))
            continue;
        RESET_CHECKRUNAPP_VARS();
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 2)
            ExpectedReason = 2;
        else if (ExpectedReason == 0x100)
            ExpectedReason = 0x50000;
        else if (ExpectedReason == 0x400)
            ExpectedReason = 0x30400;
        else
            ExpectedReason |= 0x50000;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        {
            if (ExpectedReason != 0x30400 || g_ModuleVersion >= WINVER_WIN10)
                expect_layeronly(AppCompatData, "256Color", expect_flags);
            else
                ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
        }
    }

    /* NULL file handle still works */
    RESET_CHECKRUNAPP_VARS();

    ret = call_ApphelpCheckRunApp(NULL, ApplicationName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x50000, 0));
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        expect_layeronly(AppCompatData, "256Color", expect_flags);

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        RESET_CHECKRUNAPP_VARS();
        if (!(kTestBits & (1<<n)))
            continue;
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(NULL, ApplicationName, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 2)
            ExpectedReason = 2;
        else if (ExpectedReason == 0x100)
            ExpectedReason = 0x50000;
        else if (ExpectedReason == 0x400)
            ExpectedReason = 0x30400;
        else
            ExpectedReason |= 0x50000;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        {
            /* W10 does not use the flags anymore? */
            if (ExpectedReason != 0x30400 || g_ModuleVersion >= WINVER_WIN10)
                expect_layeronly(AppCompatData, "256Color", expect_flags);
            else
                ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
        }
    }


    /* INVALID_HANDLE_VALUE file handle results in failure (according to flags!), but still results in AppCompatData */
    RESET_CHECKRUNAPP_VARS();

    ret = call_ApphelpCheckRunApp(INVALID_HANDLE_VALUE, ApplicationName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, 0);
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        expect_layeronly(AppCompatData, "256Color", expect_flags);

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        if (!(kTestBits & (1<<n)))
            continue;
        RESET_CHECKRUNAPP_VARS();
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(INVALID_HANDLE_VALUE, NULL, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 0x100)
            ExpectedReason = 0;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        {
            if (ExpectedReason != 0x400 && g_ModuleVersion < WINVER_WIN7)
                expect_layeronly(AppCompatData, "256Color", expect_flags);
            else
                ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
        }
    }

    /* NULL filename crashes, showing this in the log before going down:
[Err ][SdbpGetLongFileName ] Failed to get NT path name for ""
[Err ][SdbpCreateSearchDBContext] Unable to parse executable path for "".
[Err ][SdbGetMatchingExe   ] Failed to create search DB context.
*/
    RESET_CHECKRUNAPP_VARS();

    ret = call_ApphelpCheckRunApp(FileHandle, EmptyName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x30000, 0));
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");

    /* 0 ExeType = don't care? */
    RESET_CHECKRUNAPP_VARS();
    ExeType = 0;

    ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x50000, 0));
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        expect_layeronly(AppCompatData, "256Color", expect_flags);

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        if (!(kTestBits & (1<<n)))
            continue;
        RESET_CHECKRUNAPP_VARS();
        ExeType = 0;
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 2)
            ExpectedReason = 2;
        else if (ExpectedReason == 0x100)
            ExpectedReason = 0x50000;
        else if (ExpectedReason == 0x400)
            ExpectedReason = 0x30400;
        else
            ExpectedReason |= 0x50000;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        {
            if (ExpectedReason != 0x30400 || g_ModuleVersion >= WINVER_WIN10)
                expect_layeronly(AppCompatData, "256Color", expect_flags);
            else
                ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
        }
    }


    RESET_CHECKRUNAPP_VARS();
    ExeType = IMAGE_FILE_MACHINE_POWERPCFP;

    ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
        &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

    CHECK_BASICS();
    ok_hex(Reason, CHECKREASON(0x50000, 0));
    if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        expect_layeronly(AppCompatData, "256Color", expect_flags);

    for (n = 0; n < 32; ++n)
    {
        ULONG ExpectedReason;
        if (!(kTestBits & (1<<n)))
            continue;
        RESET_CHECKRUNAPP_VARS();
        ExeType = IMAGE_FILE_MACHINE_POWERPCFP;
        ExpectedReason = Reason = (1 << n);
        ret = call_ApphelpCheckRunApp(FileHandle, ApplicationName, Environment, ExeType, &Reason,
            &AppCompatData, &AppCompatDataSize, &SxsData, &SxsDataSize, &FusionFlags);

        CHECK_BASICS();
        if (ExpectedReason == 2)
            ExpectedReason = 2;
        else if (ExpectedReason == 0x100)
            ExpectedReason = 0x50000;
        else if (ExpectedReason == 0x400)
            ExpectedReason = 0x30400;
        else
            ExpectedReason |= 0x50000;
        ok_hex(Reason, CHECKREASON(ExpectedReason, (1 << n)));
        if (AppCompatData && AppCompatDataSize == sizeof(SDBQUERYRESULT_VISTA))
        {
            if (ExpectedReason != 0x30400 || g_ModuleVersion >= WINVER_WIN10)
                expect_layeronly(AppCompatData, "256Color", expect_flags);
            else
                ok(!memcmp(AppCompatData, &empty_result, sizeof(empty_result)), "Expected result to be empty\n");
        }
    }


    if (AppCompatData && AppCompatData != &Query)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AppCompatData);

    FreeEnvironmentStringsW(Environment);
    DestroyEnvironmentBlock(DuplicatedEnv);
    NtClose(FileHandle);
}


START_TEST(env)
{
    WCHAR szApphelp[MAX_PATH];
    ShimData_QueryOffset QueryOffset;
    DWORD ShimDataType;
    NTSTATUS ExceptionStatus = STATUS_SUCCESS;

    //SetEnvironmentVariable("SHIM_DEBUG_LEVEL", "127");
    //SetEnvironmentVariable("SHIMENG_DEBUG_LEVEL", "127");

    silence_debug_output();

    hdll = LoadLibraryA("apphelp.dll");



    g_WinVersion = get_host_winver();
    g_ModuleVersion = get_module_version(hdll);
    trace("Detected host: 0x%x, module: 0x%x\n", g_WinVersion, g_ModuleVersion);

    GetModuleFileNameW(hdll, szApphelp, _countof(szApphelp));


    pSdbGetMatchingExe = (void*)GetProcAddress(hdll, "SdbGetMatchingExe");
    pSdbInitDatabase = (void*)GetProcAddress(hdll, "SdbInitDatabase");
    pSdbReleaseDatabase = (void*)GetProcAddress(hdll, "SdbReleaseDatabase");
    pSdbTagRefToTagID = (void*)GetProcAddress(hdll, "SdbTagRefToTagID");
    pSdbGetTagFromTagID = (void*)GetProcAddress(hdll, "SdbGetTagFromTagID");
    pSdbGetLayerTagRef = (void*)GetProcAddress(hdll, "SdbGetLayerTagRef");

    switch (g_ModuleVersion)
    {
    case WINVER_WIN7:
        pApphelpCheckRunAppEx_w7 = (void*)GetProcAddress(hdll, "ApphelpCheckRunAppEx");
        break;
    case WINVER_WIN10:
        pApphelpCheckRunAppEx_w10 = (void*)GetProcAddress(hdll, "ApphelpCheckRunAppEx");
        break;
    default:
        skip("Unknown apphelp.dll version %x, cannot determine which ApphelpCheckRunApp(Ex) function to use\n", g_ModuleVersion);
        break;
    }

    pSdbPackAppCompatData = (void*)GetProcAddress(hdll, "SdbPackAppCompatData");
    pSdbUnpackAppCompatData = (void*)GetProcAddress(hdll, "SdbUnpackAppCompatData");
    pSdbGetAppCompatDataSize = (void*)GetProcAddress(hdll, "SdbGetAppCompatDataSize");


    memset(&QueryOffset, 0, sizeof(QueryOffset));
    QueryOffset.dwMagic_2k3 = QueryOffset.dwMagic_7_10 = QueryOffset.dwMagic_10_v2 = SHIMDATA_MAGIC;
    QueryOffset.dwSize_2k3 = 1;
    QueryOffset.dwSize_7_10 = 2;
    QueryOffset.dwSize_10_v2 = 3;

    g_ShimDataSize = g_WinVersion < WINVER_WIN10 ? 4096 : 8192;
    _SEH2_TRY
    {
        ShimDataType = pSdbGetAppCompatDataSize(&QueryOffset);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExceptionStatus = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(ExceptionStatus == STATUS_SUCCESS, "Exception 0x%08x, expected 0x%08x\n", ExceptionStatus, STATUS_SUCCESS);
    if (ExceptionStatus != STATUS_SUCCESS)
    {
        skip("SdbGetAppCompatDataSize not functional\n");
        return;
    }

    /* New version of Win10.. */
    if (g_WinVersion == WINVER_WIN10 && ShimDataType == 3)
        g_ShimDataSize = 4096;

    if (g_WinVersion == g_ModuleVersion)
    {
        Test_layers(szApphelp);
        Test_repeatlayer(szApphelp);
    }
    else
    {
        skip("Tests requiring process launch, reported OS version (0x%x) does not match apphelp version (0x%x)\n", g_WinVersion, g_ModuleVersion);
    }

    {
        Test_GetMatchingExe();
    }

    Test_ApphelpCheckRunApp(szApphelp);
    if (g_LayerDB)
        pSdbReleaseDatabase(g_LayerDB);
}

