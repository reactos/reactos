/**************************************************************************
*
*   Copyright (C) 2000-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
***************************************************************************
*   file name:  make.c
*   encoding:   ANSI X3.4 (1968)
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000jul18
*   created by: Vladimir Weinstein
*   created on: 2000may17
*   created by: Steven \u24C7 Loomis
*   merged on: 2003sep14
*   merged by: George Rhoten
*   merged from nmake.c and gmake.c
*
* Emit a NMAKE or GNU makefile
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "makefile.h"
#include "cstring.h"
#include "cmemory.h"
#include <stdio.h>

#ifdef U_MAKE_IS_NMAKE

char linebuf[2048];

/* Write any setup/initialization stuff */
void
pkg_mak_writeHeader(FileStream *f, const UPKGOptions *o)
{
    const char *appendVersion = NULL;
    char *srcDir = convertToNativePathSeparators(uprv_strdup(o->srcDir));

    if(o->version && !uprv_strstr(o->shortName,o->version)) { /* do not append version if 
                                                                 already contained in the name */
      appendVersion = o->version;
    }

    sprintf(linebuf, "## Makefile for %s (%s) created by pkgdata\n"
                    "## from ICU Version %s\n"
                    "\n",
                    o->shortName,
                    o->libName,
                    U_ICU_VERSION);
    T_FileStream_writeLine(f, linebuf);
    
    sprintf(linebuf, "NAME=%s%s\n"
                    "CNAME=%s\n"
                    "LIBNAME=%s\n"
                    "SRCDIR=%s\n"
                    "TARGETDIR=%s\n"
                    "TEMP_DIR=%s\n"
                    "MODE=%s\n"
                    "MAKEFILE=%s\n"
                    "ENTRYPOINT=%s\n"
                    "TARGET_VERSION=%s\n"
                    "MKINSTALLDIRS=mkdir\n"
                    "INSTALL_DATA=copy\n"
                    "RMV=del /F"
                    "\n\n\n",
                    o->shortName,
                    (appendVersion ? appendVersion : ""),
                    o->cShortName,
                    o->libName,
                    srcDir,
                    o->targetDir,
                    o->tmpDir,
                    o->mode,
                    o->makeFile,
                    o->entryName,
                    o->version);
    T_FileStream_writeLine(f, linebuf);

    sprintf(linebuf, "## List files [%d] containing data files to process (note: - means stdin)\n"
        "LISTFILES= ",
        pkg_countCharList(o->fileListFiles));
    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->fileListFiles, " ", " \\\n", 0);

    T_FileStream_writeLine(f, "\n\n\n");

    sprintf(linebuf, "## Data Files [%d]\n"
        "DATAFILES= ",
        pkg_countCharList(o->files));

    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->files, " ", " \\\n", -1);

    T_FileStream_writeLine(f, "\n\n\n");

    sprintf(linebuf, "## Data File Paths [%d]\n"
        "DATAFILEPATHS= ",
        pkg_countCharList(o->filePaths));

    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->filePaths, " ", " \\\n", 1);

    T_FileStream_writeLine(f, "\n\n\n");

    uprv_free(srcDir);
}

/* Write a stanza in the makefile, with specified   "target: parents...  \n\n\tcommands" [etc] */
void
pkg_mak_writeStanza(FileStream *f, const UPKGOptions *o, 
                    const char *target,
                    CharList* parents,
                    CharList* commands )
{
    T_FileStream_write(f, target, (int32_t)uprv_strlen(target));
    T_FileStream_write(f, " : ", 3);
    pkg_writeCharList(f, parents, " ",1);
    T_FileStream_write(f, "\n", 1);
    
    if(commands)
    {
        T_FileStream_write(f, "\t", 1);
        pkg_writeCharList(f, commands, "\n\t",0);
    }
    T_FileStream_write(f, "\n\n", 2);
}

/* write any cleanup/post stuff */
void
pkg_mak_writeFooter(FileStream *f, const UPKGOptions *o)
{
    char buf[256];
    sprintf(buf, "\n\n# End of makefile for %s [%s mode]\n\n", o->shortName, o->mode);
    T_FileStream_write(f, buf, (int32_t)uprv_strlen(buf));
}

#else   /* #ifdef U_MAKE_IS_NMAKE */

#include "cmemory.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unewdata.h"
#include "uoptions.h"
#include "pkgtypes.h"
#include <string.h>

char linebuf[2048];

