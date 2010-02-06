/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Initialization and main loop
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "version.h"
#include "compat.h"
#include "config.h"
#include "list.h"
#include "options.h"
#include "image.h"
#include "cache.h"
#include "log2lines.h"
#include "help.h"


static FILE *stdIn          = NULL;
static FILE *stdOut         = NULL;

LIST sources;
LINEINFO lastLine;
FILE *logFile        = NULL;
LIST cache;
SUMM summ;
REVINFO revinfo;


static void
clearLastLine(void)
{
    memset(&lastLine, 0, sizeof(LINEINFO));
}

static void
log_file(FILE *outFile, char *fileName, int line)
{
    int i = 0, min = 0, max = 0;
    char s[LINESIZE];
    FILE *src;

    strcpy(s, opt_SourcesPath);
    strcat(s, fileName);

    max = line + opt_SrcPlus;
    if ((src = fopen(s, "r")))
    {
        min = line - opt_Source;
        min = (min < 0) ? 0 : min;
        while (i < max && fgets(s, LINESIZE, src))
        {
            if (i >= min)
            {
                if (i == line)
                    log(outFile, "| ----\n");
                log(outFile, "| %4.4d  %s", i + 1, s);
            }
            i++;
        }
        fclose(src);
    }
    else
        l2l_dbg(1, "Can't open: %s (check " SOURCES_ENV ")\n", s);
}

static void
logSource(FILE *outFile)
{
    log_file(outFile, lastLine.file1, lastLine.nr1);
    if (lastLine.nr2)
    {
        log(outFile, "| ---- [%u] ----\n", lastLine.nr2);
        log_file(outFile, lastLine.file2, lastLine.nr2);
    }
}

static void
reportSource(FILE *outFile)
{
    if (!opt_Source)
        return;
    if (lastLine.valid)
        logSource(outFile);
}

static void
report(FILE *outFile)
{
    reportRevision(outFile);
    reportSource(outFile);
    clearLastLine();
}


static int
print_offset(void *data, size_t offset, char *toString)
{
    PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER)data;
    PROSSYM_ENTRY e = NULL;
    PROSSYM_ENTRY e2 = NULL;
    int bFileOffsetChanged = 0;
    char fmt[LINESIZE];
    char *Strings = (char *)data + RosSymHeader->StringsOffset;

    fmt[0] = '\0';
    e = find_offset(data, offset);
    if (opt_twice)
    {
        e2 = find_offset(data, offset - 1);

        if (e == e2)
            e2 = NULL;
        else
            summ.diff++;

        if (opt_Twice && e2)
        {
            e = e2;
            e2 = NULL;
            /* replaced (transparantly), but updated stats */
        }
    }
    if (e || e2)
    {
        strcpy(lastLine.file1, &Strings[e->FileOffset]);
        strcpy(lastLine.func1, &Strings[e->FunctionOffset]);
        lastLine.nr1 = e->SourceLine;
        sources_entry_create(&sources, lastLine.file1, SVN_PREFIX);
        lastLine.valid = 1;
        if (e2)
        {
            strcpy(lastLine.file2, &Strings[e2->FileOffset]);
            strcpy(lastLine.func2, &Strings[e2->FunctionOffset]);
            lastLine.nr2 = e2->SourceLine;
            sources_entry_create(&sources, lastLine.file2, SVN_PREFIX);
            bFileOffsetChanged = e->FileOffset != e2->FileOffset;
            if (e->FileOffset != e2->FileOffset || e->FunctionOffset != e2->FunctionOffset)
                summ.majordiff++;

            /*
             * - "%.0s" displays nothing, but processes argument
             * - bFileOffsetChanged implies always display 2nd SourceLine even if the same
             * - also for FunctionOffset
             */
            strcat(fmt, "%s");
            if (bFileOffsetChanged)
                strcat(fmt, "[%s]");
            else
                strcat(fmt, "%.0s");

            strcat(fmt, ":%u");
            if (e->SourceLine != e2->SourceLine || bFileOffsetChanged)
                strcat(fmt, "[%u]");
            else
                strcat(fmt, "%.0u");

            strcat(fmt, " (%s");
            if (e->FunctionOffset != e2->FunctionOffset || bFileOffsetChanged)
                strcat(fmt, "[%s])");
            else
                strcat(fmt, "%.0s)");

            if (toString)
            {   // put in toString if provided
                snprintf(toString, LINESIZE, fmt,
                    &Strings[e->FileOffset],
                    &Strings[e2->FileOffset],
                    (unsigned int)e->SourceLine,
                    (unsigned int)e2->SourceLine,
                    &Strings[e->FunctionOffset],
                    &Strings[e2->FunctionOffset]);
            }
            else
            {
                strcat(fmt, "\n");
                printf(fmt,
                    &Strings[e->FileOffset],
                    &Strings[e2->FileOffset],
                    (unsigned int)e->SourceLine,
                    (unsigned int)e2->SourceLine,
                    &Strings[e->FunctionOffset],
                    &Strings[e2->FunctionOffset]);
            }
        }
        else
        {
            if (toString)
            {   // put in toString if provided
                snprintf(toString, LINESIZE, "%s:%u (%s)",
                    &Strings[e->FileOffset],
                    (unsigned int)e->SourceLine,
                    &Strings[e->FunctionOffset]);
            }
            else
            {
                printf("%s:%u (%s)\n",
                    &Strings[e->FileOffset],
                    (unsigned int)e->SourceLine,
                    &Strings[e->FunctionOffset]);
            }
        }
        return 0;
    }
    return 1;
}

