/*
 * Copyright (C) 2006 James Hawkins
 * Copyright 2010 Hans Leidekker for CodeWeavers
 *
 * Tests concentrating on standard actions
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
#include <stdio.h>
#include <stdlib.h>

#define COBJMACROS
#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <fci.h>
#include <wtypes.h>
#include <shellapi.h>
#include <winsvc.h>
#include <odbcinst.h>
#ifdef __REACTOS__
#include <oleauto.h>
#endif
#include <sddl.h>

#include "wine/test.h"
#include "utils.h"
#include "typelib.h"

static BOOL is_wow64;
static const BOOL is_64bit = sizeof(void *) > sizeof(int);

static const char *msifile = "msitest.msi";

/* msi database data */

static const char component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "Five\t{8CC92E9D-14B2-4CA4-B2AA-B11D02078087}\tNEWDIR\t2\t\tfive.txt\n"
    "Four\t{FD37B4EA-7209-45C0-8917-535F35A2F080}\tCABOUTDIR\t2\t\tfour.txt\n"
    "One\t{783B242E-E185-4A56-AF86-C09815EC053C}\tMSITESTDIR\t2\tNOT REINSTALL\tone.txt\n"
    "Three\t{010B6ADD-B27D-4EDD-9B3D-34C4F7D61684}\tCHANGEDDIR\t2\t\tthree.txt\n"
    "Two\t{BF03D1A6-20DA-4A65-82F3-6CAC995915CE}\tFIRSTDIR\t2\t\ttwo.txt\n"
    "dangler\t{6091DF25-EF96-45F1-B8E9-A9B1420C7A3C}\tTARGETDIR\t4\t\tregdata\n"
    "component\t\tMSITESTDIR\t0\t1\tfile\n"
    "service_comp\t{935A0A91-22A3-4F87-BCA8-928FFDFE2353}\tMSITESTDIR\t0\t\tservice_file\n"
    "service_comp2\t{3F7B04A4-9521-4649-BDC9-0C8722740A49}\tMSITESTDIR\t0\t\tservice_file2\n"
    "service_comp3\t{DBCD1502-20E3-423F-B53E-F37E263CDC7E}\tMSITESTDIR\t0\t\t\n";

static const char directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "CABOUTDIR\tMSITESTDIR\tcabout\n"
    "CHANGEDDIR\tMSITESTDIR\tchanged:second\n"
    "FIRSTDIR\tMSITESTDIR\tfirst\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "NEWDIR\tCABOUTDIR\tnew\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "Five\t\tFive\tThe Five Feature\t5\t3\tNEWDIR\t0\n"
    "Four\t\tFour\tThe Four Feature\t4\t3\tCABOUTDIR\t0\n"
    "One\t\tOne\tThe One Feature\t1\t3\tMSITESTDIR\t0\n"
    "Three\t\tThree\tThe Three Feature\t3\t3\tCHANGEDDIR\t0\n"
    "Two\t\tTwo\tThe Two Feature\t2\t3\tFIRSTDIR\t0\n"
    "feature\t\t\t\t2\t1\tTARGETDIR\t0\n"
    "service_feature\t\t\t\t2\t1\tTARGETDIR\t0";

static const char feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "Five\tFive\n"
    "Four\tFour\n"
    "One\tOne\n"
    "Three\tThree\n"
    "Two\tTwo\n"
    "feature\tcomponent\n"
    "service_feature\tservice_comp\n"
    "service_feature\tservice_comp2\n"
    "service_feature\tservice_comp3";

static const char file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "five.txt\tFive\tfive.txt\t1000\t\t\t16384\t5\n"
    "four.txt\tFour\tfour.txt\t1000\t\t\t16384\t4\n"
    "one.txt\tOne\tone.txt\t1000\t\t\t0\t1\n"
    "three.txt\tThree\tthree.txt\t1000\t\t\t0\t3\n"
    "two.txt\tTwo\ttwo.txt\t1000\t\t\t0\t2\n"
    "file\tcomponent\tfilename\t100\t\t\t8192\t1\n"
    "service_file\tservice_comp\tservice.exe\t100\t\t\t8192\t6\n"
    "service_file2\tservice_comp2\tservice2.exe\t100\t\t\t8192\t7";

static const char install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
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
    "CreateShortcuts\t\t4600\n"
    "InstallServices\t\t5000\n"
    "InstallFinalize\t\t6600\n"
    "InstallInitialize\t\t1500\n"
    "InstallValidate\t\t1400\n"
    "LaunchConditions\t\t100\n"
    "WriteRegistryValues\tSourceDir And SOURCEDIR\t5000";

static const char media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t3\t\t\tDISK1\t\n"
    "2\t7\t\tmsitest.cab\tDISK2\t\n";

static const char property_dat[] =
    "Property\tValue\n"
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
    "SERVNAME2\tTestService2\n"
    "SERVDISP\tTestServiceDisp\n"
    "SERVDISP2\tTestServiceDisp2\n"
    "MSIFASTINSTALL\t1\n"
    "ARPNOMODIFY\t1\n"
    "regdata17\t#1\n";

static const char env_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t100\n"
    "FileCost\t\t200\n"
    "CostFinalize\t\t300\n"
    "InstallValidate\t\t400\n"
    "InstallInitialize\t\t500\n"
    "ProcessComponents\t\t600\n"
    "WriteEnvironmentStrings\t\t700\n"
    "wes_immediate\tNOT REMOVE\t701\n"
    "wes_deferred\tNOT REMOVE\t702\n"
    "RemoveEnvironmentStrings\t\t800\n"
    "res_immediate\tREMOVE\t801\n"
    "res_deferred\tREMOVE\t802\n"
    "RegisterProduct\t\t900\n"
    "PublishFeatures\t\t1000\n"
    "PublishProduct\t\t1100\n"
    "UnpublishFeatures\t\t1150\n"
    "InstallFinalize\t\t1200\n";

static const char env_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "wes_immediate\t1\tcustom.dll\tenv_absent\n"
    "wes_deferred\t1025\tcustom.dll\tenv_present\n"
    "res_immediate\t1\tcustom.dll\tenv_present\n"
    "res_deferred\t1025\tcustom.dll\tenv_absent\n";

static const char env_environment_dat[] =
    "Environment\tName\tValue\tComponent_\n"
    "s72\tl255\tL255\ts72\n"
    "Environment\tEnvironment\n"
    "Var1\t=-MSITESTVAR1\t1\tOne\n"
    "Var2\tMSITESTVAR2\t1\tOne\n"
    "Var3\t=-MSITESTVAR3\t1\tOne\n"
    "Var4\tMSITESTVAR4\t1\tOne\n"
    "Var5\t-MSITESTVAR5\t\tOne\n"
    "Var6\tMSITESTVAR6\t\tOne\n"
    "Var7\t!-MSITESTVAR7\t\tOne\n"
    "Var8\t!-*MSITESTVAR8\t\tOne\n"
    "Var9\t=-MSITESTVAR9\t\tOne\n"
    "Var10\t=MSITESTVAR10\t\tOne\n"
    "Var11\t+-MSITESTVAR11\t[~];1\tOne\n"
    "Var12\t+-MSITESTVAR11\t[~];2\tOne\n"
    "Var13\t+-MSITESTVAR12\t[~];1\tOne\n"
    "Var14\t=MSITESTVAR13\t[~];1\tOne\n"
    "Var15\t=MSITESTVAR13\t[~];2\tOne\n"
    "Var16\t=MSITESTVAR14\t;1;\tOne\n"
    "Var17\t=MSITESTVAR15\t;;1;;\tOne\n"
    "Var18\t=MSITESTVAR16\t 1 \tOne\n"
    "Var19\t+-MSITESTVAR17\t1\tOne\n"
    "Var20\t+-MSITESTVAR17\t;;2;;[~]\tOne\n"
    "Var21\t+-MSITESTVAR18\t1\tOne\n"
    "Var22\t+-MSITESTVAR18\t[~];;2;;\tOne\n"
    "Var23\t+-MSITESTVAR19\t1\tOne\n"
    "Var24\t+-MSITESTVAR19\t[~]2\tOne\n"
    "Var25\t+-MSITESTVAR20\t1\tOne\n"
    "Var26\t+-MSITESTVAR20\t2[~]\tOne\n"
    "Var27\t+-MSITESTVAR21\t[~];1\tOne\n"
    "Var28\t-MSITESTVAR22\t1\tOne\n"
    "Var29\t-MSITESTVAR23\t2\tOne\n"
    "Var30\t*MSITESTVAR100\t1\tOne\n"
    "Var31\t-=MSITESTVAR24\t[SERVNAME]\tOne\n"
    "Var32\t-=MSITESTVAR25\t[bogus_prop]\tOne\n";

static const char service_install_dat[] =
    "ServiceInstall\tName\tDisplayName\tServiceType\tStartType\tErrorControl\t"
    "LoadOrderGroup\tDependencies\tStartName\tPassword\tArguments\tComponent_\tDescription\n"
    "s72\ts255\tL255\ti4\ti4\ti4\tS255\tS255\tS255\tS255\tS255\ts72\tL255\n"
    "ServiceInstall\tServiceInstall\n"
    "TestService\t[SERVNAME]\t[SERVDISP]\t2\t3\t0\t\tservice1[~]+group1[~]service2[~]+group2[~][~]\tTestService\t\t-a arg\tservice_comp\tdescription\n"
    "TestService2\t[SERVNAME2]\t[SERVDISP2]\t2\t3\t0\t\tservice1[~]+group1[~]service2[~]+group2[~][~]\tTestService2\t\t-a arg\tservice_comp2\tdescription\n";

static const char service_install2_dat[] =
    "ServiceInstall\tName\tDisplayName\tServiceType\tStartType\tErrorControl\t"
    "LoadOrderGroup\tDependencies\tStartName\tPassword\tArguments\tComponent_\tDescription\n"
    "s72\ts255\tL255\ti4\ti4\ti4\tS255\tS255\tS255\tS255\tS255\ts72\tL255\n"
    "ServiceInstall\tServiceInstall\n"
    "TestService\tTestService\tTestService\t2\t3\t32768\t\t\tTestService\t\t\tservice_comp\t\n"
    "TestService4\tTestService4\tTestService4\t2\t3\t0\t\t\tTestService4\t\t\tservice_comp3\t\n";

static const char service_control_dat[] =
    "ServiceControl\tName\tEvent\tArguments\tWait\tComponent_\n"
    "s72\tl255\ti2\tL255\tI2\ts72\n"
    "ServiceControl\tServiceControl\n"
    "ServiceControl\tTestService3\t8\t\t0\tservice_comp\n"
    "ServiceControl2\tTestService3\t128\t\t0\tservice_comp2";

static const char sss_service_control_dat[] =
    "ServiceControl\tName\tEvent\tArguments\tWait\tComponent_\n"
    "s72\tl255\ti2\tL255\tI2\ts72\n"
    "ServiceControl\tServiceControl\n"
    "ServiceControl\tSpooler\t1\t\t1\tservice_comp\n"
    "ServiceControl2\tSpooler\t2\t\t1\tservice_comp\n"
    "ServiceControl3\tSpooler\t16\t\t1\tservice_comp\n"
    "ServiceControl4\tSpooler\t32\t\t1\tservice_comp\n";

static const char sss_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "ResolveSource\t\t950\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "StopServices\t\t4000\n"
    "stop_immediate\tNOT REMOVE\t4001\n"
    "stop_deferred\tNOT REMOVE\t4002\n"
    "DeleteServices\t\t5000\n"
    "MoveFiles\t\t5100\n"
    "InstallFiles\t\t5200\n"
    "DuplicateFiles\t\t5300\n"
    "StartServices\t\t5400\n"
    "start_deferred\tNOT REMOVE\t5401\n"
    "RegisterProduct\t\t5500\n"
    "PublishFeatures\t\t5600\n"
    "PublishProduct\t\t5700\n"
    "UnpublishFeatures\t\t5800\n"
    "InstallFinalize\t\t6000\n";

static const char sss_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "stop_immediate\t1\tcustom.dll\tsss_started\n"
    "stop_deferred\t1025\tcustom.dll\tsss_stopped\n"
    "start_deferred\t1025\tcustom.dll\tsss_started\n";

static const char sds_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "ResolveSource\t\t950\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "StopServices\t\t5000\n"
    "DeleteServices\t\t5050\n"
    "sds_immediate\tNOT REMOVE\t5051\n"
    "sds_deferred\tNOT REMOVE\t5052\n"
    "MoveFiles\t\t5100\n"
    "InstallFiles\t\t5200\n"
    "DuplicateFiles\t\t5300\n"
    "InstallServices\t\t5400\n"
    "StartServices\t\t5450\n"
    "RegisterProduct\t\t5500\n"
    "PublishFeatures\t\t5600\n"
    "PublishProduct\t\t5700\n"
    "UnpublishFeatures\t\t5800\n"
    "InstallFinalize\t\t6000\n";

static const char sds_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "sds_immediate\t1\tcustom.dll\tsds_present\n"
    "sds_deferred\t1025\tcustom.dll\tsds_absent\n";

static const char sis_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "StopServices\t\t5000\n"
    "DeleteServices\t\t5050\n"
    "InstallFiles\t\t5200\n"
    "InstallServices\t\t5400\n"
    "sis_immediate\tNOT REMOVE\t5401\n"
    "sis_deferred\tNOT REMOVE\t5402\n"
    "StartServices\t\t5450\n"
    "RegisterProduct\t\t5500\n"
    "PublishFeatures\t\t5600\n"
    "PublishProduct\t\t5700\n"
    "UnpublishFeatures\t\t5800\n"
    "InstallFinalize\t\t6000\n";

static const char sis_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "sis_immediate\t1\tcustom.dll\tsis_absent\n"
    "sis_deferred\t1025\tcustom.dll\tsis_present\n";

static const char rof_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t\tMSITESTDIR\t0\t1\tmaximus\n";

static const char rof_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "feature\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0\n"
    "montecristo\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0";

static const char rof_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\tmaximus\n"
    "montecristo\tmaximus";

static const char rof_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1";

static const char rof_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t1\t\t\tDISK1\t\n";

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

static const char pp_install_exec_seq_dat[] =
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
    "ppc_immediate\tPROCESS_COMPONENTS AND ALLUSERS\t1601\n"
    "ppc_deferred\tPROCESS_COMPONENTS AND ALLUSERS\t1602\n"
    "UnpublishFeatures\tUNPUBLISH_FEATURES=1 Or FULL=1\t1800\n"
    "uf_immediate\tUNPUBLISH_FEATURES AND ALLUSERS\t1801\n"
    "uf_deferred\tUNPUBLISH_FEATURES AND ALLUSERS\t1802\n"
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterUser\tREGISTER_USER=1 Or FULL=1\t6000\n"
    "ru_immediate\tREGISTER_USER AND ALLUSERS\t6001\n"
    "ru_deferred\tREGISTER_USER AND ALLUSERS\t6002\n"
    "RegisterProduct\tREGISTER_PRODUCT=1 Or FULL=1\t6100\n"
    "rp_immediate\tREGISTER_PRODUCT AND ALLUSERS AND NOT RP_TEST64\t6101\n"
    "rp_deferred\tREGISTER_PRODUCT AND ALLUSERS AND NOT RP_TEST64\t6102\n"
    "PublishFeatures\tPUBLISH_FEATURES=1 Or FULL=1\t6300\n"
    "pf_immediate\tPUBLISH_FEATURES AND ALLUSERS\t6301\n"
    "pf_deferred\tPUBLISH_FEATURES AND ALLUSERS\t6302\n"
    "PublishProduct\tPUBLISH_PRODUCT=1 Or FULL=1\t6400\n"
    "pp_immediate\tPUBLISH_PRODUCT AND ALLUSERS\t6401\n"
    "pp_deferred\tPUBLISH_PRODUCT AND ALLUSERS\t6402\n"
    "InstallFinalize\t\t6600";

static const char pp_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "pf_immediate\t1\tcustom.dll\tpf_absent\n"
    "pf_deferred\t1025\tcustom.dll\tpf_present\n"
    "uf_immediate\t1\tcustom.dll\tpf_present\n"
    "uf_deferred\t1025\tcustom.dll\tpf_absent\n"
    "pp_immediate\t1\tcustom.dll\tpp_absent\n"
    "pp_deferred\t1025\tcustom.dll\tpp_present\n"
    "rp_immediate\t1\tcustom.dll\trp_absent\n"
    "rp_deferred\t1025\tcustom.dll\trp_present\n"
    "ru_immediate\t1\tcustom.dll\tru_absent\n"
    "ru_deferred\t1025\tcustom.dll\tru_present\n"
    "ppc_immediate\t1\tcustom.dll\tppc_absent\n"
    "ppc_deferred\t1025\tcustom.dll\tppc_present\n";

static const char pp_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\t\tmaximus\n";

static const char ppc_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "maximus\t{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}\tMSITESTDIR\t0\t\tmaximus\n"
    "augustus\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t1\t\taugustus\n";

static const char ppc_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "maximus\tmaximus\tmaximus\t500\t\t\t8192\t1\n"
    "augustus\taugustus\taugustus\t500\t\t\t8192\t2";

static const char ppc_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t2\t\t\tDISK1\t\n";

static const char ppc_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\tmaximus\n"
    "feature\taugustus\n"
    "montecristo\tmaximus";

static const char cwd_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "augustus\t\tMSITESTDIR\t0\t\taugustus\n";

static const char rem_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "hydrogen\t{C844BD1E-1907-4C00-8BC9-150BD70DF0A1}\tMSITESTDIR\t0\t\thydrogen\n"
    "helium\t{5AD3C142-CEF8-490D-B569-784D80670685}\tMSITESTDIR\t1\t\thelium\n"
    "lithium\t\tMSITESTDIR\t2\t\tlithium\n";

static const char rem_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\thydrogen\n"
    "feature\thelium\n"
    "feature\tlithium";

static const char rem_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "hydrogen\thydrogen\thydrogen\t0\t\t\t8192\t1\n"
    "helium\thelium\thelium\t0\t\t\t8192\t1\n"
    "lithium\tlithium\tlithium\t0\t\t\t8192\t1";

static const char rem_install_exec_seq_dat[] =
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
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6300\n"
    "PublishProduct\t\t6400\n"
    "UnpublishFeatures\t\t6500\n"
    "InstallFinalize\t\t6600";

static const char rem_remove_files_dat[] =
    "FileKey\tComponent_\tFileName\tDirProperty\tInstallMode\n"
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

static const char mov_move_file_dat[] =
    "FileKey\tComponent_\tSourceName\tDestName\tSourceFolder\tDestFolder\tOptions\n"
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

static const char mov_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t100\n"
    "FileCost\t\t200\n"
    "ResolveSource\t\t300\n"
    "CostFinalize\t\t400\n"
    "InstallValidate\t\t500\n"
    "InstallInitialize\t\t600\n"
    "MoveFiles\t\t700\n"
    "mov_immediate\t\t701\n"
    "mov_deferred\t\t702\n"
    "InstallFiles\t\t800\n"
    "InstallFinalize\t\t900\n";

static const char mov_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "mov_immediate\t1\tcustom.dll\tmov_absent\n"
    "mov_deferred\t1025\tcustom.dll\tmov_present\n";

static const char df_directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "THIS\tMSITESTDIR\tthis\n"
    "DOESNOT\tTHIS\tdoesnot\n"
    "NONEXISTENT\tDOESNOT\texist\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char df_duplicate_file_dat[] =
    "FileKey\tComponent_\tFile_\tDestName\tDestFolder\n"
    "s72\ts72\ts72\tS255\tS72\n"
    "DuplicateFile\tFileKey\n"
    "maximus\tmaximus\tmaximus\taugustus\t\n"
    "caesar\tmaximus\tmaximus\t\tNONEXISTENT\n"
    "augustus\tnosuchcomponent\tmaximus\t\tMSITESTDIR\n";

