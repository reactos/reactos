/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Print a stacktrace
 * COPYRIGHT:   Copyright 2017,2018 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"
#include <dbghelp.h>

#define STACK_MAX_RECURSION_DEPTH   50


void BeginStackBacktrace(DumpData& data)
{
    DWORD symOptions = SymGetOptions();
    symOptions |= SYMOPT_UNDNAME | SYMOPT_AUTO_PUBLICS | SYMOPT_DEFERRED_LOADS;
    SymSetOptions(symOptions);
    SymInitialize(data.ProcessHandle, NULL, TRUE);
}

void EndStackBacktrace(DumpData& data)
{
    SymCleanup(data.ProcessHandle);
}

static char ToChar(UCHAR data)
{
    if (data < 0xa)
        return '0' + data;
    else if (data <= 0xf)
        return 'a' + data - 0xa;
    return '?';
}

void PrintStackBacktrace(FILE* output, DumpData& data, ThreadData& thread)
{
    DWORD MachineType;
    STACKFRAME64 StackFrame = { { 0 } };

    StackFrame.AddrPC.Mode = AddrModeFlat;
    StackFrame.AddrReturn.Mode = AddrModeFlat;
    StackFrame.AddrFrame.Mode = AddrModeFlat;
    StackFrame.AddrStack.Mode = AddrModeFlat;
    StackFrame.AddrBStore.Mode = AddrModeFlat;


#if defined(_M_IX86)
    MachineType = IMAGE_FILE_MACHINE_I386;
    StackFrame.AddrPC.Offset = thread.Context.Eip;
    StackFrame.AddrStack.Offset = thread.Context.Esp;
    StackFrame.AddrFrame.Offset = thread.Context.Ebp;
#elif defined(_M_AMD64)
    MachineType = IMAGE_FILE_MACHINE_AMD64;
    StackFrame.AddrPC.Offset = thread.Context.Rip;
    StackFrame.AddrStack.Offset = thread.Context.Rsp;
    StackFrame.AddrFrame.Offset = thread.Context.Rbp;
#elif defined(_M_ARM)
    MachineType = IMAGE_FILE_MACHINE_ARMNT;
    StackFrame.AddrPC.Offset = thread.Context.Pc;
    StackFrame.AddrStack.Offset = thread.Context.Sp;
    StackFrame.AddrFrame.Offset = thread.Context.R11;
#elif defined(_M_ARM64)
    MachineType = IMAGE_FILE_MACHINE_ARM64;
    StackFrame.AddrPC.Offset = thread.Context.Pc;
    StackFrame.AddrStack.Offset = thread.Context.Sp;
    StackFrame.AddrFrame.Offset = thread.Context.u.s.Fp;
#else
#error "Unknown architecture"
#endif

#define STACKWALK_MAX_NAMELEN   512
    char buf[sizeof(SYMBOL_INFO) + STACKWALK_MAX_NAMELEN] = {0};
    SYMBOL_INFO* sym = (SYMBOL_INFO *)buf;
    IMAGEHLP_MODULE64 Module = { 0 };
    LONG RecursionDepth = 0;
    sym->SizeOfStruct = sizeof(sym);

    /* FIXME: dump x bytes at PC here + disasm it! */

    xfprintf(output, NEWLINE "*----> Stack Back Trace <----*" NEWLINE NEWLINE);
    bool first = true;
    while (StackWalk64(MachineType, data.ProcessHandle, thread.Handle, &StackFrame, &thread.Context,
                       NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
    {
        if (StackFrame.AddrPC.Offset == StackFrame.AddrReturn.Offset)
        {
            if (RecursionDepth++ > STACK_MAX_RECURSION_DEPTH)
            {
                xfprintf(output, "- Aborting stackwalk -" NEWLINE);
                break;
            }
        }
        else
        {
            RecursionDepth = 0;
        }

        if (first)
        {
            xfprintf(output, "FramePtr ReturnAd Param#1  Param#2  Param#3  Param#4  Function Name" NEWLINE);
            first = false;
        }

        Module.SizeOfStruct = sizeof(Module);
        DWORD64 ModBase = SymGetModuleBase64(data.ProcessHandle, StackFrame.AddrPC.Offset);
        if (!ModBase || !SymGetModuleInfo64(data.ProcessHandle, ModBase, &Module))
            strcpy(Module.ModuleName, "<nomod>");

        memset(sym, '\0', sizeof(*sym) + STACKWALK_MAX_NAMELEN);
        sym->SizeOfStruct = sizeof(*sym);
        sym->MaxNameLen = STACKWALK_MAX_NAMELEN;
        DWORD64 displacement = 0;

        if (!StackFrame.AddrPC.Offset || !SymFromAddr(data.ProcessHandle, StackFrame.AddrPC.Offset, &displacement, sym))
            strcpy(sym->Name, "<nosymbols>");

        xfprintf(output, "%p %p %p %p %p %p %s!%s +0x%I64x" NEWLINE,
                 (ULONG_PTR)StackFrame.AddrFrame.Offset, (ULONG_PTR)StackFrame.AddrPC.Offset,
                 (ULONG_PTR)StackFrame.Params[0], (ULONG_PTR)StackFrame.Params[1],
                 (ULONG_PTR)StackFrame.Params[2], (ULONG_PTR)StackFrame.Params[3],
                 Module.ModuleName, sym->Name, displacement);
    }

    UCHAR stackData[0x10 * 10];
    SIZE_T sizeRead;
#if defined(_M_IX86)
    ULONG_PTR stackPointer = thread.Context.Esp;
#elif defined(_M_AMD64)
    ULONG_PTR stackPointer = thread.Context.Rsp;
#elif defined(_M_ARM) || defined(_M_ARM64)
    ULONG_PTR stackPointer = thread.Context.Sp;
#else
#error Unknown architecture
#endif
    if (!ReadProcessMemory(data.ProcessHandle, (PVOID)stackPointer, stackData, sizeof(stackData), &sizeRead))
        return;

    xfprintf(output, NEWLINE "*----> Raw Stack Dump <----*" NEWLINE NEWLINE);
    for (size_t n = 0; n < sizeof(stackData); n += 0x10)
    {
        char HexData1[] = "?? ?? ?? ?? ?? ?? ?? ??";
        char HexData2[] = "?? ?? ?? ?? ?? ?? ?? ??";
        char AsciiData1[] = "????????";
        char AsciiData2[] = "????????";

        for (size_t j = 0; j < 8; ++j)
        {
            size_t idx = j + n;
            if (idx < sizeRead)
            {
                HexData1[j * 3] = ToChar(stackData[idx] >> 4);
                HexData1[j * 3 + 1] = ToChar(stackData[idx] & 0xf);
                AsciiData1[j] = isprint(stackData[idx]) ? stackData[idx] : '.';
            }
            idx += 8;
            if (idx < sizeRead)
            {
                HexData2[j * 3] = ToChar(stackData[idx] >> 4);
                HexData2[j * 3 + 1] = ToChar(stackData[idx] & 0xf);
                AsciiData2[j] = isprint(stackData[idx]) ? stackData[idx] : '.';
            }
        }

        xfprintf(output, "%p %s - %s  %s%s" NEWLINE, stackPointer+n, HexData1, HexData2, AsciiData1, AsciiData2);
    }
}
