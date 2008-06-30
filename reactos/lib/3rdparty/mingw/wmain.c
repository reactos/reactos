/*
 * main.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Extra startup code for applications which do not have a main function
 * of their own (but do have a WinMain). Generally these are GUI
 * applications, but they don't *have* to be.
 *
 */

#include <stdlib.h>
#include <process.h>
#include <windows.h>

#define ISSPACE(a)	(a == ' ' || a == '\t')

extern void __main();
extern int PASCAL wWinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                            LPWSTR szCmdLine, int nShow);

int
wmain (int argc, const wchar_t *argv[], const wchar_t *environ[])
{
  wchar_t *szCmd;
  STARTUPINFOW startinfo;
  int nRet;

#ifdef __GNUC__
  /* C++ initialization. (gcc inserts this call automatically for
   * a function called "main", but not for "wmain") */
  __main();
#endif

  /* Get the command line passed to the process. */
  szCmd = GetCommandLineW ();
  GetStartupInfoW (&startinfo);

  /* Strip off the name of the application and any leading
   * whitespace. */
  if (szCmd)
    {
      while (ISSPACE (*szCmd))
	{
	  szCmd++;
	}

      /* On my system I always get the app name enclosed
       * in quotes... */
      if (*szCmd == '\"')
	{
	  do
	    {
	      szCmd++;
	    }
	  while (*szCmd != '\"' && *szCmd != '\0');

	  if (*szCmd == '\"')
	    {
	      szCmd++;
	    }
	}
      else
	{
	  /* If no quotes then assume first token is program
	   * name. */
	  while (!ISSPACE (*szCmd) && *szCmd != '\0')
	    {
	      szCmd++;
	    }
	}

      while (ISSPACE (*szCmd))
	{
	  szCmd++;
	}
    }

  nRet = wWinMain (GetModuleHandle (NULL), NULL, szCmd,
		  (startinfo.dwFlags & STARTF_USESHOWWINDOW) ?
		  startinfo.wShowWindow : SW_SHOWDEFAULT);

  return nRet;
}

