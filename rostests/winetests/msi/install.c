/*
 * Copyright (C) 2006 James Hawkins
 *
 * A test program for installing MSI products.
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

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>

#include "wine/test.h"

static const char *msifile = "winetest.msi";
CHAR CURR_DIR[MAX_PATH];
CHAR PROG_FILES_DIR[MAX_PATH];

/* msi database data */

static const CHAR admin_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                         "s72\tS255\tI2\n"
                                         "AdminExecuteSequence\tAction\n"
                                         "CostFinalize\t\t1000\n"
                                         "CostInitialize\t\t800\n"
                                         "FileCost\t\t900\n"
                                         "InstallAdminPackage\t\t3900\n"
                                         "InstallFiles\t\t4000\n"
                                         "InstallFinalize\t\t6600\n"
                                         "InstallInitialize\t\t1500\n"
                                         "InstallValidate\t\t1400";

static const CHAR advt_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                        "s72\tS255\tI2\n"
                                        "AdvtExecuteSequence\tAction\n"
                                        "CostFinalize\t\t1000\n"
                                        "CostInitialize\t\t800\n"
                                        "CreateShortcuts\t\t4500\n"
                                        "InstallFinalize\t\t6600\n"
                                        "InstallInitialize\t\t1500\n"
                                        "InstallValidate\t\t1400\n"
                                        "PublishComponents\t\t6200\n"
                                        "PublishFeatures\t\t6300\n"
                                        "PublishProduct\t\t6400\n"
                                        "RegisterClassInfo\t\t4600\n"
                                        "RegisterExtensionInfo\t\t4700\n"
                                        "RegisterMIMEInfo\t\t4900\n"
                                        "RegisterProgIdInfo\t\t4800";

static const CHAR component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                    "s72\tS38\ts72\ti2\tS255\tS72\n"
                                    "Component\tComponent\n"
                                    "Five\t{8CC92E9D-14B2-4CA4-B2AA-B11D02078087}\tNEWDIR\t2\t\tfive.txt\n"
                                    "Four\t{FD37B4EA-7209-45C0-8917-535F35A2F080}\tCABOUTDIR\t2\t\tfour.txt\n"
                                    "One\t{783B242E-E185-4A56-AF86-C09815EC053C}\tMSITESTDIR\t2\t\tone.txt\n"
                                    "Three\t{010B6ADD-B27D-4EDD-9B3D-34C4F7D61684}\tCHANGEDDIR\t2\t\tthree.txt\n"
                                    "Two\t{BF03D1A6-20DA-4A65-82F3-6CAC995915CE}\tFIRSTDIR\t2\t\ttwo.txt\n"
                                    "dangler\t{6091DF25-EF96-45F1-B8E9-A9B1420C7A3C}\tTARGETDIR\t4\t\tregdata";

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
                                  "Three\t\tThree\tThe Three Feature\t3\t3\tCHANGEDDIR\t0\n"
                                  "Two\t\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0";

static const CHAR feature_comp_dat[] = "Feature_\tComponent_\n"
                                       "s38\ts72\n"
                                       "FeatureComponents\tFeature_\tComponent_\n"
                                       "Five\tFive\n"
                                       "Four\tFour\n"
                                       "One\tOne\n"
                                       "Three\tThree\n"
                                       "Two\tTwo";

static const CHAR file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                               "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                               "File\tFile\n"
                               "five.txt\tFive\tfive.txt\t1000\t\t\t16384\t5\n"
                               "four.txt\tFour\tfour.txt\t1000\t\t\t16384\t4\n"
                               "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n"
                               "three.txt\tThree\tthree.txt\t1000\t\t\t0\t3\n"
                               "two.txt\tTwo\ttwo.txt\t1000\t\t\t0\t2";

static const CHAR install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallExecuteSequence\tAction\n"
                                           "AllocateRegistrySpace\tNOT Installed\t1550\n"
                                           "CostFinalize\t\t1000\n"
                                           "CostInitialize\t\t800\n"
                                           "FileCost\t\t900\n"
                                           "InstallFiles\t\t4000\n"
                                           "InstallFinalize\t\t6600\n"
                                           "InstallInitialize\t\t1500\n"
                                           "InstallValidate\t\t1400\n"
                                           "LaunchConditions\t\t100\n"
                                           "WriteRegistryValues\t\t5000";

static const CHAR media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                "i2\ti4\tL64\tS255\tS32\tS72\n"
                                "Media\tDiskId\n"
                                "1\t3\t\t\tDISK1\t\n"
                                "2\t5\t\tmsitest.cab\tDISK2\t\n";

static const CHAR property_dat[] = "Property\tValue\n"
                                   "s72\tl0\n"
                                   "Property\tProperty\n"
                                   "DefaultUIFont\tDlgFont8\n"
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
                                   "regdata\t2\tSOFTWARE\\Wine\\msitest\tblah\tbad\tdangler";

typedef struct _msi_table
{
    const CHAR *filename;
    const CHAR *data;
    int size;
} msi_table;

#define ADD_TABLE(x) {#x".idt", x##_dat, sizeof(x##_dat)}

