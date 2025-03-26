/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Headers for ARM fpcsr
 * COPYRIGHT:   Copyright 2021 Roman Masanin <36927roma@gmail.com>
 */

#include <float.h>

#define ARM_CW_STATUS_MASK 0x9F
#define ARM_CW_IM          (1 << 0)      /* Invalid operation mask */
#define ARM_CW_ZM          (1 << 1)      /* Zero divide mask */
#define ARM_CW_OM          (1 << 2)      /* Overflow mask */
#define ARM_CW_UM          (1 << 3)      /* Underflow mask */
#define ARM_CW_PM          (1 << 4)      /* Precision mask */
#define ARM_CW_DM          (1 << 7)      /* Denormal operand mask */

#define ARM_CW_RC_NEAREST  0  /* round to nearest */
#define ARM_CW_RC_UP       1  /* round up */
#define ARM_CW_RC_DOWN     2  /* round down */
#define ARM_CW_RC_ZERO     3  /* round toward zero (chop) */

typedef union _ARM_FPSCR
{
    unsigned int raw;
    struct
    {
        unsigned int exception: 8;
        unsigned int ex_control: 8;
        unsigned int len: 3;
        unsigned int unused3: 1;
        unsigned int stride: 2;
        unsigned int rounding_mode: 2;
        unsigned int flush_to_zero: 1;
        unsigned int unused4: 3;
        unsigned int status_flag: 4;
    } data;
} ARM_FPSCR;

void __setfp(unsigned int);
unsigned int __getfp(void);
