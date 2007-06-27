/*
 *  REN.C - rename internal command.
 *
 *
 *  History:
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Added support for quoted long file names with spaces.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    17-Oct-2001 (Eric Kohl)
 *        Implemented basic rename code.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_RENAME

enum
{
  REN_ATTRIBUTES = 0x001,   /* /A : not implemented */
  REN_ERROR      = 0x002,   /* /E */
  REN_NOTHING    = 0x004,   /* /N */
  REN_PROMPT     = 0x008,   /* /P : not implemented */
  REN_QUIET      = 0x010,   /* /Q */
  REN_SUBDIR     = 0x020,   /* /S */
  REN_TOTAL      = 0x040,   /* /T */
};


/*
 *  file rename internal command.
 *
 */
INT cmd_rename (LPTSTR cmd, LPTSTR param)
{
  TCHAR szMsg[RC_STRING_MAX_SIZE];
  LPTSTR *arg = NULL;
  INT args = 0;
  INT nEvalArgs = 0; /* nunber of evaluated arguments */
  DWORD dwFlags = 0;
  DWORD dwFiles = 0; /* number of renamedd files */
  INT i;
  LPTSTR srcPattern = NULL;
  LPTSTR dstPattern = NULL;
  TCHAR dstFile[MAX_PATH];
  BOOL bDstWildcard = FALSE;

  LPTSTR p,q,r;

  HANDLE hFile;
  WIN32_FIND_DATA f;

  if (!_tcsncmp(param, _T("/?"), 2))
  {
    ConOutResPaging(TRUE,STRING_REN_HELP1);
    return 0;
  }

  nErrorLevel = 0;

  /* split the argument list */
  arg = split(param, &args, FALSE);

  if (args < 2)
    {
      if (!(dwFlags & REN_ERROR))
	error_req_param_missing();
      freep(arg);
      return 1;
    }

  /* read options */
  for (i = 0; i < args; i++)
    {
      if (*arg[i] == _T('/'))
	{
	  if (_tcslen(arg[i]) >= 2)
	    {
	      switch (_totupper(arg[i][1]))
		{
		  case _T('E'):
		    dwFlags |= REN_ERROR;
		    break;

		  case _T('N'):
		    dwFlags |= REN_NOTHING;
		    break;

		  case _T('P'):
		    dwFlags |= REN_PROMPT;
		    break;

		  case _T('Q'):
		    dwFlags |= REN_QUIET;
		    break;

		  case _T('S'):
		    dwFlags |= REN_SUBDIR;
		    break;

		  case _T('T'):
		    dwFlags |= REN_TOTAL;
		    break;
		}
	    }
	  nEvalArgs++;
	}
    }

  /* keep quiet within batch files */
  if (bc != NULL)
    dwFlags |= REN_QUIET;

  /* there are only options on the command line --> error!!! */
  if (args < nEvalArgs + 2)
    {
      if (!(dwFlags & REN_ERROR))
	error_req_param_missing();
      freep(arg);
      return 1;
    }

  /* get destination pattern */
  for (i = 0; i < args; i++)
    {
      if (*arg[i] == _T('/'))
	continue;
      dstPattern = arg[i];
    }

  if (_tcschr(dstPattern, _T('*')) || _tcschr(dstPattern, _T('?')))
    bDstWildcard = TRUE;

  /* enumerate source patterns */
  for (i = 0; i < args; i++)
    {
      if (*arg[i] == _T('/') || arg[i] == dstPattern)
	continue;

      srcPattern = arg[i];

#ifdef _DEBUG
      ConErrPrintf(_T("\n\nSourcePattern: %s\n"), srcPattern);
      ConErrPrintf(_T("DestinationPattern: %s\n"), dstPattern);
#endif

      hFile = FindFirstFile(srcPattern, &f);
      if (hFile == INVALID_HANDLE_VALUE)
	{
	  if (!(dwFlags & REN_ERROR))
	    error_file_not_found();
	  continue;
	}

      do
	{
	  /* ignore "." and ".." */
	  if (!_tcscmp (f.cFileName, _T(".")) ||
	      !_tcscmp (f.cFileName, _T("..")))
	    continue;

	  /* do not rename hidden or system files */
	  if (f.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
	    continue;

	  /* do not rename directories when the destination pattern contains
	   * wildcards, unless option /S is used */
	  if ((f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	      && bDstWildcard
	      && !(dwFlags & REN_SUBDIR))
	    continue;

#ifdef _DEBUG
	  ConErrPrintf(_T("Found source name: %s\n"), f.cFileName);
#endif

	  /* build destination file name */
	  p = f.cFileName;
	  q = dstPattern;
	  r = dstFile;
	  while(*q != 0)
	    {
	      if (*q == '*')
		{
		  q++;
		  while (*p != 0 && *p != *q)
		    {
		      *r = *p;
		      p++;
		      r++;
		    }
		}
	      else if (*q == '?')
		{
		  q++;
		  if (*p != 0)
		    {
		      *r = *p;
		      p++;
		      r++;
		    }
		}
	      else
		{
		  *r = *q;
		  if (*p != 0)
		    p++;
		  q++;
		  r++;
		}
	    }
	  *r = 0;

#ifdef _DEBUG
	  ConErrPrintf(_T("DestinationFile: %s\n"), dstFile);
#endif

	  if (!(dwFlags & REN_QUIET) && !(dwFlags & REN_TOTAL))
	    ConOutPrintf(_T("%s -> %s\n"), f.cFileName, dstFile);

	  /* rename the file */
	  if (!(dwFlags & REN_NOTHING))
	    {
	      if (MoveFile(f.cFileName, dstFile))
		{
		  dwFiles++;
		}
	      else
		{
		  if (!(dwFlags & REN_ERROR))
		  {
		    LoadString(CMD_ModuleHandle, STRING_REN_ERROR1, szMsg, RC_STRING_MAX_SIZE);
		    ConErrPrintf(szMsg, GetLastError());
		  }
		}
	    }
	}
      while (FindNextFile(hFile, &f));
      FindClose(hFile);
    }

  if (!(dwFlags & REN_QUIET))
  {
    if (dwFiles == 1)
      LoadString( CMD_ModuleHandle, STRING_REN_HELP2, szMsg, RC_STRING_MAX_SIZE);
    else
      LoadString( CMD_ModuleHandle, STRING_REN_HELP3, szMsg, RC_STRING_MAX_SIZE);
    ConOutPrintf(szMsg,dwFiles);
  }

  freep(arg);

  return 0;
}

#endif

/* EOF */