static const char wrv_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "augustus\t{bafe0297-cbcc-4e08-8a78-7e9b83456eef}\tMSITESTDIR\t0\t\t\n"
    "caesar\t{bafe0298-cbcc-4e08-8a78-7e9b83456eef}\tMSITESTDIR\t1\t\t\n";

static const char wrv_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "feature\t\tFeature\tFeature\t2\t1\tTARGETDIR\t0\n"
    "feature2\t\tFeature2\tFeature2\t2\t1\tTARGETDIR\t1";

static const char wrv_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "feature\taugustus\n"
    "feature2\tcaesar";

static const char wrv_registry_dat[] =
    "Registry\tRoot\tKey\tName\tValue\tComponent_\n"
    "s72\ti2\tl255\tL255\tL0\ts72\n"
    "Registry\tRegistry\n"
    "regdata0\t1\tmsitest\tsz\tstring\taugustus\n"
    "regdata1\t1\tmsitest\tmultisz\tone[~]two[~]three\taugustus\n"
    "regdata2\t1\tmsitest\texpandsz\t#%string\taugustus\n"
    "regdata3\t1\tmsitest\tdword\t#123\taugustus\n"
    "regdata4\t1\tmsitest\tbinary\t#x01234567\taugustus\n"
    "regdata5\t1\tmsitest\t\tdefault\taugustus\n"
    "regdata6\t1\tmsitest\\subkey\t*\t\taugustus\n"
    "regdata7\t1\tmsitest\\subkey\t*\t#%\taugustus\n"
    "regdata8\t1\tmsitest\\subkey\t*\t#x\taugustus\n"
    "regdata9\t1\tmsitest\\VisualStudio\\10.0\\AD7Metrics\\Exception\\{049EC4CC-30D2-4032-9256-EE18EB41B62B}\\Common Language Runtime Exceptions\\System.Workflow.ComponentModel.Serialization\\System.Workflow.ComponentModel.Serialization.WorkflowMarkupSerializationException\tlong\tkey\taugustus\n"
    "regdata10\t1\tmsitest\tValue1\t[~]one[~]\taugustus\n"
    "regdata11\t1\tmsitest\tValue2\t[~]two\taugustus\n"
    "regdata12\t1\tmsitest\tValue3\tone[~]\taugustus\n"
    "regdata13\t1\tmsitest\tValue4\tone[~]two\taugustus\n"
    "regdata14\t1\tmsitest\tValue5\t[~]one[~]two[~]three\taugustus\n"
    "regdata15\t1\tmsitest\tValue6\t[~]\taugustus\n"
    "regdata16\t1\tmsitest\tValue7\t[~]two\taugustus\n"
    "regdata17\t1\tmsitest\tformat\t[regdata17]\taugustus\n"
    "regdata18\t1\tmsitest\tsource\t#1\tcaesar\n"
    "regdata19\t1\tmsitest\\create\t+\t\taugustus\n"
    "regdata20\t1\tmsitest\\delete\t-\t\taugustus\n";

static const char wrv_remove_registry_dat[] =
    "RemoveRegistry\tRoot\tKey\tName\tComponent_\n"
    "s72\ti2\tl255\tL255\ts72\n"
    "RemoveRegistry\tRemoveRegistry\n"
    "regdata0\t1\tmsitest\tinstremove\taugustus\n"
    "regdata1\t1\tmsitest\\instremove\t-\taugustus\n";

static const char wrv_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveRegistryValues\t\t4900\n"
    "rrv_immediate\tREMOVE\t4901\n"
    "rrv_deferred\tREMOVE\t4902\n"
    "WriteRegistryValues\t\t5000\n"
    "wrv_immediate\tNOT REMOVE\t5001\n"
    "wrv_deferred\tNOT REMOVE\t5002\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6200\n"
    "PublishProduct\t\t6300\n"
    "UnpublishFeatures\t\t6400\n"
    "InstallFinalize\t\t6600\n";

static const char wrv_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "wrv_immediate\t1\tcustom.dll\twrv_absent\n"
    "wrv_deferred\t1025\tcustom.dll\twrv_present\n"
    "rrv_immediate\t1\tcustom.dll\twrv_present\n"
    "rrv_deferred\t1025\tcustom.dll\twrv_absent\n";

static const char cf_directory_dat[] =
    "Directory\tDirectory_Parent\tDefaultDir\n"
    "s72\tS72\tl255\n"
    "Directory\tDirectory\n"
    "FIRSTDIR\tMSITESTDIR\tfirst\n"
    "SECONDDIR\tMSITESTDIR\tsecond\n"
    "THIRDDIR\tMSITESTDIR\tthird\n"
    "MSITESTDIR\tProgramFilesFolder\tmsitest\n"
    "ProgramFilesFolder\tTARGETDIR\t.\n"
    "TARGETDIR\t\tSourceDir";

static const char cf_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "One\t{F8CD42AC-9C38-48FE-8664-B35FD121012A}\tFIRSTDIR\t0\t\tone.txt\n"
    "Two\t{DE2DB02E-2DDF-4E34-8CF6-DCA13E29DF52}\tSECONDDIR\t0\t\ttwo.txt\n";

static const char cf_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "One\t\tOne\tThe One Feature\t1\t3\tFIRSTDIR\t0\n"
    "Two\t\tTwo\tThe Two Feature\t1\t3\tSECONDDIR\t0\n";

static const char cf_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "One\tOne\n"
    "Two\tTwo\n";

static const char cf_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "one.txt\tOne\tone.txt\t0\t\t\t0\t1\n"
    "two.txt\tTwo\ttwo.txt\t0\t\t\t0\t2\n";

static const char cf_create_folders_dat[] =
    "Directory_\tComponent_\n"
    "s72\ts72\n"
    "CreateFolder\tDirectory_\tComponent_\n"
    "FIRSTDIR\tOne\n"
    "SECONDDIR\tTwo\n"
    "THIRDDIR\tTwo\n";

static const char cf_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostFinalize\t\t1000\n"
    "ValidateProductID\t\t700\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "RemoveFiles\t\t3500\n"
    "rfi_immediate\tREMOVE\t3501\n"
    "rfi_deferred\tREMOVE\t3502\n"
    "CreateFolders\t\t3700\n"
    "cf_immediate\tNOT REMOVE\t3701\n"
    "cf_deferred\tNOT REMOVE\t3702\n"
    "RemoveFolders\t\t3800\n"
    "rf_immediate\tREMOVE\t3801\n"
    "rf_deferred\tREMOVE\t3802\n"
    "InstallFiles\t\t4000\n"
    "ifi_immediate\tNOT REMOVE\t4001\n"
    "ifi_deferred\tNOT REMOVE\t4002\n"
    "RegisterUser\t\t6000\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6300\n"
    "PublishProduct\t\t6400\n"
    "InstallFinalize\t\t6600\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "UnpublishFeatures\t\t1800\n"
    "InstallValidate\t\t1400\n"
    "LaunchConditions\t\t100\n";

static const char cf_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "ifi_immediate\t1\tcustom.dll\tfile_absent\n"
    "ifi_deferred\t1025\tcustom.dll\tfile_present\n"
    "rfi_immediate\t1\tcustom.dll\tfile_present\n"
    "rfi_deferred\t1025\tcustom.dll\tfile_absent\n"
    "cf_immediate\t1\tcustom.dll\tcf_absent\n"
    "cf_deferred\t1025\tcustom.dll\tcf_present\n"
    "rf_immediate\t1\tcustom.dll\tcf_present\n"
    "rf_deferred\t1025\tcustom.dll\tcf_absent\n";

static const char sr_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "selfreg.dll\tselfreg\tselfreg.dll\t1000\t\t\t8192\t1\n";

static const char sr_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "selfreg\t\t\tselfreg feature\t1\t2\tMSITESTDIR\t0\n";

static const char sr_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "selfreg\tselfreg\n";

static const char sr_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "selfreg\t{BB4C26FD-89D8-4E49-AF1C-DB4DCB5BF1B0}\tMSITESTDIR\t0\t\tselfreg.dll\n";

static const char sr_selfreg_dat[] =
    "File_\tCost\n"
    "s72\tI2\n"
    "SelfReg\tFile_\n"
    "selfreg.dll\t\n";

static const char sr_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "SelfUnregModules\t\t3800\n"
    "su_immediate\tREMOVE\t3801\n"
    "su_deferred\tREMOVE\t3802\n"
    "RemoveFiles\t\t3900\n"
    "InstallFiles\t\t4000\n"
    "SelfRegModules\t\t4100\n"
    "sr_immediate\tNOT REMOVE\t4101\n"
    "sr_deferred\tNOT REMOVE\t4102\n"
    "CreateShortcuts\t\t4600\n"
    "RegisterProduct\t\t5100\n"
    "PublishFeatures\t\t5200\n"
    "PublishProduct\t\t5300\n"
    "UnpublishFeatures\t\t5400\n"
    "InstallFinalize\t\t6600\n";

static const char sr_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "sr_immediate\t1\tcustom.dll\tsr_absent\n"
    "sr_deferred\t1025\tcustom.dll\tsr_present\n"
    "su_immediate\t1\tcustom.dll\tsr_present\n"
    "su_deferred\t1025\tcustom.dll\tsr_absent\n";

static const char font_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t3\t\t\tDISK1\t\n";

static const char font_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "font.ttf\tfonts\tfont.ttf\t1000\t\t\t8192\t1\n";

static const char font_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "fonts\t\t\tfont feature\t1\t2\tMSITESTDIR\t0\n";

static const char font_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "fonts\t{F5920ED0-1183-4B8F-9330-86CE56557C05}\tMSITESTDIR\t0\t\tfont.ttf\n";

static const char font_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "fonts\tfonts\n";

static const char font_dat[] =
    "File_\tFontTitle\n"
    "s72\tS128\n"
    "Font\tFile_\n"
    "font.ttf\tmsi test font\n";

static const char font_install_exec_seq_dat[] =
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
    "RemoveFiles\t\t3500\n"
    "InstallFiles\t\t4000\n"
    "RegisterFonts\t\t4100\n"
    "rf_immediate\tNOT REMOVE\t4101\n"
    "rf_deferred\tNOT REMOVE\t4102\n"
    "UnregisterFonts\t\t4200\n"
    "uf_immediate\tREMOVE\t4201\n"
    "uf_deferred\tREMOVE\t4202\n"
    "RegisterUser\t\t6000\n"
    "RegisterProduct\t\t6100\n"
    "PublishFeatures\t\t6300\n"
    "PublishProduct\t\t6400\n"
    "UnpublishFeatures\t\t6500\n"
    "InstallFinalize\t\t6600";

static const char font_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rf_immediate\t1\tcustom.dll\tfont_absent\n"
    "rf_deferred\t1025\tcustom.dll\tfont_present\n"
    "uf_immediate\t1\tcustom.dll\tfont_present\n"
    "uf_deferred\t1025\tcustom.dll\tfont_absent\n";

static const char vp_property_dat[] =
    "Property\tValue\n"
    "s72\tl0\n"
    "Property\tProperty\n"
    "HASUIRUN\t0\n"
    "INSTALLLEVEL\t3\n"
    "InstallMode\tTypical\n"
    "Manufacturer\tWine\n"
    "PIDTemplate\t###-#######\n"
    "ProductCode\t{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}\n"
    "ProductLanguage\t1033\n"
    "ProductName\tMSITEST\n"
    "ProductVersion\t1.1.1\n"
    "UpgradeCode\t{4C0EAA15-0264-4E5A-8758-609EF142B92D}\n"
    "MSIFASTINSTALL\t1\n";

static const char vp_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "SetProductID1\t51\tProductID\t1\t\n"
    "SetProductID2\t51\tProductID\t2\t\n"
    "TestProductID1\t19\t\t\tHalts installation\n"
    "TestProductID2\t19\t\t\tHalts installation\n";

static const char vp_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "SetProductID1\tSET_PRODUCT_ID=1\t3000\n"
    "SetProductID2\tSET_PRODUCT_ID=2\t3100\n"
    "ValidateProductID\t\t3200\n"
    "InstallExecute\t\t3300\n"
    "TestProductID1\tProductID=1\t3400\n"
    "TestProductID2\tProductID=\"123-1234567\"\t3500\n"
    "InstallFiles\t\t4000\n"
    "InstallFinalize\t\t6000\n";

static const char odbc_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "ODBCdriver.dll\todbc\tODBCdriver.dll\t1000\t\t\t8192\t1\n"
    "ODBCdriver2.dll\todbc\tODBCdriver2.dll\t1000\t\t\t8192\t2\n"
    "ODBCtranslator.dll\todbc\tODBCtranslator.dll\t1000\t\t\t8192\t3\n"
    "ODBCtranslator2.dll\todbc\tODBCtranslator2.dll\t1000\t\t\t8192\t4\n"
    "ODBCsetup.dll\todbc\tODBCsetup.dll\t1000\t\t\t8192\t5\n";

static const char odbc_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "odbc\t\t\todbc feature\t1\t2\tMSITESTDIR\t0\n";

static const char odbc_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "odbc\todbc64\n"
    "odbc\todbc\n";

static const char odbc_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "odbc64\t{B6F3E4AF-35D1-4B72-9044-989F03E20A43}\tMSITESTDIR\t256\tMsix64\tODBCdriver.dll\n"
    "odbc\t{B6F3E4AE-35D1-4B72-9044-989F03E20A43}\tMSITESTDIR\t0\t\tODBCdriver.dll\n";

static const char odbc_driver_dat[] =
    "Driver\tComponent_\tDescription\tFile_\tFile_Setup\n"
    "s72\ts72\ts255\ts72\tS72\n"
    "ODBCDriver\tDriver\n"
    "64-bit driver\todbc64\tODBC test driver\tODBCdriver.dll\t\n"
    "64-bit driver2\todbc64\tODBC test driver2\tODBCdriver2.dll\tODBCsetup.dll\n"
    "ODBC test driver\todbc\tODBC test driver\tODBCdriver.dll\t\n"
    "ODBC test driver2\todbc\tODBC test driver2\tODBCdriver2.dll\tODBCsetup.dll\n";

static const char odbc_translator_dat[] =
    "Translator\tComponent_\tDescription\tFile_\tFile_Setup\n"
    "s72\ts72\ts255\ts72\tS72\n"
    "ODBCTranslator\tTranslator\n"
    "ODBC test translator\todbc\tODBC test translator\tODBCtranslator.dll\t\n"
    "ODBC test translator2\todbc\tODBC test translator2\tODBCtranslator2.dll\tODBCsetup.dll\n";

static const char odbc_datasource_dat[] =
    "DataSource\tComponent_\tDescription\tDriverDescription\tRegistration\n"
    "s72\ts72\ts255\ts255\ti2\n"
    "ODBCDataSource\tDataSource\n"
    "ODBC data source\todbc\tODBC data source\tODBC driver\t0\n";

static const char odbc_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "InstallODBC\t\t3000\n"
    "io_immediate\tNOT REMOVE\t3001\n"
    "io_deferred\tNOT REMOVE\t3002\n"
    "RemoveODBC\t\t3100\n"
    "ro_immediate\tREMOVE\t3101\n"
    "ro_deferred\tREMOVE\t3102\n"
    "RemoveFiles\t\t3900\n"
    "InstallFiles\t\t4000\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char odbc_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "io_immediate\t1\tcustom.dll\todbc_absent\n"
    "io_deferred\t1025\tcustom.dll\todbc_present\n"
    "ro_immediate\t1\tcustom.dll\todbc_present\n"
    "ro_deferred\t1025\tcustom.dll\todbc_absent\n";

static const char odbc_media_dat[] =
    "DiskId\tLastSequence\tDiskPrompt\tCabinet\tVolumeLabel\tSource\n"
    "i2\ti4\tL64\tS255\tS32\tS72\n"
    "Media\tDiskId\n"
    "1\t5\t\t\tDISK1\t\n";

static const char tl_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "typelib.dll\ttypelib\ttypelib.dll\t1000\t\t\t8192\t1\n";

static const char tl_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "typelib\t\t\ttypelib feature\t1\t2\tMSITESTDIR\t0\n";

static const char tl_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "typelib\ttypelib\n";

static const char tl_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "typelib\t{BB4C26FD-89D8-4E49-AF1C-DB4DCB5BF1B0}\tMSITESTDIR\t0\t\ttypelib.dll\n";

static const char tl_typelib_dat[] =
    "LibID\tLanguage\tComponent_\tVersion\tDescription\tDirectory_\tFeature_\tCost\n"
    "s38\ti2\ts72\tI4\tL128\tS72\ts38\tI4\n"
    "TypeLib\tLibID\tLanguage\tComponent_\n"
    "{EAC5166A-9734-4D91-878F-1DD02304C66C}\t0\ttypelib\t1793\t\tMSITESTDIR\ttypelib\t\n";

static const char tl_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "UnregisterTypeLibraries\t\t3100\n"
    "ut_immediate\tREMOVE\t3101\n"
    "ut_deferred\tREMOVE\t3102\n"
    "RemoveFiles\t\t3200\n"
    "InstallFiles\t\t3300\n"
    "RegisterTypeLibraries\t\t3400\n"
    "rt_immediate\tNOT REMOVE\t3401\n"
    "rt_deferred\tNOT REMOVE\t3402\n"
    "RegisterProduct\t\t5100\n"
    "PublishFeatures\t\t5200\n"
    "PublishProduct\t\t5300\n"
    "UnpublishFeatures\t\t5400\n"
    "InstallFinalize\t\t6000\n";

static const char tl_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rt_immediate\t1\tcustom.dll\ttl_absent\n"
    "rt_deferred\t1025\tcustom.dll\ttl_present\n"
    "ut_immediate\t1\tcustom.dll\ttl_present\n"
    "ut_deferred\t1025\tcustom.dll\ttl_absent\n";

static const char crs_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "target.txt\tshortcut\ttarget.txt\t1000\t\t\t8192\t1\n";

static const char crs_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "shortcut\t\t\tshortcut feature\t1\t2\tMSITESTDIR\t0\n";

static const char crs_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "shortcut\tshortcut\n";

static const char crs_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "shortcut\t{5D20E3C6-7206-498F-AC28-87AF2F9AD4CC}\tMSITESTDIR\t0\t\ttarget.txt\n";

static const char crs_shortcut_dat[] =
    "Shortcut\tDirectory_\tName\tComponent_\tTarget\tArguments\tDescription\tHotkey\tIcon_\tIconIndex\tShowCmd\tWkDir\n"
    "s72\ts72\tl128\ts72\ts72\tL255\tL255\tI2\tS72\tI2\tI2\tS72\n"
    "Shortcut\tShortcut\n"
    "shortcut\tMSITESTDIR\tshortcut\tshortcut\t[MSITESTDIR]target.txt\t\t\t\t\t\t\t\n";

static const char crs_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "RemoveShortcuts\t\t3000\n"
    "rs_immediate\tREMOVE\t3001\n"
    "rs_deferred\tREMOVE\t3002\n"
    "CreateShortcuts\t\t3100\n"
    "cs_immediate\tNOT REMOVE\t3101\n"
    "cs_deferred\tNOT REMOVE\t3102\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char crs_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "cs_immediate\t1\tcustom.dll\tcrs_absent\n"
    "cs_deferred\t1025\tcustom.dll\tcrs_present\n"
    "rs_immediate\t1\tcustom.dll\tcrs_present\n"
    "rs_deferred\t1025\tcustom.dll\tcrs_absent\n";

static const char pub_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "english.txt\tpublish\tenglish.txt\t1000\t\t\t8192\t1\n";

static const char pub_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "publish\t\t\tpublish feature\t1\t2\tMSITESTDIR\t0\n";

static const char pub_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "publish\tpublish\n";

static const char pub_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "publish\t{B4EA0ACF-6238-426E-9C6D-7869F0F9C768}\tMSITESTDIR\t0\t\tenglish.txt\n";

