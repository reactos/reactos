/* stdio.h */

#ifndef __MSVCRT_INTERNAL_STDIO_H
#define __MSVCRT_INTERNAL_STDIO_H

int __vfscanf (FILE *s, const char *format, va_list argptr);
int __vscanf (const char *format, va_list arg);
int __vsscanf (const char *s,const char *format,va_list arg);

#endif /* __MSVCRT_INTERNAL_STDIO_H */

/* EOF */