/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Statistics
 */

#pragma once

#include <stdio.h>

typedef struct summ_struct
{
    int translated;
    int undo;
    int redo;
    int skipped;
    int diff;
    int majordiff;
    int revconflicts;
    int regfound;
    int offset_errors;
    int total;
} SUMM, *PSUMM;

void stat_print(FILE *outFile, PSUMM psumm);
void stat_clear(PSUMM psumm);

/* EOF */
