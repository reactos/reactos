/******************************************************************************
*
*   Copyright (C) 2002-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  staticmode.c
*   encoding:   ANSI X3.4 (1968)
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002mar14
*   created by: Steven \u24C7 Loomis
*
*   This program packages the ICU data into a static library.
*   It is *mainly* used by POSIX, but the top function (for writing READMEs) is
*   shared with Win32.
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uloc.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unewdata.h"
#include "uoptions.h"
#include "pkgtypes.h"
#include "filestrm.h"

#include <stdio.h>
#include <stdlib.h>

void pkg_sttc_writeReadme(struct UPKGOptions_ *o, const char *libName, UErrorCode *status)
{
  char tmp[1024];
  FileStream  *out;

  if(U_FAILURE(*status))
  {
      return;
  }

  /* Makefile pathname */
  uprv_strcpy(tmp, o->targetDir);
  uprv_strcat(tmp, U_FILE_SEP_STRING "README_");
  uprv_strcat(tmp, o->shortName);
  uprv_strcat(tmp, ".txt");

  out = T_FileStream_open(tmp, "w");
  if (!out) {
      fprintf(stderr, "err: couldn't create README file %s\n", tmp);
      *status = U_FILE_ACCESS_ERROR;
      return;
  }

  sprintf(tmp, "## README for \"%s\"'s static data (%s)\n"
               "## created by pkgdata, ICU Version %s\n",
             o->shortName,
             libName,
             U_ICU_VERSION);

  T_FileStream_writeLine(out, tmp);

  sprintf(tmp, "\n\nTo use this data in your application:\n\n"
               "1. At the top of your source file, add the following lines:\n"
               "\n"
               "     #include \"unicode/utypes.h\"\n"
               "     #include \"unicode/udata.h\"\n"
               "     U_CFUNC char %s_dat[];\n",
               o->cShortName);
  T_FileStream_writeLine(out, tmp);

  sprintf(tmp, "2. *Early* in your application, call the following function:\n"
               "\n"
               "     UErrorCode myError = U_ZERO_ERROR;\n"
               "     udata_setAppData( \"%s\", (const void*) %s_dat, &myError);\n"
               "     if(U_FAILURE(myError))\n"
               "     {\n"
               "          handle error condition ...\n"
               "     }\n"
               "\n",
               o->cShortName, o->cShortName);
  T_FileStream_writeLine(out, tmp);

  sprintf(tmp, "3. Link your application against %s\n"
               "\n\n"
               "4. Now, you may access this data with a 'path' of \"%s\" as in the following example:\n"
               "\n"
               "     ... ures_open( \"%s\", NULL /* Get the default locale */, &err ); \n",
               libName, o->shortName, o->shortName);
  T_FileStream_writeLine(out, tmp);

  T_FileStream_close(out);
}


#ifndef U_MAKE_IS_NMAKE


#include "makefile.h"


