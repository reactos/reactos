typedef int size_t;

void * memset(void *src,int val,size_t count)
{
	char *char_src = src;

	while(count>0) {
		*char_src = val;
		char_src++;
		count--;
	}
	return src;
}
