size_t _mbslen(const unsigned char *str)
{
	int i = 0;
	unsigned char *s;

	if (str == 0)
		return 0;
		
    	for (s = (unsigned char *)str; *s; s+=mbclen(*s),i++);
	return i;
}