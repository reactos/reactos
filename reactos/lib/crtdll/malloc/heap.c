
#include <windows.h>
#include <msvcrt/malloc.h>

/*
 * @implemented
 */
int	_heapchk(void)
{
	if (!HeapValidate(GetProcessHeap(), 0, NULL))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapmin(void)
{
	if (!HeapCompact(GetProcessHeap(), 0)) 
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapset(unsigned int unFill)
{
	if (_heapchk() == -1)
		return -1;
	return 0;
		
}

/*
 * @implemented
 */
int _heapwalk(struct _heapinfo* entry)
{
	return 0;
}