/* Write any setup/initialization stuff */
void
pkg_mak_writeHeader(FileStream *f, const UPKGOptions *o)
{
    sprintf(linebuf, "## Makefile for %s created by pkgdata\n"
                    "## from ICU Version %s\n"
                    "\n",
                    o->shortName,
                    U_ICU_VERSION);
    T_FileStream_writeLine(f, linebuf);
    
    sprintf(linebuf, "NAME=%s\n"
                     "LIBNAME=%s\n"
                    "CNAME=%s\n"
                    "TARGETDIR=%s\n"
                    "TEMP_DIR=%s\n"
                    "srcdir=$(TEMP_DIR)\n"
                    "SRCDIR=%s\n"
                    "MODE=%s\n"
                    "MAKEFILE=%s\n"
                    "ENTRYPOINT=%s\n"
                    "include %s\n"
                    "\n\n\n",
                    o->shortName,
                    o->libName,
                    o->cShortName,
                    o->targetDir,
                    o->tmpDir,
                    o->srcDir,
                    o->mode,
                    o->makeFile,
                    o->entryName,
                    o->options);
    T_FileStream_writeLine(f, linebuf);
    
    /* TEMP_PATH  and TARG_PATH will be empty if the respective dir is . */
    /* Avoid //'s and .'s which confuse make ! */
    if(!strcmp(o->tmpDir,"."))
    {
        T_FileStream_writeLine(f, "TEMP_PATH=\n");
    }
    else
    {
        T_FileStream_writeLine(f, "TEMP_PATH=$(TEMP_DIR)/\n");
    }
    
    if(!strcmp(o->targetDir,"."))
    {
        T_FileStream_writeLine(f, "TARG_PATH=\n");
    }
    else
    {
        T_FileStream_writeLine(f, "TARG_PATH=$(TARGETDIR)/\n");
    }

    sprintf(linebuf, "## List files [%u] containing data files to process (note: - means stdin)\n"
                    "LISTFILES= ",
                    (int)pkg_countCharList(o->fileListFiles));
    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->fileListFiles, " ", " \\\n",0);

    T_FileStream_writeLine(f, "\n\n\n");

    sprintf(linebuf, "## Data Files [%u]\n"
        "DATAFILES= ",
        (int)pkg_countCharList(o->files));

    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->files, " ", " \\\n",-1);

    T_FileStream_writeLine(f, "\n\n\n");

    sprintf(linebuf, "## Data File Paths [%u]\n"
                    "DATAFILEPATHS= ",
                    (int)pkg_countCharList(o->filePaths));

    T_FileStream_writeLine(f, linebuf);

    pkg_writeCharListWrap(f, o->filePaths, " ", " \\\n",0);

    T_FileStream_writeLine(f, "\n\n\n");

}

/* Write a stanza in the makefile, with specified   "target: parents...  \n\n\tcommands" [etc] */
void
pkg_mak_writeStanza(FileStream *f, const UPKGOptions *o, 
                    const char *target,
                    CharList* parents,
                    CharList* commands)
{
    T_FileStream_write(f, target, uprv_strlen(target));
    T_FileStream_write(f, " : ", 3);
    pkg_writeCharList(f, parents, " ",0);
    T_FileStream_write(f, "\n", 1);
    
    if(commands)
    {
        T_FileStream_write(f, "\t", 1);
        pkg_writeCharList(f, commands, "\n\t",0);
    }
    T_FileStream_write(f, "\n\n", 2);
}

/* write any cleanup/post stuff */
void
pkg_mak_writeFooter(FileStream *f, const UPKGOptions *o)
{
    T_FileStream_writeLine(f, "\nrebuild: clean all\n");
}


void
pkg_mak_writeObjRules(UPKGOptions *o,  FileStream *makefile, CharList **objects, const char* objSuffix)
{
    const char *p, *baseName;
    char *tmpPtr;
    char tmp[1024];
    char stanza[1024];
    char cfile[1024];
    CharList *oTail = NULL;
    CharList *infiles;
    CharList *parents = NULL, *commands = NULL;
    int32_t genFileOffset = 0;  /* offset from beginning of .c and .o file name, use to chop off package name for AS/400 */
    char *parentPath;
    const char *tchar;
    char tree[1024];

    infiles = o->files; /* raw files - no paths other than tree paths */
    
#if defined (OS400)
    if(infiles != NULL) {
        baseName = findBasename(infiles->str);
        p = uprv_strchr(baseName, '_');
        if(p != NULL) { 
            genFileOffset = (p-baseName)+1; /* "package_"  - name + underscore */
        }
    }
#endif
    
    for(;infiles;infiles = infiles->next) {
      baseName = infiles->str; /* skip the icudt28b/ part */
      p = uprv_strrchr(baseName, '.');
      if( (p == NULL) || (*p == '\0' ) ) {
        continue;
      }
      
      uprv_strncpy(tmp, baseName, p-baseName);
      p++;
      
      uprv_strcpy(tmp+(p-1-baseName), "_"); /* to append */
      uprv_strcat(tmp, p);
      uprv_strcat(tmp, objSuffix );
      
      /* iSeries cannot have '-' in the .o objects. */
      for( tmpPtr = tmp; *tmpPtr; tmpPtr++ ) {
        if ( *tmpPtr == U_FILE_SEP_CHAR ) { /* map tree names with underscores */
          *tmpPtr = '_';
        }
        if ( *tmpPtr == '-' ) {
          *tmpPtr = '_';
        }
      }
      
      *objects = pkg_appendToList(*objects, &oTail, uprv_strdup(tmp + genFileOffset)); /* Offset for AS/400 */
      
      /* write source list */
      uprv_strcpy(cfile,tmp);
      uprv_strcpy(cfile+uprv_strlen(cfile)-uprv_strlen(objSuffix), ".c" ); /* replace .o with .c */
      
      /* Make up parents.. */
      parentPath = uprv_malloc(1+uprv_strlen(baseName) + uprv_strlen("$(SRCDIR)/"));
      sprintf(parentPath, "$(SRCDIR)/%s", baseName);
      parents = pkg_appendToList(parents, NULL, parentPath);
      
      /* make up commands.. */
        /* search for tree.. */
        if((tchar=uprv_strchr(baseName, '/'))) {
          tree[0]='_';
          strncpy(tree+1,baseName,tchar-baseName);
          tree[tchar-baseName+1]=0;
        } else {
          tree[0] = 0;
        }
#ifdef OS400
        sprintf(stanza, "$(INVOKE) $(GENCCODE) -n $(CNAME)%s -d $(TEMP_DIR) $(SRCDIR)/%s", tree, infiles->str);
#else
        sprintf(stanza, "$(INVOKE) $(GENCCODE) -n $(CNAME)%s -d $(TEMP_DIR) $<", tree);
#endif
      
      if(uprv_strchr(baseName, '/')) {
        /* append actual file - ex:   coll_en_res  otherwise the tree name will be lost */
        strcat(stanza, " -f ");
        strncat(stanza, tmp, (strlen(tmp)-strlen(objSuffix)));
      }
      
      commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));
      
