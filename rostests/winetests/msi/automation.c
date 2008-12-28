/*
 * Copyright (C) 2007 Mike McCormack for CodeWeavers
 * Copyright (C) 2007 Misha Koshelev
 *
 * A test program for Microsoft Installer OLE automation functionality.
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

#include <stdio.h>

#include <initguid.h>
#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>

#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const char *msifile = "winetest.msi";
static const WCHAR szMsifile[] = {'w','i','n','e','t','e','s','t','.','m','s','i',0};
static const WCHAR szMSITEST[] = { 'M','S','I','T','E','S','T',0 };
static const WCHAR szProductCode[] = { '{','F','1','C','3','A','F','5','0','-','8','B','5','6','-','4','A','6','9','-','A','0','0','C','-','0','0','7','7','3','F','E','4','2','F','3','0','}',0 };
static const WCHAR szUpgradeCode[] = { '{','C','E','0','6','7','E','8','D','-','2','E','1','A','-','4','3','6','7','-','B','7','3','4','-','4','E','B','2','B','D','A','D','6','5','6','5','}',0 };
static const WCHAR szProductInfoException[] = { 'P','r','o','d','u','c','t','I','n','f','o',',','P','r','o','d','u','c','t',',','A','t','t','r','i','b','u','t','e',0 };
static const WCHAR WINE_INSTALLPROPERTY_PACKAGENAMEW[] = {'P','a','c','k','a','g','e','N','a','m','e',0};
static const WCHAR WINE_INSTALLPROPERTY_PRODUCTNAMEW[] = {'P','r','o','d','u','c','t','N','a','m','e',0};
static FILETIME systemtime;
static CHAR CURR_DIR[MAX_PATH];
static EXCEPINFO excepinfo;

/*
 * OLE automation data
 **/
static const WCHAR szProgId[] = { 'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r','.','I','n','s','t','a','l','l','e','r',0 };
static IDispatch *pInstaller;

/* msi database data */

static const CHAR component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                    "s72\tS38\ts72\ti2\tS255\tS72\n"
                                    "Component\tComponent\n"
                                    "Five\t{8CC92E9D-14B2-4CA4-B2AA-B11D02078087}\tNEWDIR\t2\t\tfive.txt\n"
                                    "Four\t{FD37B4EA-7209-45C0-8917-535F35A2F080}\tCABOUTDIR\t2\t\tfour.txt\n"
                                    "One\t{783B242E-E185-4A56-AF86-C09815EC053C}\tMSITESTDIR\t2\t\tone.txt\n"
                                    "Three\t{010B6ADD-B27D-4EDD-9B3D-34C4F7D61684}\tCHANGEDDIR\t2\t\tthree.txt\n"
                                    "Two\t{BF03D1A6-20DA-4A65-82F3-6CAC995915CE}\tFIRSTDIR\t2\t\ttwo.txt\n"
                                    "dangler\t{6091DF25-EF96-45F1-B8E9-A9B1420C7A3C}\tTARGETDIR\t4\t\tregdata\n"
                                    "component\t\tMSITESTDIR\t0\t1\tfile\n";

static const CHAR directory_dat[] = "Directory\tDirectory_Parent\tDefaultDir\n"
                                    "s72\tS72\tl255\n"
                                    "Directory\tDirectory\n"
                                    "CABOUTDIR\tMSITESTDIR\tcabout\n"
                                    "CHANGEDDIR\tMSITESTDIR\tchanged:second\n"
                                    "FIRSTDIR\tMSITESTDIR\tfirst\n"
                                    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
                                    "NEWDIR\tCABOUTDIR\tnew\n"
                                    "ProgramFilesFolder\tTARGETDIR\t.\n"
                                    "TARGETDIR\t\tSourceDir";

static const CHAR feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                  "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                  "Feature\tFeature\n"
                                  "Five\t\tFive\tThe Five Feature\t5\t3\tNEWDIR\t0\n"
                                  "Four\t\tFour\tThe Four Feature\t4\t3\tCABOUTDIR\t0\n"
                                  "One\t\tOne\tThe One Feature\t1\t3\tMSITESTDIR\t0\n"
                                  "Three\tOne\tThree\tThe Three Feature\t3\t3\tCHANGEDDIR\t0\n"
                                  "Two\tOne\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0\n"
                                  "feature\t\t\t\t2\t1\tTARGETDIR\t0\n";

static const CHAR feature_comp_dat[] = "Feature_\tComponent_\n"
                                       "s38\ts72\n"
                                       "FeatureComponents\tFeature_\tComponent_\n"
                                       "Five\tFive\n"
                                       "Four\tFour\n"
                                       "One\tOne\n"
                                       "Three\tThree\n"
                                       "Two\tTwo\n"
                                       "feature\tcomponent\n";

static const CHAR file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                               "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                               "File\tFile\n"
                               "five.txt\tFive\tfive.txt\t1000\t\t\t0\t5\n"
                               "four.txt\tFour\tfour.txt\t1000\t\t\t0\t4\n"
                               "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n"
                               "three.txt\tThree\tthree.txt\t1000\t\t\t0\t3\n"
                               "two.txt\tTwo\ttwo.txt\t1000\t\t\t0\t2\n"
                               "file\tcomponent\tfilename\t100\t\t\t8192\t1\n";

static const CHAR install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallExecuteSequence\tAction\n"
                                           "AllocateRegistrySpace\tNOT Installed\t1550\n"
                                           "CostFinalize\t\t1000\n"
                                           "CostInitialize\t\t800\n"
                                           "FileCost\t\t900\n"
                                           "InstallFiles\t\t4000\n"
                                           "RegisterProduct\t\t6100\n"
                                           "PublishProduct\t\t6400\n"
                                           "InstallFinalize\t\t6600\n"
                                           "InstallInitialize\t\t1500\n"
                                           "InstallValidate\t\t1400\n"
                                           "LaunchConditions\t\t100\n"
                                           "WriteRegistryValues\tSourceDir And SOURCEDIR\t5000";

static const CHAR media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                "i2\ti4\tL64\tS255\tS32\tS72\n"
                                "Media\tDiskId\n"
                                "1\t5\t\t\tDISK1\t\n";

static const CHAR property_dat[] = "Property\tValue\n"
                                   "s72\tl0\n"
                                   "Property\tProperty\n"
                                   "DefaultUIFont\tDlgFont8\n"
                                   "HASUIRUN\t0\n"
                                   "INSTALLLEVEL\t3\n"
                                   "InstallMode\tTypical\n"
                                   "Manufacturer\tWine\n"
                                   "PIDTemplate\t12345<###-%%%%%%%>@@@@@\n"
                                   "ProductCode\t{F1C3AF50-8B56-4A69-A00C-00773FE42F30}\n"
                                   "ProductID\tnone\n"
                                   "ProductLanguage\t1033\n"
                                   "ProductName\tMSITEST\n"
                                   "ProductVersion\t1.1.1\n"
                                   "PROMPTROLLBACKCOST\tP\n"
                                   "Setup\tSetup\n"
                                   "UpgradeCode\t{CE067E8D-2E1A-4367-B734-4EB2BDAD6565}";

static const CHAR registry_dat[] = "Registry\tRoot\tKey\tName\tValue\tComponent_\n"
                                   "s72\ti2\tl255\tL255\tL0\ts72\n"
                                   "Registry\tRegistry\n"
                                   "Apples\t2\tSOFTWARE\\Wine\\msitest\tName\timaname\tOne\n"
                                   "Oranges\t2\tSOFTWARE\\Wine\\msitest\tnumber\t#314\tTwo\n"
                                   "regdata\t2\tSOFTWARE\\Wine\\msitest\tblah\tbad\tdangler\n"
                                   "OrderTest\t2\tSOFTWARE\\Wine\\msitest\tOrderTestName\tOrderTestValue\tcomponent";

typedef struct _msi_table
{
    const CHAR *filename;
    const CHAR *data;
    int size;
} msi_table;

#define ADD_TABLE(x) {#x".idt", x##_dat, sizeof(x##_dat)}

static const msi_table tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property),
    ADD_TABLE(registry)
};

typedef struct _msi_summary_info
{
    UINT property;
    UINT datatype;
    INT iValue;
    FILETIME *pftValue;
    const CHAR *szValue;
} msi_summary_info;

#define ADD_INFO_I2(property, iValue) {property, VT_I2, iValue, NULL, NULL}
#define ADD_INFO_I4(property, iValue) {property, VT_I4, iValue, NULL, NULL}
#define ADD_INFO_LPSTR(property, szValue) {property, VT_LPSTR, 0, NULL, szValue}
#define ADD_INFO_FILETIME(property, pftValue) {property, VT_FILETIME, 0, pftValue, NULL}

static const msi_summary_info summary_info[] =
{
    ADD_INFO_LPSTR(PID_TEMPLATE, ";1033"),
    ADD_INFO_LPSTR(PID_REVNUMBER, "{004757CA-5092-49c2-AD20-28E1CE0DF5F2}"),
    ADD_INFO_I4(PID_PAGECOUNT, 100),
    ADD_INFO_I4(PID_WORDCOUNT, 0),
    ADD_INFO_FILETIME(PID_CREATE_DTM, &systemtime),
    ADD_INFO_FILETIME(PID_LASTPRINTED, &systemtime)
};

/*
 * Database Helpers
 */

