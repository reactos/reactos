
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

CPU_INT ConvertToIntelProcess(FILE *outfp,  CPU_INT cpuid)
{
    CPU_INT eax = 0;
    CPU_INT stack = 0;
    CPU_INT regbits = 0;
    CPU_UNINT tmp;

    pMyBrainAnalys = pStartMyBrainAnalys;

    if (cpuid == IMAGE_FILE_MACHINE_POWERPC)
    {
        regbits = 64 / 8;
        eax = 3;  /* eax = r3 */
        stack = 31 * regbits;  /* r0-r31 are 64bits reg  ? */
        /* exemple  :
         *          : [ebp - 256]  = r0 
         *          : [ebp - 248]  = r1 
         */
    }
    else
    {
        printf("not supported yet\n");
        return -1;
    }
    

    fprintf(outfp,"BITS 32\n");
    fprintf(outfp,"GLOBAL _main\n");
    fprintf(outfp,"SECTION .text\n\n");
    fprintf(outfp,"; compile with nasm filename.asm -f win32, gcc filename.obj -o filename.exe\n\n");
    fprintf(outfp,"_main:\n");

    /* setup a frame pointer */
    fprintf(outfp,"\n; Setup frame pointer \n");
    fprintf(outfp,"push ebp\n");
    fprintf(outfp,"mov  ebp,esp\n");
    fprintf(outfp,"sub  esp, %d ; Alloc %d bytes for reg\n\n",stack,stack);

    fprintf(outfp,"; Start the program \n");
    while (pMyBrainAnalys!=NULL)
    {
        /* fixme the line lookup from anaylysing process */

        /* mov not full implement */
        if (pMyBrainAnalys->op == OP_ANY_mov)
        {
            printf("waring OP_ANY_mov are not full implement\n");

            if ((pMyBrainAnalys->type & 8)== 8)
            {
                /* dst are register */
                tmp = stack - (pMyBrainAnalys->dst*regbits);

                if ((pMyBrainAnalys->type & 16)== 16)
                {
                    /* source are imm */
                    fprintf(outfp,"mov dword [ebp - %d], %llu\n", tmp, pMyBrainAnalys->src);
                    if (pMyBrainAnalys->dst == eax)
                    {
                        fprintf(outfp,"mov eax,[ebp - %d]\n", tmp);
                    }
                }
            } /* end pMyBrainAnalys->type & 8 */
        }

        /* return */
        if (pMyBrainAnalys->op == OP_ANY_ret)
        {
            if (pMyBrainAnalys->ptr_next == NULL)
            {
               fprintf(outfp,"\n; clean up after the frame \n");
               fprintf(outfp,"mov esp, ebp\n");
               fprintf(outfp,"pop ebp\n");
            }
            fprintf(outfp,"ret\n");
        }
        pMyBrainAnalys = (PMYBrainAnalys) pMyBrainAnalys->ptr_next;
    }
    return 0;
}
