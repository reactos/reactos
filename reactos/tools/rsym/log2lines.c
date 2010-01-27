/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "rsym.h"

#define LOG2LINES_VERSION   "1.11"

/* Assume if an offset > ABS_TRESHOLD, then it must be absolute */
#define ABS_TRESHOLD    0x00400000L
#define INVALID_BASE    0xFFFFFFFFL

#define LOGBOTTOM       "--------"
#define SVNDB           "svndb.log"
#define SVNDB_INX       "svndb.inx"
#define DEF_RANGE       500
#define MAGIC_INX       0x494E585F //'INX_'
#define DEF_OPT_DIR     "output-i386"
#define SOURCES_ENV     "_ROSBE_ROSSOURCEDIR"
#define CACHEFILE       "log2lines.cache"
#define TRKBUILDPREFIX  "bootcd-"
#define SVN_PREFIX      "/trunk/reactos/"
#define KDBG_PROMPT     "kdbg>"
#define PIPEREAD_CMD    "piperead -c"

#if defined (__DJGPP__) || defined (__WIN32__)

#include <direct.h>

#define POPEN           _popen
#define PCLOSE          _pclose
#define MKDIR(d)        _mkdir(d)
#define DEV_NULL        "NUL"
#define DOS_PATHS
#define PATH_CHAR       '\\'
#define PATH_STR        "\\"
#define PATHCMP         strcasecmp
#define CP_CMD          "copy /Y "
#define DIR_FMT         "dir /a:-d /s /b %s > %s"

#else /* not defined (__DJGPP__) || defined (__WIN32__) */

#include <limits.h>
#include <sys/stat.h>

#define MAX_PATH        PATH_MAX
#define POPEN           popen
#define PCLOSE          pclose
#define MKDIR(d)        mkdir(d, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)
#define DEV_NULL        "/dev/null"
#define UNIX_PATHS
#define PATH_CHAR       '/'
#define PATH_STR        "/"
#define PATHCMP         strcasecmp
#define CP_CMD          "cp -f "
#define DIR_FMT         "find %s -type f > %s"

#endif /* not defined (__DJGPP__) || defined (__WIN32__) */

#define CP_FMT          CP_CMD "%s %s > " DEV_NULL

#define CMD_7Z          "7z"
#define UNZIP_FMT_7Z    "%s e -y %s -o%s > " DEV_NULL
#define UNZIP_FMT       "%s x -y -r %s -o%s > " DEV_NULL
#define UNZIP_FMT_CAB \
"%s x -y -r %s" PATH_STR "reactos" PATH_STR "reactos.cab -o%s" \
PATH_STR "reactos" PATH_STR "reactos > " DEV_NULL

#define LINESIZE        1024
#define NAMESIZE        80

