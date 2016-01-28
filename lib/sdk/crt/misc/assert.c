/*
 * PROJECT:         ReactOS C runtime library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/sdk/crt/misc/assert.c
 * PURPOSE:         _assert implementation
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <precomp.h>

static const char formatstr[] =
    "Assertion failed!\n\n"
    "Program: %s\n"
    "File: %s\n"
    "Line: %ld\n\n"
    "Expression: %s\n"
    "Press Retry to debug the application\n\0";

void
_assert (
    const char *exp,
    const char *file,
    unsigned line)
{
    char achProgram[MAX_PATH];
    char *pszBuffer;
    size_t len;
    int iResult;

    /* First common debug message */
    FIXME("Assertion failed: %s, file %s, line %d\n", exp, file, line);

    /* Check if output should go to stderr */
    if (((msvcrt_error_mode == _OUT_TO_DEFAULT) && (__app_type == _CONSOLE_APP)) ||
        (msvcrt_error_mode == _OUT_TO_STDERR))
    {
        /* Print 'Assertion failed: x<y, file foo.c, line 45' to stderr */
        fprintf(stderr, "Assertion failed: %s, file %s, line %u\n", exp, file, line);
        abort();
    }

    /* Get the file name of the module */
    len = GetModuleFileNameA(NULL, achProgram, sizeof(achProgram));

    /* Calculate full length of the message */
    len += sizeof(formatstr) + len + strlen(exp) + strlen(file);

    /* Allocate a buffer */
    pszBuffer = malloc(len + 1);

    /* Format a message */
    _snprintf(pszBuffer, len, formatstr, achProgram, file, line, exp);

    /* Display a message box */
    iResult = __crt_MessageBoxA(pszBuffer, MB_ABORTRETRYIGNORE | MB_ICONERROR);

    /* Free the buffer */
    free(pszBuffer);

    /* Does the user want to ignore? */
    if (iResult == IDIGNORE)
    {
        /* Just return to the caller */
        return;
    }

    /* Does the user want to debug? */
    if (iResult == IDRETRY)
    {
        /* Break and return to the caller */
        __debugbreak();
        return;
    }

    /* Reset all abort flags (we don*t want another message box) and abort */
    _set_abort_behavior(0, 0xffffffff);
    abort();
}

