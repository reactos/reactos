/*
 * Logitech PS/2++ mouse driver
 *
 * Copyright (c) 1999-2003 Vojtech Pavlik <vojtech@suse.cz>
 * Copyright (c) 2003 Eric Wong <eric@yhbt.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include "mouse.h"
#include "logips2pp.h"

/*
 * Process a PS2++ or PS2T++ packet.
 */

void PS2PPProcessPacket(PDEVICE_EXTENSION DeviceExtension, PMOUSE_INPUT_DATA Input, int *wheel)
{
    unsigned char *packet = DeviceExtension->MouseBuffer;

	if ((packet[0] & 0x48) == 0x48 && (packet[1] & 0x02) == 0x02) {

		switch ((packet[1] >> 4) | (packet[0] & 0x30)) {

			case 0x0d: /* Mouse extra info */

				/* FIXME - mouse seems to have 2 wheels
                input_report_rel(dev, packet[2] & 0x80 ? REL_HWHEEL : REL_WHEEL,
					(int) (packet[2] & 8) - (int) (packet[2] & 7)); */
				
				*wheel = (int)(packet[2] & 8) - (int)(packet[2] & 7);
				Input->RawButtons |= (((packet[2] >> 4) & 1) ? GPM_B_FOURTH : 0);
				Input->RawButtons |= (((packet[2] >> 5) & 1) ? GPM_B_FIFTH : 0);

				break;

			case 0x0e: /* buttons 4, 5, 6, 7, 8, 9, 10 info */
				
				Input->RawButtons |= ((packet[2] & 1) ? GPM_B_FOURTH : 0);
				Input->RawButtons |= (((packet[2] >> 1) & 1) ? GPM_B_FIFTH : 0);
				
				/* FIXME - support those buttons???
				input_report_key(dev, BTN_BACK, (packet[2] >> 3) & 1);
				input_report_key(dev, BTN_FORWARD, (packet[2] >> 4) & 1);
				input_report_key(dev, BTN_TASK, (packet[2] >> 2) & 1);
				*/

				break;

			case 0x0f: /* TouchPad extra info */

				/* FIXME - mouse seems to have 2 wheels
				input_report_rel(dev, packet[2] & 0x08 ? REL_HWHEEL : REL_WHEEL,
					(int) ((packet[2] >> 4) & 8) - (int) ((packet[2] >> 4) & 7)); */
				
				*wheel = (int) ((packet[2] >> 4) & 8) - (int) ((packet[2] >> 4) & 7);

				packet[0] = packet[2] | 0x08;
				break;

			default:
				DbgPrint("logips2pp.c: Received PS2++ packet 0x%x, but don't know how to handle.\n",
					(packet[1] >> 4) | (packet[0] & 0x30));
		}

		packet[0] &= 0x0f;
		packet[1] = 0;
		packet[2] = 0;

	}
}

/*
 * ps2pp_cmd() sends a PS2++ command, sliced into two bit
 * pieces through the SETRES command. This is needed to send extended
 * commands to mice on notebooks that try to understand the PS/2 protocol
 * Ugly.
 */

static int ps2pp_cmd(PDEVICE_EXTENSION DeviceExtension, unsigned char *param, unsigned char command)
{
	unsigned char d;
	int i;

	if (SendCommand(DeviceExtension,  NULL, PSMOUSE_CMD_SETSCALE11))
		return -1;

	for (i = 6; i >= 0; i -= 2) {
		d = (command >> i) & 3;
		if(SendCommand(DeviceExtension, &d, PSMOUSE_CMD_SETRES))
			return -1;
	}

	if (SendCommand(DeviceExtension, param, PSMOUSE_CMD_POLL))
		return -1;

	return 0;
}

/*
 * SmartScroll / CruiseControl for some newer Logitech mice Defaults to
 * enabled if we do nothing to it. Of course I put this in because I want it
 * disabled :P
 * 1 - enabled (if previously disabled, also default)
 * 0/2 - disabled 
 */

static void ps2pp_set_smartscroll(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param[4];

	ps2pp_cmd(DeviceExtension, param, 0x32);

	param[0] = 0;
	SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
	SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
	SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);

	if (DeviceExtension->SmartScroll == 1) 
		param[0] = 1;
	else
	if (DeviceExtension->SmartScroll > 2)
		return;

	/* else leave param[0] == 0 to disable */
	SendCommand(DeviceExtension, param, PSMOUSE_CMD_SETRES);
}

/*
 * Support 800 dpi resolution _only_ if the user wants it (there are good
 * reasons to not use it even if the mouse supports it, and of course there are
 * also good reasons to use it, let the user decide).
 */

