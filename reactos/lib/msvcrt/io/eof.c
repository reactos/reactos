//#include <windows.h>
#include <msvcrt/io.h>

int _eof(int _fd)
{
  __int64 cur_pos = _lseeki64(_fd, 0, SEEK_CUR);
  __int64 end_pos = _lseeki64(_fd, 0, SEEK_END);
  if ( cur_pos == -1 || end_pos == -1)
    return -1;

  if (cur_pos == end_pos)
    return 1;

  return 0;
}
