/*
 * Synaptics TouchPad PS/2 mouse driver
 *
 *   2003 Peter Osterlund <petero2@telia.com>
 *     Ported to 2.5 input device infrastructure.
 *
 *   Copyright (C) 2001 Stefan Gmeiner <riddlebox@freesurf.ch>
 *     start merging tpconfig and gpm code to a xfree-input module
 *     adding some changes and extensions (ex. 3rd and 4th button)
 *
 *   Copyright (c) 1997 C. Scott Ananian <cananian@alumni.priceton.edu>
 *   Copyright (c) 1998-2000 Bruce Kalk <kall@compass.com>
 *     code for the special synaptics commands (from the tpconfig-source)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * Trademarks are the property of their respective owners.
 */
 
 
 
           #define NO_SYNAPTICS
           
           
           

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <ddk/iotypes.h>
#include "mouse.h"
#include "psaux.h"

#ifndef NO_SYNAPTICS

#include "synaptics.h"

/*****************************************************************************
 *	Synaptics communications functions
 ****************************************************************************/

/*
 * Use the Synaptics extended ps/2 syntax to write a special command byte.
 * special command: 0xE8 rr 0xE8 ss 0xE8 tt 0xE8 uu where (rr*64)+(ss*16)+(tt*4)+uu
 *                  is the command. A 0xF3 or 0xE9 must follow (see synaptics_send_cmd
 *                  and synaptics_set_mode)
 */
static int synaptics_special_cmd(PDEVICE_EXTENSION DeviceExtension, unsigned char command)
{
	int i;

	if (psmouse_command(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11))
		return -1;

	for (i = 6; i >= 0; i -= 2) {
		unsigned char d = (command >> i) & 3;
		if (psmouse_command(DeviceExtension, &d, PSMOUSE_CMD_SETRES))
			return -1;
	}

	return 0;
}

/*
 * Send a command to the synpatics touchpad by special commands
 */
static int synaptics_send_cmd(PDEVICE_EXTENSION DeviceExtension, unsigned char c, unsigned char *param)
{
	if (synaptics_special_cmd(DeviceExtension, c))
		return -1;
	if (psmouse_command(DeviceExtension, param, PSMOUSE_CMD_GETINFO))
		return -1;
	return 0;
}

/*
 * Set the synaptics touchpad mode byte by special commands
 */
static int synaptics_set_mode(PDEVICE_EXTENSION DeviceExtension, unsigned char mode)
{
	unsigned char param[1];

	if (synaptics_special_cmd(DeviceExtension, mode))
		return -1;
	param[0] = 0x14;
	if (psmouse_command(DeviceExtension, param, PSMOUSE_CMD_SETRATE))
		return -1;
	return 0;
}

static int synaptics_reset(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char r[2];

	if (psmouse_command(DeviceExtension, r, PSMOUSE_CMD_RESET_BAT))
		return -1;
	if (r[0] == 0xAA && r[1] == 0x00)
		return 0;
	return -1;
}

/*
 * Read the model-id bytes from the touchpad
 * see also SYN_MODEL_* macros
 */
static int synaptics_model_id(struct psmouse *psmouse, unsigned long int *model_id)
{
	unsigned char mi[3];

	if (synaptics_send_cmd(psmouse, SYN_QUE_MODEL, mi))
		return -1;
	*model_id = (mi[0]<<16) | (mi[1]<<8) | mi[2];
	return 0;
}

/*
 * Read the capability-bits from the touchpad
 * see also the SYN_CAP_* macros
 */
static int synaptics_capability(struct psmouse *psmouse, unsigned long int *capability)
{
	unsigned char cap[3];

	if (synaptics_send_cmd(psmouse, SYN_QUE_CAPABILITIES, cap))
		return -1;
	*capability = (cap[0]<<16) | (cap[1]<<8) | cap[2];
	if (SYN_CAP_VALID(*capability))
		return 0;
	return -1;
}

/*
 * Identify Touchpad
 * See also the SYN_ID_* macros
 */
static int synaptics_identify(struct psmouse *psmouse, unsigned long int *ident)
{
	unsigned char id[3];

	if (synaptics_send_cmd(psmouse, SYN_QUE_IDENTIFY, id))
		return -1;
	*ident = (id[0]<<16) | (id[1]<<8) | id[2];
	if (SYN_ID_IS_SYNAPTICS(*ident))
		return 0;
	return -1;
}

static int synaptics_enable_device(struct psmouse *psmouse)
{
	if (psmouse_command(psmouse, NULL, PSMOUSE_CMD_ENABLE))
		return -1;
	return 0;
}

