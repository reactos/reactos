/*
 * Unit test of the Program Manager DDE Interfaces
 *
 * Copyright 2009 Mikey Alexander
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

/* DDE Program Manager Tests
 * - Covers basic CreateGroup, ShowGroup, DeleteGroup, AddItem, and DeleteItem
 *   functionality
 * - Todo: Handle CommonGroupFlag
 *         Better AddItem Tests (Lots of parameters to test)
 *         Tests for Invalid Characters in Names / Invalid Parameters
 */

#include <stdio.h>
#include <wine/test.h>
#include <winbase.h>
#include "dde.h"
#include "ddeml.h"
#include "winuser.h"
#include "shlobj.h"

/* Timeout on DdeClientTransaction Call */
#define MS_TIMEOUT_VAL 1000
/* # of times to poll for window creation */
#define PDDE_POLL_NUM 150
/* time to sleep between polls */
#define PDDE_POLL_TIME 300

/* Call Info */
#define DDE_TEST_MISC            0x00010000
#define DDE_TEST_CREATEGROUP     0x00020000
#define DDE_TEST_DELETEGROUP     0x00030000
#define DDE_TEST_SHOWGROUP       0x00040000
#define DDE_TEST_ADDITEM         0x00050000
#define DDE_TEST_DELETEITEM      0x00060000
#define DDE_TEST_COMPOUND        0x00070000
#define DDE_TEST_CALLMASK        0x00ff0000

#define DDE_TEST_NUMMASK           0x0000ffff

static HRESULT (WINAPI *pSHGetLocalizedName)(LPCWSTR, LPWSTR, UINT, int *);
static BOOL (WINAPI *pSHGetSpecialFolderPathA)(HWND, LPSTR, int, BOOL);
static BOOL (WINAPI *pReadCabinetState)(CABINETSTATE *, int);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("shell32.dll");
    pSHGetLocalizedName = (void*)GetProcAddress(hmod, "SHGetLocalizedName");
    pSHGetSpecialFolderPathA = (void*)GetProcAddress(hmod, "SHGetSpecialFolderPathA");
    pReadCabinetState = (void*)GetProcAddress(hmod, "ReadCabinetState");
    if (!pReadCabinetState)
        pReadCabinetState = (void*)GetProcAddress(hmod, (LPSTR)651);
}

static BOOL use_common(void)
{
    HMODULE hmod;
    static BOOL (WINAPI *pIsNTAdmin)(DWORD, LPDWORD);

    /* IsNTAdmin() is available on all platforms. */
    hmod = LoadLibraryA("advpack.dll");
    pIsNTAdmin = (void*)GetProcAddress(hmod, "IsNTAdmin");

    if (!pIsNTAdmin(0, NULL))
    {
        /* We are definitely not an administrator */
        FreeLibrary(hmod);
        return FALSE;
    }
    FreeLibrary(hmod);

    /* If we end up here we are on NT4+ as Win9x and WinMe don't have the
     * notion of administrators (as we need it).
     */

    /* As of Vista  we should always use the users directory. Tests with the
     * real Administrator account on Windows 7 proved this.
     *
     * FIXME: We need a better way of identifying Vista+ as currently this check
     * also covers Wine and we don't know yet which behavior we want to follow.
     */
    if (pSHGetLocalizedName)
        return FALSE;

    return TRUE;
}

static BOOL full_title(void)
{
    CABINETSTATE cs;

    memset(&cs, 0, sizeof(cs));
    if (pReadCabinetState)
    {
        pReadCabinetState(&cs, sizeof(cs));
    }
    else
    {
        HKEY key;
        DWORD size;

        win_skip("ReadCabinetState is not available, reading registry directly\n");
        RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState", &key);
        size = sizeof(cs);
        RegQueryValueExA(key, "Settings", NULL, NULL, (LPBYTE)&cs, &size);
        RegCloseKey(key);
    }

    return (cs.fFullPathTitle == -1);
}

static char ProgramsDir[MAX_PATH];

