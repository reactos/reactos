/*
 * PROJECT:     ReactOS Automatic Testing Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various helper functions
 * COPYRIGHT:   Copyright 2008-2009 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * Escapes a string according to RFC 1738.
 * Required for passing parameters to the web service.
 *
 * @param Output
 * Pointer to a CHAR array, which will receive the escaped string.
 * The output buffer must be large enough to hold the full escaped string. You're on the safe side
 * if you make the output buffer three times as large as the input buffer.
 *
 * @param Input
 * Pointer to a CHAR array, which contains the input buffer to escape.
 */
VOID
EscapeString(PCHAR Output, PCHAR Input)
{
    const CHAR HexCharacters[] = "0123456789ABCDEF";

    do
    {
        if(strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~", *Input) )
        {
            /* It's a character we don't need to escape, just add it to the output string */
            *Output++ = *Input;
        }
        else
        {
            /* We need to escape this character */
            *Output++ = '%';
            *Output++ = HexCharacters[((UCHAR)*Input >> 4) % 16];
            *Output++ = HexCharacters[(UCHAR)*Input % 16];
        }
    }
    while(*++Input);

    *Output = 0;
}

/**
 * Outputs a string through the standard output and the debug output.
 * The string may have LF or CRLF line endings.
 *
 * @param String
 * The string to output
 */
VOID
StringOut(PCHAR String)
{
    PCHAR NewString;
    PCHAR pNewString;
    size_t Length;

    /* The piped output of the tests may use CRLF line endings, so convert them to LF.
       As both printf and OutputDebugStringA operate in text mode, the line-endings will be properly converted again later. */
    Length = strlen(String);
    NewString = HeapAlloc(hProcessHeap, 0, Length + 1);
    pNewString = NewString;

    do
    {
        /* If this is a CRLF line-ending, only copy a \n to the new string and skip the next character */
        if(*String == '\r' && *(String + 1) == '\n')
        {
            *pNewString = '\n';
            ++String;
        }
        else
        {
            /* Otherwise copy the string */
            *pNewString = *String;
        }

        ++pNewString;
    }
    while(*++String);

    /* Null-terminate it */
    *pNewString = 0;

    /* Output it */
    printf(NewString);
    OutputDebugStringA(NewString);

    /* Cleanup */
    HeapFree(hProcessHeap, 0, NewString);
}
