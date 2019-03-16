/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Cli for escape commands
 */

#pragma once

#include <stdio.h>

#define KDBG_BS_CHAR    0x08
#define KDBG_ESC_CHAR   '`'
#define KDBG_ESC_STR    "`"
#define KDBG_ESC_RESP   "| L2L- "
#define KDBG_ESC_OFF    "off"
#define KDBG_PROMPT     "kdb:>"                     //Start interactive (-c) after this pattern
#define KDBG_CONT       "---"                       //Also after this pattern (prompt with no line ending)
#define KDBG_DISCARD    "Command '" KDBG_ESC_STR    //Discard responses at l2l escape commands

char handle_escape_cmd(FILE *outFile, char *Line);

/* EOF */
