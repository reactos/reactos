/******************************************************************************
 *
 * Name: acpi.h - Master include file, Publics and external data.
 *       $Revision: 1.2 $
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

#ifndef __ACPI_H__
#define __ACPI_H__

#include "platform/types.h"
#undef ROUND_DOWN
#undef ROUND_UP

/*
 * Common includes for all ACPI driver files
 * We put them here because we don't want to duplicate them
 * in the rest of the source code again and again.
 */
#include "acconfig.h"           /* Configuration constants */
#include "platform/acenv.h"     /* Target environment specific items */
#include "actypes.h"            /* Fundamental common data types */
#include "acexcep.h"            /* ACPI exception codes */
#include "acmacros.h"           /* C macros */
#include "actbl.h"              /* ACPI table definitions */
#include "aclocal.h"            /* Internal data types */
#include "acoutput.h"           /* Error output and Debug macros */
#include "acpiosxf.h"           /* Interfaces to the ACPI-to-OS layer*/
#include "acpixf.h"             /* ACPI core subsystem external interfaces */
#include "acobject.h"           /* ACPI internal object */
#include "acstruct.h"           /* Common structures */
#include "acglobal.h"           /* All global variables */
#include "achware.h"            /* Hardware defines and interfaces */
#include "accommon.h"           /* Common interfaces */


#endif /* __ACPI_H__ */