static int
process_data(const void *FileData, size_t offset, char *toString)
{
    int res;

    PIMAGE_SECTION_HEADER PERosSymSectionHeader = get_sectionheader((char *)FileData);
    if (!PERosSymSectionHeader)
        return 2;

    res = print_offset((char *)FileData + PERosSymSectionHeader->PointerToRawData, offset, toString);
    if (res)
    {
        if (toString)
            sprintf(toString, "??:0");
        else
            printf("??:0");
        l2l_dbg(1, "Offset not found: %x\n", (unsigned int)offset);
        summ.offset_errors++;
    }

    return res;
}

static int
process_file(const char *file_name, size_t offset, char *toString)
{
    void *FileData;
    size_t FileSize;
    int res = 1;

    FileData = load_file(file_name, &FileSize);
    if (!FileData)
    {
        l2l_dbg(0, "An error occured loading '%s'\n", file_name);
    }
    else
    {
        res = process_data(FileData, offset, toString);
        free(FileData);
    }
    return res;
}

static int
translate_file(const char *cpath, size_t offset, char *toString)
{
    size_t base = 0;
    LIST_MEMBER *pentry = NULL;
    int res = 0;
    char *path, *dpath;

    dpath = path = convert_path(cpath);
    if (!path)
        return 1;

    // The path could be absolute:
    if (get_ImageBase(path, &base))
    {
        pentry = entry_lookup(&cache, path);
        if (pentry)
        {
            path = pentry->path;
            base = pentry->ImageBase;
            if (base == INVALID_BASE)
            {
                l2l_dbg(1, "No, or invalid base address: %s\n", path);
                res = 2;
            }
        }
        else
        {
            l2l_dbg(1, "Not found in cache: %s\n", path);
            res = 3;
        }
    }

    if (!res)
    {
        res = process_file(path, offset, toString);
    }

    free(dpath);
    return res;
}

static void
translate_char(int c, FILE *outFile)
{
    fputc(c, outFile);
    if (logFile)
        fputc(c, logFile);
}

static char *
remove_mark(char *Line)
{
    if (Line[1] == ' ' && Line[2] == '<')
        if (Line[0] == '*' || Line[0] == '?')
            return Line + 2;
    return Line;
}

