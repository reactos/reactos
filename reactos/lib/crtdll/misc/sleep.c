#include <windows.h>


void sleep(unsigned long timeout) 
{
	Sleep((timeout)?timeout:1);
}
