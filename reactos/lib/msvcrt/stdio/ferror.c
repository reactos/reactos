/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

#ifdef ferror
#undef ferror
int ferror(FILE *stream);
#endif

//int *_errno(void)
//{
//  return(&GetThreadData()->terrno);
//}
//int __set_errno(int error)
//{
//  PTHREADDATA ThreadData;
//  ThreadData = GetThreadData();
//  if (ThreadData)
//    ThreadData->terrno = error;
//  return(error);
//}
int *_errno(void);

int ferror(FILE *stream)
{
//  return stream->_flag & _IOERR;
  return *_errno();
}
