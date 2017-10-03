/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - SVN interface and revision analysis
 */

#pragma once

#include <stdio.h>

typedef struct revinfo_struct
{
    int     rev;
    int     buildrev;
    int     range;
    int     opt_verbose;
} REVINFO, *PREVINFO;

int getRevision(char *fileName, int lastChanged);
int getTBRevision(char *fileName);
void reportRevision(FILE *outFile);
unsigned long findRev(FILE *finx, int *rev);
int regscan(FILE *outFile);
int updateSvnlog(void);

/* EOF */