void pkg_mode_static(UPKGOptions *o, FileStream *makefile, UErrorCode *status)
{
    char tmp[1024];
    CharList *tail = NULL;
    CharList *objects = NULL;

    if(U_FAILURE(*status)) {
        return;
    }

    uprv_strcpy(tmp, LIB_STATIC_PREFIX);
    uprv_strcat(tmp, o->libName);
    uprv_strcat(tmp, UDATA_LIB_SUFFIX);

    o->outFiles = pkg_appendToList(o->outFiles, &tail, uprv_strdup(tmp));

    if (!o->quiet) {
        pkg_sttc_writeReadme(o, tmp, status);
    }
    if(U_FAILURE(*status)) {
        return;
    }


    if(o->nooutput || o->verbose) {
        fprintf(stdout, "# Output file: %s%s%s\n", o->targetDir, U_FILE_SEP_STRING, tmp);
    }

    if(o->nooutput) {
        *status = U_ZERO_ERROR;
        return;
    }

    /* begin writing makefile ========================= */


    T_FileStream_writeLine(makefile, "# Version numbers:\nVERSIONED=");
    if (o->version) {
        sprintf(tmp, ".%s", o->version);
        if (!uprv_strchr(o->version, '.')) {
            uprv_strcat(tmp, ".0");
        }
        T_FileStream_writeLine(makefile, tmp);
        T_FileStream_writeLine(makefile, "\nDLL_LDFLAGS=$(LD_SONAME) $(RPATH_LDFLAGS)\n");
    } else {
        T_FileStream_writeLine(makefile, "\nDLL_LDFLAGS=$(BIR_LDFLAGS)\n");
    }
    T_FileStream_writeLine(makefile, "\n");

    sprintf(tmp, "# File to make:\nTARGET=%s\n\n", o->outFiles->str);
    T_FileStream_writeLine(makefile, tmp);
    T_FileStream_writeLine(makefile, "LIB_TARGET=$(TARGET)\n");

    uprv_strcpy(tmp, "all: $(TARG_PATH)$(LIB_TARGET)");
    uprv_strcat(tmp, "\n\n");
    T_FileStream_writeLine(makefile, tmp);

#ifdef OS400
    /* New for iSeries: All packaged data in one .c */
    sprintf(tmp, "# Create a file which contains all .c data files/structures\n"
                 "$(TEMP_DIR)/$(NAME)all.c: $(CMNLIST)\n\n");
    T_FileStream_writeLine(makefile, tmp);
#endif

    /* Write compile rules */
    pkg_mak_writeObjRules(o, makefile, &objects, ".$(STATIC_O)"); /* use special .o suffix */

    sprintf(tmp, "# List file for gencmn:\n"
        "CMNLIST=%s%s$(NAME)_static.lst\n\n",
        o->tmpDir,
        U_FILE_SEP_STRING);
    T_FileStream_writeLine(makefile, tmp);

    if(o->hadStdin == FALSE) { /* shortcut */
        T_FileStream_writeLine(makefile, "$(CMNLIST): $(LISTFILES)\n"
            "\tcat $(LISTFILES) > $(CMNLIST)\n\n");
    } else {
        T_FileStream_writeLine(makefile, "$(CMNLIST): \n"
            "\t@echo \"generating $@ (list of data files)\"\n"
            "\t@-$(RMV) $@\n"
            "\t@for file in $(DATAFILEPATHS); do \\\n"
            "\t  echo $$file >> $@; \\\n"
            "\tdone;\n\n");
    }

    pkg_mak_writeAssemblyHeader(makefile, o);

    sprintf(tmp,"$(TEMP_PATH)$(NAME)_dat.$(STATIC_O) : $(TEMP_PATH)$(NAME)_dat.c\n"
        "\t$(COMPILE.c) -o $@ $<\n\n");
    T_FileStream_writeLine(makefile, tmp);

    T_FileStream_writeLine(makefile, "# 'TOCOBJ' contains C Table of Contents objects [if any]\n");

    sprintf(tmp, "$(TEMP_PATH)$(NAME)_dat.c: $(CMNLIST)\n"
            "\t$(INVOKE) $(GENCMN) -e $(ENTRYPOINT) -n $(NAME) -S -s $(SRCDIR) -d $(TEMP_DIR) 0 $(CMNLIST)\n\n");
    T_FileStream_writeLine(makefile, tmp);

    sprintf(tmp, "TOCOBJ= $(NAME)_dat.$(STATIC_O)\n\n");
    T_FileStream_writeLine(makefile, tmp);

#ifdef OS400
    /* New for iSeries: All packaged data in one .c */
    sprintf(tmp,"$(TEMP_PATH)$(NAME)all.$(STATIC_O) : $(TEMP_PATH)$(NAME)all.c\n"
        "\t$(COMPILE.c) -o $@ $<\n\n");
    T_FileStream_writeLine(makefile, tmp);

    T_FileStream_writeLine(makefile, "# 'ALLDATAOBJ' contains all .c data structures\n");

    sprintf(tmp, "ALLDATAOBJ= $(NAME)all%s \n\n", OBJ_SUFFIX);
    T_FileStream_writeLine(makefile, tmp);
#endif

    sprintf(tmp, "TOCSYM= $(ENTRYPOINT)_dat \n\n"); /* entrypoint not always shortname! */
    T_FileStream_writeLine(makefile, tmp);

    T_FileStream_writeLine(makefile, "BASE_OBJECTS= $(TOCOBJ) ");

#ifdef OS400
    T_FileStream_writeLine(makefile, "$(ALLDATAOBJ) ");
#else
    pkg_writeCharListWrap(makefile, objects, " ", " \\\n",0);
#endif
    pkg_mak_writeAssemblyFooter(makefile, o);

    T_FileStream_writeLine(makefile, "\n\n");
    T_FileStream_writeLine(makefile, "OBJECTS=$(BASE_OBJECTS:%=$(TEMP_PATH)%)\n\n");

    T_FileStream_writeLine(makefile,"$(TEMP_PATH)%.$(STATIC_O): $(TEMP_PATH)%.c\n\t  $(COMPILE.c) -o $@ $<\n\n");

    T_FileStream_writeLine(makefile, "$(TARG_PATH)$(LIB_TARGET): $(OBJECTS) $(LISTFILES)\n"
                            "\t$(AR) $(ARFLAGS) $(AR_OUTOPT)$@ $(OBJECTS)\n"
                            "\t$(RANLIB) $@\n\n");


    T_FileStream_writeLine(makefile, "CLEANFILES= $(CMNLIST) $(OBJECTS) $(TARG_PATH)$(LIB_TARGET) $(TARG_PATH)$(MIDDLE_STATIC_LIB_TARGET) $(TARG_PATH)$(TARGET)\n\nclean:\n\t-$(RMV) $(CLEANFILES) $(MAKEFILE)");
    T_FileStream_writeLine(makefile, "\n\n");

    T_FileStream_writeLine(makefile, "# static mode shouldn't need to be installed, but we will install the header and static library for them.\n");

    T_FileStream_writeLine(makefile, "install: $(TARG_PATH)$(LIB_TARGET)\n"
                "\t$(INSTALL-L) $(TARG_PATH)$(LIB_TARGET) $(INSTALLTO)/$(LIB_TARGET)\n");
    T_FileStream_writeLine(makefile, "\t$(RANLIB) $(INSTALLTO)/$(LIB_TARGET)\n");
    if (o->version) {
        T_FileStream_writeLine(makefile, "\tcd $(INSTALLTO) && $(RM) $(MIDDLE_STATIC_LIB_TARGET) && ln -s $(LIB_TARGET) $(MIDDLE_STATIC_LIB_TARGET)\n\tcd $(INSTALLTO) && $(RM) $(STATIC_LIB_TARGET) && ln -s $(LIB_TARGET) $(STATIC_LIB_TARGET)\n");
    T_FileStream_writeLine(makefile, "\t$(RANLIB) $(INSTALLTO)/$(STATIC_LIB_TARGET)\n\n");

    }

    *status = U_ZERO_ERROR;

}



#endif
