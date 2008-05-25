/******************************************************************************
*
*   Copyright (C) 2000-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  winmode.c
*   encoding:   ANSI X3.4 (1968)
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000july14
*   created by: Vladimir Weinstein
*
*   This program packages the ICU data into different forms
*   (DLL, common data, etc.)
*/

#include "unicode/utypes.h"

#ifdef U_MAKE_IS_NMAKE

#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unewdata.h"
#include "uoptions.h"
#include "pkgtypes.h"
#include "makefile.h"
#include <stdio.h>
#include <stdlib.h>

/*
MSVC 2005 has the annoying habit of creating a manifest when one isn't needed.
The generated library doesn't depend on anything due to the /NOENTRY usage.
*/
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define NO_MANIFEST "/MANIFEST:NO "
#else
#define NO_MANIFEST ""
#endif

/*#define WINBUILDMODE (*(o->options)=='R'?"Release":"Debug")*/
#define CONTAINS_REAL_PATH(o) (*(o->options)==PKGDATA_DERIVED_PATH)

void writeCmnRules(UPKGOptions *o, const char *targetDirVar, FileStream *makefile)
{
    char tmp[1024];
    CharList *infiles;

    infiles = o->files; 
    sprintf(tmp, "\"$(%s)\\$(CMNTARGET)\" : $(DATAFILEPATHS)\n"
        "\t%s\"$(ICUPKG)\" -t%c %s%s%s -s \"$(SRCDIR)\" -a \"$(LISTFILES)\" new \"$(%s)\\$(CMNTARGET)\"\n",
        targetDirVar,
        (o->verbose ? "" : "@"),
        (U_IS_BIG_ENDIAN ? 'b' : 'l'),
        (o->comment ? "-C \"" : ""),
        (o->comment ? o->comment : ""),
        (o->comment ? "\" " : ""),
        targetDirVar);
    T_FileStream_writeLine(makefile, tmp);
}



