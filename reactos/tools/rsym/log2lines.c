/*
 * Usage: log2lines [-cd:fFhmrv] [<exefile> <offset>]
 * Try log2lines -h
 *
 * This is a tool and is compiled using the host compiler,
 * i.e. on Linux gcc and not mingw-gcc (cross-compiler).
 * Therefore we can't include SDK headers and we have to
 * duplicate some definitions here.
 * Also note that the internal functions are "old C-style",
 * returning an int, where a return of 0 means success and
 * non-zero is failure.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "rsym.h"

#define LOG2LINES_VERSION   "0.8"

#define INVALID_BASE    0xFFFFFFFFL

#define DEF_OPT_DIR     "output-i386"

#if defined (__DJGPP__) || defined (__WIN32__)

#define DEV_NULL        "NUL"
#define DOS_PATHS
#define PATH_CHAR       '\\'
#define PATH_STR        "\\"
#define PATHCMP         strcasecmp
#define CP_CMD          "copy /Y "
#define DIR_FMT         "dir /a:-d /s /b %s > %s"

#else  /* not defined (__DJGPP__) || defined (__WIN32__) */

#include <errno.h>
#include <limits.h>

#define MAX_PATH        PATH_MAX
#define DEV_NULL        "/dev/null"
#define UNIX_PATHS
#define PATH_CHAR       '/'
#define PATH_STR        "/"
#define PATHCMP         strcasecmp
#define CP_CMD          "cp -f "
#define DIR_FMT         "find %s -type f > %s"

#endif  /* not defined (__DJGPP__) || defined (__WIN32__) */

#define CP_FMT          CP_CMD "%s %s > " DEV_NULL

#define CMD_7Z          "7z"
#define UNZIP_FMT       "%s x -y -r %s -o%s > " DEV_NULL
#define UNZIP_FMT_CAB \
"%s x -y -r %s" PATH_STR "reactos" PATH_STR "reactos.cab -o%s" PATH_STR "reactos" PATH_STR "reactos > " DEV_NULL

#define LINESIZE        1024

struct entry_struct
{
    char *buf;
    char *name;
    char *path;
    size_t ImageBase;
    struct entry_struct *pnext;
};

typedef struct entry_struct CACHE_ENTRY;

struct cache_struct
{
    off_t st_size;
    CACHE_ENTRY *phead;
    CACHE_ENTRY *ptail;
};

typedef struct cache_struct CACHE;

static CACHE cache;

static char *optchars  = "bcd:fFhl:mMrvz:";
static int opt_buffered= 0;         // -b
static int opt_help    = 0;         // -h
static int opt_force   = 0;         // -f
static int opt_exit    = 0;         // -e
static int opt_verbose = 0;         // -v
static int opt_console = 0;         // -c
static int opt_mark    = 0;         // -m
static int opt_Mark    = 0;         // -M
static int opt_raw     = 0;         // -r
static char opt_dir[MAX_PATH];      // -d
static char opt_logFile[MAX_PATH];  // -l
static char opt_7z[MAX_PATH];       // -z
static FILE *logFile   = NULL;

static char *cache_name;
static char *tmp_name;

static char *
basename(char *path)
{
    char *base;

    base = strrchr(path, PATH_CHAR);
    if (base)
    {
        return ++base;
    }
    return path;
}

static size_t
fixup_offset(size_t ImageBase, size_t offset)
{
    if (offset >= ImageBase)
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

static int
find_and_print_offset(void *data, size_t offset, char *toString)
{
    PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER) data;
    PROSSYM_ENTRY Entries = (PROSSYM_ENTRY) ((char *)data + RosSymHeader->SymbolsOffset);
    char *Strings = (char *)data + RosSymHeader->StringsOffset;
    size_t symbols = RosSymHeader->SymbolsLength / sizeof (ROSSYM_ENTRY);
    size_t i;

    //if (RosSymHeader->SymbolsOffset)

    for (i = 0; i < symbols; i++)
    {
        if (Entries[i].Address > offset)
        {
            if (!i--)
                return 1;
            else
            {
                PROSSYM_ENTRY e = &Entries[i];
                if (toString)
                {  // put in toString if provided
                    snprintf(toString, LINESIZE, "%s:%u (%s)",
                             &Strings[e->FileOffset],
                             (unsigned int)e->SourceLine,
                             &Strings[e->FunctionOffset]);
                    return 0;
                }
                else
                {  // to stdout
                    printf("%s:%u (%s)\n", &Strings[e->FileOffset],
                           (unsigned int)e->SourceLine,
                           &Strings[e->FunctionOffset]);
                    return 0;
                }
            }
        }
    }
    return 1;
}

