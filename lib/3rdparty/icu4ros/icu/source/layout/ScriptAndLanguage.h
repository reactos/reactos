/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __SCRIPTANDLANGUAGE_H
#define __SCRIPTANDLANGUAGE_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

typedef TagAndOffsetRecord LangSysRecord;

struct LangSysTable
{
    Offset    lookupOrderOffset;
    le_uint16 reqFeatureIndex;
    le_uint16 featureCount;
    le_uint16 featureIndexArray[ANY_NUMBER];
};

struct ScriptTable
{
    Offset              defaultLangSysTableOffset;
    le_uint16           langSysCount;
    LangSysRecord       langSysRecordArray[ANY_NUMBER];

    const LangSysTable  *findLanguage(LETag languageTag, le_bool exactMatch = FALSE) const;
};

typedef TagAndOffsetRecord ScriptRecord;

struct ScriptListTable
{
    le_uint16           scriptCount;
    ScriptRecord        scriptRecordArray[ANY_NUMBER];

    const ScriptTable   *findScript(LETag scriptTag) const;
    const LangSysTable  *findLanguage(LETag scriptTag, LETag languageTag, le_bool exactMatch = FALSE) const;
};

U_NAMESPACE_END
#endif

