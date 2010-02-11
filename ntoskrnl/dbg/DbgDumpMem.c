void _CDECL DbgDumpMem(void *p, int cnt)
{
	i8u *p1 = p;
	i8u *p2;
	int cnt1, cnt2, i;
	char c;

	i = 0;
	while(cnt)
	{
		cnt1 = min(cnt, 0x10);
		cnt2 = cnt1;
		cnt -= cnt1;
		DbgPrintf("%04X:", i);
		i += 0x10;
		p2 = p1;
		while(cnt1--)
			DbgPrintf(" %02X", (int)(*p1++));
		DbgPrintf("    ");
		while(cnt2--)
		{
			c = *p2++;
			if(c < 0x20)
				c = '.';
			DbgPrintf("%c", (int)c);
		}
		DbgPrintf("\n");
	}
}