static int
process_data(const void *FileData, size_t FileSize, size_t offset, char *toString)
{
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_FILE_HEADER PEFileHeader;
    PIMAGE_OPTIONAL_HEADER PEOptHeader;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    PIMAGE_SECTION_HEADER PERosSymSectionHeader;
    size_t ImageBase;
    int res;

    /* Check if MZ header exists */
    PEDosHeader = (PIMAGE_DOS_HEADER) FileData;
    if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
    {
        perror("Input file is not a PE image.\n");
        return 1;
    }

    /* Locate PE file header */
    /* sizeof(ULONG) = sizeof(MAGIC) */
    PEFileHeader = (PIMAGE_FILE_HEADER) ((char *)FileData + PEDosHeader->e_lfanew + sizeof (ULONG));

    /* Locate optional header */
    PEOptHeader = (PIMAGE_OPTIONAL_HEADER) (PEFileHeader + 1);
    ImageBase = PEOptHeader->ImageBase;

    /* Locate PE section headers */
    PESectionHeaders = (PIMAGE_SECTION_HEADER) ((char *)PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

    /* make sure offset is what we want */
    offset = fixup_offset(ImageBase, offset);

    /* find rossym section */
    PERosSymSectionHeader = find_rossym_section(PEFileHeader, PESectionHeaders);
    if (!PERosSymSectionHeader)
    {
        fprintf(stderr, "Couldn't find rossym section in executable\n");
        return 1;
    }
    res = find_and_print_offset((char *)FileData + PERosSymSectionHeader->PointerToRawData, offset, toString);
    if (res)
    {
        if (toString)
        {
            sprintf(toString, "??:0\n");
        }
        else
        {
            printf("??:0\n");
        }
    }

    return res;
}

static long
my_atoi(const char *a)
{
    int i = 0;
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
    sscanf(a, fmt, &i);
    return i;
}

static int
file_exists(char *name)
{
    FILE *f;

    f = fopen(name, "r");
    if (!f)
    {
        return 0;
    }
    fclose(f);
    return 1;
}

static int
copy_file(char *src, char *dst)
{
    char Line[LINESIZE];

    sprintf(Line, CP_FMT, src, dst);
    if (opt_verbose > 1)
        fprintf(stderr, "Executing: %s\n", Line);
    remove(dst);
    if (file_exists(dst))
    {
        fprintf(stderr, "Cannot remove dst %s before copy\n", dst);
        return 1;
    }
    system(Line);
    if (!file_exists(dst))
    {
        fprintf(stderr, "Dst %s does not exist after copy \n", dst);
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
        fprintf(stderr, "An error occured loading '%s'\n", file_name);
    }
    else
    {
        res = process_data(FileData, FileSize, offset, toString);
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
        if (opt_verbose)
            fprintf(stderr, "get_ImageBase, cannot open '%s' (%s)\n", fname, strerror(errno));
        return 1;
    }

    readLen = fread(&PEDosHeader, sizeof (IMAGE_DOS_HEADER), 1, fr);
    if (1 != readLen)
    {
        if (opt_verbose)
            fprintf(stderr, "get_ImageBase %s, read error IMAGE_DOS_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 2;
    }

    /* Check if MZ header exists */
    if (PEDosHeader.e_magic != IMAGE_DOS_MAGIC || PEDosHeader.e_lfanew == 0L)
    {
        if (opt_verbose > 1)
            fprintf(stderr, "get_ImageBase %s, MZ header missing\n", fname);
        fclose(fr);
        return 3;
    }

    /* Locate PE file header */
    res = fseek(fr, PEDosHeader.e_lfanew + sizeof (ULONG), SEEK_SET);
    readLen = fread(&PEFileHeader, sizeof (IMAGE_FILE_HEADER), 1, fr);
    if (1 != readLen)
    {
        if (opt_verbose)
            fprintf(stderr, "get_ImageBase %s, read error IMAGE_FILE_HEADER (%s)\n", fname, strerror(errno));
        return 4;
    }

    /* Locate optional header */
    readLen = fread(&PEOptHeader, sizeof (IMAGE_OPTIONAL_HEADER), 1, fr);
    if (1 != readLen)
    {
        if (opt_verbose)
            fprintf(stderr, "get_ImageBase %s, read error IMAGE_OPTIONAL_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 5;
    }

    /* Check if it's really an IMAGE_OPTIONAL_HEADER we are interested in */
    if ((PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) &&
        (PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC))
    {
        if (opt_verbose > 1)
            fprintf(stderr, "get_ImageBase %s, not an IMAGE_NT_OPTIONAL_HDR<32|64>\n", fname);
        fclose(fr);
        return 6;
    }

    *ImageBase = PEOptHeader.ImageBase;
    fclose(fr);
    return 0;
}

static CACHE_ENTRY *
entry_delete(CACHE_ENTRY *pentry)
{
    if (!pentry)
        return NULL;
    if (pentry->buf)
        free(pentry->buf);
    free(pentry);
    return NULL;
}

static CACHE_ENTRY *
entry_insert(CACHE_ENTRY *pentry)
{
    if (!pentry)
        return NULL;
    pentry->pnext = cache.phead;
    cache.phead = pentry;
    if (!cache.ptail)
        cache.ptail = pentry;
    return pentry;
}

#if 0
static CACHE_ENTRY *
entry_append(CACHE_ENTRY *pentry)
{
    if (!pentry)
        return NULL;
    if (!cache.ptail)
        return entry_insert(pentry);
    cache.ptail->pnext = pentry;
    pentry->pnext = NULL;
    cache.ptail = pentry;
    return pentry;
}
#endif

static CACHE_ENTRY *
entry_create(char *Line)
{
    CACHE_ENTRY *pentry;
    char *s = NULL;
    int l;

    if (!Line)
        return NULL;

    pentry = malloc(sizeof (CACHE_ENTRY));
    if (!pentry)
        return NULL;

    l = strlen(Line);
    pentry->buf = s = malloc(l + 1);
    if (!s)
    {
        if (opt_verbose)
            fprintf(stderr, "Alloc entry failed\n");
        return entry_delete(pentry);
    }

    strcpy(s, Line);
    if (s[l] == '\n')
        s[l] = '\0';

    pentry->name = s;
    s = strchr(s, '|');
    if (!s)
    {
        if (opt_verbose)
            fprintf(stderr, "Name field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';

    pentry->path = s;
    s = strchr(s, '|');
    if (!s)
    {
        if (opt_verbose)
            fprintf(stderr, "Path field missing\n");
        return entry_delete(pentry);
    }
    *s++ = '\0';
    if (1 != sscanf(s, "%x", &pentry->ImageBase))
    {
        if (opt_verbose)
            fprintf(stderr, "ImageBase field missing\n");
        return entry_delete(pentry);
    }
    return pentry;
}

static CACHE_ENTRY *
entry_lookup(char *name)
{
    CACHE_ENTRY *pprev = NULL;
    CACHE_ENTRY *pnext;

    pnext = cache.phead;
    while (pnext != NULL)
    {
        if (PATHCMP(name, pnext->name) == 0)
        {
            if (pprev)
            {  // move to head for faster lookup next time
                pprev->pnext = pnext->pnext;
                pnext->pnext = cache.phead;
                cache.phead = pnext;
            }
            return pnext;
        }
        pprev = pnext;
        pnext = pnext->pnext;
    }
    return NULL;
}

static int
read_cache(void)
{
    FILE *fr;
    CACHE_ENTRY *pentry;
    char *Line = NULL;
    int result = 0;

    //fprintf(stderr, "Reading cache ...\n");
    Line = malloc(LINESIZE + 1);
    if (!Line)
    {
        if (opt_verbose)
            fprintf(stderr, "Alloc Line failed\n");
        return 1;
    }
    Line[LINESIZE] = '\0';

    fr = fopen(cache_name, "r");
    if (!fr)
    {
        if (opt_verbose)
            fprintf(stderr, "Open %s failed\n", cache_name);
        free(Line);
        return 2;
    }
    cache.phead = cache.ptail = NULL;

    while (fgets(Line, LINESIZE, fr) != NULL)
    {
        pentry = entry_create(Line);
        if (!pentry)
        {
            if (opt_verbose > 1)
                fprintf(stderr, "** FAILED: %s\n", Line);
        }
        else
        {
            entry_insert(pentry);
        }
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
        if (opt_verbose)
            fprintf(stderr, "Apparently %s is not writable (mounted ISO?), using current dir\n", tmp_name);
        cache_name = basename(cache_name);
        tmp_name = basename(tmp_name);
    }
    else
    {
        if (opt_verbose > 2)
            fprintf(stderr, "%s is writable\n", tmp_name);
        fclose(fw);
        remove(tmp_name);
    }

    if (force)
    {
        if (opt_verbose > 2)
            fprintf(stderr, "Removing %s ...\n", cache_name);
        remove(cache_name);
    }
    else
    {
        if (file_exists(cache_name))
        {
            if (opt_verbose > 2)
                fprintf(stderr, "Cache %s already exists\n", cache_name);
            return 0;
        }
    }

    Line = malloc(LINESIZE + 1);
    if (!Line)
        return 1;
    Line[LINESIZE] = '\0';

    remove(tmp_name);
    fprintf(stderr, "Scanning %s ...\n", opt_dir);
    snprintf(Line, LINESIZE, DIR_FMT, opt_dir, tmp_name);
    system(Line);
    fprintf(stderr, "Creating cache ...");

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

                while ((Fname > Line) && (*Fname != PATH_CHAR))
                    Fname--;
                if (*Fname == PATH_CHAR)
                    Fname++;
                if (*Fname && !skipImageBase)
                {
                    if ((err = get_ImageBase(Line, &ImageBase)) != 0)
                    {
                        if (opt_verbose > 2)
                            fprintf(stderr, "%s|%s|%0x, ERR=%d\n", Fname, Line, ImageBase, err);
                    }
                    else
                    {
                        fprintf(fw, "%s|%s|%0x\n", Fname, Line, ImageBase);
                    }
                }
            }
            fclose(fw);
        }
        fprintf(stderr, "... done\n");
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
    CACHE_ENTRY *pentry = NULL;
    int res = 0;
    char *path, *dpath;

    /* First get the ImageBase of the File. If its smaller than the given
     * Parameter, everything is ok, because it was already added onto the
     * adress and can be given directly to process_file. If not, add it and
     * give the result to process_file.
     */
    dpath = path = convert_path(cpath);
    if (!path)
    {
        return 1;
    }

    // The path could be absolute:
    if (get_ImageBase(path, &base))
    {
        pentry = entry_lookup(path);
        if (pentry)
        {
            path = pentry->path;
            base = pentry->ImageBase;
            if (base == INVALID_BASE)
            {
                if (opt_verbose)
                    fprintf(stderr, "No, or invalid base address: %s\n", path);
                res = 2;
            }
        }
        else
        {
            if (opt_verbose)
                fprintf(stderr, "Not found in cache: %s\n", path);
            res = 3;
        }
    }

    if (!res)
    {
        offset = (base < offset) ? offset : base + offset;
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

static void
translate_line(FILE *outFile, char *Line, char *path, char *LineOut)
{
    size_t offset;
    int cnt, res;
    char *sep, *tail, *mark;
    unsigned char ch;

    if (!*Line)
        return;
    res = 1;
    mark = "";
    sep = strchr(Line, ':');
    if (sep)
    {
        *sep = ' ';
        cnt = sscanf(Line, "<%s %x%c", path, &offset, &ch);
        if (cnt == 3 && ch == '>')
        {
            tail = strchr(Line, '>') + 1;
            if (!(res = translate_file(path, offset, LineOut)))
            {
                mark = opt_mark ? "* " : "";
                fprintf(outFile, "%s<%s:%x (%s)>%s", mark, path, offset, LineOut, tail);
                if (logFile)
                    fprintf(logFile, "%s<%s:%x (%s)>%s", mark, path, offset, LineOut, tail);
            }
            else
            {
                *sep = ':';  // restore because not translated
                mark = opt_Mark ? "? " : "";
            }
        }
    }
    if (res)
    {
        fprintf(outFile, "%s%s", mark, Line);  // just copy
        if (logFile)
            fprintf(logFile, "%s%s", mark, Line);  // just copy
    }
    memset(Line, '\0', LINESIZE);  // flushed
}

static int
translate_files(FILE * inFile, FILE * outFile)
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
                            i = 0;
                        }
                        else
                        {
                            translate_char(c, outFile);
                        }
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
                        {
                            translate_char(c, outFile);
                        }
                    }
                }
                else
                {
                    translate_char(c, outFile);
                }
            }
        }
        else
        {  // Line by line, slightly faster but less interactive
            while (fgets(Line, LINESIZE, inFile) != NULL)
            {
                if (!opt_raw)
                {
                    translate_line(outFile, Line, path, LineOut);
                }
                else
                {
                    fprintf(outFile, "%s", Line);  // just copy
                    if (logFile)
                        fprintf(logFile, "%s", Line);  // just copy
                }
            }
        }
    }
    free(LineOut);
    free(Line);
    free(path);
    return 0;
}

