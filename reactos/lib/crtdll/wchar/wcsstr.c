wchar_t *wcsstr(const wchar_t *s,const wchar_t *b)
{
	wchar_t *x;
	wchar_t *y;
	wchar_t *c;
	x=(wchar_t *)s;
	while (*x) {
		if (*x==*b) {
			y=x;
			c=(wchar_t *)b;
			while (*y && *c && *y==*c) { 
				c++;
				y++; 
			}
			if (!*c)
				return x;
		}
		x++;
	}
	return NULL;
}