static char Group1Title[MAX_PATH]  = "Group1";
static char Group2Title[MAX_PATH]  = "Group2";
static char Group3Title[MAX_PATH]  = "Group3";
static char StartupTitle[MAX_PATH] = "Startup";

static void init_strings(void)
{
    char startup[MAX_PATH];
    char commonprograms[MAX_PATH];
    char programs[MAX_PATH];

    if (pSHGetSpecialFolderPathA)
    {
        pSHGetSpecialFolderPathA(NULL, programs, CSIDL_PROGRAMS, FALSE);
        pSHGetSpecialFolderPathA(NULL, commonprograms, CSIDL_COMMON_PROGRAMS, FALSE);
        pSHGetSpecialFolderPathA(NULL, startup, CSIDL_STARTUP, FALSE);
    }
    else
    {
        HKEY key;
        DWORD size;

        /* Older Win9x and NT4 */

        RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &key);
        size = sizeof(programs);
        RegQueryValueExA(key, "Programs", NULL, NULL, (LPBYTE)&programs, &size);
        size = sizeof(startup);
        RegQueryValueExA(key, "Startup", NULL, NULL, (LPBYTE)&startup, &size);
        RegCloseKey(key);

        RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", &key);
        size = sizeof(commonprograms);
        RegQueryValueExA(key, "Common Programs", NULL, NULL, (LPBYTE)&commonprograms, &size);
        RegCloseKey(key);
    }

    /* ProgramsDir on Vista+ is always the users one (CSIDL_PROGRAMS). Before Vista
     * it depends on whether the user is an administrator (CSIDL_COMMON_PROGRAMS) or
     * not (CSIDL_PROGRAMS).
     */
    if (use_common())
        lstrcpyA(ProgramsDir, commonprograms);
    else
        lstrcpyA(ProgramsDir, programs);

    if (full_title())
    {
        lstrcpyA(Group1Title, ProgramsDir);
        lstrcatA(Group1Title, "\\Group1");
        lstrcpyA(Group2Title, ProgramsDir);
        lstrcatA(Group2Title, "\\Group2");
        lstrcpyA(Group3Title, ProgramsDir);
        lstrcatA(Group3Title, "\\Group3");

        lstrcpyA(StartupTitle, startup);
    }
    else
    {
        /* Vista has the nice habit of displaying the full path in English
         * and the short one localized. CSIDL_STARTUP on Vista gives us the
         * English version so we have to 'translate' this one.
         *
         * MSDN claims it should be used for files not folders but this one
         * suits our purposes just fine.
         */
        if (pSHGetLocalizedName)
        {
            WCHAR startupW[MAX_PATH];
            WCHAR module[MAX_PATH];
            WCHAR module_expanded[MAX_PATH];
            WCHAR localized[MAX_PATH];
            HRESULT hr;
            int id;

            MultiByteToWideChar(CP_ACP, 0, startup, -1, startupW, sizeof(startupW)/sizeof(WCHAR));
            hr = pSHGetLocalizedName(startupW, module, MAX_PATH, &id);
            todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
            /* check to be removed when SHGetLocalizedName is implemented */
            if (hr == S_OK)
            {
                ExpandEnvironmentStringsW(module, module_expanded, MAX_PATH);
                LoadStringW(GetModuleHandleW(module_expanded), id, localized, MAX_PATH);

                WideCharToMultiByte(CP_ACP, 0, localized, -1, StartupTitle, sizeof(StartupTitle), NULL, NULL);
            }
            else
                lstrcpyA(StartupTitle, (strrchr(startup, '\\') + 1));
        }
        else
        {
            lstrcpyA(StartupTitle, (strrchr(startup, '\\') + 1));
        }
    }
}

static HDDEDATA CALLBACK DdeCallback(UINT type, UINT format, HCONV hConv, HSZ hsz1, HSZ hsz2,
                                     HDDEDATA hDDEData, ULONG_PTR data1, ULONG_PTR data2)
{
    trace("Callback: type=%i, format=%i\n", type, format);
    return NULL;
}

