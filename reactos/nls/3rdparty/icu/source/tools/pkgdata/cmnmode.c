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
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unewdata.h"
#include "uoptions.h"
#include "pkgtypes.h"
#include "makefile.h"

void pkg_mode_common(UPKGOptions *o, FileStream *makefile, UErrorCode *status)
{
  char tmp[1024];
  CharList *tail = NULL;

  uprv_strcpy(tmp, UDATA_CMN_PREFIX);
  uprv_strcat(tmp, o->shortName);
  uprv_strcat(tmp, UDATA_CMN_SUFFIX);
  
  if(!uprv_strcmp(o->mode, "common")) {
    /* If we're not the main mode.. don't change the output file list */
    
    /* We should be the only item. So we don't care about the order. */
    o->outFiles = pkg_appendToList(o->outFiles, &tail, uprv_strdup(tmp));
    
    if(o->nooutput || o->verbose) {
      fprintf(stdout, "# Output file: %s%s%s\n", o->targetDir, U_FILE_SEP_STRING, tmp);
    }
    
    if(o->nooutput) {
      *status = U_ZERO_ERROR;
      return;
    }
    
    sprintf(tmp, "# File to make:\nTARGET=%s%s%s\n\nTARGETNAME=%s\n", o->targetDir,
            U_FILE_SEP_STRING,
            o->outFiles->str,
            o->outFiles->str);
    T_FileStream_writeLine(makefile, tmp);
  } else {
    /* We're in another mode. but, set the target so they can find us.. */
    T_FileStream_writeLine(makefile, "TARGET=");
    T_FileStream_writeLine(makefile, tmp);
    T_FileStream_writeLine(makefile, "\n\n");
    
  } /* end [check to make sure we are in mode 'common' ] */
  
  sprintf(tmp, "# List file for gencmn:\n"
          "CMNLIST=%s%s%s_common.lst\n\n",
          o->tmpDir,
          U_FILE_SEP_STRING,
          o->shortName);
  T_FileStream_writeLine(makefile, tmp);

  sprintf(tmp, "all: $(TARGET)\n\n");
  T_FileStream_writeLine(makefile, tmp);
  
  T_FileStream_writeLine(makefile, "$(TARGET): $(CMNLIST) $(DATAFILEPATHS)\n"
               "\t$(INVOKE) $(ICUPKG) -t$(ICUDATA_CHAR) -c -s $(SRCDIR) -a $(CMNLIST) new $(TARGETDIR)/$(CNAME).dat\n\n");

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

  if(!uprv_strcmp(o->mode, "common")) { /* only install/clean in our own mode */
    T_FileStream_writeLine(makefile, "CLEANFILES= $(CMNLIST) $(TARGET)\n\nclean:\n\t-$(RMV) $(CLEANFILES) $(MAKEFILE)");
    T_FileStream_writeLine(makefile, "\n\n");
    
    sprintf(tmp, "install: $(TARGET)\n"
            "\t$(INSTALL_DATA) $(TARGET) $(INSTALLTO)%s$(TARGETNAME)\n\n",
            U_FILE_SEP_STRING);

    T_FileStream_writeLine(makefile, tmp);

  }
}
