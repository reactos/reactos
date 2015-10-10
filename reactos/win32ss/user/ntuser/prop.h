#pragma once

HANDLE
FASTCALL
UserGetProp(
    _In_ PWND Window,
    _In_ ATOM Atom);

HANDLE
FASTCALL
UserRemoveProp(
    _In_ PWND Window,
    _In_ ATOM Atom);

_Success_(return)
BOOL
FASTCALL
UserSetProp(
    _In_ PWND Window,
    _In_ ATOM Atom,
    _In_ HANDLE Data);

VOID
FASTCALL
UserRemoveWindowProps(
    _In_ PWND Window);
