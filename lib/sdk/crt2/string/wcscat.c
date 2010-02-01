_NOINTRINSIC(wcscat)

wchar_t * _CDECL wcscat(wchar_t * dst, const wchar_t * src)
{
	wchar_t *dst1 = dst;

	while(*dst)
		dst++;
	while(*dst++ = *src++);
	return(dst1);
}
