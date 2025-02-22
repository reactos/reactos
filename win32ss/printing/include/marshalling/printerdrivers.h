/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for DRIVER_INFO_*
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

static const MARSHALLING PrinterDriver1Marshalling = {
    sizeof(DRIVER_INFO_1W),
    {
        { FIELD_OFFSET(DRIVER_INFO_1W, pName), RTL_FIELD_SIZE(DRIVER_INFO_1W, pName), RTL_FIELD_SIZE(DRIVER_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver2Marshalling = {
    sizeof(DRIVER_INFO_2W),
    {
        { FIELD_OFFSET(DRIVER_INFO_2W, pName), RTL_FIELD_SIZE(DRIVER_INFO_2W, pName), RTL_FIELD_SIZE(DRIVER_INFO_2W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_2W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_2W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_2W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_2W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_2W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_2W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_2W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_2W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_2W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_2W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_2W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_2W, pConfigFile), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver3Marshalling = {
    sizeof(DRIVER_INFO_3W),
    {
        { FIELD_OFFSET(DRIVER_INFO_3W, pName), RTL_FIELD_SIZE(DRIVER_INFO_3W, pName), RTL_FIELD_SIZE(DRIVER_INFO_3W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_3W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_3W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pConfigFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_3W, pHelpFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDependentFiles), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_3W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_3W, pMonitorName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_3W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_3W, pDefaultDataType), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver4Marshalling = {
    sizeof(DRIVER_INFO_4W),
    {
        { FIELD_OFFSET(DRIVER_INFO_4W, pName), RTL_FIELD_SIZE(DRIVER_INFO_4W, pName), RTL_FIELD_SIZE(DRIVER_INFO_4W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_4W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_4W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pConfigFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_4W, pHelpFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDependentFiles), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_4W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_4W, pMonitorName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDefaultDataType), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_4W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_4W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_4W, pszzPreviousNames), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver5Marshalling = {
    sizeof(DRIVER_INFO_5W),
    {
        { FIELD_OFFSET(DRIVER_INFO_5W, pName), RTL_FIELD_SIZE(DRIVER_INFO_5W, pName), RTL_FIELD_SIZE(DRIVER_INFO_5W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_5W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_5W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_5W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_5W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_5W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_5W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_5W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_5W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_5W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_5W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_5W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_5W, pConfigFile), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver6Marshalling = {
    sizeof(DRIVER_INFO_6W),
    {
        { FIELD_OFFSET(DRIVER_INFO_6W, pName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_6W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_6W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pConfigFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_6W, pHelpFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDependentFiles), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pMonitorName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_6W, pDefaultDataType), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszzPreviousNames), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pszMfgName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszMfgName), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszMfgName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pszOEMUrl), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszOEMUrl), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszOEMUrl), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pszHardwareID), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszHardwareID), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszHardwareID), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_6W, pszProvider), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszProvider), RTL_FIELD_SIZE(DRIVER_INFO_6W, pszProvider), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrinterDriver8Marshalling = {
    sizeof(DRIVER_INFO_8W),
    {
        { FIELD_OFFSET(DRIVER_INFO_8W, pName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_8W, pEnvironment), RTL_FIELD_SIZE(DRIVER_INFO_8W, pEnvironment), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDriverPath), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDriverPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDataFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDataFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pConfigFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pConfigFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pHelpFile), RTL_FIELD_SIZE(DRIVER_INFO_8W, pHelpFile), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDependentFiles), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDependentFiles), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pMonitorName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pMonitorName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDefaultDataType), RTL_FIELD_SIZE(DRIVER_INFO_8W, pDefaultDataType), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzPreviousNames), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszMfgName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszMfgName), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszMfgName), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszOEMUrl), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszOEMUrl), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszOEMUrl), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszHardwareID), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszHardwareID), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszHardwareID), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszProvider), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszProvider), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszProvider), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszPrintProcessor), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszPrintProcessor), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszPrintProcessor), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszVendorSetup), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszVendorSetup), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszVendorSetup), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszzColorProfiles), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzColorProfiles), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzColorProfiles), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszInfPath), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszInfPath), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszInfPath), TRUE },
        { FIELD_OFFSET(DRIVER_INFO_8W, pszzCoreDriverDependencies), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzCoreDriverDependencies), RTL_FIELD_SIZE(DRIVER_INFO_8W, pszzCoreDriverDependencies), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pPrinterDriverMarshalling[] = {
    NULL,
    &PrinterDriver1Marshalling,
    &PrinterDriver2Marshalling,
    &PrinterDriver3Marshalling,
    &PrinterDriver4Marshalling,
    &PrinterDriver5Marshalling,
    &PrinterDriver6Marshalling,
    NULL,
    &PrinterDriver8Marshalling,
};
