/*
*******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  gencmn.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov01
*   created by: Markus W. Scherer
*
*   This program reads a list of data files and combines them
*   into one common, memory-mappable file.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unicode/uclean.h"
#include "unewdata.h"
#include "uoptions.h"
#include "putilimp.h"

#define STRING_STORE_SIZE 100000
#define MAX_FILE_COUNT 2000

#define COMMON_DATA_NAME U_ICUDATA_NAME
#define DATA_TYPE "dat"

/* ICU package data file format (.dat files) ------------------------------- ***

Description of the data format after the usual ICU data file header
(UDataInfo etc.).

Format version 1

A .dat package file contains a simple Table of Contents of item names,
followed by the items themselves:

1. ToC table

uint32_t count; - number of items
UDataOffsetTOCEntry entry[count]; - pair of uint32_t values per item:
    uint32_t nameOffset; - offset of the item name
    uint32_t dataOffset; - offset of the item data
both are byte offsets from the beginning of the data

2. item name strings

All item names are stored as char * strings in one block between the ToC table
and the data items.

3. data items

The data items are stored following the item names block.
Each data item is 16-aligned.
The data items are stored in the sorted order of their names.

Therefore, the top of the name strings block is the offset of the first item,
the length of the last item is the difference between its offset and
the .dat file length, and the length of all previous items is the difference
between its offset and the next one.

----------------------------------------------------------------------------- */

/* UDataInfo cf. udata.h */
static const UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x43, 0x6d, 0x6e, 0x44},     /* dataFormat="CmnD" */
    {1, 0, 0, 0},                 /* formatVersion */
    {3, 0, 0, 0}                  /* dataVersion */
};

static uint32_t maxSize;

static char stringStore[STRING_STORE_SIZE];
static uint32_t stringTop=0, basenameTotal=0;

typedef struct {
    char *pathname, *basename;
    uint32_t basenameLength, basenameOffset, fileSize, fileOffset;
} File;

static File files[MAX_FILE_COUNT];
static uint32_t fileCount=0;

/* prototypes --------------------------------------------------------------- */

static void
addFile(const char *filename, UBool sourceTOC, UBool verbose);

static char *
allocString(uint32_t length);

static int
compareFiles(const void *file1, const void *file2);

static char * 
pathToFullPath(const char *path);

/* map non-tree separator (such as '\') to tree separator ('/') inplace. */
static void
fixDirToTreePath(char *s);
/* -------------------------------------------------------------------------- */

static UOption options[]={
/*0*/ UOPTION_HELP_H,
/*1*/ UOPTION_HELP_QUESTION_MARK,
/*2*/ UOPTION_VERBOSE,
/*3*/ UOPTION_COPYRIGHT,
/*4*/ UOPTION_DESTDIR,
/*5*/ UOPTION_DEF( "comment", 'C', UOPT_REQUIRES_ARG),
/*6*/ UOPTION_DEF( "name", 'n', UOPT_REQUIRES_ARG),
/*7*/ UOPTION_DEF( "type", 't', UOPT_REQUIRES_ARG),
/*8*/ UOPTION_DEF( "source", 'S', UOPT_NO_ARG),
/*9*/ UOPTION_DEF( "entrypoint", 'e', UOPT_REQUIRES_ARG),
/*10*/UOPTION_SOURCEDIR,
};

static char *symPrefix = NULL;

