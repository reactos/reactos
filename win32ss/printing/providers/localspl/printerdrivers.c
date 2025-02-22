/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions for printer driver information
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"
#include <strsafe.h>

typedef struct {
    WCHAR   src[MAX_PATH+MAX_PATH];
    WCHAR   dst[MAX_PATH+MAX_PATH];
    DWORD   srclen;
    DWORD   dstlen;
    DWORD   copyflags;
    BOOL    lazy;
} apd_data_t;

static const WCHAR backslashW[] = {'\\',0};
static const WCHAR configuration_fileW[] = {'C','o','n','f','i','g','u','r','a','t','i','o','n',' ','F','i','l','e',0};
static const WCHAR datatypeW[] = {'D','a','t','a','t','y','p','e',0};
static const WCHAR data_fileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static const WCHAR dependent_filesW[] = {'D','e','p','e','n','d','e','n','t',' ','F','i','l','e','s',0};
static const WCHAR driverW[] = {'D','r','i','v','e','r',0};
static const WCHAR emptyW[] = {0};
static const WCHAR fmt_driversW[] = { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'c','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'E','n','v','i','r','o','n','m','e','n','t','s','\\',
                                  '%','s','\\','D','r','i','v','e','r','s','%','s',0 };
static const WCHAR monitorW[] = {'M','o','n','i','t','o','r',0};
static const WCHAR previous_namesW[] = {'P','r','e','v','i','o','u','s',' ','N','a','m','e','s',0};

static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};

static const WCHAR spoolW[] = {'\\','s','p','o','o','l',0};
static const WCHAR driversW[] = {'\\','d','r','i','v','e','r','s','\\',0};
static const WCHAR ia64_envnameW[] = {'W','i','n','d','o','w','s',' ','I','A','6','4',0};
static const WCHAR ia64_subdirW[] = {'i','a','6','4',0};
static const WCHAR version3_regpathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR version3_subdirW[] = {'\\','3',0};
static const WCHAR version0_regpathW[] = {'\\','V','e','r','s','i','o','n','-','0',0};
static const WCHAR version0_subdirW[] = {'\\','0',0};
static const WCHAR help_fileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static const WCHAR x64_envnameW[] = {'W','i','n','d','o','w','s',' ','x','6','4',0};
static const WCHAR x64_subdirW[] = {'x','6','4',0};
static const WCHAR x86_envnameW[] = {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR x86_subdirW[] = {'w','3','2','x','8','6',0};
static const WCHAR win40_envnameW[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR win40_subdirW[] = {'w','i','n','4','0',0};

static PRINTENV_T env_ia64 =  {ia64_envnameW, ia64_subdirW, 3, version3_regpathW, version3_subdirW};

static PRINTENV_T env_x86 =   {x86_envnameW, x86_subdirW, 3, version3_regpathW, version3_subdirW};

static PRINTENV_T env_x64 =   {x64_envnameW, x64_subdirW, 3, version3_regpathW, version3_subdirW};

static PRINTENV_T env_win40 = {win40_envnameW, win40_subdirW, 0, version0_regpathW, version0_subdirW};

static PPRINTENV_T all_printenv[] = {&env_x86, &env_x64, &env_ia64, &env_win40};

static const DWORD di_sizeof[] = {0, sizeof(DRIVER_INFO_1W), sizeof(DRIVER_INFO_2W),
                                     sizeof(DRIVER_INFO_3W), sizeof(DRIVER_INFO_4W),
                                     sizeof(DRIVER_INFO_5W), sizeof(DRIVER_INFO_6W),
                                  0, sizeof(DRIVER_INFO_8W)};

static WCHAR wszScratchPad[MAX_PATH] = L"";

static WCHAR wszLocalSplFile[MAX_PATH] = L"";
static WCHAR wszPrintUiFile[MAX_PATH] = L"";
static WCHAR wszDriverPath[MAX_PATH] = L"";

BOOL
InitializePrinterDrivers(VOID)
{
    WCHAR szSysDir[MAX_PATH];
    DWORD cbBuf;

    if (wszLocalSplFile[0] && wszPrintUiFile[0])
        return TRUE;

    if (!GetSystemDirectoryW(szSysDir, _countof(szSysDir)))
    {
        ERR("GetSystemDirectoryW failed\n");
        return FALSE;
    }

    StringCbCopyW(wszLocalSplFile, sizeof(wszLocalSplFile), szSysDir);
    StringCbCatW(wszLocalSplFile, sizeof(wszLocalSplFile), L"\\localspl.dll");

    StringCbCopyW(wszPrintUiFile, sizeof(wszPrintUiFile), szSysDir);
    StringCbCatW(wszPrintUiFile, sizeof(wszPrintUiFile), L"\\printui.dll");

    if (!LocalGetPrinterDriverDirectory(NULL, (PWSTR)wszCurrentEnvironment, 1, (PBYTE)szSysDir, (DWORD)sizeof(szSysDir), &cbBuf))
    {
        ERR("LocalGetPrinterDriverDirectory failed\n");
        return FALSE;
    }

    StringCbCopyW(wszDriverPath, sizeof(wszDriverPath), szSysDir);
    StringCbCatW(wszDriverPath, sizeof(wszDriverPath), version3_subdirW);
    StringCbCatW(wszDriverPath, sizeof(wszDriverPath), backslashW);

    // HAX! need to get it from the Reg Key L"Driver"!
    StringCbCatW(wszDriverPath, sizeof(wszDriverPath), L"UniDrv.dll");

    FIXME("DriverPath : %S\n",wszDriverPath);

    return TRUE;
}

// Local Constants
static DWORD dwDriverInfo1Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_1W, pName),
    MAXDWORD
};

