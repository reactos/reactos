
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

CPU_INT ConvertToIntelProcess( FILE *outfp, CPU_INT eax, CPU_INT ebp,
                               CPU_INT edx, CPU_INT esp, 
                               PMYBrainAnalys pMystart, 
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse)
{

    CPU_INT stack = 0;
    CPU_UNINT tmp;
    CPU_INT setup_ebp = 0 ; /* 0 = no, 1 = yes */

    if (HowManyRegInUse > 8)
    {
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

                    if (pMystart->dst == eax)
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor eax,eax\n");
                        else
                            fprintf(outfp,"mov eax,%llu\n",pMystart->src);
                    }
                    else if (pMystart->dst == ebp)
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor ebp,ebp\n");
                        else
                            fprintf(outfp,"mov ebp,%llu\n",pMystart->src);
                    }
                    else if (pMystart->dst == edx)
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor edx,edx\n");
                        else
                            fprintf(outfp,"mov edx,%llu\n",pMystart->src);
                    }
                    else if (pMystart->dst == esp)
                    {
                        if (pMystart->src == 0)
                            fprintf(outfp,"xor esp,esp\n");
                        else
                            fprintf(outfp,"mov esp,%llu\n",pMystart->src);
                    }
                    else
                    {
                        fprintf(outfp,"mov dword [ebx - %d], %llu\n", tmp, pMystart->src);
                    }
                }
            } /* end pMyBrainAnalys->type & 8 */
        }

        /* return */
        if (pMystart->op == OP_ANY_ret)
        {
            if (pMyBrainAnalys->ptr_next == NULL)
            {
               fprintf(outfp,"\n; clean up after the frame \n");
               fprintf(outfp,"mov esp, ebx\n");
               fprintf(outfp,"pop ebx\n");
            }
            fprintf(outfp,"ret\n");
        }
        pMystart = (PMYBrainAnalys) pMystart->ptr_next;
    }
    return 0;
}
