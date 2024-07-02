/*
 * Theming - Initialization
 *
 * Copyright (c) 2005 by Frank Richter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(theming);

typedef LRESULT (CALLBACK* THEMING_SUBCLASSPROC)(HWND, UINT, WPARAM, LPARAM,
    ULONG_PTR);

#ifndef __REACTOS__ /* r73803 */
extern LRESULT CALLBACK THEMING_DialogSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                    ULONG_PTR) DECLSPEC_HIDDEN;
#endif
extern LRESULT CALLBACK THEMING_ScrollbarSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                       ULONG_PTR) DECLSPEC_HIDDEN;

#ifndef __REACTOS__
static const WCHAR dialogClass[] = {'#','3','2','7','7','0',0};
#endif

static const struct ThemingSubclass
{
    const WCHAR* className;
    THEMING_SUBCLASSPROC subclassProc;
} subclasses[] = {
    /* Note: list must be sorted by class name */
#ifndef __REACTOS__ /* r73803 & r73871 */
    {dialogClass,          THEMING_DialogSubclassProc},
#endif
    {WC_SCROLLBARW,        THEMING_ScrollbarSubclassProc}
};

#define NUM_SUBCLASSES        (ARRAY_SIZE(subclasses))

static WNDPROC originalProcs[NUM_SUBCLASSES];
static ATOM atRefDataProp;
static ATOM atSubclassProp;

/* Generate a number of subclass window procs.
 * With a single proc alone, we can't really reliably find out the superclass,
 * so have one for each subclass. The subclass number is also stored in a prop
 * since it's needed by THEMING_CallOriginalClass(). Then, the subclass
 * proc and ref data are fetched and the proc called.
 */
#define MAKE_SUBCLASS_PROC(N)                                               \
static LRESULT CALLBACK subclass_proc ## N (HWND wnd, UINT msg,             \
                                            WPARAM wParam, LPARAM lParam)   \
{                                                                           \
    LRESULT result;                                                         \
    ULONG_PTR refData;                                                      \
    SetPropW (wnd, (LPCWSTR)MAKEINTATOM(atSubclassProp), (HANDLE)N);        \
    refData = (ULONG_PTR)GetPropW (wnd, (LPCWSTR)MAKEINTATOM(atRefDataProp)); \
    TRACE ("%d; (%p, %x, %lx, %lx, %lx)\n", N, wnd, msg, wParam, lParam,     \
        refData);                                                           \
    result = subclasses[N].subclassProc (wnd, msg, wParam, lParam, refData);\
    TRACE ("result = %lx\n", result);                                       \
    return result;                                                          \
}

MAKE_SUBCLASS_PROC(0)
#ifndef __REACTOS__
MAKE_SUBCLASS_PROC(1)
#endif

static const WNDPROC subclassProcs[NUM_SUBCLASSES] = {
    subclass_proc0,
#ifndef __REACTOS__
    subclass_proc1,
#endif
};

/***********************************************************************
 * THEMING_Initialize
 *
 * Register classes for standard controls that will shadow the system
 * classes.
 */
#ifdef __REACTOS__ /* r73803 */
void THEMING_Initialize(HANDLE hActCtx5, HANDLE hActCtx6)
#else
void THEMING_Initialize (void)
#endif
{
    unsigned int i;
    static const WCHAR subclassPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','S','u','b','C','l',0 };
    static const WCHAR refDataPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','D','a','t','a',0 };
#ifdef __REACTOS__ /* r73803 */
    ULONG_PTR ulCookie;
    BOOL ret, bActivated;
#else
    if (!IsThemeActive()) return;
#endif

    atSubclassProp = GlobalAddAtomW (subclassPropName);
    atRefDataProp = GlobalAddAtomW (refDataPropName);

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        WNDCLASSEXW class;

        class.cbSize = sizeof(class);

#ifdef __REACTOS__ /* r73803 */
        bActivated = ActivateActCtx(hActCtx5, &ulCookie);
        ret = GetClassInfoExW (NULL, subclasses[i].className, &class);
        if (bActivated)
            DeactivateActCtx(0, ulCookie);

        if (!ret)
#else
        if (!GetClassInfoExW (NULL, subclasses[i].className, &class))
#endif
        {
            ERR("Could not retrieve information for class %s\n",
                debugstr_w (subclasses[i].className));
            continue;
        }
        originalProcs[i] = class.lpfnWndProc;
        class.lpfnWndProc = subclassProcs[i];
#ifdef __REACTOS__ /* r73803 */
        class.style |= CS_GLOBALCLASS;
        class.hInstance = COMCTL32_hModule;
#endif
        
        if (!class.lpfnWndProc)
        {
            ERR("Missing proc for class %s\n", 
                debugstr_w (subclasses[i].className));
            continue;
        }

#ifdef __REACTOS__ /* r73803 */
        bActivated = ActivateActCtx(hActCtx6, &ulCookie);
#endif
        if (!RegisterClassExW (&class))
        {
#ifdef __REACTOS__ /* r73803 */
            WARN("Could not re-register class %s: %x\n",
#else
            ERR("Could not re-register class %s: %x\n",
#endif
                debugstr_w (subclasses[i].className), GetLastError ());
        }
        else
        {
            TRACE("Re-registered class %s\n", 
                debugstr_w (subclasses[i].className));
        }

#ifdef __REACTOS__ /* r73803 */
        if (bActivated)
            DeactivateActCtx(0, ulCookie);
#endif
    }
}

/***********************************************************************
 * THEMING_Uninitialize
 *
 * Unregister shadow classes for standard controls.
 */
void THEMING_Uninitialize (void)
{
    unsigned int i;

    if (!atSubclassProp) return;  /* not initialized */

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        UnregisterClassW (subclasses[i].className, NULL);
    }
}

/***********************************************************************
 * THEMING_CallOriginalClass
 *
 * Determines the original window proc and calls it.
 */
LRESULT THEMING_CallOriginalClass (HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR subclass = (INT_PTR)GetPropW (wnd, (LPCWSTR)MAKEINTATOM(atSubclassProp));
    WNDPROC oldProc = originalProcs[subclass];
    return CallWindowProcW (oldProc, wnd, msg, wParam, lParam);
}
