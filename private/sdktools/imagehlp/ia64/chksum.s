
#include "ksia64.h"

        LEAF_ENTRY (ChkSum)

        alloc    t22 = ar.pfs, 3, 0, 0, 0
        mov      t10 = 0xffff
        zxt4     a2 = a2
        ;;

        cmp4.eq  pt0 = zero, a2
        zxt4     a0 = a0
(pt0)   br.cond.spnt cs20

cs10:
        ld2      t0 = [a1], 2
        add      a2 = -1, a2
        ;;
        add      a0 = t0, a0
        ;;

        cmp4.ne  pt1 = zero, a2
        extr.u   t1 = a0, 16, 16
        and      t2 = a0, t10
        ;;

        add      a0 = t1, t2
        nop.f    0
(pt1)   br.cond.sptk cs10
        ;;
        
cs20:
  
        nop.m    0
        extr.u   t1 = a0, 16, 16
        ;;
        add      a0 = t1, a0
        ;;

        nop.m    0
        and      v0 = a0, t10
        br.ret.sptk brp

        LEAF_EXIT (ChkSum)