void PS2PPSet800dpi(PDEVICE_EXTENSION DeviceExtension)
{
	unsigned char param = 3;
	SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
	SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
	SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_SETSCALE11);
	SendCommand(DeviceExtension, &param, PSMOUSE_CMD_SETRES);
}

/*
 * Detect the exact model and features of a PS2++ or PS2T++ Logitech mouse or
 * touchpad.
 */

int PS2PPDetectModel(PDEVICE_EXTENSION DeviceExtension, unsigned char *param)
{
	int i;
	//char *vendor, *name;
	static int logitech_4btn[] = { 12, 40, 41, 42, 43, 52, 73, 80, -1 };
	static int logitech_wheel[] = { 52, 53, 75, 76, 80, 81, 83, 88, 112, -1 };
	static int logitech_ps2pp[] = { 12, 13, 40, 41, 42, 43, 50, 51, 52, 53, 73, 75,
						76, 80, 81, 83, 88, 96, 97, 112, -1 };
	static int logitech_mx[] = { 112, -1 };

	//vendor = "Logitech";
	//DbgPrint("Vendor: %s, name: %s\n", vendor, name);
	DeviceExtension->MouseModel = ((param[0] >> 4) & 0x07) | ((param[0] << 3) & 0x78);

	/*if (param[1] < 3)
		clear_bit(BTN_MIDDLE, DeviceExtension->dev.keybit);
	if (param[1] < 2)
		clear_bit(BTN_RIGHT, DeviceExtension->dev.keybit);*/

	DeviceExtension->MouseType = PSMOUSE_PS2;

	for (i = 0; logitech_ps2pp[i] != -1; i++)
		if (logitech_ps2pp[i] == DeviceExtension->MouseModel)
			DeviceExtension->MouseType = PSMOUSE_PS2PP;

	if (DeviceExtension->MouseType == PSMOUSE_PS2PP) {

	/*	for (i = 0; logitech_4btn[i] != -1; i++)
			if (logitech_4btn[i] == DeviceExtension->MouseModel)
				set_bit(BTN_SIDE, psmouse->dev.keybit);
*/
		for (i = 0; logitech_wheel[i] != -1; i++)
			if (logitech_wheel[i] == DeviceExtension->MouseModel) {
//				set_bit(REL_WHEEL, psmouse->dev.relbit);
				//name = "Wheel Mouse";DbgPrint("Vendor: %s, name: %s\n", vendor, name);
			}

		for (i = 0; logitech_mx[i] != -1; i++)
			if (logitech_mx[i]  == DeviceExtension->MouseModel) {
	/*			set_bit(BTN_SIDE, psmouse->dev.keybit);
				set_bit(BTN_EXTRA, psmouse->dev.keybit);
				set_bit(BTN_BACK, psmouse->dev.keybit);
				set_bit(BTN_FORWARD, psmouse->dev.keybit);
				set_bit(BTN_TASK, psmouse->dev.keybit);
		*/		//name = "MX Mouse";DbgPrint("Vendor: %s, name: %s\n", vendor, name);
			}

/*
 * Do Logitech PS2++ / PS2T++ magic init.
 */

		if (DeviceExtension->MouseModel == 97) { /* TouchPad 3 */

//			set_bit(REL_WHEEL, psmouse->dev.relbit);
//			set_bit(REL_HWHEEL, psmouse->dev.relbit);

			param[0] = 0x11; param[1] = 0x04; param[2] = 0x68; /* Unprotect RAM */
			SendCommand(DeviceExtension, param, 0x30d1);
			param[0] = 0x11; param[1] = 0x05; param[2] = 0x0b; /* Enable features */
			SendCommand(DeviceExtension, param, 0x30d1);
			param[0] = 0x11; param[1] = 0x09; param[2] = 0xc3; /* Enable PS2++ */
			SendCommand(DeviceExtension, param, 0x30d1);

			param[0] = 0;
			if (!SendCommand(DeviceExtension, param, 0x13d1) &&
				param[0] == 0x06 && param[1] == 0x00 && param[2] == 0x14) {
				//name = "TouchPad 3";DbgPrint("Vendor: %s, name: %s\n", vendor, name);
				return PSMOUSE_PS2TPP;
			}

		} else {

			param[0] = param[1] = param[2] = 0;
			ps2pp_cmd(DeviceExtension, param, 0x39); /* Magic knock */
			ps2pp_cmd(DeviceExtension, param, 0xDB);

			if ((param[0] & 0x78) == 0x48 && (param[1] & 0xf3) == 0xc2 &&
				(param[2] & 3) == ((param[1] >> 2) & 3)) {
					ps2pp_set_smartscroll(DeviceExtension);
					return PSMOUSE_PS2PP;
			}
		}
	}

	return 0;
}