extern int
main(int argc, char* argv[]) {
    static char buffer[4096];
    char line[512];
    FileStream *in, *file;
    char *s;
    UErrorCode errorCode=U_ZERO_ERROR;
    uint32_t i, fileOffset, basenameOffset, length, nread;
    UBool sourceTOC, verbose;
    const char *entrypointName = NULL;

    U_MAIN_INIT_ARGS(argc, argv);

    /* preset then read command line options */
    options[4].value=u_getDataDirectory();
    options[6].value=COMMON_DATA_NAME;
    options[7].value=DATA_TYPE;
    options[10].value=".";
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    } else if(argc<2) {
        argc=-1;
    }

    if(argc<0 || options[0].doesOccur || options[1].doesOccur) {
        FILE *where = argc < 0 ? stderr : stdout;
        
        /*
         * Broken into chucks because the C89 standard says the minimum
         * required supported string length is 509 bytes.
         */
        fprintf(where,
                "%csage: %s [ -h, -?, --help ] [ -v, --verbose ] [ -c, --copyright ] [ -C, --comment comment ] [ -d, --destdir dir ] [ -n, --name filename ] [ -t, --type filetype ] [ -S, --source tocfile ] [ -e, --entrypoint name ] maxsize listfile\n", argc < 0 ? 'u' : 'U', *argv);
        if (options[0].doesOccur || options[1].doesOccur) {
            fprintf(where, "\n"
                "Read the list file (default: standard input) and create a common data\n"
                "file from specified files. Omit any files larger than maxsize, if maxsize > 0.\n");
            fprintf(where, "\n"
            "Options:\n"
            "\t-h, -?, --help              this usage text\n"
            "\t-v, --verbose               verbose output\n"
            "\t-c, --copyright             include the ICU copyright notice\n"
            "\t-C, --comment comment       include a comment string\n"
            "\t-d, --destdir dir           destination directory\n");
            fprintf(where,
            "\t-n, --name filename         output filename, without .type extension\n"
            "\t                            (default: " COMMON_DATA_NAME ")\n"
            "\t-t, --type filetype         type of the destination file\n"
            "\t                            (default: \"" DATA_TYPE "\")\n"
            "\t-S, --source tocfile        write a .c source file with the table of\n"
            "\t                            contents\n"
            "\t-e, --entrypoint name       override the c entrypoint name\n"
            "\t                            (default: \"<name>_<type>\")\n");
        }
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    sourceTOC=options[8].doesOccur;

    verbose = options[2].doesOccur;

    maxSize=(uint32_t)uprv_strtoul(argv[1], NULL, 0);

    if(argc==2) {
        in=T_FileStream_stdin();
    } else {
        in=T_FileStream_open(argv[2], "r");
        if(in==NULL) {
            fprintf(stderr, "gencmn: unable to open input file %s\n", argv[2]);
            exit(U_FILE_ACCESS_ERROR);
        }
    }

    if (verbose) {
        if(sourceTOC) {
            printf("generating %s_%s.c (table of contents source file)\n", options[6].value, options[7].value);
        } else {
            printf("generating %s.%s (common data file with table of contents)\n", options[6].value, options[7].value);
        }
    }

    /* read the list of files and get their lengths */
    while(T_FileStream_readLine(in, line, sizeof(line))!=NULL) {
        /* remove trailing newline characters */
        s=line;
        while(*s!=0) {
            if(*s=='\r' || *s=='\n') {
                *s=0;
                break;
            }
            ++s;
        }

        /* check for comment */

        if (*line == '#') {
            continue;
        }

        /* add the file */
#if (U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR)
        {
          char *t;
          while((t = uprv_strchr(line,U_FILE_ALT_SEP_CHAR))) {
            *t = U_FILE_SEP_CHAR;
          }
        }
#endif
        addFile(getLongPathname(line), sourceTOC, verbose);
    }

    if(in!=T_FileStream_stdin()) {
        T_FileStream_close(in);
    }

    if(fileCount==0) {
        fprintf(stderr, "gencmn: no files listed in %s\n", argc==2 ? "<stdin>" : argv[2]);
        return 0;
    }

    /* sort the files by basename */
    qsort(files, fileCount, sizeof(File), compareFiles);

    if(!sourceTOC) {
        UNewDataMemory *out;

        /* determine the offsets of all basenames and files in this common one */
        basenameOffset=4+8*fileCount;
        fileOffset=(basenameOffset+(basenameTotal+15))&~0xf;
        for(i=0; i<fileCount; ++i) {
            files[i].fileOffset=fileOffset;
            fileOffset+=(files[i].fileSize+15)&~0xf;
            files[i].basenameOffset=basenameOffset;
            basenameOffset+=files[i].basenameLength;
        }

        /* create the output file */
        out=udata_create(options[4].value, options[7].value, options[6].value,
                         &dataInfo,
                         options[3].doesOccur ? U_COPYRIGHT_STRING : options[5].value,
                         &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gencmn: udata_create(-d %s -n %s -t %s) failed - %s\n",
                options[4].value, options[6].value, options[7].value,
                u_errorName(errorCode));
            exit(errorCode);
        }

        /* write the table of contents */
        udata_write32(out, fileCount);
        for(i=0; i<fileCount; ++i) {
            udata_write32(out, files[i].basenameOffset);
            udata_write32(out, files[i].fileOffset);
        }

        /* write the basenames */
        for(i=0; i<fileCount; ++i) {
            udata_writeString(out, files[i].basename, files[i].basenameLength);
        }
        length=4+8*fileCount+basenameTotal;

        /* copy the files */
        for(i=0; i<fileCount; ++i) {
            /* pad to 16-align the next file */
            length&=0xf;
            if(length!=0) {
                udata_writePadding(out, 16-length);
            }

            if (verbose) {
                printf("adding %s (%ld byte%s)\n", files[i].pathname, (long)files[i].fileSize, files[i].fileSize == 1 ? "" : "s");
            }

            /* copy the next file */
            file=T_FileStream_open(files[i].pathname, "rb");
            if(file==NULL) {
                fprintf(stderr, "gencmn: unable to open listed file %s\n", files[i].pathname);
                exit(U_FILE_ACCESS_ERROR);
            }
            for(nread = 0;;) {
                length=T_FileStream_read(file, buffer, sizeof(buffer));
                if(length <= 0) {
                    break;
                }
                nread += length;
                udata_writeBlock(out, buffer, length);
            }
            T_FileStream_close(file);
            length=files[i].fileSize;

            if (nread != files[i].fileSize) {
              fprintf(stderr, "gencmn: unable to read %s properly (got %ld/%ld byte%s)\n", files[i].pathname,  (long)nread, (long)files[i].fileSize, files[i].fileSize == 1 ? "" : "s");
                exit(U_FILE_ACCESS_ERROR);
            }
        }

        /* pad to 16-align the last file (cleaner, avoids growing .dat files in icuswap) */
        length&=0xf;
        if(length!=0) {
            udata_writePadding(out, 16-length);
        }

        /* finish */
        udata_finish(out, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "gencmn: udata_finish() failed - %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
    } else {
        /* write a .c source file with the table of contents */
        char *filename;
        FileStream *out;

        /* create the output filename */
        filename=s=buffer;
        uprv_strcpy(filename, options[4].value);
        s=filename+uprv_strlen(filename);
        if(s>filename && *(s-1)!=U_FILE_SEP_CHAR) {
            *s++=U_FILE_SEP_CHAR;
        }
        uprv_strcpy(s, options[6].value);
        if(*(options[7].value)!=0) {
            s+=uprv_strlen(s);
            *s++='_';
            uprv_strcpy(s, options[7].value);
        }
        s+=uprv_strlen(s);
        uprv_strcpy(s, ".c");

        /* open the output file */
        out=T_FileStream_open(filename, "w");
        if(out==NULL) {
            fprintf(stderr, "gencmn: unable to open .c output file %s\n", filename);
            exit(U_FILE_ACCESS_ERROR);
        }

        /* If an entrypoint is specified, use it. */
        if(options[9].doesOccur) {
            entrypointName = options[9].value;
        } else {
            entrypointName = options[6].value;
        }


        /* write the source file */
        sprintf(buffer,
            "/*\n"
            " * ICU common data table of contents for %s.%s ,\n"
            " * Automatically generated by icu/source/tools/gencmn/gencmn .\n"
            " */\n\n"
            "#include \"unicode/utypes.h\"\n"
            "#include \"unicode/udata.h\"\n"
            "\n"
            "/* external symbol declarations for data */\n",
            options[6].value, options[7].value);
        T_FileStream_writeLine(out, buffer);

        sprintf(buffer, "extern const char\n    %s%s[]", symPrefix?symPrefix:"", files[0].pathname);
        T_FileStream_writeLine(out, buffer);
        for(i=1; i<fileCount; ++i) {
            sprintf(buffer, ",\n    %s%s[]", symPrefix?symPrefix:"", files[i].pathname);
            T_FileStream_writeLine(out, buffer);
        }
        T_FileStream_writeLine(out, ";\n\n");

        sprintf(
            buffer,
            "U_EXPORT struct {\n"
            "    uint16_t headerSize;\n"
            "    uint8_t magic1, magic2;\n"
            "    UDataInfo info;\n"
            "    char padding[%lu];\n"
            "    uint32_t count, reserved;\n"
            "    struct {\n"
            "        const char *name;\n"
            "        const void *data;\n"
            "    } toc[%lu];\n"
            "} U_EXPORT2 %s_dat = {\n"
            "    32, 0xda, 0x27, {\n"
            "        %lu, 0,\n"
            "        %u, %u, %u, 0,\n"
            "        {0x54, 0x6f, 0x43, 0x50},\n"
            "        {1, 0, 0, 0},\n"
            "        {0, 0, 0, 0}\n"
            "    },\n"
            "    \"\", %lu, 0, {\n",
            (unsigned long)32-4-sizeof(UDataInfo),
            (unsigned long)fileCount,
            entrypointName,
            (unsigned long)sizeof(UDataInfo),
            U_IS_BIG_ENDIAN,
            U_CHARSET_FAMILY,
            U_SIZEOF_UCHAR,
            (unsigned long)fileCount
        );
        T_FileStream_writeLine(out, buffer);

        sprintf(buffer, "        { \"%s\", %s%s }", files[0].basename, symPrefix?symPrefix:"", files[0].pathname);
        T_FileStream_writeLine(out, buffer);
        for(i=1; i<fileCount; ++i) {
            sprintf(buffer, ",\n        { \"%s\", %s%s }", files[i].basename, symPrefix?symPrefix:"", files[i].pathname);
            T_FileStream_writeLine(out, buffer);
        }

        T_FileStream_writeLine(out, "\n    }\n};\n");
        T_FileStream_close(out);

        uprv_free(symPrefix);
    }

    return 0;
}