static void
translate_line(FILE *outFile, char *Line, char *path, char *LineOut)
{
    size_t offset;
    int cnt, res;
    char *sep, *tail, *mark, *s;
    unsigned char ch;

    if (!*Line)
        return;

    res = 1;
    mark = "";
    s = remove_mark(Line);
    if (opt_undo)
    {
        /* Strip all lines added by this tool: */
        char buf[NAMESIZE];
        if (sscanf(s, "| %s", buf) == 1)
            if (buf[0] == '0' || strcmp(buf, "----") == 0 || strcmp(buf, "R---") == 0 || atoi(buf))
                res = 0;
    }

    sep = strchr(s, ':');
    if (sep)
    {
        *sep = ' ';
        cnt = sscanf(s, "<%s %x%c", path, (unsigned int *)(&offset), &ch);
        if (opt_undo)
        {
            if (cnt == 3 && ch == ' ')
            {
                tail = strchr(s, '>');
                tail = tail ? tail - 1 : tail;
                if (tail && tail[0] == ')' && tail[1] == '>')
                {
                    res = 0;
                    tail += 2;
                    mark = opt_mark ? "* " : "";
                    if (opt_redo && !(res = translate_file(path, offset, LineOut)))
                    {
                        log(outFile, "%s<%s:%x (%s)>%s", mark, path, (unsigned int)offset, LineOut, tail);
                        summ.redo++;
                    }
                    else
                    {
                        log(outFile, "%s<%s:%x>%s", mark, path, (unsigned int)offset, tail);
                        summ.undo++;
                    }
                }
                else
                {
                    mark = opt_Mark ? "? " : "";
                    summ.skipped++;
                }
                summ.total++;
            }
        }

        if (!opt_undo || opt_redo)
        {
            if (cnt == 3 && ch == '>')
            {
                tail = strchr(s, '>') + 1;
                if (!(res = translate_file(path, offset, LineOut)))
                {
                    mark = opt_mark ? "* " : "";
                    log(outFile, "%s<%s:%x (%s)>%s", mark, path, (unsigned int)offset, LineOut, tail);
                    summ.translated++;
                }
                else
                {
                    mark = opt_Mark ? "? " : "";
                    summ.skipped++;
                }
                summ.total++;
            }
        }
    }
    if (res)
    {
        if (sep)
            *sep = ':';  // restore because not translated
        log(outFile, "%s%s", mark, s);
    }
    memset(Line, '\0', LINESIZE);  // flushed
}

static int
translate_files(FILE *inFile, FILE *outFile)
{
    char *Line = malloc(LINESIZE + 1);
    char *path = malloc(LINESIZE + 1);
    char *LineOut = malloc(LINESIZE + 1);
    int c;
    unsigned char ch;
    int i = 0;

    if (Line && path && LineOut)
    {
        memset(Line, '\0', LINESIZE + 1);
        if (opt_console)
        {
            while ((c = fgetc(inFile)) != EOF)
            {
                ch = (unsigned char)c;
                if (!opt_raw)
                {
                    switch (ch)
                    {
                    case '\n':
                        translate_line(outFile, Line, path, LineOut);
                        i = 0;
                        translate_char(c, outFile);
                        report(outFile);
                        break;
                    case '<':
                        i = 0;
                        Line[i++] = ch;
                        break;
                    case '>':
                        if (i)
                        {
                            if (i < LINESIZE)
                            {
                                Line[i++] = ch;
                                translate_line(outFile, Line, path, LineOut);
                            }
                            else
                            {
                                translate_line(outFile, Line, path, LineOut);
                                translate_char(c, outFile);
                            }
                        }
                        else
                            translate_char(c, outFile);
                        i = 0;
                        break;
                    default:
                        if (i)
                        {
                            if (i < LINESIZE)
                            {
                                Line[i++] = ch;
                            }
                            else
                            {
                                translate_line(outFile, Line, path, LineOut);
                                translate_char(c, outFile);
                                i = 0;
                            }
                        }
                        else
                            translate_char(c, outFile);
                    }
                }
                else
                    translate_char(c, outFile);
            }
        }
        else
        {   // Line by line, slightly faster but less interactive
            while (fgets(Line, LINESIZE, inFile) != NULL)
            {
                if (!opt_raw)
                {
                    translate_line(outFile, Line, path, LineOut);
                    report(outFile);
                }
                else
                    log(outFile, "%s", Line);
            }
        }
    }

    if (opt_Revision && (strstr(opt_Revision, "regscan") == opt_Revision))
    {
        char *s = strchr(opt_Revision, ',');
        if (s)
        {
            *s++ = '\0';
            revinfo.range = atoi(s);
        }
        regscan(outFile);
    }

    if (opt_stats)
    {
        stat_print(outFile, &summ);
        if (logFile)
            stat_print(logFile, &summ);
    }
    free(LineOut);
    free(Line);
    free(path);
    return 0;
}


