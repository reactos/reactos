/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_h_
#define __dj_include_h_

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE



struct ftime {
  unsigned ft_tsec:5;	/* 0-29, double to get real seconds */
  unsigned ft_min:6;	/* 0-59 */
  unsigned ft_hour:5;	/* 0-23 */
  unsigned ft_day:5;	/* 1-31 */
  unsigned ft_month:4;	/* 1-12 */
  unsigned ft_year:7;	/* since 1980 */
};

struct date {
  short da_year;
  char  da_day;
  char  da_mon;
};

struct time {
  unsigned char ti_min;
  unsigned char ti_hour;
  unsigned char ti_hund;
  unsigned char ti_sec;
};

struct dfree {
  unsigned df_avail;
  unsigned df_total;
  unsigned df_bsec;
  unsigned df_sclus;
};

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned short   _osmajor, _osminor;
extern const    char  * _os_flavor;

unsigned short _get_version(int);




int getftime(int handle, struct ftime *ftimep);
int setftime(int handle, struct ftime *ftimep);

int getcbrk(void);
int setcbrk(int new_value);

void getdate(struct date *);
void gettime(struct time *);
void setdate(struct date *);
void settime(struct time *);

void getdfree(unsigned char drive, struct dfree *ptr);

void delay(unsigned msec);
/* int _get_default_drive(void);
void _fixpath(const char *, char *); */


/*
 *  For compatibility with other DOS C compilers.
 */

#define _A_NORMAL   0x00    /* Normal file - No read/write restrictions */
#define _A_RDONLY   0x01    /* Read only file */
#define _A_HIDDEN   0x02    /* Hidden file */
#define _A_SYSTEM   0x04    /* System file */
#define _A_VOLID    0x08    /* Volume ID file */
#define _A_SUBDIR   0x10    /* Subdirectory */
#define _A_ARCH     0x20    /* Archive file */

#define _enable   enable
#define _disable  disable

struct date_t {
  unsigned char  day;       /* 1-31 */
  unsigned char  month;     /* 1-12 */
  unsigned short year;      /* 1980-2099 */
  unsigned char  dayofweek; /* 0-6, 0=Sunday */
};
#define dosdate_t date_t

struct time_t {
  unsigned char hour;     /* 0-23 */
  unsigned char minute;   /* 0-59 */
  unsigned char second;   /* 0-59 */
  unsigned char hsecond;  /* 0-99 */
};
#define dostime_t time_t



#define finddata_t _finddata_t


#define diskfree_t _diskfree_t

struct _DOSERROR {
  int  exterror;
  #ifdef __cplusplus
  char errclass;
  #else
  char class;
  #endif
  char action;
  char locus;
};
#define DOSERROR _DOSERROR




void           _getdate(struct date_t *_date);
unsigned int   _setdate(struct date_t *_date);
void           _gettime(struct time_t *_time);
unsigned int   _settime(struct time_t *_time);

unsigned int   _getftime(int _handle, unsigned int *_p_date, unsigned int *_p_time);
unsigned int   _setftime(int _handle, unsigned int _date, unsigned int _time);
unsigned int   _getfileattr(const char *_filename, unsigned int *_p_attr);
unsigned int   _setfileattr(const char *_filename, unsigned int _attr);


void           _setdrive(unsigned int _drive, unsigned int *_p_drives);


int            exterr(struct _DOSERROR *_p_error);
#define dosexterr(_ep) exterr(_ep)

#include <direct.h>

#define int386(_i, _ir, _or)         int86(_i, _ir, _or)
#define int386x(_i, _ir, _or, _sr)   int86x(_i, _ir, _or, _sr)

#ifdef __cplusplus
}
#endif

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#endif /* !__dj_include_h_ */
