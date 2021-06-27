/*
* PROJECT:     ReactOS Hardware Abstraction Layer
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     Defines HalpBuildType for either UP or SMP
* COPYRIGHT:   Copyright 2021 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

/* INCLUDES *****************************************************************/

#include <hal.h>

/* GLOBALS ******************************************************************/

const USHORT HalpBuildType = HAL_BUILD_TYPE;
