
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

static void standardreg(CPU_INT *RegTableCount, CPU_UNINT reg,
                        CPU_INT setup_ebp, FILE *outfp)
{
    CPU_INT t, found = 0;
    for (t=0;t<31;t++)
    {
        if (reg == RegTableCount[t])
        {
            fprintf(outfp,"r%d",t);
            found++;
            break;
        }
    }

    if (found == 0)
    {
        fprintf(outfp,"r%d",reg);
    }
}

CPU_INT ConvertToPPCProcess( FILE *outfp,
                               PMYBrainAnalys pMystart,
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse,
                               CPU_INT *RegTableCount)
{

    CPU_INT stack = 0;
    //CPU_UNINT tmp;
    CPU_INT setup_ebp = 0 ; /* 0 = no, 1 = yes */
    CPU_INT t=0;

    if (HowManyRegInUse > 31)
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


/*  fixme gas compatible
    fprintf(outfp,"BITS 32\n");
    fprintf(outfp,"GLOBAL _main\n");
    fprintf(outfp,"SECTION .text\n\n");
    fprintf(outfp,"; compile with nasm filename.asm -f win32, ld filename.obj -o filename.exe\n\n");
    fprintf(outfp,"_main:\n");
*/

    /* setup a frame pointer */
    if (setup_ebp == 1)
    {
        /* fixme ppc frame pointer */
        // fprintf(outfp,"\n; Setup frame pointer \n");
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
                // FIXME frame pointer setup
                // tmp = stack - (pMystart->dst*regbits);

                if ((pMystart->type & 2)== 2)
                {
                    fprintf(outfp,"mr ");
                    standardreg( RegTableCount,
                                 pMystart->dst,
                                 setup_ebp, outfp);
                    fprintf(outfp,",");
                    standardreg( RegTableCount,
                                 pMystart->src,
                                 setup_ebp, outfp);
                    fprintf(outfp,"\n");
                }

                if ((pMystart->type & 16)== 16)
                {
                    /* source are imm */
                    if (setup_ebp == 1)
                        fprintf(outfp,"not supporet\n");
                    else
                    {
                        fprintf(outfp,"li ");
                        standardreg( RegTableCount,
                                     pMystart->dst,
                                     setup_ebp, outfp);
                        fprintf(outfp," , %llu\n",pMystart->src);
                    }
                }
            } /* end pMyBrainAnalys->type & 8 */

            if ((pMystart->type & 64)== 64)
            {
                if ((pMystart->type & 2)== 2)
                {
                    /* dest [eax - 0x20], source reg */
                    if ((pMystart->type & 128)== 128)
                    {
                         fprintf(outfp,"stwu ");
                    }
                    else
                    {
                        fprintf(outfp,"stw ");
                    }

                    standardreg( RegTableCount,
                                 pMystart->src,
                                 setup_ebp, outfp);
                    fprintf(outfp,", %d(",pMystart->dst_extra);

                    standardreg( RegTableCount,
                                 pMystart->dst,
                                 setup_ebp, outfp);
                    fprintf(outfp,")\n");
                }
            } /* end pMyBrainAnalys->type & 64 */
        }

        /* return */
        if (pMystart->op == OP_ANY_ret)
        {
            if (pMyBrainAnalys->ptr_next == NULL)
            {
               if (setup_ebp == 1)
               {
                    // FIXME end our own frame pointer
                    fprintf(outfp,"\n; clean up after the frame \n");
               }
            }
            fprintf(outfp,"blr\n");
        }
        if (pMystart == pMyend)
            pMystart=NULL;
        else
            pMystart = (PMYBrainAnalys) pMystart->ptr_next;
    }
    return 0;
}