#ifdef OS400
      /* This builds the file into one .c file */
      sprintf(stanza, "@cat $(TEMP_PATH)%s >> $(TEMP_PATH)/$(NAME)all.c", cfile);
      commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));

      sprintf(stanza, "@$(RMV) $(TEMP_DIR)/%s", cfile);
      commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));
        
      T_FileStream_write(makefile, "\t", 1);
      pkg_writeCharList(makefile, commands, "\n\t",0);
      T_FileStream_write(makefile, "\n\n", 2);
#else
      if(genFileOffset > 0) {    /* for AS/400 */
        sprintf(stanza, "@mv $(TEMP_PATH)%s $(TEMP_PATH)%s", cfile, cfile+genFileOffset);
        commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));
      }
      
      sprintf(stanza, "@$(COMPILE.c) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $(TEMP_DIR)/%s", cfile+genFileOffset); /* for AS/400 */
      commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));
      
      sprintf(stanza, "@$(RMV) $(TEMP_DIR)/%s", cfile+genFileOffset);
      commands = pkg_appendToList(commands, NULL, uprv_strdup(stanza));
      
      sprintf(stanza, "$(TEMP_PATH)%s", tmp+genFileOffset); /* for AS/400 */
      pkg_mak_writeStanza(makefile, o, stanza, parents, commands);
#endif
      
      pkg_deleteList(parents);
      pkg_deleteList(commands);
      parents = NULL;
      commands = NULL;
    }   
}

#endif  /* #ifdef U_MAKE_IS_NMAKE */

void
pkg_mak_writeAssemblyHeader(FileStream *f, const UPKGOptions *o)
{
    T_FileStream_writeLine(f, "\n");
    T_FileStream_writeLine(f, "ifneq ($(GENCCODE_ASSEMBLY),)\n");
    T_FileStream_writeLine(f, "\n");
    T_FileStream_writeLine(f, "BASE_OBJECTS=$(NAME)_dat.o\n");
    T_FileStream_writeLine(f, "\n");
    T_FileStream_writeLine(f, "$(TEMP_DIR)/$(NAME).dat: $(CMNLIST) $(DATAFILEPATHS)\n");
    T_FileStream_writeLine(f, "\t$(INVOKE) $(ICUPKG) -t$(ICUDATA_CHAR) -c -s $(SRCDIR) -a $(CMNLIST) new $(TEMP_DIR)/$(CNAME).dat\n");
    T_FileStream_writeLine(f, "\n");
    T_FileStream_writeLine(f, "$(TEMP_DIR)/$(NAME)_dat.o : $(TEMP_DIR)/$(NAME).dat\n");
    T_FileStream_writeLine(f, "\t$(INVOKE) $(GENCCODE) $(GENCCODE_ASSEMBLY) -n $(NAME) -e $(ENTRYPOINT) -d $(TEMP_DIR) $<\n");
    T_FileStream_writeLine(f, "\t$(COMPILE.c) $(DYNAMICCPPFLAGS) $(DYNAMICCXXFLAGS) -o $@ $(TEMP_DIR)/$(NAME)_dat"ASM_SUFFIX"\n");
    T_FileStream_writeLine(f, "\t$(RMV) $(TEMP_DIR)/$(NAME)_dat"ASM_SUFFIX"\n");
    T_FileStream_writeLine(f, "\n");
    T_FileStream_writeLine(f, "else\n");
    T_FileStream_writeLine(f, "\n");
}

void
pkg_mak_writeAssemblyFooter(FileStream *f, const UPKGOptions *o)
{
    T_FileStream_writeLine(f, "\nendif\n");
}

