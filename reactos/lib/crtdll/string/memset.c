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

void *__memset_generic(void *src,int val,size_t count)
{
	return memset(src,val,count);
}

void *  __constant_c_and_count_memset(void *  s, unsigned long pattern, size_t count)
{
	return memset(s,pattern,count); 
}
  

void * __constant_c_memset(void *src,int val,size_t count)
{
	return memset(src,val,count);
}
