#pragma once

HANDLE
FASTCALL
UserGetProp(
    _In_ PWND Window,
    _In_ ATOM Atom,
    _In_ BOOLEAN SystemProp);

HANDLE
FASTCALL
UserRemoveProp(
    _In_ PWND Window,
    _In_ ATOM Atom,
    _In_ BOOLEAN SystemProp);

_Success_(return)
BOOL
FASTCALL
UserSetProp(
    _In_ PWND Window,
    _In_ ATOM Atom,
    _In_ HANDLE Data,
    _In_ BOOLEAN SystemProp);

VOID
FASTCALL
UserRemoveWindowProps(
    _In_ PWND Window);
