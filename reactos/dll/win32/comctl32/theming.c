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

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(theming);

typedef LRESULT (CALLBACK* THEMING_SUBCLASSPROC)(HWND, UINT, WPARAM, LPARAM,
    ULONG_PTR);

extern LRESULT CALLBACK THEMING_ComboSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                   ULONG_PTR) DECLSPEC_HIDDEN;
extern LRESULT CALLBACK THEMING_EditSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                  ULONG_PTR) DECLSPEC_HIDDEN;
extern LRESULT CALLBACK THEMING_ListBoxSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                     ULONG_PTR) DECLSPEC_HIDDEN;
extern LRESULT CALLBACK THEMING_ScrollbarSubclassProc (HWND, UINT, WPARAM, LPARAM,
                                                       ULONG_PTR) DECLSPEC_HIDDEN;

static const WCHAR dialogClass[] = {'#','3','2','7','7','0',0};
static const WCHAR comboLboxClass[] = {'C','o','m','b','o','L','b','o','x',0};

static const struct ThemingSubclass
{
    const WCHAR* className;
    THEMING_SUBCLASSPROC subclassProc;
} subclasses[] = {
    /* Note: list must be sorted by class name */
    {WC_COMBOBOXW,         THEMING_ComboSubclassProc},
    {comboLboxClass,       THEMING_ListBoxSubclassProc},
    {WC_EDITW,             THEMING_EditSubclassProc},
    {WC_LISTBOXW,          THEMING_ListBoxSubclassProc},
    {WC_SCROLLBARW,        THEMING_ScrollbarSubclassProc}
};

#define NUM_SUBCLASSES        (sizeof(subclasses)/sizeof(subclasses[0]))

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
MAKE_SUBCLASS_PROC(1)
MAKE_SUBCLASS_PROC(2)
MAKE_SUBCLASS_PROC(3)
MAKE_SUBCLASS_PROC(4)

static const WNDPROC subclassProcs[NUM_SUBCLASSES] = {
    subclass_proc0,
    subclass_proc1,
    subclass_proc2,
    subclass_proc3,
    subclass_proc4
};

/***********************************************************************
 * THEMING_Initialize
 *
 * Register classes for standard controls that will shadow the system
 * classes.
 */
void THEMING_Initialize (HANDLE hActCtx5, HANDLE hActCtx6)
{
    unsigned int i;
    static const WCHAR subclassPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','S','u','b','C','l',0 };
    static const WCHAR refDataPropName[] = 
        { 'C','C','3','2','T','h','e','m','i','n','g','D','a','t','a',0 };
    ULONG_PTR ulCookie;
    BOOL ret, bActivated;

    atSubclassProp = GlobalAddAtomW (subclassPropName);
    atRefDataProp = GlobalAddAtomW (refDataPropName);

    for (i = 0; i < NUM_SUBCLASSES; i++)
    {
        WNDCLASSEXW class;

        class.cbSize = sizeof(class);

        bActivated = ActivateActCtx(hActCtx5, &ulCookie);
        ret = GetClassInfoExW (NULL, subclasses[i].className, &class);
        if (bActivated)
            DeactivateActCtx(0, ulCookie);

        if (!ret)
        {
            ERR("Could not retrieve information for class %s\n",
                debugstr_w (subclasses[i].className));
            continue;
        }
        originalProcs[i] = class.lpfnWndProc;
        class.lpfnWndProc = subclassProcs[i];
        class.style |= CS_GLOBALCLASS;
        class.hInstance = COMCTL32_hModule;

        if (!class.lpfnWndProc)
        {
            ERR("Missing proc for class %s\n", 
                debugstr_w (subclasses[i].className));
            continue;
        }

        bActivated = ActivateActCtx(hActCtx6, &ulCookie);

        if (!RegisterClassExW (&class))
        {
            WARN("Could not re-register class %s: %x\n",
                debugstr_w (subclasses[i].className), GetLastError ());
        }
        else
        {
            TRACE("Re-registered class %s\n", 
                debugstr_w (subclasses[i].className));
        }

        if (bActivated)
            DeactivateActCtx(0, ulCookie);
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

/***********************************************************************
 * THEMING_SetSubclassData
 *
 * Update the "refData" value of the subclassed window.
 */
void THEMING_SetSubclassData (HWND wnd, ULONG_PTR refData)
{
    SetPropW (wnd, (LPCWSTR)MAKEINTATOM(atRefDataProp), (HANDLE)refData);
}
