function write(x) { System.print(x); }'
function regs(n) { return System.regs(n); }
function ebp() { return regs(0); }
function eip() { return regs(1); }
function tf_argmark() { return regs(2); }
function tf_pointer() { return regs(3); }
function tf_tempcs() { return regs(4); } 
function tf_tempeip() { return regs(5); } 
function dr0() { return regs(6); } 
function dr1() { return regs(7); } 
function dr2() { return regs(8); } 
function dr3() { return regs(9); } 
function dr6() { return regs(10); } 
function dr7() { return regs(11); } 
function gs()  { return regs(12) & 0xffff; }
function es()  { return regs(13) & 0xffff; }
function ds()  { return regs(14) & 0xffff; }
function edx() { return regs(15); }
function ecx() { return regs(16); }
function eax() { return regs(17); }
function tf_pmode() { return regs(18); }
function tf_exl() { return regs(19); }
function fs()  { return regs(20) & 0xffff; }
function edi() { return regs(21); }
function cs()  { return regs(22) & 0xffff; }
function eflags() { return regs(23); }
function esp() { return regs(24); }
function ss()  { return regs(25) & 0xffff; }
function v86_es() { return regs(26) & 0xffff; }
function v86_ds() { return regs(27) & 0xffff; }
function v86_fs() { return regs(28) & 0xffff; }
function v86_gs() { return regs(29) & 0xffff; }
function peekl(a) { return System.mread(4,a); }
function pokel(a,b) { return System.mwrite(4,a,b); }
function peekw(a) { return System.mread(2,a); }
function pokew(a,b) { return System.mwrite(2,a,b); }
function peek(a) { return System.mread(1,a); }
function poke(a,b) { return System.mwrite(1,a,b); }
function regread(x,y) { return System.regread(x,y); }
function findmodule(str) { return System.findmodule(str); }
write('JS Registry Init Complete.  Welcome to ReactOS kernel scripting');