/*********************************************************************
*              _chkesp (MSVCRT.@)
*
* Trap to a debugger if the value of the stack pointer has changed.
*
* PARAMS
*  None.
*
* RETURNS
*  Does not return.
*
* NOTES
*  This function is available for iX86 only.
*
*  When VC++ generates debug code, it stores the value of the stack pointer
*  before calling any external function, and checks the value following
*  the call. It then calls this function, which will trap if the values are
*  not the same. Usually this means that the prototype used to call
*  the function is incorrect.  It can also mean that the .spec entry has
*  the wrong calling convention or parameters.
*/

#ifdef __i386__

void _chkesp(void)
{
}

#else

void _chkesp(void)
{
}

#endif  /* __i386__ */
