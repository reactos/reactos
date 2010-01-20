/*
 * FILE:                  tools/nci/ncitool.c
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               Native Call Interface Support Tool
 * PURPOSE:               Generates NCI Tables and Stubs.
 * PROGRAMMER;            Alex Ionescu (alex@relsoft.net)
 * CHANGE HISTORY:        14/01/05 - Created. Based on original code by
 *                                   KJK::Hyperion and Emanuelle Aliberti.
 *
 */

/* INCLUDE ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__FreeBSD__) && !defined(__APPLE__)
# include <malloc.h>
#endif // __FreeBSD__

/* DEFINES  ****************************************************************/

#define INPUT_BUFFER_SIZE 255
#define Arguments 8

/******* Table Indexes ************/
#define MAIN_INDEX 0x0
#define WIN32K_INDEX 0x1000

/******* Argument List ************/
/* Databases */
#define NativeSystemDb 0
#define NativeGuiDb 1

/* Service Tables */
#define NtosServiceTable 2
#define Win32kServiceTable 3

/* Stub Files */
#define NtosUserStubs 4
#define NtosKernelStubs 5
#define Win32kStubs 6

/* Spec Files */
#define NtSpec 7

/********** Stub Code ************/

/*
 * This stubs calls into KUSER_SHARED_DATA where either a
 * sysenter or interrupt is performed, depending on CPU support.
 */
#if defined(__GNUC__)
#define UserModeStub_x86    "    movl $0x%x, %%eax\n" \
                            "    movl $KUSER_SHARED_SYSCALL, %%ecx\n" \
                            "    call *(%%ecx)\n" \
                            "    ret $0x%x\n\n"

#define UserModeStub_amd64  "    movl $0x%x, %%eax\n" \
                            "    movq %%rcx, %%r10\n" \
                            "    syscall\n" \
                            "    ret $0x%x\n\n"

#define UserModeStub_ppc    "    stwu 1,-16(1)\n" \
                            "    mflr 0\n\t" \
                            "    stw  0,0(1)\n" \
                            "    li   0,0x%x\n" \
                            "    sc\n" \
                            "    lwz 0,0(1)\n" \
                            "    mtlr 0\n" \
                            "    addi 1,1,16\n" \
                            "    blr\n"

#define UserModeStub_mips   "    li $8, KUSER_SHARED_SYSCALL\n" \
                            "    lw $8,0($8)\n" \
                            "    j $8\n" \
                            "    nop\n"

#define UserModeStub_arm    "    swi #0x%x\n"      \
                            "    bx lr\n\n"

#elif defined(_MSC_VER)
#define UserModeStub_x86    "    asm { \n" \
                            "        mov eax, %xh\n" \
                            "        mov ecx, KUSER_SHARED_SYSCALL\n" \
                            "        call [ecx]\n" \
                            "        ret %xh\n" \
                            "    }\n"
#else
#error Unknown compiler for inline assembler
#endif

/*
 * This stub calls KiSystemService directly with a fake INT2E stack.
 * Because EIP is pushed during the call, the handler will return here.
 */
#if defined(__GNUC__)
#define KernelModeStub_x86  "    movl $0x%x, %%eax\n" \
                            "    leal 4(%%esp), %%edx\n" \
                            "    pushfl\n" \
                            "    pushl $KGDT_R0_CODE\n" \
                            "    call _KiSystemService\n" \
                            "    ret $0x%x\n\n"

#define KernelModeStub_amd64 "    movl $0x%x, %%eax\n" \
                            "    call _KiSystemService\n" \
                            "    ret $0x%x\n\n"

/* For now, use the usermode stub.  We'll optimize later */
#define KernelModeStub_ppc  UserModeStub_ppc

#define KernelModeStub_mips "    j KiSystemService\n" \
                            "    nop\n"

#define KernelModeStub_arm  "    mov ip, lr\n"      \
                            "    swi #0x%x\n"      \
                            "    bx ip\n\n"

#elif defined(_MSC_VER)
#define KernelModeStub_x86  "    asm { \n" \
                            "        mov eax, %xh\n" \
                            "        lea edx, [esp+4]\n" \
                            "        pushf\n" \
                            "        push KGDT_R0_CODE\n" \
                            "        call _KiSystemService\n" \
                            "        ret %xh\n" \
                            "    }\n"
#else
#error Unknown compiler for inline assembler
#endif

