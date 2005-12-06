
/******************************************************************************
 *
 * Name: hwsleep.c - ACPI Hardware Sleep/Wake Interface
 *              $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "acpi.h"
#include "acnamesp.h"
#include "achware.h"

#define _COMPONENT          ACPI_HARDWARE
	 MODULE_NAME         ("hwsleep")


/******************************************************************************
 *
 * FUNCTION:    Acpi_set_firmware_waking_vector
 *
 * PARAMETERS:  Physical_address    - Physical address of ACPI real mode
 *                                    entry point.
 *
 * RETURN:      AE_OK or AE_ERROR
 *
 * DESCRIPTION: Access function for d_firmware_waking_vector field in FACS
 *
 ******************************************************************************/

ACPI_STATUS
acpi_set_firmware_waking_vector (
	ACPI_PHYSICAL_ADDRESS physical_address)
{


	/* Make sure that we have an FACS */

	if (!acpi_gbl_FACS) {
		return (AE_NO_ACPI_TABLES);
	}

	/* Set the vector */

	if (acpi_gbl_FACS->vector_width == 32) {
		* (u32 *) acpi_gbl_FACS->firmware_waking_vector = (u32) physical_address;
	}
	else {
		*acpi_gbl_FACS->firmware_waking_vector = physical_address;
	}

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_firmware_waking_vector
 *
 * PARAMETERS:  *Physical_address   - Output buffer where contents of
 *                                    the Firmware_waking_vector field of
 *                                    the FACS will be stored.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Access function for d_firmware_waking_vector field in FACS
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_firmware_waking_vector (
	ACPI_PHYSICAL_ADDRESS *physical_address)
{


	if (!physical_address) {
		return (AE_BAD_PARAMETER);
	}

	/* Make sure that we have an FACS */

	if (!acpi_gbl_FACS) {
		return (AE_NO_ACPI_TABLES);
	}

	/* Get the vector */

	if (acpi_gbl_FACS->vector_width == 32) {
		*physical_address = * (u32 *) acpi_gbl_FACS->firmware_waking_vector;
	}
	else {
		*physical_address = *acpi_gbl_FACS->firmware_waking_vector;
	}

	return (AE_OK);
}

/******************************************************************************
 *
 * FUNCTION:    Acpi_enter_sleep_state
 *
 * PARAMETERS:  Sleep_state         - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state (see ACPI 2.0 spec p 231)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_enter_sleep_state (
	u8 sleep_state)
{
	ACPI_STATUS status;
	ACPI_OBJECT_LIST arg_list;
	ACPI_OBJECT arg;
	u8 type_a;
	u8 type_b;
	u16 PM1_acontrol;
	u16 PM1_bcontrol;

	/*
	 * _PSW methods could be run here to enable wake-on keyboard, LAN, etc.
	 */

	status = acpi_hw_obtain_sleep_type_register_data(sleep_state, &type_a, &type_b);

	if (!ACPI_SUCCESS(status)) {
		return status;
	}

	/* run the _PTS and _GTS methods */
	MEMSET(&arg_list, 0, sizeof(arg_list));
	arg_list.count = 1;
	arg_list.pointer = &arg;

	MEMSET(&arg, 0, sizeof(arg));
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = sleep_state;

	acpi_evaluate_object(NULL, "\\_PTS", &arg_list, NULL);
	acpi_evaluate_object(NULL, "\\_GTS", &arg_list, NULL);

	/* clear wake status */
	acpi_hw_register_bit_access(ACPI_WRITE, ACPI_MTX_LOCK, WAK_STS, 1);

	PM1_acontrol = (u16) acpi_hw_register_read(ACPI_MTX_LOCK, PM1_CONTROL);

	/* mask off SLP_EN and SLP_TYP fields */
	PM1_acontrol &= 0xC3FF;

	/* mask in SLP_EN */
	PM1_acontrol |= (1 << acpi_hw_get_bit_shift (SLP_EN_MASK));

	PM1_bcontrol = PM1_acontrol;

	/* mask in SLP_TYP */
	PM1_acontrol |= (type_a << acpi_hw_get_bit_shift (SLP_TYPE_X_MASK));
	PM1_bcontrol |= (type_b << acpi_hw_get_bit_shift (SLP_TYPE_X_MASK));

	disable();

	acpi_hw_register_write(ACPI_MTX_LOCK, PM1_a_CONTROL, PM1_acontrol);
	acpi_hw_register_write(ACPI_MTX_LOCK, PM1_b_CONTROL, PM1_bcontrol);
	acpi_hw_register_write(ACPI_MTX_LOCK, PM1_CONTROL,
		(1 << acpi_hw_get_bit_shift (SLP_EN_MASK)));

	enable();

	return (AE_OK);
}