static void print_ident(struct synaptics_data *priv)
{
	printk(KERN_INFO "Synaptics Touchpad, model: %ld\n", SYN_ID_MODEL(priv->identity));
	printk(KERN_INFO " Firware: %ld.%ld\n", SYN_ID_MAJOR(priv->identity),
	       SYN_ID_MINOR(priv->identity));

	if (SYN_MODEL_ROT180(priv->model_id))
		printk(KERN_INFO " 180 degree mounted touchpad\n");
	if (SYN_MODEL_PORTRAIT(priv->model_id))
		printk(KERN_INFO " portrait touchpad\n");
	printk(KERN_INFO " Sensor: %ld\n", SYN_MODEL_SENSOR(priv->model_id));
	if (SYN_MODEL_NEWABS(priv->model_id))
		printk(KERN_INFO " new absolute packet format\n");
	if (SYN_MODEL_PEN(priv->model_id))
		printk(KERN_INFO " pen detection\n");

	if (SYN_CAP_EXTENDED(priv->capabilities)) {
		printk(KERN_INFO " Touchpad has extended capability bits\n");
		if (SYN_CAP_FOUR_BUTTON(priv->capabilities))
			printk(KERN_INFO " -> four buttons\n");
		if (SYN_CAP_MULTIFINGER(priv->capabilities))
			printk(KERN_INFO " -> multifinger detection\n");
		if (SYN_CAP_PALMDETECT(priv->capabilities))
			printk(KERN_INFO " -> palm detection\n");
	}
}

static int query_hardware(struct psmouse *psmouse)
{
	struct synaptics_data *priv = psmouse->private;
	int retries = 0;

	while ((retries++ < 3) && synaptics_reset(psmouse))
		printk(KERN_ERR "synaptics reset failed\n");

	if (synaptics_identify(psmouse, &priv->identity))
		return -1;
	if (synaptics_model_id(psmouse, &priv->model_id))
		return -1;
	if (synaptics_capability(psmouse, &priv->capabilities))
		return -1;
	if (synaptics_set_mode(psmouse, (SYN_BIT_ABSOLUTE_MODE |
					 SYN_BIT_HIGH_RATE |
					 SYN_BIT_DISABLE_GESTURE |
					 SYN_BIT_W_MODE)))
		return -1;

	synaptics_enable_device(psmouse);

	print_ident(priv);

	return 0;
}

/*****************************************************************************
 *	Driver initialization/cleanup functions
 ****************************************************************************/

static inline void set_abs_params(struct input_dev *dev, int axis, int min, int max, int fuzz, int flat)
{
	dev->absmin[axis] = min;
	dev->absmax[axis] = max;
	dev->absfuzz[axis] = fuzz;
	dev->absflat[axis] = flat;

	set_bit(axis, dev->absbit);
}

int synaptics_init(struct psmouse *psmouse)
{
	struct synaptics_data *priv;

	psmouse->private = priv = kmalloc(sizeof(struct synaptics_data), GFP_KERNEL);
	if (!priv)
		return -1;
	memset(priv, 0, sizeof(struct synaptics_data));

	priv->inSync = 1;

	if (query_hardware(psmouse)) {
		printk(KERN_ERR "Unable to query/initialize Synaptics hardware.\n");
		goto init_fail;
	}

	/*
	 * The x/y limits are taken from the Synaptics TouchPad interfacing Guide,
	 * which says that they should be valid regardless of the actual size of
	 * the senser.
	 */
	set_bit(EV_ABS, psmouse->dev.evbit);
	set_abs_params(&psmouse->dev, ABS_X, 1472, 5472, 0, 0);
	set_abs_params(&psmouse->dev, ABS_Y, 1408, 4448, 0, 0);
	set_abs_params(&psmouse->dev, ABS_PRESSURE, 0, 255, 0, 0);

	set_bit(EV_MSC, psmouse->dev.evbit);
	set_bit(MSC_GESTURE, psmouse->dev.mscbit);

	set_bit(EV_KEY, psmouse->dev.evbit);
	set_bit(BTN_LEFT, psmouse->dev.keybit);
	set_bit(BTN_RIGHT, psmouse->dev.keybit);
	set_bit(BTN_FORWARD, psmouse->dev.keybit);
	set_bit(BTN_BACK, psmouse->dev.keybit);

	clear_bit(EV_REL, psmouse->dev.evbit);
	clear_bit(REL_X, psmouse->dev.relbit);
	clear_bit(REL_Y, psmouse->dev.relbit);

	return 0;

 init_fail:
	kfree(priv);
	return -1;
}