/*
 * Encoded String for Error Messages so that inner failures can determine
 * what test is failing.  Format is: [Code:TestNum]
 */
static const char * GetStringFromTestParams(int testParams)
{
    int testNum;
    static char testParamString[64];
    const char *callId;

    testNum = testParams & DDE_TEST_NUMMASK;
    switch (testParams & DDE_TEST_CALLMASK)
    {
    default:
    case DDE_TEST_MISC:
        callId = "MISC";
        break;
    case DDE_TEST_CREATEGROUP:
        callId = "C_G";
        break;
    case DDE_TEST_DELETEGROUP:
        callId = "D_G";
        break;
    case DDE_TEST_SHOWGROUP:
        callId = "S_G";
        break;
    case DDE_TEST_ADDITEM:
        callId = "A_I";
        break;
    case DDE_TEST_DELETEITEM:
        callId = "D_I";
        break;
    case DDE_TEST_COMPOUND:
        callId = "CPD";
        break;
    }

    sprintf(testParamString, "  [%s:%i]", callId, testNum);
    return testParamString;
}

/* Transfer DMLERR's into text readable strings for Error Messages */
#define DMLERR_TO_STR(x) case x: return#x;
static const char * GetStringFromError(UINT err)
{
    switch (err)
    {
    DMLERR_TO_STR(DMLERR_NO_ERROR);
    DMLERR_TO_STR(DMLERR_ADVACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_BUSY);
    DMLERR_TO_STR(DMLERR_DATAACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_DLL_NOT_INITIALIZED);
    DMLERR_TO_STR(DMLERR_DLL_USAGE);
    DMLERR_TO_STR(DMLERR_EXECACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_INVALIDPARAMETER);
    DMLERR_TO_STR(DMLERR_LOW_MEMORY);
    DMLERR_TO_STR(DMLERR_MEMORY_ERROR);
    DMLERR_TO_STR(DMLERR_NOTPROCESSED);
    DMLERR_TO_STR(DMLERR_NO_CONV_ESTABLISHED);
    DMLERR_TO_STR(DMLERR_POKEACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_POSTMSG_FAILED);
    DMLERR_TO_STR(DMLERR_REENTRANCY);
    DMLERR_TO_STR(DMLERR_SERVER_DIED);
    DMLERR_TO_STR(DMLERR_SYS_ERROR);
    DMLERR_TO_STR(DMLERR_UNADVACKTIMEOUT);
    DMLERR_TO_STR(DMLERR_UNFOUND_QUEUE_ID);
    default:
        return "Unknown DML Error";
    }
}

/* Helper Function to Transfer DdeGetLastError into a String */
static const char * GetDdeLastErrorStr(DWORD instance)
{
    UINT err = DdeGetLastError(instance);

    return GetStringFromError(err);
}

/* Execute a Dde Command and return the error & result */
/* Note: Progman DDE always returns a pointer to 0x00000001 on a successful result */
static void DdeExecuteCommand(DWORD instance, HCONV hConv, const char *strCmd, HDDEDATA *hData, UINT *err, int testParams)
{
    HDDEDATA command;

    command = DdeCreateDataHandle(instance, (LPBYTE) strCmd, strlen(strCmd)+1, 0, 0L, 0, 0);
    ok (command != NULL, "DdeCreateDataHandle Error %s.%s\n",
        GetDdeLastErrorStr(instance), GetStringFromTestParams(testParams));
    *hData = DdeClientTransaction((void *) command,
                                  -1,
                                  hConv,
                                  0,
                                  0,
                                  XTYP_EXECUTE,
                                  MS_TIMEOUT_VAL,
                                  NULL);

    /* hData is technically a pointer, but for Program Manager,
     * it is NULL (error) or 1 (success)
     * TODO: Check other versions of Windows to verify 1 is returned.
     * While it is unlikely that anyone is actually testing that the result is 1
     * if all versions of windows return 1, Wine should also.
     */
    if (*hData == NULL)
    {
        *err = DdeGetLastError(instance);
    }
    else
    {
        *err = DMLERR_NO_ERROR;
        todo_wine
        {
            ok(*hData == (HDDEDATA) 1, "Expected HDDEDATA Handle == 1, actually %p.%s\n",
               *hData, GetStringFromTestParams(testParams));
        }
    }
    DdeFreeDataHandle(command);
}

