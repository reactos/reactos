
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

/*
 * eax = register 3
 * edx = register 4
 * esp = register 1
 * ebp = register 31

 * ecx = 8
 * ebx = 9
 * esi = 10
 * edi = 11
 * mmx/sse/fpu 0 = 12
 * mmx/sse/fpu 1 = 14
 * mmx/sse/fpu 2 = 16
 * mmx/sse/fpu 3 = 18
 * mmx/sse/fpu 4 = 20
 * mmx/sse/fpu 5 = 22
 * mmx/sse/fpu 6 = 24
 * mmx/sse/fpu 7 = 28
 */

static void standardreg(CPU_INT *RegTableCount, CPU_INT reg, CPU_INT setup_ebp, FILE *outfp)
{
    /* eax */
    if (reg == RegTableCount[3])
    {
        fprintf(outfp,"eax");
    }
    /* ebp */
    else if (reg == RegTableCount[31])
    {
        fprintf(outfp,"ebp");
    }
    /* edx */
    else if (reg == RegTableCount[4])
    {
        fprintf(outfp,"edx");
    }
    /* esp */
    else if (reg == RegTableCount[1])
    {
        fprintf(outfp,"esp");
    }
    /* ecx */
    else if (reg == RegTableCount[8])
    {
        fprintf(outfp,"ecx");
    }
    /* ebx */
    else if (reg == RegTableCount[9])
    {
        fprintf(outfp,"ebx");
    }
    /* esi */
    else if (reg == RegTableCount[10])
    {
        fprintf(outfp,"esi");
    }
    /* edi */
    else if (reg == RegTableCount[11])
    {
        fprintf(outfp,"edi");
    }
    else
    {
        if (setup_ebp == 1)
            fprintf(outfp,"dword [ebx - %d]");
        else
            fprintf(outfp,"; unsuported should not happen it happen :(\n");
    }
}

CPU_INT ConvertToIA32Process( FILE *outfp,
                               PMYBrainAnalys pMystart,
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse,
                               CPU_INT *RegTableCount)
{

    CPU_INT stack = 0;
    CPU_UNINT tmp;
    CPU_INT setup_ebp = 0 ; /* 0 = no, 1 = yes */
    CPU_INT t=0;

    /* Fixme optimze the RegTableCount table  */

    //if (HowManyRegInUse > 9)
    if (HowManyRegInUse > 8)
    {
        setup_ebp =1; /* we will use ebx as ebp */
        stack = HowManyRegInUse * regbits;
    }

    if (RegTableCount[1]!=0)
        t++;
    if (RegTableCount[3]!=0)
        t++;
    if (RegTableCount[4]!=0)
        t++;
    if (RegTableCount[8]!=0)
        t++;
    if (RegTableCount[9]!=0)
        t++;
    if (RegTableCount[10]!=0)
        t++;
    if (RegTableCount[11]!=0)
        t++;
    if (RegTableCount[31]!=0)
        t++;

    if (HowManyRegInUse != t)
    {
        /* fixme optimze the table or active the frame pointer */
        setup_ebp =1; /* we will use ebx as ebp */
        stack = HowManyRegInUse * regbits;
    }

    fprintf(outfp,"BITS 32\n");
    fprintf(outfp,"GLOBAL _main\n");
    fprintf(outfp,"SECTION .text\n\n");
    fprintf(outfp,"; compile with nasm filename.asm -f win32, ld filename.obj -o filename.exe\n\n");
    fprintf(outfp,"_main:\n");

    /* setup a frame pointer */

    if (setup_ebp == 1)
    {
        fprintf(outfp,"\n; Setup frame pointer \n");
        fprintf(outfp,"push ebx\n");
        fprintf(outfp,"mov  ebx,esp\n");
        fprintf(outfp,"sub  esp, %d ; Alloc %d bytes for reg\n\n",stack,stack);
    }


    fprintf(outfp,"; Start the program \n");
    while (pMystart!=NULL)
    {
        /* fixme the line lookup from anaylysing process */

        /* mov not full implement */
        if (pMystart->op == OP_ANY_mov)
        {
            printf("waring OP_ANY_mov are not full implement\n");

            if ((pMystart->type & 8)== 8)
            {
                /* dst are register */
                tmp = stack - (pMystart->dst*regbits);

                if ((pMystart->type & 2)== 2)
                {
                        fprintf(outfp,"mov ");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp," , ");
                        standardreg( RegTableCount,
                                     pMystart->src,
                                     setup_ebp, outfp);
                        fprintf(outfp,"\n");

                }
                if ((pMystart->type & 16)== 16)
                {
                    /* source are imm */
                    if ((pMystart->src == 0) &&
                        (setup_ebp == 0))
                    {
                        /* small optimze */
                        fprintf(outfp,"xor ");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp,",");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp,"\n");
                    }
                    else
                    {
                        fprintf(outfp,"mov ");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp,",%llu\n",pMystart->src);
                    }
                } /* end "source are imm" */
            } /* end pMyBrainAnalys->type & 8 */

            if ((pMystart->type & 64)== 64)
            {
                if ((pMystart->type & 2)== 2)
                {
                    /* dest [eax - 0x20], source reg */

                    fprintf(outfp,"mov dword [");
                    standardreg( RegTableCount,
                                 pMystart->dst,
                                 setup_ebp, outfp);
                    if (pMystart->dst_extra>=0)
                        fprintf(outfp," +%d], ",pMystart->dst_extra);
                    else
                        fprintf(outfp," %d], ",pMystart->dst_extra);

                    standardreg( RegTableCount,
                                 pMystart->src,
                                 setup_ebp, outfp);
                    fprintf(outfp,"\n");

                   if ((pMystart->type & 128)== 128)
                   {
                        fprintf(outfp,"mov ");
                        standardreg( RegTableCount,
                                     pMystart->src,
                                     setup_ebp, outfp);
                        fprintf(outfp," , ");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp," %d\n",pMystart->dst_extra);
                   }
                }
            }




        }

        /* return */
        if (pMystart->op == OP_ANY_ret)
        {
            if (pMyBrainAnalys->ptr_next == NULL)
            {
               if (setup_ebp == 1)
               {
                    fprintf(outfp,"\n; clean up after the frame \n");
                    fprintf(outfp,"mov esp, ebx\n");
                    fprintf(outfp,"pop ebx\n");
               }
            }
            fprintf(outfp,"ret\n");
        }
        if (pMystart == pMyend)
            pMystart=NULL;
        else
            pMystart = (PMYBrainAnalys) pMystart->ptr_next;

    }
    return 0;
}
