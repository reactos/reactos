/*
 * direct.h
 *
 * Functions for manipulating paths and directories (included from io.h)
 * plus functions for setting the current drive.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef _DIRECT_H_
#define _DIRECT_H_

#ifndef _WCHAR_T_
typedef unsigned short wchar_t;
#define _WCHAR_T_
#endif

#ifndef _SIZE_T_
typedef unsigned int size_t;
#define _SIZE_T_
#endif

#ifdef  __cplusplus
extern "C" {
#endif

struct _diskfree_t {
    unsigned short total_clusters;
    unsigned short avail_clusters;
    unsigned short sectors_per_cluster;
    unsigned short bytes_per_sector;
};

int _getdrive(void);
int _chdrive(int);
char* _getcwd(char*, int);

unsigned int _getdiskfree(unsigned int, struct _diskfree_t*);

int _chdir(const char*);
int _mkdir(const char*);
int _rmdir(const char*);

#define chdir  _chdir
#define getcwd _getcwd
#define mkdir  _mkdir
#define rmdir  _rmdir

char* _getdcwd(int nDrive, char* caBuffer, int nBufLen);

wchar_t* _wgetcwd(wchar_t *buffer, int maxlen);
wchar_t* _wgetdcwd(int nDrive, wchar_t* caBuffer, int nBufLen);

int _wchdir(const wchar_t* _path);
int _wmkdir(const wchar_t* _path);
int _wrmdir(const wchar_t* _path);

#ifdef  __cplusplus
}
#endif

#endif  /* Not _DIRECT_H_ */

