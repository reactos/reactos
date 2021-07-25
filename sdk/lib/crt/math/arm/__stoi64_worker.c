/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __stoi64_worker
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#define __fto64_worker __stoi64_worker
#define _USE_SIGNED_

#include "__fto64_worker.h"

/* __stoi64 is implemented in __stoi64.s */