static const msi_table tables[] =
{
    ADD_TABLE(admin_exec_seq),
    ADD_TABLE(advt_exec_seq),
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

/* cabinet definitions */

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          999999999
#define FOLDER_THRESHOLD    900000

/* The following defintions were copied from dlls/cabinet/cabinet.h
 * because they are undocumented in windows.
 */

/* EXTRACTdest flags */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

struct ExtractFileList {
    LPSTR  filename;
    struct ExtractFileList *next;
    BOOL   unknown;  /* always 1L */
};

/* the first parameter of the function extract */
typedef struct {
    long   result1;          /* 0x000 */
    long   unknown1[3];      /* 0x004 */
    struct ExtractFileList *filelist; /* 0x010 */
    long   filecount;        /* 0x014 */
    long   flags;            /* 0x018 */
    char   directory[0x104]; /* 0x01c */
    char   lastfile[0x20c];  /* 0x120 */
} EXTRACTDEST;

/* cabinet function pointers */
HMODULE hCabinet;
static HRESULT (WINAPI *pExtract)(EXTRACTDEST*, LPCSTR);

/* the FCI callbacks */

static void *mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    return TRUE;
}

static long progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int file_placed(PCCAB pccab, char *pszFile, long cbFile,
                       BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    
    dwAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;
    
    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static long fci_seek(INT_PTR hf, long dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;
    
    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static BOOL check_record(MSIHANDLE rec, UINT field, LPCSTR val)
{
    CHAR buffer[0x20];
    UINT r;
    DWORD sz;

    sz = sizeof buffer;
    r = MsiRecordGetString(rec, field, buffer, &sz);
    return (r == ERROR_SUCCESS ) && !strcmp(val, buffer);
}

static BOOL get_temp_file(char *pszTempName, int cbTempName, void *pv)
{
    LPSTR tempname;

    tempname = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    GetTempFileNameA(".", "xx", 0, tempname);

    if (tempname && (strlen(tempname) < (unsigned)cbTempName))
    {
        lstrcpyA(pszTempName, tempname);
        HeapFree(GetProcessHeap(), 0, tempname);
        return TRUE;
    }

    HeapFree(GetProcessHeap(), 0, tempname);

    return FALSE;
}

static INT_PTR get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
                             USHORT *pattribs, int *err, void *pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME filetime;
    HANDLE handle;
    DWORD attrs;
    BOOL res;

    handle = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszName);

    res = GetFileInformationByHandle(handle, &finfo);
    ok(res, "Expected GetFileInformationByHandle to succeed\n");
   
    FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
    FileTimeToDosDateTime(&filetime, pdate, ptime);

    attrs = GetFileAttributes(pszName);
    ok(attrs != INVALID_FILE_ATTRIBUTES, "Failed to GetFileAttributes\n");

    return (INT_PTR)handle;
}

static void add_file(HFCI hfci, char *file)
{
    char path[MAX_PATH];
    BOOL res;

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    res = FCIAddFile(hfci, path, file, FALSE, get_next_cabinet, progress,
                     get_open_info, tcompTYPE_MSZIP);
    ok(res, "Expected FCIAddFile to succeed\n");
}

static void set_cab_parameters(PCCAB pCabParams, const CHAR *name)
{
    ZeroMemory(pCabParams, sizeof(CCAB));

    pCabParams->cb = MEDIA_SIZE;
    pCabParams->cbFolderThresh = FOLDER_THRESHOLD;
    pCabParams->setID = 0xbeef;
    lstrcpyA(pCabParams->szCabPath, CURR_DIR);
    lstrcatA(pCabParams->szCabPath, "\\");
    lstrcpyA(pCabParams->szCab, name);
}

static void create_cab_file(const CHAR *name)
{
    CCAB cabParams;
    HFCI hfci;
    ERF erf;
    static CHAR four_txt[] = "four.txt",
                five_txt[] = "five.txt";
    BOOL res;

    set_cab_parameters(&cabParams, name);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    add_file(hfci, four_txt);
    add_file(hfci, five_txt);

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static BOOL init_function_pointers(void)
{
    hCabinet = LoadLibraryA("cabinet.dll");
    if (!hCabinet)
        return FALSE;

    pExtract = (void *)GetProcAddress(hCabinet, "Extract");
    if (!pExtract)
        return FALSE;

    return TRUE;
}

static BOOL get_program_files_dir(LPSTR buf)
{
    HKEY hkey;
    CHAR temp[MAX_PATH];
    DWORD type = REG_EXPAND_SZ, size;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   "Software\\Microsoft\\Windows\\CurrentVersion", &hkey))
        return FALSE;

    size = MAX_PATH;
    if (RegQueryValueEx(hkey, "ProgramFilesPath", 0, &type, (LPBYTE)temp, &size))
        return FALSE;

    ExpandEnvironmentStrings(temp, buf, MAX_PATH);

    RegCloseKey(hkey);
    return TRUE;
}

