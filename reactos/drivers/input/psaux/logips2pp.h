/*
 * Logitech PS/2++ mouse driver header
 *
 * Copyright (c) 2003 Vojtech Pavlik <vojtech@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <ddk/ntddk.h>
#include <ddk/iotypes.h>
#include "psaux.h"

#ifndef _LOGIPS2PP_H
#define _LOGIPS2PP_H
void ps2pp_process_packet(PDEVICE_EXTENSION DeviceExtension, PMOUSE_INPUT_DATA Input);
void ps2pp_set_800dpi(PDEVICE_EXTENSION DeviceExtension);
int ps2pp_detect_model(PDEVICE_EXTENSION DeviceExtension, unsigned char *param);
#endif
