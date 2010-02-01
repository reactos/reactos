_NOINTRINSIC(_strset)

char * _CDECL _strset(char *szToFill, int szFill)
{
	char *t = szToFill;
	while (*szToFill != 0)
	{
		*szToFill = szFill;
		szToFill++;

	}
	return t;
}