static char *verboseUsage =
"\n"
"Description:\n"
"  When <exefile> <offset> are given, log2lines works just like raddr2line\n"
"  Otherwise it reads stdin and tries to translate lines of the form:\n"
"  <IMAGENAME:ADDRESS>\n\n"
"  The result is written to stdout.\n"
"  log2lines uses a cache in order to avoid a directory scan at each\n"
"  image lookup, greatly increasing performance. Only image path and its\n"
"  base address are cached.\n\n"
"Options:\n"
"  -b   Use this combined with '-l'. Enable buffering on logFile.\n"
"       This may solve loosing output on real hardware.\n\n"
"  -c   Console mode. Outputs text per character instead of per line.\n"
"       This is slightly slower but enables to see what you type.\n\n"
"  -d <directory>|<ISO image>\n"
"       Directory to scan for images. (Do not append a '" PATH_STR "')\n"
"       This option also takes an ISO image as argument:\n"
"       - The image is recognized by the '.iso' extension.\n"
"       - The image will be unpacked to a directory with the same name.\n"
"       - The embedded reactos.cab file will also be unpacked.\n"
"       - Combined with -f the file will be re-unpacked.\n"
"       - NOTE: this ISO unpack feature needs 7z to be in the PATH.\n"
"       Default: " DEF_OPT_DIR "\n\n"
"  -f   Force creating new cache.\n\n"
"  -F   As -f but exits immediately after creating cache.\n\n"
"  -h   This text.\n\n"
"  -l <logFile>\n"
"       Append copy to specified logFile.\n"
"       Default: no logFile\n\n"
"  -m   Prefix (mark) each translated line with '* '.\n\n"
"  -M   Prefix (mark) each NOT translated line with '? '.\n"
"       ( Only for lines of the form: <IMAGENAME:ADDRESS> )\n\n"
"  -r   Raw output without translation.\n\n"
"  -v   Show detailed errors and tracing.\n"
"       Repeating this option adds more verbosity.\n"
"       Default: only (major) errors\n" "\n"
"  -z <path to 7z>\n"
"       Specify path to 7z.\n"
"       Default: '7z'\n"
"\n"
"Examples:\n"
"  Setup is a VMware machine with its serial port set to: '\\\\.\\pipe\\kdbg'.\n\n"
"  Just recreate cache after a svn update or a new module has been added:\n"
"       log2lines -F\n\n" "  Use kdbg debugger via console (interactive):\n"
"       log2lines -c < \\\\.\\pipe\\kdbg\n\n"
"  Use kdbg debugger via console, and append copy to logFile:\n"
"       log2lines -c -l dbg.log < \\\\.\\pipe\\kdbg\n\n"
"  Use kdbg debugger to send output to logfile:\n"
"       log2lines < \\\\.\\pipe\\kdbg > dbg.log\n\n"
"  Re-translate a debug log:\n"
"       log2lines -d bootcd-38701-dbg.iso < bugxxxx.log\n\n"
"\n";

