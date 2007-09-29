/* common bit magic */

static inline void
printbits(FILE *f, int code, int nbits)
{
	int n, b;
	for (n = nbits - 1; n >= 0; n--)
	{
		b = (code >> n) & 1;
		fprintf(f, "%c", b ? '1' : '0');
	}
}

static inline int
getbit(const unsigned char *buf, int x)
{
	return ( buf[x >> 3] >> ( 7 - (x & 7) ) ) & 1;
}

static inline void
printline(FILE *f, unsigned char *line, int w)
{
	int i;
	for (i = 0; i < w; i++)
		fprintf(f, "%c", getbit(line, i) ? '#' : '.');
	fprintf(f, "\n");
}

static inline int
getrun(const unsigned char *line, int x, int w, int c)
{
	int z;
	int b;
	
	if (x < 0)
		x = 0;

	z = x;
	while (z < w)
	{
		b = getbit(line, z);
		if (c != b)
			break;
		z ++;
	}

	return z - x;
}

static inline int 
findchanging(const unsigned char *line, int x, int w)
{
	int a, b;

	if (line == 0)
		return w;

	if (x == -1)
	{
		a = 0;
		x = 0;
	}
	else
	{
		a = getbit(line, x);
		x++;
	}

	while (x < w)
	{
		b = getbit(line, x);
		if (a != b)
			break;
		x++;
	}

	return x;
}

static inline int 
findchangingcolor(const unsigned char *line, int x, int w, int color)
{
	if (line == 0)
		return w;

	x = findchanging(line, x, w);

	if (x < w && getbit(line, x) != color)
		x = findchanging(line, x, w);

	return x;
}

static const unsigned char lm[8] =
	{ 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

static const unsigned char rm[8] =
	{ 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };

static inline void 
setbits(unsigned char *line, int x0, int x1)
{
	int a0, a1, b0, b1, a;

	a0 = x0 >> 3;
	a1 = x1 >> 3;

	b0 = x0 & 7;
	b1 = x1 & 7;

	if (a0 == a1)
	{
		if (b1)
			line[a0] |= lm[b0] & rm[b1];
	}
	else
	{
		line[a0] |= lm[b0];
		for (a = a0 + 1; a < a1; a++)
			line[a] = 0xFF;
		if (b1)
			line[a1] |= rm[b1];
	}
}

