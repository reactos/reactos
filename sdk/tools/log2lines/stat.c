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
#include "util.h"
#include "log2lines.h"

void
stat_print(FILE *outFile, PSUMM psumm)
{
    if (outFile)
    {
        clilog(outFile, "*** LOG2LINES SUMMARY ***\n");
        clilog(outFile, "Translated:               %d\n", psumm->translated);
        clilog(outFile, "Reverted:                 %d\n", psumm->undo);
        clilog(outFile, "Retranslated:             %d\n", psumm->redo);
        clilog(outFile, "Skipped:                  %d\n", psumm->skipped);
        clilog(outFile, "Differ:                   %d\n", psumm->diff);
        clilog(outFile, "Differ (function/source): %d\n", psumm->majordiff);
        clilog(outFile, "Revision conflicts:       %d\n", psumm->revconflicts);
        clilog(outFile, "Regression candidates:    %d\n", psumm->regfound);
        clilog(outFile, "Offset error:             %d\n", psumm->offset_errors);
        clilog(outFile, "Total:                    %d\n", psumm->total);
        clilog(outFile, "-------------------------------\n");
        clilog(outFile, "Log2lines version: " LOG2LINES_VERSION "\n");
        clilog(outFile, "Directory:         %s\n", opt_dir);
        clilog(outFile, "Passed options:    %s\n", opt_scanned);
        clilog(outFile, "-------------------------------\n");
    }
}

void
stat_clear(PSUMM psumm)
{
    memset(psumm, 0, sizeof(SUMM));
}

/* EOF */