static DWORD dwDriverInfo2Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_2W, pName),
    FIELD_OFFSET(DRIVER_INFO_2W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_2W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_2W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_2W, pConfigFile),
    MAXDWORD
};

static DWORD dwDriverInfo3Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_3W, pName),
    FIELD_OFFSET(DRIVER_INFO_3W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_3W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_3W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pConfigFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pHelpFile),
    FIELD_OFFSET(DRIVER_INFO_3W, pDependentFiles),
    FIELD_OFFSET(DRIVER_INFO_3W, pMonitorName),
    FIELD_OFFSET(DRIVER_INFO_3W, pDefaultDataType),
    MAXDWORD
};

static DWORD dwDriverInfo4Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_4W, pName),
    FIELD_OFFSET(DRIVER_INFO_4W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_4W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_4W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_4W, pConfigFile),
    FIELD_OFFSET(DRIVER_INFO_4W, pHelpFile),
    FIELD_OFFSET(DRIVER_INFO_4W, pDependentFiles),
    FIELD_OFFSET(DRIVER_INFO_4W, pMonitorName),
    FIELD_OFFSET(DRIVER_INFO_4W, pDefaultDataType),
    FIELD_OFFSET(DRIVER_INFO_4W, pszzPreviousNames),
    MAXDWORD
};

static DWORD dwDriverInfo5Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_5W, pName),
    FIELD_OFFSET(DRIVER_INFO_5W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_5W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_5W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_5W, pConfigFile),
    MAXDWORD
};

static DWORD dwDriverInfo6Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_6W, pName),
    FIELD_OFFSET(DRIVER_INFO_6W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_6W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_6W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_6W, pConfigFile),
    FIELD_OFFSET(DRIVER_INFO_6W, pHelpFile),
    FIELD_OFFSET(DRIVER_INFO_6W, pDependentFiles),
    FIELD_OFFSET(DRIVER_INFO_6W, pMonitorName),
    FIELD_OFFSET(DRIVER_INFO_6W, pDefaultDataType),
    FIELD_OFFSET(DRIVER_INFO_6W, pszzPreviousNames),
    FIELD_OFFSET(DRIVER_INFO_6W, pszMfgName),
    FIELD_OFFSET(DRIVER_INFO_6W, pszOEMUrl),
    FIELD_OFFSET(DRIVER_INFO_6W, pszHardwareID),
    FIELD_OFFSET(DRIVER_INFO_6W, pszProvider),
    MAXDWORD
};

static DWORD dwDriverInfo8Offsets[] = {
    FIELD_OFFSET(DRIVER_INFO_8W, pName),
    FIELD_OFFSET(DRIVER_INFO_8W, pEnvironment),
    FIELD_OFFSET(DRIVER_INFO_8W, pDriverPath),
    FIELD_OFFSET(DRIVER_INFO_8W, pDataFile),
    FIELD_OFFSET(DRIVER_INFO_8W, pConfigFile),
    FIELD_OFFSET(DRIVER_INFO_8W, pHelpFile),
    FIELD_OFFSET(DRIVER_INFO_8W, pDependentFiles),
    FIELD_OFFSET(DRIVER_INFO_8W, pMonitorName),
    FIELD_OFFSET(DRIVER_INFO_8W, pDefaultDataType),
    FIELD_OFFSET(DRIVER_INFO_8W, pszzPreviousNames),
    FIELD_OFFSET(DRIVER_INFO_8W, pszMfgName),
    FIELD_OFFSET(DRIVER_INFO_8W, pszOEMUrl),
    FIELD_OFFSET(DRIVER_INFO_8W, pszHardwareID),
    FIELD_OFFSET(DRIVER_INFO_8W, pszProvider),
    FIELD_OFFSET(DRIVER_INFO_8W, pszPrintProcessor),
    FIELD_OFFSET(DRIVER_INFO_8W, pszVendorSetup),
    FIELD_OFFSET(DRIVER_INFO_8W, pszzColorProfiles),
    FIELD_OFFSET(DRIVER_INFO_8W, pszInfPath),
    FIELD_OFFSET(DRIVER_INFO_8W, pszzCoreDriverDependencies),
    MAXDWORD
};

