#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
void _tsplitpath(const _TCHAR* path, _TCHAR* drive, _TCHAR* dir, _TCHAR* fname, _TCHAR* ext)
{
  _TCHAR* tmp_drive = NULL;
  _TCHAR* tmp_dir = NULL;
  _TCHAR* tmp_ext = NULL;

  tmp_drive = (_TCHAR*)_tcschr(path,':');
  if (drive)
    {
      if (tmp_drive)
        {
          _tcsncpy(drive,tmp_drive-1,2);
          *(drive+2) = 0;
        }
      else
        {
          *drive = 0;
        }
    }
  if (!tmp_drive)
    {
      tmp_drive = (_TCHAR*)path - 1;
    }

  tmp_dir = (_TCHAR*)_tcsrchr(path,'\\');
  if (dir)
    {
      if (tmp_dir)
        {
          _tcsncpy(dir,tmp_drive+1,tmp_dir-tmp_drive);
          *(dir+(tmp_dir-tmp_drive)) = 0;
        }
      else
        {
          *dir =0;
        }
    }

  /* If the dot is before the last dir separator, it's part
   * of a directory name, not the start of the extension */
  if (!tmp_ext || tmp_ext < tmp_dir)
    {
      tmp_ext = (_TCHAR*)path+_tcslen(path);
    }
  if (ext)
    {
      _tcscpy(ext,tmp_ext);
    }

  if (fname)
    {
      if (tmp_dir)
        {
          _tcsncpy(fname,tmp_dir+1,tmp_ext-tmp_dir-1);
          *(fname+(tmp_ext-tmp_dir-1)) = 0;
        }
      else
        {
          _tcsncpy(fname,tmp_drive+1,tmp_ext-tmp_drive-1);
          *(fname+(tmp_ext-path))=0;
        }
    }
}
