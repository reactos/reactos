
#pragma once

PENTRY
GdiQueryTable(
    VOID);

BOOL
GdiIsHandleValid(
    _In_ HGDIOBJ hobj);

BOOL
GdiIsHandleValidEx(
    _In_ HGDIOBJ hobj,
    _In_ GDILOOBJTYPE ObjectType);

PVOID
GdiGetHandleUserData(
    _In_ HGDIOBJ hobj);

