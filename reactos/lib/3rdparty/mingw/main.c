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

extern int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                           LPSTR szCmdLine, int nShow);

int
main (int argc, const char *argv[])
{
  char *szCmd;
  STARTUPINFO startinfo;
  int nRet;

  /* Get the command line passed to the process. */
  szCmd = GetCommandLineA ();
  GetStartupInfoA (&startinfo);

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

  nRet = WinMain (GetModuleHandle (NULL), NULL, szCmd,
		  (startinfo.dwFlags & STARTF_USESHOWWINDOW) ?
		  startinfo.wShowWindow : SW_SHOWDEFAULT);

  return nRet;
}

