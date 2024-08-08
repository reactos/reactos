/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#define _WIN32_MSI 300
#define COBJMACROS

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <msidefs.h>
#include <sddl.h>
#include <fci.h>
#include <shellapi.h>
#include <objidl.h>

#include "wine/test.h"
#include "utils.h"

#define GUID_SIZE (39)
#define SQUASHED_GUID_SIZE (33)

static BOOL is_wow64;
static const char msifile[] = "winetest.msi";
static const WCHAR msifileW[] = L"winetest.msi";

/* cabinet definitions */

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          0x7FFFFFFF
#define FOLDER_THRESHOLD    900000

static BOOL add_cabinet_storage(LPCSTR db, LPCSTR cabinet)
{
    WCHAR dbW[MAX_PATH], cabinetW[MAX_PATH];
    IStorage *stg;
    IStream *stm;
    HRESULT hr;
    HANDLE handle;

    MultiByteToWideChar(CP_ACP, 0, db, -1, dbW, MAX_PATH);
    hr = StgOpenStorage(dbW, NULL, STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    if (FAILED(hr))
        return FALSE;

    MultiByteToWideChar(CP_ACP, 0, cabinet, -1, cabinetW, MAX_PATH);
    hr = IStorage_CreateStream(stg, cabinetW, STGM_WRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    if (FAILED(hr))
    {
        IStorage_Release(stg);
        return FALSE;
    }

    handle = CreateFileW(cabinetW, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        DWORD count;
        char buffer[1024];
        if (ReadFile(handle, buffer, sizeof(buffer), &count, NULL))
            IStream_Write(stm, buffer, count, &count);
        CloseHandle(handle);
    }

    IStream_Release(stm);
    IStorage_Release(stg);

    return TRUE;
}

/* msi database data */

static const char directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "One\t{8F5BAEEF-DD92-40AC-9397-BE3CF9F97C81}\tMSITESTDIR\t2\tNOT REINSTALL\tone.txt\n";

static const char feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "One\t\tOne\tOne\t1\t3\tMSITESTDIR\t0\n"
    "Two\t\t\t\t2\t1\tTARGETDIR\t0\n";

static const char feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "One\tOne\n";

static const char file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n";

static const char install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "ValidateProductID\t\t700\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "UnpublishFeatures\t\t1800\n"
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6300\n"
    "PublishProduct\t\t6400\n"
    "InstallFinalize\t\t6600";

static const char media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\t\tDISK1\t\n";

static const char property_dat[] =
    "Property\tValue\n"
    "s72\tl0\n"
    "Property\tProperty\n"
    "INSTALLLEVEL\t3\n"
    "Manufacturer\tWine\n"
    "ProductCode\t{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}\n"
    "ProductName\tMSITEST\n"
    "ProductVersion\t1.1.1\n"
    "UpgradeCode\t{9574448F-9B86-4E07-B6F6-8D199DA12127}\n"
    "MSIFASTINSTALL\t1\n";

static const char ci2_property_dat[] =
    "Property\tValue\n"
    "s72\tl0\n"
    "Property\tProperty\n"
    "INSTALLLEVEL\t3\n"
    "Manufacturer\tWine\n"
    "ProductCode\t{FF4AFE9C-6AC2-44F9-A060-9EA6BD16C75E}\n"
    "ProductName\tMSITEST2\n"
    "ProductVersion\t1.1.1\n"
    "UpgradeCode\t{6B60C3CA-B8CA-4FB7-A395-092D98FF5D2A}\n"
    "MSIFASTINSTALL\t1\n";

static const char mcp_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "hydrogen\t{C844BD1E-1907-4C00-8BC9-150BD70DF0A1}\tMSITESTDIR\t2\t\thydrogen\n"
    "helium\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t2\t\thelium\n"
    "lithium\t{4AF28FFC-71C7-4307-BDE4-B77C5338F56F}\tMSITESTDIR\t2\tPROPVAR=42\tlithium\n";

static const char mcp_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "hydroxyl\t\thydroxyl\thydroxyl\t2\t1\tTARGETDIR\t0\n"
    "heliox\t\theliox\theliox\t2\t5\tTARGETDIR\t0\n"
    "lithia\t\tlithia\tlithia\t2\t10\tTARGETDIR\t0";

static const char mcp_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "hydroxyl\thydrogen\n"
    "heliox\thelium\n"
    "lithia\tlithium";

static const char mcp_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "hydrogen\thydrogen\thydrogen\t0\t\t\t8192\t1\n"
    "helium\thelium\thelium\t0\t\t\t8192\t1\n"
    "lithium\tlithium\tlithium\t0\t\t\t8192\t1";

static const char lus_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\tUILevel=5\tmaximus\n";

static const char lus_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "feature\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0\n"
    "montecristo\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0";

static const char lus_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1";

static const char lus_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\tmaximus\n"
    "montecristo\tmaximus";

static const char lus_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "ValidateProductID\t\t700\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\tPROCESS_COMPONENTS=1 Or FULL=1\t1600\n"
    "UnpublishFeatures\tUNPUBLISH_FEATURES=1 Or FULL=1\t1800\n"
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterUser\tREGISTER_USER=1 Or FULL=1\t6000\n"
    "RegisterProduct\tREGISTER_PRODUCT=1 Or FULL=1\t6100\n"
    "PublishFeatures\tPUBLISH_FEATURES=1 Or FULL=1\t6300\n"
    "PublishProduct\tPUBLISH_PRODUCT=1 Or FULL=1\t6400\n"
    "InstallFinalize\t\t6600";

static const char lus0_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\t\tDISK1\t\n";

static const char lus1_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\ttest1.cab\tDISK1\t\n";

static const char lus2_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\t#test1.cab\tDISK1\t\n";

static const char spf_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "SetFolderProp\t51\tMSITESTDIR\t[ProgramFilesFolder]\\msitest\\added\t\n"
    "SetFolderProp2\t51\tMSITESTDIR\t[ProgramFilesFolder]\\msitest\\added\\added2\t\n";

static const char spf_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "SetFolderProp\t\t950\n"
    "SetFolderProp2\t\t960\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "InstallFiles\t\t4000\n"
    "InstallServices\t\t5000\n"
    "InstallFinalize\t\t6600\n";

static const char spf_install_ui_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallUISequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "ExecuteAction\t\t1100\n";

static const char spf_directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "PARENTDIR\tTARGETDIR\tparent\n"
    "CHILDDIR\tPARENTDIR\tchild\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char spf_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\tUILevel=5\tmaximus\n";

static const char spf2_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "FormatParentFolderCheck\t\t910\n"
    "FormatChildFolderCheck\t\t920\n"
    "CheckParentFolder\tNOT PARENTDIR=PARENTDIRCHECK\t930\n"
    "CheckChildFolder\tNOT CHILDDIR=CHILDDIRCHECK\t940\n"
    "FormatParentFolderCheck2\t\t945\n"
    "SetParentFolder\t\t950\n"
    "CheckParentFolder2\tNOT PARENTDIR=PARENTDIRCHECK\t960\n"
    "CheckChildFolder2\tNOT CHILDDIR=CHILDDIRCHECK\t970\n"
    "CostFinalize\t\t1000\n"
    "FormatParentFolderCheck3\t\t1005\n"
    "CheckParentFolder3\tNOT PARENTDIR=PARENTDIRCHECK\t1010\n"
    "CheckChildFolder3\tNOT CHILDDIR=CHILDDIRCHECK\t1020\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "InstallFiles\t\t4000\n"
    "CreateShortcuts\t\t4100\n"
    "InstallFinalize\t\t6600\n";

static const char spf2_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "FormatParentFolderCheck\t51\tPARENTDIRCHECK\t[TARGETDIR]parent\\\t\n"
    "FormatChildFolderCheck\t51\tCHILDDIRCHECK\t[TARGETDIR]parent\\child\\\t\n"
    "CheckParentFolder\t19\tPARENTDIR\tparent prop wrong before set: [PARENTDIR]\t\n"
    "CheckChildFolder\t19\tCHILDDIR\tchild prop wrong before set: [CHILDDIR]\t\n"
    "FormatParentFolderCheck2\t51\tPARENTDIRCHECK\t[ProgramFilesFolder]msitest\\parent\t\n"
    "SetParentFolder\t51\tPARENTDIR\t[PARENTDIRCHECK]\t\n"
    "CheckParentFolder2\t19\tPARENTDIR\tparent prop wrong after set: [PARENTDIR]\t\n"
    "CheckChildFolder2\t19\tCHILDDIR\tchild prop wrong after set: [CHILDDIR]\t\n"
    "FormatParentFolderCheck3\t51\tPARENTDIRCHECK\t[ProgramFilesFolder]msitest\\parent\\\t\n"
    "CheckParentFolder3\t19\tPARENTDIR\tparent prop wrong after CostFinalize: [PARENTDIR]\t\n"
    "CheckChildFolder3\t19\tCHILDDIR\tchild prop wrong after CostFinalize: [CHILDDIR]\t\n";

static const char shortcut_dat[] =
    "Shortcut\tDirectory_\tName\tComponent_\tTarget\tArguments\tDescription\tHotkey\tIcon_\tIconIndex\tShowCmd\tWkDir\n"
    "s72\ts72\tl128\ts72\ts72\tS255\tL255\tI2\tS72\tI2\tI2\tS72\n"
    "Shortcut\tShortcut\n"
    "Shortcut\tCHILDDIR\tShortcut\tmaximus\t[#maximus]\t\tShortcut\t\t\t\t\tMSITESTDIR\n";

static const char sd_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "sourcedir.txt\tsourcedir\tsourcedir.txt\t1000\t\t\t8192\t1\n";

static const char sd_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "sourcedir\t\t\tsourcedir feature\t1\t2\tMSITESTDIR\t0\n";

static const char sd_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "sourcedir\tsourcedir\n";

static const char sd_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "sourcedir\t{DD422F92-3ED8-49B5-A0B7-F266F98357DF}\tMSITESTDIR\t0\t\tsourcedir.txt\n";

static const char sd_install_ui_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallUISequence\tAction\n"
    "TestSourceDirProp1\tnot SourceDir and not SOURCEDIR and not Installed\t99\n"
    "AppSearch\t\t100\n"
    "TestSourceDirProp2\tnot SourceDir and not SOURCEDIR and not Installed\t101\n"
    "LaunchConditions\tnot Installed \t110\n"
    "TestSourceDirProp3\tnot SourceDir and not SOURCEDIR and not Installed\t111\n"
    "FindRelatedProducts\t\t120\n"
    "TestSourceDirProp4\tnot SourceDir and not SOURCEDIR and not Installed\t121\n"
    "CCPSearch\t\t130\n"
    "TestSourceDirProp5\tnot SourceDir and not SOURCEDIR and not Installed\t131\n"
    "RMCCPSearch\t\t140\n"
    "TestSourceDirProp6\tnot SourceDir and not SOURCEDIR and not Installed\t141\n"
    "ValidateProductID\t\t150\n"
    "TestSourceDirProp7\tnot SourceDir and not SOURCEDIR and not Installed\t151\n"
    "CostInitialize\t\t800\n"
    "TestSourceDirProp8\tnot SourceDir and not SOURCEDIR and not Installed\t801\n"
    "FileCost\t\t900\n"
    "TestSourceDirProp9\tnot SourceDir and not SOURCEDIR and not Installed\t901\n"
    "IsolateComponents\t\t1000\n"
    "TestSourceDirProp10\tnot SourceDir and not SOURCEDIR and not Installed\t1001\n"
    "CostFinalize\t\t1100\n"
    "TestSourceDirProp11\tnot SourceDir and not SOURCEDIR and not Installed\t1101\n"
    "MigrateFeatureStates\t\t1200\n"
    "TestSourceDirProp12\tnot SourceDir and not SOURCEDIR and not Installed\t1201\n"
    "ExecuteAction\t\t1300\n"
    "TestSourceDirProp13\tnot SourceDir and not SOURCEDIR and not Installed\t1301\n";

static const char sd_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "TestSourceDirProp14\tSourceDir and SOURCEDIR and not Installed\t99\n"
    "LaunchConditions\t\t100\n"
    "TestSourceDirProp15\tSourceDir and SOURCEDIR and not Installed\t101\n"
    "ValidateProductID\t\t700\n"
    "TestSourceDirProp16\tSourceDir and SOURCEDIR and not Installed\t701\n"
    "CostInitialize\t\t800\n"
    "TestSourceDirProp17\tSourceDir and SOURCEDIR and not Installed\t801\n"
    "ResolveSource\tResolveSource and not Installed\t850\n"
    "TestSourceDirProp18\tResolveSource and not SourceDir and not SOURCEDIR and not Installed\t851\n"
    "TestSourceDirProp19\tnot ResolveSource and SourceDir and SOURCEDIR and not Installed\t852\n"
    "FileCost\t\t900\n"
    "TestSourceDirProp20\tSourceDir and SOURCEDIR and not Installed\t901\n"
    "IsolateComponents\t\t1000\n"
    "TestSourceDirProp21\tSourceDir and SOURCEDIR and not Installed\t1001\n"
    "CostFinalize\t\t1100\n"
    "TestSourceDirProp22\tSourceDir and SOURCEDIR and not Installed\t1101\n"
    "MigrateFeatureStates\t\t1200\n"
    "TestSourceDirProp23\tSourceDir and SOURCEDIR and not Installed\t1201\n"
    "InstallValidate\t\t1400\n"
    "TestSourceDirProp24\tSourceDir and SOURCEDIR and not Installed\t1401\n"
    "InstallInitialize\t\t1500\n"
    "TestSourceDirProp25\tSourceDir and SOURCEDIR and not Installed\t1501\n"
    "ProcessComponents\t\t1600\n"
    "TestSourceDirProp26\tnot SourceDir and not SOURCEDIR and not Installed\t1601\n"
    "UnpublishFeatures\t\t1800\n"
    "TestSourceDirProp27\tnot SourceDir and not SOURCEDIR and not Installed\t1801\n"
    "RemoveFiles\t\t3500\n"
    "TestSourceDirProp28\tnot SourceDir and not SOURCEDIR and not Installed\t3501\n"
    "InstallFiles\t\t4000\n"
    "TestSourceDirProp29\tnot SourceDir and not SOURCEDIR and not Installed\t4001\n"
    "RegisterUser\t\t6000\n"
    "TestSourceDirProp30\tnot SourceDir and not SOURCEDIR and not Installed\t6001\n"
    "RegisterProduct\t\t6100\n"
    "TestSourceDirProp31\tnot SourceDir and not SOURCEDIR and not Installed\t6101\n"
    "PublishFeatures\t\t6300\n"
    "TestSourceDirProp32\tnot SourceDir and not SOURCEDIR and not Installed\t6301\n"
    "PublishProduct\t\t6400\n"
    "TestSourceDirProp33\tnot SourceDir and not SOURCEDIR and not Installed\t6401\n"
    "InstallExecute\t\t6500\n"
    "TestSourceDirProp34\tnot SourceDir and not SOURCEDIR and not Installed\t6501\n"
    "InstallFinalize\t\t6600\n"
    "TestSourceDirProp35\tnot SourceDir and not SOURCEDIR and not Installed\t6601\n";

static const char sd_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "TestSourceDirProp1\t19\t\tTest 1 failed\t\n"
    "TestSourceDirProp2\t19\t\tTest 2 failed\t\n"
    "TestSourceDirProp3\t19\t\tTest 3 failed\t\n"
    "TestSourceDirProp4\t19\t\tTest 4 failed\t\n"
    "TestSourceDirProp5\t19\t\tTest 5 failed\t\n"
    "TestSourceDirProp6\t19\t\tTest 6 failed\t\n"
    "TestSourceDirProp7\t19\t\tTest 7 failed\t\n"
    "TestSourceDirProp8\t19\t\tTest 8 failed\t\n"
    "TestSourceDirProp9\t19\t\tTest 9 failed\t\n"
    "TestSourceDirProp10\t19\t\tTest 10 failed\t\n"
    "TestSourceDirProp11\t19\t\tTest 11 failed\t\n"
    "TestSourceDirProp12\t19\t\tTest 12 failed\t\n"
    "TestSourceDirProp13\t19\t\tTest 13 failed\t\n"
    "TestSourceDirProp14\t19\t\tTest 14 failed\t\n"
    "TestSourceDirProp15\t19\t\tTest 15 failed\t\n"
    "TestSourceDirProp16\t19\t\tTest 16 failed\t\n"
    "TestSourceDirProp17\t19\t\tTest 17 failed\t\n"
    "TestSourceDirProp18\t19\t\tTest 18 failed\t\n"
    "TestSourceDirProp19\t19\t\tTest 19 failed\t\n"
    "TestSourceDirProp20\t19\t\tTest 20 failed\t\n"
    "TestSourceDirProp21\t19\t\tTest 21 failed\t\n"
    "TestSourceDirProp22\t19\t\tTest 22 failed\t\n"
    "TestSourceDirProp23\t19\t\tTest 23 failed\t\n"
    "TestSourceDirProp24\t19\t\tTest 24 failed\t\n"
    "TestSourceDirProp25\t19\t\tTest 25 failed\t\n"
    "TestSourceDirProp26\t19\t\tTest 26 failed\t\n"
    "TestSourceDirProp27\t19\t\tTest 27 failed\t\n"
    "TestSourceDirProp28\t19\t\tTest 28 failed\t\n"
    "TestSourceDirProp29\t19\t\tTest 29 failed\t\n"
    "TestSourceDirProp30\t19\t\tTest 30 failed\t\n"
    "TestSourceDirProp31\t19\t\tTest 31 failed\t\n"
    "TestSourceDirProp32\t19\t\tTest 32 failed\t\n"
    "TestSourceDirProp33\t19\t\tTest 33 failed\t\n"
    "TestSourceDirProp34\t19\t\tTest 34 failed\t\n"
    "TestSourceDirProp35\t19\t\tTest 35 failed\t\n";

static const char ci_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "RunInstall\tnot Installed\t1550\n"
    "ProcessComponents\t\t1600\n"
    "UnpublishFeatures\t\t1800\n"
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6300\n"
    "PublishProduct\t\t6400\n"
    "InstallFinalize\t\t6600\n";

static const char ci_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "RunInstall\t23\tmsitest\\concurrent.msi\tMYPROP=[UILevel]\t\n";

static const char ci_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\tUILevel=5\tmaximus\n";

static const char ci2_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "augustus\t\tMSITESTDIR\t0\tUILevel=3 AND MYPROP=5\taugustus\n";

static const char ci2_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\taugustus";

static const char ci2_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "augustus\taugustus\taugustus\t500\t\t\t8192\t1";

static const char cl_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "TestCommandlineProp\t19\t\tTest1\t\n";

static const char cl_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "ValidateProductID\t\t700\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "TestCommandlineProp\tP=\"one\"\t1100\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "InstallValidate\t\t1400\n"
    "InstallFinalize\t\t5000\n";

static const msi_table tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(component),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property),
};

