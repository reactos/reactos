typedef int size_t;

void * memset(void *src,int val,size_t count)
{
	int *int_src = src;
	while(count>0) {
		*int_src = val;
		count--;
	}
	return src;
}