int
main(int argc, const char **argv)
{
    int res = 0;
    int optCount = 0;

    stdIn = stdin;
    stdOut = stdout;

    memset(&cache, 0, sizeof(LIST));
    memset(&sources, 0, sizeof(LIST));
    stat_clear(&summ);
    memset(&revinfo, 0, sizeof(REVINFO));
    clearLastLine();

    optionInit(argc, argv);
    optCount = optionParse(argc, argv);
    if (optCount < 0)
    {
        return optCount;
    }

    argc -= optCount;

    if (opt_Revision && (strcmp(opt_Revision, "update") == 0))
    {
        res = updateSvnlog();
        return res;
    }

    if (check_directory(opt_force))
        return 3;

    create_cache(opt_force, 0);
    if (opt_exit)
        return 0;

    read_cache();
    l2l_dbg(4, "Cache read complete\n");

    if (*opt_logFile)
    {
        logFile = fopen(opt_logFile, "a");
        if (logFile)
        {
            // disable buffering so fflush is not needed
            if (!opt_buffered)
            {
                l2l_dbg(1, "Disabling log buffering on %s\n", opt_logFile);
                setbuf(logFile, NULL);
            }
            else
                l2l_dbg(1, "Enabling log buffering on %s\n", opt_logFile);
        }
        else
        {
            l2l_dbg(0, "Could not open logfile %s (%s)\n", opt_logFile, strerror(errno));
            return 2;
        }
    }
    l2l_dbg(4, "opt_logFile processed\n");

    if (opt_Pipe)
    {
        l2l_dbg(3, "Command line: \"%s\"\n",opt_Pipe);

        if (!(stdIn = POPEN(opt_Pipe, "r")))
        {
            stdIn = stdin; //restore
            l2l_dbg(0, "Could not popen '%s' (%s)\n", opt_Pipe, strerror(errno));
            free(opt_Pipe); opt_Pipe = NULL;
        }

        free(opt_Pipe); opt_Pipe = NULL;
    }
    l2l_dbg(4, "opt_Pipe processed\n");

    if (argc > 1)
    {   // translate {<exefile> <offset>}
        int i = 1;
        const char *exefile = NULL;
        const char *offset = NULL;
        char Line[LINESIZE + 1];

        while (i < argc)
        {
            Line[0] = '\0';
            offset = argv[optCount + i++];
            if (isOffset(offset))
            {
                if (exefile)
                {
                    l2l_dbg(2, "translating %s %s\n", exefile, offset);
                    translate_file(exefile, my_atoi(offset), Line);
                    printf("%s\n", Line);
                    report(stdOut);
                }
                else
                {
                    l2l_dbg(0, "<exefile> expected\n");
                    res = 3;
                    break;
                }
            }
            else
            {
                // Not an offset so must be an exefile:
                exefile = offset;
            }
        }
    }
    else
    {   // translate logging from stdin
        translate_files(stdIn, stdOut);
    }

    if (logFile)
        fclose(logFile);

    if (opt_Pipe)
        PCLOSE(stdIn);

    return res;
}