/***** Arch Dependent Stuff ******/
struct ncitool_data_t {
    const char *arch;
    int args_to_bytes;
    const char *km_stub;
    const char *um_stub;
    const char *global_header;
    const char *declaration;
};

struct ncitool_data_t ncitool_data[] = {
    { "i386", 4, KernelModeStub_x86, UserModeStub_x86,
      ".global _%s@%d\n", "_%s@%d:\n" },
    { "amd64", 4, KernelModeStub_amd64, UserModeStub_amd64,
      ".global _%s\n", "_%s:\n" },
    { "powerpc", 4, KernelModeStub_ppc, UserModeStub_ppc,
      "\t.globl %s\n", "%s:\n" },
    { "mips", 4, KernelModeStub_mips, UserModeStub_mips,
      "\t.globl %s\n", "%s:\n" },
    { "arm", 4, KernelModeStub_arm, UserModeStub_arm,
    "\t.globl %s\n", "%s:\n" },
    { 0, }
};
int arch_sel = 0;
#define ARGS_TO_BYTES(x) (x)*(ncitool_data[arch_sel].args_to_bytes)
#define UserModeStub ncitool_data[arch_sel].um_stub
#define KernelModeStub ncitool_data[arch_sel].km_stub
#define GlobalHeader ncitool_data[arch_sel].global_header
#define Declaration ncitool_data[arch_sel].declaration

/* FUNCTIONS ****************************************************************/

/*++
 * WriteFileHeader
 *
 *     Prints out the File Header for a Stub File.
 *
 * Params:
 *     StubFile - Stub File to which to write the header.
 *
 *     FileDescription - Description of the Stub file to which to write the header.
 *
 *     FileLocation - Name of the Stub file to which to write the header.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     FileLocation is only used for printing the header.
 *
 *--*/
void
WriteFileHeader(FILE * StubFile,
                char* FileDescription,
                char* FileLocation)
{
    /* This prints out the file header */
    fprintf(StubFile,
            "/* FILE:            %s\n"
            " * COPYRIGHT:       See COPYING in the top level directory\n"
            " * PURPOSE:         %s\n"
            " * PROGRAMMER:      Computer Generated File. See tools/nci/ncitool.c\n"
            " * REMARK:          DO NOT EDIT OR COMMIT MODIFICATIONS TO THIS FILE\n"
            " */\n\n\n"
            "#include <ndk/asm.h>\n\n",
            FileDescription,
            FileLocation);
}

/*++
 * WriteFileHeader
 *
 *     Prints out the File Header for a Stub File.
 *
 * Params:
 *     StubFile - Stub File to which to write the header.
 *
 *     FileDescription - Description of the Stub file to which to write the header.
 *
 *     FileLocation - Name of the Stub file to which to write the header.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     FileLocation is only used for printing the header.
 *
 *--*/
void
WriteStubHeader(FILE* StubFile,
                char* SyscallName,
                unsigned StackBytes)
{
    /* Export the function */
    fprintf(StubFile, GlobalHeader, SyscallName, StackBytes);

    /* Define it */
    fprintf(StubFile, Declaration, SyscallName, StackBytes);
}


/*++
 * WriteKernelModeStub
 *
 *     Prints out the Kernel Mode Stub for a System Call.
 *
 * Params:
 *     StubFile - Stub File to which to write the header.
 *
 *     SyscallName - Name of System Call for which to add the stub.
 *
 *     StackBytes - Number of bytes on the stack to return after doing the system call.
 *
 *     SyscallId - Service Descriptor Table ID for this System Call.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     On i386, StackBytes is the number of arguments x 4.
 *
 *--*/
void
WriteKernelModeStub(FILE* StubFile,
                    char* SyscallName,
                    unsigned StackBytes,
                    unsigned int SyscallId)
{
    /* Write the Stub Header and export the Function */
    WriteStubHeader(StubFile, SyscallName, StackBytes);

    /* Write the Stub Code */
    fprintf(StubFile, KernelModeStub, SyscallId, StackBytes);
}

/*++
 * WriteUserModeStub
 *
 *     Prints out the User Mode Stub for a System Call.
 *
 * Params:
 *     StubFile - Stub File to which to write the header.
 *
 *     SyscallName - Name of System Call for which to add the stub.
 *
 *     StackBytes - Number of bytes on the stack to return after doing the system call.
 *
 *     SyscallId - Service Descriptor Table ID for this System Call.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     On i386, StackBytes is the number of arguments x 4.
 *
 *--*/