static const char pub_publish_component_dat[] =
    "ComponentId\tQualifier\tComponent_\tAppData\tFeature_\n"
    "s38\ts255\ts72\tL255\ts38\n"
    "PublishComponent\tComponentId\tQualifier\tComponent_\n"
    "{92AFCBC0-9CA6-4270-8454-47C5EE2B8FAA}\tenglish.txt\tpublish\t\tpublish\n";

static const char pub_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "PublishComponents\t\t3000\n"
    "pub_immediate\tNOT REMOVE\t3001\n"
    "pub_deferred\tNOT REMOVE\t3002\n"
    "UnpublishComponents\t\t3100\n"
    "unp_immediate\tREMOVE\t3101\n"
    "unp_deferred\tREMOVE\t3102\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char pub_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "pub_immediate\t1\tcustom.dll\tpub_absent\n"
    "pub_deferred\t1025\tcustom.dll\tpub_present\n"
    "unp_immediate\t1\tcustom.dll\tpub_present\n"
    "unp_deferred\t1025\tcustom.dll\tpub_absent\n";

static const char rd_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "original.txt\tduplicate\toriginal.txt\t1000\t\t\t8192\t1\n"
    "original2.txt\tduplicate\toriginal2.txt\t1000\t\t\t8192\t2\n"
    "original3.txt\tduplicate2\toriginal3.txt\t1000\t\t\t8192\t3\n";

static const char rd_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "duplicate\t\t\tduplicate feature\t1\t2\tMSITESTDIR\t0\n";

static const char rd_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "duplicate\tduplicate\n";

static const char rd_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "duplicate\t{EB45D06A-ADFE-44E3-8D41-B7DE150E41AD}\tMSITESTDIR\t0\t\toriginal.txt\n"
    "duplicate2\t{B8BA60E0-B2E9-488E-9D0E-E60F25F04F97}\tMSITESTDIR\t0\tDUPLICATE2=1\toriginal3.txt\n";

static const char rd_duplicate_file_dat[] =
    "FileKey\tComponent_\tFile_\tDestName\tDestFolder\n"
    "s72\ts72\ts72\tS255\tS72\n"
    "DuplicateFile\tFileKey\n"
    "duplicate\tduplicate\toriginal.txt\tduplicate.txt\t\n"
    "duplicate2\tduplicate\toriginal2.txt\t\tMSITESTDIR\n"
    "duplicate3\tduplicate2\toriginal3.txt\tduplicate2.txt\t\n";

static const char rd_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveDuplicateFiles\t\t1900\n"
    "rd_immediate\tREMOVE\t1901\n"
    "rd_deferred\tREMOVE\t1902\n"
    "InstallFiles\t\t2000\n"
    "DuplicateFiles\t\t2100\n"
    "df_immediate\tNOT REMOVE\t2101\n"
    "df_deferred\tNOT REMOVE\t2102\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char rd_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "df_immediate\t1\tcustom.dll\trd_absent\n"
    "df_deferred\t1025\tcustom.dll\trd_present\n"
    "rd_immediate\t1\tcustom.dll\trd_present\n"
    "rd_deferred\t1025\tcustom.dll\trd_absent\n";

static const char frp_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "product.txt\tproduct\tproduct.txt\t1000\t\t\t8192\t1\n";

static const char frp_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "product\t\t\tproduct feature\t1\t2\tMSITESTDIR\t0\n";

static const char frp_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "product\tproduct\n";

static const char frp_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "product\t{44725EE0-EEA8-40BD-8162-A48224A2FEA1}\tMSITESTDIR\t0\t\tproduct.txt\n";

static const char frp_custom_action_dat[] =
    "Action\tType\tSource\tTarget\tISComments\n"
    "s72\ti2\tS64\tS0\tS255\n"
    "CustomAction\tAction\n"
    "TestProp\t19\t\t\tPROP set\n";

static const char frp_upgrade_dat[] =
    "UpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes\tRemove\tActionProperty\n"
    "s38\tS20\tS20\tS255\ti4\tS255\ts72\n"
    "Upgrade\tUpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes\n"
    "{4C0EAA15-0264-4E5A-8758-609EF142B92D}\t1.1.1\t2.2.2\t\t768\t\tPROP\n";

static const char frp_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "FindRelatedProducts\t\t50\n"
    "TestProp\tPROP AND NOT REMOVE\t51\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char ini_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "inifile.txt\tinifile\tinifile.txt\t1000\t\t\t8192\t1\n";

static const char ini_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "inifile\t\t\tinifile feature\t1\t2\tMSITESTDIR\t0\n";

static const char ini_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "inifile\tinifile\n";

static const char ini_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "inifile\t{A0F15705-4F57-4437-88C4-6C8B37ACC6DE}\tMSITESTDIR\t0\t\tinifile.txt\n";

static const char ini_ini_file_dat[] =
    "IniFile\tFileName\tDirProperty\tSection\tKey\tValue\tAction\tComponent_\n"
    "s72\tl255\tS72\tl96\tl128\tl255\ti2\ts72\n"
    "IniFile\tIniFile\n"
    "inifile1\ttest.ini\tMSITESTDIR\tsection1\tkey1\tvalue1\t0\tinifile\n";

static const char ini_remove_ini_file_dat[] =
    "RemoveIniFile\tFileName\tDirProperty\tSection\tKey\tValue\tAction\tComponent_\n"
    "s72\tl255\tS72\tl96\tl128\tL255\ti2\ts72\n"
    "RemoveIniFile\tRemoveIniFile\n"
    "inifile1\ttest.ini\tMSITESTDIR\tsectionA\tkeyA\tvalueA\t2\tinifile\n";

static const char ini_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "RemoveIniValues\t\t3000\n"
    "riv_immediate\tREMOVE\t3001\n"
    "riv_deferred\tREMOVE\t3002\n"
    "WriteIniValues\t\t3100\n"
    "wiv_immediate\tNOT REMOVE\t3101\n"
    "wiv_deferred\tNOT REMOVE\t3102\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char ini_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "wiv_immediate\t1\tcustom.dll\tini_absent\n"
    "wiv_deferred\t1025\tcustom.dll\tini_present\n"
    "riv_immediate\t1\tcustom.dll\tini_present\n"
    "riv_deferred\t1025\tcustom.dll\tini_absent\n";

static const char rci_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "class.txt\tclass\tclass.txt\t1000\t\t\t8192\t1\n";

static const char rci_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "class\t\t\tclass feature\t1\t2\tMSITESTDIR\t0\n";

static const char rci_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "class\tclass\n";

static const char rci_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "class\t{89A98345-F8A1-422E-A48B-0250B5809F2D}\tMSITESTDIR\t0\t\tclass.txt\n";

static const char rci_appid_dat[] =
    "AppId\tRemoteServerName\tLocalService\tServiceParameters\tDllSurrogate\tActivateAtStorage\tRunAsInteractiveUser\n"
    "s38\tS255\tS255\tS255\tS255\tI2\tI2\n"
    "AppId\tAppId\n"
    "{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\t\t\t\t\t\t\n";

static const char rci_class_dat[] =
    "CLSID\tContext\tComponent_\tProgId_Default\tDescription\tAppId_\tFileTypeMask\tIcon_\tIconIndex\tDefInprocHandler\tArgument\tFeature_\tAttributes\n"
    "s38\ts32\ts72\tS255\tL255\tS38\tS255\tS72\tI2\tS32\tS255\ts38\tI2\n"
    "Class\tCLSID\tContext\tComponent_\n"
    "{110913E7-86D1-4BF3-9922-BA103FCDDDFA}\tLocalServer\tclass\t\tdescription\t{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\tmask1;mask2\t\t\t2\t\tclass\t\n";

static const char rci_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "UnregisterClassInfo\t\t3000\n"
    "uci_immediate\tREMOVE\t3001\n"
    "uci_deferred\tREMOVE\t3002\n"
    "RegisterClassInfo\t\t4000\n"
    "rci_immediate\tNOT REMOVE\t4001\n"
    "rci_deferred\tNOT REMOVE\t4002\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char rci_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rci_immediate\t1\tcustom.dll\trci_absent\n"
    "rci_deferred\t1025\tcustom.dll\trci_present\n"
    "uci_immediate\t1\tcustom.dll\trci_present\n"
    "uci_deferred\t1025\tcustom.dll\trci_absent\n";

static const char rei_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "extension.txt\textension\textension.txt\t1000\t\t\t8192\t1\n";

static const char rei_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "extension\t\t\textension feature\t1\t2\tMSITESTDIR\t0\n";

static const char rei_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "extension\textension\n";

static const char rei_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "extension\t{9A3060D4-60BA-4A82-AB55-9FB148AD013C}\tMSITESTDIR\t0\t\textension.txt\n";

static const char rei_extension_dat[] =
    "Extension\tComponent_\tProgId_\tMIME_\tFeature_\n"
    "s255\ts72\tS255\tS64\ts38\n"
    "Extension\tExtension\tComponent_\n"
    "extension\textension\tProg.Id.1\t\textension\n";

static const char rei_verb_dat[] =
    "Extension_\tVerb\tSequence\tCommand\tArgument\n"
    "s255\ts32\tI2\tL255\tL255\n"
    "Verb\tExtension_\tVerb\n"
    "extension\tOpen\t1\t&Open\t/argument\n";

static const char rei_progid_dat[] =
    "ProgId\tProgId_Parent\tClass_\tDescription\tIcon_\tIconIndex\n"
    "s255\tS255\tS38\tL255\tS72\tI2\n"
    "ProgId\tProgId\n"
    "Prog.Id.1\t\t\tdescription\t\t\n";

static const char rei_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "UnregisterExtensionInfo\t\t3000\n"
    "uei_immediate\tREMOVE\t3001\n"
    "uei_deferred\tREMOVE\t3002\n"
    "RegisterExtensionInfo\t\t4000\n"
    "rei_immediate\tNOT REMOVE\t4001\n"
    "rei_deferred\tNOT REMOVE\t4002\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char rei_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rei_immediate\t1\tcustom.dll\trei_absent\n"
    "rei_deferred\t1025\tcustom.dll\trei_present\n"
    "uei_immediate\t1\tcustom.dll\trei_present\n"
    "uei_deferred\t1025\tcustom.dll\trei_absent\n";

static const char rpi_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "progid.txt\tprogid\tprogid.txt\t1000\t\t\t8192\t1\n";

static const char rpi_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "progid\t\t\tprogid feature\t1\t2\tMSITESTDIR\t0\n";

static const char rpi_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "progid\tprogid\n";

static const char rpi_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "progid\t{89A98345-F8A1-422E-A48B-0250B5809F2D}\tMSITESTDIR\t0\t\tprogid.txt\n";

static const char rpi_appid_dat[] =
    "AppId\tRemoteServerName\tLocalService\tServiceParameters\tDllSurrogate\tActivateAtStorage\tRunAsInteractiveUser\n"
    "s38\tS255\tS255\tS255\tS255\tI2\tI2\n"
    "AppId\tAppId\n"
    "{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\t\t\t\t\t\t\n";

static const char rpi_class_dat[] =
    "CLSID\tContext\tComponent_\tProgId_Default\tDescription\tAppId_\tFileTypeMask\tIcon_\tIconIndex\tDefInprocHandler\tArgument\tFeature_\tAttributes\n"
    "s38\ts32\ts72\tS255\tL255\tS38\tS255\tS72\tI2\tS32\tS255\ts38\tI2\n"
    "Class\tCLSID\tContext\tComponent_\n"
    "{110913E7-86D1-4BF3-9922-BA103FCDDDFA}\tLocalServer\tprogid\tWinetest.Class.1\tdescription\t{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\tmask1;mask2\t\t\t2\t\tprogid\t\n"
    "{904E6BC9-F57F-4412-B460-D40DE2F256E2}\tLocalServer\tprogid\tWinetest.VerClass\tdescription\t{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\tmask1;mask2\t\t\t2\t\tprogid\t\n"
    "{57C413FB-CA02-498A-81F6-7E769BDB7C97}\tLocalServer\tprogid\t\tdescription\t{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}\tmask1;mask2\t\t\t2\t\tprogid\t\n";

static const char rpi_extension_dat[] =
    "Extension\tComponent_\tProgId_\tMIME_\tFeature_\n"
    "s255\ts72\tS255\tS64\ts38\n"
    "Extension\tExtension\tComponent_\n"
    "winetest\tprogid\tWinetest.Extension\t\tprogid\n";

static const char rpi_verb_dat[] =
    "Extension_\tVerb\tSequence\tCommand\tArgument\n"
    "s255\ts32\tI2\tL255\tL255\n"
    "Verb\tExtension_\tVerb\n"
    "winetest\tOpen\t1\t&Open\t/argument\n";

static const char rpi_progid_dat[] =
    "ProgId\tProgId_Parent\tClass_\tDescription\tIcon_\tIconIndex\n"
    "s255\tS255\tS38\tL255\tS72\tI2\n"
    "ProgId\tProgId\n"
    "Winetest.Class.1\t\t{110913E7-86D1-4BF3-9922-BA103FCDDDFA}\tdescription\t\t\n"
    "Winetest.Class\tWinetest.Class.1\t\tdescription\t\t\n"
    "Winetest.Class.2\t\t{110913E7-86D1-4BF3-9922-BA103FCDDDFA}\tdescription\t\t\n"
    "Winetest.VerClass.1\t\t{904E6BC9-F57F-4412-B460-D40DE2F256E2}\tdescription\t\t\n"
    "Winetest.VerClass\tWinetest.VerClass.1\t\tdescription\t\t\n"
    "Winetest.NoProgIdClass.1\t\t{57C413FB-CA02-498A-81F6-7E769BDB7C97}\tdescription\t\t\n"
    "Winetest.NoProgIdClass\tWinetest.NoProgIdClass.1\t\tdescription\t\t\n"
    "Winetest.Orphaned\t\t\tdescription\t\t\n"
    "Winetest.Orphaned2\t\t\tdescription\t\t\n"
    "Winetest.Extension\t\t\tdescription\t\t\n";

static const char rpi_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "UnregisterClassInfo\t\t3000\n"
    "UnregisterExtensionInfo\t\t3200\n"
    "UnregisterProgIdInfo\t\t3400\n"
    "upi_immediate\tREMOVE\t3401\n"
    "upi_deferred\tREMOVE\t3402\n"
    "InstallFiles\t\t3600\n"
    "RegisterClassInfo\t\t4000\n"
    "RegisterExtensionInfo\t\t4200\n"
    "RegisterProgIdInfo\t\t4400\n"
    "rpi_immediate\tNOT REMOVE\t4401\n"
    "rpi_deferred\tNOT REMOVE\t4402\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char rpi_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rpi_immediate\t1\tcustom.dll\trpi_absent\n"
    "rpi_deferred\t1025\tcustom.dll\trpi_present\n"
    "upi_immediate\t1\tcustom.dll\trpi_present\n"
    "upi_deferred\t1025\tcustom.dll\trpi_absent\n";

static const char rmi_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "mime.txt\tmime\tmime.txt\t1000\t\t\t8192\t1\n";

static const char rmi_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "mime\t\t\tmime feature\t1\t2\tMSITESTDIR\t0\n";

static const char rmi_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "mime\tmime\n";

static const char rmi_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "mime\t{A1D630CE-13A7-4882-AFDD-148E2BBAFC6D}\tMSITESTDIR\t0\t\tmime.txt\n";

static const char rmi_extension_dat[] =
    "Extension\tComponent_\tProgId_\tMIME_\tFeature_\n"
    "s255\ts72\tS255\tS64\ts38\n"
    "Extension\tExtension\tComponent_\n"
    "mime\tmime\t\tmime/type\tmime\n";

static const char rmi_verb_dat[] =
    "Extension_\tVerb\tSequence\tCommand\tArgument\n"
    "s255\ts32\tI2\tL255\tL255\n"
    "Verb\tExtension_\tVerb\n"
    "mime\tOpen\t1\t&Open\t/argument\n";

static const char rmi_mime_dat[] =
    "ContentType\tExtension_\tCLSID\n"
    "s64\ts255\tS38\n"
    "MIME\tContentType\n"
    "mime/type\tmime\t\n";

static const char rmi_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "UnregisterExtensionInfo\t\t3000\n"
    "UnregisterMIMEInfo\t\t3500\n"
    "umi_immediate\tREMOVE\t3501\n"
    "umi_deferred\tREMOVE\t3502\n"
    "RegisterExtensionInfo\t\t4000\n"
    "RegisterMIMEInfo\t\t4500\n"
    "rmi_immediate\tNOT REMOVE\t4501\n"
    "rmi_deferred\tNOT REMOVE\t4502\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char rmi_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "rmi_immediate\t1\tcustom.dll\trmi_absent\n"
    "rmi_deferred\t1025\tcustom.dll\trmi_present\n"
    "umi_immediate\t1\tcustom.dll\trmi_present\n"
    "umi_deferred\t1025\tcustom.dll\trmi_absent\n";

static const char pa_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "fake_local.txt\tfake_local\tfake_local.txt\t1000\t\t\t8192\t1\n"
    "win32.txt\twin32\twin32.txt\t1000\t\t\t8192\t1\n"
    "manifest.txt\twin32\tmanifest.txt\t1000\t\t\t8192\t1\n"
    "win32_local.txt\twin32_local\twin32_local.txt\t1000\t\t\t8192\t1\n"
    "manifest_local.txt\twin32_local\tmanifest_local.txt\t1000\t\t\t8192\t1\n"
    "dotnet.txt\tdotnet\tdotnet.txt\t1000\t\t\t8192\t1\n"
    "dotnet_local.txt\tdotnet_local\tdotnet_local.txt\t1000\t\t\t8192\t1\n"
    "application_win32.txt\twin32_local\tapplication_win32.txt\t1000\t\t\t8192\t1\n"
    "application_dotnet.txt\tdotnet_local\tapplication_dotnet.txt\t1000\t\t\t8192\t1\n";

static const char pa_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "assembly\t\t\tassembly feature\t1\t2\tMSITESTDIR\t0\n";

static const char pa_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "assembly\tfake_local\n"
    "assembly\twin32\n"
    "assembly\twin32_local\n"
    "assembly\tdotnet\n"
    "assembly\tdotnet_local\n";

static const char pa_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "fake_local\t{F515549D-7E61-425D-AAC1-9BEF2E066D06}\tMSITESTDIR\t0\t\tfake_local.txt\n"
    "win32\t{F515549E-7E61-425D-AAC1-9BEF2E066D06}\tMSITESTDIR\t0\t\twin32.txt\n"
    "win32_local\t{D34D3FBA-6789-4E57-AD1A-1281297DC201}\tMSITESTDIR\t0\t\twin32_local.txt\n"
    "dotnet\t{8943164F-2B31-4C09-A894-493A8CBDE0A4}\tMSITESTDIR\t0\t\tdotnet.txt\n"
    "dotnet_local\t{4E8567E8-8EAE-4E36-90F1-B99D33C663F8}\tMSITESTDIR\t0\t\tdotnet_local.txt\n";

static const char pa_msi_assembly_dat[] =
    "Component_\tFeature_\tFile_Manifest\tFile_Application\tAttributes\n"
    "s72\ts38\tS72\tS72\tI2\n"
    "MsiAssembly\tComponent_\n"
    "fake_local\tassembly\t\tnonexistent.txt\t0\n"
    "win32\tassembly\tmanifest.txt\t\t1\n"
    "win32_local\tassembly\tmanifest_local.txt\tapplication_win32.txt\t1\n"
    "dotnet\tassembly\t\t\t0\n"
    "dotnet_local\tassembly\t\tapplication_dotnet.txt\t0\n";