static void write_file(const CHAR *filename, const char *data, int data_size)
{
    DWORD size;

    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static void write_msi_summary_info(MSIHANDLE db, const msi_summary_info *info, int num_info)
{
    MSIHANDLE summary;
    UINT r;
    int j;

    r = MsiGetSummaryInformationA(db, NULL, num_info, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* import summary information into the stream */
    for (j = 0; j < num_info; j++)
    {
        const msi_summary_info *entry = &info[j];

        r = MsiSummaryInfoSetPropertyA(summary, entry->property, entry->datatype,
                                       entry->iValue, entry->pftValue, entry->szValue);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    }

    /* write the summary changes back to the stream */
    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void create_database(const CHAR *name, const msi_table *tables, int num_tables,
                            const msi_summary_info *info, int num_info)
{
    MSIHANDLE db;
    UINT r;
    int j;

    r = MsiOpenDatabaseA(name, MSIDBOPEN_CREATE, &db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* import the tables into the database */
    for (j = 0; j < num_tables; j++)
    {
        const msi_table *table = &tables[j];

        write_file(table->filename, table->data, (table->size - 1) * sizeof(char));

        r = MsiDatabaseImportA(db, CURR_DIR, table->filename);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

        DeleteFileA(table->filename);
    }

    write_msi_summary_info(db, info, num_info);

    r = MsiDatabaseCommit(db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(db);
}

/*
 * Installation helpers
 */

static char PROG_FILES_DIR[MAX_PATH];

static BOOL get_program_files_dir(LPSTR buf)
{
    HKEY hkey;
    DWORD type = REG_EXPAND_SZ, size;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   "Software\\Microsoft\\Windows\\CurrentVersion", &hkey))
        return FALSE;

    size = MAX_PATH;
    if (RegQueryValueEx(hkey, "ProgramFilesDir", 0, &type, (LPBYTE)buf, &size))
        return FALSE;

    RegCloseKey(hkey);
    return TRUE;
}

static void create_file(const CHAR *name, DWORD size)
{
    HANDLE file;
    DWORD written, left;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);

    left = size - lstrlen(name) - 1;

    SetFilePointer(file, left, NULL, FILE_CURRENT);
    SetEndOfFile(file);

    CloseHandle(file);
}

static void create_test_files(void)
{
    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\one.txt", 100);
    CreateDirectoryA("msitest\\first", NULL);
    create_file("msitest\\first\\two.txt", 100);
    CreateDirectoryA("msitest\\second", NULL);
    create_file("msitest\\second\\three.txt", 100);
    CreateDirectoryA("msitest\\cabout",NULL);
    create_file("msitest\\cabout\\four.txt", 100);
    CreateDirectoryA("msitest\\cabout\\new",NULL);
    create_file("msitest\\cabout\\new\\five.txt", 100);
    create_file("msitest\\filename", 100);
}

static BOOL delete_pf(const CHAR *rel_path, BOOL is_file)
{
    CHAR path[MAX_PATH];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, rel_path);

    if (is_file)
        return DeleteFileA(path);
    else
        return RemoveDirectoryA(path);
}

static void delete_test_files(void)
{
    DeleteFileA(msifile);
    DeleteFileA("msitest\\cabout\\new\\five.txt");
    DeleteFileA("msitest\\cabout\\four.txt");
    DeleteFileA("msitest\\second\\three.txt");
    DeleteFileA("msitest\\first\\two.txt");
    DeleteFileA("msitest\\one.txt");
    DeleteFileA("msitest\\filename");
    RemoveDirectoryA("msitest\\cabout\\new");
    RemoveDirectoryA("msitest\\cabout");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

/*
 * Automation helpers and tests
 */

/* ok-like statement which takes two unicode strings or one unicode and one ANSI string as arguments */
static CHAR string1[MAX_PATH], string2[MAX_PATH];

#define ok_w2(format, szString1, szString2) \
\
    if (lstrcmpW(szString1, szString2) != 0) \
    { \
        WideCharToMultiByte(CP_ACP, 0, szString1, -1, string1, MAX_PATH, NULL, NULL); \
        WideCharToMultiByte(CP_ACP, 0, szString2, -1, string2, MAX_PATH, NULL, NULL); \
        ok(0, format, string1, string2); \
    }

#define ok_w2n(format, szString1, szString2, len) \
\
    if (memcmp(szString1, szString2, len * sizeof(WCHAR)) != 0) \
    { \
        WideCharToMultiByte(CP_ACP, 0, szString1, -1, string1, MAX_PATH, NULL, NULL); \
        WideCharToMultiByte(CP_ACP, 0, szString2, -1, string2, MAX_PATH, NULL, NULL); \
        ok(0, format, string1, string2); \
    }

#define ok_aw(format, aString, wString) \
\
    WideCharToMultiByte(CP_ACP, 0, wString, -1, string1, MAX_PATH, NULL, NULL); \
    if (lstrcmpA(string1, aString) != 0) \
        ok(0, format, string1, aString); \

#define ok_awplus(format, extra, aString, wString)       \
\
    WideCharToMultiByte(CP_ACP, 0, wString, -1, string1, MAX_PATH, NULL, NULL); \
    if (lstrcmpA(string1, aString) != 0) \
        ok(0, format, extra, string1, aString);  \

/* exception checker */
static WCHAR szSource[] = {'M','s','i',' ','A','P','I',' ','E','r','r','o','r',0};

#define ok_exception(hr, szDescription)           \
    if (hr == DISP_E_EXCEPTION) \
    { \
        /* Compare wtype, source, and destination */                    \
        ok(excepinfo.wCode == 1000, "Exception info was %d, expected 1000\n", excepinfo.wCode); \
\
        ok(excepinfo.bstrSource != NULL, "Exception source was NULL\n"); \
        if (excepinfo.bstrSource)                                       \
            ok_w2("Exception source was \"%s\" but expected to be \"%s\"\n", excepinfo.bstrSource, szSource); \
\
        ok(excepinfo.bstrDescription != NULL, "Exception description was NULL\n"); \
        if (excepinfo.bstrDescription) \
            ok_w2("Exception description was \"%s\" but expected to be \"%s\"\n", excepinfo.bstrDescription, szDescription); \
\
        SysFreeString(excepinfo.bstrSource); \
        SysFreeString(excepinfo.bstrDescription); \
        SysFreeString(excepinfo.bstrHelpFile); \
    }

static DISPID get_dispid( IDispatch *disp, const char *name )
{
    LPOLESTR str;
    UINT len;
    DISPID id = -1;
    HRESULT r;

    len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0 );
    str = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if (str)
    {
        len = MultiByteToWideChar(CP_ACP, 0, name, -1, str, len );
        r = IDispatch_GetIDsOfNames( disp, &IID_NULL, &str, 1, 0, &id );
        HeapFree(GetProcessHeap(), 0, str);
        if (r != S_OK)
            return -1;
    }

    return id;
}

static void test_dispid(void)
{
    DISPID dispid;

    dispid = get_dispid(pInstaller, "CreateRecord");
    ok(dispid  == 1, "Expected 1, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "OpenPackage");
    ok(dispid  == 2, "Expected 2, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "OpenDatabase");
    ok(dispid == 4, "Expected 4, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "UILevel" );
    ok(dispid == 6, "Expected 6, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "InstallProduct");
    ok(dispid == 8, "Expected 8, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "Version");
    ok(dispid == 9, "Expected 9, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "RegistryValue");
    ok(dispid == 11, "Expected 11, got %d\n", dispid);
    todo_wine
    {
        dispid = get_dispid(pInstaller, "OpenProduct");
        ok(dispid  == 3, "Expected 3, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "SummaryInformation");
        ok(dispid == 5, "Expected 5, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "EnableLog");
        ok(dispid == 7, "Expected 7, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "LastErrorRecord");
        ok(dispid == 10, "Expected 10, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "Environment");
        ok(dispid == 12, "Expected 12, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FileAttributes");
        ok(dispid == 13, "Expected 13, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FileSize");
        ok(dispid == 15, "Expected 15, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FileVersion");
        ok(dispid == 16, "Expected 16, got %d\n", dispid);
    }
    dispid = get_dispid(pInstaller, "ProductState");
    ok(dispid == 17, "Expected 17, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "ProductInfo");
    ok(dispid == 18, "Expected 18, got %d\n", dispid);
    todo_wine
    {
        dispid = get_dispid(pInstaller, "ConfigureProduct");
        ok(dispid == 19, "Expected 19, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ReinstallProduct");
        ok(dispid == 20 , "Expected 20, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "CollectUserInfo");
        ok(dispid == 21, "Expected 21, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ApplyPatch");
        ok(dispid == 22, "Expected 22, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FeatureParent");
        ok(dispid == 23, "Expected 23, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FeatureState");
        ok(dispid == 24, "Expected 24, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "UseFeature");
        ok(dispid == 25, "Expected 25, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FeatureUsageCount");
        ok(dispid == 26, "Expected 26, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FeatureUsageDate");
        ok(dispid == 27, "Expected 27, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ConfigureFeature");
        ok(dispid == 28, "Expected 28, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ReinstallFeature");
        ok(dispid == 29, "Expected 29, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ProvideComponent");
        ok(dispid == 30, "Expected 30, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ComponentPath");
        ok(dispid == 31, "Expected 31, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ProvideQualifiedComponent");
        ok(dispid == 32, "Expected 32, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "QualifierDescription");
        ok(dispid == 33, "Expected 33, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ComponentQualifiers");
        ok(dispid == 34, "Expected 34, got %d\n", dispid);
    }
    dispid = get_dispid(pInstaller, "Products");
    ok(dispid == 35, "Expected 35, got %d\n", dispid);
    todo_wine
    {
        dispid = get_dispid(pInstaller, "Features");
        ok(dispid == 36, "Expected 36, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "Components");
        ok(dispid == 37, "Expected 37, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ComponentClients");
        ok(dispid == 38, "Expected 38, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "Patches");
        ok(dispid == 39, "Expected 39, got %d\n", dispid);
    }
    dispid = get_dispid(pInstaller, "RelatedProducts");
    ok(dispid == 40, "Expected 40, got %d\n", dispid);
    todo_wine
    {
        dispid = get_dispid(pInstaller, "PatchInfo");
        ok(dispid == 41, "Expected 41, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "PatchTransforms");
        ok(dispid == 42, "Expected 42, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "AddSource");
        ok(dispid == 43, "Expected 43, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ClearSourceList");
        ok(dispid == 44, "Expected 44, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ForceSourceListResolution");
        ok(dispid == 45, "Expected 45, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "ShortcutTarget");
        ok(dispid == 46, "Expected 46, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FileHash");
        ok(dispid == 47, "Expected 47, got %d\n", dispid);
        dispid = get_dispid(pInstaller, "FileSignatureInfo");
        ok(dispid == 48, "Expected 48, got %d\n", dispid);
    }
    dispid = get_dispid(pInstaller, "RemovePatches");
    ok(dispid == 49 || dispid == -1, "Expected 49 or -1, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "ApplyMultiplePatches");
    ok(dispid == 51 || dispid == -1, "Expected 51 or -1, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "ProductsEx");
    ok(dispid == 52 || dispid == -1, "Expected 52 or -1, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "PatchesEx");
    ok(dispid == 55 || dispid == -1, "Expected 55 or -1, got %d\n", dispid);
    dispid = get_dispid(pInstaller, "ExtractPatchXMLData");
    ok(dispid == 57 || dispid == -1, "Expected 57 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "ProductElevated" );
    ok(dispid == 59 || dispid == -1, "Expected 59 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "ProvideAssembly" );
    ok(dispid == 60 || dispid == -1, "Expected 60 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "ProductInfoFromScript" );
    ok(dispid == 61 || dispid == -1, "Expected 61 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "AdvertiseProduct" );
    ok(dispid == 62 || dispid == -1, "Expected 62 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "CreateAdvertiseScript" );
    ok(dispid == 63 || dispid == -1, "Expected 63 or -1, got %d\n", dispid);
    dispid = get_dispid( pInstaller, "PatchFiles" );
    ok(dispid == 65 || dispid == -1, "Expected 65 or -1, got %d\n", dispid);
}

/* Test basic IDispatch functions */
static void test_dispatch(void)
{
    static WCHAR szOpenPackage[] = { 'O','p','e','n','P','a','c','k','a','g','e',0 };
    static WCHAR szOpenPackageException[] = {'O','p','e','n','P','a','c','k','a','g','e',',','P','a','c','k','a','g','e','P','a','t','h',',','O','p','t','i','o','n','s',0};
    static WCHAR szProductState[] = { 'P','r','o','d','u','c','t','S','t','a','t','e',0 };
    HRESULT hr;
    DISPID dispid;
    OLECHAR *name;
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};

    /* Test getting ID of a function name that does not exist */
    name = (WCHAR *)szMsifile;
    hr = IDispatch_GetIDsOfNames(pInstaller, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    ok(hr == DISP_E_UNKNOWNNAME, "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);

    /* Test invoking this function */
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Test getting ID of a function name that does exist */
    name = (WCHAR *)szOpenPackage;
    hr = IDispatch_GetIDsOfNames(pInstaller, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    ok(hr == S_OK, "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);

    /* Test invoking this function (without parameters passed) */
    if (0) /* All of these crash MSI on Windows XP */
    {
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, NULL, &excepinfo, NULL);
        VariantInit(&varresult);
        hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, NULL, &varresult, &excepinfo, NULL);
    }

    /* Try with NULL params */
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    todo_wine ok(hr == DISP_E_TYPEMISMATCH, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Try one empty parameter */
    dispparams.rgvarg = vararg;
    dispparams.cArgs = 1;
    VariantInit(&vararg[0]);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    todo_wine ok(hr == DISP_E_TYPEMISMATCH, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Try one parameter, function requires two */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szMsifile);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    VariantClear(&vararg[0]);

    ok(hr == DISP_E_EXCEPTION, "IDispatch::Invoke returned 0x%08x\n", hr);
    ok_exception(hr, szOpenPackageException);

    /* Test invoking a method as a DISPATCH_PROPERTYGET or DISPATCH_PROPERTYPUT */
    VariantInit(&vararg[0]);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);

    VariantInit(&vararg[0]);
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);

    /* Test invoking a read-only property as DISPATCH_PROPERTYPUT or as a DISPATCH_METHOD */
    name = (WCHAR *)szProductState;
    hr = IDispatch_GetIDsOfNames(pInstaller, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    ok(hr == S_OK, "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);

    dispparams.rgvarg = NULL;
    dispparams.cArgs = 0;
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);

    dispparams.rgvarg = NULL;
    dispparams.cArgs = 0;
    hr = IDispatch_Invoke(pInstaller, dispid, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IDispatch::Invoke returned 0x%08x\n", hr);
}

/* invocation helper function */
static int _invoke_todo_vtResult = 0;

static HRESULT invoke(IDispatch *pDispatch, LPCSTR szName, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, VARTYPE vtResult)
{
    OLECHAR *name = NULL;
    DISPID dispid;
    HRESULT hr;
    UINT i;
    UINT len;

    memset(pVarResult, 0, sizeof(VARIANT));
    VariantInit(pVarResult);

    len = MultiByteToWideChar(CP_ACP, 0, szName, -1, NULL, 0 );
    name = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if (!name) return E_FAIL;
    len = MultiByteToWideChar(CP_ACP, 0, szName, -1, name, len );
    hr = IDispatch_GetIDsOfNames(pDispatch, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
    HeapFree(GetProcessHeap(), 0, name);
    ok(hr == S_OK, "IDispatch::GetIDsOfNames returned 0x%08x\n", hr);
    if (!hr == S_OK) return hr;

    memset(&excepinfo, 0, sizeof(excepinfo));
    hr = IDispatch_Invoke(pDispatch, dispid, &IID_NULL, LOCALE_NEUTRAL, wFlags, pDispParams, pVarResult, &excepinfo, NULL);

    if (hr == S_OK)
    {
        if (_invoke_todo_vtResult) todo_wine
            ok(V_VT(pVarResult) == vtResult, "Variant result type is %d, expected %d\n", V_VT(pVarResult), vtResult);
        else
            ok(V_VT(pVarResult) == vtResult, "Variant result type is %d, expected %d\n", V_VT(pVarResult), vtResult);
        if (vtResult != VT_EMPTY)
        {
            hr = VariantChangeTypeEx(pVarResult, pVarResult, LOCALE_NEUTRAL, 0, vtResult);
            ok(hr == S_OK, "VariantChangeTypeEx returned 0x%08x\n", hr);
        }
    }

    for (i=0; i<pDispParams->cArgs; i++)
        VariantClear(&pDispParams->rgvarg[i]);

    return hr;
}

/* Object_Property helper functions */

static HRESULT Installer_CreateRecord(int count, IDispatch **pRecord)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = count;

    hr = invoke(pInstaller, "CreateRecord", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pRecord = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValue(HKEY hkey, LPCWSTR szKey, VARIANT vValue, VARIANT *pVarResult, VARTYPE vtExpect)
{
    VARIANTARG vararg[3];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[2]);
    V_VT(&vararg[2]) = VT_I4;
    V_I4(&vararg[2]) = (int)hkey;
    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szKey);
    VariantInit(&vararg[0]);
    VariantCopy(&vararg[0], &vValue);
    VariantClear(&vValue);

    return invoke(pInstaller, "RegistryValue", DISPATCH_METHOD, &dispparams, pVarResult, vtExpect);
}

static HRESULT Installer_RegistryValueE(HKEY hkey, LPCWSTR szKey, BOOL *pBool)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_EMPTY;
    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, VT_BOOL);
    *pBool = V_BOOL(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValueW(HKEY hkey, LPCWSTR szKey, LPCWSTR szValue, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_BSTR;
    V_BSTR(&vararg) = SysAllocString(szValue);

    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_RegistryValueI(HKEY hkey, LPCWSTR szKey, int iValue, LPWSTR szString, VARTYPE vtResult)
{
    VARIANT varresult;
    VARIANTARG vararg;
    HRESULT hr;

    VariantInit(&vararg);
    V_VT(&vararg) = VT_I4;
    V_I4(&vararg) = iValue;

    hr = Installer_RegistryValue(hkey, szKey, vararg, &varresult, vtResult);
    if (SUCCEEDED(hr) && vtResult == VT_BSTR) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_OpenPackage(LPCWSTR szPackagePath, int options, IDispatch **pSession)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szPackagePath);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = options;

    hr = invoke(pInstaller, "OpenPackage", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pSession = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_OpenDatabase(LPCWSTR szDatabasePath, int openmode, IDispatch **pDatabase)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szDatabasePath);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = openmode;

    hr = invoke(pInstaller, "OpenDatabase", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pDatabase = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_InstallProduct(LPCWSTR szPackagePath, LPCWSTR szPropertyValues)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szPackagePath);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szPropertyValues);

    return invoke(pInstaller, "InstallProduct", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Installer_ProductState(LPCWSTR szProduct, int *pInstallState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szProduct);

    hr = invoke(pInstaller, "ProductState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pInstallState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_ProductInfo(LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szProduct);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szAttribute);

    hr = invoke(pInstaller, "ProductInfo", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Installer_Products(IDispatch **pStringList)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pInstaller, "Products", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pStringList = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_RelatedProducts(LPCWSTR szProduct, IDispatch **pStringList)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szProduct);

    hr = invoke(pInstaller, "RelatedProducts", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pStringList = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Installer_VersionGet(LPWSTR szVersion)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pInstaller, "Version", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szVersion, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_Installer(IDispatch *pSession, IDispatch **pInst)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Installer", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pInst = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Session_PropertyGet(IDispatch *pSession, LPCWSTR szName, LPWSTR szReturn)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "Property", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szReturn, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_PropertyPut(IDispatch *pSession, LPCWSTR szName, LPCWSTR szValue)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szName);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szValue);

    return invoke(pSession, "Property", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_LanguageGet(IDispatch *pSession, UINT *pLangId)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Language", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pLangId = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_ModeGet(IDispatch *pSession, int iFlag, BOOL *pMode)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iFlag;

    hr = invoke(pSession, "Mode", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BOOL);
    *pMode = V_BOOL(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_ModePut(IDispatch *pSession, int iFlag, BOOL bMode)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iFlag;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BOOL;
    V_BOOL(&vararg[0]) = bMode;

    return invoke(pSession, "Mode", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_Database(IDispatch *pSession, IDispatch **pDatabase)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSession, "Database", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pDatabase = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Session_DoAction(IDispatch *pSession, LPCWSTR szAction, int *iReturn)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szAction);

    hr = invoke(pSession, "DoAction", DISPATCH_METHOD, &dispparams, &varresult, VT_I4);
    *iReturn = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_EvaluateCondition(IDispatch *pSession, LPCWSTR szCondition, int *iReturn)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szCondition);

    hr = invoke(pSession, "EvaluateCondition", DISPATCH_METHOD, &dispparams, &varresult, VT_I4);
    *iReturn = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_Message(IDispatch *pSession, long kind, IDispatch *record, int *ret)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&varresult);
    V_VT(vararg) = VT_DISPATCH;
    V_DISPATCH(vararg) = record;
    V_VT(vararg+1) = VT_I4;
    V_I4(vararg+1) = kind;

    hr = invoke(pSession, "Message", DISPATCH_METHOD, &dispparams, &varresult, VT_I4);

    ok(V_VT(&varresult) == VT_I4, "V_VT(varresult) = %d\n", V_VT(&varresult));
    *ret = V_I4(&varresult);

    return hr;
}

static HRESULT Session_SetInstallLevel(IDispatch *pSession, long iInstallLevel)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iInstallLevel;

    return invoke(pSession, "SetInstallLevel", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Session_FeatureCurrentState(IDispatch *pSession, LPCWSTR szName, int *pState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "FeatureCurrentState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_FeatureRequestStateGet(IDispatch *pSession, LPCWSTR szName, int *pState)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szName);

    hr = invoke(pSession, "FeatureRequestState", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pState = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Session_FeatureRequestStatePut(IDispatch *pSession, LPCWSTR szName, int iState)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szName);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iState;

    return invoke(pSession, "FeatureRequestState", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Database_OpenView(IDispatch *pDatabase, LPCWSTR szSql, IDispatch **pView)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szSql);

    hr = invoke(pDatabase, "OpenView", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *pView = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT Database_SummaryInformation(IDispatch *pDatabase, int iUpdateCount, IDispatch **pSummaryInfo)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iUpdateCount;

    hr = invoke(pDatabase, "SummaryInformation", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_DISPATCH);
    *pSummaryInfo = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT View_Execute(IDispatch *pView, IDispatch *pRecord)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_DISPATCH;
    V_DISPATCH(&vararg[0]) = pRecord;

    return invoke(pView, "Execute", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT View_Fetch(IDispatch *pView, IDispatch **ppRecord)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pView, "Fetch", DISPATCH_METHOD, &dispparams, &varresult, VT_DISPATCH);
    *ppRecord = V_DISPATCH(&varresult);
    return hr;
}

static HRESULT View_Modify(IDispatch *pView, int iMode, IDispatch *pRecord)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iMode;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_DISPATCH;
    V_DISPATCH(&vararg[0]) = pRecord;
    if (pRecord)
        IDispatch_AddRef(pRecord);   /* VariantClear in invoke will call IDispatch_Release */

    return invoke(pView, "Modify", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT View_Close(IDispatch *pView)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    return invoke(pView, "Close", DISPATCH_METHOD, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Record_FieldCountGet(IDispatch *pRecord, int *pFieldCount)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pRecord, "FieldCount", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pFieldCount = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Record_StringDataGet(IDispatch *pRecord, int iField, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iField;

    hr = invoke(pRecord, "StringData", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT Record_StringDataPut(IDispatch *pRecord, int iField, LPCWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iField;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = SysAllocString(szString);

    return invoke(pRecord, "StringData", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT Record_IntegerDataGet(IDispatch *pRecord, int iField, int *pValue)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iField;

    hr = invoke(pRecord, "IntegerData", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pValue = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT Record_IntegerDataPut(IDispatch *pRecord, int iField, int iValue)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = iField;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iValue;

    return invoke(pRecord, "IntegerData", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT StringList__NewEnum(IDispatch *pList, IUnknown **ppEnumVARIANT)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pList, "_NewEnum", DISPATCH_METHOD, &dispparams, &varresult, VT_UNKNOWN);
    *ppEnumVARIANT = V_UNKNOWN(&varresult);
    return hr;
}

static HRESULT StringList_Item(IDispatch *pStringList, int iIndex, LPWSTR szString)
{
    VARIANT varresult;
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};
    HRESULT hr;

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = iIndex;

    hr = invoke(pStringList, "Item", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_BSTR);
    if (V_BSTR(&varresult)) lstrcpyW(szString, V_BSTR(&varresult));
    VariantClear(&varresult);
    return hr;
}

static HRESULT StringList_Count(IDispatch *pStringList, int *pCount)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr = invoke(pStringList, "Count", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pCount = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

static HRESULT SummaryInfo_PropertyGet(IDispatch *pSummaryInfo, int pid, VARIANT *pVarResult, VARTYPE vtExpect)
{
    VARIANTARG vararg[1];
    DISPPARAMS dispparams = {vararg, NULL, sizeof(vararg)/sizeof(VARIANTARG), 0};

    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = pid;
    return invoke(pSummaryInfo, "Property", DISPATCH_PROPERTYGET, &dispparams, pVarResult, vtExpect);
}

static HRESULT SummaryInfo_PropertyPut(IDispatch *pSummaryInfo, int pid, VARIANT *pVariant)
{
    VARIANT varresult;
    VARIANTARG vararg[2];
    DISPID dispid = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = {vararg, &dispid, sizeof(vararg)/sizeof(VARIANTARG), 1};

    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = pid;
    VariantInit(&vararg[0]);
    VariantCopyInd(vararg, pVariant);

    return invoke(pSummaryInfo, "Property", DISPATCH_PROPERTYPUT, &dispparams, &varresult, VT_EMPTY);
}

static HRESULT SummaryInfo_PropertyCountGet(IDispatch *pSummaryInfo, int *pCount)
{
    VARIANT varresult;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    HRESULT hr;

    hr = invoke(pSummaryInfo, "PropertyCount", DISPATCH_PROPERTYGET, &dispparams, &varresult, VT_I4);
    *pCount = V_I4(&varresult);
    VariantClear(&varresult);
    return hr;
}

/* Test the various objects */

#define TEST_SUMMARYINFO_PROPERTIES_MODIFIED 4

static void test_SummaryInfo(IDispatch *pSummaryInfo, const msi_summary_info *info, int num_info, BOOL readonly)
{
    static const WCHAR szPropertyException[] = { 'P','r','o','p','e','r','t','y',',','P','i','d',0 };
    static const WCHAR szTitle[] = { 'T','i','t','l','e',0 };
    VARIANT varresult, var;
    SYSTEMTIME st;
    HRESULT hr;
    int j;

    /* SummaryInfo::PropertyCount */
    hr = SummaryInfo_PropertyCountGet(pSummaryInfo, &j);
    ok(hr == S_OK, "SummaryInfo_PropertyCount failed, hresult 0x%08x\n", hr);
    ok(j == num_info, "SummaryInfo_PropertyCount returned %d, expected %d\n", j, num_info);

    /* SummaryInfo::Property, get for properties we have set */
    for (j = 0; j < num_info; j++)
    {
        const msi_summary_info *entry = &info[j];

        int vt = entry->datatype;
        if (vt == VT_LPSTR) vt = VT_BSTR;
        else if (vt == VT_FILETIME) vt = VT_DATE;
        else if (vt == VT_I2) vt = VT_I4;

        hr = SummaryInfo_PropertyGet(pSummaryInfo, entry->property, &varresult, vt);
        ok(hr == S_OK, "SummaryInfo_Property (pid %d) failed, hresult 0x%08x\n", entry->property, hr);
        if (V_VT(&varresult) != vt)
            skip("Skipping property tests due to type mismatch\n");
        else if (vt == VT_I4)
            ok(V_I4(&varresult) == entry->iValue, "SummaryInfo_Property (pid %d) I4 result expected to be %d, but was %d\n",
               entry->property, entry->iValue, V_I4(&varresult));
        else if (vt == VT_DATE)
        {
            FILETIME ft;
            DATE d;

            FileTimeToLocalFileTime(entry->pftValue, &ft);
            FileTimeToSystemTime(&ft, &st);
            SystemTimeToVariantTime(&st, &d);
            ok(d == V_DATE(&varresult), "SummaryInfo_Property (pid %d) DATE result expected to be %lf, but was %lf\n", entry->property, d, V_DATE(&varresult));
        }
        else if (vt == VT_BSTR)
        {
            ok_awplus("SummaryInfo_Property (pid %d) BSTR result expected to be %s, but was %s\n", entry->property, entry->szValue, V_BSTR(&varresult));
        }
        else
            skip("SummaryInfo_Property (pid %d) unhandled result type %d\n", entry->property, vt);

        VariantClear(&varresult);
    }

    /* SummaryInfo::Property, get; invalid arguments */

    /* Invalid pids */
    hr = SummaryInfo_PropertyGet(pSummaryInfo, -1, &varresult, VT_EMPTY);
    ok(hr == DISP_E_EXCEPTION, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szPropertyException);

    hr = SummaryInfo_PropertyGet(pSummaryInfo, 1000, &varresult, VT_EMPTY);
    ok(hr == DISP_E_EXCEPTION, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szPropertyException);

    /* Unsupported pids */
    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_DICTIONARY, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_THUMBNAIL, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    /* Pids we have not set, one for each type */
    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_CODEPAGE, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_TITLE, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_EDITTIME, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_CHARCOUNT, &varresult, VT_EMPTY);
    ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);

    if (!readonly)
    {
        /* SummaryInfo::Property, put; one for each type */

        /* VT_I2 */
        VariantInit(&var);
        V_VT(&var) = VT_I2;
        V_I2(&var) = 1;
        hr = SummaryInfo_PropertyPut(pSummaryInfo, PID_CODEPAGE, &var);
        ok(hr == S_OK, "SummaryInfo_PropertyPut failed, hresult 0x%08x\n", hr);

        hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_CODEPAGE, &varresult, VT_I4 /* NOT VT_I2 */);
        ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
        ok(V_I2(&var) == V_I2(&varresult), "SummaryInfo_PropertyGet expected %d, but returned %d\n", V_I2(&var), V_I2(&varresult));
        VariantClear(&varresult);
        VariantClear(&var);

        /* VT_BSTR */
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(szTitle);
        hr = SummaryInfo_PropertyPut(pSummaryInfo, PID_TITLE, &var);
        ok(hr == S_OK, "SummaryInfo_PropertyPut failed, hresult 0x%08x\n", hr);

        hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_TITLE, &varresult, V_VT(&var));
        ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
        ok_w2("SummaryInfo_PropertyGet expected %s, but returned %s\n", V_BSTR(&var), V_BSTR(&varresult));
        VariantClear(&varresult);
        VariantClear(&var);

        /* VT_DATE */
        V_VT(&var) = VT_DATE;
        FileTimeToSystemTime(&systemtime, &st);
        SystemTimeToVariantTime(&st, &V_DATE(&var));
        hr = SummaryInfo_PropertyPut(pSummaryInfo, PID_LASTSAVE_DTM, &var);
        ok(hr == S_OK, "SummaryInfo_PropertyPut failed, hresult 0x%08x\n", hr);

        hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_LASTSAVE_DTM, &varresult, V_VT(&var));
        ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
        /* FIXME: Off by one second */
        todo_wine ok(V_DATE(&var) == V_DATE(&varresult), "SummaryInfo_PropertyGet expected %lf, but returned %lf\n", V_DATE(&var), V_DATE(&varresult));
        VariantClear(&varresult);
        VariantClear(&var);

        /* VT_I4 */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 1000;
        hr = SummaryInfo_PropertyPut(pSummaryInfo, PID_CHARCOUNT, &var);
        ok(hr == S_OK, "SummaryInfo_PropertyPut failed, hresult 0x%08x\n", hr);

        hr = SummaryInfo_PropertyGet(pSummaryInfo, PID_CHARCOUNT, &varresult, V_VT(&var));
        ok(hr == S_OK, "SummaryInfo_PropertyGet failed, hresult 0x%08x\n", hr);
        ok(V_I4(&var) == V_I4(&varresult), "SummaryInfo_PropertyGet expected %d, but returned %d\n", V_I4(&var), V_I4(&varresult));
        VariantClear(&varresult);
        VariantClear(&var);

        /* SummaryInfo::PropertyCount */
        hr = SummaryInfo_PropertyCountGet(pSummaryInfo, &j);
        ok(hr == S_OK, "SummaryInfo_PropertyCount failed, hresult 0x%08x\n", hr);
        ok(j == num_info+4, "SummaryInfo_PropertyCount returned %d, expected %d\n", j, num_info);
    }
}

static void test_Database(IDispatch *pDatabase, BOOL readonly)
{
    static WCHAR szSql[] = { 'S','E','L','E','C','T',' ','`','F','e','a','t','u','r','e','`',' ','F','R','O','M',' ','`','F','e','a','t','u','r','e','`',' ','W','H','E','R','E',' ','`','F','e','a','t','u','r','e','_','P','a','r','e','n','t','`','=','\'','O','n','e','\'',0 };
    static WCHAR szThree[] = { 'T','h','r','e','e',0 };
    static WCHAR szTwo[] = { 'T','w','o',0 };
    static WCHAR szStringDataField[] = { 'S','t','r','i','n','g','D','a','t','a',',','F','i','e','l','d',0 };
    static WCHAR szModifyModeRecord[] = { 'M','o','d','i','f','y',',','M','o','d','e',',','R','e','c','o','r','d',0 };
    IDispatch *pView = NULL, *pSummaryInfo = NULL;
    HRESULT hr;

    hr = Database_OpenView(pDatabase, szSql, &pView);
    ok(hr == S_OK, "Database_OpenView failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        IDispatch *pRecord = NULL;
        WCHAR szString[MAX_PATH];

        /* View::Execute */
        hr = View_Execute(pView, NULL);
        ok(hr == S_OK, "View_Execute failed, hresult 0x%08x\n", hr);

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(hr == S_OK, "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord != NULL, "View_Fetch should not have returned NULL record\n");
        if (pRecord)
        {
            /* Record::StringDataGet */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(hr == S_OK, "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szThree);

            /* Record::StringDataPut with correct index */
            hr = Record_StringDataPut(pRecord, 1, szTwo);
            ok(hr == S_OK, "Record_StringDataPut failed, hresult 0x%08x\n", hr);

            /* Record::StringDataGet */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(hr == S_OK, "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szTwo);

            /* Record::StringDataPut with incorrect index */
            hr = Record_StringDataPut(pRecord, -1, szString);
            ok(hr == DISP_E_EXCEPTION, "Record_StringDataPut failed, hresult 0x%08x\n", hr);
            ok_exception(hr, szStringDataField);

            /* View::Modify with incorrect parameters */
            hr = View_Modify(pView, -5, NULL);
            ok(hr == DISP_E_EXCEPTION, "View_Modify failed, hresult 0x%08x\n", hr);
            ok_exception(hr, szModifyModeRecord);

            hr = View_Modify(pView, -5, pRecord);
            ok(hr == DISP_E_EXCEPTION, "View_Modify failed, hresult 0x%08x\n", hr);
            ok_exception(hr, szModifyModeRecord);

            hr = View_Modify(pView, MSIMODIFY_REFRESH, NULL);
            ok(hr == DISP_E_EXCEPTION, "View_Modify failed, hresult 0x%08x\n", hr);
            ok_exception(hr, szModifyModeRecord);

            hr = View_Modify(pView, MSIMODIFY_REFRESH, pRecord);
            ok(hr == S_OK, "View_Modify failed, hresult 0x%08x\n", hr);

            /* Record::StringDataGet, confirm that the record is back to its unmodified value */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(hr == S_OK, "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            todo_wine ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szThree);

            IDispatch_Release(pRecord);
        }

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(hr == S_OK, "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord != NULL, "View_Fetch should not have returned NULL record\n");
        if (pRecord)
        {
            /* Record::StringDataGet */
            memset(szString, 0, sizeof(szString));
            hr = Record_StringDataGet(pRecord, 1, szString);
            ok(hr == S_OK, "Record_StringDataGet failed, hresult 0x%08x\n", hr);
            ok_w2("Record_StringDataGet result was %s but expected %s\n", szString, szTwo);

            IDispatch_Release(pRecord);
        }

        /* View::Fetch */
        hr = View_Fetch(pView, &pRecord);
        ok(hr == S_OK, "View_Fetch failed, hresult 0x%08x\n", hr);
        ok(pRecord == NULL, "View_Fetch should have returned NULL record\n");
        if (pRecord)
            IDispatch_Release(pRecord);

        /* View::Close */
        hr = View_Close(pView);
        ok(hr == S_OK, "View_Close failed, hresult 0x%08x\n", hr);

        IDispatch_Release(pView);
    }

    /* Database::SummaryInformation */
    hr = Database_SummaryInformation(pDatabase, TEST_SUMMARYINFO_PROPERTIES_MODIFIED, &pSummaryInfo);
    ok(hr == S_OK, "Database_SummaryInformation failed, hresult 0x%08x\n", hr);
    ok(pSummaryInfo != NULL, "Database_SummaryInformation should not have returned NULL record\n");
    if (pSummaryInfo)
    {
        test_SummaryInfo(pSummaryInfo, summary_info, sizeof(summary_info)/sizeof(msi_summary_info), readonly);
        IDispatch_Release(pSummaryInfo);
    }
}

static void test_Session(IDispatch *pSession)
{
    static WCHAR szProductName[] = { 'P','r','o','d','u','c','t','N','a','m','e',0 };
    static WCHAR szOne[] = { 'O','n','e',0 };
    static WCHAR szOneStateFalse[] = { '!','O','n','e','>','0',0 };
    static WCHAR szOneStateTrue[] = { '!','O','n','e','=','-','1',0 };
    static WCHAR szOneActionFalse[] = { '$','O','n','e','=','-','1',0 };
    static WCHAR szOneActionTrue[] = { '$','O','n','e','>','0',0 };
    static WCHAR szCostInitialize[] = { 'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0 };
    static WCHAR szEmpty[] = { 0 };
    static WCHAR szEquals[] = { '=',0 };
    static WCHAR szPropertyName[] = { 'P','r','o','p','e','r','t','y',',','N','a','m','e',0 };
    WCHAR stringw[MAX_PATH];
    CHAR string[MAX_PATH];
    UINT len;
    BOOL bool;
    int myint;
    IDispatch *pDatabase = NULL, *pInst = NULL, *record = NULL;
    HRESULT hr;

    /* Session::Installer */
    hr = Session_Installer(pSession, &pInst);
    ok(hr == S_OK, "Session_Installer failed, hresult 0x%08x\n", hr);
    ok(pInst != NULL, "Session_Installer returned NULL IDispatch pointer\n");
    ok(pInst == pInstaller, "Session_Installer does not match Installer instance from CoCreateInstance\n");

    /* Session::Property, get */
    memset(stringw, 0, sizeof(stringw));
    hr = Session_PropertyGet(pSession, szProductName, stringw);
    ok(hr == S_OK, "Session_PropertyGet failed, hresult 0x%08x\n", hr);
    if (lstrcmpW(stringw, szMSITEST) != 0)
    {
        len = WideCharToMultiByte(CP_ACP, 0, stringw, -1, string, MAX_PATH, NULL, NULL);
        ok(len, "WideCharToMultiByteChar returned error %d\n", GetLastError());
        ok(0, "Property \"ProductName\" expected to be \"MSITEST\" but was \"%s\"\n", string);
    }

    /* Session::Property, put */
    hr = Session_PropertyPut(pSession, szProductName, szProductName);
    ok(hr == S_OK, "Session_PropertyPut failed, hresult 0x%08x\n", hr);
    memset(stringw, 0, sizeof(stringw));
    hr = Session_PropertyGet(pSession, szProductName, stringw);
    ok(hr == S_OK, "Session_PropertyGet failed, hresult 0x%08x\n", hr);
    if (lstrcmpW(stringw, szProductName) != 0)
    {
        len = WideCharToMultiByte(CP_ACP, 0, stringw, -1, string, MAX_PATH, NULL, NULL);
        ok(len, "WideCharToMultiByteChar returned error %d\n", GetLastError());
        ok(0, "Property \"ProductName\" expected to be \"ProductName\" but was \"%s\"\n", string);
    }

    /* Try putting a property using empty property identifier */
    hr = Session_PropertyPut(pSession, szEmpty, szProductName);
    ok(hr == DISP_E_EXCEPTION, "Session_PropertyPut failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szPropertyName);

    /* Try putting a property using illegal property identifier */
    hr = Session_PropertyPut(pSession, szEquals, szProductName);
    ok(hr == S_OK, "Session_PropertyPut failed, hresult 0x%08x\n", hr);

    /* Session::Language, get */
    hr = Session_LanguageGet(pSession, &len);
    ok(hr == S_OK, "Session_LanguageGet failed, hresult 0x%08x\n", hr);
    /* Not sure how to check the language is correct */

    /* Session::Mode, get */
    hr = Session_ModeGet(pSession, MSIRUNMODE_REBOOTATEND, &bool);
    ok(hr == S_OK, "Session_ModeGet failed, hresult 0x%08x\n", hr);
    todo_wine ok(!bool, "Reboot at end session mode is %d\n", bool);

    /* Session::Mode, put */
    hr = Session_ModePut(pSession, MSIRUNMODE_REBOOTATEND, TRUE);
    todo_wine ok(hr == S_OK, "Session_ModePut failed, hresult 0x%08x\n", hr);
    hr = Session_ModeGet(pSession, MSIRUNMODE_REBOOTATEND, &bool);
    ok(hr == S_OK, "Session_ModeGet failed, hresult 0x%08x\n", hr);
    ok(bool, "Reboot at end session mode is %d, expected 1\n", bool);
    hr = Session_ModePut(pSession, MSIRUNMODE_REBOOTATEND, FALSE);  /* set it again so we don't reboot */
    todo_wine ok(hr == S_OK, "Session_ModePut failed, hresult 0x%08x\n", hr);

    /* Session::Database, get */
    hr = Session_Database(pSession, &pDatabase);
    ok(hr == S_OK, "Session_Database failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        test_Database(pDatabase, TRUE);
        IDispatch_Release(pDatabase);
    }

    /* Session::EvaluateCondition */
    hr = Session_EvaluateCondition(pSession, NULL, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_NONE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    hr = Session_EvaluateCondition(pSession, szEmpty, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_NONE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    hr = Session_EvaluateCondition(pSession, szEquals, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_ERROR, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    /* Session::DoAction(CostInitialize) must occur before the next statements */
    hr = Session_DoAction(pSession, szCostInitialize, &myint);
    ok(hr == S_OK, "Session_DoAction failed, hresult 0x%08x\n", hr);
    ok(myint == IDOK, "DoAction(CostInitialize) returned %d, %d expected\n", myint, IDOK);

    /* Session::SetInstallLevel */
    hr = Session_SetInstallLevel(pSession, INSTALLLEVEL_MINIMUM);
    ok(hr == S_OK, "Session_SetInstallLevel failed, hresult 0x%08x\n", hr);

    /* Session::FeatureCurrentState, get */
    hr = Session_FeatureCurrentState(pSession, szOne, &myint);
    ok(hr == S_OK, "Session_FeatureCurrentState failed, hresult 0x%08x\n", hr);
    ok(myint == INSTALLSTATE_UNKNOWN, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    /* Session::Message */
    hr = Installer_CreateRecord(0, &record);
    ok(hr == S_OK, "Installer_CreateRecord failed: %08x\n", hr);
    hr = Session_Message(pSession, INSTALLMESSAGE_INFO, record, &myint);
    ok(hr == S_OK, "Session_Message failed: %08x\n", hr);
    ok(myint == 0, "Session_Message returned %x\n", myint);

    /* Session::EvaluateCondition */
    hr = Session_EvaluateCondition(pSession, szOneStateFalse, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_FALSE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    hr = Session_EvaluateCondition(pSession, szOneStateTrue, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_TRUE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    /* Session::FeatureRequestState, put */
    hr = Session_FeatureRequestStatePut(pSession, szOne, INSTALLSTATE_ADVERTISED);
    ok(hr == S_OK, "Session_FeatureRequestStatePut failed, hresult 0x%08x\n", hr);
    hr = Session_FeatureRequestStateGet(pSession, szOne, &myint);
    ok(hr == S_OK, "Session_FeatureRequestStateGet failed, hresult 0x%08x\n", hr);
    ok(myint == INSTALLSTATE_ADVERTISED, "Feature request state was %d but expected %d\n", myint, INSTALLSTATE_ADVERTISED);

    /* Session::EvaluateCondition */
    hr = Session_EvaluateCondition(pSession, szOneActionFalse, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_FALSE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);

    hr = Session_EvaluateCondition(pSession, szOneActionTrue, &myint);
    ok(hr == S_OK, "Session_EvaluateCondition failed, hresult 0x%08x\n", hr);
    ok(myint == MSICONDITION_TRUE, "Feature current state was %d but expected %d\n", myint, INSTALLSTATE_UNKNOWN);
}

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    char name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyA(hkey, 0, name, sizeof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExA( hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp )))
        {
            ret = delete_key( tmp );
            RegCloseKey( tmp );
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA( hkey, "" );
    return 0;
}

static void test_Installer_RegistryValue(void)
{
    static const DWORD qw[2] = { 0x12345678, 0x87654321 };
    static const WCHAR szKey[] = { 'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','T','e','s','t',0 };
    static const WCHAR szOne[] = { 'O','n','e',0 };
    static const WCHAR szTwo[] = { 'T','w','o',0 };
    static const WCHAR szThree[] = { 'T','h','r','e','e',0 };
    static const WCHAR szREG_BINARY[] = { '(','R','E','G','_','B','I','N','A','R','Y',')',0 };
    static const WCHAR szFour[] = { 'F','o','u','r',0 };
    static const WCHAR szExpand[] = { '%','M','S','I','T','E','S','T','%',0 };
    static const WCHAR szFive[] = { 'F','i','v','e',0,'H','i',0,0 };
    static const WCHAR szFiveHi[] = { 'F','i','v','e','\n','H','i',0 };
    static const WCHAR szSix[] = { 'S','i','x',0 };
    static const WCHAR szREG_[] = { '(','R','E','G','_',']',0 };
    static const WCHAR szSeven[] = { 'S','e','v','e','n',0 };
    static const WCHAR szEight[] = { 'E','i','g','h','t',0 };
    static const WCHAR szBlank[] = { 0 };
    VARIANT varresult;
    VARIANTARG vararg;
    WCHAR szString[MAX_PATH];
    HKEY hkey, hkey_sub;
    HKEY curr_user = (HKEY)1;
    HRESULT hr;
    BOOL bRet;
    LONG lRet;

    /* Delete keys */
    SetLastError(0xdeadbeef);
    lRet = RegOpenKeyW( HKEY_CURRENT_USER, szKey, &hkey );
    if (!lRet && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Needed W-functions are not implemented\n");
        return;
    }
    if (!lRet)
        delete_key( hkey );

    /* Does our key exist? Shouldn't; check with all three possible value parameter types */
    hr = Installer_RegistryValueE(curr_user, szKey, &bRet);
    ok(hr == S_OK, "Installer_RegistryValueE failed, hresult 0x%08x\n", hr);
    ok(!bRet, "Registry key expected to not exist, but Installer_RegistryValue claims it does\n");

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, NULL, szString);
    ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, 0, szString, VT_BSTR);
    ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);

    /* Create key */
    ok(!RegCreateKeyW( HKEY_CURRENT_USER, szKey, &hkey ), "RegCreateKeyW failed\n");

    ok(!RegSetValueExW(hkey,szOne,0,REG_SZ, (const BYTE *)szOne, sizeof(szOne)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szTwo,0,REG_DWORD, (const BYTE *)qw, 4),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szThree,0,REG_BINARY, (const BYTE *)qw, 4),
        "RegSetValueExW failed\n");
    ok(SetEnvironmentVariableA("MSITEST", "Four"), "SetEnvironmentVariableA failed %d\n", GetLastError());
    ok(!RegSetValueExW(hkey,szFour,0,REG_EXPAND_SZ, (const BYTE *)szExpand, sizeof(szExpand)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szFive,0,REG_MULTI_SZ, (const BYTE *)szFive, sizeof(szFive)),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szSix,0,REG_QWORD, (const BYTE *)qw, 8),
        "RegSetValueExW failed\n");
    ok(!RegSetValueExW(hkey,szSeven,0,REG_NONE, NULL, 0),
        "RegSetValueExW failed\n");

    ok(!RegSetValueExW(hkey,NULL,0,REG_SZ, (const BYTE *)szOne, sizeof(szOne)),
        "RegSetValueExW failed\n");

    ok(!RegCreateKeyW( hkey, szEight, &hkey_sub ), "RegCreateKeyW failed\n");

    /* Does our key exist? It should, and make sure we retrieve the correct default value */
    bRet = FALSE;
    hr = Installer_RegistryValueE(curr_user, szKey, &bRet);
    ok(hr == S_OK, "Installer_RegistryValueE failed, hresult 0x%08x\n", hr);
    ok(bRet, "Registry key expected to exist, but Installer_RegistryValue claims it does not\n");

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, NULL, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2("Default registry value \"%s\" does not match expected \"%s\"\n", szString, szOne);

    /* Ask for the value of a nonexistent key */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szExpand, szString);
    ok(hr == DISP_E_BADINDEX, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);

    /* Get values of keys */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szOne, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szOne);

    VariantInit(&vararg);
    V_VT(&vararg) = VT_BSTR;
    V_BSTR(&vararg) = SysAllocString(szTwo);
    hr = Installer_RegistryValue(curr_user, szKey, vararg, &varresult, VT_I4);
    ok(hr == S_OK, "Installer_RegistryValue failed, hresult 0x%08x\n", hr);
    ok(V_I4(&varresult) == 305419896, "Registry value %d does not match expected value\n", V_I4(&varresult));
    VariantClear(&varresult);

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szThree, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szREG_BINARY);

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szFour, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szFour);

    /* Vista does not NULL-terminate this case */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szFive, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2n("Registry value \"%s\" does not match expected \"%s\"\n",
           szString, szFiveHi, lstrlenW(szFiveHi));

    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueW(curr_user, szKey, szSix, szString);
    ok(hr == S_OK, "Installer_RegistryValueW failed, hresult 0x%08x\n", hr);
    ok_w2("Registry value \"%s\" does not match expected \"%s\"\n", szString, szREG_);

    VariantInit(&vararg);
    V_VT(&vararg) = VT_BSTR;
    V_BSTR(&vararg) = SysAllocString(szSeven);
    hr = Installer_RegistryValue(curr_user, szKey, vararg, &varresult, VT_EMPTY);
    ok(hr == S_OK, "Installer_RegistryValue failed, hresult 0x%08x\n", hr);

    /* Get string class name for the key */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, 0, szString, VT_BSTR);
    ok(hr == S_OK, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
    ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szBlank);

    /* Get name of a value by positive number (RegEnumValue like), valid index */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, 2, szString, VT_BSTR);
    ok(hr == S_OK, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
    /* RegEnumValue order seems different on wine */
    todo_wine ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szTwo);

    /* Get name of a value by positive number (RegEnumValue like), invalid index */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, 10, szString, VT_EMPTY);
    ok(hr == S_OK, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);

    /* Get name of a subkey by negative number (RegEnumValue like), valid index */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, -1, szString, VT_BSTR);
    ok(hr == S_OK, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);
    ok_w2("Registry name \"%s\" does not match expected \"%s\"\n", szString, szEight);

    /* Get name of a subkey by negative number (RegEnumValue like), invalid index */
    memset(szString, 0, sizeof(szString));
    hr = Installer_RegistryValueI(curr_user, szKey, -10, szString, VT_EMPTY);
    ok(hr == S_OK, "Installer_RegistryValueI failed, hresult 0x%08x\n", hr);

    /* clean up */
    delete_key(hkey);
}

static void test_Installer_Products(BOOL bProductInstalled)
{
    WCHAR szString[MAX_PATH];
    HRESULT hr;
    int idx;
    IUnknown *pUnk = NULL;
    IEnumVARIANT *pEnum = NULL;
    VARIANT var;
    ULONG celt;
    int iCount, iValue;
    IDispatch *pStringList = NULL;
    BOOL bProductFound = FALSE;

    /* Installer::Products */
    hr = Installer_Products(&pStringList);
    ok(hr == S_OK, "Installer_Products failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        /* StringList::_NewEnum */
        hr = StringList__NewEnum(pStringList, &pUnk);
        ok(hr == S_OK, "StringList_NewEnum failed, hresult 0x%08x\n", hr);
        if (hr == S_OK)
        {
            hr = IUnknown_QueryInterface(pUnk, &IID_IEnumVARIANT, (void **)&pEnum);
            ok (hr == S_OK, "IUnknown::QueryInterface returned 0x%08x\n", hr);
        }
        if (!pEnum)
            skip("IEnumVARIANT tests\n");

        /* StringList::Count */
        hr = StringList_Count(pStringList, &iCount);
        ok(hr == S_OK, "StringList_Count failed, hresult 0x%08x\n", hr);

        for (idx=0; idx<iCount; idx++)
        {
            /* StringList::Item */
            memset(szString, 0, sizeof(szString));
            hr = StringList_Item(pStringList, idx, szString);
            ok(hr == S_OK, "StringList_Item failed (idx %d, count %d), hresult 0x%08x\n", idx, iCount, hr);

            if (hr == S_OK)
            {
                /* Installer::ProductState */
                hr = Installer_ProductState(szString, &iValue);
                ok(hr == S_OK, "Installer_ProductState failed, hresult 0x%08x\n", hr);
                if (hr == S_OK)
                    ok(iValue == INSTALLSTATE_DEFAULT || iValue == INSTALLSTATE_ADVERTISED, "Installer_ProductState returned %d, expected %d or %d\n", iValue, INSTALLSTATE_DEFAULT, INSTALLSTATE_ADVERTISED);

                /* Not found our product code yet? Check */
                if (!bProductFound && !lstrcmpW(szString, szProductCode))
                    bProductFound = TRUE;

                /* IEnumVARIANT::Next */
                if (pEnum)
                {
                    hr = IEnumVARIANT_Next(pEnum, 1, &var, &celt);
                    ok(hr == S_OK, "IEnumVARIANT_Next failed (idx %d, count %d), hresult 0x%08x\n", idx, iCount, hr);
                    ok(celt == 1, "%d items were retrieved, expected 1\n", celt);
                    ok(V_VT(&var) == VT_BSTR, "IEnumVARIANT_Next returned variant of type %d, expected %d\n", V_VT(&var), VT_BSTR);
                    ok_w2("%s returned by StringList_Item does not match %s returned by IEnumVARIANT_Next\n", szString, V_BSTR(&var));
                    VariantClear(&var);
                }
            }
        }

        if (bProductInstalled) todo_wine
        {
            ok(bProductInstalled == bProductFound, "Product expected to %s installed but product code was %s\n",
               bProductInstalled ? "be" : "not be",
               bProductFound ? "found" : "not found");
        }

        if (pEnum)
        {
            IEnumVARIANT *pEnum2 = NULL;

            if (0) /* Crashes on Windows XP */
            {
                /* IEnumVARIANT::Clone, NULL pointer */
                hr = IEnumVARIANT_Clone(pEnum, NULL);
            }

            /* IEnumVARIANT::Clone */
            hr = IEnumVARIANT_Clone(pEnum, &pEnum2);
            ok(hr == S_OK, "IEnumVARIANT_Clone failed, hresult 0x%08x\n", hr);
            if (hr == S_OK)
            {
                /* IEnumVARIANT::Clone is supposed to save the position, but it actually just goes back to the beginning */

                /* IEnumVARIANT::Next of the clone */
                if (iCount)
                {
                    hr = IEnumVARIANT_Next(pEnum2, 1, &var, &celt);
                    ok(hr == S_OK, "IEnumVARIANT_Next failed, hresult 0x%08x\n", hr);
                    ok(celt == 1, "%d items were retrieved, expected 0\n", celt);
                    ok(V_VT(&var) == VT_BSTR, "IEnumVARIANT_Next returned variant of type %d, expected %d\n", V_VT(&var), VT_BSTR);
                    VariantClear(&var);
                }
                else
                    skip("IEnumVARIANT::Next of clone will not return success with 0 products\n");

                IEnumVARIANT_Release(pEnum2);
            }

            /* IEnumVARIANT::Skip should fail */
            hr = IEnumVARIANT_Skip(pEnum, 1);
            ok(hr == S_FALSE, "IEnumVARIANT_Skip failed, hresult 0x%08x\n", hr);

            /* IEnumVARIANT::Next, NULL variant pointer */
            hr = IEnumVARIANT_Next(pEnum, 1, NULL, &celt);
            ok(hr == S_FALSE, "IEnumVARIANT_Next failed, hresult 0x%08x\n", hr);
            ok(celt == 0, "%d items were retrieved, expected 0\n", celt);

            /* IEnumVARIANT::Next, should not return any more items */
            hr = IEnumVARIANT_Next(pEnum, 1, &var, &celt);
            ok(hr == S_FALSE, "IEnumVARIANT_Next failed, hresult 0x%08x\n", hr);
            ok(celt == 0, "%d items were retrieved, expected 0\n", celt);
            VariantClear(&var);

            /* IEnumVARIANT::Reset */
            hr = IEnumVARIANT_Reset(pEnum);
            ok(hr == S_OK, "IEnumVARIANT_Reset failed, hresult 0x%08x\n", hr);

            if (iCount)
            {
                /* IEnumVARIANT::Skip to the last product */
                hr = IEnumVARIANT_Skip(pEnum, iCount-1);
                ok(hr == S_OK, "IEnumVARIANT_Skip failed, hresult 0x%08x\n", hr);

                /* IEnumVARIANT::Next should match the very last retrieved value, also makes sure it works with
                 * NULL celt pointer. */
                hr = IEnumVARIANT_Next(pEnum, 1, &var, NULL);
                ok(hr == S_OK, "IEnumVARIANT_Next failed (idx %d, count %d), hresult 0x%08x\n", idx, iCount, hr);
                ok(V_VT(&var) == VT_BSTR, "IEnumVARIANT_Next returned variant of type %d, expected %d\n", V_VT(&var), VT_BSTR);
                ok_w2("%s returned by StringList_Item does not match %s returned by IEnumVARIANT_Next\n", szString, V_BSTR(&var));
                VariantClear(&var);
            }
            else
                skip("IEnumVARIANT::Skip impossible for 0 products\n");
        }

        /* StringList::Item using an invalid index */
        memset(szString, 0, sizeof(szString));
        hr = StringList_Item(pStringList, iCount, szString);
        ok(hr == DISP_E_BADINDEX, "StringList_Item for an invalid index did not return DISP_E_BADINDEX, hresult 0x%08x\n", hr);

        if (pEnum) IEnumVARIANT_Release(pEnum);
        if (pUnk) IUnknown_Release(pUnk);
        IDispatch_Release(pStringList);
    }
}

/* Delete a registry subkey, including all its subkeys (RegDeleteKey does not work on keys with subkeys without
 * deleting the subkeys first) */
static UINT delete_registry_key(HKEY hkeyParent, LPCSTR subkey)
{
    UINT ret;
    CHAR *string = NULL;
    HKEY hkey;
    DWORD dwSize;

    ret = RegOpenKey(hkeyParent, subkey, &hkey);
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, &dwSize, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != ERROR_SUCCESS) return ret;
    if (!(string = HeapAlloc(GetProcessHeap(), 0, ++dwSize))) return ERROR_NOT_ENOUGH_MEMORY;

    while (RegEnumKeyA(hkey, 0, string, dwSize) == ERROR_SUCCESS)
        delete_registry_key(hkey, string);

    RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, string);
    RegDeleteKeyA(hkeyParent, subkey);
    return ERROR_SUCCESS;
}

/* Find a specific registry subkey at any depth within the given key and subkey and return its parent key. */
static UINT find_registry_key(HKEY hkeyParent, LPCSTR subkey, LPCSTR findkey, HKEY *phkey)
{
    UINT ret;
    CHAR *string = NULL;
    int idx = 0;
    HKEY hkey;
    DWORD dwSize;
    BOOL found = FALSE;

    *phkey = 0;

    ret = RegOpenKey(hkeyParent, subkey, &hkey);
    if (ret != ERROR_SUCCESS) return ret;
    ret = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, &dwSize, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ret != ERROR_SUCCESS) return ret;
    if (!(string = HeapAlloc(GetProcessHeap(), 0, ++dwSize))) return ERROR_NOT_ENOUGH_MEMORY;

    while (!found &&
           RegEnumKeyA(hkey, idx++, string, dwSize) == ERROR_SUCCESS)
    {
        if (!strcmp(string, findkey))
        {
            *phkey = hkey;
            found = TRUE;
        }
        else if (find_registry_key(hkey, string, findkey, phkey) == ERROR_SUCCESS) found = TRUE;
    }

    if (*phkey != hkey) RegCloseKey(hkey);
    HeapFree(GetProcessHeap(), 0, string);
    return (found ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND);
}

static void test_Installer_InstallProduct(void)
{
    HRESULT hr;
    CHAR path[MAX_PATH];
    WCHAR szString[MAX_PATH];
    LONG res;
    HKEY hkey;
    DWORD num, size, type;
    int iValue, iCount;
    IDispatch *pStringList = NULL;

    create_test_files();

    /* Installer::InstallProduct */
    hr = Installer_InstallProduct(szMsifile, NULL);
    if (hr == DISP_E_EXCEPTION)
    {
        skip("Installer object not supported.\n");
        delete_test_files();
        return;
    }
    ok(hr == S_OK, "Installer_InstallProduct failed, hresult 0x%08x\n", hr);

    /* Installer::ProductState for our product code, which has been installed */
    hr = Installer_ProductState(szProductCode, &iValue);
    ok(hr == S_OK, "Installer_ProductState failed, hresult 0x%08x\n", hr);
    ok(iValue == INSTALLSTATE_DEFAULT, "Installer_ProductState returned %d, expected %d\n", iValue, INSTALLSTATE_DEFAULT);

    /* Installer::ProductInfo for our product code */

    /* NULL attribute */
    memset(szString, 0, sizeof(szString));
    hr = Installer_ProductInfo(szProductCode, NULL, szString);
    ok(hr == DISP_E_EXCEPTION, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szProductInfoException);

    /* Nonexistent attribute */
    memset(szString, 0, sizeof(szString));
    hr = Installer_ProductInfo(szProductCode, szMsifile, szString);
    ok(hr == DISP_E_EXCEPTION, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szProductInfoException);

    /* Package name */
    memset(szString, 0, sizeof(szString));
    hr = Installer_ProductInfo(szProductCode, WINE_INSTALLPROPERTY_PACKAGENAMEW, szString);
    ok(hr == S_OK, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    todo_wine ok_w2("Installer_ProductInfo returned %s but expected %s\n", szString, szMsifile);

    /* Product name */
    memset(szString, 0, sizeof(szString));
    hr = Installer_ProductInfo(szProductCode, WINE_INSTALLPROPERTY_PRODUCTNAMEW, szString);
    ok(hr == S_OK, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    todo_wine ok_w2("Installer_ProductInfo returned %s but expected %s\n", szString, szMSITEST);

    /* Installer::Products */
    test_Installer_Products(TRUE);

    /* Installer::RelatedProducts for our upgrade code */
    hr = Installer_RelatedProducts(szUpgradeCode, &pStringList);
    ok(hr == S_OK, "Installer_RelatedProducts failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        /* StringList::Count */
        hr = StringList_Count(pStringList, &iCount);
        ok(hr == S_OK, "StringList_Count failed, hresult 0x%08x\n", hr);
        ok(iCount == 1, "Expected one related product but found %d\n", iCount);

        /* StringList::Item */
        memset(szString, 0, sizeof(szString));
        hr = StringList_Item(pStringList, 0, szString);
        ok(hr == S_OK, "StringList_Item failed (idx 0, count %d), hresult 0x%08x\n", iCount, hr);
        ok_w2("StringList_Item returned %s but expected %s\n", szString, szProductCode);

        IDispatch_Release(pStringList);
    }

    /* Check & clean up installed files & registry keys */
    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "Name", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(path, "imaname"), "Expected imaname, got %s\n", path);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "blah", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    size = sizeof(num);
    type = REG_DWORD;
    res = RegQueryValueExA(hkey, "number", NULL, &type, (LPBYTE)&num, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(num == 314, "Expected 314, got %d\n", num);

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "OrderTestName", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(path, "OrderTestValue"), "Expected imaname, got %s\n", path);

    RegCloseKey(hkey);

    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest");
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Remove registry keys written by RegisterProduct standard action */
    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}");
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UpgradeCodes\\D8E760ECA1E276347B43E42BDBDA5656");
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = find_registry_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData", "05FA3C1F65B896A40AC00077F34EF203", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    if (res == ERROR_SUCCESS)
    {
        res = delete_registry_key(hkey, "05FA3C1F65B896A40AC00077F34EF203");
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        RegCloseKey(hkey);
    }

    /* Remove registry keys written by PublishProduct standard action */
    res = RegOpenKey(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Installer", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = delete_registry_key(hkey, "Products\\05FA3C1F65B896A40AC00077F34EF203");
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegDeleteKeyA(hkey, "UpgradeCodes\\D8E760ECA1E276347B43E42BDBDA5656");
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    RegCloseKey(hkey);

    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products\\05FA3C1F65B896A40AC00077F34EF203");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* Delete installation files we installed */
    delete_test_files();
}

static void test_Installer(void)
{
    static WCHAR szBackslash[] = { '\\',0 };
    static WCHAR szCreateRecordException[] = { 'C','r','e','a','t','e','R','e','c','o','r','d',',','C','o','u','n','t',0 };
    static WCHAR szIntegerDataException[] = { 'I','n','t','e','g','e','r','D','a','t','a',',','F','i','e','l','d',0 };
    WCHAR szPath[MAX_PATH];
    HRESULT hr;
    UINT len;
    IDispatch *pSession = NULL, *pDatabase = NULL, *pRecord = NULL, *pStringList = NULL;
    int iValue, iCount;

    if (!pInstaller) return;

    /* Installer::CreateRecord */

    /* Test for error */
    hr = Installer_CreateRecord(-1, &pRecord);
    ok(hr == DISP_E_EXCEPTION, "Installer_CreateRecord failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szCreateRecordException);

    /* Test for success */
    hr = Installer_CreateRecord(1, &pRecord);
    ok(hr == S_OK, "Installer_CreateRecord failed, hresult 0x%08x\n", hr);
    ok(pRecord != NULL, "Installer_CreateRecord should not have returned NULL record\n");
    if (pRecord)
    {
        /* Record::FieldCountGet */
        hr = Record_FieldCountGet(pRecord, &iValue);
        ok(hr == S_OK, "Record_FiledCountGet failed, hresult 0x%08x\n", hr);
        ok(iValue == 1, "Record_FieldCountGet result was %d but expected 1\n", iValue);

        /* Record::IntegerDataGet */
        hr = Record_IntegerDataGet(pRecord, 1, &iValue);
        ok(hr == S_OK, "Record_IntegerDataGet failed, hresult 0x%08x\n", hr);
        ok(iValue == MSI_NULL_INTEGER, "Record_IntegerDataGet result was %d but expected %d\n", iValue, MSI_NULL_INTEGER);

        /* Record::IntegerDataGet, bad index */
        hr = Record_IntegerDataGet(pRecord, 10, &iValue);
        ok(hr == S_OK, "Record_IntegerDataGet failed, hresult 0x%08x\n", hr);
        ok(iValue == MSI_NULL_INTEGER, "Record_IntegerDataGet result was %d but expected %d\n", iValue, MSI_NULL_INTEGER);

        /* Record::IntegerDataPut */
        hr = Record_IntegerDataPut(pRecord, 1, 100);
        ok(hr == S_OK, "Record_IntegerDataPut failed, hresult 0x%08x\n", hr);

        /* Record::IntegerDataPut, bad index */
        hr = Record_IntegerDataPut(pRecord, 10, 100);
        ok(hr == DISP_E_EXCEPTION, "Record_IntegerDataPut failed, hresult 0x%08x\n", hr);
        ok_exception(hr, szIntegerDataException);

        /* Record::IntegerDataGet */
        hr = Record_IntegerDataGet(pRecord, 1, &iValue);
        ok(hr == S_OK, "Record_IntegerDataGet failed, hresult 0x%08x\n", hr);
        ok(iValue == 100, "Record_IntegerDataGet result was %d but expected 100\n", iValue);

        IDispatch_Release(pRecord);
    }

    /* Prepare package */
    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table),
                    summary_info, sizeof(summary_info) / sizeof(msi_summary_info));

    len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, CURR_DIR, -1, szPath, MAX_PATH);
    ok(len, "MultiByteToWideChar returned error %d\n", GetLastError());
    if (!len) return;

    lstrcatW(szPath, szBackslash);
    lstrcatW(szPath, szMsifile);

    /* Installer::OpenPackage */
    hr = Installer_OpenPackage(szPath, 0, &pSession);
    ok(hr == S_OK, "Installer_OpenPackage failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        test_Session(pSession);
        IDispatch_Release(pSession);
    }

    /* Installer::OpenDatabase */
    hr = Installer_OpenDatabase(szPath, (int)MSIDBOPEN_TRANSACT, &pDatabase);
    ok(hr == S_OK, "Installer_OpenDatabase failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        test_Database(pDatabase, FALSE);
        IDispatch_Release(pDatabase);
    }

    /* Installer::RegistryValue */
    test_Installer_RegistryValue();

    /* Installer::ProductState for our product code, which should not be installed */
    hr = Installer_ProductState(szProductCode, &iValue);
    ok(hr == S_OK, "Installer_ProductState failed, hresult 0x%08x\n", hr);
    ok(iValue == INSTALLSTATE_UNKNOWN, "Installer_ProductState returned %d, expected %d\n", iValue, INSTALLSTATE_UNKNOWN);

    /* Installer::ProductInfo for our product code, which should not be installed */

    /* Package name */
    memset(szPath, 0, sizeof(szPath));
    hr = Installer_ProductInfo(szProductCode, WINE_INSTALLPROPERTY_PACKAGENAMEW, szPath);
    ok(hr == DISP_E_EXCEPTION, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szProductInfoException);

    /* NULL attribute and NULL product code */
    memset(szPath, 0, sizeof(szPath));
    hr = Installer_ProductInfo(NULL, NULL, szPath);
    ok(hr == DISP_E_EXCEPTION, "Installer_ProductInfo failed, hresult 0x%08x\n", hr);
    ok_exception(hr, szProductInfoException);

    /* Installer::Products */
    test_Installer_Products(FALSE);

    /* Installer::RelatedProducts for our upgrade code, should not find anything */
    hr = Installer_RelatedProducts(szUpgradeCode, &pStringList);
    ok(hr == S_OK, "Installer_RelatedProducts failed, hresult 0x%08x\n", hr);
    if (hr == S_OK)
    {
        /* StringList::Count */
        hr = StringList_Count(pStringList, &iCount);
        ok(hr == S_OK, "StringList_Count failed, hresult 0x%08x\n", hr);
        ok(!iCount, "Expected no related products but found %d\n", iCount);

        IDispatch_Release(pStringList);
    }

    /* Installer::Version */
    memset(szPath, 0, sizeof(szPath));
    hr = Installer_VersionGet(szPath);
    ok(hr == S_OK, "Installer_VersionGet failed, hresult 0x%08x\n", hr);

    /* Installer::InstallProduct and other tests that depend on our product being installed */
    test_Installer_InstallProduct();
}

START_TEST(automation)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    HRESULT hr;
    CLSID clsid;
    IUnknown *pUnk;

    GetSystemTimeAsFileTime(&systemtime);

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPath(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    get_program_files_dir(PROG_FILES_DIR);

    hr = OleInitialize(NULL);
    ok (hr == S_OK, "OleInitialize returned 0x%08x\n", hr);
    hr = CLSIDFromProgID(szProgId, &clsid);
    ok (hr == S_OK, "CLSIDFromProgID returned 0x%08x\n", hr);
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "CoCreateInstance returned 0x%08x\n", hr);

    if (pUnk)
    {
        hr = IUnknown_QueryInterface(pUnk, &IID_IDispatch, (void **)&pInstaller);
        ok (hr == S_OK, "IUnknown::QueryInterface returned 0x%08x\n", hr);

        test_dispid();
        test_dispatch();
        test_Installer();

        IDispatch_Release(pInstaller);
        IUnknown_Release(pUnk);
    }

    OleUninitialize();

    SetCurrentDirectoryA(prev_path);
}
