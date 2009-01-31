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

#define _WIN32_MSI 300
#define COBJMACROS

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>
#include <objidl.h>
#include <srrestoreptapi.h>

#include "wine/test.h"

static UINT (WINAPI *pMsiQueryComponentStateA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, LPCSTR, INSTALLSTATE*);
static UINT (WINAPI *pMsiSourceListEnumSourcesA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, DWORD, LPSTR, LPDWORD);
static UINT (WINAPI *pMsiSourceListGetInfoA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, LPCSTR, LPSTR, LPDWORD);

static HMODULE hsrclient = 0;
static BOOL (WINAPI *pSRRemoveRestorePoint)(DWORD);
static BOOL (WINAPI *pSRSetRestorePointA)(RESTOREPOINTINFOA*, STATEMGRSTATUS*);

static BOOL on_win9x = FALSE;

static const char *msifile = "msitest.msi";
static const char *msifile2 = "winetest2.msi";
static const char *mstfile = "winetest.mst";
static CHAR CURR_DIR[MAX_PATH];
static CHAR PROG_FILES_DIR[MAX_PATH];
static CHAR COMMON_FILES_DIR[MAX_PATH];

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
                                    "component\t\tMSITESTDIR\t0\t1\tfile\n"
                                    "service_comp\t\tMSITESTDIR\t0\t1\tservice_file";

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
                                  "Two\t\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0\n"
                                  "feature\t\t\t\t2\t1\tTARGETDIR\t0\n"
                                  "service_feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const CHAR feature_comp_dat[] = "Feature_\tComponent_\n"
                                       "s38\ts72\n"
                                       "FeatureComponents\tFeature_\tComponent_\n"
                                       "Five\tFive\n"
                                       "Four\tFour\n"
                                       "One\tOne\n"
                                       "Three\tThree\n"
                                       "Two\tTwo\n"
                                       "feature\tcomponent\n"
                                       "service_feature\tservice_comp\n";

static const CHAR file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                               "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                               "File\tFile\n"
                               "five.txt\tFive\tfive.txt\t1000\t\t\t16384\t5\n"
                               "four.txt\tFour\tfour.txt\t1000\t\t\t16384\t4\n"
                               "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n"
                               "three.txt\tThree\tthree.txt\t1000\t\t\t0\t3\n"
                               "two.txt\tTwo\ttwo.txt\t1000\t\t\t0\t2\n"
                               "file\tcomponent\tfilename\t100\t\t\t8192\t1\n"
                               "service_file\tservice_comp\tservice.exe\t100\t\t\t8192\t1";

static const CHAR install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallExecuteSequence\tAction\n"
                                           "AllocateRegistrySpace\tNOT Installed\t1550\n"
                                           "CostFinalize\t\t1000\n"
                                           "CostInitialize\t\t800\n"
                                           "FileCost\t\t900\n"
                                           "ResolveSource\t\t950\n"
                                           "MoveFiles\t\t1700\n"
                                           "InstallFiles\t\t4000\n"
                                           "DuplicateFiles\t\t4500\n"
                                           "InstallServices\t\t5000\n"
                                           "InstallFinalize\t\t6600\n"
                                           "InstallInitialize\t\t1500\n"
                                           "InstallValidate\t\t1400\n"
                                           "LaunchConditions\t\t100\n"
                                           "WriteRegistryValues\tSourceDir And SOURCEDIR\t5000";

static const CHAR media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                "i2\ti4\tL64\tS255\tS32\tS72\n"
                                "Media\tDiskId\n"
                                "1\t3\t\t\tDISK1\t\n"
                                "2\t5\t\tmsitest.cab\tDISK2\t\n";

static const CHAR property_dat[] = "Property\tValue\n"
                                   "s72\tl0\n"
                                   "Property\tProperty\n"
                                   "DefaultUIFont\tDlgFont8\n"
                                   "HASUIRUN\t0\n"
                                   "INSTALLLEVEL\t3\n"
                                   "InstallMode\tTypical\n"
                                   "Manufacturer\tWine\n"
                                   "PIDTemplate\t12345<###-%%%%%%%>@@@@@\n"
                                   "ProductCode\t{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}\n"
                                   "ProductID\tnone\n"
                                   "ProductLanguage\t1033\n"
                                   "ProductName\tMSITEST\n"
                                   "ProductVersion\t1.1.1\n"
                                   "PROMPTROLLBACKCOST\tP\n"
                                   "Setup\tSetup\n"
                                   "UpgradeCode\t{4C0EAA15-0264-4E5A-8758-609EF142B92D}\n"
                                   "AdminProperties\tPOSTADMIN\n"
                                   "ROOTDRIVE\tC:\\\n"
                                   "SERVNAME\tTestService\n"
                                   "SERVDISP\tTestServiceDisp\n";

static const CHAR registry_dat[] = "Registry\tRoot\tKey\tName\tValue\tComponent_\n"
                                   "s72\ti2\tl255\tL255\tL0\ts72\n"
                                   "Registry\tRegistry\n"
                                   "Apples\t2\tSOFTWARE\\Wine\\msitest\tName\timaname\tOne\n"
                                   "Oranges\t2\tSOFTWARE\\Wine\\msitest\tnumber\t#314\tTwo\n"
                                   "regdata\t2\tSOFTWARE\\Wine\\msitest\tblah\tbad\tdangler\n"
                                   "OrderTest\t2\tSOFTWARE\\Wine\\msitest\tOrderTestName\tOrderTestValue\tcomponent";

static const CHAR service_install_dat[] = "ServiceInstall\tName\tDisplayName\tServiceType\tStartType\tErrorControl\t"
                                          "LoadOrderGroup\tDependencies\tStartName\tPassword\tArguments\tComponent_\tDescription\n"
                                          "s72\ts255\tL255\ti4\ti4\ti4\tS255\tS255\tS255\tS255\tS255\ts72\tL255\n"
                                          "ServiceInstall\tServiceInstall\n"
                                          "TestService\t[SERVNAME]\t[SERVDISP]\t2\t3\t0\t\t\tTestService\t\t\tservice_comp\t\t";

static const CHAR service_control_dat[] = "ServiceControl\tName\tEvent\tArguments\tWait\tComponent_\n"
                                          "s72\tl255\ti2\tL255\tI2\ts72\n"
                                          "ServiceControl\tServiceControl\n"
                                          "ServiceControl\tTestService\t8\t\t0\tservice_comp";

/* tables for test_continuouscabs */
static const CHAR cc_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                       "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                       "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n";

static const CHAR cc2_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                        "augustus\t\tMSITESTDIR\t0\t0\taugustus\n"
                                        "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n";

static const CHAR cc_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                     "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                     "Feature\tFeature\n"
                                     "feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const CHAR cc_feature_comp_dat[] = "Feature_\tComponent_\n"
                                          "s38\ts72\n"
                                          "FeatureComponents\tFeature_\tComponent_\n"
                                          "feature\tmaximus\n"
                                          "feature\taugustus\n"
                                          "feature\tcaesar";

static const CHAR cc_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                  "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t12";

static const CHAR cc2_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                   "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                   "tiberius\tmaximus\ttiberius\t500\t\t\t16384\t3\n"
                                   "caesar\tcaesar\tcaesar\t500\t\t\t16384\t12";

static const CHAR cc_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t10\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t12\t\ttest3.cab\tDISK3\t\n";

static const CHAR co_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                  "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t3";

static const CHAR co_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t10\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t3\t\ttest3.cab\tDISK3\t\n";

static const CHAR co2_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t10\t\ttest1.cab\tDISK1\t\n"
                                    "2\t12\t\ttest3.cab\tDISK3\t\n"
                                    "3\t2\t\ttest2.cab\tDISK2\t\n";

static const CHAR mm_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t512\t1\n"
                                  "augustus\taugustus\taugustus\t500\t\t\t512\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t3";

static const CHAR mm_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t3\t\ttest1.cab\tDISK1\t\n";

static const CHAR ss_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t2\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t12\t\ttest3.cab\tDISK3\t\n";

/* tables for test_uiLevelFlags */
static const CHAR ui_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t\tMSITESTDIR\t0\tHASUIRUN=1\tmaximus\n"
                                       "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                       "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n";

static const CHAR ui_install_ui_seq_dat[] = "Action\tCondition\tSequence\n"
                                           "s72\tS255\tI2\n"
                                           "InstallUISequence\tAction\n"
                                           "SetUIProperty\t\t5\n"
                                           "ExecuteAction\t\t1100\n";

static const CHAR ui_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                           "s72\ti2\tS64\tS0\tS255\n"
                                           "CustomAction\tAction\n"
                                           "SetUIProperty\t51\tHASUIRUN\t1\t\n";

static const CHAR rof_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n";

static const CHAR rof_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                      "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                      "Feature\tFeature\n"
                                      "feature\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0\n"
                                      "montecristo\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0";

static const CHAR rof_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\tmaximus\n"
                                           "montecristo\tmaximus";

static const CHAR rof_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1";

static const CHAR rof_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t1\t\t\tDISK1\t\n";

static const CHAR sdp_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
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
                                               "SetDirProperty\t\t950";

static const CHAR sdp_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "SetDirProperty\t51\tMSITESTDIR\t[CommonFilesFolder]msitest\\\t\n";

static const CHAR cie_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                        "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                        "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n"
                                        "gaius\t\tMSITESTDIR\t0\t1\tgaius\n";

static const CHAR cie_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\tmaximus\n"
                                           "feature\taugustus\n"
                                           "feature\tcaesar\n"
                                           "feature\tgaius";

static const CHAR cie_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                   "augustus\taugustus\taugustus\t50000\t\t\t16384\t2\n"
                                   "caesar\tcaesar\tcaesar\t500\t\t\t16384\t12\n"
                                   "gaius\tgaius\tgaius\t500\t\t\t8192\t11";

static const CHAR cie_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t1\t\ttest1.cab\tDISK1\t\n"
                                    "2\t2\t\ttest2.cab\tDISK2\t\n"
                                    "3\t12\t\ttest3.cab\tDISK3\t\n";

static const CHAR ci_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                              "s72\tS255\tI2\n"
                                              "InstallExecuteSequence\tAction\n"
                                              "CostFinalize\t\t1000\n"
                                              "CostInitialize\t\t800\n"
                                              "FileCost\t\t900\n"
                                              "InstallFiles\t\t4000\n"
                                              "InstallServices\t\t5000\n"
                                              "InstallFinalize\t\t6600\n"
                                              "InstallInitialize\t\t1500\n"
                                              "RunInstall\t\t1600\n"
                                              "InstallValidate\t\t1400\n"
                                              "LaunchConditions\t\t100";

static const CHAR ci_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "RunInstall\t87\tmsitest\\concurrent.msi\tMYPROP=[UILevel]\t\n";

static const CHAR ci_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\tUILevel=5\tmaximus\n";

static const CHAR ci2_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\tUILevel=3 AND MYPROP=5\taugustus\n";

static const CHAR ci2_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\taugustus";

static const CHAR ci2_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "augustus\taugustus\taugustus\t500\t\t\t8192\t1";

static const CHAR spf_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "SetFolderProp\t51\tMSITESTDIR\t[ProgramFilesFolder]\\msitest\\added\t\n";

static const CHAR spf_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                               "s72\tS255\tI2\n"
                                               "InstallExecuteSequence\tAction\n"
                                               "CostFinalize\t\t1000\n"
                                               "CostInitialize\t\t800\n"
                                               "FileCost\t\t900\n"
                                               "SetFolderProp\t\t950\n"
                                               "InstallFiles\t\t4000\n"
                                               "InstallServices\t\t5000\n"
                                               "InstallFinalize\t\t6600\n"
                                               "InstallInitialize\t\t1500\n"
                                               "InstallValidate\t\t1400\n"
                                               "LaunchConditions\t\t100";

static const CHAR spf_install_ui_seq_dat[] = "Action\tCondition\tSequence\n"
                                             "s72\tS255\tI2\n"
                                             "InstallUISequence\tAction\n"
                                             "CostInitialize\t\t800\n"
                                             "FileCost\t\t900\n"
                                             "CostFinalize\t\t1000\n"
                                             "ExecuteAction\t\t1100\n";

static const CHAR pp_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
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

static const CHAR ppc_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\tUILevel=5\tmaximus\n"
                                        "augustus\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t1\t\taugustus\n";

static const CHAR ppc_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1\n"
                                   "augustus\taugustus\taugustus\t500\t\t\t8192\t2";

static const CHAR ppc_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                    "i2\ti4\tL64\tS255\tS32\tS72\n"
                                    "Media\tDiskId\n"
                                    "1\t2\t\t\tDISK1\t\n";

static const CHAR ppc_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\tmaximus\n"
                                           "feature\taugustus\n"
                                           "montecristo\tmaximus";

static const CHAR tp_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "augustus\t\tMSITESTDIR\t0\tprop=\"val\"\taugustus\n";

static const CHAR cwd_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\t\taugustus\n";

static const CHAR adm_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\tPOSTADMIN=1\taugustus";

static const CHAR adm_custom_action_dat[] = "Action\tType\tSource\tTarget\tISComments\n"
                                            "s72\ti2\tS64\tS0\tS255\n"
                                            "CustomAction\tAction\n"
                                            "SetPOSTADMIN\t51\tPOSTADMIN\t1\t\n";

static const CHAR adm_admin_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                             "s72\tS255\tI2\n"
                                             "AdminExecuteSequence\tAction\n"
                                             "CostFinalize\t\t1000\n"
                                             "CostInitialize\t\t800\n"
                                             "FileCost\t\t900\n"
                                             "SetPOSTADMIN\t\t950\n"
                                             "InstallFiles\t\t4000\n"
                                             "InstallFinalize\t\t6600\n"
                                             "InstallInitialize\t\t1500\n"
                                             "InstallValidate\t\t1400\n"
                                             "LaunchConditions\t\t100";

static const CHAR amp_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\tMYPROP=2718 and MyProp=42\taugustus\n";

static const CHAR rem_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "hydrogen\t{C844BD1E-1907-4C00-8BC9-150BD70DF0A1}\tMSITESTDIR\t0\t\thydrogen\n"
                                        "helium\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t1\t\thelium\n"
                                        "lithium\t\tMSITESTDIR\t2\t\tlithium\n";

