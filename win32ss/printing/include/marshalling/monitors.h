/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for MONITOR_INFO_*
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

static const MARSHALLING MonitorInfo1Marshalling = {
    sizeof(MONITOR_INFO_1W),
    {
        { FIELD_OFFSET(MONITOR_INFO_1W, pName), RTL_FIELD_SIZE(MONITOR_INFO_1W, pName), RTL_FIELD_SIZE(MONITOR_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING MonitorInfo2Marshalling = {
    sizeof(MONITOR_INFO_2W),
    {
        { FIELD_OFFSET(MONITOR_INFO_2W, pName), RTL_FIELD_SIZE(MONITOR_INFO_2W, pName), RTL_FIELD_SIZE(MONITOR_INFO_2W, pName), TRUE },
        { FIELD_OFFSET(MONITOR_INFO_2W, pEnvironment), RTL_FIELD_SIZE(MONITOR_INFO_2W, pEnvironment), RTL_FIELD_SIZE(MONITOR_INFO_2W, pEnvironment), TRUE },
        { FIELD_OFFSET(MONITOR_INFO_2W, pDLLName), RTL_FIELD_SIZE(MONITOR_INFO_2W, pDLLName), RTL_FIELD_SIZE(MONITOR_INFO_2W, pDLLName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pMonitorInfoMarshalling[] = {
    NULL,
    &MonitorInfo1Marshalling,
    &MonitorInfo2Marshalling
};