static void
ToMultiSz(LPWSTR pString)
{
    while (*pString)
    {
        if (*pString == '|')
            *pString = '\0';
        pString++;
    }
}

static void
_LocalGetPrinterDriverLevel1(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_1W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[1];

    /* This value is only here to send something, I have not verified if it is actually correct */
    pwszStrings[0] = pPrinter->pwszPrinterDriver;

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
        }

        *pcbNeeded += sizeof(DRIVER_INFO_1W);
        return;
    }

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo1Offsets, *ppDriverInfoEnd);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel2(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_2W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[5];

    pwszStrings[0] = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = wszDriverPath;          // pDriverPath
    pwszStrings[3] = wszLocalSplFile;        // pDataFile
    pwszStrings[4] = wszPrintUiFile;         // pConfigFile

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_2W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo2Offsets, *ppDriverInfoEnd);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel3(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_3W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[9];

    pwszStrings[0] = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = wszDriverPath;          // pDriverPath
    pwszStrings[3] = wszLocalSplFile;        // pDataFile
    pwszStrings[4] = wszPrintUiFile;         // pConfigFile
    pwszStrings[5] = L"";  // pHelpFile
    pwszStrings[6] = L"localspl.dll|printui.dll|";  // pDependentFiles, | is separator and terminator!
    pwszStrings[7] = NULL;
    if (pPrinter->pPort && pPrinter->pPort->pPrintMonitor)
    {
        pwszStrings[7]  = pPrinter->pPort->pPrintMonitor->pwszName;
    }
    pwszStrings[8] = pPrinter->pwszDefaultDatatype;


    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_3W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo3Offsets, *ppDriverInfoEnd);
    ToMultiSz((*ppDriverInfo)->pDependentFiles);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel4(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_4W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[10];

    pwszStrings[0] = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = wszDriverPath;          // pDriverPath
    pwszStrings[3] = wszLocalSplFile;        // pDataFile
    pwszStrings[4] = wszPrintUiFile;         // pConfigFile
    pwszStrings[5] = L"";  // pHelpFile
    pwszStrings[6] = L"localspl.dll|printui.dll|";  // pDependentFiles, | is separator and terminator!
    pwszStrings[7] = NULL;
    if (pPrinter->pPort && pPrinter->pPort->pPrintMonitor)
    {
        pwszStrings[7] = pPrinter->pPort->pPrintMonitor->pwszName;
    }
    pwszStrings[8] = pPrinter->pwszDefaultDatatype;
    pwszStrings[9] = NULL; // pszzPreviousNames

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_4W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo4Offsets, *ppDriverInfoEnd);
    ToMultiSz((*ppDriverInfo)->pDependentFiles);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel5(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_5W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[5];

    pwszStrings[0] = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1] = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2] = wszDriverPath;          // pDriverPath UniDrv.dll
    pwszStrings[3] = wszLocalSplFile;        // pDataFile.ppd
    pwszStrings[4] = wszPrintUiFile;         // pConfigFile UniDrvUI.dll

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_5W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;
    // Driver attributes, like UMPD/KMPD.
    (*ppDriverInfo)->dwDriverAttributes = 0; // UMPD/KMPD, So where are they?
    // Number of times the configuration file for this driver has been upgraded or downgraded since the last spooler restart.
    (*ppDriverInfo)->dwConfigVersion = 1;
    // Number of times the driver file for this driver has been upgraded or downgraded since the last spooler restart.
    (*ppDriverInfo)->dwDriverVersion = 1;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo5Offsets, *ppDriverInfoEnd);
    (*ppDriverInfo)++;
}


