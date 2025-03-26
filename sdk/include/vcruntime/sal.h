/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Standard Annotation Language (SAL) definitions
 * COPYRIGHT:   2021 - Jérôme Gardou
 */

#pragma once

/* Include MS version first */
#include "ms_sal.h"

/* Some overrides with GCC attributes */
#ifdef __GNUC__
#include "gcc_sal.h"
#endif
