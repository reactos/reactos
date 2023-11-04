/*
 * IGameStatisticsMgr tests
 *
 * Copyright (C) 2010 Mariusz Pluciński
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

#define COBJMACROS

#include "shlwapi.h"
#include "oleauto.h"
#include "shlobj.h"

#include "gameux.h"
#include "wine/test.h"

/*******************************************************************************
 * utilities
 */
static WCHAR sExeName[MAX_PATH] = {0};
static GUID gameInstanceId;

/*******************************************************************************
 * Registers test suite executable as game in Games Explorer. Required to test
 * game statistics.
 */
static void test_register_game(IGameExplorer **explorer)
{
    HRESULT hr;
    WCHAR pathW[MAX_PATH];
    BSTR bstrExeName, bstrExePath;

    /* prepare path to binary */
    GetModuleFileNameW(NULL, sExeName, ARRAY_SIZE(sExeName));

    lstrcpyW(pathW, sExeName);
    PathRemoveFileSpecW(pathW);

    hr = CoCreateInstance(&CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (void**)explorer);
    ok(hr == S_OK, "got 0x%08Ix\n", hr);

    gameInstanceId = GUID_NULL;
    bstrExeName = SysAllocString(sExeName);
    bstrExePath = SysAllocString(pathW);
    hr = IGameExplorer_AddGame(*explorer, bstrExeName, bstrExePath, GIS_CURRENT_USER, &gameInstanceId);
    ok(hr == S_OK, "got 0x%08Ix\n", hr);

    SysFreeString(bstrExeName);
    SysFreeString(bstrExePath);
}

/*******************************************************************************
 * Unregisters test suite from Games Explorer.
 */
static void test_unregister_game(IGameExplorer *ge)
{
    HRESULT hr;

    if (!ge) return;

    hr = IGameExplorer_RemoveGame(ge, gameInstanceId);
    ok(hr == S_OK, "got 0x%08Ix\n", hr);
    IGameExplorer_Release(ge);
}

/*******************************************************************************
 * _buildStatisticsFilePath
 * Creates path to file containing statistics of game with given id.
 *
 * Parameters:
 *  guidApplicationId                       [I]     application id of game
 *  lpStatisticsFile                        [O]     pointer where address of
 *                                                  string with path will be
 *                                                  stored. Path must be deallocated
 *                                                  using CoTaskMemFree(...)
 */
static HRESULT _buildStatisticsFilePath(LPCGUID guidApplicationId, LPWSTR *lpStatisticsFile)
{
    HRESULT hr;
    WCHAR sGuid[49], sPath[MAX_PATH];

    hr = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, sPath);

    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(guidApplicationId, sGuid, ARRAY_SIZE(sGuid)) != 0 ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
    {
        lstrcatW(sPath, L"\\Microsoft\\Windows\\GameExplorer\\GameStatistics\\");
        lstrcatW(sPath, sGuid);
        lstrcatW(sPath, L"\\");
        lstrcatW(sPath, sGuid);
        lstrcatW(sPath, L".gamestats");

        *lpStatisticsFile = CoTaskMemAlloc((lstrlenW(sPath)+1)*sizeof(WCHAR));
        if(!*lpStatisticsFile) hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        lstrcpyW(*lpStatisticsFile, sPath);

    return hr;
}
/*******************************************************************************
 * _isFileExist
 * Checks if given file exists
 *
 * Parameters:
 *  lpFile                          [I]     path to file
 *
 * Result:
 *  TRUE        file exists
 *  FALSE       file does not exist
 */
static BOOL _isFileExists(LPCWSTR lpFile)
{
    HANDLE hFile = CreateFileW(lpFile, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE) return FALSE;
    CloseHandle(hFile);
    return TRUE;
}
/*******************************************************************************
 * test routines
 */