static void
addFile(const char *filename, UBool sourceTOC, UBool verbose) {
    char *s;
    uint32_t length;
    char *fullPath = NULL;

    if(fileCount==MAX_FILE_COUNT) {
        fprintf(stderr, "gencmn: too many files, maximum is %d\n", MAX_FILE_COUNT);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    if(!sourceTOC) {
        FileStream *file;

        if(uprv_pathIsAbsolute(filename)) {
            fprintf(stderr, "gencmn: Error: absolute path encountered. Old style paths are not supported. Use relative paths such as 'fur.res' or 'translit%cfur.res'.\n\tBad path: '%s'\n", U_FILE_SEP_CHAR, filename);
            exit(U_ILLEGAL_ARGUMENT_ERROR);
        }
        fullPath = pathToFullPath(filename);

        /* store the pathname */
        length = (uint32_t)(uprv_strlen(filename) + 1 + uprv_strlen(options[6].value) + 1);
        s=allocString(length);
        uprv_strcpy(s, options[6].value);
        uprv_strcat(s, U_TREE_ENTRY_SEP_STRING);
        uprv_strcat(s, filename);

        /* get the basename */
        fixDirToTreePath(s);
        files[fileCount].basename=s;
        files[fileCount].basenameLength=length;

        files[fileCount].pathname=fullPath;

        basenameTotal+=length;

        /* try to open the file */
        file=T_FileStream_open(fullPath, "rb");
        if(file==NULL) {
            fprintf(stderr, "gencmn: unable to open listed file %s\n", fullPath);
            exit(U_FILE_ACCESS_ERROR);
        }

        /* get the file length */
        length=T_FileStream_size(file);
        if(T_FileStream_error(file) || length<=20) {
            fprintf(stderr, "gencmn: unable to get length of listed file %s\n", fullPath);
            exit(U_FILE_ACCESS_ERROR);
        }
        
        T_FileStream_close(file);

        /* do not add files that are longer than maxSize */
        if(maxSize && length>maxSize) {
            if (verbose) {
                printf("%s ignored (size %ld > %ld)\n", fullPath, (long)length, (long)maxSize);
            }
            return;
        }
        files[fileCount].fileSize=length;
    } else {
        char *t;

        /* get and store the basename */
        /* need to include the package name */
        length = (uint32_t)(uprv_strlen(filename) + 1 + uprv_strlen(options[6].value) + 1);
        s=allocString(length);
        uprv_strcpy(s, options[6].value);
        uprv_strcat(s, U_TREE_ENTRY_SEP_STRING);
        uprv_strcat(s, filename);
        fixDirToTreePath(s);
        files[fileCount].basename=s;


        /* turn the basename into an entry point name and store in the pathname field */
        t=files[fileCount].pathname=allocString(length);
        while(--length>0) {
            if(*s=='.' || *s=='-' || *s=='/') {
                *t='_';
            } else {
                *t=*s;
            }
            ++s;
            ++t;
        }
        *t=0;
    }
    ++fileCount;
}

static char *
allocString(uint32_t length) {
    uint32_t top=stringTop+length;
    char *p;

    if(top>STRING_STORE_SIZE) {
        fprintf(stderr, "gencmn: out of memory\n");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    p=stringStore+stringTop;
    stringTop=top;
    return p;
}

static char * 
pathToFullPath(const char *path) {
    int32_t length;
    int32_t newLength;
    char *fullPath;
    int32_t n;

    length = (uint32_t)(uprv_strlen(path) + 1);
    newLength = (length + 1 + (int32_t)uprv_strlen(options[10].value));
    fullPath = uprv_malloc(newLength);
    if(options[10].doesOccur) {
        uprv_strcpy(fullPath, options[10].value);
        uprv_strcat(fullPath, U_FILE_SEP_STRING);
    } else {
        fullPath[0] = 0;
    }
    n = (int32_t)uprv_strlen(fullPath);
    uprv_strcat(fullPath, path);

#if (U_FILE_ALT_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR)
#if (U_FILE_ALT_SEP_CHAR != U_FILE_SEP_CHAR)
    /* replace tree separator (such as '/') with file sep char (such as ':' or '\\') */
    for(;fullPath[n];n++) {
        if(fullPath[n] == U_FILE_ALT_SEP_CHAR) {
            fullPath[n] = U_FILE_SEP_CHAR;
        }
    }
#endif
#endif
#if (U_FILE_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR)
    /* replace tree separator (such as '/') with file sep char (such as ':' or '\\') */
    for(;fullPath[n];n++) {
        if(fullPath[n] == U_TREE_ENTRY_SEP_CHAR) {
            fullPath[n] = U_FILE_SEP_CHAR;
        }
    }
#endif
    return fullPath;
}

static int
compareFiles(const void *file1, const void *file2) {
    /* sort by basename */
    return uprv_strcmp(((File *)file1)->basename, ((File *)file2)->basename);
}

static void
fixDirToTreePath(char *s)
{
#if (U_FILE_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR) || ((U_FILE_ALT_SEP_CHAR != U_FILE_SEP_CHAR) && (U_FILE_ALT_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR))
    char *t;
#endif
#if (U_FILE_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR)
    for(t=s;t=uprv_strchr(t,U_FILE_SEP_CHAR);) {
        *t = U_TREE_ENTRY_SEP_CHAR;
    }
#endif
#if (U_FILE_ALT_SEP_CHAR != U_FILE_SEP_CHAR) && (U_FILE_ALT_SEP_CHAR != U_TREE_ENTRY_SEP_CHAR)
    for(t=s;t=uprv_strchr(t,U_FILE_ALT_SEP_CHAR);) {
        *t = U_TREE_ENTRY_SEP_CHAR;
    }
#endif
}
/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
