/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for PORT_INFO_*
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

static const MARSHALLING PortInfo1Marshalling = {
    sizeof(PORT_INFO_1W),
    {
        { FIELD_OFFSET(PORT_INFO_1W, pName), RTL_FIELD_SIZE(PORT_INFO_1W, pName), RTL_FIELD_SIZE(PORT_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PortInfo2Marshalling = {
    sizeof(PORT_INFO_2W),
    {
        { FIELD_OFFSET(PORT_INFO_2W, pPortName), RTL_FIELD_SIZE(PORT_INFO_2W, pPortName), RTL_FIELD_SIZE(PORT_INFO_2W, pPortName), TRUE },
        { FIELD_OFFSET(PORT_INFO_2W, pMonitorName), RTL_FIELD_SIZE(PORT_INFO_2W, pMonitorName), RTL_FIELD_SIZE(PORT_INFO_2W, pMonitorName), TRUE },
        { FIELD_OFFSET(PORT_INFO_2W, pDescription), RTL_FIELD_SIZE(PORT_INFO_2W, pDescription), RTL_FIELD_SIZE(PORT_INFO_2W, pDescription), TRUE },
        { FIELD_OFFSET(PORT_INFO_2W, fPortType), RTL_FIELD_SIZE(PORT_INFO_2W, fPortType), RTL_FIELD_SIZE(PORT_INFO_2W, fPortType), FALSE },
        { FIELD_OFFSET(PORT_INFO_2W, Reserved), RTL_FIELD_SIZE(PORT_INFO_2W, Reserved), RTL_FIELD_SIZE(PORT_INFO_2W, Reserved), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pPortInfoMarshalling[] = {
    NULL,
    &PortInfo1Marshalling,
    &PortInfo2Marshalling
};
