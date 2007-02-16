#include "../usb_wrapper.h"


void wait_ms(int mils)
{
	LARGE_INTEGER Interval;

	DPRINT1("wait_ms(%d)\n", mils);

	Interval.QuadPart = -(mils+1)*10000;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);

//	schedule_timeout(1 + mils * HZ / 1000);
}

void my_udelay(int us)
{
	LARGE_INTEGER Interval;

	DPRINT1("udelay(%d)\n", us);

	Interval.QuadPart = -us*10;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}
