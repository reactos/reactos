
static i64u next = 0;

/*
 * @implemented
 */
int _CDECL rand(void)
{
	next = next * 0x5deece66d + 11;
	return (int)((next >> 16) & RAND_MAX);
}

/*
 * @implemented
 */
void _CDECL srand(unsigned seed)
{
	next = seed;
}
