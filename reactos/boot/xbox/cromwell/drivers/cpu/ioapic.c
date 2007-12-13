#include "boot.h"
#include "config.h"
#include "cpu.h"

struct ioapicreg {
	unsigned int reg;
	unsigned int value_low, value_high;
};
struct ioapicreg ioapicregvalues[] = {
#define ALL		(0xff << 24)
#define NONE		(0)
#define DISABLED	(1 << 16)
#define ENABLED		(0 << 16)
#define TRIGGER_EDGE	(0 << 15)
#define TRIGGER_LEVEL	(1 << 15)
#define POLARITY_HIGH	(0 << 13)
#define POLARITY_LOW	(1 << 13)
#define PHYSICAL_DEST	(0 << 11)
#define LOGICAL_DEST	(1 << 11)
#define ExtINT		(7 << 8)
#define NMI		(4 << 8)
#define SMI		(2 << 8)
#define INT		(1 << 8)
	/* mask, trigger, polarity, destination, delivery, vector */
	{0x00, DISABLED, NONE},
	{0x01, DISABLED, NONE},
	{0x02, DISABLED, NONE},
	{0x03, DISABLED, NONE},
	{0x04, DISABLED, NONE},
	{0x05, DISABLED, NONE},
	{0x06, DISABLED, NONE},
	{0x07, DISABLED, NONE},
	{0x08, DISABLED, NONE},
	{0x09, DISABLED, NONE},
	{0x0a, DISABLED, NONE},
	{0x0b, DISABLED, NONE},
	{0x0c, DISABLED, NONE},
	{0x0d, DISABLED, NONE},
	{0x0e, DISABLED, NONE},
	{0x0f, DISABLED, NONE},
	{0x10, DISABLED, NONE},
	{0x11, DISABLED, NONE},
	{0x12, DISABLED, NONE},
	{0x13, DISABLED, NONE},
	{0x14, DISABLED, NONE},
	{0x14, DISABLED, NONE},
	{0x15, DISABLED, NONE},
	{0x16, DISABLED, NONE},
	{0x17, DISABLED, NONE},
};

void setup_ioapic(void)
{
	int i;
	unsigned long value_low, value_high;
	unsigned long nvram = 0xfec00000;
	volatile unsigned long *l;
	struct ioapicreg *a = ioapicregvalues;

	l = (unsigned long *) nvram;

	for (i = 0; i < sizeof(ioapicregvalues) / sizeof(ioapicregvalues[0]);
	     i++, a++) {
		l[0] = (a->reg * 2) + 0x10;
		l[4] = a->value_low;
		value_low = l[4];
		l[0] = (a->reg *2) + 0x11;
		l[4] = a->value_high;
		value_high = l[4];
		if ((i==0) && (value_low == 0xffffffff)) {
			printk("IO APIC not responding.\n");
			return;
		}
		printk("for IRQ, reg 0x%08x value 0x%08x 0x%08x\n", 
			a->reg, a->value_low, a->value_high);
	}
}
