#ifndef _ACPI_PCH_
#define _ACPI_PCH_

#include <stdio.h>

#include <acpi.h>

// ACPI_BIOS_ERROR was defined in acoutput.h, but is
// redefined in bugcodes.h included from the DDK.
#undef ACPI_BIOS_ERROR

#include <acpisys.h>
#include <acpi_bus.h>
#include <acpi_drivers.h>
#include <glue.h>
#include <wdmguid.h>
#include <acpiioct.h>

#endif /* _ACPI_PCH_ */