static const CHAR rem_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "feature\thydrogen\n"
                                           "feature\thelium\n"
                                           "feature\tlithium";

static const CHAR rem_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                   "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                   "File\tFile\n"
                                   "hydrogen\thydrogen\thydrogen\t0\t\t\t8192\t1\n"
                                   "helium\thelium\thelium\t0\t\t\t8192\t1\n"
                                   "lithium\tlithium\tlithium\t0\t\t\t8192\t1";

static const CHAR rem_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
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

static const CHAR rem_remove_files_dat[] = "FileKey\tComponent_\tFileName\tDirProperty\tInstallMode\n"
                                           "s72\ts72\tS255\ts72\tI2\n"
                                           "RemoveFile\tFileKey\n"
                                           "furlong\thydrogen\tfurlong\tMSITESTDIR\t1\n"
                                           "firkin\thelium\tfirkin\tMSITESTDIR\t1\n"
                                           "fortnight\tlithium\tfortnight\tMSITESTDIR\t1\n"
                                           "becquerel\thydrogen\tbecquerel\tMSITESTDIR\t2\n"
                                           "dioptre\thelium\tdioptre\tMSITESTDIR\t2\n"
                                           "attoparsec\tlithium\tattoparsec\tMSITESTDIR\t2\n"
                                           "storeys\thydrogen\tstoreys\tMSITESTDIR\t3\n"
                                           "block\thelium\tblock\tMSITESTDIR\t3\n"
                                           "siriometer\tlithium\tsiriometer\tMSITESTDIR\t3\n"
                                           "nanoacre\thydrogen\t\tCABOUTDIR\t3\n";

static const CHAR mov_move_file_dat[] = "FileKey\tComponent_\tSourceName\tDestName\tSourceFolder\tDestFolder\tOptions\n"
                                        "s72\ts72\tS255\tS255\tS72\ts72\ti2\n"
                                        "MoveFile\tFileKey\n"
                                        "abkhazia\taugustus\tnonexistent\tdest\tSourceDir\tMSITESTDIR\t0\n"
                                        "bahamas\taugustus\tnonexistent\tdest\tSourceDir\tMSITESTDIR\t1\n"
                                        "cambodia\taugustus\tcameroon\tcanada\tSourceDir\tMSITESTDIR\t0\n"
                                        "denmark\taugustus\tdjibouti\tdominica\tSourceDir\tMSITESTDIR\t1\n"
                                        "ecuador\taugustus\tegypt\telsalvador\tNotAProp\tMSITESTDIR\t1\n"
                                        "fiji\taugustus\tfinland\tfrance\tSourceDir\tNotAProp\t1\n"
                                        "gabon\taugustus\tgambia\tgeorgia\tSOURCEFULL\tMSITESTDIR\t1\n"
                                        "haiti\taugustus\thonduras\thungary\tSourceDir\tDESTFULL\t1\n"
                                        "iceland\taugustus\tindia\tindonesia\tMSITESTDIR\tMSITESTDIR\t1\n"
                                        "jamaica\taugustus\tjapan\tjordan\tFILEPATHBAD\tMSITESTDIR\t1\n"
                                        "kazakhstan\taugustus\t\tkiribati\tFILEPATHGOOD\tMSITESTDIR\t1\n"
                                        "laos\taugustus\tlatvia\tlebanon\tSourceDir\tMSITESTDIR\t1\n"
                                        "namibia\taugustus\tnauru\tkiribati\tSourceDir\tMSITESTDIR\t1\n"
                                        "pakistan\taugustus\tperu\tsfn|poland\tSourceDir\tMSITESTDIR\t1\n"
                                        "wildcard\taugustus\tapp*\twildcard\tSourceDir\tMSITESTDIR\t1\n"
                                        "single\taugustus\tf?o\tsingle\tSourceDir\tMSITESTDIR\t1\n"
                                        "wildcardnodest\taugustus\tbudd*\t\tSourceDir\tMSITESTDIR\t1\n"
                                        "singlenodest\taugustus\tb?r\t\tSourceDir\tMSITESTDIR\t1\n";

static const CHAR mc_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n"
                                        "augustus\t\tMSITESTDIR\t0\t1\taugustus\n"
                                        "caesar\t\tMSITESTDIR\t0\t1\tcaesar\n"
                                        "gaius\t\tMSITESTDIR\t0\tGAIUS=1\tgaius\n";

static const CHAR mc_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "maximus\tmaximus\tmaximus\t500\t\t\t16384\t1\n"
                                  "augustus\taugustus\taugustus\t500\t\t\t0\t2\n"
                                  "caesar\tcaesar\tcaesar\t500\t\t\t16384\t3\n"
                                  "gaius\tgaius\tgaius\t500\t\t\t16384\t4";

static const CHAR mc_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t1\t\ttest1.cab\tDISK1\t\n"
                                   "2\t2\t\ttest2.cab\tDISK2\t\n"
                                   "3\t3\t\ttest3.cab\tDISK3\t\n"
                                   "4\t4\t\ttest3.cab\tDISK3\t\n";

static const CHAR mc_file_hash_dat[] = "File_\tOptions\tHashPart1\tHashPart2\tHashPart3\tHashPart4\n"
                                       "s72\ti2\ti4\ti4\ti4\ti4\n"
                                       "MsiFileHash\tFile_\n"
                                       "caesar\t0\t850433704\t-241429251\t675791761\t-1221108824";

static const CHAR df_directory_dat[] = "Directory\tDirectory_Parent\tDefaultDir\n"
                                       "s72\tS72\tl255\n"
                                       "Directory\tDirectory\n"
                                       "THIS\tMSITESTDIR\tthis\n"
                                       "DOESNOT\tTHIS\tdoesnot\n"
                                       "NONEXISTENT\tDOESNOT\texist\n"
                                       "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
                                       "ProgramFilesFolder\tTARGETDIR\t.\n"
                                       "TARGETDIR\t\tSourceDir";

static const CHAR df_duplicate_file_dat[] = "FileKey\tComponent_\tFile_\tDestName\tDestFolder\n"
                                            "s72\ts72\ts72\tS255\tS72\n"
                                            "DuplicateFile\tFileKey\n"
                                            "maximus\tmaximus\tmaximus\taugustus\t\n"
                                            "caesar\tmaximus\tmaximus\t\tNONEXISTENT\n";

static const CHAR wrv_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tMSITESTDIR\t0\t\taugustus\n";

static const CHAR wrv_registry_dat[] = "Registry\tRoot\tKey\tName\tValue\tComponent_\n"
                                       "s72\ti2\tl255\tL255\tL0\ts72\n"
                                       "Registry\tRegistry\n"
                                       "regdata\t2\tSOFTWARE\\Wine\\msitest\tValue\t[~]one[~]two[~]three\taugustus";

static const CHAR ca51_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                         "s72\tS38\ts72\ti2\tS255\tS72\n"
                                         "Component\tComponent\n"
                                         "augustus\t\tMSITESTDIR\t0\tMYPROP=42\taugustus\n";

static const CHAR ca51_install_exec_seq_dat[] = "Action\tCondition\tSequence\n"
                                                "s72\tS255\tI2\n"
                                                "InstallExecuteSequence\tAction\n"
                                                "ValidateProductID\t\t700\n"
                                                "GoodSetProperty\t\t725\n"
                                                "BadSetProperty\t\t750\n"
                                                "CostInitialize\t\t800\n"
                                                "ResolveSource\t\t810\n"
                                                "FileCost\t\t900\n"
                                                "SetSourceDir\tSRCDIR\t910\n"
                                                "CostFinalize\t\t1000\n"
                                                "InstallValidate\t\t1400\n"
                                                "InstallInitialize\t\t1500\n"
                                                "InstallFiles\t\t4000\n"
                                                "InstallFinalize\t\t6600";

static const CHAR ca51_custom_action_dat[] = "Action\tType\tSource\tTarget\n"
                                             "s72\ti2\tS64\tS0\n"
                                             "CustomAction\tAction\n"
                                             "GoodSetProperty\t51\tMYPROP\t42\n"
                                             "BadSetProperty\t51\t\tMYPROP\n"
                                             "SetSourceDir\t51\tSourceDir\t[SRCDIR]\n";

static const CHAR is_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                     "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                     "Feature\tFeature\n"
                                     "one\t\t\t\t2\t1\t\t0\n" /* favorLocal */
                                     "two\t\t\t\t2\t1\t\t1\n" /* favorSource */
                                     "three\t\t\t\t2\t1\t\t4\n" /* favorAdvertise */
                                     "four\t\t\t\t2\t0\t\t0"; /* disabled */

static const CHAR is_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                       "s72\tS38\ts72\ti2\tS255\tS72\n"
                                       "Component\tComponent\n"
                                       "alpha\t\tMSITESTDIR\t0\t\talpha_file\n" /* favorLocal:Local */
                                       "beta\t\tMSITESTDIR\t1\t\tbeta_file\n" /* favorLocal:Source */
                                       "gamma\t\tMSITESTDIR\t2\t\tgamma_file\n" /* favorLocal:Optional */
                                       "theta\t\tMSITESTDIR\t0\t\ttheta_file\n" /* favorSource:Local */
                                       "delta\t\tMSITESTDIR\t1\t\tdelta_file\n" /* favorSource:Source */
                                       "epsilon\t\tMSITESTDIR\t2\t\tepsilon_file\n" /* favorSource:Optional */
                                       "zeta\t\tMSITESTDIR\t0\t\tzeta_file\n" /* favorAdvertise:Local */
                                       "iota\t\tMSITESTDIR\t1\t\tiota_file\n" /* favorAdvertise:Source */
                                       "eta\t\tMSITESTDIR\t2\t\teta_file\n" /* favorAdvertise:Optional */
                                       "kappa\t\tMSITESTDIR\t0\t\tkappa_file\n" /* disabled:Local */
                                       "lambda\t\tMSITESTDIR\t1\t\tlambda_file\n" /* disabled:Source */
                                       "mu\t\tMSITESTDIR\t2\t\tmu_file\n"; /* disabled:Optional */

static const CHAR is_feature_comp_dat[] = "Feature_\tComponent_\n"
                                          "s38\ts72\n"
                                          "FeatureComponents\tFeature_\tComponent_\n"
                                          "one\talpha\n"
                                          "one\tbeta\n"
                                          "one\tgamma\n"
                                          "two\ttheta\n"
                                          "two\tdelta\n"
                                          "two\tepsilon\n"
                                          "three\tzeta\n"
                                          "three\tiota\n"
                                          "three\teta\n"
                                          "four\tkappa\n"
                                          "four\tlambda\n"
                                          "four\tmu";

static const CHAR is_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "alpha_file\talpha\talpha\t500\t\t\t8192\t1\n"
                                  "beta_file\tbeta\tbeta\t500\t\t\t8291\t2\n"
                                  "gamma_file\tgamma\tgamma\t500\t\t\t8192\t3\n"
                                  "theta_file\ttheta\ttheta\t500\t\t\t8192\t4\n"
                                  "delta_file\tdelta\tdelta\t500\t\t\t8192\t5\n"
                                  "epsilon_file\tepsilon\tepsilon\t500\t\t\t8192\t6\n"
                                  "zeta_file\tzeta\tzeta\t500\t\t\t8192\t7\n"
                                  "iota_file\tiota\tiota\t500\t\t\t8192\t8\n"
                                  "eta_file\teta\teta\t500\t\t\t8192\t9\n"
                                  "kappa_file\tkappa\tkappa\t500\t\t\t8192\t10\n"
                                  "lambda_file\tlambda\tlambda\t500\t\t\t8192\t11\n"
                                  "mu_file\tmu\tmu\t500\t\t\t8192\t12";

static const CHAR is_media_dat[] = "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
                                   "i2\ti4\tL64\tS255\tS32\tS72\n"
                                   "Media\tDiskId\n"
                                   "1\t12\t\t\tDISK1\t\n";

static const CHAR sp_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "augustus\t\tTWODIR\t0\t\taugustus\n";

static const CHAR sp_directory_dat[] = "Directory\tDirectory_Parent\tDefaultDir\n"
                                       "s72\tS72\tl255\n"
                                       "Directory\tDirectory\n"
                                       "TARGETDIR\t\tSourceDir\n"
                                       "ProgramFilesFolder\tTARGETDIR\t.\n"
                                       "MSITESTDIR\tProgramFilesFolder\tmsitest:.\n"
                                       "ONEDIR\tMSITESTDIR\t.:shortone|longone\n"
                                       "TWODIR\tONEDIR\t.:shorttwo|longtwo";

static const CHAR mcp_component_dat[] = "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
                                        "s72\tS38\ts72\ti2\tS255\tS72\n"
                                        "Component\tComponent\n"
                                        "hydrogen\t{C844BD1E-1907-4C00-8BC9-150BD70DF0A1}\tMSITESTDIR\t2\t\thydrogen\n"
                                        "helium\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t2\t\thelium\n"
                                        "lithium\t{4AF28FFC-71C7-4307-BDE4-B77C5338F56F}\tMSITESTDIR\t2\tPROPVAR=42\tlithium\n";

static const CHAR mcp_feature_dat[] = "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
                                      "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
                                      "Feature\tFeature\n"
                                      "hydroxyl\t\thydroxyl\thydroxyl\t2\t1\tTARGETDIR\t0\n"
                                      "heliox\t\theliox\theliox\t2\t5\tTARGETDIR\t0\n"
                                      "lithia\t\tlithia\tlithia\t2\t10\tTARGETDIR\t0";

static const CHAR mcp_feature_comp_dat[] = "Feature_\tComponent_\n"
                                           "s38\ts72\n"
                                           "FeatureComponents\tFeature_\tComponent_\n"
                                           "hydroxyl\thydrogen\n"
                                           "heliox\thelium\n"
                                           "lithia\tlithium";

static const CHAR mcomp_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                     "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                     "File\tFile\n"
                                     "hydrogen\thydrogen\thydrogen\t0\t\t\t8192\t1\n"
                                     "helium\thelium\thelium\t0\t\t\t8192\t1\n"
                                     "lithium\tlithium\tlithium\t0\t\t\t8192\t1\n"
                                     "beryllium\tmissingcomp\tberyllium\t0\t\t\t8192\t1";

