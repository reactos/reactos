// #include <string.h>

_NOINTRINSIC(memcpy) 

// memcpy does not check for overlapping buffers
void * _CDECL memcpy(void* dest, const void* src, size_t count)
{
    char *char_dest = (char *)dest;
    char *char_src = (char *)src;

	/*  non-overlapping buffers */
    while(count)
	{
		*char_dest = *char_src;
		char_dest++;
		char_src++;
		count--;
	}

    return dest;
}
