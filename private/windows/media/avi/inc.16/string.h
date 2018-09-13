/***
*string.h - declarations for string manipulation functions
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file contains the function declarations for the string
*   manipulation functions.
*   [ANSI/System V]
*
****/

#ifndef _INC_STRING

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#define __near      _near
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif 

/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else 
#define NULL    ((void *)0)
#endif 
#endif 

/* function prototypes */

void * __cdecl _memccpy(void *, const void *,
    int, unsigned int);
void * __cdecl memchr(const void *, int, size_t);
int __cdecl memcmp(const void *, const void *,
    size_t);
int __cdecl _memicmp(const void *, const void *,
    unsigned int);
void * __cdecl memcpy(void *, const void *,
    size_t);
void * __cdecl memmove(void *, const void *,
    size_t);
void * __cdecl memset(void *, int, size_t);
void __cdecl _movedata(unsigned int, unsigned int, unsigned int,
    unsigned int, unsigned int);
char * __cdecl strcat(char *, const char *);
char * __cdecl strchr(const char *, int);
int __cdecl strcmp(const char *, const char *);
int __cdecl _strcmpi(const char *, const char *);
int __cdecl strcoll(const char *, const char *);
int __cdecl _stricmp(const char *, const char *);
char * __cdecl strcpy(char *, const char *);
size_t __cdecl strcspn(const char *, const char *);
char * __cdecl _strdup(const char *);
char * __cdecl _strerror(const char *);
char * __cdecl strerror(int);
size_t __cdecl strlen(const char *);
char * __cdecl _strlwr(char *);
char * __cdecl strncat(char *, const char *,
    size_t);
int __cdecl strncmp(const char *, const char *,
    size_t);
int __cdecl _strnicmp(const char *, const char *,
    size_t);
char * __cdecl strncpy(char *, const char *,
    size_t);
char * __cdecl _strnset(char *, int, size_t);
char * __cdecl strpbrk(const char *,
    const char *);
char * __cdecl strrchr(const char *, int);
char * __cdecl _strrev(char *);
char * __cdecl _strset(char *, int);
size_t __cdecl strspn(const char *, const char *);
char * __cdecl strstr(const char *,
    const char *);
char * __cdecl strtok(char *, const char *);
char * __cdecl _strupr(char *);
size_t __cdecl strxfrm (char *, const char *,
    size_t);


/* model independent function prototypes */

void __far * __far __cdecl _fmemccpy(void __far *, const void __far *,
    int, unsigned int);
void __far * __far __cdecl _fmemchr(const void __far *, int, size_t);
int __far __cdecl _fmemcmp(const void __far *, const void __far *,
    size_t);
void __far * __far __cdecl _fmemcpy(void __far *, const void __far *,
    size_t);
int __far __cdecl _fmemicmp(const void __far *, const void __far *,
    unsigned int);
void __far * __far __cdecl _fmemmove(void __far *, const void __far *,
    size_t);
void __far * __far __cdecl _fmemset(void __far *, int, size_t);
char __far * __far __cdecl _fstrcat(char __far *, const char __far *);
char __far * __far __cdecl _fstrchr(const char __far *, int);
int __far __cdecl _fstrcmp(const char __far *, const char __far *);
int __far __cdecl _fstricmp(const char __far *, const char __far *);
char __far * __far __cdecl _fstrcpy(char __far *, const char __far *);
size_t __far __cdecl _fstrcspn(const char __far *, const char __far *);
char __far * __far __cdecl _fstrdup(const char __far *);
char __near * __far __cdecl _nstrdup(const char __far *);
size_t __far __cdecl _fstrlen(const char __far *);
char __far * __far __cdecl _fstrlwr(char __far *);
char __far * __far __cdecl _fstrncat(char __far *, const char __far *,
    size_t);
int __far __cdecl _fstrncmp(const char __far *, const char __far *,
    size_t);
int __far __cdecl _fstrnicmp(const char __far *, const char __far *,
    size_t);
char __far * __far __cdecl _fstrncpy(char __far *, const char __far *,
    size_t);
char __far * __far __cdecl _fstrnset(char __far *, int, size_t);
char __far * __far __cdecl _fstrpbrk(const char __far *,
    const char __far *);
char __far * __far __cdecl _fstrrchr(const char __far *, int);
char __far * __far __cdecl _fstrrev(char __far *);
char __far * __far __cdecl _fstrset(char __far *, int);
size_t __far __cdecl _fstrspn(const char __far *, const char __far *);
char __far * __far __cdecl _fstrstr(const char __far *,
    const char __far *);
char __far * __far __cdecl _fstrtok(char __far *, const char __far *);
char __far * __far __cdecl _fstrupr(char __far *);


#ifndef __STDC__
/* Non-ANSI names for compatibility */
void * __cdecl memccpy(void *, const void *,
    int, unsigned int);
int __cdecl memicmp(const void *, const void *,
    unsigned int);
void __cdecl movedata(unsigned int, unsigned int, unsigned int,
    unsigned int, unsigned int);
int __cdecl strcmpi(const char *, const char *);
int __cdecl stricmp(const char *, const char *);
char * __cdecl strdup(const char *);
char * __cdecl strlwr(char *);
int __cdecl strnicmp(const char *, const char *,
    size_t);
char * __cdecl strnset(char *, int, size_t);
char * __cdecl strrev(char *);
char * __cdecl strset(char *, int);
char * __cdecl strupr(char *);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_STRING
#endif 
