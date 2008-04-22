
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

BOOL myCreateProcessStartGetSzie(CHAR *cmdline, LONG *size)
{
    HANDLE hChildStdinRd;
	HANDLE hChildStdinWr;
	HANDLE hChildStdoutRd;
	HANDLE hChildStdoutWr;
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
    {
        return FALSE;
    }

    if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
    {
        return FALSE;
    }

    myCreateProcess(hChildStdoutWr, hChildStdinRd,"rpcclient -c \"service enum\"");
    *size = ReadPipeSize(hChildStdoutWr, hChildStdoutRd);
	return TRUE;
}

BOOL myCreateProcessStart(CHAR *cmdline, CHAR *srvlst, LONG size)
{
    HANDLE hChildStdinRd;
	HANDLE hChildStdinWr;
	HANDLE hChildStdoutRd;
	HANDLE hChildStdoutWr;
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
    {
        return FALSE;
    }

    if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
    {
        return FALSE;
    }

    myCreateProcess(hChildStdoutWr, hChildStdinRd,"rpcclient -c \"service enum\"");

	return ReadPipe(hChildStdoutWr, hChildStdoutRd, srvlst, size);
}

BOOL myCreateProcess(HANDLE hStdoutWr, HANDLE hStdinRd, CHAR *cmdline)
{
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL status = FALSE;

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = hStdoutWr;
   siStartInfo.hStdOutput = hStdoutWr;
   siStartInfo.hStdInput = hStdinRd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
   status = CreateProcess(NULL, cmdline, NULL, NULL,
	                      TRUE, 0, NULL, NULL, &siStartInfo,  &piProcInfo);

   if (status != 0)
   {
      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
      return status;
   }
   return status;
}

LONG ReadPipeSize(HANDLE hStdoutWr, HANDLE hStdoutRd)
{
  CHAR chBuf[2];
  LONG pos=0;

  if (!CloseHandle(hStdoutWr))
  {
      return 0; /* fail */
  }

  for (;;)
  {
	  long dwRead;
      if( !ReadFile( hStdoutRd, chBuf, 1, (LPDWORD)&dwRead, NULL) || dwRead == 0)
	  {
		 break;
	  }
	  else
	  {
	    pos+=dwRead;
	  }
   }
   return pos;
}

LONG ReadPipe(HANDLE hStdoutWr, HANDLE hStdoutRd, CHAR *srvlst, LONG size)
{
  CHAR chBuf[2];
  LONG pos;

  pos=0;

  if (!CloseHandle(hStdoutWr))
  {
      return 0; /* fail */
  }

   for (;;)
   {
	  LONG dwRead;
      if( !ReadFile( hStdoutRd, chBuf, 1, (LPDWORD)&dwRead, NULL) || dwRead == 0)
	  {
		 break;
	  }
	  else
	  {
	    srvlst[pos++] = chBuf[0] ;
	  }
   }
   return 0;
}

INT row_scanner_service(CHAR *buffer, LONG* pos, LONG size,
					  CHAR *name,CHAR *save)
{
	LONG get_semi;
	LONG t;
	LONG row_size=0;
	LONG start_pos;

	start_pos = *pos;

	if (*pos>=size)
	{
		return 0;
	}


	/* get row start */
	for (t=start_pos;t<size;t++)
	{
		if (buffer[t]=='\n')
		{
			buffer[t]='\0';
			if (buffer[t-1]==0x09)
			{
			  buffer[t-1]='\0';
			}
			if (buffer[t-1]==0x0d)
			{
			  buffer[t-1]='\0';
			}
			*pos = t+1;
            row_size = t;
            break;
		}
	}

	/* get : */
    get_semi=-1;
	for (t=start_pos;t<row_size;t++)
	{
		if (buffer[t]==':')
		{
			get_semi=t;
            break;
		}
	}

	if (get_semi==-1)
	{
		return 0;
	}

	/* lock for space */
	for (t=get_semi+1;t<row_size;t++)
	{
		if (!isspace(buffer[t]))
		{
			break;
		}
	}
	if (t==0)
	{
		/* : not found next row*/
		return 0;
	}

	/* Compare now */
	if (strnicmp(name,&buffer[t],strlen(&buffer[t]))==0)
	{
		if (save != NULL)
		{
			/* lock for space */
	        for (t=start_pos;t<get_semi;t++)
	        {
		       if (!isspace(buffer[t]))
		       {
			       break;
		       }
	         }

			 memcpy(save,&buffer[t],get_semi-t);
		}
		return 1;
	}
  return 0;
}