static const char pa_msi_assembly_name_dat[] =
    "Component_\tName\tValue\n"
    "s72\ts255\ts255\n"
    "MsiAssemblyName\tComponent_\tName\n"
    "fake_local\tName\tWine.Fake.Application.Assembly\n"
    "fake_local\tprocessorArchitecture\tx86\n"
    "fake_local\tpublicKeyToken\tabcdef0123456789\n"
    "fake_local\ttype\twin32\n"
    "fake_local\tversion\t1.0.0.0\n"
    "win32\tName\tWine.Win32.Assembly\n"
    "win32\tprocessorArchitecture\tx86\n"
    "win32\tpublicKeyToken\tabcdef0123456789\n"
    "win32\ttype\twin32\n"
    "win32\tversion\t1.0.0.0\n"
    "win32_local\tName\tWine.Win32.Local.Assembly\n"
    "win32_local\tprocessorArchitecture\tx86\n"
    "win32_local\tpublicKeyToken\tabcdef0123456789\n"
    "win32_local\ttype\twin32\n"
    "win32_local\tversion\t1.0.0.0\n"
    "dotnet\tName\tWine.Dotnet.Assembly\n"
    "dotnet\tprocessorArchitecture\tMSIL\n"
    "dotnet\tpublicKeyToken\tabcdef0123456789\n"
    "dotnet\tculture\tneutral\n"
    "dotnet\tversion\t1.0.0.0\n"
    "dotnet_local\tName\tWine.Dotnet.Local.Assembly\n"
    "dotnet_local\tprocessorArchitecture\tMSIL\n"
    "dotnet_local\tpublicKeyToken\tabcdef0123456789\n"
    "dotnet_local\tculture\tneutral\n"
    "dotnet_local\tversion\t1.0.0.0\n";

static const char pa_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "LaunchConditions\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "MsiPublishAssemblies\t\t3000\n"
    "pa_immediate\tNOT REMOVE AND NOT ALLUSERS\t3001\n"
    "pa_deferred\tNOT REMOVE AND NOT ALLUSERS\t3002\n"
    "MsiUnpublishAssemblies\t\t4000\n"
    "ua_immediate\tREMOVE AND NOT ALLUSERS\t4001\n"
    "ua_deferred\tREMOVE AND NOT ALLUSERS\t4002\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const char pa_custom_action_dat[] =
    "Action\tType\tSource\tTarget\n"
    "s72\ti2\tS64\tS0\n"
    "CustomAction\tAction\n"
    "pa_immediate\t1\tcustom.dll\tpa_absent\n"
    "pa_deferred\t1025\tcustom.dll\tpa_present\n"
    "ua_immediate\t1\tcustom.dll\tpa_present\n"
    "ua_deferred\t1025\tcustom.dll\tpa_absent\n";

static const char rep_file_dat[] =
    "File\tComponent_\tFileName\tFileSize\tVersion\tLanguage\tAttributes\tSequence\n"
    "s72\ts72\tl255\ti4\tS72\tS20\tI2\ti2\n"
    "File\tFile\n"
    "rep.txt\trep\trep.txt\t1000\t\t\t8192\t1\n";

static const char rep_feature_dat[] =
    "Feature\tFeature_Parent\tTitle\tDescription\tDisplay\tLevel\tDirectory_\tAttributes\n"
    "s38\tS38\tL64\tL255\tI2\ti2\tS72\ti2\n"
    "Feature\tFeature\n"
    "rep\t\t\trep feature\t1\t2\tMSITESTDIR\t0\n";

static const char rep_feature_comp_dat[] =
    "Feature_\tComponent_\n"
    "s38\ts72\n"
    "FeatureComponents\tFeature_\tComponent_\n"
    "rep\trep\n";

static const char rep_component_dat[] =
    "Component\tComponentId\tDirectory_\tAttributes\tCondition\tKeyPath\n"
    "s72\tS38\ts72\ti2\tS255\tS72\n"
    "Component\tComponent\n"
    "rep\t{A24FAF2A-3B2E-41EF-AA78-331542E1A29D}\tMSITESTDIR\t0\t\trep.txt\n";

static const char rep_upgrade_dat[] =
    "UpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes\tRemove\tActionProperty\n"
    "s38\tS20\tS20\tS255\ti4\tS255\ts72\n"
    "Upgrade\tUpgradeCode\tVersionMin\tVersionMax\tLanguage\tAttributes\n"
    "{2967C1CC-34D4-42EE-8D96-CD6836F192BF}\t\t\t\t256\t\tPRODUCT\n";

static const char rep_property_dat[] =
    "Property\tValue\n"
    "s72\tl0\n"
    "Property\tProperty\n"
    "HASUIRUN\t0\n"
    "INSTALLLEVEL\t3\n"
    "InstallMode\tTypical\n"
    "Manufacturer\tWine\n"
    "PIDTemplate\t###-#######\n"
    "ProductCode\t{1699F0BB-0B61-4A89-AFE4-CFD60DFD76F3}\n"
    "ProductLanguage\t1033\n"
    "ProductName\tMSITEST\n"
    "ProductVersion\t1.1.1\n"
    "UpgradeCode\t{2967C1CC-34D4-42EE-8D96-CD6836F192BF}\n"
    "PRODUCT\t2F41860D-7B4C-4DA7-BED9-B64F26594C56\n"
    "MSIFASTINSTALL\t1\n";

static const char rep_install_exec_seq_dat[] =
    "Action\tCondition\tSequence\n"
    "s72\tS255\tI2\n"
    "InstallExecuteSequence\tAction\n"
    "FindRelatedProducts\t\t100\n"
    "CostInitialize\t\t800\n"
    "FileCost\t\t900\n"
    "CostFinalize\t\t1000\n"
    "InstallValidate\t\t1400\n"
    "RemoveExistingProducts\t\t1499\n"
    "InstallInitialize\t\t1500\n"
    "ProcessComponents\t\t1600\n"
    "RemoveFiles\t\t1700\n"
    "InstallFiles\t\t2000\n"
    "UnregisterExtensionInfo\t\t3000\n"
    "UnregisterMIMEInfo\t\t3500\n"
    "RegisterExtensionInfo\t\t4000\n"
    "RegisterMIMEInfo\t\t4500\n"
    "RegisterProduct\t\t5000\n"
    "PublishFeatures\t\t5100\n"
    "PublishProduct\t\t5200\n"
    "UnpublishFeatures\t\t5300\n"
    "InstallFinalize\t\t6000\n";

static const msi_table env_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(env_install_exec_seq),
    ADD_TABLE(env_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property),
    ADD_TABLE(env_environment)
};

static const msi_table pp_tables[] =
{
    ADD_TABLE(pp_component),
    ADD_TABLE(directory),
    ADD_TABLE(rof_feature),
    ADD_TABLE(rof_feature_comp),
    ADD_TABLE(rof_file),
    ADD_TABLE(pp_install_exec_seq),
    ADD_TABLE(pp_custom_action),
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
    ADD_TABLE(pp_custom_action),
    ADD_TABLE(ppc_media),
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
    ADD_TABLE(mov_install_exec_seq),
    ADD_TABLE(mov_custom_action),
    ADD_TABLE(rof_media),
    ADD_TABLE(property),
    ADD_TABLE(mov_move_file),
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
    ADD_TABLE(wrv_feature),
    ADD_TABLE(wrv_feature_comp),
    ADD_TABLE(wrv_install_exec_seq),
    ADD_TABLE(wrv_custom_action),
    ADD_TABLE(property),
    ADD_TABLE(media),
    ADD_TABLE(wrv_registry),
    ADD_TABLE(wrv_remove_registry),
};

