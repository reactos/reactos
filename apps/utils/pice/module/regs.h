/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    regs.h

Abstract:

    HEADER for disasm.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

#define REGGS           0
#define REGFS           1
#define REGES           2
#define REGDS           3
#define REGEDI          4
#define REGESI          5
#define REGEBX          6
#define REGEDX          7
#define REGECX          8
#define REGEAX          9
#define REGEBP          10
#define REGEIP          11
#define REGCS           12
#define REGEFL          13
#define REGESP          14
#define REGSS           15

#ifdef  KERNEL
#define REGCR0          16
#define REGCR2          17
#define REGCR3          18
#define REGCR4          19
#endif

#define REGDR0          20
#define REGDR1          21
#define REGDR2          22
#define REGDR3          23
#define REGDR6          24
#define REGDR7          25

#ifdef  KERNEL
#define REGGDTR         26
#define REGGDTL         27
#define REGIDTR         28
#define REGIDTL         29
#define REGTR           30
#define REGLDTR         31
#endif

// Pseudo-registers:
#define PREGEA          40
#define PREGBASE    PREGEA
#define PREGEXP         41
#define PREGRA          42
#define PREGP           43
#define PREGU0          44
#define PREGU1          45
#define PREGU2          46
#define PREGU3          47
#define PREGU4          48
#define PREGU5          49
#define PREGU6          50
#define PREGU7          51
#define PREGU8          52
#define PREGU9          53

#define FLAGBASE        100
#define REGDI           100
#define REGSI           101
#define REGBX           102
#define REGDX           103
#define REGCX           104
#define REGAX           105
#define REGBP           106
#define REGIP           107
#define REGFL           108
#define REGSP           109
#define REGBL           110
#define REGDL           111
#define REGCL           112
#define REGAL           113
#define REGBH           114
#define REGDH           115
#define REGCH           116
#define REGAH           117
#define FLAGIOPL        118
#define FLAGOF          119
#define FLAGDF          120
#define FLAGIF          121
#define FLAGTF          122
#define FLAGSF          123
#define FLAGZF          124
#define FLAGAF          125
#define FLAGPF          126
#define FLAGCF          127
#define FLAGVIP         128
#define FLAGVIF         129


#define REGFIR          REGEIP
