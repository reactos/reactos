/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for FORM_INFO_*
 * COPYRIGHT:   Copyright 1998-2020 ReactOS
 */

static const MARSHALLING FormInfo1Marshalling = {
    sizeof(FORM_INFO_1W),
    {
        { FIELD_OFFSET(FORM_INFO_1W, pName), RTL_FIELD_SIZE(FORM_INFO_1W, pName), RTL_FIELD_SIZE(FORM_INFO_1W, pName), TRUE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING FormInfo2Marshalling = {
    sizeof(FORM_INFO_2W),
    {
        { FIELD_OFFSET(FORM_INFO_2W, pName), RTL_FIELD_SIZE(FORM_INFO_2W, pName), RTL_FIELD_SIZE(FORM_INFO_2W, pName), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, pKeyword), RTL_FIELD_SIZE(FORM_INFO_2W, pKeyword), RTL_FIELD_SIZE(FORM_INFO_2W, pKeyword), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, pMuiDll), RTL_FIELD_SIZE(FORM_INFO_2W, pMuiDll), RTL_FIELD_SIZE(FORM_INFO_2W, pMuiDll), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, pDisplayName), RTL_FIELD_SIZE(FORM_INFO_2W, pDisplayName), RTL_FIELD_SIZE(FORM_INFO_2W, pDisplayName), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pFormInfoMarshalling[] = {
    NULL,
    &FormInfo1Marshalling,
    &FormInfo2Marshalling
};
