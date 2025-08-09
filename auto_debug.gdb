# Automated GDB script
set pagination off
set logging file gdb_trace.log
set logging on
set architecture i386:x86-64
target remote localhost:1234

# Set breakpoints
break *0xFFFFF80000200000

# Run until crash
continue

# When stopped, auto-collect info
echo \n=== CRASH POINT ===\n
info registers rip rsp cr2
x/10i $rip-20
bt
x/20xg $rsp

# Quit
quit