static void
_LocalGetPrinterDriverLevel6(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_6W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[14];

    StringCbCopyW(wszScratchPad, sizeof(wszPrintProviderInfo[1]), wszPrintProviderInfo[1]); // Provider Name

    pwszStrings[0]  = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1]  = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2]  = wszDriverPath;          // pDriverPath
    pwszStrings[3]  = wszLocalSplFile;        // pDataFile
    pwszStrings[4]  = wszPrintUiFile;         // pConfigFile
    pwszStrings[5]  = L"";  // pHelpFile
    pwszStrings[6]  = L"localspl.dll|printui.dll|";  // pDependentFiles, | is separator and terminator!
    pwszStrings[7]  = NULL;
    if (pPrinter->pPort && pPrinter->pPort->pPrintMonitor)
    {
        pwszStrings[7] = pPrinter->pPort->pPrintMonitor->pwszName;
    }
    pwszStrings[8]  = pPrinter->pwszDefaultDatatype;
    pwszStrings[9]  = NULL; // pszzPreviousNames
    pwszStrings[10] = NULL; // pszMfgName
    pwszStrings[11] = NULL; // pszOEMUrl
    pwszStrings[12] = NULL; // pszHardwareID
    pwszStrings[13] = wszScratchPad;

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_6W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo6Offsets, *ppDriverInfoEnd);
    ToMultiSz((*ppDriverInfo)->pDependentFiles);
    (*ppDriverInfo)++;
}

static void
_LocalGetPrinterDriverLevel8(PLOCAL_PRINTER pPrinter, PDRIVER_INFO_8W* ppDriverInfo, PBYTE* ppDriverInfoEnd, PDWORD pcbNeeded)
{
    DWORD n;
    PCWSTR pwszStrings[19];

    StringCbCopyW(wszScratchPad, sizeof(wszPrintProviderInfo[1]), wszPrintProviderInfo[1]); // Provider Name

    pwszStrings[0]  = pPrinter->pwszPrinterDriver;  // pName
    pwszStrings[1]  = wszCurrentEnvironment;  // pEnvironment
    pwszStrings[2]  = wszDriverPath;          // pDriverPath
    pwszStrings[3]  = wszLocalSplFile;        // pDataFile
    pwszStrings[4]  = wszPrintUiFile;         // pConfigFile
    pwszStrings[5]  = L"";  // pHelpFile
    pwszStrings[6]  = L"localspl.dll|printui.dll|";  // pDependentFiles, | is separator and terminator!
    pwszStrings[7]  = NULL;
    if (pPrinter->pPort && pPrinter->pPort->pPrintMonitor)
    {
        pwszStrings[7] = pPrinter->pPort->pPrintMonitor->pwszName;
    }
    pwszStrings[8]  = pPrinter->pwszDefaultDatatype;
    pwszStrings[9]  = NULL; // pszzPreviousNames
    pwszStrings[10] = NULL; // pszMfgName
    pwszStrings[11] = NULL; // pszOEMUrl
    pwszStrings[12] = NULL; // pszHardwareID
    pwszStrings[13] = wszScratchPad;
    pwszStrings[14] = NULL;
    if ( pPrinter->pPrintProcessor )
    {
        pwszStrings[14] = pPrinter->pPrintProcessor->pwszName;
    }
    pwszStrings[15] = NULL; // pszVendorSetup
    pwszStrings[16] = NULL; // pszzColorProfiles
    pwszStrings[17] = NULL; // pszInfPath
    pwszStrings[18] = NULL; // pszzCoreDriverDependencies

    // Calculate the string lengths.
    if (!ppDriverInfo)
    {
        for (n = 0; n < _countof(pwszStrings); ++n)
        {
            if (pwszStrings[n])
            {
                *pcbNeeded += (wcslen(pwszStrings[n]) + 1) * sizeof(WCHAR);
            }
        }

        *pcbNeeded += sizeof(DRIVER_INFO_8W);
        return;
    }

    (*ppDriverInfo)->cVersion = 3;

    // Finally copy the structure and advance to the next one in the output buffer.
    *ppDriverInfoEnd = PackStrings(pwszStrings, (PBYTE)(*ppDriverInfo), dwDriverInfo8Offsets, *ppDriverInfoEnd);
    ToMultiSz((*ppDriverInfo)->pDependentFiles);
    (*ppDriverInfo)++;
}

typedef void (*PLocalPrinterDriverLevelFunc)(PLOCAL_PRINTER, PVOID, PBYTE*, PDWORD);

static const PLocalPrinterDriverLevelFunc pfnPrinterDriverLevels[] = {
    NULL,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel1,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel2,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel3,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel4,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel5,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel6,
    NULL,
    (PLocalPrinterDriverLevelFunc)&_LocalGetPrinterDriverLevel8
};