static void create_file(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void create_test_files(void)
{
    int len;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len-1] == '\\'))
        CURR_DIR[len - 1] = 0;

    get_program_files_dir(PROG_FILES_DIR);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\one.txt");
    CreateDirectoryA("msitest\\first", NULL);
    create_file("msitest\\first\\two.txt");
    CreateDirectoryA("msitest\\second", NULL);
    create_file("msitest\\second\\three.txt");

    create_file("four.txt");
    create_file("five.txt");
    create_cab_file("msitest.cab");

    DeleteFileA("four.txt");
    DeleteFileA("five.txt");
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
    DeleteFileA("msitest.msi");
    DeleteFileA("msitest.cab");
    DeleteFileA("msitest\\second\\three.txt");
    DeleteFileA("msitest\\first\\two.txt");
    DeleteFileA("msitest\\one.txt");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

static void write_file(const CHAR *filename, const char *data, int data_size)
{
    DWORD size;

    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static void write_msi_summary_info(MSIHANDLE db)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(db, NULL, 4, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_TEMPLATE, VT_LPSTR, 0, NULL, ";1033");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_REVNUMBER, VT_LPSTR, 0, NULL,
                                   "{004757CA-5092-49c2-AD20-28E1CE0DF5F2}");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_PAGECOUNT, VT_I4, 100, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_WORDCOUNT, VT_I4, 0, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* write the summary changes back to the stream */
    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void create_database(const CHAR *name, const msi_table *tables, int num_tables)
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
        todo_wine
        {
            ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        }

        DeleteFileA(table->filename);
    }

    write_msi_summary_info(db);

    r = MsiDatabaseCommit(db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(db);
}

static void test_MsiInstallProduct(void)
{
    UINT r;
    CHAR path[MAX_PATH];
    LONG res;
    HKEY hkey;
    DWORD num, size, type;

    r = MsiInstallProductA(msifile, NULL);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    }

    todo_wine
    {
        ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\cabout\\new", FALSE), "File not installed\n");
        ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\cabout", FALSE), "File not installed\n");
        ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\changed", FALSE), "File not installed\n");
        ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\first", FALSE), "File not installed\n");
        ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }

    res = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest", &hkey);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "Name", NULL, &type, (LPBYTE)path, &size);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
        ok(!lstrcmpA(path, "imaname"), "Expected imaname, got %s\n", path);
    }

    size = MAX_PATH;
    type = REG_SZ;
    res = RegQueryValueExA(hkey, "blah", NULL, &type, (LPBYTE)path, &size);
    todo_wine
    {
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
    }

    size = sizeof(num);
    type = REG_DWORD;
    res = RegQueryValueExA(hkey, "number", NULL, &type, (LPBYTE)&num, &size);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
        ok(num == 314, "Expected 314, got %ld\n", num);
    }

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest");
}

static void test_MsiSetComponentState(void)
{
    MSIHANDLE package;
    char path[MAX_PATH];
    UINT r;

    CoInitialize(NULL);

    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    r = MsiOpenPackage(path, &package);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "CostInitialize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "FileCost");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiDoAction(package, "CostFinalize");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSetComponentState(package, "dangler", INSTALLSTATE_SOURCE);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    }

    MsiCloseHandle(package);
    CoUninitialize();
}

static void test_packagecoltypes(void)
{
    MSIHANDLE hdb, view, rec;
    char path[MAX_PATH];
    LPCSTR query;
    UINT r, count;

    CoInitialize(NULL);

    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    r = MsiOpenDatabase(path, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    query = "SELECT * FROM `Media`";
    r = MsiDatabaseOpenView( hdb, query, &view );
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    }

    r = MsiViewGetColumnInfo( view, MSICOLINFO_NAMES, &rec );
    count = MsiRecordGetFieldCount( rec );
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
        ok(count == 6, "Expected 6, got %d\n", count);
        ok(check_record(rec, 1, "DiskId"), "wrong column label\n");
        ok(check_record(rec, 2, "LastSequence"), "wrong column label\n");
        ok(check_record(rec, 3, "DiskPrompt"), "wrong column label\n");
        ok(check_record(rec, 4, "Cabinet"), "wrong column label\n");
        ok(check_record(rec, 5, "VolumeLabel"), "wrong column label\n");
        ok(check_record(rec, 6, "Source"), "wrong column label\n");
    }

    r = MsiViewGetColumnInfo( view, MSICOLINFO_TYPES, &rec );
    count = MsiRecordGetFieldCount( rec );
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
        ok(count == 6, "Expected 6, got %d\n", count);
        ok(check_record(rec, 1, "i2"), "wrong column label\n");
        ok(check_record(rec, 2, "i4"), "wrong column label\n");
        ok(check_record(rec, 3, "L64"), "wrong column label\n");
        ok(check_record(rec, 4, "S255"), "wrong column label\n");
        ok(check_record(rec, 5, "S32"), "wrong column label\n");
        ok(check_record(rec, 6, "S72"), "wrong column label\n");
    }

    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

START_TEST(install)
{
    if (!init_function_pointers())
        return;

    create_test_files();
    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));
    
    test_MsiInstallProduct();
    test_MsiSetComponentState();
    test_packagecoltypes();
    
    delete_test_files();
}
