#pragma once

PPROPERTY
FASTCALL
IntGetProp(
    _In_ PWND Window,
    _In_ ATOM Atom);

HANDLE
FASTCALL
UserGetProp(
    _In_ PWND Window,
    _In_ ATOM Atom);

HANDLE
FASTCALL
IntRemoveProp(
    _In_ PWND Window,
    _In_ ATOM Atom);

_Success_(return)
BOOL
FASTCALL
IntSetProp(
    _In_ PWND Window,
    _In_ ATOM Atom,
    _In_ HANDLE Data);

VOID
FASTCALL
IntRemoveWindowProp(
    _In_ PWND Window);
