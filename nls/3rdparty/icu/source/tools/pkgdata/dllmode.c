/******************************************************************************
*
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  pkgdata.c
*   encoding:   ANSI X3.4 (1968)
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000may15
*   created by: Steven \u24C7 Loomis
*
*   This program packages the ICU data into different forms
*   (DLL, common data, etc.)
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"

#ifndef U_MAKE_IS_NMAKE
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unewdata.h"
#include "uoptions.h"
#include "pkgtypes.h"
#include "makefile.h"


void pkg_mode_dll(UPKGOptions *o, FileStream *makefile, UErrorCode *status)
{
    char tmp[1024];
    CharList *tail = NULL;
    CharList *objects = NULL;
    
    if(U_FAILURE(*status)) {
        return;
    }
    
    uprv_strcpy(tmp, LIB_PREFIX "$(LIBNAME)" UDATA_SO_SUFFIX);
    
    /* We should be the only item. So we don't care about the order. */
    o->outFiles = pkg_appendToList(o->outFiles, &tail, uprv_strdup(tmp));
    
    if(o->nooutput || o->verbose) {
        fprintf(stdout, "# Output file: %s%s%s\n", o->targetDir, U_FILE_SEP_STRING, tmp);
    }
    
    if(o->nooutput) {
        *status = U_ZERO_ERROR;
        return;
    }
    
    /* begin writing makefile ========================= */
    
    
    /*390port start*/
#ifdef OS390BATCH
    if (uprv_strcmp(o->libName, U_LIBICUDATA_NAME) == 0)
        sprintf(tmp, "# File to make:\nBATCH_TARGET=\"//'${LOADMOD}(IXMI" U_ICU_VERSION_SHORT "DA)'\"\n\n");
    else if (uprv_strcmp(o->libName, "testdata") == 0)
        sprintf(tmp, "# File to make:\nBATCH_TARGET=\"//'${LOADMOD}(IXMI" U_ICU_VERSION_SHORT "TE)'\"\n\n");
    else if (uprv_strcmp(o->libName, U_LIBICUDATA_NAME"_stub") == 0)
        sprintf(tmp, "# File to make:\nBATCH_TARGET=\"//'${LOADMOD}(IXMI" U_ICU_VERSION_SHORT "D1)'\"\n\n");
    else
        sprintf(tmp, "\n");
    T_FileStream_writeLine(makefile, tmp);
#endif
    
    T_FileStream_writeLine(makefile, "# Version numbers:\nVERSIONED=");
    if (o->version) {
        sprintf(tmp, ".%s", o->version);
        if (!uprv_strchr(o->version, '.')) {
            uprv_strcat(tmp, ".0");
        }
        T_FileStream_writeLine(makefile, tmp);
        T_FileStream_writeLine(makefile, "\nDLL_LDFLAGS=$(LD_SONAME) $(RPATH_LDFLAGS) $(BIR_LDFLAGS)\nDLL_DEPS=$(BIR_DEPS)\n");
    } else {
        T_FileStream_writeLine(makefile, "\nDLL_LDFLAGS=$(BIR_LDFLAGS)\nDLL_DEPS=$(BIR_DEPS)\n"); 
    }
    T_FileStream_writeLine(makefile, "\n");
    
    sprintf(tmp, "# File to make:\nTARGET=%s\n\n", o->outFiles->str);
    T_FileStream_writeLine(makefile, tmp);
    if (o->version) {
        char *p;
        const char *v;
        
        T_FileStream_writeLine(makefile, "SO_TARGET=$(TARGET)\n");
        sprintf(tmp, "SO_TARGET_VERSION=%s\n", o->version);
        T_FileStream_writeLine(makefile, tmp);
        uprv_strcpy(tmp, "SO_TARGET_VERSION_MAJOR=");
        for (p = tmp + uprv_strlen(tmp), v = o->version; *v && *v != '.'; ++v) {
            *p++ = *v;
        }
        *p++ = '\n';
        *p++ = '\n';
        *p++ = 0;
        T_FileStream_writeLine(makefile, tmp);
    } else {
        T_FileStream_writeLine(makefile, "FINAL_SO_TARGET=$(TARGET)\n");
        T_FileStream_writeLine(makefile, "MIDDLE_SO_TARGET=$(TARGET)\n");
    }

    T_FileStream_writeLine(makefile, "DYNAMICCPPFLAGS=$(SHAREDLIBCPPFLAGS)\n");
    T_FileStream_writeLine(makefile, "DYNAMICCFLAGS=$(SHAREDLIBCFLAGS)\n");
    T_FileStream_writeLine(makefile, "DYNAMICCXXFLAGS=$(SHAREDLIBCXXFLAGS)\n");
    T_FileStream_writeLine(makefile, "\n");
    