static const msi_table mcp_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(mcp_component),
    ADD_TABLE(mcp_feature),
    ADD_TABLE(mcp_feature_comp),
    ADD_TABLE(mcp_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table lus0_tables[] =
{
    ADD_TABLE(lus_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(lus_install_exec_seq),
    ADD_TABLE(lus0_media),
    ADD_TABLE(property)
};

static const msi_table lus1_tables[] =
{
    ADD_TABLE(lus_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(lus_install_exec_seq),
    ADD_TABLE(lus1_media),
    ADD_TABLE(property)
};

static const msi_table lus2_tables[] =
{
    ADD_TABLE(lus_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(lus_install_exec_seq),
    ADD_TABLE(lus2_media),
    ADD_TABLE(property)
};

static const msi_table spf_tables[] =
{
    ADD_TABLE(lus_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(lus0_media),
    ADD_TABLE(property),
    ADD_TABLE(spf_custom_action),
    ADD_TABLE(spf_install_exec_seq),
    ADD_TABLE(spf_install_ui_seq)
};

static const msi_table spf2_tables[] =
{
    ADD_TABLE(spf_component),
    ADD_TABLE(spf_directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(lus0_media),
    ADD_TABLE(property),
    ADD_TABLE(spf2_custom_action),
    ADD_TABLE(spf2_install_exec_seq),
    ADD_TABLE(spf_install_ui_seq),
    ADD_TABLE(shortcut)
};

static const msi_table sd_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(sd_component),
    ADD_TABLE(sd_feature),
    ADD_TABLE(sd_feature_comp),
    ADD_TABLE(sd_file),
    ADD_TABLE(sd_install_exec_seq),
    ADD_TABLE(sd_install_ui_seq),
    ADD_TABLE(sd_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table ci_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(lus_feature_comp),
    ADD_TABLE(lus_file),
    ADD_TABLE(ci_install_exec_seq),
    ADD_TABLE(lus0_media),
    ADD_TABLE(property),
    ADD_TABLE(ci_custom_action),
};

static const msi_table ci2_tables[] =
{
    ADD_TABLE(ci2_component),
    ADD_TABLE(directory),
    ADD_TABLE(lus_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(lus0_media),
    ADD_TABLE(ci2_property),
};

static const msi_table cl_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(cl_custom_action),
    ADD_TABLE(cl_install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static UINT set_summary_info(MSIHANDLE hdb, LPSTR prodcode)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summary info */
    res = MsiGetSummaryInformationA(hdb, NULL, 7, &suminfo);
    ok(res == ERROR_SUCCESS, "Failed to open summaryinfo\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 2, VT_LPSTR, 0, NULL,
                                    "Installation Database");
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 3, VT_LPSTR, 0, NULL,
                                    "Installation Database");
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 4, VT_LPSTR, 0, NULL,
                                    "Wine Hackers");
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 7, VT_LPSTR, 0, NULL,
                                    ";1033");
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, PID_REVNUMBER, VT_LPSTR, 0, NULL,
                                    "{A2078D65-94D6-4205-8DEE-F68D6FD622AA}");
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoSetPropertyA(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok(res == ERROR_SUCCESS, "Failed to set summary info\n");

    res = MsiSummaryInfoPersist(suminfo);
    ok(res == ERROR_SUCCESS, "Failed to make summary info persist\n");

    res = MsiCloseHandle(suminfo);
    ok(res == ERROR_SUCCESS, "Failed to close suminfo\n");

    return res;
}

static MSIHANDLE create_package_db(LPSTR prodcode)
{
    MSIHANDLE hdb = 0;
    CHAR query[MAX_PATH + 72];
    UINT res;

    DeleteFileA(msifile);

    /* create an empty database */
    res = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if (res != ERROR_SUCCESS)
        return hdb;

    res = MsiDatabaseCommit(hdb);
    ok(res == ERROR_SUCCESS, "Failed to commit database\n");

    set_summary_info(hdb, prodcode);

    res = run_query(hdb, 0,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)");
    ok(res == ERROR_SUCCESS , "Failed to create directory table\n");

    res = run_query(hdb, 0,
            "CREATE TABLE `Property` ( "
            "`Property` CHAR(72) NOT NULL, "
            "`Value` CHAR(255) "
            "PRIMARY KEY `Property`)");
    ok(res == ERROR_SUCCESS , "Failed to create directory table\n");

    sprintf(query, "INSERT INTO `Property` "
            "(`Property`, `Value`) "
            "VALUES( 'ProductCode', '%s' )", prodcode);
    res = run_query(hdb, 0, query);
    ok(res == ERROR_SUCCESS , "Failed\n");

    res = MsiDatabaseCommit(hdb);
    ok(res == ERROR_SUCCESS, "Failed to commit database\n");

    return hdb;
}

static void test_usefeature(void)
{
    INSTALLSTATE r;

    r = MsiQueryFeatureStateA(NULL, NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiQueryFeatureStateA("{9085040-6000-11d3-8cfe-0150048383c9}" ,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA(NULL,NULL,0,0);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA(NULL, "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}",
                         NULL, -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA("{9085040-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA("{0085040-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");
}

static void test_null(void)
{
    MSIHANDLE hpkg;
    UINT r;
    HKEY hkey;
    DWORD dwType, cbData;
    LPBYTE lpData = NULL;
    INSTALLSTATE state;
    REGSAM access = KEY_ALL_ACCESS;

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    r = MsiOpenPackageExW(NULL, 0, &hpkg);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    state = MsiQueryProductStateW(NULL);
    ok( state == INSTALLSTATE_INVALIDARG, "wrong return\n");

    r = MsiEnumFeaturesW(NULL,0,NULL,NULL);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiConfigureFeatureW(NULL, NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000001}", "foo", 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000002}", "foo", INSTALLSTATE_DEFAULT);
    ok( r == ERROR_UNKNOWN_PRODUCT, "wrong error %d\n", r);

    /* make sure empty string to MsiGetProductInfo is not a handle to default registry value, saving and restoring the
     * necessary registry values */

    /* empty product string */
    r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, access, &hkey);
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
    ok ( r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND, "wrong error %d\n", r);
    if ( r == ERROR_SUCCESS )
    {
        if (!(lpData = malloc(cbData))) skip("Out of memory\n");
        else
        {
            r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
            ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
        }
    }

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        free(lpData);
        RegCloseKey(hkey);
        return;
    }
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("", "", NULL, NULL);
    ok ( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    if (lpData)
    {
        r = RegSetValueExA(hkey, NULL, 0, dwType, lpData, cbData);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
        free(lpData);
    }
    else
    {
        r = RegDeleteValueA(hkey, NULL);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
    }

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    /* empty attribute */
    r = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}",
                        0, NULL, 0, access, NULL, &hkey, NULL);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("{F1C3AF50-8B56-4A69-A00C-00773FE42F30}", "", NULL, NULL);
    ok ( r == ERROR_UNKNOWN_PROPERTY, "wrong error %d\n", r);

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegDeleteKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}",
                        access & KEY_WOW64_64KEY, 0);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);
}

static void test_getcomponentpath(void)
{
    INSTALLSTATE r;
    char buffer[0x100];
    DWORD sz;

    r = MsiGetComponentPathA( NULL, NULL, NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = MsiGetComponentPathA( "bogus", "bogus", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = MsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = MsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = MsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");

    r = MsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-00000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = MsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{029E403D-A86A-1D11-5B5B0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = MsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C9987e}",
                            "{00000000-A68A-11d1-5B5B-0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");
}

static void create_test_files(void)
{
    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\one.txt", 100);
    CreateDirectoryA("msitest\\first", NULL);
    create_file("msitest\\first\\two.txt", 100);
    CreateDirectoryA("msitest\\second", NULL);
    create_file("msitest\\second\\three.txt", 100);

    create_file("four.txt", 100);
    create_file("five.txt", 100);
    create_cab_file("msitest.cab", MEDIA_SIZE, "four.txt\0five.txt\0");

    create_file("msitest\\filename", 100);
    create_file("msitest\\service.exe", 100);

    DeleteFileA("four.txt");
    DeleteFileA("five.txt");
}

static void delete_test_files(void)
{
    DeleteFileA("msitest.msi");
    DeleteFileA("msitest.cab");
    DeleteFileA("msitest\\second\\three.txt");
    DeleteFileA("msitest\\first\\two.txt");
    DeleteFileA("msitest\\one.txt");
    DeleteFileA("msitest\\service.exe");
    DeleteFileA("msitest\\filename");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

#define HASHSIZE sizeof(MSIFILEHASHINFO)

static const struct
{
    LPCSTR data;
    DWORD size;
    MSIFILEHASHINFO hash;
} hash_data[] =
{
    { "", 0,
      { HASHSIZE,
        { 0, 0, 0, 0 },
      },
    },

    { "abc", 0,
      { HASHSIZE,
        { 0x98500190, 0xb04fd23c, 0x7d3f96d6, 0x727fe128 },
      },
    },

    { "C:\\Program Files\\msitest\\caesar\n", 0,
      { HASHSIZE,
        { 0x2b566794, 0xfd42181b, 0x2514d6e4, 0x5768b4e2 },
      },
    },

    { "C:\\Program Files\\msitest\\caesar\n", 500,
      { HASHSIZE,
        { 0x58095058, 0x805efeff, 0x10f3483e, 0x0147d653 },
      },
    },
};

static void test_MsiGetFileHash(void)
{
    const char name[] = "msitest.bin";
    UINT r;
    MSIFILEHASHINFO hash;
    DWORD i;

    hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);

    /* szFilePath is NULL */
    r = MsiGetFileHashA(NULL, 0, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szFilePath is empty */
    r = MsiGetFileHashA("", 0, &hash);
    ok(r == ERROR_PATH_NOT_FOUND || r == ERROR_BAD_PATHNAME,
       "Expected ERROR_PATH_NOT_FOUND or ERROR_BAD_PATHNAME, got %d\n", r);

    /* szFilePath is nonexistent */
    r = MsiGetFileHashA(name, 0, &hash);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    /* dwOptions is non-zero */
    r = MsiGetFileHashA(name, 1, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* pHash.dwFileHashInfoSize is not correct */
    hash.dwFileHashInfoSize = 0;
    r = MsiGetFileHashA(name, 0, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* pHash is NULL */
    r = MsiGetFileHashA(name, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    for (i = 0; i < ARRAY_SIZE(hash_data); i++)
    {
        int ret;

        create_file_data(name, hash_data[i].data, hash_data[i].size);

        memset(&hash, 0, sizeof(MSIFILEHASHINFO));
        hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);

        r = MsiGetFileHashA(name, 0, &hash);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        ret = memcmp(&hash, &hash_data[i].hash, HASHSIZE);
        ok(!ret, "Hash incorrect\n");

        DeleteFileA(name);
    }
}

/* copied from dlls/msi/registry.c */
static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPCOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void create_test_guid(LPSTR prodcode, LPSTR squashed)
{
    WCHAR guidW[GUID_SIZE];
    WCHAR squashedW[SQUASHED_GUID_SIZE];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    size = StringFromGUID2(&guid, guidW, ARRAY_SIZE(guidW));
    ok(size == GUID_SIZE, "Expected %d, got %d.\n", GUID_SIZE, size);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, GUID_SIZE, NULL, NULL);
    if (squashed)
    {
        squash_guid(guidW, squashedW);
        WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, SQUASHED_GUID_SIZE, NULL, NULL);
    }
}

static char *get_user_sid(void)
{
    HANDLE token;
    DWORD size = 0;
    TOKEN_USER *user;
    char *usersid = NULL;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    GetTokenInformation(token, TokenUser, NULL, size, &size);

    user = malloc(size);
    GetTokenInformation(token, TokenUser, user, size, &size);
    ConvertSidToStringSidA(user->User.Sid, &usersid);
    free(user);

    CloseHandle(token);
    return usersid;
}

static void test_MsiQueryProductState(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    LPSTR usersid;
    INSTALLSTATE state;
    LONG res;
    HKEY userkey, localkey, props;
    HKEY prodkey;
    DWORD data, error;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(NULL);
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* empty prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA("");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* garbage prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA("garbage");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* guid without brackets */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* guid with brackets */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    /* same length as guid, but random */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    lstrcatA(keypath, prodcode);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        RegDeleteKeyA(userkey, "");
        RegCloseKey(userkey);
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local uninstall key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    data = 1;
    res = RegSetValueExA(localkey, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* WindowsInstaller value exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    RegDeleteValueA(localkey, "WindowsInstaller");
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        RegDeleteKeyA(userkey, "");
        RegCloseKey(userkey);
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local product key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* install properties key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* WindowsInstaller value exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    data = 2;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* WindowsInstaller value is not 1 */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    RegDeleteKeyA(userkey, "");

    /* user product key does not exist */
    SetLastError(0xdeadbeef);
    state = MsiQueryProductStateA(prodcode);
    error = GetLastError();
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(localkey);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(localkey);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(localkey);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    LocalFree(usersid);
}

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 */
static void encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;
}

static void compose_base85_guid(LPSTR component, LPSTR comp_base85, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR base85W[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    size = StringFromGUID2(&guid, guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", size);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, component, MAX_PATH, NULL, NULL);
    encode_base85_guid(&guid, base85W);
    WideCharToMultiByte(CP_ACP, 0, base85W, -1, comp_base85, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void test_MsiQueryFeatureState(void)
{
    HKEY userkey, localkey, compkey, compkey2;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH], comp_squashed2[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    INSTALLSTATE state;
    LPSTR usersid;
    LONG res;
    REGSAM access = KEY_ALL_ACCESS;
    DWORD error;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    compose_base85_guid(component, comp_base85 + 20, comp_squashed2);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(NULL, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* empty prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA("", "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* garbage prodcode */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA("garbage", "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* guid without brackets */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* guid with brackets */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    /* same length as guid, but random */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* NULL szFeature */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, NULL);
    error = GetLastError();
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* empty szFeature */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    /* feature key does not exist yet */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Features\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(userkey, "feature", 0, REG_SZ, (const BYTE *)"", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature value exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\Features");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        RegDeleteKeyA(userkey, "");
        RegCloseKey(userkey);
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* userdata features key exists */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaa", 20);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_BADCONFIG, "Expected INSTALLSTATE_BADCONFIG, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaa", 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaaa", 22);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)comp_base85, 41);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed2);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey2, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"apple", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey2, prod_squashed, 0, REG_SZ, (const BYTE *)"orange", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_LOCAL */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01\\", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_SOURCE */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* bad INSTALLSTATE_SOURCE */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01a", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_SOURCE */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* bad INSTALLSTATE_SOURCE */
    SetLastError(0xdeadbeef);
    state = MsiQueryFeatureStateA(prodcode, "feature");
    error = GetLastError();
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %lu\n", error);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey2, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(compkey2, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(localkey, "feature");
    RegDeleteValueA(userkey, "feature");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(compkey);
    RegCloseKey(compkey2);
    RegCloseKey(localkey);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Features\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(userkey, "feature", 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature value exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\Features");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* userdata features key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaa", 20);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_BADCONFIG, "Expected INSTALLSTATE_BADCONFIG, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaa", 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaaa", 22);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)comp_base85, 41);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed2);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey2, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"apple", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey2, prod_squashed, 0, REG_SZ, (const BYTE *)"orange", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey2, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(compkey2, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(localkey, "feature");
    RegDeleteValueA(userkey, "feature");
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    RegCloseKey(compkey2);
    RegCloseKey(localkey);
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Features\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(userkey, "feature", 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* feature value exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\Features");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* userdata features key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaa", 20);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_BADCONFIG, "Expected INSTALLSTATE_BADCONFIG, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaa", 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaaa", 22);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)comp_base85, 41);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed2);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey2, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"apple", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey2, prod_squashed, 0, REG_SZ, (const BYTE *)"orange", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey2, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(compkey2, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(localkey, "feature");
    RegDeleteValueA(userkey, "feature");
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    RegCloseKey(compkey2);
    RegCloseKey(localkey);
    RegCloseKey(userkey);
    LocalFree(usersid);
}

static void test_MsiQueryComponentState(void)
{
    HKEY compkey, prodkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    INSTALLSTATE state;
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;
    DWORD error;

    static const INSTALLSTATE MAGIC_ERROR = 0xdeadbeef;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProductCode */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(NULL, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* empty szProductCode */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA("", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* random szProductCode */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA("random", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* GUID-length szProductCode */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA("DJANE93KNDNAS-2KN2NR93KMN3LN13=L1N3KDE", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* GUID-length with brackets */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA("{JANE93KNDNAS-2KN2NR93KMN3LN13=L1N3KD}", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* actual GUID */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* create local system product key */
    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local system product key exists */
    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* component key exists */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* component\product exists */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    error = GetLastError();
    ok(state == INSTALLSTATE_NOTUSED || state == INSTALLSTATE_LOCAL,
       "Expected INSTALLSTATE_NOTUSED or INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    /* NULL component, product exists */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, NULL, &state);
    error = GetLastError();
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected state not changed, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"hi", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_LOCAL */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01\\", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_SOURCE */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* bad INSTALLSTATE_SOURCE */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01a", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* INSTALLSTATE_SOURCE */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"01:", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* registry component */
    state = MAGIC_ERROR;
    SetLastError(0xdeadbeef);
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(error == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", error);

    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegCloseKey(compkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    RegCloseKey(prodkey);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* component key exists */
    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* component\product exists */
    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_NOTUSED || state == INSTALLSTATE_LOCAL,
       "Expected INSTALLSTATE_NOTUSED or INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"hi", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(prodkey, "ManagedLocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    state = MAGIC_ERROR;
    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "ManagedLocalPackage");
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    LocalFree(usersid);
}

static void test_MsiGetComponentPath(void)
{
    HKEY compkey, prodkey, installprop;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR path[MAX_PATH];
    INSTALLSTATE state;
    LPSTR usersid;
    DWORD size, val;
    REGSAM access = KEY_ALL_ACCESS;
    LONG res;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProduct */
    size = MAX_PATH;
    state = MsiGetComponentPathA(NULL, component, path, &size);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* NULL szComponent */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, NULL, path, &size);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(NULL, path, &size);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* NULL lpPathBuf */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, NULL, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, NULL, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* NULL pcchBuf */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* all params valid */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local system component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &installprop, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* install properties key exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    create_file("C:\\imapath", 11);

    /* file exists */
    path[0] = 'a';
    size = 0;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_MOREDATA, "Expected INSTALLSTATE_MOREDATA, got %d\n", state);
    ok(path[0] == 'a', "got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    size = 0;
    path[0] = 'a';
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_MOREDATA, "Expected INSTALLSTATE_MOREDATA, got %d\n", state);
    ok(path[0] == 'a', "got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyExA(installprop, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &installprop, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* install properties key exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    create_file("C:\\imapath", 11);

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyExA(installprop, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &installprop, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* install properties key exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    create_file("C:\\imapath", 11);

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyExA(installprop, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    create_file("C:\\imapath", 11);

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(prodkey, "");
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local classes product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %ld\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    create_file("C:\\imapath", 11);

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    path[0] = 0;
    size = MAX_PATH;
    state = MsiLocateComponentA(component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    DeleteFileA("C:\\imapath");
    LocalFree(usersid);
}

static void test_MsiGetComponentPathEx(void)
{
    HKEY key_comp, key_installprop, key_prod;
    char prod[MAX_PATH], prod_squashed[MAX_PATH];
    char comp[MAX_PATH], comp_base85[MAX_PATH], comp_squashed[MAX_PATH];
    char path[MAX_PATH], path_key[MAX_PATH], *usersid;
    INSTALLSTATE state;
    DWORD size, val;
    REGSAM access = KEY_ALL_ACCESS;
    LONG res;

    if (is_wow64) access |= KEY_WOW64_64KEY;

    create_test_guid( prod, prod_squashed );
    compose_base85_guid( comp, comp_base85, comp_squashed );
    usersid = get_user_sid();

    /* NULL product */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( NULL, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_INVALIDARG, "got %d\n", state );
    todo_wine ok( !size, "got %lu\n", size );

    /* NULL component */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, NULL, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_INVALIDARG, "got %d\n", state );
    todo_wine ok( !size, "got %lu\n", size );

    /* non-NULL usersid, MSIINSTALLCONTEXT_MACHINE */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, usersid, MSIINSTALLCONTEXT_MACHINE, path, &size);
    ok( state == INSTALLSTATE_INVALIDARG, "got %d\n", state );
    todo_wine ok( !size, "got %lu\n", size );

    /* NULL buf */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, NULL, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok( size == MAX_PATH * 2, "got %lu\n", size );

    /* NULL buflen */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, NULL );
    ok( state == INSTALLSTATE_INVALIDARG, "got %d\n", state );
    ok( size == MAX_PATH, "got %lu\n", size );

    /* all params valid */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok( !size, "got %lu\n", size );

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\S-1-5-18\\Components\\" );
    lstrcatA( path_key, comp_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_comp, NULL );
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* local system component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );

    res = RegSetValueExA( key_comp, prod_squashed, 0, REG_SZ, (const BYTE *)"c:\\testcomponentpath", 20 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\S-1-5-18\\Products\\" );
    lstrcatA( path_key, prod_squashed );
    lstrcatA( path_key, "\\InstallProperties" );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_installprop, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    val = 1;
    res = RegSetValueExA( key_installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(val) );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* install properties key exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath"), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    create_file( "c:\\testcomponentpath", 21 );

    /* file exists */
    path[0] = 0;
    size = 0;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_MOREDATA, "got %d\n", state );
    ok( !path[0], "got %s\n", path );
    todo_wine ok( size == 40, "got %lu\n", size );

    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_LOCAL, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    RegDeleteValueA( key_comp, prod_squashed );
    RegDeleteKeyExA( key_comp, "", access & KEY_WOW64_64KEY, 0 );
    RegDeleteValueA( key_installprop, "WindowsInstaller" );
    RegDeleteKeyExA( key_installprop, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( key_comp );
    RegCloseKey( key_installprop );
    DeleteFileA( "c:\\testcomponentpath" );

    lstrcpyA( path_key, "Software\\Microsoft\\Installer\\Products\\" );
    lstrcatA( path_key, prod_squashed );

    res = RegCreateKeyA( HKEY_CURRENT_USER, path_key, &key_prod );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* user unmanaged product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok(!size, "got %lu\n", size);

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\" );
    lstrcatA( path_key, usersid );
    lstrcatA( path_key, "\\Components\\" );
    lstrcatA( path_key, comp_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_comp, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* user unmanaged component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok(!size, "got %lu\n", size);

    res = RegSetValueExA( key_comp, prod_squashed, 0, REG_SZ, (const BYTE *)"c:\\testcomponentpath", 20 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath"), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    create_file( "c:\\testcomponentpath", 21 );

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, path, &size );
    ok( state == INSTALLSTATE_LOCAL, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath"), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    RegDeleteValueA( key_comp, prod_squashed );
    RegDeleteKeyA( key_prod, "" );
    RegDeleteKeyExA( key_comp, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( key_prod );
    RegCloseKey( key_comp );
    DeleteFileA( "c:\\testcomponentpath" );

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\Managed\\" );
    lstrcatA( path_key, usersid );
    lstrcatA( path_key, "\\Installer\\Products\\" );
    lstrcatA( path_key, prod_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_prod, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* user managed product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\" );
    lstrcatA( path_key, usersid );
    lstrcatA( path_key, "\\Components\\" );
    lstrcatA( path_key, comp_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_comp, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* user managed component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );

    res = RegSetValueExA( key_comp, prod_squashed, 0, REG_SZ, (const BYTE *)"c:\\testcomponentpath", 20 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\S-1-5-18\\Products\\" );
    lstrcatA( path_key, prod_squashed );
    lstrcatA( path_key, "\\InstallProperties" );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_installprop, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    val = 1;
    res = RegSetValueExA( key_installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(val) );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* install properties key exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    create_file( "c:\\testcomponentpath", 21 );

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_USERMANAGED, path, &size );
    ok( state == INSTALLSTATE_LOCAL, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    RegDeleteValueA( key_comp, prod_squashed );
    RegDeleteKeyExA( key_prod, "", access & KEY_WOW64_64KEY, 0 );
    RegDeleteKeyExA( key_comp, "", access & KEY_WOW64_64KEY, 0 );
    RegDeleteValueA( key_installprop, "WindowsInstaller" );
    RegDeleteKeyExA( key_installprop, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( key_prod );
    RegCloseKey( key_comp );
    RegCloseKey( key_installprop );
    DeleteFileA( "c:\\testcomponentpath" );
    lstrcpyA( path_key, "Software\\Classes\\Installer\\Products\\" );
    lstrcatA( path_key, prod_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, path_key, 0, NULL, 0, access, NULL, &key_prod, NULL );
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* local classes product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok(!size, "got %lu\n", size);

    lstrcpyA( path_key, "Software\\Microsoft\\Windows\\CurrentVersion\\" );
    lstrcatA( path_key, "Installer\\UserData\\S-1-5-18\\Components\\" );
    lstrcatA( path_key, comp_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE,  path_key, 0, NULL, 0, access, NULL, &key_comp, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* local user component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );
    todo_wine ok(!size, "got %lu\n", size);

    res = RegSetValueExA( key_comp, prod_squashed, 0, REG_SZ, (const BYTE *)"c:\\testcomponentpath", 20 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    /* product value exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_ABSENT, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    create_file( "c:\\testcomponentpath", 21 );

    /* file exists */
    path[0] = 0;
    size = MAX_PATH;
    state = MsiGetComponentPathExA( prod, comp, NULL, MSIINSTALLCONTEXT_MACHINE, path, &size );
    ok( state == INSTALLSTATE_LOCAL, "got %d\n", state );
    ok( !lstrcmpA( path, "c:\\testcomponentpath" ), "got %s\n", path );
    ok( size == 20, "got %lu\n", size );

    RegDeleteValueA( key_comp, prod_squashed );
    RegDeleteKeyExA( key_prod, "", access & KEY_WOW64_64KEY, 0 );
    RegDeleteKeyExA( key_comp, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( key_prod );
    RegCloseKey( key_comp );
    DeleteFileA( "c:\\testcomponentpath" );
    LocalFree( usersid );
}

static void test_MsiProvideComponent(void)
{
    INSTALLSTATE state;
    char buf[0x100];
    WCHAR bufW[0x100];
    DWORD len, len2;
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\sourcedir.txt", 1000);
    create_database(msifile, sd_tables, ARRAY_SIZE(sd_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    buf[0] = 0;
    len = sizeof(buf);
    r = MsiProvideComponentA("{90120000-0070-0000-0000-4000000FF1CE}",
                             "{17961602-C4E2-482E-800A-DF6E627549CF}",
                             "ProductFiles", INSTALLMODE_NODETECTION, buf, &len);
    ok(r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    state = MsiQueryFeatureStateA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", "sourcedir");
    ok(state == INSTALLSTATE_LOCAL, "got %d\n", state);

    buf[0] = 0;
    len = sizeof(buf);
    r = MsiProvideComponentA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", "sourcedir",
                             "{DD422F92-3ED8-49B5-A0B7-F266F98357DF}",
                             INSTALLMODE_NODETECTION, buf, &len);
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(buf[0], "empty path\n");
    ok(len == lstrlenA(buf), "got %lu\n", len);

    len2 = 0;
    r = MsiProvideComponentA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", "sourcedir",
                             "{DD422F92-3ED8-49B5-A0B7-F266F98357DF}",
                             INSTALLMODE_NODETECTION, NULL, &len2);
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(len2 == len, "got %lu\n", len2);

    len2 = 0;
    r = MsiProvideComponentA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", "sourcedir",
                             "{DD422F92-3ED8-49B5-A0B7-F266F98357DF}",
                             INSTALLMODE_NODETECTION, buf, &len2);
    ok(r == ERROR_MORE_DATA, "got %u\n", r);
    ok(len2 == len, "got %lu\n", len2);

    /* wide version */

    bufW[0] = 0;
    len = sizeof(buf);
    r = MsiProvideComponentW(L"{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", L"sourcedir",
                             L"{DD422F92-3ED8-49B5-A0B7-F266F98357DF}", INSTALLMODE_NODETECTION, bufW, &len);
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(bufW[0], "empty path\n");
    ok(len == lstrlenW(bufW), "got %lu\n", len);

    len2 = 0;
    r = MsiProvideComponentW(L"{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", L"sourcedir",
                             L"{DD422F92-3ED8-49B5-A0B7-F266F98357DF}", INSTALLMODE_NODETECTION, NULL, &len2);
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(len2 == len, "got %lu\n", len2);

    len2 = 0;
    r = MsiProvideComponentW(L"{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", L"sourcedir",
                             L"{DD422F92-3ED8-49B5-A0B7-F266F98357DF}", INSTALLMODE_NODETECTION, bufW, &len2);
    ok(r == ERROR_MORE_DATA, "got %u\n", r);
    ok(len2 == len, "got %lu\n", len2);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "got %u\n", r);

    DeleteFileA("msitest\\sourcedir.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_MsiProvideQualifiedComponentEx(void)
{
    UINT r;
    INSTALLSTATE state;
    char comp[39], comp_squashed[33], comp2[39], comp2_base85[21], comp2_squashed[33];
    char prod[39], prod_base85[21], prod_squashed[33];
    char desc[MAX_PATH], buf[MAX_PATH], keypath[MAX_PATH], path[MAX_PATH];
    DWORD len = sizeof(buf);
    REGSAM access = KEY_ALL_ACCESS;
    HKEY hkey, hkey2, hkey3, hkey4, hkey5;
    LONG res;

    if (!is_process_elevated())
    {
        skip( "process is limited\n" );
        return;
    }

    create_test_guid( comp, comp_squashed );
    compose_base85_guid( comp2, comp2_base85, comp2_squashed );
    compose_base85_guid( prod, prod_base85, prod_squashed );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    ok( r == ERROR_UNKNOWN_COMPONENT, "got %u\n", r );

    lstrcpyA( keypath, "Software\\Classes\\Installer\\Components\\" );
    lstrcatA( keypath, comp_squashed );

    if (is_wow64) access |= KEY_WOW64_64KEY;
    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    lstrcpyA( desc, prod_base85 );
    memcpy( desc + lstrlenA(desc), "feature<\0", sizeof("feature<\0") );
    res = RegSetValueExA( hkey, "qualifier", 0, REG_MULTI_SZ, (const BYTE *)desc,
                          lstrlenA(prod_base85) + sizeof("feature<\0") );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    ok( r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, NULL, 0, 0, buf, &len );
    ok( r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r );

    state = MsiQueryProductStateA( prod );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );

    lstrcpyA( keypath, "Software\\Classes\\Installer\\Products\\" );
    lstrcatA( keypath, prod_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey2, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    state = MsiQueryProductStateA( prod );
    ok( state == INSTALLSTATE_ADVERTISED, "got %d\n", state );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    todo_wine ok( r == ERROR_UNKNOWN_FEATURE, "got %u\n", r );

    lstrcpyA( keypath, "Software\\Classes\\Installer\\Features\\" );
    lstrcatA( keypath, prod_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey3, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    state = MsiQueryFeatureStateA( prod, "feature" );
    ok( state == INSTALLSTATE_UNKNOWN, "got %d\n", state );

    res = RegSetValueExA( hkey3, "feature", 0, REG_SZ, (const BYTE *)"", 1 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    state = MsiQueryFeatureStateA( prod, "feature" );
    ok( state == INSTALLSTATE_ADVERTISED, "got %d\n", state );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    ok( r == ERROR_FILE_NOT_FOUND, "got %u\n", r );

    len = sizeof(buf);
    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, NULL, 0, 0, buf, &len );
    ok( r == ERROR_FILE_NOT_FOUND, "got %u\n", r );

    lstrcpyA( keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\" );
    lstrcatA( keypath, prod_squashed );
    lstrcatA( keypath, "\\Features" );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey4, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    res = RegSetValueExA( hkey4, "feature", 0, REG_SZ, (const BYTE *)comp2_base85, sizeof(comp2_base85) );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    state = MsiQueryFeatureStateA( prod, "feature" );
    ok( state == INSTALLSTATE_ADVERTISED, "got %d\n", state );

    lstrcpyA( keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Components\\" );
    lstrcatA( keypath, comp2_squashed );

    res = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey5, NULL );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    res = RegSetValueExA( hkey5, prod_squashed, 0, REG_SZ, (const BYTE *)"c:\\nosuchfile", sizeof("c:\\nosuchfile") );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    state = MsiQueryFeatureStateA( prod, "feature" );
    ok( state == INSTALLSTATE_LOCAL, "got %d\n", state );

    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    ok( r == ERROR_FILE_NOT_FOUND, "got %u\n", r );

    GetCurrentDirectoryA( MAX_PATH, path );
    lstrcatA( path, "\\msitest" );
    CreateDirectoryA( path, NULL );
    lstrcatA( path, "\\test.txt" );
    create_file_data( path, "test", 100 );

    res = RegSetValueExA( hkey5, prod_squashed, 0, REG_SZ, (const BYTE *)path, lstrlenA(path) + 1 );
    ok( res == ERROR_SUCCESS, "got %ld\n", res );

    buf[0] = 0;
    len = sizeof(buf);
    r = MsiProvideQualifiedComponentExA( comp, "qualifier", INSTALLMODE_EXISTING, prod, 0, 0, buf, &len );
    ok( r == ERROR_SUCCESS, "got %u\n", r );
    ok( len == lstrlenA(path), "got %lu\n", len );
    ok( !lstrcmpA( path, buf ), "got '%s'\n", buf );

    DeleteFileA( "msitest\\text.txt" );
    RemoveDirectoryA( "msitest" );

    RegDeleteKeyExA( hkey5, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( hkey5 );
    RegDeleteKeyExA( hkey4, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( hkey4 );
    RegDeleteKeyExA( hkey3, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( hkey3 );
    RegDeleteKeyExA( hkey2, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( hkey2 );
    RegDeleteKeyExA( hkey, "", access & KEY_WOW64_64KEY, 0 );
    RegCloseKey( hkey );
}

static void test_MsiGetProductCode(void)
{
    HKEY compkey, prodkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR prodcode2[MAX_PATH];
    CHAR prod2_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR product[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(prodcode2, prod2_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* szComponent is NULL */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(NULL, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* szComponent is empty */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* garbage szComponent */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("garbage", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* guid without brackets */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* guid with brackets */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* same length as guid, but random */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* all params correct, szComponent not published */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged component key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed product key of first product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local classes product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod2_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed product key of second product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode2), "Expected %s, got %s\n", prodcode2, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user component key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed product key of first product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local classes product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod2_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user managed product key of second product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode2), "Expected %s, got %s\n", prodcode2, product);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    LocalFree(usersid);
}

static void test_MsiEnumClients(void)
{
    HKEY compkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR prodcode2[MAX_PATH];
    CHAR prod2_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR product[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(prodcode2, prod2_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szComponent */
    product[0] = '\0';
    r = MsiEnumClientsA(NULL, 0, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* empty szComponent */
    product[0] = '\0';
    r = MsiEnumClientsA("", 0, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* NULL lpProductBuf */
    r = MsiEnumClientsA(component, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* all params correct, component missing */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user unmanaged component key exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* index > 0, no products exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 0 again */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 1, second product value does not exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* try index 1, second product value does exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    todo_wine
    {
        ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
        ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);
    }

    /* start the enumeration over */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    /* correctly query second product */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &compkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user local component key exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* index > 0, no products exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product value exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 0 again */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try index 1, second product value does not exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* try index 1, second product value does exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    todo_wine
    {
        ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
        ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);
    }

    /* start the enumeration over */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    /* correctly query second product */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyExA(compkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(compkey);
    LocalFree(usersid);
}

static void get_version_info(LPSTR path, LPSTR *vercheck, LPDWORD verchecksz,
                             LPSTR *langcheck, LPDWORD langchecksz)
{
    LPSTR version;
    VS_FIXEDFILEINFO *ffi;
    DWORD size = GetFileVersionInfoSizeA(path, NULL);
    UINT len;
    USHORT *lang;

    version = malloc(size);
    GetFileVersionInfoA(path, 0, size, version);

    VerQueryValueA(version, "\\", (LPVOID *)&ffi, &len);
    *vercheck = malloc(MAX_PATH);
    sprintf(*vercheck, "%d.%d.%d.%d", HIWORD(ffi->dwFileVersionMS),
            LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS),
            LOWORD(ffi->dwFileVersionLS));
    *verchecksz = lstrlenA(*vercheck);

    VerQueryValueA(version, "\\VarFileInfo\\Translation", (void **)&lang, &len);
    *langcheck = malloc(MAX_PATH);
    sprintf(*langcheck, "%d", *lang);
    *langchecksz = lstrlenA(*langcheck);

    free(version);
}

static void test_MsiGetFileVersion(void)
{
    UINT r;
    DWORD versz, langsz;
    char version[MAX_PATH];
    char lang[MAX_PATH];
    char path[MAX_PATH];
    LPSTR vercheck, langcheck;
    DWORD verchecksz, langchecksz;

    /* NULL szFilePath */
    r = MsiGetFileVersionA(NULL, NULL, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(NULL, version, &versz, lang, &langsz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* empty szFilePath */
    r = MsiGetFileVersionA("", NULL, NULL, NULL, NULL);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* nonexistent szFilePath */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* nonexistent szFilePath, valid lpVersionBuf, NULL pcchVersionBuf */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, NULL, lang, &langsz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* nonexistent szFilePath, valid lpLangBuf, NULL pcchLangBuf */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* nonexistent szFilePath, valid lpVersionBuf, pcchVersionBuf is zero */
    versz = 0;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == 0, "Expected 0, got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);

    /* nonexistent szFilePath, valid lpLangBuf, pcchLangBuf is zero */
    versz = MAX_PATH;
    langsz = 0;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == 0, "Expected 0, got %lu\n", langsz);

    /* nonexistent szFilePath, rest NULL */
    r = MsiGetFileVersionA("nonexistent", NULL, NULL, NULL, NULL);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    create_file("ver.txt", 20);

    /* file exists, no version information */
    r = MsiGetFileVersionA("ver.txt", NULL, NULL, NULL, NULL);
    ok(r == ERROR_FILE_INVALID, "Expected ERROR_FILE_INVALID, got %d\n", r);

    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("ver.txt", version, &versz, lang, &langsz);
    ok(versz == MAX_PATH, "got %lu\n", versz);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(langsz == MAX_PATH, "got %lu\n", langsz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(r == ERROR_FILE_INVALID,
       "Expected ERROR_FILE_INVALID, got %d\n", r);

    DeleteFileA("ver.txt");

    /* relative path, has version information */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("kernel32.dll", version, &versz, lang, &langsz);
    todo_wine
    {
        ok(r == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
        ok(!lstrcmpA(version, "version"),
           "Expected version to be unchanged, got %s\n", version);
        ok(versz == MAX_PATH, "got %lu\n", versz);
        ok(!lstrcmpA(lang, "lang"),
           "Expected lang to be unchanged, got %s\n", lang);
        ok(langsz == MAX_PATH, "got %lu\n", langsz);
    }

    GetSystemDirectoryA(path, MAX_PATH);
    lstrcatA(path, "\\kernel32.dll");

    get_version_info(path, &vercheck, &verchecksz, &langcheck, &langchecksz);

    /* absolute path, has version information */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, version, &versz, lang, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);
    ok(strstr(lang, langcheck) != NULL, "Expected \"%s\" in \"%s\"\n", langcheck, lang);
    ok(!lstrcmpA(version, vercheck),
        "Expected %s, got %s\n", vercheck, version);

    /* only check version */
    versz = MAX_PATH;
    lstrcpyA(version, "version");
    r = MsiGetFileVersionA(path, version, &versz, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);
    ok(!lstrcmpA(version, vercheck), "Expected %s, got %s\n", vercheck, version);

    /* only check language */
    langsz = MAX_PATH;
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, NULL, NULL, lang, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(strstr(lang, langcheck) != NULL, "Expected \"%s\" in \"%s\"\n", langcheck, lang);

    /* check neither version nor language */
    r = MsiGetFileVersionA(path, NULL, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* get pcchVersionBuf */
    versz = MAX_PATH;
    r = MsiGetFileVersionA(path, NULL, &versz, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);

    /* get pcchLangBuf */
    langsz = MAX_PATH;
    r = MsiGetFileVersionA(path, NULL, NULL, NULL, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(langsz >= langchecksz, "Expected %lu >= %lu\n", langsz, langchecksz);

    /* pcchVersionBuf not big enough */
    versz = 5;
    lstrcpyA(version, "version");
    r = MsiGetFileVersionA(path, version, &versz, NULL, NULL);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(version, vercheck, 4),
       "Expected first 4 characters of \"%s\", got \"%s\"\n", vercheck, version);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);

    /* pcchLangBuf not big enough */
    langsz = 4;
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, NULL, NULL, lang, &langsz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(lstrcmpA(lang, "lang"), "lang not set\n");
    ok(langsz >= langchecksz, "Expected %lu >= %lu\n", langsz, langchecksz);

    /* pcchVersionBuf big enough, pcchLangBuf not big enough */
    versz = MAX_PATH;
    langsz = 0;
    lstrcpyA(version, "version");
    r = MsiGetFileVersionA(path, version, &versz, NULL, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);
    ok(!lstrcmpA(version, vercheck), "Expected \"%s\", got \"%s\"\n", vercheck, version);
    ok(langsz >= langchecksz && langsz < MAX_PATH, "Expected %lu >= %lu\n", langsz, langchecksz);

    /* pcchVersionBuf not big enough, pcchLangBuf big enough */
    versz = 5;
    langsz = MAX_PATH;
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, NULL, &versz, lang, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %lu, got %lu\n", verchecksz, versz);
    ok(langsz >= langchecksz && langsz < MAX_PATH, "Expected %lu >= %lu\n", langsz, langchecksz);
    ok(strstr(lang, langcheck) != NULL, "expected %s in %s\n", langcheck, lang);

    /* NULL pcchVersionBuf and pcchLangBuf */
    r = MsiGetFileVersionA(path, version, NULL, lang, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* All NULL except szFilePath */
    r = MsiGetFileVersionA(path, NULL, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    free(vercheck);
    free(langcheck);
}

static void test_MsiGetProductInfo(void)
{
    UINT r;
    LONG res;
    HKEY propkey, source;
    HKEY prodkey, localkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR packcode[MAX_PATH];
    CHAR pack_squashed[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    DWORD sz, val = 42;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(packcode, pack_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(NULL, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* empty szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("", INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* garbage szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("garbage", INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* guid without brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D",
                           INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* guid with brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}",
                           INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* same length as guid, but random */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93",
                           INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* not installed, NULL szAttribute */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, NULL, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* not installed, NULL lpValueBuf */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, NULL, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* not installed, NULL pcchValueBuf */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    /* created guid cannot possibly be an installed product code */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product code exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user product code exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* both local and managed product code exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "%lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    /* pcchBuf is NULL */
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* lpValueBuf is NULL */
    sz = MAX_PATH;
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    /* lpValueBuf is NULL, pcchValueBuf is too small */
    sz = 2;
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    /* lpValueBuf is non-NULL, pcchValueBuf is too small */
    sz = 2;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to remain unchanged, got \"%s\"\n", buf);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    /* lpValueBuf is non-NULL, pcchValueBuf is exactly 4 */
    sz = 4;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"),
       "Expected buf to remain unchanged, got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "IMadeThis", 0, REG_SZ, (LPBYTE)"random", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* random property not supported by MSI, value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, "IMadeThis", buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteValueA(propkey, "IMadeThis");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* classes product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(propkey);
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Local System product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
        "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"1.1.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONSTRINGA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1.1.1"), "Expected \"1.1.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0,
                         REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONSTRINGA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"tele", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPTELEPHONEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "tele"), "Expected \"tele\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPTELEPHONEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLLOCATIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLLOCATIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLSOURCEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLSOURCEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLDATEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLDATEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PUBLISHERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PUBLISHERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"pack", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LOCALPACKAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pack"), "Expected \"pack\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LOCALPACKAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "UrlInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UrlInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLINFOABOUTA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "UrlInfoAbout", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UrlInfoAbout type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLINFOABOUTA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "UrlUpdateInfo", 0, REG_SZ, (LPBYTE)"info", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UrlUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLUPDATEINFOA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "info"), "Expected \"info\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "UrlUpdateInfo", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UrlUpdateInfo type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLUPDATEINFOA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMINORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMINORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMAJORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMAJORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTIDA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTIDA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGCOMPANYA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGCOMPANYA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"own", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGOWNERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "own"), "Expected \"own\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGOWNERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstanceType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstanceType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstanceType", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstanceType type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstanceType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstanceType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "type"), "Expected \"type\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstanceType", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstanceType type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"tforms", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"tforms", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "tforms"), "Expected \"tforms\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Assignment", 0, REG_SZ, (LPBYTE)"at", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Assignment value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Assignment", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Assignment type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Assignment", 0, REG_SZ, (LPBYTE)"at", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Assignment value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "at"), "Expected \"at\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Assignment", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Assignment type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_BAD_CONFIGURATION,
       "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(buf, "code"), "Expected \"code\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)pack_squashed, 33);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, packcode), "Expected \"%s\", got \"%s\"\n", packcode, buf);
    ok(sz == 38, "Expected 38, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Version", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"ico", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"ico", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ico"), "Expected \"ico\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    /* SourceList key does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"),
       "Expected buf to be unchanged, got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected sz to be unchanged, got %lu\n", sz);

    res = RegCreateKeyExA(prodkey, "SourceList", 0, NULL, 0, access, NULL, &source, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList key exists, but PackageName val does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(source, "PackageName", 0, REG_SZ, (LPBYTE)"packname", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName val exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "packname"), "Expected \"packname\", got \"%s\"\n", buf);
    ok(sz == 8, "Expected 8, got %lu\n", sz);

    res = RegSetValueExA(source, "PackageName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Authorized value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APPA, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(sz == 0, "Expected 0, got %lu\n", sz);
    }

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APPA, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(sz == 0, "Expected 0, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Authorized value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APPA, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
        ok(sz == 4, "Expected 4, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APPA, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
        ok(sz == 2, "Expected 2, got %lu\n", sz);
    }

    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "UrlInfoAbout");
    RegDeleteValueA(propkey, "UrlUpdateInfo");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "InstanceType");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Assignment");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(prodkey, "InstanceType");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Assignment");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(source, "PackageName");
    RegDeleteKeyExA(source, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(source);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiGetProductInfoEx(void)
{
    UINT r;
    LONG res;
    HKEY propkey, userkey;
    HKEY prodkey, localkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR packcode[MAX_PATH];
    CHAR pack_squashed[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    DWORD sz;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(packcode, pack_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* empty szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* garbage szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* guid without brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", usersid,
                             MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* guid with brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", usersid,
                             MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* szValue is non-NULL while pcchValue is NULL */
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);

    /* dwContext is out of range */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, 42, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* szProperty is NULL */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, NULL, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* szProperty is empty */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, "", buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* szProperty is not a valid property */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, "notvalid", buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* same length as guid, but random */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                             INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    RegDeleteValueA(propkey, "LocalPackage");

    /* LocalPackage value must exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage exists, but HelpLink does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    /* szValue and pcchValue are NULL */
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* pcchValue is exactly 5 */
    sz = 5;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(sz == 10, "Expected 10, got %lu\n", sz);

    /* szValue is NULL, pcchValue is exactly 5 */
    sz = 5;
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 10, "Expected 10, got %lu\n", sz);

    /* szValue is NULL, pcchValue is MAX_PATH */
    sz = MAX_PATH;
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 10, "Expected 10, got %lu\n", sz);

    /* pcchValue is exactly 0 */
    sz = 0;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == 10, "Expected 10, got %lu\n", sz);

    res = RegSetValueExA(propkey, "notvalid", 0, REG_SZ, (LPBYTE)"invalid", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* szProperty is not a valid property */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, "notvalid", buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLDATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLLOCATIONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLSOURCEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PUBLISHERA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_URLINFOABOUTA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_URLUPDATEINFOA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONMINORA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONMAJORA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRINGA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTIDA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_REGCOMPANYA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_REGOWNERA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LANGUAGEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_ASSIGNMENTTYPEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PACKAGECODEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTICONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PACKAGENAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_AUTHORIZED_LUA_APPA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteKeyA(propkey, "");
    RegCloseKey(propkey);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_CURRENT_USER, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPLINKA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLDATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLLOCATIONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_INSTALLSOURCEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PUBLISHERA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_URLINFOABOUTA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_URLUPDATEINFOA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONMINORA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONMAJORA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRINGA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTIDA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_REGCOMPANYA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_REGOWNERA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LANGUAGEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_ASSIGNMENTTYPEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PACKAGECODEA,
                             buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PRODUCTICONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_PACKAGENAMEA,
                             buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_AUTHORIZED_LUA_APPA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(prodkey, "PackageName");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "AssignmentType");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "RegOwner");
    RegDeleteValueA(prodkey, "RegCompany");
    RegDeleteValueA(prodkey, "ProductID");
    RegDeleteValueA(prodkey, "DisplayVersion");
    RegDeleteValueA(prodkey, "VersionMajor");
    RegDeleteValueA(prodkey, "VersionMinor");
    RegDeleteValueA(prodkey, "URLUpdateInfo");
    RegDeleteValueA(prodkey, "URLInfoAbout");
    RegDeleteValueA(prodkey, "Publisher");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "InstallSource");
    RegDeleteValueA(prodkey, "InstallLocation");
    RegDeleteValueA(prodkey, "DisplayName");
    RegDeleteValueA(prodkey, "InstallDate");
    RegDeleteValueA(prodkey, "HelpTelephone");
    RegDeleteValueA(prodkey, "HelpLink");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ManagedLocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ManagedLocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_HELPLINKA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLDATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLLOCATIONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLSOURCEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PUBLISHERA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_URLINFOABOUTA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_URLUPDATEINFOA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONMINORA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONMAJORA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONSTRINGA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTIDA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_REGCOMPANYA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_REGOWNERA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LANGUAGEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_ASSIGNMENTTYPEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PACKAGECODEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTICONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PACKAGENAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_AUTHORIZED_LUA_APPA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "ManagedLocalPackage");
    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(propkey);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* current user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTSTATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists, user product key does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_HELPLINKA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(userkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists, user product key does exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_HELPLINKA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_HELPTELEPHONEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLDATEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLLOCATIONA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLSOURCEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PUBLISHERA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_URLINFOABOUTA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_URLUPDATEINFOA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONMINORA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONMAJORA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONSTRINGA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTIDA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_REGCOMPANYA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_REGOWNERA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(userkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(userkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LANGUAGEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(userkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(userkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_ASSIGNMENTTYPEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(userkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PACKAGECODEA,
                             buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(userkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_VERSIONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(userkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PRODUCTICONA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(userkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PACKAGENAMEA,
                             buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(userkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_AUTHORIZED_LUA_APPA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    RegDeleteValueA(userkey, "AuthorizedLUAApp");
    RegDeleteValueA(userkey, "PackageName");
    RegDeleteValueA(userkey, "ProductIcon");
    RegDeleteValueA(userkey, "Version");
    RegDeleteValueA(userkey, "PackageCode");
    RegDeleteValueA(userkey, "AssignmentType");
    RegDeleteValueA(userkey, "ProductName");
    RegDeleteValueA(userkey, "Language");
    RegDeleteValueA(userkey, "Transforms");
    RegDeleteValueA(userkey, "RegOwner");
    RegDeleteValueA(userkey, "RegCompany");
    RegDeleteValueA(userkey, "ProductID");
    RegDeleteValueA(userkey, "DisplayVersion");
    RegDeleteValueA(userkey, "VersionMajor");
    RegDeleteValueA(userkey, "VersionMinor");
    RegDeleteValueA(userkey, "URLUpdateInfo");
    RegDeleteValueA(userkey, "URLInfoAbout");
    RegDeleteValueA(userkey, "Publisher");
    RegDeleteValueA(userkey, "LocalPackage");
    RegDeleteValueA(userkey, "InstallSource");
    RegDeleteValueA(userkey, "InstallLocation");
    RegDeleteValueA(userkey, "DisplayName");
    RegDeleteValueA(userkey, "InstallDate");
    RegDeleteValueA(userkey, "HelpTelephone");
    RegDeleteValueA(userkey, "HelpLink");
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* szUserSid is non-NULL */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &localkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local system product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegCreateKeyExA(localkey, "InstallProperties", 0, NULL, 0, access, NULL, &propkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_HELPTELEPHONEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLDATEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLLOCATIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLSOURCEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PUBLISHERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_URLINFOABOUTA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_URLUPDATEINFOA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONMINORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONMAJORA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONSTRINGA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTIDA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_REGCOMPANYA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_REGOWNERA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_AUTHORIZED_LUA_APPA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteKeyExA(propkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(propkey);
    RegDeleteKeyExA(localkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local classes product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTSTATEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_HELPLINKA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_HELPTELEPHONEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLDATEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLEDPRODUCTNAMEA,
                             buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLLOCATIONA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_INSTALLSOURCEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PUBLISHERA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_URLINFOABOUTA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_URLUPDATEINFOA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONMINORA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONMAJORA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONSTRINGA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTIDA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_REGCOMPANYA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_REGOWNERA, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_TRANSFORMSA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LANGUAGEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTNAMEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_ASSIGNMENTTYPEA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PACKAGECODEA, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PRODUCTICONA, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    res = RegSetValueExA(prodkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_PACKAGENAMEA, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %lu\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_AUTHORIZED_LUA_APPA,
                             buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %lu\n", sz);

    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(prodkey, "PackageName");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "AssignmentType");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "RegOwner");
    RegDeleteValueA(prodkey, "RegCompany");
    RegDeleteValueA(prodkey, "ProductID");
    RegDeleteValueA(prodkey, "DisplayVersion");
    RegDeleteValueA(prodkey, "VersionMajor");
    RegDeleteValueA(prodkey, "VersionMinor");
    RegDeleteValueA(prodkey, "URLUpdateInfo");
    RegDeleteValueA(prodkey, "URLInfoAbout");
    RegDeleteValueA(prodkey, "Publisher");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "InstallSource");
    RegDeleteValueA(prodkey, "InstallLocation");
    RegDeleteValueA(prodkey, "DisplayName");
    RegDeleteValueA(prodkey, "InstallDate");
    RegDeleteValueA(prodkey, "HelpTelephone");
    RegDeleteValueA(prodkey, "HelpLink");
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

#define INIT_USERINFO() \
    lstrcpyA(user, "apple"); \
    lstrcpyA(org, "orange"); \
    lstrcpyA(serial, "banana"); \
    usersz = orgsz = serialsz = MAX_PATH;

static void test_MsiGetUserInfo(void)
{
    USERINFOSTATE state;
    CHAR user[MAX_PATH];
    CHAR org[MAX_PATH];
    CHAR serial[MAX_PATH];
    DWORD usersz, orgsz, serialsz;
    CHAR keypath[MAX_PATH * 2];
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    HKEY prodkey, userprod, props;
    LPSTR usersid;
    LONG res;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProduct */
    INIT_USERINFO();
    state = MsiGetUserInfoA(NULL, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* empty szProductCode */
    INIT_USERINFO();
    state = MsiGetUserInfoA("", user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* garbage szProductCode */
    INIT_USERINFO();
    state = MsiGetUserInfoA("garbage", user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* guid without brackets */
    INIT_USERINFO();
    state = MsiGetUserInfoA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D",
                            user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* guid with brackets */
    INIT_USERINFO();
    state = MsiGetUserInfoA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}",
                            user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL lpUserNameBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL pcchUserNameBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, NULL, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* both lpUserNameBuf and pcchUserNameBuf NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL lpOrgNameBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, NULL, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL pcchOrgNameBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, NULL, serial, &serialsz);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* both lpOrgNameBuf and pcchOrgNameBuf NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, NULL, NULL, serial, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL lpSerialBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, NULL, &serialsz);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* NULL pcchSerialBuf */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, NULL);
    ok(state == USERINFOSTATE_INVALIDARG,
       "Expected USERINFOSTATE_INVALIDARG, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);

    /* both lpSerialBuf and pcchSerialBuf NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, NULL, NULL);
    ok(state == USERINFOSTATE_UNKNOWN,
       "Expected USERINFOSTATE_UNKNOWN, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    /* create local system product key */
    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userprod, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegCreateKeyExA(userprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* RegOwner doesn't exist, lpUserNameBuf and pcchUserNameBuf are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    /* RegOwner, RegCompany don't exist, out params are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, NULL, NULL, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegCompany", 0, REG_SZ, (LPBYTE)"company", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "ProductID", 0, REG_SZ, (LPBYTE)"ID", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_PRESENT,
       "Expected USERINFOSTATE_PRESENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "ID"), "Expected \"ID\", got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == 2, "Expected 2, got %lu\n", serialsz);

    /* pcchUserNameBuf is too small */
    INIT_USERINFO();
    usersz = 0;
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_MOREDATA,
       "Expected USERINFOSTATE_MOREDATA, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* pcchUserNameBuf has no room for NULL terminator */
    INIT_USERINFO();
    usersz = 5;
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_MOREDATA,
       "Expected USERINFOSTATE_MOREDATA, got %d\n", state);
    todo_wine
    {
        ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    }
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* pcchUserNameBuf is too small, lpUserNameBuf is NULL */
    INIT_USERINFO();
    usersz = 0;
    state = MsiGetUserInfoA(prodcode, NULL, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_PRESENT,
       "Expected USERINFOSTATE_PRESENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "ID"), "Expected \"ID\", got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == 2, "Expected 2, got %lu\n", serialsz);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userprod);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    /* create local system product key */
    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userprod, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegCreateKeyExA(userprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* RegOwner doesn't exist, lpUserNameBuf and pcchUserNameBuf are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    /* RegOwner, RegCompany don't exist, out params are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, NULL, NULL, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegCompany", 0, REG_SZ, (LPBYTE)"company", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "ProductID", 0, REG_SZ, (LPBYTE)"ID", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_PRESENT,
       "Expected USERINFOSTATE_PRESENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "ID"), "Expected \"ID\", got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == 2, "Expected 2, got %lu\n", serialsz);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userprod);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* create local system product key */
    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* product key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH, "Expected MAX_PATH, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18");
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userprod, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegCreateKeyExA(userprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "apple"), "Expected user to be unchanged, got \"%s\"\n", user);
    ok(!lstrcmpA(org, "orange"), "Expected org to be unchanged, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", usersz);
    ok(orgsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH, "Expected MAX_PATH, got %lu\n", serialsz);

    /* RegOwner doesn't exist, lpUserNameBuf and pcchUserNameBuf are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    /* RegOwner, RegCompany don't exist, out params are NULL */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, NULL, NULL, NULL, NULL, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegOwner value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, ""), "Expected empty string, got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 0, "Expected 0, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "RegCompany", 0, REG_SZ, (LPBYTE)"company", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* RegCompany value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_ABSENT,
       "Expected USERINFOSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "banana"), "Expected serial to be unchanged, got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == MAX_PATH - 1, "Expected MAX_PATH - 1, got %lu\n", serialsz);

    res = RegSetValueExA(props, "ProductID", 0, REG_SZ, (LPBYTE)"ID", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ProductID value exists */
    INIT_USERINFO();
    state = MsiGetUserInfoA(prodcode, user, &usersz, org, &orgsz, serial, &serialsz);
    ok(state == USERINFOSTATE_PRESENT,
       "Expected USERINFOSTATE_PRESENT, got %d\n", state);
    ok(!lstrcmpA(user, "owner"), "Expected \"owner\", got \"%s\"\n", user);
    ok(!lstrcmpA(org, "company"), "Expected \"company\", got \"%s\"\n", org);
    ok(!lstrcmpA(serial, "ID"), "Expected \"ID\", got \"%s\"\n", serial);
    ok(usersz == 5, "Expected 5, got %lu\n", usersz);
    ok(orgsz == 7, "Expected 7, got %lu\n", orgsz);
    ok(serialsz == 2, "Expected 2, got %lu\n", serialsz);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userprod);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiOpenProduct(void)
{
    MSIHANDLE hprod, hdb;
    CHAR val[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    HKEY prodkey, userkey, props;
    LPSTR usersid;
    DWORD size;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    GetCurrentDirectoryA(MAX_PATH, path);
    lstrcatA(path, "\\");

    create_test_guid(prodcode, prod_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    hdb = create_package_db(prodcode);
    MsiCloseHandle(hdb);

    /* NULL szProduct */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(NULL, &hprod);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* empty szProduct */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA("", &hprod);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* garbage szProduct */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA("garbage", &hprod);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* guid without brackets */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", &hprod);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* guid with brackets */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* same length as guid, but random */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", &hprod);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* hProduct is NULL */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    res = RegCreateKeyExA(userkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(val, path);
    lstrcatA(val, "\\winetest.msi");
    res = RegSetValueExA(props, "ManagedLocalPackage", 0, REG_SZ,
                         (const BYTE *)val, lstrlenA(val) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ManagedLocalPackage value exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(hprod != 0 && hprod != 0xdeadbeef, "Expected a valid product handle\n");

    size = MAX_PATH;
    r = MsiGetPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode), "Expected %d, got %lu\n", lstrlenA(prodcode), size);

    MsiCloseHandle(hprod);

    RegDeleteValueA(props, "ManagedLocalPackage");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* unmanaged product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    res = RegCreateKeyExA(userkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(val, path);
    lstrcatA(val, "\\winetest.msi");
    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)val, lstrlenA(val) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(hprod != 0 && hprod != 0xdeadbeef, "Expected a valid product handle\n");

    size = MAX_PATH;
    r = MsiGetPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode), "Expected %d, got %lu\n", lstrlenA(prodcode), size);

    MsiCloseHandle(hprod);

    RegDeleteValueA(props, "LocalPackage");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* user product key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    res = RegCreateKeyExA(userkey, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    lstrcpyA(val, path);
    lstrcatA(val, "\\winetest.msi");
    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)val, lstrlenA(val) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage value exists */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(hprod != 0 && hprod != 0xdeadbeef, "Expected a valid product handle\n");

    size = MAX_PATH;
    r = MsiGetPropertyA(hprod, "ProductCode", val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, val);
    ok(size == lstrlenA(prodcode), "Expected %d, got %lu\n", lstrlenA(prodcode), size);

    MsiCloseHandle(hprod);

    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"winetest.msi", 13);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(val, path);
    lstrcatA(val, "\\winetest.msi");
    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)val, lstrlenA(val) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    DeleteFileA(msifile);

    /* local package does not exist */
    hprod = 0xdeadbeef;
    r = MsiOpenProductA(prodcode, &hprod);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(hprod == 0xdeadbeef, "Expected hprod to be unchanged\n");

    RegDeleteValueA(props, "LocalPackage");
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    DeleteFileA(msifile);
    LocalFree(usersid);
}

static void test_MsiEnumPatchesEx_usermanaged(LPCSTR usersid, LPCSTR expectedsid)
{
    MSIINSTALLCONTEXT context;
    CHAR keypath[MAX_PATH], patch[MAX_PATH];
    CHAR patch_squashed[MAX_PATH], patchcode[MAX_PATH];
    CHAR targetsid[MAX_PATH], targetprod[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    HKEY prodkey, patches, udprod, udpatch, hpatch;
    DWORD size, data;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(patch, patch_squashed);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* MSIPATCHSTATE_APPLIED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED,
                          MSIPATCHSTATE_APPLIED, 0, patchcode, targetprod,
                          &context, targetsid, &size);
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, expectedsid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = '\0';
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ,
                         (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch squashed value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* increase the index */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          1, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* increase again */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          2, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szPatchCode is NULL */
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, NULL, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* szTargetProductCode is NULL */
    lstrcpyA(patchcode, "apple");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, NULL, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* pdwTargetProductContext is NULL */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, NULL, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* szTargetUserSid is NULL */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(size == lstrlenA(expectedsid) * sizeof(WCHAR), "Expected %d*sizeof(WCHAR), got %lu\n", lstrlenA(expectedsid), size);

    /* pcchTargetUserSid is exactly the length of szTargetUserSid */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = lstrlenA(expectedsid);
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!strncmp(targetsid, expectedsid, lstrlenA(expectedsid) - 1), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid) * sizeof(WCHAR), "Expected %d*sizeof(WCHAR), got %lu\n", lstrlenA(expectedsid), size);

    /* pcchTargetUserSid has enough room for NULL terminator */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = lstrlenA(expectedsid) + 1;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* both szTargetuserSid and pcchTargetUserSid are NULL */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);

    /* MSIPATCHSTATE_SUPERSEDED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, expectedsid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udpatch, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* specific UserData patch key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_SUPERSEDED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* MSIPATCHSTATE_OBSOLETED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_OBSOLETED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value is obsoleted */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* MSIPATCHSTATE_REGISTERED */
    /* FIXME */

    /* MSIPATCHSTATE_ALL */

    /* 1st */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERMANAGED, "Expected MSIINSTALLCONTEXT_USERMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* same patch in multiple places, only one is enumerated */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, MSIPATCHSTATE_ALL,
                          1, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    RegDeleteValueA(hpatch, "State");
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);
    RegDeleteValueA(patches, "Patches");
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
}

static void test_MsiEnumPatchesEx_userunmanaged(LPCSTR usersid, LPCSTR expectedsid)
{
    MSIINSTALLCONTEXT context;
    CHAR keypath[MAX_PATH], patch[MAX_PATH];
    CHAR patch_squashed[MAX_PATH], patchcode[MAX_PATH];
    CHAR targetsid[MAX_PATH], targetprod[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    HKEY prodkey, patches, udprod, udpatch;
    HKEY userkey, hpatch;
    DWORD size, data;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(patch, patch_squashed);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* MSIPATCHSTATE_APPLIED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* current user product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyA(prodkey, "Patches", &patches);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = 0;
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ,
                         (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch code value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, expectedsid);
    lstrcatA(keypath, "\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* userdata patch key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERUNMANAGED, "Expected MSIINSTALLCONTEXT_USERUNMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* MSIPATCHSTATE_SUPERSEDED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, expectedsid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udpatch, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* specific UserData patch key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_SUPERSEDED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERUNMANAGED, "Expected MSIINSTALLCONTEXT_USERUNMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* MSIPATCHSTATE_OBSOLETED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_OBSOLETED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value is obsoleted */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERUNMANAGED, "Expected MSIINSTALLCONTEXT_USERUNMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* MSIPATCHSTATE_REGISTERED */
    /* FIXME */

    /* MSIPATCHSTATE_ALL */

    /* 1st */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_USERUNMANAGED, "Expected MSIINSTALLCONTEXT_USERUNMANAGED, got %d\n", context);
    ok(!lstrcmpA(targetsid, expectedsid), "Expected \"%s\", got \"%s\"\n", expectedsid, targetsid);
    ok(size == lstrlenA(expectedsid), "Expected %d, got %lu\n", lstrlenA(expectedsid), size);

    /* same patch in multiple places, only one is enumerated */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          1, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    RegDeleteValueA(hpatch, "State");
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);
    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteValueA(patches, patch_squashed);
    RegDeleteValueA(patches, "Patches");

error:
    RegDeleteKeyA(patches, "");
    RegCloseKey(patches);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
}

static void test_MsiEnumPatchesEx_machine(void)
{
    CHAR keypath[MAX_PATH], patch[MAX_PATH];
    CHAR patch_squashed[MAX_PATH], patchcode[MAX_PATH];
    CHAR targetsid[MAX_PATH], targetprod[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    HKEY prodkey, patches, udprod, udpatch;
    HKEY hpatch;
    MSIINSTALLCONTEXT context;
    DWORD size, data;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(patch, patch_squashed);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* MSIPATCHSTATE_APPLIED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                         0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = '\0';
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ,
                         (const BYTE *)patch_squashed,
                         lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ,
                         (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch code value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        goto done;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData product key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData Patches key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegCreateKeyExA(udpatch, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData Product patch key exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_APPLIED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_APPLIED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* MSIPATCHSTATE_SUPERSEDED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_SUPERSEDED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value is MSIPATCHSTATE_SUPERSEDED */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_SUPERSEDED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* MSIPATCHSTATE_OBSOLETED */

    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_OBSOLETED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD,
                         (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value is obsoleted */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_OBSOLETED,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* MSIPATCHSTATE_REGISTERED */
    /* FIXME */

    /* MSIPATCHSTATE_ALL */

    /* 1st */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patchcode, patch), "Expected \"%s\", got \"%s\"\n", patch, patchcode);
    ok(!lstrcmpA(targetprod, prodcode), "Expected \"%s\", got \"%s\"\n", prodcode, targetprod);
    ok(context == MSIINSTALLCONTEXT_MACHINE, "Expected MSIINSTALLCONTEXT_MACHINE, got %d\n", context);
    ok(!lstrcmpA(targetsid, ""), "Expected \"\", got \"%s\"\n", targetsid);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* same patch in multiple places, only one is enumerated */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_ALL,
                          1, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteValueA(hpatch, "State");
    RegCloseKey(hpatch);
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);

done:
    RegDeleteValueA(patches, patch_squashed);
    RegDeleteValueA(patches, "Patches");
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
}

static void test_MsiEnumPatchesEx(void)
{
    CHAR targetsid[MAX_PATH], targetprod[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    CHAR patchcode[MAX_PATH];
    MSIINSTALLCONTEXT context;
    LPSTR usersid;
    DWORD size;
    UINT r;

    create_test_guid(prodcode, prod_squashed);
    usersid = get_user_sid();

    /* empty szProductCode */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* garbage szProductCode */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid without brackets */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                          MSIPATCHSTATE_ALL, 0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid with brackets */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA("{6700E8CF-95AB-4D9C-BC2C-15840DDA7A5D}", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                          MSIPATCHSTATE_ALL, 0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szUserSid is S-1-5-18 */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, "S-1-5-18", MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is MSIINSTALLCONTEXT_MACHINE, but szUserSid is non-NULL */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_MACHINE, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is out of bounds */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, 0, MSIPATCHSTATE_ALL, 0, patchcode, targetprod,
                          &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is out of bounds */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_ALL + 1, MSIPATCHSTATE_ALL, 0, patchcode,
                          targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwFilter is out of bounds */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_INVALID,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwFilter is out of bounds */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    size = MAX_PATH;
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL + 1,
                          0, patchcode, targetprod, &context, targetsid, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* pcchTargetUserSid is NULL while szTargetUserSid is non-NULL */
    lstrcpyA(patchcode, "apple");
    lstrcpyA(targetprod, "banana");
    context = 0xdeadbeef;
    lstrcpyA(targetsid, "kiwi");
    r = MsiEnumPatchesExA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, MSIPATCHSTATE_ALL,
                          0, patchcode, targetprod, &context, targetsid, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patchcode, "apple"), "Expected patchcode to be unchanged, got %s\n", patchcode);
    ok(!lstrcmpA(targetprod, "banana"), "Expected targetprod to be unchanged, got %s\n", targetprod);
    ok(context == 0xdeadbeef, "Expected context to be unchanged, got %d\n", context);
    ok(!lstrcmpA(targetsid, "kiwi"), "Expected targetsid to be unchanged, got %s\n", targetsid);

    test_MsiEnumPatchesEx_usermanaged(usersid, usersid);
    test_MsiEnumPatchesEx_usermanaged(NULL, usersid);
    test_MsiEnumPatchesEx_usermanaged("S-1-2-34", "S-1-2-34");
    test_MsiEnumPatchesEx_userunmanaged(usersid, usersid);
    test_MsiEnumPatchesEx_userunmanaged(NULL, usersid);
    /* FIXME: Successfully test userunmanaged with a different user */
    test_MsiEnumPatchesEx_machine();
    LocalFree(usersid);
}

static void test_MsiEnumPatches(void)
{
    CHAR keypath[MAX_PATH], patch[MAX_PATH];
    CHAR patchcode[MAX_PATH], patch_squashed[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    CHAR transforms[MAX_PATH];
    WCHAR patchW[MAX_PATH], prodcodeW[MAX_PATH], transformsW[MAX_PATH];
    HKEY prodkey, patches, udprod;
    HKEY userkey, hpatch, udpatch;
    DWORD size, data;
    LPSTR usersid;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(patchcode, patch_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szProduct */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(NULL, 0, patch, transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* empty szProduct */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA("", 0, patch, transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* garbage szProduct */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA("garbage", 0, patch, transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid without brackets */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", 0, patch,
                        transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid with brackets */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", 0, patch,
                        transforms, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* same length as guid, but random */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", 0, patch,
                        transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"),
       "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"),
       "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patches key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = '\0';
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ, (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch squashed value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    /* lpPatchBuf is NULL */
    size = MAX_PATH;
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, NULL, transforms, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* lpTransformsBuf is NULL, pcchTransformsBuf is not */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    r = MsiEnumPatchesA(prodcode, 0, patch, NULL, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* pcchTransformsBuf is NULL, lpTransformsBuf is not */
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);

    /* pcchTransformsBuf is too small */
    size = 6;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whate"), "Expected \"whate\", got \"%s\"\n", transforms);
    ok(size == 8 || size == 16, "Expected 8 or 16, got %lu\n", size);

    /* increase the index */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 1, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* increase again */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 2, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    RegDeleteValueA(patches, "Patches");
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* current user product key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyA(prodkey, "Patches", &patches);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = '\0';
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ, (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch code value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &userkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* userdata patch key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    RegDeleteKeyExA(userkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(userkey);
    RegDeleteValueA(patches, patch_squashed);
    RegDeleteValueA(patches, "Patches");
    RegDeleteKeyA(patches, "");
    RegCloseKey(patches);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local product key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not REG_MULTI_SZ */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)"a\0b\0c\0\0", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists, is not a squashed guid */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    patch_squashed[lstrlenA(patch_squashed) + 1] = '\0';
    res = RegSetValueExA(patches, "Patches", 0, REG_MULTI_SZ, (const BYTE *)patch_squashed, lstrlenA(patch_squashed) + 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(patches, patch_squashed, 0, REG_SZ, (const BYTE *)"whatever", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* patch code value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData product key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData Patches key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    res = RegCreateKeyExA(udpatch, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData Product patch key exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(patch, "apple"), "Expected lpPatchBuf to be unchanged, got \"%s\"\n", patch);
    ok(!lstrcmpA(transforms, "banana"), "Expected lpTransformsBuf to be unchanged, got \"%s\"\n", transforms);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    data = MSIPATCHSTATE_APPLIED;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    size = MAX_PATH;
    lstrcpyA(patch, "apple");
    lstrcpyA(transforms, "banana");
    r = MsiEnumPatchesA(prodcode, 0, patch, transforms, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    /* now duplicate some of the tests for the W version */

    /* pcchTransformsBuf is too small */
    size = 6;
    MultiByteToWideChar( CP_ACP, 0, prodcode, -1, prodcodeW, MAX_PATH );
    MultiByteToWideChar( CP_ACP, 0, "apple", -1, patchW, MAX_PATH );
    MultiByteToWideChar( CP_ACP, 0, "banana", -1, transformsW, MAX_PATH );
    r = MsiEnumPatchesW(prodcodeW, 0, patchW, transformsW, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    WideCharToMultiByte( CP_ACP, 0, patchW, -1, patch, MAX_PATH, NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, transformsW, -1, transforms, MAX_PATH, NULL, NULL );
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whate"), "Expected \"whate\", got \"%s\"\n", transforms);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* patch code value exists */
    size = MAX_PATH;
    MultiByteToWideChar( CP_ACP, 0, "apple", -1, patchW, MAX_PATH );
    MultiByteToWideChar( CP_ACP, 0, "banana", -1, transformsW, MAX_PATH );
    r = MsiEnumPatchesW(prodcodeW, 0, patchW, transformsW, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    WideCharToMultiByte( CP_ACP, 0, patchW, -1, patch, MAX_PATH, NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, transformsW, -1, transforms, MAX_PATH, NULL, NULL );
    ok(!lstrcmpA(patch, patchcode), "Expected \"%s\", got \"%s\"\n", patchcode, patch);
    ok(!lstrcmpA(transforms, "whatever"), "Expected \"whatever\", got \"%s\"\n", transforms);
    ok(size == 8 || size == MAX_PATH, "Expected 8 or MAX_PATH, got %lu\n", size);

    RegDeleteValueA(patches, patch_squashed);
    RegDeleteValueA(patches, "Patches");
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteValueA(hpatch, "State");
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);
    LocalFree(usersid);
}

static void test_MsiGetPatchInfoEx(void)
{
    CHAR keypath[MAX_PATH], val[MAX_PATH];
    CHAR patchcode[MAX_PATH], patch_squashed[MAX_PATH];
    CHAR prodcode[MAX_PATH], prod_squashed[MAX_PATH];
    HKEY prodkey, patches, udprod, props;
    HKEY hpatch, udpatch, prodpatches;
    LPSTR usersid;
    DWORD size;
    LONG res;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(patchcode, patch_squashed);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    /* NULL szPatchCode */
    lstrcpyA(val, "apple");
    size = MAX_PATH;
    r = MsiGetPatchInfoExA(NULL, prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* empty szPatchCode */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA("", prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* garbage szPatchCode */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA("garbage", prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid without brackets */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid with brackets */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* same length as guid, but random */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* NULL szProductCode */
    lstrcpyA(val, "apple");
    size = MAX_PATH;
    r = MsiGetPatchInfoExA(patchcode, NULL, NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* empty szProductCode */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, "", NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* garbage szProductCode */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, "garbage", NULL, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid without brackets */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, "6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* guid with brackets */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, "{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* same length as guid, but random */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, "A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", NULL, MSIINSTALLCONTEXT_USERMANAGED,
                           INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szUserSid cannot be S-1-5-18 for MSIINSTALLCONTEXT_USERMANAGED */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, "S-1-5-18", MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szUserSid cannot be S-1-5-18 for MSIINSTALLCONTEXT_USERUNMANAGED */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, "S-1-5-18", MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szUserSid cannot be S-1-5-18 for MSIINSTALLCONTEXT_MACHINE */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, "S-1-5-18", MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* szUserSid must be NULL for MSIINSTALLCONTEXT_MACHINE */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is out of range */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_NONE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is out of range */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_ALL, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* dwContext is invalid */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, 3, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCHA, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(patches, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* managed product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &prodpatches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(prodpatches, patch_squashed, 0, REG_SZ, (const BYTE *)"transforms", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* specific patch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegSetValueExA(udpatch, "ManagedLocalPackage", 0, REG_SZ, (const BYTE *)"pack", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* ManagedLocalPatch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "transforms"), "Expected \"transforms\", got \"%s\"\n", val);
    ok(size == 10, "Expected 10, got %lu\n", size);

    res = RegSetValueExA(hpatch, "Installed", 0, REG_SZ, (const BYTE *)"mydate", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Installed value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_INSTALLDATEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "mydate"), "Expected \"mydate\", got \"%s\"\n", val);
    ok(size == 6, "Expected 6, got %lu\n", size);

    res = RegSetValueExA(hpatch, "Uninstallable", 0, REG_SZ, (const BYTE *)"yes", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Uninstallable value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_UNINSTALLABLEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "yes"), "Expected \"yes\", got \"%s\"\n", val);
    ok(size == 3, "Expected 3, got %lu\n", size);

    res = RegSetValueExA(hpatch, "State", 0, REG_SZ, (const BYTE *)"good", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PATCHSTATEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "good"), "Expected \"good\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    size = 1;
    res = RegSetValueExA(hpatch, "State", 0, REG_DWORD, (const BYTE *)&size, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* State value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_PATCHSTATEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "1"), "Expected \"1\", got \"%s\"\n", val);
    ok(size == 1, "Expected 1, got %lu\n", size);

    size = 1;
    res = RegSetValueExA(hpatch, "Uninstallable", 0, REG_DWORD, (const BYTE *)&size, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Uninstallable value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_UNINSTALLABLEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "1"), "Expected \"1\", got \"%s\"\n", val);
    ok(size == 1, "Expected 1, got %lu\n", size);

    res = RegSetValueExA(hpatch, "DisplayName", 0, REG_SZ, (const BYTE *)"display", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* DisplayName value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_DISPLAYNAMEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "display"), "Expected \"display\", got \"%s\"\n", val);
    ok(size == 7, "Expected 7, got %lu\n", size);

    res = RegSetValueExA(hpatch, "MoreInfoURL", 0, REG_SZ, (const BYTE *)"moreinfo", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* MoreInfoURL value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "moreinfo"), "Expected \"moreinfo\", got \"%s\"\n", val);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* szProperty is invalid */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, "IDontExist", val, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected \"apple\", got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* lpValue is NULL, while pcchValue is non-NULL */
    size = MAX_PATH;
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 16, "Expected 16, got %lu\n", size);

    /* pcchValue is NULL, while lpValue is non-NULL */
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           val, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected \"apple\", got \"%s\"\n", val);

    /* both lpValue and pcchValue are NULL */
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* pcchValue doesn't have enough room for NULL terminator */
    size = 8;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           val, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(val, "moreinf"), "Expected \"moreinf\", got \"%s\"\n", val);
    ok(size == 16, "Expected 16, got %lu\n", size);

    /* pcchValue has exactly enough room for NULL terminator */
    size = 9;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "moreinfo"), "Expected \"moreinfo\", got \"%s\"\n", val);
    ok(size == 8, "Expected 8, got %lu\n", size);

    /* pcchValue is too small, lpValue is NULL */
    size = 0;
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_MOREINFOURLA,
                           NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 16, "Expected 16, got %lu\n", size);

    RegDeleteValueA(prodpatches, patch_squashed);
    RegDeleteKeyExA(prodpatches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodpatches);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected \"apple\", got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    RegDeleteValueA(hpatch, "MoreInfoURL");
    RegDeleteValueA(hpatch, "Display");
    RegDeleteValueA(hpatch, "State");
    RegDeleteValueA(hpatch, "Uninstallable");
    RegDeleteValueA(hpatch, "Installed");
    RegDeleteValueA(udpatch, "ManagedLocalPackage");
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(patches, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* current user product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyA(prodkey, "Patches", &prodpatches);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(prodpatches, patch_squashed, 0, REG_SZ,
                         (const BYTE *)"transforms", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* specific patch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegSetValueExA(udpatch, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"pack", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPatch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "transforms"), "Expected \"transforms\", got \"%s\"\n", val);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(prodpatches, patch_squashed);
    RegDeleteKeyExA(prodpatches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodpatches);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected \"apple\", got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    RegDeleteValueA(udpatch, "LocalPackage");
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);

    /* MSIINSTALLCONTEXT_MACHINE */

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer");
    lstrcatA(keypath, "\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udprod, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local UserData product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "InstallProperties", 0, NULL, 0, access, NULL, &props, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* InstallProperties key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(udprod, "Patches", 0, NULL, 0, access, NULL, &patches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(patches, patch_squashed, 0, NULL, 0, access, NULL, &hpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &prodkey, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* local product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegCreateKeyExA(prodkey, "Patches", 0, NULL, 0, access, NULL, &prodpatches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    res = RegSetValueExA(prodpatches, patch_squashed, 0, REG_SZ,
                         (const BYTE *)"transforms", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* specific patch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer");
    lstrcatA(keypath, "\\UserData\\S-1-5-18\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &udpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, ""), "Expected \"\", got \"%s\"\n", val);
    ok(size == 0, "Expected 0, got %lu\n", size);

    res = RegSetValueExA(udpatch, "LocalPackage", 0, REG_SZ, (const BYTE *)"pack", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPatch value exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "transforms"), "Expected \"transforms\", got \"%s\"\n", val);
    ok(size == 10, "Expected 10, got %lu\n", size);

    RegDeleteValueA(prodpatches, patch_squashed);
    RegDeleteKeyExA(prodpatches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodpatches);
    RegDeleteKeyExA(prodkey, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(prodkey);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_LOCALPACKAGEA,
                           val, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(val, "pack"), "Expected \"pack\", got \"%s\"\n", val);
    ok(size == 4, "Expected 4, got %lu\n", size);

    /* UserData is sufficient for all properties
     * except INSTALLPROPERTY_TRANSFORMS
     */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoExA(patchcode, prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_TRANSFORMSA,
                           val, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(!lstrcmpA(val, "apple"), "Expected \"apple\", got \"%s\"\n", val);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    RegDeleteValueA(udpatch, "LocalPackage");
    RegDeleteKeyExA(udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udpatch);
    RegDeleteKeyExA(hpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hpatch);
    RegDeleteKeyExA(patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(props, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(props);
    RegDeleteKeyExA(udprod, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(udprod);
    LocalFree(usersid);
}

static void test_MsiGetPatchInfo(void)
{
    UINT r;
    char prod_code[MAX_PATH], prod_squashed[MAX_PATH], val[MAX_PATH];
    char patch_code[MAX_PATH], patch_squashed[MAX_PATH], keypath[MAX_PATH];
    WCHAR valW[MAX_PATH], patch_codeW[MAX_PATH];
    HKEY hkey_product, hkey_patch, hkey_patches, hkey_udprops, hkey_udproduct;
    HKEY hkey_udpatch, hkey_udpatches, hkey_udproductpatches, hkey_udproductpatch;
    DWORD size;
    LONG res;
    REGSAM access = KEY_ALL_ACCESS;

    create_test_guid(patch_code, patch_squashed);
    create_test_guid(prod_code, prod_squashed);
    MultiByteToWideChar(CP_ACP, 0, patch_code, -1, patch_codeW, MAX_PATH);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    r = MsiGetPatchInfoA(NULL, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiGetPatchInfoA(patch_code, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);

    size = 0;
    r = MsiGetPatchInfoA(patch_code, NULL, NULL, &size);
    ok(r == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiGetPatchInfoA(patch_code, "", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "expected ERROR_UNKNOWN_PROPERTY, got %u\n", r);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey_product, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    res = RegCreateKeyExA(hkey_product, "Patches", 0, NULL, 0, access, NULL, &hkey_patches, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    res = RegCreateKeyExA(hkey_patches, patch_squashed, 0, NULL, 0, access, NULL, &hkey_patch, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* patch key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer");
    lstrcatA(keypath, "\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey_udproduct, NULL);
    if (res == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        goto done;
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS got %ld\n", res);

    /* UserData product key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    res = RegCreateKeyExA(hkey_udproduct, "InstallProperties", 0, NULL, 0, access, NULL, &hkey_udprops, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* InstallProperties key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged, got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    res = RegCreateKeyExA(hkey_udproduct, "Patches", 0, NULL, 0, access, NULL, &hkey_udpatches, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* UserData Patches key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    res = RegCreateKeyExA(hkey_udproduct, "Patches", 0, NULL, 0, access, NULL, &hkey_udproductpatches, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegCreateKeyExA(hkey_udproductpatches, patch_squashed, 0, NULL, 0, access, NULL, &hkey_udproductpatch, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* UserData product patch key exists */
    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected val to be unchanged got \"%s\"\n", val);
    ok(size == MAX_PATH, "expected size to be unchanged got %lu\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer");
    lstrcatA(keypath, "\\UserData\\S-1-5-18\\Patches\\");
    lstrcatA(keypath, patch_squashed);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, NULL, 0, access, NULL, &hkey_udpatch, NULL);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    res = RegSetValueExA(hkey_udpatch, "LocalPackage", 0, REG_SZ, (const BYTE *)"c:\\test.msp", 12);
    ok(res == ERROR_SUCCESS, "expected ERROR_SUCCESS got %ld\n", res);

    /* UserData Patch key exists */
    size = 0;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", r);
    ok(!lstrcmpA(val, "apple"), "expected \"apple\", got \"%s\"\n", val);
    ok(size == 11, "expected 11 got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(val, "apple");
    r = MsiGetPatchInfoA(patch_code, INSTALLPROPERTY_LOCALPACKAGEA, val, &size);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS got %u\n", r);
    ok(!lstrcmpA(val, "c:\\test.msp"), "expected \"c:\\test.msp\", got \"%s\"\n", val);
    ok(size == 11, "expected 11 got %lu\n", size);

    size = 0;
    valW[0] = 0;
    r = MsiGetPatchInfoW(patch_codeW, INSTALLPROPERTY_LOCALPACKAGEW, valW, &size);
    ok(r == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", r);
    ok(!valW[0], "expected 0 got %u\n", valW[0]);
    ok(size == 11, "expected 11 got %lu\n", size);

    size = MAX_PATH;
    valW[0] = 0;
    r = MsiGetPatchInfoW(patch_codeW, INSTALLPROPERTY_LOCALPACKAGEW, valW, &size);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS got %u\n", r);
    ok(valW[0], "expected > 0 got %u\n", valW[0]);
    ok(size == 11, "expected 11 got %lu\n", size);

    RegDeleteKeyExA(hkey_udproductpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udproductpatch);
    RegDeleteKeyExA(hkey_udproductpatches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udproductpatches);
    RegDeleteKeyExA(hkey_udpatch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udpatch);
    RegDeleteKeyExA(hkey_udpatches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udpatches);
    RegDeleteKeyExA(hkey_udprops, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udprops);
    RegDeleteKeyExA(hkey_udproduct, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_udproduct);

done:
    RegDeleteKeyExA(hkey_patches, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_patches);
    RegDeleteKeyExA(hkey_product, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_product);
    RegDeleteKeyExA(hkey_patch, "", access & KEY_WOW64_64KEY, 0);
    RegCloseKey(hkey_patch);
}

static void test_MsiEnumProducts(void)
{
    UINT r;
    BOOL found1, found2, found3;
    DWORD index;
    char product1[39], product2[39], product3[39], guid[39];
    char product_squashed1[33], product_squashed2[33], product_squashed3[33];
    char keypath1[MAX_PATH], keypath2[MAX_PATH], keypath3[MAX_PATH];
    char *usersid;
    HKEY key1, key2, key3;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip( "process is limited\n" );
        return;
    }

    create_test_guid(product1, product_squashed1);
    create_test_guid(product2, product_squashed2);
    create_test_guid(product3, product_squashed3);
    usersid = get_user_sid();

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    strcpy(keypath2, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    strcat(keypath2, usersid);
    strcat(keypath2, "\\Installer\\Products\\");
    strcat(keypath2, product_squashed2);

    r = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath2, 0, NULL, 0, access, NULL, &key2, NULL);
    if (r == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        LocalFree(usersid);
        return;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    strcpy(keypath1, "Software\\Classes\\Installer\\Products\\");
    strcat(keypath1, product_squashed1);

    r = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath1, 0, NULL, 0, access, NULL, &key1, NULL);
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        LocalFree( usersid );
        return;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    strcpy(keypath3, "Software\\Microsoft\\Installer\\Products\\");
    strcat(keypath3, product_squashed3);

    r = RegCreateKeyA(HKEY_CURRENT_USER, keypath3, &key3);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    index = 0;
    r = MsiEnumProductsA(index, guid);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiEnumProductsA(index, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);

    index = 2;
    r = MsiEnumProductsA(index, guid);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", r);

    index = 0;
    r = MsiEnumProductsA(index, guid);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    found1 = found2 = found3 = FALSE;
    while ((r = MsiEnumProductsA(index, guid)) == ERROR_SUCCESS)
    {
        if (!strcmp(product1, guid)) found1 = TRUE;
        if (!strcmp(product2, guid)) found2 = TRUE;
        if (!strcmp(product3, guid)) found3 = TRUE;
        if (found1 && found2 && found3) break;
        index++;
    }
    ok(found1, "product1 not found\n");
    ok(found2, "product2 not found\n");
    ros_skip_flaky
    ok(found3, "product3 not found\n");

    RegDeleteKeyExA(key1, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyExA(key2, "", access & KEY_WOW64_64KEY, 0);
    RegDeleteKeyA(key3, "");
    RegCloseKey(key1);
    RegCloseKey(key2);
    RegCloseKey(key3);
    LocalFree(usersid);
}

static void test_MsiGetFileSignatureInformation(void)
{
    HRESULT hr;
    const CERT_CONTEXT *cert;
    DWORD len;

    hr = MsiGetFileSignatureInformationA( NULL, 0, NULL, NULL, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( NULL, 0, NULL, NULL, &len );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( NULL, 0, &cert, NULL, &len );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( "", 0, NULL, NULL, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, NULL, NULL, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, NULL, NULL, &len );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, &cert, NULL, &len );
    todo_wine ok(hr == CRYPT_E_FILE_ERROR, "expected CRYPT_E_FILE_ERROR got %#lx\n", hr);

    create_file_data( "signature.bin", "signature", sizeof("signature") );

    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, NULL, NULL, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, NULL, NULL, &len );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %#lx\n", hr);

    cert = (const CERT_CONTEXT *)0xdeadbeef;
    hr = MsiGetFileSignatureInformationA( "signature.bin", 0, &cert, NULL, &len );
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_FUNCTION_FAILED), "got %#lx\n", hr);
    ok(cert == NULL, "got %p\n", cert);

    DeleteFileA( "signature.bin" );
}

static void test_MsiEnumProductsEx(void)
{
    UINT r;
    DWORD len, index;
    MSIINSTALLCONTEXT context;
    char product0[39], product1[39], product2[39], product3[39], guid[39], sid[128];
    char product_squashed1[33], product_squashed2[33], product_squashed3[33];
    char keypath1[MAX_PATH], keypath2[MAX_PATH], keypath3[MAX_PATH];
    HKEY key1 = NULL, key2 = NULL, key3 = NULL;
    REGSAM access = KEY_ALL_ACCESS;
    char *usersid = get_user_sid();
    BOOL found1, found2, found3;

    create_test_guid( product0, NULL );
    create_test_guid( product1, product_squashed1 );
    create_test_guid( product2, product_squashed2 );
    create_test_guid( product3, product_squashed3 );

    if (is_wow64) access |= KEY_WOW64_64KEY;

    strcpy( keypath2, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\" );
    strcat( keypath2, usersid );
    strcat( keypath2, "\\Installer\\Products\\" );
    strcat( keypath2, product_squashed2 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath2, 0, NULL, 0, access, NULL, &key2, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    strcpy( keypath1, "Software\\Classes\\Installer\\Products\\" );
    strcat( keypath1, product_squashed1 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath1, 0, NULL, 0, access, NULL, &key1, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    strcpy( keypath3, usersid );
    strcat( keypath3, "\\Software\\Microsoft\\Installer\\Products\\" );
    strcat( keypath3, product_squashed3 );

    r = RegCreateKeyExA( HKEY_USERS, keypath3, 0, NULL, 0, access, NULL, &key3, NULL );
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    r = MsiEnumProductsExA( NULL, NULL, 0, 0, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "got %u\n", r );

    len = sizeof(sid);
    r = MsiEnumProductsExA( NULL, NULL, 0, 0, NULL, NULL, NULL, &len );
    ok( r == ERROR_INVALID_PARAMETER, "got %u\n", r );
    ok( len == sizeof(sid), "got %lu\n", len );

    r = MsiEnumProductsExA( NULL, NULL, MSIINSTALLCONTEXT_ALL, 0, NULL, NULL, NULL, NULL );
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    sid[0] = 0;
    len = sizeof(sid);
    r = MsiEnumProductsExA( product0, NULL, MSIINSTALLCONTEXT_ALL, 0, NULL, NULL, sid, &len );
    ok( r == ERROR_NO_MORE_ITEMS, "got %u\n", r );
    ok( len == sizeof(sid), "got %lu\n", len );
    ok( !sid[0], "got %s\n", sid );

    sid[0] = 0;
    len = sizeof(sid);
    r = MsiEnumProductsExA( product0, usersid, MSIINSTALLCONTEXT_ALL, 0, NULL, NULL, sid, &len );
    ok( r == ERROR_NO_MORE_ITEMS, "got %u\n", r );
    ok( len == sizeof(sid), "got %lu\n", len );
    ok( !sid[0], "got %s\n", sid );

    sid[0] = 0;
    len = 0;
    r = MsiEnumProductsExA( NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED, 0, NULL, NULL, sid, &len );
    ok( r == ERROR_MORE_DATA, "got %u\n", r );
    ok( len, "length unchanged\n" );
    ok( !sid[0], "got %s\n", sid );

    guid[0] = 0;
    context = 0xdeadbeef;
    sid[0] = 0;
    len = sizeof(sid);
    r = MsiEnumProductsExA( NULL, NULL, MSIINSTALLCONTEXT_ALL, 0, guid, &context, sid, &len );
    ok( r == ERROR_SUCCESS, "got %u\n", r );
    ok( guid[0], "empty guid\n" );
    ok( context != 0xdeadbeef, "context unchanged\n" );
    ok( !len, "got %lu\n", len );
    ok( !sid[0], "got %s\n", sid );

    guid[0] = 0;
    context = 0xdeadbeef;
    sid[0] = 0;
    len = sizeof(sid);
    r = MsiEnumProductsExA( NULL, usersid, MSIINSTALLCONTEXT_ALL, 0, guid, &context, sid, &len );
    ok( r == ERROR_SUCCESS, "got %u\n", r );
    ok( guid[0], "empty guid\n" );
    ok( context != 0xdeadbeef, "context unchanged\n" );
    ok( !len, "got %lu\n", len );
    ok( !sid[0], "got %s\n", sid );

    guid[0] = 0;
    context = 0xdeadbeef;
    sid[0] = 0;
    len = sizeof(sid);
    r = MsiEnumProductsExA( NULL, "S-1-1-0", MSIINSTALLCONTEXT_ALL, 0, guid, &context, sid, &len );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );
    ok( guid[0], "empty guid\n" );
    ok( context != 0xdeadbeef, "context unchanged\n" );
    ok( !len, "got %lu\n", len );
    ok( !sid[0], "got %s\n", sid );

    index = 0;
    guid[0] = 0;
    context = 0xdeadbeef;
    sid[0] = 0;
    len = sizeof(sid);
    found1 = found2 = found3 = FALSE;
    while (!MsiEnumProductsExA( NULL, "S-1-1-0", MSIINSTALLCONTEXT_ALL, index, guid, &context, sid, &len ))
    {
        if (!strcmp( product1, guid ))
        {
            ok( context == MSIINSTALLCONTEXT_MACHINE, "got %u\n", context );
            ok( !sid[0], "got \"%s\"\n", sid );
            ok( !len, "unexpected length %lu\n", len );
            found1 = TRUE;
        }
        if (!strcmp( product2, guid ))
        {
            ok( context == MSIINSTALLCONTEXT_USERMANAGED, "got %u\n", context );
            ok( sid[0], "empty sid\n" );
            ok( len == strlen(sid), "unexpected length %lu\n", len );
            found2 = TRUE;
        }
        if (!strcmp( product3, guid ))
        {
            ok( context == MSIINSTALLCONTEXT_USERUNMANAGED, "got %u\n", context );
            ok( sid[0], "empty sid\n" );
            ok( len == strlen(sid), "unexpected length %lu\n", len );
            found3 = TRUE;
        }
        if (found1 && found2 && found3) break;
        index++;
        guid[0] = 0;
        context = 0xdeadbeef;
        sid[0] = 0;
        len = sizeof(sid);
    }
    ok(found1, "product1 not found\n");
    ok(found2, "product2 not found\n");
    ok(found3, "product3 not found\n");

done:
    RegDeleteKeyExA( key1, "", access, 0 );
    RegDeleteKeyExA( key2, "", access, 0 );
    RegDeleteKeyExA( key3, "", access, 0 );
    RegCloseKey( key1 );
    RegCloseKey( key2 );
    RegCloseKey( key3 );
    LocalFree( usersid );
}

static void test_MsiEnumComponents(void)
{
    UINT r;
    BOOL found1, found2;
    DWORD index;
    char comp1[39], comp2[39], guid[39];
    char comp_squashed1[33], comp_squashed2[33];
    char keypath1[MAX_PATH], keypath2[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;
    char *usersid = get_user_sid();
    HKEY key1 = NULL, key2 = NULL;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid( comp1, comp_squashed1 );
    create_test_guid( comp2, comp_squashed2 );

    if (is_wow64) access |= KEY_WOW64_64KEY;

    strcpy( keypath1, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\" );
    strcat( keypath1, "S-1-5-18\\Components\\" );
    strcat( keypath1, comp_squashed1 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath1, 0, NULL, 0, access, NULL, &key1, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    strcpy( keypath2, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\" );
    strcat( keypath2, usersid );
    strcat( keypath2, "\\Components\\" );
    strcat( keypath2, comp_squashed2 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath2, 0, NULL, 0, access, NULL, &key2, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }

    r = MsiEnumComponentsA( 0, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "got %u\n", r );

    index = 0;
    guid[0] = 0;
    found1 = found2 = FALSE;
    while (!MsiEnumComponentsA( index, guid ))
    {
        if (!strcmp( guid, comp1 )) found1 = TRUE;
        if (!strcmp( guid, comp2 )) found2 = TRUE;
        ok( guid[0], "empty guid\n" );
        if (found1 && found2) break;
        guid[0] = 0;
        index++;
    }
    ok( found1, "comp1 not found\n" );
    ok( found2, "comp2 not found\n" );

done:
    RegDeleteKeyExA( key1, "", access, 0 );
    RegDeleteKeyExA( key2, "", access, 0 );
    RegCloseKey( key1 );
    RegCloseKey( key2 );
    LocalFree( usersid );
}

static void test_MsiEnumComponentsEx(void)
{
    UINT r;
    BOOL found1, found2;
    DWORD len, index;
    MSIINSTALLCONTEXT context;
    char comp1[39], comp2[39], guid[39], sid[128];
    char comp_squashed1[33], comp_squashed2[33];
    char keypath1[MAX_PATH], keypath2[MAX_PATH];
    HKEY key1 = NULL, key2 = NULL;
    REGSAM access = KEY_ALL_ACCESS;
    char *usersid = get_user_sid();

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_guid( comp1, comp_squashed1 );
    create_test_guid( comp2, comp_squashed2 );

    if (is_wow64) access |= KEY_WOW64_64KEY;

    strcpy( keypath1, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\" );
    strcat( keypath1, "S-1-5-18\\Components\\" );
    strcat( keypath1, comp_squashed1 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath1, 0, NULL, 0, access, NULL, &key1, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    strcpy( keypath2, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\" );
    strcat( keypath2, usersid );
    strcat( keypath2, "\\Components\\" );
    strcat( keypath2, comp_squashed2 );

    r = RegCreateKeyExA( HKEY_LOCAL_MACHINE, keypath2, 0, NULL, 0, access, NULL, &key2, NULL );
    if (r == ERROR_ACCESS_DENIED)
    {
        skip( "insufficient rights\n" );
        goto done;
    }
    ok( r == ERROR_SUCCESS, "got %u\n", r );
    r = RegSetValueExA( key2, comp_squashed2, 0, REG_SZ, (const BYTE *)"c:\\doesnotexist",
                        sizeof("c:\\doesnotexist"));
    ok( r == ERROR_SUCCESS, "got %u\n", r );

    index = 0;
    guid[0] = 0;
    context = 0xdeadbeef;
    sid[0] = 0;
    len = sizeof(sid);
    found1 = found2 = FALSE;
    while (!MsiEnumComponentsExA( "S-1-1-0", MSIINSTALLCONTEXT_ALL, index, guid, &context, sid, &len ))
    {
        if (!strcmp( comp1, guid ))
        {
            ok( context == MSIINSTALLCONTEXT_MACHINE, "got %u\n", context );
            ok( !sid[0], "got \"%s\"\n", sid );
            ok( !len, "unexpected length %lu\n", len );
            found1 = TRUE;
            if (found2) break;
        }
        if (!strcmp( comp2, guid ))
        {
            ok( context == MSIINSTALLCONTEXT_USERUNMANAGED, "got %u\n", context );
            ok( sid[0], "empty sid\n" );
            ok( len == strlen(sid), "unexpected length %lu\n", len );
            found2 = TRUE;
            if (found1) break;
        }
        index++;
        guid[0] = 0;
        context = 0xdeadbeef;
        sid[0] = 0;
        len = sizeof(sid);
    }
    ok( found1, "comp1 not found\n" );
    ok( found2, "comp2 not found\n" );

    r = MsiEnumComponentsExA( NULL, 0, 0, NULL, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "got %u\n", r );

    r = MsiEnumComponentsExA( NULL, MSIINSTALLCONTEXT_ALL, 0, NULL, NULL, sid, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "got %u\n", r );

done:
    RegDeleteValueA( key2, comp_squashed2 );
    RegDeleteKeyExA( key1, "", access, 0 );
    RegDeleteKeyExA( key2, "", access, 0 );
    RegCloseKey( key1 );
    RegCloseKey( key2 );
    LocalFree( usersid );
}

static void test_MsiConfigureProductEx(void)
{
    UINT r;
    LONG res;
    DWORD type, size;
    HKEY props, source;
    CHAR keypath[MAX_PATH * 2], localpackage[MAX_PATH], packagename[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file_data("msitest\\hydrogen", "hydrogen", 500);
    create_file_data("msitest\\helium", "helium", 500);
    create_file_data("msitest\\lithium", "lithium", 500);

    create_database(msifile, mcp_tables, ARRAY_SIZE(mcp_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* NULL szProduct */
    r = MsiConfigureProductExA(NULL, INSTALLLEVEL_DEFAULT,
                               INSTALLSTATE_DEFAULT, "PROPVAR=42");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProduct */
    r = MsiConfigureProductExA("", INSTALLLEVEL_DEFAULT,
                               INSTALLSTATE_DEFAULT, "PROPVAR=42");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProduct */
    r = MsiConfigureProductExA("garbage", INSTALLLEVEL_DEFAULT,
                               INSTALLSTATE_DEFAULT, "PROPVAR=42");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiConfigureProductExA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiConfigureProductExA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* same length as guid, but random */
    r = MsiConfigureProductExA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* product not installed yet */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* install the product, per-user unmanaged */
    r = MsiInstallProductA(msifile, "INSTALLLEVEL=10 PROPVAR=42");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* product is installed per-user managed, remove it */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest", FALSE), "Directory not removed\n");

    /* product has been removed */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);

    /* install the product, machine */
    r = MsiInstallProductA(msifile, "ALLUSERS=1 INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* product is installed machine, remove it */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest", FALSE), "Directory not removed\n");

    /* product has been removed */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_DEFAULT,
                               "PROPVAR=42");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);

    /* install the product, machine */
    r = MsiInstallProductA(msifile, "ALLUSERS=1 INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    DeleteFileA(msifile);

    /* msifile is removed */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest", FALSE), "Directory not removed\n");

    create_database(msifile, mcp_tables, ARRAY_SIZE(mcp_tables));

    /* install the product, machine */
    r = MsiInstallProductA(msifile, "ALLUSERS=1 INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    DeleteFileA(msifile);

    lstrcpyA(keypath, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, "83374883CBB1401418CAF2AA7CCEDDDC\\InstallProperties");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    type = REG_SZ;
    size = MAX_PATH;
    res = RegQueryValueExA(props, "LocalPackage", NULL, &type,
                           (LPBYTE)localpackage, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"C:\\idontexist.msi", 18);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* LocalPackage is used to find the cached msi package */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_INSTALL_SOURCE_ABSENT,
       "Expected ERROR_INSTALL_SOURCE_ABSENT, got %d\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    RegCloseKey(props);
    create_database(msifile, mcp_tables, ARRAY_SIZE(mcp_tables));

    /* LastUsedSource can be used as a last resort */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest", FALSE), "Directory not removed\n");
    DeleteFileA( localpackage );

    /* install the product, machine */
    r = MsiInstallProductA(msifile, "ALLUSERS=1 INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    lstrcpyA(keypath, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, "83374883CBB1401418CAF2AA7CCEDDDC\\InstallProperties");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    type = REG_SZ;
    size = MAX_PATH;
    res = RegQueryValueExA(props, "LocalPackage", NULL, &type,
                           (LPBYTE)localpackage, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"C:\\idontexist.msi", 18);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(keypath, "SOFTWARE\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, "83374883CBB1401418CAF2AA7CCEDDDC\\SourceList");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    type = REG_SZ;
    size = MAX_PATH;
    res = RegQueryValueExA(source, "PackageName", NULL, &type,
                           (LPBYTE)packagename, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(source, "PackageName", 0, REG_SZ,
                         (const BYTE *)"idontexist.msi", 15);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* SourceList is altered */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_INSTALL_SOURCE_ABSENT,
       "Expected ERROR_INSTALL_SOURCE_ABSENT, got %d\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* restore PackageName */
    res = RegSetValueExA(source, "PackageName", 0, REG_SZ,
                         (const BYTE *)packagename, lstrlenA(packagename) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* restore LocalPackage */
    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)localpackage, lstrlenA(localpackage) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    /* finally remove the product */
    r = MsiConfigureProductExA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest", FALSE), "Directory not removed\n");

    RegCloseKey(source);
    RegCloseKey(props);

error:
    DeleteFileA("msitest\\hydrogen");
    DeleteFileA("msitest\\helium");
    DeleteFileA("msitest\\lithium");
    RemoveDirectoryA("msitest");
    DeleteFileA(msifile);
}

static void test_MsiSetFeatureAttributes(void)
{
    UINT r;
    DWORD attrs;
    char path[MAX_PATH];
    MSIHANDLE package;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }
    create_database( msifile, tables, ARRAY_SIZE( tables ));

    strcpy( path, CURR_DIR );
    strcat( path, "\\" );
    strcat( path, msifile );

    r = MsiOpenPackageA( path, &package );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA( msifile );
        return;
    }
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, "One", INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %u\n", r);

    r = MsiDoActionA( package, "CostInitialize" );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSetFeatureAttributesA( 0, "One", INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, "", INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_UNKNOWN_FEATURE, "expected ERROR_UNKNOWN_FEATURE, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, NULL, INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_UNKNOWN_FEATURE, "expected ERROR_UNKNOWN_FEATURE, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, "One", 0 );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    attrs = 0xdeadbeef;
    r = MsiGetFeatureInfoA( package, "One", &attrs, NULL, NULL, NULL, NULL );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(attrs == INSTALLFEATUREATTRIBUTE_FAVORLOCAL,
       "expected INSTALLFEATUREATTRIBUTE_FAVORLOCAL, got %#lx\n", attrs);

    r = MsiSetFeatureAttributesA( package, "One", INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    attrs = 0;
    r = MsiGetFeatureInfoA( package, "One", &attrs, NULL, NULL, NULL, NULL );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(attrs == INSTALLFEATUREATTRIBUTE_FAVORLOCAL,
       "expected INSTALLFEATUREATTRIBUTE_FAVORLOCAL, got %#lx\n", attrs);

    r = MsiDoActionA( package, "FileCost" );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, "One", INSTALLFEATUREATTRIBUTE_FAVORSOURCE );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    attrs = 0;
    r = MsiGetFeatureInfoA( package, "One", &attrs, NULL, NULL, NULL, NULL );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(attrs == INSTALLFEATUREATTRIBUTE_FAVORSOURCE,
       "expected INSTALLFEATUREATTRIBUTE_FAVORSOURCE, got %#lx\n", attrs);

    r = MsiDoActionA( package, "CostFinalize" );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSetFeatureAttributesA( package, "One", INSTALLFEATUREATTRIBUTE_FAVORLOCAL );
    ok(r == ERROR_FUNCTION_FAILED, "expected ERROR_FUNCTION_FAILED, got %u\n", r);

    MsiCloseHandle( package );
    DeleteFileA( msifile );
}

static void test_MsiGetFeatureInfo(void)
{
    UINT r;
    MSIHANDLE package;
    char title[32], help[32], path[MAX_PATH];
    DWORD attrs, title_len, help_len;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }
    create_database( msifile, tables, ARRAY_SIZE( tables ));

    strcpy( path, CURR_DIR );
    strcat( path, "\\" );
    strcat( path, msifile );

    r = MsiOpenPackageA( path, &package );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        DeleteFileA( msifile );
        return;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiGetFeatureInfoA( 0, NULL, NULL, NULL, NULL, NULL, NULL );
    ok(r == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiGetFeatureInfoA( package, NULL, NULL, NULL, NULL, NULL, NULL );
    ok(r == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %u\n", r);

    r = MsiGetFeatureInfoA( package, "", NULL, NULL, NULL, NULL, NULL );
    ok(r == ERROR_UNKNOWN_FEATURE, "expected ERROR_UNKNOWN_FEATURE, got %u\n", r);

    r = MsiGetFeatureInfoA( package, "One", NULL, NULL, NULL, NULL, NULL );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    r = MsiGetFeatureInfoA( 0, "One", NULL, NULL, NULL, NULL, NULL );
    ok(r == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %u\n", r);

    title_len = help_len = 0;
    r = MsiGetFeatureInfoA( package, "One", NULL, NULL, &title_len, NULL, &help_len );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(title_len == 3, "expected 3, got %lu\n", title_len);
    ok(help_len == 3, "expected 3, got %lu\n", help_len);

    title[0] = help[0] = 0;
    title_len = help_len = 0;
    r = MsiGetFeatureInfoA( package, "One", NULL, title, &title_len, help, &help_len );
    ok(r == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %u\n", r);
    ok(title_len == 3, "expected 3, got %lu\n", title_len);
    ok(help_len == 3, "expected 3, got %lu\n", help_len);

    attrs = 0;
    title[0] = help[0] = 0;
    title_len = sizeof(title);
    help_len = sizeof(help);
    r = MsiGetFeatureInfoA( package, "One", &attrs, title, &title_len, help, &help_len );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(attrs == INSTALLFEATUREATTRIBUTE_FAVORLOCAL, "expected INSTALLFEATUREATTRIBUTE_FAVORLOCAL, got %lu\n", attrs);
    ok(title_len == 3, "expected 3, got %lu\n", title_len);
    ok(help_len == 3, "expected 3, got %lu\n", help_len);
    ok(!strcmp(title, "One"), "expected \"One\", got \"%s\"\n", title);
    ok(!strcmp(help, "One"), "expected \"One\", got \"%s\"\n", help);

    attrs = 0;
    title[0] = help[0] = 0;
    title_len = sizeof(title);
    help_len = sizeof(help);
    r = MsiGetFeatureInfoA( package, "Two", &attrs, title, &title_len, help, &help_len );
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(attrs == INSTALLFEATUREATTRIBUTE_FAVORLOCAL, "expected INSTALLFEATUREATTRIBUTE_FAVORLOCAL, got %lu\n", attrs);
    ok(!title_len, "expected 0, got %lu\n", title_len);
    ok(!help_len, "expected 0, got %lu\n", help_len);
    ok(!title[0], "expected \"\", got \"%s\"\n", title);
    ok(!help[0], "expected \"\", got \"%s\"\n", help);

    MsiCloseHandle( package );
    DeleteFileA( msifile );
}

static INT CALLBACK handler_a(LPVOID context, UINT type, LPCSTR msg)
{
    return IDOK;
}

static INT CALLBACK handler_w(LPVOID context, UINT type, LPCWSTR msg)
{
    return IDOK;
}

static INT CALLBACK handler_record(LPVOID context, UINT type, MSIHANDLE record)
{
    return IDOK;
}

static void test_MsiSetInternalUI(void)
{
    INSTALLUILEVEL level;

    level = MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);
    ok(level == INSTALLUILEVEL_DEFAULT, "expected INSTALLUILEVEL_DEFAULT, got %d\n", level);

    level = MsiSetInternalUI(INSTALLUILEVEL_DEFAULT, NULL);
    ok(level == INSTALLUILEVEL_FULL, "expected INSTALLUILEVEL_FULL, got %d\n", level);

    level = MsiSetInternalUI(INSTALLUILEVEL_NOCHANGE, NULL);
    ok(level == INSTALLUILEVEL_DEFAULT, "expected INSTALLUILEVEL_DEFAULT, got %d\n", level);

    level = MsiSetInternalUI(0xdeadbeef, NULL);
    ok(level == INSTALLUILEVEL_NOCHANGE, "expected INSTALLUILEVEL_NOCHANGE, got %d\n", level);
}

static void test_MsiSetExternalUI(void)
{
    INSTALLUI_HANDLERA ret_a;
    INSTALLUI_HANDLERW ret_w;
    INSTALLUI_HANDLER_RECORD prev;
    UINT error;

    ret_a = MsiSetExternalUIA(handler_a, INSTALLLOGMODE_ERROR, NULL);
    ok(ret_a == NULL, "expected NULL, got %p\n", ret_a);

    ret_a = MsiSetExternalUIA(NULL, 0, NULL);
    ok(ret_a == handler_a, "expected %p, got %p\n", handler_a, ret_a);

    error = MsiSetExternalUIRecord(handler_record, INSTALLLOGMODE_ERROR, NULL, &prev);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);
    ok(prev == NULL, "expected NULL, got %p\n", prev);

    prev = (INSTALLUI_HANDLER_RECORD)0xdeadbeef;
    error = MsiSetExternalUIRecord(NULL, INSTALLLOGMODE_ERROR, NULL, &prev);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);
    ok(prev == handler_record, "expected %p, got %p\n", handler_record, prev);

    ret_w = MsiSetExternalUIW(handler_w, INSTALLLOGMODE_ERROR, NULL);
    ok(ret_w == NULL, "expected NULL, got %p\n", ret_w);

    ret_w = MsiSetExternalUIW(NULL, 0, NULL);
    ok(ret_w == handler_w, "expected %p, got %p\n", handler_w, ret_w);

    ret_a = MsiSetExternalUIA(handler_a, INSTALLLOGMODE_ERROR, NULL);
    ok(ret_a == NULL, "expected NULL, got %p\n", ret_a);

    ret_w = MsiSetExternalUIW(handler_w, INSTALLLOGMODE_ERROR, NULL);
    ok(ret_w == NULL, "expected NULL, got %p\n", ret_w);

    prev = (INSTALLUI_HANDLER_RECORD)0xdeadbeef;
    error = MsiSetExternalUIRecord(handler_record, INSTALLLOGMODE_ERROR, NULL, &prev);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);
    ok(prev == NULL, "expected NULL, got %p\n", prev);

    ret_a = MsiSetExternalUIA(NULL, 0, NULL);
    ok(ret_a == NULL, "expected NULL, got %p\n", ret_a);

    ret_w = MsiSetExternalUIW(NULL, 0, NULL);
    ok(ret_w == NULL, "expected NULL, got %p\n", ret_w);

    prev = (INSTALLUI_HANDLER_RECORD)0xdeadbeef;
    error = MsiSetExternalUIRecord(NULL, 0, NULL, &prev);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);
    ok(prev == handler_record, "expected %p, got %p\n", handler_record, prev);

    error = MsiSetExternalUIRecord(handler_record, INSTALLLOGMODE_ERROR, NULL, NULL);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);

    error = MsiSetExternalUIRecord(NULL, 0, NULL, NULL);
    ok(!error, "MsiSetExternalUIRecord failed %u\n", error);
}

static void test_lastusedsource(void)
{
    static const char prodcode[] = "{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}";
    char value[MAX_PATH], path[MAX_PATH];
    DWORD size;
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("maximus", 500);
    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");
    DeleteFileA("maximus");

    create_database("msifile0.msi", lus0_tables, ARRAY_SIZE(lus0_tables));
    create_database("msifile1.msi", lus1_tables, ARRAY_SIZE(lus1_tables));
    create_database("msifile2.msi", lus2_tables, ARRAY_SIZE(lus2_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* no cabinet file */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);
    ok(!lstrcmpA(value, "aaa"), "expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA("msifile0.msi", "PUBLISH_PRODUCT=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(!lstrcmpA(value, path), "expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "expected %d, got %lu\n", lstrlenA(path), size);

    r = MsiInstallProductA("msifile0.msi", "REMOVE=ALL FULL=1");
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    /* separate cabinet file */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);
    ok(!lstrcmpA(value, "aaa"), "expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA("msifile1.msi", "PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(!lstrcmpA(value, path), "expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "expected %d, got %lu\n", lstrlenA(path), size);

    r = MsiInstallProductA("msifile1.msi", "REMOVE=ALL FULL=1");
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);
    ok(!lstrcmpA(value, "aaa"), "expected \"aaa\", got \"%s\"\n", value);

    /* embedded cabinet stream */

    add_cabinet_storage("msifile2.msi", "test1.cab");

    r = MsiInstallProductA("msifile2.msi", "PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);
    ok(!lstrcmpA(value, path), "expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "expected %d, got %lu\n", lstrlenA(path), size);

    r = MsiInstallProductA("msifile2.msi", "REMOVE=ALL FULL=1");
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %u\n", r);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "expected ERROR_UNKNOWN_PRODUCT, got %u\n", r);
    ok(!lstrcmpA(value, "aaa"), "expected \"aaa\", got \"%s\"\n", value);

error:
    delete_cab_files();
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    DeleteFileA("msifile0.msi");
    DeleteFileA("msifile1.msi");
    DeleteFileA("msifile2.msi");
}

static void test_setpropertyfolder(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, spf_tables, ARRAY_SIZE(spf_tables));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\added\\added2\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\added\\added2", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\added", FALSE), "Directory not created\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    CreateDirectoryA("parent", NULL);
    CreateDirectoryA("parent\\child", NULL);
    create_file("parent\\child\\maximus", 500);

    create_database(msifile, spf2_tables, ARRAY_SIZE(spf2_tables));

    r = MsiInstallProductA(msifile, "TARGETDIR=c:\\");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(delete_pf("msitest\\maximus", TRUE), "file not installed\n");
    ok(delete_pf("msitest", FALSE), "directory not created\n");

    ok(DeleteFileA("c:\\parent\\child\\Shortcut.lnk"), "file not installed\n");
    ok(RemoveDirectoryA("c:\\parent\\child"), "directory not created\n");
    ok(RemoveDirectoryA("c:\\parent"), "directory not created\n");

    DeleteFileA("parent\\child\\maximus");
    RemoveDirectoryA("parent\\child");
    RemoveDirectoryA("parent");

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
}

static void test_sourcedir_props(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\sourcedir.txt", 1000);
    create_database(msifile, sd_tables, ARRAY_SIZE(sd_tables));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* full UI, no ResolveSource action */
    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\sourcedir.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    /* full UI, ResolveSource action */
    r = MsiInstallProductA(msifile, "ResolveSource=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\sourcedir.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* no UI, no ResolveSource action */
    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\sourcedir.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    /* no UI, ResolveSource action */
    r = MsiInstallProductA(msifile, "ResolveSource=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\sourcedir.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    DeleteFileA("msitest\\sourcedir.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_concurrentinstall(void)
{
    UINT r;
    CHAR path[MAX_PATH];

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    CreateDirectoryA("msitest\\msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_file("msitest\\msitest\\augustus", 500);

    create_database(msifile, ci_tables, ARRAY_SIZE(ci_tables));

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\msitest\\concurrent.msi");
    create_database(path, ci2_tables, ARRAY_SIZE(ci2_tables));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    r = MsiConfigureProductA("{38847338-1BBC-4104-81AC-2FAAC7ECDDCD}", INSTALLLEVEL_DEFAULT,
                             INSTALLSTATE_ABSENT);
    ok(r == ERROR_SUCCESS, "got %u\n", r);

    r = MsiConfigureProductA("{FF4AFE9C-6AC2-44F9-A060-9EA6BD16C75E}", INSTALLLEVEL_DEFAULT,
                             INSTALLSTATE_ABSENT);
    ok(r == ERROR_SUCCESS, "got %u\n", r);

error:
    DeleteFileA(path);
    DeleteFileA(msifile);
    DeleteFileA("msitest\\msitest\\augustus");
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest\\msitest");
    RemoveDirectoryA("msitest");
}

static void test_command_line_parsing(void)
{
    UINT r;
    const char *cmd;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_database(msifile, cl_tables, ARRAY_SIZE(cl_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    cmd = " ";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "=";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "==";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "one";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "=one";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "  P=";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=  ";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=\"\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"\"\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=\"\"\"\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\" ";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P= \"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P= \"\" ";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"  \"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=one";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P= one";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P=\"one";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=one\"";
    r = MsiInstallProductA(msifile, cmd);
    todo_wine ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"one\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P= \"one\" ";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P=\"one\"\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=\"\"one\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=\"\"one\"\"";
    r = MsiInstallProductA(msifile, cmd);
    todo_wine ok(r == ERROR_INVALID_COMMAND_LINE, "Expected ERROR_INVALID_COMMAND_LINE, got %u\n", r);

    cmd = "P=\"one two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"\"\"one\"\" two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"\"\"one\"\" two\" Q=three";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"\" Q=\"two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"one\" Q=\"two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P=\"one=two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "Q=\"\" P=\"one\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    cmd = "P=\"\"\"one\"\"\" Q=\"two\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"one \"\"two\"\"\" Q=\"three\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    cmd = "P=\"\"\"one\"\" two\" Q=\"three\"";
    r = MsiInstallProductA(msifile, cmd);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    DeleteFileA(msifile);
    delete_test_files();
}

START_TEST(msi)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];

#ifdef __REACTOS__
    if (!winetest_interactive &&
        !strcmp(winetest_platform, "windows"))
    {
        skip("ROSTESTS-180: Skipping msi_winetest:msi because it hangs on WHS-Testbot. Set winetest_interactive to run it anyway.\n");
        return;
    }
#endif

    if (!is_process_elevated()) restart_as_admin_elevated();

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    ok(get_system_dirs(), "failed to retrieve system dirs\n");

    test_usefeature();
    test_null();
    test_getcomponentpath();
    test_MsiGetFileHash();
    test_MsiSetInternalUI();
    test_MsiSetExternalUI();
    test_MsiQueryProductState();
    test_MsiQueryFeatureState();
    test_MsiQueryComponentState();
    test_MsiGetComponentPath();
    test_MsiGetComponentPathEx();
    test_MsiProvideComponent();
    test_MsiGetProductCode();
    test_MsiEnumClients();
    test_MsiGetProductInfo();
    test_MsiGetProductInfoEx();
    test_MsiGetUserInfo();
    test_MsiOpenProduct();
    test_MsiEnumPatchesEx();
    test_MsiEnumPatches();
    test_MsiGetPatchInfoEx();
    test_MsiGetPatchInfo();
    test_MsiEnumProducts();
    test_MsiEnumProductsEx();
    test_MsiEnumComponents();
    test_MsiEnumComponentsEx();
    test_MsiGetFileVersion();
    test_MsiGetFileSignatureInformation();
    test_MsiConfigureProductEx();
    test_MsiSetFeatureAttributes();
    test_MsiGetFeatureInfo();
    test_lastusedsource();
    test_setpropertyfolder();
    test_sourcedir_props();
    test_concurrentinstall();
    test_command_line_parsing();
    test_MsiProvideQualifiedComponentEx();

    SetCurrentDirectoryA(prev_path);
}
