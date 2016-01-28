/*
 *  acpi_system.c - ACPI System Driver ($Revision: 57 $)
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

/* Modified for ReactOS and latest ACPICA
 * Copyright (C)2009  Samuel Serapion 
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

ACPI_STATUS acpi_system_save_state(UINT32);

#define _COMPONENT		ACPI_SYSTEM_COMPONENT
ACPI_MODULE_NAME		("acpi_system")

#define PREFIX			"ACPI: "

static int acpi_system_add (struct acpi_device *device);
static int acpi_system_remove (struct acpi_device *device, int type);

ACPI_STATUS acpi_suspend (UINT32 state);

static struct acpi_driver acpi_system_driver = {
    {0,0},
    ACPI_SYSTEM_DRIVER_NAME,
    ACPI_SYSTEM_CLASS,
    0,
    0,
    ACPI_SYSTEM_HID,
    {acpi_system_add, acpi_system_remove}
};

struct acpi_system
{
	ACPI_HANDLE		handle;
	UINT8			states[ACPI_S_STATE_COUNT];
};


static int
acpi_system_add (
	struct acpi_device	*device)
{
	int			result = 0;
	ACPI_STATUS		status = AE_OK;
	struct acpi_system	*system = NULL;
	UINT8			i = 0;

	ACPI_FUNCTION_TRACE("acpi_system_add");

	if (!device)
		return_VALUE(-1);

	system = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct acpi_system),'IPCA');
	if (!system)
		return_VALUE(-14);
	memset(system, 0, sizeof(struct acpi_system));

	system->handle = device->handle;
	sprintf(acpi_device_name(device), "%s", ACPI_SYSTEM_DEVICE_NAME);
	sprintf(acpi_device_class(device), "%s", ACPI_SYSTEM_CLASS);
	acpi_driver_data(device) = system;

	DPRINT("%s [%s] (supports", 
		acpi_device_name(device), acpi_device_bid(device));
	for (i=0; i<ACPI_S_STATE_COUNT; i++) {
		UINT8 type_a, type_b;
		status = AcpiGetSleepTypeData(i, &type_a, &type_b);
		switch (i) {
		case ACPI_STATE_S4:
			if (/*AcpiGbl_FACS->S4bios_f &&*/
			    0 != AcpiGbl_FADT.SmiCommand) {
				DPRINT(" S4bios\n");
				system->states[i] = 1;
			}
			/* no break */
		default: 
			if (ACPI_SUCCESS(status)) {
				system->states[i] = 1;
				DPRINT(" S%d", i);
			}
		}
	}

//#ifdef CONFIG_PM
//	/* Install the soft-off (S5) handler. */
//	if (system->states[ACPI_STATE_S5]) {
//		pm_power_off = acpi_power_off;
//		register_sysrq_key('o', &sysrq_acpi_poweroff_op);
//	}
//#endif

	if (result)
		ExFreePoolWithTag(system, 'IPCA');

	return_VALUE(result);
}

static int
acpi_system_remove (
	struct acpi_device	*device,
	int			type)
{
	struct acpi_system	*system = NULL;

	ACPI_FUNCTION_TRACE("acpi_system_remove");

	if (!device || !acpi_driver_data(device))
		return_VALUE(-1);

	system = (struct acpi_system *) acpi_driver_data(device);

//#ifdef CONFIG_PM
//	/* Remove the soft-off (S5) handler. */
//	if (system->states[ACPI_STATE_S5]) {
//		unregister_sysrq_key('o', &sysrq_acpi_poweroff_op);
//		pm_power_off = NULL;
//	}
//#endif
//
//
	ExFreePoolWithTag(system, 'IPCA');

	return 0;
}

/**
 * acpi_system_restore_state - OS-specific restoration of state
 * @state:	sleep state we're exiting
 *
 * Note that if we're coming back from S4, the memory image should have
 * already been loaded from the disk and is already in place.  (Otherwise how
 * else would we be here?).
 */
