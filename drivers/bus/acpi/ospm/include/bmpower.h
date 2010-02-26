/*****************************************************************************
 *
 * Module name: bmpower.h
 *   $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 Andrew Grover
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

#ifndef __BMPOWER_H__
#define __BMPOWER_H__

#include "bm.h"


/*****************************************************************************
 *                               Types & Defines
 *****************************************************************************/


/*
 * BM_POWER_RESOURCE:
 * ------------------
 */
typedef struct
{
	BM_HANDLE           device_handle;
	ACPI_HANDLE         acpi_handle;
	BM_POWER_STATE      system_level;
	u32                 resource_order;
	BM_POWER_STATE      state;
	u32                 reference_count;
} BM_POWER_RESOURCE;


/*****************************************************************************
 *                             Function Prototypes
 *****************************************************************************/

/* bmpower.c */

ACPI_STATUS
bm_pr_initialize (void);

ACPI_STATUS
bm_pr_terminate (void);

ACPI_STATUS
bm_pr_list_get_state (
	BM_HANDLE_LIST          *resource_list,
	BM_POWER_STATE          *power_state);

ACPI_STATUS
bm_pr_list_transition (
	BM_HANDLE_LIST          *current_list,
	BM_HANDLE_LIST          *target_list);


#endif  /* __BMPOWER_H__ */
