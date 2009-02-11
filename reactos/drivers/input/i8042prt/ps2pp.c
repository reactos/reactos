/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/ps2pp.c
 * PURPOSE:     ps2pp protocol handling
 * PROGRAMMERS: Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
 *              Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

/* FUNCTIONS *****************************************************************/

VOID
i8042MouHandlePs2pp(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Input)
{
	UCHAR PktType;
	PMOUSE_INPUT_DATA MouseInput;

	MouseInput = DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer;

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

	switch (DeviceExtension->MouseState)
	{
		case MouseIdle:
		case XMovement:
			DeviceExtension->MouseLogiBuffer[DeviceExtension->MouseState] = Input;
			DeviceExtension->MouseState++;
			break;

		case YMovement:
			DeviceExtension->MouseLogiBuffer[2] = Input;
			DeviceExtension->MouseState = MouseIdle;

			/* first check if it's a normal packet */

			if (!(DeviceExtension->MouseLogiBuffer[0] & 0xC0))
			{
				DeviceExtension->MouseState = MouseIdle;
				i8042MouHandle(DeviceExtension, DeviceExtension->MouseLogiBuffer[0]);
				i8042MouHandle(DeviceExtension, DeviceExtension->MouseLogiBuffer[1]);
				i8042MouHandle(DeviceExtension, DeviceExtension->MouseLogiBuffer[2]);
				/* We could care about wether MouseState really
				 * advances, but we don't need to because we're
				 * only doing three bytes anyway, so the packet
				 * will never complete if it's broken.
				 */
				return;
			}

			/* sanity check */
			if (((DeviceExtension->MouseLogiBuffer[0] & 0x48) != 0x48) ||
			   (((DeviceExtension->MouseLogiBuffer[1] & 0x0C) >> 2) !=
			     (DeviceExtension->MouseLogiBuffer[2] & 0x03)))
				{
					WARN_(I8042PRT, "Ps2pp packet fails sanity checks\n");
					return;
				}

			/* Now get the packet type */
			PktType = ((DeviceExtension->MouseLogiBuffer[0] & 0x30) >> 4) &
			          ((DeviceExtension->MouseLogiBuffer[1] & 0x30) >> 6);

			switch (PktType)
			{
				case 0:
					/* The packet contains the device ID, but we
					 * already read that in the initialization
					 * sequence. Ignore it.
					 */
					return;
				case 1:
					RtlZeroMemory(MouseInput, sizeof(MOUSE_INPUT_DATA));
					if (DeviceExtension->MouseLogiBuffer[2] & 0x10)
						MouseInput->RawButtons |= MOUSE_BUTTON_4_DOWN;

					if (DeviceExtension->MouseLogiBuffer[2] & 0x20)
						MouseInput->RawButtons |= MOUSE_BUTTON_5_DOWN;

					if (DeviceExtension->MouseLogiBuffer[2] & 0x0F)
					{
						MouseInput->ButtonFlags |= MOUSE_WHEEL;
						if (DeviceExtension->MouseLogiBuffer[2] & 0x08)
							MouseInput->ButtonData = (DeviceExtension->MouseLogiBuffer[2] & 0x07) - 8;
						else
							MouseInput->ButtonData = DeviceExtension->MouseLogiBuffer[2] & 0x07;
					}
					i8042MouHandleButtons(
						DeviceExtension,
						MOUSE_BUTTON_4_DOWN | MOUSE_BUTTON_5_DOWN);
					DeviceExtension->MouseHook.QueueMousePacket(DeviceExtension->MouseHook.CallContext);
					return;
				default:
					/* These are for things that would probably
					 * be handled by logitechs own driver.
					 */
					return;
			}

		default:
			WARN_(I8042PRT, "Unexpected input state for ps2pp!\n");
	}
}