static const CHAR ai_file_dat[] = "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
                                  "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
                                  "File\tFile\n"
                                  "five.txt\tFive\tfive.txt\t1000\t\t\t16384\t5\n"
                                  "four.txt\tFour\tfour.txt\t1000\t\t\t16384\t4\n"
                                  "one.txt\tOne\tone.txt\t1000\t\t\t16384\t1\n"
                                  "three.txt\tThree\tthree.txt\t1000\t\t\t16384\t3\n"
                                  "two.txt\tTwo\ttwo.txt\t1000\t\t\t16384\t2\n"
                                  "file\tcomponent\tfilename\t100\t\t\t8192\t1\n"
                                  "service_file\tservice_comp\tservice.exe\t100\t\t\t8192\t1";

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
    ADD_TABLE(registry),
    ADD_TABLE(service_install),
    ADD_TABLE(service_control)
};

static const msi_table cc_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(cc_media),
    ADD_TABLE(property),
};

static const msi_table cc2_tables[] =
{
    ADD_TABLE(cc2_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(cc_media),
    ADD_TABLE(property),
};

static const msi_table co_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(co_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(co_media),
    ADD_TABLE(property),
};

static const msi_table co2_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(co2_media),
    ADD_TABLE(property),
};

static const msi_table mm_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(mm_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(mm_media),
    ADD_TABLE(property),
};

