#include <windows.h>
#include <crtdll/stdlib.h>


void* malloc(size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),0,_size));
}

void free(void* _ptr)
{
   HeapFree(GetProcessHeap(),0,_ptr);
}

void* calloc(size_t _nmemb, size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, _nmemb*_size));
}

void* realloc(void* _ptr, size_t _size)
{
   return(HeapReAlloc(GetProcessHeap(),0,_ptr,_size));
}
#undef alloca
void *alloca(size_t s)
{
	register unsigned int as = s;

// alloca(0) should not return the stack pointer
	if ( s == 0 )
		return NULL;

	
	if ( (s & 0xfffffffc)  != 0 )
		as += 4;
		
	as &= 0xfffffffc;
	
	__asm__ __volatile__(
	"mov %0, %%edx  	\n"
//	"popl %%ebp		\n"
	"leave			\n"
	"popl  %%ecx		\n"
        "subl  %%edx, %%esp	\n"
        "movl  %%esp, %%eax	\n"
        "addl  $20, %%eax        \n"//4 bytes + 16 bytes = arguments
        "push  %%ecx		\n"
        "ret			\n"
        :
        :"g" ( as)
        );
        
       
        return NULL;
}

void *_alloca(size_t s)
{
	register unsigned int as = s;

// alloca(0) should not return the stack pointer
	if ( s == 0 )
		return NULL;

	
	if ( (s & 0xfffffffc)  != 0 )
		as += 4;
		
	as &= 0xfffffffc;
	
	__asm__ __volatile__(
	"mov %0, %%edx  	\n"
//	"popl %%ebp		\n"
	"leave			\n"
	"popl  %%ecx		\n"
        "subl  %%edx, %%esp	\n"
        "movl  %%esp, %%eax	\n"
        "addl  $20, %%eax        \n"//4 bytes + 16 bytes = arguments
        "push  %%ecx		\n"
        "ret			\n"
        :
        :"g" ( as)
        );
        
       
        return NULL;
}