ACPI_STATUS
acpi_system_restore_state(
	UINT32			state)
{
	/* 
	 * We should only be here if we're coming back from STR or STD.
	 * And, in the case of the latter, the memory image should have already
	 * been loaded from disk.
	 */
	if (state > ACPI_STATE_S1) {
		//acpi_restore_state_mem();

		/* Do _early_ resume for irqs.  Required by
		 * ACPI specs.
		 */
		/* TBD: call arch dependant reinitialization of the 
		 * interrupts.
		 */
#ifdef _X86_
		//init_8259A(0);
#endif
		/* wait for power to come back */
		KeStallExecutionProcessor(100);

	}

	/* Be really sure that irqs are disabled. */
	//ACPI_DISABLE_IRQS();

	/* Wait a little again, just in case... */
	KeStallExecutionProcessor(10);

	/* enable interrupts once again */
	//ACPI_ENABLE_IRQS();

	/* turn all the devices back on */
	//if (state > ACPI_STATE_S1)
		//pm_send_all(PM_RESUME, (void *)0);

	return AE_OK;
}


/**
 * acpi_system_save_state - save OS specific state and power down devices
 * @state:	sleep state we're entering.
 *
 * This handles saving all context to memory, and possibly disk.
 * First, we call to the device driver layer to save device state.
 * Once we have that, we save whatevery processor and kernel state we
 * need to memory.
 * If we're entering S4, we then write the memory image to disk.
 *
 * Only then it is safe for us to power down devices, since we may need
 * the disks and upstream buses to write to.
 */
ACPI_STATUS
acpi_system_save_state(
	UINT32			state)
{
	int			error = 0;

	/* Send notification to devices that they will be suspended.
	 * If any device or driver cannot make the transition, either up
	 * or down, we'll get an error back.
	 */
	/*if (state > ACPI_STATE_S1) {
		error = pm_send_all(PM_SAVE_STATE, (void *)3);
		if (error)
			return AE_ERROR;
	}*/

	//if (state <= ACPI_STATE_S5) {
	//	/* Tell devices to stop I/O and actually save their state.
	//	 * It is theoretically possible that something could fail,
	//	 * so handle that gracefully..
	//	 */
	//	if (state > ACPI_STATE_S1 && state != ACPI_STATE_S5) {
	//		error = pm_send_all(PM_SUSPEND, (void *)3);
	//		if (error) {
	//			/* Tell devices to restore state if they have
	//			 * it saved and to start taking I/O requests.
	//			 */
	//			pm_send_all(PM_RESUME, (void *)0);
	//			return error;
	//		}
	//	}
		
		/* flush caches */
		ACPI_FLUSH_CPU_CACHE();

		/* Do arch specific saving of state. */
		if (state > ACPI_STATE_S1) {
			error = 0;//acpi_save_state_mem();

			/* TBD: if no s4bios, write codes for
			 * acpi_save_state_disk()...
			 */
#if 0
			if (!error && (state == ACPI_STATE_S4))
				error = acpi_save_state_disk();
#endif
			/*if (error) {
				pm_send_all(PM_RESUME, (void *)0);
				return error;
			}*/
		}
	//}
	/* disable interrupts
	 * Note that acpi_suspend -- our caller -- will do this once we return.
	 * But, we want it done early, so we don't get any suprises during
	 * the device suspend sequence.
	 */
	//ACPI_DISABLE_IRQS();

	/* Unconditionally turn off devices.
	 * Obvious if we enter a sleep state.
	 * If entering S5 (soft off), this should put devices in a
	 * quiescent state.
	 */

	//if (state > ACPI_STATE_S1) {
	//	error = pm_send_all(PM_SUSPEND, (void *)3);

	//	/* We're pretty screwed if we got an error from this.
	//	 * We try to recover by simply calling our own restore_state
	//	 * function; see above for definition.
	//	 *
	//	 * If it's S5 though, go through with it anyway..
	//	 */
	//	if (error && state != ACPI_STATE_S5)
	//		acpi_system_restore_state(state);
	//}
	return error ? AE_ERROR : AE_OK;
}


