/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __dtou64_worker
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#define __fto64_worker __dtou64_worker
#define _USE_64_BITS_

#include "__fto64_worker.h"

/* __dtou64 is implemented in __dtou64.s */
