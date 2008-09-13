#define _USE_STAT64 0
#include "stat64.c"


//only version needed
//int CDECL _fstati64(int fd, struct _stati64* buf)
//{
//  int ret;
//  struct __stat64 buf64;
//
//  ret = _fstat64(fd, &buf64);
//  if (!ret)
//    stat64_to_stati64(&buf64, buf);
//  return ret;
//}
