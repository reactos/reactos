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
        { FIELD_OFFSET(DRIVER_INFO_4W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_4W, pszzPreviousNames), RTL_FIELD_SIZE(DRIVER_INFO_4W, pDefaultDataType), TRUE },
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


static const MARSHALLING* pPrinterDriverMarshalling[] = {
    NULL,
    &PrinterDriver1Marshalling,
    &PrinterDriver2Marshalling,
    &PrinterDriver3Marshalling,
    &PrinterDriver4Marshalling,
    &PrinterDriver5Marshalling,
};
