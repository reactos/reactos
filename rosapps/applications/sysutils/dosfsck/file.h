/* file.h  -  Additional file attributes */

/* Written 1993 by Werner Almesberger */


#ifndef _FILE_H
#define _FILE_H

typedef enum { fdt_none,fdt_drop,fdt_undelete } FD_TYPE;

typedef struct _fptr {
    char name[MSDOS_NAME];
    FD_TYPE type;
    struct _fptr *first; /* first entry */
    struct _fptr *next; /* next file in directory */
} FDSC;


extern FDSC *fp_root;


char *file_name(unsigned char *fixed);

/* Returns a pointer to a pretty-printed representation of a fixed MS-DOS file
   name. */

int file_cvt(unsigned char *name,unsigned char *fixed);

/* Converts a pretty-printed file name to the fixed MS-DOS format. Returns a
   non-zero integer on success, zero on failure. */

void file_add(char *path,FD_TYPE type);

/* Define special attributes for a path. TYPE can be either FDT_DROP or
   FDT_UNDELETE. */

FDSC **file_cd(FDSC **curr,char *fixed);

/* Returns a pointer to the directory descriptor of the subdirectory FIXED of
   CURR, or NULL if no such subdirectory exists. */

FD_TYPE file_type(FDSC **curr,char *fixed);

/* Returns the attribute of the file FIXED in directory CURR or FDT_NONE if no
   such file exists or if CURR is NULL. */

void file_modify(FDSC **curr,char *fixed);

/* Performs the necessary operation on the entry of CURR that is named FIXED. */

void file_unused(void);

/* Displays warnings for all unused file attributes. */

#endif
