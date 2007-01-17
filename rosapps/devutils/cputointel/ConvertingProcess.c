#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"

/* hack should be in misc.h*/


CPU_INT ConvertProcess(FILE *outfp, CPU_INT FromCpuid, CPU_INT ToCpuid)
{
    CPU_INT ret=0;
   CPU_INT eax =-1;
   CPU_INT ebp =-1;
   CPU_INT edx =-1;
   CPU_INT esp =-1;
   CPU_INT regbits=-1;
   CPU_INT HowManyRegInUse = 0;

   PMYBrainAnalys pMystart = pStartMyBrainAnalys;
   PMYBrainAnalys pMyend = pMyBrainAnalys;

   if (FromCpuid == IMAGE_FILE_MACHINE_POWERPC)
    {
        regbits = 32 / 8;
        esp = 1;
        eax = 3;
        edx = 4;
        ebp = 31;
    }


    /* FIXME calc where todo first split */

    /* FIXME calc who many register are in use */

    //ret = ConvertToIntelProcess(FILE *outfp,
    //                      CPU_INT eax,
    //                      CPU_INT edx,
    //                      CPU_INT edx,
    //                      CPU_INT esp,
    //                      PMYBrainAnalys start,
    //                      PMYBrainAnalys end);



    switch (ToCpuid)
    {
        case IMAGE_FILE_MACHINE_I386:
             ret = ConvertToIA32Process( outfp, eax, ebp,
                                edx, esp, 
                               pMystart, 
                               pMyend, regbits,
                               HowManyRegInUse);
             if (ret !=0)
             {
                 printf("should not happen contact a devloper, x86 fail\n");
                 return -1;
             }
             break;

        case IMAGE_FILE_MACHINE_POWERPC:
             ret = ConvertToPPCProcess( outfp, eax, ebp,
                                edx, esp, 
                               pMystart, 
                               pMyend, regbits,
                               HowManyRegInUse);
             if (ret !=0)
             {
                 printf("should not happen contact a devloper, x86 fail\n");
                 return -1;
             }
             break;

        default:
            printf("should not happen contact a devloper, unknown fail\n");
            return -1;
    }

    return ret;
}