static void
usage(int verbose)
{
    fprintf(stderr, "log2lines " LOG2LINES_VERSION "\n\n");
    fprintf(stderr, "Usage: log2lines [-%s] [<exefile> <offset>]\n", optchars);
    if (verbose)
    {
        fprintf(stderr, "%s", verboseUsage);
    }
    else
    {
        fprintf(stderr, "Try log2lines -h\n");
    }
}

static int
unpack_iso(char *dir, char *iso)
{
    char Line[LINESIZE];
    int res = 0;
    char iso_tmp[MAX_PATH];
    int  iso_copied = 0;
    FILE *fiso;

    strcpy(iso_tmp, iso);
    if ((fiso = fopen(iso, "a")) == NULL)
    {
        if (opt_verbose)
            fprintf(stderr, "Open of %s failed (locked for writing?), trying to copy first\n", iso);

        strcat(iso_tmp,"~");
        if (copy_file(iso,iso_tmp))
            return 3;
        iso_copied = 1;
    }
    else
    {
        fclose(fiso);
    }

    sprintf(Line, UNZIP_FMT, opt_7z, iso_tmp, dir);
    if (system(Line) < 0)
    {
        fprintf(stderr, "\nCannot unpack %s (check 7z path!)\n", iso_tmp);
        if (opt_verbose)
            fprintf(stderr, "Failed to execute: '%s'\n", Line);
        res = 1;
    }
    else
    {
        if (opt_verbose > 1)
            fprintf(stderr, "\nUnpacking reactos.cab in %s\n", dir);
        sprintf(Line, UNZIP_FMT_CAB, opt_7z, dir, dir);
        if (system(Line) < 0)
        {
            fprintf(stderr, "\nCannot unpack reactos.cab in %s\n", dir);
            if (opt_verbose)
                fprintf(stderr, "Failed to execute: '%s'\n", Line);
            res = 2;
        }
    }
    if (iso_copied)
    {
        remove(iso_tmp);
    }
    return res;
}