static void test_gamestatisticsmgr( void )
{
    static const GUID guidApplicationId = { 0x17A6558E, 0x60BE, 0x4078, { 0xB6, 0x6F, 0x9C, 0x3A, 0xDA, 0x2A, 0x32, 0xE6 } };

    HRESULT hr;
    GAMESTATS_OPEN_RESULT openResult;
    LPWSTR lpStatisticsFile = NULL;
    LPWSTR lpName = NULL, lpValue = NULL, sTooLongString = NULL;
    UINT uMaxCategoryLength = 0, uMaxNameLength = 0, uMaxValueLength = 0;
    WORD wMaxStatsPerCategory = 0, wMaxCategories = 0;

    IGameStatisticsMgr* gsm = NULL;
    IGameStatistics* gs;

    hr = CoCreateInstance( &CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (LPVOID*)&gsm);
    ok(hr == S_OK, "IGameStatisticsMgr creating failed (result false)\n");

    /* test trying to create interface IGameStatistics using GetGameStatistics method */

    /* this should fail, because statistics don't exist yet */
    gs = (void *)0xdeadbeef;
    hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENONLY, &openResult, &gs);
    if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        /* With win10 1803 game explorer functionality was removed and gameux became a stub */
        win_skip("gameux is partially stubbed, skipping tests\n");
        return;
    }
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "GetGameStatistics returned unexpected value: 0x%08Ix\n", hr);
    ok(gs == NULL, "Expected output pointer to be NULL, got %s\n",
       (gs == (void *)0xdeadbeef ? "deadbeef" : "neither NULL nor deadbeef"));

    /* now, allow them to be created */
    hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENORCREATE, &openResult, &gs);
    ok(SUCCEEDED(hr), "GetGameStatistics returned error: 0x%Ix\n", hr);
    ok(gs!=NULL, "GetGameStatistics did not return valid interface pointer\n");
    if(gs)
    {
        /* test of limit values returned from interface */
        hr = IGameStatistics_GetMaxCategoryLength(gs, &uMaxCategoryLength);
        ok(hr==S_OK, "getting maximum length of category failed\n");
        ok(uMaxCategoryLength==60, "getting maximum length of category returned invalid value: %d\n", uMaxCategoryLength);

        hr = IGameStatistics_GetMaxNameLength(gs, &uMaxNameLength);
        ok(hr==S_OK, "getting maximum name length failed\n");
        ok(uMaxNameLength==30, "getting maximum name length returned invalid value: %d\n", uMaxNameLength);

        hr = IGameStatistics_GetMaxValueLength(gs, &uMaxValueLength);
        ok(hr==S_OK, "getting maximum value length failed\n");
        ok(uMaxValueLength==30, "getting maximum value length returned invalid value: %d\n", uMaxValueLength);

        hr = IGameStatistics_GetMaxCategories(gs, &wMaxCategories);
        ok(hr==S_OK, "getting maximum number of categories failed\n");
        ok(wMaxCategories==10, "getting maximum number of categories returned invalid value: %d\n", wMaxCategories);

        hr = IGameStatistics_GetMaxStatsPerCategory(gs, &wMaxStatsPerCategory);
        ok(hr==S_OK, "getting maximum number of statistics per category failed\n");
        ok(wMaxStatsPerCategory==10, "getting maximum number of statistics per category returned invalid value: %d\n", wMaxStatsPerCategory);

        /* create name of statistics file */
        hr = _buildStatisticsFilePath(&guidApplicationId, &lpStatisticsFile);
        ok(SUCCEEDED(hr), "cannot build path to game statistics (error 0x%Ix)\n", hr);
        trace("statistics file path: %s\n", wine_dbgstr_w(lpStatisticsFile));
        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s already exists\n", wine_dbgstr_w(lpStatisticsFile));

        /* write sample statistics */
        hr = IGameStatistics_SetCategoryTitle(gs, wMaxCategories, NULL);
        ok(hr==E_INVALIDARG, "setting category title invalid value: 0x%Ix\n", hr);

        hr = IGameStatistics_SetCategoryTitle(gs, wMaxCategories, L"Category0");
        ok(hr==E_INVALIDARG, "setting category title invalid value: 0x%Ix\n", hr);

        /* check what happen if string is too long */
        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxCategoryLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxCategoryLength+1));
        sTooLongString[uMaxCategoryLength+1]=0;

        /* when string is too long, Windows returns S_FALSE, but saves string (stripped to expected number of characters) */
        hr = IGameStatistics_SetCategoryTitle(gs, 0, sTooLongString);
        ok(hr==S_FALSE, "setting category title invalid result: 0x%Ix\n", hr);
        CoTaskMemFree(sTooLongString);

        ok(IGameStatistics_SetCategoryTitle(gs, 0, L"Category0")==S_OK, "setting category title failed: Category0\n");
        ok(IGameStatistics_SetCategoryTitle(gs, 1, L"Category1")==S_OK, "setting category title failed: Category1\n");
        ok(IGameStatistics_SetCategoryTitle(gs, 2, L"Category2")==S_OK, "setting category title failed: Category2\n");

        /* check what happen if any string is NULL */
        hr = IGameStatistics_SetStatistic(gs, 0, 0, NULL, L"Value00");
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%Ix)\n", hr);

        hr = IGameStatistics_SetStatistic(gs, 0, 0, L"Statistic00", NULL);
        ok(hr == S_OK, "setting statistic returned unexpected value: 0x%Ix)\n", hr);

        /* check what happen if any string is too long */
        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxNameLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxNameLength+1));
        sTooLongString[uMaxNameLength+1]=0;
        hr = IGameStatistics_SetStatistic(gs, 0, 0, sTooLongString, L"Value00");
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%Ix)\n", hr);
        CoTaskMemFree(sTooLongString);

        sTooLongString = CoTaskMemAlloc(sizeof(WCHAR)*(uMaxValueLength+2));
        memset(sTooLongString, 'a', sizeof(WCHAR)*(uMaxValueLength+1));
        sTooLongString[uMaxValueLength+1]=0;
        hr = IGameStatistics_SetStatistic(gs, 0, 0, L"Statistic00", sTooLongString);
        ok(hr == S_FALSE, "setting statistic returned unexpected value: 0x%Ix)\n", hr);
        CoTaskMemFree(sTooLongString);

        /* check what happen on too big index of category or statistic */
        hr = IGameStatistics_SetStatistic(gs, wMaxCategories, 0, L"Statistic00", L"Value00");
        ok(hr == E_INVALIDARG, "setting statistic returned unexpected value: 0x%Ix)\n", hr);

        hr = IGameStatistics_SetStatistic(gs, 0, wMaxStatsPerCategory, L"Statistic00", L"Value00");
        ok(hr == E_INVALIDARG, "setting statistic returned unexpected value: 0x%Ix)\n", hr);

        ok(IGameStatistics_SetStatistic(gs, 0, 0, L"Statistic00", L"Value00")==S_OK,
                "setting statistic failed: name=Statistic00, value=Value00\n");
        ok(IGameStatistics_SetStatistic(gs, 0, 1, L"Statistic01", L"Value01")==S_OK,
                "setting statistic failed: name=Statistic01, value=Value01\n");
        ok(IGameStatistics_SetStatistic(gs, 1, 0, L"Statistic10", L"Value10")==S_OK,
                "setting statistic failed: name=Statistic10, value=Value10\n");
        ok(IGameStatistics_SetStatistic(gs, 1, 1, L"Statistic11", L"Value11")==S_OK,
                "setting statistic failed: name=Statistic11, value=Value11\n");
        ok(IGameStatistics_SetStatistic(gs, 2, 0, L"Statistic20", L"Value20")==S_OK,
                "setting statistic failed: name=Statistic20, value=Value20\n");
        ok(IGameStatistics_SetStatistic(gs, 2, 1, L"Statistic21", L"Value21")==S_OK,
                "setting statistic failed: name=Statistic21, value=Value21\n");

        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s already exists\n", wine_dbgstr_w(lpStatisticsFile));

        ok(IGameStatistics_Save(gs, FALSE)==S_OK, "statistic saving failed\n");

        ok(_isFileExists(lpStatisticsFile) == TRUE, "statistics file %s does not exists\n", wine_dbgstr_w(lpStatisticsFile));

        /* this value should not be stored in storage, we need it only to test is it not saved */
        ok(IGameStatistics_SetCategoryTitle(gs, 0, L"Category0a")==S_OK, "setting category title failed: Category0a\n");

        hr = IGameStatistics_Release(gs);
        ok(SUCCEEDED(hr), "releasing IGameStatistics returned error: 0x%08Ix\n", hr);

        /* try to read written statistics */
        hr = IGameStatisticsMgr_GetGameStatistics(gsm, sExeName, GAMESTATS_OPEN_OPENORCREATE, &openResult, &gs);
        ok(SUCCEEDED(hr), "GetGameStatistics returned error: 0x%08Ix\n", hr);
        ok(openResult == GAMESTATS_OPEN_OPENED, "GetGameStatistics returned invalid open result: 0x%x\n", openResult);
        ok(gs!=NULL, "GetGameStatistics did not return valid interface pointer\n");

        /* verify values with these which we stored before*/
        hr = IGameStatistics_GetCategoryTitle(gs, 0, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, L"Category0")==0, "getting category title returned invalid string %s\n",
                wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetCategoryTitle(gs, 1, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, L"Category1")==0, "getting category title returned invalid string %s\n",
                wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetCategoryTitle(gs, 2, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lstrcmpW(lpName, L"Category2")==0, "getting category title returned invalid string %s\n",
                wine_dbgstr_w(lpName));
        CoTaskMemFree(lpName);

        /* check result if category doesn't exists */
        hr = IGameStatistics_GetCategoryTitle(gs, 3, &lpName);
        ok(hr == S_OK, "getting category title failed\n");
        ok(lpName == NULL, "getting category title failed\n");
        CoTaskMemFree(lpName);

        hr = IGameStatistics_GetStatistic(gs, 0, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic00")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value00")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 0, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic01")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value01")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 1, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic10")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value10")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 1, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic11")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value11")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 2, 0, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic20")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value20")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_GetStatistic(gs, 2, 1, &lpName, &lpValue);
        ok(hr == S_OK, "getting statistic failed\n");
        ok(lstrcmpW(lpName, L"Statistic21")==0, "getting statistic returned invalid name\n");
        ok(lstrcmpW(lpValue, L"Value21")==0, "getting statistic returned invalid value\n");
        CoTaskMemFree(lpName);
        CoTaskMemFree(lpValue);

        hr = IGameStatistics_Release(gs);
        ok(SUCCEEDED(hr), "releasing IGameStatistics returned error: 0x%Ix\n", hr);

        /* test of removing game statistics from underlying storage */
        ok(_isFileExists(lpStatisticsFile) == TRUE, "statistics file %s does not exists\n", wine_dbgstr_w(lpStatisticsFile));
        hr = IGameStatisticsMgr_RemoveGameStatistics(gsm, sExeName);
        ok(SUCCEEDED(hr), "cannot remove game statistics, error: 0x%Ix\n", hr);
        ok(_isFileExists(lpStatisticsFile) == FALSE, "statistics file %s still exists\n", wine_dbgstr_w(lpStatisticsFile));
    }

    hr = IGameStatisticsMgr_Release(gsm);
    ok(SUCCEEDED(hr), "releasing IGameStatisticsMgr returned error: 0x%Ix\n", hr);

    CoTaskMemFree(lpStatisticsFile);
}

START_TEST(gamestatistics)
{
    HRESULT hr;
    IGameStatisticsMgr* gsm;
    IGameExplorer *ge;

    hr = CoInitialize( NULL );
    ok(hr == S_OK, "failed to init COM\n");

    /* interface available up from Win7 */
    hr = CoCreateInstance(&CLSID_GameStatistics, NULL, CLSCTX_INPROC_SERVER, &IID_IGameStatisticsMgr, (void**)&gsm);
    if (FAILED(hr))
    {
        win_skip("IGameStatisticsMgr is not supported.\n");
        CoUninitialize();
        return;
    }
    IGameStatisticsMgr_Release(gsm);

    test_register_game(&ge);
    test_gamestatisticsmgr();
    test_unregister_game(ge);

    CoUninitialize();
}