void
WriteUserModeStub(FILE* StubFile,
                  char* SyscallName,
                  unsigned StackBytes,
                  unsigned int SyscallId)
{
    /* Write the Stub Header and export the Function */
    WriteStubHeader(StubFile, SyscallName, StackBytes);

    /* Write the Stub Code */
    fprintf(StubFile, UserModeStub, SyscallId, StackBytes);
}

/*++
 * GetNameAndArgumentsFromDb
 *
 *     Parses an entry from a System Call Database, extracting
 *     the function's name and arguments that it takes.
 *
 * Params:
 *     Line - Entry from the Database to parse.
 *
 *     NtSyscallName - Output string to which to save the Function Name
 *
 *     SyscallArguments - Output string to which to save the number of
 *                        arguments that the function takes.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     On i386, StackBytes is the number of arguments x 4.
 *
 *--*/
void
GetNameAndArgumentsFromDb(char Line[],
                          char ** NtSyscallName,
                          char ** SyscallArguments)
{
    char *s;
    char *stmp;

    /* Remove new line */
    if ((s = (char *) strchr(Line,'\r')) != NULL) {
        *s = '\0';
    }

    /* Skip comments (#) and empty lines */
    s = &Line[0];
    if ((*s) != '#' && (*s) != '\0') {

        /* Extract the NtXXX name */
        *NtSyscallName = (char *)strtok(s," \t");

        /* Extract the argument count */
        *SyscallArguments = (char *)strtok(NULL," \t");

        /* Remove, if present, the trailing LF */
        if ((stmp = strchr(*SyscallArguments, '\n')) != NULL) {
            *stmp = '\0';
        }

    } else {

        /* Skip this entry */
        *NtSyscallName = NULL;
        *SyscallArguments = NULL;
    }
}

/*++
 * CreateStubs
 *
 *     Parses a System Call Database and creates stubs for all the entries.
 *
 * Params:
 *     SyscallDb - System Call Database to parse.
 *
 *     UserModeFiles - Array of Usermode Stub Files to which to write the stubs.
 *
 *     KernelModeFile - Kernelmode Stub Files to which to write the stubs.
 *
 *     Index - Number of first syscall
 *
 *     UserFiles - Number of Usermode Stub Files to create
 *
 *     NeedsZw - Write Zw prefix?
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
void
CreateStubs(FILE * SyscallDb,
            FILE * UserModeFiles[],
            FILE * KernelModeFile,
            unsigned Index,
            unsigned UserFiles,
            unsigned NeedsZw)
{
    char Line[INPUT_BUFFER_SIZE];
    char *NtSyscallName;
    char *SyscallArguments;
    int SyscallId;
    unsigned StackBytes;

    /* We loop, incrementing the System Call Index, until the end of the file  */
    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);
        if (SyscallArguments != NULL)
            StackBytes = ARGS_TO_BYTES(strtoul(SyscallArguments, NULL, 0));
        else
            StackBytes = 0;

        /* Make sure we really extracted something */
        if (NtSyscallName) {

            /* Create Usermode Stubs for Nt/Zw syscalls in each Usermode file */
            int i;
            for (i= 0; i < UserFiles; i++) {

                /* Write the Nt Version */
                WriteUserModeStub(UserModeFiles[i],
                                  NtSyscallName,
                                  StackBytes,
                                  SyscallId | Index);

                /* If a Zw Version is needed (was specified), write it too */
                if (NeedsZw) {

                    NtSyscallName[0] = 'Z';
                    NtSyscallName[1] = 'w';
                    WriteUserModeStub(UserModeFiles[i],
                                      NtSyscallName,
                                      StackBytes,
                                      SyscallId | Index);
                }

            }

            /* Create the Kernel coutnerparts (only Zw*, Nt* are the real functions!) */
            if (KernelModeFile) {

                NtSyscallName[0] = 'Z';
                NtSyscallName[1] = 'w';
                WriteKernelModeStub(KernelModeFile,
                                    NtSyscallName,
                                    StackBytes,
                                    SyscallId | Index);
            }

            /* Only increase if we actually added something */
            SyscallId++;
        }
    }
}

/*++
 * CreateSystemServiceTable
 *
 *     Parses a System Call Database and creates a System Call Service Table for it.
 *
 * Params:
 *     SyscallDb - System Call Database to parse.
 *
 *     SyscallTable - File in where to create System Call Service Table.
 *
 *     Name - Name of the Service Table.
 *
 *     FileLocation - Filename containing the Table.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     FileLocation is only used for the header generation.
 *
 *--*/
