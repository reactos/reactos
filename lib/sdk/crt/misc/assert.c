/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static const char formatstr[] =
    "Assertion failed!\n\n"
    "Program: %s\n"
    "File: %s\n"
    "Line: %ld\n\n"
    "Expression: %s\n"
    "Press Retry to debug the application\n";


/*
 * @implemented
 */
void _assert(const char *exp, const char *file, unsigned line)
{
    int (WINAPI *pMessageBoxA)(HWND, LPCTSTR, LPCTSTR, UINT);
    HMODULE hmodUser32;
    char achProgram[40];
    char *pszBuffer;
    size_t len;
    int iResult;

    /* Assertion failed at foo.c line 45: x<y */
    fprintf(stderr, "Assertion failed at %s line %d: %s\n", file, line, exp);
    FIXME("Assertion failed at %s line %d: %s\n", file, line, exp);

    /* Get MessageBoxA function pointer */
    hmodUser32 = LoadLibrary("user32.dll");
    pMessageBoxA = (PVOID)GetProcAddress(hmodUser32, "MessageBoxA");
    if (!pMessageBoxA)
    {
        abort();
    }

    /* Get the file name of the module */
    len = GetModuleFileNameA(NULL, achProgram, 40);

    /* Calculate full length of the message */
    len += sizeof(formatstr) + len + strlen(exp) + strlen(file);

    /* Allocate a buffer */
    pszBuffer = malloc(len + 1);

    /* Format a message */
    _snprintf(pszBuffer, len, formatstr, achProgram, file, line, exp);

    /* Display a message box */
    iResult = pMessageBoxA(NULL,
                          pszBuffer,
                          "ReactOS C Runtime Library",
                          MB_ABORTRETRYIGNORE | MB_ICONERROR);

    free(pszBuffer);

    /* Does the user want to abort? */
    if (iResult == IDABORT)
    {
        abort();
    }

    /* Does the user want to debug? */
    if (iResult == IDRETRY)
    {
        DbgRaiseAssertionFailure();
    }
}
