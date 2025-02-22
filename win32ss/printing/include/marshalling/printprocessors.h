/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for DATATYPES_INFO_* and PRINTPROCESSOR_INFO_*
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

static const MARSHALLING DatatypesInfo1Marshalling = {
    sizeof(DATATYPES_INFO_1W),
    {
        { FIELD_OFFSET(DATATYPES_INFO_1W, pName), RTL_FIELD_SIZE(DATATYPES_INFO_1W, pName), RTL_FIELD_SIZE(DATATYPES_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING PrintProcessorInfo1Marshalling = {
    sizeof(PRINTPROCESSOR_INFO_1W),
    {
        { FIELD_OFFSET(PRINTPROCESSOR_INFO_1W, pName), RTL_FIELD_SIZE(PRINTPROCESSOR_INFO_1W, pName), RTL_FIELD_SIZE(PRINTPROCESSOR_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};
