#include <msvcrti.h>


long wcstol(const wchar_t *cp,wchar_t **endp,int base)
{
	long result = 0,value;
	int sign = 1;

	if ( *cp == L'-' ) {
		sign = -1;
		cp++;
	}

	if (!base) {
		base = 10;
		if (*cp == L'0') {
			base = 8;
			cp++;
			if ((*cp == L'x') && iswxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (iswxdigit(*cp) && (value = iswdigit(*cp) ? *cp-L'0' : (iswlower(*cp)
	    ? towupper(*cp) : *cp)-L'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (wchar_t *)cp;
	return result * sign;
}
