#include "boot.h"

int printf(const char *fmt, ...)
{
	va_list args;
	int i;
	va_start(args, fmt);
	i=printk(fmt, args);
	va_end(args);
	return 0;
}
