/* $Id: wmain.c,v 1.1 1999/05/16 07:27:35 ea Exp $
 *
 * Entry point for programs that use wmain()
 */
#include <windows.h>
#include <stdio.h>

int wmain(int argc,wchar_t *argv[]);

wchar_t *
a2w( char * a, wchar_t * w )
{
	wchar_t * ww = w;
	while (*a) *w++ = (wchar_t) *a++;
	*w = 0;
	return ww;
}

wchar_t *
fgetws(wchar_t *buf, int bufsize, FILE *file)
{
	char * abuf = GlobalAlloc(bufsize,0);
	if (!buf)return NULL;
	fgets(abuf,bufsize,file);
	a2w(abuf,buf);
	GlobalFree(abuf);
	return buf;
}

int main(int argc, char *argv[])
{
	wchar_t	** wargv;
	int	i;
	int	ec;
	
	wargv = (wchar_t **) GlobalAlloc(
				sizeof(void*) * argc,
				0
				);
	for(i=0;i<argc;++i)
	{
		wargv[i] = (wchar_t*) GlobalAlloc(
				sizeof(wchar_t) * (1+lstrlenA(argv[i])),
				0
				);
		a2w(argv[i],wargv[i]);
	}
	wargv[i] = NULL;
	ec = wmain(argc,wargv);
	for (i=0;wargv[i];++i) GlobalFree(wargv[i]);
	return ec;
}
