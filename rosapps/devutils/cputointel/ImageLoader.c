#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "From/ARM/ARM.h"
#include "From/m68k/m68k.h"
#include "From/PPC/PPC.h"

static  CPU_INT machine_type = 0;
//static  CPU_INT ToMachine_type = IMAGE_FILE_MACHINE_I386;
static  CPU_INT ToMachine_type = IMAGE_FILE_MACHINE_POWERPC;
/*
 * infileName       file name to convert or disambler 
 * outputfileName   file name to save to
 * BaseAddress      the address we should emulate
 * cpuid            the cpu we choice not vaild for pe loader
 * type             the loading mode Auto, PE, bin
 * mode             disambler mode : 0 the arch cpu.
 *                  translate mode : 1 intel
 *                  translate mode : 2 ppc
 * 
 */

static void SetCPU(CPU_INT FromCpu, CPU_INT mode)
{
    machine_type = FromCpu;
    switch(mode)
    {
        case 0:
            ToMachine_type = machine_type;
            break;

        case 1:
            ToMachine_type = IMAGE_FILE_MACHINE_I386;
            break;

        case 2:
            ToMachine_type = IMAGE_FILE_MACHINE_POWERPC;
            break;

        default:
            printf("Not supported mode\n");
            break;

    }
}

static void Convert(FILE *outfp, CPU_INT FromCpu, CPU_INT mode)
{
    SetCPU(machine_type,mode);
    AnyalsingProcess();
    ConvertProcess(outfp, machine_type, ToMachine_type);
    FreeAny();
}


