#include "../usb_wrapper.h"


void wait_ms(int mils)
{
	LARGE_INTEGER Interval;

	Interval.QuadPart = -mils*10;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}