void
CreateSystemServiceTable(FILE *SyscallDb,
                         FILE *SyscallTable,
                         char * Name,
                         char * FileLocation)
{
    char Line[INPUT_BUFFER_SIZE];
    char *NtSyscallName;
    char *SyscallArguments;
    int SyscallId;

    /* Print the Header */
    WriteFileHeader(SyscallTable, "System Call Table for Native API", FileLocation);

    /* First we build the SSDT */
    fprintf(SyscallTable,"\n\n\n");
    fprintf(SyscallTable,"ULONG_PTR %sSSDT[] = {\n", Name);

    /* We loop, incrementing the System Call Index, until the end of the file */
    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);

        /* Make sure we really extracted something */
        if (NtSyscallName) {

            /* Add a new line */
            if (SyscallId > 0) fprintf(SyscallTable,",\n");

            /* Write the syscall name in the service table. */
            fprintf(SyscallTable,"\t\t(ULONG_PTR)%s", NtSyscallName);

            /* Only increase if we actually added something */
            SyscallId++;
        }
    }

    /* Close the service table (C syntax) */
    fprintf(SyscallTable,"\n};\n");

    /* Now we build the SSPT */
    rewind(SyscallDb);
    fprintf(SyscallTable,"\n\n\n");
    fprintf(SyscallTable,"UCHAR %sSSPT[] = {\n", Name);

    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);

        /* Make sure we really extracted something */
        if (NtSyscallName) {

            /* Add a new line */
            if (SyscallId > 0) fprintf(SyscallTable,",\n");

            /* Write the syscall arguments in the argument table. */
            if (SyscallArguments != NULL)
                fprintf(SyscallTable,"\t\t%lu * sizeof(void *)",strtoul(SyscallArguments, NULL, 0));
            else
                fprintf(SyscallTable,"\t\t0");

            /* Only increase if we actually added something */
            SyscallId++;
        }
    }

    /* Close the service table (C syntax) */
    fprintf(SyscallTable,"\n};\n");

    /*
     * We write some useful defines
     */
    fprintf(SyscallTable, "\n\n#define MIN_SYSCALL_NUMBER    0\n");
    fprintf(SyscallTable, "#define MAX_SYSCALL_NUMBER    %d\n", SyscallId - 1);
    fprintf(SyscallTable, "#define NUMBER_OF_SYSCALLS    %d\n", SyscallId);
    fprintf(SyscallTable, "ULONG %sNumberOfSysCalls = %d;\n", Name, SyscallId);
}

/*++
 * WriteSpec
 *
 *     Prints out the Spec Entry for a System Call.
 *
 * Params:
 *     SpecFile - Spec File to which to write the header.
 *
 *     SyscallName - Name of System Call for which to add the stub.
 *
 *     CountArguments - Number of arguments to the System Call.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
void
WriteSpec(FILE* StubFile,
          char* SyscallName,
          unsigned CountArguments)
{
    unsigned i;

    fprintf(StubFile, "@ stdcall %s", SyscallName);

    fputc ('(', StubFile);

    for (i = 0; i < CountArguments; ++ i)
        fputs ("ptr ", StubFile);

    fputc (')', StubFile);
    fputc ('\n', StubFile);
}

/*++
 * CreateSpec
 *
 *     Parses a System Call Database and creates a spec file for all the entries.
 *
 * Params:
 *     SyscallDb - System Call Database to parse.
 *
 *     Files - Array of Spec Files to which to write.
 *
 *     CountFiles - Number of Spec Files to create
 *
 *     UseZw - Use Zw prefix?
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
void
CreateSpec(FILE * SyscallDb,
           FILE * Files[],
           unsigned CountFiles,
           unsigned UseZw)
{
    char Line[INPUT_BUFFER_SIZE];
    char *NtSyscallName;
    char *SyscallArguments;
    unsigned CountArguments;

    /* We loop until the end of the file  */
    while ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL)) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);
        CountArguments = strtoul(SyscallArguments, NULL, 0);

        /* Make sure we really extracted something */
        if (NtSyscallName) {

            int i;
            for (i= 0; i < CountFiles; i++) {

                if (!UseZw) {
                    WriteSpec(Files[i],
                              NtSyscallName,
                              CountArguments);
                }

                if (UseZw && NtSyscallName[0] == 'N' && NtSyscallName[1] == 't') {

                    NtSyscallName[0] = 'Z';
                    NtSyscallName[1] = 'w';
                    WriteSpec(Files[i],
                              NtSyscallName,
                              CountArguments);
                }

            }
        }
    }
}

