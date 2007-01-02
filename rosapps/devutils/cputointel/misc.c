
/* only for getting the pe struct */
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "ARM/ARM.h"
#include "m68k/m68k.h"
#include "PPC/PPC.h"


/* retun 
 * 0 = Ok 
 * 1 = unimplemt 
 * 2 = Unkonwn Opcode 
 * 3 = can not open read file
 * 4 = can not open write file
 * 5 = can not seek to end of read file
 * 6 = can not get the file size of the read file
 * 7 = read file size is Zero
 * 8 = can not alloc memory
 * 9 = can not read file
 *-------------------------
 * type 0 : auto
 * type 1 : bin
 * type 2 : exe/dll/sys
 */

CPU_INT LoadPFileImage( char *infileName, char *outputfileName, 
                     CPU_UNINT BaseAddress, char *cpuid,
                     CPU_UNINT type)
{
    FILE *infp;
    FILE *outfp;
    CPU_BYTE *cpu_buffer;   
    CPU_UNINT cpu_pos = 0;
    CPU_UNINT cpu_size=0;


    /* Open file for read */
    if (!(infp = fopen(infileName,"RB")))
    {
        printf("Can not open file %s\n",infileName);
        return 3;
    }

    /* Open file for write */
    if (!(outfp = fopen(outputfileName,"WB")))
    {
        printf("Can not open file %s\n",outputfileName);
        return 4;
    }

    /* Load the binary file to a memory buffer */
    fseek(infp,0,SEEK_END);
    if (!ferror(infp))
    {
        printf("error can not seek in the read file");
        fclose(infp);
        fclose(outfp);
        return 5;
    }
    
    /* get the memory size buffer */
    cpu_size = ftell(infp);
    if (!ferror(infp))
    {
        printf("error can not get file size of the read file");
        fclose(infp);
        fclose(outfp);
        return 6;
    }

    if (cpu_size==0)
    {
        printf("error file size is Zero lenght of the read file");
        fclose(infp);
        fclose(outfp);
        return 7;
    }

    /* alloc memory now */
    if (!(cpu_buffer = (unsigned char *) malloc(cpu_size)))
    {
        printf("error can not alloc %uld size for memory buffer",cpu_size);
        fclose(infp);
        fclose(outfp);
        return 8;
    }

    /* read from the file now in one sweep */
    fread(cpu_buffer,1,cpu_size,infp);
    if (!ferror(infp))
    {
        printf("error can not read file ");
        fclose(infp);
        fclose(outfp);
        return 9;
    }
    fclose(infp);

    if (type==0) 
    {
       if ( PEFileStart(cpu_buffer, 0, BaseAddress, cpu_size) !=0)
       {
            type=1;
       }
    }

    if (type== 1)
    {
        if (stricmp(cpuid,"m68000"))
                return M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68000,outfp);
        else if (stricmp(cpuid,"m68010"))
                return M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68010,outfp);
        else if (stricmp(cpuid,"m68020"))
                return M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68020,outfp);
        else if (stricmp(cpuid,"m68030"))
                return M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68030,outfp);
        else if (stricmp(cpuid,"m68040"))
                return M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68040,outfp);
        else if (stricmp(cpuid,"ppc"))
                return PPCBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,0,outfp);
        else if (stricmp(cpuid,"arm4"))
                return ARMBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,4,outfp);
    }

    if (type==2) 
    {
       return PEFileStart(cpu_buffer, 0, BaseAddress, cpu_size);

    }

    return 0;
}

CPU_INT PEFileStart( CPU_BYTE *memory, CPU_UNINT pos,
                     CPU_UNINT base,  CPU_UNINT size)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;

    DosHeader = (PIMAGE_DOS_HEADER)memory;
    if ( (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) ||
         (size < 0x3c+2) )
    {
        printf("No MZ file \n");
        return -1;
    }

    NtHeader = (PIMAGE_NT_HEADERS) memory+ DosHeader->e_lfanew;
    if (NtHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        printf("No PE header found \n");
    }

    if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        printf("No execute image found \n");
        return -1;
    }

    switch(NtHeader->OptionalHeader.Subsystem)
    {
        case IMAGE_SUBSYSTEM_EFI_APPLICATION:
             printf("This exe file is desgin run in EFI bios as applactions\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
             printf("This exe file is desgin run in EFI bios as service driver\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_ROM:
             printf("This exe file is EFI ROM\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
             printf("This exe file is desgin run in EFI bios as driver\n");
             break;
        case IMAGE_SUBSYSTEM_NATIVE:
             printf("This exe file does not need any subsystem\n");
             break;
        case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:
             printf("This exe file is desgin run on Windows 9x as driver \n");
             break;
        case IMAGE_SUBSYSTEM_OS2_CUI:
             printf("This exe file is desgin run on OS2 as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:
             printf("This exe file is desgin run on POSIX as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
             printf("This exe file is desgin run on Windows CE as GUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
             printf("This exe file is desgin run on Windows as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
             printf("This exe file is desgin run on Windows as GUI\n");
             break;
        case IMAGE_SUBSYSTEM_XBOX:
             printf("This exe file is desgin run on X-Box\n");
             break;
        default:
            printf("Unknown OS : SubID : %d\n",NtHeader->OptionalHeader.Subsystem);
            break;
    }

    //*base =  NtHeader->OptionalHeader.AddressOfEntryPoint;


    /* return */
    switch (NtHeader->FileHeader.Machine)
   {
        case IMAGE_FILE_MACHINE_ALPHA:
             printf("CPU ALPHA Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_ALPHA64:
             printf("CPU ALPHA64/AXP64 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_AM33:
             printf("CPU AM33 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_AMD64:
             printf("CPU AMD64 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_ARM:
             printf("CPU ARM Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_CEE:
             printf("CPU CEE Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_CEF:
             printf("CPU CEF Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_EBC:
             printf("CPU EBC Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_I386:
             printf("CPU I386 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_IA64:
             printf("CPU IA64 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_M32R:
             printf("CPU M32R Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_MIPS16:
             printf("CPU MIPS16 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_MIPSFPU:
             printf("CPU MIPSFPU Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_MIPSFPU16:
             printf("CPU MIPSFPU16 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_POWERPC:
             printf("CPU POWERPC Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_POWERPCFP:
             printf("CPU POWERPCFP Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_R10000:
             printf("CPU R10000 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_R3000:
             printf("CPU R3000 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_R4000:
             printf("CPU R4000 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_SH3:
             printf("CPU SH3 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_SH3DSP:
             printf("CPU SH3DSP Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_SH3E:
             printf("CPU SH3E Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_SH4:
             printf("CPU SH4 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_SH5:
             printf("CPU SH5 Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_THUMB:
             printf("CPU THUMB Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_TRICORE:
             printf("CPU TRICORE Detected no CPUBrain implement for it\n");
             return -1;

        case IMAGE_FILE_MACHINE_WCEMIPSV2:
             printf("CPU WCEMIPSV2 Detected no CPUBrain implement for it\n");
             return -1;

        default:
            printf("Unknown Machine : %d",NtHeader->FileHeader.Machine);
            return -1;
   }

}


/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}

/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte32(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte32(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}


