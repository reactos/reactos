/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	ReactOS kernel
 * FILE:	drivers/input/i8042prt/ps2pp.c
 * PURPOSE:	i8042prt driver
 * 		ps2pp protocol handling
 * PROGRAMMER:	Tinus
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

VOID I8042MouseHandlePs2pp(PDEVICE_EXTENSION DevExt, UCHAR Input)
{
	UCHAR PktType;
	PMOUSE_INPUT_DATA MouseInput = DevExt->MouseBuffer +
	                                         DevExt->MouseInBuffer;

/* First, collect 3 bytes for a packet
 * We can detect out-of-sync only by checking
 * the whole packet anyway.
 *
 * If bit 7 and 8 of the first byte are 0, its
 * a normal packet.
 *
 * Otherwise, the packet is different, like this:
 * 1: E  1 b3 b2  x  x  x  x
 * 2: x  x b1 b0 x1 x0  1  0
 * 3: x  x  x  x  x  x x1 x0
 *
 * b3-0 form a code that specifies the packet type:
 *
 * 0  Device Type
 * 1  Rollers and buttons
 * 2   Reserved
 * 3   Reserved
 * 4  Device ID
 * 5  Channel & Battery
 * 6  Wireless notifications
 * 7   Reserved
 * 8  ShortID LSB (ShortID is a number that is supposed to differentiate
 * 9  ShortID MSB  between your mouse and your neighbours')
 * 10  Reserved
 * 11 Mouse capabilities
 * 12 Remote control LSB
 * 13 Remote control MSB
 * 14  Reserved
 * 15 Extended packet
 */

	switch (DevExt->MouseState) {
	case MouseIdle:
	case XMovement:
		DevExt->MouseLogiBuffer[DevExt->MouseState] = Input;
		DevExt->MouseState++;
		break;

	case YMovement:
		DevExt->MouseLogiBuffer[2] = Input;
		DevExt->MouseState = MouseIdle;

		/* first check if it's a normal packet */

		if (!(DevExt->MouseLogiBuffer[0] & 0xC0)) {
			DevExt->MouseState = MouseIdle;
			I8042MouseHandle(DevExt, DevExt->MouseLogiBuffer[0]);
			I8042MouseHandle(DevExt, DevExt->MouseLogiBuffer[1]);
			I8042MouseHandle(DevExt, DevExt->MouseLogiBuffer[2]);
			/* We could care about wether MouseState really
			 * advances, but we don't need to because we're
			 * only doing three bytes anyway, so the packet
			 * will never complete if it's broken.
			 */
			return;
		}

		/* sanity check */

		if (((DevExt->MouseLogiBuffer[0] & 0x48) != 0x48) ||
		    (((DevExt->MouseLogiBuffer[1] & 0x0C) >> 2) !=
		     (DevExt->MouseLogiBuffer[2] & 0x03))) {
			DPRINT1("Ps2pp packet fails sanity checks\n");
			return;
		}

		/* Now get the packet type */

		PktType = ((DevExt->MouseLogiBuffer[0] & 0x30) >> 4) &
		          ((DevExt->MouseLogiBuffer[1] & 0x30) >> 6);

		switch (PktType) {
		case 0:
			/* The packet contains the device ID, but we
			 * already read that in the initialization
			 * sequence. Ignore it.
			 */
			return;
		case 1:
			RtlZeroMemory(MouseInput, sizeof(MOUSE_INPUT_DATA));
			if (DevExt->MouseLogiBuffer[2] & 0x10)
				MouseInput->RawButtons |= MOUSE_BUTTON_4_DOWN;

			if (DevExt->MouseLogiBuffer[2] & 0x20)
				MouseInput->RawButtons |= MOUSE_BUTTON_5_DOWN;

			if (DevExt->MouseLogiBuffer[2] & 0x0F) {
				MouseInput->ButtonFlags |= MOUSE_WHEEL;
				if (DevExt->MouseLogiBuffer[2] & 0x08)
					MouseInput->ButtonData =
					   (DevExt->MouseLogiBuffer[2] & 0x07) -
					       8;
				else
					MouseInput->ButtonData =
					     DevExt->MouseLogiBuffer[2] & 0x07;
			}
			I8042MouseHandleButtons(DevExt, MOUSE_BUTTON_4_DOWN |
			                                MOUSE_BUTTON_5_DOWN);
			I8042QueueMousePacket(
			                 DevExt->MouseObject);
			return;
		default:
			/* These are for things that would probably
			 * be handled by logitechs own driver.
			 */
			return;
		}
	default:
		DPRINT1("Unexpected input state for ps2pp!\n");
	}
}
