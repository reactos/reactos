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
#include <malloc.h>

/* DEFINES  ****************************************************************/

#define INPUT_BUFFER_SIZE 255
#define Arguments 8

/******* Table Indexes ************/
#define MAIN_INDEX 0x0
#define WIN32K_INDEX 0x1000

/******* Argument List ************/
/* First, define the Databases */
#define NativeSystemDb 0
#define NativeGuiDb 1

/* Now the Service Tables */
#define NtosServiceTable 2
#define Win32kServiceTable 3

/* And finally, the stub files. */
#define NtosUserStubs 4
#define NtosKernelStubs 5
#define Win32kGdiStubs 6
#define Win32kUserStubs 7

/********** Stub Code ************/

/*
 * This stubs calls into KUSER_SHARED_DATA where either a 
 * sysenter or interrupt is performed, depending on CPU support.
 */    
#if defined(__GNUC__)
#define UserModeStub_x86    "    movl $0x%x, %%eax\n" \
                            "    movl $KUSER_SHARED_SYSCALL, %%ecx\n" \
                            "    call *%%ecx\n" \
                            "    ret $0x%x\n\n"
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
                            "    pushl $KERNEL_CS\n" \
                            "    call _KiSystemService\n" \
                            "    ret $0x%x\n\n"
#elif defined(_MSC_VER)
#define KernelModeStub_x86  "    asm { \n" \
                            "        mov eax, %xh\n" \
                            "        lea edx, [esp+4]\n" \
                            "        pushf\n" \
                            "        push KERNEL_CS\n" \
                            "        call _KiSystemService\n" \
                            "        ret %xh\n" \
                            "    }\n"
#else
#error Unknown compiler for inline assembler
#endif

/***** Arch Dependent Stuff ******/
//#ifdef _M_IX86
#define ARGS_TO_BYTES(x) x*4
#define UserModeStub UserModeStub_x86
#define KernelModeStub KernelModeStub_x86

