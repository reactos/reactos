#include <windows.h>


#ifdef __GNUC__

int _abnormal_termination(void)
{
	printf("Abnormal Termination\n");
//	return AbnormalTermination();
}

#else
#endif
