// See license at end of file

/* For gcc, compile with -fno-builtin to suppress warnings */

#include "1275.h"

extern void *malloc();
extern char *get_str_prop();
extern int  get_int_prop();
extern int  get_int_prop_def();

void
abort()
{
  OFExit();
}

void
exit()
{
  OFExit();
}

VOID
sleep(ULONG delay)
{
	delay = (delay * 1000) + OFMilliseconds();
	while ((OFMilliseconds() - delay) < 0)
		;
}

/* files */

#include "stdio.h"

FILE _stdin  = { -1, 0, 0};
FILE _stdout = { -1, 0, 0};
FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;

char _homedir[128];

char *
gethomedir()
{
  return(_homedir);
}

parse_homedir(char *progname)
{
  char *p, *q, c;

  p = progname + strlen(progname);
  while (p > progname) {
    c = *--p;
    if (c == ',' || c == ':' || c == '\\') {
      ++p;
      break;
    }
  }
  for (q = _homedir; progname < p; )
    *q++ = *progname++;

  *q = '\0';
}

int
ofw_setup()
{
  static char *argv[8];
  phandle ph;
  char *argstr;
  int i;

  if ((ph = OFFinddevice("/chosen")) == -1)
    abort() ;
  stdin->id  = get_int_prop(ph, "stdin");
  stdout->id = get_int_prop(ph, "stdout");

  argv[0] = get_str_prop(ph, "bootpath", ALLOC);
  argstr  = get_str_prop(ph, "bootargs", ALLOC);

  for (i = 1; i < 8;) {
    if (*argstr == '\0')
      break;
    argv[i++] = argstr;
    while (*argstr != ' ' && *argstr != '\0')
      ++argstr;
    if (*argstr == '\0')
      break;
    *argstr++ = '\0';
  }
  parse_homedir(argv[0]);
  main(i, argv);
}

FILE *
fopen (char *name, char *mode)
{
  FILE *fp;

  fp = (FILE *)malloc(sizeof(struct _file));
  if (fp == (FILE *)NULL)
      return ((FILE *)NULL);

  if ((fp->id = OFOpen(name)) == 0)
      return ((FILE *)NULL);
  
  fp->bufc = 0;
  return(fp);
}

int
ferror(FILE *fp)
{
  return(0);	/* Implement me */
}

void
fputc(char c, FILE *fp)
{
  if (fp == stdout && c == '\n')
    fputc('\r', fp);

  fp->buf[fp->bufc++] = c;

  if ((fp->bufc == 127) || (fp == stdout && c == '\n')) {
    OFWrite(fp->id, fp->buf, fp->bufc);
    fp->bufc = 0;
  }
}

void
fflush (FILE *fp)
{
  if (fp->bufc != 0) {
    OFWrite(fp->id, fp->buf, fp->bufc);
    fp->bufc = 0;
  }
}

int
fgetc(FILE *fp)
{
  int  count;

  /* try to read from the buffer */
  if (fp->bufc != 0) {
    fp->bufc--;
    return(*fp->inbufp++);
  }

  /* read from the file */
  do {
      count = OFRead(fp->id, fp->buf, 128);
  } while (count == -2);	/* Wait until input available */

  if (count > 0)
    {
      fp->bufc = count-1;
      fp->inbufp = fp->buf;
      return(*fp->inbufp++);
    }

  /* otherwise return EOF */
  return (-1);
}

int
fclose (FILE *fp)
{
  fflush(fp);
  OFClose(fp->id);
  free(fp);
  return(0);
}

int
getchar()
{
  return(fgetc(stdin));
}

VOID
putchar(char c)
{
  fputc(c, stdout);
}

int
puts(char *s)
{
  fputs(s, stdout);
  putchar('\n');
  return(0);
}

VOID
gets(char *buf)
{
  while ((*buf = getchar()) != '\r')
    buf++;
  *buf = '\0';
}

int
fputs(char *s, FILE *f)
{
  register char c;
  while(c = *s++)
    fputc(c, f);
  return(0);
}

char *
fgets(char *buf, int n, FILE *f)
{
  char *p = buf;

  while ((n > 1) && ((*p = fgetc(f)) != '\n')) {
    p++;
    n--;
  }
  *p = '\0';
  return(buf);
}

unlink(char *filename)
{
  return(-1);
/* XXX Implement me */
}

system(char *str)
{
  OFInterpret0(str);
}

#define MAXENV 256
char *
getenv(char *str)
{
  phandle ph;
  int res;

  if ((ph = OFFinddevice("/options")) == -1)
      return(NULL);

  return (get_str_prop(ph, str, 0));
}

int
stdout_rows()
{
  phandle ph;
  int res;

  if ((ph = OFFinddevice("/chosen")) == -1)
    return(24);
  res = get_int_prop_def(ph, "stdout-#lines", 24);
  if (res < 0)
    return(24);		/* XXX should look in device node too */
  return (res);
}

int
stdout_columns()
{
  phandle ph;
  int res;

  if ((ph = OFFinddevice("/chosen")) == -1)
      return(80);
  res = get_int_prop_def(ph, "stdout-#columns", 80);
  if (res < 0)
    return(80);		/* XXX should look in device node too */
  return (res);
}

// LICENSE_BEGIN
// Copyright (c) 2006 FirmWorks
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// LICENSE_END
