/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/errno.h>


char __syserr00[] = "No Error";
char __syserr01[] = "Input to function out of range (EDOM)";
char __syserr02[] = "Output of function out of range (ERANGE)";
char __syserr03[] = "Argument list too long (E2BIG)";
char __syserr04[] = "Permission denied (EACCES)";
char __syserr05[] = "Resource temporarily unavailable (EAGAIN)";
char __syserr06[] = "Bad file descriptor (EBADF)";
char __syserr07[] = "Resource busy (EBUSY)";
char __syserr08[] = "No child processes (ECHILD)";
char __syserr09[] = "Resource deadlock avoided (EDEADLK)";
char __syserr10[] = "File exists (EEXIST)";
char __syserr11[] = "Bad address (EFAULT)";
char __syserr12[] = "File too large (EFBIG)";
char __syserr13[] = "Interrupted system call (EINTR)";
char __syserr14[] = "Invalid argument (EINVAL)";
char __syserr15[] = "Input or output error (EIO)";
char __syserr16[] = "Is a directory (EISDIR)";
char __syserr17[] = "Too many open files (EMFILE)";
char __syserr18[] = "Too many links (EMLINK)";
char __syserr19[] = "File name too long (ENAMETOOLONG)";
char __syserr20[] = "Too many open files in system (ENFILE)";
char __syserr21[] = "No such device (ENODEV)";
char __syserr22[] = "No such file or directory (ENOENT)";
char __syserr23[] = "Unable to execute file (ENOEXEC)";
char __syserr24[] = "No locks available (ENOLCK)";
char __syserr25[] = "Not enough memory (ENOMEM)";
char __syserr26[] = "No space left on drive (ENOSPC)";
char __syserr27[] = "Function not implemented (ENOSYS)";
char __syserr28[] = "Not a directory (ENOTDIR)";
char __syserr29[] = "Directory not empty (ENOTEMPTY)";
char __syserr30[] = "Inappropriate I/O control operation (ENOTTY)";
char __syserr31[] = "No such device or address (ENXIO)";
char __syserr32[] = "Operation not permitted (EPERM)";
char __syserr33[] = "Broken pipe (EPIPE)";
char __syserr34[] = "Read-only file system (EROFS)";
char __syserr35[] = "Invalid seek (ESPIPE)";
char __syserr36[] = "No such process (ESRCH)";
char __syserr37[] = "Improper link (EXDEV)";
char __syserr38[] = "No more files (ENMFILE)";

const char *_sys_errlist[] = {
__syserr00, __syserr01, __syserr02, __syserr03, __syserr04,
__syserr05, __syserr06, __syserr07, __syserr08, __syserr09,
__syserr10, __syserr11, __syserr12, __syserr13, __syserr14,
__syserr15, __syserr16, __syserr17, __syserr18, __syserr19,
__syserr20, __syserr21, __syserr22, __syserr23, __syserr24,
__syserr25, __syserr26, __syserr27, __syserr28, __syserr29,
__syserr30, __syserr31, __syserr32, __syserr33, __syserr34,
__syserr35, __syserr36, __syserr37, __syserr38
};

int __sys_nerr = sizeof(_sys_errlist) / sizeof(_sys_errlist[0]);

int*	_sys_nerr_dll = &__sys_nerr;

char *strerror(int errnum)
{
  static char ebuf[40];		/* 64-bit number + slop */
  char *cp;
  int v=1000000, lz=0;

  if (errnum >= 0 && errnum < __sys_nerr)
    return((char *)_sys_errlist[errnum]);

  strcpy(ebuf, "Unknown error: ");
  cp = ebuf + 15;
  if (errnum < 0)
  {
    *cp++ = '-';
    errnum = -errnum;
  }
  while (v)
  {
    int d = errnum / v;
    if (d || lz || (v == 1))
    {
      *cp++ = d+'0';
      lz = 1;
    }
    errnum %= v;
    v /= 10;
  }

  return ebuf;
}


char *_strerror(const char *s)
{
	if ( s == NULL )
		return strerror(errno);

	return strerror(atoi(s));
}
