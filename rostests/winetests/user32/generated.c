/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: Not sure if is possible to do without compiler extension
 *        (if type is not just a name that is, if so the normal)
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_ACCESSTIMEOUT(void)
{
    /* ACCESSTIMEOUT (pack 4) */
    TEST_TYPE(ACCESSTIMEOUT, 12, 4);
    TEST_FIELD(ACCESSTIMEOUT, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(ACCESSTIMEOUT, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(ACCESSTIMEOUT, DWORD, iTimeOutMSec, 8, 4, 4);
}

static void test_pack_ANIMATIONINFO(void)
{
    /* ANIMATIONINFO (pack 4) */
    TEST_TYPE(ANIMATIONINFO, 8, 4);
    TEST_FIELD(ANIMATIONINFO, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(ANIMATIONINFO, INT, iMinAnimate, 4, 4, 4);
}

static void test_pack_CBTACTIVATESTRUCT(void)
{
    /* CBTACTIVATESTRUCT (pack 4) */
    TEST_TYPE(CBTACTIVATESTRUCT, 8, 4);
    TEST_FIELD(CBTACTIVATESTRUCT, BOOL, fMouse, 0, 4, 4);
    TEST_FIELD(CBTACTIVATESTRUCT, HWND, hWndActive, 4, 4, 4);
}

static void test_pack_CBT_CREATEWNDA(void)
{
    /* CBT_CREATEWNDA (pack 4) */
    TEST_TYPE(CBT_CREATEWNDA, 8, 4);
    TEST_FIELD(CBT_CREATEWNDA, CREATESTRUCTA *, lpcs, 0, 4, 4);
    TEST_FIELD(CBT_CREATEWNDA, HWND, hwndInsertAfter, 4, 4, 4);
}

static void test_pack_CBT_CREATEWNDW(void)
{
    /* CBT_CREATEWNDW (pack 4) */
    TEST_TYPE(CBT_CREATEWNDW, 8, 4);
    TEST_FIELD(CBT_CREATEWNDW, CREATESTRUCTW *, lpcs, 0, 4, 4);
    TEST_FIELD(CBT_CREATEWNDW, HWND, hwndInsertAfter, 4, 4, 4);
}

static void test_pack_CLIENTCREATESTRUCT(void)
{
    /* CLIENTCREATESTRUCT (pack 4) */
    TEST_TYPE(CLIENTCREATESTRUCT, 8, 4);
    TEST_FIELD(CLIENTCREATESTRUCT, HMENU, hWindowMenu, 0, 4, 4);
    TEST_FIELD(CLIENTCREATESTRUCT, UINT, idFirstChild, 4, 4, 4);
}

static void test_pack_COMBOBOXINFO(void)
{
    /* COMBOBOXINFO (pack 4) */
    TEST_TYPE(COMBOBOXINFO, 52, 4);
    TEST_FIELD(COMBOBOXINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(COMBOBOXINFO, RECT, rcItem, 4, 16, 4);
    TEST_FIELD(COMBOBOXINFO, RECT, rcButton, 20, 16, 4);
    TEST_FIELD(COMBOBOXINFO, DWORD, stateButton, 36, 4, 4);
    TEST_FIELD(COMBOBOXINFO, HWND, hwndCombo, 40, 4, 4);
    TEST_FIELD(COMBOBOXINFO, HWND, hwndItem, 44, 4, 4);
    TEST_FIELD(COMBOBOXINFO, HWND, hwndList, 48, 4, 4);
}

static void test_pack_COMPAREITEMSTRUCT(void)
{
    /* COMPAREITEMSTRUCT (pack 4) */
    TEST_TYPE(COMPAREITEMSTRUCT, 32, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, UINT, CtlType, 0, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, UINT, CtlID, 4, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, HWND, hwndItem, 8, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, UINT, itemID1, 12, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, ULONG_PTR, itemData1, 16, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, UINT, itemID2, 20, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, ULONG_PTR, itemData2, 24, 4, 4);
    TEST_FIELD(COMPAREITEMSTRUCT, DWORD, dwLocaleId, 28, 4, 4);
}

static void test_pack_COPYDATASTRUCT(void)
{
    /* COPYDATASTRUCT (pack 4) */
    TEST_TYPE(COPYDATASTRUCT, 12, 4);
    TEST_FIELD(COPYDATASTRUCT, ULONG_PTR, dwData, 0, 4, 4);
    TEST_FIELD(COPYDATASTRUCT, DWORD, cbData, 4, 4, 4);
    TEST_FIELD(COPYDATASTRUCT, PVOID, lpData, 8, 4, 4);
}

static void test_pack_CREATESTRUCTA(void)
{
    /* CREATESTRUCTA (pack 4) */
    TEST_TYPE(CREATESTRUCTA, 48, 4);
    TEST_FIELD(CREATESTRUCTA, LPVOID, lpCreateParams, 0, 4, 4);
    TEST_FIELD(CREATESTRUCTA, HINSTANCE, hInstance, 4, 4, 4);
    TEST_FIELD(CREATESTRUCTA, HMENU, hMenu, 8, 4, 4);
    TEST_FIELD(CREATESTRUCTA, HWND, hwndParent, 12, 4, 4);
    TEST_FIELD(CREATESTRUCTA, INT, cy, 16, 4, 4);
    TEST_FIELD(CREATESTRUCTA, INT, cx, 20, 4, 4);
    TEST_FIELD(CREATESTRUCTA, INT, y, 24, 4, 4);
    TEST_FIELD(CREATESTRUCTA, INT, x, 28, 4, 4);
    TEST_FIELD(CREATESTRUCTA, LONG, style, 32, 4, 4);
    TEST_FIELD(CREATESTRUCTA, LPCSTR, lpszName, 36, 4, 4);
    TEST_FIELD(CREATESTRUCTA, LPCSTR, lpszClass, 40, 4, 4);
    TEST_FIELD(CREATESTRUCTA, DWORD, dwExStyle, 44, 4, 4);
}

static void test_pack_CREATESTRUCTW(void)
{
    /* CREATESTRUCTW (pack 4) */
    TEST_TYPE(CREATESTRUCTW, 48, 4);
    TEST_FIELD(CREATESTRUCTW, LPVOID, lpCreateParams, 0, 4, 4);
    TEST_FIELD(CREATESTRUCTW, HINSTANCE, hInstance, 4, 4, 4);
    TEST_FIELD(CREATESTRUCTW, HMENU, hMenu, 8, 4, 4);
    TEST_FIELD(CREATESTRUCTW, HWND, hwndParent, 12, 4, 4);
    TEST_FIELD(CREATESTRUCTW, INT, cy, 16, 4, 4);
    TEST_FIELD(CREATESTRUCTW, INT, cx, 20, 4, 4);
    TEST_FIELD(CREATESTRUCTW, INT, y, 24, 4, 4);
    TEST_FIELD(CREATESTRUCTW, INT, x, 28, 4, 4);
    TEST_FIELD(CREATESTRUCTW, LONG, style, 32, 4, 4);
    TEST_FIELD(CREATESTRUCTW, LPCWSTR, lpszName, 36, 4, 4);
    TEST_FIELD(CREATESTRUCTW, LPCWSTR, lpszClass, 40, 4, 4);
    TEST_FIELD(CREATESTRUCTW, DWORD, dwExStyle, 44, 4, 4);
}

static void test_pack_CURSORINFO(void)
{
    /* CURSORINFO (pack 4) */
    TEST_TYPE(CURSORINFO, 20, 4);
    TEST_FIELD(CURSORINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(CURSORINFO, DWORD, flags, 4, 4, 4);
    TEST_FIELD(CURSORINFO, HCURSOR, hCursor, 8, 4, 4);
    TEST_FIELD(CURSORINFO, POINT, ptScreenPos, 12, 8, 4);
}

static void test_pack_CWPRETSTRUCT(void)
{
    /* CWPRETSTRUCT (pack 4) */
    TEST_TYPE(CWPRETSTRUCT, 20, 4);
    TEST_FIELD(CWPRETSTRUCT, LRESULT, lResult, 0, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, LPARAM, lParam, 4, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, WPARAM, wParam, 8, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, DWORD, message, 12, 4, 4);
    TEST_FIELD(CWPRETSTRUCT, HWND, hwnd, 16, 4, 4);
}

static void test_pack_CWPSTRUCT(void)
{
    /* CWPSTRUCT (pack 4) */
    TEST_TYPE(CWPSTRUCT, 16, 4);
    TEST_FIELD(CWPSTRUCT, LPARAM, lParam, 0, 4, 4);
    TEST_FIELD(CWPSTRUCT, WPARAM, wParam, 4, 4, 4);
    TEST_FIELD(CWPSTRUCT, UINT, message, 8, 4, 4);
    TEST_FIELD(CWPSTRUCT, HWND, hwnd, 12, 4, 4);
}

static void test_pack_DEBUGHOOKINFO(void)
{
    /* DEBUGHOOKINFO (pack 4) */
    TEST_TYPE(DEBUGHOOKINFO, 20, 4);
    TEST_FIELD(DEBUGHOOKINFO, DWORD, idThread, 0, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, DWORD, idThreadInstaller, 4, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, LPARAM, lParam, 8, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, WPARAM, wParam, 12, 4, 4);
    TEST_FIELD(DEBUGHOOKINFO, INT, code, 16, 4, 4);
}

static void test_pack_DELETEITEMSTRUCT(void)
{
    /* DELETEITEMSTRUCT (pack 4) */
    TEST_TYPE(DELETEITEMSTRUCT, 20, 4);
    TEST_FIELD(DELETEITEMSTRUCT, UINT, CtlType, 0, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, UINT, CtlID, 4, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, UINT, itemID, 8, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, HWND, hwndItem, 12, 4, 4);
    TEST_FIELD(DELETEITEMSTRUCT, ULONG_PTR, itemData, 16, 4, 4);
}

static void test_pack_DESKTOPENUMPROCA(void)
{
    /* DESKTOPENUMPROCA */
    TEST_TYPE(DESKTOPENUMPROCA, 4, 4);
}

static void test_pack_DESKTOPENUMPROCW(void)
{
    /* DESKTOPENUMPROCW */
    TEST_TYPE(DESKTOPENUMPROCW, 4, 4);
}

static void test_pack_DLGITEMTEMPLATE(void)
{
    /* DLGITEMTEMPLATE (pack 2) */
    TEST_TYPE(DLGITEMTEMPLATE, 18, 2);
    TEST_FIELD(DLGITEMTEMPLATE, DWORD, style, 0, 4, 2);
    TEST_FIELD(DLGITEMTEMPLATE, DWORD, dwExtendedStyle, 4, 4, 2);
    TEST_FIELD(DLGITEMTEMPLATE, short, x, 8, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, short, y, 10, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, short, cx, 12, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, short, cy, 14, 2, 2);
    TEST_FIELD(DLGITEMTEMPLATE, WORD, id, 16, 2, 2);
}

static void test_pack_DLGPROC(void)
{
    /* DLGPROC */
    TEST_TYPE(DLGPROC, 4, 4);
}

static void test_pack_DLGTEMPLATE(void)
{
    /* DLGTEMPLATE (pack 2) */
    TEST_TYPE(DLGTEMPLATE, 18, 2);
    TEST_FIELD(DLGTEMPLATE, DWORD, style, 0, 4, 2);
    TEST_FIELD(DLGTEMPLATE, DWORD, dwExtendedStyle, 4, 4, 2);
    TEST_FIELD(DLGTEMPLATE, WORD, cdit, 8, 2, 2);
    TEST_FIELD(DLGTEMPLATE, short, x, 10, 2, 2);
    TEST_FIELD(DLGTEMPLATE, short, y, 12, 2, 2);
    TEST_FIELD(DLGTEMPLATE, short, cx, 14, 2, 2);
    TEST_FIELD(DLGTEMPLATE, short, cy, 16, 2, 2);
}

static void test_pack_DRAWITEMSTRUCT(void)
{
    /* DRAWITEMSTRUCT (pack 4) */
    TEST_TYPE(DRAWITEMSTRUCT, 48, 4);
    TEST_FIELD(DRAWITEMSTRUCT, UINT, CtlType, 0, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, UINT, CtlID, 4, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, UINT, itemID, 8, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, UINT, itemAction, 12, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, UINT, itemState, 16, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, HWND, hwndItem, 20, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, HDC, hDC, 24, 4, 4);
    TEST_FIELD(DRAWITEMSTRUCT, RECT, rcItem, 28, 16, 4);
    TEST_FIELD(DRAWITEMSTRUCT, ULONG_PTR, itemData, 44, 4, 4);
}

static void test_pack_DRAWSTATEPROC(void)
{
    /* DRAWSTATEPROC */
    TEST_TYPE(DRAWSTATEPROC, 4, 4);
}

static void test_pack_DRAWTEXTPARAMS(void)
{
    /* DRAWTEXTPARAMS (pack 4) */
    TEST_TYPE(DRAWTEXTPARAMS, 20, 4);
    TEST_FIELD(DRAWTEXTPARAMS, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, INT, iTabLength, 4, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, INT, iLeftMargin, 8, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, INT, iRightMargin, 12, 4, 4);
    TEST_FIELD(DRAWTEXTPARAMS, UINT, uiLengthDrawn, 16, 4, 4);
}

static void test_pack_EDITWORDBREAKPROCA(void)
{
    /* EDITWORDBREAKPROCA */
    TEST_TYPE(EDITWORDBREAKPROCA, 4, 4);
}

static void test_pack_EDITWORDBREAKPROCW(void)
{
    /* EDITWORDBREAKPROCW */
    TEST_TYPE(EDITWORDBREAKPROCW, 4, 4);
}

static void test_pack_EVENTMSG(void)
{
    /* EVENTMSG (pack 4) */
    TEST_TYPE(EVENTMSG, 20, 4);
    TEST_FIELD(EVENTMSG, UINT, message, 0, 4, 4);
    TEST_FIELD(EVENTMSG, UINT, paramL, 4, 4, 4);
    TEST_FIELD(EVENTMSG, UINT, paramH, 8, 4, 4);
    TEST_FIELD(EVENTMSG, DWORD, time, 12, 4, 4);
    TEST_FIELD(EVENTMSG, HWND, hwnd, 16, 4, 4);
}

static void test_pack_FILTERKEYS(void)
{
    /* FILTERKEYS (pack 4) */
    TEST_TYPE(FILTERKEYS, 24, 4);
    TEST_FIELD(FILTERKEYS, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(FILTERKEYS, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(FILTERKEYS, DWORD, iWaitMSec, 8, 4, 4);
    TEST_FIELD(FILTERKEYS, DWORD, iDelayMSec, 12, 4, 4);
    TEST_FIELD(FILTERKEYS, DWORD, iRepeatMSec, 16, 4, 4);
    TEST_FIELD(FILTERKEYS, DWORD, iBounceMSec, 20, 4, 4);
}

static void test_pack_FLASHWINFO(void)
{
    /* FLASHWINFO (pack 4) */
    TEST_TYPE(FLASHWINFO, 20, 4);
    TEST_FIELD(FLASHWINFO, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(FLASHWINFO, HWND, hwnd, 4, 4, 4);
    TEST_FIELD(FLASHWINFO, DWORD, dwFlags, 8, 4, 4);
    TEST_FIELD(FLASHWINFO, UINT, uCount, 12, 4, 4);
    TEST_FIELD(FLASHWINFO, DWORD, dwTimeout, 16, 4, 4);
}

static void test_pack_GRAYSTRINGPROC(void)
{
    /* GRAYSTRINGPROC */
    TEST_TYPE(GRAYSTRINGPROC, 4, 4);
}

static void test_pack_GUITHREADINFO(void)
{
    /* GUITHREADINFO (pack 4) */
    TEST_TYPE(GUITHREADINFO, 48, 4);
    TEST_FIELD(GUITHREADINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(GUITHREADINFO, DWORD, flags, 4, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndActive, 8, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndFocus, 12, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndCapture, 16, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndMenuOwner, 20, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndMoveSize, 24, 4, 4);
    TEST_FIELD(GUITHREADINFO, HWND, hwndCaret, 28, 4, 4);
    TEST_FIELD(GUITHREADINFO, RECT, rcCaret, 32, 16, 4);
}

static void test_pack_HARDWAREHOOKSTRUCT(void)
{
    /* HARDWAREHOOKSTRUCT (pack 4) */
    TEST_TYPE(HARDWAREHOOKSTRUCT, 16, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, HWND, hwnd, 0, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, UINT, message, 4, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, WPARAM, wParam, 8, 4, 4);
    TEST_FIELD(HARDWAREHOOKSTRUCT, LPARAM, lParam, 12, 4, 4);
}

static void test_pack_HARDWAREINPUT(void)
{
    /* HARDWAREINPUT (pack 4) */
    TEST_TYPE(HARDWAREINPUT, 8, 4);
    TEST_FIELD(HARDWAREINPUT, DWORD, uMsg, 0, 4, 4);
    TEST_FIELD(HARDWAREINPUT, WORD, wParamL, 4, 2, 2);
    TEST_FIELD(HARDWAREINPUT, WORD, wParamH, 6, 2, 2);
}

static void test_pack_HDEVNOTIFY(void)
{
    /* HDEVNOTIFY */
    TEST_TYPE(HDEVNOTIFY, 4, 4);
}

static void test_pack_HDWP(void)
{
    /* HDWP */
    TEST_TYPE(HDWP, 4, 4);
}

static void test_pack_HELPINFO(void)
{
    /* HELPINFO (pack 4) */
    TEST_TYPE(HELPINFO, 28, 4);
    TEST_FIELD(HELPINFO, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(HELPINFO, INT, iContextType, 4, 4, 4);
    TEST_FIELD(HELPINFO, INT, iCtrlId, 8, 4, 4);
    TEST_FIELD(HELPINFO, HANDLE, hItemHandle, 12, 4, 4);
    TEST_FIELD(HELPINFO, DWORD_PTR, dwContextId, 16, 4, 4);
    TEST_FIELD(HELPINFO, POINT, MousePos, 20, 8, 4);
}

static void test_pack_HELPWININFOA(void)
{
    /* HELPWININFOA (pack 4) */
    TEST_TYPE(HELPWININFOA, 28, 4);
    TEST_FIELD(HELPWININFOA, int, wStructSize, 0, 4, 4);
    TEST_FIELD(HELPWININFOA, int, x, 4, 4, 4);
    TEST_FIELD(HELPWININFOA, int, y, 8, 4, 4);
    TEST_FIELD(HELPWININFOA, int, dx, 12, 4, 4);
    TEST_FIELD(HELPWININFOA, int, dy, 16, 4, 4);
    TEST_FIELD(HELPWININFOA, int, wMax, 20, 4, 4);
    TEST_FIELD(HELPWININFOA, CHAR[2], rgchMember, 24, 2, 1);
}

static void test_pack_HELPWININFOW(void)
{
    /* HELPWININFOW (pack 4) */
    TEST_TYPE(HELPWININFOW, 28, 4);
    TEST_FIELD(HELPWININFOW, int, wStructSize, 0, 4, 4);
    TEST_FIELD(HELPWININFOW, int, x, 4, 4, 4);
    TEST_FIELD(HELPWININFOW, int, y, 8, 4, 4);
    TEST_FIELD(HELPWININFOW, int, dx, 12, 4, 4);
    TEST_FIELD(HELPWININFOW, int, dy, 16, 4, 4);
    TEST_FIELD(HELPWININFOW, int, wMax, 20, 4, 4);
    TEST_FIELD(HELPWININFOW, WCHAR[2], rgchMember, 24, 4, 2);
}

static void test_pack_HIGHCONTRASTA(void)
{
    /* HIGHCONTRASTA (pack 4) */
    TEST_TYPE(HIGHCONTRASTA, 12, 4);
    TEST_FIELD(HIGHCONTRASTA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(HIGHCONTRASTA, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(HIGHCONTRASTA, LPSTR, lpszDefaultScheme, 8, 4, 4);
}

static void test_pack_HIGHCONTRASTW(void)
{
    /* HIGHCONTRASTW (pack 4) */
    TEST_TYPE(HIGHCONTRASTW, 12, 4);
    TEST_FIELD(HIGHCONTRASTW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(HIGHCONTRASTW, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(HIGHCONTRASTW, LPWSTR, lpszDefaultScheme, 8, 4, 4);
}

static void test_pack_HOOKPROC(void)
{
    /* HOOKPROC */
    TEST_TYPE(HOOKPROC, 4, 4);
}

static void test_pack_ICONINFO(void)
{
    /* ICONINFO (pack 4) */
    TEST_TYPE(ICONINFO, 20, 4);
    TEST_FIELD(ICONINFO, BOOL, fIcon, 0, 4, 4);
    TEST_FIELD(ICONINFO, DWORD, xHotspot, 4, 4, 4);
    TEST_FIELD(ICONINFO, DWORD, yHotspot, 8, 4, 4);
    TEST_FIELD(ICONINFO, HBITMAP, hbmMask, 12, 4, 4);
    TEST_FIELD(ICONINFO, HBITMAP, hbmColor, 16, 4, 4);
}

static void test_pack_ICONMETRICSA(void)
{
    /* ICONMETRICSA (pack 4) */
    TEST_TYPE(ICONMETRICSA, 76, 4);
    TEST_FIELD(ICONMETRICSA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(ICONMETRICSA, int, iHorzSpacing, 4, 4, 4);
    TEST_FIELD(ICONMETRICSA, int, iVertSpacing, 8, 4, 4);
    TEST_FIELD(ICONMETRICSA, int, iTitleWrap, 12, 4, 4);
    TEST_FIELD(ICONMETRICSA, LOGFONTA, lfFont, 16, 60, 4);
}

static void test_pack_ICONMETRICSW(void)
{
    /* ICONMETRICSW (pack 4) */
    TEST_TYPE(ICONMETRICSW, 108, 4);
    TEST_FIELD(ICONMETRICSW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(ICONMETRICSW, int, iHorzSpacing, 4, 4, 4);
    TEST_FIELD(ICONMETRICSW, int, iVertSpacing, 8, 4, 4);
    TEST_FIELD(ICONMETRICSW, int, iTitleWrap, 12, 4, 4);
    TEST_FIELD(ICONMETRICSW, LOGFONTW, lfFont, 16, 92, 4);
}

static void test_pack_INPUT(void)
{
    /* INPUT (pack 4) */
    TEST_FIELD(INPUT, DWORD, type, 0, 4, 4);
}

static void test_pack_KBDLLHOOKSTRUCT(void)
{
    /* KBDLLHOOKSTRUCT (pack 4) */
    TEST_TYPE(KBDLLHOOKSTRUCT, 20, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, DWORD, vkCode, 0, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, DWORD, scanCode, 4, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, DWORD, flags, 8, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, DWORD, time, 12, 4, 4);
    TEST_FIELD(KBDLLHOOKSTRUCT, ULONG_PTR, dwExtraInfo, 16, 4, 4);
}

static void test_pack_KEYBDINPUT(void)
{
    /* KEYBDINPUT (pack 4) */
    TEST_TYPE(KEYBDINPUT, 16, 4);
    TEST_FIELD(KEYBDINPUT, WORD, wVk, 0, 2, 2);
    TEST_FIELD(KEYBDINPUT, WORD, wScan, 2, 2, 2);
    TEST_FIELD(KEYBDINPUT, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(KEYBDINPUT, DWORD, time, 8, 4, 4);
    TEST_FIELD(KEYBDINPUT, ULONG_PTR, dwExtraInfo, 12, 4, 4);
}

static void test_pack_LPACCESSTIMEOUT(void)
{
    /* LPACCESSTIMEOUT */
    TEST_TYPE(LPACCESSTIMEOUT, 4, 4);
    TEST_TYPE_POINTER(LPACCESSTIMEOUT, 12, 4);
}

static void test_pack_LPANIMATIONINFO(void)
{
    /* LPANIMATIONINFO */
    TEST_TYPE(LPANIMATIONINFO, 4, 4);
    TEST_TYPE_POINTER(LPANIMATIONINFO, 8, 4);
}

static void test_pack_LPCBTACTIVATESTRUCT(void)
{
    /* LPCBTACTIVATESTRUCT */
    TEST_TYPE(LPCBTACTIVATESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCBTACTIVATESTRUCT, 8, 4);
}

static void test_pack_LPCBT_CREATEWNDA(void)
{
    /* LPCBT_CREATEWNDA */
    TEST_TYPE(LPCBT_CREATEWNDA, 4, 4);
    TEST_TYPE_POINTER(LPCBT_CREATEWNDA, 8, 4);
}

static void test_pack_LPCBT_CREATEWNDW(void)
{
    /* LPCBT_CREATEWNDW */
    TEST_TYPE(LPCBT_CREATEWNDW, 4, 4);
    TEST_TYPE_POINTER(LPCBT_CREATEWNDW, 8, 4);
}

static void test_pack_LPCDLGTEMPLATEA(void)
{
    /* LPCDLGTEMPLATEA */
    TEST_TYPE(LPCDLGTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPCDLGTEMPLATEA, 18, 2);
}

static void test_pack_LPCDLGTEMPLATEW(void)
{
    /* LPCDLGTEMPLATEW */
    TEST_TYPE(LPCDLGTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPCDLGTEMPLATEW, 18, 2);
}

static void test_pack_LPCLIENTCREATESTRUCT(void)
{
    /* LPCLIENTCREATESTRUCT */
    TEST_TYPE(LPCLIENTCREATESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCLIENTCREATESTRUCT, 8, 4);
}

static void test_pack_LPCMENUINFO(void)
{
    /* LPCMENUINFO */
    TEST_TYPE(LPCMENUINFO, 4, 4);
    TEST_TYPE_POINTER(LPCMENUINFO, 28, 4);
}

static void test_pack_LPCMENUITEMINFOA(void)
{
    /* LPCMENUITEMINFOA */
    TEST_TYPE(LPCMENUITEMINFOA, 4, 4);
    TEST_TYPE_POINTER(LPCMENUITEMINFOA, 48, 4);
}

static void test_pack_LPCMENUITEMINFOW(void)
{
    /* LPCMENUITEMINFOW */
    TEST_TYPE(LPCMENUITEMINFOW, 4, 4);
    TEST_TYPE_POINTER(LPCMENUITEMINFOW, 48, 4);
}

static void test_pack_LPCOMBOBOXINFO(void)
{
    /* LPCOMBOBOXINFO */
    TEST_TYPE(LPCOMBOBOXINFO, 4, 4);
    TEST_TYPE_POINTER(LPCOMBOBOXINFO, 52, 4);
}

static void test_pack_LPCOMPAREITEMSTRUCT(void)
{
    /* LPCOMPAREITEMSTRUCT */
    TEST_TYPE(LPCOMPAREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCOMPAREITEMSTRUCT, 32, 4);
}

static void test_pack_LPCREATESTRUCTA(void)
{
    /* LPCREATESTRUCTA */
    TEST_TYPE(LPCREATESTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPCREATESTRUCTA, 48, 4);
}

static void test_pack_LPCREATESTRUCTW(void)
{
    /* LPCREATESTRUCTW */
    TEST_TYPE(LPCREATESTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPCREATESTRUCTW, 48, 4);
}

static void test_pack_LPCSCROLLINFO(void)
{
    /* LPCSCROLLINFO */
    TEST_TYPE(LPCSCROLLINFO, 4, 4);
    TEST_TYPE_POINTER(LPCSCROLLINFO, 28, 4);
}

static void test_pack_LPCURSORINFO(void)
{
    /* LPCURSORINFO */
    TEST_TYPE(LPCURSORINFO, 4, 4);
    TEST_TYPE_POINTER(LPCURSORINFO, 20, 4);
}

static void test_pack_LPCWPRETSTRUCT(void)
{
    /* LPCWPRETSTRUCT */
    TEST_TYPE(LPCWPRETSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCWPRETSTRUCT, 20, 4);
}

static void test_pack_LPCWPSTRUCT(void)
{
    /* LPCWPSTRUCT */
    TEST_TYPE(LPCWPSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPCWPSTRUCT, 16, 4);
}

static void test_pack_LPDEBUGHOOKINFO(void)
{
    /* LPDEBUGHOOKINFO */
    TEST_TYPE(LPDEBUGHOOKINFO, 4, 4);
    TEST_TYPE_POINTER(LPDEBUGHOOKINFO, 20, 4);
}

static void test_pack_LPDELETEITEMSTRUCT(void)
{
    /* LPDELETEITEMSTRUCT */
    TEST_TYPE(LPDELETEITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPDELETEITEMSTRUCT, 20, 4);
}

static void test_pack_LPDLGITEMTEMPLATEA(void)
{
    /* LPDLGITEMTEMPLATEA */
    TEST_TYPE(LPDLGITEMTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPDLGITEMTEMPLATEA, 18, 2);
}

static void test_pack_LPDLGITEMTEMPLATEW(void)
{
    /* LPDLGITEMTEMPLATEW */
    TEST_TYPE(LPDLGITEMTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPDLGITEMTEMPLATEW, 18, 2);
}

static void test_pack_LPDLGTEMPLATEA(void)
{
    /* LPDLGTEMPLATEA */
    TEST_TYPE(LPDLGTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(LPDLGTEMPLATEA, 18, 2);
}

static void test_pack_LPDLGTEMPLATEW(void)
{
    /* LPDLGTEMPLATEW */
    TEST_TYPE(LPDLGTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(LPDLGTEMPLATEW, 18, 2);
}

static void test_pack_LPDRAWITEMSTRUCT(void)
{
    /* LPDRAWITEMSTRUCT */
    TEST_TYPE(LPDRAWITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPDRAWITEMSTRUCT, 48, 4);
}

static void test_pack_LPDRAWTEXTPARAMS(void)
{
    /* LPDRAWTEXTPARAMS */
    TEST_TYPE(LPDRAWTEXTPARAMS, 4, 4);
    TEST_TYPE_POINTER(LPDRAWTEXTPARAMS, 20, 4);
}

static void test_pack_LPEVENTMSG(void)
{
    /* LPEVENTMSG */
    TEST_TYPE(LPEVENTMSG, 4, 4);
    TEST_TYPE_POINTER(LPEVENTMSG, 20, 4);
}

static void test_pack_LPFILTERKEYS(void)
{
    /* LPFILTERKEYS */
    TEST_TYPE(LPFILTERKEYS, 4, 4);
    TEST_TYPE_POINTER(LPFILTERKEYS, 24, 4);
}

static void test_pack_LPGUITHREADINFO(void)
{
    /* LPGUITHREADINFO */
    TEST_TYPE(LPGUITHREADINFO, 4, 4);
    TEST_TYPE_POINTER(LPGUITHREADINFO, 48, 4);
}

static void test_pack_LPHARDWAREHOOKSTRUCT(void)
{
    /* LPHARDWAREHOOKSTRUCT */
    TEST_TYPE(LPHARDWAREHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPHARDWAREHOOKSTRUCT, 16, 4);
}

static void test_pack_LPHARDWAREINPUT(void)
{
    /* LPHARDWAREINPUT */
    TEST_TYPE(LPHARDWAREINPUT, 4, 4);
    TEST_TYPE_POINTER(LPHARDWAREINPUT, 8, 4);
}

static void test_pack_LPHELPINFO(void)
{
    /* LPHELPINFO */
    TEST_TYPE(LPHELPINFO, 4, 4);
    TEST_TYPE_POINTER(LPHELPINFO, 28, 4);
}

static void test_pack_LPHELPWININFOA(void)
{
    /* LPHELPWININFOA */
    TEST_TYPE(LPHELPWININFOA, 4, 4);
    TEST_TYPE_POINTER(LPHELPWININFOA, 28, 4);
}

static void test_pack_LPHELPWININFOW(void)
{
    /* LPHELPWININFOW */
    TEST_TYPE(LPHELPWININFOW, 4, 4);
    TEST_TYPE_POINTER(LPHELPWININFOW, 28, 4);
}

static void test_pack_LPHIGHCONTRASTA(void)
{
    /* LPHIGHCONTRASTA */
    TEST_TYPE(LPHIGHCONTRASTA, 4, 4);
    TEST_TYPE_POINTER(LPHIGHCONTRASTA, 12, 4);
}

static void test_pack_LPHIGHCONTRASTW(void)
{
    /* LPHIGHCONTRASTW */
    TEST_TYPE(LPHIGHCONTRASTW, 4, 4);
    TEST_TYPE_POINTER(LPHIGHCONTRASTW, 12, 4);
}

static void test_pack_LPICONMETRICSA(void)
{
    /* LPICONMETRICSA */
    TEST_TYPE(LPICONMETRICSA, 4, 4);
    TEST_TYPE_POINTER(LPICONMETRICSA, 76, 4);
}

static void test_pack_LPICONMETRICSW(void)
{
    /* LPICONMETRICSW */
    TEST_TYPE(LPICONMETRICSW, 4, 4);
    TEST_TYPE_POINTER(LPICONMETRICSW, 108, 4);
}

static void test_pack_LPINPUT(void)
{
    /* LPINPUT */
    TEST_TYPE(LPINPUT, 4, 4);
}

static void test_pack_LPKBDLLHOOKSTRUCT(void)
{
    /* LPKBDLLHOOKSTRUCT */
    TEST_TYPE(LPKBDLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPKBDLLHOOKSTRUCT, 20, 4);
}

static void test_pack_LPKEYBDINPUT(void)
{
    /* LPKEYBDINPUT */
    TEST_TYPE(LPKEYBDINPUT, 4, 4);
    TEST_TYPE_POINTER(LPKEYBDINPUT, 16, 4);
}

static void test_pack_LPMDICREATESTRUCTA(void)
{
    /* LPMDICREATESTRUCTA */
    TEST_TYPE(LPMDICREATESTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPMDICREATESTRUCTA, 36, 4);
}

static void test_pack_LPMDICREATESTRUCTW(void)
{
    /* LPMDICREATESTRUCTW */
    TEST_TYPE(LPMDICREATESTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPMDICREATESTRUCTW, 36, 4);
}

static void test_pack_LPMDINEXTMENU(void)
{
    /* LPMDINEXTMENU */
    TEST_TYPE(LPMDINEXTMENU, 4, 4);
    TEST_TYPE_POINTER(LPMDINEXTMENU, 12, 4);
}

static void test_pack_LPMEASUREITEMSTRUCT(void)
{
    /* LPMEASUREITEMSTRUCT */
    TEST_TYPE(LPMEASUREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMEASUREITEMSTRUCT, 24, 4);
}

static void test_pack_LPMENUINFO(void)
{
    /* LPMENUINFO */
    TEST_TYPE(LPMENUINFO, 4, 4);
    TEST_TYPE_POINTER(LPMENUINFO, 28, 4);
}

static void test_pack_LPMENUITEMINFOA(void)
{
    /* LPMENUITEMINFOA */
    TEST_TYPE(LPMENUITEMINFOA, 4, 4);
    TEST_TYPE_POINTER(LPMENUITEMINFOA, 48, 4);
}

static void test_pack_LPMENUITEMINFOW(void)
{
    /* LPMENUITEMINFOW */
    TEST_TYPE(LPMENUITEMINFOW, 4, 4);
    TEST_TYPE_POINTER(LPMENUITEMINFOW, 48, 4);
}

static void test_pack_LPMINIMIZEDMETRICS(void)
{
    /* LPMINIMIZEDMETRICS */
    TEST_TYPE(LPMINIMIZEDMETRICS, 4, 4);
    TEST_TYPE_POINTER(LPMINIMIZEDMETRICS, 20, 4);
}

static void test_pack_LPMINMAXINFO(void)
{
    /* LPMINMAXINFO */
    TEST_TYPE(LPMINMAXINFO, 4, 4);
    TEST_TYPE_POINTER(LPMINMAXINFO, 40, 4);
}

static void test_pack_LPMONITORINFO(void)
{
    /* LPMONITORINFO */
    TEST_TYPE(LPMONITORINFO, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFO, 40, 4);
}

static void test_pack_LPMONITORINFOEXA(void)
{
    /* LPMONITORINFOEXA */
    TEST_TYPE(LPMONITORINFOEXA, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFOEXA, 72, 4);
}

static void test_pack_LPMONITORINFOEXW(void)
{
    /* LPMONITORINFOEXW */
    TEST_TYPE(LPMONITORINFOEXW, 4, 4);
    TEST_TYPE_POINTER(LPMONITORINFOEXW, 104, 4);
}

static void test_pack_LPMOUSEHOOKSTRUCT(void)
{
    /* LPMOUSEHOOKSTRUCT */
    TEST_TYPE(LPMOUSEHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEHOOKSTRUCT, 20, 4);
}

static void test_pack_LPMOUSEINPUT(void)
{
    /* LPMOUSEINPUT */
    TEST_TYPE(LPMOUSEINPUT, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEINPUT, 24, 4);
}

static void test_pack_LPMOUSEKEYS(void)
{
    /* LPMOUSEKEYS */
    TEST_TYPE(LPMOUSEKEYS, 4, 4);
    TEST_TYPE_POINTER(LPMOUSEKEYS, 28, 4);
}

static void test_pack_LPMSG(void)
{
    /* LPMSG */
    TEST_TYPE(LPMSG, 4, 4);
    TEST_TYPE_POINTER(LPMSG, 28, 4);
}

static void test_pack_LPMSGBOXPARAMSA(void)
{
    /* LPMSGBOXPARAMSA */
    TEST_TYPE(LPMSGBOXPARAMSA, 4, 4);
    TEST_TYPE_POINTER(LPMSGBOXPARAMSA, 40, 4);
}

static void test_pack_LPMSGBOXPARAMSW(void)
{
    /* LPMSGBOXPARAMSW */
    TEST_TYPE(LPMSGBOXPARAMSW, 4, 4);
    TEST_TYPE_POINTER(LPMSGBOXPARAMSW, 40, 4);
}

static void test_pack_LPMSLLHOOKSTRUCT(void)
{
    /* LPMSLLHOOKSTRUCT */
    TEST_TYPE(LPMSLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPMSLLHOOKSTRUCT, 24, 4);
}

static void test_pack_LPMULTIKEYHELPA(void)
{
    /* LPMULTIKEYHELPA */
    TEST_TYPE(LPMULTIKEYHELPA, 4, 4);
    TEST_TYPE_POINTER(LPMULTIKEYHELPA, 8, 4);
}

static void test_pack_LPMULTIKEYHELPW(void)
{
    /* LPMULTIKEYHELPW */
    TEST_TYPE(LPMULTIKEYHELPW, 4, 4);
    TEST_TYPE_POINTER(LPMULTIKEYHELPW, 8, 4);
}

static void test_pack_LPNCCALCSIZE_PARAMS(void)
{
    /* LPNCCALCSIZE_PARAMS */
    TEST_TYPE(LPNCCALCSIZE_PARAMS, 4, 4);
    TEST_TYPE_POINTER(LPNCCALCSIZE_PARAMS, 52, 4);
}

static void test_pack_LPNMHDR(void)
{
    /* LPNMHDR */
    TEST_TYPE(LPNMHDR, 4, 4);
    TEST_TYPE_POINTER(LPNMHDR, 12, 4);
}

static void test_pack_LPNONCLIENTMETRICSA(void)
{
    /* LPNONCLIENTMETRICSA */
    TEST_TYPE(LPNONCLIENTMETRICSA, 4, 4);
    TEST_TYPE_POINTER(LPNONCLIENTMETRICSA, 340, 4);
}

static void test_pack_LPNONCLIENTMETRICSW(void)
{
    /* LPNONCLIENTMETRICSW */
    TEST_TYPE(LPNONCLIENTMETRICSW, 4, 4);
    TEST_TYPE_POINTER(LPNONCLIENTMETRICSW, 500, 4);
}

static void test_pack_LPPAINTSTRUCT(void)
{
    /* LPPAINTSTRUCT */
    TEST_TYPE(LPPAINTSTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPPAINTSTRUCT, 64, 4);
}

static void test_pack_LPSCROLLINFO(void)
{
    /* LPSCROLLINFO */
    TEST_TYPE(LPSCROLLINFO, 4, 4);
    TEST_TYPE_POINTER(LPSCROLLINFO, 28, 4);
}

static void test_pack_LPSERIALKEYSA(void)
{
    /* LPSERIALKEYSA */
    TEST_TYPE(LPSERIALKEYSA, 4, 4);
    TEST_TYPE_POINTER(LPSERIALKEYSA, 28, 4);
}

static void test_pack_LPSERIALKEYSW(void)
{
    /* LPSERIALKEYSW */
    TEST_TYPE(LPSERIALKEYSW, 4, 4);
    TEST_TYPE_POINTER(LPSERIALKEYSW, 28, 4);
}

static void test_pack_LPSOUNDSENTRYA(void)
{
    /* LPSOUNDSENTRYA */
    TEST_TYPE(LPSOUNDSENTRYA, 4, 4);
    TEST_TYPE_POINTER(LPSOUNDSENTRYA, 48, 4);
}

static void test_pack_LPSOUNDSENTRYW(void)
{
    /* LPSOUNDSENTRYW */
    TEST_TYPE(LPSOUNDSENTRYW, 4, 4);
    TEST_TYPE_POINTER(LPSOUNDSENTRYW, 48, 4);
}

static void test_pack_LPSTICKYKEYS(void)
{
    /* LPSTICKYKEYS */
    TEST_TYPE(LPSTICKYKEYS, 4, 4);
    TEST_TYPE_POINTER(LPSTICKYKEYS, 8, 4);
}

static void test_pack_LPSTYLESTRUCT(void)
{
    /* LPSTYLESTRUCT */
    TEST_TYPE(LPSTYLESTRUCT, 4, 4);
    TEST_TYPE_POINTER(LPSTYLESTRUCT, 8, 4);
}

static void test_pack_LPTITLEBARINFO(void)
{
    /* LPTITLEBARINFO */
    TEST_TYPE(LPTITLEBARINFO, 4, 4);
    TEST_TYPE_POINTER(LPTITLEBARINFO, 44, 4);
}

static void test_pack_LPTOGGLEKEYS(void)
{
    /* LPTOGGLEKEYS */
    TEST_TYPE(LPTOGGLEKEYS, 4, 4);
    TEST_TYPE_POINTER(LPTOGGLEKEYS, 8, 4);
}

static void test_pack_LPTPMPARAMS(void)
{
    /* LPTPMPARAMS */
    TEST_TYPE(LPTPMPARAMS, 4, 4);
    TEST_TYPE_POINTER(LPTPMPARAMS, 20, 4);
}

static void test_pack_LPTRACKMOUSEEVENT(void)
{
    /* LPTRACKMOUSEEVENT */
    TEST_TYPE(LPTRACKMOUSEEVENT, 4, 4);
    TEST_TYPE_POINTER(LPTRACKMOUSEEVENT, 16, 4);
}

static void test_pack_LPWINDOWINFO(void)
{
    /* LPWINDOWINFO */
    TEST_TYPE(LPWINDOWINFO, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWINFO, 60, 4);
}

static void test_pack_LPWINDOWPLACEMENT(void)
{
    /* LPWINDOWPLACEMENT */
    TEST_TYPE(LPWINDOWPLACEMENT, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWPLACEMENT, 44, 4);
}

static void test_pack_LPWINDOWPOS(void)
{
    /* LPWINDOWPOS */
    TEST_TYPE(LPWINDOWPOS, 4, 4);
    TEST_TYPE_POINTER(LPWINDOWPOS, 28, 4);
}

static void test_pack_LPWNDCLASSA(void)
{
    /* LPWNDCLASSA */
    TEST_TYPE(LPWNDCLASSA, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSA, 40, 4);
}

static void test_pack_LPWNDCLASSEXA(void)
{
    /* LPWNDCLASSEXA */
    TEST_TYPE(LPWNDCLASSEXA, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSEXA, 48, 4);
}

static void test_pack_LPWNDCLASSEXW(void)
{
    /* LPWNDCLASSEXW */
    TEST_TYPE(LPWNDCLASSEXW, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSEXW, 48, 4);
}

static void test_pack_LPWNDCLASSW(void)
{
    /* LPWNDCLASSW */
    TEST_TYPE(LPWNDCLASSW, 4, 4);
    TEST_TYPE_POINTER(LPWNDCLASSW, 40, 4);
}

static void test_pack_MDICREATESTRUCTA(void)
{
    /* MDICREATESTRUCTA (pack 4) */
    TEST_TYPE(MDICREATESTRUCTA, 36, 4);
    TEST_FIELD(MDICREATESTRUCTA, LPCSTR, szClass, 0, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, LPCSTR, szTitle, 4, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, HINSTANCE, hOwner, 8, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, INT, x, 12, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, INT, y, 16, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, INT, cx, 20, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, INT, cy, 24, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, DWORD, style, 28, 4, 4);
    TEST_FIELD(MDICREATESTRUCTA, LPARAM, lParam, 32, 4, 4);
}

static void test_pack_MDICREATESTRUCTW(void)
{
    /* MDICREATESTRUCTW (pack 4) */
    TEST_TYPE(MDICREATESTRUCTW, 36, 4);
    TEST_FIELD(MDICREATESTRUCTW, LPCWSTR, szClass, 0, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, LPCWSTR, szTitle, 4, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, HINSTANCE, hOwner, 8, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, INT, x, 12, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, INT, y, 16, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, INT, cx, 20, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, INT, cy, 24, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, DWORD, style, 28, 4, 4);
    TEST_FIELD(MDICREATESTRUCTW, LPARAM, lParam, 32, 4, 4);
}

static void test_pack_MDINEXTMENU(void)
{
    /* MDINEXTMENU (pack 4) */
    TEST_TYPE(MDINEXTMENU, 12, 4);
    TEST_FIELD(MDINEXTMENU, HMENU, hmenuIn, 0, 4, 4);
    TEST_FIELD(MDINEXTMENU, HMENU, hmenuNext, 4, 4, 4);
    TEST_FIELD(MDINEXTMENU, HWND, hwndNext, 8, 4, 4);
}

static void test_pack_MEASUREITEMSTRUCT(void)
{
    /* MEASUREITEMSTRUCT (pack 4) */
    TEST_TYPE(MEASUREITEMSTRUCT, 24, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, UINT, CtlType, 0, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, UINT, CtlID, 4, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, UINT, itemID, 8, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, UINT, itemWidth, 12, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, UINT, itemHeight, 16, 4, 4);
    TEST_FIELD(MEASUREITEMSTRUCT, ULONG_PTR, itemData, 20, 4, 4);
}

static void test_pack_MENUINFO(void)
{
    /* MENUINFO (pack 4) */
    TEST_TYPE(MENUINFO, 28, 4);
    TEST_FIELD(MENUINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(MENUINFO, DWORD, fMask, 4, 4, 4);
    TEST_FIELD(MENUINFO, DWORD, dwStyle, 8, 4, 4);
    TEST_FIELD(MENUINFO, UINT, cyMax, 12, 4, 4);
    TEST_FIELD(MENUINFO, HBRUSH, hbrBack, 16, 4, 4);
    TEST_FIELD(MENUINFO, DWORD, dwContextHelpID, 20, 4, 4);
    TEST_FIELD(MENUINFO, ULONG_PTR, dwMenuData, 24, 4, 4);
}

static void test_pack_MENUITEMINFOA(void)
{
    /* MENUITEMINFOA (pack 4) */
    TEST_TYPE(MENUITEMINFOA, 48, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, fMask, 4, 4, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, fType, 8, 4, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, fState, 12, 4, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, wID, 16, 4, 4);
    TEST_FIELD(MENUITEMINFOA, HMENU, hSubMenu, 20, 4, 4);
    TEST_FIELD(MENUITEMINFOA, HBITMAP, hbmpChecked, 24, 4, 4);
    TEST_FIELD(MENUITEMINFOA, HBITMAP, hbmpUnchecked, 28, 4, 4);
    TEST_FIELD(MENUITEMINFOA, ULONG_PTR, dwItemData, 32, 4, 4);
    TEST_FIELD(MENUITEMINFOA, LPSTR, dwTypeData, 36, 4, 4);
    TEST_FIELD(MENUITEMINFOA, UINT, cch, 40, 4, 4);
    TEST_FIELD(MENUITEMINFOA, HBITMAP, hbmpItem, 44, 4, 4);
}

static void test_pack_MENUITEMINFOW(void)
{
    /* MENUITEMINFOW (pack 4) */
    TEST_TYPE(MENUITEMINFOW, 48, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, fMask, 4, 4, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, fType, 8, 4, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, fState, 12, 4, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, wID, 16, 4, 4);
    TEST_FIELD(MENUITEMINFOW, HMENU, hSubMenu, 20, 4, 4);
    TEST_FIELD(MENUITEMINFOW, HBITMAP, hbmpChecked, 24, 4, 4);
    TEST_FIELD(MENUITEMINFOW, HBITMAP, hbmpUnchecked, 28, 4, 4);
    TEST_FIELD(MENUITEMINFOW, ULONG_PTR, dwItemData, 32, 4, 4);
    TEST_FIELD(MENUITEMINFOW, LPWSTR, dwTypeData, 36, 4, 4);
    TEST_FIELD(MENUITEMINFOW, UINT, cch, 40, 4, 4);
    TEST_FIELD(MENUITEMINFOW, HBITMAP, hbmpItem, 44, 4, 4);
}

static void test_pack_MENUITEMTEMPLATE(void)
{
    /* MENUITEMTEMPLATE (pack 4) */
    TEST_TYPE(MENUITEMTEMPLATE, 6, 2);
    TEST_FIELD(MENUITEMTEMPLATE, WORD, mtOption, 0, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATE, WORD, mtID, 2, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATE, WCHAR[1], mtString, 4, 2, 2);
}

static void test_pack_MENUITEMTEMPLATEHEADER(void)
{
    /* MENUITEMTEMPLATEHEADER (pack 4) */
    TEST_TYPE(MENUITEMTEMPLATEHEADER, 4, 2);
    TEST_FIELD(MENUITEMTEMPLATEHEADER, WORD, versionNumber, 0, 2, 2);
    TEST_FIELD(MENUITEMTEMPLATEHEADER, WORD, offset, 2, 2, 2);
}

static void test_pack_MINIMIZEDMETRICS(void)
{
    /* MINIMIZEDMETRICS (pack 4) */
    TEST_TYPE(MINIMIZEDMETRICS, 20, 4);
    TEST_FIELD(MINIMIZEDMETRICS, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, int, iWidth, 4, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, int, iHorzGap, 8, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, int, iVertGap, 12, 4, 4);
    TEST_FIELD(MINIMIZEDMETRICS, int, iArrange, 16, 4, 4);
}

static void test_pack_MINMAXINFO(void)
{
    /* MINMAXINFO (pack 4) */
    TEST_TYPE(MINMAXINFO, 40, 4);
    TEST_FIELD(MINMAXINFO, POINT, ptReserved, 0, 8, 4);
    TEST_FIELD(MINMAXINFO, POINT, ptMaxSize, 8, 8, 4);
    TEST_FIELD(MINMAXINFO, POINT, ptMaxPosition, 16, 8, 4);
    TEST_FIELD(MINMAXINFO, POINT, ptMinTrackSize, 24, 8, 4);
    TEST_FIELD(MINMAXINFO, POINT, ptMaxTrackSize, 32, 8, 4);
}

static void test_pack_MONITORENUMPROC(void)
{
    /* MONITORENUMPROC */
    TEST_TYPE(MONITORENUMPROC, 4, 4);
}

static void test_pack_MONITORINFO(void)
{
    /* MONITORINFO (pack 4) */
    TEST_TYPE(MONITORINFO, 40, 4);
    TEST_FIELD(MONITORINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFO, RECT, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFO, RECT, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFO, DWORD, dwFlags, 36, 4, 4);
}

static void test_pack_MONITORINFOEXA(void)
{
    /* MONITORINFOEXA (pack 4) */
    TEST_TYPE(MONITORINFOEXA, 72, 4);
    TEST_FIELD(MONITORINFOEXA, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFOEXA, RECT, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFOEXA, RECT, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFOEXA, DWORD, dwFlags, 36, 4, 4);
    TEST_FIELD(MONITORINFOEXA, CHAR[CCHDEVICENAME], szDevice, 40, 32, 1);
}

static void test_pack_MONITORINFOEXW(void)
{
    /* MONITORINFOEXW (pack 4) */
    TEST_TYPE(MONITORINFOEXW, 104, 4);
    TEST_FIELD(MONITORINFOEXW, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(MONITORINFOEXW, RECT, rcMonitor, 4, 16, 4);
    TEST_FIELD(MONITORINFOEXW, RECT, rcWork, 20, 16, 4);
    TEST_FIELD(MONITORINFOEXW, DWORD, dwFlags, 36, 4, 4);
    TEST_FIELD(MONITORINFOEXW, WCHAR[CCHDEVICENAME], szDevice, 40, 64, 2);
}

static void test_pack_MOUSEHOOKSTRUCT(void)
{
    /* MOUSEHOOKSTRUCT (pack 4) */
    TEST_TYPE(MOUSEHOOKSTRUCT, 20, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, POINT, pt, 0, 8, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, HWND, hwnd, 8, 4, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, UINT, wHitTestCode, 12, 4, 4);
    TEST_FIELD(MOUSEHOOKSTRUCT, ULONG_PTR, dwExtraInfo, 16, 4, 4);
}

static void test_pack_MOUSEINPUT(void)
{
    /* MOUSEINPUT (pack 4) */
    TEST_TYPE(MOUSEINPUT, 24, 4);
    TEST_FIELD(MOUSEINPUT, LONG, dx, 0, 4, 4);
    TEST_FIELD(MOUSEINPUT, LONG, dy, 4, 4, 4);
    TEST_FIELD(MOUSEINPUT, DWORD, mouseData, 8, 4, 4);
    TEST_FIELD(MOUSEINPUT, DWORD, dwFlags, 12, 4, 4);
    TEST_FIELD(MOUSEINPUT, DWORD, time, 16, 4, 4);
    TEST_FIELD(MOUSEINPUT, ULONG_PTR, dwExtraInfo, 20, 4, 4);
}

static void test_pack_MOUSEKEYS(void)
{
    /* MOUSEKEYS (pack 4) */
    TEST_TYPE(MOUSEKEYS, 28, 4);
    TEST_FIELD(MOUSEKEYS, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, iMaxSpeed, 8, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, iTimeToMaxSpeed, 12, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, iCtrlSpeed, 16, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, dwReserved1, 20, 4, 4);
    TEST_FIELD(MOUSEKEYS, DWORD, dwReserved2, 24, 4, 4);
}

static void test_pack_MSG(void)
{
    /* MSG (pack 4) */
    TEST_TYPE(MSG, 28, 4);
    TEST_FIELD(MSG, HWND, hwnd, 0, 4, 4);
    TEST_FIELD(MSG, UINT, message, 4, 4, 4);
    TEST_FIELD(MSG, WPARAM, wParam, 8, 4, 4);
    TEST_FIELD(MSG, LPARAM, lParam, 12, 4, 4);
    TEST_FIELD(MSG, DWORD, time, 16, 4, 4);
    TEST_FIELD(MSG, POINT, pt, 20, 8, 4);
}

static void test_pack_MSGBOXCALLBACK(void)
{
    /* MSGBOXCALLBACK */
    TEST_TYPE(MSGBOXCALLBACK, 4, 4);
}

static void test_pack_MSGBOXPARAMSA(void)
{
    /* MSGBOXPARAMSA (pack 4) */
    TEST_TYPE(MSGBOXPARAMSA, 40, 4);
    TEST_FIELD(MSGBOXPARAMSA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, HWND, hwndOwner, 4, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, HINSTANCE, hInstance, 8, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, LPCSTR, lpszText, 12, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, LPCSTR, lpszCaption, 16, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, DWORD, dwStyle, 20, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, LPCSTR, lpszIcon, 24, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, DWORD_PTR, dwContextHelpId, 28, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, MSGBOXCALLBACK, lpfnMsgBoxCallback, 32, 4, 4);
    TEST_FIELD(MSGBOXPARAMSA, DWORD, dwLanguageId, 36, 4, 4);
}

static void test_pack_MSGBOXPARAMSW(void)
{
    /* MSGBOXPARAMSW (pack 4) */
    TEST_TYPE(MSGBOXPARAMSW, 40, 4);
    TEST_FIELD(MSGBOXPARAMSW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, HWND, hwndOwner, 4, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, HINSTANCE, hInstance, 8, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, LPCWSTR, lpszText, 12, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, LPCWSTR, lpszCaption, 16, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, DWORD, dwStyle, 20, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, LPCWSTR, lpszIcon, 24, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, DWORD_PTR, dwContextHelpId, 28, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, MSGBOXCALLBACK, lpfnMsgBoxCallback, 32, 4, 4);
    TEST_FIELD(MSGBOXPARAMSW, DWORD, dwLanguageId, 36, 4, 4);
}

static void test_pack_MSLLHOOKSTRUCT(void)
{
    /* MSLLHOOKSTRUCT (pack 4) */
    TEST_TYPE(MSLLHOOKSTRUCT, 24, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, POINT, pt, 0, 8, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, DWORD, mouseData, 8, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, DWORD, flags, 12, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, DWORD, time, 16, 4, 4);
    TEST_FIELD(MSLLHOOKSTRUCT, ULONG_PTR, dwExtraInfo, 20, 4, 4);
}

static void test_pack_MULTIKEYHELPA(void)
{
    /* MULTIKEYHELPA (pack 4) */
    TEST_TYPE(MULTIKEYHELPA, 8, 4);
    TEST_FIELD(MULTIKEYHELPA, DWORD, mkSize, 0, 4, 4);
    TEST_FIELD(MULTIKEYHELPA, CHAR, mkKeylist, 4, 1, 1);
    TEST_FIELD(MULTIKEYHELPA, CHAR[1], szKeyphrase, 5, 1, 1);
}

static void test_pack_MULTIKEYHELPW(void)
{
    /* MULTIKEYHELPW (pack 4) */
    TEST_TYPE(MULTIKEYHELPW, 8, 4);
    TEST_FIELD(MULTIKEYHELPW, DWORD, mkSize, 0, 4, 4);
    TEST_FIELD(MULTIKEYHELPW, WCHAR, mkKeylist, 4, 2, 2);
    TEST_FIELD(MULTIKEYHELPW, WCHAR[1], szKeyphrase, 6, 2, 2);
}

static void test_pack_NAMEENUMPROCA(void)
{
    /* NAMEENUMPROCA */
    TEST_TYPE(NAMEENUMPROCA, 4, 4);
}

static void test_pack_NAMEENUMPROCW(void)
{
    /* NAMEENUMPROCW */
    TEST_TYPE(NAMEENUMPROCW, 4, 4);
}

static void test_pack_NCCALCSIZE_PARAMS(void)
{
    /* NCCALCSIZE_PARAMS (pack 4) */
    TEST_TYPE(NCCALCSIZE_PARAMS, 52, 4);
    TEST_FIELD(NCCALCSIZE_PARAMS, RECT[3], rgrc, 0, 48, 4);
    TEST_FIELD(NCCALCSIZE_PARAMS, WINDOWPOS *, lppos, 48, 4, 4);
}

static void test_pack_NMHDR(void)
{
    /* NMHDR (pack 4) */
    TEST_TYPE(NMHDR, 12, 4);
    TEST_FIELD(NMHDR, HWND, hwndFrom, 0, 4, 4);
    TEST_FIELD(NMHDR, UINT_PTR, idFrom, 4, 4, 4);
    TEST_FIELD(NMHDR, UINT, code, 8, 4, 4);
}

static void test_pack_NONCLIENTMETRICSA(void)
{
    /* NONCLIENTMETRICSA (pack 4) */
    TEST_TYPE(NONCLIENTMETRICSA, 340, 4);
    TEST_FIELD(NONCLIENTMETRICSA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iBorderWidth, 4, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iScrollWidth, 8, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iScrollHeight, 12, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iCaptionWidth, 16, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iCaptionHeight, 20, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, LOGFONTA, lfCaptionFont, 24, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iSmCaptionWidth, 84, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iSmCaptionHeight, 88, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, LOGFONTA, lfSmCaptionFont, 92, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iMenuWidth, 152, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, INT, iMenuHeight, 156, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSA, LOGFONTA, lfMenuFont, 160, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, LOGFONTA, lfStatusFont, 220, 60, 4);
    TEST_FIELD(NONCLIENTMETRICSA, LOGFONTA, lfMessageFont, 280, 60, 4);
}

static void test_pack_NONCLIENTMETRICSW(void)
{
    /* NONCLIENTMETRICSW (pack 4) */
    TEST_TYPE(NONCLIENTMETRICSW, 500, 4);
    TEST_FIELD(NONCLIENTMETRICSW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iBorderWidth, 4, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iScrollWidth, 8, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iScrollHeight, 12, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iCaptionWidth, 16, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iCaptionHeight, 20, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, LOGFONTW, lfCaptionFont, 24, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iSmCaptionWidth, 116, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iSmCaptionHeight, 120, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, LOGFONTW, lfSmCaptionFont, 124, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iMenuWidth, 216, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, INT, iMenuHeight, 220, 4, 4);
    TEST_FIELD(NONCLIENTMETRICSW, LOGFONTW, lfMenuFont, 224, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, LOGFONTW, lfStatusFont, 316, 92, 4);
    TEST_FIELD(NONCLIENTMETRICSW, LOGFONTW, lfMessageFont, 408, 92, 4);
}

static void test_pack_PAINTSTRUCT(void)
{
    /* PAINTSTRUCT (pack 4) */
    TEST_TYPE(PAINTSTRUCT, 64, 4);
    TEST_FIELD(PAINTSTRUCT, HDC, hdc, 0, 4, 4);
    TEST_FIELD(PAINTSTRUCT, BOOL, fErase, 4, 4, 4);
    TEST_FIELD(PAINTSTRUCT, RECT, rcPaint, 8, 16, 4);
    TEST_FIELD(PAINTSTRUCT, BOOL, fRestore, 24, 4, 4);
    TEST_FIELD(PAINTSTRUCT, BOOL, fIncUpdate, 28, 4, 4);
    TEST_FIELD(PAINTSTRUCT, BYTE[32], rgbReserved, 32, 32, 1);
}

static void test_pack_PCOMBOBOXINFO(void)
{
    /* PCOMBOBOXINFO */
    TEST_TYPE(PCOMBOBOXINFO, 4, 4);
    TEST_TYPE_POINTER(PCOMBOBOXINFO, 52, 4);
}

static void test_pack_PCOMPAREITEMSTRUCT(void)
{
    /* PCOMPAREITEMSTRUCT */
    TEST_TYPE(PCOMPAREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCOMPAREITEMSTRUCT, 32, 4);
}

static void test_pack_PCOPYDATASTRUCT(void)
{
    /* PCOPYDATASTRUCT */
    TEST_TYPE(PCOPYDATASTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCOPYDATASTRUCT, 12, 4);
}

static void test_pack_PCURSORINFO(void)
{
    /* PCURSORINFO */
    TEST_TYPE(PCURSORINFO, 4, 4);
    TEST_TYPE_POINTER(PCURSORINFO, 20, 4);
}

static void test_pack_PCWPRETSTRUCT(void)
{
    /* PCWPRETSTRUCT */
    TEST_TYPE(PCWPRETSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCWPRETSTRUCT, 20, 4);
}

static void test_pack_PCWPSTRUCT(void)
{
    /* PCWPSTRUCT */
    TEST_TYPE(PCWPSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PCWPSTRUCT, 16, 4);
}

static void test_pack_PDEBUGHOOKINFO(void)
{
    /* PDEBUGHOOKINFO */
    TEST_TYPE(PDEBUGHOOKINFO, 4, 4);
    TEST_TYPE_POINTER(PDEBUGHOOKINFO, 20, 4);
}

static void test_pack_PDELETEITEMSTRUCT(void)
{
    /* PDELETEITEMSTRUCT */
    TEST_TYPE(PDELETEITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PDELETEITEMSTRUCT, 20, 4);
}

static void test_pack_PDLGITEMTEMPLATEA(void)
{
    /* PDLGITEMTEMPLATEA */
    TEST_TYPE(PDLGITEMTEMPLATEA, 4, 4);
    TEST_TYPE_POINTER(PDLGITEMTEMPLATEA, 18, 2);
}

static void test_pack_PDLGITEMTEMPLATEW(void)
{
    /* PDLGITEMTEMPLATEW */
    TEST_TYPE(PDLGITEMTEMPLATEW, 4, 4);
    TEST_TYPE_POINTER(PDLGITEMTEMPLATEW, 18, 2);
}

static void test_pack_PDRAWITEMSTRUCT(void)
{
    /* PDRAWITEMSTRUCT */
    TEST_TYPE(PDRAWITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PDRAWITEMSTRUCT, 48, 4);
}

static void test_pack_PEVENTMSG(void)
{
    /* PEVENTMSG */
    TEST_TYPE(PEVENTMSG, 4, 4);
    TEST_TYPE_POINTER(PEVENTMSG, 20, 4);
}

static void test_pack_PFLASHWINFO(void)
{
    /* PFLASHWINFO */
    TEST_TYPE(PFLASHWINFO, 4, 4);
    TEST_TYPE_POINTER(PFLASHWINFO, 20, 4);
}

static void test_pack_PGUITHREADINFO(void)
{
    /* PGUITHREADINFO */
    TEST_TYPE(PGUITHREADINFO, 4, 4);
    TEST_TYPE_POINTER(PGUITHREADINFO, 48, 4);
}

static void test_pack_PHARDWAREHOOKSTRUCT(void)
{
    /* PHARDWAREHOOKSTRUCT */
    TEST_TYPE(PHARDWAREHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PHARDWAREHOOKSTRUCT, 16, 4);
}

static void test_pack_PHARDWAREINPUT(void)
{
    /* PHARDWAREINPUT */
    TEST_TYPE(PHARDWAREINPUT, 4, 4);
    TEST_TYPE_POINTER(PHARDWAREINPUT, 8, 4);
}

static void test_pack_PHDEVNOTIFY(void)
{
    /* PHDEVNOTIFY */
    TEST_TYPE(PHDEVNOTIFY, 4, 4);
    TEST_TYPE_POINTER(PHDEVNOTIFY, 4, 4);
}

static void test_pack_PHELPWININFOA(void)
{
    /* PHELPWININFOA */
    TEST_TYPE(PHELPWININFOA, 4, 4);
    TEST_TYPE_POINTER(PHELPWININFOA, 28, 4);
}

static void test_pack_PHELPWININFOW(void)
{
    /* PHELPWININFOW */
    TEST_TYPE(PHELPWININFOW, 4, 4);
    TEST_TYPE_POINTER(PHELPWININFOW, 28, 4);
}

static void test_pack_PICONINFO(void)
{
    /* PICONINFO */
    TEST_TYPE(PICONINFO, 4, 4);
    TEST_TYPE_POINTER(PICONINFO, 20, 4);
}

static void test_pack_PICONMETRICSA(void)
{
    /* PICONMETRICSA */
    TEST_TYPE(PICONMETRICSA, 4, 4);
    TEST_TYPE_POINTER(PICONMETRICSA, 76, 4);
}

static void test_pack_PICONMETRICSW(void)
{
    /* PICONMETRICSW */
    TEST_TYPE(PICONMETRICSW, 4, 4);
    TEST_TYPE_POINTER(PICONMETRICSW, 108, 4);
}

static void test_pack_PINPUT(void)
{
    /* PINPUT */
    TEST_TYPE(PINPUT, 4, 4);
}

static void test_pack_PKBDLLHOOKSTRUCT(void)
{
    /* PKBDLLHOOKSTRUCT */
    TEST_TYPE(PKBDLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PKBDLLHOOKSTRUCT, 20, 4);
}

static void test_pack_PKEYBDINPUT(void)
{
    /* PKEYBDINPUT */
    TEST_TYPE(PKEYBDINPUT, 4, 4);
    TEST_TYPE_POINTER(PKEYBDINPUT, 16, 4);
}

static void test_pack_PMDINEXTMENU(void)
{
    /* PMDINEXTMENU */
    TEST_TYPE(PMDINEXTMENU, 4, 4);
    TEST_TYPE_POINTER(PMDINEXTMENU, 12, 4);
}

static void test_pack_PMEASUREITEMSTRUCT(void)
{
    /* PMEASUREITEMSTRUCT */
    TEST_TYPE(PMEASUREITEMSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMEASUREITEMSTRUCT, 24, 4);
}

static void test_pack_PMENUITEMTEMPLATE(void)
{
    /* PMENUITEMTEMPLATE */
    TEST_TYPE(PMENUITEMTEMPLATE, 4, 4);
    TEST_TYPE_POINTER(PMENUITEMTEMPLATE, 6, 2);
}

static void test_pack_PMENUITEMTEMPLATEHEADER(void)
{
    /* PMENUITEMTEMPLATEHEADER */
    TEST_TYPE(PMENUITEMTEMPLATEHEADER, 4, 4);
    TEST_TYPE_POINTER(PMENUITEMTEMPLATEHEADER, 4, 2);
}

static void test_pack_PMINIMIZEDMETRICS(void)
{
    /* PMINIMIZEDMETRICS */
    TEST_TYPE(PMINIMIZEDMETRICS, 4, 4);
    TEST_TYPE_POINTER(PMINIMIZEDMETRICS, 20, 4);
}

static void test_pack_PMINMAXINFO(void)
{
    /* PMINMAXINFO */
    TEST_TYPE(PMINMAXINFO, 4, 4);
    TEST_TYPE_POINTER(PMINMAXINFO, 40, 4);
}

static void test_pack_PMOUSEHOOKSTRUCT(void)
{
    /* PMOUSEHOOKSTRUCT */
    TEST_TYPE(PMOUSEHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMOUSEHOOKSTRUCT, 20, 4);
}

static void test_pack_PMOUSEINPUT(void)
{
    /* PMOUSEINPUT */
    TEST_TYPE(PMOUSEINPUT, 4, 4);
    TEST_TYPE_POINTER(PMOUSEINPUT, 24, 4);
}

static void test_pack_PMSG(void)
{
    /* PMSG */
    TEST_TYPE(PMSG, 4, 4);
    TEST_TYPE_POINTER(PMSG, 28, 4);
}

static void test_pack_PMSGBOXPARAMSA(void)
{
    /* PMSGBOXPARAMSA */
    TEST_TYPE(PMSGBOXPARAMSA, 4, 4);
    TEST_TYPE_POINTER(PMSGBOXPARAMSA, 40, 4);
}

static void test_pack_PMSGBOXPARAMSW(void)
{
    /* PMSGBOXPARAMSW */
    TEST_TYPE(PMSGBOXPARAMSW, 4, 4);
    TEST_TYPE_POINTER(PMSGBOXPARAMSW, 40, 4);
}

static void test_pack_PMSLLHOOKSTRUCT(void)
{
    /* PMSLLHOOKSTRUCT */
    TEST_TYPE(PMSLLHOOKSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PMSLLHOOKSTRUCT, 24, 4);
}

static void test_pack_PMULTIKEYHELPA(void)
{
    /* PMULTIKEYHELPA */
    TEST_TYPE(PMULTIKEYHELPA, 4, 4);
    TEST_TYPE_POINTER(PMULTIKEYHELPA, 8, 4);
}

static void test_pack_PMULTIKEYHELPW(void)
{
    /* PMULTIKEYHELPW */
    TEST_TYPE(PMULTIKEYHELPW, 4, 4);
    TEST_TYPE_POINTER(PMULTIKEYHELPW, 8, 4);
}

static void test_pack_PNONCLIENTMETRICSA(void)
{
    /* PNONCLIENTMETRICSA */
    TEST_TYPE(PNONCLIENTMETRICSA, 4, 4);
    TEST_TYPE_POINTER(PNONCLIENTMETRICSA, 340, 4);
}

static void test_pack_PNONCLIENTMETRICSW(void)
{
    /* PNONCLIENTMETRICSW */
    TEST_TYPE(PNONCLIENTMETRICSW, 4, 4);
    TEST_TYPE_POINTER(PNONCLIENTMETRICSW, 500, 4);
}

static void test_pack_PPAINTSTRUCT(void)
{
    /* PPAINTSTRUCT */
    TEST_TYPE(PPAINTSTRUCT, 4, 4);
    TEST_TYPE_POINTER(PPAINTSTRUCT, 64, 4);
}

static void test_pack_PROPENUMPROCA(void)
{
    /* PROPENUMPROCA */
    TEST_TYPE(PROPENUMPROCA, 4, 4);
}

static void test_pack_PROPENUMPROCEXA(void)
{
    /* PROPENUMPROCEXA */
    TEST_TYPE(PROPENUMPROCEXA, 4, 4);
}

static void test_pack_PROPENUMPROCEXW(void)
{
    /* PROPENUMPROCEXW */
    TEST_TYPE(PROPENUMPROCEXW, 4, 4);
}

static void test_pack_PROPENUMPROCW(void)
{
    /* PROPENUMPROCW */
    TEST_TYPE(PROPENUMPROCW, 4, 4);
}

static void test_pack_PTITLEBARINFO(void)
{
    /* PTITLEBARINFO */
    TEST_TYPE(PTITLEBARINFO, 4, 4);
    TEST_TYPE_POINTER(PTITLEBARINFO, 44, 4);
}

static void test_pack_PUSEROBJECTFLAGS(void)
{
    /* PUSEROBJECTFLAGS */
    TEST_TYPE(PUSEROBJECTFLAGS, 4, 4);
    TEST_TYPE_POINTER(PUSEROBJECTFLAGS, 12, 4);
}

static void test_pack_PWINDOWINFO(void)
{
    /* PWINDOWINFO */
    TEST_TYPE(PWINDOWINFO, 4, 4);
    TEST_TYPE_POINTER(PWINDOWINFO, 60, 4);
}

static void test_pack_PWINDOWPLACEMENT(void)
{
    /* PWINDOWPLACEMENT */
    TEST_TYPE(PWINDOWPLACEMENT, 4, 4);
    TEST_TYPE_POINTER(PWINDOWPLACEMENT, 44, 4);
}

static void test_pack_PWINDOWPOS(void)
{
    /* PWINDOWPOS */
    TEST_TYPE(PWINDOWPOS, 4, 4);
    TEST_TYPE_POINTER(PWINDOWPOS, 28, 4);
}

static void test_pack_PWNDCLASSA(void)
{
    /* PWNDCLASSA */
    TEST_TYPE(PWNDCLASSA, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSA, 40, 4);
}

static void test_pack_PWNDCLASSEXA(void)
{
    /* PWNDCLASSEXA */
    TEST_TYPE(PWNDCLASSEXA, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSEXA, 48, 4);
}

static void test_pack_PWNDCLASSEXW(void)
{
    /* PWNDCLASSEXW */
    TEST_TYPE(PWNDCLASSEXW, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSEXW, 48, 4);
}

static void test_pack_PWNDCLASSW(void)
{
    /* PWNDCLASSW */
    TEST_TYPE(PWNDCLASSW, 4, 4);
    TEST_TYPE_POINTER(PWNDCLASSW, 40, 4);
}

static void test_pack_SCROLLINFO(void)
{
    /* SCROLLINFO (pack 4) */
    TEST_TYPE(SCROLLINFO, 28, 4);
    TEST_FIELD(SCROLLINFO, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(SCROLLINFO, UINT, fMask, 4, 4, 4);
    TEST_FIELD(SCROLLINFO, INT, nMin, 8, 4, 4);
    TEST_FIELD(SCROLLINFO, INT, nMax, 12, 4, 4);
    TEST_FIELD(SCROLLINFO, UINT, nPage, 16, 4, 4);
    TEST_FIELD(SCROLLINFO, INT, nPos, 20, 4, 4);
    TEST_FIELD(SCROLLINFO, INT, nTrackPos, 24, 4, 4);
}

static void test_pack_SENDASYNCPROC(void)
{
    /* SENDASYNCPROC */
    TEST_TYPE(SENDASYNCPROC, 4, 4);
}

static void test_pack_SERIALKEYSA(void)
{
    /* SERIALKEYSA (pack 4) */
    TEST_TYPE(SERIALKEYSA, 28, 4);
    TEST_FIELD(SERIALKEYSA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(SERIALKEYSA, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(SERIALKEYSA, LPSTR, lpszActivePort, 8, 4, 4);
    TEST_FIELD(SERIALKEYSA, LPSTR, lpszPort, 12, 4, 4);
    TEST_FIELD(SERIALKEYSA, UINT, iBaudRate, 16, 4, 4);
    TEST_FIELD(SERIALKEYSA, UINT, iPortState, 20, 4, 4);
    TEST_FIELD(SERIALKEYSA, UINT, iActive, 24, 4, 4);
}

static void test_pack_SERIALKEYSW(void)
{
    /* SERIALKEYSW (pack 4) */
    TEST_TYPE(SERIALKEYSW, 28, 4);
    TEST_FIELD(SERIALKEYSW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(SERIALKEYSW, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(SERIALKEYSW, LPWSTR, lpszActivePort, 8, 4, 4);
    TEST_FIELD(SERIALKEYSW, LPWSTR, lpszPort, 12, 4, 4);
    TEST_FIELD(SERIALKEYSW, UINT, iBaudRate, 16, 4, 4);
    TEST_FIELD(SERIALKEYSW, UINT, iPortState, 20, 4, 4);
    TEST_FIELD(SERIALKEYSW, UINT, iActive, 24, 4, 4);
}

static void test_pack_SOUNDSENTRYA(void)
{
    /* SOUNDSENTRYA (pack 4) */
    TEST_TYPE(SOUNDSENTRYA, 48, 4);
    TEST_FIELD(SOUNDSENTRYA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSTextEffect, 8, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSTextEffectMSec, 12, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSTextEffectColorBits, 16, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSGrafEffect, 20, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSGrafEffectMSec, 24, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iFSGrafEffectColor, 28, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iWindowsEffect, 32, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iWindowsEffectMSec, 36, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, LPSTR, lpszWindowsEffectDLL, 40, 4, 4);
    TEST_FIELD(SOUNDSENTRYA, DWORD, iWindowsEffectOrdinal, 44, 4, 4);
}

static void test_pack_SOUNDSENTRYW(void)
{
    /* SOUNDSENTRYW (pack 4) */
    TEST_TYPE(SOUNDSENTRYW, 48, 4);
    TEST_FIELD(SOUNDSENTRYW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSTextEffect, 8, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSTextEffectMSec, 12, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSTextEffectColorBits, 16, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSGrafEffect, 20, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSGrafEffectMSec, 24, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iFSGrafEffectColor, 28, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iWindowsEffect, 32, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iWindowsEffectMSec, 36, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, LPWSTR, lpszWindowsEffectDLL, 40, 4, 4);
    TEST_FIELD(SOUNDSENTRYW, DWORD, iWindowsEffectOrdinal, 44, 4, 4);
}

static void test_pack_STICKYKEYS(void)
{
    /* STICKYKEYS (pack 4) */
    TEST_TYPE(STICKYKEYS, 8, 4);
    TEST_FIELD(STICKYKEYS, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(STICKYKEYS, DWORD, dwFlags, 4, 4, 4);
}

static void test_pack_STYLESTRUCT(void)
{
    /* STYLESTRUCT (pack 4) */
    TEST_TYPE(STYLESTRUCT, 8, 4);
    TEST_FIELD(STYLESTRUCT, DWORD, styleOld, 0, 4, 4);
    TEST_FIELD(STYLESTRUCT, DWORD, styleNew, 4, 4, 4);
}

static void test_pack_TIMERPROC(void)
{
    /* TIMERPROC */
    TEST_TYPE(TIMERPROC, 4, 4);
}

static void test_pack_TITLEBARINFO(void)
{
    /* TITLEBARINFO (pack 4) */
    TEST_TYPE(TITLEBARINFO, 44, 4);
    TEST_FIELD(TITLEBARINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(TITLEBARINFO, RECT, rcTitleBar, 4, 16, 4);
    TEST_FIELD(TITLEBARINFO, DWORD[CCHILDREN_TITLEBAR+1], rgstate, 20, 24, 4);
}

static void test_pack_TOGGLEKEYS(void)
{
    /* TOGGLEKEYS (pack 4) */
    TEST_TYPE(TOGGLEKEYS, 8, 4);
    TEST_FIELD(TOGGLEKEYS, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(TOGGLEKEYS, DWORD, dwFlags, 4, 4, 4);
}

static void test_pack_TPMPARAMS(void)
{
    /* TPMPARAMS (pack 4) */
    TEST_TYPE(TPMPARAMS, 20, 4);
    TEST_FIELD(TPMPARAMS, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(TPMPARAMS, RECT, rcExclude, 4, 16, 4);
}

static void test_pack_TRACKMOUSEEVENT(void)
{
    /* TRACKMOUSEEVENT (pack 4) */
    TEST_TYPE(TRACKMOUSEEVENT, 16, 4);
    TEST_FIELD(TRACKMOUSEEVENT, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, HWND, hwndTrack, 8, 4, 4);
    TEST_FIELD(TRACKMOUSEEVENT, DWORD, dwHoverTime, 12, 4, 4);
}

static void test_pack_USEROBJECTFLAGS(void)
{
    /* USEROBJECTFLAGS (pack 4) */
    TEST_TYPE(USEROBJECTFLAGS, 12, 4);
    TEST_FIELD(USEROBJECTFLAGS, BOOL, fInherit, 0, 4, 4);
    TEST_FIELD(USEROBJECTFLAGS, BOOL, fReserved, 4, 4, 4);
    TEST_FIELD(USEROBJECTFLAGS, DWORD, dwFlags, 8, 4, 4);
}

static void test_pack_WINDOWINFO(void)
{
    /* WINDOWINFO (pack 4) */
    TEST_TYPE(WINDOWINFO, 60, 4);
    TEST_FIELD(WINDOWINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(WINDOWINFO, RECT, rcWindow, 4, 16, 4);
    TEST_FIELD(WINDOWINFO, RECT, rcClient, 20, 16, 4);
    TEST_FIELD(WINDOWINFO, DWORD, dwStyle, 36, 4, 4);
    TEST_FIELD(WINDOWINFO, DWORD, dwExStyle, 40, 4, 4);
    TEST_FIELD(WINDOWINFO, DWORD, dwWindowStatus, 44, 4, 4);
    TEST_FIELD(WINDOWINFO, UINT, cxWindowBorders, 48, 4, 4);
    TEST_FIELD(WINDOWINFO, UINT, cyWindowBorders, 52, 4, 4);
    TEST_FIELD(WINDOWINFO, ATOM, atomWindowType, 56, 2, 2);
    TEST_FIELD(WINDOWINFO, WORD, wCreatorVersion, 58, 2, 2);
}

static void test_pack_WINDOWPLACEMENT(void)
{
    /* WINDOWPLACEMENT (pack 4) */
    TEST_TYPE(WINDOWPLACEMENT, 44, 4);
    TEST_FIELD(WINDOWPLACEMENT, UINT, length, 0, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, UINT, flags, 4, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, UINT, showCmd, 8, 4, 4);
    TEST_FIELD(WINDOWPLACEMENT, POINT, ptMinPosition, 12, 8, 4);
    TEST_FIELD(WINDOWPLACEMENT, POINT, ptMaxPosition, 20, 8, 4);
    TEST_FIELD(WINDOWPLACEMENT, RECT, rcNormalPosition, 28, 16, 4);
}

static void test_pack_WINDOWPOS(void)
{
    /* WINDOWPOS (pack 4) */
    TEST_TYPE(WINDOWPOS, 28, 4);
    TEST_FIELD(WINDOWPOS, HWND, hwnd, 0, 4, 4);
    TEST_FIELD(WINDOWPOS, HWND, hwndInsertAfter, 4, 4, 4);
    TEST_FIELD(WINDOWPOS, INT, x, 8, 4, 4);
    TEST_FIELD(WINDOWPOS, INT, y, 12, 4, 4);
    TEST_FIELD(WINDOWPOS, INT, cx, 16, 4, 4);
    TEST_FIELD(WINDOWPOS, INT, cy, 20, 4, 4);
    TEST_FIELD(WINDOWPOS, UINT, flags, 24, 4, 4);
}

static void test_pack_WINEVENTPROC(void)
{
    /* WINEVENTPROC */
    TEST_TYPE(WINEVENTPROC, 4, 4);
}

static void test_pack_WINSTAENUMPROCA(void)
{
    /* WINSTAENUMPROCA */
    TEST_TYPE(WINSTAENUMPROCA, 4, 4);
}

static void test_pack_WINSTAENUMPROCW(void)
{
    /* WINSTAENUMPROCW */
    TEST_TYPE(WINSTAENUMPROCW, 4, 4);
}

static void test_pack_WNDCLASSA(void)
{
    /* WNDCLASSA (pack 4) */
    TEST_TYPE(WNDCLASSA, 40, 4);
    TEST_FIELD(WNDCLASSA, UINT, style, 0, 4, 4);
    TEST_FIELD(WNDCLASSA, WNDPROC, lpfnWndProc, 4, 4, 4);
    TEST_FIELD(WNDCLASSA, INT, cbClsExtra, 8, 4, 4);
    TEST_FIELD(WNDCLASSA, INT, cbWndExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSA, HINSTANCE, hInstance, 16, 4, 4);
    TEST_FIELD(WNDCLASSA, HICON, hIcon, 20, 4, 4);
    TEST_FIELD(WNDCLASSA, HCURSOR, hCursor, 24, 4, 4);
    TEST_FIELD(WNDCLASSA, HBRUSH, hbrBackground, 28, 4, 4);
    TEST_FIELD(WNDCLASSA, LPCSTR, lpszMenuName, 32, 4, 4);
    TEST_FIELD(WNDCLASSA, LPCSTR, lpszClassName, 36, 4, 4);
}

static void test_pack_WNDCLASSEXA(void)
{
    /* WNDCLASSEXA (pack 4) */
    TEST_TYPE(WNDCLASSEXA, 48, 4);
    TEST_FIELD(WNDCLASSEXA, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(WNDCLASSEXA, UINT, style, 4, 4, 4);
    TEST_FIELD(WNDCLASSEXA, WNDPROC, lpfnWndProc, 8, 4, 4);
    TEST_FIELD(WNDCLASSEXA, INT, cbClsExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSEXA, INT, cbWndExtra, 16, 4, 4);
    TEST_FIELD(WNDCLASSEXA, HINSTANCE, hInstance, 20, 4, 4);
    TEST_FIELD(WNDCLASSEXA, HICON, hIcon, 24, 4, 4);
    TEST_FIELD(WNDCLASSEXA, HCURSOR, hCursor, 28, 4, 4);
    TEST_FIELD(WNDCLASSEXA, HBRUSH, hbrBackground, 32, 4, 4);
    TEST_FIELD(WNDCLASSEXA, LPCSTR, lpszMenuName, 36, 4, 4);
    TEST_FIELD(WNDCLASSEXA, LPCSTR, lpszClassName, 40, 4, 4);
    TEST_FIELD(WNDCLASSEXA, HICON, hIconSm, 44, 4, 4);
}

static void test_pack_WNDCLASSEXW(void)
{
    /* WNDCLASSEXW (pack 4) */
    TEST_TYPE(WNDCLASSEXW, 48, 4);
    TEST_FIELD(WNDCLASSEXW, UINT, cbSize, 0, 4, 4);
    TEST_FIELD(WNDCLASSEXW, UINT, style, 4, 4, 4);
    TEST_FIELD(WNDCLASSEXW, WNDPROC, lpfnWndProc, 8, 4, 4);
    TEST_FIELD(WNDCLASSEXW, INT, cbClsExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSEXW, INT, cbWndExtra, 16, 4, 4);
    TEST_FIELD(WNDCLASSEXW, HINSTANCE, hInstance, 20, 4, 4);
    TEST_FIELD(WNDCLASSEXW, HICON, hIcon, 24, 4, 4);
    TEST_FIELD(WNDCLASSEXW, HCURSOR, hCursor, 28, 4, 4);
    TEST_FIELD(WNDCLASSEXW, HBRUSH, hbrBackground, 32, 4, 4);
    TEST_FIELD(WNDCLASSEXW, LPCWSTR, lpszMenuName, 36, 4, 4);
    TEST_FIELD(WNDCLASSEXW, LPCWSTR, lpszClassName, 40, 4, 4);
    TEST_FIELD(WNDCLASSEXW, HICON, hIconSm, 44, 4, 4);
}

static void test_pack_WNDCLASSW(void)
{
    /* WNDCLASSW (pack 4) */
    TEST_TYPE(WNDCLASSW, 40, 4);
    TEST_FIELD(WNDCLASSW, UINT, style, 0, 4, 4);
    TEST_FIELD(WNDCLASSW, WNDPROC, lpfnWndProc, 4, 4, 4);
    TEST_FIELD(WNDCLASSW, INT, cbClsExtra, 8, 4, 4);
    TEST_FIELD(WNDCLASSW, INT, cbWndExtra, 12, 4, 4);
    TEST_FIELD(WNDCLASSW, HINSTANCE, hInstance, 16, 4, 4);
    TEST_FIELD(WNDCLASSW, HICON, hIcon, 20, 4, 4);
    TEST_FIELD(WNDCLASSW, HCURSOR, hCursor, 24, 4, 4);
    TEST_FIELD(WNDCLASSW, HBRUSH, hbrBackground, 28, 4, 4);
    TEST_FIELD(WNDCLASSW, LPCWSTR, lpszMenuName, 32, 4, 4);
    TEST_FIELD(WNDCLASSW, LPCWSTR, lpszClassName, 36, 4, 4);
}

static void test_pack_WNDENUMPROC(void)
{
    /* WNDENUMPROC */
    TEST_TYPE(WNDENUMPROC, 4, 4);
}

static void test_pack_WNDPROC(void)
{
    /* WNDPROC */
    TEST_TYPE(WNDPROC, 4, 4);
}

static void test_pack(void)
{
    test_pack_ACCESSTIMEOUT();
    test_pack_ANIMATIONINFO();
    test_pack_CBTACTIVATESTRUCT();
    test_pack_CBT_CREATEWNDA();
    test_pack_CBT_CREATEWNDW();
    test_pack_CLIENTCREATESTRUCT();
    test_pack_COMBOBOXINFO();
    test_pack_COMPAREITEMSTRUCT();
    test_pack_COPYDATASTRUCT();
    test_pack_CREATESTRUCTA();
    test_pack_CREATESTRUCTW();
    test_pack_CURSORINFO();
    test_pack_CWPRETSTRUCT();
    test_pack_CWPSTRUCT();
    test_pack_DEBUGHOOKINFO();
    test_pack_DELETEITEMSTRUCT();
    test_pack_DESKTOPENUMPROCA();
    test_pack_DESKTOPENUMPROCW();
    test_pack_DLGITEMTEMPLATE();
    test_pack_DLGPROC();
    test_pack_DLGTEMPLATE();
    test_pack_DRAWITEMSTRUCT();
    test_pack_DRAWSTATEPROC();
    test_pack_DRAWTEXTPARAMS();
    test_pack_EDITWORDBREAKPROCA();
    test_pack_EDITWORDBREAKPROCW();
    test_pack_EVENTMSG();
    test_pack_FILTERKEYS();
    test_pack_FLASHWINFO();
    test_pack_GRAYSTRINGPROC();
    test_pack_GUITHREADINFO();
    test_pack_HARDWAREHOOKSTRUCT();
    test_pack_HARDWAREINPUT();
    test_pack_HDEVNOTIFY();
    test_pack_HDWP();
    test_pack_HELPINFO();
    test_pack_HELPWININFOA();
    test_pack_HELPWININFOW();
    test_pack_HIGHCONTRASTA();
    test_pack_HIGHCONTRASTW();
    test_pack_HOOKPROC();
    test_pack_ICONINFO();
    test_pack_ICONMETRICSA();
    test_pack_ICONMETRICSW();
    test_pack_INPUT();
    test_pack_KBDLLHOOKSTRUCT();
    test_pack_KEYBDINPUT();
    test_pack_LPACCESSTIMEOUT();
    test_pack_LPANIMATIONINFO();
    test_pack_LPCBTACTIVATESTRUCT();
    test_pack_LPCBT_CREATEWNDA();
    test_pack_LPCBT_CREATEWNDW();
    test_pack_LPCDLGTEMPLATEA();
    test_pack_LPCDLGTEMPLATEW();
    test_pack_LPCLIENTCREATESTRUCT();
    test_pack_LPCMENUINFO();
    test_pack_LPCMENUITEMINFOA();
    test_pack_LPCMENUITEMINFOW();
    test_pack_LPCOMBOBOXINFO();
    test_pack_LPCOMPAREITEMSTRUCT();
    test_pack_LPCREATESTRUCTA();
    test_pack_LPCREATESTRUCTW();
    test_pack_LPCSCROLLINFO();
    test_pack_LPCURSORINFO();
    test_pack_LPCWPRETSTRUCT();
    test_pack_LPCWPSTRUCT();
    test_pack_LPDEBUGHOOKINFO();
    test_pack_LPDELETEITEMSTRUCT();
    test_pack_LPDLGITEMTEMPLATEA();
    test_pack_LPDLGITEMTEMPLATEW();
    test_pack_LPDLGTEMPLATEA();
    test_pack_LPDLGTEMPLATEW();
    test_pack_LPDRAWITEMSTRUCT();
    test_pack_LPDRAWTEXTPARAMS();
    test_pack_LPEVENTMSG();
    test_pack_LPFILTERKEYS();
    test_pack_LPGUITHREADINFO();
    test_pack_LPHARDWAREHOOKSTRUCT();
    test_pack_LPHARDWAREINPUT();
    test_pack_LPHELPINFO();
    test_pack_LPHELPWININFOA();
    test_pack_LPHELPWININFOW();
    test_pack_LPHIGHCONTRASTA();
    test_pack_LPHIGHCONTRASTW();
    test_pack_LPICONMETRICSA();
    test_pack_LPICONMETRICSW();
    test_pack_LPINPUT();
    test_pack_LPKBDLLHOOKSTRUCT();
    test_pack_LPKEYBDINPUT();
    test_pack_LPMDICREATESTRUCTA();
    test_pack_LPMDICREATESTRUCTW();
    test_pack_LPMDINEXTMENU();
    test_pack_LPMEASUREITEMSTRUCT();
    test_pack_LPMENUINFO();
    test_pack_LPMENUITEMINFOA();
    test_pack_LPMENUITEMINFOW();
    test_pack_LPMINIMIZEDMETRICS();
    test_pack_LPMINMAXINFO();
    test_pack_LPMONITORINFO();
    test_pack_LPMONITORINFOEXA();
    test_pack_LPMONITORINFOEXW();
    test_pack_LPMOUSEHOOKSTRUCT();
    test_pack_LPMOUSEINPUT();
    test_pack_LPMOUSEKEYS();
    test_pack_LPMSG();
    test_pack_LPMSGBOXPARAMSA();
    test_pack_LPMSGBOXPARAMSW();
    test_pack_LPMSLLHOOKSTRUCT();
    test_pack_LPMULTIKEYHELPA();
    test_pack_LPMULTIKEYHELPW();
    test_pack_LPNCCALCSIZE_PARAMS();
    test_pack_LPNMHDR();
    test_pack_LPNONCLIENTMETRICSA();
    test_pack_LPNONCLIENTMETRICSW();
    test_pack_LPPAINTSTRUCT();
    test_pack_LPSCROLLINFO();
    test_pack_LPSERIALKEYSA();
    test_pack_LPSERIALKEYSW();
    test_pack_LPSOUNDSENTRYA();
    test_pack_LPSOUNDSENTRYW();
    test_pack_LPSTICKYKEYS();
    test_pack_LPSTYLESTRUCT();
    test_pack_LPTITLEBARINFO();
    test_pack_LPTOGGLEKEYS();
    test_pack_LPTPMPARAMS();
    test_pack_LPTRACKMOUSEEVENT();
    test_pack_LPWINDOWINFO();
    test_pack_LPWINDOWPLACEMENT();
    test_pack_LPWINDOWPOS();
    test_pack_LPWNDCLASSA();
    test_pack_LPWNDCLASSEXA();
    test_pack_LPWNDCLASSEXW();
    test_pack_LPWNDCLASSW();
    test_pack_MDICREATESTRUCTA();
    test_pack_MDICREATESTRUCTW();
    test_pack_MDINEXTMENU();
    test_pack_MEASUREITEMSTRUCT();
    test_pack_MENUINFO();
    test_pack_MENUITEMINFOA();
    test_pack_MENUITEMINFOW();
    test_pack_MENUITEMTEMPLATE();
    test_pack_MENUITEMTEMPLATEHEADER();
    test_pack_MINIMIZEDMETRICS();
    test_pack_MINMAXINFO();
    test_pack_MONITORENUMPROC();
    test_pack_MONITORINFO();
    test_pack_MONITORINFOEXA();
    test_pack_MONITORINFOEXW();
    test_pack_MOUSEHOOKSTRUCT();
    test_pack_MOUSEINPUT();
    test_pack_MOUSEKEYS();
    test_pack_MSG();
    test_pack_MSGBOXCALLBACK();
    test_pack_MSGBOXPARAMSA();
    test_pack_MSGBOXPARAMSW();
    test_pack_MSLLHOOKSTRUCT();
    test_pack_MULTIKEYHELPA();
    test_pack_MULTIKEYHELPW();
    test_pack_NAMEENUMPROCA();
    test_pack_NAMEENUMPROCW();
    test_pack_NCCALCSIZE_PARAMS();
    test_pack_NMHDR();
    test_pack_NONCLIENTMETRICSA();
    test_pack_NONCLIENTMETRICSW();
    test_pack_PAINTSTRUCT();
    test_pack_PCOMBOBOXINFO();
    test_pack_PCOMPAREITEMSTRUCT();
    test_pack_PCOPYDATASTRUCT();
    test_pack_PCURSORINFO();
    test_pack_PCWPRETSTRUCT();
    test_pack_PCWPSTRUCT();
    test_pack_PDEBUGHOOKINFO();
    test_pack_PDELETEITEMSTRUCT();
    test_pack_PDLGITEMTEMPLATEA();
    test_pack_PDLGITEMTEMPLATEW();
    test_pack_PDRAWITEMSTRUCT();
    test_pack_PEVENTMSG();
    test_pack_PFLASHWINFO();
    test_pack_PGUITHREADINFO();
    test_pack_PHARDWAREHOOKSTRUCT();
    test_pack_PHARDWAREINPUT();
    test_pack_PHDEVNOTIFY();
    test_pack_PHELPWININFOA();
    test_pack_PHELPWININFOW();
    test_pack_PICONINFO();
    test_pack_PICONMETRICSA();
    test_pack_PICONMETRICSW();
    test_pack_PINPUT();
    test_pack_PKBDLLHOOKSTRUCT();
    test_pack_PKEYBDINPUT();
    test_pack_PMDINEXTMENU();
    test_pack_PMEASUREITEMSTRUCT();
    test_pack_PMENUITEMTEMPLATE();
    test_pack_PMENUITEMTEMPLATEHEADER();
    test_pack_PMINIMIZEDMETRICS();
    test_pack_PMINMAXINFO();
    test_pack_PMOUSEHOOKSTRUCT();
    test_pack_PMOUSEINPUT();
    test_pack_PMSG();
    test_pack_PMSGBOXPARAMSA();
    test_pack_PMSGBOXPARAMSW();
    test_pack_PMSLLHOOKSTRUCT();
    test_pack_PMULTIKEYHELPA();
    test_pack_PMULTIKEYHELPW();
    test_pack_PNONCLIENTMETRICSA();
    test_pack_PNONCLIENTMETRICSW();
    test_pack_PPAINTSTRUCT();
    test_pack_PROPENUMPROCA();
    test_pack_PROPENUMPROCEXA();
    test_pack_PROPENUMPROCEXW();
    test_pack_PROPENUMPROCW();
    test_pack_PTITLEBARINFO();
    test_pack_PUSEROBJECTFLAGS();
    test_pack_PWINDOWINFO();
    test_pack_PWINDOWPLACEMENT();
    test_pack_PWINDOWPOS();
    test_pack_PWNDCLASSA();
    test_pack_PWNDCLASSEXA();
    test_pack_PWNDCLASSEXW();
    test_pack_PWNDCLASSW();
    test_pack_SCROLLINFO();
    test_pack_SENDASYNCPROC();
    test_pack_SERIALKEYSA();
    test_pack_SERIALKEYSW();
    test_pack_SOUNDSENTRYA();
    test_pack_SOUNDSENTRYW();
    test_pack_STICKYKEYS();
    test_pack_STYLESTRUCT();
    test_pack_TIMERPROC();
    test_pack_TITLEBARINFO();
    test_pack_TOGGLEKEYS();
    test_pack_TPMPARAMS();
    test_pack_TRACKMOUSEEVENT();
    test_pack_USEROBJECTFLAGS();
    test_pack_WINDOWINFO();
    test_pack_WINDOWPLACEMENT();
    test_pack_WINDOWPOS();
    test_pack_WINEVENTPROC();
    test_pack_WINSTAENUMPROCA();
    test_pack_WINSTAENUMPROCW();
    test_pack_WNDCLASSA();
    test_pack_WNDCLASSEXA();
    test_pack_WNDCLASSEXW();
    test_pack_WNDCLASSW();
    test_pack_WNDENUMPROC();
    test_pack_WNDPROC();
}

START_TEST(generated)
{
    test_pack();
}