static int
check_directory(int force)
{
    char freeldr_path[MAX_PATH];
    char iso_path[MAX_PATH];

    char *check_iso = strrchr(opt_dir, '.');
    if (check_iso && PATHCMP(check_iso, ".iso") == 0)
    {
        if (opt_verbose)
            fprintf(stderr, "Using ISO image: %s\n", opt_dir);
        if (file_exists(opt_dir))
        {
            if (opt_verbose > 1)
                fprintf(stderr, "ISO image exists: %s\n", opt_dir);

            strcpy(iso_path, opt_dir);
            *check_iso = '\0';
            sprintf(freeldr_path, "%s" PATH_STR "freeldr.ini", opt_dir);
            if (!file_exists(freeldr_path) || force)
            {
                fprintf(stderr, "Unpacking %s to: %s ...", iso_path, opt_dir);
                unpack_iso(opt_dir, iso_path);
                fprintf(stderr, "... done\n");
            }
            else
            {
                if (opt_verbose > 1)
                    fprintf(stderr, "%s already unpacked in: %s\n", iso_path, opt_dir);
            }
        }
        else
        {
            fprintf(stderr, "ISO image not found: %s\n", opt_dir);
            return 1;
        }
    }
    cache_name = malloc(MAX_PATH);
    tmp_name = malloc(MAX_PATH);
    strcpy(cache_name, opt_dir);
    strcat(cache_name, PATH_STR "log2lines.cache");
    strcpy(tmp_name, cache_name);
    strcat(tmp_name, "~");
    return 0;
}

