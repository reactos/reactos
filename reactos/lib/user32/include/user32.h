/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/user32.h
 * PURPOSE:     Global user32 definitions
 */
#include <windows.h>
#include <win32k/win32k.h>

/* see lib/user32/misc/devmode.c */
void
USER32_DevModeA2W ( LPDEVMODEW pW, const LPDEVMODEA pA );
