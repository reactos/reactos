/*
 * Windows hook functions
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *                 1996 Andrew Lewycky
 *
 * Based on investigations by Alex Korobka
 */

/***********************************************************************
 *           HOOK_CallHooks32W
 *
 * Call a hook chain.
 */
#include <windows.h>

LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
}

LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
}

WINBOOL HOOK_IsHooked( INT id )
{
	return FALSE;
}