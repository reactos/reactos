/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/user32.h
 * PURPOSE:     Global user32 definitions
 */
#define WINVER 0x1000
#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <user32/wininternal.h>
#include <user32/callback.h>
#include <win32k/win32k.h>
#include <window.h>
#include <debug.h>

#define SLOWORD(l) ((SHORT)((LONG)(l)))
#define SHIWORD(l) ((SHORT)(((LONG)(l) >> 16) & 0xFFFF))

extern HANDLE ProcessHeap;
VOID
User32FreeHeap(PVOID Block);
PVOID
User32AllocHeap(ULONG Size);
VOID
User32ConvertUnicodeString(PWSTR SrcString, PSTR DestString, ULONG DestSize);
PWSTR
User32ConvertString(PCSTR String);
VOID
User32FreeString(PWSTR String);
