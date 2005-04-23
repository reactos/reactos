/*
 * Synaptics TouchPad PS/2 mouse driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef NO_SYNAPTICS

#ifndef _SYNAPTICS_H
#define _SYNAPTICS_H

extern void synaptics_process_byte(DeviceExtension, struct pt_regs *regs);
#endif
extern int synaptics_init(DeviceExtension);
#ifndef NO_SYNAPTICS
extern void synaptics_disconnect(DeviceExtension);

/* synaptics queries */
#define SYN_QUE_IDENTIFY		0x00
#define SYN_QUE_MODES			0x01
#define SYN_QUE_CAPABILITIES		0x02
#define SYN_QUE_MODEL			0x03
#define SYN_QUE_SERIAL_NUMBER_PREFIX	0x06
#define SYN_QUE_SERIAL_NUMBER_SUFFIX	0x07
#define SYN_QUE_RESOLUTION		0x08

/* synatics modes */
#define SYN_BIT_ABSOLUTE_MODE		(1 << 7)
#define SYN_BIT_HIGH_RATE		(1 << 6)
#define SYN_BIT_SLEEP_MODE		(1 << 3)
#define SYN_BIT_DISABLE_GESTURE		(1 << 2)
#define SYN_BIT_W_MODE			(1 << 0)

/* synaptics model ID bits */
#define SYN_MODEL_ROT180(m)		((m) & (1 << 23))
#define SYN_MODEL_PORTRAIT(m)		((m) & (1 << 22))
#define SYN_MODEL_SENSOR(m)		(((m) >> 16) & 0x3f)
#define SYN_MODEL_HARDWARE(m)		(((m) >> 9) & 0x7f)
#define SYN_MODEL_NEWABS(m)		((m) & (1 << 7))
#define SYN_MODEL_PEN(m)		((m) & (1 << 6))
#define SYN_MODEL_SIMPLIC(m)		((m) & (1 << 5))
#define SYN_MODEL_GEOMETRY(m)		((m) & 0x0f)

/* synaptics capability bits */
#define SYN_CAP_EXTENDED(c)		((c) & (1 << 23))
#define SYN_CAP_SLEEP(c)		((c) & (1 << 4))
#define SYN_CAP_FOUR_BUTTON(c)		((c) & (1 << 3))
#define SYN_CAP_MULTIFINGER(c)		((c) & (1 << 1))
#define SYN_CAP_PALMDETECT(c)		((c) & (1 << 0))
#define SYN_CAP_VALID(c)		((((c) & 0x00ff00) >> 8) == 0x47)

/* synaptics modes query bits */
#define SYN_MODE_ABSOLUTE(m)		((m) & (1 << 7))
#define SYN_MODE_RATE(m)		((m) & (1 << 6))
#define SYN_MODE_BAUD_SLEEP(m)		((m) & (1 << 3))
#define SYN_MODE_DISABLE_GESTURE(m)	((m) & (1 << 2))
#define SYN_MODE_PACKSIZE(m)		((m) & (1 << 1))
#define SYN_MODE_WMODE(m)		((m) & (1 << 0))

/* synaptics identify query bits */
#define SYN_ID_MODEL(i) 		(((i) >> 4) & 0x0f)
#define SYN_ID_MAJOR(i) 		((i) & 0x0f)
#define SYN_ID_MINOR(i) 		(((i) >> 16) & 0xff)
#define SYN_ID_IS_SYNAPTICS(i)		((((i) >> 8) & 0xff) == 0x47)

/*
 * A structure to describe the state of the touchpad hardware (buttons and pad)
 */

struct synaptics_hw_state {
	int x;
	int y;
	int z;
	int w;
	int left;
	int right;
	int up;
	int down;
};

struct synaptics_data {
	/* Data read from the touchpad */
	unsigned long int model_id;		/* Model-ID */
	unsigned long int capabilities; 	/* Capabilities */
	unsigned long int identity;		/* Identification */

	/* Data for normal processing */
	unsigned char proto_buf[6];		/* Buffer for Packet */
	unsigned char last_byte;		/* last received byte */
	int inSync;				/* Packets in sync */
	int proto_buf_tail;

	int old_w;				/* Previous w value */
};

#endif /* _SYNAPTICS_H */

#endif /* NO_SYNAPTICS */

