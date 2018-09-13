
/*
**  Cycle count overhead. This is a number of cycles required to actually
**  calculate the cycle count. To get the actual number of net cycles between
**  two calls to GetCycleCount, subtract CCNT_OVERHEAD.
**
**  For example:
**
**	__int64 start, finish, actual_cycle_count;
**
**	start = GetCycleCount ();
**
**		... do some stuff ...
**
**	finish = GetCycleCount ();
**
**	actual_cycle_count = finish - start - CCNT_OVERHEAD;
**
**
*/

#define CCNT_OVERHEAD 8


#pragma warning( disable: 4035 )	/* Don't complain about lack of return value */

__inline __int64 GetCycleCount ()
{
__asm   _emit   0x0F
__asm   _emit   0x31    /* rdtsc */
    // return EDX:EAX       causes annoying warning
};

__inline unsigned GetCycleCount32 ()  // enough for about 40 seconds
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31    /* rdtsc */
__asm   pop     EDX
    // return EAX       causes annoying warning
};

#pragma warning( default: 4035 )
