unsigned int _mbctoupper(unsigned int c)
{
	if (!_ismbblead(c) )
		return toupper(c);
	
	return c;
}

unsigned char * _mbsupr(unsigned char *str)
{
       char  *y=x;

        while (*y) {
                *y=_mbctoupper(*y);
                y++;
        }
        return x;
}