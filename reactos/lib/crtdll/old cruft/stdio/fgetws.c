/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

//wchar_t* fgetws(wchar_t* wcaBuffer, int nBufferSize, FILE* fileRead);
//char* fgets(char *s, int n, FILE *f);
//int _getw(FILE *stream);
/*
// Read a word (int) from STREAM.
int _getw(FILE *stream)
{
  int w;

  // Is there a better way?
  if (fread( &w, sizeof(w), 1, stream) != 1)
    return(EOF);
  return(w);
}

//getc can be a macro
#undef getc

int getc(FILE *fp)
{
    int c = -1;
// check for invalid stream
	if ( !__validfp (fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
// check for read access on stream
	if ( !OPEN4READING(fp) ) {
		__set_errno(EINVAL);
		return -1;
	}
	if(fp->_cnt > 0) {
		fp->_cnt--;
		c =  (int)*fp->_ptr++;
	} 
	else {
		c =  _filbuf(fp);
	}
	return c;
}
 */

wchar_t* fgetws(wchar_t* s, int n, FILE* f)
{
  int c = 0;
  wchar_t* cs;

  cs = s;
  while (--n > 0 && (c = _getw(f)) != EOF) {
    *cs++ = c;
    if (c == L'\n')
      break;
  }
  if (c == EOF && cs == s) {
    return NULL;
  }
  *cs++ = L'\0';
  return s;
}
