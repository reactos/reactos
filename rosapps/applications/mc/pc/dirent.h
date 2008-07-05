/*
 *  direct.h    Defines the types and structures used by the directory routines
 *
 */
#ifndef _DIRENT_H_incl
#define _DIRENT_H_incl

#ifdef __cplupplus
extern "C" {
#endif

#include <sys/types.h>

#define NAME_MAX        255             /* maximum filename for HPFS or NTFS */

typedef struct dirent {
    unsigned long* d_handle;
    unsigned	d_attr;                 /* file's attribute */
    unsigned short int d_time;          /* file's time */
    unsigned short int d_date;          /* file's date */
    long        d_size;                 /* file's size */
    char        d_name[ NAME_MAX + 1 ]; /* file's name */
    unsigned short d_ino;               /* serial number (not used) */
    char        d_first;                /* flag for 1st time */
} DIR;

extern int      closedir( DIR * );
extern DIR      *opendir( const char * );
extern struct dirent *readdir( DIR * );

#ifdef __cplusplus
};
#endif

#endif /* _DIRENT_H_incl */