/*
 * Check if Window is onscreen with the appropriate name.
 *
 * Windows are not created synchronously.  So we do not know
 * when and if the window will be created/shown on screen.
 * This function implements a polling mechanism to determine
 * creation.
 * A more complicated method would be to use SetWindowsHookEx.
 * Since polling worked fine in my testing, no reason to implement
 * the other.  Comments about other methods of determining when
 * window creation happened were not encouraging (not including
 * SetWindowsHookEx).
 */
static void CheckWindowCreated(const char *winName, BOOL closeWindow, int testParams)
{
    HWND window = NULL;
    int i;

    /* Poll for Window Creation */
    for (i = 0; window == NULL && i < PDDE_POLL_NUM; i++)
    {
        Sleep(PDDE_POLL_TIME);
        window = FindWindowA(NULL, winName);
    }
    ok (window != NULL, "Window \"%s\" was not created in %i seconds - assumed failure.%s\n",
        winName, PDDE_POLL_NUM*PDDE_POLL_TIME/1000, GetStringFromTestParams(testParams));

    /* Close Window as desired. */
    if (window != NULL && closeWindow)
    {
        SendMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}

/* Check for Existence (or non-existence) of a file or group
 *   When testing for existence of a group, groupName is not needed
 */
static void CheckFileExistsInProgramGroups(const char *nameToCheck, BOOL shouldExist, BOOL isGroup,
                                           const char *groupName, int testParams)
{
    char path[MAX_PATH];
    DWORD attributes;
    int len;

    lstrcpyA(path, ProgramsDir);

    len = strlen(path) + strlen(nameToCheck)+1;
    if (groupName != NULL)
    {
        len += strlen(groupName)+1;
    }
    ok (len <= MAX_PATH, "Path Too Long.%s\n", GetStringFromTestParams(testParams));
    if (len <= MAX_PATH)
    {
        if (groupName != NULL)
        {
            strcat(path, "\\");
            strcat(path, groupName);
        }
        strcat(path, "\\");
        strcat(path, nameToCheck);
        attributes = GetFileAttributesA(path);
        if (!shouldExist)
        {
            ok (attributes == INVALID_FILE_ATTRIBUTES , "File exists and shouldn't %s.%s\n",
                path, GetStringFromTestParams(testParams));
        } else {
            if (attributes == INVALID_FILE_ATTRIBUTES)
            {
                ok (FALSE, "Created File %s doesn't exist.%s\n", path, GetStringFromTestParams(testParams));
            } else if (isGroup) {
                ok (attributes & FILE_ATTRIBUTE_DIRECTORY, "%s is not a folder (attr=%x).%s\n",
                    path, attributes, GetStringFromTestParams(testParams));
            } else {
                ok (attributes & FILE_ATTRIBUTE_ARCHIVE, "Created File %s has wrong attributes (%x).%s\n",
                    path, attributes, GetStringFromTestParams(testParams));
            }
        }
    }
}

/* Create Group Test.
 *   command and expected_result.
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. group was created
 *        2. window is open
 */
static void CreateGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                            const char *groupName, const char *windowTitle, int testParams)
{
    HDDEDATA hData;
    UINT error;

    /* Execute Command & Check Result */
    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "CreateGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    /* No Error */
    if (error == DMLERR_NO_ERROR)
    {

        /* Check if Group Now Exists */
        CheckFileExistsInProgramGroups(groupName, TRUE, TRUE, NULL, testParams);
        /* Check if Window is Open (polling) */
        CheckWindowCreated(windowTitle, TRUE, testParams);
    }
}