#ifdef OS400
    sprintf(tmp, "# Force override for iSeries compilation since data does not need to be\n"
                 "# nor can excessively large files be compiled for debug\n"
                 "override COMPILE.c= $(CC) $(DEFS) $(CPPFLAGS) -O4 -c -qTERASPACE=*YES -qSTGMDL=*INHERIT -qPFROPT=*STRDONLY\n\n");
    T_FileStream_writeLine(makefile, tmp);
#endif

    uprv_strcpy(tmp, "all: $(TARGETDIR)/$(FINAL_SO_TARGET) $(BATCH_TARGET)");
    if (o->version) {
        uprv_strcat(tmp, " $(TARGETDIR)/$(MIDDLE_SO_TARGET) $(TARGETDIR)/$(SO_TARGET)");
    }
    uprv_strcat(tmp, "\n\n");
    T_FileStream_writeLine(makefile, tmp);
    
#ifdef OS400
    /* New for iSeries: All packaged data in one .c */
    sprintf(tmp, "# Create a file which contains all .c data files/structures\n"
                 "$(TEMP_DIR)/$(NAME)all.c: $(CMNLIST)\n\n");
    T_FileStream_writeLine(makefile, tmp);
#endif

    /* Write compile rules */
    pkg_mak_writeObjRules(o, makefile, &objects, OBJ_SUFFIX);
    
    sprintf(tmp, "# List file for gencmn:\n"
                 "CMNLIST=%s%s$(NAME)_dll.lst\n\n",
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
 
    sprintf(tmp, "TOCSYM= %s_dat \n\n", o->entryName); /* entrypoint not always shortname! */
    T_FileStream_writeLine(makefile, tmp);

    pkg_mak_writeAssemblyHeader(makefile, o);

    sprintf(tmp,"$(TEMP_DIR)/$(NAME)_dat.o : $(TEMP_DIR)/$(NAME)_dat.c\n"
                "\t$(COMPILE.c) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $<\n\n");
    T_FileStream_writeLine(makefile, tmp);
    
    T_FileStream_writeLine(makefile, "# 'TOCOBJ' contains C Table of Contents objects [if any]\n");
    
    sprintf(tmp, "$(TEMP_DIR)/$(NAME)_dat.c: $(CMNLIST)\n"
                 "\t$(INVOKE) $(GENCMN) -e $(ENTRYPOINT) -n $(NAME) -S -s $(SRCDIR) -d $(TEMP_DIR) 0 $(CMNLIST)\n\n");

    T_FileStream_writeLine(makefile, tmp);
    sprintf(tmp, "TOCOBJ= $(NAME)_dat%s \n\n", OBJ_SUFFIX);
    T_FileStream_writeLine(makefile, tmp);

#ifdef OS400
    /* New for iSeries: All packaged data in one .c */
    sprintf(tmp, "$(TEMP_DIR)/$(NAME)all.o : $(TEMP_DIR)/$(NAME)all.c\n"
                 "\t$(COMPILE.c) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $<\n\n");
    T_FileStream_writeLine(makefile, tmp);

    T_FileStream_writeLine(makefile, "# 'ALLDATAOBJ' contains all .c data structures\n");

    sprintf(tmp, "ALLDATAOBJ= $(NAME)all%s \n\n", OBJ_SUFFIX);
    T_FileStream_writeLine(makefile, tmp);
#endif

    T_FileStream_writeLine(makefile, "BASE_OBJECTS= $(TOCOBJ) ");
#ifdef OS400
    T_FileStream_writeLine(makefile, "$(ALLDATAOBJ) ");
#else
    pkg_writeCharListWrap(makefile, objects, " ", " \\\n",0);
#endif
    pkg_mak_writeAssemblyFooter(makefile, o);

    T_FileStream_writeLine(makefile, "\n\n");
    T_FileStream_writeLine(makefile, "OBJECTS=$(BASE_OBJECTS:%=$(TEMP_DIR)/%)\n\n");
    
    T_FileStream_writeLine(makefile,"$(TEMP_DIR)/%.o: $(TEMP_DIR)/%.c\n\t$(COMPILE.c) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $<\n\n");
    
    T_FileStream_writeLine(makefile,"build-objs: $(SOURCES) $(OBJECTS)\n\n$(OBJECTS): $(SOURCES)\n\n");
    
#ifdef U_HPUX
    T_FileStream_writeLine(makefile, "$(TARGETDIR)/$(FINAL_SO_TARGET): $(OBJECTS) $(HPUX_JUNK_OBJ) $(LISTFILES) $(DLL_DEPS)\n"
                                     "\t$(SHLIB.cc) -o $@ $(OBJECTS) $(HPUX_JUNK_OBJ) $(DLL_LDFLAGS)\n"
                                     "\t-ls -l $@\n\n");
    
    T_FileStream_writeLine(makefile, "$(TEMP_DIR)/hpux_junk_obj.cpp:\n"
                                     "\techo \"void to_emit_cxx_stuff_in_the_linker(){}\" >> $(TEMP_DIR)/hpux_junk_obj.cpp\n"
                                     "\n"
                                     "$(TEMP_DIR)/hpux_junk_obj.o: $(TEMP_DIR)/hpux_junk_obj.cpp\n"
                                     "\t$(COMPILE.cc) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $<\n"
                                     "\n");
#else
    
    /*390port*/
#ifdef OS390BATCH
    T_FileStream_writeLine(makefile, "$(BATCH_TARGET): $(OBJECTS) $(LISTFILES) $(DLL_DEPS)\n"
                                     "\t$(SHLIB.c) -o $@ $(OBJECTS) $(DLL_LDFLAGS)\n\n");
#endif
    
#ifdef U_AIX
    T_FileStream_writeLine(makefile, "$(TARGETDIR)/$(FINAL_SO_TARGET): $(OBJECTS) $(LISTFILES) $(DLL_DEPS)\n"
                                     "\t$(SHLIB.c) -o $(FINAL_SO_TARGET:.$(SO)=.$(SOBJ)) $(OBJECTS) $(DLL_LDFLAGS)\n"
                                     "\t$(AR) $(ARFLAGS) $@ $(FINAL_SO_TARGET:.$(SO)=.$(SOBJ))\n"
                                     "\t-$(AR) vt $@\n\n");
#else
    T_FileStream_writeLine(makefile, "$(TARGETDIR)/$(FINAL_SO_TARGET): $(OBJECTS) $(LISTFILES) $(DLL_DEPS)\n"
                                     "\t$(SHLIB.c) -o $@ $(OBJECTS) $(DLL_LDFLAGS)\n"
                                     "\t-ls -l $@\n\n");
#endif

#ifdef OS390
  /*
     jdc26:  The above link resolves to

               -o ../data/out/libicudata26.0.dll

             the first time or

               -o ../data/out/libicudata_stub26.0.dll

             the second time in the build.  OS/390 puts each DLL into
             the specified directory and the sidedeck into the current
             directory.  Move the sidedeck into the same directory as
             the DLL so it can be found with the DLL later in the
             build.

   */
  T_FileStream_writeLine(makefile, "\t-cp -f ./$(basename $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT) $(TARGETDIR)/$(basename $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT)\n");
#endif   /* OS/390 */
#endif
    
    T_FileStream_writeLine(makefile, "CLEANFILES= $(CMNLIST) $(OBJECTS) $(HPUX_JUNK_OBJ) $(TARGETDIR)/$(FINAL_SO_TARGET) $(TARGETDIR)/$(MIDDLE_SO_TARGET) $(TARGETDIR)/$(TARGET)\n\nclean:\n\t-$(RMV) $(CLEANFILES) $(MAKEFILE)");
    T_FileStream_writeLine(makefile, "\n\n");
    
    T_FileStream_writeLine(makefile, "install: $(TARGETDIR)/$(FINAL_SO_TARGET)\n"
                                     "\t$(INSTALL-L) $(TARGETDIR)/$(FINAL_SO_TARGET) $(INSTALLTO)/$(FINAL_SO_TARGET)\n");
    
    T_FileStream_writeLine(makefile, "ifneq ($(IMPORT_LIB_EXT),)\n");
    T_FileStream_writeLine(makefile, "\t$(INSTALL-L) $(TARGETDIR)/$(basename $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT) $(INSTALLTO)/$(basename( $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT)\n");
    T_FileStream_writeLine(makefile, "endif\n");
    if (o->version) {
        T_FileStream_writeLine(makefile, "ifneq ($(FINAL_SO_TARGET),$(SO_TARGET))\n");
        T_FileStream_writeLine(makefile, "\tcd $(INSTALLTO) && $(RM) $(SO_TARGET) && ln -s $(FINAL_SO_TARGET) $(SO_TARGET)\n");
        T_FileStream_writeLine(makefile, "ifneq ($(FINAL_SO_TARGET),$(MIDDLE_SO_TARGET))\n");
        T_FileStream_writeLine(makefile, "\tcd $(INSTALLTO) && $(RM) $(MIDDLE_SO_TARGET) && ln -s $(FINAL_SO_TARGET) $(MIDDLE_SO_TARGET)\n");
        T_FileStream_writeLine(makefile, "endif\n");
        T_FileStream_writeLine(makefile, "endif\n");

#ifdef OS390
        T_FileStream_writeLine(makefile, "ifneq ($(IMPORT_LIB_EXT),)\n");
        T_FileStream_writeLine(makefile, "\tcd $(INSTALLTO) && $(RM) $(basename $(MIDDLE_SO_TARGET))$(IMPORT_LIB_EXT) && ln -s $(basename $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT) $(basename $(MIDDLE_SO_TARGET))$(IMPORT_LIB_EXT)\n");
        T_FileStream_writeLine(makefile, "\tcd $(INSTALLTO) && $(RM) $(basename $(SO_TARGET))$(IMPORT_LIB_EXT) && ln -s $(basename $(FINAL_SO_TARGET))$(IMPORT_LIB_EXT) $(basename $(SO_TARGET))$(IMPORT_LIB_EXT)\n");
        T_FileStream_writeLine(makefile, "endif\n");
#endif
    }
    T_FileStream_writeLine(makefile, "\n");
    
#ifdef U_SOLARIS
    T_FileStream_writeLine(makefile, "$(NAME).map:\n\techo \"{global: $(TOCSYM); local: *; };\" > $@\n\n");
#endif
    
#ifdef U_AIX
    T_FileStream_writeLine(makefile, "$(NAME).map:\n\techo \"$(TOCSYM)\" > $@\n\n");
#endif
    
    
    *status = U_ZERO_ERROR;
    
}

#endif  /* #ifndef U_MAKE_IS_NMAKE */