BOOL WINAPI LocalGetPrinterDriver(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pEnd = &pDriverInfo[cbBuf];
    PLOCAL_HANDLE pHandle;
    PLOCAL_PRINTER_HANDLE pPrinterHandle;

    TRACE("LocalGetPrinterDriver(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Check if this is a printer handle.
    pHandle = (PLOCAL_HANDLE)hPrinter;
    if (pHandle->HandleType != HandleType_Printer)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pPrinterHandle = (PLOCAL_PRINTER_HANDLE)pHandle->pSpecificHandle;

    // Only support 8 levels and not 7
    if (Level < 1 || Level == 7 || Level > 8)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;

    pfnPrinterDriverLevels[Level](pPrinterHandle->pPrinter, NULL, NULL, pcbNeeded);

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        ERR("Insuffisient Buffer size\n");
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the information.
    pEnd = &pDriverInfo[*pcbNeeded];

    pfnPrinterDriverLevels[Level](pPrinterHandle->pPrinter, &pDriverInfo, &pEnd, NULL);

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI LocalGetPrinterDriverEx(
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    DWORD dwClientMajorVersion,
    DWORD dwClientMinorVersion,
    PDWORD pdwServerMajorVersion,
    PDWORD pdwServerMinorVersion )
{
    FIXME("LocalGetPrinterDriverEx(%p, %lu, %lu, %p, %lu, %p, %lu, %lu, %p, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMajorVersion, pdwServerMinorVersion);
    //// HACK-plement
    return LocalGetPrinterDriver( hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded );
}

BOOL WINAPI
LocalEnumPrinterDrivers(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PSKIPLIST_NODE pNode;
    PBYTE pEnd;
    PLOCAL_PRINTER pPrinter;

    FIXME("LocalEnumPrinterDrivers(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned);

    // Only support 8 levels and not 7
    if (Level < 1 || Level == 7 || Level > 8)
    {
        // The caller supplied an invalid level.
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Count the required buffer size.
    *pcbNeeded = 0;

    // Count the required buffer size and the number of printers.
    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        pfnPrinterDriverLevels[Level](pPrinter, NULL, NULL, pcbNeeded);
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }

    // Copy over the Printer information.
    pEnd = &pDriverInfo[*pcbNeeded];

    for (pNode = PrinterList.Head.Next[0]; pNode; pNode = pNode->Next[0])
    {
        pPrinter = (PLOCAL_PRINTER)pNode->Element;

        pfnPrinterDriverLevels[Level](pPrinter, &pDriverInfo, &pEnd, NULL);
        (*pcReturned)++;
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

/******************************************************************
 * Return the number of bytes for an multi_sz string.
 * The result includes all \0s
 * (specifically the extra \0, that is needed as multi_sz terminator).
 */
static int multi_sz_lenW(const WCHAR *str)
{
    const WCHAR *ptr = str;
    if (!str) return 0;
    do
    {
        ptr += lstrlenW(ptr) + 1;
    } while (*ptr);

    return (ptr - str + 1) * sizeof(WCHAR);
}


/******************************************************************
 * validate_envW [internal]
 *
 * validate the user-supplied printing-environment
 *
 * PARAMS
 *  env  [I] PTR to Environment-String or NULL
 *
 * RETURNS
 *  Success:  PTR to printenv_t
 *  Failure:  NULL and ERROR_INVALID_ENVIRONMENT
 *
 * NOTES
 *  An empty string is handled the same way as NULL.
 *
 */
PPRINTENV_T validate_envW(LPCWSTR env)
{
    PPRINTENV_T result = NULL;
    unsigned int i;

    TRACE("(%s)\n", debugstr_w(env));
    if (env && env[0])
    {
        for (i = 0; i < ARRAYSIZE(all_printenv); i++)
        {
            if (lstrcmpiW(env, all_printenv[i]->envname) == 0)
            {
                result = all_printenv[i];
                break;
            }
        }
        if (result == NULL)
        {
            FIXME("unsupported Environment: %s\n", debugstr_w(env));
            SetLastError(ERROR_INVALID_ENVIRONMENT);
        }
        /* on win9x, only "Windows 4.0" is allowed, but we ignore this */
    }
    else
    {
        result = (GetVersion() & 0x80000000) ? &env_win40 : &env_x86;
    }

    TRACE("=> using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));
    return result;
}

/*****************************************************************************
 * open_driver_reg [internal]
 *
 * opens the registry for the printer drivers depending on the given input
 * variable pEnvironment
 *
 * RETURNS:
 *    Success: the opened hkey
 *    Failure: NULL
 */
HKEY open_driver_reg(LPCWSTR pEnvironment)
{
    HKEY  retval = NULL;
    LPWSTR buffer;
    const PRINTENV_T * env;

    TRACE("(%s)\n", debugstr_w(pEnvironment));

    env = validate_envW(pEnvironment);
    if (!env) return NULL;

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(fmt_driversW) +
                (lstrlenW(env->envname) + lstrlenW(env->versionregpath)) * sizeof(WCHAR));

    if (buffer)
    {
        wsprintfW(buffer, fmt_driversW, env->envname, env->versionregpath);
        RegCreateKeyW(HKEY_LOCAL_MACHINE, buffer, &retval);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    return retval;
}


/******************************************************************************
 * LocalGetPrintProcessorDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Print-Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (this computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  level        [I] Structure-Level (must be 1)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pPPInfo
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 *  Native Values returned in pPPInfo on Success for this computer:
 *| NT(Windows x64):    "%winsysdir%\\spool\\PRTPROCS\\x64"
 *| NT(Windows NT x86): "%winsysdir%\\spool\\PRTPROCS\\w32x86"
 *| NT(Windows 4.0):    "%winsysdir%\\spool\\PRTPROCS\\win40"
 *
 *  "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
BOOL WINAPI LocalGetPrinterDriverDirectory(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD needed;
    const PRINTENV_T * env = NULL;
    WCHAR * const dir = (WCHAR *)pDriverDirectory;

    FIXME("LocalGetPrinterDriverDirectory(%S, %S, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (pName != NULL && pName[0])
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;  /* pEnvironment invalid or unsupported */

    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spoolW);
    needed += lstrlenW(driversW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (dir == NULL)
    {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW( dir, cbBuf / sizeof(WCHAR) );
    /* add the Subdirectories */
    lstrcatW( dir, spoolW );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, driversW );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, env->subdir );
    CreateDirectoryW( dir, NULL );

    FIXME( "=> %s\n", debugstr_w( dir ) );
    return TRUE;
}

/******************************************************************
 *  apd_copyfile [internal]
 *
 * Copy a file from the driverdirectory to the versioned directory
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL apd_copyfile( WCHAR *pathname, WCHAR *file_part, apd_data_t *apd )
{
    WCHAR *srcname;
    BOOL res;

    apd->src[apd->srclen] = '\0';
    apd->dst[apd->dstlen] = '\0';

    if (!pathname || !pathname[0]) {
        /* nothing to copy */
        return TRUE;
    }

    if (apd->copyflags & APD_COPY_FROM_DIRECTORY)
        srcname = pathname;
    else
    {
        srcname = apd->src;
        lstrcatW( srcname, file_part );
    }
    lstrcatW( apd->dst, file_part );

    FIXME("%s => %s\n", debugstr_w(srcname), debugstr_w(apd->dst));

    /* FIXME: handle APD_COPY_NEW_FILES */
    res = CopyFileW(srcname, apd->dst, FALSE);
    FIXME("got %d with %u\n", res, GetLastError());

    return apd->lazy || res;
}

/******************************************************************
 * driver_load [internal]
 *
 * load a driver user interface dll
 *
 * On failure, NULL is returned
 *
 */

static HMODULE driver_load(const PRINTENV_T * env, LPWSTR dllname)
{
    WCHAR fullname[MAX_PATH];
    HMODULE hui;
    DWORD len;

    FIXME("(%p, %s)\n", env, debugstr_w(dllname));

    /* build the driverdir */
    len = sizeof(fullname) - (lstrlenW(env->versionsubdir) + 1 + lstrlenW(dllname) + 1) * sizeof(WCHAR);

    if (!LocalGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1, (LPBYTE) fullname, len, &len))
    {
        /* Should never fail */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    lstrcatW(fullname, env->versionsubdir);
    lstrcatW(fullname, backslashW);
    lstrcatW(fullname, dllname);

    hui = LoadLibraryW(fullname);
    FIXME("%p: LoadLibrary(%s) %d\n", hui, debugstr_w(fullname), GetLastError());

    return hui;
}

static inline WCHAR *get_file_part( WCHAR *name )
{
    WCHAR *ptr = wcsrchr( name, '\\' );
    if (ptr) return ptr + 1;
    return name;
}

/******************************************************************************
 *  myAddPrinterDriverEx [internal]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 * and a special mode with lazy error checking.
 *
 */
BOOL myAddPrinterDriverEx(DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags, BOOL lazy)
{
    const PRINTENV_T *env;
    apd_data_t apd;
    DRIVER_INFO_8W di;
    BOOL    (WINAPI *pDrvDriverEvent)(DWORD, DWORD, LPBYTE, LPARAM);
    HMODULE hui;
    WCHAR *file;
    HKEY    hroot;
    HKEY    hdrv;
    DWORD   disposition;
    DWORD   len;
    LONG    lres;
    BOOL    res;

    /* we need to set all entries in the Registry, independent from the Level of
       DRIVER_INFO, that the caller supplied */

    ZeroMemory(&di, sizeof(di));
    if (pDriverInfo && (level < ARRAYSIZE(di_sizeof)))
    {
        memcpy(&di, pDriverInfo, di_sizeof[level]);
    }

    /* dump the most used infos */
    FIXME("%p: .cVersion    : 0x%x/%d\n", pDriverInfo, di.cVersion, di.cVersion);
    FIXME("%p: .pName       : %s\n", di.pName, debugstr_w(di.pName));
    FIXME("%p: .pEnvironment: %s\n", di.pEnvironment, debugstr_w(di.pEnvironment));
    FIXME("%p: .pDriverPath : %s\n", di.pDriverPath, debugstr_w(di.pDriverPath));
    FIXME("%p: .pDataFile   : %s\n", di.pDataFile, debugstr_w(di.pDataFile));
    FIXME("%p: .pConfigFile : %s\n", di.pConfigFile, debugstr_w(di.pConfigFile));
    FIXME("%p: .pHelpFile   : %s\n", di.pHelpFile, debugstr_w(di.pHelpFile));
    /* dump only the first of the additional Files */
    FIXME("%p: .pDependentFiles: %s\n", di.pDependentFiles, debugstr_w(di.pDependentFiles));


    /* check environment */
    env = validate_envW(di.pEnvironment);
    if (env == NULL) return FALSE;        /* ERROR_INVALID_ENVIRONMENT */

    /* fill the copy-data / get the driverdir */
    len = sizeof(apd.src) - sizeof(version3_subdirW) - sizeof(WCHAR);
    if (!LocalGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1, (LPBYTE) apd.src, len, &len))
    {
        /* Should never fail */
        return FALSE;
    }
    memcpy(apd.dst, apd.src, len);
    lstrcatW(apd.src, backslashW);
    apd.srclen = lstrlenW(apd.src);
    lstrcatW(apd.dst, env->versionsubdir);
    lstrcatW(apd.dst, backslashW);
    apd.dstlen = lstrlenW(apd.dst);
    apd.copyflags = dwFileCopyFlags;
    apd.lazy = lazy;
    CreateDirectoryW(apd.src, NULL);
    CreateDirectoryW(apd.dst, NULL);

    hroot = open_driver_reg(env->envname);
    if (!hroot)
    {
        ERR("Can't create Drivers key\n");
        return FALSE;
    }

    /* Fill the Registry for the Driver */
    if ((lres = RegCreateKeyExW(hroot, di.pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_QUERY_VALUE, NULL,
                                &hdrv, &disposition)) != ERROR_SUCCESS)
    {
        ERR("can't create driver %s: %u\n", debugstr_w(di.pName), lres);
        RegCloseKey(hroot);
        SetLastError(lres);
        return FALSE;
    }
    RegCloseKey(hroot);

    /* Verified with the Adobe PS Driver, that w2k does not use di.Version */
    RegSetValueExW(hdrv, versionW, 0, REG_DWORD, (const BYTE*) &env->driverversion,
                   sizeof(DWORD));

    file = get_file_part( di.pDriverPath );
    RegSetValueExW( hdrv, driverW, 0, REG_SZ, (LPBYTE)file, (lstrlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDriverPath, file, &apd );

    file = get_file_part( di.pDataFile );
    RegSetValueExW( hdrv, data_fileW, 0, REG_SZ, (LPBYTE)file, (lstrlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDataFile, file, &apd );

    file = get_file_part( di.pConfigFile );
    RegSetValueExW( hdrv, configuration_fileW, 0, REG_SZ, (LPBYTE)file, (lstrlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pConfigFile, file, &apd );

    /* settings for level 3 */
    if (di.pHelpFile)
    {
        file = get_file_part( di.pHelpFile );
        RegSetValueExW( hdrv, help_fileW, 0, REG_SZ, (LPBYTE)file, (lstrlenW( file ) + 1) * sizeof(WCHAR) );
        apd_copyfile( di.pHelpFile, file, &apd );
    }
    else
        RegSetValueExW( hdrv, help_fileW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW) );

    if (di.pDependentFiles && *di.pDependentFiles)
    {
        WCHAR *reg, *reg_ptr, *in_ptr;
        reg = reg_ptr = HeapAlloc( GetProcessHeap(), 0, multi_sz_lenW( di.pDependentFiles ) );

        for (in_ptr = di.pDependentFiles; *in_ptr; in_ptr += lstrlenW( in_ptr ) + 1)
        {
            file = get_file_part( in_ptr );
            len = lstrlenW( file ) + 1;
            memcpy( reg_ptr, file, len * sizeof(WCHAR) );
            reg_ptr += len;
            apd_copyfile( in_ptr, file, &apd );
        }
        *reg_ptr = 0;

        RegSetValueExW( hdrv, dependent_filesW, 0, REG_MULTI_SZ, (LPBYTE)reg, (reg_ptr - reg + 1) * sizeof(WCHAR) );
        HeapFree( GetProcessHeap(), 0, reg );
    }
    else
        RegSetValueExW(hdrv, dependent_filesW, 0, REG_MULTI_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    /* The language-Monitor was already copied by the caller to "%SystemRoot%\system32" */
    if (di.pMonitorName)
        RegSetValueExW(hdrv, monitorW, 0, REG_SZ, (LPBYTE) di.pMonitorName,
                       (lstrlenW(di.pMonitorName)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, monitorW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    if (di.pDefaultDataType)
        RegSetValueExW(hdrv, datatypeW, 0, REG_SZ, (LPBYTE) di.pDefaultDataType,
                       (lstrlenW(di.pDefaultDataType)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, datatypeW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    /* settings for level 4 */
    if (di.pszzPreviousNames)
        RegSetValueExW(hdrv, previous_namesW, 0, REG_MULTI_SZ, (LPBYTE) di.pszzPreviousNames,
                       multi_sz_lenW(di.pszzPreviousNames));
    else
        RegSetValueExW(hdrv, previous_namesW, 0, REG_MULTI_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    if (level > 5) FIXME("level %u for Driver %s is incomplete\n", level, debugstr_w(di.pName));

    RegCloseKey(hdrv);

    //
    // Locate driver and send the event.
    //

    hui = driver_load(env, di.pConfigFile);

    pDrvDriverEvent = (void *)GetProcAddress(hui, "DrvDriverEvent");

    if (hui && pDrvDriverEvent)
    {
        /* Support for DrvDriverEvent is optional */
        TRACE("DRIVER_EVENT_INITIALIZE for %s (%s)\n", debugstr_w(di.pName), debugstr_w(di.pConfigFile));
        /* MSDN: level for DRIVER_INFO is 1 to 3 */
        res = pDrvDriverEvent(DRIVER_EVENT_INITIALIZE, 3, (LPBYTE) &di, 0);
        TRACE("got %d from DRIVER_EVENT_INITIALIZE\n", res);
    }
    FreeLibrary(hui);

    FIXME("=> TRUE with %u\n", GetLastError());
    return TRUE;
}

/******************************************************************************
 * AddPrinterDriverEx [exported through PRINTPROVIDOR]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 *
 * PARAMS
 *  pName           [I] Servername or NULL (local Computer)
 *  level           [I] Level for the supplied DRIVER_INFO_*W struct
 *  pDriverInfo     [I] PTR to DRIVER_INFO_*W struct with the Driver Parameter
 *  dwFileCopyFlags [I] How to Copy / Upgrade / Downgrade the needed Files
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
BOOL WINAPI LocalAddPrinterDriverEx(LPWSTR pName, DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    LONG lres;

    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);

    lres = copy_servername_from_name(pName, NULL);

    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if ((dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY) != APD_COPY_ALL_FILES)
    {
        TRACE("Flags 0x%x ignored (using APD_COPY_ALL_FILES)\n", dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY);
    }

    return myAddPrinterDriverEx(level, pDriverInfo, dwFileCopyFlags, TRUE);
}

BOOL WINAPI LocalAddPrinterDriver(LPWSTR pName, DWORD level, LPBYTE pDriverInfo)
{
    LONG lres;

    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_w(pName), level, pDriverInfo);

    lres = copy_servername_from_name(pName, NULL);

    if (lres)
    {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    // Should be APD_COPY_NEW_FILES. Cheap wine.

    return myAddPrinterDriverEx(level, pDriverInfo, APD_COPY_NEW_FILES, TRUE);
}
