#include <windows.h>
#include <msvcrt/stdlib.h>


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
