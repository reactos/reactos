/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Statistics
 */

#ifndef __L2L_STAT_H__
#define __L2L_STAT_H__

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

#endif /* __L2L_STAT_H__ */