/* Show Group Test.
 *   DDE command, expected_result, and the group name to check for existence
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. window is open
 */
static void ShowGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                          const char *groupName, const char *windowTitle, BOOL closeAfterShowing, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
/* todo_wine...  Is expected to fail, wine stubbed functions DO fail */
/* TODO REMOVE THIS CODE!!! */
    if (expected_result == DMLERR_NOTPROCESSED)
    {
        ok (expected_result == error, "ShowGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    } else {
        todo_wine
        {
            ok (expected_result == error, "ShowGroup %s: Expected Error %s, received %s.%s\n",
                groupName, GetStringFromError(expected_result), GetStringFromError(error),
                GetStringFromTestParams(testParams));
        }
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check if Window is Open (polling) */
        CheckWindowCreated(windowTitle, closeAfterShowing, testParams);
    }
}

/* Delete Group Test.
 *   DDE command, expected_result, and the group name to check for existence
 *   if expected_result is DMLERR_NO_ERROR, test
 *        1. group does not exist
 */
static void DeleteGroupTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                            const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "DeleteGroup %s: Expected Error %s, received %s.%s\n",
            groupName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that Group does not exist */
        CheckFileExistsInProgramGroups(groupName, FALSE, TRUE, NULL, testParams);
    }
}

/* Add Item Test
 *   DDE command, expected result, and group and file name where it should exist.
 *   checks to make sure error code matches expected error code
 *   checks to make sure item exists if successful
 */
static void AddItemTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                        const char *fileName, const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "AddItem %s: Expected Error %s, received %s.%s\n",
            fileName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File exists */
        CheckFileExistsInProgramGroups(fileName, TRUE, FALSE, groupName, testParams);
    }
}

/* Delete Item Test.
 *   DDE command, expected result, and group and file name where it should exist.
 *   checks to make sure error code matches expected error code
 *   checks to make sure item does not exist if successful
 */
static void DeleteItemTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                           const char *fileName, const char *groupName, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "DeleteItem %s: Expected Error %s, received %s.%s\n",
            fileName, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File does not exist */
        CheckFileExistsInProgramGroups(fileName, FALSE, FALSE, groupName, testParams);
    }
}

/* Compound Command Test.
 *   not really generic, assumes command of the form:
 *          [CreateGroup ...][AddItem ...][AddItem ...]
 *   All samples I've seen using Compound were of this form (CreateGroup,
 *   AddItems) so this covers minimum expected functionality.
 */
static void CompoundCommandTest(DWORD instance, HCONV hConv, const char *command, UINT expected_result,
                                const char *groupName, const char *windowTitle, const char *fileName1,
                                const char *fileName2, int testParams)
{
    HDDEDATA hData;
    UINT error;

    DdeExecuteCommand(instance, hConv, command, &hData, &error, testParams);
    todo_wine
    {
        ok (expected_result == error, "Compound String %s: Expected Error %s, received %s.%s\n",
            command, GetStringFromError(expected_result), GetStringFromError(error),
            GetStringFromTestParams(testParams));
    }

    if (error == DMLERR_NO_ERROR)
    {
        /* Check that File exists */
        CheckFileExistsInProgramGroups(groupName, TRUE, TRUE, NULL, testParams);
        CheckWindowCreated(windowTitle, FALSE, testParams);
        CheckFileExistsInProgramGroups(fileName1, TRUE, FALSE, groupName, testParams);
        CheckFileExistsInProgramGroups(fileName2, TRUE, FALSE, groupName, testParams);
    }
}

static void CreateAddItemText(char *itemtext, const char *cmdline, const char *name)
{
    lstrcpyA(itemtext, "[AddItem(");
    lstrcatA(itemtext, cmdline);
    lstrcatA(itemtext, ",");
    lstrcatA(itemtext, name);
    lstrcatA(itemtext, ")]");
}

