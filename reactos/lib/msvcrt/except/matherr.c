struct _exception {
        int type;
        char *name;
        double arg1;
        double arg2;
        double retval;
        } ;

int _matherr(struct _exception *e)
{
    return 0;
}

void __setusermatherr(int (* handler)(struct _exception *))
{

}

#define _FPIEEE_RECORD void

int _fpieee_flt(
	unsigned long exception_code,
        struct _EXCEPTION_POINTERS *ExceptionPointer,
        int (* handler)(_FPIEEE_RECORD *)
        )
{
	return 0;
}