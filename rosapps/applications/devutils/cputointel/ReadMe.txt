CpuToIntel is a experment tools and is strict under havy devloping


The Idea
The idea is to converting binary files or win pe files 
from one cpu to another cpu, But it does not exists
plan to port over diffent hardware architect like
how diffent hw comucate, example x86 DMA controller
to PPC like that stuff. It is only to convert the 
binary or pe files to another cpu. it mean a user 
mode apps will always be ported, but if it self 
modify code it will not work. But it exists idea how 
to deal with self modify code. 


The idea to handling self modify code
The idea is to add a small emulator or adding 
anaylysing  process to dectect self modify code 
and extract it. This is very hard part todo, some say
imposible, some other say almost imposble. and I say
it is posible todo but extream hard todo. for it is
very diffcul to dectect self modify code with a
analysing process.


Why the name are CpuToIntel
When I start write on it it was only ment to convert 
from ARM, PPC, m68k to X86 but then I come think of
ReactOS PPC port that is going on. for or later we
will need something that doing convert from x86 to
PPC apps. It exists two way todo it. One is to use 
dymatic translation a jit, like UAE or QEMU doing 
converting. But it will lose of allot of speed if
it is a game or a havy apps to much. So the idea
is to convert the whole file in one sweep. will give
 one other problem it will be a slow process todo it,
and hard dectect self modify program. so not all program
can be really convert with this process. 


Who will it work 
we take it step for step and I will describe the 
binary translations how it works. The PE file
work simluare way. 

step 1 : it will disambler the program frist 

step 2 : translate everthing to a middle asm dialect,
         it is own asm dialect it is not suite for a real

step 3 : (not implement) send it to ananalysing processs 
         to get any name or mark out which row is a new functions

step 3.5 (not implement) split the code into functions here

step 4 : Now it start the convert process. 

step 4.5 (not implement) maybe a optimzer. 
  
step 5 : now it is finish. 


The arch that are plan 
PPC  to IA32, PPC (work in progress)
m68k to IA32, PPC (stubed)
ARM  to IA32, PPC (stubed)
IA32 to IA32, PPC (work in progress)


The Winodws NT PPC and x85 diffrent 
R1  The stack pointer equal with x86 esp
R3  The return reg equal with x86 eax
R4  The return reg equal with x86 edx
R31 The base pointer equal with x86 ebp
     
 