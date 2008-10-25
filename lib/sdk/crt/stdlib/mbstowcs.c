#include <precomp.h>


/*
 * @implemented
 */
size_t mbstowcs (wchar_t *widechar, const char *multibyte, size_t number)
{
    int bytes;
    int n = 0;

    while (n < number) {

	if ((bytes = mbtowc (widechar, multibyte, MB_LEN_MAX)) < 0)
	    return -1;

	if (bytes == 0) {
	    *widechar = (wchar_t) '\0';
	    return n;
	}

	widechar++;
	multibyte += bytes;
	n++;
    }

    return n;
}