void usage(char * argv0)
{
    printf("Usage: %s [-arch <arch>] sysfuncs.lst w32ksvc.db napi.h ssdt.h napi.S zw.S win32k.S win32k.S\n"
           "  sysfuncs.lst  native system functions database\n"
           "  w32ksvc.db    native graphic functions database\n"
           "  napi.h        NTOSKRNL service table\n"
           "  ssdt.h        WIN32K service table\n"
           "  napi.S        NTDLL stubs\n"
           "  zw.S          NTOSKRNL Zw stubs\n"
           "  win32k.S      GDI32 stubs\n"
           "  win32k.S      USER32 stubs\n"
           "  nt.pspec      NTDLL exports\n"
           "  -arch is optional, default is %s\n",
           argv0,
           ncitool_data[0].arch
           );
}

int main(int argc, char* argv[])
{
    FILE * Files[Arguments] = { };
    int FileNumber, ArgOffset = 1;
    char * OpenType = "r";

    /* Catch architecture argument */
    if (argc > 3 && !strcmp(argv[1],"-arch")) {
        for( arch_sel = 0; ncitool_data[arch_sel].arch; arch_sel++ )
            if (strcmp(argv[2],ncitool_data[arch_sel].arch) == 0)
                break;
        if (!ncitool_data[arch_sel].arch) {
            printf("Invalid arch '%s'\n", argv[2]);
            usage(argv[0]);
            return 1;
        }
        ArgOffset = 3;
    }
    /* Make sure all arguments all there */
    if (argc != Arguments + ArgOffset) {
        usage(argv[0]);
        return(1);
    }

    /* Open all Output and bail out if any fail */
    for (FileNumber = 0; FileNumber < Arguments; FileNumber++) {

        /* Open the File */
        if (FileNumber == 2) OpenType = "wb";
        Files[FileNumber] = fopen(argv[FileNumber + ArgOffset], OpenType);

        /* Check for failure and error out if so */
        if (!Files[FileNumber]) {
            perror(argv[FileNumber + ArgOffset]);
            return (1);
        }
    }

    /* Write the File Headers */
    WriteFileHeader(Files[NtosUserStubs],
                    "System Call Stubs for Native API",
                    argv[NtosUserStubs + ArgOffset]);

    WriteFileHeader(Files[NtosKernelStubs],
                    "System Call Stubs for Native API",
                    argv[NtosKernelStubs + ArgOffset]);
    fputs("#include <ndk/asm.h>\n\n", Files[NtosKernelStubs]);

    WriteFileHeader(Files[Win32kStubs],
                    "System Call Stubs for Native API",
                    argv[Win32kStubs + ArgOffset]);

    /* Create the System Stubs */
    CreateStubs(Files[NativeSystemDb],
                &Files[NtosUserStubs],
                Files[NtosKernelStubs],
                MAIN_INDEX,
                1,
                1);

    /* Create the Graphics Stubs */
    CreateStubs(Files[NativeGuiDb],
                &Files[Win32kStubs],
                NULL,
                WIN32K_INDEX,
                1,
                0);

    /* Create the Service Tables */
    rewind(Files[NativeSystemDb]);
    CreateSystemServiceTable(Files[NativeSystemDb],
                            Files[NtosServiceTable],
                            "Main",
                            argv[NtosServiceTable + ArgOffset]);

    rewind(Files[NativeGuiDb]);
    CreateSystemServiceTable(Files[NativeGuiDb],
                            Files[Win32kServiceTable],
                            "Win32k",
                            argv[Win32kServiceTable + ArgOffset]);

    /* Create the Spec Files */
    rewind(Files[NativeSystemDb]);
    CreateSpec(Files[NativeSystemDb],
               &Files[NtSpec],
               1,
               0);

    rewind(Files[NativeSystemDb]);
    CreateSpec(Files[NativeSystemDb],
               &Files[NtSpec],
               1,
               1);

    /* Close all files */
    for (FileNumber = 0; FileNumber < Arguments-ArgOffset; FileNumber++) {

        /* Close the File */
        fclose(Files[FileNumber]);

    }

    return(0);
}
