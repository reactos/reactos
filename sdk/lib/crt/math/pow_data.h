/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Data tables for pow.
 * COPYRIGHT:   Imported from musl libc
 *              https://git.musl-libc.org/cgit/musl/tree/src/math/pow_data.h
 *              blob: 5d609ae80ec210badf3f705417e80d0f31da9f51
 *              See https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT
 */

/*
 * Copyright (c) 2018, Arm Limited.
 * SPDX-License-Identifier: MIT
 */
#ifndef _POW_DATA_H
#define _POW_DATA_H

#include "libm.h"

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8
extern hidden const struct pow_log_data {
	double ln2hi;
	double ln2lo;
	double poly[POW_LOG_POLY_ORDER - 1]; /* First coefficient is 1.  */
	/* Note: the pad field is unused, but allows slightly faster indexing.  */
	struct {
		double invc, pad, logc, logctail;
	} tab[1 << POW_LOG_TABLE_BITS];
} __pow_log_data;

#endif
