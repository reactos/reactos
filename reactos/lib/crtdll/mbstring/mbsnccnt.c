size_t _mbsnccnt(const unsigned char *str, size_t n)
{
}

size_t _mbsnbcnt(const unsigned char *str, size_t n)
{
	unsigned char *s = str;
	while(*s != 0 && n > 0)
		if (!_ismbblead(*s) )
			n--;
		s++;
	}
	
	return (size_t)(s - str);
}