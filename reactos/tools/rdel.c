/*  $Id$
 * COPYRIGHT:             See COPYING in the top level directory
 * PROGRAMMER:            Rex Jolliff (rex@lvcablemodem.com)
 * PURPOSE:               Platform independent delete command
 */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __WIN32__
# include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

void
convertPath (char * pathToConvert)
{
  while (*pathToConvert != 0)
  {
    if (*pathToConvert == '\\')
    {
      *pathToConvert = '/';
    }
    pathToConvert++;
  }
}

#if 0
void
getDirectory (const char *filename, char * directorySpec)
{
  int  lengthOfDirectory;

  if (strrchr (filename, '/') != 0)
  {
    lengthOfDirectory = strrchr (filename, '/') - filename;
    strncpy (directorySpec, filename, lengthOfDirectory);
    directorySpec [lengthOfDirectory] = '\0';
  }
  else
  {
    strcpy (directorySpec, ".");
  }
}
#endif

void
getFilename (const char *filename, char * fileSpec)
{
  if (strrchr (filename, '/') != 0)
  {
    strcpy (fileSpec, strrchr (filename, '/') + 1);
  }
  else
  {
    strcpy (fileSpec, filename);
  }
}

int
main (int argc, char* argv[])
{
  int  justPrint = 0;
  int  idx;
  int  returnCode;

  for (idx = 1; idx < argc; idx++)
  {
    convertPath (argv [idx]);

    if (justPrint)
    {
      printf ("delete %s\n", argv [idx]);
    }
    else
    {
#ifdef __WIN32__
      _chmod (argv [idx], _S_IREAD | _S_IWRITE);
#else
      chmod (argv [idx], 0666);
#endif
      returnCode = remove (argv [idx]);
      if (returnCode != 0 && errno != ENOENT)
      {
      /* Continue even if there is errors */
#if 0
        printf ("Unlink of %s failed.  Unlink returned %d.\n",
                argv [idx],
                returnCode);
        return  returnCode;
#endif
      }
    }
  }

  return  0;
}
