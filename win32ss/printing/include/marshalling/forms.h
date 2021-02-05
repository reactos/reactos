/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions for FORM_INFO_*
 * COPYRIGHT:   Copyright 1998-2020 ReactOS
 */

static const MARSHALLING FormInfo1Marshalling = {
    sizeof(FORM_INFO_1W),
    {
        { FIELD_OFFSET(FORM_INFO_1W, Flags), RTL_FIELD_SIZE(FORM_INFO_1W, Flags), RTL_FIELD_SIZE(FORM_INFO_1W, Flags), FALSE },
        { FIELD_OFFSET(FORM_INFO_1W, pName), RTL_FIELD_SIZE(FORM_INFO_1W, pName), RTL_FIELD_SIZE(FORM_INFO_1W, pName), TRUE },
        { FIELD_OFFSET(FORM_INFO_1W, Size), RTL_FIELD_SIZE(FORM_INFO_1W, Size), RTL_FIELD_SIZE(FORM_INFO_1W, Size), FALSE },
        { FIELD_OFFSET(FORM_INFO_1W, ImageableArea), RTL_FIELD_SIZE(FORM_INFO_1W, ImageableArea), RTL_FIELD_SIZE(FORM_INFO_1W, ImageableArea), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING FormInfo2Marshalling = {
    sizeof(FORM_INFO_2W),
    {
        { FIELD_OFFSET(FORM_INFO_2W, Flags), RTL_FIELD_SIZE(FORM_INFO_2W, Flags), RTL_FIELD_SIZE(FORM_INFO_2W, Flags), FALSE },
        { FIELD_OFFSET(FORM_INFO_2W, pName), RTL_FIELD_SIZE(FORM_INFO_2W, pName), RTL_FIELD_SIZE(FORM_INFO_2W, pName), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, Size), RTL_FIELD_SIZE(FORM_INFO_2W, Size), RTL_FIELD_SIZE(FORM_INFO_2W, Size), FALSE },
        { FIELD_OFFSET(FORM_INFO_2W, ImageableArea), RTL_FIELD_SIZE(FORM_INFO_2W, ImageableArea), RTL_FIELD_SIZE(FORM_INFO_2W, ImageableArea), FALSE },
        { FIELD_OFFSET(FORM_INFO_2W, pKeyword), RTL_FIELD_SIZE(FORM_INFO_2W, pKeyword), RTL_FIELD_SIZE(FORM_INFO_2W, pKeyword), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, StringType), RTL_FIELD_SIZE(FORM_INFO_2W, StringType), RTL_FIELD_SIZE(FORM_INFO_2W, StringType), FALSE },
        { FIELD_OFFSET(FORM_INFO_2W, pMuiDll), RTL_FIELD_SIZE(FORM_INFO_2W, pMuiDll), RTL_FIELD_SIZE(FORM_INFO_2W, pMuiDll), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, dwResourceId), RTL_FIELD_SIZE(FORM_INFO_2W, dwResourceId), RTL_FIELD_SIZE(FORM_INFO_2W, dwResourceId), FALSE },
        { FIELD_OFFSET(FORM_INFO_2W, pDisplayName), RTL_FIELD_SIZE(FORM_INFO_2W, pDisplayName), RTL_FIELD_SIZE(FORM_INFO_2W, pDisplayName), TRUE },
        { FIELD_OFFSET(FORM_INFO_2W, wLangId), RTL_FIELD_SIZE(FORM_INFO_2W, wLangId), RTL_FIELD_SIZE(FORM_INFO_2W, wLangId), FALSE },
        { MAXDWORD, 0, 0, FALSE }
    }
};

static const MARSHALLING* pFormInfoMarshalling[] = {
    NULL,
    &FormInfo1Marshalling,
    &FormInfo2Marshalling
};
