// copy a wide character string to a multibyte string
size_t __cdecl wcstombs( char *dst, const wchar_t *src, size_t len )
{
	lstrcpynWtoA( dst, src, len );
	return strlen(dst);  /* FIXME: is this right? */
}