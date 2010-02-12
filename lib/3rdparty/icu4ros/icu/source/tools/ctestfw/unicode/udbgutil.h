/*
************************************************************************
* Copyright (c) 2007, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/

/** C Utilities to aid in debugging **/

#ifndef _UDBGUTIL_H
#define _UDBGUTIL_H

#include "unicode/testtype.h"
#include "unicode/utypes.h"


enum UDebugEnumType {
    UDBG_UDebugEnumType = 0, /* Self-referential, strings for UDebugEnumType. Count=ENUM_COUNT. */
    UDBG_UCalendarDateFields, /* UCalendarDateFields. Count=UCAL_FIELD_COUNT.  Unsupported if UCONFIG_NO_FORMATTING. */
    UDBG_UCalendarMonths, /* UCalendarMonths. Count= (UCAL_UNDECIMBER+1) */
    UDBG_UDateFormatStyle, /* Count = UDAT_SHORT=1 */
    UDBG_ENUM_COUNT
};

typedef enum UDebugEnumType UDebugEnumType;

/**
 * @param type the type of enum
 * Print how many enums are contained for this type. 
 * Should be equal to the appropriate _COUNT constant or there is an error. Return -1 if unsupported.
 */
T_CTEST_API int32_t T_CTEST_EXPORT2 udbg_enumCount(UDebugEnumType type);

/**
 * Convert an enum to a string
 * @param type type of enum
 * @param field field number
 * @return string of the format "ERA", "YEAR", etc, or NULL if out of range or unsupported
 */
T_CTEST_API const char * T_CTEST_EXPORT2 udbg_enumName(UDebugEnumType type, int32_t field);

/**
 * for consistency checking
 * @param type the type of enum
 * Print how many enums should be contained for this type. 
 * This is equal to the appropriate _COUNT constant or there is an error. Returns -1 if unsupported.
 */
T_CTEST_API int32_t T_CTEST_EXPORT2 udbg_enumExpectedCount(UDebugEnumType type);

/**
 * For consistency checking, returns the expected enum ordinal value for the given index value. 
 * @param type which type
 * @param field field number
 * @return should be equal to 'field' or -1 if out of range.
 */
T_CTEST_API int32_t T_CTEST_EXPORT2 udbg_enumArrayValue(UDebugEnumType type, int32_t field);

#endif