static const msi_table cf_tables[] =
{
    ADD_TABLE(cf_component),
    ADD_TABLE(cf_directory),
    ADD_TABLE(cf_feature),
    ADD_TABLE(cf_feature_comp),
    ADD_TABLE(cf_file),
    ADD_TABLE(cf_create_folders),
    ADD_TABLE(cf_install_exec_seq),
    ADD_TABLE(cf_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table sss_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(sss_install_exec_seq),
    ADD_TABLE(sss_service_control),
    ADD_TABLE(sss_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table sds_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(sds_install_exec_seq),
    ADD_TABLE(sds_custom_action),
    ADD_TABLE(service_control),
    ADD_TABLE(service_install),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table sis_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(sis_install_exec_seq),
    ADD_TABLE(sis_custom_action),
    ADD_TABLE(service_install2),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table sr_tables[] =
{
    ADD_TABLE(sr_component),
    ADD_TABLE(directory),
    ADD_TABLE(sr_feature),
    ADD_TABLE(sr_feature_comp),
    ADD_TABLE(sr_file),
    ADD_TABLE(sr_selfreg),
    ADD_TABLE(sr_install_exec_seq),
    ADD_TABLE(sr_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table font_tables[] =
{
    ADD_TABLE(font_component),
    ADD_TABLE(directory),
    ADD_TABLE(font_feature),
    ADD_TABLE(font_feature_comp),
    ADD_TABLE(font_file),
    ADD_TABLE(font),
    ADD_TABLE(font_install_exec_seq),
    ADD_TABLE(font_custom_action),
    ADD_TABLE(font_media),
    ADD_TABLE(property)
};

static const msi_table vp_tables[] =
{
    ADD_TABLE(component),
    ADD_TABLE(directory),
    ADD_TABLE(feature),
    ADD_TABLE(feature_comp),
    ADD_TABLE(file),
    ADD_TABLE(vp_custom_action),
    ADD_TABLE(vp_install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(vp_property)
};

static const msi_table odbc_tables[] =
{
    ADD_TABLE(odbc_component),
    ADD_TABLE(directory),
    ADD_TABLE(odbc_feature),
    ADD_TABLE(odbc_feature_comp),
    ADD_TABLE(odbc_file),
    ADD_TABLE(odbc_driver),
    ADD_TABLE(odbc_translator),
    ADD_TABLE(odbc_datasource),
    ADD_TABLE(odbc_install_exec_seq),
    ADD_TABLE(odbc_custom_action),
    ADD_TABLE(odbc_media),
    ADD_TABLE(property)
};

static const msi_table tl_tables[] =
{
    ADD_TABLE(tl_component),
    ADD_TABLE(directory),
    ADD_TABLE(tl_feature),
    ADD_TABLE(tl_feature_comp),
    ADD_TABLE(tl_file),
    ADD_TABLE(tl_typelib),
    ADD_TABLE(tl_install_exec_seq),
    ADD_TABLE(tl_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table crs_tables[] =
{
    ADD_TABLE(crs_component),
    ADD_TABLE(directory),
    ADD_TABLE(crs_feature),
    ADD_TABLE(crs_feature_comp),
    ADD_TABLE(crs_file),
    ADD_TABLE(crs_shortcut),
    ADD_TABLE(crs_install_exec_seq),
    ADD_TABLE(crs_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table pub_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(pub_component),
    ADD_TABLE(pub_feature),
    ADD_TABLE(pub_feature_comp),
    ADD_TABLE(pub_file),
    ADD_TABLE(pub_publish_component),
    ADD_TABLE(pub_install_exec_seq),
    ADD_TABLE(pub_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rd_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rd_component),
    ADD_TABLE(rd_feature),
    ADD_TABLE(rd_feature_comp),
    ADD_TABLE(rd_file),
    ADD_TABLE(rd_duplicate_file),
    ADD_TABLE(rd_install_exec_seq),
    ADD_TABLE(rd_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table frp_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(frp_component),
    ADD_TABLE(frp_feature),
    ADD_TABLE(frp_feature_comp),
    ADD_TABLE(frp_file),
    ADD_TABLE(frp_upgrade),
    ADD_TABLE(frp_custom_action),
    ADD_TABLE(frp_install_exec_seq),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table ini_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(ini_component),
    ADD_TABLE(ini_feature),
    ADD_TABLE(ini_feature_comp),
    ADD_TABLE(ini_file),
    ADD_TABLE(ini_ini_file),
    ADD_TABLE(ini_remove_ini_file),
    ADD_TABLE(ini_install_exec_seq),
    ADD_TABLE(ini_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rci_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rci_component),
    ADD_TABLE(rci_feature),
    ADD_TABLE(rci_feature_comp),
    ADD_TABLE(rci_file),
    ADD_TABLE(rci_appid),
    ADD_TABLE(rci_class),
    ADD_TABLE(rci_install_exec_seq),
    ADD_TABLE(rci_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rei_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rei_component),
    ADD_TABLE(rei_feature),
    ADD_TABLE(rei_feature_comp),
    ADD_TABLE(rei_file),
    ADD_TABLE(rei_extension),
    ADD_TABLE(rei_verb),
    ADD_TABLE(rei_progid),
    ADD_TABLE(rei_install_exec_seq),
    ADD_TABLE(rei_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rpi_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rpi_component),
    ADD_TABLE(rpi_feature),
    ADD_TABLE(rpi_feature_comp),
    ADD_TABLE(rpi_file),
    ADD_TABLE(rpi_appid),
    ADD_TABLE(rpi_class),
    ADD_TABLE(rpi_extension),
    ADD_TABLE(rpi_verb),
    ADD_TABLE(rpi_progid),
    ADD_TABLE(rpi_install_exec_seq),
    ADD_TABLE(rpi_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rmi_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rmi_component),
    ADD_TABLE(rmi_feature),
    ADD_TABLE(rmi_feature_comp),
    ADD_TABLE(rmi_file),
    ADD_TABLE(rmi_extension),
    ADD_TABLE(rmi_verb),
    ADD_TABLE(rmi_mime),
    ADD_TABLE(rmi_install_exec_seq),
    ADD_TABLE(rmi_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table pa_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(pa_component),
    ADD_TABLE(pa_feature),
    ADD_TABLE(pa_feature_comp),
    ADD_TABLE(pa_file),
    ADD_TABLE(pa_msi_assembly),
    ADD_TABLE(pa_msi_assembly_name),
    ADD_TABLE(pa_install_exec_seq),
    ADD_TABLE(pa_custom_action),
    ADD_TABLE(media),
    ADD_TABLE(property)
};

static const msi_table rep_tables[] =
{
    ADD_TABLE(directory),
    ADD_TABLE(rep_component),
    ADD_TABLE(rep_feature),
    ADD_TABLE(rep_feature_comp),
    ADD_TABLE(rep_file),
    ADD_TABLE(rep_upgrade),
    ADD_TABLE(rep_property),
    ADD_TABLE(rep_install_exec_seq),
    ADD_TABLE(media)
};

/* cabinet definitions */

/* make the max size large so there is only one cab file */
#define MEDIA_SIZE          0x7FFFFFFF

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
    create_file("msitest\\service2.exe", 100);

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
    DeleteFileA("msitest\\service2.exe");
    DeleteFileA("msitest\\filename");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest");
}

static void delete_pfmsitest_files(void)
{
    SHFILEOPSTRUCTA shfl;
    CHAR path[MAX_PATH+11];

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\msitest\\*");
    path[strlen(path) + 1] = '\0';

    shfl.hwnd = NULL;
    shfl.wFunc = FO_DELETE;
    shfl.pFrom = path;
    shfl.pTo = NULL;
    shfl.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_NORECURSION | FOF_SILENT | FOF_NOERRORUI;

    SHFileOperationA(&shfl);

    lstrcpyA(path, PROG_FILES_DIR);
    lstrcatA(path, "\\msitest");
    RemoveDirectoryA(path);
}

static void check_reg_str(HKEY prodkey, LPCSTR name, LPCSTR expected, BOOL bcase, BOOL todo, DWORD line)
{
    char val[MAX_PATH];
    DWORD size, type;
    LONG res;

    size = MAX_PATH;
    val[0] = '\0';
    res = RegQueryValueExA(prodkey, name, NULL, &type, (BYTE *)val, &size);
    ok_(__FILE__, line)(!res, "Failed to query value, error %ld\n", res);
    ok_(__FILE__, line)(type == REG_SZ || type == REG_EXPAND_SZ, "Got wrong type %lu\n", type);
    todo_wine_if (todo)
    {
        if (bcase)
            ok_(__FILE__, line)(!strcmp(val, expected), "got %s\n", debugstr_a(val));
        else
            ok_(__FILE__, line)(!strcasecmp(val, expected), "got %s\n", debugstr_a(val));
    }
}

static void check_reg_multi(HKEY prodkey, const char *name, const char *expect, DWORD line)
{
    char val[MAX_PATH];
    DWORD expect_size = 0, size, type;
    const char *p;
    LONG res;

    for (p = expect; *p; p += strlen(p) + 1)
        ;
    expect_size = (p + 1) - expect;

    size = MAX_PATH;
    val[0] = '\0';
    res = RegQueryValueExA(prodkey, name, NULL, &type, (BYTE *)val, &size);
    ok_(__FILE__, line)(!res, "Failed to query value, error %ld\n", res);
    ok_(__FILE__, line)(type == REG_MULTI_SZ, "Got wrong type %lu\n", type);
    ok_(__FILE__, line)(size == expect_size, "expected size %lu, got %lu\n", expect_size, size);
    ok_(__FILE__, line)(!memcmp(val, expect, size), "got %s\n", debugstr_an(val, size));
}

static void check_reg_dword(HKEY prodkey, LPCSTR name, DWORD expected, BOOL todo, DWORD line)
{
    DWORD val, size, type;
    LONG res;

    size = sizeof(DWORD);
    res = RegQueryValueExA(prodkey, name, NULL, &type, (BYTE *)&val, &size);
    ok_(__FILE__, line)(!res, "Failed to query value, error %ld\n", res);
    ok_(__FILE__, line)(type == REG_DWORD, "Got wrong type %lu\n", type);
    todo_wine_if (todo)
        ok_(__FILE__, line)(val == expected, "Expected %lu, got %lu\n", expected, val);
}

#define CHECK_REG_STR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, TRUE, FALSE, __LINE__);

#define CHECK_DEL_REG_STR(prodkey, name, expected) \
    do { \
        check_reg_str(prodkey, name, expected, TRUE, FALSE, __LINE__); \
        RegDeleteValueA(prodkey, name); \
    } while(0)

#define CHECK_DEL_REG_STR_TODO(prodkey, name, expected) \
    do { \
        check_reg_str(prodkey, name, expected, TRUE, TRUE, __LINE__); \
        RegDeleteValueA(prodkey, name); \
    } while(0)

#define CHECK_REG_ISTR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, FALSE, FALSE, __LINE__);

#define CHECK_DEL_REG_ISTR(prodkey, name, expected) \
    do { \
        check_reg_str(prodkey, name, expected, FALSE, FALSE, __LINE__); \
        RegDeleteValueA(prodkey, name); \
    } while(0)

#define CHECK_REG_MULTI(key, name, expect) \
    check_reg_multi(key, name, expect, __LINE__);

#define CHECK_DEL_REG_MULTI(key, name, expect) \
    do { \
        check_reg_multi(key, name, expect, __LINE__); \
        RegDeleteValueA(key, name); \
    } while(0)

#define CHECK_REG_DWORD(prodkey, name, expected) \
    check_reg_dword(prodkey, name, expected, FALSE, __LINE__);

#define CHECK_REG_DWORD_TODO(prodkey, name, expected) \
    check_reg_dword(prodkey, name, expected, TRUE, __LINE__);

#define CHECK_DEL_REG_DWORD(prodkey, name, expected) \
    do { \
        check_reg_dword(prodkey, name, expected, FALSE, __LINE__); \
        RegDeleteValueA(prodkey, name); \
    } while(0)

#define CHECK_DEL_REG_DWORD_TODO(prodkey, name, expected) \
    do { \
        check_reg_dword(prodkey, name, expected, TRUE, __LINE__); \
        RegDeleteValueA(prodkey, name); \
    } while(0)

static void get_date_str(LPSTR date)
{
    SYSTEMTIME systime;

    static const char date_fmt[] = "%d%02d%02d";
    GetLocalTime(&systime);
    sprintf(date, date_fmt, systime.wYear, systime.wMonth, systime.wDay);
}

/* EstimatedSize is the size in KiB of .msi + installed files, rounded up to page size. */
static DWORD get_estimated_size(void)
{
    SYSTEM_INFO si;
    HANDLE file;
    DWORD size;

    GetSystemInfo(&si);

    file = CreateFileA(msifile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    size = ((GetFileSize(file, NULL) + si.dwPageSize - 1) / si.dwPageSize + 1);
    size = size * si.dwPageSize / 1024;
    CloseHandle(file);
    return size;
}

static void extract_resource(const char *name, const char *type, const char *path)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %lu\n", path, GetLastError());

    res = FindResourceA(NULL, name, type);
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static void test_register_product(void)
{
    UINT r;
    LONG res;
    HKEY hkey, props, usage;
    LPSTR usersid;
    char date[MAX_PATH], temp[MAX_PATH], keypath[MAX_PATH], path[MAX_PATH];
    DWORD size, type;

    static const CHAR uninstall[] = "Software\\Microsoft\\Windows\\CurrentVersion"
                                    "\\Uninstall\\{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";
    static const CHAR userdata[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer"
                                   "\\UserData\\%s\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const CHAR ugkey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer"
                                "\\UpgradeCodes\\51AAE0C44620A5E4788506E91F249BD2";
    static const CHAR userugkey[] = "Software\\Microsoft\\Installer\\UpgradeCodes"
                                    "\\51AAE0C44620A5E4788506E91F249BD2";

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    if (!(usersid = get_user_sid()))
        return;

    get_date_str(date);
    GetTempPathA(MAX_PATH, temp);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* RegisterProduct */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyA(HKEY_CURRENT_USER, userugkey, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, uninstall, 0, KEY_READ | KEY_WOW64_32KEY, &hkey);
    ok(!res, "got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(hkey, "InstallDate", date);
    CHECK_DEL_REG_STR(hkey, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(hkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "Publisher", "Wine");
    CHECK_DEL_REG_STR(hkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "AuthorizedCDFPrefix", "");
    CHECK_DEL_REG_STR(hkey, "Comments", "");
    CHECK_DEL_REG_STR(hkey, "Contact", "");
    CHECK_DEL_REG_STR(hkey, "HelpLink", "");
    CHECK_DEL_REG_STR(hkey, "HelpTelephone", "");
    CHECK_DEL_REG_STR(hkey, "InstallLocation", "");
    CHECK_DEL_REG_DWORD(hkey, "NoModify", 1);
    CHECK_DEL_REG_STR(hkey, "Readme", "");
    CHECK_DEL_REG_STR(hkey, "Size", "");
    CHECK_DEL_REG_STR(hkey, "URLInfoAbout", "");
    CHECK_DEL_REG_STR(hkey, "URLUpdateInfo", "");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(hkey, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(hkey, "WindowsInstaller", 1);
    CHECK_DEL_REG_DWORD_TODO(hkey, "EstimatedSize", get_estimated_size());

    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    sprintf(keypath, userdata, usersid);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "InstallProperties", 0, KEY_READ, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = sizeof(path);
    RegQueryValueExA(props, "LocalPackage", NULL, &type, (LPBYTE)path, &size);
    DeleteFileA(path);
    RegDeleteValueA(props, "LocalPackage"); /* LocalPackage is nondeterministic */

    CHECK_DEL_REG_STR(props, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(props, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(props, "InstallDate", date);
    CHECK_DEL_REG_STR(props, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(props, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "Publisher", "Wine");
    CHECK_DEL_REG_STR(props, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "AuthorizedCDFPrefix", "");
    CHECK_DEL_REG_STR(props, "Comments", "");
    CHECK_DEL_REG_STR(props, "Contact", "");
    CHECK_DEL_REG_STR(props, "HelpLink", "");
    CHECK_DEL_REG_STR(props, "HelpTelephone", "");
    CHECK_DEL_REG_STR(props, "InstallLocation", "");
    CHECK_DEL_REG_DWORD(props, "NoModify", 1);
    CHECK_DEL_REG_STR(props, "Readme", "");
    CHECK_DEL_REG_STR(props, "Size", "");
    CHECK_DEL_REG_STR(props, "URLInfoAbout", "");
    CHECK_DEL_REG_STR(props, "URLUpdateInfo", "");
    CHECK_DEL_REG_DWORD(props, "Language", 1033);
    CHECK_DEL_REG_DWORD(props, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(props, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(props, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(props, "WindowsInstaller", 1);
    CHECK_DEL_REG_DWORD_TODO(props, "EstimatedSize", get_estimated_size());

    res = RegDeleteKeyA(props, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(props);

    res = RegOpenKeyExA(hkey, "Usage", 0, KEY_READ, &usage);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    res = RegDeleteKeyA(usage, "");
    todo_wine
    ok(!res, "got %ld\n", res);
    RegCloseKey(usage);
    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ugkey, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", "");

    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    /* RegisterProduct, machine */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, userugkey, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, uninstall, 0, KEY_READ | KEY_WOW64_32KEY, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(hkey, "InstallDate", date);
    CHECK_DEL_REG_STR(hkey, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(hkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "Publisher", "Wine");
    CHECK_DEL_REG_STR(hkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(hkey, "AuthorizedCDFPrefix", "");
    CHECK_DEL_REG_STR(hkey, "Comments", "");
    CHECK_DEL_REG_STR(hkey, "Contact", "");
    CHECK_DEL_REG_STR(hkey, "HelpLink", "");
    CHECK_DEL_REG_STR(hkey, "HelpTelephone", "");
    CHECK_DEL_REG_STR(hkey, "InstallLocation", "");
    CHECK_DEL_REG_DWORD(hkey, "NoModify", 1);
    CHECK_DEL_REG_STR(hkey, "Readme", "");
    CHECK_DEL_REG_STR(hkey, "Size", "");
    CHECK_DEL_REG_STR(hkey, "URLInfoAbout", "");
    CHECK_DEL_REG_STR(hkey, "URLUpdateInfo", "");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(hkey, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(hkey, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(hkey, "WindowsInstaller", 1);
    CHECK_DEL_REG_DWORD_TODO(hkey, "EstimatedSize", get_estimated_size());

    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    sprintf(keypath, userdata, "S-1-5-18");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "InstallProperties", 0, KEY_READ, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = sizeof(path);
    RegQueryValueExA(props, "LocalPackage", NULL, &type, (LPBYTE)path, &size);
    DeleteFileA(path);
    RegDeleteValueA(props, "LocalPackage"); /* LocalPackage is nondeterministic */

    CHECK_DEL_REG_STR(props, "DisplayName", "MSITEST");
    CHECK_DEL_REG_STR(props, "DisplayVersion", "1.1.1");
    CHECK_DEL_REG_STR(props, "InstallDate", date);
    CHECK_DEL_REG_STR(props, "InstallSource", temp);
    CHECK_DEL_REG_ISTR(props, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "Publisher", "Wine");
    CHECK_DEL_REG_STR(props, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_DEL_REG_STR(props, "AuthorizedCDFPrefix", "");
    CHECK_DEL_REG_STR(props, "Comments", "");
    CHECK_DEL_REG_STR(props, "Contact", "");
    CHECK_DEL_REG_STR(props, "HelpLink", "");
    CHECK_DEL_REG_STR(props, "HelpTelephone", "");
    CHECK_DEL_REG_STR(props, "InstallLocation", "");
    CHECK_DEL_REG_DWORD(props, "NoModify", 1);
    CHECK_DEL_REG_STR(props, "Readme", "");
    CHECK_DEL_REG_STR(props, "Size", "");
    CHECK_DEL_REG_STR(props, "URLInfoAbout", "");
    CHECK_DEL_REG_STR(props, "URLUpdateInfo", "");
    CHECK_DEL_REG_DWORD(props, "Language", 1033);
    CHECK_DEL_REG_DWORD(props, "Version", 0x1010001);
    CHECK_DEL_REG_DWORD(props, "VersionMajor", 1);
    CHECK_DEL_REG_DWORD(props, "VersionMinor", 1);
    CHECK_DEL_REG_DWORD(props, "WindowsInstaller", 1);
    CHECK_DEL_REG_DWORD_TODO(props, "EstimatedSize", get_estimated_size());

    res = RegDeleteKeyA(props, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(props);

    res = RegOpenKeyExA(hkey, "Usage", 0, KEY_READ, &usage);
    todo_wine
    {
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    res = RegDeleteKeyA(usage, "");
    todo_wine
    ok(!res, "got %ld\n", res);
    RegCloseKey(usage);
    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ugkey, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", "");

    res = RegDeleteKeyA(hkey, "");
    ok(!res, "got %ld\n", res);
    RegCloseKey(hkey);

    if (is_wow64 || is_64bit)
    {
        DeleteFileA(msifile);
        create_database_template(msifile, pp_tables, ARRAY_SIZE(pp_tables), 200, "x64;1033");

        r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1 ALLUSERS=1 RP_TEST64=1");
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
        ok(delete_pf("msitest", FALSE), "Directory not created\n");

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, userugkey, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, uninstall, 0, KEY_READ | KEY_WOW64_32KEY, &hkey);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_SUCCESS, got %ld\n", res);

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, uninstall, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

        CHECK_DEL_REG_STR(hkey, "DisplayName", "MSITEST");
        CHECK_DEL_REG_STR(hkey, "DisplayVersion", "1.1.1");
        CHECK_DEL_REG_STR(hkey, "InstallDate", date);
        CHECK_DEL_REG_STR(hkey, "InstallSource", temp);
        CHECK_DEL_REG_ISTR(hkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
        CHECK_DEL_REG_STR(hkey, "Publisher", "Wine");
        CHECK_DEL_REG_STR(hkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
        CHECK_DEL_REG_STR(hkey, "AuthorizedCDFPrefix", "");
        CHECK_DEL_REG_STR(hkey, "Comments", "");
        CHECK_DEL_REG_STR(hkey, "Contact", "");
        CHECK_DEL_REG_STR(hkey, "HelpLink", "");
        CHECK_DEL_REG_STR(hkey, "HelpTelephone", "");
        CHECK_DEL_REG_STR(hkey, "InstallLocation", "");
        CHECK_DEL_REG_DWORD(hkey, "NoModify", 1);
        CHECK_DEL_REG_STR(hkey, "Readme", "");
        CHECK_DEL_REG_STR(hkey, "Size", "");
        CHECK_DEL_REG_STR(hkey, "URLInfoAbout", "");
        CHECK_DEL_REG_STR(hkey, "URLUpdateInfo", "");
        CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
        CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
        CHECK_DEL_REG_DWORD(hkey, "VersionMajor", 1);
        CHECK_DEL_REG_DWORD(hkey, "VersionMinor", 1);
        CHECK_DEL_REG_DWORD(hkey, "WindowsInstaller", 1);
        CHECK_DEL_REG_DWORD_TODO(hkey, "EstimatedSize", get_estimated_size());

        res = RegDeleteKeyA(hkey, "");
        ok(!res, "got %ld\n", res);
        RegCloseKey(hkey);

        sprintf(keypath, userdata, "S-1-5-18");
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

        res = RegOpenKeyExA(hkey, "InstallProperties", 0, KEY_READ, &props);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

        size = sizeof(path);
        RegQueryValueExA(props, "LocalPackage", NULL, &type, (LPBYTE)path, &size);
        DeleteFileA(path);
        RegDeleteValueA(props, "LocalPackage"); /* LocalPackage is nondeterministic */

        CHECK_DEL_REG_STR(props, "DisplayName", "MSITEST");
        CHECK_DEL_REG_STR(props, "DisplayVersion", "1.1.1");
        CHECK_DEL_REG_STR(props, "InstallDate", date);
        CHECK_DEL_REG_STR(props, "InstallSource", temp);
        CHECK_DEL_REG_ISTR(props, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
        CHECK_DEL_REG_STR(props, "Publisher", "Wine");
        CHECK_DEL_REG_STR(props, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
        CHECK_DEL_REG_STR(props, "AuthorizedCDFPrefix", "");
        CHECK_DEL_REG_STR(props, "Comments", "");
        CHECK_DEL_REG_STR(props, "Contact", "");
        CHECK_DEL_REG_STR(props, "HelpLink", "");
        CHECK_DEL_REG_STR(props, "HelpTelephone", "");
        CHECK_DEL_REG_STR(props, "InstallLocation", "");
        CHECK_DEL_REG_DWORD(props, "NoModify", 1);
        CHECK_DEL_REG_STR(props, "Readme", "");
        CHECK_DEL_REG_STR(props, "Size", "");
        CHECK_DEL_REG_STR(props, "URLInfoAbout", "");
        CHECK_DEL_REG_STR(props, "URLUpdateInfo", "");
        CHECK_DEL_REG_DWORD(props, "Language", 1033);
        CHECK_DEL_REG_DWORD(props, "Version", 0x1010001);
        CHECK_DEL_REG_DWORD(props, "VersionMajor", 1);
        CHECK_DEL_REG_DWORD(props, "VersionMinor", 1);
        CHECK_DEL_REG_DWORD(props, "WindowsInstaller", 1);
        CHECK_DEL_REG_DWORD_TODO(props, "EstimatedSize", get_estimated_size());

        res = RegDeleteKeyA(props, "");
        ok(!res, "got %ld\n", res);
        RegCloseKey(props);

        res = RegOpenKeyExA(hkey, "Usage", 0, KEY_READ, &usage);
        todo_wine
        {
            ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
        }

        res = RegDeleteKeyA(usage, "");
        todo_wine
        ok(!res, "got %ld\n", res);
        RegCloseKey(usage);
        res = RegDeleteKeyA(hkey, "");
        ok(!res, "got %ld\n", res);
        RegCloseKey(hkey);

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ugkey, 0, KEY_READ | KEY_WOW64_64KEY, &hkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

        CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", "");

        res = RegDeleteKeyA(hkey, "");
        ok(!res, "got %ld\n", res);
        RegCloseKey(hkey);
    }
    else
        skip("64-bit RegisterProduct tests\n");

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    LocalFree(usersid);
}

static void test_publish_product(void)
{
    static const char prodpath[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products"
        "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char cuprodpath[] =
        "Software\\Microsoft\\Installer\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char cuupgrades[] =
        "Software\\Microsoft\\Installer\\UpgradeCodes\\51AAE0C44620A5E4788506E91F249BD2";
    static const char badprod[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Products"
        "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char machprod[] =
        "Software\\Classes\\Installer\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char machup[] =
        "Software\\Classes\\Installer\\UpgradeCodes\\51AAE0C44620A5E4788506E91F249BD2";
    UINT r;
    LONG res;
    LPSTR usersid;
    HKEY sourcelist, net, props;
    HKEY hkey, patches, media;
    CHAR keypath[MAX_PATH];
    CHAR temp[MAX_PATH];
    CHAR path[MAX_PATH];
    BOOL old_installer = FALSE;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    if (!(usersid = get_user_sid()))
        return;

    GetTempPathA(MAX_PATH, temp);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* PublishProduct, current user */
    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, badprod, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    sprintf(keypath, prodpath, usersid);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey);
    if (res == ERROR_FILE_NOT_FOUND)
    {
        res = RegOpenKeyA(HKEY_CURRENT_USER, cuprodpath, &hkey);
        if (res == ERROR_SUCCESS)
        {
            win_skip("Windows Installer < 3.0 detected\n");
            RegCloseKey(hkey);
            old_installer = TRUE;
            goto currentuser;
        }
        else
        {
            win_skip("Install failed, no need to continue\n");
            return;
        }
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "InstallProperties", 0, access, &props);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "Patches", 0, access, &patches);
    todo_wine ok(!res, "Expected ERROR_SUCCESS, got %ld\n", res);
    if (!res)
        CHECK_DEL_REG_STR(patches, "AllPatches", "");

    RegDeleteKeyExA(patches, "", access, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);

currentuser:
    res = RegOpenKeyA(HKEY_CURRENT_USER, cuprodpath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "ProductName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "PackageCode", "AC75740029052C94DA02821EECD05F2F");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    if (!old_installer)
        CHECK_DEL_REG_DWORD(hkey, "AuthorizedLUAApp", 0);
    CHECK_DEL_REG_DWORD(hkey, "Assignment", 0);
    CHECK_DEL_REG_DWORD(hkey, "AdvertiseFlags", 0x184);
    CHECK_DEL_REG_DWORD(hkey, "InstanceType", 0);
    CHECK_DEL_REG_MULTI(hkey, "Clients", ":\0");

    res = RegOpenKeyA(hkey, "SourceList", &sourcelist);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(path, "n;1;");
    lstrcatA(path, temp);
    CHECK_DEL_REG_STR(sourcelist, "LastUsedSource", path);
    CHECK_DEL_REG_STR(sourcelist, "PackageName", "msitest.msi");

    res = RegOpenKeyA(sourcelist, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(net, "1", temp);

    RegDeleteKeyA(net, "");
    RegCloseKey(net);

    res = RegOpenKeyA(sourcelist, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(media, "1", "DISK1;");

    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(sourcelist, "");
    RegCloseKey(sourcelist);
    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cuupgrades, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", "");

    RegDeleteKeyA(hkey, "");
    RegCloseKey(hkey);

    /* PublishProduct, machine */
    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1 ALLUSERS=1");
    if (old_installer)
        goto machprod;
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, badprod, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    sprintf(keypath, prodpath, "S-1-5-18");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "InstallProperties", 0, access, &props);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyExA(hkey, "Patches", 0, access, &patches);
    todo_wine ok(!res, "Expected ERROR_SUCCESS, got %ld\n", res);
    if (!res)
        CHECK_DEL_REG_STR(patches, "AllPatches", "");

    RegDeleteKeyExA(patches, "", access, 0);
    RegCloseKey(patches);
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);

machprod:
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, machprod, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "ProductName", "MSITEST");
    CHECK_DEL_REG_STR(hkey, "PackageCode", "AC75740029052C94DA02821EECD05F2F");
    CHECK_DEL_REG_DWORD(hkey, "Language", 1033);
    CHECK_DEL_REG_DWORD(hkey, "Version", 0x1010001);
    if (!old_installer)
        CHECK_DEL_REG_DWORD(hkey, "AuthorizedLUAApp", 0);
    CHECK_DEL_REG_DWORD_TODO(hkey, "Assignment", 1);
    CHECK_DEL_REG_DWORD(hkey, "AdvertiseFlags", 0x184);
    CHECK_DEL_REG_DWORD(hkey, "InstanceType", 0);
    CHECK_DEL_REG_MULTI(hkey, "Clients", ":\0");

    res = RegOpenKeyExA(hkey, "SourceList", 0, access, &sourcelist);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(path, "n;1;");
    lstrcatA(path, temp);
    CHECK_DEL_REG_STR(sourcelist, "LastUsedSource", path);
    CHECK_DEL_REG_STR(sourcelist, "PackageName", "msitest.msi");

    res = RegOpenKeyExA(sourcelist, "Net", 0, access, &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(net, "1", temp);

    res = RegDeleteKeyExA(net, "", access, 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(net);

    res = RegOpenKeyExA(sourcelist, "Media", 0, access, &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(media, "1", "DISK1;");

    res = RegDeleteKeyExA(media, "", access, 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(media);
    res = RegDeleteKeyExA(sourcelist, "", access, 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(sourcelist);
    res = RegDeleteKeyExA(hkey, "", access, 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(hkey);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, machup, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_DEL_REG_STR(hkey, "84A88FD7F6998CE40A22FB59F6B9C2BB", "");

    res = RegDeleteKeyExA(hkey, "", access, 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(hkey);

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    LocalFree(usersid);
}

static void test_publish_features(void)
{
    static const char cupath[] =
        "Software\\Microsoft\\Installer\\Features\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char udfeatpath[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products"
        "\\84A88FD7F6998CE40A22FB59F6B9C2BB\\Features";
    static const char udpridpath[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Products"
        "\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    static const char featkey[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Features";
    static const char classfeat[] =
        "Software\\Classes\\Installer\\Features\\84A88FD7F6998CE40A22FB59F6B9C2BB";
    UINT r;
    LONG res;
    HKEY hkey;
    LPSTR usersid;
    CHAR keypath[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    if (!(usersid = get_user_sid()))
        return;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);

    /* PublishFeatures, current user */
    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, featkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, classfeat, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cupath, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(hkey, "feature", "");
    CHECK_REG_STR(hkey, "montecristo", "");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);

    sprintf(keypath, udfeatpath, usersid);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(hkey, "feature", "VGtfp^p+,?82@JU1j_KE");
    CHECK_REG_STR(hkey, "montecristo", "VGtfp^p+,?82@JU1j_KE");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);
    sprintf(keypath, udpridpath, usersid);
    RegDeleteKeyExA(HKEY_LOCAL_MACHINE, keypath, access, 0);

    /* PublishFeatures, machine */
    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, featkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyA(HKEY_CURRENT_USER, cupath, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, classfeat, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(hkey, "feature", "");
    CHECK_REG_STR(hkey, "montecristo", "");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);

    sprintf(keypath, udfeatpath, "S-1-5-18");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(hkey, "feature", "VGtfp^p+,?82@JU1j_KE");
    CHECK_REG_STR(hkey, "montecristo", "VGtfp^p+,?82@JU1j_KE");

    RegDeleteValueA(hkey, "feature");
    RegDeleteValueA(hkey, "montecristo");
    RegDeleteKeyExA(hkey, "", access, 0);
    RegCloseKey(hkey);
    sprintf(keypath, udpridpath, "S-1-5-18");
    RegDeleteKeyExA(HKEY_LOCAL_MACHINE, keypath, access, 0);

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    LocalFree(usersid);
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
    val = malloc(len);
    if (!val) return NULL;
    val[0] = 0;
    RegQueryValueExA(hkey, name, NULL, NULL, (LPBYTE)val, &len);
    return val;
}

static void get_owner_company(LPSTR *owner, LPSTR *company)
{
    LONG res;
    HKEY hkey;
    REGSAM access = KEY_ALL_ACCESS;

    *owner = *company = NULL;

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

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
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            "Software\\Microsoft\\Windows NT\\CurrentVersion", 0, access, &hkey);
        if (res == ERROR_SUCCESS)
        {
            *owner = reg_get_val_str(hkey, "RegisteredOwner");
            *company = reg_get_val_str(hkey, "RegisteredOrganization");
            RegCloseKey(hkey);
        }
    }

    if (!*owner || !*company)
    {
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            "Software\\Microsoft\\Windows\\CurrentVersion", 0, access, &hkey);
        if (res == ERROR_SUCCESS)
        {
            *owner = reg_get_val_str(hkey, "RegisteredOwner");
            *company = reg_get_val_str(hkey, "RegisteredOrganization");
            RegCloseKey(hkey);
        }
    }
}

static void test_register_user(void)
{
    UINT r;
    LONG res;
    HKEY props;
    LPSTR usersid;
    LPSTR owner, company;
    CHAR keypath[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;

    static const CHAR keypropsfmt[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\"
        "UserData\\%s\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB\\InstallProperties";
    static const CHAR keypridfmt[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\"
        "UserData\\%s\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    if (!(usersid = get_user_sid()))
        return;

    get_owner_company(&owner, &company);

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* RegisterUser, per-user */
    r = MsiInstallProductA(msifile, "REGISTER_USER=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    sprintf(keypath, keypropsfmt, usersid);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(props, "ProductID", "none");
    CHECK_REG_STR(props, "RegCompany", company);
    CHECK_REG_STR(props, "RegOwner", owner);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyExA(props, "", access, 0);
    RegCloseKey(props);
    sprintf(keypath, keypridfmt, usersid);
    RegDeleteKeyExA(HKEY_LOCAL_MACHINE, keypath, access, 0);

    /* RegisterUser, machine */
    r = MsiInstallProductA(msifile, "REGISTER_USER=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    sprintf(keypath, keypropsfmt, "S-1-5-18");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    CHECK_REG_STR(props, "ProductID", "none");
    CHECK_REG_STR(props, "RegCompany", company);
    CHECK_REG_STR(props, "RegOwner", owner);

    RegDeleteValueA(props, "ProductID");
    RegDeleteValueA(props, "RegCompany");
    RegDeleteValueA(props, "RegOwner");
    RegDeleteKeyExA(props, "", access, 0);
    RegCloseKey(props);
    sprintf(keypath, keypridfmt, "S-1-5-18");
    RegDeleteKeyExA(HKEY_LOCAL_MACHINE, keypath, access, 0);

error:
    free(company);
    free(owner);

    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    LocalFree(usersid);
}

static void test_process_components(void)
{
    static const char keyfmt[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\%s\\Components\\%s";
    static const char compkey[] =
        "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Components";
    UINT r;
    LONG res;
    DWORD size;
    HKEY comp, hkey;
    LPSTR usersid;
    CHAR val[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR program_files_maximus[MAX_PATH];
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    if (!(usersid = get_user_sid()))
        return;

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, ppc_tables, ARRAY_SIZE(ppc_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* ProcessComponents, per-user */
    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1");
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    sprintf(keypath, keyfmt, usersid, "CBABC2FDCCB35E749A8944D8C1C098B5");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    lstrcpyA(program_files_maximus,PROG_FILES_DIR);
    lstrcatA(program_files_maximus,"\\msitest\\maximus");

    ok(!lstrcmpiA(val, program_files_maximus),
       "Expected \"%s\", got \"%s\"\n", program_files_maximus, val);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, compkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyExA(comp, "", access, 0);
    RegCloseKey(comp);

    sprintf(keypath, keyfmt, usersid, "241C3DA58FECD0945B9687D408766058");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(val, "01\\msitest\\augustus"),
       "Expected \"01\\msitest\\augustus\", got \"%s\"\n", val);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, compkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyExA(comp, "", access, 0);
    RegCloseKey(comp);

    /* ProcessComponents, machine */
    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1 ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    sprintf(keypath, keyfmt, "S-1-5-18", "CBABC2FDCCB35E749A8944D8C1C098B5");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpiA(val, program_files_maximus),
       "Expected \"%s\", got \"%s\"\n", program_files_maximus, val);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, compkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyExA(comp, "", access, 0);
    RegCloseKey(comp);

    sprintf(keypath, keyfmt, "S-1-5-18", "241C3DA58FECD0945B9687D408766058");
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keypath, 0, access, &comp);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB",
                           NULL, NULL, (LPBYTE)val, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    ok(!lstrcmpA(val, "01\\msitest\\augustus"),
       "Expected \"01\\msitest\\augustus\", got \"%s\"\n", val);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, compkey, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    RegDeleteValueA(comp, "84A88FD7F6998CE40A22FB59F6B9C2BB");
    RegDeleteKeyExA(comp, "", access, 0);
    RegCloseKey(comp);

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    LocalFree(usersid);
}

static void test_publish(void)
{
    static const char subkey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    static const char subkey_32node[] = "Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    UINT r;
    LONG res;
    HKEY uninstall, prodkey, uninstall_32node = NULL;
    INSTALLSTATE state;
    char date[MAX_PATH], date2[MAX_PATH], temp[MAX_PATH], buf[MAX_PATH];
    const char prodcode[] = "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";
    REGSAM access = KEY_ALL_ACCESS;
    DWORD error, type, size;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    get_date_str(date);
    GetTempPathA(MAX_PATH, temp);

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, KEY_ALL_ACCESS, &uninstall);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    if (is_64bit)
    {
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey_32node, 0, KEY_ALL_ACCESS, &uninstall_32node);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r);
    ok(state == 0xdead, "got %d\n", state);
    ok(error == 0xdeadbeef, "got %lu\n", error);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r);
    ok(state == 0xdead, "got %d\n", state);
    ok(error == ERROR_SUCCESS, "got %lu\n", error);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r);
    ok(state == 0xdead, "got %d\n", state);
    ok(error == ERROR_SUCCESS, "got %lu\n", error);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* nothing published */
    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* PublishProduct and RegisterProduct */
    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1 PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
        if (is_wow64 && res == ERROR_FILE_NOT_FOUND) /* XP - Vista, Wow64 */
            res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File deleted\n");
    ok(pf_exists("msitest"), "File deleted\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r);
    ok(state == 0xdead, "got %d\n", state);
    ok(error == 0xdeadbeef, "got %lu\n", error);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_UNKNOWN_PRODUCT, "got %u\n", r);
    ok(state == 0xdead, "got %d\n", state);
    ok(error == ERROR_SUCCESS, "got %lu\n", error);

    state = 0xdead;
    SetLastError(0xdeadbeef);
    r = MsiQueryFeatureStateExA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, "feature", &state);
    error = GetLastError();
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(state == INSTALLSTATE_LOCAL, "got %d\n", state);
    ok(error == ERROR_SUCCESS, "got %lu\n", error);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    /* no UnpublishFeatures */
    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    ok(!pf_exists("msitest"), "Directory not deleted\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    /* UnpublishFeatures, only feature removed.  Only works when entire product is removed */
    r = MsiInstallProductA(msifile, "UNPUBLISH_FEATURES=1 REMOVE=feature");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File deleted\n");
    ok(pf_exists("msitest"), "Directory deleted\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");
    CHECK_REG_STR(prodkey, "InstallDate", date);
    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    /* UnpublishFeatures, both features removed */
    r = MsiInstallProductA(msifile, "UNPUBLISH_FEATURES=1 REMOVE=feature,montecristo");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    ok(!pf_exists("msitest"), "Directory not deleted\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* complete install */
    r = MsiInstallProductA(msifile, "FULL=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    if (is_64bit)
    {
        res = RegOpenKeyExA(uninstall_32node, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }
    else
    {
        res = RegOpenKeyExA(uninstall, prodcode, 0, KEY_ALL_ACCESS, &prodkey);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    }

    CHECK_REG_STR(prodkey, "DisplayName", "MSITEST");
    CHECK_REG_STR(prodkey, "DisplayVersion", "1.1.1");

    get_date_str(date2);
    size = ARRAY_SIZE(buf);
    buf[0] = '\0';
    res = RegQueryValueExA(prodkey, "InstallDate", NULL, &type, (BYTE *)buf, &size);
    ok(!res, "Failed to query value, error %ld\n", res);
    ok(type == REG_SZ, "Got wrong type %lu\n", type);
    ok(!strcmp(buf, date) || !strcmp(buf, date2), "got %s\n", debugstr_a(buf));

    CHECK_REG_STR(prodkey, "InstallSource", temp);
    CHECK_REG_ISTR(prodkey, "ModifyPath", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "Publisher", "Wine");
    CHECK_REG_STR(prodkey, "UninstallString", "MsiExec.exe /X{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}");
    CHECK_REG_STR(prodkey, "AuthorizedCDFPrefix", "");
    CHECK_REG_STR(prodkey, "Comments", "");
    CHECK_REG_STR(prodkey, "Contact", "");
    CHECK_REG_STR(prodkey, "HelpLink", "");
    CHECK_REG_STR(prodkey, "HelpTelephone", "");
    CHECK_REG_STR(prodkey, "InstallLocation", "");
    CHECK_REG_DWORD(prodkey, "NoModify", 1);
    CHECK_REG_STR(prodkey, "Readme", "");
    CHECK_REG_STR(prodkey, "Size", "");
    CHECK_REG_STR(prodkey, "URLInfoAbout", "");
    CHECK_REG_STR(prodkey, "URLUpdateInfo", "");
    CHECK_REG_DWORD(prodkey, "Language", 1033);
    CHECK_REG_DWORD(prodkey, "Version", 0x1010001);
    CHECK_REG_DWORD(prodkey, "VersionMajor", 1);
    CHECK_REG_DWORD(prodkey, "VersionMinor", 1);
    CHECK_REG_DWORD(prodkey, "WindowsInstaller", 1);
    CHECK_REG_DWORD_TODO(prodkey, "EstimatedSize", get_estimated_size());

    RegCloseKey(prodkey);

    /* complete uninstall */
    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    ok(!pf_exists("msitest"), "Directory not deleted\n");

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    state = MsiQueryFeatureStateA(prodcode, "montecristo");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    r = MsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                "{DF2CBABC-3BCC-47E5-A998-448D1C0C895B}", &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegOpenKeyExA(uninstall, prodcode, 0, access, &prodkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    /* make sure 'Program Files\msitest' is removed */
    delete_pfmsitest_files();

error:
    RegCloseKey(uninstall);
    RegCloseKey(uninstall_32node);
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
}

static void test_publish_sourcelist(void)
{
    UINT r;
    DWORD size;
    CHAR value[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR prodcode[] = "{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);

    create_database(msifile, pp_tables, ARRAY_SIZE(pp_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* nothing published */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "REGISTER_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after RegisterProduct */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PROCESS_COMPONENTS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after ProcessComponents */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PUBLISH_FEATURES=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after PublishFeatures */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == MAX_PATH, "got %lu\n", size);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got \"%s\"\n", value);

    r = MsiInstallProductA(msifile, "PUBLISH_PRODUCT=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(pf_exists("msitest\\maximus"), "File not installed\n");
    ok(pf_exists("msitest"), "File not installed\n");

    /* after PublishProduct */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "msitest.msi"), "Expected 'msitest.msi', got %s\n", value);
    ok(size == 11, "Expected 11, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATHA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_DISKPROMPTA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %lu\n", size);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, path), "Expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "Expected %d, got %lu\n", lstrlenA(path), size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListGetInfoA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPEA, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
    ok(size == 1, "Expected 1, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, path), "Expected \"%s\", got \"%s\"\n", path, value);
    ok(size == lstrlenA(path), "Expected %d, got %lu\n", lstrlenA(path), size);

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 1, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %lu\n", size);

    /* complete uninstall */
    r = MsiInstallProductA(msifile, "FULL=1 REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!pf_exists("msitest\\maximus"), "File not deleted\n");
    ok(!pf_exists("msitest"), "Directory not deleted\n");

    /* make sure 'Program Files\msitest' is removed */
    delete_pfmsitest_files();

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
}

static void test_remove_files(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\hydrogen", 500);
    create_file("msitest\\helium", 500);
    create_file("msitest\\lithium", 500);

    create_database(msifile, rem_tables, ARRAY_SIZE(rem_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
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
    ok(delete_pf("msitest", FALSE), "Directory deleted\n");

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
    ok(delete_pf("msitest", FALSE), "Directory deleted\n");

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
    delete_pf("msitest", FALSE);

error:
    DeleteFileA(msifile);
    DeleteFileA("msitest\\hydrogen");
    DeleteFileA("msitest\\helium");
    DeleteFileA("msitest\\lithium");
    RemoveDirectoryA("msitest");
}

static void test_move_files(void)
{
    UINT r;
    char props[4 * MAX_PATH + 74];

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

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

    create_database(msifile, mov_tables, ARRAY_SIZE(mov_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* if the source or dest property is not a full path,
     * windows tries to access it as a network resource
     */

    sprintf(props, "SOURCEFULL=\"%s\\\" DESTFULL=\"%s\\msitest\" "
            "FILEPATHBAD=\"%s\\japan\" FILEPATHGOOD=\"%s\\kenya\"",
            CURR_DIR, PROG_FILES_DIR, CURR_DIR, CURR_DIR);

    r = MsiInstallProductA(msifile, props);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
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
    ok(delete_pf("msitest", FALSE), "Directory not created\n");
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

error:
    DeleteFileA("cameroon");
    DeleteFileA("djibouti");
    DeleteFileA("egypt");
    DeleteFileA("finland");
    DeleteFileA("gambai");
    DeleteFileA("honduras");
    DeleteFileA("japan");
    DeleteFileA("kenya");
    DeleteFileA("nauru");
    DeleteFileA("peru");
    DeleteFileA("apple");
    DeleteFileA("application");
    DeleteFileA("ape");
    DeleteFileA("foo");
    DeleteFileA("fao");
    DeleteFileA("fbod");
    DeleteFileA("budding");
    DeleteFileA("buddy");
    DeleteFileA("bud");
    DeleteFileA("bar");
    DeleteFileA("bur");
    DeleteFileA("bird");
    DeleteFileA("msitest\\india");
    DeleteFileA("msitest\\augustus");
    RemoveDirectoryA("latvia");
    RemoveDirectoryA("msitest");
    DeleteFileA(msifile);
}

static void test_duplicate_files(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    create_file("msitest\\maximus", 500);
    create_database(msifile, df_tables, ARRAY_SIZE(df_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    /* fails if the destination folder is not a valid property */

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(delete_pf("msitest\\maximus", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\augustus", TRUE), "File not duplicated\n");
    ok(delete_pf("msitest\\this\\doesnot\\exist\\maximus", TRUE), "File not duplicated\n");
    ok(delete_pf("msitest\\this\\doesnot\\exist", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\this\\doesnot", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\this", FALSE), "Directory not created\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

error:
    DeleteFileA("msitest\\maximus");
    RemoveDirectoryA("msitest");
    DeleteFileA(msifile);
}

static void test_write_registry_values(void)
{
    HKEY hkey, subkey;
    DWORD type, size;
    char buf[20];
    LONG res;
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);

    create_database(msifile, wrv_tables, ARRAY_SIZE(wrv_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    res = RegCreateKeyA(HKEY_CURRENT_USER, "msitest", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(hkey, "Value1", 0, REG_MULTI_SZ, (const BYTE *)"two\0", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value2", 0, REG_MULTI_SZ, (const BYTE *)"one\0", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value3", 0, REG_MULTI_SZ, (const BYTE *)"two\0", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value4", 0, REG_MULTI_SZ, (const BYTE *)"one\0", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value5", 0, REG_MULTI_SZ, (const BYTE *)"one\0two\0", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value6", 0, REG_MULTI_SZ, (const BYTE *)"one\0", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    res = RegSetValueExA(hkey, "Value7", 0, REG_SZ, (const BYTE *)"one", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(hkey, "instremove", 0, REG_SZ, (const BYTE *)"val", 3);
    ok(!res, "got %ld\n", res);

    res = RegCreateKeyA(hkey, "instremove", &subkey);
    ok(!res, "got %ld\n", res);
    RegCloseKey(subkey);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    CHECK_REG_STR(hkey, "sz", "string");
    CHECK_REG_MULTI(hkey, "multisz", "one\0two\0three\0");
    CHECK_REG_DWORD(hkey, "dword", 123);

    size = sizeof(buf);
    res = RegQueryValueExA(hkey, "expandsz", NULL, &type, (BYTE *)buf, &size);
    ok(!res, "got %ld\n", res);
    ok(type == REG_EXPAND_SZ, "got %ld\n", type);
    ok(!strcmp(buf, "string"), "got %s\n", buf);

    size = sizeof(buf);
    res = RegQueryValueExA(hkey, "binary", NULL, &type, (BYTE *)buf, &size);
    ok(!res, "got %ld\n", res);
    ok(type == REG_BINARY, "got %ld\n", type);
    ok(size == 4, "got size %lu\n", size);
    ok(!memcmp(buf, "\x01\x23\x45\x67", 4), "wrong data\n");

    CHECK_REG_STR(hkey, "", "default");

    res = RegOpenKeyA(hkey, "VisualStudio", &subkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    RegCloseKey(subkey);

    CHECK_REG_MULTI(hkey, "Value1", "one\0");
    CHECK_REG_MULTI(hkey, "Value2", "one\0two\0");
    CHECK_REG_MULTI(hkey, "Value3", "one\0two\0");
    CHECK_REG_MULTI(hkey, "Value4", "one\0two\0");
    CHECK_REG_MULTI(hkey, "Value5", "one\0two\0three\0");
    CHECK_REG_MULTI(hkey, "Value6", "");
    CHECK_REG_MULTI(hkey, "Value7", "two\0");

    CHECK_REG_DWORD(hkey, "format", 1);
    CHECK_REG_DWORD(hkey, "source", 1);

    res = RegOpenKeyA(hkey, "subkey", &subkey);
    ok(!res, "got %ld\n", res);
    res = RegQueryValueExA(subkey, "", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    RegCloseKey(subkey);

    res = RegOpenKeyA(hkey, "create", &subkey);
    ok(!res, "got %ld\n", res);
    RegCloseKey(subkey);

    res = RegOpenKeyA(hkey, "delete", &subkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegCreateKeyA(hkey, "delete", &subkey);
    ok(!res, "got %ld\n", res);
    RegCloseKey(subkey);

    res = RegQueryValueExA(hkey, "instremove", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegOpenKeyA(hkey, "instremove", &subkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(!r, "got %u\n", r);

    res = RegQueryValueExA(hkey, "sz", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "multisz", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "dword", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "expandsz", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "binary", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(hkey, "VisualStudio", &subkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegQueryValueExA(hkey, "Value1", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "Value4", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "Value5", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "Value6", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "Value7", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

todo_wine {
    CHECK_REG_MULTI(hkey, "Value2", "one\0");
    CHECK_REG_MULTI(hkey, "Value3", "two\0");
}

    res = RegQueryValueExA(hkey, "format", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    res = RegQueryValueExA(hkey, "source", NULL, NULL, NULL, NULL);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(hkey, "subkey", &subkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(hkey, "create", &subkey);
    ok(!res, "got %ld\n", res);
    RegCloseKey(subkey);
    res = RegDeleteKeyA(hkey, "create");
    ok(!res, "got %ld\n", res);

    res = RegOpenKeyA(hkey, "delete", &subkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    RegCloseKey(hkey);
    res = RegDeleteKeyA(HKEY_CURRENT_USER, "msitest");
    ok(!res, "got %ld\n", res);

error:
    DeleteFileA(msifile);
    RemoveDirectoryA("msitest");
}

static void test_envvar(void)
{
    char buffer[16];
    UINT r, i;
    HKEY env, env2;
    LONG res;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_database(msifile, env_tables, ARRAY_SIZE(env_tables));

    res = RegCreateKeyExA(HKEY_CURRENT_USER, "Environment", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &env, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Session Manager\\Environment",
                          0, NULL, 0, KEY_ALL_ACCESS, NULL, &env2, NULL);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(env, "MSITESTVAR1", 0, REG_SZ, (const BYTE *)"0", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(env, "MSITESTVAR2", 0, REG_SZ, (const BYTE *)"0", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(env, "MSITESTVAR21", 0, REG_SZ, (const BYTE *)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    CHECK_REG_STR(env, "MSITESTVAR1", "1");
    CHECK_REG_STR(env, "MSITESTVAR2", "1");
    CHECK_REG_STR(env, "MSITESTVAR3", "1");
    CHECK_REG_STR(env, "MSITESTVAR4", "1");

    res = RegDeleteValueA(env, "MSITESTVAR5");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR6");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR7");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR8");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR9");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR10");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    CHECK_REG_STR(env, "MSITESTVAR11", "1;2");
    CHECK_REG_STR(env, "MSITESTVAR12", "1");
    CHECK_REG_STR(env, "MSITESTVAR13", "1;2");
    CHECK_REG_STR(env, "MSITESTVAR14", ";1;");
    CHECK_REG_STR(env, "MSITESTVAR15", ";;1;;");
    CHECK_REG_STR(env, "MSITESTVAR16", " 1 ");
    CHECK_REG_STR(env, "MSITESTVAR17", ";;2;;1");
    CHECK_REG_STR(env, "MSITESTVAR18", "1;;2;;");
    CHECK_REG_STR(env, "MSITESTVAR19", "1");
    CHECK_REG_STR(env, "MSITESTVAR20", "1");
    CHECK_REG_STR(env, "MSITESTVAR21", "1");
    CHECK_REG_STR(env, "MSITESTVAR24", "TestService");
    CHECK_REG_STR(env2, "MSITESTVAR100", "1");

    res = RegSetValueExA(env, "MSITESTVAR22", 0, REG_SZ, (const BYTE *)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegSetValueExA(env, "MSITESTVAR23", 0, REG_SZ, (const BYTE *)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    res = RegDeleteValueA(env, "MSITESTVAR25");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(!r, "got %u\n", r);

    CHECK_DEL_REG_STR(env, "MSITESTVAR13", "1;2");
    CHECK_DEL_REG_STR(env, "MSITESTVAR14", ";1;");
    CHECK_DEL_REG_STR(env, "MSITESTVAR15", ";;1;;");
    CHECK_DEL_REG_STR(env, "MSITESTVAR16", " 1 ");
    CHECK_DEL_REG_STR_TODO(env, "MSITESTVAR17", "1");
    CHECK_DEL_REG_STR_TODO(env, "MSITESTVAR18", "1");
    CHECK_DEL_REG_STR(env, "MSITESTVAR23", "1");

    for (i = 1; i <= 23; i++)
    {
        sprintf(buffer, "MSITESTVAR%i", i);
        res = RegDeleteValueA(env, buffer);
        ok(res == ERROR_FILE_NOT_FOUND, "[%d] got %ld\n", i, res);
    }

    res = RegDeleteValueA(env2, "MSITESTVAR100");
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

error:
    RegDeleteValueA(env, "MSITESTVAR1");
    RegDeleteValueA(env, "MSITESTVAR2");
    RegDeleteValueA(env, "MSITESTVAR21");
    RegCloseKey(env);
    RegCloseKey(env2);

    DeleteFileA(msifile);
}

static void test_create_remove_folder(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    CreateDirectoryA("msitest", NULL);
    CreateDirectoryA("msitest\\first", NULL);
    CreateDirectoryA("msitest\\second", NULL);
    create_file("msitest\\first\\one.txt", 1000);
    create_file("msitest\\second\\two.txt", 1000);
    create_database(msifile, cf_tables, ARRAY_SIZE(cf_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(pf_exists("msitest\\first\\one.txt"), "file not installed\n");
    ok(pf_exists("msitest\\first"), "directory not created\n");
    ok(pf_exists("msitest\\second\\two.txt"), "file not installed\n");
    ok(pf_exists("msitest\\second"), "directory not created\n");
    ok(pf_exists("msitest\\third"), "directory not created\n");
    ok(pf_exists("msitest"), "directory not created\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!pf_exists("msitest\\first\\one.txt"), "file not removed\n");
    ok(!pf_exists("msitest\\first"), "directory not removed\n");
    ok(!pf_exists("msitest\\second\\two.txt"), "file not removed\n");
    ok(!pf_exists("msitest\\second"), "directory not removed\n");
    ok(!pf_exists("msitest\\third"), "directory not removed\n");
    todo_wine ok(!pf_exists("msitest"), "directory not removed\n");

error:
    DeleteFileA("msitest\\first\\one.txt");
    DeleteFileA("msitest\\second\\two.txt");
    RemoveDirectoryA("msitest\\first");
    RemoveDirectoryA("msitest\\second");
    RemoveDirectoryA("msitest");
    DeleteFileA(msifile);
}

static void test_start_stop_services(void)
{
    UINT r;
    SC_HANDLE scm, service;
    BOOL ret;
    DWORD error = ERROR_SUCCESS;

    scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok(scm != NULL, "Failed to open the SC Manager\n");
    if (!scm) return;

    service = OpenServiceA(scm, "Spooler", SC_MANAGER_ALL_ACCESS);
    if (!service && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
    {
        win_skip("The 'Spooler' service does not exist\n");
        CloseServiceHandle(scm);
        return;
    }
    ok(service != NULL, "Failed to open Spooler, error %lu\n", GetLastError());
    if (!service) {
        CloseServiceHandle(scm);
        return;
    }

    ret = StartServiceA(service, 0, NULL);
    if (!ret && (error = GetLastError()) != ERROR_SERVICE_ALREADY_RUNNING)
    {
        skip("Spooler service not available, skipping test\n");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return;
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    create_test_files();
    create_database(msifile, sss_tables, ARRAY_SIZE(sss_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    if (error == ERROR_SUCCESS)
    {
        SERVICE_STATUS status;

        scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        service = OpenServiceA(scm, "Spooler", SC_MANAGER_ALL_ACCESS);

        ret = ControlService(service, SERVICE_CONTROL_STOP, &status);
        ok(ret, "ControlService failed %lu\n", GetLastError());

        CloseServiceHandle(service);
        CloseServiceHandle(scm);
    }

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service2.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

    if (error == ERROR_SUCCESS)
    {
        SERVICE_STATUS status;

        scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        service = OpenServiceA(scm, "Spooler", SC_MANAGER_ALL_ACCESS);

        ret = ControlService(service, SERVICE_CONTROL_STOP, &status);
        ok(ret, "ControlService failed %lu\n", GetLastError());

        CloseServiceHandle(service);
        CloseServiceHandle(scm);
    }

    delete_test_files();
    DeleteFileA(msifile);
}

static void delete_test_service(const char *name)
{
    BOOL ret;
    SC_HANDLE manager, service;

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(manager != NULL, "can't open service manager\n");
    if (!manager) return;

    service = OpenServiceA(manager, name, GENERIC_ALL);
    if (service)
    {
        ret = DeleteService( service );
        ok( ret, "failed to delete service %lu\n", GetLastError() );
        CloseServiceHandle(service);
    }
    CloseServiceHandle(manager);
}

static void test_delete_services(void)
{
    UINT r;
    SC_HANDLE manager, service;
    DWORD error;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(manager != NULL, "can't open service manager %lu\n", GetLastError());
    if (!manager) return;

    service = CreateServiceA(manager, "TestService3", "TestService3",
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL, "C:\\doesnt_exist.exe", NULL, NULL, NULL, NULL, NULL);
    ok(service != NULL, "can't create service %lu\n", GetLastError());
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    if (!service) return;

    create_test_files();
    create_database(msifile, sds_tables, ARRAY_SIZE(sds_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(manager != NULL, "can't open service manager\n");
    if (!manager) goto error;

    service = OpenServiceA(manager, "TestService3", GENERIC_ALL);
    error = GetLastError();
    ok(service == NULL, "TestService3 not deleted\n");
    ok(error == ERROR_SERVICE_DOES_NOT_EXIST, "wrong error %lu\n", error);
    CloseServiceHandle(manager);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service2.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

error:
    delete_test_service("TestService");
    delete_test_service("TestService2");
    delete_test_service("TestService3");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_install_services(void)
{
    UINT r;
    SC_HANDLE manager, service;
    LONG res;
    HKEY hKey;
    DWORD err_control, err_controlsize, err_controltype;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_database(msifile, sis_tables, ARRAY_SIZE(sis_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    ok(manager != NULL, "can't open service manager\n");
    if (!manager) goto error;

    service = OpenServiceA(manager, "TestService", GENERIC_ALL);
    ok(service != NULL, "TestService not installed\n");
    CloseServiceHandle(service);

    service = OpenServiceA(manager, "TestService4", GENERIC_ALL);
    ok(service == NULL, "TestService4 installed\n");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TestService", &hKey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);

    if (res == ERROR_SUCCESS)
    {
        err_control = 0xBEEF;
        err_controltype = REG_DWORD;
        err_controlsize = sizeof(err_control);
        res = RegQueryValueExA(hKey, "ErrorControl", NULL, &err_controltype, (LPBYTE)&err_control, &err_controlsize);
        ok(err_control == 0, "TestService.ErrorControl wrong, expected 0, got %lu\n", err_control);
        RegCloseKey(hKey);
    }

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    service = OpenServiceA(manager, "TestService", GENERIC_ALL);
    ok(service != NULL, "TestService deleted\n");
    CloseServiceHandle(service);
    CloseServiceHandle(manager);

    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service2.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

error:
    delete_test_service("TestService");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_self_registration(void)
{
    HKEY key;
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    extract_resource("selfreg.dll", "TESTDLL", "msitest\\selfreg.dll");
    create_database(msifile, sr_tables, ARRAY_SIZE(sr_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(!r, "got %u\n", r);
    RegCloseKey(key);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(!r, "got %u\n", r);

    r = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(r == ERROR_FILE_NOT_FOUND, "got %u\n", r);

    ok(!delete_pf("msitest\\selfreg.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\selfreg.dll");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_register_font(void)
{
    static const char regfont1[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    static const char regfont2[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Fonts";
    LONG ret;
    HKEY key;
    UINT r;
    REGSAM access = KEY_ALL_ACCESS;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\font.ttf", 1000);
    create_database(msifile, font_tables, ARRAY_SIZE(font_tables));

    if (is_wow64)
        access |= KEY_WOW64_64KEY;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, regfont1, 0, access, &key);
    if (ret)
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, regfont2, 0, access, &key);

    ret = RegQueryValueExA(key, "msi test font", NULL, NULL, NULL, NULL);
    ok(ret != ERROR_FILE_NOT_FOUND, "unexpected result %ld\n", ret);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    ret = RegQueryValueExA(key, "msi test font", NULL, NULL, NULL, NULL);
    ok(ret == ERROR_FILE_NOT_FOUND, "unexpected result %ld\n", ret);

    RegDeleteValueA(key, "msi test font");
    RegCloseKey(key);

error:
    DeleteFileA("msitest\\font.ttf");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_validate_product_id(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_database(msifile, vp_tables, ARRAY_SIZE(vp_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "SET_PRODUCT_ID=1");
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    r = MsiInstallProductA(msifile, "SET_PRODUCT_ID=2");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "PIDKEY=123-1234567");
    ok(r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %u\n", r);

    ok(delete_pf("msitest\\cabout\\new\\five.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout\\new", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\cabout\\four.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\cabout", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\changed\\three.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\changed", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\first\\two.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\first", FALSE), "Directory not created\n");
    ok(delete_pf("msitest\\filename", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\one.txt", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest\\service2.exe", TRUE), "File not installed\n");
    ok(delete_pf("msitest", FALSE), "Directory not created\n");

error:
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_install_remove_odbc(void)
{
    int gotdriver = 0, gotdriver2 = 0;
    char buffer[1000], *p;
    WORD len;
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\ODBCdriver.dll", 1000);
    create_file("msitest\\ODBCdriver2.dll", 1000);
    create_file("msitest\\ODBCtranslator.dll", 1000);
    create_file("msitest\\ODBCtranslator2.dll", 1000);
    create_file("msitest\\ODBCsetup.dll", 1000);
    create_database(msifile, odbc_tables, ARRAY_SIZE(odbc_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(pf_exists("msitest\\ODBCdriver.dll"), "file not created\n");
    ok(pf_exists("msitest\\ODBCdriver2.dll"), "file not created\n");
    ok(pf_exists("msitest\\ODBCtranslator.dll"), "file not created\n");
    ok(pf_exists("msitest\\ODBCtranslator2.dll"), "file not created\n");
    ok(pf_exists("msitest\\ODBCsetup.dll"), "file not created\n");

    r = SQLGetInstalledDrivers(buffer, sizeof(buffer), &len);
    ok(len < sizeof(buffer), "buffer too small\n");
    ok(r, "SQLGetInstalledDrivers failed\n");
    for (p = buffer; *p; p += strlen(p) + 1)
    {
        if (!strcmp(p, "ODBC test driver"))
            gotdriver = 1;
        if (!strcmp(p, "ODBC test driver2"))
            gotdriver2 = 1;
    }
    ok(gotdriver, "driver not installed\n");
    ok(gotdriver2, "driver 2 not installed\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\ODBCdriver.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\ODBCdriver2.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\ODBCtranslator.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\ODBCtranslator2.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\ODBCsetup.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

    gotdriver = gotdriver2 = 0;
    r = SQLGetInstalledDrivers(buffer, sizeof(buffer), &len);
    ok(len < sizeof(buffer), "buffer too small\n");
    ok(r, "SQLGetInstalledDrivers failed\n");
    for (p = buffer; *p; p += strlen(p) + 1)
    {
        if (!strcmp(p, "ODBC test driver"))
            gotdriver = 1;
        if (!strcmp(p, "ODBC test driver2"))
            gotdriver2 = 1;
    }
    ok(!gotdriver, "driver not installed\n");
    ok(!gotdriver2, "driver 2 not installed\n");

error:
    DeleteFileA("msitest\\ODBCdriver.dll");
    DeleteFileA("msitest\\ODBCdriver2.dll");
    DeleteFileA("msitest\\ODBCtranslator.dll");
    DeleteFileA("msitest\\ODBCtranslator2.dll");
    DeleteFileA("msitest\\ODBCsetup.dll");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_register_typelib(void)
{
    ITypeLib *tlb;
    HRESULT hr;
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    /* UnregisterTypeLibraries action fails in 64-bit Windows <= 7 */
    if (sizeof(void *) == 8 && strcmp( winetest_platform, "wine" ))
    {
        win_skip("broken on 64-bit Windows\n");
        return;
    }

    create_test_files();
    extract_resource(MAKEINTRESOURCEA(1), "TYPELIB", "msitest\\typelib.dll");
    create_database(msifile, tl_tables, ARRAY_SIZE(tl_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "got %u\n", r);

    hr = LoadRegTypeLib(&LIBID_register_test, 7, 1, 0, &tlb);
    ok(hr == S_OK, "got %#lx\n", hr);
    ITypeLib_Release(tlb);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    hr = LoadRegTypeLib(&LIBID_register_test, 7, 1, 0, &tlb);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got %#lx\n", hr);

    ok(!delete_pf("msitest\\typelib.dll", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\typelib.dll");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_create_remove_shortcut(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\target.txt", 1000);
    create_database(msifile, crs_tables, ARRAY_SIZE(crs_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(pf_exists("msitest\\target.txt"), "file not created\n");
    ok(pf_exists("msitest\\shortcut.lnk"), "file not created\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\shortcut.lnk", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\target.txt", TRUE), "file not removed\n");
    todo_wine ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\target.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_publish_components(void)
{
    static const char keypath[] =
        "Software\\Microsoft\\Installer\\Components\\0CBCFA296AC907244845745CEEB2F8AA";
    static const char keypath2[] =
        "Software\\Classes\\Installer\\Components\\0CBCFA296AC907244845745CEEB2F8AA";

    UINT r;
    LONG res;
    HKEY key;
    BYTE *data;
    DWORD size;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\english.txt", 1000);
    create_database(msifile, pub_tables, ARRAY_SIZE(pub_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    size = 0;
    r = MsiProvideQualifiedComponentA("{92AFCBC0-9CA6-4270-8454-47C5EE2B8FAA}",
            "english.txt", INSTALLMODE_DEFAULT, NULL, &size);
    ok(r == ERROR_SUCCESS, "MsiProvideQualifiedComponent returned %d\n", r);

    res = RegOpenKeyA(HKEY_CURRENT_USER, keypath, &key);
    ok(res == ERROR_SUCCESS, "components key not created %ld\n", res);

    res = RegQueryValueExA(key, "english.txt", NULL, NULL, NULL, &size);
    ok(res == ERROR_SUCCESS, "value not found %ld\n", res);

    data = malloc(size);
    res = RegQueryValueExA(key, "english.txt", NULL, NULL, data, &size);
    ok(res == ERROR_SUCCESS, "value not found %ld\n", res);
    RegCloseKey(key);

    res = RegDeleteKeyA(HKEY_CURRENT_USER, keypath);
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed %ld\n", res);

    res = RegCreateKeyExA(HKEY_LOCAL_MACHINE, keypath2, 0, NULL, REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED | KEY_WOW64_64KEY, NULL, &key, NULL );
    ok(res == ERROR_SUCCESS, "RegCreateKeyEx failed %ld\n", res);
    res = RegSetValueExA(key, "english.txt", 0, REG_MULTI_SZ, data, size);
    ok(res == ERROR_SUCCESS, "RegSetValueEx failed %ld\n", res);
    RegCloseKey(key);

    size = 0;
    r = MsiProvideQualifiedComponentA("{92AFCBC0-9CA6-4270-8454-47C5EE2B8FAA}",
            "english.txt", INSTALLMODE_DEFAULT, NULL, &size);
    ok(r == ERROR_SUCCESS, "MsiProvideQualifiedComponent returned %d\n", r);

    res = RegDeleteKeyExA(HKEY_LOCAL_MACHINE, keypath2, KEY_WOW64_64KEY, 0);
    ok(res == ERROR_SUCCESS, "RegDeleteKey failed %ld\n", res);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &key);
    ok(res == ERROR_SUCCESS, "RegCreateKey failed %ld\n", res);

    res = RegSetValueExA(key, "english.txt", 0, REG_MULTI_SZ, data, size);
    ok(res == ERROR_SUCCESS, "RegSetValueEx failed %ld\n", res);
    free(data);
    RegCloseKey(key);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CURRENT_USER, keypath, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "unexpected result %ld\n", res);

    ok(!delete_pf("msitest\\english.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\english.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_remove_duplicate_files(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\original.txt", 1000);
    create_file("msitest\\original2.txt", 1000);
    create_file("msitest\\original3.txt", 1000);
    create_database(msifile, rd_tables, ARRAY_SIZE(rd_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(pf_exists("msitest\\original.txt"), "file not created\n");
    ok(pf_exists("msitest\\original2.txt"), "file not created\n");
    ok(!pf_exists("msitest\\original3.txt"), "file created\n");
    ok(pf_exists("msitest\\duplicate.txt"), "file not created\n");
    ok(!pf_exists("msitest\\duplicate2.txt"), "file created\n");

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(delete_pf("msitest\\original.txt", TRUE), "file removed\n");
    ok(!delete_pf("msitest\\original2.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\original3.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\duplicate.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest\\duplicate2.txt", TRUE), "file not removed\n");
    ok(delete_pf("msitest", FALSE), "directory removed\n");

error:
    DeleteFileA("msitest\\original.txt");
    DeleteFileA("msitest\\original2.txt");
    DeleteFileA("msitest\\original3.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_find_related_products(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\product.txt", 1000);
    create_database(msifile, frp_tables, ARRAY_SIZE(frp_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* install again, so it finds the upgrade code */
    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\product.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\product.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_ini_values(void)
{
    UINT r;
    DWORD len;
    char inifile[MAX_PATH], buf[0x10];
    HANDLE file;
    BOOL ret;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\inifile.txt", 1000);
    create_database(msifile, ini_tables, ARRAY_SIZE(ini_tables));

    lstrcpyA(inifile, PROG_FILES_DIR);
    lstrcatA(inifile, "\\msitest");
    ret = CreateDirectoryA(inifile, NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    lstrcatA(inifile, "\\test.ini");
    file = CreateFileA(inifile, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(file);

    ret = WritePrivateProfileStringA("sectionA", "keyA", "valueA", inifile);
    ok(ret, "failed to write profile string %lu\n", GetLastError());

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    len = GetPrivateProfileStringA("section1", "key1", NULL, buf, sizeof(buf), inifile);
    ok(len == 6, "got %lu expected 6\n", len);

    len = GetPrivateProfileStringA("sectionA", "keyA", NULL, buf, sizeof(buf), inifile);
    ok(!len, "got %lu expected 0\n", len);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    len = GetPrivateProfileStringA("section1", "key1", NULL, buf, sizeof(buf), inifile);
    ok(!len, "got %lu expected 0\n", len);

    len = GetPrivateProfileStringA("sectionA", "keyA", NULL, buf, sizeof(buf), inifile);
    ok(!len, "got %lu expected 0\n", len);

    todo_wine ok(!delete_pf("msitest\\test.ini", TRUE), "file removed\n");
    ok(!delete_pf("msitest\\inifile.txt", TRUE), "file not removed\n");
    ok(delete_pf("msitest", FALSE), "directory removed\n");

error:
    DeleteFileA("msitest\\inifile.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_register_class_info(void)
{
    UINT r;
    LONG res;
    HKEY hkey;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\class.txt", 1000);
    create_database(msifile, rci_tables, ARRAY_SIZE(rci_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    if (is_64bit)
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Wow6432Node\\CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    else
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "FileType\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "AppID\\{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    if (is_64bit)
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Wow6432Node\\CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    else
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "FileType\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "AppID\\{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    ok(!delete_pf("msitest\\class.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\class.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_register_extension_info(void)
{
    UINT r;
    LONG res;
    HKEY hkey;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\extension.txt", 1000);
    create_database(msifile, rei_tables, ARRAY_SIZE(rei_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, ".extension", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Prog.Id.1\\shell\\Open\\command", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, ".extension", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Prog.Id.1", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    ok(!delete_pf("msitest\\extension.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\extension.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_register_progid_info(void)
{
    UINT r;
    LONG res;
    HKEY hkey;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\progid.txt", 1000);
    create_database(msifile, rpi_tables, ARRAY_SIZE(rpi_tables));

    res = RegCreateKeyExA(HKEY_CLASSES_ROOT, "Winetest.Orphaned", 0, NULL, 0,
                          KEY_ALL_ACCESS, NULL, &hkey, NULL);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    if (is_64bit)
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Wow6432Node\\CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    else
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.1", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.2", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.VerClass.1", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.VerClass", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.NoProgIdClass.1", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key created\n");
    if (res == ERROR_SUCCESS) RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.NoProgIdClass", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key created\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Orphaned", &hkey);
    ok(res == ERROR_SUCCESS, "key deleted\n");
    if (res == ERROR_SUCCESS) RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Orphaned2", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key created\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Extension", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    if (is_64bit)
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Wow6432Node\\CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    else
        res = RegOpenKeyA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.1", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.2", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.VerClass.1", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.VerClass", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.NoProgIdClass.1", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.NoProgIdClass", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Orphaned", &hkey);
    ok(res == ERROR_SUCCESS, "key deleted\n");
    if (res == ERROR_SUCCESS) RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Orphaned2", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Extension", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    ok(!delete_pf("msitest\\progid.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\progid.txt");
    delete_test_files();
    DeleteFileA(msifile);
    RegDeleteKeyA(HKEY_CLASSES_ROOT, "Winetest.Orphaned");
}

static void test_register_mime_info(void)
{
    UINT r;
    LONG res;
    HKEY hkey;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\mime.txt", 1000);
    create_database(msifile, rmi_tables, ARRAY_SIZE(rmi_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "MIME\\Database\\Content Type\\mime/type", &hkey);
    ok(res == ERROR_SUCCESS, "key not created\n");
    RegCloseKey(hkey);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "MIME\\Database\\Content Type\\mime/type", &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "key not removed\n");

    ok(!delete_pf("msitest\\mime.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\mime.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_publish_assemblies(void)
{
    static const char manifest[] =
        "<assemblyIdentity type=\"win32\" name=\"Wine.Win32.Assembly\" "
        "version=\"1.0.0.0\" publicKeyToken=\"abcdef0123456789\" "
        "processorArchitecture=\"x86\"/>";
    static const char manifest_local[] =
        "<assemblyIdentity type=\"win32\" name=\"Wine.Win32.Local.Assembly\" "
        "version=\"1.0.0.0\" publicKeyToken=\"abcdef0123456789\" "
        "processorArchitecture=\"x86\"/>";
    static const char classes_path_dotnet[] =
        "Installer\\Assemblies\\Global";
    static const char classes_path_dotnet_local[] =
        "Installer\\Assemblies\\C:|Program Files|msitest|application_dotnet.txt";
    static const char classes_path_dotnet_local_wow64[] =
        "Installer\\Assemblies\\C:|Program Files (x86)|msitest|application_dotnet.txt";
    static const char classes_path_fake_local[] =
        "Installer\\Assemblies\\C:|Program Files|msitest|nonexistent.txt";
    static const char classes_path_fake_local_wow64[] =
        "Installer\\Assemblies\\C:|Program Files (x86)|msitest|nonexistent.txt";
    static const char classes_path_win32[] =
        "Installer\\Win32Assemblies\\Global";
    static const char classes_path_win32_local[] =
        "Installer\\Win32Assemblies\\C:|Program Files|msitest|application_win32.txt";
    static const char classes_path_win32_local_wow64[] =
        "Installer\\Win32Assemblies\\C:|Program Files (x86)|msitest|application_win32.txt";
    static const char path_dotnet[] =
        "Software\\Microsoft\\Installer\\Assemblies\\Global";
    static const char path_dotnet_local[] =
        "Software\\Microsoft\\Installer\\Assemblies\\C:|Program Files|msitest|application_dotnet.txt";
    static const char path_dotnet_local_wow64[] =
        "Software\\Microsoft\\Installer\\Assemblies\\C:|Program Files (x86)|msitest|application_dotnet.txt";
    static const char path_fake_local[] =
        "Software\\Microsoft\\Installer\\Assemblies\\C:|Program Files|msitest|nonexistent.txt";
    static const char path_fake_local_wow64[] =
        "Software\\Microsoft\\Installer\\Assemblies\\C:|Program Files (x86)|msitest|nonexistent.txt";
    static const char path_win32[] =
        "Software\\Microsoft\\Installer\\Win32Assemblies\\Global";
    static const char path_win32_local[] =
        "Software\\Microsoft\\Installer\\Win32Assemblies\\C:|Program Files|msitest|application_win32.txt";
    static const char path_win32_local_wow64[] =
        "Software\\Microsoft\\Installer\\Win32Assemblies\\C:|Program Files (x86)|msitest|application_win32.txt";
    static const char name_dotnet[] =
        "Wine.Dotnet.Assembly,processorArchitecture=\"MSIL\",publicKeyToken=\"abcdef0123456789\","
        "version=\"1.0.0.0\",culture=\"neutral\"";
    static const char name_dotnet_local[] =
        "Wine.Dotnet.Local.Assembly,processorArchitecture=\"MSIL\",publicKeyToken=\"abcdef0123456789\","
        "version=\"1.0.0.0\",culture=\"neutral\"";
    static const char name_win32[] =
        "Wine.Win32.Assembly,processorArchitecture=\"x86\",publicKeyToken=\"abcdef0123456789\","
        "type=\"win32\",version=\"1.0.0.0\"";
    static const char name_win32_local[] =
        "Wine.Win32.Local.Assembly,processorArchitecture=\"x86\",publicKeyToken=\"abcdef0123456789\","
        "type=\"win32\",version=\"1.0.0.0\"";
    UINT r;
    LONG res;
    HKEY hkey;
    const char *path;
    int access;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\win32.txt", 1000);
    create_file("msitest\\win32_local.txt", 1000);
    create_file("msitest\\dotnet.txt", 1000);
    create_file("msitest\\dotnet_local.txt", 1000);
    create_file_data("msitest\\manifest.txt", manifest, 0);
    create_file_data("msitest\\manifest_local.txt", manifest_local, 0);
    create_file("msitest\\application_win32.txt", 1000);
    create_file("msitest\\application_dotnet.txt", 1000);
    create_file("msitest\\fake_local.txt", 1000);
    create_database(msifile, pa_tables, ARRAY_SIZE(pa_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto done;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_dotnet, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_dotnet, "rcHQPHq?CA@Uv-XqMI1e>Z'q,T*76M@=YEg6My?~]\0");
    RegCloseKey(hkey);

    path = (is_wow64 || is_64bit) ? path_dotnet_local_wow64 : path_dotnet_local;
    res = RegOpenKeyA(HKEY_CURRENT_USER, path, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_dotnet_local, "rcHQPHq?CA@Uv-XqMI1e>LF,8A?0d.AW@vcZ$Cgox\0");
    RegCloseKey(hkey);

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_win32, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_win32, "rcHQPHq?CA@Uv-XqMI1e>}NJjwR'%D9v1p!v{WV(%\0");
    RegCloseKey(hkey);

    path = (is_wow64 || is_64bit) ? path_win32_local_wow64 : path_win32_local;
    res = RegOpenKeyA(HKEY_CURRENT_USER, path, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_win32_local, "rcHQPHq?CA@Uv-XqMI1e>C)Uvlj*53A)u(QQ9=)X!\0");
    RegCloseKey(hkey);

    /* No registration is done for a local assembly with no matching file */
    path = (is_wow64 || is_64bit) ? path_fake_local_wow64 : path_fake_local;
    res = RegOpenKeyA(HKEY_CURRENT_USER, path, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_dotnet, &hkey);
    if (res == ERROR_SUCCESS)
    {
        res = RegDeleteValueA(hkey, name_dotnet);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
        RegCloseKey(hkey);
    }

    path = (is_wow64 || is_64bit) ? path_dotnet_local_wow64 : path_dotnet_local;
    res = RegOpenKeyA(HKEY_CURRENT_USER, path, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_win32, &hkey);
    if (res == ERROR_SUCCESS)
    {
        res = RegDeleteValueA(hkey, name_win32);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
        RegCloseKey(hkey);
    }

    path = (is_wow64 || is_64bit) ? path_win32_local_wow64 : path_win32_local;
    res = RegOpenKeyA(HKEY_CURRENT_USER, path, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    r = MsiInstallProductA(msifile, "ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    access = KEY_QUERY_VALUE;
    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, classes_path_dotnet, 0, access, &hkey);
    if (res == ERROR_FILE_NOT_FOUND && is_wow64) /* Vista WOW64 */
    {
        trace("Using 64-bit registry view for HKCR\\Installer\n");
        access = KEY_QUERY_VALUE | KEY_WOW64_64KEY;
        res = RegOpenKeyExA(HKEY_CLASSES_ROOT, classes_path_dotnet, 0, access, &hkey);
    }
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_dotnet, "rcHQPHq?CA@Uv-XqMI1e>Z'q,T*76M@=YEg6My?~]\0");
    RegCloseKey(hkey);

    path = (is_wow64 || is_64bit) ? classes_path_dotnet_local_wow64 : classes_path_dotnet_local;
    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, path, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_dotnet_local, "rcHQPHq?CA@Uv-XqMI1e>LF,8A?0d.AW@vcZ$Cgox\0");
    RegCloseKey(hkey);

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, classes_path_win32, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_win32, "rcHQPHq?CA@Uv-XqMI1e>}NJjwR'%D9v1p!v{WV(%\0");
    RegCloseKey(hkey);

    path = (is_wow64 || is_64bit) ? classes_path_win32_local_wow64 : classes_path_win32_local;
    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, path, 0, access, &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %ld\n", res);
    CHECK_REG_MULTI(hkey, name_win32_local, "rcHQPHq?CA@Uv-XqMI1e>C)Uvlj*53A)u(QQ9=)X!\0");
    RegCloseKey(hkey);

    /* No registration is done for a local assembly with no matching file */
    path = (is_wow64 || is_64bit) ? classes_path_fake_local_wow64 : classes_path_fake_local;
    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, path, 0, access, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    r = MsiInstallProductA(msifile, "REMOVE=ALL ALLUSERS=1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, classes_path_dotnet, &hkey);
    if (res == ERROR_SUCCESS)
    {
        res = RegDeleteValueA(hkey, name_dotnet);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
        RegCloseKey(hkey);
    }

    path = (is_wow64 || is_64bit) ? classes_path_dotnet_local_wow64 : classes_path_dotnet_local;
    res = RegOpenKeyA(HKEY_CLASSES_ROOT, path, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, classes_path_win32, &hkey);
    if (res == ERROR_SUCCESS)
    {
        res = RegDeleteValueA(hkey, name_win32);
        ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);
        RegCloseKey(hkey);
    }

    path = (is_wow64 || is_64bit) ? classes_path_win32_local_wow64 : classes_path_win32_local;
    res = RegOpenKeyA(HKEY_CLASSES_ROOT, path, &hkey);
    ok(res == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", res);

done:
    DeleteFileA("msitest\\win32.txt");
    DeleteFileA("msitest\\win32_local.txt");
    DeleteFileA("msitest\\dotnet.txt");
    DeleteFileA("msitest\\dotnet_local.txt");
    DeleteFileA("msitest\\manifest.txt");
    DeleteFileA("msitest\\manifest_local.txt");
    DeleteFileA("msitest\\application_win32.txt");
    DeleteFileA("msitest\\application_dotnet.txt");
    DeleteFileA("msitest\\fake_local.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static void test_remove_existing_products(void)
{
    UINT r;

    if (!is_process_elevated())
    {
        skip("process is limited\n");
        return;
    }

    create_test_files();
    create_file("msitest\\rep.txt", 1000);
    create_database(msifile, rep_tables, ARRAY_SIZE(rep_tables));

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    r = MsiInstallProductA(msifile, NULL);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiInstallProductA(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    ok(!delete_pf("msitest\\rep.txt", TRUE), "file not removed\n");
    ok(!delete_pf("msitest", FALSE), "directory not removed\n");

error:
    DeleteFileA("msitest\\rep.txt");
    delete_test_files();
    DeleteFileA(msifile);
}

static HANDLE get_admin_token(void)
{
    TOKEN_ELEVATION_TYPE type;
    TOKEN_LINKED_TOKEN linked;
    DWORD size;

#ifdef __REACTOS__
#ifndef GetCurrentThreadEffectiveToken
#define GetCurrentProcessToken() ((HANDLE)~(ULONG_PTR)3)
#define GetCurrentThreadEffectiveToken() GetCurrentProcessToken()
#endif
#endif

    if (!GetTokenInformation(GetCurrentThreadEffectiveToken(), TokenElevationType, &type, sizeof(type), &size)
            || type == TokenElevationTypeFull)
        return NULL;

    if (!GetTokenInformation(GetCurrentThreadEffectiveToken(), TokenLinkedToken, &linked, sizeof(linked), &size))
        return NULL;
    return linked.LinkedToken;
}

void restart_as_admin_elevated(void)
{
    HANDLE token;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;

    if (!(token = get_admin_token())) return;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessAsUserW(token, NULL, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        DWORD exit_code;

        trace("restarting\n");
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        ExitProcess(exit_code);
    }
    else trace("failed to restart as admin %lu\n", GetLastError());

    CloseHandle(token);
}

START_TEST(action)
{
    DWORD len;
    char temp_path[MAX_PATH], prev_path[MAX_PATH], log_file[MAX_PATH];

    if (!is_process_elevated()) restart_as_admin_elevated();

    subtest("custom");

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    GetCurrentDirectoryA(MAX_PATH, prev_path);
    GetTempPathA(MAX_PATH, temp_path);
    SetCurrentDirectoryA(temp_path);

    lstrcpyA(CURR_DIR, temp_path);
    len = lstrlenA(CURR_DIR);

    if(len && (CURR_DIR[len - 1] == '\\'))
        CURR_DIR[len - 1] = 0;

    ok(get_system_dirs(), "failed to retrieve system dirs\n");
    ok(get_user_dirs(), "failed to retrieve user dirs\n");

    /* Create only one log file and don't append. We have to pass something
     * for the log mode for this to work. The logfile needs to have an absolute
     * path otherwise we still end up with some extra logfiles as some tests
     * change the current directory.
     */
    lstrcpyA(log_file, temp_path);
    lstrcatA(log_file, "\\msitest.log");
    MsiEnableLogA(INSTALLLOGMODE_FATALEXIT, log_file, 0);

    test_register_product();
    test_publish_product();
    test_publish_features();
    test_register_user();
    test_process_components();
    test_publish();
    test_publish_sourcelist();
    test_remove_files();
    test_move_files();
    test_duplicate_files();
    test_write_registry_values();
    test_envvar();
    test_create_remove_folder();
    test_start_stop_services();
    test_delete_services();
    test_install_services();
    test_self_registration();
    test_register_font();
    test_validate_product_id();
    test_install_remove_odbc();
    test_register_typelib();
    test_create_remove_shortcut();
    test_publish_components();
    test_remove_duplicate_files();
    test_find_related_products();
    test_ini_values();
    test_register_class_info();
    test_register_extension_info();
    test_register_progid_info();
    test_register_mime_info();
    test_publish_assemblies();
    test_remove_existing_products();

    DeleteFileA(log_file);
    SetCurrentDirectoryA(prev_path);
}