int
main(int argc, const char **argv)
{
    int res = 0;
    int opt;
    int optCount = 0;

    strcpy(opt_dir, DEF_OPT_DIR);
    strcpy(opt_logFile, "");
    strcpy(opt_7z, CMD_7Z);
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
            exit(0);
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
        case 'v':
            opt_verbose++;
            break;
        case 'z':
            optCount++;
            strcpy(opt_7z, optarg);
            break;
        default:
            usage(0);
            exit(2);
            break;
        }
        optCount++;
    }

    argc -= optCount;
    if (argc != 1 && argc != 3)
    {
        usage(0);
        exit(1);
    }

    if (check_directory(opt_force))
        exit(3);

    create_cache(opt_force, 0);
    if (opt_exit)
        exit(0);

    read_cache();

    if (*opt_logFile)
    {
        logFile = fopen(opt_logFile, "a");
        if (logFile)
        {
            // disable buffering so fflush is not needed
            if (!opt_buffered)
            {
                if (opt_verbose)
                    fprintf(stderr, "Disabling log buffering on %s\n", opt_logFile);
                setbuf(logFile,NULL);
            }
            else
            {
                if (opt_verbose)
                    fprintf(stderr, "Enabling log buffering on %s\n", opt_logFile);
            }
        }
        else
        {
            fprintf(stderr, "Could not open logfile %s (%s)\n", opt_logFile, strerror(errno));
            exit(2);

        }
    }
    if (argc == 3)
    {  // translate <exefile> <offset>
        translate_file(argv[optCount + 1], my_atoi(argv[optCount + 2]), NULL);
    }
    else
    {  // translate logging from stdin
        translate_files(stdin, stdout);
    }

    if (logFile)
        fclose(logFile);
    return res;
}