/* 1st set of tests */
static int DdeTestProgman(DWORD instance, HCONV hConv)
{
    HDDEDATA hData;
    UINT error;
    int testnum;
    char temppath[MAX_PATH];
    char f1g1[MAX_PATH], f2g1[MAX_PATH], f3g1[MAX_PATH], f1g3[MAX_PATH], f2g3[MAX_PATH];
    char itemtext[MAX_PATH + 20];
    char comptext[2 * (MAX_PATH + 20) + 21];

    testnum = 1;
    /* Invalid Command */
    DdeExecuteCommand(instance, hConv, "[InvalidCommand()]", &hData, &error, DDE_TEST_MISC|testnum++);
    ok (error == DMLERR_NOTPROCESSED, "InvalidCommand(), expected error %s, received %s.\n",
        GetStringFromError(DMLERR_NOTPROCESSED), GetStringFromError(error));

    /* On Vista+ the files have to exist when adding a link */
    GetTempPathA(MAX_PATH, temppath);
    GetTempFileNameA(temppath, "dde", 0, f1g1);
    GetTempFileNameA(temppath, "dde", 0, f2g1);
    GetTempFileNameA(temppath, "dde", 0, f3g1);
    GetTempFileNameA(temppath, "dde", 0, f1g3);
    GetTempFileNameA(temppath, "dde", 0, f2g3);

    /* CreateGroup Tests (including AddItem, DeleteItem) */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group1)]", DMLERR_NO_ERROR, "Group1", Group1Title, DDE_TEST_CREATEGROUP|testnum++);
    CreateAddItemText(itemtext, f1g1, "f1g1Name");
    AddItemTest(instance, hConv, itemtext, DMLERR_NO_ERROR, "f1g1Name.lnk", "Group1", DDE_TEST_ADDITEM|testnum++);
    CreateAddItemText(itemtext, f2g1, "f2g1Name");
    AddItemTest(instance, hConv, itemtext, DMLERR_NO_ERROR, "f2g1Name.lnk", "Group1", DDE_TEST_ADDITEM|testnum++);
    DeleteItemTest(instance, hConv, "[DeleteItem(f2g1Name)]", DMLERR_NO_ERROR, "f2g1Name.lnk", "Group1", DDE_TEST_DELETEITEM|testnum++);
    CreateAddItemText(itemtext, f3g1, "f3g1Name");
    AddItemTest(instance, hConv, itemtext, DMLERR_NO_ERROR, "f3g1Name.lnk", "Group1", DDE_TEST_ADDITEM|testnum++);
    CreateGroupTest(instance, hConv, "[CreateGroup(Group2)]", DMLERR_NO_ERROR, "Group2", Group2Title, DDE_TEST_CREATEGROUP|testnum++);
    /* Create Group that already exists - same instance */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group1)]", DMLERR_NO_ERROR, "Group1", Group1Title, DDE_TEST_CREATEGROUP|testnum++);

    /* ShowGroup Tests */
    ShowGroupTest(instance, hConv, "[ShowGroup(Group1)]", DMLERR_NOTPROCESSED, "Group1", Group1Title, TRUE, DDE_TEST_SHOWGROUP|testnum++);
    DeleteItemTest(instance, hConv, "[DeleteItem(f3g1Name)]", DMLERR_NO_ERROR, "f3g1Name.lnk", "Group1", DDE_TEST_DELETEITEM|testnum++);
    ShowGroupTest(instance, hConv, "[ShowGroup(Startup,0)]", DMLERR_NO_ERROR, "Startup", StartupTitle, TRUE, DDE_TEST_SHOWGROUP|testnum++);
    ShowGroupTest(instance, hConv, "[ShowGroup(Group1,0)]", DMLERR_NO_ERROR, "Group1", Group1Title, FALSE, DDE_TEST_SHOWGROUP|testnum++);

    /* DeleteGroup Test - Note that Window is Open for this test */
    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group1)]", DMLERR_NO_ERROR, "Group1", DDE_TEST_DELETEGROUP|testnum++);

    /* Compound Execute String Command */
    lstrcpyA(comptext, "[CreateGroup(Group3)]");
    CreateAddItemText(itemtext, f1g3, "f1g3Name");
    lstrcatA(comptext, itemtext);
    CreateAddItemText(itemtext, f2g3, "f2g3Name");
    lstrcatA(comptext, itemtext);
    CompoundCommandTest(instance, hConv, comptext, DMLERR_NO_ERROR, "Group3", Group3Title, "f1g3Name.lnk", "f2g3Name.lnk", DDE_TEST_COMPOUND|testnum++);

    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group3)]", DMLERR_NO_ERROR, "Group3", DDE_TEST_DELETEGROUP|testnum++);

    /* Full Parameters of Add Item */
    /* AddItem(CmdLine[,Name[,IconPath[,IconIndex[,xPos,yPos[,DefDir[,HotKey[,fMinimize[fSeparateSpace]]]]]]]) */

    DeleteFileA(f1g1);
    DeleteFileA(f2g1);
    DeleteFileA(f3g1);
    DeleteFileA(f1g3);
    DeleteFileA(f2g3);

    return testnum;
}

