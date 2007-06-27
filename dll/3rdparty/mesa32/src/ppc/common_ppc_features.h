/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file common_ppc_features.h
 * Interface for determining which CPU features were detected.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#ifndef COMMON_PPC_FEATURES_H
#define COMMON_PPC_FEATURES_H

#ifdef USE_PPC_ASM
#include <asm/cputable.h>

extern unsigned long _mesa_ppc_cpu_features;


/* The PPC_FEATURE_* values come from asm/cputable.h.  Should we define
 * versions of them here if that file does not exist?  This will only
 * matter once these code paths are supported on non-Linux platforms.
 */

#define cpu_has_64  ((_mesa_ppc_cpu_features & PPC_FEATURE_64) != 0)
#define cpu_has_vmx ((_mesa_ppc_cpu_features & PPC_FEATURE_HAS_ALTIVEC) != 0)
#define cpu_has_fpu ((_mesa_ppc_cpu_features & PPC_FEATURE_HAS_FPU) != 0)

#endif /* USE_PPC_ASM */
#endif /* COMMON_PPC_FEATURES_H */