CPU_INT LoadPFileImage( char *infileName, char *outputfileName, 
                     CPU_UNINT BaseAddress, char *cpuid,
                     CPU_UNINT type, CPU_INT mode)
{
    FILE *infp;
    FILE *outfp;
    CPU_BYTE *cpu_buffer;   
    CPU_UNINT cpu_pos = 0;
    CPU_UNINT cpu_size=0;
    CPU_INT ret;
     //fopen("testms.exe","RB");
    

    /* Open file for read */

    if (!(infp = fopen(infileName, "rb")))
    {
        printf("Can not open file %s\n",infileName);
        return 3;
    }

    /* Open file for write */
    if (!(outfp = fopen(outputfileName,"wb")))
    {
        printf("Can not open file %s\n",outputfileName);
        return 4;
    }

    /* Load the binary file to a memory buffer */
    fseek(infp,0,SEEK_END);
    if (ferror(infp))
    {
        printf("error can not seek in the read file");
        fclose(infp);
        fclose(outfp);
        return 5;
    }
    
    /* get the memory size buffer */
    cpu_size = ftell(infp);
    if (ferror(infp))
    {
        printf("error can not get file size of the read file");
        fclose(infp);
        fclose(outfp);
        return 6;
    }

    /* Load the binary file to a memory buffer */
    fseek(infp,0,SEEK_SET);
    if (ferror(infp))
    {
        printf("error can not seek in the read file");
        fclose(infp);
        fclose(outfp);
        return 5;
    }

    if (cpu_size==0)
    {
        printf("error file size is Zero lenght of the read file");
        fclose(infp);
        fclose(outfp);
        return 7;
    }

    /* alloc memory now */
   ;
    if (!(cpu_buffer = (unsigned char *) malloc(cpu_size+1)))
    {
        printf("error can not alloc %uld size for memory buffer",cpu_size);
        fclose(infp);
        fclose(outfp);
        return 8;
    }
    ZeroMemory(cpu_buffer,cpu_size);

    /* read from the file now in one sweep */
    fread((void *)cpu_buffer,1,cpu_size,infp);
    if (ferror(infp))
    {
        printf("error can not read file ");
        fclose(infp);
        fclose(outfp);
        return 9;
    }
    fclose(infp);

    if (type==0) 
    {
       if ( PEFileStart(cpu_buffer, 0, BaseAddress, cpu_size, outfp, mode) !=0)
       {
            type=1;
       }
       else
       {
            if (mode > 0)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return 0;
       }

       /* fixme */
       return -1;
    }

    if (type== 1)
    {
        if (stricmp(cpuid,"m68000"))
        {
            ret = M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68000,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
        }
        else if (stricmp(cpuid,"m68010"))
        {
            ret = M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68010,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
        else if (stricmp(cpuid,"m68020"))
        {
            ret = M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68020,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
        else if (stricmp(cpuid,"m68030"))
        {
            ret = M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68030,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
        else if (stricmp(cpuid,"m68040"))
        {
            ret = M68KBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,68040,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
        else if (stricmp(cpuid,"ppc"))
        {
            ret = PPCBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,0,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
        else if (stricmp(cpuid,"arm4"))
        {
            ret = ARMBrain(cpu_buffer,cpu_pos,cpu_size,BaseAddress,4,outfp);
            if (mode > 1)
            {
                Convert(outfp,machine_type,mode);
            }
            fclose(outfp);
            return ret;
        }
    }

    if (type==2) 
    {

       ret = PEFileStart(cpu_buffer, 0, BaseAddress, cpu_size, outfp, mode);
       if (mode > 1)
       {
           Convert(outfp,machine_type,mode);
       }
       fclose(outfp);
       return ret;
    }

    return 0;
}

CPU_INT PEFileStart( CPU_BYTE *memory, CPU_UNINT pos,
                     CPU_UNINT base,  CPU_UNINT size,
                     FILE *outfp, CPU_INT mode)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    INT NumberOfSections;
    INT NumberOfSectionsCount=0;

    DosHeader = (PIMAGE_DOS_HEADER)memory;
    if ( (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) ||
         (size < 0x3c+2) )
    {
        printf("No MZ file \n");
        return -1;
    }

    NtHeader = (PIMAGE_NT_HEADERS) (((ULONG)memory) + ((ULONG)DosHeader->e_lfanew));
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
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_EFI_APPLICATION\n");
             printf("This exe file is desgin run in EFI bios as applactions\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER\n");
             printf("This exe file is desgin run in EFI bios as service driver\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_ROM:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_EFI_ROM\n");
             printf("This exe file is EFI ROM\n");
             break;
        case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER\n");
             printf("This exe file is desgin run in EFI bios as driver\n");
             break;
        case IMAGE_SUBSYSTEM_NATIVE:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_NATIVE\n");
             printf("This exe file does not need any subsystem\n");
             break;
        case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_NATIVE_WINDOWS\n");
             printf("This exe file is desgin run on Windows 9x as driver \n");
             break;
        case IMAGE_SUBSYSTEM_OS2_CUI:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_OS2_CUI\n");
             printf("This exe file is desgin run on OS2 as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_POSIX_CUI\n");
             printf("This exe file is desgin run on POSIX as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_WINDOWS_CE_GUI\n");
             printf("This exe file is desgin run on Windows CE as GUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_WINDOWS_CUI\n");
             printf("This exe file is desgin run on Windows as CUI\n");
             break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_WINDOWS_GUI\n");
             printf("This exe file is desgin run on Windows as GUI\n");
             break;
        case IMAGE_SUBSYSTEM_XBOX:
             fprintf(outfp,"; OS type : IMAGE_SUBSYSTEM_XBOX\n");
             printf("This exe file is desgin run on X-Box\n");
             break;
        default:
            fprintf(outfp,"; OS type : Unknown\n");
            printf("Unknown OS : SubID : %d\n",NtHeader->OptionalHeader.Subsystem);
            break;
    }

    /*
    SectionHeader->Name == ".tls$"
    SectionHeader->Name == ".tls"
    SectionHeader->Name == ".text"  // Executable code 
    SectionHeader->Name == ".sxdata"
    SectionHeader->Name == ".sdata"
    SectionHeader->Name == ".sbss"
    SectionHeader->Name == ".rsrc" // rc data
    SectionHeader->Name == ".reloc"
    SectionHeader->Name == ".rdata" // read only initialized data
    SectionHeader->Name == ".pdata"
    SectionHeader->Name == ".idlsym" 
    SectionHeader->Name == ".idata" // Import tables 
    SectionHeader->Name == ".edata" // Export tables 
    SectionHeader->Name == ".drective"
    SectionHeader->Name == ".debug$T"
    SectionHeader->Name == ".debug$S"
    SectionHeader->Name == ".debug$P"
    SectionHeader->Name == ".debug$F"
    SectionHeader->Name == ".data"  //data segment 
    SectionHeader->Name == ".cormeta"
    SectionHeader->Name == ".bss"  // bss segment 

    undoc
    SectionHeader->Name == ".textbss"  // bss segment 
    */

    //*base =  NtHeader->OptionalHeader.AddressOfEntryPoint;

    SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
    NumberOfSections = NtHeader->FileHeader.NumberOfSections;

    for (NumberOfSectionsCount = 0; NumberOfSectionsCount < NumberOfSections; NumberOfSectionsCount++, SectionHeader++)
    {
       if (strnicmp((PCHAR) SectionHeader->Name,".rsrc",5)==0)
       {
           /* FIXME add a rc bin to text scanner */
       }

       else if (strnicmp((PCHAR) SectionHeader->Name,".textbss",8)==0)
       {
          /* FIXME add a bss to text scanner */
       }

       
       else if (strnicmp((PCHAR) SectionHeader->Name,".text\0",6)==0)
       {
           /*
                FIXME we should output gas syntax 
                BITS 32
                GLOBAL _lrotate;
                EXTERN _printf;
                COMMON _commvar 4;
           */

            switch (NtHeader->FileHeader.Machine)
            {
                case IMAGE_FILE_MACHINE_ALPHA:
                     printf("CPU ALPHA Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found Alpha\n");
                     machine_type = IMAGE_FILE_MACHINE_ALPHA;
                     return 3;

                case IMAGE_FILE_MACHINE_ALPHA64:
                     printf("CPU ALPHA64/AXP64 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found Alpha64/AXP64\n");
                     machine_type = IMAGE_FILE_MACHINE_ALPHA64;
                     return 3;

                case IMAGE_FILE_MACHINE_AM33:
                     printf("CPU AM33 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found AM33\n");
                     machine_type = IMAGE_FILE_MACHINE_AM33;
                     return 3;

                case IMAGE_FILE_MACHINE_AMD64:
                     printf("CPU AMD64 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found AMD64\n");
                     machine_type = IMAGE_FILE_MACHINE_AMD64;
                     return 3;

                case IMAGE_FILE_MACHINE_ARM:
                     printf("CPU ARM Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found ARM\n");
                     machine_type = IMAGE_FILE_MACHINE_ARM;
                     return 3;

                case IMAGE_FILE_MACHINE_CEE:
                     printf("CPU CEE Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found CEE\n");
                     machine_type = IMAGE_FILE_MACHINE_CEE;
                     return 3;

                case IMAGE_FILE_MACHINE_CEF:
                     printf("CPU CEF Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found CEF\n");
                     machine_type = IMAGE_FILE_MACHINE_CEF;
                     return 3;

                case IMAGE_FILE_MACHINE_EBC:
                     printf("CPU EBC Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found EBC\n");
                     machine_type = IMAGE_FILE_MACHINE_EBC;
                     return 3;

                case IMAGE_FILE_MACHINE_I386:
                     printf("CPU I386 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found I386\n");
                     machine_type = IMAGE_FILE_MACHINE_I386;
                     return 3;

                case IMAGE_FILE_MACHINE_IA64:
                     printf("CPU IA64 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found IA64\n");
                     machine_type = IMAGE_FILE_MACHINE_IA64;
                     return 3;

                case IMAGE_FILE_MACHINE_M32R:
                     printf("CPU M32R Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found M32R\n");
                     machine_type = IMAGE_FILE_MACHINE_M32R;
                     return 3;

                case IMAGE_FILE_MACHINE_MIPS16:
                     printf("CPU MIPS16 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found MIPS16\n");
                     machine_type = IMAGE_FILE_MACHINE_MIPS16;
                     return 3;

                case IMAGE_FILE_MACHINE_MIPSFPU:
                     printf("CPU MIPSFPU Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found MIPSFPU\n");
                     machine_type = IMAGE_FILE_MACHINE_MIPSFPU;
                     return 3;

                case IMAGE_FILE_MACHINE_MIPSFPU16:
                     printf("CPU MIPSFPU16 Detected no CPUBrain implement for it\n");
                     fprintf(outfp,"; CPU found MIPSFPU16\n");
                     machine_type = IMAGE_FILE_MACHINE_MIPSFPU16;
                     return 3;

               case IMAGE_FILE_MACHINE_POWERPC:
                    printf("CPU POWERPC Detected partily CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found POWERPC\n");
                         //PPCBrain(memory, pos, cpu_size, base, 0, outfp);
                    machine_type = IMAGE_FILE_MACHINE_POWERPC;
                    return PPCBrain(memory+SectionHeader->PointerToRawData,  0, SectionHeader->SizeOfRawData, 0, 0, outfp);


               case IMAGE_FILE_MACHINE_POWERPCFP:
                    printf("CPU POWERPCFP Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found POWERPCFP\n");
                    machine_type = IMAGE_FILE_MACHINE_POWERPCFP;
                    return 3;

               case IMAGE_FILE_MACHINE_R10000:
                    printf("CPU R10000 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found R10000\n");
                    machine_type = IMAGE_FILE_MACHINE_R10000;
                    return 3;

               case IMAGE_FILE_MACHINE_R3000:
                    printf("CPU R3000 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found R3000\n");
                    machine_type = IMAGE_FILE_MACHINE_R3000;
                    return 3;

               case IMAGE_FILE_MACHINE_R4000:
                    printf("CPU R4000 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found R4000\n");
                    machine_type = IMAGE_FILE_MACHINE_R4000;
                    return 3;

               case IMAGE_FILE_MACHINE_SH3:
                    printf("CPU SH3 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found SH3\n");
                    machine_type = IMAGE_FILE_MACHINE_SH3;
                    return 3;

               case IMAGE_FILE_MACHINE_SH3DSP:
                    printf("CPU SH3DSP Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found SH3DSP\n");
                    machine_type = IMAGE_FILE_MACHINE_SH3DSP;
                    return 3;

               case IMAGE_FILE_MACHINE_SH3E:
                    printf("CPU SH3E Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found SH3E\n");
                    machine_type = IMAGE_FILE_MACHINE_SH3E;
                    return 3;

               case IMAGE_FILE_MACHINE_SH4:
                    printf("CPU SH4 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found SH4\n");
                    machine_type = IMAGE_FILE_MACHINE_SH4;
                    return 3;

               case IMAGE_FILE_MACHINE_SH5:
                    printf("CPU SH5 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found SH5\n");
                    machine_type = IMAGE_FILE_MACHINE_SH5;
                    return 3;

               case IMAGE_FILE_MACHINE_THUMB:
                    printf("CPU THUMB Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found THUMB\n");
                    machine_type = IMAGE_FILE_MACHINE_THUMB;
                    return 3;

               case IMAGE_FILE_MACHINE_TRICORE:
                    printf("CPU TRICORE Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found TRICORE\n");
                    machine_type = IMAGE_FILE_MACHINE_TRICORE;
                    return 3;

               case IMAGE_FILE_MACHINE_WCEMIPSV2:
                    printf("CPU WCEMIPSV2 Detected no CPUBrain implement for it\n");
                    fprintf(outfp,"; CPU found WCEMIPSV2\n");
                    machine_type = IMAGE_FILE_MACHINE_WCEMIPSV2;
                    return 3;

               default:
                    printf("Unknown Machine : %d",NtHeader->FileHeader.Machine);
                    return 4;
            /* End case swich */
            }
       /* End if .text statment */
      }
    /* End for loop */
   } 

   return 0;
}
