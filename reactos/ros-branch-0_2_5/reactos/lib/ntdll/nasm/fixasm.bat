@echo off
echo fix temp slov for to make ntdll use this asm code insted of the c code
echo 1. copy makefile-asm ../makefile

cd rtl 
del *.o
del *.d
del i386_RtlCompareMemory
del i386_RtlCompareMemoryUlong
del i386_RtlFillMemory
del i386_RtlFillMemoryUlong
del i386_RtlMoveMemory
del i386_RtlRandom
del i386_RtlZeroMemory
cd ..

rem nasmw -f coff i386_RtlCompareMemory.asm
rem nasmw -f coff i386_RtlCompareMemoryUlong.asm
rem nasmw -f coff i386_RtlFillMemory.asm
rem nasmw -f coff i386_RtlFillMemoryUlong.asm
rem nasmw -f coff i386_RtlMoveMemory.asm
rem nasmw -f coff i386_RtlRandom.asm
rem nasmw -f coff i386_RtlZeroMemory.asm