void pkg_mode_windows(UPKGOptions *o, FileStream *makefile, UErrorCode *status) {
    char tmp[1024];
    char tmp2[1024];
    const char *separator = o->icuroot[uprv_strlen(o->icuroot)-1]=='\\'?"":"\\";
    UBool isDll = (UBool)(uprv_strcmp(o->mode, "dll") == 0);
    UBool isStatic = (UBool)(uprv_strcmp(o->mode, "static") == 0);

    if(U_FAILURE(*status)) {
        return;
    }

    sprintf(tmp2, "ICUROOT=%s\n\n", o->icuroot);
    T_FileStream_writeLine(makefile, tmp2);

    if (CONTAINS_REAL_PATH(o)) {
        sprintf(tmp2,
            "ICUPKG = $(ICUROOT)%sicupkg.exe\n", separator);
    }
    else {
        sprintf(tmp2,
            "ICUPKG = $(ICUROOT)%sbin\\icupkg.exe\n", separator);
    }
    T_FileStream_writeLine(makefile, tmp2);

    if(isDll) {
        uprv_strcpy(tmp, LIB_PREFIX);
        uprv_strcat(tmp, o->libName);
        if (o->version) {
            uprv_strcat(tmp, "$(TARGET_VERSION)");
        }
        uprv_strcat(tmp, UDATA_SO_SUFFIX);

        if(o->nooutput || o->verbose) {
            fprintf(stdout, "# Output %s file: %s%s%s\n", UDATA_SO_SUFFIX, o->targetDir, U_FILE_SEP_STRING, tmp);
        }

        if(o->nooutput) {
            *status = U_ZERO_ERROR;
            return;
        }

        sprintf(tmp2, "# DLL file to make:\nDLLTARGET=%s\n\n", tmp);
        T_FileStream_writeLine(makefile, tmp2);

        sprintf(tmp2,
            "LINK32 = link.exe\n"
            "LINK32_FLAGS = /nologo /release /out:\"$(TARGETDIR)\\$(DLLTARGET)\" /DLL /NOENTRY " NO_MANIFEST "$(LDFLAGS) $(PKGDATA_LDFLAGS) /implib:\"$(TARGETDIR)\\$(LIBNAME).lib\"\n");
        T_FileStream_writeLine(makefile, tmp2);

        if (CONTAINS_REAL_PATH(o)) {
            sprintf(tmp2,
                "GENCCODE = $(ICUROOT)%sgenccode.exe\n", separator);
        }
        else {
            sprintf(tmp2,
                "GENCCODE = $(ICUROOT)%sbin\\genccode.exe\n", separator);
        }
        T_FileStream_writeLine(makefile, tmp2);

        /* If you modify this, remember to modify makedata.mak too. */
        T_FileStream_writeLine(makefile, "\n"
            "# Windows specific DLL version information.\n"
            "!IF EXISTS(\"$(TEMP_DIR)\\icudata.res\")\n"
            "DATA_VER_INFO=\"$(TEMP_DIR)\\icudata.res\"\n"
            "!ELSE\n"
            "DATA_VER_INFO=\n"
            "!ENDIF\n\n");


        uprv_strcpy(tmp, UDATA_CMN_PREFIX "$(NAME)" UDATA_CMN_INTERMEDIATE_SUFFIX OBJ_SUFFIX);

        sprintf(tmp2, "# intermediate obj file:\nCMNOBJTARGET=%s\n\n", tmp);
        T_FileStream_writeLine(makefile, tmp2);
    }
    else if (isStatic)
    {
        uprv_strcpy(tmp, LIB_PREFIX);
        uprv_strcat(tmp, o->libName);
        uprv_strcat(tmp, UDATA_LIB_SUFFIX);

        if (!o->quiet) {
            pkg_sttc_writeReadme(o, tmp, status);
        }
        if(U_FAILURE(*status))
        {
            return;
        }

        if(o->nooutput || o->verbose) {
            fprintf(stdout, "# Output %s file: %s%s%s\n", UDATA_SO_SUFFIX, o->targetDir, U_FILE_SEP_STRING, tmp);
        }

        if(o->nooutput) {
            *status = U_ZERO_ERROR;
            return;
        }

        sprintf(tmp2, "# LIB file to make:\nDLLTARGET=%s\n\n", tmp);
        T_FileStream_writeLine(makefile, tmp2);

        sprintf(tmp2,
            "LINK32 = LIB.exe\n"
            "LINK32_FLAGS = /nologo /out:\"$(TARGETDIR)\\$(DLLTARGET)\"\n"
            );
        T_FileStream_writeLine(makefile, tmp2);


        if (CONTAINS_REAL_PATH(o)) {
            sprintf(tmp2,
                "GENCCODE = $(ICUROOT)%sgenccode.exe\n", separator);
        }
        else {
            sprintf(tmp2,
                "GENCCODE = $(ICUROOT)%sbin\\genccode.exe\n", separator);
        }
        T_FileStream_writeLine(makefile, tmp2);

        uprv_strcpy(tmp, UDATA_CMN_PREFIX "$(NAME)" UDATA_CMN_INTERMEDIATE_SUFFIX OBJ_SUFFIX);

        sprintf(tmp2, "# intermediate obj file\nCMNOBJTARGET=%s\n\n", tmp);
        T_FileStream_writeLine(makefile, tmp2);
    }
    uprv_strcpy(tmp, UDATA_CMN_PREFIX);
    uprv_strcat(tmp, o->cShortName);
    if (o->version && !uprv_strstr(o->shortName,o->version)) {
        uprv_strcat(tmp, "$(TARGET_VERSION)");
    }
    uprv_strcat(tmp, UDATA_CMN_SUFFIX);

    if(o->nooutput || o->verbose) {
        fprintf(stdout, "# Output file: %s%s%s\n", o->targetDir, U_FILE_SEP_STRING, tmp);
    }

    if(o->nooutput) {
        *status = U_ZERO_ERROR;
        return;
    }

    sprintf(tmp2, "# common file to make:\nCMNTARGET=%s\n\n", tmp);
    T_FileStream_writeLine(makefile, tmp2);


    if(isDll || isStatic) {
        sprintf(tmp, "all: \"$(TARGETDIR)\\$(DLLTARGET)\"\n\n");
        T_FileStream_writeLine(makefile, tmp);

        sprintf(tmp, "\"$(TARGETDIR)\\$(DLLTARGET)\": \"$(TEMP_DIR)\\$(CMNOBJTARGET)\"\n"
            "\t$(LINK32) $(LINK32_FLAGS) \"$(TEMP_DIR)\\$(CMNOBJTARGET)\" $(DATA_VER_INFO)\n\n");
        T_FileStream_writeLine(makefile, tmp);
        sprintf(tmp, "\"$(TEMP_DIR)\\$(CMNOBJTARGET)\": \"$(TEMP_DIR)\\$(CMNTARGET)\"\n"
            "\t@\"$(GENCCODE)\" $(GENCOPTIONS) -e $(ENTRYPOINT) -o -d \"$(TEMP_DIR)\" \"$(TEMP_DIR)\\$(CMNTARGET)\"\n\n");
        T_FileStream_writeLine(makefile, tmp);

        sprintf(tmp2,
            "clean:\n"
            "\t-@erase \"$(TARGETDIR)\\$(DLLTARGET)\"\n"
            "\t-@erase \"$(TEMP_DIR)\\$(CMNOBJTARGET)\"\n"
            "\t-@erase \"$(TEMP_DIR)\\$(CMNTARGET)\"\n\n");
        T_FileStream_writeLine(makefile, tmp2);

        T_FileStream_writeLine(makefile, "install: \"$(TARGETDIR)\\$(DLLTARGET)\"\n"
                                         "\tcopy \"$(TARGETDIR)\\$(DLLTARGET)\" \"$(INSTALLTO)\\$(DLLTARGET)\"\n\n");
        /* Write compile rules */
        writeCmnRules(o, "TEMP_DIR", makefile);
    } else { /* common */
        sprintf(tmp, "all: \"$(TARGETDIR)\\$(CMNTARGET)\"\n\n");
        T_FileStream_writeLine(makefile, tmp);

        sprintf(tmp2,
            "clean:\n"
            "\t-@erase \"$(TARGETDIR)\\$(CMNTARGET)\"\n\n");
        T_FileStream_writeLine(makefile, tmp2);

        T_FileStream_writeLine(makefile, "install: \"$(TARGETDIR)\\$(CMNTARGET)\"\n"
                                         "\tcopy \"$(TARGETDIR)\\$(CMNTARGET)\" \"$(INSTALLTO)\\$(CMNTARGET)\"\n\n");

        /* Write compile rules */
        writeCmnRules(o, "TARGETDIR", makefile);
    }

    T_FileStream_writeLine(makefile, "rebuild: clean all\n\n");

}

#endif