void synaptics_disconnect(struct psmouse *psmouse)
{
	struct synaptics_data *priv = psmouse->private;

	kfree(priv);
}

/*****************************************************************************
 *	Functions to interpret the absolute mode packets
 ****************************************************************************/

static void synaptics_parse_hw_state(struct synaptics_data *priv, struct synaptics_hw_state *hw)
{
	unsigned char *buf = priv->proto_buf;

	hw->x = (((buf[3] & 0x10) << 8) |
		 ((buf[1] & 0x0f) << 8) |
		 buf[4]);
	hw->y = (((buf[3] & 0x20) << 7) |
		 ((buf[1] & 0xf0) << 4) |
		 buf[5]);

	hw->z = buf[2];
	hw->w = (((buf[0] & 0x30) >> 2) |
		 ((buf[0] & 0x04) >> 1) |
		 ((buf[3] & 0x04) >> 2));

	hw->left  = (buf[0] & 0x01) ? 1 : 0;
	hw->right = (buf[0] & 0x2) ? 1 : 0;
	hw->up    = 0;
	hw->down  = 0;

	if (SYN_CAP_EXTENDED(priv->capabilities) &&
	    (SYN_CAP_FOUR_BUTTON(priv->capabilities))) {
		hw->up = ((buf[3] & 0x01)) ? 1 : 0;
		if (hw->left)
			hw->up = !hw->up;
		hw->down = ((buf[3] & 0x02)) ? 1 : 0;
		if (hw->right)
			hw->down = !hw->down;
	}
}

/*
 *  called for each full received packet from the touchpad
 */
static void synaptics_process_packet(struct psmouse *psmouse)
{
	struct input_dev *dev = &psmouse->dev;
	struct synaptics_data *priv = psmouse->private;
	struct synaptics_hw_state hw;

	synaptics_parse_hw_state(priv, &hw);

	if (hw.z > 0) {
		int w_ok = 0;
		/*
		 * Use capability bits to decide if the w value is valid.
		 * If not, set it to 5, which corresponds to a finger of
		 * normal width.
		 */
		if (SYN_CAP_EXTENDED(priv->capabilities)) {
			switch (hw.w) {
			case 0 ... 1:
				w_ok = SYN_CAP_MULTIFINGER(priv->capabilities);
				break;
			case 2:
				w_ok = SYN_MODEL_PEN(priv->model_id);
				break;
			case 4 ... 15:
				w_ok = SYN_CAP_PALMDETECT(priv->capabilities);
				break;
			}
		}
		if (!w_ok)
			hw.w = 5;
	}

	/* Post events */
	input_report_abs(dev, ABS_X,        hw.x);
	input_report_abs(dev, ABS_Y,        hw.y);
	input_report_abs(dev, ABS_PRESSURE, hw.z);

	if (hw.w != priv->old_w) {
		input_event(dev, EV_MSC, MSC_GESTURE, hw.w);
		priv->old_w = hw.w;
	}

	input_report_key(dev, BTN_LEFT,    hw.left);
	input_report_key(dev, BTN_RIGHT,   hw.right);
	input_report_key(dev, BTN_FORWARD, hw.up);
	input_report_key(dev, BTN_BACK,    hw.down);

	input_sync(dev);
}

void synaptics_process_byte(struct psmouse *psmouse, struct pt_regs *regs)
{
	struct input_dev *dev = &psmouse->dev;
	struct synaptics_data *priv = psmouse->private;
	unsigned char *pBuf = priv->proto_buf;
	unsigned char u = psmouse->packet[0];

	input_regs(dev, regs);

	pBuf[priv->proto_buf_tail++] = u;

	/* check first byte */
	if ((priv->proto_buf_tail == 1) && ((u & 0xC8) != 0x80)) {
		priv->inSync = 0;
		priv->proto_buf_tail = 0;
		printk(KERN_WARNING "Synaptics driver lost sync at 1st byte\n");
		return;
	}

	/* check 4th byte */
	if ((priv->proto_buf_tail == 4) && ((u & 0xc8) != 0xc0)) {
		priv->inSync = 0;
		priv->proto_buf_tail = 0;
		printk(KERN_WARNING "Synaptics driver lost sync at 4th byte\n");
		return;
	}

	if (priv->proto_buf_tail >= 6) { /* Full packet received */
		if (!priv->inSync) {
			priv->inSync = 1;
			printk(KERN_NOTICE "Synaptics driver resynced.\n");
		}
		synaptics_process_packet(psmouse);
		priv->proto_buf_tail = 0;
	}
}

#else

int synaptics_init(PDEVICE_EXTENSION DeviceExtension)
{
  return -1;
}

#endif
