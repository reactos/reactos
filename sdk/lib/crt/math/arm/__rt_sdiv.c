/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __rt_sdiv
 * COPYRIGHT:   Copyright 2015 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#define __rt_div_worker __rt_sdiv
#define _SIGNED_DIV_

#include "__rt_div_worker.h"

/* __rt_sdiv is implemented in __rt_div_worker.h */
