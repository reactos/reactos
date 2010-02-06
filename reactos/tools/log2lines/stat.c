/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Statistics
 */

#include <stdio.h>
#include <string.h>

#include "version.h"
#include "options.h"
#include "stat.h"
#include "log2lines.h"

void
stat_print(FILE *outFile, PSUMM psumm)
{
    if (outFile)
    {
        fprintf(outFile, "\n*** LOG2LINES SUMMARY ***\n");
        fprintf(outFile, "Translated:               %d\n", psumm->translated);
        fprintf(outFile, "Reverted:                 %d\n", psumm->undo);
        fprintf(outFile, "Retranslated:             %d\n", psumm->redo);
        fprintf(outFile, "Skipped:                  %d\n", psumm->skipped);
        fprintf(outFile, "Differ:                   %d\n", psumm->diff);
        fprintf(outFile, "Differ (function/source): %d\n", psumm->majordiff);
        fprintf(outFile, "Revision conflicts:       %d\n", psumm->revconflicts);
        fprintf(outFile, "Regression candidates:    %d\n", psumm->regfound);
        fprintf(outFile, "Offset error:             %d\n", psumm->offset_errors);
        fprintf(outFile, "Total:                    %d\n", psumm->total);
        fprintf(outFile, "-------------------------------\n");
        fprintf(outFile, "Log2lines version: " LOG2LINES_VERSION "\n");
        fprintf(outFile, "Directory:         %s\n", opt_dir);
        fprintf(outFile, "Passed options:    %s\n", opt_scanned);
        fprintf(outFile, "-------------------------------\n");
    }
}

void 
stat_clear(PSUMM psumm)
{
    memset(psumm, 0, sizeof(SUMM));
}