//#elseif
//#error Unsupported Architecture
//#endif

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
            "#define KUSER_SHARED_SYSCALL 0x7FFE0300\n\n",
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
    fprintf(StubFile, ".global _%s@%d\n", SyscallName, StackBytes);
    
    /* Define it */
    fprintf(StubFile, "_%s@%d:\n\n", SyscallName, StackBytes);
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
 *     Index - Name of System Call for which to add the stub.
 *
 *     UserFiles - Number of bytes on the stack to return after doing the system call.
 *
 *     NeedsZw - Service Descriptor Table ID for this System Call.
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
    char *ZwSyscallName = NULL;
    char *SyscallArguments;
    int SyscallId;
    unsigned StackBytes;
    
    /* We loop, incrementing the System Call Index, until the end of the file  */
    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {
             
        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments); 
        StackBytes = ARGS_TO_BYTES(strtoul(SyscallArguments, NULL, 0));
 
        /* Make sure we really extracted something */
        if (NtSyscallName) {
            
            /* Create the ZwXXX name, if requested */
            if (NeedsZw) {
                ZwSyscallName = alloca(strlen(NtSyscallName));
                strcpy(ZwSyscallName, NtSyscallName);
                ZwSyscallName[0] = 'Z';
                ZwSyscallName[1] = 'w';
            }
     
            /* Create Usermode Stubs for Nt/Zw syscalls in each Usermode file */
            int i;
            for (i= 0; i < UserFiles; i++) {
    
                /* Write the Nt Version */
                WriteUserModeStub(UserModeFiles[i], 
                                  NtSyscallName, 
                                  StackBytes, 
                                  SyscallId | Index);

                /* If a Zw Version is needed (was specified), write it too */
                if (ZwSyscallName) WriteUserModeStub(UserModeFiles[i], 
                                                     ZwSyscallName,
                                                     StackBytes,
                                                     SyscallId | Index);

            }

            /* Create the Kernel coutnerparts (only Zw*, Nt* are the real functions!) */
            if (KernelModeFile) WriteKernelModeStub(KernelModeFile, 
                                                    ZwSyscallName, 
                                                    StackBytes, 
                                                    SyscallId | Index);
        
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
    fprintf(SyscallTable,"SSDT %sSSDT[] = {\n", Name);

    /* We loop, incrementing the System Call Index, until the end of the file */
    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);
        
        /* Make sure we really extracted something */
        if (NtSyscallName) {
            
            /* Add a new line */
            if (SyscallId > 0) fprintf(SyscallTable,",\n");
        
            /* Write the syscall name in the service table. */
            fprintf(SyscallTable,"\t\t(PVOID (NTAPI *)(VOID))%s", NtSyscallName);
            
            /* Only increase if we actually added something */
            SyscallId++;
        }
    }
    
    /* Close the service table (C syntax) */
    fprintf(SyscallTable,"\n};\n");

    /* Now we build the SSPT */
    rewind(SyscallDb);
    fprintf(SyscallTable,"\n\n\n");
    fprintf(SyscallTable,"SSPT %sSSPT[] = {\n", Name);

    for (SyscallId = 0; ((!feof(SyscallDb)) && (fgets(Line, sizeof(Line), SyscallDb) != NULL));) {

        /* Extract the Name and Arguments */
        GetNameAndArgumentsFromDb(Line, &NtSyscallName, &SyscallArguments);
        
        /* Make sure we really extracted something */
        if (NtSyscallName) {
            
            /* Add a new line */
            if (SyscallId > 0) fprintf(SyscallTable,",\n");
            
            /* Write the syscall arguments in the argument table. */
            fprintf(SyscallTable,"\t\t%lu * sizeof(void *)",strtoul(SyscallArguments, NULL, 0));
                    
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

void usage(char * argv0)
{
    printf("Usage: %s sysfuncs.lst w32ksvc.db napi.h ssdt.h napi.S zw.S win32k.S win32k.S\n"
           "  sysfuncs.lst  native system functions database\n"
           "  w32ksvc.db    native graphic functions database\n"
           "  napi.h        NTOSKRNL service table\n"
           "  ssdt.h        WIN32K service table\n"
           "  napi.S        NTDLL stubs\n"
           "  zw.S          NTOSKRNL Zw stubs\n"
           "  win32k.S      GDI32 stubs\n"
           "  win32k.S      USER32 stubs\n",
           argv0
           );
}

int main(int argc, char* argv[])
{
    FILE * Files[Arguments];
    int FileNumber;
    char * OpenType = "r";

    /* Make sure all arguments all there */
    if (argc != Arguments + 1) {
        usage(argv[0]);
        return(1);
    }
  
    /* Open all Output and bail out if any fail */
    for (FileNumber = 0; FileNumber < Arguments; FileNumber++) {
    
        /* Open the File */
        if (FileNumber == 2) OpenType = "wb";
        Files[FileNumber] = fopen(argv[FileNumber + 1], OpenType);
        
        /* Check for failure and error out if so */
        if (!Files[FileNumber]) {
            perror(argv[FileNumber + 1]);
            return (1);
        }
    
    }

    /* Write the File Headers */
    WriteFileHeader(Files[NtosUserStubs], 
                    "System Call Stubs for Native API", 
                    argv[NtosUserStubs + 1]);
    
    WriteFileHeader(Files[NtosKernelStubs], 
                    "System Call Stubs for Native API", 
                    argv[NtosKernelStubs + 1]);
    fputs("#include <ndk/i386/segment.h>\n\n", Files[NtosKernelStubs]);
    
    WriteFileHeader(Files[Win32kGdiStubs], 
                    "System Call Stubs for Native API", 
                    argv[Win32kGdiStubs + 1]);
    
    WriteFileHeader(Files[Win32kUserStubs], 
                    "System Call Stubs for Native API", 
                    argv[Win32kUserStubs + 1]);
    

    /* Create the System Stubs */
    CreateStubs(Files[NativeSystemDb],
                &Files[NtosUserStubs], 
                Files[NtosKernelStubs], 
                MAIN_INDEX, 
                1,
                1);

    /* Create the Graphics Stubs */
    CreateStubs(Files[NativeGuiDb], 
                &Files[Win32kGdiStubs], 
                NULL, 
                WIN32K_INDEX, 
                2,
                0);

    /* Rewind the databases */
    rewind(Files[NativeSystemDb]);
    rewind(Files[NativeGuiDb]);

    /* Create the Service Tables */
    CreateSystemServiceTable(Files[NativeSystemDb], 
                            Files[NtosServiceTable],
                            "Main",
                            argv[NtosServiceTable + 1]);
    
    CreateSystemServiceTable(Files[NativeGuiDb], 
                            Files[Win32kServiceTable],
                            "Win32k",
                            argv[Win32kServiceTable + 1]);

    /* Close all files */
    for (FileNumber = 0; FileNumber < Arguments; FileNumber++) {
    
        /* Close the File */
        fclose(Files[FileNumber]);
        
    }

    return(0);
}
