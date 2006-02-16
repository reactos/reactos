#include <stdio.h>
#include <time.h>
#include <winsock.h>
#include "fake.h"
#include "prototypes.h"

#define MAX_ASCII 100

int checkRecv(SOCKET s);

int checkRecv(SOCKET s)
{
   int testVal;
   fd_set sSet;
   struct timeval timeout;
   timeout.tv_sec = 60;

   FD_ZERO(&sSet);

   FD_SET(s, &sSet);

   testVal = select(0, &sSet, NULL, NULL, &timeout);

   if (testVal == SOCKET_ERROR)
      fprintf(stderr, "Socket Error");

   return testVal;
}

void blkfree(char **av0)
{
	register char **av = av0;

	while (*av)
		free(*av++);
}

char **glob(register char *v)
{
   return NULL;
}

int sleep(int time)
{
   return time;
}

int herror(char *string)
{
   return 0;
}

#if 0
int gettimeofday(struct timeval *timenow,
				 struct timezone *zone)
{
	time_t t;

	t = clock();

	timenow->tv_usec = t;
	timenow->tv_sec = t / CLK_TCK;

	return 0;
}

int fgetcSocket(int s)
{
   int c;
   char buffer[10];

//   checkRecv(s);

   c = recv(s, buffer, 1, 0);

#ifdef DEBUG_IN
   printf("%c", buffer[0]);
#endif

   if (c == INVALID_SOCKET)
      return c;

   if (c == 0)
      return EOF;

   return buffer[0];
}

#else

int fgetcSocket(int s)
{
   static int index = 0;
   static int total = 0;
   static char buffer[4096];

   if (index == total)
     {
       index = 0;
       total = recv(s, buffer, sizeof(buffer), 0);

       if (total == SOCKET_ERROR)
	 {
	   total = 0;
	   return ERROR;
	 }

       if (total == 0)
	 return EOF;
     }
   return buffer[index++];
}

#endif

const char *fprintfSocket(int s, const char *format, ...)
{
   va_list argptr;
   char buffer[10009];

   va_start(argptr, format);
   vsprintf(buffer, format, argptr);
   va_end(argptr);

   send(s, buffer, strlen(buffer), 0);

   return NULL;
}

const char *fputsSocket(const char *format, int s)
{
   send(s, format, strlen(format), 0);

   return NULL;
}

int fputcSocket(int s, char putChar)
{
   char buffer[2];

   buffer[0] = putChar;
   buffer[1] = '\0';

   if(SOCKET_ERROR==send(s, buffer, 1, 0)) {
	   int iret=WSAGetLastError ();
	   fprintf(stdout,"fputcSocket: %d\n",iret);
	   return 0;
   }
   else {
	return putChar;
   }
}
int fputSocket(int s, char *buffer, int len)
{
	int iret;
	while(len) {
		if(SOCKET_ERROR==(iret=send(s, buffer, len, 0)))
		{
			iret=WSAGetLastError ();
			fprintf(stdout,"fputcSocket: %d\n",iret);
			return 0;
		}
		else {
			return len-=iret;
		}
	}
	return 0;
}

char *fgetsSocket(int s, char *string)
{
   char buffer[2] = {0};
   int i, count;

   for (i = 0, count = 1; count != 0 && buffer[0] != '\n'; i++)
   {
      checkRecv(s);

      count = recv(s, buffer, 1, 0);

      if (count == SOCKET_ERROR)
      {
	 printf("Error in fgetssocket");
	 return NULL;
      }

      if (count == 1)
      {
	 string[i] = buffer[0];

	 if (i == MAX_ASCII - 3)
	 {
	    count = 0;
	    string[++i] = '\n';
	    string[++i] = '\0';
	 }
      }
      else
      {
	 if (i == 0)
	    return NULL;
	 else
	 {
	    string[i] = '\n';
	    string[i + 1] = '\0'; // This is risky
	    return string;
	 }

      }

   }
   string[i] = '\0';

#ifdef DEBUG_IN
   printf("%s", string);
#endif
   return string;
}


#if 0
char *getpass(const char *prompt)
{
   static char string[64];

   printf("%s", prompt);

   gets(string);

   return string;
}
#endif
char *getpass (const char * prompt)
{
  static char input[256];
  HANDLE in;
  HANDLE err;
  DWORD    count;

  in = GetStdHandle (STD_INPUT_HANDLE);
  err = GetStdHandle (STD_ERROR_HANDLE);

  if (in == INVALID_HANDLE_VALUE || err == INVALID_HANDLE_VALUE)
    return NULL;

  if (WriteFile (err, prompt, strlen (prompt), &count, NULL))
    {
      int istty = (GetFileType (in) == FILE_TYPE_CHAR);
      DWORD old_flags;
      int rc;

      if (istty)
	{
	  if (GetConsoleMode (in, &old_flags))
	    SetConsoleMode (in, ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
	  else
	    istty = 0;
	}
      /* Need to read line one byte at time to avoid blocking, if not a
         tty, so always do it this way.  */
      count = 0;
      while (1)
	{
	  DWORD  dummy;
	  char   one_char;

	  rc = ReadFile (in, &one_char, 1, &dummy, NULL);
	  if (rc == 0)
	    break;
	  if (one_char == '\r')
	    {
	      /* CR is always followed by LF if reading from tty.  */
	      if (istty)
		continue;
	      else
		break;
	    }
	  if (one_char == '\n')
	    break;
	  /* Silently truncate password string if overly long.  */
	  if (count < sizeof (input) - 1)
	    input[count++] = one_char;
	}
      input[count] = '\0';

      WriteFile (err, "\r\n", 2, &count, NULL);
      if (istty)
	SetConsoleMode (in, old_flags);
      if (rc)
	return input;
    }

  return NULL;
}

#if 0
// Stubbed out here. Should be changed in Source code...
int access(const char *filename, int accessmethod)
{
   return 0;
}
#endif

#ifndef __GNUC__
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
