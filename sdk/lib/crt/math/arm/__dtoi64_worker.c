/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __dtoi64_worker
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#define __fto64_worker __dtoi64_worker
#define _USE_64_BITS_
#define _USE_SIGNED_

#include "__fto64_worker.h"

/* __dtoi64 is implemented in __dtoi64.s */
