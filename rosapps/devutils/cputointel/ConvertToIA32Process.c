
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

                if ((pMystart->type & 16)== 16)
                {
                    /* source are imm */

                     /* 
                     * esi = 10
                     * edi = 11 */

                    /* eax */
                    if (pMystart->dst == RegTableCount[3])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor eax,eax\n");
                        else
                            fprintf(outfp,"mov eax,%llu\n",pMystart->src);
                    }
                    /* ebp */
                    else if (pMystart->dst == RegTableCount[31])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor ebp,ebp\n");
                        else
                            fprintf(outfp,"mov ebp,%llu\n",pMystart->src);
                    }
                    /* edx */
                    else if (pMystart->dst == RegTableCount[4])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor edx,edx\n");
                        else
                            fprintf(outfp,"mov edx,%llu\n",pMystart->src);
                    }
                    /* esp */
                    else if (pMystart->dst == RegTableCount[1])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor esp,esp\n");
                        else
                            fprintf(outfp,"mov esp,%llu\n",pMystart->src);
                    }
                    /* ecx */
                    else if (pMystart->dst == RegTableCount[8])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor ecx,ecx\n");
                        else
                            fprintf(outfp,"mov ecx,%llu\n",pMystart->src);
                    }
                    /* ebx */
                    else if (pMystart->dst == RegTableCount[9])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor ebx,ebx\n");
                        else
                            fprintf(outfp,"mov ebx,%llu\n",pMystart->src);
                    }
                    /* esi */
                    else if (pMystart->dst == RegTableCount[10])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor esi,esi\n");
                        else
                            fprintf(outfp,"mov esi,%llu\n",pMystart->src);
                    }
                    /* edi */
                    else if (pMystart->dst == RegTableCount[10])
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor edi,edi\n");
                        else
                            fprintf(outfp,"mov edi,%llu\n",pMystart->src);
                    }
                    else
                    {
                        if (setup_ebp == 1)
                            fprintf(outfp,"mov dword [ebx - %d], %llu\n", tmp, pMystart->src);
                        else
                        {
                            fprintf(outfp,"unsuported optimze should not happen it happen :(\n");
                        }
                    }
                }
            } /* end pMyBrainAnalys->type & 8 */
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
