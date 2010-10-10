/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - SVN interface and revision analysis
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "version.h"
#include "compat.h"
#include "util.h"
#include "options.h"
#include "log2lines.h"

static void
log_rev_check(FILE *outFile, char *fileName, int showfile)
{
    int rev = 0;
    char s[LINESIZE];

    strcpy(s, opt_SourcesPath);
    strcat(s, fileName);
    rev = getRevision(s, 1);
    if (!showfile)
        s[0] = '\0';
    if (revinfo.opt_verbose)
        log(outFile, "| R--- %s Last Changed Rev: %d\n", s, rev);

    if (rev && opt_Revision_check)
    {
        if (revinfo.rev < revinfo.buildrev)
        {
            summ.revconflicts++;
            log(outFile, "| R--- Conflict %s: source tree(%d) < build(%d)\n", s, rev, revinfo.buildrev);
        }
        else if (rev > revinfo.buildrev)
        {
            summ.revconflicts++;
            log(outFile, "| R--- Conflict %s: file(%d) > build(%d)\n", s, rev, revinfo.buildrev);
        }
    }
}

static void
logRevCheck(FILE *outFile)
{
    int twice = 0;

    twice = (lastLine.nr2 && strcmp(lastLine.file1, lastLine.file2) != 0);
    log_rev_check(outFile, lastLine.file1, twice);
    if (twice)
    {
        log_rev_check(outFile, lastLine.file2, twice);
    }
}

int
getRevision(char *fileName, int lastChanged)
{
    char s[LINESIZE];
    FILE *psvn;
    int rev = 0;

    if (!fileName)
        fileName = opt_SourcesPath;
    sprintf(s, "svn info %s", fileName);
    if ((psvn = POPEN(s, "r")))
    {
        while (fgets(s, LINESIZE, psvn))
        {
            if (lastChanged)
            {
                if (sscanf(s, "Last Changed Rev: %d", &rev))
                    break;
            }
            else
            {
                if (sscanf(s, "Revision: %d", &rev))
                    break;
            }
        }
    }
    else
        l2l_dbg(1, "Can't popen: \"%s\"\n", s);

    if (psvn)
        PCLOSE(psvn);

    return rev;
}

int
getTBRevision(char *fileName)
{
    char *s;
    int rev = 0;

    s = strrchr(fileName, PATH_CHAR);
    if (s)
        s += 1;
    else
        s = fileName;

    sscanf(s, TRKBUILDPREFIX "%d", &rev);
    if (!rev)
    {
        s = strrchr(fileName, PATH_CHAR);
        if (s)
            *s = '\0'; // clear, so we have the parent dir
        else
        {
            // where else to look?
            fileName = opt_SourcesPath;
        }
        rev = getRevision(fileName, 1);
        if (s)
            *s = PATH_CHAR; // restore
    }

    l2l_dbg(1, "TBRevision: %d\n", rev);
    return rev;
}


void
reportRevision(FILE *outFile)
{
    if (opt_Revision_check)
    {
        if (lastLine.valid)
            logRevCheck(outFile);
    }
}

unsigned long
findRev(FILE *finx, int *rev)
{
    unsigned long pos = 0L;

    while (!fseek(finx, (*rev) * sizeof(unsigned long), SEEK_SET))
    {
        fread(&pos, sizeof(long), 1, finx);
        (*rev)--;
        if (pos)
            break;
    }
    return pos;
}

int
regscan(FILE *outFile)
{
    int res = 0;
    char logname[MAX_PATH];
    char inxname[MAX_PATH];
    char line[LINESIZE + 1];
    char line2[LINESIZE + 1];
    FILE *flog = NULL;
    FILE *finx = NULL;
    unsigned long pos = 0L;
    int r;

    sprintf(logname, "%s" PATH_STR "%s", opt_SourcesPath, SVNDB);
    sprintf(inxname, "%s" PATH_STR "%s", opt_SourcesPath, SVNDB_INX);
    flog = fopen(logname, "rb");
    finx = fopen(inxname, "rb");

    if (flog && finx)
    {
        r = revinfo.buildrev;
        if (!fread(&pos, sizeof(long), 1, finx))
        {
            res = 2;
            l2l_dbg(0, "Cannot read magic number\n");
        }

        if (!res)
        {
            if (pos != MAGIC_INX)
            {
                res = 3;
                l2l_dbg(0, "Incorrect magic number (%lx)\n", pos);
            }
        }

        if (!res)
        {
            char flag[2];
            char path[MAX_PATH];
            char path2[MAX_PATH];
            int wflag = 0;
            clilog(outFile, "Regression candidates:\n");
            while (( pos = findRev(finx, &r) ))
            {
                if (r < (revinfo.buildrev - revinfo.range))
                {
                    l2l_dbg(1, "r%d is outside range of %d revisions\n", r, revinfo.range);
                    break;
                }
                fseek(flog, pos, SEEK_SET);
                wflag = 1;
                fgets(line, LINESIZE, flog);
                fgets(line2, LINESIZE, flog);
                while (fgets(line2, LINESIZE, flog))
                {
                    path2[0] = '\0';
                    if (sscanf(line2, "%1s %s %s", flag, path, path2) >= 2)
                    {
                        if (entry_lookup(&sources, path) || entry_lookup(&sources, path2))
                        {
                            if (wflag == 1)
                            {
                                clilog(outFile, "%sChanged paths:\n", line);
                                summ.regfound++;
                                wflag = 2;
                            }
                            clilog(outFile, "%s", line2);
                        }
                    }
                    else
                        break;
                }
                if (wflag == 2)
                {
                    int i = 0;
                    clilog(outFile, "\n");
                    while (fgets(line2, LINESIZE, flog))
                    {
                        i++;
                        clilog(outFile, "%s", line2);
                        if (strncmp(LOGBOTTOM, line2, sizeof(LOGBOTTOM) - 1) == 0)
                            break;
                    }
                }
            }
        }
    }
    else
    {
        res = 1;
        l2l_dbg(0, "Cannot open %s or %s\n", logname, inxname);
    }

    if (flog)
        fclose(flog);
    if (finx)
        fclose(finx);

    return res;
}


int
updateSvnlog(void)
{
    int res = 0;
    char logname[MAX_PATH];
    char inxname[MAX_PATH];
    char line[LINESIZE + 1];
    FILE *flog = NULL;
    FILE *finx = NULL;
    unsigned long pos;
    int r, y, m, d;
    char name[NAMESIZE];

    sprintf(logname, "%s" PATH_STR "%s", opt_SourcesPath, SVNDB);
    sprintf(inxname, "%s" PATH_STR "%s", opt_SourcesPath, SVNDB_INX);
    flog = fopen(logname, "rb");
    finx = fopen(inxname, "wb");

    if (flog && finx)
    {
        pos = MAGIC_INX;
        fwrite(&pos, sizeof(long), 1, finx);
        pos = ftell(flog);
        while (fgets(line, LINESIZE, flog))
        {
            if (sscanf(line, "r%d | %s | %d-%d-%d", &r, name, &y, &m, &d) == 5)
            {
                l2l_dbg(1, "%ld r%d | %s | %d-%d-%d\n", pos, r, name, y, m, d);
                fseek(finx, r * sizeof(unsigned long), SEEK_SET);
                fwrite(&pos, sizeof(unsigned long), 1, finx);
            }
            pos = ftell(flog);
        }
    }

    if (flog)
        fclose(flog);
    if (finx)
        fclose(finx);

    return res;
}

/* EOF */
