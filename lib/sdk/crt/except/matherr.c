#include <precomp.h>
#include <math.h>


int _matherr(struct _exception* e)
{
    return 0;
}

/*
 * not exported by NTDLL
 *
 * @unimplemented
 */
void __setusermatherr(int (*handler)(struct _exception*))
{

}


#define _FPIEEE_RECORD void

/*
 * @unimplemented
 */
int _fpieee_flt(
        unsigned long exception_code,
        struct _EXCEPTION_POINTERS* ExceptionPointer,
        int (*handler)(_FPIEEE_RECORD*)
        )
{
    return 0;
}
