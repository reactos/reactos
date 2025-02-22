/*
 * PROJECT:     ReactOS TimeZone Utilities Library
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Provides time-zone utility wrappers around Win32 functions,
 *              that are used by different ReactOS modules such as
 *              timedate.cpl, syssetup.dll.
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2016 Carlo Bramini
 *              Copyright 2020 Hermes Belusca-Maito
 */

#pragma once

typedef struct _REG_TZI_FORMAT
{
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT, *PREG_TZI_FORMAT;

typedef LONG
(*PENUM_TIMEZONE_CALLBACK)(
    IN HKEY hZoneKey,
    IN PVOID Context OPTIONAL);

BOOL
GetTimeZoneListIndex(
    IN OUT PULONG pIndex);

LONG
QueryTimeZoneData(
    IN HKEY hZoneKey,
    OUT PULONG Index OPTIONAL,
    OUT PREG_TZI_FORMAT TimeZoneInfo,
    OUT PWCHAR Description OPTIONAL,
    IN OUT PULONG DescriptionSize OPTIONAL,
    OUT PWCHAR StandardName OPTIONAL,
    IN OUT PULONG StandardNameSize OPTIONAL,
    OUT PWCHAR DaylightName OPTIONAL,
    IN OUT PULONG DaylightNameSize OPTIONAL);

VOID
EnumerateTimeZoneList(
    IN PENUM_TIMEZONE_CALLBACK Callback,
    IN PVOID Context OPTIONAL);

// Returns TRUE if AutoDaylight is ON.
// Returns FALSE if AutoDaylight is OFF.
BOOL
GetAutoDaylight(VOID);

VOID
SetAutoDaylight(
    IN BOOL EnableAutoDaylightTime);