/****************************************************************************
 *
 * FUNCTION:    acpi_system_suspend
 *
 * PARAMETERS:  %state: Sleep state to enter.
 *
 * RETURN:      ACPI_STATUS, whether or not we successfully entered and
 *              exited sleep.
 *
 * DESCRIPTION: Perform OS-specific action to enter sleep state.
 *              This is the final step in going to sleep, per spec.  If we
 *              know we're coming back (i.e. not entering S5), we save the
 *              processor flags. [ We'll have to save and restore them anyway,
 *              so we use the arch-agnostic save_flags and restore_flags
 *              here.]  We then set the place to return to in arch-specific
 *              globals using arch_set_return_point. Finally, we call the
 *              ACPI function to write the proper values to I/O ports.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_system_suspend(
	UINT32		state)
{
	ACPI_STATUS		status = AE_ERROR;
	//unsigned long		flags = 0;

	//local_irq_save(flags);
	/* kernel_fpu_begin(); */

	switch (state) {
	case ACPI_STATE_S1:
	case ACPI_STATE_S5:
		//barrier();
		status = AcpiEnterSleepState(state);
		break;
	case ACPI_STATE_S4:
		//do_suspend_lowlevel_s4bios(0);
		break;
	}

	/* kernel_fpu_end(); */
	//local_irq_restore(flags);

	return status;
}



/**
 * acpi_suspend - OS-agnostic system suspend/resume support (S? states)
 * @state:	state we're entering
 *
 */
ACPI_STATUS
acpi_suspend (
	UINT32			state)
{
	ACPI_STATUS status;

	/* only support S1 and S5 on kernel 2.4 */
	//if (state != ACPI_STATE_S1 && state != ACPI_STATE_S4
	//    && state != ACPI_STATE_S5)
	//	return AE_ERROR;


	//if (ACPI_STATE_S4 == state) {
	//	/* For s4bios, we need a wakeup address. */
	//	if (1 == AcpiGbl_FACS->S4bios_f &&
	//	    0 != AcpiGbl_FADT->smi_cmd) {
	//		if (!acpi_wakeup_address)
	//			return AE_ERROR;
	//		AcpiSetFirmwareWakingVector((acpi_physical_address) acpi_wakeup_address);
	//	} else
	//		/* We don't support S4 under 2.4.  Give up */
	//		return AE_ERROR;
	//}
	AcpiEnterSleepStatePrep(state);

	status = AcpiEnterSleepState(state);
	if (!ACPI_SUCCESS(status) && state != ACPI_STATE_S5)
		return status;

	/* disable interrupts and flush caches */
	_disable();
	ACPI_FLUSH_CPU_CACHE();

	/* perform OS-specific sleep actions */
	status = acpi_system_suspend(state);

	/* Even if we failed to go to sleep, all of the devices are in an suspended
	 * mode. So, we run these unconditionaly to make sure we have a usable system
	 * no matter what.
	 */
	AcpiLeaveSleepState(state);
	acpi_system_restore_state(state);

	/* make sure interrupts are enabled */
	_enable();

	/* reset firmware waking vector */
	AcpiSetFirmwareWakingVector(0, 0);

	return status;
}

int 
acpi_system_init (void)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_system_init");

	result = acpi_bus_register_driver(&acpi_system_driver);
	if (result < 0)
		return_VALUE(AE_NOT_FOUND);

	return_VALUE(0);
}


void
acpi_system_exit (void)
{
	ACPI_FUNCTION_TRACE("acpi_system_exit");
	acpi_bus_unregister_driver(&acpi_system_driver);
	return_VOID;
}