#define log(outFile, fmt, ...)                          \
    {                                                   \
        fprintf(outFile, fmt, ##__VA_ARGS__);           \
        if (logFile)                                    \
            fprintf(logFile, fmt, ##__VA_ARGS__);       \
    }

#define l2l_dbg(level, ...)                     \
    {                                           \
        if (opt_verbose >= level)               \
            fprintf(stderr, ##__VA_ARGS__);     \
    }

struct entry_struct
{
    char *buf;
    char *name;
    char *path;
    size_t ImageBase;
    struct entry_struct *pnext;
};

typedef struct entry_struct LIST_MEMBER;

struct list_struct
{
    off_t st_size;
    LIST_MEMBER *phead;
    LIST_MEMBER *ptail;
};

struct summ_struct
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
};

struct lineinfo_struct
{
    int     valid; 
    char    file1[LINESIZE];
    char    func1[NAMESIZE];
    int     nr1;
    char    file2[LINESIZE];
    char    func2[NAMESIZE];
    int     nr2;
};

struct revinfo_struct
{
    int     rev; 
    int     buildrev;
    int     range;
    int     opt_verbose;
};

typedef struct list_struct LIST;
typedef struct summ_struct SUMM;
typedef struct lineinfo_struct LINEINFO;
typedef struct revinfo_struct REVINFO;

static LIST cache;
static LIST sources;
static SUMM summ;
static LINEINFO lastLine;
static REVINFO revinfo;

static char *optchars       = "bcd:fFhl:mMP:rR:sS:tTuUvz:";
static int   opt_buffered   = 0;        // -b
static int   opt_help       = 0;        // -h
static int   opt_force      = 0;        // -f
static int   opt_exit       = 0;        // -e
static int   opt_verbose    = 0;        // -v
static int   opt_console    = 0;        // -c
static int   opt_mark       = 0;        // -m
static int   opt_Mark       = 0;        // -M
static char *opt_Pipe       = NULL;     // -P
static int   opt_raw        = 0;        // -r
static int   opt_stats      = 0;        // -s
static int   opt_Source     = 0;        // -S <opt_Source>[+<opt_SrcPlus>][,<sources_path>]
static int   opt_SrcPlus    = 0;        // -S <opt_Source>[+<opt_SrcPlus>][,<sources_path>]
static int   opt_twice      = 0;        // -t
static int   opt_Twice      = 0;        // -T
static int   opt_undo       = 0;        // -u
static int   opt_redo       = 0;        // -U
static char *opt_Revision   = NULL;     // -R
static char  opt_dir[MAX_PATH];         // -d <opt_dir>
static char  opt_logFile[MAX_PATH];     // -l <opt_logFile>
static char  opt_7z[MAX_PATH];          // -z <opt_7z>
static char  opt_scanned[LINESIZE];     // all scanned options
static FILE *logFile        = NULL;
static FILE *stdIn          = NULL;
static FILE *stdOut         = NULL;

static char *cache_name;
static char *tmp_name;

static char sources_path[LINESIZE];

static int
file_exists(char *name)
{
    FILE *f;

    f = fopen(name, "r");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

/* Do this in reverse (recursively)
   This saves many system calls if the path is likely
   to already exist (creating large trees).
*/
static int
mkPath(char *path, int isDir)
{
    char *s;
    int res = 0;

    if (isDir)
    {
        res = MKDIR(path);
        if (!res || (res == -1 && errno == EEXIST))
            return 0;
    }
    // create parent dir
    if ((s = strrchr(path, PATH_CHAR)))
    {
        *s = '\0';
        res = mkPath(path, 1);
        *s = PATH_CHAR;
    }

    if (!res && isDir)
        res = MKDIR(path);

    return res;
}

#if 0
static FILE *
rfopen(char *path, char *mode)
{
    FILE *f = NULL;
    char tmppath[MAX_PATH]; // Don't modify const strings

    strcpy(tmppath, path);
    f = fopen(tmppath, mode);
    if (!f && !mkPath(tmppath, 0))
        f = fopen(tmppath, mode);
    return f;
}
#endif

static LIST_MEMBER *
entry_lookup(LIST *list, char *name)
{
    LIST_MEMBER *pprev = NULL;
    LIST_MEMBER *pnext;

    if (!name || !name[0])
        return NULL;

    pnext = list->phead;
    while (pnext != NULL)
    {
        if (PATHCMP(name, pnext->name) == 0)
        {
            if (pprev)
            {   // move to head for faster lookup next time
                pprev->pnext = pnext->pnext;
                pnext->pnext = list->phead;
                list->phead = pnext;
            }
            return pnext;
        }
        pprev = pnext;
        pnext = pnext->pnext;
    }
    return NULL;
}

static LIST_MEMBER *
entry_delete(LIST_MEMBER *pentry)
{
    if (!pentry)
        return NULL;
    if (pentry->buf)
        free(pentry->buf);
    free(pentry);
    return NULL;
}

static LIST_MEMBER *
entry_insert(LIST *list, LIST_MEMBER *pentry)
{
    if (!pentry)
        return NULL;

    pentry->pnext = list->phead;
    list->phead = pentry;
    if (!list->ptail)
        list->ptail = pentry;
    return pentry;
}

#if 0
static LIST_MEMBER *
entry_remove(LIST *list, LIST_MEMBER *pentry)
{
    LIST_MEMBER *pprev = NULL, *p = NULL;

    if (!pentry)
        return NULL;

    if (pentry == list->phead)
    {
        list->phead = pentry->pnext;
        p = pentry;
    }
    else
    {
        pprev = list->phead;
        while (pprev->pnext)
        {
            if (pprev->pnext == pentry)
            {
                pprev->pnext = pentry->pnext;
                p = pentry;
                break;
            }
            pprev = pprev->pnext;
        }
    }
    if (pentry == list->ptail)
        list->ptail = pprev;

    return p;
}
#endif

static LIST_MEMBER *
cache_entry_create(char *Line)
{
    LIST_MEMBER *pentry;
    char *s = NULL;
    int l;

    if (!Line)
        return NULL;

    pentry = malloc(sizeof(LIST_MEMBER));
    if (!pentry)
        return NULL;

    l = strlen(Line);
    pentry->buf = s = malloc(l + 1);
    if (!s)
    {
        l2l_dbg(1, "Alloc entry failed\n");
        return entry_delete(pentry);
    }

    strcpy(s, Line);
    if (s[l] == '\n')
        s[l] = '\0';

    pentry->name = s;
    s = strchr(s, '|');
    if (!s)
    {
        l2l_dbg(1, "Name field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';

    pentry->path = s;
    s = strchr(s, '|');
    if (!s)
    {
        l2l_dbg(1, "Path field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';
    if (1 != sscanf(s, "%x", (unsigned int *)(&pentry->ImageBase)))
    {
        l2l_dbg(1, "ImageBase field missing\n");
        return entry_delete(pentry);
    }
    return pentry;
}


static LIST_MEMBER *
sources_entry_create(LIST *list, char *path, char *prefix)
{
    LIST_MEMBER *pentry;
    char *s = NULL;
    int l;

    if (!path)
        return NULL;
    if (!prefix)
        prefix = "";

    pentry = malloc(sizeof(LIST_MEMBER));
    if (!pentry)
        return NULL;

    l = strlen(path) + strlen(prefix);
    pentry->buf = s = malloc(l + 1);
    if (!s)
    {
        l2l_dbg(1, "Alloc entry failed\n");
        return entry_delete(pentry);
    }

    strcpy(s, prefix);
    strcat(s, path);
    if (s[l] == '\n')
        s[l] = '\0';

    pentry->name = s;
    if (list)
    {
        if (entry_lookup(list, pentry->name))
        {
            l2l_dbg(1, "Entry %s exists\n", pentry->name);
            pentry = entry_delete(pentry);
        }
        else
        {
            l2l_dbg(1, "Inserting entry %s\n", pentry->name);
            entry_insert(list, pentry);
        }
    }

    return pentry;
}

static void
clearLastLine(void)
{
    memset(&lastLine, 0, sizeof(LINEINFO));
}

static int
getRevision(char *fileName, int lastChanged)
{
    char s[LINESIZE];
    FILE *psvn;
    int rev = 0;

    if (!fileName)
        fileName = sources_path;
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

static int
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
            fileName = sources_path;
        }
        rev = getRevision(fileName, 1);
        if (s)
            *s = PATH_CHAR; // restore
    }

    l2l_dbg(1, "TBRevision: %d\n", rev);
    return rev;
}

static void
log_file(FILE *outFile, char *fileName, int line)
{
    int i = 0, min = 0, max = 0;
    char s[LINESIZE];
    FILE *src;

    strcpy(s, sources_path);
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
log_rev_check(FILE *outFile, char *fileName, int showfile)
{
    int rev = 0;
    char s[LINESIZE];

    strcpy(s, sources_path);
    strcat(s, fileName);
    rev = getRevision(s, 1);
    if (!showfile)
        s[0] = '\0';
    if (revinfo.opt_verbose)
        log(outFile, "| R--- %s Last Changed Rev: %d\n", s, rev);

    if (rev && opt_Revision)
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

static void
reportRevision(FILE *outFile)
{
    if (!opt_Revision)
        return;
    if (strcmp(opt_Revision, "check") == 0)
    {
        if (lastLine.valid)
            logRevCheck(outFile);
    }
}

static char *
basename(char *path)
{
    char *base;

    base = strrchr(path, PATH_CHAR);
    if (base)
        return ++base;
    return path;
}

static void
report(FILE *outFile)
{
    reportRevision(outFile);
    reportSource(outFile);
    clearLastLine();
}


static size_t
fixup_offset(size_t ImageBase, size_t offset)
{
    if (offset > ABS_TRESHOLD)
        offset -= ImageBase;
    return offset;
}

static PIMAGE_SECTION_HEADER
find_rossym_section(PIMAGE_FILE_HEADER PEFileHeader, PIMAGE_SECTION_HEADER PESectionHeaders)
{
    size_t i;
    for (i = 0; i < PEFileHeader->NumberOfSections; i++)
    {
        if (0 == strcmp((char *)PESectionHeaders[i].Name, ".rossym"))
            return &PESectionHeaders[i];
    }
    return NULL;
}

static PROSSYM_ENTRY
find_offset(void *data, size_t offset)
{
    PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER)data;
    PROSSYM_ENTRY Entries = (PROSSYM_ENTRY)((char *)data + RosSymHeader->SymbolsOffset);
    size_t symbols = RosSymHeader->SymbolsLength / sizeof(ROSSYM_ENTRY);
    size_t i;

    for (i = 0; i < symbols; i++)
    {
        if (Entries[i].Address > offset)
        {
            if (!i--)
                return NULL;
            else
                return &Entries[i];
        }
    }
    return NULL;
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
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_FILE_HEADER PEFileHeader;
    PIMAGE_OPTIONAL_HEADER PEOptHeader;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    PIMAGE_SECTION_HEADER PERosSymSectionHeader;
    size_t ImageBase;
    int res;

    /* Check if MZ header exists */
    PEDosHeader = (PIMAGE_DOS_HEADER)FileData;
    if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
    {
        l2l_dbg(0, "Input file is not a PE image.\n");
        summ.offset_errors++;
        return 1;
    }

    /* Locate PE file header */
    /* sizeof(ULONG) = sizeof(MAGIC) */
    PEFileHeader = (PIMAGE_FILE_HEADER)((char *)FileData + PEDosHeader->e_lfanew + sizeof(ULONG));

    /* Locate optional header */
    PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
    ImageBase = PEOptHeader->ImageBase;

    /* Locate PE section headers */
    PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *)PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

    /* make sure offset is what we want */
    offset = fixup_offset(ImageBase, offset);

    /* find rossym section */
    PERosSymSectionHeader = find_rossym_section(PEFileHeader, PESectionHeaders);
    if (!PERosSymSectionHeader)
    {
        l2l_dbg(0, "Couldn't find rossym section in executable\n");
        summ.offset_errors++;
        return 1;
    }
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

static const char *
getFmt(const char *a)
{
    const char *fmt = "%x";

    if (*a == '0')
    {
        switch (*++a)
        {
        case 'x':
            fmt = "%x";
            ++a;
            break;
        case 'd':
            fmt = "%d";
            ++a;
            break;
        default:
            fmt = "%o";
            break;
        }
    }
    return fmt;
}

static long
my_atoi(const char *a)
{
    int i = 0;
    sscanf(a, getFmt(a), &i);
    return i;
}

static int
isOffset(const char *a)
{
    int i = 0;
    if (strchr(a, '.'))
        return 0;
    return sscanf(a, getFmt(a), &i);
}

static int
copy_file(char *src, char *dst)
{
    char Line[LINESIZE];

    sprintf(Line, CP_FMT, src, dst);
    l2l_dbg(2, "Executing: %s\n", Line);
    remove(dst);
    if (file_exists(dst))
    {
        l2l_dbg(0, "Cannot remove dst %s before copy\n", dst);
        return 1;
    }
    if (system(Line) < 0)
    {
        l2l_dbg(0, "Cannot copy %s to %s\n", src, dst);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        return 2;
    }

    if (!file_exists(dst))
    {
        l2l_dbg(0, "Dst %s does not exist after copy\n", dst);
        return 2;
    }
    return 0;
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
get_ImageBase(char *fname, size_t *ImageBase)
{
    IMAGE_DOS_HEADER PEDosHeader;
    IMAGE_FILE_HEADER PEFileHeader;
    IMAGE_OPTIONAL_HEADER PEOptHeader;

    FILE *fr;
    off_t readLen;
    int res;

    *ImageBase = INVALID_BASE;
    fr = fopen(fname, "rb");
    if (!fr)
    {
        l2l_dbg(3, "get_ImageBase, cannot open '%s' (%s)\n", fname, strerror(errno));
        return 1;
    }

    readLen = fread(&PEDosHeader, sizeof(IMAGE_DOS_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_DOS_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 2;
    }

    /* Check if MZ header exists */
    if (PEDosHeader.e_magic != IMAGE_DOS_MAGIC || PEDosHeader.e_lfanew == 0L)
    {
        l2l_dbg(2, "get_ImageBase %s, MZ header missing\n", fname);
        fclose(fr);
        return 3;
    }

    /* Locate PE file header */
    res = fseek(fr, PEDosHeader.e_lfanew + sizeof(ULONG), SEEK_SET);
    readLen = fread(&PEFileHeader, sizeof(IMAGE_FILE_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_FILE_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 4;
    }

    /* Locate optional header */
    readLen = fread(&PEOptHeader, sizeof(IMAGE_OPTIONAL_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_OPTIONAL_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 5;
    }

    /* Check if it's really an IMAGE_OPTIONAL_HEADER we are interested in */
    if (PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        l2l_dbg(2, "get_ImageBase %s, not an IMAGE_NT_OPTIONAL_HDR 32/64 bit\n", fname);
        fclose(fr);
        return 6;
    }

    *ImageBase = PEOptHeader.ImageBase;
    fclose(fr);
    return 0;
}

static int
read_cache(void)
{
    FILE *fr;
    LIST_MEMBER *pentry;
    char *Line = NULL;
    int result = 0;

    Line = malloc(LINESIZE + 1);
    if (!Line)
    {
        l2l_dbg(1, "Alloc Line failed\n");
        return 1;
    }
    Line[LINESIZE] = '\0';

    fr = fopen(cache_name, "r");
    if (!fr)
    {
        l2l_dbg(1, "Open %s failed\n", cache_name);
        free(Line);
        return 2;
    }
    cache.phead = cache.ptail = NULL;

    while (fgets(Line, LINESIZE, fr) != NULL)
    {
        pentry = cache_entry_create(Line);
        if (!pentry)
        {
            l2l_dbg(2, "** Create entry failed of: %s\n", Line);
        }
        else
            entry_insert(&cache, pentry);
    }

    fclose(fr);
    free(Line);
    return result;
}

static int
create_cache(int force, int skipImageBase)
{
    FILE *fr, *fw;
    char *Line = NULL, *Fname = NULL;
    int len, err;
    size_t ImageBase;

    if ((fw = fopen(tmp_name, "w")) == NULL)
    {
        l2l_dbg(1, "Apparently %s is not writable (mounted ISO?), using current dir\n", tmp_name);
        cache_name = basename(cache_name);
        tmp_name = basename(tmp_name);
    }
    else
    {
        l2l_dbg(3, "%s is writable\n", tmp_name);
        fclose(fw);
        remove(tmp_name);
    }

    if (force)
    {
        l2l_dbg(3, "Removing %s ...\n", cache_name);
        remove(cache_name);
    }
    else
    {
        if (file_exists(cache_name))
        {
            l2l_dbg(3, "Cache %s already exists\n", cache_name);
            return 0;
        }
    }

    Line = malloc(LINESIZE + 1);
    if (!Line)
        return 1;
    Line[LINESIZE] = '\0';

    remove(tmp_name);
    l2l_dbg(0, "Scanning %s ...\n", opt_dir);
    snprintf(Line, LINESIZE, DIR_FMT, opt_dir, tmp_name);
    l2l_dbg(1, "Executing: %s\n", Line);
    if (system(Line) < 0)
    {
        l2l_dbg(0, "Cannot list directory %s\n", opt_dir);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        return 2;
    }
    l2l_dbg(0, "Creating cache ...");

    if ((fr = fopen(tmp_name, "r")) != NULL)
    {
        if ((fw = fopen(cache_name, "w")) != NULL)
        {
            while (fgets(Line, LINESIZE, fr) != NULL)
            {
                len = strlen(Line);
                if (!len)
                    continue;

                Fname = Line + len - 1;
                if (*Fname == '\n')
                    *Fname = '\0';

                while (Fname > Line && *Fname != PATH_CHAR)
                    Fname--;
                if (*Fname == PATH_CHAR)
                    Fname++;
                if (*Fname && !skipImageBase)
                {
                    if ((err = get_ImageBase(Line, &ImageBase)) == 0)
                        fprintf(fw, "%s|%s|%0x\n", Fname, Line, (unsigned int)ImageBase);
                    else
                        l2l_dbg(3, "%s|%s|%0x, ERR=%d\n", Fname, Line, (unsigned int)ImageBase, err);
                }
            }
            fclose(fw);
        }
        l2l_dbg(0, "... done\n");
        fclose(fr);
    }
    remove(tmp_name);
    free(Line);
    return 0;
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

static unsigned long
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

static int
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

    sprintf(logname, "%s" PATH_STR "%s", sources_path, SVNDB);
    sprintf(inxname, "%s" PATH_STR "%s", sources_path, SVNDB_INX);
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
            log(outFile, "\nRegression candidates:\n");
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
                                log(outFile, "%sChanged paths:\n", line);
                                summ.regfound++;
                                wflag = 2;
                            }
                            log(outFile, "%s", line2);
                        }
                    }
                    else
                        break;
                }
                if (wflag == 2)
                {
                    int i = 0;
                    log(outFile, "\n");
                    while (fgets(line2, LINESIZE, flog))
                    {
                        i++;
                        log(outFile, "%s", line2);
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

static void
print_summary(FILE *outFile)
{
    if (outFile)
    {
        fprintf(outFile, "\n*** LOG2LINES SUMMARY ***\n");
        fprintf(outFile, "Translated:               %d\n", summ.translated);
        fprintf(outFile, "Reverted:                 %d\n", summ.undo);
        fprintf(outFile, "Retranslated:             %d\n", summ.redo);
        fprintf(outFile, "Skipped:                  %d\n", summ.skipped);
        fprintf(outFile, "Differ:                   %d\n", summ.diff);
        fprintf(outFile, "Differ (function/source): %d\n", summ.majordiff);
        fprintf(outFile, "Revision conflicts:       %d\n", summ.revconflicts);
        fprintf(outFile, "Regression candidates:    %d\n", summ.regfound);
        fprintf(outFile, "Offset error:             %d\n", summ.offset_errors);
        fprintf(outFile, "Total:                    %d\n", summ.total);
        fprintf(outFile, "-------------------------------\n");
        fprintf(outFile, "Log2lines version: " LOG2LINES_VERSION "\n");
        fprintf(outFile, "Directory:         %s\n", opt_dir);
        fprintf(outFile, "Passed options:    %s\n", opt_scanned);
        fprintf(outFile, "-------------------------------\n");
    }
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
        print_summary(outFile);
        if (logFile)
            print_summary(logFile);
    }
    free(LineOut);
    free(Line);
    free(path);
    return 0;
}

static char *verboseUsage =
"\n"
"Description:\n"
"  When <exefile> <offset> are given, log2lines works like raddr2line:\n"
"      - The <exefile> <offset> combination can be repeated\n"
"      - Also, <offset> can be repeated for each <exefile>\n"
"      - NOTE: Some of the options below will have no effect in this form.\n"
"  Otherwise it reads stdin and tries to translate lines of the form:\n"
"      <IMAGENAME:ADDRESS>\n"
"  The result is written to stdout.\n\n"
"  <offset> or <ADDRESS> can be absolute or relative with the restrictions:\n"
"  - An image with base < 0x400000 MUST be relocated to a > 0x400000 address.\n"
"  - The offset of a relocated image MUST be relative.\n\n"
"  log2lines uses a cache in order to avoid a directory scan at each\n"
"  image lookup, greatly increasing performance. Only image path and its\n"
"  base address are cached.\n\n"
"Options:\n"
"  -b   Use this combined with '-l'. Enable buffering on logFile.\n"
"       This may solve loosing output on real hardware (ymmv).\n\n"
"  -c   Console mode. Outputs text per character instead of per line.\n"
"       This is slightly slower but enables to see what you type.\n\n"
"  -d <directory>|<ISO image>\n"
"       <directory>: Directory to scan for images. (Do not append a '" PATH_STR "')\n"
"       <ISO image>: This option also takes an ISO image as argument:\n"
"       - The image is recognized by the '.iso' or '.7z' extension.\n"
"       - NOTE: The '.7z' and extracted '.iso' basenames must be identical,\n"
"         which is normally true for ReactOS trunk builds.\n"
"       - The image will be unpacked to a directory with the same name.\n"
"       - The embedded reactos.cab file will also be unpacked.\n"
"       - Combined with -f the file will be re-unpacked.\n"
"       - NOTE: this ISO unpack feature needs 7z to be in the PATH.\n"
"       Default: " DEF_OPT_DIR "\n\n"
"  -f   Force creating new cache.\n\n"
"  -F   As -f but exits immediately after creating cache.\n\n"
"  -h   This text.\n\n"
"  -l <logFile>\n"
"       <logFile>: Append copy to specified logFile.\n"
"       Default: no logFile\n\n"
"  -m   Prefix (mark) each translated line with '* '.\n\n"
"  -M   Prefix (mark) each NOT translated line with '? '.\n"
"       ( Only for lines of the form: <IMAGENAME:ADDRESS> )\n\n"
"  -P <cmd line>\n"
"       Pipeline command line. Spawn <cmd line> and pipeline its output to\n"
"       log2lines (as stdin). This is for shells lacking support of (one of):\n"
"       - Input file redirection.\n"
"       - Pipelining byte streams, needed for the -c option.\n\n"
"  -r   Raw output without translation.\n\n"
"  -R <cmd>\n"
"       Revision commands interfacing with SVN. <cmd> is one of:\n"
"       - check:\n"
"         To be combined with -S. Check each source file in the log and issue\n"
"         a warning if its revision is higher than that of the tested build.\n"
"         Also when the revison of the source tree is lower than that of the\n"
"         tested build (for every source file).\n"
"         In both cases the source file's -S output would be unreliable.\n"
"       - update:\n"
"         Updates the SVN log file. Currently only generates the index file\n"
"         The SVN log file itself must be generated by hand in the sources\n"
"         directory like this (-v is mandatory here):\n"
"             svn log -v > svndb.log ('svn log' accepts also a range)\n"
"         'svndb.log' and its index are needed for '-R regscan'\n"
"       - regscan[,<range>]:\n"
"         Scan for regression candidates. Essentially it tries to find\n"
"         matches between the SVN log entries and the sources hit by\n"
"         the backtrace.\n"
"         <range> is the amount of revisions to look back from the build\n"
"         revision (default 500)\n"
"         The output of '-R regscan' is printed after EOF. The 'Changed path'\n"
"         lists will contain only matched files.\n"
"         Limitations:\n"
"         - The bug should really be a regression.\n"
"         - Expect a number of false positives.\n"
"         - The offending change must be in the sources hit by the backtrace.\n"
"           This mostly excludes changes in headerfiles for example.\n"
"         - Must be combined with -S.\n"
"       Can be combined with -tTS.\n\n"
"  -s   Statistics. A summary with the following info is printed after EOF:\n"
"       *** LOG2LINES SUMMARY ***\n"
"       - Translated:      Translated lines.\n"
"       - Reverted:        Lines translated back. See -u option\n"
"       - Retranslated:    Lines retranslated. See -U option\n"
"       - Skipped:         Lines not translated.\n"
"       - Differ:          Lines where (addr-1) info differs. See -tT options\n"
"       - Differ(func/src):Lines where also function or source info differ.\n"
"       - Rev conflicts:   Source files conflicting with build. See '-R check'\n"
"       - Reg candidates:  Regression candidates. See '-R regscan'\n"
"       - Offset error:    Image exists, but error retrieving offset info.\n"
"       - Total:           Total number of lines attempted to translate.\n"
"       Also some version info is displayed.\n\n"
"  -S <context>[+<add>][,<sources>]\n"
"       Source line options:\n"
"       <context>: Source lines. Display up to <context> lines until linenumber.\n"
"       <add>    : Optional. Display additional <add> lines after linenumber.\n"
"       <sources>: Optional. Specify alternate source tree.\n"
"       The environment variable " SOURCES_ENV " should be correctly set\n"
"       or specify <sources>. Use double quotes if the path contains spaces.\n"
"       For a reliable result, these sources should be up to date with\n"
"       the tested revision (or try '-R check').\n"
"       Can be combined with -tTR.\n"
"       Implies -U (For retrieving source info).\n\n"
"  -t   Translate twice. The address itself and for (address-1).\n"
"       Show extra filename, func and linenumber between [..] if they differ\n"
"       So if only the linenumbers differ, then only show the extra\n"
"       linenumber.\n\n"
"  -T   As -t, but show only filename+func+linenumber for (address-1)\n\n"
"  -u   Undo translations.\n"
"       Lines are translated back (reverted) to the form <IMAGENAME:ADDRESS>\n"
"       Also removes all lines previously added by this tool (e.g. see -S)\n\n"
"  -U   Undo and reprocess.\n"
"       Reverted to the form <IMAGENAME:ADDRESS>, and then retranslated\n"
"       Implies -u.\n\n"
"  -v   Show detailed errors and tracing.\n"
"       Repeating this option adds more verbosity.\n"
"       Default: only (major) errors\n\n"
"  -z <path to 7z>\n"
"       <path to 7z>: Specify path to 7z. See also option -d.\n"
"       Default: '7z'\n"
"\n"
"Examples:\n"
"  Setup: A VMware machine with its serial port set to: '\\\\.\\pipe\\kdbg'.\n\n"
"  Just recreate cache after a svn update or a new module has been added:\n"
"       log2lines -F\n\n"
"  Use kdbg debugger via console (interactive):\n"
"       log2lines -c < \\\\.\\pipe\\kdbg\n\n"
"  Use kdbg debugger via console, and append copy to logFile:\n"
"       log2lines -c -l dbg.log < \\\\.\\pipe\\kdbg\n\n"
"  Same as above, but for PowerShell:\n"
"       log2lines -c -l dbg.log -P \"piperead -c \\\\.\\pipe\\kdbg\"\n\n"
"  Use kdbg debugger to send output to logfile:\n"
"       log2lines < \\\\.\\pipe\\kdbg > dbg.log\n\n"
"  Re-translate a debug log:\n"
"       log2lines -U -d bootcd-38701-dbg.iso < bugxxxx.log\n\n"
"  Re-translate a debug log. Specify a 7z file, which wil be decompressed.\n"
"  Also check for (address) - (address-1) differences:\n"
"       log2lines -U -t -d bootcd-38701-dbg.7z < bugxxxx.log\n"
"  Output:\n"
"       <ntdll.dll:60f1 (dll/ntdll/ldr/utils.c:337[331] (LdrPEStartup))>\n\n"
"  The following commands are equivalent:\n"
"       log2lines msi.dll 2e35d msi.dll 2235 msiexec.exe 30a8 msiexec.exe 2e89\n"
"       log2lines msi.dll 2e35d 2235 msiexec.exe 30a8 2e89\n\n"
"  Generate source lines from backtrace ('bt') output. Show 2 lines of context:\n"
"       log2lines -S 2 -d bootcd-38701-dbg.7z < bugxxxx.log\n"
"  Output:\n"
"       <msiexec.exe:2e89 (lib/3rdparty/mingw/crtexe.c:259 (__tmainCRTStartup))>\n"
"       | 0258  #else\n"
"       | 0259      mainret = main (\n"
"       <msiexec.exe:2fad (lib/3rdparty/mingw/crtexe.c:160 (WinMainCRTStartup))>\n"
"       | 0159    return __tmainCRTStartup ();\n"
"       | 0160  }\n\n"
"  Generate source lines. Show 2 lines of context plus 1 additional line and\n"
"  specify an alternate source tree:\n"
"       log2lines -S 2+1,\"c:\\ros trees\\r44000\" -d bootcd-44000-dbg < dbg.log\n"
"  Output:\n"
"       <msi.dll:2e35d (dll/win32/msi/msiquery.c:189 (MSI_IterateRecords))>\n"
"       | 0188      {\n"
"       | 0189          r = MSI_ViewFetch( view, &rec );\n"
"       | ----\n"
"       | 0190          if( r != ERROR_SUCCESS )\n\n"
"  Use '-R check' to show that action.c has been changed after the build:\n"
"       log2lines -s -d bootcd-43850-dbg.iso -R check -S 2  < dbg.log\n"
"  Output:\n"
"       <msi.dll:35821 (dll/win32/msi/registry.c:781 (MSIREG_OpenUserDataKey))>\n"
"       | 0780      if (create)\n"
"       | 0781          rc = RegCreateKeyW(HKEY_LOCAL_MACHINE, keypath, key);\n"
"       <msi.dll:5262 (dll/win32/msi/action.c:2665 (ACTION_ProcessComponents))>\n"
"       | R--- Conflict : source(44191) > build(43850)\n"
"       | 2664              else\n"
"       | 2665                  rc = MSIREG_OpenUserDataKey(comp->ComponentId,\n"
"\n";

static void
usage(int verbose)
{
    fprintf(stderr, "log2lines " LOG2LINES_VERSION "\n\n");
    fprintf(stderr, "Usage: log2lines -%s {<exefile> <offset> {<offset>}}\n", optchars);
    if (verbose)
        fprintf(stderr, "%s", verboseUsage);
    else
        fprintf(stderr, "Try log2lines -h\n");
}

static int
unpack_iso(char *dir, char *iso)
{
    char Line[LINESIZE];
    int res = 0;
    char iso_tmp[MAX_PATH];
    int iso_copied = 0;
    FILE *fiso;

    strcpy(iso_tmp, iso);
    if ((fiso = fopen(iso, "a")) == NULL)
    {
        l2l_dbg(1, "Open of %s failed (locked for writing?), trying to copy first\n", iso);

        strcat(iso_tmp, "~");
        if (copy_file(iso, iso_tmp))
            return 3;
        iso_copied = 1;
    }
    else
        fclose(fiso);

    sprintf(Line, UNZIP_FMT, opt_7z, iso_tmp, dir);
    if (system(Line) < 0)
    {
        l2l_dbg(0, "\nCannot unpack %s (check 7z path!)\n", iso_tmp);
        l2l_dbg(1, "Failed to execute: '%s'\n", Line);
        res = 1;
    }
    else
    {
        l2l_dbg(2, "\nUnpacking reactos.cab in %s\n", dir);
        sprintf(Line, UNZIP_FMT_CAB, opt_7z, dir, dir);
        if (system(Line) < 0)
        {
            l2l_dbg(0, "\nCannot unpack reactos.cab in %s\n", dir);
            l2l_dbg(1, "Failed to execute: '%s'\n", Line);
            res = 2;
        }
    }
    if (iso_copied)
        remove(iso_tmp);
    return res;
}

static int
check_directory(int force)
{
    char Line[LINESIZE];
    char freeldr_path[MAX_PATH];
    char iso_path[MAX_PATH];
    char compressed_7z_path[MAX_PATH];
    char *check_iso;
    char *check_dir;

    if (opt_Revision)
    {
        revinfo.rev = getRevision(NULL, 1);
        revinfo.range = DEF_RANGE;
    }
    check_iso = strrchr(opt_dir, '.');
    l2l_dbg(1, "Checking directory: %s\n", opt_dir);
    if (check_iso && PATHCMP(check_iso, ".7z") == 0)
    {
        l2l_dbg(1, "Checking 7z image: %s\n", opt_dir);

        // First attempt to decompress to an .iso image
        strcpy(compressed_7z_path, opt_dir);
        if ((check_dir = strrchr(compressed_7z_path, PATH_CHAR)))
            *check_dir = '\0';
        else
            strcpy(compressed_7z_path, "."); // default to current dir

        sprintf(Line, UNZIP_FMT_7Z, opt_7z, opt_dir, compressed_7z_path);

        /* This of course only works if the .7z and .iso basenames are identical
         * which is normally true for ReactOS trunk builds:
         */
        strcpy(check_iso, ".iso");
        if (!file_exists(opt_dir) || force)
        {
            l2l_dbg(1, "Decompressing 7z image: %s\n", opt_dir);
            if (system(Line) < 0)
            {
                l2l_dbg(0, "\nCannot decompress to iso image %s\n", opt_dir);
                l2l_dbg(1, "Failed to execute: '%s'\n", Line);
                return 2;
            }
        }
        else
            l2l_dbg(2, "%s already decompressed\n", opt_dir);
    }

    if (check_iso && PATHCMP(check_iso, ".iso") == 0)
    {
        l2l_dbg(1, "Checking ISO image: %s\n", opt_dir);
        if (file_exists(opt_dir))
        {
            l2l_dbg(2, "ISO image exists: %s\n", opt_dir);
            strcpy(iso_path, opt_dir);
            *check_iso = '\0';
            sprintf(freeldr_path, "%s" PATH_STR "freeldr.ini", opt_dir);
            if (!file_exists(freeldr_path) || force)
            {
                l2l_dbg(0, "Unpacking %s to: %s ...", iso_path, opt_dir);
                unpack_iso(opt_dir, iso_path);
                l2l_dbg(0, "... done\n");
            }
            else
                l2l_dbg(2, "%s already unpacked in: %s\n", iso_path, opt_dir);
        }
        else
        {
            l2l_dbg(0, "ISO image not found: %s\n", opt_dir);
            return 1;
        }
    }
    if (opt_Revision)
    {
        revinfo.buildrev = getTBRevision(opt_dir);
        l2l_dbg(1, "Trunk build revision: %d\n", revinfo.buildrev);
    }
    cache_name = malloc(MAX_PATH);
    tmp_name = malloc(MAX_PATH);
    strcpy(cache_name, opt_dir);
    strcat(cache_name, PATH_STR CACHEFILE);
    strcpy(tmp_name, cache_name);
    strcat(tmp_name, "~");
    return 0;
}

static int
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

    sprintf(logname, "%s" PATH_STR "%s", sources_path, SVNDB);
    sprintf(inxname, "%s" PATH_STR "%s", sources_path, SVNDB_INX);
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

int
main(int argc, const char **argv)
{
    int res = 0;
    int opt;
    int optCount = 0;
    int i;
    char *s;

    stdIn = stdin;
    stdOut = stdout;
    strcpy(opt_dir, "");
    strcpy(sources_path, "");
    if ((s = getenv(SOURCES_ENV)))
        strcpy(sources_path, s);

    strcpy(opt_scanned, "");
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i],"-P")==0)
        {
            //Because its argument can contain spaces we cant use getopt(), a known bug:
            if (i+1 < argc)
            {
                free(opt_Pipe);
                opt_Pipe = malloc(LINESIZE);
                strcpy(opt_Pipe, argv[i+1]);
            }
        }
        strcat(opt_scanned, argv[i]);
        strcat(opt_scanned, " ");
    }
    l2l_dbg(4,"opt_scanned=[%s]\n",opt_scanned);
    strcpy(opt_logFile, "");
    strcpy(opt_7z, CMD_7Z);

    memset(&cache, 0, sizeof(LIST));
    memset(&sources, 0, sizeof(LIST));
    memset(&summ, 0, sizeof(SUMM));
    memset(&revinfo, 0, sizeof(REVINFO));
    clearLastLine();

    while (-1 != (opt = getopt(argc, (char **const)argv, optchars)))
    {
        switch (opt)
        {
        case 'b':
            opt_buffered++;
            break;
        case 'c':
            opt_console++;
            break;
        case 'd':
            optCount++;
            strcpy(opt_dir, optarg);
            break;
        case 'f':
            opt_force++;
            break;
        case 'h':
            opt_help++;
            usage(1);
            return 0;
            break;
        case 'F':
            opt_exit++;
            opt_force++;
            break;
        case 'l':
            optCount++;
            strcpy(opt_logFile, optarg);
            break;
        case 'm':
            opt_mark++;
            break;
        case 'M':
            opt_Mark++;
            break;
        case 'r':
            opt_raw++;
            break;
        case 'P':
            optCount++;
            //just count, see above
            break;
        case 'R':
            optCount++;
            free(opt_Revision);
            opt_Revision = malloc(LINESIZE);
            sscanf(optarg, "%s", opt_Revision);
            break;
        case 's':
            opt_stats++;
            break;
        case 'S':
            optCount++;
            i = sscanf(optarg, "%d+%d,%s", &opt_Source, &opt_SrcPlus, sources_path);
            if (i == 1)
                sscanf(optarg, "%*d,%s", sources_path);
            l2l_dbg(3, "Sources option parse result: %d+%d,\"%s\"\n", opt_Source, opt_SrcPlus, sources_path);
            break;
        case 't':
            opt_twice++;
            break;
        case 'T':
            opt_twice++;
            opt_Twice++;
            break;
        case 'u':
            opt_undo++;
            break;
        case 'U':
            opt_undo++;
            opt_redo++;
            break;
        case 'v':
            opt_verbose++;
            break;
        case 'z':
            optCount++;
            strcpy(opt_7z, optarg);
            break;
        default:
            usage(0);
            return 2;
            break;
        }
        optCount++;
    }
    if (sources_path[0])
    {
        strcat(sources_path, PATH_STR);
    }
    if (opt_Source)
    {
        /* need to retranslate for source info: */
        opt_undo++;
        opt_redo++;
    }
    if (!opt_dir[0])
    {
        strcpy(opt_dir, sources_path);
        strcat(opt_dir, DEF_OPT_DIR);
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
