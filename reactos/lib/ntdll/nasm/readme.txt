
This is Intel i386 or higher asm code version of ntdll.dll
The minium cpu req by reactos are : 486. 

The asm src in this folder need nasm to be compile with.


1. Rename reactos/lib/ntdll/makefile to reactos/lib/ntdll/makefile.c 
   or make a backup up of it. 
   (it contain the C code version of ntdll makefile)


2. Copy reactos/lib/ntdll/nasm/makefile-asm to reactos/lib/ntdll/makefile
   (it contain the asm verison of ntdll makefile, I need to change some 
   part to get the asm version be in use insted of the c version)

3. Now run reactos/make so u get the asm version of ntdll





Hope everyone like this asm optimze version. 
but for momment are only thing optimze are

ntdll/rtl/mem.c
-----------------------------------------
RtlCompareMemory
RtlCompareMemoryUlong
RtlFillMemoryUlong
RtlMoveMemory
RtlZeroMemory

RtlFillMemory          
(can make it lite faster not maxium optimze yet)



ntdll/rtl/RtlRandom.c
-----------------------------------------
RtlRandom
RtlUniform
SavedValue




info about ntdll/nasm/fixasm.bat 
-----------------------------------------
delete all file that are not src or bat file 
in the folder ntdll/nasm/