static const msi_table ss_tables[] =
{
    ADD_TABLE(cc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(ss_media),
    ADD_TABLE(property),
};

static const msi_table ui_tables[] =
{
    ADD_TABLE(ui_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cc_feature_comp),
    ADD_TABLE(cc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(ui_install_ui_seq),
    ADD_TABLE(ui_custom_action),
    ADD_TABLE(cc_media),
    ADD_TABLE(property),
};

static const msi_table rof_tables[] =
{
    ADD_TABLE(rof_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table sdp_tables[] =
{
    ADD_TABLE(rof_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(sdp_install_exec_seq),
    ADD_TABLE(sdp_custom_action),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table cie_tables[] =
{
    ADD_TABLE(cie_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cie_feature_comp),
    ADD_TABLE(cie_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(cie_media),
    ADD_TABLE(property),
};

static const msi_table ci_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(ci_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(ci_custom_action),
};

static const msi_table ci2_tables[] =
{
    ADD_TABLE(ci2_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table spf_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(spf_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(spf_custom_action),
    ADD_TABLE(spf_install_ui_seq),
};

static const msi_table pp_tables[] =
{
    ADD_TABLE(ci_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(pp_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table ppc_tables[] =
{
    ADD_TABLE(ppc_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ppc_feature_comp),
    ADD_TABLE(ppc_file),
    ADD_TABLE(pp_install_exec_seq),
    ADD_TABLE(ppc_media),
    ADD_TABLE(property),
};

static const msi_table tp_tables[] =
{
    ADD_TABLE(tp_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table cwd_tables[] =
{
    ADD_TABLE(cwd_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table adm_tables[] =
{
    ADD_TABLE(adm_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(adm_custom_action),
    ADD_TABLE(adm_admin_exec_seq),
};

static const msi_table amp_tables[] =
{
    ADD_TABLE(amp_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table rem_tables[] =
{
    ADD_TABLE(rem_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rem_feature_comp),
    ADD_TABLE(rem_file),
    ADD_TABLE(rem_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(rem_remove_files),
};

static const msi_table mov_tables[] =
{
    ADD_TABLE(cwd_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(mov_move_file),
};

static const msi_table mc_tables[] =
{
    ADD_TABLE(mc_component),
    ADD_TABLE(directory),
    ADD_TABLE(cc_feature),
    ADD_TABLE(cie_feature_comp),
    ADD_TABLE(mc_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(mc_media),
    ADD_TABLE(property),
    ADD_TABLE(mc_file_hash),
};

static const msi_table df_tables[] =
{
    ADD_TABLE(rof_component),
    ADD_TABLE(df_directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(df_duplicate_file),
};

static const msi_table wrv_tables[] =
{
    ADD_TABLE(wrv_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(wrv_registry),
};

static const msi_table sf_tables[] =
{
    ADD_TABLE(wrv_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table ca51_tables[] =
{
    ADD_TABLE(ca51_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(ca51_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(ca51_custom_action),
};

static const msi_table is_tables[] =
{
    ADD_TABLE(is_component),
    ADD_TABLE(directory),
    ADD_TABLE(is_feature),
    ADD_TABLE(is_feature_comp),
    ADD_TABLE(is_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(is_media),
    ADD_TABLE(property),
};

static const msi_table sp_tables[] =
{
    ADD_TABLE(sp_component),
    ADD_TABLE(sp_directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table mcp_tables[] =
{
    ADD_TABLE(mcp_component),
    ADD_TABLE(directory),
    ADD_TABLE(mcp_feature),
    ADD_TABLE(mcp_feature_comp),
    ADD_TABLE(rem_file),
    ADD_TABLE(rem_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table mcomp_tables[] =
{
    ADD_TABLE(mcp_component),
    ADD_TABLE(directory),
    ADD_TABLE(mcp_feature),
    ADD_TABLE(mcp_feature_comp),
    ADD_TABLE(mcomp_file),
    ADD_TABLE(rem_install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
};

static const msi_table ai_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(ai_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table pc_tables[] =
{
    ADD_TABLE(ca51_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(ci2_feature_comp),
    ADD_TABLE(ci2_file),
    ADD_TABLE(install_exec_seq),
    ADD_TABLE(rof_media),
    ADD_TABLE(property)
};

/* cabinet definitions */

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          0x7FFFFFFF
#define FOLDER_THRESHOLD    900000

/* the FCI callbacks */

static void * CDECL mem_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void CDECL mem_free(void *memory)
{
    HeapFree(GetProcessHeap(), 0, memory);
}

static BOOL CDECL get_next_cabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
    sprintf(pccab->szCab, pv, pccab->iCab);
    return TRUE;
}

static LONG CDECL progress(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
    return 0;
}

static int CDECL file_placed(PCCAB pccab, char *pszFile, LONG cbFile,
                             BOOL fContinuation, void *pv)
{
    return 0;
}

static INT_PTR CDECL fci_open(char *pszFile, int oflag, int pmode, int *err, void *pv)
{
    HANDLE handle;
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    
    dwAccess = GENERIC_READ | GENERIC_WRITE;
    /* FILE_SHARE_DELETE is not supported by Windows Me/98/95 */
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    if (GetFileAttributesA(pszFile) != INVALID_FILE_ATTRIBUTES)
        dwCreateDisposition = OPEN_EXISTING;
    else
        dwCreateDisposition = CREATE_NEW;

    handle = CreateFileA(pszFile, dwAccess, dwShareMode, NULL,
                         dwCreateDisposition, 0, NULL);

    ok(handle != INVALID_HANDLE_VALUE, "Failed to CreateFile %s\n", pszFile);

    return (INT_PTR)handle;
}

static UINT CDECL fci_read(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwRead;
    BOOL res;
    
    res = ReadFile(handle, memory, cb, &dwRead, NULL);
    ok(res, "Failed to ReadFile\n");

    return dwRead;
}

static UINT CDECL fci_write(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD dwWritten;
    BOOL res;

    res = WriteFile(handle, memory, cb, &dwWritten, NULL);
    ok(res, "Failed to WriteFile\n");

    return dwWritten;
}

static int CDECL fci_close(INT_PTR hf, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    ok(CloseHandle(handle), "Failed to CloseHandle\n");

    return 0;
}

static LONG CDECL fci_seek(INT_PTR hf, LONG dist, int seektype, int *err, void *pv)
{
    HANDLE handle = (HANDLE)hf;
    DWORD ret;
    
    ret = SetFilePointer(handle, dist, NULL, seektype);
    ok(ret != INVALID_SET_FILE_POINTER, "Failed to SetFilePointer\n");

    return ret;
}

static int CDECL fci_delete(char *pszFile, int *err, void *pv)
{
    BOOL ret = DeleteFileA(pszFile);
    ok(ret, "Failed to DeleteFile %s\n", pszFile);

    return 0;
}

static void init_functionpointers(void)
{
    HMODULE hmsi = GetModuleHandleA("msi.dll");

#define GET_PROC(mod, func) \
    p ## func = (void*)GetProcAddress(mod, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(hmsi, MsiQueryComponentStateA);
    GET_PROC(hmsi, MsiSourceListEnumSourcesA);
    GET_PROC(hmsi, MsiSourceListGetInfoA);

    hsrclient = LoadLibraryA("srclient.dll");
    GET_PROC(hsrclient, SRRemoveRestorePoint);
    GET_PROC(hsrclient, SRSetRestorePointA);

#undef GET_PROC
}

static BOOL check_win9x(void)
{
    SC_HANDLE scm;

    scm = OpenSCManager(NULL, NULL, GENERIC_ALL);
    if (!scm && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
        return TRUE;

    CloseServiceHandle(scm);

    return FALSE;
}

static void get_user_sid(LPSTR *usersid)
{
    HANDLE token;
    BYTE buf[1024];
    DWORD size;
    PTOKEN_USER user;
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
    static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);

    *usersid = NULL;
    pConvertSidToStringSidA = (void *)GetProcAddress(hadvapi32, "ConvertSidToStringSidA");
    if (!pConvertSidToStringSidA)
        return;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    size = sizeof(buf);
    GetTokenInformation(token, TokenUser, buf, size, &size);
    user = (PTOKEN_USER)buf;
    pConvertSidToStringSidA(user->User.Sid, usersid);
    CloseHandle(token);
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

static BOOL CDECL get_temp_file(char *pszTempName, int cbTempName, void *pv)
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

static INT_PTR CDECL get_open_info(char *pszName, USHORT *pdate, USHORT *ptime,
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

static BOOL add_file(HFCI hfci, const char *file, TCOMP compress)
{
    char path[MAX_PATH];
    char filename[MAX_PATH];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    lstrcpyA(filename, file);

    return FCIAddFile(hfci, path, filename, FALSE, get_next_cabinet,
                      progress, get_open_info, compress);
}

static void set_cab_parameters(PCCAB pCabParams, const CHAR *name, DWORD max_size)
{
    ZeroMemory(pCabParams, sizeof(CCAB));

    pCabParams->cb = max_size;
    pCabParams->cbFolderThresh = FOLDER_THRESHOLD;
    pCabParams->setID = 0xbeef;
    pCabParams->iCab = 1;
    lstrcpyA(pCabParams->szCabPath, CURR_DIR);
    lstrcatA(pCabParams->szCabPath, "\\");
    lstrcpyA(pCabParams->szCab, name);
}

static void create_cab_file(const CHAR *name, DWORD max_size, const CHAR *files)
{
    CCAB cabParams;
    LPCSTR ptr;
    HFCI hfci;
    ERF erf;
    BOOL res;

    set_cab_parameters(&cabParams, name, max_size);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, NULL);

    ok(hfci != NULL, "Failed to create an FCI context\n");

    ptr = files;
    while (*ptr)
    {
        res = add_file(hfci, ptr, tcompTYPE_MSZIP);
        ok(res, "Failed to add file: %s\n", ptr);
        ptr += lstrlen(ptr) + 1;
    }

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");
}

static BOOL get_program_files_dir(LPSTR buf, LPSTR buf2)
{
    HKEY hkey;
    DWORD type, size;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   "Software\\Microsoft\\Windows\\CurrentVersion", &hkey))
        return FALSE;

    size = MAX_PATH;
    if (RegQueryValueExA(hkey, "ProgramFilesDir", 0, &type, (LPBYTE)buf, &size)) {
        RegCloseKey(hkey);
        return FALSE;
    }

    size = MAX_PATH;
    if (RegQueryValueExA(hkey, "CommonFilesDir", 0, &type, (LPBYTE)buf2, &size)) {
        RegCloseKey(hkey);
        return FALSE;
    }

    RegCloseKey(hkey);
    return TRUE;
}

static void create_file_data(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, data, strlen(data), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define create_file(name, size) create_file_data(name, name, size)

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

static BOOL delete_cf(const CHAR *rel_path, BOOL is_file)
{
    CHAR path[MAX_PATH];

    lstrcpyA(path, COMMON_FILES_DIR);
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
    DeleteFileA("msitest\\service.exe");
    DeleteFileA("msitest\\filename");
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

static void write_msi_summary_info(MSIHANDLE db, INT wordcount)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(db, NULL, 5, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_TEMPLATE, VT_LPSTR, 0, NULL, ";1033");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_REVNUMBER, VT_LPSTR, 0, NULL,
                                   "{004757CA-5092-49c2-AD20-28E1CE0DF5F2}");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_PAGECOUNT, VT_I4, 100, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_WORDCOUNT, VT_I4, wordcount, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_TITLE, VT_LPSTR, 0, NULL, "MSITEST");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* write the summary changes back to the stream */
    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

#define create_database(name, tables, num_tables) \
    create_database_wordcount(name, tables, num_tables, 0);

static void create_database_wordcount(const CHAR *name, const msi_table *tables,
                                      int num_tables, INT wordcount)
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

    write_msi_summary_info(db, wordcount);

    r = MsiDatabaseCommit(db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(db);
}

static void check_service_is_installed(void)
{
    SC_HANDLE scm, service;
    BOOL res;

    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(scm != NULL, "Failed to open the SC Manager\n");

    service = OpenService(scm, "TestService", SC_MANAGER_ALL_ACCESS);
    ok(service != NULL, "Failed to open TestService\n");

    res = DeleteService(service);
    ok(res, "Failed to delete TestService\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
}

static BOOL notify_system_change(DWORD event_type, STATEMGRSTATUS *status)
{
    RESTOREPOINTINFOA spec;

    spec.dwEventType = event_type;
    spec.dwRestorePtType = APPLICATION_INSTALL;
    spec.llSequenceNumber = status->llSequenceNumber;
    lstrcpyA(spec.szDescription, "msitest restore point");

    return pSRSetRestorePointA(&spec, status);
}

static void remove_restore_point(DWORD seq_number)
{
    DWORD res;

    res = pSRRemoveRestorePoint(seq_number);
    if (res != ERROR_SUCCESS)
        trace("Failed to remove the restore point : %08x\n", res);
}

static void test_MsiInstallProduct(void)
{
    UINT r;
    CHAR path[MAX_PATH];
    LONG res;
    HKEY hkey;
    DWORD num, size, type;

    if (on_win9x)
    {
        win_skip("Services are not implemented on Win9x and WinMe\n");
        return;
    }

    create_test_files();
    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

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
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
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

    check_service_is_installed();

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest");

    delete_test_files();
}

static void test_MsiSetComponentState(void)
{
    INSTALLSTATE installed, action;
    MSIHANDLE package;
    char path[MAX_PATH];
    UINT r;

    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

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

    r = MsiGetComponentState(package, "dangler", &installed, &action);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(installed == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", installed);
    ok(action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiSetComponentState(package, "dangler", INSTALLSTATE_SOURCE);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(package);
    CoUninitialize();

    DeleteFileA(msifile);
}

static void test_packagecoltypes(void)
{
    MSIHANDLE hdb, view, rec;
    char path[MAX_PATH];
    LPCSTR query;
    UINT r, count;

    create_database(msifile, tables, sizeof(tables) / sizeof(msi_table));

    CoInitialize(NULL);

    lstrcpy(path, CURR_DIR);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    r = MsiOpenDatabase(path, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    query = "SELECT * FROM `Media`";
    r = MsiDatabaseOpenView( hdb, query, &view );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewGetColumnInfo( view, MSICOLINFO_NAMES, &rec );
    count = MsiRecordGetFieldCount( rec );
    ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "DiskId"), "wrong column label\n");
    ok(check_record(rec, 2, "LastSequence"), "wrong column label\n");
    ok(check_record(rec, 3, "DiskPrompt"), "wrong column label\n");
    ok(check_record(rec, 4, "Cabinet"), "wrong column label\n");
    ok(check_record(rec, 5, "VolumeLabel"), "wrong column label\n");
    ok(check_record(rec, 6, "Source"), "wrong column label\n");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo( view, MSICOLINFO_TYPES, &rec );
    count = MsiRecordGetFieldCount( rec );
    ok(r == ERROR_SUCCESS, "MsiViewGetColumnInfo failed\n");
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "i2"), "wrong column label\n");
    ok(check_record(rec, 2, "i4"), "wrong column label\n");
    ok(check_record(rec, 3, "L64"), "wrong column label\n");
    ok(check_record(rec, 4, "S255"), "wrong column label\n");
    ok(check_record(rec, 5, "S32"), "wrong column label\n");
    ok(check_record(rec, 6, "S72"), "wrong column label\n");

    MsiCloseHandle(rec);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    CoUninitialize();

    DeleteFile(msifile);
}

static void create_cc_test_files(void)
{
    CCAB cabParams;
    HFCI hfci;
    ERF erf;
    static CHAR cab_context[] = "test%d.cab";
    BOOL res;

    create_file("maximus", 500);
    create_file("augustus", 50000);
    create_file("tiberius", 500);
    create_file("caesar", 500);

    set_cab_parameters(&cabParams, "test1.cab", 40000);

    hfci = FCICreate(&erf, file_placed, mem_alloc, mem_free, fci_open,
                      fci_read, fci_write, fci_close, fci_seek, fci_delete,
                      get_temp_file, &cabParams, cab_context);
    ok(hfci != NULL, "Failed to create an FCI context\n");

    res = add_file(hfci, "maximus", tcompTYPE_NONE);
    ok(res, "Failed to add file maximus\n");

    res = add_file(hfci, "augustus", tcompTYPE_NONE);
    ok(res, "Failed to add file augustus\n");

    res = add_file(hfci, "tiberius", tcompTYPE_NONE);
    ok(res, "Failed to add file tiberius\n");

    res = FCIFlushCabinet(hfci, FALSE, get_next_cabinet, progress);
    ok(res, "Failed to flush the cabinet\n");

    res = FCIDestroy(hfci);
    ok(res, "Failed to destroy the cabinet\n");

    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    DeleteFile("maximus");
    DeleteFile("augustus");
    DeleteFile("tiberius");
    DeleteFile("caesar");
}

static void delete_cab_files(void)
{
    SHFILEOPSTRUCT shfl;
    CHAR path[MAX_PATH+10];

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\*.cab");
    path[strlen(path) + 1] = '\0';

    shfl.hwnd = NULL;
    shfl.wFunc = FO_DELETE;
    shfl.pFrom = path;
    shfl.pTo = NULL;
    shfl.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_NORECURSION | FOF_SILENT;

    SHFileOperation(&shfl);
}

static void test_continuouscabs(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, cc_tables, sizeof(cc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_SUCCESS) /* win9x has a problem with this */
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);

    create_cc_test_files();
    create_database(msifile, cc2_tables, sizeof(cc2_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest\\tiberius", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    delete_cab_files();
    DeleteFile(msifile);
}

static void test_caborder(void)
{
    UINT r;

    create_file("imperator", 100);
    create_file("maximus", 500);
    create_file("augustus", 50000);
    create_file("caesar", 500);

    create_database(msifile, cc_tables, sizeof(cc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();

    create_cab_file("test1.cab", MEDIA_SIZE, "imperator\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "maximus\0augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);

    create_cc_test_files();
    create_database(msifile, co_tables, sizeof(co_tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    ok(!delete_pf("msitest", FALSE), "File is installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);

    create_cc_test_files();
    create_database(msifile, co2_tables, sizeof(co2_tables) / sizeof(msi_table));

    r = MsiInstallProductA(msifile, NULL);
    ok(!delete_pf("msitest\\caesar", TRUE), "File is installed\n");
    todo_wine
    {
        ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
        ok(!delete_pf("msitest\\augustus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest\\maximus", TRUE), "File is installed\n");
        ok(!delete_pf("msitest", FALSE), "File is installed\n");
    }

    delete_cab_files();
    DeleteFile("imperator");
    DeleteFile("maximus");
    DeleteFile("augustus");
    DeleteFile("caesar");
    DeleteFile(msifile);
}

static void test_mixedmedia(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_file("msitest\\augustus", 500);
    create_file("caesar", 500);

    create_database(msifile, mm_tables, sizeof(mm_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    create_cab_file("test1.cab", MEDIA_SIZE, "caesar\0");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile("msitest\\maximus");
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
    DeleteFile("caesar");
    DeleteFile("test1.cab");
    DeleteFile(msifile);
}

static void test_samesequence(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, ss_tables, sizeof(ss_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_SUCCESS) /* win9x has a problem with this */
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);
}

static void test_uiLevelFlags(void)
{
    UINT r;

    create_cc_test_files();
    create_database(msifile, ui_tables, sizeof(ui_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE | INSTALLUILEVEL_SOURCERESONLY, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_SUCCESS) /* win9x has a problem with this */
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
        ok(!delete_pf("msitest\\maximus", TRUE), "UI install occurred, but execute-only was requested.\n");
        ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }

    delete_cab_files();
    DeleteFile(msifile);
}

static BOOL file_matches(LPSTR path)
{
    CHAR buf[MAX_PATH];
    HANDLE file;
    DWORD size;

    file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL);

    ZeroMemory(buf, MAX_PATH);
    ReadFile(file, buf, 15, &size, NULL);
    CloseHandle(file);

    return !lstrcmp(buf, "msitest\\maximus");
}

static void test_readonlyfile(void)
{
    UINT r;
    DWORD size;
    HANDLE file;
    CHAR path[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, rof_tables, sizeof(rof_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    lstrcpy(path, PROG_FILES_DIR);
    lstrcat(path, "\\msitest");
    CreateDirectory(path, NULL);

    lstrcat(path, "\\maximus");
    file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, CREATE_NEW, FILE_ATTRIBUTE_READONLY, NULL);

    WriteFile(file, "readonlyfile", strlen("readonlyfile"), &size, NULL);
    CloseHandle(file);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(file_matches(path), "Expected file to be overwritten\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    DeleteFile(msifile);
}

static void test_setdirproperty(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, sdp_tables, sizeof(sdp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_cf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_cf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static void test_cabisextracted(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\gaius", 500);
    create_file("maximus", 500);
    create_file("augustus", 500);
    create_file("caesar", 500);

    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");
    create_cab_file("test2.cab", MEDIA_SIZE, "augustus\0");
    create_cab_file("test3.cab", MEDIA_SIZE, "caesar\0");

    create_database(msifile, cie_tables, sizeof(cie_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\gaius", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    delete_cab_files();
    DeleteFile(msifile);
    DeleteFile("maximus");
    DeleteFile("augustus");
    DeleteFile("caesar");
    DeleteFile("msitest\\gaius");
    RemoveDirectory("msitest");
}

static void test_concurrentinstall(void)
{
    UINT r;
    CHAR path[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    CreateDirectoryA("msitest\\msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_file("msitest\\msitest\\augustus", 500);

    create_database(msifile, ci_tables, sizeof(ci_tables) / sizeof(msi_table));

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\msitest\\concurrent.msi");
    create_database(path, ci2_tables, sizeof(ci2_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    if (!delete_pf("msitest\\augustus", TRUE))
        trace("concurrent installs not supported\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile(path);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\msitest\\augustus");
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest\\msitest");
    RemoveDirectory("msitest");
}

static void test_setpropertyfolder(void)
{
    UINT r;
    CHAR path[MAX_PATH];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\msitest\\added");

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, spf_tables, sizeof(spf_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    if (GetFileAttributesA(path) == FILE_ATTRIBUTE_DIRECTORY)
    {
        ok(delete_pf("msitest\\added\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest\\added", FALSE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }
    else
    {
        trace("changing folder property not supported\n");
        ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "File not installed\n");
    }

    /* Delete the files in the temp (current) folder */
    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static BOOL file_exists(LPCSTR file)
{
    return GetFileAttributes(file) != INVALID_FILE_ATTRIBUTES;
}

static BOOL pf_exists(LPCSTR file)
{
    CHAR path[MAX_PATH];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    return file_exists(path);
}

static void delete_pfmsitest_files(void)
{
    SHFILEOPSTRUCT shfl;
    CHAR path[MAX_PATH+11];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\msitest\\*");
    path[strlen(path) + 1] = '\0';

    shfl.hwnd = NULL;
    shfl.wFunc = FO_DELETE;
    shfl.pFrom = path;
    shfl.pTo = NULL;
    shfl.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_NORECURSION | FOF_SILENT;

    SHFileOperation(&shfl);

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\msitest");
    RemoveDirectoryA(path);
}

static void check_reg_str(HKEY prodkey, LPCSTR name, LPCSTR expected, BOOL bcase, DWORD line)
{
    char val[MAX_PATH];
    DWORD size, type;
    LONG res;

    size = MAX_PATH;
    val[0] = '\0';
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)val, &size);

    if (res != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ && type != REG_MULTI_SZ))
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    if (!expected)
        ok_(__FILE__, line)(lstrlenA(val) == 0, "Expected empty string, got %s\n", val);
    else
    {
        if (bcase)
            ok_(__FILE__, line)(!lstrcmpA(val, expected), "Expected %s, got %s\n", expected, val);
        else
            ok_(__FILE__, line)(!lstrcmpiA(val, expected), "Expected %s, got %s\n", expected, val);
    }
}

static void check_reg_dword(HKEY prodkey, LPCSTR name, DWORD expected, DWORD line)
{
    DWORD val, size, type;
    LONG res;

    size = sizeof(DWORD);
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)&val, &size);

    if (res != ERROR_SUCCESS || type != REG_DWORD)
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    ok_(__FILE__, line)(val == expected, "Expected %d, got %d\n", expected, val);
}

static void check_reg_dword2(HKEY prodkey, LPCSTR name, DWORD expected1, DWORD expected2, DWORD line)
{
    DWORD val, size, type;
    LONG res;

    size = sizeof(DWORD);
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)&val, &size);

    if (res != ERROR_SUCCESS || type != REG_DWORD)
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    ok_(__FILE__, line)(val == expected1 || val == expected2, "Expected %d or %d, got %d\n", expected1, expected2, val);
}

static void check_reg_dword3(HKEY prodkey, LPCSTR name, DWORD expected1, DWORD expected2, DWORD expected3, DWORD line)
{
    DWORD val, size, type;
    LONG res;

    size = sizeof(DWORD);
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)&val, &size);

    if (res != ERROR_SUCCESS || type != REG_DWORD)
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    ok_(__FILE__, line)(val == expected1 || val == expected2 || val == expected3,
                        "Expected %d, %d or %d, got %d\n", expected1, expected2, expected3, val);
}

#define CHECK_REG_STR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, TRUE, __LINE__);

#define CHECK_DEL_REG_STR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, TRUE, __LINE__); \
    RegDeleteValueA(prodkey, name);

#define CHECK_REG_ISTR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, FALSE, __LINE__);

#define CHECK_DEL_REG_ISTR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, FALSE, __LINE__); \
    RegDeleteValueA(prodkey, name);

#define CHECK_REG_DWORD(prodkey, name, expected) \
    check_reg_dword(prodkey, name, expected, __LINE__);

#define CHECK_DEL_REG_DWORD(prodkey, name, expected) \
    check_reg_dword(prodkey, name, expected, __LINE__); \
    RegDeleteValueA(prodkey, name);

#define CHECK_REG_DWORD2(prodkey, name, expected1, expected2) \
    check_reg_dword2(prodkey, name, expected1, expected2, __LINE__);

#define CHECK_DEL_REG_DWORD2(prodkey, name, expected1, expected2) \
    check_reg_dword2(prodkey, name, expected1, expected2, __LINE__); \
    RegDeleteValueA(prodkey, name);

#define CHECK_REG_DWORD3(prodkey, name, expected1, expected2, expected3) \
    check_reg_dword3(prodkey, name, expected1, expected2, expected3, __LINE__);

#define CHECK_DEL_REG_DWORD3(prodkey, name, expected1, expected2, expected3) \
    check_reg_dword3(prodkey, name, expected1, expected2, expected3, __LINE__); \
    RegDeleteValueA(prodkey, name);

static void get_date_str(LPSTR date)
{
    SYSTEMTIME systime;

    static const char date_fmt[] = "%d%02d%02d";
    GetLocalTime(&systime);
    sprintf(date, date_fmt, systime.wYear, systime.wMonth, systime.wDay);
}

static void test_publish_registerproduct(void)
{
    UINT r;
    LONG res;
    HKEY hkey;
    HKEY props, usage;
    LPSTR usersid;
    char date[MAX_PATH];
    char temp[MAX_PATH];
    char keypath[MAX_PATH];

    static const CHAR uninstall[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                    "\\Uninstall\\{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";
    static const CHAR userdata[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer"
                                   "\\UserData\\%s\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR ugkey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer"
                                "\\UpgradeCodes\\51AAE0C44620A5E4788506E91F249BD2";
    static const CHAR userugkey[] = "Software\\Microsoft\\Installer\\UpgradeCodes"
                                    "\\51AAE0C44620A5E4788506E91F249BD2";

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    get_date_str(date);
    GetTempPath(MAX_PATH, temp);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* RegisterProduct */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_CURRENT_USER, userugkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, uninstall, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(hkey, "InstallDate", date);
    CHECK_DEL_REG_STR(hkey, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(hkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "Publisher", "Wine");
    CHECK_DEL_REG_STR(hkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "AuthorizedCDFPrefix", NULL);
    CHECK_DEL_REG_STR(hkey, "Comments", NULL);
    CHECK_DEL_REG_STR(hkey, "Contact", NULL);
    CHECK_DEL_REG_STR(hkey, "HelpLink", NULL);
    CHECK_DEL_REG_STR(hkey, "HelpTelephone", NULL);
    CHECK_DEL_REG_STR(hkey, "InstallLocation", NULL);
    CHECK_DEL_REG_STR(hkey, "Readme", NULL);
    CHECK_DEL_REG_STR(hkey, "Size", NULL);
    CHECK_DEL_REG_STR(hkey, "URLInfoAbout", NULL);
    CHECK_DEL_REG_STR(hkey, "URLUpdateInfo", NULL);
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(hkey, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(hkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_DEL_REG_DWORD3(hkey, "EstimatedSize", 12, -12, 4);
    }

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    sprintf(keypath, userdata, usersid);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegOpenKeyA(hkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    RegDeleteValueA(props, "LocalPackage"); /* LocalPackage is nondeterministic */
    CHECK_DEL_REG_STR(props, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(props, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(props, "InstallDate", date);
    CHECK_DEL_REG_STR(props, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(props, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "Publisher", "Wine");
    CHECK_DEL_REG_STR(props, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "AuthorizedCDFPrefix", NULL);
    CHECK_DEL_REG_STR(props, "Comments", NULL);
    CHECK_DEL_REG_STR(props, "Contact", NULL);
    CHECK_DEL_REG_STR(props, "HelpLink", NULL);
    CHECK_DEL_REG_STR(props, "HelpTelephone", NULL);
    CHECK_DEL_REG_STR(props, "InstallLocation", NULL);
    CHECK_DEL_REG_STR(props, "Readme", NULL);
    CHECK_DEL_REG_STR(props, "Size", NULL);
    CHECK_DEL_REG_STR(props, "URLInfoAbout", NULL);
    CHECK_DEL_REG_STR(props, "URLUpdateInfo", NULL);
    CHECK_DEL_REG_DWORD(props, "Language", 1033);
    CHECK_DEL_REG_DWORD(props, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(props, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(props, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(props, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_DEL_REG_DWORD3(props, "EstimatedSize", 12, -12, 4);
    }

    RegDeleteKeyA(props, "");
    RegCloseKey(props);

    res = RegOpenKeyA(hkey, "Usage", &usage);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    RegDeleteKeyA(usage, "");
    RegCloseKey(usage);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, ugkey, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", NULL);

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    /* RegisterProduct, machine */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, userugkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, uninstall, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(hkey, "InstallDate", date);
    CHECK_DEL_REG_STR(hkey, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(hkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "Publisher", "Wine");
    CHECK_DEL_REG_STR(hkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "AuthorizedCDFPrefix", NULL);
    CHECK_DEL_REG_STR(hkey, "Comments", NULL);
    CHECK_DEL_REG_STR(hkey, "Contact", NULL);
    CHECK_DEL_REG_STR(hkey, "HelpLink", NULL);
    CHECK_DEL_REG_STR(hkey, "HelpTelephone", NULL);
    CHECK_DEL_REG_STR(hkey, "InstallLocation", NULL);
    CHECK_DEL_REG_STR(hkey, "Readme", NULL);
    CHECK_DEL_REG_STR(hkey, "Size", NULL);
    CHECK_DEL_REG_STR(hkey, "URLInfoAbout", NULL);
    CHECK_DEL_REG_STR(hkey, "URLUpdateInfo", NULL);
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(hkey, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(hkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_DEL_REG_DWORD3(hkey, "EstimatedSize", 12, -12, 4);
    }

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    sprintf(keypath, userdata, "S-1-5-18");
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegOpenKeyA(hkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    RegDeleteValueA(props, "LocalPackage"); /* LocalPackage is nondeterministic */
    CHECK_DEL_REG_STR(props, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(props, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(props, "InstallDate", date);
    CHECK_DEL_REG_STR(props, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(props, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "Publisher", "Wine");
    CHECK_DEL_REG_STR(props, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "AuthorizedCDFPrefix", NULL);
    CHECK_DEL_REG_STR(props, "Comments", NULL);
    CHECK_DEL_REG_STR(props, "Contact", NULL);
    CHECK_DEL_REG_STR(props, "HelpLink", NULL);
    CHECK_DEL_REG_STR(props, "HelpTelephone", NULL);
    CHECK_DEL_REG_STR(props, "InstallLocation", NULL);
    CHECK_DEL_REG_STR(props, "Readme", NULL);
    CHECK_DEL_REG_STR(props, "Size", NULL);
    CHECK_DEL_REG_STR(props, "URLInfoAbout", NULL);
    CHECK_DEL_REG_STR(props, "URLUpdateInfo", NULL);
    CHECK_DEL_REG_DWORD(props, "Language", 1033);
    CHECK_DEL_REG_DWORD(props, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(props, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(props, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(props, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_DEL_REG_DWORD3(props, "EstimatedSize", 12, -12, 4);
    }

    RegDeleteKeyA(props, "");
    RegCloseKey(props);

    res = RegOpenKeyA(hkey, "Usage", &usage);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    RegDeleteKeyA(usage, "");
    RegCloseKey(usage);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, ugkey, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", NULL);

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    HeapFree(GetProcessHeap(), 0, usersid);
}

static void test_publish_publishproduct(void)
{
    UINT r;
    LONG res;
    LPSTR usersid;
    HKEY sourcelist, net, props;
    HKEY hkey, patches, media;
    CHAR keypath[MAX_PATH];
    CHAR temp[MAX_PATH];
    CHAR path[MAX_PATH];

    static const CHAR prodpath[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                   "\\Installer\\UserData\\%s\\Products"
                                   "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR cuprodpath[] = "Software\\Microsoft\\Installer\\Products"
                                     "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR cuupgrades[] = "Software\\Microsoft\\Installer\\UpgradeCodes"
                                     "\\51AAE0C44620A5E4788506E91F249BD2";
    static const CHAR badprod[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                  "\\Installer\\Products"
                                  "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR machprod[] = "Installer\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR machup[] = "Installer\\UpgradeCodes\\51AAE0C44620A5E4788506E91F249BD2";

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    GetTempPath(MAX_PATH, temp);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* PublishProduct, current user */
    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, badprod, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    sprintf(keypath, prodpath, usersid);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegOpenKeyA(hkey, "InstallProperties", &props);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(hkey, "Patches", &patches);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        CHECK_DEL_REG_STR(patches, "AllPatches", NULL);
    }

    RegDeleteKeyA(patches, "");
    RegCloseKey(patches);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cuprodpath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "ProductName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "PackageCode", "AC75740029052c94DA02821EECD05F2F");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "AuthorizedLUAApp", 0);
    CHECK_DEL_REG_DWORD(hkey, "Assignment", 0);
    CHECK_DEL_REG_DWORD(hkey, "AdvertiseFlags", 0x184);
    CHECK_DEL_REG_DWORD(hkey, "InstanceType", 0);
    CHECK_DEL_REG_STR(hkey, "Clients", ":");

    res = RegOpenKeyA(hkey, "SourceList", &sourcelist);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(path, "n;1;");
    lstrcatA(path, temp);
    CHECK_DEL_REG_STR(sourcelist, "LastUsedSource", path);
    CHECK_DEL_REG_STR(sourcelist, "PackageName", "msitest.msi");

    res = RegOpenKeyA(sourcelist, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(net, "1", temp);

    RegDeleteKeyA(net, "");
    RegCloseKey(net);

    res = RegOpenKeyA(sourcelist, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(media, "1", "DISK1;");

    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(sourcelist, "");
    RegCloseKey(sourcelist);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cuupgrades, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", NULL);

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    /* PublishProduct, machine */
    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, badprod, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    sprintf(keypath, prodpath, "S-1-5-18");
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegOpenKeyA(hkey, "InstallProperties", &props);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(hkey, "Patches", &patches);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        CHECK_DEL_REG_STR(patches, "AllPatches", NULL);
    }

    RegDeleteKeyA(patches, "");
    RegCloseKey(patches);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, machprod, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "ProductName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "PackageCode", "AC75740029052c94DA02821EECD05F2F");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "AuthorizedLUAApp", 0);
    todo_wine CHECK_DEL_REG_DWORD(hkey, "Assignment", 1);
    CHECK_DEL_REG_DWORD(hkey, "AdvertiseFlags", 0x184);
    CHECK_DEL_REG_DWORD(hkey, "InstanceType", 0);
    CHECK_DEL_REG_STR(hkey, "Clients", ":");

    res = RegOpenKeyA(hkey, "SourceList", &sourcelist);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(path, "n;1;");
    lstrcatA(path, temp);
    CHECK_DEL_REG_STR(sourcelist, "LastUsedSource", path);
    CHECK_DEL_REG_STR(sourcelist, "PackageName", "msitest.msi");

    res = RegOpenKeyA(sourcelist, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(net, "1", temp);

    RegDeleteKeyA(net, "");
    RegCloseKey(net);

    res = RegOpenKeyA(sourcelist, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(media, "1", "DISK1;");

    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(sourcelist, "");
    RegCloseKey(sourcelist);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, machup, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", NULL);

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    HeapFree(GetProcessHeap(), 0, usersid);
}

static void test_publish_publishfeatures(void)
{
    UINT r;
    LONG res;
    HKEY hkey;
    LPSTR usersid;
    CHAR keypath[MAX_PATH];

    static const CHAR cupath[] = "Software\\Microsoft\\Installer\\Features"
                                 "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR udpath[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                 "\\Installer\\UserData\\%s\\Products"
                                 "\\84A88FD7F6998CE40A22FB59F6B9C2BB\\Features";
    static const CHAR featkey[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                  "\\Installer\\Features";
    static const CHAR classfeat[] = "Software\\Classes\\Installer\\Features"
                                    "\\84A88FD7F6998CE40A22FB59F6B9C2BB";

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* PublishFeatures, current user */
    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, featkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, classfeat, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cupath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(hkey, "feature", "");
    CHECK_REG_STR(hkey, "montecristo", "");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    sprintf(keypath, udpath, usersid);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(hkey, "feature", "VGtfp^p+,?82@JU1j_KE");
    CHECK_REG_STR(hkey, "montecristo", "VGtfp^p+,?82@JU1j_KE");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    /* PublishFeatures, machine */
    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, featkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cupath, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, classfeat, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(hkey, "feature", "");
    CHECK_REG_STR(hkey, "montecristo", "");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    sprintf(keypath, udpath, "S-1-5-18");
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(hkey, "feature", "VGtfp^p+,?82@JU1j_KE");
    CHECK_REG_STR(hkey, "montecristo", "VGtfp^p+,?82@JU1j_KE");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    HeapFree(GetProcessHeap(), 0, usersid);
}

static LPSTR reg_get_val_str(HKEY hkey, LPCSTR name)
{
    DWORD len = 0;
    LPSTR val;
    LONG r;

    r = RegQueryValueExA(hkey, name, NULL, NULL, NULL, &len);
    if (r != ERROR_SUCCESS)
        return NULL;

    len += sizeof (WCHAR);
    val = HeapAlloc(GetProcessHeap(), 0, len);
    if (!val) return NULL;
    val[0] = 0;
    RegQueryValueExA(hkey, name, NULL, NULL, (LPBYTE)val, &len);
    return val;
}

static void get_owner_company(LPSTR *owner, LPSTR *company)
{
    LONG res;
    HKEY hkey;

    *owner = *company = NULL;

    res = RegOpenKeyA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\MS Setup (ACME)\\User Info", &hkey);
    if (res == ERROR_SUCCESS)
    {
        *owner = reg_get_val_str(hkey, "DefName");
        *company = reg_get_val_str(hkey, "DefCompany");
        RegCloseKey(hkey);
    }

    if (!*owner || !*company)
    {
        res = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                          "Software\\Microsoft\\Windows\\CurrentVersion", &hkey);
        if (res == ERROR_SUCCESS)
        {
            *owner = reg_get_val_str(hkey, "RegisteredOwner");
            *company = reg_get_val_str(hkey, "RegisteredOrganization");
            RegCloseKey(hkey);
        }
    }

    if (!*owner || !*company)
    {
        res = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                          "Software\\Microsoft\\Windows NT\\CurrentVersion", &hkey);
        if (res == ERROR_SUCCESS)
        {
            *owner = reg_get_val_str(hkey, "RegisteredOwner");
            *company = reg_get_val_str(hkey, "RegisteredOrganization");
            RegCloseKey(hkey);
        }
    }
}

static void test_publish_registeruser(void)
{
    UINT r;
    LONG res;
    HKEY props;
    LPSTR usersid;
    LPSTR owner, company;
    CHAR keypath[MAX_PATH];

    static const CHAR keyfmt[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\"
        "UserData\\%s\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB\\InstallProperties";

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    get_owner_company(&owner, &company);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* RegisterUser, per-user */
    r = MsiInstallProductA(msifile, "REGISTER_USER=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    sprintf(keypath, keyfmt, usersid);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(props, "ProductID", "none");
    CHECK_REG_STR(props, "RegCompany", company);
    CHECK_REG_STR(props, "RegOwner", owner);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyA(props, "");
    RegCloseKey(props);

    /* RegisterUser, machine */
    r = MsiInstallProductA(msifile, "REGISTER_USER=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    sprintf(keypath, keyfmt, "S-1-5-18");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(props, "ProductID", "none");
    CHECK_REG_STR(props, "RegCompany", company);
    CHECK_REG_STR(props, "RegOwner", owner);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyA(props, "");
    RegCloseKey(props);

    HeapFree(GetProcessHeap(), 0, company);
    HeapFree(GetProcessHeap(), 0, owner);

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static void test_publish_processcomponents(void)
{
    UINT r;
    LONG res;
    DWORD size;
    HKEY comp, hkey;
    LPSTR usersid;
    CHAR val[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR program_files_maximus[MAX_PATH];

    static const CHAR keyfmt[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\"
        "UserData\\%s\\Components\\%s";
    static const CHAR compkey[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components";

    get_user_sid(&usersid);
    if (!usersid)
    {
        skip("ConvertSidToStringSidA is not available\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, ppc_tables, sizeof(ppc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* ProcessComponents, per-user */
    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    sprintf(keypath, keyfmt, usersid, "CBABC2FDCCB35E749A8944D8C1C098B5");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(program_files_maximus,PROG_FILES_DIR);
    lstrcatA(program_files_maximus,"\\msitest\\maximus");

    ok(!lstrcmpA(val, program_files_maximus),
       "Expected \"%s\", got \"%s\"\n", program_files_maximus, val);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, compkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyA(comp, "");
    RegCloseKey(comp);

    sprintf(keypath, keyfmt, usersid, "241C3DA58FECD0945B9687D408766058");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(val, "01\\msitest\\augustus"),
       "Expected \"01\\msitest\\augustus\", got \"%s\"\n", val);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, compkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyA(comp, "");
    RegCloseKey(comp);

    /* ProcessComponents, machine */
    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    sprintf(keypath, keyfmt, "S-1-5-18", "CBABC2FDCCB35E749A8944D8C1C098B5");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(val, program_files_maximus),
       "Expected \"%s\", got \"%s\"\n", program_files_maximus, val);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, compkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyA(comp, "");
    RegCloseKey(comp);

    sprintf(keypath, keyfmt, "S-1-5-18", "241C3DA58FECD0945B9687D408766058");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(val, "01\\msitest\\augustus"),
       "Expected \"01\\msitest\\augustus\", got \"%s\"\n", val);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, compkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyA(comp, "");
    RegCloseKey(comp);

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static void test_publish(void)
{
    UINT r;
    LONG res;
    HKEY uninstall, prodkey;
    INSTALLSTATE state;
    CHAR prodcode[] = "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";
    char date[MAX_PATH];
    char temp[MAX_PATH];

    static const CHAR subkey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

    if (!pMsiQueryComponentStateA)
    {
        skip("MsiQueryComponentStateA is not available\n");
        return;
    }

    get_date_str(date);
    GetTempPath(MAX_PATH, temp);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, subkey, &uninstall);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* nothing published */
    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* PublishProduct and RegisterProduct */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1 PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -12);
    }

    RegCloseKey(prodkey);

    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File deleted\n");
    ok(pf_exists("msitest"), "File deleted\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -12);
    }

    RegCloseKey(prodkey);

    /* no UnpublishFeatures */
    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File deleted\n");
    todo_wine
    {
        ok(!pf_exists("msitest"), "File deleted\n");
    }

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -12);
    }

    RegCloseKey(prodkey);

    /* UnpublishFeatures, only feature removed.  Only works when entire product is removed */
    r = MsiInstallProductA(msifile, "UNPUBLISH_FEATURES=1 REMOVE=feature");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine ok(pf_exists("msitest\\maximus"), "File deleted\n");
    ok(pf_exists("msitest"), "File deleted\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -12);
    }

    RegCloseKey(prodkey);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -20);
    }

    RegCloseKey(prodkey);

    /* UnpublishFeatures, both features removed */
    r = MsiInstallProductA(msifile, "UNPUBLISH_FEATURES=1 REMOVE=feature,montecristo");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    todo_wine
    {
        ok(!pf_exists("msitest"), "File not deleted\n");
    }

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /I{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", NULL);
    CHECK_REG_STR(prodkey, "Comments", NULL);
    CHECK_REG_STR(prodkey, "Contact", NULL);
    CHECK_REG_STR(prodkey, "HelpLink", NULL);
    CHECK_REG_STR(prodkey, "HelpTelephone", NULL);
    CHECK_REG_STR(prodkey, "InstallLocation", NULL);
    CHECK_REG_STR(prodkey, "Readme", NULL);
    CHECK_REG_STR(prodkey, "Size", NULL);
    CHECK_REG_STR(prodkey, "URLInfoAbout", NULL);
    CHECK_REG_STR(prodkey, "URLUpdateInfo", NULL);
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    todo_wine
    {
        CHECK_REG_DWORD2(prodkey, "EstimatedSize", 12, -12);
    }

    RegCloseKey(prodkey);

    /* complete uninstall */
    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    todo_wine
    {
        ok(!pf_exists("msitest"), "File not deleted\n");
    }

    state = MsiQueryProductState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureState("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}", "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyA(uninstall, prodcode, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", res);

    /* make sure 'Program Files\msitest' is removed */
    delete_pfmsitest_files();

    RegCloseKey(uninstall);
    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static void test_publishsourcelist(void)
{
    UINT r;
    DWORD size;
    CHAR value[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR prodcode[] = "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";

    if (!pMsiSourceListEnumSourcesA || !pMsiSourceListGetInfoA)
    {
        skip("MsiSourceListEnumSourcesA and/or MsiSourceListGetInfoA are not available\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, sizeof(pp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* nothing published */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after RegisterProduct */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after ProcessComponents */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after PublishFeatures */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after PublishProduct */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "msitest.msi"), "Expected 'msitest.msi', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATH, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_DISKPROMPT, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %d\n", size);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, path), "Expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "Expected %d, got %d\n", lstrlenA(path), size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, path), "Expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "Expected %d, got %d\n", lstrlenA(path), size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                   MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 1, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* complete uninstall */
    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    todo_wine
    {
        ok(!pf_exists("msitest"), "File not deleted\n");
    }

    /* make sure 'Program Files\msitest' is removed */
    delete_pfmsitest_files();

    DeleteFile(msifile);
    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
}

static UINT run_query(MSIHANDLE hdb, MSIHANDLE hrec, const char *query)
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if(r != ERROR_SUCCESS)
        return r;

    r = MsiViewExecute(hview, hrec);
    if(r == ERROR_SUCCESS)
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static void set_transform_summary_info(void)
{
    UINT r;
    MSIHANDLE suminfo = 0;

    /* build summary info */
    r = MsiGetSummaryInformation(0, mstfile, 3, &suminfo);
    todo_wine
    {
        ok(r == ERROR_SUCCESS , "Failed to open summaryinfo\n");
    }

    r = MsiSummaryInfoSetProperty(suminfo, PID_TITLE, VT_LPSTR, 0, NULL, "MSITEST");
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Failed to set summary info\n");
    }

    r = MsiSummaryInfoSetProperty(suminfo, PID_REVNUMBER, VT_LPSTR, 0, NULL,
                        "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}1.1.1;"
                        "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}1.1.1;"
                        "{4C0EAA15-0264-4E5A-8758-609EF142B92D}");
    todo_wine
    {
        ok(r == ERROR_SUCCESS , "Failed to set summary info\n");
    }

    r = MsiSummaryInfoSetProperty(suminfo, PID_PAGECOUNT, VT_I4, 100, NULL, NULL);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Failed to set summary info\n");
    }

    r = MsiSummaryInfoPersist(suminfo);
    todo_wine
    {
        ok(r == ERROR_SUCCESS , "Failed to make summary info persist\n");
    }

    r = MsiCloseHandle(suminfo);
    ok(r == ERROR_SUCCESS , "Failed to close suminfo\n");
}

static void generate_transform(void)
{
    MSIHANDLE hdb1, hdb2;
    LPCSTR query;
    UINT r;

    /* start with two identical databases */
    CopyFile(msifile, msifile2, FALSE);

    r = MsiOpenDatabase(msifile2, MSIDBOPEN_TRANSACT, &hdb1);
    ok(r == ERROR_SUCCESS , "Failed to create database\n");

    r = MsiDatabaseCommit(hdb1);
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb2);
    ok(r == ERROR_SUCCESS , "Failed to create database\n");

    query = "INSERT INTO `Property` ( `Property`, `Value` ) VALUES ( 'prop', 'val' )";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add property\n");

    /* database needs to be committed */
    MsiDatabaseCommit(hdb1);

    r = MsiDatabaseGenerateTransform(hdb1, hdb2, mstfile, 0, 0);
    ok(r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r);

#if 0  /* not implemented in wine yet */
    r = MsiCreateTransformSummaryInfo(hdb2, hdb2, mstfile, 0, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
#endif

    MsiCloseHandle(hdb1);
    MsiCloseHandle(hdb2);
}

/* data for generating a transform */

/* tables transform names - encoded as they would be in an msi database file */
static const WCHAR name1[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR name2[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR name3[] = { 0x4840, 0x4559, 0x44f2, 0x4568, 0x4737, 0 }; /* Property */

/* data in each table */
static const char data1[] = /* _StringData */
    "propval";  /* all the strings squashed together */

static const WCHAR data2[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''     */
    4,   1,    /* string 1 'prop' */
    3,   1,    /* string 2 'val'  */
};

static const WCHAR data3[] = { /* Property */
    0x0201, 0x0001, 0x0002,
};

static const struct {
    LPCWSTR name;
    const void *data;
    DWORD size;
} table_transform_data[] =
{
    { name1, data1, sizeof data1 - 1 },
    { name2, data2, sizeof data2 },
    { name3, data3, sizeof data3 },
};

#define NUM_TRANSFORM_TABLES (sizeof table_transform_data/sizeof table_transform_data[0])

static void generate_transform_manual(void)
{
    IStorage *stg = NULL;
    IStream *stm;
    WCHAR name[0x20];
    HRESULT r;
    DWORD i, count;
    const DWORD mode = STGM_CREATE|STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE;

    const CLSID CLSID_MsiTransform = { 0xc1082,0,0,{0xc0,0,0,0,0,0,0,0x46}};

    MultiByteToWideChar(CP_ACP, 0, mstfile, -1, name, 0x20);

    r = StgCreateDocfile(name, mode, 0, &stg);
    ok(r == S_OK, "failed to create storage\n");
    if (!stg)
        return;

    r = IStorage_SetClass(stg, &CLSID_MsiTransform);
    ok(r == S_OK, "failed to set storage type\n");

    for (i=0; i<NUM_TRANSFORM_TABLES; i++)
    {
        r = IStorage_CreateStream(stg, table_transform_data[i].name,
                            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
        if (FAILED(r))
        {
            ok(0, "failed to create stream %08x\n", r);
            continue;
        }

        r = IStream_Write(stm, table_transform_data[i].data,
                          table_transform_data[i].size, &count);
        if (FAILED(r) || count != table_transform_data[i].size)
            ok(0, "failed to write stream\n");
        IStream_Release(stm);
    }

    IStorage_Release(stg);

    set_transform_summary_info();
}

static void test_transformprop(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, tp_tables, sizeof(tp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(!delete_pf("msitest", FALSE), "File installed\n");

    if (0)
        generate_transform();
    else
        generate_transform_manual();

    r = MsiInstallProductA(msifile, "TRANSFORMS=winetest.mst");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    /* Delete the files in the temp (current) folder */
    DeleteFile(msifile);
    DeleteFile(msifile2);
    DeleteFile(mstfile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
}

static void test_currentworkingdir(void)
{
    UINT r;
    CHAR path[MAX_PATH];
    LPSTR ptr, ptr2;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, cwd_tables, sizeof(cwd_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    CreateDirectoryA("diffdir", NULL);
    SetCurrentDirectoryA("diffdir");

    sprintf(path, "..\\%s", msifile);
    r = MsiInstallProductA(path, NULL);
    todo_wine
    {
        ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %u\n", r);
        ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
        ok(!delete_pf("msitest", FALSE), "File installed\n");
    }

    sprintf(path, "%s\\%s", CURR_DIR, msifile);
    r = MsiInstallProductA(path, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    lstrcpyA(path, CURR_DIR);
    if (path[lstrlenA(path) - 1] != '\\')
        lstrcatA(path, "\\");
    lstrcatA(path, "msitest.msi");

    ptr2 = strrchr(path, '\\');
    *ptr2 = '\0';
    ptr = strrchr(path, '\\');
    *ptr2 = '\\';
    *(ptr++) = '\0';

    SetCurrentDirectoryA(path);

    r = MsiInstallProductA(ptr, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    SetCurrentDirectoryA(CURR_DIR);

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
    RemoveDirectory("diffdir");
}

static void set_admin_summary_info(const CHAR *name)
{
    MSIHANDLE db, summary;
    UINT r;

    r = MsiOpenDatabaseA(name, MSIDBOPEN_DIRECT, &db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiGetSummaryInformationA(db, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, PID_WORDCOUNT, VT_I4, 5, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* write the summary changes back to the stream */
    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);

    r = MsiDatabaseCommit(db);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(db);
}

static void test_admin(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, adm_tables, sizeof(adm_tables) / sizeof(msi_table));
    set_admin_summary_info(msifile);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(!delete_pf("msitest", FALSE), "File installed\n");
    ok(!DeleteFile("c:\\msitest\\augustus"), "File installed\n");
    ok(!RemoveDirectory("c:\\msitest"), "File installed\n");

    r = MsiInstallProductA(msifile, "ACTION=ADMIN");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(!delete_pf("msitest", FALSE), "File installed\n");
    todo_wine
    {
        ok(DeleteFile("c:\\msitest\\augustus"), "File not installed\n");
        ok(RemoveDirectory("c:\\msitest"), "File not installed\n");
    }

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
}

static void set_admin_property_stream(LPCSTR file)
{
    IStorage *stg;
    IStream *stm;
    WCHAR fileW[MAX_PATH];
    HRESULT hr;
    DWORD count;
    const DWORD mode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

    /* AdminProperties */
    static const WCHAR stmname[] = {0x41ca,0x4330,0x3e71,0x44b5,0x4233,0x45f5,0x422c,0x4836,0};
    static const WCHAR data[] = {'M','Y','P','R','O','P','=','2','7','1','8',' ',
        'M','y','P','r','o','p','=','4','2',0};

    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH);

    hr = StgOpenStorage(fileW, NULL, mode, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    if (!stg)
        return;

    hr = IStorage_CreateStream(stg, stmname, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStream_Write(stm, data, sizeof(data), &count);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    IStream_Release(stm);
    IStorage_Release(stg);
}

static void test_adminprops(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, amp_tables, sizeof(amp_tables) / sizeof(msi_table));
    set_admin_summary_info(msifile);
    set_admin_property_stream(msifile);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File installed\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
}

static void create_pf_data(LPCSTR file, LPCSTR data, BOOL is_file)
{
    CHAR path[MAX_PATH];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    if (is_file)
        create_file_data(path, data, 500);
    else
        CreateDirectoryA(path, NULL);
}

#define create_pf(file, is_file) create_pf_data(file, file, is_file)

static void test_removefiles(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\hydrogen", 500);
    create_file("msitest\\helium", 500);
    create_file("msitest\\lithium", 500);

    create_database(msifile, rem_tables, sizeof(rem_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(!pf_exists("msitest\\helium"), "File installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!pf_exists("msitest\\hydrogen"), "File not deleted\n");
    ok(!pf_exists("msitest\\helium"), "File not deleted\n");
    ok(delete_pf("msitest\\lithium", TRUE), "File deleted\n");
    ok(delete_pf("msitest", FALSE), "File deleted\n");

    create_pf("msitest", FALSE);
    create_pf("msitest\\hydrogen", TRUE);
    create_pf("msitest\\helium", TRUE);
    create_pf("msitest\\lithium", TRUE);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!pf_exists("msitest\\hydrogen"), "File not deleted\n");
    ok(delete_pf("msitest\\helium", TRUE), "File deleted\n");
    ok(delete_pf("msitest\\lithium", TRUE), "File deleted\n");
    ok(delete_pf("msitest", FALSE), "File deleted\n");

    create_pf("msitest", FALSE);
    create_pf("msitest\\furlong", TRUE);
    create_pf("msitest\\firkin", TRUE);
    create_pf("msitest\\fortnight", TRUE);
    create_pf("msitest\\becquerel", TRUE);
    create_pf("msitest\\dioptre", TRUE);
    create_pf("msitest\\attoparsec", TRUE);
    create_pf("msitest\\storeys", TRUE);
    create_pf("msitest\\block", TRUE);
    create_pf("msitest\\siriometer", TRUE);
    create_pf("msitest\\cabout", FALSE);
    create_pf("msitest\\cabout\\blocker", TRUE);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(!pf_exists("msitest\\helium"), "File installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(!pf_exists("msitest\\furlong"), "File not deleted\n");
    ok(!pf_exists("msitest\\firkin"), "File not deleted\n");
    ok(!pf_exists("msitest\\fortnight"), "File not deleted\n");
    ok(pf_exists("msitest\\becquerel"), "File not installed\n");
    ok(pf_exists("msitest\\dioptre"), "File not installed\n");
    ok(pf_exists("msitest\\attoparsec"), "File not installed\n");
    ok(!pf_exists("msitest\\storeys"), "File not deleted\n");
    ok(!pf_exists("msitest\\block"), "File not deleted\n");
    ok(!pf_exists("msitest\\siriometer"), "File not deleted\n");
    ok(pf_exists("msitest\\cabout"), "Directory removed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    create_pf("msitest\\furlong", TRUE);
    create_pf("msitest\\firkin", TRUE);
    create_pf("msitest\\fortnight", TRUE);
    create_pf("msitest\\storeys", TRUE);
    create_pf("msitest\\block", TRUE);
    create_pf("msitest\\siriometer", TRUE);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not deleted\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not deleted\n");
    ok(delete_pf("msitest\\lithium", TRUE), "File deleted\n");
    ok(delete_pf("msitest\\furlong", TRUE), "File deleted\n");
    ok(delete_pf("msitest\\firkin", TRUE), "File deleted\n");
    ok(delete_pf("msitest\\fortnight", TRUE), "File deleted\n");
    ok(!delete_pf("msitest\\becquerel", TRUE), "File not deleted\n");
    ok(!delete_pf("msitest\\dioptre", TRUE), "File not deleted\n");
    ok(delete_pf("msitest\\attoparsec", TRUE), "File deleted\n");
    ok(!delete_pf("msitest\\storeys", TRUE), "File not deleted\n");
    ok(!delete_pf("msitest\\block", TRUE), "File not deleted\n");
    ok(delete_pf("msitest\\siriometer", TRUE), "File deleted\n");
    ok(pf_exists("msitest\\cabout"), "Directory deleted\n");
    ok(pf_exists("msitest"), "Directory deleted\n");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\hydrogen", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File installed\n");
    ok(delete_pf("msitest\\lithium", TRUE), "File not installed\n");
    ok(pf_exists("msitest\\cabout"), "Directory deleted\n");
    ok(pf_exists("msitest"), "Directory deleted\n");

    delete_pf("msitest\\cabout\\blocker", TRUE);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\cabout", FALSE), "Directory not deleted\n");
    ok(delete_pf("msitest", FALSE), "Directory deleted\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\hydrogen");
    DeleteFile("msitest\\helium");
    DeleteFile("msitest\\lithium");
    RemoveDirectory("msitest");
}

static void test_movefiles(void)
{
    UINT r;
    char props[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 100);
    create_file("cameroon", 100);
    create_file("djibouti", 100);
    create_file("egypt", 100);
    create_file("finland", 100);
    create_file("gambai", 100);
    create_file("honduras", 100);
    create_file("msitest\\india", 100);
    create_file("japan", 100);
    create_file("kenya", 100);
    CreateDirectoryA("latvia", NULL);
    create_file("nauru", 100);
    create_file("peru", 100);
    create_file("apple", 100);
    create_file("application", 100);
    create_file("ape", 100);
    create_file("foo", 100);
    create_file("fao", 100);
    create_file("fbod", 100);
    create_file("budding", 100);
    create_file("buddy", 100);
    create_file("bud", 100);
    create_file("bar", 100);
    create_file("bur", 100);
    create_file("bird", 100);

    create_database(msifile, mov_tables, sizeof(mov_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* if the source or dest property is not a full path,
     * windows tries to access it as a network resource
     */

    sprintf(props, "SOURCEFULL=\"%s\\\" DESTFULL=\"%s\\msitest\" "
            "FILEPATHBAD=\"%s\\japan\" FILEPATHGOOD=\"%s\\kenya\"",
            CURR_DIR, PROG_FILES_DIR, CURR_DIR, CURR_DIR);

    r = MsiInstallProductA(msifile, props);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\dest", TRUE), "File copied\n");
    ok(delete_pf("msitest\\canada", TRUE), "File not copied\n");
    ok(delete_pf("msitest\\dominica", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\elsalvador", TRUE), "File moved\n");
    ok(!delete_pf("msitest\\france", TRUE), "File moved\n");
    ok(!delete_pf("msitest\\georgia", TRUE), "File moved\n");
    ok(delete_pf("msitest\\hungary", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\indonesia", TRUE), "File moved\n");
    ok(!delete_pf("msitest\\jordan", TRUE), "File moved\n");
    ok(delete_pf("msitest\\kiribati", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\lebanon", TRUE), "File moved\n");
    ok(!delete_pf("msitest\\lebanon", FALSE), "Directory moved\n");
    ok(delete_pf("msitest\\poland", TRUE), "File not moved\n");
    /* either apple or application will be moved depending on directory order */
    if (!delete_pf("msitest\\apple", TRUE))
        ok(delete_pf("msitest\\application", TRUE), "File not moved\n");
    else
        ok(!delete_pf("msitest\\application", TRUE), "File should not exist\n");
    ok(delete_pf("msitest\\wildcard", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\ape", TRUE), "File moved\n");
    /* either fao or foo will be moved depending on directory order */
    if (delete_pf("msitest\\foo", TRUE))
        ok(!delete_pf("msitest\\fao", TRUE), "File should not exist\n");
    else
        ok(delete_pf("msitest\\fao", TRUE), "File not moved\n");
    ok(delete_pf("msitest\\single", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\fbod", TRUE), "File moved\n");
    ok(delete_pf("msitest\\budding", TRUE), "File not moved\n");
    ok(delete_pf("msitest\\buddy", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\bud", TRUE), "File moved\n");
    ok(delete_pf("msitest\\bar", TRUE), "File not moved\n");
    ok(delete_pf("msitest\\bur", TRUE), "File not moved\n");
    ok(!delete_pf("msitest\\bird", TRUE), "File moved\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");
    ok(DeleteFileA("cameroon"), "File moved\n");
    ok(!DeleteFileA("djibouti"), "File not moved\n");
    ok(DeleteFileA("egypt"), "File moved\n");
    ok(DeleteFileA("finland"), "File moved\n");
    ok(DeleteFileA("gambai"), "File moved\n");
    ok(!DeleteFileA("honduras"), "File not moved\n");
    ok(DeleteFileA("msitest\\india"), "File moved\n");
    ok(DeleteFileA("japan"), "File moved\n");
    ok(!DeleteFileA("kenya"), "File not moved\n");
    ok(RemoveDirectoryA("latvia"), "Directory moved\n");
    ok(!DeleteFileA("nauru"), "File not moved\n");
    ok(!DeleteFileA("peru"), "File not moved\n");
    ok(!DeleteFileA("apple"), "File not moved\n");
    ok(!DeleteFileA("application"), "File not moved\n");
    ok(DeleteFileA("ape"), "File moved\n");
    ok(!DeleteFileA("foo"), "File not moved\n");
    ok(!DeleteFileA("fao"), "File not moved\n");
    ok(DeleteFileA("fbod"), "File moved\n");
    ok(!DeleteFileA("budding"), "File not moved\n");
    ok(!DeleteFileA("buddy"), "File not moved\n");
    ok(DeleteFileA("bud"), "File moved\n");
    ok(!DeleteFileA("bar"), "File not moved\n");
    ok(!DeleteFileA("bur"), "File not moved\n");
    ok(DeleteFileA("bird"), "File moved\n");

    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
    DeleteFile(msifile);
}

static void test_missingcab(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);
    create_file("maximus", 500);

    create_database(msifile, mc_tables, sizeof(mc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    create_cab_file("test1.cab", MEDIA_SIZE, "maximus\0");

    create_pf("msitest", FALSE);
    create_pf_data("msitest\\caesar", "abcdefgh", TRUE);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\caesar", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\gaius", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    create_pf("msitest", FALSE);
    create_pf_data("msitest\\caesar", "abcdefgh", TRUE);
    create_pf("msitest\\gaius", TRUE);

    r = MsiInstallProductA(msifile, "GAIUS=1");
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    todo_wine
    {
        ok(!delete_pf("msitest\\maximus", TRUE), "File installed\n");
        ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    }
    ok(delete_pf("msitest\\caesar", TRUE), "File removed\n");
    ok(delete_pf("msitest\\gaius", TRUE), "File removed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
    DeleteFile("maximus");
    DeleteFile("test1.cab");
    DeleteFile(msifile);
}

static void test_duplicatefiles(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, df_tables, sizeof(df_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* fails if the destination folder is not a valid property */

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\augustus", TRUE), "File not duplicated\n");
    ok(delete_pf("msitest\\this\\doesnot\\exist\\maximus", TRUE), "File not duplicated\n");
    ok(delete_pf("msitest\\this\\doesnot\\exist", FALSE), "File not duplicated\n");
    ok(delete_pf("msitest\\this\\doesnot", FALSE), "File not duplicated\n");
    ok(delete_pf("msitest\\this", FALSE), "File not duplicated\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile("msitest\\maximus");
    RemoveDirectory("msitest");
    DeleteFile(msifile);
}

static void test_writeregistryvalues(void)
{
    UINT r;
    LONG res;
    HKEY hkey;
    DWORD type, size;
    CHAR path[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, wrv_tables, sizeof(wrv_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File installed\n");

    res = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    type = REG_MULTI_SZ;
    memset(path, 'a', MAX_PATH);
    res = RegQueryValueExA(hkey, "Value", NULL, &type, (LPBYTE)path, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!memcmp(path, "one\0two\0three\0\0", size), "Wrong multi-sz data\n");
    ok(size == 15, "Expected 15, got %d\n", size);
    ok(type == REG_MULTI_SZ, "Expected REG_MULTI_SZ, got %d\n", type);

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");

    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine\\msitest");
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine");
}

static void test_sourcefolder(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("augustus", 500);

    create_database(msifile, sf_tables, sizeof(sf_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE,
       "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File installed\n");
    }

    RemoveDirectoryA("msitest");

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_INSTALL_FAILURE,
       "Expected ERROR_INSTALL_FAILURE, got %u\n", r);
    ok(!delete_pf("msitest\\augustus", TRUE), "File installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File installed\n");
    }

    DeleteFile(msifile);
    DeleteFile("augustus");
}

static void test_customaction51(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, ca51_tables, sizeof(ca51_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File installed\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
}

static void test_installstate(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\alpha", 500);
    create_file("msitest\\beta", 500);
    create_file("msitest\\gamma", 500);
    create_file("msitest\\theta", 500);
    create_file("msitest\\delta", 500);
    create_file("msitest\\epsilon", 500);
    create_file("msitest\\zeta", 500);
    create_file("msitest\\iota", 500);
    create_file("msitest\\eta", 500);
    create_file("msitest\\kappa", 500);
    create_file("msitest\\lambda", 500);
    create_file("msitest\\mu", 500);

    create_database(msifile, is_tables, sizeof(is_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\alpha", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\beta", TRUE), "File installed\n");
    ok(delete_pf("msitest\\gamma", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\theta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\delta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\epsilon", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\zeta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\iota", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\eta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\kappa", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\lambda", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\mu", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    r = MsiInstallProductA(msifile, "ADDLOCAL=\"one,two,three,four\"");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\alpha", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\beta", TRUE), "File installed\n");
    ok(delete_pf("msitest\\gamma", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\theta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\delta", TRUE), "File installed\n");
    ok(delete_pf("msitest\\epsilon", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\zeta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\iota", TRUE), "File installed\n");
    ok(delete_pf("msitest\\eta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\kappa", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\lambda", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\mu", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    r = MsiInstallProductA(msifile, "ADDSOURCE=\"one,two,three,four\"");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\alpha", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\beta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\gamma", TRUE), "File installed\n");
    ok(delete_pf("msitest\\theta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\delta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\epsilon", TRUE), "File installed\n");
    ok(delete_pf("msitest\\zeta", TRUE), "File not installed\n");
    ok(!delete_pf("msitest\\iota", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\eta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\kappa", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\lambda", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\mu", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    r = MsiInstallProductA(msifile, "REMOVE=\"one,two,three,four\"");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\alpha", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\beta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\gamma", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\theta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\delta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\epsilon", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\zeta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\iota", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\eta", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\kappa", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\lambda", TRUE), "File installed\n");
    ok(!delete_pf("msitest\\mu", TRUE), "File installed\n");
    ok(!delete_pf("msitest", FALSE), "File installed\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\alpha");
    DeleteFile("msitest\\beta");
    DeleteFile("msitest\\gamma");
    DeleteFile("msitest\\theta");
    DeleteFile("msitest\\delta");
    DeleteFile("msitest\\epsilon");
    DeleteFile("msitest\\zeta");
    DeleteFile("msitest\\iota");
    DeleteFile("msitest\\eta");
    DeleteFile("msitest\\kappa");
    DeleteFile("msitest\\lambda");
    DeleteFile("msitest\\mu");
    RemoveDirectory("msitest");
}

struct sourcepathmap
{
    BOOL sost; /* shortone\shorttwo */
    BOOL solt; /* shortone\longtwo */
    BOOL lost; /* longone\shorttwo */
    BOOL lolt; /* longone\longtwo */
    BOOL soste; /* shortone\shorttwo source exists */
    BOOL solte; /* shortone\longtwo source exists */
    BOOL loste; /* longone\shorttwo source exists */
    BOOL lolte; /* longone\longtwo source exists */
    UINT err;
    DWORD size;
} spmap[256] =
{
    {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, ERROR_SUCCESS, 200},
    {FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, ERROR_INSTALL_FAILURE, 0},
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, ERROR_INSTALL_FAILURE, 0},
};

static DWORD get_pf_file_size(LPCSTR file)
{
    CHAR path[MAX_PATH];
    HANDLE hfile;
    DWORD size;

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, file);

    hfile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hfile == INVALID_HANDLE_VALUE)
        return 0;

    size = GetFileSize(hfile, NULL);
    CloseHandle(hfile);
    return size;
}

static void test_sourcepath(void)
{
    UINT r, i;

    if (!winetest_interactive)
    {
        skip("Run in interactive mode to run source path tests.\n");
        return;
    }

    create_database(msifile, sp_tables, sizeof(sp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    for (i = 0; i < sizeof(spmap) / sizeof(spmap[0]); i++)
    {
        if (spmap[i].sost)
        {
            CreateDirectoryA("shortone", NULL);
            CreateDirectoryA("shortone\\shorttwo", NULL);
        }

        if (spmap[i].solt)
        {
            CreateDirectoryA("shortone", NULL);
            CreateDirectoryA("shortone\\longtwo", NULL);
        }

        if (spmap[i].lost)
        {
            CreateDirectoryA("longone", NULL);
            CreateDirectoryA("longone\\shorttwo", NULL);
        }

        if (spmap[i].lolt)
        {
            CreateDirectoryA("longone", NULL);
            CreateDirectoryA("longone\\longtwo", NULL);
        }

        if (spmap[i].soste)
            create_file("shortone\\shorttwo\\augustus", 50);
        if (spmap[i].solte)
            create_file("shortone\\longtwo\\augustus", 100);
        if (spmap[i].loste)
            create_file("longone\\shorttwo\\augustus", 150);
        if (spmap[i].lolte)
            create_file("longone\\longtwo\\augustus", 200);

        r = MsiInstallProductA(msifile, NULL);
        ok(r == spmap[i].err, "%d: Expected %d, got %d\n", i, spmap[i].err, r);
        ok(get_pf_file_size("msitest\\augustus") == spmap[i].size,
           "%d: Expected %d, got %d\n", i, spmap[i].size,
           get_pf_file_size("msitest\\augustus"));

        if (r == ERROR_SUCCESS)
        {
            ok(delete_pf("msitest\\augustus", TRUE), "%d: File not installed\n", i);
            ok(delete_pf("msitest", FALSE), "%d: File not installed\n", i);
        }
        else
        {
            ok(!delete_pf("msitest\\augustus", TRUE), "%d: File installed\n", i);
            todo_wine ok(!delete_pf("msitest", FALSE), "%d: File installed\n", i);
        }

        DeleteFileA("shortone\\shorttwo\\augustus");
        DeleteFileA("shortone\\longtwo\\augustus");
        DeleteFileA("longone\\shorttwo\\augustus");
        DeleteFileA("longone\\longtwo\\augustus");
        RemoveDirectoryA("shortone\\shorttwo");
        RemoveDirectoryA("shortone\\longtwo");
        RemoveDirectoryA("longone\\shorttwo");
        RemoveDirectoryA("longone\\longtwo");
        RemoveDirectoryA("shortone");
        RemoveDirectoryA("longone");
    }

    DeleteFileA(msifile);
}

static void test_MsiConfigureProductEx(void)
{
    UINT r;
    LONG res;
    DWORD type, size;
    HKEY props, source;
    CHAR keypath[MAX_PATH * 2];
    CHAR localpack[MAX_PATH];

    if (on_win9x)
    {
        win_skip("Different registry keys on Win9x and WinMe\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\hydrogen", 500);
    create_file("msitest\\helium", 500);
    create_file("msitest\\lithium", 500);

    create_database(msifile, mcp_tables, sizeof(mcp_tables) / sizeof(msi_table));

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
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* product is installed per-user managed, remove it */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    /* product has been removed */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
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
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    /* product has been removed */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
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

    /* local msifile is removed */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    create_database(msifile, mcp_tables, sizeof(mcp_tables) / sizeof(msi_table));

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
    lstrcatA(keypath, "84A88FD7F6998CE40A22FB59F6B9C2BB\\InstallProperties");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"C:\\idontexist.msi", 18);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage is used to find the cached msi package */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_INSTALL_SOURCE_ABSENT,
       "Expected ERROR_INSTALL_SOURCE_ABSENT, got %d\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    RegCloseKey(props);
    create_database(msifile, mcp_tables, sizeof(mcp_tables) / sizeof(msi_table));

    /* LastUsedSource (local msi package) can be used as a last resort */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    /* install the product, machine */
    r = MsiInstallProductA(msifile, "ALLUSERS=1 INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    lstrcpyA(keypath, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, "84A88FD7F6998CE40A22FB59F6B9C2BB\\InstallProperties");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(props, "LocalPackage", 0, REG_SZ,
                         (const BYTE *)"C:\\idontexist.msi", 18);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(keypath, "SOFTWARE\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, "84A88FD7F6998CE40A22FB59F6B9C2BB\\SourceList");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    type = REG_SZ;
    size = MAX_PATH;
    res = RegQueryValueExA(source, "PackageName", NULL, &type,
                           (LPBYTE)localpack, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(source, "PackageName", 0, REG_SZ,
                         (const BYTE *)"idontexist.msi", 15);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList is altered */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_INSTALL_SOURCE_ABSENT,
       "Expected ERROR_INSTALL_SOURCE_ABSENT, got %d\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* restore the SourceList */
    res = RegSetValueExA(source, "PackageName", 0, REG_SZ,
                         (const BYTE *)localpack, lstrlenA(localpack) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* finally remove the product */
    r = MsiConfigureProductExA("{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}",
                               INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT,
                               "PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    DeleteFileA(msifile);
    RegCloseKey(source);
    RegCloseKey(props);
    DeleteFileA("msitest\\hydrogen");
    DeleteFileA("msitest\\helium");
    DeleteFileA("msitest\\lithium");
    RemoveDirectoryA("msitest");
}

static void test_missingcomponent(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\hydrogen", 500);
    create_file("msitest\\helium", 500);
    create_file("msitest\\lithium", 500);
    create_file("beryllium", 500);

    create_database(msifile, mcomp_tables, sizeof(mcomp_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, "INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\hydrogen"), "File not installed\n");
    ok(pf_exists("msitest\\helium"), "File not installed\n");
    ok(pf_exists("msitest\\lithium"), "File not installed\n");
    ok(!pf_exists("msitest\\beryllium"), "File installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL INSTALLLEVEL=10 PROPVAR=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(!delete_pf("msitest\\hydrogen", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\helium", TRUE), "File not removed\n");
    ok(!delete_pf("msitest\\lithium", TRUE), "File not removed\n");
    ok(!pf_exists("msitest\\beryllium"), "File installed\n");
    todo_wine
    {
        ok(!delete_pf("msitest", FALSE), "File not removed\n");
    }

    DeleteFileA(msifile);
    DeleteFileA("msitest\\hydrogen");
    DeleteFileA("msitest\\helium");
    DeleteFileA("msitest\\lithium");
    DeleteFileA("beryllium");
    RemoveDirectoryA("msitest");
}

static void test_sourcedirprop(void)
{
    UINT r;
    CHAR props[MAX_PATH];

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, ca51_tables, sizeof(ca51_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File installed\n");

    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");

    CreateDirectoryA("altsource", NULL);
    CreateDirectoryA("altsource\\msitest", NULL);
    create_file("altsource\\msitest\\augustus", 500);

    sprintf(props, "SRCDIR=%s\\altsource\\", CURR_DIR);

    r = MsiInstallProductA(msifile, props);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File installed\n");
    ok(delete_pf("msitest", FALSE), "File installed\n");

    DeleteFile(msifile);
    DeleteFile("altsource\\msitest\\augustus");
    RemoveDirectory("altsource\\msitest");
    RemoveDirectory("altsource");
}

static void test_adminimage(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    CreateDirectoryA("msitest\\first", NULL);
    CreateDirectoryA("msitest\\second", NULL);
    CreateDirectoryA("msitest\\cabout", NULL);
    CreateDirectoryA("msitest\\cabout\\new", NULL);
    create_file("msitest\\one.txt", 100);
    create_file("msitest\\first\\two.txt", 100);
    create_file("msitest\\second\\three.txt", 100);
    create_file("msitest\\cabout\\four.txt", 100);
    create_file("msitest\\cabout\\new\\five.txt", 100);
    create_file("msitest\\filename", 100);
    create_file("msitest\\service.exe", 100);

    create_database_wordcount(msifile, ai_tables,
                              sizeof(ai_tables) / sizeof(msi_table),
                              msidbSumInfoSourceTypeAdminImage);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

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
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFileA("msitest.msi");
    DeleteFileA("msitest\\cabout\\new\\five.txt");
    DeleteFileA("msitest\\cabout\\four.txt");
    DeleteFileA("msitest\\second\\three.txt");
    DeleteFileA("msitest\\first\\two.txt");
    DeleteFileA("msitest\\one.txt");
    DeleteFileA("msitest\\service.exe");
    DeleteFileA("msitest\\filename");
    RemoveDirectoryA("msitest\\cabout\\new");
    RemoveDirectoryA("msitest\\cabout");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

static void test_propcase(void)
{
    UINT r;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\augustus", 500);

    create_database(msifile, pc_tables, sizeof(pc_tables) / sizeof(msi_table));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, "MyProp=42");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\augustus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "File not installed\n");

    DeleteFile(msifile);
    DeleteFile("msitest\\augustus");
    RemoveDirectory("msitest");
}

START_TEST(install)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH];
    STATEMGRSTATUS status;
    BOOL ret = FALSE;

    init_functionpointers();

    on_win9x = check_win9x();

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPath(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    get_program_files_dir(PROG_FILES_DIR, COMMON_FILES_DIR);

    /* Create a restore point ourselves so we circumvent the multitude of restore points
     * that would have been created by all the installation and removal tests.
     */
    if (pSRSetRestorePointA)
    {
        memset(&status, 0, sizeof(status));
        ret = notify_system_change(BEGIN_NESTED_SYSTEM_CHANGE, &status);
    }

    /* Create only one log file and don't append. We have to pass something
     * for the log mode for this to work.
     */
    MsiEnableLogA(INSTALLLOGMODE_FATALEXIT, "msitest.log", 0);

    test_MsiInstallProduct();
    test_MsiSetComponentState();
    test_packagecoltypes();
    test_continuouscabs();
    test_caborder();
    test_mixedmedia();
    test_samesequence();
    test_uiLevelFlags();
    test_readonlyfile();
    test_setdirproperty();
    test_cabisextracted();
    test_concurrentinstall();
    test_setpropertyfolder();
    test_publish_registerproduct();
    test_publish_publishproduct();
    test_publish_publishfeatures();
    test_publish_registeruser();
    test_publish_processcomponents();
    test_publish();
    test_publishsourcelist();
    test_transformprop();
    test_currentworkingdir();
    test_admin();
    test_adminprops();
    test_removefiles();
    test_movefiles();
    test_missingcab();
    test_duplicatefiles();
    test_writeregistryvalues();
    test_sourcefolder();
    test_customaction51();
    test_installstate();
    test_sourcepath();
    test_MsiConfigureProductEx();
    test_missingcomponent();
    test_sourcedirprop();
    test_adminimage();
    test_propcase();

    DeleteFileA("msitest.log");

    if (pSRSetRestorePointA && ret)
    {
        ret = notify_system_change(END_NESTED_SYSTEM_CHANGE, &status);
        if (ret)
            remove_restore_point(status.llSequenceNumber);
    }
    FreeLibrary(hsrclient);

    SetCurrentDirectoryA(prev_path);
}