/* 2nd set of tests - 2nd connection */
static void DdeTestProgman2(DWORD instance, HCONV hConv, int testnum)
{
    /* Create Group that already exists on a separate connection */
    CreateGroupTest(instance, hConv, "[CreateGroup(Group2)]", DMLERR_NO_ERROR, "Group2", Group2Title, DDE_TEST_CREATEGROUP|testnum++);
    DeleteGroupTest(instance, hConv, "[DeleteGroup(Group2)]", DMLERR_NO_ERROR, "Group2", DDE_TEST_DELETEGROUP|testnum++);
}

START_TEST(progman_dde)
{
    DWORD instance = 0;
    UINT err;
    HSZ hszProgman;
    HCONV hConv;
    int testnum;

    init_function_pointers();
    init_strings();

    /* Initialize DDE Instance */
    err = DdeInitializeA(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok (err == DMLERR_NO_ERROR, "DdeInitialize Error %s\n", GetStringFromError(err));

    /* Create Connection */
    hszProgman = DdeCreateStringHandleA(instance, "PROGMAN", CP_WINANSI);
    ok (hszProgman != NULL, "DdeCreateStringHandle Error %s\n", GetDdeLastErrorStr(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok (DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle failure\n");
    /* Seeing failures on early versions of Windows Connecting to progman, exit if connection fails */
    if (hConv == NULL)
    {
        ok (DdeUninitialize(instance), "DdeUninitialize failed\n");
        return;
    }

    /* Run Tests */
    testnum = DdeTestProgman(instance, hConv);

    /* Cleanup & Exit */
    ok (DdeDisconnect(hConv), "DdeDisonnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeUninitialize(instance), "DdeUninitialize failed\n");

    /* 2nd Instance (Followup Tests) */
    /* Initialize DDE Instance */
    instance = 0;
    err = DdeInitializeA(&instance, DdeCallback, APPCMD_CLIENTONLY, 0);
    ok (err == DMLERR_NO_ERROR, "DdeInitialize Error %s\n", GetStringFromError(err));

    /* Create Connection */
    hszProgman = DdeCreateStringHandleA(instance, "PROGMAN", CP_WINANSI);
    ok (hszProgman != NULL, "DdeCreateStringHandle Error %s\n", GetDdeLastErrorStr(instance));
    hConv = DdeConnect(instance, hszProgman, hszProgman, NULL);
    ok (hConv != NULL, "DdeConnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeFreeStringHandle(instance, hszProgman), "DdeFreeStringHandle failure\n");

    /* Run Tests */
    DdeTestProgman2(instance, hConv, testnum);

    /* Cleanup & Exit */
    ok (DdeDisconnect(hConv), "DdeDisonnect Error %s\n", GetDdeLastErrorStr(instance));
    ok (DdeUninitialize(instance), "DdeUninitialize failed\n");
}
