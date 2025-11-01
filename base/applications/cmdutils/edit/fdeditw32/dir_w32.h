#ifndef __dir_w32_h__
#define __dir_w32_h__

struct ffblk {
    char lfn_magic[6];
    short lfn_handle;
    unsigned short lfn_ctime;
    unsigned short lfn_cdate;
    unsigned short lfn_atime;
    unsigned short lfn_adate;
    char _ff_reserved[5];
    unsigned char  ff_attrib;
    unsigned short ff_ftime;
    unsigned short ff_fdate;
    unsigned long  ff_fsize;
    char ff_name[260];
/* Win32 private date */
    int __hFile;
    unsigned int __wmask;
};

#define FA_RDONLY       1
#define FA_HIDDEN       2
#define FA_SYSTEM       4
#define FA_LABEL        8
#define FA_DIREC        16
#define FA_ARCH         32

/* for fnmerge/fnsplit */
#define MAXPATH   260
#define MAXDRIVE  3
#define MAXDIR	  256
#define MAXFILE   256
#define MAXEXT	  255

#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE	  0x10

int findfirst(const char *_pathname, struct ffblk *_ffblk, int _attrib);
int findnext(struct ffblk *_ffblk);
void fnmerge (char *_path, const char *_drive, const char *_dir, const char *_name, const char *_ext);
int fnsplit (const char *_path, char *_drive, char *_dir, char *_name, char *_ext);
int getdisk(void);
int setdisk(int _drive);

#endif
